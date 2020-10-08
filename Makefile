all: tools/ctx-fontgen pre_subdirs ctx.o ctx subdirs

PRE_SUBDIRS=fonts
SUBDIRS=clients

pre_subdirs: tools/ctx-fontgen
	for a in $(PRE_SUBDIRS); do make -C $$a; done

subdirs: pre_subdirs ctx.o tools/ctx-info
	for a in $(SUBDIRS); do make -C $$a; done

test: all
	make -C tests
post: test
clean:
	rm -f test-renderpaths ctx ctx.asan ctx.O1
	rm -f tests/index.html
	for a in tools $(PRE_SUBDIRS) $(SUBDIRS); do make -C $$a clean; done

CFLAGS= -O3 -g -march=native -Wno-array-bounds 
#CFLAGS= -Os 

tools/%: tools/%.c ctx.h test-size/tiny-config.h
	gcc $< -o $@ -lm -I. -Ifonts -Wall -lm -Ideps
tools/%-32bit: tools/%.c ctx.h test-size/tiny-config.h
	i686-linux-gnu-gcc $< -o $@ -lm -I. -Ifonts -Wall -lm -Ideps

ctx.o: ctx-lib.c ctx.h Makefile pre_subdirs
	$(CC) ctx-lib.c -c -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 --cflags --libs` -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps 

ctx-nosdl.o: ctx-lib.c ctx.h Makefile pre_subdirs
	musl-gcc ctx-lib.c -c -o $@ $(CFLAGS) -I. -Ifonts -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps -DNO_SDL=1 -DCTX_FB=1

ctx: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx.o
	$(CC) main.c terminal/*.c convert/*.c -o $@ $(CFLAGS) -I. -Ifonts `pkg-config sdl2 --cflags --libs` ctx.o -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps -lpthread

ctx-static: main.c ctx.h  Makefile terminal/*.[ch] convert/*.[ch] ctx-nosdl.o
	musl-gcc main.c terminal/*.c convert/*.c -o $@ $(CFLAGS) -I. -Ifonts ctx-nosdl.o -lutil -Wall  -lz -Wextra -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-missing-field-initializers  -lm -Ideps -lpthread -DNO_SDL=1 -DCTX_FB=1 -static 
	strip -s -x $@
	upx $@

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
	cp -R binaries/* ~/pgo/ctx.graphics/binaries
	cp -R tests/* ~/pgo/ctx.graphics/tests
	cp -R protocol/* ~/pgo/ctx.graphics/protocol
	cp -R rasterizer/* ~/pgo/ctx.graphics/rasterizer
	cp -R glitch/* ~/pgo/ctx.graphics/glitch
	cp -R terminal/* ~/pgo/ctx.graphics/terminal
	cp *.css *.html ctx.h fonts/ctx-font-regular.h ~/pgo/ctx.graphics/
flatpak:
	rm -rf build-dir;flatpak-builder --user --install build-dir graphics.ctx.terminal.yml
