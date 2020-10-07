PROGRAMS=ctx-fontgen ctx-info

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

%: %.c ../ctx.h ../test-size/tiny-config.h
	gcc $< -o $@ -lm -I.. -I../fonts -Wall -lm -I../deps

%-32bit: %.c ../ctx.h ../test-size/tiny-config.h
	i686-linux-gnu-gcc $< -o $@ -lm -I.. -I../fonts  -I../deps
