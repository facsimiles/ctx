all: mic speaker

mic: mic.c Makefile
	gcc mic.c -o mic -g -Wall -Wextra -pedantic
speaker: speaker.c Makefile
	gcc speaker.c -o speaker -g -Wall -Wextra
clean:
	rm mic speaker -f
