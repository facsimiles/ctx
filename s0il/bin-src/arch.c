#include "s0il.h"

MAIN(arch) {
#if defined(S0IL_NATIVE)
  puts("x86_64");
#elif defined(RISCV)
  puts("rv32");
#elif defined(CTX_ESP)
  puts("xtensa");
#endif
  return 0;
}
