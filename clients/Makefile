CFLAGS=-Os -march=x86-64 -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program -fno-math-errno -ffast-math -fsingle-precision-constant -g
#SMALL_CFLAGS=-Ofast -fomit-frame-pointer -ffunction-sections -fdata-sections -Wl,--gc-sections  -Wl,--hash-style=gnu -flto -fmerge-all-constants -fgcse-sm -fgcse-las -fwhole-program 

PROGRAMS=img clock test ui clock2

all: $(PROGRAMS)
clean:
	rm -rf $(PROGRAMS)

%: %.c ../*.h ../fonts/*.h Makefile ../ctx.o
	$(CC) -g $< -o $@ -I.. -I../fonts ../ctx.o `pkg-config sdl2 --cflags --libs` -lm  -lpthread

clock2: clock2.c ../*.h ../fonts/*.h Makefile
	$(CC) -g $< --static -o $@ -I.. -I../fonts -lm 

../ctx.o: ../ctx.h
	make -C .. ctx.o
