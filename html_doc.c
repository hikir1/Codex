#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "util.h"
#include "doc.h"

#define WRITE_ERR() ERR("Failed to write to file")
#define CLOSE_ERR() LIBERR("Failed to close output file")

const char * const doc_suffix = ".html";

#define DALFRAGLEN 56
struct area_list_node {
	struct area_list_node * next;
	char frag[DALFRAGLEN];
};
	
struct doc_area_list {
	struct area_list_node head;
	struct area_list_node * tailp;
	size_t idx;
	bool nospace;
};

#define ASSERT_DAL(list) do { \
	assert(list->tailp); \
	assert(list->idx >= 0); \
	assert(list->idx <= DALFRAGLEN); \
} while (0)

/**
 * The `Doc` representation.
 *
 * @file - the open file handle associated with the `doc`
 *
 */
struct doc {
	FILE * file;
};

#define ASSERT_FILE(file) do { \
	assert(file); \
	assert(!ferror(file)); \
} while(0)

#define ASSERT_DOC(doc) do { \
	assert(doc); \
	ASSERT_FILE(doc->file); \
} while(0)

struct doc * doc_open(const char * name) {
	assert(name);
	
	// open file
	FILE * file = fopen(name, "w");
	if (!file) {
		LIBERR("Failed create file '%s'", name);
		return NULL;
	}

	// create doc
	struct doc * doc = malloc(sizeof(struct doc));
	if (!doc) {
		LIBERRN();
		goto err;
	}

	// initialize
	doc->file = file;

	// document header
	if (fputs("<!DOCTYPE html>\n<html>\n<body>\n", file) == EOF) {
		WRITE_ERR();
		goto err;
	}

	return doc;
err:
	// free and attempt close
	free(doc);
	if (fclose(file) == EOF)
		CLOSE_ERR();
	return NULL;
}

Status doc_add_p(struct doc * doc, struct doc_area_list * list) {
	ASSERT_DOC(doc);
	ASSERT_DAL(list);

	if (fputs("<p>\n", doc->file) == EOF) {
		WRITE_ERR();
		return FAILURE;
	}

	struct area_list_node * node = &list->head;
	while (node) {
		if (fputs(node->frag, doc->file) == EOF) {
			WRITE_ERR();
			return FAILURE;
		}
		node = node->next;
	}

	if (fputs("\n</p>\n", doc->file) == EOF) {
		WRITE_ERR();
		return FAILURE;
	}

	// TODO: make this built in
	return doc_area_list_free(list);
}

Status doc_close(struct doc * doc) {
	assert(doc);
	assert(doc->file);
	
	Status ret = SUCCESS;

	// document footer
	if (fputs("</body>\n</html>\n", doc->file) == EOF) {
		WRITE_ERR();
		ret = FAILURE;
	}

	// close file
	if (fclose(doc->file) == EOF) {
		CLOSE_ERR();
		ret = FAILURE;
	}

	// free
	ZERO(doc, sizeof(struct doc));
	free(doc);

	return ret;
}

struct doc_area_list * doc_area_list_new() {
	struct doc_area_list * list = malloc(sizeof(struct doc_area_list));
	if (!list) {
		LIBERRN();
		return NULL;
	}
	list->tailp = &list->head;
	list->idx = 0;
	list->head.frag[0] = '\0';
	list->nospace = true;
	return list;
}

Status doc_area_list_free(struct doc_area_list * list) {
	ASSERT_DAL(list);

	struct area_list_node * node = list->head.next;
	struct area_list_node * next;
	while (node) {
		next = node->next;
		ZERO(node, sizeof(struct area_list_node));
		free(node);
		node = next;
	}

	ZERO(list, sizeof(struct doc_area_list));
	free(list);

	return SUCCESS;
}

Status doc_area_list_add_word(struct doc_area_list * list, const char * word, size_t len) {
	ASSERT_DAL(list);

	size_t idx = list->idx;
	#define MAXWORDLEN DALFRAGLEN - 1 // (1 nul)
	size_t rem = MAXWORDLEN - idx;
	struct area_list_node * node = list->tailp;
	char * ptr = node->frag + idx;
	size_t lenwspc = list->nospace? len: len + 1;
	while (lenwspc > rem) {
		memcpy(ptr, word, rem);
		ptr[rem] = '\0';
		
		struct area_list_node * next = malloc(sizeof(struct area_list_node));
		if (!next) {
			LIBERRN();
			return FAILURE;
		}

		node->next = next;
		list->tailp = next;
		list->idx = 0;
		ptr = next->frag;
		word += rem;
		len -= rem;
		rem = MAXWORDLEN;
	}
	memcpy(ptr, word, len);
	if (list->nospace) {
		ptr[len] = '\0';
		list>idx += len;
		list->nospace = false;
	}
	else {
		ptr[len] = ' ';
		ptr[len + 1] = '\0';
		list->idx += len + 1;
	}
}

Status doc_area_list_u_begin(struct doc_area_list * list) {
	#define UBEG "<u>"
	Status stat = doc_area_list_add_word(list, UBEG, SSTRLEN(UBEG));
	list->nospace = true;
	return stat;
}

Status doc_area_list_u_end(struct doc_area_list * list) {
	#define UEND "</u>"
	list->nospace = true;
	return doc_area_list_add_word(list, UEND, SSTRLEN(UEND));
}

Status doc_area_list_sentence_period(struct doc_area_list * list) {
	list->nospace = true;
	return doc_area_list_add_word(list, ".", 1);
}

Status doc_area_list_ellipsis(struct doc_area_list * list) {
	#define ELLIPSIS "..."
	list->nospace = true;
	return doc_area_list_add_word(list, ELLIPSIS, SSTRLEN(ELLIPSIS));
}
