SMALL_CFLAGS=-Os -march=x86-64 -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program -fno-math-errno -ffast-math -fsingle-precision-constant
#SMALL_CFLAGS=-Ofast -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program 
PERF_CFLAGS=-Ofast -march=native  -g

PROGRAMS=img perf-circ dir perf ui ui2 tiny small baseline musl-static-tiny musl-static-small musl-static-baseline all mmm

report: all
	(cd ../tools; ./size-report.sh)

all: $(PROGRAMS)
clean:
	rm -rf $(PROGRAMS)

text: text.c ../*.h ../fonts/*.h Makefile
	$(CC) $(PERF_CFLAGS)  $< -o $@ -I.. -I../fonts -march=native `pkg-config sdl2 --cflags --libs` 

%: %.c ../*.h ../fonts/*.h Makefile
	$(CC) $(SMALL_CFLAGS)  $< -o $@ -I.. -I../fonts -march=native `pkg-config sdl2 --cflags --libs` 

perf.S: perf.c ../*.h ../fonts/*.h Makefile
	gcc-10 $(PERF_CFLAGS) $< -S -o $@ -I.. -I../fonts 
perf.s: perf.S
	as -alhnd perf.S > perf.s
perf: perf.c ../*.h ../fonts/*.h Makefile
	gcc-10 $(PERF_CFLAGS) $< -o $@ -I.. -I../fonts 
perf-circ: perf-circ.c ../*.h ../fonts/*.h Makefile
	gcc $(PERF_CFLAGS)  $< -o $@ -I.. -I../fonts `pkg-config cairo --cflags --libs` -lm
perf-text: perf-text.c ../*.h ../fonts/*.h Makefile
	gcc-10 $(PERF_CFLAGS) $< -o $@ -I.. -I../fonts 

musl-static-%: %.c ../*.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static

mmm: mmm.c ../ctx.h  Makefile
	$(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs mmm` -Wall

