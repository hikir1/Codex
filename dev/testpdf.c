#include "pdfproxy.h"

int test_pdfa_init(struct pdfarea * area) {
	assert(area);

	// TODO

int test_pdf_open(FILE ** pdf, const char * name) {
	assert(pdf);
	assert(name);

	FILE * file = fopen(name, "w");
	if (!file) {
		LIBERR("Failed to create file %s", name);
		return FAILURE;
	}
	*pdf = file;
	return SUCCESS;
}

int test_pdf_push(FILE ** pdf, struct pdfarea * area) {
	assert(pdf);
	assert(*pdf);
	assert(!ferror(*pdf));
	assert(area);
	assert(area->buf);

	if (fwrite(area->buf, 1, area->len, area->buf, pdf) != area->len) {
		ERR("Failed to write to pdf file");
		return FAILURE;
	}
	return SUCCESS;
}

int test_pdf_close(FILE ** pdf) {
	assert(pdf);
	assert(*pdf);
	
	if (fclose(*pdf) == -1) {
		LIBERR("Failed to close pdf file");
		return FAILURE;
	}
	return SUCCESS;
}
