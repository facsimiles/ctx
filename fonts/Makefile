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
Tinos-Regular.h \
Caladea-Bold.h \
Caladea-BoldItalic.h \
Caladea-Italic.h \
Caladea-Regular.h \
Carlito-Bold.h \
Carlito-BoldItalic.h \
Carlito-Italic.h \
Carlito-Regular.h \
Roboto-Regular.h \
Roboto-Bold.h \
Roboto-Thin.h \
Roboto-Italic.h \
Roboto-BoldItalic.h 



CTX_FONTGEN=../tools/ctx-fontgen

TTF_TO_ID=sed -e s/-/_/g -e s/\.ttf// -e 's:.*/::'

clean:
	rm -f *.h 

ctxf/ascii.ctxf: Makefile
	$(CTX_FONTGEN) /usr/share/fonts/truetype/croscore/Arimo-Regular.ttf regular ascii binary > $@
ctxf/regular.ctxf: Makefile
	$(CTX_FONTGEN) /usr/share/fonts/truetype/croscore/Arimo-Regular.ttf regular ascii-extras binary > $@
ctx-font-ascii.h: Makefile
	$(CTX_FONTGEN) /usr/share/fonts/truetype/croscore/Arimo-Regular.ttf ascii ascii > $@

%.h: ttf/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` ascii-extras > $@
%.h: /usr/share/fonts/truetype/croscore/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` ascii-extras > $@
%.h: /usr/share/fonts/truetype/crosextra/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` ascii-extras > $@

%.h: /usr/share/fonts/truetype/roboto/unhinted/RobotoTTF/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` ascii-extras > $@


