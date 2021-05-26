switch_text = {
	"CHAR": write_char,
	"SPACE": write_space,
}


def compile(lexer):
	for tok in lexer:
		
