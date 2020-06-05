#  To add more tests, drop in new .ctx files

TEST_SVG = $(wildcard *.svg)
TEST_CTX = $(wildcard *.ctx)
TEST_NAMES  = $(patsubst %.ctx, %, $(TEST_CTX)) $(patsubst %.svg, %, $(TEST_SVG))
TEST_GRAY1= $(patsubst %, gray1/%, $(TEST_NAMES))
TEST_GRAY8= $(patsubst %, gray8/%, $(TEST_NAMES))
TEST_PNG  = $(patsubst %, png/%.png, $(TEST_NAMES))
TEST_PPNG  = $(patsubst %, png-80x160/%.png, $(TEST_NAMES))
TEST_LPNG  = $(patsubst %, png-640x480/%.png, $(TEST_NAMES))
TEST_LBPNG  = $(patsubst %, png-640x480b/%.png, $(TEST_NAMES))

TESTS=$(TEST_GRAY1) $(TEST_PNG) $(TEST_GRAY8)

all: pre index.html $(TESTS) check
png: $(TEST_LPNG)  $(TEST_PPNG)  $(TEST_LBPNG)

pre:
	@mkdir -p png gray1 gray8 reference/gray1 reference/gray8 png-640x480 png-640x480b png-80x160


webupdate:
	cat index.html | sed 's/.*script.*//' > tmp
	mv tmp index.html
	cp -R * ~/pgo/ctx/tests

update-reference:
	cp gray1/* reference/gray1
	cp gray8/* reference/gray8

check: 
	@for a in $(TEST_GRAY1) $(TEST_GRAY8); do \
	  diff -u reference/$$a $$a ; true;  \
	done

index.html: $(TEST_CTX) $(TEST_SVG) Makefile
	echo "<html><head><title></title>" > $@
	echo "<style>td{vertical-align:top;}" >> $@
	echo "img{image-rendering: -moz-crisp-edges; \
	image-rendering:   -o-crisp-edges;        \
    image-rendering: -webkit-optimize-contrast;\
    image-rendering: crisp-edges;\
    -ms-interpolation-mode: nearest-neighbor;  }" >> $@
	echo "img.side{height:40vh;}" >> $@
	echo "img.thumb{height:10vh;}" >> $@
	echo "pre{height:40vh;width:50vw; overflow:scroll}" >> $@
	echo "</style>" >> $@
	echo "<script>setTimeout(function(){location.reload();},8000);</script>" >> $@
	echo "</head>" >> $@ 
	echo "<body>" >> $@ 
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
	  echo "<td><a href='png-80x160/$$a.png'><img class='side' src='png-80x160/$$a.png'/></a></td>" >> $@; \
	  echo "<td><a href='png-640x480/$$a.png'><img class='side' src='png-640x480/$$a.png'/></a>" >> $@; \
	  echo "<td><a href='png-640x480b/$$a.png'><img class='side' src='png-640x480b/$$a.png'/></a>" >> $@; \
	  echo "</td></tr>" >> $@; \
	done
	echo "</table>" >> $@
	echo "</body>" >> $@ 


clean:
	rm -rf gray1 png gray8 png-640x480 png-640x480b

gray1/%: %.ctx ../ctx 
	../ctx $< --width 160 --height 80 GRAY1 > $@
png/%.png: %.ctx ../ctx 
	../ctx $< $@ --width 160 --height 80
png/%.png: %.svg ../ctx 
	../ctx $< $@ --width 160 --height 80
gray1/%: %.svg ../ctx 
	../ctx $< --width 160 --height 80 GRAY1 > $@
png-80x160/%.png: %.ctx ../ctx 
	../ctx $< $@ --width 80 --height 160 --rows 10
png-80x160/%.png: %.svg ../ctx 
	../ctx $< $@ --width 80 --height 160 --rows 10
png-640x480/%.png: %.ctx ../ctx 
	../ctx $< $@ --width 640 --height 480
png-640x480/%.png: %.svg ../ctx 
	../ctx $< $@ --width 640 --height 480
png-640x480b/%.png: %.ctx ../ctx 
	../ctx $< $@ --rows 24 --width 640 --height 480
png-640x480b/%.png: %.svg ../ctx 
	../ctx $< $@ --rows 24 --width 640 --height 480
gray8/%: %.ctx ../ctx
	../ctx $< GRAY8 --width 80 --height 24 > $@
gray8/%: %.svg ../ctx
	../ctx $< GRAY8 --width 80 --height 24 > $@
