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

