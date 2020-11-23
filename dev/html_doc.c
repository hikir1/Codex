#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "util.h"
#include "doc.h"

#define WRITE_ERR() ERR("Failed to write to file")
#define CLOSE_ERR() LIBERR("Failed to close output file")

const char * const doc_suffix = ".html";

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
	Status (*write)(struct area * self, FILE * file);

 	/**
	 * prepares the `area` and its contents to be released, then frees it
	 *
	 * @self - the `area` that is to do the freeing
	 *
	 * RETURNS: Status
	 *
	 */
	Status (*free)(struct area * self);
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
struct area {
	struct area_ops * ops;
};

#define ASSERT_AREA(area) do { \
	assert(area); \
	ASSERT_AREA_OPS(area->ops); \
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

Status doc_add(struct doc * doc, struct area * area) {
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

Status doc_area_free(struct area * area) {
	ASSERT_AREA(area);

	return area->ops->free(area);
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
	struct area * area;
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
static struct area_list * area_list_new(struct area * area) {
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
		ZERO(itr, sizeof(struct str_list) + strlen(itr->text));
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
	memcpy(list->text, text, len);
	list->text[len] = 0;

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
struct str_list_area {
	struct area area;
	struct str_list * head;
	struct str_list ** tail_ptr;
};

#define ASSERT_STR_LIST_AREA(list_area) do { \
	ASSERT_AREA((struct area *)list_area); \
	assert(list_area->tail_ptr); \
} while(0)

static Status str_list_area_free(struct str_list_area * list_area) {
	ASSERT_STR_LIST_AREA(list_area);

	// free the contained str_list
	str_list_free_all(list_area->head);

	// free the list area
	ZERO(list_area, sizeof(struct str_list_area);
	free(list_area);

	return SUCCESS;
}

/**
 * The deafult function for writing a `str_list_area`.
 * Writes all the strings in the contained list, each followed by a space.
 *
 * If this implementation is not sufficient for a given purpose,
 * then a set of operations containing a different `write` function
 * should be supplied to the constructor (`str_list_area_new()`).
 */
static Status str_list_area_default_write(struct str_list_area * list_area, FILE * file) {
	ASSERT_STR_LIST_AREA(list_area);
	ASSERT_FILE(file);

	struct text_list * itr = list_area->head;

	// go through each element
	while (itr) {

		// print the element, followed by a space
		if (fputs(itr->text, file) == EOF
				|| fputc(' ', file) == EOF) {
			WRITE_ERR();
			return FAILURE;
		}

		itr = itr->next;
	}

	return SUCCESS;
}

/**
 * The default set of operations for `str_list_area`.
 * If these implementations are insufficient, a diferent set
 * may be provided to the constructor.
 */
const struct area_ops str_list_area_default_ops = {
	.write = str_list_area_default_write,
	.free = str_list_area_free,
};

/**
 * Creates a new `str_list_area` with VFT operations `ops`.
 * If the `str_list_area_default_ops` are insufficient for any reason,
 * a different set of operations may be provided.
 *
 * @ops - the VFT operations for this `area`
 *
 * RETURNS
 * 	the `area` on success, NULL on failure
 */
static struct str_list_area * str_list_area_new(struct area_ops * ops) {
	ASSERT_AREA_OPS(ops);

	// create the list area
	struct str_list_area * list_area = malloc(sizeof(struct str_list_area));
	if (!list_area) {
		LIBERRN();
		return NULL;
	}

	// initialize
	list_area->area.ops = ops;
	list_area->head = NULL;
	list_area->tail_ptr = &list->head;

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
static inline Status str_list_area_add(struct str_list_area * list_area,
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
struct pg_area {
	struct area area;
	struct area_list * head;
	struct area_list * tail;
};

#define ASSERT_PG_AREA(pg) do { \
	ASSERT_AREA((struct area *)pg); \
	assert(!head && !tail || head && tail); \
} while (0)

static Status pg_area_free(struct pg_area * pg) {
	ASSERT_PG_AREA(pg);

	// free the contained list
	Status ret = area_list_free_all(pg->head);

	// free the pg
	ZERO(pg, sizeof(struct pg_area));
	free(pg);

	return ret;
}

static Status pg_area_write(struct pg_area * pg, FILE * file) {
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

const struct area_ops pg_area_ops = {
	.write = pg_area_write,
	.free = pg_area_free,
}

struct pg_area * doc_area_pg_new() {

	// create the pg
	struct pg_area * pg = malloc(sizeof(struct pg_area));
	if (!pg) {
		LIBERRN();
		return NULL;
	}

	// initialize
	pg->area.ops = pg_area_ops;
	pg->head = NULL;
	pg->tail = NULL;

	return pg;
}

Status doc_area_pg_add_word(struct pg_area * pg, const char * word, size_t len) {
	ASSERT_PG_AREA(pg);
	assert(word);

	// check if last element not a text list area
	if (!pg->tail || pg->tail->ops != str_list_area_ops) {

		// create a new text list area
		struct str_list_area * list_area = str_list_area_new(str_list_area_default_ops);
		if (!list_area)
			return FAILURE;

		// if pg is empty, set as head and tail
		if (!pg->tail) {
			pg->head = list_area;
			pg->tail = list_area;
		}
		// else, add to end of list
		else {
			pg->tail->next = list_area;
			pg->tail = list_area;
		}
	}

	// copy the word to the terminating text list area
	if (str_list_area_add(pg->tail, word, len) == FAILURE)
		return FAILURE;

	return SUCCESS;
}

