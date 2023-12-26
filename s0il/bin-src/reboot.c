#include "s0il.h"

MAIN(reboot) {
#if EMSCRIPTEN
  EM_ASM(FS.syncfs(function(err) { assert(!err); }));
#endif
  return 0;
}
