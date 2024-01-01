
// for wasm and other platforms without actual ability top open
// binaries, bundle ass much as possible.

#include "port_config.h"
#define S0IL_DEFINES
#define S0IL_BUNDLE
#include "s0il.h"
// what follows is a list of programs to include directly

#include "bin-src/clock.c"
#include "bin-src/s0il-image.c"
#include "bin-src/s0il-text.c"
//#include "bin-src/wifi.c"
#include "programs/lua.c"
#include "programs/qjs.c"

#include "programs/picoc.c"
#include "bin-src/demo-pcm_audio.c"
#include "bin-src/demo-ctx_host.c"
#include "bin-src/bundled.c"
#include "bin-src/demo-setpixels.c"
#include "bin-src/demo-tsr.c"
#include "bin-src/demo-ui.c"

void add_mains(void) {
  static bool done = false;
  if (done)
    return;
  done = true;

  s0il_bundle_main("picoc", picoc_main);
  s0il_bundle_main("s0il-image", s0il_image_main);
  s0il_bundle_main("s0il-text", s0il_text_main);
  s0il_bundle_main("clock", clock_main);
  s0il_bundle_main("demo-setpixels", demo_setpixels_main);
  s0il_bundle_main("demo-ctx_host", demo_ctx_host_main);
  s0il_bundle_main("bundled", bundled_main);
  s0il_bundle_main("demo-tsr", demo_tsr_main);
  s0il_bundle_main("lua", lua_main);
  s0il_bundle_main("qjs", qjs_main);
  s0il_bundle_main("luac", luac_main);
  //s0il_bundle_main("app", app_main);
  s0il_bundle_main("demo-pcm_audio", demo_pcm_audio_main);
  //s0il_bundle_main("wifi", wifi_main);
}
