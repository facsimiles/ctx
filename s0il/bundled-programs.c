#include "port_config.h"
#define S0IL_REDEFINE_CLIB
#define S0IL_BUNDLE
#include "s0il.h"
// what follows is a list of programs to include directly

#include "qjs_bundle.c"
#include "lua_bundle.c"
#include "picoc_bundle.c"
#include "programs/clock.c"
#include "programs/image.c"
#include "programs/text.c"

void add_mains(void) {
  static bool done = false;
  if (done)
    return;
  done = true;

  s0il_bundle_main("image", image_main);
  s0il_bundle_main("text", text_main);
  s0il_bundle_main("clock", clock_main);
  s0il_bundle_main("picoc", picoc_main);
  s0il_bundle_main("lua", lua_main);
  s0il_bundle_main("luac", luac_main);
  s0il_bundle_main("qjs", qjs_main);
}
