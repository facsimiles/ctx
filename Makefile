DESTDIR ?=
PREFIX  ?= /usr/local

all: tools/ctx-fontgen ctx clients/demo clients/itk-sampler clients/dots ctx.avx2

fonts/ctx-font-ascii.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf regular ascii > $@
fonts/ctxf/ascii.ctxf: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf regular ascii binary > $@
fonts/ctx-font-regular.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf regular ascii-extras > $@
fonts/ctx-font-mono.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSansMono.ttf mono ascii-extras > $@

used_fonts: fonts/ctx-font-ascii.h fonts/ctx-font-regular.h fonts/ctx-font-mono.h fonts/ctxf/ascii.ctxf

test: ctx
	make -C tests

clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1 ctx.static *.o
	rm -f terminal/*.o
	rm -f tests/index.html fonts/*.h fonts/ctxf/* tools/ctx-fontgen

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

CFLAGS+=-I. -Ifonts -Ideps
LIBS   =-lutil -lz -lm -lpthread

tools/%: tools/%.c ctx.h 
	gcc $< -o $@ -lm -I. -Ifonts -Wall -lm -Ideps

ctx.o: ctx.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h fonts/ctx-font-ascii.h
	$(CC) ctx.c -c -o $@ $(CFLAGS) `pkg-config sdl2 --cflags` -O1

ctx-avx2.o: ctx.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h fonts/ctx-font-ascii.h
	$(CC) ctx.c -c -o $@ $(CFLAGS) `pkg-config sdl2 --cflags` -DCTX_AVX2=1 -march=native -O3

ctx-nosdl.o: ctx.c ctx.h Makefile used_fonts
	musl-gcc ctx.c -c -o $@ $(CFLAGS) -DNO_SDL=1 -DCTX_FB=1

terminal/%.o: terminal/%.c *.h terminal/*.h
	$(CC) -c $< -o $@ `pkg-config --cflags sdl2` -O2 $(CFLAGS)

ctx: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx.o clients/itk.h terminal/vt.o terminal/terminal.o terminal/vt-line.o
	$(CC) main.c terminal/*.o convert/*.c -o $@ $(CFLAGS) $(LIBS) `pkg-config sdl2 --cflags --libs` ctx.o -O1

ctx.avx2: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx-avx2.o terminal/vt.o terminal/terminal.o terminal/vt-line.o
	$(CC) main.c terminal/*.o convert/*.c -o $@ $(CFLAGS) $(LIBS) `pkg-config sdl2 --cflags --libs` ctx-avx2.o -O1

ctx.static: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx-nosdl.o terminal/vt.o terminal/terminal.o terminal/vt-line.o
	musl-gcc main.c terminal/*.o convert/*.c -o $@ $(CFLAGS) $(LIBS) ctx-nosdl.o -DNO_SDL=1 -DCTX_FB=1 -static 
	strip -s -x $@
	upx $@

docs/ctx.h.html: ctx.h Makefile
	highlight -l -a --encoding=utf8 -W ctx.h > docs/ctx.h.html
docs/ctx-font-regular.h.html: fonts/ctx-font-regular.h Makefile
	highlight -l -a --encoding=utf8 -W fonts/ctx-font-regular.h > docs/ctx-font-regular.h.html
	
#git gc

updateweb: all test docs/ctx.h.html docs/ctx-font-regular.h.html
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

clients/%: clients/%.c used_fonts Makefile ctx.o clients/itk.h
	$(CC) -g $< -o $@ $(CFLAGS) ctx.o $(LIBS) `pkg-config sdl2 --cflags --libs`

