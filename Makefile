codex: *.c *.h
	gcc -o codex -O3 *.c

test:
	gcc -o codex -g -Wall -Werror -DPTEST *.c && ./codex tests/test.txt && vim tests/test.html

clean:
	rm codex
