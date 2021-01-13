#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "doc.h"

#define WRITE_ERR() ERR("Failed to write to file")
#define CLOSE_ERR() LIBERR("Failed to close output file")

const char * const doc_suffix = ".html";

typedef Status (*Vft_write)(struct doc_area *, FILE *);
typedef Status (*Vft_free)(struct doc_area *);

/**
 * A VFT of operations that the associated `area` can perform
 *
 * @write - function
 * @free - function
 *
 */
struct area_ops {
	/**
	 * write's the `area`s representation to the given file
	 *
	 * @self - the `area` that is to do the writing
	 * @file - the file to write to
	 *
	 * RETURNS: Status
	 *
	 */
	Vft_write write;

 	/**
	 * prepares the `area` and its contents to be released, then frees it
	 *
	 * @self - the `area` that is to do the freeing
	 *
	 * RETURNS: Status
	 *
	 */
	Vft_free free;
};

#define ASSERT_AREA_OPS(ops) do { \
	assert(ops); \
	assert(ops->write); \
	assert(ops->free); \
} while(0)

/**
 * The abstract `Doc_area` representation.
 * This is the parent struct of all `Doc_area`s,
 *  henceforth called `area`s.
 *
 * @ops - The VFT associated with the `area`.
 * 	This allows specific operations to be performed
 * 	an an `area` without knowing its exact type (Polymorphism).
 *
 */
struct doc_area {
	const struct area_ops * ops;
};

#define ASSERT_AREA(area) do { \
	assert((struct doc_area *)(area)); \
	ASSERT_AREA_OPS(((struct doc_area *)(area))->ops); \
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

Status doc_add(struct doc * doc, struct doc_area * area) {
	ASSERT_DOC(doc);
	ASSERT_AREA(area);

	// write, then free
	Status write_ret = area->ops->write(area, doc->file);
	return area->ops->free(area) == FAILURE? FAILURE: write_ret;
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

/////////////////////////////////////
// Area List
/////////////////////////////////////

/**
 * simple recursive linked list for `area`s
 *
 * @next - the next list node
 * @area - the current element
 *
 * RETURNS: Status
 *
 */
struct area_list {
	struct area_list * next;
	struct doc_area * area;
};

#define ASSERT_AREA_LIST(list) do { \
	assert(list); \
	assert(list->area); \
} while(0)

/**
 * recursively frees the entire list, starting from `head`
 *
 * @head - the first node of the list
 *
 * RETURNS: Status
 *
 */
static Status area_list_free_all(struct area_list * head) {
	struct area_list * itr = head;
	struct area_list * next;
	Status ret = SUCCESS;

	// go through each node
	while (itr) {
		ASSERT_AREA_LIST(itr);
		ASSERT_AREA(itr->area);

		next = itr->next;

		// attempt to free the current elemnt
		if (itr->area->ops->free(itr->area) == FAILURE)
			ret = FAILURE;

		// free the node
		ZERO(itr, sizeof(struct area_list));
		free(itr);

		itr = next;
	}

	return ret;
}

/**
 * creates a new `area_list` node
 *
 * @area - the area to put in the node
 *
 * RETURNS
 * 	the new node on success, NULL on failure
 *
 */
static struct area_list * area_list_new(struct doc_area * area) {
	ASSERT_AREA(area);

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
// String List
/////////////////////////////////////

/**
 * a recursive linked-list of strings
 *
 * @next - the next node in the list
 * @str - the current element
 *
 */
struct str_list {
	struct str_list * next;
	char str[];
};

#define ASSERT_STR_LIST(list) assert(list)

/**
 * recursively frees the entire list, starting from `head`
 *
 * @head - the first node of the list
 *
 */
static void str_list_free_all(struct str_list * head) {

	struct str_list * itr = head;
	struct str_list * next;

	// go through each node
	while (itr) {
		ASSERT_STR_LIST(itr);

		next = itr->next;

		// free the current node
		ZERO(itr, sizeof(struct str_list) + strlen(itr->str));
		free(itr);

		itr = next;
	}
}

/**
 * Creates a new str_list node with a copy of the string `str`.
 *
 * @str - the string to copy to the node
 * @len - the length of the string
 *
 * RETURNS
 * 	the new node on success, NULL on failure
 *
 */
static inline struct str_list * str_list_new(const char * str, size_t len) {
	assert(str);
	
	// Prevent overflow
	if (len > SIZE_MAX - 1 - sizeof(struct str_list)) {
		ERR("Token too big.");
		return NULL;
	}

	// create the list
	struct str_list * list = malloc(sizeof(struct str_list) + len + 1);
	if (!list) {
		LIBERRN();
		return NULL;
	}

	// intialize
	list->next = NULL;
	memcpy(list->str, str, len);
	list->str[len] = 0;

	return list;
}


/////////////////////////////////////
// String List Area
/////////////////////////////////////

/**
 * A helper `area` which efficiently maintains
 * a list of strings.
 *
 * @area - parent `area` struct
 * @head - the first node in the list
 * @tail_ptr - a pointer to the next available space
 * 	at the end of the list
 */
struct doc_area_str_list {
	struct doc_area area;
	struct str_list * head;
	struct str_list ** tail_ptr;
};

#define ASSERT_STR_LIST_AREA(list_area) do { \
	ASSERT_AREA((struct doc_area *)list_area); \
	assert(list_area->tail_ptr); \
} while(0)

static Status doc_area_str_list_free(struct doc_area_str_list * list_area) {
	ASSERT_STR_LIST_AREA(list_area);

	// free the contained str_list
	str_list_free_all(list_area->head);

	// free the list area
	ZERO(list_area, sizeof(struct doc_area_str_list));
	free(list_area);

	return SUCCESS;
}

/**
 * The deafult function for writing a `doc_area_str_list`.
 * Writes all the strings in the contained list, each followed by a space.
 *
 * If this implementation is not sufficient for a given purpose,
 * then a set of operations containing a different `write` function
 * should be supplied to the constructor (`doc_area_str_list_new()`).
 */
static Status doc_area_str_list_default_write(struct doc_area_str_list * list_area, FILE * file) {
	ASSERT_STR_LIST_AREA(list_area);
	ASSERT_FILE(file);

	struct str_list * itr = list_area->head;

	// go through each element
	while (itr) {

		// print the element, followed by a space
		if (fputs(itr->str, file) == EOF
				|| fputc(' ', file) == EOF) {
			WRITE_ERR();
			return FAILURE;
		}

		itr = itr->next;
	}

	return SUCCESS;
}

/**
 * The default set of operations for `doc_area_str_list`.
 * If these implementations are insufficient, a diferent set
 * may be provided to the constructor.
 */
const struct area_ops doc_area_str_list_default_ops = {
	.write = (Vft_write) doc_area_str_list_default_write,
	.free = (Vft_free) doc_area_str_list_free,
};

/**
 * Creates a new `doc_area_str_list` with VFT operations `ops`.
 * If the `doc_area_str_list_default_ops` are insufficient for any reason,
 * a different set of operations may be provided.
 *
 * @ops - the VFT operations for this `area`
 *
 * RETURNS
 * 	the `area` on success, NULL on failure
 */
static struct doc_area_str_list * doc_area_str_list_new(const struct area_ops * ops) {
	ASSERT_AREA_OPS(ops);

	// create the list area
	struct doc_area_str_list * list_area = malloc(sizeof(struct doc_area_str_list));
	if (!list_area) {
		LIBERRN();
		return NULL;
	}

	// initialize
	list_area->area.ops = ops;
	list_area->head = NULL;
	list_area->tail_ptr = &list_area->head;

	return list_area;
}

/**
 * Adds a copy of `str` to the end of the list of maintained strings.
 *
 * @str - the string to add to the list
 * @len - the length of the string
 *
 * RETURNS: Status
 */
static inline Status doc_area_str_list_add(struct doc_area_str_list * list_area,
		const char * str, size_t len) {
	ASSERT_STR_LIST_AREA(list_area);
	assert(str);

	// create a new list node with the given string
	struct str_list * list = str_list_new(str, len);
	if (!list)
		return FAILURE;

	// add the node to the list
	*list_area->tail_ptr = list;
	list_area->tail_ptr = &list->next;

	return SUCCESS;
}


////////////////////////////////////////////
// PG Area
////////////////////////////////////////////

/**
 * An `area` which forms a paragraph out of given words
 * and other areas.
 *
 * @area - the `area` parent struct
 * @head - the first element of the list containing all
 * 	the `area`s in the paragraph
 * @tail - the last element in the list
 */
struct doc_area_pg {
	struct doc_area area;
	struct area_list * head;
	struct area_list * tail;
};

#define ASSERT_PG_AREA(pg) do { \
	ASSERT_AREA(pg); \
	assert(!pg->head && !pg->tail || pg->head && pg->tail); \
} while (0)

Status doc_area_pg_free(struct doc_area_pg * pg) {
	ASSERT_PG_AREA(pg);

	// free the contained list
	Status ret = area_list_free_all(pg->head);

	// free the pg
	ZERO(pg, sizeof(struct doc_area_pg));
	free(pg);

	return ret;
}

static Status doc_area_pg_write(struct doc_area_pg * pg, FILE * file) {
	ASSERT_PG_AREA(pg);
	ASSERT_FILE(file);

	struct area_list * itr = pg->head;

	// start the pg
	if (fputs("<p>\n", file) == EOF) {
		WRITE_ERR();
		return FAILURE;
	}
	// for each element in the pg
	while (itr) {

		// write the element
		Status write_ret = itr->area->ops->write(itr->area, file);
		if (write_ret == FAILURE)
			return FAILURE;

		// follow with a space
		if (fputc(' ', file) == EOF) {
			WRITE_ERR();
			return FAILURE;
		}

		itr = itr->next;
	}

	// end the pg
	if (fputs("\n</p>\n", file) == EOF) {
		WRITE_ERR();
		return FAILURE;
	}

	return SUCCESS;
}

const struct area_ops doc_area_pg_ops = {
	.write = (Vft_write) doc_area_pg_write,
	.free = (Vft_free) doc_area_pg_free,
};

struct doc_area_pg * doc_area_pg_new() {

	// create the pg
	struct doc_area_pg * pg = malloc(sizeof(struct doc_area_pg));
	if (!pg) {
		LIBERRN();
		return NULL;
	}

	// initialize
	pg->area.ops = &doc_area_pg_ops;
	pg->head = NULL;
	pg->tail = NULL;

	return pg;
}

Status doc_area_pg_add_word(struct doc_area_pg * pg, const char * word, size_t len) {
	ASSERT_PG_AREA(pg);
	assert(word);
	if (pg->tail) {
		ASSERT_AREA_LIST(pg->tail);
		ASSERT_AREA(pg->tail->area);
	}

	// check if last element not a text list area
	if (!pg->tail || pg->tail->area->ops != &doc_area_str_list_default_ops) {

		// create a new text list area
		struct doc_area * area = (struct doc_area *) doc_area_str_list_new(&doc_area_str_list_default_ops);
		if (!area)
			return FAILURE;

		// put it in a node
		struct area_list * node = area_list_new(area);
		if (!node)
			return FAILURE;

		// if pg is empty, set as head and tail
		if (!pg->tail) {
			pg->head = node;
			pg->tail = node;
		}
		// else, add to end of list
		else {
			pg->tail->next = node;
			pg->tail = node;
		}
	}

	// copy the word to the terminating text list area
	if (doc_area_str_list_add((struct doc_area_str_list *)pg->tail->area, word, len) == FAILURE)
		return FAILURE;

	return SUCCESS;
}

