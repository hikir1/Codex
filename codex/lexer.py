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

class Tag(enum.Enum):
	STRONG = "**"
	EMPH = "*"

t_CHAR = r"."

def t_INVALID(t):
	r"\*{4,}"
	print(f"ERROR line {t.lexer.lineno}: Invalid sequence '{t.value}'")
	t.lexer.errors += 1

def lex_tag(t, tag):
	try:
		t.lexer.tag_linenos.pop(tag)
		if t.lexer.tag_stack[-1] != tag:
			print(f"ERROR line {t.lexer.lineno}: Expected"
					+ f"{t.lexer.tag_stack[-1].value} before {tag.value}")
			t.lexer.errors += 1
			t.lexer.tag_stack.remove(tag)
		else:
			t.lexer.tag_stack.pop()
	except KeyError:
		t.lexer.tag_stack.append(tag)
		t.lexer.tag_linenos[tag] = t.lexer.lineno
	t.value = None
	return t

def t_STRONG(t):
	r"\*\*"
	return lex_tag(t, Tag.STRONG)

def t_EMPH(t):
	r"\*"
	return lex_tag(t, Tag.EMPH)

def check_tags(t):
	for tag in t.lexer.tag_stack:
		print(f"ERROR: Unmatched {tag.value} tag on line {t.lexer.tag_linenos[tag]}")
		t.lexer.errors += 1
	t.lexer.tag_stack.clear()
	t.lexer.tag_linenos.clear()

def t_ENDPARA(t):
	r"[ ]*\n[ ]*\n[ ]*(\n[ ]*)*"
	check_tags(t)
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
	check_tags(t)
	if t.lexer.errors != 0:
		print(f"*** {t.lexer.errors} errors detected ***")
		sys.exit(1)

def new_lexer():
	lexer = ply.lex.lex()
	lexer.tag_stack = []
	lexer.tag_linenos = {}
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
