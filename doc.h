#ifndef DOC_H
#define DOC_H

#include <assert.h>
#include "util.h"

	typedef struct doc Doc;
	typedef struct doc_area_list Doc_area_list;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Status doc_add_p(Doc *, Doc_area_list *);
	Status doc_close(Doc *);

	Doc_area_list * doc_area_list_new(void);
	Status doc_area_list_free(Doc_area_list *);
	Status doc_area_list_add_word(Doc_area_list *, const char * word, size_t len);
	Status doc_area_list_u_begin(Doc_area_list *);
	Status doc_area_list_u_end(Doc_area_list *);
	Status doc_area_list_sentence_period(Doc_area_list *);
	Status doc_area_list_ellipsis(Doc_area_list *);

#endif // DOC_H
