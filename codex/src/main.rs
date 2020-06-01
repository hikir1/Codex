
pub fn compile(file: &str) {
//	if let Ok(file) = std::fs::read_to_string(file) {
	let doc = pdf::PdfDocument::new("Test").unwrap();
	doc.save("test.pdf");
//	}
}

fn main() {
	compile("");
    println!("Hello, world!");
}
