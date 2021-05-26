import ply.yacc
from lexer import tokens
import lexer as codex_lexer
import sys

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
	p[2].insert(0, p[1])
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

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print(f"Usage: {sys.argv[0]} INPUT")
		sys.exit(1)
	lexer = codex_lexer.new_lexer()
	parser = ply.yacc.yacc()
	print(parser.parse(sys.argv[1]))
