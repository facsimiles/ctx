all: mic speaker

mic: mic.c Makefile
	gcc mic.c -o mic -g -Wall -Wextra -Wno-unused-parameter 
speaker: speaker.c Makefile
	gcc speaker.c -o speaker -g -Wall -Wextra -Wno-unused-parameter
clean:
	rm mic speaker -f
