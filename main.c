#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <pango/pangocairo.h>

int main() {
	cairo_surface_t * surf = cairo_pdf_surface_create("Test.pdf", 792, 612); // 11 x 8.5 in
	cairo_t * cairo = cairo_create(surf);

	PangoFontDescription * fontdesc = pango_font_description_new();
	pango_font_description_set_family(fontdesc, "serif");
	pango_font_description_set_weight(fontdesc, PANGO_WEIGHT_BOLD);
	pango_font_description_set_absolute_size(fontdesc, 32 * PANGO_SCALE);

	PangoLayout * layout = pango_cairo_create_layout(cairo);
	pango_layout_set_font_description(layout, fontdesc);
	pango_layout_set_text(layout, "Hello, world", -1);

	cairo_set_source_rgb(cairo, 0, 0, 1.0);
	cairo_move_to(cairo, 10.0, 50.0);
	pango_cairo_show_layout(cairo, layout);

	g_object_unref(layout);
	pango_font_description_free(fontdesc);

	cairo_destroy(cairo);
	cairo_surface_destroy(surf);
}
