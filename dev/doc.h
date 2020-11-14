#ifndef _DOC_H
#define _DOC_H

#include "util.h"

	typedef void Doc, Doc_area, Doc_pg;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Doc_pg * doc_new_pg(const Doc * doc);
	Status doc_commit_pg(const Doc * doc, Doc_pg * pg);
	Status doc_close(Doc * doc);

	Doc_area * doc_pg_new_area();
	Status doc_pg_commit_area(const Doc_pg * pg, Doc_area * area);

	Status doc_area_write(const Doc_area * area, const char * text);

#endif // _DOC_H
