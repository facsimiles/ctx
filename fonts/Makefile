all: ctx-font-regular.h \
	ctx-font-mono.h \
	ctx-font-emoji.h \
	DejaVuSans.h \
	DejaVuSansMono.h \

clean:
	rm -f ctx-font-*.h

ctx-font-regular.h: ctx-fontgen Makefile
	./ctx-fontgen ttf/DejaVuSans.ttf regular ascii-extras > $@
ctx-font-mono.h: ctx-fontgen Makefile
	./ctx-fontgen ttf/DejaVuSansMono.ttf mono ascii-extras-cp437-vt100 > $@
ctx-font-emoji.h: ctx-fontgen Makefile
	./ctx-fontgen ttf/DejaVuSans.ttf emoji emoji > $@

DejaVuSansMono.h: Makefile
	xxd -i ttf/DejaVuSansMono.ttf > DejaVuSansMono.h
	echo '#define DEJAVU_SANS_MONO 1' >> DejaVuSansMono.h

DejaVuSans.h: Makefile
	xxd -i ttf/DejaVuSans.ttf > DejaVuSans.h
	echo '#define DEJAVU_SANS 1' >> DejaVuSans.h


%: %.c ../ctx.h
	gcc $< -o $@ -lm -I.. -I../fonts
