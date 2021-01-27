#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "stream.h"
#include "doc.h"


struct buffer {
	FILE * in;
	char * start;
	size_t cap;
	char * line;
	size_t lineno;
};

void showctx(const struct buffer * buf, const char * pos) {
	assert(buf);
	assert(pos);
	assert(buf->line);
	assert(pos >= buf->line); // TODO: = ok here?
	#define MAX_CTX 64
	#define MIN_POST 3
	#define ELLIPSIS "..."
	#define ARROW "^"
	size_t posidx = pos - buf->line;
	// test probably not needed
	size_t minend = posidx > SIZE_MAX - 3? SIZE_MAX: posidx + 3;
	if (minend >= MAX_CTX) {
		char * start = buf->line + minend - MAX_CTX + 1;
		fprintf(stderr, ELLIPSIS "%.*s\n", MAX_CTX, start);
		fprintf(stderr, "%*s\n", (int)(pos - start + 1), ARROW);
	}
	else {
		size_t linelen = strcspn(buf->line, "\n");
		if (linelen > MAX_CTX)
			fprintf(stderr, "%.*s" ELLIPSIS "\n", MAX_CTX, buf->line); 
		else
			fprintf(stderr, "%.*s\n", (int) linelen, buf->line);
		fprintf(stderr, "%*s\n", (int) posidx + 1, ARROW);
	}
}

/*
	Attempts to read from input file
	@buf buffer to read to
	@idx the index from which to start copying data
	@amt amount of data to read, discluding null byte
	@in the input file
*/
Status try_read(struct buffer * buf, size_t idx, size_t amt) {
	assert(buf);
	assert(buf->in);
	assert(idx < buf->cap);
	assert(idx + amt < buf->cap);
	assert(!ferror(buf->in));
	assert(!feof(buf->in));

	char * readstart = buf->start + idx;
	size_t amtread = fread(readstart, sizeof(char), amt, buf->in);
	readstart[amtread] = '\0';
	if (amtread != amt && ferror(buf->in)) {
		ERR("Failed to read from input file");
		return FAILURE;
	}
	return SUCCESS;
}

/*
	Reads more data from input file when hit end of buffer
	@buf - buffer information to update
	@tok - pointer to current token being parsed
	SIDE EFFECTS: external pointers to buffer will be invalidated
*/
//	TODO: review and debug this, then test
Status read_more(struct buffer * buf) {
	assert(buf);
	assert(buf->in);
	assert(!ferror(buf->in));
	assert(!feof(buf->in));
	assert(buf->line);
	assert(buf->line >= buf->start);
	assert(buf->start + buf->cap > buf->line);
	assert(buf->cap > 0);
	assert(buf->start[buf->cap - 1] == '\0');

	size_t amtparsed = buf->line - buf->start;
	if (amtparsed == 0) {
		if (buf->cap > SIZE_MAX/2) {
			SYNERR(buf, buf->line, "This line is too big");
			return FAILURE;
		}
		size_t oldcap = buf->cap;
		size_t newcap = oldcap << 1;
		char * newstart;
		REALLOC(buf->start, newstart, oldcap, oldcap, newcap);
		if (!newstart) {
			LIBERRN();
			return FAILURE;
		}
		buf->start = newstart;
		buf->line = newstart;
		buf->cap = newcap;
		int ret = try_read(buf, oldcap - 1, oldcap);
		buf->cap = newcap;
		return ret;
	}
	size_t remaining = buf->cap - amtparsed - 1; // discludes null
	memmove(buf->start, buf->line, remaining);
	buf->line = buf->start;
	// all the bytes that have been parsed are now free space
	return try_read(buf, remaining, amtparsed);
}

static inline void update_line(struct buffer * buf, char * itr) {
	buf->line = itr;
	buf->lineno++;
}

enum span {
	SP_NONE,
	SP_UNDER,
};

struct span_stack {
	enum span * vals;
	size_t len;
	size_t cap;
};

#define ASSERT_SPAN_STACK(stack) do { \
	assert(stack->len <= stack->cap); \
	assert(stack->vals); \
} while (0)

#ifndef NDEBUG
#define DEF_SPAN_STACK_CAP 1
#else
#define DEF_SPAN_STACK_CAP 8
#endif

Status span_stack_init(struct span_stack * stack) {
	_Static_assert(DEF_SPAN_STACK_CAP <= SIZE_MAX / sizeof(enum span), "default span stack cap too big");

	enum span * vals = malloc(DEF_SPAN_STACK_CAP);
	if (!vals) {
		LIBERRN();
		return FAILURE;
	}
	stack->vals = vals;
	stack->len = 0;
	stack->cap = DEF_SPAN_STACK_CAP;
	return SUCCESS;
}

void span_stack_clean(struct span_stack * stack) {
	ASSERT_SPAN_STACK(stack);

	free(stack->vals);

	// cause error if dangling
	stack->vals = NULL;
	stack->len = 1;
}

Status span_stack_grow(struct span_stack * stack, const struct buffer * buf, const char * itr) {
	ASSERT_SPAN_STACK(stack);

	if (stack->cap > SIZE_MAX / sizeof(enum span) / 2) {
		SYNERR(buf, itr, "Too many nested spans");
		return FAILURE;
	}
	stack->cap <<= 1;
	enum span * vals = realloc(stack->vals, stack->cap * sizeof(enum span));
	if (!vals) {
		LIBERRN();
		return FAILURE;
	}
	stack->vals = vals;
	return SUCCESS;
}

static inline Status span_stack_push(struct span_stack * stack, enum span span,
		const struct buffer * buf, const char * itr) {
	ASSERT_SPAN_STACK(stack);

	if (stack->len == stack->cap
			&& span_stack_grow(stack, buf, itr) == FAILURE)
		return FAILURE;
	stack->vals[stack->len++] = span;
	return SUCCESS;
}

static inline void span_stack_clear(struct span_stack * stack) {
	ASSERT_SPAN_STACK(stack);

	stack->len = 0;
}

static inline Status span_stack_pop(struct span_stack * stack) {
	ASSERT_SPAN_STACK(stack);

	return stack->len == 0? SP_NONE: stack->vals[--stack->len];
}

static Status add_chars(const char * start, const char * end,
		struct tok_stream * toks, struct text_stream * texts) {
	assert(start);
	assert(end);
	assert(toks);
	assert(texts);
	assert(end > start);

	if (tok_stream_push(toks, END_WORD_TOK) == FAILURE)
		return FAILURE;

	size_t size = end - start;
	char * text = malloc(size);
	if (!text) {
		LIBERRN();
		return FAILURE;
	}
	memcpy(text, start, size);
	if (text_stream_push(texts, text) == FAILURE) {
		free(text);
		return FAILURE;
	}

	return SUCCESS;
}

#define READMORE_EOF_ERR(buf, do_eof, do_err) \
	if (feof((buf)->in)) \
		do_eof; \
	else if (read_more(buf) == FAILURE) \
		do_err; 

#define WORDC 0x80
#define IGNOREC 0
#define ISSPACE(c) ((c) <= ' ')
#define NOIDX SIZE_MAX

enum ctx {
	INITIAL_CTX,
	MAYBE_WORD_CTX,
	PRE_WORD_CTX,
	WORD_CTX,
	POST_WORD_CTX,
	PARAGRAPH_CTX,
	COMMENT_CTX,
};

struct parser {
	struct buffer * const buf;
	struct span_stack * const spans;
	struct tok_stream * const toks;
	struct text_stream * const texts;
	char * itr;
	char * wordstart;
	enum ctx ctx;
	int prevc;
	int c;
};

static Status parse_space(struct parser * p) {

	// end of word
	if (p->ctx == WORD_CTX || p->ctx == MAYBE_WORD_CTX) {
		if (add_chars(p->wordstart, p->itr - 1, p->toks, p->texts) == FAILURE
				|| tok_stream_push(p->toks, END_WORD_TOK))
			return FAILURE;
		p->ctx = PARAGRAPH_CTX;
	}
	else if (p->ctx == PRE_WORD_CTX) {
		SYNERR(p->buf, p->itr - 1, "Expected start of word");
		return FAILURE;
	}
	else if (p->ctx == POST_WORD_CTX)
		p->ctx = PARAGRAPH_CTX;

	// end of line
	if (p->c == '\n') {
		PTEST("NEWLINE");
		update_line(p->buf, p->itr);

		// end of comment
		if (p->ctx == COMMENT_CTX) {
			p->ctx = PARAGRAPH_CTX;
			p->prevc = '\n';
		}

		// end of paragraph
		else if (p->prevc == '\n') {
			tok_stream_push(p->toks, END_PARAGRAPH_TOK);
			p->ctx = INITIAL_CTX;
			p->prevc = IGNOREC;
		}

		else
			p->prevc = '\n';
	}

	// space
	else {
		PTEST("SPACE");
		p->prevc = IGNOREC;
	}

	p->wordstart = p->itr;
	return SUCCESS;
}

static inline Status parse(
		struct buffer * buf,
		struct span_stack * spans,
		struct tok_stream * toks,
		struct text_stream * texts
		) {

	struct parser p = {
		.buf = buf,
		.spans = spans,
		.toks = toks,
		.texts = texts,
		.itr = buf->line,
		.wordstart = buf->line,
		.ctx = WORD_CTX,
		.prevc = IGNOREC,
	};

	// build paragraph until double newline, eof, or error
	while (1) {

		p.c = *p.itr++;

		// nul
		if (!p.c) { 
			PTEST("NULL");
			char * old_line = p.buf->line; 
			READMORE_EOF_ERR(p.buf, break, return FAILURE)
			size_t shift = p.buf->line - old_line;
			p.itr += shift - 1;
			p.wordstart += shift;
			continue;
		} 

		// white space
		if (ISSPACE(p.c)) {

			if (parse_space(&p) == FAILURE)
				return FAILURE;

			continue;
		}

		// skip comment
		if (p.ctx == COMMENT_CTX)
			continue;

		// pound
		if (p.c == '#' && p.ctx != WORD_CTX && p.ctx != POST_WORD_CTX) {
			PTEST("POUND");
			// second pound = line comment
			if (p.prevc == '#') {
				p.ctx = COMMENT_CTX;
				p.prevc = IGNOREC;
			}
			else if (p.ctx == MAYBE_WORD_CTX) {
				p.ctx = WORD_CTX;
				p.prevc = WORDC;
			}
			else {
				p.ctx = MAYBE_WORD_CTX;
				p.prevc = '#';
			}
			continue;
		}

		// under
		if (p.c == '_') {
			PTEST("UNDER");

			// double under
			if (p.prevc == '_') {

				// end underline
				if (p.ctx == WORD_CTX || p.ctx == POST_WORD_CTX) {
					if (p.ctx == WORD_CTX && add_chars(p.wordstart, p.itr - 1, p.toks, p.texts) == FAILURE)
						return FAILURE;
					if (span_stack_pop(p.spans) != SP_UNDER) {
						SYNERR(p.buf, p.itr - 2, "Unmatched underline terminator");
						return FAILURE;
					}
					if (tok_stream_push(p.toks, END_UNDERLINE_TOK) == FAILURE)
						return FAILURE;
					p.ctx = POST_WORD_CTX;
				}

				// start underline
				else {
					p.wordstart += 2;
					if (span_stack_push(p.spans, SP_UNDER, p.buf, p.itr) == FAILURE
							|| tok_stream_push(p.toks, START_UNDERLINE_TOK) == FAILURE)
						return FAILURE;
					p.ctx = PRE_WORD_CTX;
				}
				p.prevc = IGNOREC;
			}

			else {
				if (p.ctx == MAYBE_WORD_CTX)
					p.ctx = WORD_CTX;
				else if (p.ctx == POST_WORD_CTX) {
					if (p.prevc != IGNOREC) {
						SYNERR(p.buf, p.itr - 2, "Expected end of word here");
						return FAILURE;
					}
				}
				else if (p.ctx != WORD_CTX)
					p.ctx = MAYBE_WORD_CTX;
				p.prevc = '_';
			}
			continue;
		}

		// else word

		if (p.ctx == POST_WORD_CTX) {
			SYNERR(p.buf, p.itr, "Unexpected character at end of word");
			return FAILURE;
		}

		PTEST("WORD: %s", (p.itr - 1));
		p.ctx = WORD_CTX;
		p.prevc = WORDC;

	} // end of file

	p.itr--;

	update_line(p.buf, p.itr);

	assert(!p.c);
	if (parse_space(&p) == FAILURE)
		return FAILURE;

	if (p.spans->len != 0) {
		// TODO: more specific message
		SYNERR(p.buf, p.itr, "Unterminated markers at end of paragraph");
		return FAILURE;
	}

	return SUCCESS;
}

#ifndef NDEBUG
// smaller size means detect problems faster
#define DEF_BUF_SIZE 4
#else
#define DEF_BUF_SIZE 32
#endif

static inline Status buf_init(struct buffer * buf, FILE * in) {

	// our custom input buffer
	buf->in = in;
	buf->start = malloc(DEF_BUF_SIZE);
	buf->cap = DEF_BUF_SIZE;
	buf->line = buf->start;
	buf->lineno = 1;

	if (!buf->start) {
		LIBERRN();
		return FAILURE;
	}

	return SUCCESS;
}

static inline Status buf_clean(struct buffer * buf) {
	free(buf->start);
	ZERO(buf, sizeof(struct buffer));
	return SUCCESS;
}


static inline Status compile(FILE * in, Doc * doc) {
	
	Status ret;

	struct buffer buf = {};
	struct span_stack spans = {};
	struct tok_stream toks = {};
	struct text_stream texts = {};
	if (buf_init(&buf, in) == FAILURE
			|| span_stack_init(&spans) == FAILURE
			|| tok_stream_init(&toks) == FAILURE
			|| text_stream_init(&texts) == FAILURE
				// attempt to read the first bytes
			|| try_read(&buf, 0, DEF_BUF_SIZE - 1) == FAILURE
			|| parse(&buf, &spans, &toks, &texts) == FAILURE)
		ret = FAILURE;
	else
		ret = SUCCESS;

	text_stream_clean(&texts);
	tok_stream_clean(&toks);
	span_stack_clean(&spans);
	buf_clean(&buf);
	return ret;
}

int main(int argc, char * argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE * in = fopen(argv[1], "r"); // input file
	char * docname = NULL; // output file name
	int ret = EXIT_FAILURE; // exit status

	// check if opened succesfully
	if (!in) {
		LIBERR("Failed to open %s", argv[1]);
		goto err;
	}
	// remove input buffering
	if (setvbuf(in, NULL, _IONBF, 0) == -1) {
		LIBERR("setvbuf");
		goto err;
	}
	{ // calculate output file name
		char * suffix = strrchr(argv[1], '.');
		size_t len_nosuffix = suffix? suffix - argv[1]: strlen(argv[1]);
		docname = malloc(len_nosuffix + strlen(doc_suffix));
		if (!docname) {
			LIBERR("Failed to parse file name.");
			goto err;
		}
		memcpy(docname, argv[1], len_nosuffix);
		memcpy(docname + len_nosuffix, doc_suffix, strlen(doc_suffix));
	}
	// ensure input and output files are different
	if (strcmp(argv[1], docname) == 0) {
		ERR("Input and output file must be different.");
		goto err;
	}
	// create output document
	Doc * doc;
	if (!(doc = doc_open(docname)))
		goto err;
	// compile input and write to document
	Status compile_status = compile(in, doc);
	// close document
	if (doc_close(doc) == FAILURE || compile_status == FAILURE)
		goto err;

	ret = EXIT_SUCCESS;
err:
	free(docname);
	if (fclose(in) == EOF) {
		LIBERR("Failed to close input file");
		return EXIT_FAILURE;
	}
	return ret;
}
