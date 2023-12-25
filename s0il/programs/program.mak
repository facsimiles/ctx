all: $(TARGETS)

native/$(PROGRAM): $(SOURCES)
	@mkdir -p `dirname $@` || true
	$(CC_NATIVE) $(CFLAGS_NATIVE) $? -o $@ -I. -Iinclude \
            -ls0il `pkg-config ctx --libs --cflags`
	../../elf_strip_pie $@

xtensa/$(PROGRAM): $(SOURCES)
	@mkdir -p `dirname $@` || true
	$(CC_XTENSA) $(CFLAGS_XTENSA) $? -o $@ -I. -Iinclude
	$(POST_XTENSA) $@

rv32/$(PROGRAM): $(SOURCES)
	@mkdir -p `dirname $@` || true
	$(CC_RISCV) $(CFLAGS_RISCV) $? -o $@ -I. -Iinclude
	$(POST_RISCV) $@

clean:
	rm -f $(TARGETS)

