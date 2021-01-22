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
int try_read(struct buffer * buf, size_t idx, size_t amt) {
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
int read_more(struct buffer * buf) {
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
	SP_PUNCT,
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
}

static inline void span_stack_clear(struct span_stack * stack) {
	ASSERT_SPAN_STACK(stack);

	stack->len = 0;
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
	if (span_stack_push(open_spans, SP_PUNCT, buf, itr) == FAILURE)
		return FAILURE;
}

static inline Status add_punct(Doc_area_list * list, enum punct punct,
		const struct buffer * buf, const char * itr) {
	assert(list);

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
	
static inline Status add_close_spans(Doc_area_list * list, struct span_stack * open_spans,
		struct span_stack * close_spans, enum punct punct,
		const struct buffer * buf, const char * itr) {
	ASSERT_SPAN_STACK(open_spans);
	ASSERT_SPAN_STACK(close_spans);
	assert(list);
	assert(close_spans->len <= open_spans->len);

	Status stat;
	for (size_t i = 0; i < close_spans->len; i++) {
		// spans should match
		assert(close_spans->vals[i] == open_spans->vals[open_spans->len - 1 - i]);
		switch (close_spans->vals[i]) {
		case SP_UNDER:
			stat = doc_area_list_u_end(list);
			break;
		case SP_PUNCT:
			stat = add_punct(list, punct, buf, itr); // TODO <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
			break;
		default:
			ERR("Unknown span: %d", close_spans->vals[i]);
			exit(1);
		}
		if (stat == FAILURE)
			return FAILURE;
	}
	open_spans->len -= close_spans->len;
	span_stack_clear(close_spans);
}

#define READMORE_EOF_ERR(buf, do_eof, do_err) \
	if (feof((buf)->in)) \
		do_eof; \
	else if (read_more(buf) == FAILURE) \
		do_err; 

#define WORDC 0x80
#define IGNOREC 0

Status compile_pg(struct buffer * buf, Doc * doc) {
	Doc_area_list * p = doc_area_list_new();
	if (!p)
		return FAILURE;
	struct span_stack open_spans = {};
	struct span_stack new_open_spans = {};
	struct span_stack close_spans = {};
	if (span_stack_init(&open_spans) == FAILURE
			|| span_stack_init(&new_open_spans) == FAILURE
			|| span_stack_init(&close_spans) == FAILURE)
		goto err;
	char * itr = buf->line;
	char * wordstart = itr;
	char * wordend = NULL;
	bool parsing_comment = false;
	bool parsing_word = false;
	enum punct punct = PU_NONE;
	int prevc = 0;
	int c;

	// build paragraph until double newline, eof, or error
	while (1) {

		c = *itr++;

		// symbols found that dont have special meaning
		if (!parsing_word && prevc > ' ' && prevc != c) {
			parsing_word = true;
			wordend = NULL;
			punct = PU_NONE;
			prevc = IGNOREC;
			span_stack_clear(&close_spans);
			if (add_open_spans(p, &open_spans, &new_open_spans, buf, itr) == FAILURE)
				goto err;
		}

		// nul
		if (!c) { 
			PTEST("NULL");
			char * old_line = buf->line; 
			READMORE_EOF_ERR(buf, break, goto err)
			size_t shift = buf->line - old_line;
			itr += shift - 1;
			wordstart += shift;
			continue;
		} 

		// space
		if (c <= ' ') {
			if (parsing_word) {
				if (!wordend)
					wordend = itr - 1;
				if (doc_area_list_add_word(p, wordstart, wordend - wordstart) == FAILURE
						|| add_close_spans(p, &open_spans, &close_spans, punct, buf, itr - 1) == FAILURE)
					goto err;
				wordend = NULL;
				parsing_word = false;
			}
			else {
				// ignore markers surrounded by space
				span_stack_clear(&new_open_spans);
			}
			if (c == '\n') {
				PTEST("NEWLINE");
				update_line(buf, itr);
				if (prevc == '\n')
					break;
				prevc = '\n';
				parsing_comment = false;
			}
			else {
				PTEST("SPACE");
				prevc = IGNOREC;
			}
			wordstart = itr;
				
			continue;
		}

		// skip comment
		if (parsing_comment)
			continue;

		// pound
		if (c == '#') {
			PTEST("POUND");
			// second pound = line comment
			if (prevc == '#') {
				parsing_comment = true;
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
				if (parsing_word) {
					if (close_spans.len == open_spans.len
							|| open_spans.vals[open_spans.len - 1 - close_spans.len] != SP_UNDER) {
						SYNERR(buf, itr - 2, "Unmatched underline terminator");
						goto err;
					}
					if (!wordend)
						wordend = itr - 2;
					if (span_stack_push(&close_spans, SP_UNDER, buf, itr - 2) == FAILURE)
						goto err;
				}
				else {
					wordstart += 2;
					if (span_stack_push(&new_open_spans, SP_UNDER, buf, itr - 2) == FAILURE)
						goto err;
				}
				prevc = IGNOREC;
			}
			else
				prevc = '_';
			continue;
		}

		if (c == '.') {
			if (!parsing_word) {
				SYNERR(buf, itr - 1, "Invalid location for period");
				goto err;
			}
			if (prevc == '.') {
				assert(punct != PU_NONE);
				assert(close_spans.len > 0);
				assert(close_spans.vals[close_spans.len - 1] == SP_PUNCT);
				if (punct == PU_DOT)
					punct = PU_2DOT;
				else if (punct == PU_2DOT)
					punct = PU_3DOT;
				else {
					SYNERR(buf, itr - 1, "Too many periods");
					goto err;
				}
			}
			else if (punct != PU_NONE) {
				SYNERR(buf, itr - 1, "Invalid location for period");
				goto err;
			}
			else {
				punct = PU_DOT;
				prevc = '.';
				if (span_stack_push(&close_spans, SP_PUNCT, buf, itr - 1) == FAILURE)
					goto err;
			}
			continue;
		}


		// else word
		PTEST("WORD: %s", (itr - 1));
		prevc = WORDC;

	} // end of pg

	if (!c)
		itr--;

	update_line(buf, itr);

	if (parsing_word || prevc > ' ') {
		if (!wordend)
			wordend = itr;
		if (doc_area_list_add_word(p, wordstart, itr - wordstart) == FAILURE
				|| add_close_spans(p, &open_spans, &close_spans, punct, buf, itr - 1) == FAILURE)
			goto err;
	}

	if (open_spans.len != 0) {
		// TODO: more specific message
		SYNERR(buf, itr, "Unterminated markers");
		goto err;
	}

	span_stack_clean(&open_spans);
	span_stack_clean(&new_open_spans);
	span_stack_clean(&close_spans);

	// add pg to doc
	return doc_add_p(doc, p);

err:
	span_stack_clean(&open_spans);
	span_stack_clean(&new_open_spans);
	span_stack_clean(&close_spans);
	doc_area_list_free(p);
	return FAILURE;
}

Status compile(FILE * in, Doc * doc) {
	#ifndef NDEBUG
	// smaller size means detect problems faster
	#define DEF_BUF_SIZE 4
	#else
	#define DEF_BUF_SIZE 32
	#endif

	// our custom input buffer
	struct buffer buf = {
		.in = in,
		.start = malloc(DEF_BUF_SIZE),
		.cap = DEF_BUF_SIZE,
		.line = buf.start,
		.lineno = 1
	};
	Status ret = FAILURE;
	if (!buf.start) {
		LIBERRN();
		return FAILURE;
	}
	
	// attempt to read the first bytes
	if (try_read(&buf, 0, DEF_BUF_SIZE - 1) == FAILURE)
		goto err;

	// read and compile until eof or error
	char * itr = buf.line;
	char c;
	while (1) {

		c = *itr++;

		// ensure there is another character
		if (!c) {
			char * prev_line = buf.line;
			READMORE_EOF_ERR(&buf, break, goto err)
			itr += buf.line - prev_line - 1;
			continue;
		}

		// end of line
		if (c == '\n') {
			update_line(&buf, itr);
			continue;
		}
		
		// if we hit a printable character,
		//  compile what follows as a paragraph
		if (c > ' ') {
			if (compile_pg(&buf, doc) == FAILURE)
				goto err;
			// buffer is shifted now
			itr = buf.line;
			continue;
		}

	} // end of file

	ret = SUCCESS;
err:
	free(buf.start);
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
