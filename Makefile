all: check ctx test-renderpaths
check:
	make -C fonts
	make -C tests check

test-renderpaths: test-renderpaths.c ctx.h fonts/*.h Makefile
	gcc $< -o $@ -g -O2 `pkg-config mrg --cflags --libs` -Ifonts 

clean:
	rm -f test-renderpaths ctx ctx.asan
	make -C tests clean
	make -C fonts clean

ctx: ctx.c vt/*.c ctx.h  Makefile
	gcc ctx.c vt/*.c -o $@ -g -O3 -I. -Ifonts -Ivt `pkg-config mmm --cflags --libs` -lutil -Wall 
	strip $@
	ls -la $@
ctx.asan: ctx.c vt/*.c ctx.h Makefile
	gcc ctx.c vt/*.c -o $@ -g -O0 -I. -Ifonts -Ivt `pkg-config mmm --cflags --libs` -lutil -lasan -fsanitize=address
