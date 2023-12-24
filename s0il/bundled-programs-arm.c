
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

#include "bundled/picoc.c"
//#include "bin-src/app.c"
#include "bin-src/demo-pcm_audio.c"
#include "bin-src/bundled.c"
#include "bin-src/demo-setpixels.c"
#include "bin-src/demo-tsr.c"
#include "bin-src/demo-ui.c"
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
  s0il_bundle_main("demo-setpixels", demo_setpixels_main);
  s0il_bundle_main("bundled", bundled_main);
  s0il_bundle_main("demo-tsr", demo_tsr_main);
  //s0il_bundle_main("app", app_main);
  s0il_bundle_main("demo-pcm_audio", demo_pcm_audio_main);
}