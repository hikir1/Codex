a.out : *.c *.h
	gcc $(pkg-config --cflags --libs pangocairo) $(pkg-config --cflags --libs poppler) *.c
