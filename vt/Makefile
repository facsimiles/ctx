termctx: term.c ../ctx.h vt.c vt.h vt-line.c vt-line.h Makefile
	ccache $(CC)  vt.c vt-line.c term.c -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall -march=native -O2 -lutil -lz -g

all:
	make -C  ..
clean:
	rm -f term
	make -C  .. clean
