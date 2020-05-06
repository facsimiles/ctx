all: check ctxvt
check:
	make -C fonts
	make -C tests check

test-renderpaths: test-renderpaths.c ctx.h fonts/*.h Makefile
	gcc $< -o $@ -g -O2 `pkg-config mrg --cflags --libs` -Ifonts 

clean:
	rm -f test-renderpaths ctxvt ctxvt.asan ctxvt.O1
	make -C tests clean
	make -C fonts clean

ctxvt: ctx.c vt/*.[ch] ctx.h  Makefile
	gcc ctx.c vt/*.c -o $@ -g -I. -Ifonts -Ivt `pkg-config mmm sdl2 --cflags --libs` -lutil -Wall  -lz -lm

ctxvt.O1: ctx.c vt/*.[ch] ctx.h  Makefile
	gcc ctx.c vt/*.c -o $@ -g -O1 -I. -Ifonts -Ivt `pkg-config mmm --cflags --libs` -lutil -Wall -lz -lm
ctxvt.asan: ctx.c vt/*.[ch] ctx.h Makefile
	gcc -DASANBUILD=1 ctx.c vt/*.c -o $@ -g -O0 -I. -Ifonts -Ivt `pkg-config mmm --cflags --libs sdl2` -lutil -lasan -fsanitize=address -lz -lm
