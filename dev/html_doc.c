#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "util.h"
#include "doc.h"

#define WRITE_ERR() ERR("Failed to write to file")
#define CLOSE_ERR() LIBERR("Failed to close output file")

const char * const doc_suffix = ".html";

struct area_ops {
	Status (*write)(struct area * seld, FILE *);
	Status (*free)(struct area * self);
};

struct area {
	struct area_ops * ops;
};

struct doc {
	FILE * file;
};

struct doc * doc_open(const char * name) {
	
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
		CLOSE_ERR();
	return NULL;
}

Status doc_add(struct doc * doc, struct area * area) {

	return area->ops->write(area, doc->file);
}

Status doc_close(struct doc * doc) {
	
	Status ret = FAILURE;

	if (fputs("</body>\n</html>\n", doc->file) == EOF) {
		WRITE_ERR();
		goto err;
	}

	if (fclose(doc->file) == EOF) {
		CLOSE_ERR();
		goto err;
	}

	ret = SUCCESS;

err:
	ZEROS(doc);
	free(doc);

	return ret;
}

Status doc_area_free(struct area * area) {

	return area->ops->free(area);
}

/////////////////////////////////////
// Area List
/////////////////////////////////////

struct area_list {
	struct area_list * next;
	struct area * area;
};

static inline Status area_list_free_first(struct area_list * itr) {
	Status ret = itr->area->ops->free(itr->area);
	ZERO(itr, sizeof(itr));
	free(itr);
	return ret;
}

static Status area_list_free_all(struct area_list * itr) {
	Status ret = SUCCESS;
	struct area_list * next;
	while (itr) {
		next = itr->next;
		if (area_list_free_first(itr) == FAILURE)
			ret = FAILURE;
		itr = next;
	}
	return ret;
}

static struct area_list * area_list_new(struct area * area) {
	struct area_list * list = malloc(sizeof(struct area_list));
	if (!list) {
		LIBERRN();
		return NULL;
	}
	list->next = NULL;
	list->area = area;
	return list;
}


/////////////////////////////////////
// Text List
/////////////////////////////////////

struct text_list {
	struct text_list * next;
	char text[];
};

struct text_list_area {
	struct area area;
	struct text_list * head;
	struct text_list ** tail_ptr;
};

static inline void text_list_free_first(struct text_list * itr) {
	ZERO(itr, sizeof(struct text_list) + strlen(itr->text));
	free(itr);
}

static void text_list_free_all(struct text_list * itr) {
	struct text_list * next;
	while (itr) {
		next = itr->next;
		text_list_free_first(itr);
		itr = next;
	}
}

static inline struct text_list * text_list_new(const char * text, size_t len) {
	if (len > SIZE_MAX - sizeof(struct text_list)) {
		ERR("Token too big.");
		return NULL;
	}
	struct text_list * list = malloc(sizeof(struct text_list) + len);
	if (!list) {
		LIBERRN();
		return NULL;
	}
	list->next = NULL;
	memcpy(list->text, text, len);
	return list;
}


/////////////////////////////////////
// Text List Area
/////////////////////////////////////

static inline void text_list_area_free_just_self(struct text_list_area * list_area) {
	ZEROS(list_area);
	free(list_area);
}

static Status text_list_area_free(struct text_list_area * list_area) {
	text_list_free_all(list_area->head);
	text_list_area_free_just_self(list_area);
	return SUCCESS;
}

static struct text_list_area * text_list_area_new(struct area_ops * ops) {
	struct text_list_area * list_area = malloc(sizeof(struct text_list_area));
	if (!list_area) {
		LIBERRN();
		return NULL;
	}
	list_area->area.ops = ops;
	list_area->head = NULL;
	list_area->tail_ptr = &list->head;
	return list_area;
}

static inline Status text_list_area_add(struct text_list_area * list_area,
		const char * text, size_t len) {
	struct text_list * list = text_list_new(text, len);
	if (!list)
		return FAILURE;
	*list_area->tail_ptr = list;
	list_area->tail_ptr = &list->next;
	return SUCCESS;
}

static text_list_area_write(struct text_list_area * list_area, FILE * file) {

	struct text_list * itr = list_area->head;
	struct text_list * next;
	Status ret = SUCCESS;

	while (itr) {
		next = itr->next;
		if (fputs(itr->text, file) == EOF
				|| fputc(' ', file) == EOF) {
			WRITE_ERR();
			text_list_free_all(itr);
			ret = FAILURE;
			break;
		}
		text_list_free_first(itr);
		itr = next;
	}

	text_list_area_free_just_self(list_area);

	return ret;
}

const struct area_ops text_list_area_ops = {
	.write = text_list_area_write,
	.free = text_list_area_free,
};


////////////////////////////////////////////
// PG Area
////////////////////////////////////////////

struct pg_area {
	struct area area;
	struct area_list * head;
	struct area_list * tail;
};

Status pg_area_free(struct pg_area * pg) {
	Status ret = area_list_free_all(pg->head);
	ZERO(pg, sizeof(struct pg_area));
	free(pg);
	return ret;
}

// TODO: write op should not free
// TODO: where freeing, NULL?

Status pg_area_write(struct pg_area * pg, FILE * file) {
	struct area_list * itr = pg->head;
	struct area_list * next;

	ZERO(pg, sizeof(struct pg));
	free(pg);

	if (fputs("<p>\n", file) == EOF) {
		WRITE_ERR();
		goto err;
	}
	while (itr) {
		next = itr->next;
		Status write_ret = itr->area->ops->write(itr->area, file);
		itr->area = NULL; // guaranteed freed <<<<<<<<<<<<<<<<< could this cause problem with asserts?
		if (write_ret == FAILURE)
			goto err;
		if (fputc(' ', file) == EOF) {
			WRITE_ERR();
			goto err;
		}
		if (area_list_free_first(itr) == FAILURE)
			goto err;
		itr = next;
	}
	if (fputs("\n</p>\n", file) == EOF) {
		WRITE_ERR();
		goto err;
	}

	return SUCCESS;

err:
	area_list_free_all(itr);
	return FAILURE;
}

const struct area_ops pg_area_ops = {
	.write = pg_area_write,
	.free = pg_area_free,
}

struct pg_area * doc_area_pg_new() {
	struct pg_area * pg = malloc(sizeof(struct pg_area));
	if (!pg) {
		LIBERRN();
		return NULL;
	}
	pg->area.ops = pg_area_ops;
	pg->head = NULL;
	pg->tail = NULL;
	return pg;
}

Status doc_area_pg_add_word(struct pg_area * pg, const char * word, size_t len) {
	if (!pg->tail || pg->tail->ops != text_list_area_ops) {
		struct text_list_area * list_area = text_list_area_new();
		if (!list_area)
			return FAILURE;
		if (!pg->tail) {
			pg->head = list_area;
			pg->tail = list_area;
		}
		else {
			pg->tail->next = list_area;
			pg->tail = list_area;
		}
	}
	if (text_list_area_add(pg->tail, word, len) == FAILURE)
		return FAILURE;
	return SUCCESS;
}

