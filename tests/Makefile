TESTS_C=$(wildcard *.c)
TESTS= $(patsubst %.c, %, $(TESTS_C))

all: $(TESTS)

check: test-output
	@diff -u24 test-reference test-output && echo PASSED || echo FAILED
test-output: all
	@for a in $(TESTS); do echo $$a;./$$a;done > test-output
clean:
	rm -f $(TESTS) test-output
%: %.c ../ctx.h
	gcc $< -g -o $@ -lm -I.. -I../fonts
