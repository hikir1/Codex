#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <pango/pangocairo.h>

// 1 in = 72 point

int main() {
	cairo_surface_t * surf = cairo_pdf_surface_create("Test.pdf", 612, 792); // 8.5 x 11 in
	cairo_t * cairo = cairo_create(surf);

	PangoFontDescription * fontdesc = pango_font_description_new();
	pango_font_description_set_family(fontdesc, "Latin Modern Math");
	// pango_font_description_set_weight(fontdesc, PANGO_WEIGHT_BOLD);
	pango_font_description_set_absolute_size(fontdesc, 12 * PANGO_SCALE);

	PangoLayout * layout = pango_cairo_create_layout(cairo);
	pango_layout_set_font_description(layout, fontdesc);
	pango_layout_set_text(layout, "Hello,\U0001d4b6 world!!! \u2112f(x) = a + yb\n\U0001d465", -1);

	cairo_set_source_rgb(cairo, 0.0, 0.0, 0.0);
	cairo_move_to(cairo, 72.0, 72.0);
	pango_cairo_show_layout(cairo, layout);

	g_object_unref(layout);
	pango_font_description_free(fontdesc);

	cairo_destroy(cairo);
	cairo_surface_destroy(surf);
}
