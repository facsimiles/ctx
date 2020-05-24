all: pre ctx post
pre:
	make -C fonts
post:
	make -C tests


clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1
	make -C fonts clean
	make -C examples clean

ctx: ctx.c vt/*.[ch] ctx.h  Makefile
	gcc ctx.c vt/*.c -o $@ -g -I. -Ifonts -Ivt -mtune=native -march=native `pkg-config sdl2 --cflags --libs` -lutil -Wall  -lz -lm

ctx.O1: ctx.c vt/*.[ch] ctx.h  Makefile
	gcc ctx.c vt/*.c -o $@ -g -O1 -I. -Ifonts -Ivt `pkg-config sdl2 --cflags --libs` -lutil -Wall -lz -lm
ctx.asan: ctx.c vt/*.[ch] ctx.h Makefile
	gcc -DASANBUILD=1 ctx.c vt/*.c -o $@ -g -O0 -I. -Ifonts -Ivt `pkg-config --cflags --libs sdl2` -lutil -lasan -fsanitize=address -lz -lm

