#ifndef _PDF_PROXY_H
#define _PDF_PROXY_H

#include "util.h"

#define TEST_PDF
#ifdef TEST_PDF

	typedef FILE * pdf_t;
	typedef struct pdfarea {
		char * buf;
		size_t len;
		size_t cap;
	} pdfarea_t;

	#define TESTF(func, params, args) \
		int test_ ## func params; \
		inline int func params { \
			func args ; \
		}
	
	TESTF(pdfa_init, (pdfarea_t * area), (area))
	TESTF(pdfa_write, (pdfarea_t * area, const char * bytes, size_t len), (area, bytes, len))
	TESTF(pdfa_shx, (pdfarea_t * area, int xpts), (area))
	TESTF(pdfa_shy, (pdfarea_t * area, int ypts), (area))
	TESTF(pdfa_push, (pdfarea_t * base, pdfarea_t * add), (base, add))
	TESTF(pdfa_free, (pdfarea_t * area), (area))

	TESTF(pdf_open, (pdf_t * pdf, const char * name), (pdf, name))
	TESTF(pdf_push, (pdf_t * pdf, pdfarea_t * area), (pdf, area))
	TESTF(pdf_close, (pdf_t * pdf), (pdf))

#endif // TEST_PDF

#endif // _PDF_PROXY_H
