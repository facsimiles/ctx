all: ctx-tiny ctx-small sdl2 mmm test-renderpaths ctx-tiny ctx-tiny-static-musl  ctx-small-static-musl
clean:
	rm -rf mmm sdl2 ctx-tiny ctx-tiny-static-musl test-renderpaths ctx-small ctx-small-static-musl
mmm: mmm.c ../ctx.h
	$(CC) $< -o $@ -I.. -I../fonts `pkg-config --cflags --libs mmm` -Wall
sdl2: sdl2.c ../ctx.h
	$(CC) $< -o $@ -I.. -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall
test-renderpaths: test-renderpaths.c ../ctx.h ../fonts/*.h
	gcc $< -o $@ -g -O3 -march=native -mtune=native `pkg-config mrg --cflags --libs` -I../fonts  -I..

ctx-tiny: ctx-tiny.c ../ctx.h ../fonts/*.h Makefile
	$(CC) $< -o $@ -I.. -I../fonts -Os
	strip -s -x $@
	ls -slab $@

ctx-tiny-static-musl: ctx-tiny.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $< -o $@ -I.. -I../fonts -Os -static
	strip -s -x $@
	ls -slab $@

ctx-small: ctx-small.c ../ctx.h ../fonts/*.h Makefile
	$(CC) $< -o $@ -I.. -I../fonts -Os
	strip -s -x $@
	ls -slab $@

ctx-small-static-musl: ctx-small.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $< -o $@ -I.. -I../fonts -Os -static
	strip -s -x $@
	ls -slab $@
