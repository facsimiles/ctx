DESTDIR ?=
PREFIX  ?= /usr/local

all: tools/ctx-fontgen ctx subdirs tools/ctx-info

subdirs: ctx.o
	make -C clients

fonts/ctx-font-ascii.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf regular ascii > $@
fonts/ctx-font-regular.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf regular ascii-extras > $@
fonts/ctx-font-mono.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSansMono.ttf mono ascii-extras > $@

used_fonts: fonts/ctx-font-ascii.h fonts/ctx-font-regular.h fonts/ctx-font-mono.h

test: ctx
	make -C tests
post: test
clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1 ctx.static *.o
	rm -f tests/index.html fonts/*.h
	for a in tools $(PRE_SUBDIRS) $(SUBDIRS); do make -C $$a clean; done

CFLAGS_warnings= -Wall \
                 -Wextra \
		 -Wno-array-bounds \
                 -Wno-implicit-fallthrough \
		 -Wno-unused-parameter \
		 -Wno-missing-field-initializers 

CFLAGS= -g $(CFLAGS_warnings)

install: ctx
	install -d $(DESTIDR)$(PREFIX)/bin
	install -m755 ctx $(DESTIDR)$(PREFIX)/bin
uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/ctx

CFLAGS+=-I. -Ifonts -Ideps -lutil -lz -lm -lpthread

tools/%: tools/%.c ctx.h test-size/tiny-config.h 
	gcc $< -o $@ -lm -I. -Ifonts -Wall -lm -Ideps

ctx.o: ctx.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h fonts/ctx-font-ascii.h
	$(CC) ctx.c -c -o $@ $(CFLAGS) `pkg-config sdl2 --cflags --libs` -O1

ctx-avx2.o: ctx.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h fonts/ctx-font-ascii.h
	$(CC) ctx.c -c -o $@ $(CFLAGS) `pkg-config sdl2 --cflags --libs` -DCTX_AVX2=1 -march=native -O3

ctx-nosdl.o: ctx.c ctx.h Makefile used_fonts
	musl-gcc ctx.c -c -o $@ $(CFLAGS) -DNO_SDL=1 -DCTX_FB=1

ctx: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx.o clients/itk.h
	$(CC) main.c terminal/*.c convert/*.c -o $@ $(CFLAGS) `pkg-config sdl2 --cflags --libs` ctx.o -O1

ctx.avx2: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx-avx2.o
	$(CC) main.c terminal/*.c convert/*.c -o $@ $(CFLAGS) `pkg-config sdl2 --cflags --libs` ctx-avx2.o -O1

ctx.static: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx-nosdl.o
	musl-gcc main.c terminal/*.c convert/*.c -o $@ $(CFLAGS) ctx-nosdl.o -DNO_SDL=1 -DCTX_FB=1 -static 
	strip -s -x $@
	upx $@

docs/ctx.h.html: ctx.h Makefile
	highlight -l -a --encoding=utf8 -W ctx.h > docs/ctx.h.html
docs/ctx-font-regular.h.html: fonts/ctx-font-regular.h Makefile
	highlight -l -a --encoding=utf8 -W fonts/ctx-font-regular.h > docs/ctx-font-regular.h.html
	
#git gc

updateweb: all post docs/ctx.h.html docs/ctx-font-regular.h.html
	(cd docs ; stagit .. )
	cat tests/index.html | sed 's/.*script.*//' > tmp
	mv tmp tests/index.html
	git update-server-info
	cp -fru .git/* /home/pippin/pgo/ctx.graphics/.git
	cp -ru docs/* ~/pgo/ctx.graphics/
	cp -ru tests/* ~/pgo/ctx.graphics/tests
	cp ctx.h fonts/ctx-font-regular.h ~/pgo/ctx.graphics/
flatpak:
	rm -rf build-dir;flatpak-builder --user --install build-dir graphics.ctx.terminal.yml
