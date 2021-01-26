#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
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

enum punct {
	PU_NONE,
	PU_DOT,
	PU_2DOT,
	PU_3DOT,
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


static inline Status add_open_spans(Doc_area_list * list, struct span_stack * open_spans,
		struct span_stack * new_open_spans, const struct buffer * buf, const char * itr) {
	ASSERT_SPAN_STACK(open_spans);
	ASSERT_SPAN_STACK(new_open_spans);

	Status stat;
	for (size_t i = 0; i < new_open_spans->len; i++) {
		switch (new_open_spans->vals[i]) {
		case SP_UNDER:
			stat = doc_area_list_u_begin(list);
			break;
		default:
			ERR("Unknown span: %d", new_open_spans->vals[i]);
			exit(1);
		}
		if (stat == FAILURE
				|| span_stack_push(open_spans, new_open_spans->vals[i], buf, itr) == FAILURE)
			return FAILURE;
	}
	span_stack_clear(new_open_spans);
	return SUCCESS;
}

static inline Status add_punct(Doc_area_list * list, enum punct punct,
		const struct buffer * buf, const char * itr) {
	assert(list);
	assert(buf);
	assert(itr);

	Status stat;
	switch (punct) {
	case PU_DOT:
		stat = doc_area_list_sentence_period(list);
		break;
	case PU_2DOT:
		SYNERR(buf, itr, "Double period");
		stat = FAILURE;
		break;
	case PU_3DOT:
		stat = doc_area_list_ellipsis(list);
		break;
	default:
		ERR("Unknown punctuation: %d", punct);
		exit(1);
	}
	return stat;
}

static inline Status add_close_span(Doc_area_list * list, enum span span) {
	Status stat;
	switch (span) {
	case SP_UNDER:
		stat = doc_area_list_u_end(list);
		break;
	default:
		ERR("Unknown span: %d", span);
		exit(EXIT_FAILURE);
	}
	return stat;
}

#define NOIDX SIZE_MAX
	
static inline Status add_close_spans(Doc_area_list * list, struct span_stack * open_spans,
		struct span_stack * close_spans, enum punct punct, size_t punct_span_idx,
		const struct buffer * buf, const char * itr) {
	ASSERT_SPAN_STACK(open_spans);
	ASSERT_SPAN_STACK(close_spans);
	assert(list);

	size_t i = 0;
	if (punct != PU_NONE) {
		assert(punct_span_idx != NOIDX);
		assert(punct_span_idx <= close_spans->len);

		for (; i < punct_span_idx; i++) {

			// spans should match
			assert(close_spans->vals[i] == open_spans->vals[open_spans->len - 1 - i]);

			if (add_close_span(list, close_spans->vals[i]) == FAILURE)
				return FAILURE;
		}

		if (add_punct(list, punct, buf, itr) == FAILURE)
			return FAILURE;
	}
	for (; i < close_spans->len; i++) {
		assert(close_spans->vals[i] == open_spans->vals[open_spans->len - 1 - i]);

		if (add_close_span(list, close_spans->vals[i]) == FAILURE)
			return FAILURE;
	}
		
	open_spans->len -= close_spans->len;
	span_stack_clear(close_spans);
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

enum ctx {
	WORD_CTX,
	PARAGRAPH_CTX,
	COMMENT_CTX,
};

static inline Status parse(
		struct buffer * buf,
		struct span_stack * open_spans,
		struct span_stack * new_open_spans,
		struct span_stack * close_spans,
		struct tok_stream * toks,
		struct text_stream * texts
		) {

	char * itr = buf->line;
	char * wordstart = itr;
	char * wordend = NULL;
	enum ctx ctx = WORD;
	enum punct punct = PU_NONE;
	size_t punct_span_idx = NOIDX;
	int prevc = 0;
	int c;

	// build paragraph until double newline, eof, or error
	while (1) {

		c = *itr++;

		// TODO: get rid of this
		// symbols found that dont have special meaning
		if (ctx == PRE_WORD && !ISSPACE(prevc) && prevc != c) {
			ctx = MID_WORD;
			wordend = NULL;
			punct = PU_NONE;
			prevc = IGNOREC;
			span_stack_clear(&close_spans);
			if (add_open_spans(p, &open_spans, &new_open_spans, buf, itr) == FAILURE)
				return FAILURE;
		}

		// nul
		if (!c) { 
			PTEST("NULL");
			char * old_line = buf->line; 
			READMORE_EOF_ERR(buf, break, return FAILURE)
			size_t shift = buf->line - old_line;
			itr += shift - 1;
			wordstart += shift;
			if (wordend)
				wordend += shift;
			continue;
		} 

		// space
		if (c <= ' ') {
			if (ctx == WORD_CTX) {
				if (!wordend)
					wordend = itr - 1;
				if (add_word(wordstart, itr - 1, toks, texts) == FAILURE)
					return FAILURE;
				ctx = PARAGRAPH_CTX;
			}
			if (c == '\n') {
				PTEST("NEWLINE");
				update_line(buf, itr);
				if (ctx == COMMENT_CTX) {
					ctx = PARAGRAPH_CTX;
					prevc = '\n';
				}
				else if (prevc == '\n') {
					tok_stream_push(toks, END_PARAGRAPH_TOK);
					prevc = IGNOREC;
					ctx = INITIAL_CTX;
				}
				else
					prevc = '\n';
			}
			else {
				PTEST("SPACE");
				prevc = IGNOREC;
			}
			wordstart = itr;
			continue;
		}

		// skip comment
		if (ctx == COMMENT_CTX)
			continue;

		// pound
		if (c == '#') {
			PTEST("POUND");
			// second pound = line comment
			if (prevc == '#') {
				ctx = COMMENT_CTX;
				prevc = IGNOREC;
			}
			else
				prevc = '#';
			continue;
		}

		// under
		if (c == '_') {
			PTEST("UNDER");
			if (prevc == '_') {
				if (ctx == WORD_CTX) {
					if (span_stack_pop(spans) != SP_UNDER)
						SYNERR(buf, itr - 2, "Unmatched underline terminator");
						return FAILURE;
					}
					if (add_word(wordstart, itr - 2, toks, texts) == FAILURE
							|| tok_stream_push(&toks, END_UNDERLINE_TOK) == FAILURE)
						return FAILURE;
					ctx = PARAGRAPH_CTX;
				}
				else {
					wordstart += 2;
					if (span_stack_push(&spans, SP_UNDER) == FAILURE
							|| tok_stream_push(&toks, START_UNDERLINE_TOK) == FAILURE)
						return FAILURE;
				}
				prevc = IGNOREC;
			}
			else
				prevc = '_';
			continue;
		}

		// else word
		PTEST("WORD: %s", (itr - 1));
		ctx = WORD_CTX;
		prevc = WORDC;

	} // end of pg

	if (!c)
		itr--;

	update_line(buf, itr);

	if (parsing_word || prevc > ' ') {
		if (!wordend)
			wordend = itr;
		if (doc_area_list_add_word(p, wordstart, itr - wordstart) == FAILURE
				|| add_close_spans(p, &open_spans, &close_spans,
						punct, punct_span_idx, buf, itr - 1) == FAILURE)
			return FAILURE;
	}

	if (open_spans.len != 0) {
		// TODO: more specific message
		SYNERR(buf, itr, "Unterminated markers");
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

inline Status buf_init(struct buffer * buf, FILE * in) {

	// our custom input buffer
	buf->in = in,
	buf->start = malloc(DEF_BUF_SIZE),
	buf->cap = DEF_BUF_SIZE,
	buf->line = buf.start,
	buf->lineno = 1

	if (!buf->start) {
		LIBERRN();
		return FAILURE;
	}

	return SUCCESS;
}

inline Status buf_clean(struct buffer * buf) {
	free(buf->start);
	ZERO(buf, sizeof(struct buffer));
}


Status compile(FILE * in, Doc * doc) {
	
	Status ret;

	struct buffer buf = {};
	struct span_stack open_spans = {};
	struct span_stack new_open_spans = {};
	struct span_stack close_spans = {};
	struct tok_stream toks = {};
	struct text_stream texts = {};
	if (buf_init(&buf, in) == FAILURE
			|| span_stack_init(&open_spans) == FAILURE
			|| span_stack_init(&new_open_spans) == FAILURE
			|| span_stack_init(&close_spans) == FAILURE
			|| tok_stream_init(&toks) == FAILURE
			|| text_stream_init(&texts) == FAILURE
				// attempt to read the first bytes
			|| try_read(&buf, 0, DEF_BUF_SIZE - 1) == FAILURE
			|| parse(&buf, &open_spans, &new_open_spans, &close_spans, &toks, &texts) == FAILURE
		ret = FAILURE;
	else
		ret = SUCCESS;

	text_stream_clean(&texts);
	tok_stream_clean(&toks);
	span_stack_clean(&close_spans);
	span_stack_clean(&new_open_spans);
	span_stack_clean(&open_spans);
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
