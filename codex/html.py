def compile(codex, f):
	f.write('<!DOCTYPE html><html><head></head><body>')
	for block in codex:
		assert isinstance(block, codex_parser.Paragraph), f"Unknown block type: {type(block)}"
		write_paragraph(block, f)
	f.write('</body></html>')

def write_paragraph(paragraph, f):
	assert len(paragraph) > 0
	f.write('<p>')
	write_word(paragraph[0], f)
	for word in paragraph[1:]:
		f.write(' ')
		write_word(word, f)
	f.write('</p>')

def write_word(word, f):
	for c in word:
		if c == '<':
			f.write('&gt;')
		elif c == '>':
			f.write('&lt;')
		elif c == '&':
			f.write('&amp;')
		else:
			f.write(c)

if __name__ == "__main__":
	import sys
	import lexer as codex_lexer
	import parser as codex_parser
	if len(sys.argv) != 2:
		print(f"Usage: {sys.argv[0]} INPUT")
		sys.exit(1)
	lexer = codex_lexer.new_lexer()
	parser = codex_parser.new_parser()
	codex = parser.parse(sys.argv[1])
	with open("test.html", "w") as f:
		compile(codex, f)
else:
	import codex.parser as codex_parser
