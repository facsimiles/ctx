all: pre ctx.o ctx 
pre:
	make -C tools
	make -C fonts
test: all
	make -C tests
post: test
clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1
	rm -f tests/index.html
	make -C fonts clean
	make -C examples clean

CFLAGS= -O3 -g -march=native -Wno-array-bounds 
#CFLAGS=-Os -flto

ctx.o: ctx-lib.c ctx.h Makefile
	$(CC) ctx-lib.c -c -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps

libctx.o: ctx-lib.c ctx.h Makefile
	$(CC) ctx-lib.c -shared -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps

ctx: ctx.c ctx.h  Makefile svg.h
	$(CC) ctx.c terminal/*.c -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps

ctx.O1: ctx.c ctx.h  Makefile svg.h
	$(CC) ctx.c -o $@ -g -O1 -I. -Ifonts `pkg-config sdl2 --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps -march=native

ctx.asan: ctx.c ctx.h Makefile
	$(CC) -DASANBUILD=1 ctx.c -o $@ -g -O0 -I. -Ifonts `pkg-config --cflags --libs ` -lutil -lasan -fsanitize=address -lz -march=native -lm -Ideps

sentry:
	sentry Makefile ctx.h tests/*.ctx -- sh -c 'make ctx  && make -C tests png'
sentry-f:
	sentry Makefile ctx.h tests/*.ctx -- make
ctx.h.html: ctx.h Makefile
	highlight -l -a --encoding=utf8 -W ctx.h > ctx.h.html
ctx-font-regular.h.html: fonts/ctx-font-regular.h Makefile
	highlight -l -a --encoding=utf8 -W fonts/ctx-font-regular.h > ctx-font-regular.h.html
	
#git gc

updateweb: all post ctx.h.html ctx-font-regular.h.html
	stagit .
	cat tests/index.html | sed 's/.*script.*//' > tmp
	mv tmp tests/index.html
	git update-server-info
	rm -rf /home/pippin/pgo/ctx.graphics/.git || true
	cp -Rv .git /home/pippin/pgo/ctx.graphics/.git
	cp -R mcu/* ~/pgo/ctx.graphics/mcu
	cp -R file/* ~/pgo/ctx.graphics/file
	cp -R commit/* ~/pgo/ctx.graphics/commit
	cp -R tests/* ~/pgo/ctx.graphics/tests
	cp -R protocol/* ~/pgo/ctx.graphics/protocol
	cp -R rasterizer/* ~/pgo/ctx.graphics/rasterizer
	cp -R glitch/* ~/pgo/ctx.graphics/glitch
	cp -R terminal/* ~/pgo/ctx.graphics/terminal
	cp *.css *.html ctx.h fonts/ctx-font-regular.h ~/pgo/ctx.graphics/
