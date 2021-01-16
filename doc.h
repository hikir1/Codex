#ifndef DOC_H
#define DOC_H

#include <assert.h>
#include "util.h"

	#define DOC_AREA_TYPE(TYPE) typedef struct doc_area_ ## TYPE Doc_area_ ## TYPE;

	#define DOC_AREA(TYPE) \
	DOC_AREA_TYPE(TYPE) \
	static inline Status doc_add_ ## TYPE (Doc * doc, Doc_area_ ## TYPE * area) { \
		return doc_add(doc, (Doc_area *) area); \
	} \
	Status doc_area_ ## TYPE ## _free(Doc_area_ ## TYPE * area);

	#define DOC_AREA_C(TYPE) \
	DOC_AREA(TYPE) \
	Doc_area_ ## TYPE * doc_area_ ## TYPE ## _new(void);

	#define TEXT_AREA(TYPE) \
	static inline Status doc_area_ ## TYPE ## _add_word(Doc_area_ ## TYPE * area, const char * word, size_t len) { \
		return doc_area_text_add_word((Doc_area_text *) area, word, len); \
	}

	#define DOC_TEXT_AREA(TYPE) \
	DOC_AREA_C(TYPE) \
	TEXT_AREA(TYPE)
	

	typedef struct doc Doc;
	typedef struct doc_area Doc_area;

	extern const char * const doc_suffix;

	Doc * doc_open(const char * name);
	Status doc_add(Doc *, Doc_area *);
	Status doc_close(Doc *);

	DOC_AREA_TYPE(text)
	Status doc_area_text_add_word(Doc_area_text *, const char * word, size_t len);

	DOC_TEXT_AREA(pg)
	DOC_TEXT_AREA(bold)
	DOC_TEXT_AREA(it)
	DOC_TEXT_AREA(under)
	DOC_TEXT_AREA(strike)


	#undef DOC_AREA
	#undef TEXT_AREA
	#undef DOC_TEXT_AREA

#endif // DOC_H
