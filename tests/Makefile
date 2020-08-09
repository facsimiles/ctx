#  To add more tests, drop in new .ctx files

TEST_SVG = $(wildcard *.svg)
TEST_CTX = $(wildcard *.ctx)
TEST_NAMES  = $(patsubst %.ctx, %, $(TEST_CTX)) $(patsubst %.svg, %, $(TEST_SVG))
TEST_GRAY1= $(patsubst %, gray1/%, $(TEST_NAMES))
TEST_GRAY8= $(patsubst %, gray8/%, $(TEST_NAMES))
TEST_PNG  = $(patsubst %, png/%.png, $(TEST_NAMES))
TEST_PPNG  = $(patsubst %, png-80x160/%.png, $(TEST_NAMES))
TEST_LPNG  = $(patsubst %, png-640x240/%.png, $(TEST_NAMES))
TEST_LVGAPNG  = $(patsubst %, png-320x240/%.png, $(TEST_NAMES))
TEST_LBPNG  = $(patsubst %, png-480x640/%.png, $(TEST_NAMES))

TESTS=$(TEST_GRAY1) $(TEST_PNG) $(TEST_GRAY8)

all: pre index.html $(TESTS) check
png: $(TEST_LPNG)  $(TEST_PPNG)  $(TEST_LBPNG) $(TEST_LVGAPNG)

pre:
	@mkdir -p png gray1 gray8 reference/gray1 reference/gray8 png-640x240 png-480x640 png-80x160 png-320x240


webupdate:
	cat index.html | sed 's/.*script.*//' > tmp
	mv tmp index.html
	cp -R * ~/pgo/ctx.graphics/tests

update-reference:
	cp gray1/* reference/gray1
	cp gray8/* reference/gray8

check: 
	@for a in $(TEST_GRAY1) $(TEST_GRAY8); do \
	  diff -u reference/$$a $$a ; true;  \
	done

index.html: $(TEST_CTX) $(TEST_SVG) Makefile ../ctx.h
	echo "<html><head><title></title>" > $@
	echo '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>' >> $@
	echo "<style>@import url('../ctx.css');td{vertical-align:top;}" >> $@
	echo "img{image-rendering: -moz-crisp-edges; \
	image-rendering:   -o-crisp-edges;        \
    image-rendering: -webkit-optimize-contrast;\
    image-rendering: crisp-edges;\
    -ms-interpolation-mode: nearest-neighbor;  }" >> $@
	echo "img.side{height:40vh;}" >> $@
	echo "img.thumb{height:10vh;}" >> $@
	echo "pre{height:40vh;width:20em; overflow:scroll}" >> $@
	echo "</style>" >> $@
	echo "<script>setTimeout(function(){location.reload();},8000);</script>" >> $@
	echo "</head>" >> $@ 
	echo "<body>" >> $@ 
	cat ../subpagemenu.txt >> $@
	echo "<div id='root'><div id='page'><div class='content'>" >> $@ 
	echo "<p><a href='https://pippin.gimp.org/ctx/'>ctx</a> renderings tests, the ones where the source entry are blank are renderings of SVG/HTML/CSS files using parsers based on <a href='https://github.com/hodefoting/mrg'>microraptor gui</a>.</p>, the SVG and HTML missteps are not the fault of ctx, but provide additional surface for monitoring regressions and advances in ctx." >> $@ 
	echo "<div id='index'>" >> $@ 
	for a in $(TEST_NAMES); do \
	  echo "<a href='#$$a'><img class='thumb' src='png/$$a.png'></a>" >> $@; \
	done ;\
	echo "</div>" >> $@ 
	echo "<table>" >> $@ 
	for a in $(TEST_NAMES); do \
	  echo "<tr id='$$a'>" >> $@;\
	  echo "<td>$$a</td>" >> $@;\
	  echo "<td>" >> $@;\
	  echo "<a href='gray1/$$a'>1bit</a> |" >> $@;\
	  echo "<a href='gray8/$$a'>8bit</a>" >> $@;\
	  echo "<a style='float:right' href='#index'>⏫</a></td>" >> $@;\
	  echo "</tr>" >> $@;\
	  echo "<tr>" >> $@;\
	  echo "<td><pre>" >> $@; \
	  test -f $$a.ctx && cat $$a.ctx >> $@; \
	  echo "</pre></td>" >> $@;\
	  echo "<td><a href='png/$$a.png'><img class='side' src='png/$$a.png'/></a></td>" >> $@; \
	  echo "<td><a href='png-320x240/$$a.png'><img class='side' src='png-320x240/$$a.png'/></a></td>" >> $@; \
	  echo "<td><a href='png-80x160/$$a.png'><img class='side' src='png-80x160/$$a.png'/></a></td>" >> $@; \
	  echo "<td><a href='png-640x240/$$a.png'><img class='side' src='png-640x240/$$a.png'/></a>" >> $@; \
	  echo "<td><a href='png-480x640/$$a.png'><img class='side' src='png-480x640/$$a.png'/></a>" >> $@; \
	  echo "</td></tr>" >> $@; \
	done
	echo "</table>" >> $@
	echo "</div></div></div>" >> $@
	echo "</body>" >> $@ 


clean:
	rm -rf gray1 png gray8 png-640x240 png-480x640 png-320x240

gray1/%: %.ctx ../ctx Makefile
	../ctx $< --width 160 --height 80 --rows 6 --cols 23 GRAY1 > $@
png/%.png: %.ctx ../ctx Makefile
	../ctx $< $@ --width 160 --height 80 --cols 31 --rows 8
png/%.png: %.svg ../ctx Makefile
	../ctx $< $@ --width 160 --height 80 --cols 31 --rows 8
gray1/%: %.svg ../ctx Makefile
	../ctx $< --width 160 --height 80 GRAY1 --cols 19 --rows 8 > $@
png-80x160/%.png: %.ctx ../ctx  Makefile
	../ctx $< $@ --width 80 --height 160 --rows 15 --cols 15
png-80x160/%.png: %.svg ../ctx  Makefile
	../ctx $< $@ --width 80 --height 160 --rows 15 --cols 15
png-640x240/%.png: %.ctx ../ctx Makefile
	../ctx $< $@ --width 640 --height 240 --rows 16 --cols 82 
png-640x240/%.png: %.svg ../ctx  Makefile
	../ctx $< $@ --width 640 --height 240 --rows 16 --cols 82

png-320x240/%.png: %.ctx ../ctx  Makefile
	../ctx $< $@ --width 320 --height 240 --rows 32 --cols 81
png-320x240/%.png: %.svg ../ctx  Makefile
	../ctx $< $@ --width 320 --height 240 --rows 32 --cols 81

png-480x640/%.png: %.ctx ../ctx  Makefile
	../ctx $< $@ --width 480 --height 640 --rows 56 --cols 81
png-480x640/%.png: %.svg ../ctx Makefile
	../ctx $< $@ --width 480 --height 640 --rows 56 --cols 81
gray8/%: %.ctx ../ctx Makefile
	../ctx $< GRAY8 --width 80 --height 24 --rows 4 --cols 12 > $@
gray8/%: %.svg ../ctx Makefile
	../ctx $< GRAY8 --width 80 --height 24 --rows 4 --cols 12 > $@
