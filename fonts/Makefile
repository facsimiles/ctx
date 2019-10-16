all: ctx-fontgen

ctx-font-regular.h: ctx-fontgen
	./ctx-fontgen ttf/DejaVuSans.ttf regular latin1-extra > $@
ctx-font-emoji.h: ctx-fontgen
	./ctx-fontgen ttf/DejaVuSans.ttf emoji emoji > $@

%: %.c ../ctx.h
	gcc $< -o $@ -lm -I.. -I../fonts
