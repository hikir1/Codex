def compile(codex, f):
	f.write('<!DOCTYPE html><html><head></head><body>')
	for block in codex:
		assert isinstance(block, Paragraph), f"Unknown block type: {type(block)}"
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
		else f.write(c)
