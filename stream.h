#ifndef STREAM_H
#define STREAM_H

#include "util.h"

#define STREAM_FRAG_CAP 2

#define DECLARE_STREAM(NAME, TYPE) \
\
struct NAME ## _stream_frag { \
	struct NAME ## _stream_frag * next; \
	TYPE vals[STREAM_FRAG_CAP]; \
}; \
\
struct NAME ## _stream { \
	struct NAME ## _stream_frag * head; \
	struct NAME ## _stream_frag * tail; \
	size_t tail_len; \
}; \
\
struct NAME ## _stream_frag * NAME ## _stream_frag_new(void); \
Status NAME ## _stream_init(struct NAME ## _stream * stream); \
Status NAME ## _stream_clean(struct NAME ## _stream * stream); \
Status NAME ## _stream_push(struct NAME ## _stream * stream, TYPE tok);

enum tok {
	WORD,
	PERIOD,
	END_PARAGRAPH,
	START_UNDERLINE,
	END_UNDERLINE,
};

DECLARE_STREAM(tok, enum tok)
DECLARE_STREAM(text, char *)

#endif // STREAM_H
