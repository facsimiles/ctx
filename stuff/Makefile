# all is done via gnu make wildcards in the parent folder,
#
# to add another program to build just drop the .c file in
# this folder

all: argvs-commands.inc
	make -C .. ctx.h && make -C .. -j10

argvs-commands.inc: stuff.c
	./argvs_extract.sh $< > $@
