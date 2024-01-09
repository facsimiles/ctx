C_FILES = ../main.c ../s0il.c ctx.c ../bundled-programs-wasm.c 

include ../common.mak

#CFLAGS_WASM+= -s EXPORTED_FUNCTIONS=_main,_free,_calloc,_malloc,_wasm_http 

s0il.js: $(C_FILES) ../bin-src/*.c Makefile  ../../ctx.h ../fs_bin_wasm.c
	$(CC_WASM) $(C_FILES) -I../.. $(CFLAGS_WASM) -O3 -lidbfs.js -o $@ -I../../fonts -s ASYNCIFY -I.. -I../../deps -sMAIN_MODULE=1 

../fs_bin_generic.c: ../bin-generic/*
	make -C .. fs_bin_generic.c

../fs_bin_wasm.c: ../bin-wasm/*
	make -C .. fs_bin_wasm.c

../../ctx.h: ../../src/*.[ch]
	make -C ../../ ctx.h

clean:
	rm -f s0il*.js *.wasm
