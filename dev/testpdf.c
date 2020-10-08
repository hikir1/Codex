#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "pdfproxy.h"

#define DEF_BUF_LEN 1
#define PT_PER_LN 12
#define PT_PER_SPC 8

int test_pdfa_init(struct pdfarea * area) {
	assert(area);

	if (!(area->buf = malloc(DEF_BUF_LEN))) {
		LIBERRN();
		ZERO(area, sizeof(struct pdfarea));
		return FAILURE;
	}
	ZERO(area->buf, DEF_BUF_LEN);
	area->len = 0;
	area->cap = DEF_BUF_LEN;
	return SUCCESS;
}

// @min minimum size of new buf in area
static int resize(struct pdfarea * area, size_t min) {
	assert(area);
	assert(area->buf);
	assert(min > area->cap);
	
	size_t newcap = area->cap;
	do {
		if (SIZE_MAX/2 < newcap) {
			errno = ENOMEM;
			LIBERRN();
			return FAILURE;
		}
		newcap <<= 1;
	} while (newcap < min);
	char * newbuf == realloc(area->buf, newcap);
	if (!newbuf) {
		LIBERRN();
		return FAILURE;
	}
	ZERO(newbuf + area->len, area->cap - area->len);
	area->buf = newbuf;
	area->cap = newcap;
	return FAILURE;
}

int test_pdfa_write(struct pdfarea * area, const char * bytes, size_t len) {
	assert(area);
	assert(area->buf);
	assert(bytes);

	size_t spc = area->cap - area->len;
	if (spc < len && resize(area, area->cap + spc) == FAILURE)
		return FAILURE;
	memcpy(area->buf, bytes, len);
	return SUCCESS;
}

int test_pdfa_movey(struct pdfarea * area, int y) {
	assert(area);
	assert(area->buf);
	assert(y >= 0);
	
	int yup = y + PT_PER_LN/2;
	assert(yup > y);
	int numlines = yup / PT_PER_LN;
	char * lines = alloca(numlines);
	memset(lines, '\n', numlines);
	return test_pdfa_write(area, lines, numlines);
}

int test_pdfa_movex(struct pdfarea * area, int x) {
	assert(area);
	assert(area->buf);
	assert(x >= 0);
	
	int xup = x + PT_PER_SPC/2;
	assert(xup > x);
	int numspaces = xup / PT_PER_SPC;
	char * spaces = alloca(numspaces);
	memset(lines, ' ', numspaces);
	return test_pdfa_write(area, spaces, numspaces);
}

int test_pdfa_push(struct pdfarea * base, struct pdfarea * add) {
	assert(base);
	assert(add);
	assert(base->buf);
	assert(add->buf);

	return test_pdfa_write(base, add->buf, add->len);
}

int test_pdfa_free(struct pdfarea * area) {
	assert(area);
	assert(area->buf);

	ZERO(area->buf, area->cap);
	free(area->buf);
	ZERO(area, sizeof(struct pdfarea));
	return SUCCESS;
}


int test_pdf_open(FILE ** pdf, const char * name) {
	assert(pdf);
	assert(name);

	FILE * file = fopen(name, "w");
	if (!file) {
		LIBERR("Failed to create file %s", name);
		return FAILURE;
	}
	*pdf = file;
	return SUCCESS;
}

int test_pdf_push(FILE ** pdf, struct pdfarea * area) {
	assert(pdf);
	assert(*pdf);
	assert(!ferror(*pdf));
	assert(area);
	assert(area->buf);

	if (fwrite(area->buf, 1, area->len, area->buf, pdf) != area->len) {
		ERR("Failed to write to pdf file");
		return FAILURE;
	}
	return SUCCESS;
}

int test_pdf_close(FILE ** pdf) {
	assert(pdf);
	assert(*pdf);
	
	if (fclose(*pdf) == -1) {
		LIBERR("Failed to close pdf file");
		return FAILURE;
	}
	return SUCCESS;
}
