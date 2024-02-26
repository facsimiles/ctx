
// for wasm and other platforms without actual ability top open
// binaries, bundle ass much as possible.

#include "port_config.h"
#define S0IL_DEFINES
#define S0IL_BUNDLE

#define BUNDLE_LUA   1
#define BUNDLE_QJS   0
#define BUNDLE_HTTPD 1
#define BUNDLE_PICOC 1
#define BUNDLE_WIFI  1


#include "s0il.h"
// what follows is a list of programs to include directly

#include "bin-src/clock.c"
#include "bin-src/s0il-image.c"
#include "bin-src/s0il-text.c"
#include "bin-src/s0il-dir.c"

#if BUNDLE_QJS
#include "programs/qjs.c"
#endif
#if BUNDLE_LUA
#include "programs/lua.c"
#endif
#if BUNDLE_PICOC
#include "programs/picoc.c"
#endif
#if BUNDLE_HTTPD
#include "bin-src/httpd.c"
#endif
//#include "bin-src/demo-pcm_audio.c"
#include "bin-src/demo-setpixels.c"
#include "bin-src/demo-tsr.c"
#include "bin-src/demo-ui-c.c"
#include "bin-src/demo-ctx_host.c"
#include "bin-src/wifi.c"
#include "bin-src/sync.c"
#include "bin-src/date.c"

void add_mains(void) {
  static bool done = false;
  if (done)
    return;
  done = true;

#if BUNDLE_PICOC
  s0il_bundle_main("picoc", picoc_main);
#endif
#if BUNDLE_LUA
  s0il_bundle_main("lua", lua_main);
  s0il_bundle_main("luac", luac_main);
#endif
#if BUNDLE_QJS
  s0il_bundle_main("qjs", qjs_main);
#endif
#if BUNDLE_HTTPD
  s0il_bundle_main("httpd", httpd_main);
#endif

  s0il_bundle_main("s0il-image", s0il_image_main);
  s0il_bundle_main("s0il-text", s0il_text_main);
  s0il_bundle_main("s0il-dir", s0il_dir_main);
  s0il_bundle_main("clock", clock_main);
  s0il_bundle_main("demo-setpixels", demo_setpixels_main);
//s0il_bundle_main("bundled", bundled_main);
  s0il_bundle_main("demo-tsr", demo_tsr_main);
  //s0il_bundle_main("app", app_main);
#if BUNDLE_WIFI
  s0il_bundle_main("wifi", wifi_main);
#endif
  s0il_bundle_main("sync", sync_main);
//s0il_bundle_main("demo-pcm_audio", demo_pcm_audio_main);
  s0il_bundle_main("demo-ctx_host", demo_ctx_host_main);
  s0il_bundle_main("date", date_main);
}
