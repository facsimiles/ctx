
#if !__COSMOPOLITAN__
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
//#include <pty.h>
#include <math.h>
//#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#endif

#include "ctx.h"
#include "terminal.h"
#include "itk.h"

Ctx *ctx = NULL; // initialized in main

int ctx_renderer_is_sdl  (Ctx *ctx);
int ctx_renderer_is_fb   (Ctx *ctx);
int ctx_renderer_is_kms  (Ctx *ctx);
int ctx_renderer_is_term (Ctx *ctx);

typedef struct _CtxClient CtxClient;

void ctx_screenshot (Ctx *ctx, const char *path);
void
vt_screenshot (const char *output_path)
{
  ctx_screenshot (ctx, output_path);
}

void ctx_client_lock (CtxClient *client);
void ctx_client_unlock (CtxClient *client);

#define VT_RECORD 0

static char *execute_self = NULL;

float font_size    = -1;
float line_spacing = 2.0;

/********************/
extern float _ctx_green;

static float start_font_size = 22.0;

float add_x = 10;
float add_y = 100;

const char *ctx_find_shell_command (void);
/*****************/

#define flag_is_set(a, f) (((a) & (f))!=0)
#define flag_set(a, f)    ((a) |= (f));
#define flag_unset(a, f)  ((a) &= ~(f));

int add_tab (Ctx  *ctx, const char *commandline, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = ctx_client_is_maximized (ctx, ctx_clients_active (ctx));
  int flags = ITK_CLIENT_UI_RESIZABLE |  ITK_CLIENT_TITLEBAR;
  if (can_launch) flags |= ITK_CLIENT_CAN_LAUNCH;

  //ctx_font_size (ctx, start_font_size); // we pass it as arg instead
  CtxClient *active = ctx_client_new (ctx, commandline, add_x, add_y,
                    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2,
                    start_font_size,
                    flags, NULL, NULL);
  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    ctx_client_maximize (ctx, ctx_client_id (active));
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
//  ensure_layout (ctx);
  ctx_queue_draw (ctx);
  return ctx_client_id (active);
}

int add_tab_argv (Ctx  *ctx, char **argv, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = ctx_client_is_maximized (ctx, ctx_clients_active (ctx));
  int flags = ITK_CLIENT_UI_RESIZABLE |  ITK_CLIENT_TITLEBAR;
  if (can_launch) flags |= ITK_CLIENT_CAN_LAUNCH;

  //ctx_font_size (ctx, start_font_size); // we pass it as arg instead
  CtxClient *active = ctx_client_new_argv (ctx, argv, add_x, add_y,
                    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2,
                    start_font_size,
                    flags, NULL, NULL);
  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    ctx_client_maximize (ctx, ctx_client_id (active));
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
  ctx_queue_draw (ctx);
  return ctx_client_id (active);
}


static void settings_thread (Ctx *ctx, void *user_data)
{
   int frame = 0;
   while (!ctx_has_quit (ctx))
   {
     fprintf (stderr, "[%i]", frame++);
     if (ctx_need_redraw (ctx))
     {
        char buf[1024];
        ctx_start_frame (ctx);
        ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
        ctx_rgb (ctx, 0.1, 0.2, 0.3);
        ctx_fill (ctx);
        ctx_rgba (ctx, 1, 1, 1, 0.5);
        ctx_line_width (ctx, 5.0);
        ctx_move_to (ctx, 0,0);
        ctx_line_to (ctx, ctx_width (ctx), ctx_height (ctx));
        ctx_move_to (ctx, ctx_width (ctx), 0);
        ctx_line_to (ctx, 0, ctx_height (ctx));
        ctx_stroke (ctx);

        sprintf (buf, "frame: %i", frame);
        ctx_rgba (ctx, 1, 1, 0, 1);
        ctx_move_to (ctx, ctx_width (ctx) * 0.1, ctx_height (ctx) * 0.5);
        ctx_font_size (ctx, ctx_height (ctx) * 0.1);
        ctx_text (ctx, buf);

        ctx_end_frame (ctx);
     }
     if (frame > 100) ctx_quit (ctx);
     
     ctx_handle_events (ctx);
   }
}


int add_settings_tab (const char *commandline, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = ctx_client_is_maximized (ctx, ctx_clients_active (ctx));
  int flags = ITK_CLIENT_UI_RESIZABLE |  ITK_CLIENT_TITLEBAR;
  if (can_launch) flags |= ITK_CLIENT_CAN_LAUNCH;

  CtxClient *active = ctx_client_new_thread (ctx, settings_thread, add_x, add_y,
                    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2,
                    start_font_size,
                    flags, NULL, NULL);

  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    ctx_client_maximize (ctx, ctx_client_id (active));
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
  ctx_queue_draw (ctx);
  return ctx_client_id (active);
}

static void add_tab_cb (CtxEvent *event, void *data, void *data2)
{
  event->stop_propagate = 1;
  add_tab (event->ctx, ctx_find_shell_command(), 1);
}

static void add_settings_tab_cb (CtxEvent *event, void *data, void *data2)
{
  event->stop_propagate = 1;
  add_settings_tab (ctx_find_shell_command(), 1);
}

int ctx_clients_tab_to_id (Ctx *ctx, int tab_no);

void switch_to_tab (Ctx *ctx, int tab_no)
{
  int id = ctx_clients_tab_to_id (ctx, tab_no);
  if (id >= 0)
    ctx_client_maximize (ctx, id);
}

CtxClient *ctx_client_by_id (Ctx *ctx, int id);

CtxEvent *ctx_event_copy (CtxEvent *event);

static void handle_event (Ctx        *ctx,
                          CtxEvent   *ctx_event,
                          const char *event)
{
  CtxBackendType backend_type = ctx_backend_type (ctx);
  int active_id = ctx_clients_active (ctx);
  CtxClient *active = active_id>=0?ctx_client_by_id (ctx, active_id):NULL;
  if (!active)
    return;
  //if (active->internal)
  //  return;
  VT *vt = ctx_client_vt (active);

  CtxClient *client = active; //vt_get_client (vt);

  if (!vt)
  {
     ctx_client_add_event (active, ctx_event);
     return;
  }

  ctx_client_lock (client);

  if (!strcmp (event, "F11"))
  {
    ctx_set_fullscreen (ctx, !ctx_get_fullscreen (ctx));
  }
  else if (!strcmp (event, "shift-return"))
  {
    ctx_client_feed_keystring (active, ctx_event, "return");
  }
  else if (!strcmp (event, "shift-control-v") )
    {
      char *text = ctx_get_clipboard (ctx);
      if (text)
        {
          ctx_client_paste (ctx, active_id, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-c") && vt)
    {
      char *text = ctx_client_get_selection (ctx, active_id);
      if (text)
        {
          ctx_set_clipboard (ctx, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-t") ||
           ((backend_type == CTX_BACKEND_FB ||  // workaround for not having
            backend_type == CTX_BACKEND_TERM ||  // raw keyboard acces
            backend_type == CTX_BACKEND_KMS)
           &&   !strcmp (event, "control-t") ))
  {
    add_tab (ctx, ctx_find_shell_command(), 1);
  }
  else if (!strcmp (event, "shift-control-n") )
    {
      pid_t pid;
      if ( (pid=fork() ) ==0)
        {
          unsetenv ("CTX_VERSION");
          unsetenv ("CTX_BACKEND");
          execlp (execute_self, execute_self, NULL);
          exit (0);
        }
    }


  else if (!strcmp (event, "alt-1"))   switch_to_tab(ctx, 0);
  else if (!strcmp (event, "alt-2"))   switch_to_tab(ctx, 1);
  else if (!strcmp (event, "alt-3"))   switch_to_tab(ctx, 2);
  else if (!strcmp (event, "alt-4"))   switch_to_tab(ctx, 3);
  else if (!strcmp (event, "alt-5"))   switch_to_tab(ctx, 4);
  else if (!strcmp (event, "alt-6"))   switch_to_tab(ctx, 5);
  else if (!strcmp (event, "alt-7"))   switch_to_tab(ctx, 6);
  else if (!strcmp (event, "alt-8"))   switch_to_tab(ctx, 7);
  else if (!strcmp (event, "alt-9"))   switch_to_tab(ctx, 8);
  else if (!strcmp (event, "alt-0"))   switch_to_tab(ctx, 9);
  else if (!strcmp (event, "shift-control-q") )
    {
      ctx_quit (ctx);
    }
  else if (!strcmp (event, "shift-control-w") )
    {
      ctx_client_quit (active);
    }
  else if (!strcmp (event, "shift-control-s") )
    {
      if (vt)
      {
        char *sel = ctx_client_get_selection (ctx, ctx_client_id (active));
        if (sel)
        {
          ctx_client_feed_keystring (active, ctx_event, sel);
          free (sel);
        }
      }
    }
  else
    {
      ctx_client_feed_keystring (active, ctx_event, event);
    }
  ctx_client_unlock (client);
}

static void ctx_client_close (CtxEvent *event, void *data, void *data2)
{
  Ctx *ctx = event->ctx;
  CtxList *clients = ctx_clients (ctx);
  CtxClient *client = data;

 // client->do_quit = 1;
  
  ctx_client_remove (ctx, client);
  if (clients == NULL)
    ctx_quit (ctx);//

  ctx_queue_draw (ctx);
  event->stop_propagate = 1;
}

static float consume_float (char **argv, int *i)
{
 float ret = 0.0;
 if (!argv[*i+1])
 {
   fprintf (stderr, "error parsing arguments\n");
   exit (2);
 }
 ret = atof (argv[*i+1]);
 (*i)++;
  int j;
  for (j = *i; argv[j]; j++)
  {
    argv[j-2] = argv[j];
  }
  argv[j-2]=0;
  (*i)-=2;
  return ret;
}
// good font size for 80col:
//
// 4 6 8 10 12 14 16 18 20 2
extern int on_screen_keyboard;
void ctx_osk_draw (Ctx *ctx);

static int malloc_trim_cb (Ctx *ctx, void *data)
{
#if GNU_C
  malloc_trim (64*1024);
#endif
  return 1;
}

static void terminal_key_any (CtxEvent *event, void *userdata, void *userdata2)
{
  if (event->type == CTX_KEY_PRESS &&
      event->string &&
      !strcmp (event->string, "resize-event"))
  {
    ctx_queue_draw (ctx);
  }
  else
  {
    switch (event->type)
    {
      case CTX_KEY_PRESS:
        handle_event (ctx, event, event->string);
        break;
      case CTX_KEY_UP:
        { char buf[1024];
          snprintf (buf, sizeof(buf)-1, "keyup %i %i", event->unicode, event->state);
          handle_event (ctx, event, buf);
        }
        break;
      case CTX_KEY_DOWN:
        { char buf[1024];
          snprintf (buf, sizeof(buf)-1, "keydown %i %i", event->unicode, event->state);
          handle_event (ctx, event, buf);
        }
        break;
      default:
        break;
    }
  }
}

extern int _ctx_enable_hash_cache;

void ctx_client_titlebar_draw (Ctx *ctx, CtxClient *client,
                               float x, float y, float width, float titlebar_height);

void draw_panel (ITK *itk, Ctx *ctx)
{
  struct tm local_time_res;
  struct timeval tv;
  gettimeofday (&tv, NULL);
  localtime_r (&tv.tv_sec, &local_time_res);

  float titlebar_height = font_size;
  float tab_width = ctx_width (ctx) - titlebar_height * 4 - titlebar_height * 2;

  ctx_save (ctx);
  ctx_rectangle (ctx, 0, 0, ctx_width (ctx), titlebar_height);
  ctx_gray (ctx, 0.0);
  ctx_fill (ctx);
  ctx_font_size (ctx, titlebar_height * 1.0);
  ctx_move_to (ctx, ctx_width (ctx), titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_END);
  ctx_gray (ctx, 0.9);
  char buf[128];
  sprintf (buf, "%02i:%02i:%02i", local_time_res.tm_hour, local_time_res.tm_min, local_time_res.tm_sec);
  ctx_text (ctx, buf);

  ctx_begin_path (ctx);
  ctx_rectangle (ctx, ctx_width(ctx)-titlebar_height * 10, 0, titlebar_height * 10, titlebar_height);
  ctx_listen (ctx, CTX_PRESS, add_settings_tab_cb, NULL, NULL);

  int tabs = 0;
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (ctx_client_flags (client) & ITK_CLIENT_MAXIMIZED)
    tabs ++;
  }

  if (tabs)
  tab_width /= tabs;

  ctx_begin_path (ctx);
  ctx_rectangle (ctx, 0, 0, titlebar_height * 1.5, titlebar_height);
  ctx_listen (ctx, CTX_PRESS, add_tab_cb, NULL, NULL);
  ctx_move_to (ctx, titlebar_height * 1.5/2, titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_gray (ctx, 0.9);
  ctx_text (ctx, "+");

  float x = titlebar_height * 1.5;
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (ctx_client_flags (client) & ITK_CLIENT_MAXIMIZED)
    {
      ctx_begin_path (ctx);
      ctx_client_titlebar_draw (ctx, client, x, titlebar_height,
                     tab_width, titlebar_height);
    }
    x += tab_width;
  }
  ctx_restore (ctx);
}

void draw_mini_panel (Ctx *ctx)
{
  float titlebar_height = font_size;

  ctx_save (ctx);
  ctx_font_size (ctx, titlebar_height * 0.9);

  ctx_rectangle (ctx, 0, 0, titlebar_height * 1.5, titlebar_height);
  ctx_listen (ctx, CTX_PRESS, add_tab_cb, NULL, NULL);
  ctx_move_to (ctx, titlebar_height * 1.5/2, titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_rgba (ctx, 0.9, 0.9, 0.9, 0.5);
  ctx_text (ctx, "+");

  ctx_restore (ctx);
}

static char *set_title = NULL;
void vt_audio_task (VT *vt, int click);


void
terminal_update_title (const char *title)
{
  if (!title)title="NULL";
  if (set_title)
  {
    if (!strcmp (set_title, title))
    {
      return;
    }
    free (set_title);
  }
  set_title = strdup (title);
  ctx_windowtitle (ctx, set_title);
}

int ctx_input_pending (Ctx *ctx, int timeout);

void terminal_long_tap (Ctx *ctx, VT *vt)
{
  //fprintf (stderr, "long tap\n");
  //
  //  need deghosting or something on fbdev?
  //
  on_screen_keyboard = !on_screen_keyboard;
  ctx_queue_draw (ctx);
}

int commandline_argv_start = 0;

int terminal_main (int argc, char **argv)
{
  execute_self = argv[0];
  const char *commandline = NULL;
  float global_scale = 1.0;
  int width = -1;
  int height = -1;
  int cols = -1;

  const char *env_val = getenv ("CTX_GREEN");
  if (env_val)
  {
    float val = atof (env_val);
    _ctx_green = val;
    if (_ctx_green > 1.0) _ctx_green = 1.0;
    if (_ctx_green < 0.0) _ctx_green = 0.0;
  }

  if (getpid () == 1)
  {
    int ign;
    ign = system ("pkill plymouth"); // needed to enable keyboard input.. with the initrd that
                               // gets used with systemd
    ign = system ("mount -o remount,rw /");
    ign = system ("mount -a");
    if (ign) {};
  }

  for (int i = 1; argv[i]; i++)
  {
    if (!strcmp (argv[i], "--help"))
    {
    }
    else if (!strcmp (argv[i], "-e"))
    {
      commandline = argv[i+1];
      i++;
      if (!strcmp (commandline, "--"))
      {
        i++;
        commandline_argv_start = i;
        fprintf (stderr, "%s\n", argv[i]);
      }
    }
    else if (!strcmp (argv[i], "--width"))
        width = consume_float (argv, &i);
    else if (!strcmp (argv[i], "--height"))
        height = consume_float (argv, &i);
    else if (!strcmp (argv[i], "--cols"))
        cols = consume_float (argv, &i);
    else if (!strcmp (argv[i], "--font-size"))
        font_size = consume_float (argv, &i) * global_scale;
    else if (argv[i][0]=='-')
    {
      switch (argv[i][1])
      {
        case 's': font_size = consume_float (argv, &i) * global_scale; break;
        case 'c': cols = consume_float (argv, &i); break;
        case 'w': width = consume_float (argv, &i); break;
        case 'h': height = consume_float (argv, &i); break;
      }
    }
  }

  if (font_size > 0 && width <0 && cols < 0)
  {
    cols = 80;
    width = font_size * cols / 2;
    height = font_size * 25;
  }

  ctx  = ctx_new (width, height, NULL);
  width = ctx_width (ctx);
  height = ctx_height (ctx);

  if (ctx_backend_type (ctx) == CTX_BACKEND_TERM && font_size <= 0)
  {
    font_size = ctx_term_get_cell_height (ctx);
  }

  if (cols <= 0)
  {
    //if (((double)(width))/height < (16.0/9.0))
    //  cols = 80;
    //else
      cols = 80;
  }

  if (font_size < 0)
    font_size = floorf (width / cols  * 2 / 1.5);

  ITK *itk = itk_new (ctx);

  itk->scale = 1.0;
  itk->font_size = font_size;
  start_font_size = font_size;
  ctx_font_size (ctx, font_size);

  if (commandline_argv_start)
  {
    ctx_client_maximize (ctx, add_tab_argv (ctx, &argv[commandline_argv_start], 1));
  }
  else
  {
    if (!commandline)
      commandline = ctx_find_shell_command();
    ctx_client_maximize (ctx, add_tab (ctx, commandline, 1));
  }

  ctx_add_timeout (ctx, 1000 * 200, malloc_trim_cb, NULL);

  //int sleep_time = 1000000/10;

  int print_shape_cache_rate = 0;
  if (getenv ("CTX_DEBUG_SHAPE_CACHE"))
    print_shape_cache_rate = 1;

  while (ctx_clients (ctx) && !ctx_has_quit (ctx))
    {
      //int changes = 0;
      int n_clients = ctx_list_length (ctx_clients (ctx));
      //ensure_layout (ctx);

      if (print_shape_cache_rate)
        fprintf (stderr, "\r%f ", ctx_shape_cache_rate);

      if (ctx_need_redraw(ctx))
      {
        itk_reset (itk);
        ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
        itk_style_color (ctx, "wallpaper");
        ctx_fill (ctx);
        ctx_font_size (ctx, itk->font_size);
        ctx_clients_draw (ctx, 0);
        if ((n_clients != 1) || (ctx_clients (ctx) &&
                                 !flag_is_set(
                                         ctx_client_flags (((CtxClient*)ctx_clients(ctx)->data)), ITK_CLIENT_MAXIMIZED)))
          draw_panel (itk, ctx);
        else
          draw_mini_panel (ctx);
        ctx_osk_draw (ctx);
        //ctx_add_key_binding (ctx, "unhandled", NULL, "", terminal_key_any, NULL);
        ctx_listen (ctx, CTX_KEY_PRESS, terminal_key_any, NULL, NULL);
        ctx_listen (ctx, CTX_KEY_DOWN,  terminal_key_any, NULL, NULL);
        ctx_listen (ctx, CTX_KEY_UP,    terminal_key_any, NULL, NULL);
        ctx_end_frame (ctx);
      }

     {
     int active_id = ctx_clients_active (ctx);
     CtxClient *active = active_id>=0?ctx_client_by_id (ctx, active_id):NULL;
     if (active)
       terminal_update_title (ctx_client_title (active));
     }

     ctx_handle_events (ctx);
    }

  itk_free (itk);
  ctx_free (ctx);

  if (getpid () == 1)
  {
    if (system ("reboot --force")){};
  }

  return 0;
}
