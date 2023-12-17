#include "port_config.h"
#include "s0il.h"

static char *root_path = NULL;

//#include <libgen.h>

#if CTX_FLOW3R 
#include "fs_bin_xtensa.c"
#elif NATIVE
#include "fs_bin_native.c"
#else
#include "fs_bin_generic.c"
#endif
// todo wasm

void add_mains(void);

static float backlight  = 100.0;
static void view_menu (Ui *ui)
{
   ui_start_frame (ui);

   if (ui_button(ui, "files"))
     ui_do(ui, "/");
   //ui_seperator (ui);  

#if 0
   if (ui_button(ui,"app"))
      ui_do(ui, "app");
#endif
   if (ui_button(ui,"console"))
      ui_do(ui, "sh");
#if 0
   if (ui_button(ui,"clock"))
      ui_do(ui, "clock");
#endif
#if 1
   if (ui_button(ui,"httpd"))
      ui_do(ui, "httpd");
#endif
#if 0
   if (ui_button(ui,"raw_fb"))
      ui_do(ui, "raw_fb");
   if (ui_button(ui,"audio-ks"))
      ui_do(ui, "audio-ks");
#endif

   if (ui_button(ui, "system"))
     ui_do(ui, "settings");


   ui_end_frame(ui);
}

#if CTX_FLOW3R
extern int flow3r_synthesize_key_events;
#endif

void view_settings (Ui *ui)
{
   ui_start_frame (ui);

   ui_title(ui,"settings");

   if (ui_button(ui, "wifi")) ui_do(ui, "wifi");
   if (ui_button(ui, "httpd")) ui_do(ui, "httpd");
   if (ui_button(ui, "ui"))   ui_do(ui, "settings-ui");

   backlight = ui_slider(ui,"backlight", 0.0f, 100.0f, 5.0, backlight);

   ui_backlight (backlight);

#if CTX_FLOW3R
   flow3r_synthesize_key_events = ui_toggle(ui,"cap-touch keys", flow3r_synthesize_key_events);
#endif

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

    ctx_windowtitle (ctx, "s0il");
    s0il_signal(SIGPIPE, SIG_IGN);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    fcntl (STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL,0)| O_NONBLOCK);
    s0il_bundle_main("magic", magic_main);
    add_mains();
    mount_bin();

    Ui *ui = ui_new(ctx);
    ui_fake_circle (ui, true);

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

  const char *temp = "Welcome to project s0il\nIt is a unix system, you know this!\n";
  s0il_add_file("/sd",  NULL, 0, S0IL_DIR|S0IL_READONLY);
  s0il_add_file("/bin", NULL, 0, S0IL_DIR|S0IL_READONLY);
  //s0il_add_file("/tmp", NULL, 0, S0IL_DIR|S0IL_READONLY);
  s0il_add_file("/welcome", temp, 0, S0IL_READONLY);

  s0il_system("_init");
  s0il_system("init");


    ui_register_view (ui, "menu", view_menu, NULL);
    ui_register_view (ui, "settings", view_settings, NULL);
    ui_do(ui, "menu"); // queue menu - as initial view

//  s0il_system("wifi --auto &");
//  ui_do(ui, "sh");
    ui_main(ui, NULL); // boot to root_path
    ui_destroy (ui);
    free (root_path);


    ctx_destroy (ctx);
}
