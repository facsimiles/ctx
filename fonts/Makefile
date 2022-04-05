all: ctx-font-ascii.h \
Arimo-Bold.h \
Arimo-BoldItalic.h \
Arimo-Italic.h \
Arimo-Regular.h \
Cousine-Bold.h \
Cousine-BoldItalic.h \
Cousine-Italic.h \
Cousine-Regular.h \
Tinos-Bold.h \
Tinos-BoldItalic.h \
Tinos-Italic.h \
Tinos-Regular.h

extra: \
Caladea-Bold.h \
Caladea-BoldItalic.h \
Caladea-Italic.h \
Caladea-Regular.h \
Carlito-Bold.h \
Carlito-BoldItalic.h \
Carlito-Italic.h \
Carlito-Regular.h


clean:
	rm -f *.h 

ctxf/ascii.ctxf: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Arimo-Regular.ttf regular ascii binary > $@
ctx-font-ascii.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Arimo-Regular.ttf regular ascii > $@

%.h: /usr/share/fonts/truetype/croscore/%.ttf  Makefile
	../tools/ctx-fontgen $< `echo $<|sed s/-/_/g|sed s/\.ttf// | sed 's:.*/::'`  ascii-extras > $@
%.h: /usr/share/fonts/truetype/crosextra/%.ttf  Makefile
	../tools/ctx-fontgen $< `echo $<|sed s/-/_/g|sed s/\.ttf// | sed 's:.*/::'`  ascii-extras > $@
