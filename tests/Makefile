#  To add more tests, drop in new .ctx files

TESTS_CTX  =$(wildcard *.ctx)
TEST_NAMES  = $(patsubst %.ctx, %, $(TESTS_CTX))
TESTS_GRAY1= $(patsubst %.ctx, gray1/%, $(TESTS_CTX))
TESTS_GRAY8= $(patsubst %.ctx, gray8/%, $(TESTS_CTX))
TESTS_PNG  = $(patsubst %.ctx, png/%.png, $(TESTS_CTX))
TESTS_LPNG  = $(patsubst %.ctx, png-640x480/%.png, $(TESTS_CTX))
TESTS_LBPNG  = $(patsubst %.ctx, png-640x480b/%.png, $(TESTS_CTX))

TESTS=$(TESTS_GRAY1) $(TESTS_PNG) $(TESTS_GRAY8)  $(TESTS_LPNG) $(TESTS_LBPNG)

all: index.html pre $(TESTS) check

pre:
	@mkdir -p png gray1 gray8 reference/gray1 reference/gray8 png-640x480 png-640x480b


webupdate:
	cat index.html | sed 's/.*script.*//' > tmp
	mv tmp index.html
	cp -R * ~/pgo/ctx/tests

update-reference:
	cp gray1/* reference/gray1
	cp gray8/* reference/gray8

check: 
	@for a in $(TESTS_GRAY1) $(TESTS_GRAY8); do diff -u reference/$$a $$a && echo $$a OK; done

index.html: $(TESTS_CTX) Makefile
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
	  cat $$a.ctx >> $@; \
	  echo "</pre></td>" >> $@;\
	  echo "<td><a href='png/$$a.png'><img class='side' src='png/$$a.png'/></a></td>" >> $@; \
	  echo "<td><a href='png-640x480/$$a.png'><img class='side' src='png-640x480/$$a.png'/></a>" >> $@; \
	  echo "<td><a href='png-640x480b/$$a.png'><img class='side' src='png-640x480b/$$a.png'/></a>" >> $@; \
	  echo "</td></tr>" >> $@; \
	done
	echo "</table>" >> $@
	echo "</body>" >> $@ 


clean:
	rm -rf gray1 png gray8 png-640x480 png-640x480b

gray1/%: %.ctx ../ctx 
	../ctx $< GRAY1 > $@
png/%.png: %.ctx ../ctx 
	../ctx $< $@
png-640x480/%.png: %.ctx ../ctx 
	../ctx $< $@ --width 640 --height 480
png-640x480b/%.png: %.ctx ../ctx 
	../ctx $< $@ --rows 24 --width 640 --height 480
gray8/%: %.ctx ../ctx
	../ctx $< GRAY8 > $@
