TARGETS+=native/$(PROGRAM_B) xtensa/$(PROGRAM_B) rv32/$(PROGRAM_B)
all: $(TARGETS)

native/$(PROGRAM_B): $(SOURCES_B)
	@mkdir -p `dirname $@` || true
	$(CC_NATIVE) $(CFLAGS_NATIVE) -include s0il.h $? -o $@ -I. -Iinclude \
            -ls0il `pkg-config ctx --libs --cflags`
	$(POST_NATIVE3) $@

xtensa/$(PROGRAM_B): $(SOURCES_B)
	@mkdir -p `dirname $@` || true
	$(CC_XTENSA) $(CFLAGS_XTENSA) -include s0il.h $? -o $@ -I. -Iinclude
	$(POST_XTENSA) $@

rv32/$(PROGRAM_B): $(SOURCES_B)
	@mkdir -p `dirname $@` || true
	$(CC_RISCV) $(CFLAGS_RISCV) -include s0il.h $? -o $@ -I. -Iinclude
	$(POST_RISCV) $@

