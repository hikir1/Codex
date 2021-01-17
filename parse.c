#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#include "util.h"
#include "doc.h"


struct buffer {
	FILE * in;
	char * start;
	size_t cap;
	char * line;
	size_t lineno;
};

void showctx(struct buffer * buf, char * pos) {
	assert(buf);
	assert(pos);
	assert(buf->line);
	assert(pos > buf->line);
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

#define ENSURE1DD(buf, itr, do_eof, do_err) do { \
	if (*(itr) == '\0') { \
		char * _old_line = (buf)->line; \
		if (feof((buf)->in)) \
			do_eof; \
		else if (read_more(buf) == FAILURE) \
			do_err; \
		(itr) += (buf)->line - _old_line; \
	} \
} while (0)

#define ENSURE1DF(buf, itr, do_eof) ENSURE1DD(buf, itr, do_eof, return FAILURE)
#define ENSURE1SF(buf, itr) ENSURE1DF(buf, itr, return SUCCESS)
#define ENSURE1BE(buf, itr) ENSURE1DD(buf, itr, break, goto err)

#define NEXTLINE(buf, itr) do { \
	(buf)->line = (itr); \
	(buf)->lineno++; \
} while(0)

#define IDX(itr, buf) ((itr) - (buf)->line)

Status compile_pg(struct buffer * buf, Doc * doc) {
	Doc_area_list * p = doc_area_list_new();
	if (!p)
		return FAILURE;
	size_t startidx = 0;
	char * itr = buf->line;

	// build paragraph until double newline, eof, or error
	while (*itr) {

		// newline
		if (*itr == '\n') {
			PTEST("NEWLINE");
			itr++;
			startidx = 0;
			NEXTLINE(buf, itr);
			ENSURE1BE(buf, itr);
			// second newline = end of paragraph
			if (*itr == '\n')
				break;
			continue;
		}

		// pound
		else if (*itr == '#') {
			PTEST("POUND");
			itr++;
			ENSURE1BE(buf, itr);
			// second pound = line comment
			if (*itr == '#') {
				do {
					itr++;
					ENSURE1BE(buf, itr);
				} while (*itr != '\n');
				startidx = IDX(itr, buf);
			}
			continue;
		}

		// white space (or non-printable)
		else if (*itr == ' ') {
			PTEST("SPACE");
			itr++;
			startidx++;
			ENSURE1BE(buf, itr);
			continue;
		}

		// under
		else if (*itr == '_') {
			PTEST("UNDER");
			itr++;
			if (*itr == '_') {
				itr++;
				startidx += 2;
				if (doc_area_list_u_begin(p) == FAILURE) {
					doc_area_list_free(p);
					return FAILURE;
				}
			}
			continue;
		}
		{ // else word
			// find the last character
			do {
				itr++;
				// if eof, this is the last word
				ENSURE1BE(buf, itr);
			} while (*itr > ' ');
			size_t endidx = IDX(itr, buf);

			// write word to pg
			if (doc_area_list_add_word(p, buf->line + startidx, endidx - startidx) == FAILURE)
				goto err;
			PTEST("START: %s", buf->start + startidx);
			startidx = endidx;
		}

	} // end of pg

	NEXTLINE(buf, itr);

	// add pg to doc
	return doc_add_p(doc, p);

err:
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
	while (1) {
		char * itr = buf.line;

		// process line by line
		while (*itr != '\n') {

			// ensure there is another character
			ENSURE1DD(&buf, itr, goto break2, goto err);
			
			// if we hit a printable character,
			//  compile what follows as a paragraph
			if (*itr > ' ') {
				if (compile_pg(&buf, doc) == FAILURE)
					goto err;
				// buffer is shifted now
				itr = buf.line;
			}
		} // end of line

		// update line count
		buf.line++;

	} // end of file

break2:
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
