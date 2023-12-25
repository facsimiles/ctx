#include "port_config.h"
#define S0IL_DEFINES
#define S0IL_BUNDLE
#include "s0il.h"
// what follows is a list of programs to include directly

#include "bundled/qjs.c"
#include "bundled/lua.c"
#include "bundled/picoc.c"
#include "bin-src/clock.c"
#include "bin-src/image.c"
#include "bin-src/text.c"

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
