#include <assert.h>
#include "stream.h"

#define ASSERT_STREAM(stream) do { \
	assert(stream->head); \
	assert(stream->tail); \
	assert(stream->tail->next == NULL); \
} while (0)

#define DEFINE_STREAM(NAME, TYPE) \
\
DECLARE_STREAM(NAME, TYPE) \
\
struct NAME ## _stream_frag * NAME ## _stream_frag_new() { \
\
	struct NAME ## _stream_frag * frag = malloc(sizeof(*frag)); \
	if (!frag) { \
		LIBERRN(); \
		return NULL; \
	} \
\
	frag->next = NULL; \
\
	return frag; \
} \
\
Status NAME ## _stream_init(struct NAME ## _stream * stream) { \
	assert(stream); \
\
	struct NAME ## _stream_frag * head = NAME ## _stream_frag_new(); \
	if (!head) \
		return FAILURE; \
\
	stream->head = head; \
	stream->tail = head; \
	stream->tail_len = 0; \
\
	return SUCCESS; \
} \
\
Status NAME ## _stream_clean(struct NAME ## _stream * stream) { \
	ASSERT_STREAM(stream); \
\
	struct NAME ## _stream_node * itr = stream->head; \
	struct NAME ## _stream_node * next; \
	while (itr) { \
		next = itr->next; \
		ZERO(itr, sizeof(struct NAME ## _stream_frag)); \
		free(itr); \
		itr = next; \
	} \
\
	ZERO(stream, sizeof(struct NAME ## _stream)); \
	return SUCCESS; \
} \
\
Status NAME ## _stream_push(struct NAME ## _stream * stream, enum tok tok) { \
	ASSERT_STREAM(stream); \
\
	if (stream->tail_len == STREAM_FRAG_CAP) { \
\
		struct NAME ## _stream_frag * frag = NAME ## _stream_frag_new(); \
		if (!frag) \
			return FAILURE; \
\
		stream->tail->next = frag; \
		stream->tail = frag; \
		stream->tail_len = 0; \
	} \
\
	stream->tail->vals[stream->tail_len++] = tok; \
}

DEFINE_STREAM(tok, enum tok)
DEFINE_STREAM(text, char *)
