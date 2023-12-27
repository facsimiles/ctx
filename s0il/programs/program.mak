TARGETS+=native/$(PROGRAM) xtensa/$(PROGRAM) rv32/$(PROGRAM)
all: $(TARGETS)

native/$(PROGRAM): $(SOURCES)
	@mkdir -p `dirname $@` || true
	$(CC_NATIVE) $(CFLAGS_NATIVE) -include s0il.h $? -o $@ -I. -Iinclude \
            -ls0il `pkg-config ctx --libs --cflags`
	$(POST_NATIVE3) $@

xtensa/$(PROGRAM): $(SOURCES)
	@mkdir -p `dirname $@` || true
	$(CC_XTENSA) $(CFLAGS_XTENSA) -include s0il.h $? -o $@ -I. -Iinclude
	$(POST_XTENSA) $@

rv32/$(PROGRAM): $(SOURCES)
	@mkdir -p `dirname $@` || true
	$(CC_RISCV) $(CFLAGS_RISCV) -include s0il.h $? -o $@ -I. -Iinclude
	$(POST_RISCV) $@

clean:
	rm -f $(TARGETS)

