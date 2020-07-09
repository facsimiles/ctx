all: pre ctx post
pre:
	make -C tools
	make -C fonts
post:
	make -C tests


clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1
	rm -f tests/index.html
	make -C fonts clean
	make -C examples clean

CFLAGS= -g -march=native -Wno-array-bounds #-fanalyzer
#CFLAGS=-Os -flto

ctx: ctx.c vt/*.[ch] ctx.h  Makefile svg.h
	ccache $(CC) ctx.c vt/*.c -o $@ $(CFLAGS) -I. -Ifonts -Ivt `pkg-config babl sdl2 --cflags --libs` -lutil -Wall  -lz -lm -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers 

ctx.O1: ctx.c vt/*.[ch] ctx.h  Makefile 
	$(CC) ctx.c vt/*.c -o $@ -g -O1 -I. -Ifonts -Ivt `pkg-config babl sdl2 --cflags --libs` -lutil -Wall -lz -lm
ctx.asan: ctx.c vt/*.[ch] ctx.h Makefile
	$(CC) -DASANBUILD=1 ctx.c vt/*.c -o $@ -g -O0 -I. -Ifonts -Ivt `pkg-config --cflags --libs babl sdl2` -lutil -lasan -fsanitize=address -lz -lm -march=native

sentry:
	sentry Makefile ctx.h tests/*.ctx -- make ctx  post
sentry-f:
	sentry Makefile ctx.h tests/*.ctx -- make
ctx.h.html: ctx.h Makefile
	highlight -l -a --encoding=utf8 -W ctx.h > ctx.h.html
ctx-font-regular.h.html: fonts/ctx-font-regular.h Makefile
	highlight -l -a --encoding=utf8 -W fonts/ctx-font-regular.h > ctx-font-regular.h.html

updateweb: clean all post ctx.h.html ctx-font-regular.h.html
	cat tests/index.html | sed 's/.*script.*//' > tmp
	mv tmp tests/index.html
	git gc
	git update-server-info
	rm -rf /home/pippin/pgo/ctx.graphics/.git || true
	cp -Rv .git /home/pippin/pgo/ctx.graphics/.git
	cp -R tests/* ~/pgo/ctx.graphics/tests
	cp index.html highlight.css ctx.h.html ctx-font-regular.h.html ctx.h fonts/ctx-font-regular.h ~/pgo/ctx.graphics/
