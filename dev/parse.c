#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#include "util.h"
#include "pdfproxy.h"


struct buffer {
	FILE * in;
	char * start;
	size_t cap;
	char * line;
	size_t lineno;
};

struct arealist {
	struct arealist * next;
	pdfarea_t area;
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
	@amt amount of data to read
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
	size_t amtread = fread(readstart, 1, amt, buf->in);
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
		size_t newcap = buf->cap << 1;
		char * newstart = realloc(buf->start, newcap);
		if (!newstart) {
			LIBERRN();
			return FAILURE;
		}
		buf->start = newstart;
		buf->line = newstart;
		int ret = try_read(buf, buf->cap - 1, buf->cap);
		buf->cap = newcap;
		return ret;
	}
	size_t remaining = buf->cap - amtparsed; // includes null
	memmove(buf->start, buf->line, remaining);
	buf->line = buf->start;
	// all the bytes that have been parsed are now free space
	return try_read(buf, remaining, amtparsed);
}

#define ENSURE1DF(buf, itr, do_eof) do { \
	size_t __idx = itr - buf->start; \
	if (**(itr) == '\0') { \
		if (feof((buf)->in)) \
			do_eof; \
		else if (read_more(buf) == FAILURE) \
			return FAILURE; \
	} \
	itr = buf->start + __idx; \
while (0)

#define ENSURE1SF(buf, itr) ENSURE1DF(buf, itr, return SUCCESS)

#define NEXTLINE(buf, itr) do { \
	(buf)->line = (itr); \
	(buf)->lineno++; \
while(0)

int parsepg(struct buffer * buf, struct arealist ** words) {
	char * itr = buf.line;
	assert(*buf->line == '\0');
	while (1) {
		if (*itr == '\n') {
			itr++;
			ENSURE1SF(buf, itr);
			NEXTLINE(buf, itr);
			if (*itr == '\n') {
				NEXTLINE(buf, itr+1);
				return SUCCESS;
			}
		}
		else if (*itr == '#') {
			itr++;
			ENSURE1SF(buf, itr);
			if (*itr == '#') {
				do {
					itr++;
					ENSURE1SF(buf, itr);
				} while (*itr != '\n');
				itr++;
				NEXTLINE(buf, itr);
			}
			// TODO :env tokens
		}
		else if (*itr > ' ') {
			char * start = itr;
			do {
				itr++;
				// if eof, break and finish parsing word, then return
				ENSURE1DF(buf, itr, break);
			} while (*itr > ' ');
			struct arealist * word = malloc(sizeof(struct arealist));
			if (!word) {
				LIBERRN();
				return FAILURE;
			}
			if (!pdfa_write(word->area, start, itr - start)) { // TODO better way to handle errors?
				free(word);
				errno = ENOMEM;
				LIBERRN();
				return FAILURE;
			}
			*words = word;
			words = &word->next;

			// check if eof
			if (!*itr)
				return SUCCESS;
			// TODO implement test_pdf_proxy.c (again?)
		}
		else {
			itr++;
			ENSURE1SF(buf, itr);
		}
	}
	return SUCCESS;
}

// the paragraph will be assembled into the first arealist of words
int assemblepg(struct arealist * words) {
	
	return SUCCESS;
}

int handlepg(struct buffer * buf, pdf_t * pdf, struct arealist * words) {
	if (parsepg(&buf, words) == FAILURE)
		return FAILURE;
	assert(!words->area);
	assert(words->next);
	// assemble areas into paragraph, then push to pdf
	if (assemblepg(words->next) == FAILURE)
		return FAILURE;
	assert(!words->area);
	assert(words->next);
	// the first arealist should always be unused
	// the second, at this point, should contain the words
	//  all squashed together in one paragraph
	if (pdf_push(pdf, words->next) == FAILURE)
		return FAILURE;

}

int clear_words(struct arealist * words) {
	struct arealist * temp;
	while (words) {
		temp = words->next;
		ZERO(words, sizeof(*words))
		free(words);
		words = temp;
	}
}

int parse_loop(struct buffer * buf, pdf_t * pdf) {
	char * itr;
	struct arealist * words;
	while (1) {
		itr = buf->line;
		while (*itr != '\n') {
			if (*itr == '\0') {
				if (feof(in))
					return SUCCESS;
				if (read_more(&buf) == FAILURE)
					return FAILURE;
			}
			else if (*itr > 32) {
				words = NULL;
				int ret = handlepg(buf, pdf, &words);
				clear_words(&words);
				if (ret == FAILURE)
					return FAILURE;
			}
		}
		buf->line++;
	}
}

int parse(FILE * in, pdf_t * pdf) {
	#ifndef NDEBUG
	#define DEF_BUF_SIZE 4
	#else
	#define DEF_BUF_SIZE 32
	#endif
	struct buffer buf = {
		.in = in,
		.start = malloc(DEF_BUF_SIZE),
		.cap = DEF_BUF_SIZE,
		.line = buf.start,
		.lineno = 1
	};
	if (!buf.start) {
		LIBERRN();
		return FAILURE;
	}
	if (try_read(&buf, 0, DEF_BUF_SIZE - 1) == FAILURE) {
		free(buf.start);
		return FAILURE;
	}
	int ret = parse_loop(&buf, pdf);
	free(buf.start);
	return ret;
}

int main(int argc, char * argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <FILE>\n", argv[0]);
		return EXIT_FAILURE;
	}
	FILE * in = fopen(argv[1], "r");
	if (!in) {
		LIBERR("Failed to open %s", argv[1]);
		return EXIT_FAILURE;
	}
	if (setvbuf(in, NULL, _IONBF, 0) == -1) {
		LIBERR("setvbuf");
		fclose(in);
		return EXIT_FAILURE;
	}
	char * suffix = strrchr(argv[1], '.');
	size_t len_nosuffix = suffix? suffix - argv[1]: strlen(argv[1]);
	#define PDF_SUFFIX ".pdf"
	char * pdfname = malloc(len_nosuffix + sizeof(PDF_SUFFIX));
	if (!pdfname) {
		LIBERR("Failed to parse file name");
		fclose(in);
		return EXIT_FAILURE;
	}
	memcpy(pdfname, argv[1], len_nosuffix);
	memcpy(pdfname + len_nosuffix, PDF_SUFFIX, sizeof(PDF_SUFFIX));
	if (strcmp(argv[1], pdfname) == 0) {
		free(pdfname);
		fclose(in);
		ERR("Input and output file must be different");
		return FAILURE;
	}
	pdf_t pdf;
	if (pdf_open(&pdf, pdfname) == FAILURE) {
		LIBERR("Failed to create file %s", pdfname);
		free(pdfname);
		fclose(in);
		return EXIT_FAILURE;
	}
	free(pdfname);
	int ret = parse(in, &pdf);
	fclose(in);
	pdf_close(&out);
	return ret == FAILURE? EXIT_FAILURE: EXIT_SUCCESS;
}
