import sys
import ply.lex
from ply.lex import TOKEN
import enum

tokens = (
	"CHAR",
	"SPACE",
	"ENDPARA",
	"EMPH",
	"STRONG",
	"INVALID",
)

class Tag:
	STRONG = "**"
	EMPH = "*"
	def __init__(self, val, lineno):
		self.val = val
		self.lineno = lineno
	def __eq__(self, other):
		return self.val == other

t_CHAR = r"."

def t_INVALID(t):
	r"\*{4,}"
	print(f"ERROR line {t.lexer.lineno}: Invalid sequence '{t.value}'")
	t.lexer.errors += 1

def lex_tag(t, tag):
	if tag in t.lexer.open_tags:
		if t.lexer.open_tags[-1] != tag:
			print(f"ERROR line {t.lexer.lineno}: Expected"
					+ f"{t.lexer.open_tags[-1].value} before {tag.value}")
			t.lexer.errors += 1
		t.lexer.open_tags.remove(tag)
	else:
		t.lexer.open_tags.append(Tag(tag, t.lineno))
	t.value = None
	return t

def t_STRONG(t):
	r"\*\*"
	return lex_tag(t, Tag.STRONG)

def t_EMPH(t):
	r"\*"
	return lex_tag(t, Tag.EMPH)

def t_ENDPARA(t):
	r"[ ]*\n[ ]*\n[ ]*(\n[ ]*)*"
	t.lexer.lineno += t.value.count("\n")
	t.value = None
	return t

def t_SPACE(t):
	r"[ \n]+"
	t.lexer.lineno += t.value.count("\n")
	t.value = None
	return t

def t_error(t):
	print(f"ERROR line {t.lexer.lineno}: Illegal character '{t.value}'")
	t.lexer.errors += 1

def t_eof(t):
	for tag in t.lexer.open_tags:
		print(f"ERROR: Unmatched {tag.val} tag on line {tag.lineno}")
		t.lexer.errors += 1
	if t.lexer.errors != 0:
		print(f"*** {t.lexer.errors} errors detected ***")
		sys.exit(1)

def new_lexer():
	lexer = ply.lex.lex()
	lexer.open_tags = []
	lexer.errors = 0
	return lexer

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print(f"Usage: {sys.argv[0]} INPUT")
		sys.exit(1)
	lexer = new_lexer()
	lexer.input(sys.argv[1])
	for tok in lexer:
		print(tok)
