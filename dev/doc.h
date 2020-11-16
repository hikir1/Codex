#ifndef _DOC_H
#define _DOC_H

#include "util.h"

	typedef void Doc, Doc_pg;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Status doc_push_pg(Doc * doc, Doc_pg * pg);
	Status doc_close(Doc * doc);

	Doc_pg * doc_pg_new();
	Status doc_pg_add_word(Doc_pg * pg, const char * word, size_t len);

#endif // _DOC_H
