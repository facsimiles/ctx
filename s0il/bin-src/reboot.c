#include "s0il.h"

MAIN(reboot) {
#if 0
#if EMSCRIPTEN
  EM_ASM(FS.syncfs(function(err) { assert(!err); }));
#endif
#endif
  return 0;
}
