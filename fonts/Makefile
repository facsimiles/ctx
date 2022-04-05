all: ctx-font-regular.h \
	ctx-font-ascii.h \
	ctx-font-mono.h \
	ctxf/regular.ctxf
extra:
	ctx-font-emoji.h \
	unscii-16.h \
	DejaVuSans.h \
	Vera.h \
	DejaVuSansMono.h \
	\
	ctxf/regular.ctxf \
	ctxf/mono.ctxf \
	ctxf/emoji.ctxf \
	ctxf/ascii.ctxf \
	Roboto-Bold.h \
	Roboto-Light.h \
	Roboto-Regular.h \
	NotoEmoji-Regular.h
	NotoMono-Regular.h


clean:
	rm -f *.h 


ctxf/ascii.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf regular ascii binary > $@
ctx-font-ascii.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Arimo-Regular.ttf regular ascii > $@
ctxf/regular.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf regular ascii-extras binary > $@
ctx-font-regular.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Arimo-Regular.ttf regular ascii-extras > $@
ctx-font-bold.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Arimo-Bold.ttf bold ascii-extras > $@
ctx-font-italic.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Arimo-Italic.ttf italic ascii-extras > $@
ctx-font-bold-italic.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Arimo-BoldItalic.ttf bold_italic ascii-extras > $@

ctxf/mono.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSansMono.ttf mono ascii-extras-cp437-vt100 binary > $@
ctx-font-mono.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Cousine-Regular.ttf mono ascii-extras-cp437-vt100 > $@
ctx-font-mono-bold.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Cousine-Bold.ttf mono_bold ascii-extras-cp437-vt100 > $@
ctx-font-mono-italic.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Cousine-Italic.ttf mono_italic ascii-extras-cp437-vt100 > $@
ctx-font-mono-bold-italic.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Cousine-BoldItalic.ttf mono_bold_italic ascii-extras-cp437-vt100 > $@

ctx-font-serif.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Tinos-Regular.ttf serif ascii-extras-cp437-vt100 > $@
ctx-font-serif-bold.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Tinos-Bold.ttf serif_bold ascii-extras-cp437-vt100 > $@
ctx-font-serif-italic.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Tinos-Italic.ttf serif_italic ascii-extras-cp437-vt100 > $@
ctx-font-serif-bold-italic.h: Makefile
	../tools/ctx-fontgen /usr/share/fonts/truetype/croscore/Tinos-BoldItalic.ttf serif_bold_italic ascii-extras-cp437-vt100 > $@

 

ctxf/emoji.ctxf: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf emoji emoji binary > $@
ctx-font-emoji.h: Makefile
	../tools/ctx-fontgen ttf/DejaVuSans.ttf emoji emoji > $@
 
%.h: /usr/share/fonts/truetype/gentium/%.ttf  Makefile
	../tools/ctx-fontgen $< `echo $<|sed s/-/_/g|sed s/\.ttf// | sed 's:.*/::'`  ascii-extras > $@
%.h: /usr/share/fonts/truetype/croscore/%.ttf  Makefile
	../tools/ctx-fontgen $< `echo $<|sed s/-/_/g|sed s/\.ttf// | sed 's:.*/::'`  ascii-extras > $@
%.h: /usr/share/fonts/truetype/crosextra/%.ttf  Makefile
	../tools/ctx-fontgen $< `echo $<|sed s/-/_/g|sed s/\.ttf// | sed 's:.*/::'`  ascii-extras > $@


%: %.c ../ctx.h
	gcc $< -o $@ -lm -I.. -I../fonts
