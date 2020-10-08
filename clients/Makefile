CFLAGS=-Os -march=x86-64 -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program -fno-math-errno -ffast-math -fsingle-precision-constant -g
#SMALL_CFLAGS=-Ofast -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program 

PROGRAMS=img clock test ui

all: $(PROGRAMS)
clean:
	rm -rf $(PROGRAMS) clock.static

%: %.c ../*.h ../fonts/*.h Makefile ../ctx.o
	$(CC) -g $< -o $@ -I.. -I../fonts ../ctx.o `pkg-config sdl2 --cflags --libs` -lm  -lpthread

clock.static: clock.static.c ../*.h ../fonts/*.h Makefile
	$(CC) -g $< --static -o $@ -I.. -I../fonts -lm 

../ctx.o: ../ctx.h
	make -C .. ctx.o
