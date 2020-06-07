all: pre ctx post
pre:
	make -C tools
	make -C fonts
post:
	make -C tests


clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1
	make -C fonts clean
	make -C examples clean

CFLAGS=-g
#CFLAGS=-Os -flto

ctx: ctx.c vt/*.[ch] ctx.h  Makefile svg.h nct.h
	$(CC) ctx.c vt/*.c -o $@ $(CFLAGS) -I. -Ifonts -Ivt `pkg-config sdl2 --cflags --libs` -lutil -Wall  -lz -lm -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers

ctx.O1: ctx.c vt/*.[ch] ctx.h  Makefile 
	$(CC) ctx.c vt/*.c -o $@ -g -O1 -I. -Ifonts -Ivt `pkg-config sdl2 --cflags --libs` -lutil -Wall -lz -lm
ctx.asan: ctx.c vt/*.[ch] ctx.h Makefile
	$(CC) -DASANBUILD=1 ctx.c vt/*.c -o $@ -g -O0 -I. -Ifonts -Ivt `pkg-config --cflags --libs sdl2` -lutil -lasan -fsanitize=address -lz -lm

