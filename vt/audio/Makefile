all: mic speaker atty

mic: mic.c Makefile
	gcc mic.c -o mic -g -Wall -Wextra -Wno-unused-parameter 
speaker: speaker.c Makefile
	gcc speaker.c -o speaker -g -Wall -Wextra -Wno-unused-parameter
atty: atty.c Makefile
	gcc atty.c -o atty -g -Wall -Wextra -Wno-unused-parameter -O2 
clean:
	rm mic speaker atty -f
