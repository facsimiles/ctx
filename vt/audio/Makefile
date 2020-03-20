all: atty

atty: atty.c Makefile
	gcc *.c -o atty -g -Wall -Wextra -Wno-unused-parameter -O2  `pkg-config --cflags --libs sdl2` -lz -lutil
clean:
	rm atty -f
