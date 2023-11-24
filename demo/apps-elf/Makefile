CC_HOST=@echo 'CC    ' $@ ; gcc

CC=   @echo 'CC    ' $@ ; xtensa-esp32s3-elf-gcc
STRIP=@echo 'STRIP ' $@ ; xtensa-esp32s3-elf-strip

CFLAGS += -I.. -I../.. 
CFLAGS += -Wall -Wextra -Wno-unused-parameter

CFLAGS_HOST = $(CFLAGS) `pkg-config ctx --cflags --libs`
CFLAGS_DEVICE = $(CFLAGS) -nostartfiles -nostdlib -fPIC -shared -e main -fdata-sections -ffunction-sections -Wl,--gc-sections -fvisibility=hidden  

CFLAGS_HOST += -fPIE -fpic -fpie -pie -rdynamic -fPIC -Wl,-E 

CFLAGS_DEVICE += -fsingle-precision-constant -Wdouble-promotion



PROGRAM_C=    $(wildcard *.c)
PROGRAM_HOST= $(patsubst %.c, %, $(PROGRAM_C))
PROGRAM_ELF=  $(patsubst %.c, %.elf, $(PROGRAM_C))

all: native xtnsa
xtnsa: $(PROGRAM_ELF) 
native: $(PROGRAM_HOST)
clean:
	rm $(PROGRAM_HOST) $(PROGRAM_ELF)

../../ctx.h: ../../src/*.[ch]
	make -C ../.. ctx.h

%.elf: %.c Makefile ../../ctx.h
	$(CC) $<  -o $@ $(CFLAGS_DEVICE)
	$(STRIP) $@

%: %.c Makefile ../../ctx.h
	$(CC_HOST) $(CFLAGS_HOST) $< ../ui.c  -o $@ $(CFLAGS_HOST) -lm -DNATIVE
	../elf_clean_pie/elf_clean_pie $@

pop: $(PROGRAM_ELF)
	cp -fv $(PROGRAM_ELF) /media/$(USER)/D*/
	sync
pop_host: $(PROGRAM_HOST)
	for a in $(PROGRAM_HOST);do cp $$a /sd/"$$a".elf;done
