
#ifndef VEC_H_BASE
#define VEC_H_BASE

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#define VEC_PROTO(SYM, ELEM_T) \
typedef struct vec ## SYM { \
	ELEM_T * vals; \
	size_t len; \
	size_t cap; \
	size_t _elem_t_size; \
} Vec ## SYM ; \

#ifndef NDEBUG
#ifndef NVEC_ZERO
#define VEC_ZERO
#endif
#endif

#ifdef VEC_PRINT
#undef VEC_PRINT
#define _VEC_PRINT
#endif

#ifdef VEC_ZERO
#include <string.h>
#define _VEC_ZERO(ptr, size) memset(ptr, 0, size)
#else
#define _VEC_ZERO(ptr, size)
#endif

struct _vec {
	char * data;
	size_t len;
	size_t cap;
	size_t _elem_t_size;
};

#ifdef BUILD_VEC
int _vec_init(struct vec * vec, size_t cap, size_t elem_t_size) { \
	assert(vec); \
	assert(cap > 0); \
	\
	if (SIZE_MAX / elem_t_size < cap) { \
		errno = ENOMEM; \
		return NULL; \
	} \
	char * vals = malloc(cap * elem_t_size); \
	if (!vals) \
		return NULL; \
	vec->vals = vals; \
	vec->cap = cap; \
	vec->len = 0; \
	vec->elem_t_size
	_VEC_ZERO(vals, cap * sizeof(ELEM_T)) \
	return vec; \
} \
\
Vec ## SYM * vec ## SYM ## _push(Vec ## SYM * vec, ELEM_T elem) { \
	assert(vec); \
	assert(vec->cap > 0); \
	\
	if (vec->len == vec->cap) { \
		if (vec->cap == SIZE_MAX / sizeof(ELEM_T)) { \
			errno = ENOMEM; \
			return NULL; \
		} \
		size_t newsize; \
		if (SIZE_MAX / sizeof(ELEM_T) / 2 < vec->cap) \
			newsize = SIZE_MAX / sizeof(ELEM_T); \
		else \
			newsize = vec->cap << 1; \
		\
		ELEM_T * tmp = realloc(vec->vals, newsize); \
		if (!tmp) \
			return NULL; \
		vec->vals = tmp; \
		vec->cap = newsize; \
		_VEC_ZERO(vec->vals + vec->len, (vec->cap - vec->len) * sizeof(ELEM_T)) \
	} \
	vec->vals[vec->len] = elem; \
	vec->len++; \
	return vec; \
}

#define VEC_PRINT(SYM, PRI_ELEM) \
static void vec ## SYM ## _print(Vec ## SYM * vec) { \
	if (!vec) { \
		puts("(null)"); \
		return; \
	} \
	if (vec->len == 0) { \
		puts("[]"); \
		return; \
	} \
	printf("[%" #PRI_ELEM , vec->vals[0]); \
	for (size_t i = 1; i < vec->len; i++) \
		printf( ", %" #PRI_ELEM , vec->vals[i]); \
	puts("]"); \
}

#define vec_free(vec) do { \
	free((vec)->vals); \
	_VEC_ZERO(vec, sizeof(*vec)); \
} while(0)

#define vecp_clear(vec) do { \
	for (size_t i = 0; i < (vec)->len; i++) \
		free((vec)->vals[i]); \
	(vec)->len = 0; \
} while(0)

#define vec_init(vec, cap) _Generic((vec), \
	Vecc *: vecc_init, \
	Vecs *: vecs_init, \
	Veci *: veci_init, \
	Vecl *: vecl_init, \
	Vecll *: vecll_init, \
	Vecf *: vecf_init, \
	Vecd *: vecd_init, \
	Vecld *: vecld_init, \
	Vecz *: vecz_init, \
	Veca *: veca_init, \
	default: vecp_init \
) (vec, cap)

#define vec_push(vec, elem) _Generic((vec), \
	Vecc *: vecc_push, \
	Vecs *: vecs_push, \
	Veci *: veci_push, \
	Vecl *: vecl_push, \
	Vecll *: vecll_push, \
	Vecf *: vecf_push, \
	Vecd *: vecd_push, \
	Vecld *: vecld_push, \
	Vecz *: vecz_push, \
	Veca *: veca_push, \
	default: vecp_push \
) (vec, elem)

#define BUILD_VEC_CHAR BUILD_VEC(c, char)
#define BUILD_VEC_SHORT BUILD_VEC(s, short)
#define BUILD_VEC_INT BUILD_VEC(i, int)
#define BUILD_VEC_LONG BUILD_VEC(l, long)
#define BUILD_VEC_LONG_LONG BUILD_VEC(ll, long long)
#define BUILD_VEC_FLOAT BUILD_VEC(f, float)
#define BUILD_VEC_DOUBLE BUILD_VEC(d, double)
#define BUILD_VEC_LONG_DOUBLE BUILD_VEC(ld, long double)
#define BUILD_VEC_SIZE BUILD_VEC(z, size_t)
#define BUILD_VEC_POINTER BUILD_VEC(p, void *)

VEC_PROTO(c, char)
VEC_PROTO(s, short)
VEC_PROTO(i, int)
VEC_PROTO(l, long)
VEC_PROTO(ll, long long)
VEC_PROTO(f, float)
VEC_PROTO(d, double)
VEC_PROTO(ld, long double)
VEC_PROTO(z, size_t)
VEC_PROTO(p, void *)

#define ALIAS_VEC_POINTER(SYM, ELEM_T) \
\
VEC_PROTO(SYM, ELEM_T) \
\
Vec ## SYM * vec ## SYM ## _init(Vec ## SYM * vec, size_t cap) { \
	return (Vec ## SYM *)vecp_init((Vecp *)vec, cap); \
} \
\
Vec ## SYM * vec ## SYM ## _push(Vec ## SYM * vec, ELEM_T elem) { \
	return (Vec ## SYM *)vecp_push((Vecp *)vec, elem); \
}

ALIAS_VEC_POINTER(a, char *)


#ifdef _VEC_PRINT
#ifndef NVEC_PRINT
#include <stdio.h>
#define vec_print(vec) _Generic((vec), \
	Vecc *: vecc_print, \
	Vecs *: vecs_print, \
	Veci *: veci_print, \
	Vecl *: vecl_print, \
	Vecll *: vecll_print, \
	Vecf *: vecf_print, \
	Vecd *: vecd_print, \
	Vecld *: vecld_print, \
	Vecz *: vecz_print, \
	Veca *: veca_print, \
	default: vecp_print \
) (vec)
VEC_PRINT(c, c)
VEC_PRINT(s, hd)
VEC_PRINT(i, d)
VEC_PRINT(l, ld)
VEC_PRINT(ll, lld)
VEC_PRINT(f, g)
VEC_PRINT(d, g)
VEC_PRINT(ld, Lg)
VEC_PRINT(z, zd)
VEC_PRINT(p, p)
VEC_PRINT(a, s)
#endif
#endif

#endif
