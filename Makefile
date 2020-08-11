all: pre ctx.o ctx post
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

CFLAGS= -g -march=native -Wno-array-bounds 
#CFLAGS=-Os -flto

ctx.o: ctx-lib.c ctx.h Makefile
	ccache $(CC) ctx-lib.c -c -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 babl --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps

libctx.o: ctx-lib.c ctx.h Makefile
	ccache $(CC) ctx-lib.c -shared -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 babl --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps

ctx: ctx.c ctx.h  Makefile svg.h
	ccache $(CC) ctx.c -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 babl --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps

ctx.O1: ctx.c ctx.h  Makefile svg.h
	ccache $(CC) ctx.c -o $@ -g -O1 -I. -Ifonts `pkg-config sdl2 babl --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps -march=native

ctx.asan: ctx.c ctx.h Makefile
	$(CC) -DASANBUILD=1 ctx.c -o $@ -g -O0 -I. -Ifonts `pkg-config --cflags --libs babl ` -lutil -lasan -fsanitize=address -lz -march=native -lm -Ideps

sentry:
	sentry Makefile ctx.h tests/*.ctx -- sh -c 'make ctx  post && make -C tests png'
sentry-f:
	sentry Makefile ctx.h tests/*.ctx -- make
ctx.h.html: ctx.h Makefile
	highlight -l -a --encoding=utf8 -W ctx.h > ctx.h.html
ctx-font-regular.h.html: fonts/ctx-font-regular.h Makefile
	highlight -l -a --encoding=utf8 -W fonts/ctx-font-regular.h > ctx-font-regular.h.html
	
#git gc

updateweb: clean all post ctx.h.html ctx-font-regular.h.html
	cat tests/index.html | sed 's/.*script.*//' > tmp
	mv tmp tests/index.html
	git update-server-info
	rm -rf /home/pippin/pgo/ctx.graphics/.git || true
	cp -Rv .git /home/pippin/pgo/ctx.graphics/.git
	cp -R mcu/* ~/pgo/ctx.graphics/mcu
	cp -R tests/* ~/pgo/ctx.graphics/tests
	cp -R rasterizer/* ~/pgo/ctx.graphics/rasterizer
	cp -R glitch/* ~/pgo/ctx.graphics/glitch
	cp ctx.css index.html highlight.css ctx.h.html ctx-font-regular.h.html ctx.h fonts/ctx-font-regular.h ~/pgo/ctx.graphics/
