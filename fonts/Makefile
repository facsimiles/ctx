all: ctx-font-regular.h \
	ctx-font-mono.h \
	ctx-font-emoji.h

ctx-font-regular.h: ctx-fontgen Makefile
	./ctx-fontgen ttf/DejaVuSans.ttf regular ascii-latin1 > $@
ctx-font-mono.h: ctx-fontgen Makefile
	./ctx-fontgen ttf/DejaVuSansMono.ttf regular ascii-latin1-terminal > $@
ctx-font-emoji.h: ctx-fontgen Makefile
	./ctx-fontgen ttf/DejaVuSans.ttf emoji emoji > $@

%: %.c ../ctx.h
	gcc $< -o $@ -lm -I.. -I../fonts
