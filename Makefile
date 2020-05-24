all: pre ctx-tiny ctx post ctx-tiny ctx-tiny-static-musl
pre:
	make -C fonts
post:
	make -C examples

test-renderpaths: test-renderpaths.c ctx.h fonts/*.h Makefile
	gcc $< -o $@ -g -ofast-math -O3 -march=native -mtune=native `pkg-config mrg --cflags --libs` -Ifonts 

clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1 ctx-tiny ctx-tiny-static-musl
	make -C fonts clean

ctx: ctx.c vt/*.[ch] ctx.h  Makefile
	gcc ctx.c vt/*.c -o $@ -g -I. -Ifonts -Ivt -mtune=native -march=native `pkg-config mmm sdl2 --cflags --libs` -lutil -Wall  -lz -lm

ctx.O1: ctx.c vt/*.[ch] ctx.h  Makefile
	gcc ctx.c vt/*.c -o $@ -g -O1 -I. -Ifonts -Ivt `pkg-config mmm --cflags --libs` -lutil -Wall -lz -lm
ctx.asan: ctx.c vt/*.[ch] ctx.h Makefile
	gcc -DASANBUILD=1 ctx.c vt/*.c -o $@ -g -O0 -I. -Ifonts -Ivt `pkg-config mmm --cflags --libs sdl2` -lutil -lasan -fsanitize=address -lz -lm

ctx-tiny: ctx-tiny.c ctx.h fonts/*.h Makefile
	$(CC) $< -o $@ -I. -Ifonts -Os
	strip -s -x $@
	ls -sh $@

ctx-tiny-static-musl: ctx-tiny.c ctx.h fonts/*.h Makefile
	musl-gcc $< -o $@ -I. -Ifonts -Os -static
	strip -s -x $@
	ls -sh $@
