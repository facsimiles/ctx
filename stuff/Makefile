# all is done via gnu make wildcards in the parent folder,
#
# to add another program to build just drop the .c file in
# this folder

all:
	make -C .. ctx.h && make -C .. -j10
test: all
	./demo
