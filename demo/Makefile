CFLAGS=-Wall -Wextra -Wno-unused-parameter

test: demo
	./demo
%: %.c *.c *.h Makefile apps/*.c
	gcc -rdynamic *.c `pkg-config --libs --cflags ctx libssh2` -lm -o $@ $(CFLAGS) -I.
clean:
	rm demo
