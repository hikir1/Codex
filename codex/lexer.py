import sys
import ply.lex

tokens = (
	"CHAR",
	"SPACE",
	"ENDPARA",
)

t_CHAR = r"."

def t_ENDPARA(t):
	r"[ ]*\n[ ]*\n[ ]*(\n[ ]*)*"
	t.value = None
	return t

def t_SPACE(t):
	r"[ \n]+"
	t.value = None
	return t

def t_error(t):
	print(f"ERROR: Illegal character 't.value'")
	sys.exit(1)

def new_lexer():
	return ply.lex.lex()

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print(f"Usage: {sys.argv[0]} INPUT")
		sys.exit(1)
	lexer = new_lexer()
	lexer.input(sys.argv[1])
	for tok in lexer:
		print(tok)
