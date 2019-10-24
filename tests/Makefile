TESTS_C=$(wildcard *.c)
TESTS= $(patsubst %.c, %, $(TESTS_C))

check: test-output
	@diff -u24 test-reference test-output && echo PASSED || echo FAILED
	ls -slab test-size
test-output: $(TESTS)
	@for a in $(TESTS); do echo $$a;./$$a;done > test-output
clean:
	rm -f $(TESTS) test-output
%: %.c ../ctx.h ../fonts/*.h
	gcc $< -g -o $@ -lm -I.. -I../fonts -O0
test-size: test-size.c ../ctx.h ../fonts/*.h
	gcc $< -g -o $@ -lm -I.. -I../fonts -Os
	strip -s test-size
