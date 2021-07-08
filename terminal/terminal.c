#define _DEFAULT_SOURCE

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
#include <pty.h>
#include <math.h>
#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#endif

#include "vt-line.h"
#include "terminal.h"
#include "vt.h"
#include "../demos/itk.h"

Ctx *ctx = NULL; // initialized in main

//#define CTX_x            CTX_STRH('x',0,0,0,0,0,0,0,0,0,0,0,0,0)
//#define CTX_y            CTX_STRH('y',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_lower_bottom CTX_STRH('l','o','w','e','r','-','b','o','t','t','o','m',0,0)
#define CTX_lower        CTX_STRH('l','o','w','e','r',0,0,0,0,0,0,0,0,0)
#define CTX_raise        CTX_STRH('r','a','i','s','e',0,0,0,0,0,0,0,0,0)
#define CTX_raise_top    CTX_STRH('r','a','i','s','e','-','t','o','p',0,0,0,0,0)
#define CTX_terminate    CTX_STRH('t','e','r','m','i','n','a','t','e',0,0,0,0,0)
#define CTX_maximize     CTX_STRH('m','a','x','i','m','i','z','e',0,0,0,0,0,0)
#define CTX_unmaximize   CTX_STRH('u','n','m','a','x','i','m','i','z','e',0,0,0,0)
//#define CTX_width        CTX_STRH('w','i','d','t','h',0,0,0,0,0,0,0,0,0)
//#define CTX_title        CTX_STRH('t','i','t','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_title        15643372
#define CTX_action       CTX_STRH('a','c','t','i','o','n',0,0,0,0,0,0,0,0)
//#define CTX_height       CTX_STRH('h','e','i','g','h','t',0,0,0,0,0,0,0,0)

int ctx_renderer_is_sdl  (Ctx *ctx);
int ctx_renderer_is_fb   (Ctx *ctx);
int ctx_renderer_is_term (Ctx *ctx);

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);

typedef struct _CtxClient CtxClient;
CtxClient *vt_find_client (VT *vt);

void ctx_screenshot (Ctx *ctx, const char *path);
void
vt_screenshot (const char *output_path)
{
  ctx_screenshot (ctx, output_path);
}


void ctx_clients_signal_child (int signum);

#define VT_RECORD 0

static char *execute_self = NULL;

#include "ctx-clients.h"

float font_size    = -1;
float line_spacing = 2.0;



static void ensure_layout ()
{
  int n_clients = ctx_list_length (clients);
  if (n_clients == 1)
  {
    CtxClient *client = clients->data;
    if (client->flags & ITK_CLIENT_MAXIMIZED)
    {
      ctx_client_move (client->id, 0, 0);
      ctx_client_resize (client->id, ctx_width (ctx), ctx_height(ctx));
      if (active_tab == NULL)
        active_tab = client;
    }
  }
  else
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->flags & ITK_CLIENT_MAXIMIZED)
    {
      ctx_client_move (client->id, 0, ctx_client_min_y_pos (ctx));
      ctx_client_resize (client->id, ctx_width (ctx), ctx_height(ctx) -
                      ctx_client_min_y_pos (ctx) / 2);   // /2 to counter the double titlebar of non-maximized
      if (active_tab == NULL)
        active_tab = client;
    }
  }
}

/********************/
extern float _ctx_green;

float add_x = 10;
float add_y = 100;

/*****************/

#define flag_is_set(a, f) (((a) & (f))!=0)
#define flag_set(a, f)    ((a) |= (f));
#define flag_unset(a, f)  ((a) &= ~(f));

int add_tab (const char *commandline, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = 0;
  int flags = ITK_CLIENT_UI_RESIZABLE |  ITK_CLIENT_TITLEBAR;
  if (active) was_maximized = flag_is_set(active->flags, ITK_CLIENT_MAXIMIZED);
  if (can_launch) flags |= ITK_CLIENT_CAN_LAUNCH;

  active = ctx_client_new (commandline, add_x, add_y,
                    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2, flags);
  vt_set_ctx (active->vt, ctx);
  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    ctx_client_maximize (active->id);
    active_tab = active;
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
  ensure_layout ();
  return active->id;
}

int add_settings_tab (const char *commandline, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = 0;
  if (active) was_maximized = flag_is_set(active->flags, ITK_CLIENT_MAXIMIZED);
  int flags = ITK_CLIENT_UI_RESIZABLE |  ITK_CLIENT_TITLEBAR;
  if (can_launch) flags |= ITK_CLIENT_CAN_LAUNCH;

  active = ctx_client_new (commandline, add_x, add_y,
                    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2, flags);

  vt_set_ctx (active->vt, ctx);
  active->internal = 1;

  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    ctx_client_maximize (active->id);
    active_tab = active;
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
  ensure_layout ();
  return active->id;
}

static void add_tab_cb (CtxEvent *event, void *data, void *data2)
{
  event->stop_propagate = 1;
  add_tab (vt_find_shell_command(), 1);
}

static void add_settings_tab_cb (CtxEvent *event, void *data, void *data2)
{
  event->stop_propagate = 1;
  add_settings_tab (vt_find_shell_command(), 1);
}


void switch_to_tab (int desired_no)
{
  int no = 0;
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED))
    {
      if (no == desired_no)
      {
        active = active_tab = client;
        vt_rev_inc (active->vt);
        return;
      }
      no++;
    }
  }
}

void ctx_sdl_set_fullscreen (Ctx *ctx, int val);
int ctx_sdl_get_fullscreen (Ctx *ctx);


static void handle_event (Ctx *ctx, CtxEvent *ctx_event, const char *event)
{
  if (!active)
    return;
  if (active->internal)
    return;
  VT *vt = active->vt;

  if (!strcmp (event, "F11"))
  {
#ifndef NO_SDL
    if (ctx_renderer_is_sdl (ctx))
    {
      ctx_sdl_set_fullscreen (ctx, !ctx_sdl_get_fullscreen (ctx));
    }
#endif
  }
  else if (!strcmp (event, "shift-return"))
  {
    vt_feed_keystring (vt, ctx_event, "return");
  }
  else if (!strcmp (event, "shift-control-v") )
    {
      char *text = ctx_get_clipboard (ctx);
      if (text)
        {
          if (vt)
            vt_paste (vt, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-c") && vt)
    {
      char *text = vt_get_selection (vt);
      if (text)
        {
          ctx_set_clipboard (ctx, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-t") ||
           ((ctx_renderer_is_fb (ctx) || ctx_renderer_is_term (ctx))
           &&   !strcmp (event, "control-t") ))
  {
    add_tab (vt_find_shell_command(), 1);
  }
  else if (!strcmp (event, "shift-control-n") )
    {
      pid_t pid;
      if ( (pid=fork() ) ==0)
        {
          unsetenv ("CTX_VERSION");
          execlp (execute_self, execute_self, NULL);
          exit (0);
        }
    }


  else if (!strcmp (event, "alt-1"))   switch_to_tab(0);
  else if (!strcmp (event, "alt-2"))   switch_to_tab(1);
  else if (!strcmp (event, "alt-3"))   switch_to_tab(2);
  else if (!strcmp (event, "alt-4"))   switch_to_tab(3);
  else if (!strcmp (event, "alt-5"))   switch_to_tab(4);
  else if (!strcmp (event, "alt-6"))   switch_to_tab(5);
  else if (!strcmp (event, "alt-7"))   switch_to_tab(6);
  else if (!strcmp (event, "alt-8"))   switch_to_tab(7);
  else if (!strcmp (event, "alt-9"))   switch_to_tab(8);
  else if (!strcmp (event, "alt-0"))   switch_to_tab(9);
  else if (!strcmp (event, "shift-control-q") )
    {
      ctx_quit (ctx);
    }
  else if (!strcmp (event, "shift-control-w") )
    {
      active->do_quit = 1;
    }
  else if (!strcmp (event, "shift-control-s") )
    {
      if (vt)
      {
        char *sel = vt_get_selection (vt);
        if (sel)
        {
          vt_feed_keystring (vt, ctx_event, sel);
          free (sel);
        }
      }
    }
  else
    {
      if (vt)
        vt_feed_keystring (vt, ctx_event, event);
    }
}

static int ctx_clients_dirty_count (void)
{
  int changes = 0;
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if ((client->drawn_rev != vt_rev (client->vt) ) ||
        vt_has_blink (client->vt))
      changes++;
  }
  return changes;
}

static void ctx_client_close (CtxEvent *event, void *data, void *data2)
{
  //Ctx *ctx = event->ctx;
  CtxClient *client = data;

 // client->do_quit = 1;
  
  ctx_client_remove (event->ctx, client);

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void ctx_client_titlebar_drag (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  if (event->type == CTX_DRAG_RELEASE)
  {
    static int prev_drag_end_time = 0;
    if (event->time - prev_drag_end_time < 500)
    {
      //client_shade_toggle (client->id);
      ctx_client_maximized_toggle (client->id);
    }
    prev_drag_end_time = event->time;
  }

  float new_x = client->x +  event->delta_x;
  float new_y = client->y +  event->delta_y;

  float snap_threshold = 8;

  if (ctx_renderer_is_term (event->ctx))
     snap_threshold = 1;

  if (new_y < ctx_client_min_y_pos (ctx))
     new_y = ctx_client_min_y_pos (ctx);
  if (new_y > ctx_client_max_y_pos (ctx))
     new_y = ctx_client_max_y_pos (ctx);

  if (fabs (new_x - 0) < snap_threshold) new_x = 0.0;
  if (fabs (ctx_width (event->ctx) - (new_x + client->width)) < snap_threshold) new_x = ctx_width (ctx) - client->width;

  ctx_client_move (client->id, new_x, new_y);

  //vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);

  event->stop_propagate = 1;
}

static void ctx_client_titlebar_drag_maximized (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  active = active_tab = client;
  if (event->type == CTX_DRAG_RELEASE)
  {
    static int prev_drag_end_time = 0;
    if (event->time - prev_drag_end_time < 500)
    {
      //client_shade_toggle (client->id);
      ctx_client_unmaximize (client->id);
      ctx_client_raise_top (client->id);
      active_tab = NULL;
    }
    prev_drag_end_time = event->time;
  }
  ctx_set_dirty (event->ctx, 1);
  vt_rev_inc (client->vt);
  event->stop_propagate = 1;
}

int ctx_clients_draw (ITK *itk, Ctx *ctx);

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
    ensure_layout ();
    ctx_set_dirty (ctx, 1);
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

static void (*_popup)(Ctx *ctx, void *data) = NULL;
static void *_popup_data = NULL;

void ctx_set_popup (Ctx *ctx, void (*popup)(Ctx *ctx, void *data), void *popup_data)
{
  _popup = popup;
  _popup_data = popup_data;
}

void ctx_popups (Ctx *ctx)
{
  if (_popup)
  {
    _popup (ctx, _popup_data);
  }
  _popup = NULL;
}

void draw_titlebar (ITK *itk, Ctx *ctx, CtxClient *client,
                    float x, float y, float width, float titlebar_height)
{
#if 0
  ctx_move_to (ctx, x, y + height * 0.8);
  if (client == active)
    ctx_rgba (ctx, 1, 1,0.4, 1.0);
  else
    ctx_rgba (ctx, 1, 1,1, 0.8);
  ctx_text (ctx, client->title);
#else

      ctx_rectangle (ctx, x, y - titlebar_height,
                     width, titlebar_height);
      if (client == active)
         itk_style_color (ctx, "titlebar-focused-bg");
      else
         itk_style_color (ctx, "titlebar-bg");

      if (flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED) || y == titlebar_height)
      {
        ctx_listen (ctx, CTX_DRAG, ctx_client_titlebar_drag_maximized, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_ALL);
      }
      else
      {
        ctx_listen (ctx, CTX_DRAG, ctx_client_titlebar_drag, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_ALL);
      }
      ctx_fill (ctx);
      ctx_font_size (ctx, itk->font_size);//titlebar_height);// * 0.85);

      if (client == active &&
          (flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED) || y != titlebar_height))
      {

#if 1
      ctx_rectangle (ctx, x + width - titlebar_height,
                      y - titlebar_height, titlebar_height,
                      titlebar_height);
#endif
      ctx_rgb (ctx, 1, 0,0);
      ctx_listen (ctx, CTX_PRESS, ctx_client_close, client, NULL);
      ctx_listen_set_cursor (ctx, CTX_CURSOR_ARROW);
      //ctx_fill (ctx);
      ctx_begin_path (ctx);
      ctx_move_to (ctx, x + width - titlebar_height * 0.8, y - titlebar_height * 0.22);
      if (client == active)
        itk_style_color (ctx, "titlebar-focused-close");
      else
        itk_style_color (ctx, "titlebar-close");
      ctx_text (ctx, "X");
      }

      ctx_move_to (ctx, x +  width/2, y - titlebar_height * 0.22);
      if (client == active)
        itk_style_color (ctx, "titlebar-focused-fg");
      else
        itk_style_color (ctx, "titlebar-fg");

      ctx_save (ctx);
      ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
      if (client->title)
      {
        ctx_text (ctx, client->title);
      }
      else
      {
        ctx_text (ctx, "untitled");
      }
      ctx_restore (ctx);
#endif
}

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
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->flags & ITK_CLIENT_MAXIMIZED)
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
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->flags & ITK_CLIENT_MAXIMIZED)
    {
      ctx_begin_path (ctx);
      draw_titlebar (itk, ctx, client, x, titlebar_height,
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
  ctx_set_title (ctx, set_title);
}

int ctx_input_pending (Ctx *ctx, int timeout);
void ctx_clients_handle_events (Ctx *ctx);

void terminal_long_tap (Ctx *ctx, VT *vt)
{
  //fprintf (stderr, "long tap\n");
  //
  //  need deghosting or something on fbdev?
  //
  on_screen_keyboard = !on_screen_keyboard;
  ctx_set_dirty (ctx, 1);
}

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

  ctx  = ctx_new_ui (width, height);
  width = ctx_width (ctx);
  height = ctx_height (ctx);

  if (ctx_renderer_is_term (ctx) && font_size <= 0)
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
    font_size = floorf (width / cols  * 2 / 2);

  if (!commandline)
    commandline = vt_find_shell_command();
  ctx_client_maximize (add_tab (commandline, 1));

  if (!active)
    return 1;
  vt_set_ctx (active->vt, ctx);

  ITK *itk = itk_new (ctx);

  signal (SIGCHLD,ctx_clients_signal_child);

  ctx_add_timeout (ctx, 1000 * 200, malloc_trim_cb, NULL);

  //int sleep_time = 1000000/10;

  int print_shape_cache_rate = 0;
  if (getenv ("CTX_DEBUG_SHAPE_CACHE"))
    print_shape_cache_rate = 1;
  itk->scale = 1.0;
  itk->font_size = font_size;

  while (clients && !ctx_has_quit (ctx))
    {
      //int changes = 0;
      int n_clients = ctx_list_length (clients);
      ensure_layout ();

      if (print_shape_cache_rate)
        fprintf (stderr, "\r%f ", ctx_shape_cache_rate);

      if (ctx_clients_need_redraw (ctx) || ctx_is_dirty(ctx))
      {
        itk_reset (itk);
        ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
        itk_style_color (ctx, "wallpaper");
        ctx_fill (ctx);
        ctx_clients_draw (itk, ctx);
        ctx_popups (ctx);
        if ((n_clients != 1) || (clients && !flag_is_set((((CtxClient*)clients->data))->flags, ITK_CLIENT_MAXIMIZED)))
          draw_panel (itk, ctx);
        else
          draw_mini_panel (ctx);
        ctx_set_dirty (ctx, 0);
        ctx_osk_draw (ctx);
        //ctx_add_key_binding (ctx, "unhandled", NULL, "", terminal_key_any, NULL);
        ctx_listen (ctx, CTX_KEY_PRESS, terminal_key_any, NULL, NULL);
        ctx_listen (ctx, CTX_KEY_DOWN,  terminal_key_any, NULL, NULL);
        ctx_listen (ctx, CTX_KEY_UP,    terminal_key_any, NULL, NULL);
        ctx_flush (ctx);
      }

      ctx_clients_handle_events (ctx);
    }

  while (clients)
    ctx_client_remove (ctx, clients->data);

  itk_free (itk);
  ctx_free (ctx);

  if (getpid () == 1)
  {
    if (system ("reboot --force")){};
  }

  return 0;
}
