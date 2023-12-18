DESTDIR ?=
PREFIX  ?= /usr/local

#CCACHE=`command -v ccache`
CCACHE=

CFLAGS=-Wall -Wextra -Wno-unused-parameter -g -Wno-deprecated-declarations
CFLAGS+= -I. -I..
CFLAGS+= -fsingle-precision-constant -Wdouble-promotion

CC_NATIVE=@   echo 'CC     ' $@ ; $(CCACHE) $(CC)
CFLAGS_NATIVE = $(CFLAGS) `pkg-config ctx --cflags --libs` \
                   -g -lm -DNATIVE -fPIC -rdynamic -fpic \
                   -Wl,-E,--unresolved-symbols=ignore-all

POST_NATIVE=@ echo 'rem-PIE' $@; ./elf_strip_pie $@
#POST_NATIVE=@echo 'rem-PIE  $@'; ./elf_strip_pie $@ && strip $@

CC_XTENSA=@   echo 'CC     ' $@ ; $(CCACHE) xtensa-esp32s3-elf-gcc 
#CC_NATIVE=$(CC)
POST_XTENSA=@echo 'STRIP  ' $@ ; xtensa-esp32s3-elf-strip 
#POST_XTENSA=@ echo 'STRIP  ' $@ ; echo
#--remove-section=.xtensa.info --remove-section=.comment

CFLAGS_XTENSA = $(CFLAGS) -nostartfiles -nostdlib -fPIC -shared -e main \
  -fdata-sections -ffunction-sections -Wl,--gc-sections -fvisibility=hidden \
  -Wno-sign-compare \
  -DCTX_FLOW3R -DCTX_ESP \
  \
  -Iflow3r/build/config \
  -I$(IDF_PATH)/components/lwip/include \
  -I$(IDF_PATH)/components/lwip/lwip/src/include \
  -I$(IDF_PATH)/components/lwip/port/include \
  -I$(IDF_PATH)/components/lwip/port/freertos/include \
  -I$(IDF_PATH)/components/lwip/port/esp32xx/include \
  -I$(IDF_PATH)/components/newlib/platform_include \
  -I$(IDF_PATH)/components/esp_system/include \
  -I$(IDF_PATH)/components/esp_common/include \
  -I$(IDF_PATH)/components/heap/include \
  -I$(IDF_PATH)/components/esp_rom/include \
  -I$(IDF_PATH)/components/esp_timer/include \
  -I$(IDF_PATH)/components/xtensa/include \
  -I$(IDF_PATH)/components/esp_hw_support/include \
  -I$(IDF_PATH)/components/soc/esp32s3/include \
  -I$(IDF_PATH)/components/xtensa/esp32s3/include \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/ \
  -I$(IDF_PATH)/components/freertos/esp_additions/include/freertos \
  -I$(IDF_PATH)/components/freertos/esp_additions/arch/xtensa/include \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/ \
  -I$(IDF_PATH)/components/freertos/FreeRTOS-Kernel/include \

native: native_bins s0il
xtensa: xtensa_bins

all: native xtensa flow3r-build wasm riscv
uninstall:
install: native
	rm -f $(DESTDIR)/sd
	ln -sf `pwd`/sd $(DESTDIR)/sd
	install -D -m755 -t $(DESTDIR)$(PREFIX)/bin s0il
	install -D -m755 -t $(DESTDIR)$(PREFIX)/lib libs0il.so
	install -D -m644 -t $(DESTDIR)$(PREFIX)/include/s0il s0il.h
	install -D -m644 -t $(DESTDIR)$(PREFIX)/include/s0il s0il-clib.h
uninstall:
	rm -f $(DESTDIR)/sd
	rm -rf $(DESTDIR)$(PREFIX)/bin/s0il
	rm -rf $(DESTDIR)$(PREFIX)/lib/libs0il.so
	rm -rf $(DESTDIR)$(PREFIX)/include/s0il

test: s0il native_bins
	LD_LIBRARY_PATH=.:$(LD_LIBRARY_PATH) ./s0il

S0IL_OBJS=ui.o s0il-run.o s0il-symbols.o s0il-magic.o s0il-clib.o

s0il: libs0il.so main.o bundled-programs.o
	$(CC_NATIVE) $(CFLAGS_NATIVE) -lm $^ -o $@ $(CFLAGS_NATIVE) -L. -ls0il

PROGRAMS_C_BUNDLED=programs/*.c
bundled-programs.o: $(PROGRAMS_C_BUNDLED)

main.o: fs_bin_native.c

%.o: %.c *.h Makefile
	$(CC_NATIVE) $(CFLAGS_NATIVE) -c -g $< -o $@

libs0il.so: $(S0IL_OBJS)  Makefile 
	$(CC_NATIVE) $(CFLAGS_NATIVE) -g -shared \
        $(S0IL_OBJS)  -o libs0il.so

clean:
	rm -rf s0il libs0il.so bin-xtensa bin-native elf_strip_pie *.o tmpfs tmpfs2 fs_bin_*.c


elf_strip_pie: elf_strip_pie.c
	$(CC_NATIVE) $< -o $@
PROGRAMS_C = $(wildcard programs/*.c)
PROGRAMS_PICOC = $(wildcard programs-picoc/*)
PROGRAMS_NATIVE = $(patsubst programs/%.c, bin-native/%, $(PROGRAMS_C)) \
                 $(patsubst programs-picoc/%, bin-native/%, $(PROGRAMS_PICOC))


PROGRAMS_XTENSA = $(patsubst programs/%.c, bin-xtensa/%, $(PROGRAMS_C)) \
                 $(patsubst programs-picoc/%, bin-xtensa/%, $(PROGRAMS_PICOC))

bin-native/%: programs/%.c ui.h Makefile elf_strip_pie libs0il.so
	@mkdir -p `dirname $@` || true
	$(CC_NATIVE) $< -L. -ls0il -o $@ $(CFLAGS_NATIVE) -DS0IL_REDEFINE_CLIB
	@echo 'rem-PIE $@'; ./elf_strip_pie $@
	$(POST_NATIVE) $@

bin-xtensa/%: programs/%.c ui.h Makefile
	@mkdir -p `dirname $@` || true
	$(CC_XTENSA) $(CFLAGS_XTENSA) $< -o $@
	$(POST_XTENSA) $@

bin-native/%: programs-picoc/%
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true
bin-xtensa/%: programs-picoc/%
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true

native_bins: $(PROGRAMS_NATIVE)
xtensa_bins: $(PROGRAMS_XTENSA)

flow3r-build: fs_bin_xtensa.c ui.c ui.h s0il*.[ch]
	(cd flow3r; idf.py build)
flow3r-flash: flow3r-build
	(cd flow3r; idf.py app-flash monitor)
monitor:
	(cd flow3r; idf.py monitor)

wasm: fs_bin_generic.c ui*.[ch] s0il*.[ch] */*.c
	(cd wasm; make)
riscv-flash: riscv
	(cd esp32c3; idf.py flash monitor)
riscv: fs_bin_generic.c ui*.[ch] s0il*.[ch] */*.c
	(cd esp32c3; idf.py build)

fs_bin_xtensa.c: $(PROGRAMS_XTENSA) Makefile gen_fs.sh
	@rm -rf $@_fs
	@mkdir $@_fs
	@cp bin-xtensa/* $@_fs
	@rm $@_fs/image $@_fs/text $@_fs/clock
	./gen_fs.sh $@_fs bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm -rf $@_ $@_fs

fs_bin_native.c: $(PROGRAMS_NATIVE) Makefile gen_fs.sh
	@rm -rf $@_fs
	@mkdir $@_fs
	@cp bin-native/* $@_fs
	./gen_fs.sh $@_fs bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm -rf $@_ $@_fs

fs_bin_generic.c: programs-picoc/* Makefile gen_fs.sh
	./gen_fs.sh programs-picoc bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm $@_
