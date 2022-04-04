DESTDIR ?=
PREFIX  ?= /usr/local

#CCACHE=`command -v ccache`
CLIENTS_CFILES = $(wildcard demos/c/*.c)
CLIENTS_BINS   = $(CLIENTS_CFILES:.c=)

all: build.conf ctx-wasm.pc ctx-wasm-simd.pc ctx.pc libctx.so ctx.h tools/ctx-fontgen ctx $(CLIENTS_BINS)
include build.conf

CFLAGS_warnings= -Wall \
                 -Wextra \
		 -Wno-array-bounds \
		 -Wno-unused-parameter \
		 -Wno-unused-function \
		 -Wno-missing-field-initializers 

CFLAGS+= $(CFLAGS_warnings) -fPIC 
CFLAGS+= -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600 \
	 -I/usr/X11R6/include -I/usr/X11R7/include

CFLAGS+= -I. -Ifonts -Ideps -Imedia-handlers
LIBS  += -lm -lpthread -lz

#TERMINAL_CFILES = $(wildcard terminal/*.c)
#TERMINAL_OBJS   = $(TERMINAL_CFILES:.c=.o)
TERMINAL_OBJS = terminal/terminal.o terminal/ctx-keyboard.o


#MEDIA_HANDLERS_CFILES = $(wildcard media-handlers/*.c)
#MEDIA_HANDLERS_OBJS   = $(MEDIA_HANDLERS_CFILES:.c=.o)
MEDIA_HANDLERS_OBJS = \
  stuff/stuff.o \
  stuff/argvs.o \
  stuff/collection.o \
  media-handlers/ctx-gif.o \
  media-handlers/ctx-img.o \
  media-handlers/ctx-mpg.o \
  media-handlers/ctx-tinyvg.o \
  media-handlers/ctx-hexview.o \
  media-handlers/convert.o \
  media-handlers/tcp.o


SRC_CFILES = $(wildcard src/*.c)
SRC_OBJS   = $(SRC_CFILES:.c=.o)

CTX_OBJS = ctx.o


ifeq ($(CTX_SIMD), 1)
ifeq ($(CTX_ARCH), x86_64)
CTX_SIMD_OBJS = ctx-x86-64-v2.o ctx-x86-64-v3.o
else ifeq ($(CTX_ARCH), armv7l)
CTX_SIMD_OBJS = ctx-arm-neon.o 
endif
CTX_OBJS += $(CTX_SIMD_OBJS)
endif

CCC=$(CCACHE) $(CC)
build.conf:
	@echo "You have not run configure, running ./configure.sh without arguments for you"
	@echo "you will have to run make again after this.";echo
	./configure.sh
	@echo "!!!!!!!!!!!!!!!!!!!!!!!!";
	@echo "!! now run Make again !!";
	@echo "!!!!!!!!!!!!!!!!!!!!!!!!";false

demos/c/%: demos/c/%.c build.conf Makefile build.conf media-handlers/itk.h libctx.a
	$(CCC) -g $< -o $@ $(CFLAGS) libctx.a $(LIBS) $(CTX_CFLAGS) $(CTX_LIBS) $(OFLAGS_LIGHT)

fonts/ctx-font-ascii.h: tools/ctx-fontgen
	./tools/ctx-fontgen fonts/ttf/DejaVuSans.ttf ascii ascii > $@
fonts/ctx-font-ascii-spacing.h: fonts/ctx-font-ascii.h tools/ctx-fontgen
	grep -v "},$$" $< > $@
fonts/ctx-font-regular-spacing.h: fonts/ctx-font-regular.h tools/ctx-fontgen
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
fonts/NotoMono-Regular.h: build.conf Makefile
	cd fonts; xxd -i ttf/NotoMono-Regular.ttf > NotoMono-Regular.h
	echo '#define NOTO_MONO_REGULAR 1' >> $@
fonts/Roboto-Regular.h: build.conf Makefile
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
	rm -f ctx.pc ctx-wasm.pc
	rm -f $(CLIENTS_BINS)
	rm -f $(TERMINAL_OBJS)
	rm -f $(MEDIA_HANDLERS_OBJS)
	rm -f $(SRC_OBJS)
	rm -f tests/index.html fonts/*.h fonts/ctxf/* tools/ctx-fontgen

ctx.pc: Makefile
	@echo "prefix=$(PREFIX)" > $@
	@echo 'exec_prefix=$${prefix}' >> $@
	@echo 'libdir=$${prefix}/lib' >> $@
	@echo 'includedir=$${prefix}/include' >> $@
	@echo 'apiversion=0.0' >> $@
	@echo '' >> $@
	@echo 'Name: ctx' >> $@
	@echo 'Description: ctx vector graphics' >> $@
	@echo 'Version: 0.0.0' >> $@
	@echo 'Libs: -L$${libdir} -lctx' >> $@
	@echo 'Cflags: -I$${includedir}' >> $@

ctx-wasm.pc: Makefile
	@echo "prefix=$(PREFIX)" > $@
	@echo 'exec_prefix=$${prefix}' >> $@
	@echo 'libdir=$${prefix}/lib' >> $@
	@echo 'includedir=$${prefix}/include' >> $@
	@echo 'apiversion=0.0' >> $@
	@echo '' >> $@
	@echo 'Name: ctx' >> $@
	@echo 'Description: ctx vector graphics - wasm target' >> $@
	@echo 'Version: 0.0.0' >> $@
	@echo 'Libs: -Wl,--lto-O3 ' >> $@
	@echo 'Cflags: -I$${includedir} -DCTX_IMPLEMENTATION -flto -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=_main,_free,_calloc,_malloc' >> $@

ctx-wasm-simd.pc: Makefile
	@echo "prefix=$(PREFIX)" > $@
	@echo 'exec_prefix=$${prefix}' >> $@
	@echo 'libdir=$${prefix}/lib' >> $@
	@echo 'includedir=$${prefix}/include' >> $@
	@echo 'apiversion=0.0' >> $@
	@echo '' >> $@
	@echo 'Name: ctx' >> $@
	@echo 'Description: ctx vector graphics - wasm target' >> $@
	@echo 'Version: 0.0.0' >> $@
	@echo 'Libs: -Wl,--lto-O3 ' >> $@
	@echo 'Cflags: -msimd128 -ftree-vectorize -I$${includedir} -DCTX_IMPLEMENTATION -flto -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=_main,_free,_calloc,_malloc' >> $@

install: ctx libctx.so ctx.h ctx.pc ctx-wasm.pc ctx-wasm-simd.pc
	install -D -m755 -t $(DESTDIR)$(PREFIX)/bin ctx
	install -D -m755 -t $(DESTDIR)$(PREFIX)/lib/pkgconfig ctx.pc
	install -D -m755 -t $(DESTDIR)$(PREFIX)/lib/pkgconfig ctx-wasm.pc
	install -D -m755 -t $(DESTDIR)$(PREFIX)/lib/pkgconfig ctx-wasm-simd.pc
	install -D -m644 -t $(DESTDIR)$(PREFIX)/include ctx.h
	install -D -m755 -t $(DESTDIR)$(PREFIX)/lib libctx.so
	install -D -m755 -t $(DESTDIR)$(PREFIX)/bin tools/ctx-audioplayer
	install -D -m644 -t $(DESTDIR)$(PREFIX)/share/appdata meta/graphics.ctx.terminal.appdata.xml
	install -D -m644 -t $(DESTDIR)$(PREFIX)/share/applications meta/graphics.ctx.terminal.desktop
	install -D -m644 -t $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps meta/graphics.ctx.terminal.svg
	ldconfig || true
uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/bin/ctx
	rm -rf $(DESTDIR)$(PREFIX)/lib/libctx.so
	rm -rf $(DESTDIR)$(PREFIX)/lib/pkgconfig/ctx.pc
	rm -rf $(DESTDIR)$(PREFIX)/lib/pkgconfig/ctx-wasm.pc
	rm -rf $(DESTDIR)$(PREFIX)/lib/pkgconfig/ctx-wasm-simd.pc
	rm -rf $(DESTDIR)$(PREFIX)/include/ctx.h
	rm -f $(DESTDIR)$(PREFIX)/share/appdata/graphics.ctx.terminal.appdata.xml
	rm -f $(DESTDIR)$(PREFIX)/share/applications/graphics.ctx.terminal.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/graphics.ctx.terminal.svg

tools/%: tools/%.c ctx-nofont.h 
	$(CCC) $< -o $@ -g -lm -I. -Ifonts -lpthread -Wall -lm -Ideps $(CFLAGS_warnings)

ctx.o: ctx.c ctx.h build.conf Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf media-handlers/itk.h
	$(CCC) $< -c -o $@ $(CFLAGS) $(CTX_CFLAGS) $(OFLAGS_LIGHT)

ctx-x86-64-v2.o: ctx.c ctx.h build.conf Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf
	rm -f vec.missed
	$(CCC) $< -c -o $@ $(CFLAGS) -DCTX_SIMD_X86_64_V2 -momit-leaf-frame-pointer -ftree-vectorize -ffast-math -mfpmath=sse -mmmx -msse -msse2 -msse4.1 -msse4.2 -mpopcnt -mssse3 $(CTX_CFLAGS) $(OFLAGS_LIGHT) 
	#-fopt-info-vec-missed=vec.missed
ctx-x86-64-v3.o: ctx.c ctx.h build.conf Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf
	rm -f vec.optimized
	$(CCC) $< -c -o $@ $(CFLAGS) -DCTX_SIMD_X86_64_V3 -mmovbe -momit-leaf-frame-pointer -mxsave -mxsaveopt -ftree-vectorize -ffast-math -mmmx -msse -msse2 -msse4.1 -msse4.2 -mpopcnt -mssse3 -mavx -mavx2 -mfma -mmovbe $(CTX_CFLAGS) $(OFLAGS_LIGHT)
	#-fopt-info-vec-optimized=vec.optimized

ctx-arm-neon.o: ctx.c ctx.h build.conf Makefile fonts/ctx-font-regular.h fonts/ctx-font-mono.h build.conf
	$(CCC) $< -c -o $@ $(CFLAGS) -DCTX_SIMD_ARM_NEON -ftree-vectorize -ffast-math -march=armv7 -mfpu=neon-vfpv4 $(CTX_CFLAGS) $(OFLAGS_LIGHT)


deps.o: deps.c build.conf Makefile 
	$(CCC) deps.c -c -o $@ $(CFLAGS) -Wno-sign-compare $(OFLAGS_LIGHT)

src/%.o: src/%.c split/*.h
	$(CCC) -c $< -o $@ $(CTX_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS)

terminal/%.o: terminal/%.c ctx.h terminal/*.h media-handlers/itk.h
	$(CCC) -c $< -o $@ $(CTX_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS) 
media-handlers/%.o: media-handlers/%.c ctx.h media-handlers/*.h media-handlers/metadata/*.c
	$(CCC) -c $< -o $@ $(CTX_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS) 
stuff/%.o: stuff/%.c ctx.h stuff/*.h stuff/*.inc
	$(CCC) -c $< -o $@ $(CTX_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS) 
libctx.a: deps.o $(CTX_OBJS) build.conf Makefile
	$(AR) rcs $@ $(CTX_OBJS) deps.o 
libctx.so: $(CTX_OBJS) deps.o
	$(LD) -shared $(LIBS) $(CTX_OBJS) deps.o $(CTX_LIBS) -o $@
	#$(LD) --retain-symbols-file=symbols -shared $(LIBS) $? $(CTX_LIBS)  -o $@

ctx: main.c ctx.h  build.conf Makefile $(TERMINAL_OBJS) $(MEDIA_HANDLERS_OBJS) libctx.a
	$(CCC) main.c $(TERMINAL_OBJS) $(MEDIA_HANDLERS_OBJS) -o $@ $(CFLAGS) libctx.a $(LIBS) $(CTX_CFLAGS)  $(OFLAGS_LIGHT) -lpthread  $(CTX_LIBS)

ctx.static: main.c ctx.h  build.conf Makefile $(MEDIA_HANDLERS_OBJS) $(CTX_SIMD_OBJS) ctx.o deps.o terminal/*.[ch] 
	$(CCC) main.c terminal/*.c $(MEDIA_HANDLERS_OBJS) -o $@ $(CFLAGS) ctx.o $(CTX_SIMD_OBJS) deps.o $(LIBS) -DCTX_BABL=0 -DCTX_SDL=0 -DCTX_FB=1 -DCTX_CURL=0 -static 
	strip -s -x $@

docs/ctx.h.html: ctx.h Makefile build.conf
	highlight -l -a --encoding=utf8 -W ctx.h > docs/ctx.h.html
docs/ctx-font-regular.h.html: fonts/ctx-font-regular.h Makefile build.conf
	highlight -l -a --encoding=utf8 -W fonts/ctx-font-regular.h > docs/ctx-font-regular.h.htm

#git gc

foo: ctx
updateweb: all ctx.static test docs/ctx.h.html docs/ctx-font-regular.h.html 
	git repack
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
	rm -rf build-dir;flatpak-builder --user build-dir meta/graphics.ctx.terminal.yml
	flatpak-builder --collection-id=graphics.ctx --repo=docs/flatpak --force-clean build-dir meta/graphics.ctx.terminal.yml
	
flatpak-install:
	rm -rf build-dir;flatpak-builder --install --user build-dir meta/graphics.ctx.terminal.yml

ctx.h: src/*.[ch] src/index fonts/ctx-font-ascii.h tools/ctx-fontgen
	(cd src; echo "/* ctx git commit: `git rev-parse --short HEAD` */"> ../$@ ;   cat `cat index` | grep -v ctx-split.h | sed 's/CTX_STATIC/static/g' >> ../$@)

ctx-nofont.h: src/*.c src/*.h src/index
	(cd src;cat `cat index|grep -v font` | grep -v ctx-split.h | sed 's/CTX_STATIC/static/g' > ../$@)
