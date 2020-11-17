#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "doc.h"

#define WRITE_ERR() ERR("Failed to write to file.")

const char * const doc_suffix = ".html";

struct text_node {
	struct text_node * next;
	char text[];
};

struct area {
	struct text_node * head;
	struct text_node ** tail_ptr;
};

struct area_node {
	struct area_node * next;
	struct area * area;
};

struct area_list {
	struct area_node * head;
	struct area_node ** tail_ptr;
};

struct doc {
	FILE * file;
};

struct doc * doc_open(const char * name) {
	assert(name);
	
	FILE * file = fopen(name, "w");
	if (!file) {
		LIBERR("Failed create file '%s'", name);
		return NULL;
	}
	struct doc * doc = malloc(sizeof(struct doc));
	if (!doc) {
		LIBERRN();
		goto err;
	}
	doc->file = file;
	if (fputs("<!DOCTYPE html>\n<html>\n<body>\n", file) == EOF) {
		WRITE_ERR();
		goto err;
	}
	return doc;
err:
	free(doc);
	if (fclose(file) == EOF)
		LIBERR("Failed to close output file");
	return NULL;
}

Status doc_push(struct doc * doc, struct area * area) {
	assert(doc);
	assert(area)
	assert(area->tail_ptr);
	assert(doc->file);
	assert(!ferror(doc->file));

	struct text_node * itr = area->head;
	FILE * file = doc->file;
	Status ret = FAILURE;

	while (itr) {
		if (fputs(itr->text, file) == FAILURE) {
			WRITE_ERR();
			goto err;
		}
		itr = itr->next;
	}

	ret = SUCCESS;

err:
	return doc_area_free(area) == FAILURE? FAILURE: ret;
}

Status doc_close(struct doc * doc) {
	assert(doc);
	assert(doc->file);

	Status ret = fclose(doc->file) == EOF? FAILURE: SUCCESS;

	ZEROS(doc);
	free(doc);

	return ret;
}

Status doc_area_free(struct area * area) {
	assert(area);
	assert(area->tail_ptr);

	struct text_node * itr = area->head;
	struct text_node * next;
	while (itr) {
		next = itr->next;
		ZEROS(itr);
		free(itr);
		itr = next;
	}
	return SUCCESS;
}

struct area_list * doc_area_list_new() {
	struct area_list * list = malloc(sizeof(struct area_list));
	if (!list) {
		LIBERRN();
		return NULL;
	}
	list->head = NULL;
	list->tail_ptr = &list->head;
	return list;
}

Status doc_area_list_add(struct area_list * list, struct area * area) {
	assert(list);
	assert(area);
	assert(list->tail_ptr);
	assert(!*list->tail_ptr);
	assert(area->tail_ptr);

	struct area_node * node = malloc(sizeof(struct area_node));
	if (!node) {
		LIBERRN();
		return FAILURE;
	}
	node->next = NULL;
	node->area = area;
	*list->tail_ptr = node;
	list->tail_ptr = &node->next;

	return SUCCESS;
}

Status doc_area_list_free(struct area_list * list) {
	// TODO: asserts
	struct area_node * itr = list->head;
	struct area_node * next;
	while (itr) {
		next = itr->next;
		ZEROS(itr);
		free(itr);
		itr = next;
	}
	return SUCCESS;
}

struct area * doc_text(const char * text, size_t len) {
	assert(text);
// TODO

Status doc_pg_add_word(struct pg * pg, const char * word, size_t len) {
	assert(pg);
	assert(word);
	assert(pg->tail_ptr);

	if (len > SIZE_MAX - sizeof(struct area_list)) {
		errno = EOVERFLOW;
		return FAILURE;
	}
	struct area_list * area_node = malloc(len + sizeof(struct area_list));
	if (!area_node) {
		LIBERRN();
		return FAILURE;
	}
	memcpy(area_node->area->text, word, len);
	*pg->tail_ptr = area_node;
	pg->tail_ptr = &area_node->next;

	return SUCCESS;

}
