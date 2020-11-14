#ifndef _DOC_H
#define _DOC_H

#include "util.h"

	typedef void Doc, Doc_area, Doc_pg;

	Doc * doc_open(const char * name);
	Doc_pg * doc_new_pg(const Doc * doc);
	Doc * doc_commit_pg(const Doc * doc, Doc_pg * pg);
	int doc_close(Doc * doc);

	Doc_area * doc_pg_new_area();
	Doc_pg * doc_pg_commit_area(const Doc_pg * pg, Doc_area * area);

	Doc_area * doc_area_write(const Doc_area * area, const char * text);

#endif // _DOC_H
