DESTDIR ?=
PREFIX  ?= /usr/local

CTX_VERSION=0.1.0

# hack to prefer clang when available
#CC=`command -v clang-16 || echo cc`
#CC=musl-gcc


CCACHE=`command -v ccache`
CLIENTS_CFILES = $(wildcard demos/c/*.c)
CLIENTS_BINS   = $(CLIENTS_CFILES:.c=)

all: build.conf ctx-wasm.pc ctx-wasm-simd.pc ctx.pc libctx.so ctx.h tools/ctx-fontgen ctx $(CLIENTS_BINS)
include build.conf

CFLAGS_warnings= -Wall \
                 -Wextra \
		 -Wno-unused-but-set-parameter \
		 -Wno-unused-parameter \
		 -Wno-unused-function \
		 -Wno-missing-field-initializers 
CFLAGS+= $(CFLAGS_warnings) -fpic -fno-builtin-memcpy -fno-semantic-interposition
CFLAGS+= -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600 \
	 -I/usr/X11R6/include -I/usr/X11R7/include

CFLAGS += -ffinite-math-only -fno-trapping-math -fno-signed-zeros -fno-math-errno

CFLAGS+= -I. -Ifonts -Ideps -Imedia-handlers
LIBS  += -lm -lpthread  


#TERMINAL_CFILES = $(wildcard terminal/*.c)
#TERMINAL_OBJS   = $(TERMINAL_CFILES:.c=.o)
TERMINAL_OBJS = terminal/terminal.o terminal/ctx-keyboard.o


#MEDIA_HANDLERS_CFILES = $(wildcard media-handlers/*.c)
#MEDIA_HANDLERS_OBJS   = $(MEDIA_HANDLERS_CFILES:.c=.o)
MEDIA_HANDLERS_OBJS = \
  media-handlers/ctx-gif.o \
  media-handlers/ctx-img.o \
  media-handlers/ctx-mpg.o \
  media-handlers/ctx-xml.o \
  media-handlers/ctx-tinyvg.o \
  media-handlers/ctx-hexview.o \
  media-handlers/convert.o


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
	@echo "!! now run make again !!";
	@echo "!!!!!!!!!!!!!!!!!!!!!!!!";false

demos/c/%: demos/c/%.c build.conf Makefile build.conf libctx.a 
	$(CCC) -g $< -o $@ $(CFLAGS) libctx.a $(LIBS) $(CTX_CFLAGS) $(CTX_LIBS) $(OFLAGS_LIGHT)

fonts/%.h: tools/ctx-fontgen
	make -C fonts `echo $@|sed s:fonts/::` 

fonts/Roboto-Regular.h: tools/ctx-fontgen Makefile #
	make -C fonts ctx-font-ascii.h #
	make -C fonts Roboto-Regular.h #
	make -C fonts Cousine-Regular.h  #

FONT_STAMP=fonts/Roboto-Regular.h


test: ctx
	make -C tests
distclean: clean
	rm -f build.*
clean:
	rm -rf nofont #
	rm -f ctx.h ctx ctx.static ctx.O0 *.o highlight.css
	rm -f libctx.a libctx.so
	rm -f ctx.pc ctx-wasm.pc
	rm -f $(CLIENTS_BINS)
	rm -f $(TERMINAL_OBJS)
	rm -f $(MEDIA_HANDLERS_OBJS)
	rm -f $(SRC_OBJS) #
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

tools/%: tools/%.c
	make nofont/ctx.h #
	$(CCC) $< -o $@ -g -lm -Inofont -I. -Ifonts -lpthread -Wall -lm -Ideps $(CFLAGS_warnings) -DCTX_NO_FONTS -DCTX_STB_TT=1 -DCTX_HARFBUZZ=1 `pkg-config harfbuzz --cflags --libs`

ctx.o: ctx.c ctx.h build.conf Makefile $(FONT_STAMP) build.conf
	$(CCC) $< -c -o $@ $(CFLAGS) $(CTX_CFLAGS) $(OFLAGS_LIGHT)

ctx-x86-64-v2.o: ctx.c ctx.h build.conf Makefile $(FONT_STAMP) build.conf
	rm -f vec.missed
	$(CCC) $< -c -o $@ $(CFLAGS) -DCTX_SIMD_X86_64_V2 -momit-leaf-frame-pointer -ftree-vectorize -mfpmath=sse -mmmx -msse -msse2 -msse4.1 -msse4.2 -mpopcnt -mssse3 $(CTX_CFLAGS) $(OFLAGS_LIGHT) \
	#-fopt-info-vec-missed=vec.missed
ctx-x86-64-v3.o: ctx.c ctx.h build.conf Makefile $(FONT_STAMP) build.conf
	rm -f vec.optimized
	$(CCC) $< -c -o $@ $(CFLAGS) -DCTX_SIMD_X86_64_V3 -mmovbe -momit-leaf-frame-pointer -mxsave -mxsaveopt -ftree-vectorize -mmmx -msse -msse2 -msse4.1 -msse4.2 -mpopcnt -mssse3 -mavx -mavx2 -mfma -mmovbe $(CTX_CFLAGS) $(OFLAGS_LIGHT) \
	#-fopt-info-vec-optimized=vec.optimized

ctx-arm-neon.o: ctx.c ctx.h build.conf Makefile $(FONT_STAMP) build.conf
	$(CCC) $< -c -o $@ $(CFLAGS) -DCTX_SIMD_ARM_NEON -ftree-vectorize -ffast-math -march=armv7 -mfpu=neon-vfpv4 $(CTX_CFLAGS) $(OFLAGS_LIGHT)

deps.o: deps.c build.conf Makefile 
	$(CCC) deps.c -c -o $@ $(CTX_CFLAGS) $(CFLAGS) -Wno-sign-compare $(OFLAGS_LIGHT)

terminal/%.o: terminal/%.c ctx.h terminal/*.h Makefile build.conf
	$(CCC) -c $< -o $@ $(CTX_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS) 
media-handlers/%.o: media-handlers/%.c ctx.h Makefile build.conf
	$(CCC) -c $< -o $@ $(CTX_CFLAGS) $(OFLAGS_LIGHT) $(CFLAGS) 
libctx.a: deps.o $(CTX_OBJS) build.conf Makefile
	$(AR) rcs $@ $(CTX_OBJS) deps.o 
libctx.so: $(CTX_OBJS) deps.o build.conf Makefile
	$(CCC) -shared $(LIBS) $(CTX_OBJS) deps.o $(CTX_LIBS) -o $@

ctx: main.c ctx.h  build.conf Makefile $(TERMINAL_OBJS) $(MEDIA_HANDLERS_OBJS) libctx.a
	$(CCC) main.c $(TERMINAL_OBJS) $(MEDIA_HANDLERS_OBJS) -o $@ $(CFLAGS) libctx.a $(LIBS) $(CTX_CFLAGS)  $(OFLAGS_LIGHT) -lpthread  $(CTX_LIBS) $(CTX_EXTRA_STATIC)

updateweb: all ctx test  #
	git repack #
	(cd docs ; stagit .. ) #
	cat tests/index.html | sed 's/.*script.*//' > tmp #
	mv tmp tests/index.html #
	git update-server-info #
	cp -ru tests/* ~/pgo/ctx.graphics/tests #
	cp -fru .git/* /home/pippin/pgo/ctx.graphics/.git #
	cp -ru docs/* ~/pgo/ctx.graphics/ #
	cp ctx.h ~/pgo/ctx.graphics/ #
#
afl/ctx: ctx.h #
	make clean #
	CC=../afl/afl-2.52b/afl-gcc make ctx -j5 #
	cp ctx afl/ctx #
#
flatpak: #
	rm -rf build-dir;flatpak-builder --user build-dir meta/graphics.ctx.terminal.yml #
	flatpak-builder --collection-id=graphics.ctx --repo=docs/flatpak --force-clean build-dir meta/graphics.ctx.terminal.yml #
#	
flatpak-install: #
	rm -rf build-dir;flatpak-builder --install --user build-dir meta/graphics.ctx.terminal.yml #
#
ctx.h: src/*.[ch] squoze/squoze.h src/index $(FONT_STAMP) tools/ctx-fontgen src/constants.h #
	(cd src; echo "/* ctx git commit: `git rev-parse --short HEAD` */"> ../$@ ;   cat `cat index` | grep -v ctx-split.h | sed 's/CTX_STATIC/static/g' >> ../$@) #
#
nofont/ctx.h: src/*.c src/*.h src/index #
	rm -rf nofont #
	mkdir nofont #
	(cd src;cat `cat index|grep -v font` | grep -v ctx-split.h | sed 's/CTX_STATIC/static/g' > ../$@) #
#
squoze/squoze: squoze/*.[ch]  #
	make -C squoze squoze #
#
src/constants.h: src/*.c Makefile squoze/squoze #
	echo '#ifndef __CTX_CONSTANTS' > $@     #
	echo '#define __CTX_CONSTANTS' >> $@    #
	for a in `cat src/*.[ch] | tr ';' ' ' | tr ',' ' ' | tr ')' ' '|tr ':' ' ' | tr '{' ' ' | tr ' ' '\n' | grep 'SQZ_[a-z][0-9a-zA-Z_]*'| sort | uniq`;do b=`echo $$a|tail -c+5|tr '_' '-'`;echo "#define $$a `./squoze/squoze -33 $$b`u // \"$$b\"";done >> $@ #
	echo '#endif' >> $@ #
#
static.inc: static/* static/*/* tools/gen_fs.sh #
	./tools/gen_fs.sh static > $@           #
#
#
#
ctx-$(CTX_VERSION).tar.bz2: ctx.h Makefile #
	rm -rf dist #
	rm -rf ctx-$(CTX_VERSION) #
	mkdir dist #
	cp ctx.h main.c configure.sh dist #
	mkdir dist/fonts #
	cp fonts/*.h dist/fonts #
	mkdir dist/terminal #
	cp terminal/*.[ch] dist/terminal #
	mkdir dist/deps #
	cp deps/*.[ch] dist/deps #
	cp deps.c dist #
	cp ctx.c dist #
	mkdir dist/tools #
	cp tools/*.[ch] dist/tools #
	mkdir dist/media-handlers #
	cp media-handlers/*.[ch] dist/media-handlers #
	mkdir dist/tests #
	mkdir dist/tests/reference #
	cp -r tests/reference/* dist/tests/reference #
	cp -r tests/*.ctx dist/tests/ #
	grep -v '.*#$$' tests/Makefile > dist/tests/Makefile #
	grep -v '.*#$$' Makefile > dist/Makefile #
	mv dist ctx-$(CTX_VERSION) #
	tar cjf ctx-$(CTX_VERSION).tar.bz2 ctx-$(CTX_VERSION) #
	rm -rf ctx-$(CTX_VERSION) #
#
dist: ctx-$(CTX_VERSION).tar.bz2 #
	
distcheck: dist #
	tar xvf ctx-$(CTX_VERSION).tar.bz2 #
	(cd ctx-$(CTX_VERSION); ./configure.sh --static && make ctx && make test ) #

