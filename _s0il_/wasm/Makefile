C_FILES = ../ui.c ../main.c ../magic.c ctx.c ../run.c ../run-symbols.c ../bundled-programs-wasm.c ../picoc/*.c ../picoc/platform/*.c ../picoc/cstdlib/*.c
#$(wildcard *.c)
JS_FILES = s0il.js
HTML_FILES = s0il.html

all: index.html s0il.js

CFLAGS=-I/usr/local/include -flto -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS=_main,_free,_calloc,_malloc -Wl,--lto-O3 -DWASM -DCTX


s0il.js: $(C_FILES) ../programs/*.c Makefile  ../../ctx.h
	emcc $(C_FILES) -I../.. $(CFLAGS) -o $@ -I../../fonts -s ASYNCIFY -I..

../../ctx.h: ../../src/*.[ch]
	make -C ../../ ctx.h

index.html: Makefile *.c
	echo "<html><body>" > $@
	for a in $(HTML_FILES);do echo "<a href='$$a'>$$a</a><br/>" >> $@;done
	echo "</body></html>" >> $@

clean:
	rm -f $(HTML_FILES) $(JS_FILES) *.wasm index.html
