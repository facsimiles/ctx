C_FILES = $(wildcard *.c)
JS_FILES = $(C_FILES:.c=.js)
HTML_FILES = $(C_FILES:.c=.html)

all: $(HTML_FILES) index.html $(JS_FILES)

%.js: %.c Makefile  ../../ctx.h
	emcc -I../.. -I../../media-handlers  `PKG_CONFIG_PATH=../.. pkg-config ctx-wasm --cflags --libs` $< -DCSS_IMPLEMENTATION   -o $@ -s ASYNCIFY

demo.js: demo.c Makefile  ../../ctx.h
	emcc -I../.. -I../../media-handlers  `PKG_CONFIG_PATH=../.. pkg-config ctx-wasm --cflags --libs` $< -DCSS_IMPLEMENTATION   -o $@ -I../../fonts 

terminal.js: terminal.c Makefile  /usr/local/include/ctx.h
	emcc `pkg-config ctx-wasm --cflags --libs` $< -DCSS_IMPLEMENTATION -I ../../media-handlers  -o $@ 

../../ctx.h: ../../src/*.[ch]
	make -C ../../ ctx.h

%.html: %.c Makefile template.html
	cat template.html | sed -e 's/file/'`echo $<|sed s/\.c//`'/' > $@ 

index.html: Makefile *.c
	echo "<html><body>" > $@
	for a in $(HTML_FILES);do echo "<a href='$$a'>$$a</a><br/>" >> $@;done
	echo "</body></html>" >> $@

clean:
	rm -f $(HTML_FILES) $(JS_FILES) *.wasm
