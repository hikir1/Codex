
#ifndef _STACK_H
#define _STACK_H

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#define STACK_PROTO(SYM, ELEM_T) \
\
struct stack ## SYM ## _seg { \
	struct stack ## SYM ## _seg * next; \
	size_t cap; \
	size_t len; \
	ELEM_T vals[]; \
}; \
\
typedef struct stack ## SYM { \
	struct stack ## SYM ## _seg * head; \
	struct stack ## SYM ## _seg * tail; \
} Stack ## SYM ; \
\
Stack ## SYM * stack ## SYM ## _init(Stack ## SYM *, size_t); \
Stack ## SYM * stack ## SYM ## _push(Stack ## SYM *, ELEM_T);

#ifndef NDEBUG
#ifndef NSTACK_ZERO
#define STACK_ZERO
#endif
#endif

#ifdef STACK_PRINT
#undef STACK_PRINT
#define _STACK_PRINT
#endif

#ifdef STACK_ZERO
#include <string.h>
#define _STACK_ZERO(ptr, size) memset(ptr, 0, size);
#else
#define _STACK_ZERO(ptr, size)
#endif

#define _STACK_NEWSEG(SYM, ELEM_T, CAP, NAME) \
if ((SIZE_MAX - sizeof(struct stack ## SYM ## _seg)) / sizeof(ELEM_T) < CAP) { \
	errno = ENOMEM; \
	return NULL; \
} \
struct stack ## SYM ## _seg * NAME = \
		malloc(sizeof(struct stack ## SYM ## _seg) + CAP * sizeof(ELEM_T)); \
if (!seg) \
	return NULL; \
seg->next = NULL; \
seg->cap = CAP; \
seg->len = 0; \
_STACK_ZERO(seg->vals, cap * sizeof(ELEM_T))

#define BUILD_STACK(SYM, ELEM_T) \
\
STACK_PROTO(SYM, ELEM_T) \
\
Stack ## SYM * stack ## SYM ## _init(Stack ## SYM * stack, size_t cap) { \
	assert(stack); \
	assert(cap > 0); \
	\
	_STACK_NEWSEG(SYM, ELEM_T, cap, seg) \
	stack->head = seg; \
	stack->tail = seg; \
	return stack; \
} \
\
Stack ## SYM * stack ## SYM ## _push(Stack ## SYM * stack, ELEM_T elem) { \
	assert(stack); \
	assert(stack->head); \
	\
	struct stack ## SYM ## _seg * tail = stack->tail; \
	if (tail->len == tail->cap) { \
		if (SIZE_MAX/2 < tail->cap) { \
			errno = ENOMEM; \
			return FAILURE; \
		} \
		size_t nextcap = tail->cap << 1; \
		_STACK_NEWSEG(SYM, ELEM_T, nextcap, nextseg) \
		tail->next = nextseg; \
		tail = nextseg; \
	} \
	tail->vals[tail->len] = elem; \
	tail->len++; \
	return stack; \
}

#define STACK_PRINT(SYM, PRI_ELEM) \
void _stack ## SYM ## _print_seg(const struct stack ## SYM ## _seg * seg) { \
	if (seg->len == 0) { \
		puts("[]"); \
		return; \
	} \
	printf("[%" #PRI_ELEM , seg->vals[0]); \
	for (size_t i = 1; i < seg->len; i++) \
		printf( ", %" #PRI_ELEM , seg->vals[i]); \
	fputc(']', stdout); \
} \
void stack ## SYM ## _print(const Stack ## SYM * stack) { \
	if (!stack) { \
		puts("(null)"); \
		return; \
	} \
	struct stack ## SYM ## _seg * seg = stack->head; \
	if (!seg) { \
		puts("(unintitialized)"); \
		return; \
	} \
	_stack ## SYM ## _print_seg(seg); \
	seg = seg->next; \
	while (seg) { \
		fputs("->", stdout); \
		_stack ## SYM ## _print_seg(seg); \
		seg = seg->next; \
	} \
}

struct _stack_seg { struct _stack_seg * next; };
#define stack_free(stack) do { \
	assert(stack); \
	assert(stack->next); \
	\
	struct _stack_seg * seg = (struct _stack_seg *) (stack->next); \
	struct _stack_seg * next; \
	do { \
		next = seg->next; \
		free(seg); \
		seg = next; \
	} while(seg); \
	_STACK_ZERO(stack, sizeof(*stack)) \
} while(0)

// TODO HERE <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define vecp_clear(vec) do { \
	for (size_t _vec_i = 0; _vec_i < (vec)->len; _vec_i++) \
		free((vec)->vals[_vec_i]); \
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

#define ALIAS_VEC_POINTER(VEC_SYM, VEC_ELEM_T) \
\
VEC_PROTO(VEC_SYM, VEC_ELEM_T) \
\
Vec ## VEC_SYM * vec ## VEC_SYM ## _init(Vec ## VEC_SYM * vec, size_t cap) { \
	return (Vec ## VEC_SYM *)vecp_init((Vecp *)vec, cap); \
} \
\
Vec ## VEC_SYM * vec ## VEC_SYM ## _push(Vec ## VEC_SYM * vec, VEC_ELEM_T elem) { \
	return (Vec ## VEC_SYM *)vecp_push((Vecp *)vec, elem); \
}

ALIAS_VEC_POINTER(a, char *)


#ifndef NDEBUG
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
