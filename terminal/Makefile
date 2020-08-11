terminal: terminal.c ../ctx.h ../ctx.o vt.c vt.h vt-line.c vt-line.h Makefile
	ccache $(CC) ../ctx.o vt.c vt-line.c terminal.c -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall -march=native -O2 -lutil -lz -g -I../deps -Wno-nonnull
terminal-static: terminal.c ../ctx.h ../ctx.o vt.c vt.h vt-line.c vt-line.h Makefile
	ccache $(CC) ../ctx.o vt.c vt-line.c terminal.c -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs --static sdl2` -lm -Wall -march=native -O2 -lutil -lz -g -I../deps -Wno-nonnull -static

all:
	make -C  ..
clean:
	rm -f terminal
	make -C  .. clean
