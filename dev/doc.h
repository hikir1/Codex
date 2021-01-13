#ifndef DOC_H
#define DOC_H

#include <assert.h>
#include "util.h"

	typedef struct doc Doc;
	typedef struct doc_area Doc_area;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Status doc_add(Doc *, Doc_area *);
	Status doc_close(Doc *);

	typedef struct doc_area_pg Doc_area_pg;
	Doc_area_pg * doc_area_pg_new();
	Status doc_area_pg_add_word(Doc_area_pg *, const char * word, size_t len);
	Status doc_area_pg_free(Doc_area_pg *);

#endif // DOC_H
