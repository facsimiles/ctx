C_FILES = ../main.c ../s0il.c ctx.c ../bundled-programs-wasm.c 


CFLAGS=-I/usr/local/include -flto -Os -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=_main,_free,_calloc,_malloc -Wl,--lto-O3 -DWASM -DCTX \
  -lidbfs.js

s0il.js: $(C_FILES) ../bin-src/*.c Makefile  ../../ctx.h ../fs_bin_generic.c
	emcc $(C_FILES) -I../.. $(CFLAGS) -o $@ -I../../fonts -s ASYNCIFY -I.. -I../../deps

../fs_bin_generic.c: ../bin-generic/*
	make -C .. fs_bin_generic.c

../../ctx.h: ../../src/*.[ch]
	make -C ../../ ctx.h

clean:
	rm -f *.js *.wasm
