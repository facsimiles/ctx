native: bin-native s0il
help:
	@echo 'targets:'
	@echo '  clean        - not often needed, and itself not complete..'
	@echo '  test         - run s0il locally with a native build'
	@echo '  gdb          - run s0il locally under gdb'
	@echo '  valgrind     - run s0il locally under valgrind'
	@echo '  flow3r       - build flow3r firmware'
	@echo ' flow3r-flash - flash (and build if needed) flow3r firmware'
	@echo ' riscv        - build esp32c3 firmware'
	@echo ' riscv-flash  - flash (and build if needed) esp32c3 firmware'
	@echo ' pico         - build rp2040 firmware'
	@echo ' pico-flash   - flash (and build if needed) rp2040 firmware'

include common.mak

xtensa: bin-xtensa
all: native xtensa flow3r wasm riscv pico programs bin-riscv bin-wasm

DESTDIR ?=
PREFIX  ?= /usr/local


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

sh: s0il bin-native
	LD_LIBRARY_PATH=.:..:$(LD_LIBRARY_PATH) ./s0il sh
test: s0il bin-native
	LD_LIBRARY_PATH=.:..:$(LD_LIBRARY_PATH) ./s0il
gdb: s0il bin-native 
	LD_LIBRARY_PATH=.:..:$(LD_LIBRARY_PATH) gdb ./s0il
valgrind: s0il bin-native 
	LD_LIBRARY_PATH=.:..:$(LD_LIBRARY_PATH) valgrind ./s0il
valgrind-full: s0il bin-native 
	LD_LIBRARY_PATH=.:..:$(LD_LIBRARY_PATH) valgrind --leak-check=full ./s0il

S0IL_OBJS=s0il-ui.o s0il-program-loader.o s0il-symbols.o s0il-magic.o s0il-clib.o s0il-on-screen-keyboard.o

s0il: ../ctx.h
s0il: libs0il.so main.o bundled-programs.o
	$(CC_NATIVE) $(CFLAGS_NATIVE) $^ -o $@ $(CFLAGS_NATIVE) -L. -ls0il 


PROGRAMS_C_BUNDLED=bin-src/*.c
bundled-programs.o: $(PROGRAMS_C_BUNDLED) programs/*.c programs/*/*.c

main.o: fs_bin_native.c fs_data.c fs_bin_generic.c

Makefile: common.mak

%.o: %.c *.h Makefile ../ctx.h
	$(CC_NATIVE) $(CFLAGS_NATIVE) -c -g $< -o $@

libs0il.so: $(S0IL_OBJS)  Makefile 
	$(CC_NATIVE) $(CFLAGS_NATIVE) -g -shared $(S0IL_OBJS)  -o libs0il.so -lm -lmbedtls -lmbedcrypto -lmbedx509

clean:
	rm -rf s0il libs0il.so bin-riscv bin-xtensa bin-native elf_strip_pie *.o tmpfs tmpfs2 fs_*.c
	rm -rf flow3r/sdkconfig flow3r/build
	rm -rf esp32c3/sdkconfig esp32c3/build
	make -C pico clean
	make -C wasm clean


elf_strip_pie: elf_strip_pie.c
	$(CC_NATIVE) $< -o $@
PROGRAMS_C = $(wildcard bin-src/*.c)
PROGRAMS_PICOC = $(wildcard bin-generic/*)
PROGRAMS_NATIVE = $(patsubst bin-src/%.c, bin-native/%, $(PROGRAMS_C)) \
                 $(patsubst bin-generic/%, bin-native/%, $(PROGRAMS_PICOC))


PROGRAMS_XTENSA = $(patsubst bin-src/%.c, bin-xtensa/%, $(PROGRAMS_C)) \
                 $(patsubst bin-generic/%, bin-xtensa/%, $(PROGRAMS_PICOC))
PROGRAMS_RISCV = $(patsubst bin-src/%.c, bin-riscv/%, $(PROGRAMS_C)) \
                 $(patsubst bin-generic/%, bin-riscv/%, $(PROGRAMS_PICOC))
PROGRAMS_WASM = $(patsubst bin-src/%.c, bin-wasm/%, $(PROGRAMS_C)) \
                $(patsubst bin-generic/%, bin-wasm/%, $(PROGRAMS_PICOC))


bin-xtensa/%: bin-src/%.c *.h Makefile flow3r/build/config/sdkconfig.h
	@mkdir -p `dirname $@` || true
	$(CC_XTENSA) $(CFLAGS_XTENSA) $< -o $@
	$(POST_XTENSA) $@

bin-riscv/%: bin-src/%.c *.h Makefile esp32c3/build/config/sdkconfig.h
	@mkdir -p `dirname $@` || true
	$(CC_RISCV) $(CFLAGS_RISCV) $< -o $@
	$(POST_RISCV) $@

bin-native/%: bin-src/%.c *.h Makefile elf_strip_pie libs0il.so
	@mkdir -p `dirname $@` || true
	$(CC_NATIVE) $< -L. -ls0il -o $@ $(CFLAGS_NATIVE) -DS0IL_DEFINES
	$(POST_NATIVE) $@

bin-wasm/%: bin-src/%.c *.h Makefile 
	@mkdir -p `dirname $@` || true
	$(CC_WASM) $(CFLAGS_WASM) $< -o $@ -s SIDE_MODULE=1 -DS0IL_DEFINES -s ASYNCIFY

bin-native/%: bin-generic/%
	@mkdir -p `dirname $@` || true
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true
bin-xtensa/%: bin-generic/%
	mkdir -p `dirname $@` || true
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true
bin-riscv/%: bin-generic/%
	@mkdir -p `dirname $@` || true
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true
bin-wasm/%: bin-generic/%
	@mkdir -p `dirname $@` || true
	@echo 'LN     ' $@; ln -sf `pwd`/$< $@ || true

bin-native: $(PROGRAMS_NATIVE)
bin-xtensa: $(PROGRAMS_XTENSA)
bin-riscv: $(PROGRAMS_RISCV)
bin-wasm: $(PROGRAMS_WASM)

flow3r: fs_data.c fs_bin_xtensa.c s0il*.[ch]
	(cd flow3r; idf.py build)
flow3r-flash: flow3r
	(cd flow3r; idf.py app-flash monitor)
monitor:
	(cd flow3r; idf.py monitor)

wasm: fs_data.c main.c fs_bin_wasm.c s0il*.[ch] */*.c bundled-programs-wasm.c wasm/Makefile
	(cd wasm; make)
riscv-flash: riscv
	(cd esp32c3; idf.py flash monitor)
riscv: fs_data.c fs_bin_generic.c s0il*.[ch] */*.c
	(cd esp32c3; idf.py build)

BUNDLED_SO_REMOVE=s0il-image s0il-text clock s0il-dir

fs_bin_riscv.c: $(PROGRAMS_RISCV) Makefile mkfs.sh
	@rm -rf $@_fs
	@mkdir $@_fs
	@cp bin-riscv/* $@_fs
	@(cd $@_fs; rm $(BUNDLED_SO_REMOVE))
	./mkfs.sh $@_fs bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm -rf $@_ $@_fs

fs_bin_xtensa.c: $(PROGRAMS_XTENSA) Makefile mkfs.sh
	@rm -rf $@_fs
	@mkdir $@_fs
	@cp bin-xtensa/* $@_fs
	@(cd $@_fs; rm $(BUNDLED_SO_REMOVE))
	./mkfs.sh $@_fs bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm -rf $@_ $@_fs

fs_bin_native.c: $(PROGRAMS_NATIVE) Makefile mkfs.sh
	@rm -rf $@_fs
	@mkdir $@_fs
	@cp bin-native/* $@_fs
	@(cd $@_fs; rm $(BUNDLED_SO_REMOVE))
	./mkfs.sh $@_fs bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm -rf $@_ $@_fs

fs_bin_wasm.c: $(PROGRAMS_WASM) Makefile mkfs.sh
	@rm -rf $@_fs
	@mkdir $@_fs
	@cp bin-wasm/* $@_fs
	@(cd $@_fs; rm $(BUNDLED_SO_REMOVE))
	./mkfs.sh $@_fs bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm -rf $@_ $@_fs

fs_bin_generic.c: bin-generic/* Makefile mkfs.sh
	./mkfs.sh bin-generic bin > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm $@_

fs_data.c: data/* Makefile mkfs.sh
	./mkfs.sh data data > $@_
	@sed $@_ -e 's/unsigned/static const unsigned/' > $@
	@rm $@_
format:
	clang-format -i s0il*.c main.c */main/ctx.c bin-src/*.c
	clang-format -i bin-generic/*
	rpl '//! picoc' '//!picoc' bin-generic/*
	rpl '//! qjs' '//!qjs' bin-generic/*

esp32c3/build/config/sdkconfig.h:
	(cd esp32c3; idf.py reconfigure )

flow3r/build/config/sdkconfig.h:
	@test -r flow3r/flow3r || ( echo "ERROR: You need to set up flow3r symlink in flow3r to point to flow3r-firmware checkout";exit 1)
	(cd flow3r; idf.py reconfigure )
programs:
	make -C programs
pico: fs_data.c fs_bin_generic.c s0il-*.[ch] Makefile
	make -C pico
pico-flash: pico
	cp pico/s0il.uf2 /media/*/RPI-RP2/ ; sync ; sleep 1; mpremote
update-doc: wasm flow3r
	cp wasm/*.js wasm/*.wasm wasm/*.html ../docs/s0il/
	cp flow3r/build/s0il.bin ../docs/s0il/s0il-flow3r.bin
	cp programs/*/xtensa/* ~/src/ctx/docs/s0il/bin/xtensa -v
	cp programs/*/native/* ~/src/ctx/docs/s0il/bin/x86_64 -v
	cp programs/*/rv32/* ~/src/ctx/docs/s0il/bin/rv32 -v
	cp -ru ../docs/* ~/pgo/ctx.graphics/

../ctx.h: ../src/*.[ch]
	make -C .. ctx
