#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "port_config.h"
#include "ctx.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "ui.h"

static char *root_path = NULL;

#include <libgen.h>

#ifdef EMSCRIPTEN
#include "fs_bin_generic.c"
#else
#if CTX_ESP
#include "fs_bin_xtensa.c"
#else
#include "fs_bin_native.c"
#endif
#endif
// todo wasm

void add_mains(void);

static void view_menu (Ui *ui)
{
   ui_start_frame (ui);

   if (ui_button(ui, "/sd"))
     ui_do(ui, "/sd");
#if CTX_FLOW3RXXX
   if (ui_button(ui, "usb sd"))
   {
     sd_msc ();
   }
#endif
   if (ui_button(ui, "/bin"))
     ui_do(ui, "/bin");
#if 1
   if (ui_button(ui,"app"))
      ui_do(ui, "app");
   if (ui_button(ui,"clock"))
      ui_do(ui, "clock");
   if (ui_button(ui,"httpd"))
      ui_do(ui, "httpd");
   if (ui_button(ui,"console"))
      ui_do(ui, "sh");
   if (ui_button(ui,"raw_fb"))
      ui_do(ui, "raw_fb");
   if (ui_button(ui,"audio-ks"))
      ui_do(ui, "audio-ks");
#endif
   if (ui_button(ui, "settings"))
     ui_do(ui, "settings");

#if CTX_ESP
   if (ui_button(ui,"reboot"))
     esp_restart();
#endif

   ui_end_frame(ui);
}



#include <signal.h>
#include <fcntl.h>
#if CTX_ESP
void app_main(void)
{
  char *argv[]={NULL, NULL};
#else
int main (int argc, char **argv)
{
#endif

    Ctx *ctx = ctx_new (DISPLAY_WIDTH,
                        DISPLAY_HEIGHT,
                        NULL);

    run_signal(SIGPIPE, SIG_IGN);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    fcntl (STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL,0)| O_NONBLOCK);
    run_bundle_main("magic", magic_main);
    add_mains();
    mount_bin();

    Ui *ui = ui_new(ctx);

    if (argv[1])
      root_path = realpath(dirname (argv[1]), NULL);
    else if (access ("/sd", R_OK)==F_OK)
      root_path = realpath("/sd", NULL);
    else if (access ("./sd", R_OK)==F_OK)
      root_path = realpath("./sd", NULL);

    magic_add("application/flow3r",
      "inode/directory",
      "flow3r.toml",
      -1, 0);

  const char *temp = "this is a file\nthat is almost just a string";
  run_add_file("/tmp/file", temp, 0, true);
  run_add_file("/tmp/file2", temp, 0, true);

  runs("_init");
//runs("init");


    ui_register_view (ui, "menu", view_menu, NULL);
    ui_do(ui, "menu"); // queue menu - as initial view

//    runs("wifi --auto ");

    ui_main(ui, NULL); // boot to root_path
    ui_destroy (ui);
    free (root_path);


    ctx_destroy (ctx);
}
