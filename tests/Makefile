TESTS_CTX=$(wildcard *.ctx)
TESTS_GRAY1= $(patsubst %.ctx, %.gray1, $(TESTS_CTX))
TESTS_GRAY4= $(patsubst %.ctx, %.gray4, $(TESTS_CTX))
TESTS_GRAY8= $(patsubst %.ctx, %.gray8, $(TESTS_CTX))

TESTS=$(TESTS_GRAY1) $(TESTS_GRAY4) $(TESTS_GRAY8)

all:  $(TESTS) check

update-reference:
	cp *.gray* reference

check:
	@for a in $(TESTS); do diff -u reference/$$a $$a ; done
clean:
	for a in $(TESTS); do rm -rf $$a;done

%.gray1: %.ctx ../ctx 
	../ctx $< GRAY1 > $@
%.gray4: %.ctx ../ctx
	../ctx $< GRAY4 > $@
%.gray8: %.ctx ../ctx
	../ctx $< GRAY8 > $@


