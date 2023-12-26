PROGRAM=lua
PROGRAM_B=luac
SOURCES_COMMON=\
lapi.c  \
lcode.c  \
lctype.c  \
ldebug.c  \
ldo.c  \
ldump.c  \
lfunc.c  \
lgc.c  \
llex.c  \
lmem.c  \
lobject.c  \
lopcodes.c  \
lparser.c  \
lstate.c  \
lstring.c  \
ltable.c  \
ltm.c  \
lundump.c  \
lvm.c  \
lzio.c  \
lauxlib.c  \
lbaselib.c  \
lcorolib.c  \
ldblib.c  \
liolib.c  \
lmathlib.c  \
loadlib.c  \
loslib.c  \
lstrlib.c  \
ltablib.c  \
lutf8lib.c  \
linit.c
SOURCES=$(SOURCES_COMMON) lua.c  
SOURCES_B=$(SOURCES_COMMON) luac.c

CFLAGS+= -I.. -I../.. -I../../../
include ../../common.mak

include ../program.mak
include ../programb.mak
