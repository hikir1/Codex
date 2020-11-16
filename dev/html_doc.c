#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "doc.h"

#define WRITE_ERR() ERR("Failed to write to file.")

extern const char * const doc_suffix = ".html"

struct area {
	char text[];
}

struct area_list {
	struct area_list * next;
	struct area area;
}

struct pg {
	struct area_list * areas;
	struct area_list ** tail_ptr;
}

struct doc {
	FILE * file;
}

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

struct pg * doc_pg_new() {
	struct pg * pg = malloc(sizeof(struct pg));

	if (!pg)
		return NULL;
	pg->areas = NULL;
	pg->tail_ptr = &pg->areas;
	return pg;
}

Status doc_push_pg(struct doc * doc, struct pg * pg) {
	assert(doc);
	assert(pg)
	assert(pg->tail_ptr);
	assert(doc->file);
	assert(!ferror(doc->file));

	Status ret = FAILURE;

	// start paragraph
	if (fputs("<p>\n", doc->file) == EOF) {
		WRITE_ERR();
		goto err;
	}

	// print text of each area as separate words
	struct area_list * area_itr = pg->areas;
	while (area_itr) {
		if (fputs(area_iter->area.text, doc->file) == EOF
				|| fputc(' ', doc->file) == EOF) {
			WRITE_ERR();
			goto err;
		}
		area_itr = area_itr->next;
	}

	if (fputs("\n<\\p>\n", doc->file) == EOF) {
		WRITE_ERR();
		goto err;
	}

	ret = SUCCESS;

err:	
	// free areas separately so not interupted if print fails
	area_itr = pg->areas;
	pg->areas = NULL;
	while (area_itr) {
		struct area_list * next = area_itr->next;
		ZEROS(area_itr);
		free(area_itr);
	}

	// free pg
	ZEROS(pg);
	free(pg);

	return ret;
}

Status doc_close(struct doc * doc) {
	assert(doc);
	assert(doc->file);

	Status ret = fclose(doc->file) == EOF? FAILURE: SUCCESS;

	ZEROS(doc);
	free(doc);

	return ret;
}

// TODO: NOTE: when errno set to EOVERFLOW its bc user's word is too big
//  An informative message must be printed by caller
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
