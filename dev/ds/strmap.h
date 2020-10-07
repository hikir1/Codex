#ifndef _STRMAP_H
#define _STRMAP_H

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#define STRMAP_PROTO(SYM, ELEM_T) \
struct strmap ## SYM ## _list { \
	char * key; \
	ELEM_T val; \
	struct strmap ## SYM ## _list * next; \
}; \
PACK_PROTO(_strmap ## SYM ## _list, struct strmap ## SYM ## _list) \
typedef struct strmap { \
	size_t nslots; \
	struct strmap ## SYM ## _list * data; \
	Pack_strmap ## SYM ## _list // TODO HERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
} Strmap; \
Strmap ## SYM * strmap ## SYM ## _init(Strmap ## SYM * map, size_t size); \
Strmap ## SYM * strmap ## SYM ## _put(Strmap ## SYM * map, ELEM_T elem);

#ifndef NDEBUG
#ifndef NSTRMAP_ZERO
#ifndef STRMAP_ZERO
#define STRMAP_ZERO
#endif
#endif
#endif

#ifdef STRMAP_ZERO
#include <string.h>
#define _STRMAP_ZERO(ptr, size) memset(ptr, 0, size);
#else
#define _STRMAP_ZERO(ptr, size)
#endif

#define STRMAP_BUILD(SYM, ELEM_T) \
STRMAP_PROTO(SYM, ELEM_T) \
Strmap ## SYM * strmap ## SYM ## _init(Strmap ## SYM * map, size_t cap, size_t nslots) { \
	if (SIZE_MAX/sizeof(struct strmap ## SYM ## _list) < nslots) { \
		errno = ENOMEM; \
		return NULL; \
	} \
	if (!(map = malloc(nslots * sizeof(struct strmap ## SYM ## _list)))) \
		return NULL; \
	map->nslots = nslots; \
	_STRMAP_ZERO(map->data, nslots * sizeof(struct strmap ## SYM ## _list)) \
} \
Strmap ## SYM * strmap ## SYM ## _put(Strmap ## SYM * map, ELEM_T elem) { \
	

#endif
