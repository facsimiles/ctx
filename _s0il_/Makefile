CFLAGS=-Wall -Wextra -Wno-unused-parameter -g -Wno-deprecated-declarations

CFLAGS+= -I. -I..
CFLAGS+= -fsingle-precision-constant -Wdouble-promotion
CFLAGS_DEVICE = $(CFLAGS) -nostartfiles -nostdlib -fPIC -shared -e main \
  -fdata-sections -ffunction-sections -Wl,--gc-sections -fvisibility=hidden \
  -Wno-sign-compare \
  -DCTX_FLOW3R \
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

CFLAGS_NATIVE = $(CFLAGS) `pkg-config ctx --cflags --libs` \
                   -g -lm -DNATIVE -fPIC -rdynamic -fpic \
                   -Wl,-E,--unresolved-symbols=ignore-all


all: native_bins demo device_bins fs_bin_xtensa.c
uninstall:
	@sudo rm -f /sd
install:
	@sudo rm -f /sd
	@sudo ln -sf `pwd`/sd /sd

test: demo native_bins
	rm -f /tmp/_s0il_*
	./demo
demo: main.o ui.o run.o run-symbols.o bundled-programs.o magic.o
	$(CC_NATIVE) $(CFLAGS_NATIVE) -lm $^ -o $@ $(CFLAGS_NATIVE)

PROGRAM_C_INLINED=programs/text.c programs/image.c programs/clock.c
bundled-programs.o: $(PROGRAM_C_INLINED)

main.o: fs_bin_native.c
run-symbols.o: run-stdio.c

%.o: %.c *.h Makefile
	$(CC_NATIVE) $(CFLAGS_NATIVE) -c -g $< -o $@

libui.so: ui.o run.o run-symbols.o magic.o Makefile 
	$(CC_NATIVE) $(CFLAGS_NATIVE) -g -shared ui.o run.o run-symbols.o -o libui.so magic.o

clean:
	rm -rf demo libui.so xtensa-bin native-bin elf_strip_pie *.o

CC_DEVICE=@   echo 'CC     ' $@ ; xtensa-esp32s3-elf-gcc 
CC_NATIVE=@   echo 'CC     ' $@ ; $(CC)
#CC_NATIVE=$(CC)
POST_DEVICE=@echo 'STRIP  ' $@ ; xtensa-esp32s3-elf-strip 
#POST_DEVICE=@ echo 'STRIP  ' $@ ; echo
#--remove-section=.xtensa.info --remove-section=.comment
POST_NATIVE=@ echo 'rem-PIE' $@; ./elf_strip_pie $@
#POST_NATIVE=@echo 'rem-PIE  $@'; ./elf_strip_pie $@ && strip $@

elf_strip_pie: elf_strip_pie.c
	$(CC_NATIVE) $< -o $@
PROGRAM_C = $(wildcard programs/*.c)
PROGRAM_PICOC = $(wildcard programs-picoc/*)
PROGRAM_NATIVE = $(patsubst programs/%.c, native-bin/%, $(PROGRAM_C)) \
                 $(patsubst programs-picoc/%, native-bin/%, $(PROGRAM_PICOC))


PROGRAM_DEVICE = $(patsubst programs/%.c, xtensa-bin/%, $(PROGRAM_C)) \
                 $(patsubst programs-picoc/%, xtensa-bin/%, $(PROGRAM_PICOC))

native-bin/%: programs/%.c ui.h Makefile elf_strip_pie
	@mkdir -p `dirname $@` || true
	$(CC_NATIVE) $< -o $@ $(CFLAGS_NATIVE) -D_RUN_REDEFINES_
	@echo 'rem-PIE $@'; ./elf_strip_pie $@
	$(POST_NATIVE) $@  
native-bin/picoc: picoc/*.c picoc/*/*.c ui.h Makefile elf_strip_pie libui.so
	@mkdir -p `dirname $@` || true
	$(CC_NATIVE) picoc/platform/*.c picoc/cstdlib/*.c picoc/*.c ./libui.so\
           -o $@ $(CFLAGS_NATIVE) -Wno-comment -DCTX -Ipicoc -D_RUN_REDEFINES_
	$(POST_NATIVE) $@ 

xtensa-bin/%: programs/%.c ui.h Makefile
	@mkdir -p `dirname $@` || true
	$(CC_DEVICE) $(CFLAGS_DEVICE) $< -o $@
	$(POST_DEVICE) $@
xtensa-bin/picoc: picoc/*.c picoc/*/*.c ui.h Makefile
	@mkdir -p `dirname $@` || true
	$(CC_DEVICE) picoc/*.c picoc/platform/*.c picoc/cstdlib/*.c \
          -o $@ $(CFLAGS_DEVICE) -Wno-comment -DCTX -Ipicoc 
	$(POST_DEVICE) $@ 


native-bin/%: programs-picoc/%
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true
xtensa-bin/%: programs-picoc/%
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true


native_bins: $(PROGRAM_NATIVE) native-bin/picoc
device_bins: $(PROGRAM_DEVICE) xtensa-bin/picoc

pop: device_bins
	cp xtensa-bin/* /media/$(USER)/*-*/bin/ -v

flow3r-build: fs_bin_xtensa.c
	(cd flow3r; idf.py build)
flow3r-flash: flow3r-build
	(cd flow3r; idf.py app-flash monitor)
monitor:
	(cd flow3r; idf.py monitor)

wasm:
	(cd wasm; make)

fs_bin_xtensa.c: $(PROGRAM_DEVICE) xtensa-bin/picoc Makefile gen_fs.sh
	@rm -rf tmpfs
	@mkdir tmpfs
	@cp xtensa-bin/* tmpfs
	@rm tmpfs/image tmpfs/text tmpfs/clock  tmpfs/busywarp
	./gen_fs.sh tmpfs bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm $@_

fs_bin_native.c: $(PROGRAM_NATIVE) native-bin/picoc Makefile gen_fs.sh
	@rm -rf tmpfs2
	@mkdir tmpfs2
	@cp native-bin/* tmpfs2
	./gen_fs.sh tmpfs2 bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm $@_

fs_bin_generic.c: programs-picoc/* Makefile gen_fs.sh
	./gen_fs.sh programs-picoc bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm $@_
