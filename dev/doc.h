#ifndef _DOC_H
#define _DOC_H

#include <assert.h>
#include "util.h"

	typedef struct doc Doc;
	typedef struct doc_area Doc_area;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Status doc_add(Doc *, Doc_area *);
	Status doc_close(Doc *);

	Status doc_area_free(Doc_area *);

	typedef void Doc_area_pg;
	Doc_area_pg * doc_area_pg_new();
	Status doc_area_pg_add_word(Doc_area_pg *, const char * word, size_t len);

#endif // _DOC_H
