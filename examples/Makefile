SMALL_CFLAGS=-Os  -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program -fno-math-errno -ffast-math -fsingle-precision-constant
#SMALL_CFLAGS=-O2 -fomit-frame-pointer -flto

PROGRAMS=ui ui2 tiny small baseline musl-static-tiny musl-static-small musl-static-baseline all sdl2 mmm

report: all
	(cd ../tools; ./size-report.sh)

all: $(PROGRAMS)
clean:
	rm -rf $(PROGRAMS)

%: %.c ../*.h ../fonts/*.h Makefile
	ccache $(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@

musl-static-%: %.c ../*.h ../fonts/*.h Makefile
	ccache musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@

mmm: mmm.c ../ctx.h  Makefile
	ccache $(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs mmm` -Wall
sdl2: sdl2.c ../ctx.h
	ccache $(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall
test-renderpaths: test-renderpaths.c ../ctx.h ../fonts/*.h
	gcc $< -o $@ -g -O3 -march=native -mtune=native `pkg-config mrg --cflags --libs` -I../fonts -I../tools -I.. -msse3


