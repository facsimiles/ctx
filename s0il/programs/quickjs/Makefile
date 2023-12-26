PROGRAM=qjs
SOURCES=\
cutils.c \
libbf.c \
libregexp.c \
libunicode.c \
quickjs-libc.c \
quickjs.c \
qjs.c \
qjscalc.c \
repl.c
  
CFLAGS+= -I.. -I../.. -I../../../ -DCONFIG_BIGNUM -DCONFIG_VERSION=\"s0il\"
include ../../common.mak
include ../program.mak
