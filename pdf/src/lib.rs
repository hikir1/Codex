use printpdf::{PdfDocumentReference, IndirectFontRef, BuiltinFont, Mm, Error};
use printpdf::indices::{PdfPageIndex, PdfLayerIndex};
use std::io::BufWriter;
use std::fs::File;

pub type Font = IndirectFontRef;

const DEFAULT_PAGE_WIDTH: Mm = Mm(215.9); // 8.5"
const DEFAULT_PAGE_HEIGHT: Mm = Mm(279.4); // 11"
const DEFAULT_FONT: BuiltinFont = BuiltinFont::TimesRoman;
const DEFAULT_FONT_SIZE: i64 = 12;
const DEFAULT_MARGIN: Mm = Mm(25.4); // 1"
const DEFAULT_LINE_MARGIN: Mm = Mm(0.5); // ?

struct Point {
	x: Mm,
	y: Mm,
}

pub struct PdfDocument<'a> {
	doc: PdfDocumentReference,
	page: PdfPageIndex,
	layer: PdfLayerIndex,
	layer_cnt: u32,
	title: &'a str,
	font: Font,
	font_size: i64,
	line_margin: Mm,
	cursor: Point,
	cur_line: String,
}

impl <'a> PdfDocument<'a> {
	pub fn new(title: &'a str) -> Result<PdfDocument, Error> {
		let (doc, page, layer) = printpdf::PdfDocument::new(title, DEFAULT_PAGE_WIDTH, DEFAULT_PAGE_HEIGHT, "Layer 1");
		let font = doc.add_builtin_font(DEFAULT_FONT)?;
		Ok(Self {
			doc,
			page,
			layer,
			layer_cnt: 1,
			title,
			font,
			font_size: DEFAULT_FONT_SIZE,
			line_margin: DEFAULT_LINE_MARGIN,
			cursor: Point{ x: DEFAULT_MARGIN, y: DEFAULT_MARGIN },
			cur_line: String::new()
		})
	}
	
	pub fn save(self, dest: &str) -> Result<(), Box<dyn std::error::Error>> {
		let file = std::fs::File::create(dest)?;
		let layer = self.doc.get_page(self.page).get_layer(self.layer);
		layer.begin_text_section();
		layer.set_font(&self.font, self.font_size);
		layer.set_text_cursor(self.cursor.x * 2.0, self.cursor.y * 10.0);
		layer.set_line_height(33);
		layer.set_word_spacing(1);
		layer.write_text("test test test test tes test test test test test test ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt aa", &self.font);
		layer.end_text_section();
		self.doc.save(&mut std::io::BufWriter::new(file))?;
		Ok(())
	}
}