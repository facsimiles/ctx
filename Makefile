DESTDIR ?=
PREFIX  ?= /usr/local

CCACHE=`which ccache`

CFLAGS_warnings= -Wall \
                 -Wextra \
		 -Wno-array-bounds \
		 -Wno-unused-parameter \
		 -Wno-unused-function \
		 -Wno-missing-field-initializers 

CFLAGS+= -g $(CFLAGS_warnings) -fPIC 
#  -ffast-math   gets rejected by duktape

CFLAGS+= -I. -Ifonts -Ideps
LIBS   = -lz -lm -lpthread

#CFLAGS+= -fsanitize=address
#LIBS+= -lasan

#OFLAGS_HARD=-O3
#OFLAGS_LIGHT=-O2

CLIENTS_CFILES = $(wildcard clients/*.c)
CLIENTS_BINS   = $(CLIENTS_CFILES:.c=)

TERMINAL_CFILES = $(wildcard terminal/*.c)
TERMINAL_OBJS   = $(TERMINAL_CFILES:.c=.o)


SRC_CFILES = $(wildcard src/*.c)
SRC_OBJS   = $(SRC_CFILES:.c=.o)

all: build.conf tools/ctx-fontgen ctx $(CLIENTS_BINS)
include build.conf

CCC=$(CCACHE) $(CC)
build.conf:
	@echo "You have not run configure, running it ./configure for you"
	@echo "you will have to run make again after this.";echo
	./configure.sh
	@echo "!!!!!!!!!!!!!!!!!!!!!!!!";
	@echo "!! now run Make again !!";
	@echo "!!!!!!!!!!!!!!!!!!!!!!!!";false

clients/%: clients/%.c Makefile ctx.o clients/itk.h libctx.a
	$(CCC) -g $< -o $@ $(CFLAGS) libctx.a $(LIBS) $(PKG_CFLAGS) $(PKG_LIBS) $(OFLAGS_LIGHT)

fonts/ctx-font-ascii.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf ascii ascii > $@
fonts/ctx-font-ascii-spacing.h: fonts/ctx-font-ascii.h
	grep -v "},$$" $< > $@
fonts/ctx-font-regular-spacing.h: fonts/ctx-font-regular.h
	grep -v "},$$" $< > $@

fonts/ctxf/ascii.ctxf: tools/ctx-fontgen
	@mkdir fonts/ctxf || true
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf ascii ascii binary > $@
fonts/ctxf/regular.ctxf: tools/ctx-fontgen
	@mkdir fonts/ctxf || true
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf regular ascii-extras binary > $@
fonts/ctx-font-regular.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf regular ascii-extras > $@
fonts/ctx-font-mono.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSansMono.ttf mono ascii-extras > $@
fonts/NotoMono-Regular.h: Makefile
	cd fonts; xxd -i ttf/NotoMono-Regular.ttf > NotoMono-Regular.h
	echo '#define NOTO_MONO_REGULAR 1' >> $@
fonts/Roboto-Regular.h: Makefile
	cd fonts; xxd -i ttf/Roboto-Regular.ttf > Roboto-Regular.h
	echo '#define ROBOTO_REGULAR 1' >> $@

used_fonts: fonts/ctx-font-regular.h fonts/ctx-font-mono.h fonts/ctxf/ascii.ctxf 
test: ctx
	make -C tests
distclean: clean
	rm -f build.*
clean:
	rm -f ctx-nofont.h ctx.h ctx ctx.static ctx.O0 *.o highlight.css
	rm -f libctx.a libctx.so
	rm -f $(CLIENTS_BINS)
	rm -f $(TERMINAL_OBJS)
	rm -f $(SRC_OBJS)
	rm -f tests/index.html fonts/*.h fonts/ctxf/* tools/ctx-fontgen

install: ctx
	install -d $(DESTIDR)$(PREFIX)/bin
	install -m755 ctx $(DESTIDR)$(PREFIX)/bin
uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/ctx

tools/%: tools/%.c ctx-nofont.h 
	$(CCC) $< -o $@ -g -lm -I. -Ifonts -lpthread -Wall -lm -Ideps $(CFLAGS_warnings) -DNO_LIBCURL

ctx.o: ctx.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf
	$(CCC) ctx.c -c -o $@ $(CFLAGS) $(PKG_CFLAGS) $(OFLAGS_LIGHT)

deps.o: deps.c Makefile
	$(CCC) deps.c -c -o $@ $(CFLAGS) -Wno-sign-compare $(OFLAGS_LIGHT)

ctx-avx2.o: ctx-avx2.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf
	$(CCC) ctx-avx2.c -c -o $@ $(CFLAGS) $(OFLAGS_HARD) -DCTX_AVX2=1 -mavx -mavx2 -mfpmath=sse -ftree-vectorize -funsafe-math-optimizations

#ctx-sse2.o: ctx-sse2.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf
#	$(CCC) ctx-sse2.c -c -o $@ $(CFLAGS)  $(OFLAGS_HARD) -march=core2 -ftree-vectorize -funsafe-math-optimizations

#ctx-mmx.o: ctx-mmx.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf
#	$(CCC) ctx-mmx.c -c -o $@ $(CFLAGS) $(OFLAGS_HARD) -mmmx -ftree-vectorize -funsafe-math-optimizations

ctx-split.o: $(SRC_OBJS)

ctx-static.o: ctx.c ctx.h Makefile used_fonts build.conf
	$(CCC) ctx.c -c -o $@ $(CFLAGS) $(OFLAGS_LIGHT) -DNO_SDL=1 -DNO_BABL=1 -DCTX_FB=1 -DNO_LIBCURL=1

src/%.o: src/%.c split/*.h
	$(CCC) -c $< -o $@ $(PKG_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS)

terminal/%.o: terminal/%.c ctx.h terminal/*.h clients/itk.h
	$(CCC) -c $< -o $@ $(PKG_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS) 
libctx.a: ctx.o ctx-avx2.o deps.o Makefile
	$(AR) rcs $@ $?
libctx.so: ctx.o ctx-avx2.o deps.o
	$(LD) -shared $(LIBS) $? $(PKG_LIBS) -o $@
	#$(LD) --retain-symbols-file=symbols -shared $(LIBS) $? $(PKG_LIBS)  -o $@

ctx: main.c ctx.h  Makefile convert/*.[ch] $(TERMINAL_OBJS) libctx.a
	$(CCC) main.c $(TERMINAL_OBJS) convert/*.c -o $@ $(CFLAGS) libctx.a $(LIBS) $(PKG_CFLAGS) $(PKG_LIBS) -lpthread $(OFLAGS_LIGHT)

ctx-O0.o: ctx.c ctx.h Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h fonts/ctx-font-ascii.h
	$(CCC) ctx.c -c -o $@ $(CFLAGS) $(PKG_CFLAGS) -O0

ctx.O0: main.c ctx.h  Makefile convert/*.[ch] ctx-O0.o $(TERMINAL_OBJS) deps.o
	$(CCC) main.c $(TERMINAL_OBJS) convert/*.c -o $@ $(CFLAGS) $(LIBS) $(PKG_CFLAGS) $(PKG_LIBS) ctx-O0.o deps.o -O0

ctx.static: main.c ctx.h  Makefile convert/*.[ch] ctx-static.o deps.o terminal/*.[ch] ctx-avx2.o 
	$(CCC) main.c terminal/*.c convert/*.c -o $@ $(CFLAGS) ctx-static.o ctx-avx2.o deps.o $(LIBS) -DNO_BABL=1 -DNO_SDL=1 -DCTX_FB=1 -DNO_LIBCURL=1 -static 
	strip -s -x $@

docs/ctx.h.html: ctx.h Makefile
	highlight -l -a --encoding=utf8 -W ctx.h > docs/ctx.h.html
docs/ctx-font-regular.h.html: fonts/ctx-font-regular.h Makefile
	highlight -l -a --encoding=utf8 -W fonts/ctx-font-regular.h > docs/ctx-font-regular.h.htm

#git gc

foo: ctx
updateweb: all ctx.static test docs/ctx.h.html docs/ctx-font-regular.h.html 
	(cd docs ; stagit .. )
	cat tests/index.html | sed 's/.*script.*//' > tmp
	mv tmp tests/index.html
	git update-server-info
	strip -s -x ctx ctx.static
	cp -f ctx docs/binaries/ctx-x86_64-SDL2
	cp -f ctx.static docs/binaries/ctx-x86_64-static
	upx docs/binaries/ctx-x86_64-SDL2
	upx docs/binaries/ctx-x86_64-static
	cp -ru tests/* ~/pgo/ctx.graphics/tests
	#make clean
	#proot -r /home/pippin/src/isthmus/i486 -b /dev -b /proc -b /sys -b /home/pippin/src/ctx ./configure.sh
	#proot -r /home/pippin/src/isthmus/i486 -b /dev -b /proc -b /sys -b /home/pippin/src/ctx make ctx.static 
	#cp -f ctx.static docs/binaries/ctx-i486-static
	#upx docs/binaries/ctx-i486-static
	cp -fru .git/* /home/pippin/pgo/ctx.graphics/.git
	cp -ru docs/* ~/pgo/ctx.graphics/
	cp ctx.h fonts/ctx-font-regular.h ~/pgo/ctx.graphics/

afl/ctx: ctx.h
	make clean
	CC=../afl/afl-2.52b/afl-gcc make ctx -j5
	cp ctx afl/ctx

flatpak:
	rm -rf build-dir;flatpak-builder --user --install build-dir meta/graphics.ctx.terminal.yml

ctx.h: src/* fonts/ctx-font-ascii.h
	(cd src;cat `cat index` | grep -v ctx-split.h | sed 's/CTX_STATIC/static/g' > ../$@)

ctx-nofont.h: src/*
	(cd src;cat `cat index|grep -v font` | grep -v ctx-split.h | sed 's/CTX_STATIC/static/g' > ../$@)
