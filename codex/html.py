def compile(codex, f):
	context = codex_lexer.Context()
	f.write('<!DOCTYPE html><html><head></head><body>')
	for block in codex:
		assert isinstance(block, codex_parser.Paragraph), f"Unknown block type: {type(block)}"
		write_paragraph(block, f, context)
	f.write('</body></html>')

def write_paragraph(paragraph, f, context):
	assert len(paragraph) > 0
	f.write('<p>')
	write_word(paragraph[0], f, context)
	for word in paragraph[1:]:
		f.write(' ')
		write_word(word, f, context)
	f.write('</p>')

def write_word(word, f, context):
	for c in word:
		if isinstance(c, codex_lexer.Tag):
			write_tag(c, f, context)
		elif c == '<':
			f.write('&lt;')
		elif c == '>':
			f.write('&gt;')
		elif c == '&':
			f.write('&amp;')
		else:
			f.write(c)

def write_tag(tag, f, context):
	if tag is codex_lexer.Tag.EMPH:
		if context[codex_lexer.Tag.EMPH]:
			f.write("</em>")
		else:
			f.write("<em>")
		context.toggle(codex_lexer.Tag.EMPH)
	elif tag is codex_lexer.Tag.STRONG:
		if context[codex_lexer.Tag.STRONG]:
			f.write("</strong>")
		else:
			f.write("<strong>")
		context.toggle(codex_lexer.Tag.STRONG)
	else:
		print(f"ERROR: Unknown tag: {tag}")

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
	import codex.lexer as codex_lexer
	import codex.parser as codex_parser
