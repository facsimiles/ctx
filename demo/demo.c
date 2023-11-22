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

#if 0
   if (ui_button(ui, "karplus strong"))
     ui_do(ui, "/sd/fretless-bass.elf");
   if (ui_button(ui, "app"))
     ui_do(ui, "/sd/app.elf");
   if (ui_button(ui, "stdout"))
     ui_do(ui, "/sd/stdout.elf");
   if (ui_button(ui, "stdin"))
     ui_do(ui, "/sd/stdin.elf");
#endif
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

#include "esp_log.h"
#include "esp_task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_elf.h"

void destroy_esp_elf (void *esp_elf)
{
  esp_elf_deinit (esp_elf);
  free (esp_elf);
}

void reset_elf_output_state (void);
int  elf_output_state (void); // 1 text 2 graphics 3 both
void _ctx_toggle_in_idle_dispatch (Ctx *ctx);

static int elf_run (char *path, int argc, char **argv)
{
  int retval = 0;
  if ((!path) && argv && argv[0])
    path = argv[0];
  FILE *file = fopen (path, "rb");
  if (file)
  {
     esp_elf_t elf;
     esp_elf_init(&elf);
     fseek(file, 0, SEEK_END);
     long length = ftell(file);
     fseek(file, 0, SEEK_SET);
     uint8_t *data = malloc(length + 1);
     if (data)
     {
       fread(data, length, 1, file);
       fclose(file);
       retval = esp_elf_relocate(&elf, data);
       free (data);
       if (retval == 0)
         retval = esp_elf_request(&elf, 0 /* request-opt*/, argc, argv);
     }
     esp_elf_deinit(&elf);
  }
  else
  {
    retval = -1;
  }
  return retval;
}

typedef struct process_info_t {
  char  *path;
  int    argc;
  char **argv;
  Ui    *ui;
} process_info_t;

static int elf_retval = 0;
static void elf_task(void *_arg)
{
  process_info_t *pi = (process_info_t*)_arg;
  elf_retval = elf_run (pi->path, pi->argc, pi->argv);
  reset_elf_output_state ();
  if (elf_retval == 0)
    ui_pop_fun (pi->ui);
  for (int i = 0; i < pi->argc; i++)
    if (pi->argv[i]) free (pi->argv[i]);
  free (pi->argv);
  free (pi);
  vTaskDelete(NULL);
}

static TaskHandle_t elf_task_handle;

static int ui_run_elf_task (Ui *ui, const char *path, int argc, char **argv)
{
   process_info_t *pi= calloc (1, sizeof (process_info_t));
   pi->ui = ui;
   pi->path = strdup(path);
   pi->argc = argc;
   pi->argv = malloc (sizeof (char*) * (argc + 1));
   pi->argv[argc] = NULL;
   for (int i = 0; i < argc; i++)
     pi->argv[i] = strdup (argv[i]);
    reset_elf_output_state ();
   BaseType_t res =
     xTaskCreate(elf_task, "elf", 16384, pi, ESP_TASK_PRIO_MIN+1,
                 &elf_task_handle);
   usleep(400 * 1000);
   return res == pdPASS;
}

static int launch_elf_handler = 0;

int launch_elf_blocking (Ctx *ctx, void *data)
{
  Ui *ui = data;
  if (launch_elf_handler) {
    _ctx_toggle_in_idle_dispatch (ctx);
    ctx_remove_idle (ctx, launch_elf_handler);
    reset_elf_output_state ();
    
        printf ("launch!\n");
    elf_retval = elf_run (ui->location, 0, NULL);
    printf("retval :%i\n", elf_retval);
    _ctx_toggle_in_idle_dispatch (ctx);
    ui_pop_fun (ui);
  }
  return 0;
}

int launch_elf_task (Ctx *ctx, void *data)
{
  Ui *ui = data;
  if (launch_elf_handler) {
    ctx_remove_idle (ctx, launch_elf_handler);
    reset_elf_output_state ();
    ui_run_elf_task (ui, ui->location, 0, NULL);
    launch_elf_handler = 0;
  }
  return 0;
}

static CtxClient *term_client = NULL;
static void draw_term (Ui *ui)
{
  Ctx *ctx = ui->ctx;
  float font_size = ui->height / 17.0f;
  if (!term_client)
  {
    int flags = 0;
    term_client = ctx_client_new_argv (ctx, NULL, 0, 0, ui->width, ui->height,
         font_size, flags, NULL, NULL);
    ctx_client_maximize (ctx, ctx_client_id(term_client));
    ctx_client_resize (ctx, ctx_client_id(term_client), ui->width*180/240, ui->height*180/240);
  }
  ctx_save (ctx);
  ctx_translate (ctx, ctx_width(ctx) * 35/240, ctx_height(ctx)*35/240);
     ctx_clients_draw (ctx, 0);
  ctx_clients_draw (ctx, 0);
  ctx_restore (ctx);
}


void view_elf(Ui *ui)
{
  if (ui->data == NULL)
  {
   //launch_elf_handler = ctx_add_timeout (ui->ctx, 0, launch_elf_blocking, ui);
    launch_elf_handler = ctx_add_timeout (ui->ctx, 0, launch_elf_task, ui);
    ui->data = (void*)1;
  }
  else
  {
     int os = elf_output_state ();
     char buf[64];
     if (os == 1)
     {
       ui_start (ui);
       ui_text (ui, "txt");
       if (elf_retval !=0)
       {
       sprintf (buf, "returned: %i", elf_retval);
       ui_text (ui, buf);
       }
       draw_term (ui);
       ui_end (ui);
     }
     else if (os == 3 && elf_retval != 0)
     {
       ui_start (ui);
       sprintf (buf, "returned: %i", elf_retval);
       ui_text (ui, buf);
       ui_end (ui);
     }
  }
}
#endif

    int16_t tone[40960];
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
    for (int i = 0; i < 20480; i++)
    {
      tone [i*2 ] =
      tone [i*2+1] = sinf (i / 400.0);
    }

    ctx_pcm_queue (ctx, (void*)tone, 20480);

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

