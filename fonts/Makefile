all: ctx-font-regular.h \
	ctx-font-ascii.h \
	ctx-font-mono.h \
	ctx-font-emoji.h \
	unscii-16.h \
	DejaVuSans.h \
	Vera.h \
	DejaVuSansMono.h \
	\
	ctxf/regular.ctxf \
	ctxf/mono.ctxf \
	ctxf/emoji.ctxf \
	ctxf/ascii.ctxf

clean:
	rm -f *.h 


ctxf/ascii.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf regular ascii binary > $@
ctx-font-ascii.h: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf regular ascii > $@

ctxf/regular.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf regular ascii-extras binary > $@
ctx-font-regular.h: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf regular ascii-extras > $@

ctxf/mono.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSansMono.ttf mono ascii-extras-cp437-vt100 binary > $@
ctx-font-mono.h: Makefile
	../tools/ctx-fontgen ttf/DejaVuSansMono.ttf mono ascii-extras-cp437-vt100 > $@

ctxf/emoji.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf emoji emoji binary > $@
ctx-font-emoji.h: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf emoji emoji > $@

DejaVuSansMono.h: Makefile
	xxd -i ttf/DejaVuSansMono.ttf > DejaVuSansMono.h
	echo '#define DEJAVU_SANS_MONO 1' >> DejaVuSansMono.h

DejaVuSans.h: Makefile
	xxd -i ttf/DejaVuSans.ttf > DejaVuSans.h
	echo '#define DEJAVU_SANS 1' >> DejaVuSans.h

Vera.h: Makefile
	xxd -i ttf/Vera.ttf > Vera.h
	echo '#define VERA 1' >> Vera.h

unscii-16.h: Makefile
	xxd -i ttf/unscii-16.ttf > unscii-16.h
	echo '#define UNSCII_16 1' >> unscii-16.h

%: %.c ../ctx.h
	gcc $< -o $@ -lm -I.. -I../fonts
