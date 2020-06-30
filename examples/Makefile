SMALL_CFLAGS=-Os -march=x86-64 -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program -fno-math-errno -ffast-math -fsingle-precision-constant --param large-function-growth=400
PERF_CFLAGS=-Ofast -fomit-frame-pointer -finline-small-functions  --param inline-unit-growth=100 -ftree-vectorize -fsingle-precision-constant -ftree-vectorizer-verbose=1 -fdump-tree-vect-details -funsafe-loop-optimizations -march=native



PROGRAMS=clock dir perf ui ui2 tiny small baseline musl-static-tiny musl-static-small musl-static-baseline all sdl2 mmm

report: all
	(cd ../tools; ./size-report.sh)

all: $(PROGRAMS)
clean:
	rm -rf $(PROGRAMS)

%: %.c ../*.h ../fonts/*.h Makefile
	ccache $(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts

perf.S: perf.c ../*.h ../fonts/*.h Makefile
	ccache gcc-10 $(PERF_CFLAGS) $< -S -o $@ -I.. -I../fonts 
perf.s: perf.S
	as -alhnd perf.S > perf.s
perf: perf.c ../*.h ../fonts/*.h Makefile
	ccache gcc-10 -s $(PERF_CFLAGS) $< -o $@ -I.. -I../fonts 

musl-static-%: %.c ../*.h ../fonts/*.h Makefile
	ccache musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static

mmm: mmm.c ../ctx.h  Makefile
	ccache $(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs mmm` -Wall
sdl2: sdl2.c ../ctx.h
	ccache $(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall
test-renderpaths: test-renderpaths.c ../ctx.h ../fonts/*.h
	gcc $< -o $@ -g -O3 -march=native -mtune=native `pkg-config mrg --cflags --libs` -I../fonts -I../tools -I.. -msse3


