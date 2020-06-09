SMALL_CFLAGS=-Os  -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto
#SMALL_CFLAGS=-O2 -fomit-frame-pointer -flto

report: ui tiny small baseline musl-static-tiny musl-static-small musl-static-baseline all
	(cd ../tools; ./size-report.sh)

all: ui tiny small sdl2 mmm test-renderpaths musl-static-tiny musl-static-small musl-static-baseline musl-static-full full
clean:
	rm -rf mmm sdl2 tiny small musl-static-tiny musl-static-small musl-static-baseline test-renderpaths baseline full musl-static-full ui
mmm: mmm.c ../ctx.h  Makefile
	ccache $(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs mmm` -Wall
sdl2: sdl2.c ../ctx.h
	ccache $(CC) $< -o $@ -I.. -I../tools -I../fonts `pkg-config --cflags --libs sdl2` -lm -Wall
test-renderpaths: test-renderpaths.c ../ctx.h ../fonts/*.h
	gcc $< -o $@ -g -O3 -march=native -mtune=native `pkg-config mrg --cflags --libs` -I../fonts -I../tools -I.. -msse3


baseline: baseline.c ../ctx.h ../fonts/*.h Makefile
	ccache $(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
ui: ui.c ../ctx.h ../fonts/*.h Makefile
	musl-gcc -static $< -o $@ -I.. -I../fonts
tiny: tiny.c ../ctx.h ../fonts/*.h Makefile
	ccache $(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
small: small.c ../ctx.h ../fonts/*.h Makefile
	ccache $(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
full: full.c ../*.h ../fonts/*.h Makefile
	ccache $(CC) $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@

musl-static-full: full.c ../*.h ../fonts/*.h Makefile
	ccache musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@

musl-static-small: small.c ../ctx.h ../fonts/*.h Makefile
	ccache musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
musl-static-tiny: tiny.c ../ctx.h ../fonts/*.h Makefile
	ccache musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
musl-static-baseline: baseline.c ../ctx.h ../fonts/*.h Makefile
	ccache musl-gcc $(SMALL_CFLAGS) $< -o $@ -I.. -I../fonts -static
	strip -S --strip-unneeded --remove-section=.note.gnu.gold-version --remove-section=.comment --remove-section=.note --remove-section=.note.gnu.build-id --remove-section=.note.ABI-tag $@
