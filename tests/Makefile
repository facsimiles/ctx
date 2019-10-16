TESTS_C=$(wildcard *.c)
TESTS= $(patsubst %.c, %, $(TESTS_C))

all: $(TESTS)
test: all
	for a in $(TESTS); do echo $$a;./$$a;done
clean:
	rm -f $(TESTS)
%: %.c ../ctx.h
	gcc $< -g -o $@ -lm -I.. -I../fonts
