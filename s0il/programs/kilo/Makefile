CFLAGS+= -I.. -I../.. -I../../../
include ../../common.mak

TARGETS=kilo.xtensa kilo.native kilo.rv32

all: $(TARGETS)

SOURCES=kilo.c

PROGRAM=kilo

$(PROGRAM).native: $(SOURCES)
	$(CC_NATIVE) $(CFLAGS_NATIVE) $? -o $@ -I. -Iinclude \
            -ls0il `pkg-config ctx --libs --cflags`
	../../elf_strip_pie $@

$(PROGRAM).xtensa: $(SOURCES)
	$(CC_XTENSA) $(CFLAGS_XTENSA) $? -o $@ -I. -Iinclude
	$(POST_XTENSA) $@

$(PROGRAM).rv32: $(SOURCES)
	$(CC_RISCV) $(CFLAGS_RISCV) $? -o $@ -I. -Iinclude
	$(POST_RISCV) $@

clean:
	rm -f $(TARGETS)

