import ply.yacc
import sys
import lexer as codex_lexer
from lexer import Tag

class Codex(list):
	pass

class Paragraph(list):
	pass

class Word(list):
	pass

def p_prepend(p):
	'''codex          : paragraph codex
	   paragraph      : word paragraph
	   last_paragraph : word last_paragraph
	   word           : CHAR word
	   last_word      : CHAR last_word'''
	if len(p[1]) != 0:
		p[2].insert(0, p[1])
	p[0] = p[2]

def p_emph(p):
	'''word      : EMPH word
	   last_word : EMPH last_word'''
	p[2].insert(0, Tag.EMPH)
	p[0] = p[2]

def p_strong(p):
	'''word      : STRONG word
	   last_word : STRONG last_word'''
	p[2].insert(0, Tag.STRONG)
	p[0] = p[2]

def p_codex_end(p):
	'codex : last_paragraph'
	p[0] = Codex()
	if len(p[1]) != 0:
		p[0].append(p[1])

def p_paragraph_end(p):
	'''paragraph      : last_word ENDPARA
	   last_paragraph : last_word'''
	p[0] = Paragraph()
	if len(p[1]) != 0:
		p[0].append(p[1])

def p_word_end(p):
	'''word      : SPACE
	   last_word :'''
	p[0] = Word()

def p_error(p):
	print("SYNTAX ERROR!")

def new_parser():
	parser = ply.yacc.yacc()
	return parser

if __name__ == '__main__':
	from lexer import tokens
	if len(sys.argv) != 2:
		print(f"Usage: {sys.argv[0]} INPUT")
		sys.exit(1)
	lexer = codex_lexer.new_lexer()
	parser = new_parser()
	print(parser.parse(sys.argv[1]))
else:
	from lexer import tokens
