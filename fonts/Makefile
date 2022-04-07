all: ctx-font-ascii.h \
ctx-font-mono.h \
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

GLYPH_COVERAGE=ascii-latin1-extras

CTX_FONTGEN=../tools/ctx-fontgen

TTF_TO_ID=sed -e s/-/_/g -e s/\.ttf// -e 's:.*/::'

clean:
	rm -f *.h  *.ctxf

ctx-font-ascii.ctxf: Makefile
	$(CTX_FONTGEN) ttf/Roboto-Regular.ttf ascii ascii binary > $@
ctx-font-ascii.h: Makefile
	$(CTX_FONTGEN) ttf/Roboto-Regular.ttf ascii ascii > $@
ctx-font-mono.ctxf: Makefile
	$(CTX_FONTGEN) ttf/Cousine-Regular.ttf mono ascii binary > $@
ctx-font-mono.h: Makefile
	$(CTX_FONTGEN) ttf/Cousine-Regular.ttf mono ascii > $@

%.ctxf: /usr/share/fonts/truetype/croscore/%.ttf Makefile
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` $(GLYPH_COVERAGE) binary > $@
%.ctxf: /usr/share/fonts/truetype/crosextra/%.ttf Makefile
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` $(GLYPH_COVERAGE) binary > $@
%.ctxf: ttf/%.ttf Makefile
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` $(GLYPH_COVERAGE) binary > $@

%.h: ttf/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` $(GLYPH_COVERAGE) > $@
%.h: /usr/share/fonts/truetype/croscore/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` $(GLYPH_COVERAGE) > $@
%.h: /usr/share/fonts/truetype/crosextra/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` $(GLYPH_COVERAGE) > $@

%.h: /usr/share/fonts/truetype/roboto/unhinted/RobotoTTF/%.ttf  Makefile $(CTX_FONTGEN)
	$(CTX_FONTGEN) $< `echo $<|$(TTF_TO_ID)` $(GLYPH_COVERAGE) > $@

