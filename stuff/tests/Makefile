# all is done via gnu make wildcards in the parent folder,
#
# to add another program to build just drop the .c file in
# this folder

TESTS=$(wildcard *.stuff)
OUTPUTS=$(patsubst %.stuff, output/%, $(TESTS))

all: verify
../../ctx: ../../src/*.[ch] ../*.[ch]
	make -C ../.. ctx
output/%: %.stuff ../../ctx Makefile
	../../ctx stuffcmd $< > $@

update_reference: $(OUTPUTS)
	mkdir reference || true
	cp output/* reference

verify: $(OUTPUTS)
	sh -c 'diff -u reference/ output/'
