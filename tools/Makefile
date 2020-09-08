PROGRAMS=ctx-fontgen ctx-info ctx-info-32bit

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

%: %.c ../ctx.h
	gcc $< -o $@ -lm -I.. -I../fonts -Wall -lm -I../deps

%-32bit: %.c ../ctx.h
	i686-linux-gnu-gcc $< -o $@ -lm -I.. -I../fonts  -I../deps
