all: ctx-font-regular.h \
	ctx-font-emoji.h

ctx-font-regular.h: ctx-fontgen
	./ctx-fontgen ttf/DejaVuSans.ttf regular ascii > $@
ctx-font-emoji.h: ctx-fontgen
	./ctx-fontgen ttf/DejaVuSans.ttf emoji emoji > $@

%: %.c ../ctx.h
	gcc $< -o $@ -lm -I.. -I../fonts
