#include "port_config.h"
#include "s0il.h"

// #include <libgen.h>

#if CTX_FLOW3R
#include "fs_bin_xtensa.c"
#elif S0IL_NATIVE
#include "fs_bin_native.c"
#elif defined(EMSCRIPTEN)
#include "fs_bin_wasm.c"
#else
#include "fs_bin_generic.c"
#endif
#include "fs_data.c"

void add_mains(void);

static float backlight = 100.0;

static void view_tests(Ui *ui) {
  ui_start_frame(ui);

  if (ui_button(ui, "C setpixels"))
    s0il_do(ui, "demo-setpixels");
  if (ui_button(ui, "C setpixels tsr"))
    s0il_do(ui, "demo-setpixels-tsr");
  if (ui_button(ui, "picoc ui"))
    s0il_do(ui, "demo-ui");
  if (ui_button(ui, "qjs text mandel"))
    s0il_do(ui, "demo-vt-mandel.js");
  if (ui_button(ui, "C ctx-host"))
    s0il_do(ui, "demo-ctx_host");
  if (ui_button(ui, "C pcm audio"))
    s0il_do(ui, "demo-pcm_audio");
  if (ui_button(ui, "C tsr"))
    s0il_do(ui, "demo-tsr");

  ui_end_frame(ui);
}

static void view_repls(Ui *ui) {
  ui_start_frame(ui);

  if (ui_button(ui, "sh"))
    s0il_do(ui, "sh");
  if (ui_button(ui, "lua"))
    s0il_do(ui, "lua");
  if (ui_button(ui, "quickjs"))
    s0il_do(ui, "qjs");
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
#if 0 // CTX_FLOW3R
  if (usb_console_connected) {
    char buf[8];
    if (!usb_had_console) {
      fprintf(stdout, "\e[5n");
      int read = s0il_fread(buf, 1, 4, stdin);
      if (read)
        usb_had_console = true;

      if (usb_had_console) // launch a shell first time we
        s0il_do(ui, "sh");   // see a terminal on the other
                           // end of USB
    }
  }
#endif

  if (ui_button(ui, "files"))
    s0il_do(ui, "/");
  if (ui_button(ui, "repls"))
    s0il_do(ui, "repls");
  if (ui_button(ui, "clock"))
    s0il_do(ui, "clock");
  if (ui_button(ui, "console"))
    s0il_do(ui, "sh");
  if (ui_button(ui, "httpd"))
    s0il_do(ui, "httpd");
  if (ui_button(ui, "tests"))
    s0il_do(ui, "tests");

  if (ui_button(ui, "system"))
    s0il_do(ui, "settings");

  ui_end_frame(ui);
}

#if CTX_FLOW3R
extern int flow3r_synthesize_key_events;
#endif

void view_magic(Ui *ui);

void s0il_view_views(Ui *ui);

void view_settings(Ui *ui) {
  ui_start_frame(ui);

  ui_title(ui, "settings");

  if (ui_button(ui, "wifi"))
    s0il_do(ui, "wifi");
  if (ui_button(ui, "httpd"))
    s0il_do(ui, "httpd");
  if (ui_button(ui, "appearance"))
    s0il_do(ui, "settings-ui");

  backlight = ui_slider(ui, "backlight", 0.0f, 100.0f, 5.0, backlight);

  s0il_backlight(backlight);

#if CTX_FLOW3R
  flow3r_synthesize_key_events =
      ui_toggle(ui, "cap-touch keys", flow3r_synthesize_key_events);
#endif

  // if (ui_button(ui, "mime types"))
  //   s0il_do(ui, "magic");
  if (ui_button(ui, "views"))
    s0il_do(ui, "s0il-views");

#if CTX_ESP
  if (ui_button(ui, "reboot"))
    system("reboot");
#endif

  ui_end_frame(ui);
}

// int magic_main(int argc, char **argv);
int file_main(int argc, char **argv);
int ps_main(int argc, char **argv);

#include <fcntl.h>
#include <signal.h>

void s0il_program_runner_init(void);
#if CTX_ESP
void app_main(void) {
  // char *argv[] = {NULL, NULL};
#else

int main(int argc, char **argv) {
#endif
  s0il_setenv("PATH", "/sd/bin:/bin", 1);
  s0il_program_runner_init();

  Ctx *ctx = ctx_new(DISPLAY_WIDTH, DISPLAY_HEIGHT, NULL);

  ctx_windowtitle(ctx, "s0il");
  s0il_signal(SIGPIPE, SIG_IGN);
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);
  // s0il_bundle_main("magic", magic_main);
  s0il_bundle_main("file", file_main);
  s0il_bundle_main("ps", ps_main);
  mount_bin();
  mount_data();
  add_mains();

  Ui *ui = ui_new(ctx);
  ui_fake_circle(ui, true);

  s0il_add_magic("application/flow3r", "inode/directory", "flow3r.toml", -1, 0);
#if 0
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
  s0il_add_file("/welcome", temp, 0, S0IL_READONLY);
#endif

  s0il_add_file("/sd", NULL, 0, S0IL_DIR | S0IL_READONLY);
  s0il_add_file("/data", NULL, 0, S0IL_DIR | S0IL_READONLY);
  s0il_add_file("/flash", NULL, 0, S0IL_DIR | S0IL_READONLY);
  s0il_add_file("/bin", NULL, 0, S0IL_DIR | S0IL_READONLY);
  //s0il_add_file("/tmp", NULL, 0, S0IL_DIR|S0IL_READONLY);
  char *t = "";
#if !defined(EMSCRIPTEN)
  s0il_add_file("/tmp/dummy", t, 1, 0);
#endif
#if EMSCRIPTEN

  EM_ASM(FS.mkdir('/sd'); FS.mount(IDBFS, {}, '/sd'); FS.syncfs(
      true, function(err) { assert(!err); }););

#endif

  s0il_system("_init");
  s0il_system("init");

  s0il_add_view(ui, "menu", view_menu, NULL);
  s0il_add_view(ui, "repls", view_repls, NULL);
  s0il_add_view(ui, "tests", view_tests, NULL);
  s0il_add_view(ui, "settings", view_settings, NULL);
  // s0il_add_view(ui, "magic", view_magic, NULL);
  s0il_add_view(ui, "s0il-views", s0il_view_views, NULL);
  s0il_do(ui, "menu");      // queue menu - as initial view
  s0il_printf("\033[?30l"); // turn off scrollbar

//#ifdef CTX_ESP
  s0il_system("wifi --auto & sleep 1 ; httpd &");
//#endif
  //  s0il_do(ui, "sh");
#ifdef S0IL_NATIVE
  s0il_main(ui);
#else
  for (;;) {
    ctx_reset_has_exited(ctx);
    s0il_main(ui);
    s0il_system("sync");
  }
#endif

  ui_destroy(ui);

  ctx_destroy(ctx);
}
