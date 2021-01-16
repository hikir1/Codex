#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ERR_PREFIX "ERROR: "
#define ERR(msg, ...) fprintf(stderr, ERR_PREFIX msg "\n", ##__VA_ARGS__)
#define LIBERR(msg, ...) do{ fprintf(stderr, ERR_PREFIX msg ": ", ##__VA_ARGS__); perror(NULL); }while(0)
#define LIBERRN() LIBERR("")
#define SYNERR(buf, pos, msg, ...) do{ \
	ERR("Line %zu: " msg ": ", (buf)->lineno, ##__VA_ARGS__); \
	showctx(buf, pos); \
}while(0)

#ifndef NDEBUG
// Fill old buffer with A's, hopefully causing an error or showing up in db
// if dangling pointer exists
#define REALLOC(oldptr, newptr, oldsize, used, newsize) do { \
	assert((used) <= (oldsize)); \
	\
	(newptr) = malloc(newsize); \
	if (newptr) { \
		memcpy(newptr, oldptr, (used) < (newsize)? (used): (newsize)); \
		memset(oldptr, 'A', oldsize); \
		free(oldptr); \
	} \
} while(0)
#else
#define REALLOC(oldptr, newptr, oldsize, used, newsize) (newptr) = realloc(oldptr, newsize)
#endif


#ifndef NDEBUG
#define ZERO(ptr, size) memset(ptr, 0, size)
#else
#define ZERO(ptr, size)
#endif

#ifdef PTEST
#undef PTEST
#define PTEST(STR, ...) fprintf(stderr, STR "\n", ##__VA_ARGS__)
#else
#define PTEST(STR, ...)
#endif

typedef int Status;
#define SUCCESS 0
#define FAILURE 1

#endif // UTIL_H
