def compile(codex, f):
	f.write('<!DOCTYPE html><html><head></head><body>')
	for block in codex:
		assert isinstance(block, codex_parser.Paragraph), f"Unknown block type: {type(block)}"
		write_paragraph(block, f)
	f.write('</body></html>')

def write_paragraph(paragraph, f):
	assert len(paragraph) > 0
	f.write('<p>')
	tag_stack = []
	write_word(paragraph[0], f, tag_stack, no_space=True)
	for word in paragraph[1:]:
		write_word(word, f, tag_stack, no_space=False)
	f.write('</p>')

def write_word(word, f, tag_stack, no_space):
	assert len(word) > 0
	if not no_space and not isinstance(word[0], codex_lexer.Tag):
		f.write(' ')
	for c in word:
		if isinstance(c, codex_lexer.Tag):
			write_tag(c, f, tag_stack)
		elif c == '<':
			f.write('&lt;')
		elif c == '>':
			f.write('&gt;')
		elif c == '&':
			f.write('&amp;')
		else:
			f.write(c)

def write_tag(tag, f, tag_stack):
	Tag = codex_lexer.Tag
	TAG_LABELS = {
		Tag.EMPH: "em",
		Tag.STRONG: "strong",
	}
	assert tag in TAG_LABELS
	if len(tag_stack) != 0 and tag_stack[-1] == tag:
		tag_stack.pop()
		f.write(f"</{TAG_LABELS[tag]}>")
	else:
		tag_stack.append(tag)
		f.write(f"<{TAG_LABELS[tag]}>")

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
