PROGRAMS=lua kilo picoc quickjs

all: 
	for a in $(PROGRAMS);do \
	make -C $$a; done
clean: 
	rm -f */native/* */xtensa/* */rv32/*
