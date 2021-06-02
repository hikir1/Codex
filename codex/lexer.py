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
	STRONG = r"\*\*"
	EMPH = r"\*"

class Context(dict):
	def __init__(self):
		super().__init__({
				Tag.EMPH: 0,
				Tag.STRONG: 0,
		})
	def toggle(self, key, lineno):
		self[key] = lineno if self[key] == 0 else 0

t_CHAR = r"."

def t_INVALID(t):
	r"\*{4,}"
	print(f"ERROR line {t.lexer.lineno}: Invalid sequence '{t.value}'")
	t.lexer.errors += 1

@TOKEN(Tag.STRONG.value)
def t_STRONG(t):
	t.lexer.context.toggle(Tag.STRONG, t.lexer.lineno)
	t.value = None
	return t

@TOKEN(Tag.EMPH.value)
def t_EMPH(t):
	t.lexer.context.toggle(Tag.EMPH, t.lexer.lineno)
	t.value = None
	return t

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
	for key, val in t.lexer.context.items():
		if val != 0:
			print(f"ERROR: Unmatched {key.value} tag on line {val}")
			t.lexer.errors += 1
	if t.lexer.errors != 0:
		print(f"*** {t.lexer.errors} errors detected ***")
		sys.exit(1)

def new_lexer():
	lexer = ply.lex.lex()
	lexer.context = Context()
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
