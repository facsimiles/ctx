PROGRAMS=ctx-fontgen ctx-info ctx-info-32bit

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

%: %.c ../ctx.h
	ccache gcc $< -o $@ -lm -I.. -I../fonts -Wall -lm

%-32bit: %.c ../ctx.h
	ccache i686-linux-gnu-gcc $< -o $@ -lm -I.. -I../fonts
