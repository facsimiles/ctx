#  To add more tests, drop in new .ctx files

TESTS_CTX  =$(wildcard *.ctx)
TESTS_GRAY1= $(patsubst %.ctx, gray1/%.gray1, $(TESTS_CTX))
TESTS_GRAY8= $(patsubst %.ctx, gray8/%.gray8, $(TESTS_CTX))
TESTS_PNG  = $(patsubst %.ctx, png/%.png, $(TESTS_CTX))

TESTS=$(TESTS_GRAY1) $(TESTS_PNG) $(TESTS_GRAY8)

all: pre $(TESTS) check

pre:
	@mkdir -p png gray1 gray8 reference/gray1 reference/gray8

update-reference:
	cp gray1/* reference/gray1
	cp gray8/* reference/gray8

check: 
	@for a in $(TESTS_GRAY1) $(TESTS_GRAY8); do diff -u reference/$$a $$a && echo $$a OK; done
clean:
	rm -rf gray1 png gray8

gray1/%.gray1: %.ctx ../ctx 
	../ctx $< GRAY1 > $@
png/%.png: %.ctx ../ctx 
	../ctx $< $@
gray8/%.gray8: %.ctx ../ctx
	../ctx $< GRAY8 > $@
