#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "port_config.h"
#include "ctx.h"

#include <sys/stat.h>
#include <dirent.h>

//#define TEST_VIEW "wifi"

#include "ui.h"

void view_menu (Ui *ui)
{
   ui_start (ui);

   if (ui_button(ui, "foo"))
     ui_do(ui, "/sd/app.elf");
   if (ui_button(ui, "apps"))
     ui_do(ui, "apps");
   if (ui_button(ui, "files"))
//#if CTX_ESP
     ui_do(ui, "/sd");
//#else
//     ui_do(ui, "/");
//#endif
   if (ui_button(ui, "settings"))
     ui_do(ui, "settings");

#if CTX_ESP
   if (ui_button(ui,"reboot"))
     esp_restart();
#endif

   ui_end(ui);
}


void view_apps (Ui *ui)
{
   ui_start (ui);

#define UI_APP(name, label, fun, category) \
   if (category && (!strcmp (category, "apps")))\
     if (ui_button(ui, label?label:name)) \
       ui_do(ui, name);
   #include "apps.inc"
#undef UI_APP

   ui_end(ui);
}




void view_splash (Ui *ui)
{
  Ctx *ctx = ui->ctx;
  ui_start (ui);
  ui->draw_tips = true;
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  float ty = height/2;
  float em = ui->font_size;
  char buf[256];

  {
    ctx_move_to(ctx, width * 0.5f,ty);ctx_text(ctx,CTX_DEMO_TITLE);
    ty+=em;
    ctx_move_to(ctx, width * 0.5f,ty);ctx_text(ctx,CTX_DEMO_SUBTITLE);
    ty+=em;
    sprintf(buf, "%.0fx%.0f", width, height);
    ctx_move_to(ctx, width * 0.5,ty);
    ctx_text(ctx,buf);
  }
 
  ctx_logo (ctx, width/2,height/5,height/3);
  ui_end (ui);

  if (ui->view_elapsed > 2.0)
  {
    ui_do (ui, "menu");
    ui->draw_tips = false;
  }
}

#define UI_APP(name, label, fun, category) \
   void fun(Ui *ui);
#define UI_APP_CODE 1
   #include "apps.inc"
#undef UI_APP_CODE
#undef UI_APP

#if CTX_FLOW3R

#include "esp_elf.h"

void destroy_esp_elf (void *esp_elf)
{
  esp_elf_deinit (esp_elf);
  free (esp_elf);
}

void reset_elf_output_state (void);
int  elf_output_state (void); // 1 text 2 graphics 3 both
void _ctx_toggle_in_idle_dispatch (Ctx *ctx);

static int elf_run (int argc, char **argv)
{
  return 0;
}


static int launch_elf_handler = 0;
static int elf_retval = 0;
int launch_elf (Ctx *ctx, void *data)
{
  if (launch_elf_handler) {
    _ctx_toggle_in_idle_dispatch (ctx);
    esp_elf_t *elf = data;
    ctx_remove_idle (ctx, launch_elf_handler);
    launch_elf_handler = 0;
    reset_elf_output_state ();
    elf_retval = esp_elf_request(elf, 0, 0, NULL);
    _ctx_toggle_in_idle_dispatch (ctx);
    destroy_esp_elf (elf);
  }
  return 0;
}

void view_elf(Ui *ui)
{
  if (ui->data == NULL)
  {
    ui_load_file (ui, ui->location);

    if (ui->data)
    {
      uint8_t *data = ui->data;
      esp_elf_t *elf = malloc (sizeof (esp_elf_t));
      int ret = esp_elf_init(elf);
      if (ret < 0) {
        destroy_esp_elf (elf);
      }
      else
      {
      ret = esp_elf_relocate(elf, data);
      free (data);
      ui->data = NULL;
      if (ret < 0) {
        destroy_esp_elf (elf);
      }
      else
      {
        // we want it to excecute outside start_frame / end_frame ; 
        // calling it here means it is called at the tail-end of end_frame
        // where things work out
        launch_elf_handler = ctx_add_timeout (ui->ctx, 0, launch_elf, elf);
      }
      }
    }
    ui_pop_fun (ui);
  }
}
#endif

#if CTX_ESP
void app_main(void)
#else
int main (int argc, char **argv)
#endif
{
    Ctx *ctx = ctx_new (DISPLAY_WIDTH,
                        DISPLAY_HEIGHT,
                        NULL);
    
    Ui *ui = ui_new(ctx);

    ui_register_view (ui, "splash", "os", view_splash);
    ui_register_view (ui, "menu",   "menus", view_menu);
    ui_register_view (ui, "apps",   "menus", view_apps);
#if CTX_FLOW3R
    ui_register_view (ui, "elf binary", ".elf", view_elf);
#endif

#define UI_APP(name, label, fun, category) \
    ui_register_view (ui, name, category, fun);
   #include "apps.inc"
#undef UI_APP 

    ui_do(ui, "splash"); // < doing this before the custom test-app
                         // allows entering menu from splash
#ifdef TEST_VIEW
    ui_do(ui, TEST_VIEW);
#endif

    ui_main(ui, NULL);
    ui_destroy (ui);
}

