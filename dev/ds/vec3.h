
#ifndef VEC_H_BASE
#define VEC_H_BASE

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#define VEC_PROTO(SYM, ELEM_T) \
\
typedef struct vec ## SYM { \
	size_t len; \
	size_t cap; \
	ELEM_T * vals; \
} Vec ## SYM ; \
\
Vec ## SYM * vec ## SYM ## _init(Vec ## SYM *, size_t); \
Vec ## SYM * vec ## SYM ## _push(Vec ## SYM *, ELEM_T);

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

#define BUILD_VEC(SYM, ELEM_T) \
\
VEC_PROTO(SYM, ELEM_T) \
\
Vec ## SYM * vec ## SYM ## _init(Vec ## SYM * vec, size_t cap) { \
	assert(vec); \
	assert(cap > 0); \
	\
	if (SIZE_MAX / sizeof(ELEM_T) < cap) { \
		errno = ENOMEM; \
		return NULL; \
	} \
	ELEM_T * vals = malloc(cap * sizeof(ELEM_T)); \
	if (!vals) \
		return NULL; \
	vec->vals = vals; \
	vec->cap = cap; \
	vec->len = 0; \
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
	_VEC_ZERO((vec)->vals, vec->len * sizeof(*(vec)->vals)) \
	free((vec)->vals); \
	_VEC_ZERO((vec), sizeof(*(vec))) \
} while(0)

#define vec_clear(vec) do { \
	_VEC_ZERO((vec)->vals, (vec)->len * sizeof(*(vec)->vals)) \
	(vec)->len = 0; \
} while(0)

#define vecp_clear(vec) do { \
	for (size_t i = 0; i < (vec)->len; i++) { \
		_VEC_ZERO((vec)->vals[i], sizeof(**(vec)->vals)) \
		free((vec)->vals[i]); \
	} \
	vec_clear(vec); \
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

#define VEC_ALIAS(ALIAS_SYM, ALIAS_ELEM_T, REAL_SYM, REAL_ELEM_T) \
VEC_PROTO(ALIAS_SYM, ALIAS_ELEM_T) \
inline Vec ## ALIAS_SYM * vec ## ALIAS_SYM ## _init(Vec ## ALIAS_SYM * vec, size_t cap) { \
	return (Vec ## ALIAS_SYM *)vec ## REAL_SYM ## _init((Vec ## REAL_SYM *)vec, cap); \
} \
inline Vec ## ALIAS_SYM * vec ## ALIAS_SYM ## _push(Vec ## ALIAS_SYM * vec, ALIAS_ELEM_T elem) { \
	return (Vec ## ALIAS_SYM *)vec ## REAL_SYM ## _push((Vec ## REAL_SYM *)vec, (REAL_ELEM_T)elem); \
}

#if sizeof(int) == sizeof(long)
#define _VECP_SYM i
#define _VECR_ELEM_T int
#define _VEC_BUILD_R // TODO HERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
#else
#define _VECR_SYM ll
#define _VECR_ELEM_T long long
#endif

#define VECP_ALIAS(SYM, ELEM_T) VEC_ALIAS(SYM, ELEM_T, _VECR_SYM, _VECR_ELEM_T)

#ifdef VECC_BUILD
	VEC_BUILD(c, char)
#endif
#ifdef VECS_BUILD
	VEC_BUILD(s, short)
#endif
#ifdef VECI_BUILD
	VEC_BUILD(i, int)
#endif
#ifdef VECLL_BUILD
	VEC_BUILD(ll, long long)
#endif
#ifdef VECLD_BUILD
	VEC_BUILD(ld, long double)
#endif
#ifdef VECF_BUILD
	#ifndef VECI_BUILD
		#define VECI_BUILD
	#endif
	VEC_ALIAS(f, float, i, int)
#endif
#ifdef VECD_BUILD
	#ifndef VECLL_BUILD
		#define VECLL_BUILD
	#endif
	VEC_ALIAS(d, double, ll, long long)
#endif
#ifdef VECL_BUILD
	#ifndef VEC_BUILD
		#define VECLL_BUILD
	#endif
	VECP_ALIAS(l, long)
#define VECZ_BUILD VECP_ALIAS(z, size_t)
#define VECP_BUILD VECP_ALIAS(p, void *)
#define VECA_BUILD VECP_ALIAS(a, char *)

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
