
// for wasm and other platforms without actual ability top open
// binaries, bundle ass much as possible.

#include "port_config.h"
#define S0IL_REDEFINE_CLIB
#define S0IL_BUNDLE
#include "s0il.h"
// what follows is a list of programs to include directly

#include "bin-src/clock.c"
#include "bin-src/image.c"
#include "bin-src/text.c"

#include "qjs_bundle.c"
#include "lua_bundle.c"
#include "picoc_bundle.c"
//#include "bin-src/app.c"
#include "bin-src/audio-ks.c"
#include "bin-src/busywarp.c"
#include "bin-src/raw-fb.c"
#include "bin-src/tsr-ui.c"
#include "bin-src/wifi.c"
//#include "bin-src/httpd.c"

void add_mains(void) {
  static bool done = false;
  if (done)
    return;
  done = true;

  s0il_bundle_main("picoc", picoc_main);
  s0il_bundle_main("image", image_main);
  s0il_bundle_main("text", text_main);
  s0il_bundle_main("clock", clock_main);
  s0il_bundle_main("raw-fb", raw_fb_main);
  s0il_bundle_main("busywarp", busywarp_main);
  s0il_bundle_main("tsr-ui", tsr_ui_main);
  //s0il_bundle_main("app", app_main);
  s0il_bundle_main("wifi", wifi_main);
  s0il_bundle_main("audio-ks", audio_ks_main);
//s0il_bundle_main("httpd", httpd_main);
  s0il_bundle_main("lua", lua_main);
  s0il_bundle_main("luac", luac_main);
  s0il_bundle_main("qjs", qjs_main);
}
