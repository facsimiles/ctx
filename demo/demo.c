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

   if (ui_button(ui, "karplus strong"))
     ui_do(ui, "/sd/audio-ks.elf");
   if (ui_button(ui, "talk"))
     ui_do(ui, "/sd/talk.elf");
   if (ui_button(ui, "app"))
     ui_do(ui, "/sd/app.elf");
   if (ui_button(ui, "raw_fb"))
     ui_do(ui, "/sd/raw_fb.elf");
#if 0
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

  if (ui->view_elapsed > 1.0)
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

void _ctx_toggle_in_idle_dispatch (Ctx *ctx);
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

static int elf_execv (char *path, char **argv)
{
  int retval = 0;
  if ((!path) && argv && argv[0])
    path = argv[0];
  int argc = 0;
  if (argv)
    for (int i = 0; argv[i]; i++)
      argc++;
  FILE *file = fopen (path, "rb");
  if (file)
  {
     esp_elf_t elf;
     esp_elf_init(&elf);
     fseek(file, 0, SEEK_END);
     int length = ftell(file);
     printf ("len: %i\n", length);
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
       if (retval != 42) // TSR
       {
         printf ("deiniting elf %i\n", retval);
         esp_elf_deinit(&elf);
       }
     }
  }
  else
  {
    retval = -1;
  }
  return retval;
}
#else
#include <dlfcn.h>

int output_state = 0;
static void reset_elf_output_state (void)
{
  output_state = 0;
}
int  elf_output_state (void){
  return output_state;
}
static int elf_execv (char *path, char **argv)
{
  int argc = 0;
  if (argv)
  for (int i = 0; argv[i]; i++)
    argc++;
  void *dlhandle = dlopen (path, RTLD_NOW);
  if (dlhandle)
  {
     int (*main)(int argc, char **argv) = dlsym (dlhandle, "main");
     if (main)
     {
        output_state = 3;
        int ret = main(argc, argv);
        if (ret != 42)
          dlclose (dlhandle);
        output_state = 0;
     }
  }
  return 0;
}
#endif

typedef enum _ui_run_flags ui_run_flags;

enum _ui_run_flags {
  ui_run_flag_block = 0,
  ui_run_flag_async = 1,
  ui_run_flag_is_fun = 2,
};

static int elf_retval = 0;

#if 0
typedef struct process_info_t {
  char          *path;
  int            argc;
  char         **argv;
  Ui            *ui;
  ui_run_flags   flags;
} process_info_t;


static void *elf_task(void *_arg)
{
  process_info_t *pi = (process_info_t*)_arg;
  elf_retval = elf_execv (pi->path, pi->argv);
  reset_elf_output_state ();
  if (elf_retval == 0)
    ui_pop_fun (pi->ui);
  if (pi->argv)
  {
    for (int i = 0; i < pi->argc; i++)
      if (pi->argv[i]) free (pi->argv[i]);
  free (pi->argv);
  }
  free (pi);
#if CTX_FLOW3R
  vTaskDelete(NULL);
#else
  return NULL;
#endif
}

#include <unistd.h>
#if CTX_FLOW3R
static TaskHandle_t elf_task_handle;
#else
#include <pthread.h>
static pthread_t elf_thread;
#endif

static int ui_elf_run_task (Ui *ui, const char *path, char **argv)
{
  int argc = 0;
  if (argv)
  {
    for (int i = 0; argv[i]; i++)
      argc++;
  }

   process_info_t *pi= calloc (1, sizeof (process_info_t));
   pi->ui = ui;
   pi->path = strdup(path);
   pi->argc = argc;
   pi->argv = malloc (sizeof (char*) * (argc + 1));
   pi->argv[argc] = NULL;
   for (int i = 0; i < argc; i++)
     pi->argv[i] = strdup (argv[i]);
    reset_elf_output_state ();
#if CTX_FLOW3R
   BaseType_t res =
     xTaskCreate(elf_task, "elf", 16384, pi, ESP_TASK_PRIO_MIN+1,
                 &elf_task_handle);
   usleep(200 * 1000);
   return res == pdPASS;
#else
   pthread_create (&elf_thread, NULL, elf_task, pi);
   usleep(200 * 1000);
   return 0;
#endif
}
int launch_elf_task (Ctx *ctx, void *data)
{
  Ui *ui = data;
  if (launch_elf_handler) {
    ctx_remove_idle (ctx, launch_elf_handler);
    reset_elf_output_state ();
    ui_elf_run_task (ui, ui->location, NULL);
    launch_elf_handler = 0;
  }
  return 0;
}
#endif




static int launch_elf_handler = 0;

int launch_elf_blocking (Ctx *ctx, void *data)
{
  Ui *ui = data;
  if (launch_elf_handler) {
    ctx_remove_idle (ctx, launch_elf_handler);
    _ctx_toggle_in_idle_dispatch (ctx);
    reset_elf_output_state ();
    elf_retval = elf_execv (ui->location, NULL);
    _ctx_toggle_in_idle_dispatch (ctx);
    reset_elf_output_state ();
    if (elf_retval != 42)
      ui_pop_fun (ui); // leave view when done
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
  ctx_restore (ctx);
}

void view_elf(Ui *ui)
{
  if (ui->data == NULL)
  {
     launch_elf_handler = ctx_add_timeout (ui->ctx, 0, launch_elf_blocking, ui);
     ui->data = (void*)1;
  }
  else if(1)
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
     else if (os == 3)// && elf_retval != 0)
     {
       ui_start (ui);
       sprintf (buf, "returned: %i", elf_retval);
       ui_text (ui, buf);
       ui_end (ui);
     }
  }
}

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

    ui_register_view (ui, "splash", "os",    view_splash);
    ui_register_view (ui, "menu",   "menus", view_menu);
    ui_register_view (ui, "apps",   "menus", view_apps);

    ui_register_view (ui, "elf binary", ".elf", view_elf);

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

