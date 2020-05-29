SMALL_CFLAGS=-Os -s  -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto
#SMALL_CFLAGS=-O2 -fomit-frame-pointer

report: tiny small baseline musl-static-tiny musl-static-small musl-static-baseline
	(cd ../tools; ./size-report.sh)

all: tiny small sdl2 mmm test-renderpaths musl-static-tiny musl-static-small musl-static-baseline
clean:
	rm -rf mmm sdl2 tiny small musl-static-tiny musl-static-small musl-static-baseline test-renderpaths
mmm: mmm.c ../ctx.h  Makefile
	$(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs mmm` -Wall
sdl2: sdl2.c ../ctx.h
	$(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall
test-renderpaths: test-renderpaths.c ../ctx.h ../fonts/*.h
	gcc $< -o $@ -g -O3 -march=native -mtune=native `pkg-config mrg --cflags --libs` -I../fonts -I../tools -I.. -msse3

tiny: tiny.c ../ctx.h ../fonts/*.h Makefile
	$(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	@ls -slab $@
small: small.c ../ctx.h ../fonts/*.h Makefile
	$(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	@ls -slab $@

musl-static-small: small.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
	@ls -slab $@
musl-static-tiny: tiny.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
	@ls -slab $@
musl-static-baseline: baseline.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
	@ls -slab $@
