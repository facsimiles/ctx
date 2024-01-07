#include "s0il.h"

MAIN(sync) {
#ifdef S0IL_BUNDLE
#if EMSCRIPTEN
  EM_ASM(FS.syncfs(function(err) { assert(!err); }));
#endif
#endif
  return 0;
}
