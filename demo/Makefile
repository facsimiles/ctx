test: demo
	./demo
%: %.c *.h Makefile
	gcc $< `pkg-config --libs --cflags ctx libssh2` -lm -o $@ -Wall
clean:
	rm demo
