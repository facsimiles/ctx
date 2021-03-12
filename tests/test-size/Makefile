SMALL_CFLAGS=-s -Os -march=x86-64 -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program -fno-math-errno -ffast-math -fsingle-precision-constant -DNO_SDL -fno-stack-protector -fno-ident -Wl,--build-id=none
#SMALL_CFLAGS=-Ofast -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program 
PERF_CFLAGS=-Ofast  -g
CC=i686-linux-gnu-gcc

PROGRAMS=medium musl-static-medium tiny small baseline musl-static-tiny musl-static-small musl-static-baseline ctx-info-32bit

report: all ../../fonts/ctxf/ascii.ctxf ../../fonts/ctxf/regular.ctxf
	./size-report.sh

all: $(PROGRAMS)
clean:
	rm -rf $(PROGRAMS)

text: text.c ../../*.h ../../fonts/*.h Makefile
	$(CC) $(PERF_CFLAGS)  $< -o $@ -I../.. -I../../fonts -march=native --cflags --libs` 

../../fonts/ctxf/ascii.ctxf:
	make -C ../.. fonts/ctxf/ascii.ctxf
../../fonts/ctxf/regular.ctxf:
	make -C ../.. fonts/ctxf/regular.ctxf

%: %.c ../../*.h ../../fonts/*.h Makefile tiny-config.h
	$(CC) $(SMALL_CFLAGS)  $< -o $@ -I../../ -I../../fonts -march=native

musl-static-%: %.c ../../*.h ../../fonts/*.h Makefile tiny-config.h
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I../.. -I../../fonts -static

perf.S: perf.c ../../*.h ../../fonts/*.h Makefile
	gcc-10 $(PERF_CFLAGS) $< -S -o $@ -I../.. -I../../fonts 
perf.s: perf.S
	as -alhnd perf.S > perf.s
perf: perf.c ../../*.h ../../fonts/*.h Makefile
	gcc-10 $(PERF_CFLAGS) $< -o $@ -I../.. -I../../fonts 
perf-circ: perf-circ.c ../../*.h ../../fonts/*.h Makefile
	gcc $(PERF_CFLAGS)  $< -o $@ -I../.. -I../../fonts `pkg-config cairo --cflags --libs` -lm

ctx-info-32bit: ctx-info.c ../../ctx.h tiny-config.h
	i686-linux-gnu-gcc $< -o $@ -lm -I../.. -I../../fonts -Wall -lm -I../../deps
