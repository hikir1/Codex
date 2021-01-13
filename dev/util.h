#ifndef UTIL_H
#define UTIL_H

#define ERR_PREFIX "ERROR: "
#define ERR(msg, ...) fprintf(stderr, ERR_PREFIX msg "\n", ##__VA_ARGS__)
#define LIBERR(msg, ...) do{ fprintf(stderr, ERR_PREFIX msg ": ", ##__VA_ARGS__); perror(NULL); }while(0)
#define LIBERRN() LIBERR("")
#define SYNERR(buf, pos, msg, ...) do{ \
	ERR("Line %zu: " msg ": ", buf->lineno, ##__VA_ARGS__); \
	showctx(buf, pos); \
}while(0)

#ifndef NDEBUG
#define ZERO(ptr, size) memset(ptr, 0, size)
#else
#define ZERO(ptr, size) (void)
#endif

typedef int Status;
#define SUCCESS 0
#define FAILURE 1

#endif // UTIL_H
