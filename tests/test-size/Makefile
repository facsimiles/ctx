SMALL_CFLAGS=-Os -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program -fno-math-errno -ffast-math -fsingle-precision-constant -DCTX_SDL=0 -fno-stack-protector -fno-ident -Wl,--build-id=none -fno-semantic-interposition
CC=gcc

PROGRAMS=medium musl-static-medium tiny small baseline musl-static-tiny musl-static-small musl-static-baseline ctx-info-32bit

report: all ../../fonts/ctx-font-ascii.ctxf ../../fonts/Roboto-Regular.ctxf
	./size-report.sh

all: $(PROGRAMS)
clean:
	rm -rf $(PROGRAMS)

../../fonts/ctx-font-ascii.ctxf:
	make -C ../../fonts ctx-font-ascii.ctxf
../../fonts/Roboto-Regular.ctxf:
	make -C ../../fonts Roboto-Regular.ctxf

%: %.c ../../*.h Makefile tiny-config.h
	$(CC) $(SMALL_CFLAGS)  $< -o $@ -I../../ -I../../fonts -march=native && strip $@

musl-static-%: %.c ../../*.h ../../*.h Makefile tiny-config.h
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I../.. -I../../fonts -static && strip $@

ctx-info-32bit: ctx-info.c ../../ctx.h tiny-config.h Makefile
	i686-linux-gnu-gcc-12 $< -o $@ -g -lm -I../.. -I../../fonts -Wall -lm -I../../deps
