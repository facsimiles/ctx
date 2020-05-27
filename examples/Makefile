SMALL_CFLAGS=-Os -s  -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -DREALLY_TINY=1

all: ctx-tiny ctx-small sdl2 mmm test-renderpaths ctx-tiny ctx-tiny-static-musl  ctx-small-static-musl ctx-tiny-musl ctx-small-musl
clean:
	rm -rf mmm sdl2 ctx-tiny ctx-tiny-static-musl test-renderpaths ctx-small ctx-small-static-musl ctx-tiny-musl ctx-small-musl
mmm: mmm.c ../ctx.h  Makefile
	$(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs mmm` -Wall
sdl2: sdl2.c ../ctx.h
	$(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall
test-renderpaths: test-renderpaths.c ../ctx.h ../fonts/*.h
	gcc $< -o $@ -g -O3 -march=native -mtune=native `pkg-config mrg --cflags --libs` -I../fonts -I../tools -I.. -msse3

ctx-tiny: ctx-tiny.c ../ctx.h ../fonts/*.h Makefile
	$(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	@ls -slab $@


ctx-small-musl: ctx-small.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS)  $< -o $@ -I.. -I../fonts
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
	@ls -slab $@

ctx-tiny-musl: ctx-tiny.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS)  $< -o $@ -I.. -I../fonts
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
	@ls -slab $@

ctx-tiny-static-musl: ctx-tiny.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
	@ls -slab $@

ctx-small: ctx-small.c ../ctx.h ../fonts/*.h Makefile
	$(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	@ls -slab $@

ctx-small-static-musl: ctx-small.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	@ls -slab $@
