all: ctx-fontgen ctx-info ctx-info-32bit

clean:
	rm -f ctx-fontgen  ctx-info-32bit

%: %.c ../ctx.h
	ccache gcc $< -o $@ -lm -I.. -I../fonts


ctx-info-32bit: ctx-info.c ../ctx.h
	ccache i686-linux-gnu-gcc $< -o $@ -lm -I.. -I../fonts
