#ifndef _DOC_H
#define _DOC_H

#include "util.h"

	typedef void Doc, Doc_pg;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Doc_pg * doc_new_pg(Doc * doc);
	Status doc_commit_pg(Doc * doc, Doc_pg * pg);
	Status doc_close(Doc * doc);

	Status doc_pg_add_word(Doc_pg * pg, const char * word, size_t len);

#endif // _DOC_H
