#ifndef _DOC_H
#define _DOC_H

#include <assert.h>
#include "util.h"

	typedef void Doc, Doc_area, Doc_area_list;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Status doc_push(Doc *, Doc_area *);
	Status doc_close(Doc *);

	Status doc_area_free(Doc_area *);

	Doc_area_list * doc_area_list_new(void);
	Status doc_area_list_add(Doc_area_list *, Doc_area *);
	Status doc_area_list_free(Doc_area_list *);

	Doc_area * doc_text(const char *, size_t len);
	Doc_area * doc_pg(Doc_area_list *);

#endif // _DOC_H
