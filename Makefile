a.out : *.c
	gcc *.c `pkg-config --cflags --libs pangocairo`
