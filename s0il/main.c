#include "port_config.h"
#include "s0il.h"

static char *root_path = NULL;

// #include <libgen.h>

#if CTX_FLOW3R
#include "fs_bin_xtensa.c"
#elif CTX_NATIVE
#include "fs_bin_native.c"
#else
#include "fs_bin_generic.c"
#endif
#include "fs_data.c"

void add_mains(void);

static float backlight = 100.0;

static void view_tests(Ui *ui) {
  ui_start_frame(ui);

  if (ui_button(ui, "setpixels"))
    ui_do(ui, "demo-setpixels");
  if (ui_button(ui, "ctx-host"))
    ui_do(ui, "demo-ctx_host");
  if (ui_button(ui, "pcm audio"))
    ui_do(ui, "demo-pcm_audio");
  if (ui_button(ui, "tsr"))
    ui_do(ui, "demo-tsr");

  ui_end_frame(ui);
}

static void view_repls(Ui *ui) {
  ui_start_frame(ui);

  if (ui_button(ui, "unix"))
    ui_do(ui, "sh");
  if (ui_button(ui, "lua"))
    ui_do(ui, "lua");
  if (ui_button(ui, "quickjs"))
    ui_do(ui, "qjs");
#if 0
  if (ui_button(ui, "picoc"))
    s0il_system("picoc -i");
#endif
  ui_end_frame(ui);
}

#if CTX_FLOW3R
extern bool usb_console_connected;
static bool usb_had_console = false;
#endif

static void view_menu(Ui *ui) {
  ui_start_frame(ui);
#if CTX_FLOW3R
  if (usb_console_connected) {
    char buf[8];
    if (!usb_had_console) {
      fprintf(stdout, "\e[5n");
      int read = s0il_fread(buf, 1, 4, stdin);
      if (read)
        usb_had_console = true;

      if (usb_had_console) // launch a shell first time we
        ui_do(ui, "sh");   // see a terminal on the other
                           // end of USB
    }
#if 0
    if (usb_had_console)
      ui_text(ui, "had TERM!");
    else
      ui_text(ui, "USB");
#endif
  }
#endif

  if (ui_button(ui, "files"))
    ui_do(ui, "/");
  if (ui_button(ui, "repls"))
    ui_do(ui, "repls");
  if (ui_button(ui, "clock"))
    ui_do(ui, "clock");
  if (ui_button(ui, "console"))
    ui_do(ui, "sh");
  if (ui_button(ui, "httpd"))
    ui_do(ui, "httpd");
  if (ui_button(ui, "tests"))
    ui_do(ui, "tests");

  if (ui_button(ui, "system"))
    ui_do(ui, "settings");

  ui_end_frame(ui);
}

#if CTX_FLOW3R
extern int flow3r_synthesize_key_events;
#endif

void view_settings(Ui *ui) {
  ui_start_frame(ui);

  ui_title(ui, "settings");

  if (ui_button(ui, "wifi"))
    ui_do(ui, "wifi");
  if (ui_button(ui, "httpd"))
    ui_do(ui, "httpd");
  if (ui_button(ui, "ui"))
    ui_do(ui, "settings-ui");

  backlight = ui_slider(ui, "backlight", 0.0f, 100.0f, 5.0, backlight);

  ui_backlight(backlight);

#if CTX_FLOW3R
  flow3r_synthesize_key_events =
      ui_toggle(ui, "cap-touch keys", flow3r_synthesize_key_events);
#endif

#if CTX_ESP
  if (ui_button(ui, "reboot"))
    esp_restart();
#endif

  ui_end_frame(ui);
}

int magic_main(int argc, char **argv);
int file_main(int argc, char **argv);
int ps_main(int argc, char **argv);

#include <fcntl.h>
#include <signal.h>

#if EMSCRIPTEN
#include <emscripten.h>
#endif

#if CTX_ESP
void app_main(void) {
  char *argv[] = {NULL, NULL};
#else
int main(int argc, char **argv) {
#endif

  Ctx *ctx = ctx_new(DISPLAY_WIDTH, DISPLAY_HEIGHT, NULL);

  ctx_windowtitle(ctx, "s0il");
  s0il_signal(SIGPIPE, SIG_IGN);
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
  s0il_bundle_main("magic", magic_main);
  s0il_bundle_main("file", file_main);
  s0il_bundle_main("ps", ps_main);
  add_mains();
  mount_bin();
  mount_data();

  Ui *ui = ui_new(ctx);
  ui_fake_circle(ui, true);

#ifdef PICO_BUILD
  root_path = "/sd";
#else
  if (argv[1])
    root_path = realpath(dirname(argv[1]), NULL);
  else if (access("/sd", R_OK) == F_OK)
    root_path = realpath("/sd", NULL);
  else if (access("./sd", R_OK) == F_OK)
    root_path = realpath("./sd", NULL);
#endif
  magic_add("application/flow3r", "inode/directory", "flow3r.toml", -1, 0);

  const char *temp =
      "s0il - vector graphics operating environment\n"
      "\n"
      "s0il is a compact and portable vector graphics UI, with small memory"
      "requirements built on top of ctx.\n"
      "\n"
      "Features:\n"
      "  - ports to risc-v, xtensa, linux and wasm\n"
      "  - nested running of programs bundled in host binary or external ELF "
      "dynamic fpic executables (linux and esp32s3)\n"
      "  - threads simulating processes (in progress)\n"
      "  - wrappers for many libc functions providing a start of process "
      "insolation, when relocating symbols in ELF binaries these wrapper "
      "functions are automatically used, when bundling programs the "
      "pre-processor replaces calls.\n"
      "  - RAM file-system, provides a pre-loaded mutable root file system.\n"
      "      system() is provided by the internal libc, it supports both #! "
      "and //! as first line of files to specify interpreter."
      "    changes in folders managed by it ,/ /bin/ and /tmp/ are lost on "
      "reboot\n"
      "  - view system based on named views/URI or path locations\n"
      "    view handlers for the following mime types built in:\n"
      "     - ELF (runs the binary)\n"
      "     - image/png and image/jpeg shows it, no real controls\n"
      "     - text/* small text editor, which auto saves on quit\n"
      "     - directory/inode shows browsable directories\n"
      "     - further mime handlers can be installed, latest registered should "
      "be used (not yet implemented).\n"

      "Key-mappings:\n"
#if CTX_FLOW3R
      " I         return\n"
      " II        tab\n"   // only upper part, when in keyboard mode
      " III       right\n" // possibly to be unused
      " IV        unused\n"
      " V         space\n"
      " VI        down\n" // to be unused
      " VII       left\n" // possible to be unused
      " VIII      up\n"   // make part of this be shift-space
      " IX        backspace\n"
      " X         shift-space\n" // TODO : make this be all of
                                 // up,down,left,right
      " tristate0 left,return,right\n"
      " tristate1 page-up,control-q,page-down\n"
      "\n"
#endif

      "Keybindings:\n"
      " left, right    previous, next\n"
      "                shit-tab, tab and up and down are also recognized\n"
      " return/space   activate\n"
      " backspace      back\n"
      " escape         exit\n"
      " shift-space    toggle keyboard\n";

  s0il_add_file("/sd", NULL, 0, S0IL_DIR | S0IL_READONLY);
  s0il_add_file("/data", NULL, 0, S0IL_DIR | S0IL_READONLY);
  s0il_add_file("/flash", NULL, 0, S0IL_DIR | S0IL_READONLY);
  s0il_add_file("/bin", NULL, 0, S0IL_DIR | S0IL_READONLY);
  // s0il_add_file("/tmp", NULL, 0, S0IL_DIR|S0IL_READONLY);
  s0il_add_file("/welcome", temp, 0, S0IL_READONLY);
  char *t = "";
  s0il_add_file("/tmp/dummy", t, 1, 0);
#if EMSCRIPTEN

  EM_ASM(
    FS.mkdir('/sd');
    FS.mount(IDBFS, {}, '/sd');
    FS.syncfs(true, function (err) { assert(!err); });
  );

#endif

  s0il_system("_init");
  s0il_system("init");

  ui_register_view(ui, "menu", view_menu, NULL);
  ui_register_view(ui, "repls", view_repls, NULL);
  ui_register_view(ui, "tests", view_tests, NULL);
  ui_register_view(ui, "settings", view_settings, NULL);
  ui_do(ui, "menu");        // queue menu - as initial view
  s0il_printf("\033[?30l"); // turn off scrollbar

  //  s0il_system("wifi --auto &");
  //  ui_do(ui, "sh");
//#ifdef CTX_NATIVE
  ui_main(ui, NULL);
//#else
//  for (;;) {
//    ctx_reset_has_exited(ctx);
//    ui_main(ui, NULL);
//  }
//#endif

  ui_destroy(ui);
  free(root_path);

#if EMSCRIPTEN
  EM_ASM(
    // Ensure IndexedDB is closed at exit.
    Module['onExit'] = function() {
      assert(Object.keys(IDBFS.dbs).length == 0);
    };
    FS.syncfs(function (err) {
      assert(!err);console.log("synced fs");
    });
  );
#endif

  ctx_destroy(ctx);
}
