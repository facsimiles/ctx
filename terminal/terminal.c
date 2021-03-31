#define _DEFAULT_SOURCE

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


#include "vt-line.h"
#include "terminal.h"
#include "vt.h"
#include "../clients/itk.h"

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

int ctx_renderer_is_sdl (Ctx *ctx);
int ctx_renderer_is_fb (Ctx *ctx);
int ctx_renderer_is_braille (Ctx *ctx);

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

int vtpty_waitdata (void  *data, int timeout)
{
  VtPty *vtpty = data;
  struct timeval tv;
  fd_set fdset;
  FD_ZERO (&fdset);
  FD_SET (vtpty->pty, &fdset);
  tv.tv_sec = 0;
  tv.tv_usec = timeout;
  tv.tv_sec  = timeout / 1000000;
  tv.tv_usec = timeout % 1000000;
  if (select (vtpty->pty+1, &fdset, NULL, NULL, &tv) == -1)
    {
      perror ("select");
      return 0;
    }
  if (FD_ISSET (vtpty->pty, &fdset) )
    {
      return 1;
    }
  return 0;
}

CtxList *vts = NULL;

static void signal_child (int signum)
{
  pid_t pid;
  int   status;
  if ( (pid = waitpid (-1, &status, WNOHANG) ) != -1)
    {
      if (pid)
        {
          for (CtxList *l = vts; l; l=l->next)
            {
              VtPty *vt = l->data;
              if (vt->pid == pid)
                {
                  vt->done = 1;
                  //vt->result = status;
                }
            }
        }
    }
}

void vtpty_resize (void *data, int cols, int rows, int px_width, int px_height)
{
  VtPty *vtpty = data;
  struct winsize ws;
  ws.ws_row = rows;
  ws.ws_col = cols;
  ws.ws_xpixel = px_width;
  ws.ws_ypixel = px_height;
  ioctl (vtpty->pty, TIOCSWINSZ, &ws);
}

ssize_t vtpty_write (void *data, const void *buf, size_t count)
{
  VtPty *vtpty = data;
  return write (vtpty->pty, buf, count);
}

ssize_t vtpty_read (void  *data, void *buf, size_t count)
{
  VtPty *vtpty = data;
  return read (vtpty->pty, buf, count);
}

struct
_CtxClient {
  VT           *vt;
  char         *title;
  int           x;
  int           y;
  int           width;
  int           height;
  int           shaded;
  int           iconified;
  int           maximized;
  int           unmaximized_x;
  int           unmaximized_y;
  int           unmaximized_width;
  int           unmaximized_height;
  int           do_quit;
  long          drawn_rev;
  int           id;
};

int vt_set_prop (VT *vt, uint32_t key_hash, const char *val)
{
#if 1
  switch (key_hash)
  {
    case CTX_title:  
  // ctx_set (ctx, CTX_title, val, strlen (val));
     {
       CtxClient *client = vt_find_client (vt);
       if (client)
       {
         if (client->title) free (client->title);
         client->title = strdup (val);
       }
     }

     break;
  }
#else
  float fval = strtod (val, NULL);
  CtxClient *client = client_by_id (ct->id);
  uint32_t val_hash = ctx_strhash (val, 0);
  if (!client)
    return 0;

  if (key_hash == ctx_strhash("start_move", 0))
  {
    start_moving (client);
    moving_client = 1;
    return 0;
  }

// set "pcm-hz"       "8000"
// set "pcm-bits"     "8"
// set "pcm-encoding" "ulaw"
// set "play-pcm"     "d41ata312313"
// set "play-pcm-ref" "foo.wav"

// get "free"
// storage of blobs for referencing when drawing or for playback
// set "foo.wav"      "\3\1\1\4\"
// set "fnord.png"    "PNG12.4a312"

  switch (key_hash)
  {
    case CTX_title:  client_set_title (ct->id, val); break;
    case CTX_x:      client->x = fval; break;
    case CTX_y:      client->y = fval; break;
    case CTX_width:  client_resize (ct->id, fval, client->height); break;
    case CTX_height: client_resize (ct->id, client->width, fval); break;
    case CTX_action:
      switch (val_hash)
      {
        case CTX_maximize:     client_maximize (client); break;
        case CTX_unmaximize:   client_unmaximize (client); break;
        case CTX_lower:        client_lower (client); break;
        case CTX_lower_bottom: client_lower_bottom (client);  break;
        case CTX_raise:        client_raise (client); break;
        case CTX_raise_top:    client_raise_top (client); break;
      }
      break;
  }
  ct->rev++;
#endif
  return 0;
}


float font_size    = -1;
float line_spacing = 2.0;

int   on_screen_keyboard = 0;

static char *execute_self = NULL;

static CtxList *clients;
static CtxClient *active = NULL;
static CtxClient *active_tab = NULL;

static CtxClient *client_by_id (int id);

int client_resize (int id, int width, int height);
void client_maximize (int id);

void terminal_long_tap (Ctx *ctx, VT *vt)
{
#if 0
  // triggers undesirably under fbdev
  on_screen_keyboard = !on_screen_keyboard;
  vt_rev_inc (vt); // forcing redraw
#endif
}

CtxClient *vt_find_client (VT *vt)
{
  for (CtxList *l = clients; l; l =l->next)
  {
    CtxClient *client = l->data;
    if (client->vt == vt)
            return client;
  }
  return NULL;
}

CtxClient *add_client (const char *commandline, int x, int y, int width, int height, int ctx, int can_launch)
{
  static int global_id = 0;
  CtxClient *client = calloc (sizeof (CtxClient), 1);
  ctx_list_append (&clients, client);
  client->id = global_id++;
  client->x = x;
  client->y = y;

  client->width = width;
  client->height = height;
  client->vt = vt_new (commandline, width, height, font_size, line_spacing, client->id, can_launch);
  return client;
}

CtxClient *add_client_argv (const char **argv, int x, int y, int width, int height, int ctx, int can_launch)
{
  CtxString *string = ctx_string_new ("");
  for (int i = 0; argv[i]; i++)
  {
    if (i > 0)
      ctx_string_append_byte (string, ' ');
    for (int c = 0; argv[i][c]; c++)
    {
       switch (argv[i][c])
       {
         case '"':ctx_string_append_str (string, "\\\"");break;
         case '\'':ctx_string_append_str (string, "\\\'");break;
         default:ctx_string_append_byte (string, argv[i][c]);break;
       }
    }
  }
  CtxClient *ret = add_client (string->str, x, y, width, height, ctx, can_launch);
  ctx_string_free (string, 1);
  return ret;
}

extern float ctx_shape_cache_rate;
extern int _ctx_max_threads;

static int focus_follows_mouse = 0;

static CtxClient *find_active (int x, int y)
{
  CtxClient *ret = NULL;
  float view_height = ctx_height (ctx);
  float titlebar_height = view_height/40;
  int resize_border = titlebar_height/2;

  for (CtxList *l = clients; l; l = l->next)
  {
     CtxClient *c = l->data;
     if (c->maximized && c == active_tab)
     if (x > c->x - resize_border && x < c->x+c->width + resize_border &&
         y > c->y - titlebar_height && y < c->y+c->height + resize_border)
     {
       ret = c;
     }
  }

  for (CtxList *l = clients; l; l = l->next)
  {
     CtxClient *c = l->data;
     if (!c->maximized)
     if (x > c->x - resize_border && x < c->x+c->width + resize_border &&
         y > c->y - titlebar_height && y < c->y+c->height + resize_border)
     {
       ret = c;
     }
  }
  return ret;
}

int id_to_no (int id)
{
  CtxList *l;
  int no = 0;

  for (l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->id == id)
      return no;
    no++;
  }
  return -1;
}

float add_x = 10;
float add_y = 100;


void client_move (int id, int x, int y);
int client_resize (int id, int w, int h);
void client_shade_toggle (int id);
static float client_min_y_pos (Ctx *ctx);
static float client_max_y_pos (Ctx *ctx);

void ensure_layout ()
{
  int n_clients = ctx_list_length (clients);
  if (n_clients == 1)
  {
    CtxClient *client = clients->data;
    if (client->maximized)
    {
      client_move (client->id, 0, 0);
      client_resize (client->id, ctx_width (ctx), ctx_height(ctx));
      if (active_tab == NULL)
        active_tab = client;
    }
  }
  else
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->maximized)
    {
      client_move (client->id, 0, client_min_y_pos (ctx));
      client_resize (client->id, ctx_width (ctx), ctx_height(ctx) -
                      client_min_y_pos (ctx) / 2);   // /2 to counter the double titlebar of non-maximized
      if (active_tab == NULL)
        active_tab = client;
    }
  }
}

int add_tab (const char *commandline, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = 0;
  if (active) was_maximized = active->maximized;

  active = add_client (commandline, add_x, add_y,
                    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2, 0, can_launch);
  vt_set_ctx (active->vt, ctx);
  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    client_maximize (active->id);
    active_tab = active;
  }

  if (add_y + ctx_height(ctx)/2 > client_max_y_pos (ctx))
  {
    add_y = client_min_y_pos (ctx);
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

static CtxClient *client_by_id (int id)
{
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->id == id)
      return client;
  }
  return NULL;
}

void client_remove (Ctx *ctx, CtxClient *client)
{
  if (client->vt)
    vt_destroy (client->vt);

  if (client->title)
    free (client->title);

  ctx_list_remove (&clients, client);
  if (clients == NULL)
    ctx_quit (ctx);//

  if (client == active_tab)
  {
    active_tab = NULL;
  }

  if (client == active)
  {
    active = find_active (ctx_pointer_x (ctx), ctx_pointer_y (ctx));
    if (!active) active = clients?clients->data:NULL;
  }
  free (client);
  ensure_layout();
}

#if 0
void client_remove_by_id (int id)
{
  int no = id_to_no (id);
  if (no>=0)
    client_remove (no);
}
#endif

void switch_to_tab (int desired_no)
{
  int no = 0;
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->maximized)
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

static void handle_event (Ctx *ctx, const char *event)
{
  if (!active)
    return;
  VT *vt = active->vt;
  if (!strcmp (event, "shift-return"))
  {
    vt_feed_keystring (vt, "return");
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
           (ctx_renderer_is_fb (ctx) &&   !strcmp (event, "control-t") ))
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
          vt_feed_keystring (vt, sel);
          free (sel);
        }
      }
    }
  else
    {
      if (vt)
        vt_feed_keystring (vt, event);
    }
}

int client_height (int id)
{
  CtxClient *client = client_by_id (id);
  if (!client) return 0;
  return client->height;
}

int client_x (int id)
{
  CtxClient *client = client_by_id (id);
  if (!client) return 0;
  return client->x;
}

int client_y (int id)
{
  CtxClient *client = client_by_id (id);
  if (!client) return 0;
  return client->y;
}

void client_raise_top (int id)
{
  CtxClient *client = client_by_id (id);
  if (!client) return;
  ctx_list_remove (&clients, client);
  ctx_list_append (&clients, client);
}

void client_lower_bottom (int id)
{
  CtxClient *client = client_by_id (id);
  if (!client) return;
  ctx_list_remove (&clients, client);
  ctx_list_prepend (&clients, client);
}


void client_iconify (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   client->iconified = 1;
}

int client_is_iconified (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return -1;
   return client->iconified;
}

void client_uniconify (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   client->iconified = 0;
}

void client_maximize (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   if (client->maximized)
     return;
   client->maximized = 1;
   client->unmaximized_x = client->x;
   client->unmaximized_y = client->y;
   client->unmaximized_width  = client->width;
   client->unmaximized_height = client->height;

   // enforce_layout does the size
   //client_resize (id, ctx_width (ctx), ctx_height(ctx) - client_min_y_pos (ctx));
   
   client_move (id, 0, client_min_y_pos (ctx));
   active_tab = client;
}

int client_is_maximized (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return -1;
   return client->maximized;
}

void client_unmaximize (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   if (client->maximized == 0)
     return;
   client->maximized = 0;
   client_resize (id, client->unmaximized_width, client->unmaximized_height);
   client_move (id, client->unmaximized_x, client->unmaximized_y);
   active_tab = NULL;
}

void client_maximized_toggle (int id)
{
  if (client_is_maximized (id))
    client_unmaximize (id);
  else
    client_maximize (id);
}


void client_shade (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   client->shaded = 1;
}

int client_is_shaded (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return -1;
   return client->shaded;
}

void client_unshade (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   client->shaded = 0;
}

void client_toggle_maximized (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   if (client->maximized)
     client_unmaximize (id);
   else
     client_maximize (id);
}

void client_shade_toggle (int id)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   client->shaded = !client->shaded;
}

void client_move (int id, int x, int y)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   client->x = x;
   client->y = y;
}

int client_resize (int id, int width, int height)
{
   CtxClient *client = client_by_id (id);

   if (client && ((height != client->height) || (width != client->width) ))
   {
     client->width  = width;
     client->height = height;
     vt_set_px_size (client->vt, width, height);
     return 1;
   }
   return 0;
}

static int dirty = 0;

int ctx_count (Ctx *ctx);

static int vt_dirty_count (void)
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

static void client_drag_maximized (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  active = active_tab = client;
  if (event->type == CTX_DRAG_RELEASE)
  {
    static int prev_drag_end_time = 0;
    if (event->time - prev_drag_end_time < 500)
    {
      //client_shade_toggle (client->id);
      client_unmaximize (client->id);
      client_raise_top (client->id);
      active_tab = NULL;
    }
    prev_drag_end_time = event->time;
  }
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static float client_min_y_pos (Ctx *ctx)
{
  float titlebar_height = ctx_height (ctx)/40;
  return titlebar_height * 2; // a titlebar and a panel
}

static float client_max_y_pos (Ctx *ctx)
{
  return ctx_height (ctx);
}

static void client_drag (CtxEvent *event, void *data, void *data2)
{
  //Ctx *ctx = event->ctx;
  CtxClient *client = data;
  //float titlebar_height = ctx_height (event->ctx)/40;

//client->x += event->delta_x;
//client->y += event->delta_y;
  if (event->type == CTX_DRAG_RELEASE)
  {
    static int prev_drag_end_time = 0;
    if (event->time - prev_drag_end_time < 500)
    {
      //client_shade_toggle (client->id);
      client_maximized_toggle (client->id);
    }
    prev_drag_end_time = event->time;
  }

  float new_x = client->x +  event->delta_x;
  float new_y = client->y +  event->delta_y;

  float snap_threshold = 8;

  if (new_y < client_min_y_pos (ctx)) new_y = client_min_y_pos (ctx);
  if (new_y > client_max_y_pos (ctx)) new_y = client_max_y_pos (ctx);

  if (fabs (new_x - 0) < snap_threshold) new_x = 0.0;
  if (fabs (ctx_width (event->ctx) - (new_x + client->width)) < snap_threshold) new_x = ctx_width (ctx) - client->width;

  client_move (client->id, new_x, new_y);

  ctx_set_dirty (event->ctx, 1);

  event->stop_propagate = 1;
}

static float min_win_dim = 32;

static void client_resize_se (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  int new_w = client->width + event->delta_x;
  int new_h = client->height + event->delta_y;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  client_resize (client->id, new_w, new_h);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void client_resize_e (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  int new_w = client->width + event->delta_x;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  client_resize (client->id, new_w, client->height);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void client_resize_s (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  int new_h = client->height + event->delta_y;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  client_resize (client->id, client->width, new_h);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void client_resize_n (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  float new_y = client->y +  event->delta_y;
  int new_h = client->height - event->delta_y;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  client_resize (client->id, client->width, new_h);
  client_move (client->id, client->x, new_y);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void client_resize_ne (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  float new_y = client->y +  event->delta_y;
  int new_h = client->height - event->delta_y;
  int new_w = client->width + event->delta_x;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  client_resize (client->id, new_w, new_h);
  client_move (client->id, client->x, new_y);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void client_resize_sw (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  float new_x = client->x +  event->delta_x;
  int new_w = client->width - event->delta_x;
  int new_h = client->height + event->delta_y;

  if (new_h <= min_win_dim) new_h = min_win_dim;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  client_resize (client->id, new_w, new_h);
  client_move (client->id, new_x, client->y);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void client_resize_nw (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  float new_x = client->x +  event->delta_x;
  float new_y = client->y +  event->delta_y;
  int new_w = client->width - event->delta_x;
  int new_h = client->height - event->delta_y;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  client_resize (client->id, new_w, new_h);
  client_move (client->id, new_x, new_y);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void client_resize_w (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  float new_x = client->x +  event->delta_x;
  int new_w = client->width - event->delta_x;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  client_resize (client->id, new_w, client->height);
  client_move (client->id, new_x, client->y);
  if (client->vt) // force redraw
    vt_rev_inc (client->vt);
  ctx_set_dirty (event->ctx, 1);

  event->stop_propagate = 1;
}

static void client_close (CtxEvent *event, void *data, void *data2)
{
  //Ctx *ctx = event->ctx;
  CtxClient *client = data;

 // client->do_quit = 1;
  
  client_remove (event->ctx, client);

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

void draw_titlebar (Ctx *ctx, CtxClient *client,
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

      if (client->maximized || y == titlebar_height)
      {
        ctx_listen (ctx, CTX_DRAG, client_drag_maximized, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_ALL);
      }
      else
      {
        ctx_listen (ctx, CTX_DRAG, client_drag, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_ALL);
      }
      ctx_fill (ctx);
      ctx_font_size (ctx, titlebar_height * 0.85);

      if (client == active &&
          (client->maximized || y != titlebar_height))
      {

#if 1
      ctx_rectangle (ctx, x + width - titlebar_height,
                      y - titlebar_height, titlebar_height,
                      titlebar_height);
#endif
      ctx_rgb (ctx, 1, 0,0);
      ctx_listen (ctx, CTX_PRESS, client_close, client, NULL);
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

static void key_down (CtxEvent *event, void *data1, void *data2)
{
  fprintf (stderr, "down %i %s\n", event->unicode, event->string);
}
static void key_up (CtxEvent *event, void *data1, void *data2)
{
  fprintf (stderr, "up %i %s\n", event->unicode, event->string);
}
static void key_press (CtxEvent *event, void *data1, void *data2)
{
  fprintf (stderr, "press %i %s\n", event->unicode, event->string);
}

static int draw_vts (Ctx *ctx)
{
  float view_height = ctx_height (ctx);
  float titlebar_height = view_height/40;
  int n_clients = ctx_list_length (clients);
  //ctx_listen (ctx, CTX_KEY_DOWN, key_down, NULL, NULL);
  //ctx_listen (ctx, CTX_KEY_UP, key_up, NULL, NULL);
  //ctx_listen (ctx, CTX_KEY_PRESS, key_press, NULL, NULL);
#if 1
  if (active && active->maximized && n_clients == 1)
  {
    vt_draw (active->vt, ctx, 0, 0);//titlebar_height);
    active->drawn_rev = vt_rev (active->vt);
    return 0;
  }
#endif
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client == active_tab && client->maximized)
    {
      vt_draw (client->vt, ctx, 0, titlebar_height);
    }
    if (client->maximized)
      client->drawn_rev = vt_rev (client->vt);
  }

#if 0
  //float px = ctx_pointer_x (ctx);
  //float py = ctx_pointer_y (ctx);
  if (clients &&  clients->next == NULL &&  ((CtxClient*)(clients->data))->maximized)
  {
    CtxClient *client = clients->data;
    VT *vt = client->vt;
    vt_draw (vt, ctx, client->x, client->y);
    client->drawn_rev = vt_rev (vt);
    //vt_draw (vt, ctx, 0, 0);
  }
  else
#endif
  {
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    VT *vt = client->vt;
    if (vt && !client->maximized)
    {
      if (!client->shaded)
      {
        vt_draw (vt, ctx, client->x, client->y);
      }

      // register resize regions
      if (client == active && !client->shaded &&  !client->maximized)
      {
        itk_style_color (ctx, "titlebar-focused-bg");

        ctx_rectangle (ctx,
                       client->x,
                       client->y - titlebar_height * 2,
                       client->width, titlebar_height);
        ctx_listen (ctx, CTX_DRAG, client_resize_n, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_N);
        ctx_begin_path (ctx); //ctx_fill (ctx);

        ctx_rectangle (ctx,
                       client->x,
                       client->y + client->height - titlebar_height,
                       client->width, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, client_resize_s, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_S);
        ctx_begin_path (ctx); //ctx_fill (ctx);

        ctx_rectangle (ctx,
                       client->x + client->width,
                       client->y - titlebar_height,
                       titlebar_height, client->height + titlebar_height);
        ctx_listen (ctx, CTX_DRAG, client_resize_e, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_E);
        ctx_begin_path (ctx); //ctx_fill (ctx);

        ctx_rectangle (ctx,
                       client->x - titlebar_height,
                       client->y - titlebar_height,
                       titlebar_height, client->height + titlebar_height);
        ctx_listen (ctx, CTX_DRAG, client_resize_w, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_W);
        ctx_begin_path (ctx); //ctx_fill (ctx);

        ctx_rectangle (ctx,
                       client->x + client->width - titlebar_height,
                       client->y - titlebar_height * 2,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, client_resize_ne, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_NE);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x - titlebar_height,
                       client->y - titlebar_height * 2,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, client_resize_nw, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_NW);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x - titlebar_height,
                       client->y + client->height - titlebar_height,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, client_resize_sw, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_SW);
        ctx_begin_path (ctx); //ctx_fill (ctx);

        ctx_rectangle (ctx,
                       client->x + client->width - titlebar_height,
                       client->y + client->height - titlebar_height,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, client_resize_se, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_SE);
        ctx_begin_path (ctx); //ctx_fill (ctx);

      }

      draw_titlebar (ctx, client, client->x, client->y, client->width, titlebar_height);

      client->drawn_rev = vt_rev (vt);
    }
  }
  }

  return 0;
}

#if 0
static int consume_toggle (char **argv, int *i)
{
 int j;
 for (j = *i; argv[j]; j++)
 {
   argv[j-1] = argv[j];
 }
 argv[j-1]=0;
 *i-=1;
 return 1;
}
#endif

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

typedef struct KeyCap {
  char *label;
  char *label_shifted;
  char *label_fn;
  char *label_fn_shifted;
  float wfactor; // 1.0 is regular, tab is 1.5
  char *sequence;
  char *sequence_shifted;
  char *sequence_fn;
  char *sequence_fn_shifted;
  int   sticky;
  int   down;
  int   hovered;
} KeyCap;

typedef struct KeyBoard {
  KeyCap keys[9][30];
  int shifted;
  int control;
  int alt;
  int fn;
  int down;
} KeyBoard;

static void ctx_on_screen_key_event (CtxEvent *event, void *data1, void *data2)
{
  KeyCap *key = data1;
  KeyBoard *kb = data2;
  float h = ctx_height (ctx);
  float w = ctx_width (ctx);
  int rows = 0;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / 14.5; // keycell
  float y0 = h - c * rows;

  if (event->y < y0)
    return;

  key = NULL;

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
      float y = row * c + y0;
      ctx_round_rectangle (ctx, x, y,
                                c * (cap->wfactor-0.1),
                                c * 0.9,
                                c * 0.1);
      if (event->x >= x &&
          event->x < x + c * cap->wfactor-0.1 &&
          event->y >= y &&
          event->y < y + c * 0.9)
       {
         key = cap;
         if (cap->hovered != 1)
         {
           dirty ++;
         }
         cap->hovered = 1;
       }
      else
       {
         cap->hovered = 0;
       }

      x += cap->wfactor * c;
    }
  }

  event->stop_propagate = 1;
  switch (event->type)
  {
     default:
       break;
     case CTX_MOTION:
         dirty ++;
       break;
     case CTX_DRAG_MOTION:
       if (!key)
         dirty ++;
       break;
     case CTX_DRAG_PRESS:
       kb->down = 1;
       dirty ++;
       break;
     case CTX_DRAG_RELEASE:
       kb->down = 0;
        dirty ++;
       if (!key)
         return;

      if (key->sticky)
      {
        if (key->down)
          key->down = 0;
        else
          key->down = 1;

        if (!strcmp (key->label, "Shift"))
        {
          kb->shifted = key->down;
        }
        else if (!strcmp (key->label, "Ctrl"))
        {
          kb->control = key->down;
        }
        else if (!strcmp (key->label, "Alt"))
        {
          kb->alt = key->down;
        }
        else if (!strcmp (key->label, "Fn"))
        {
          kb->fn = key->down;
        }
      }
      else
      {
        if (kb->control || kb->alt)
        {
          char combined[200]="";
          if (kb->shifted)
          {
            sprintf (&combined[strlen(combined)], "shift-");
          }
          if (kb->control)
          {
            sprintf (&combined[strlen(combined)], "control-");
          }
          if (kb->alt)
          {
            sprintf (&combined[strlen(combined)], "alt-");
          }
          if (kb->fn)
            sprintf (&combined[strlen(combined)], key->sequence_fn);
          else
            sprintf (&combined[strlen(combined)], key->sequence);
          ctx_key_press (ctx, 0, combined, 0);
        }
        else
        {
          const char *sequence = key->sequence;

          if (kb->fn && kb->shifted && key->sequence_fn_shifted)
          {
            sequence = key->sequence_fn_shifted;
          }
          else if (kb->fn && key->sequence_fn)
          {
            sequence = key->sequence_fn;
          }
          else if (kb->shifted && key->sequence_shifted)
          {
            sequence = key->sequence_shifted;
          }
          ctx_key_press (ctx, 0, sequence, 0);
        }
      }
      break;
  }
}

KeyBoard en_intl = {
   {  
     { {"Esc","Esc","`","~",1.0f,"escape","escape","`","~",0},
       {"1","!","F1","F1",1.0f,"1","!","F1","F1",0},
       {"2","@","F2","F2",1.0f,"2","@","F2","F2",0},
       {"3","#","F3","F3",1.0f,"3","#","F3","F3",0},
       {"4","$","F4","F4",1.0f,"4","$","F4","F4",0},
       {"5","%","F5","F5",1.0f,"5","%","F5","F5",0},
       {"6","^","F6","F6",1.0f,"6","^","F6","F6",0},
       {"7","&","F7","F7",1.0f,"7","&","F7","F7",0},
       {"8","*","F8","F8",1.0f,"8","*","F8","F8",0},
       {"9","(","F9","F9",1.0f,"9","(","F9","F9",0},
       {"0",")","F10","F10",1.0f,"0",")","F10","F10",0},
       {"-","_","F11","F11",1.0f,"-","_","F11","F11",0},
       {"=","+","F12","F12",1.0f,"=","+","F12","F12",0},
       {"⌫","⌫",NULL,NULL,1.2f,"backspace","backspace",NULL,NULL,0},
       {NULL}},
     //⌨
     { {"Tab","Tab",NULL,NULL,1.3f,"tab","tab",NULL,NULL,0},
       {"q","Q",NULL,NULL,1.0f,"q","Q",NULL,NULL,0},
       {"w","W",NULL,NULL,1.0f,"w","W",NULL,NULL,0},
       {"e","E",NULL,NULL,1.0f,"e","E",NULL,NULL,0},
       {"r","R",NULL,NULL,1.0f,"r","R",NULL,NULL,0},
       {"t","T",NULL,NULL,1.0f,"t","T",NULL,NULL,0},
       {"y","Y",NULL,NULL,1.0f,"y","Y",NULL,NULL,0},
       {"u","U",NULL,NULL,1.0f,"u","U",NULL,NULL,0},
       {"i","I",NULL,NULL,1.0f,"i","I",NULL,NULL,0},
       {"o","O",NULL,NULL,1.0f,"o","O",NULL,NULL,0},
       {"p","P",NULL,NULL,1.0f,"p","P",NULL,NULL,0},
       {"[","{",NULL,NULL,1.0f,"[","{",NULL,NULL,0},
       {"]","}",NULL,NULL,1.0f,"]","}",NULL,NULL,0},
       {"\\","|",NULL,NULL,1.0f,"\\","|",NULL,NULL,0},
       {NULL} },
     { {"Fn","Fn",NULL,NULL,1.5f," "," ",NULL,NULL,1},
       {"a","A",NULL,NULL,1.0f,"a","A",NULL,NULL,0},
       {"s","S",NULL,NULL,1.0f,"s","S",NULL,NULL,0},
       {"d","D",NULL,NULL,1.0f,"d","D",NULL,NULL,0},
       {"f","F",NULL,NULL,1.0f,"f","F",NULL,NULL,0},
       {"g","G",NULL,NULL,1.0f,"g","G",NULL,NULL,0},
       {"h","H",NULL,NULL,1.0f,"h","H",NULL,NULL,0},
       {"j","J",NULL,NULL,1.0f,"j","J",NULL,NULL,0},
       {"k","K",NULL,NULL,1.0f,"k","K",NULL,NULL,0},
       {"l","L",NULL,NULL,1.0f,"l","L",NULL,NULL,0},
       {";",":",NULL,NULL,1.0f,";",":",NULL,NULL,0},
       {"'","\"",NULL,NULL,1.0f,"'","\"",NULL,NULL,0},
       {"⏎","⏎",NULL,NULL,1.5f,"return","return",NULL,NULL,0},
       {NULL} },
     { {"Ctrl","Ctrl",NULL,NULL,1.7f,"","",NULL,NULL,1},
       {"z","Z",NULL,NULL,1.0f,"z","Z",NULL,NULL,0},
       {"x","X",NULL,NULL,1.0f,"x","X",NULL,NULL,0},
       {"c","C",NULL,NULL,1.0f,"c","C",NULL,NULL,0},
       {"v","V",NULL,NULL,1.0f,"v","V",NULL,NULL,0},
       {"b","B",NULL,NULL,1.0f,"b","B",NULL,NULL,0},
       {"n","N",NULL,NULL,1.0f,"n","N",NULL,NULL,0},
       {"m","M",NULL,NULL,1.0f,"m","M",NULL,NULL,0},
       {",","<",NULL,NULL,1.0f,",","<",NULL,NULL,0},
       {".",">",NULL,NULL,1.0f,".",">",NULL,NULL,0},
       {"/","?",NULL,NULL,1.0f,"/","?",NULL,NULL,0},
       {"↑","↑","PgUp","PgUp",1.0f,"up","up","page-up","page-up",0},
       {NULL} },
     { {"Shift","Shift",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {"Alt","Alt",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {" "," ",NULL,NULL,8.1f,"space","space",NULL,NULL,0},

       {"←","←","Home","Home",1.0f,"left","left","home","home",0},
       {"↓","↓","PgDn","PgDn",1.0f,"down","down","page-down","page-down",0},
       {"→","→","End","End",1.0f,"right","right","end","end",0},
       {NULL} },
     { {NULL}},
   }
};



void ctx_osk_draw (Ctx *ctx)
{
  if (!on_screen_keyboard)
    return;
  static float fade = 0.0;
  KeyBoard *kb = &en_intl;

  fade = 0.2;
  if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
     fade = 0.9;

  float h = ctx_height (ctx);
  float w = ctx_width (ctx);
  float m = h;
  int rows = 0;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / 14.5; // keycell
  float y0 = h - c * rows;
  if (w < h)
    m = w;
      
  ctx_save (ctx);
  ctx_round_rectangle (ctx, 0,
                            y0,
                            w,
                            c * rows,
                            c * 0.0);
  ctx_listen (ctx, CTX_DRAG, ctx_on_screen_key_event, NULL, &en_intl);
  ctx_rgba (ctx, 0,0,0, 0.5 * fade);
  ctx_preserve (ctx);
  if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
    ctx_fill (ctx);
  //ctx_line_width (ctx, m * 0.01);
  ctx_begin_path (ctx);
#if 0
  ctx_rgba (ctx, 1,1,1, 0.5);
  ctx_stroke (ctx);
#endif

  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_line_width (ctx, m * 0.01);

  float font_size = c * 0.7;
  ctx_font_size (ctx, font_size);

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
      float y = row * c + y0;
  
      const char *label = cap->label;

      if ((kb->fn && kb->shifted && cap->label_fn_shifted))
      {
        label = cap->label_fn_shifted;
      }
      else if (kb->fn && cap->label_fn)
      {
        label = cap->label_fn;
      }
      else if (kb->shifted && cap->label_shifted)
      {
        label = cap->label_shifted;
      }

      if (ctx_utf8_strlen (label) > 1)
      {
        if (font_size != c * 0.33)
        {
          font_size = c * 0.33;
          ctx_font_size (ctx, font_size);
        }
      }
      else
      {
        if (font_size != c * 0.7)
        {
          font_size = c * 0.7;
          ctx_font_size (ctx, font_size);
        }
      }

      ctx_begin_path (ctx);
      ctx_round_rectangle (ctx, x, y,
                                c * (cap->wfactor-0.1),
                                c * 0.9,
                                c * 0.1);
      //ctx_listen (ctx, CTX_MOTION|CTX_PRESS|CTX_RELEASE|CTX_ENTER|CTX_LEAVE, ctx_on_screen_key_event, cap, &en_intl);
      
      if (cap->down || (cap->hovered && kb->down))
      {
        ctx_rgba (ctx, 1,1,1, fade);
#if 1
      ctx_fill (ctx);
#else
      ctx_preserve (ctx);
      ctx_fill (ctx);

      ctx_rgba (ctx, 0,0,0, fade);

      ctx_stroke (ctx);
#endif
      }
      if (cap->down || (cap->hovered && kb->down))
        ctx_rgba (ctx, 1,1,1, fade);
      else
        ctx_rgba (ctx, 0,0,0, fade);

      ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
      ctx_text_baseline (ctx, CTX_TEXT_BASELINE_MIDDLE);

#if 0
      ctx_move_to (ctx, x + cap->wfactor * c*0.5, y + c * 0.5);
      ctx_text_stroke (ctx, label);
#endif

      ctx_move_to (ctx, x + cap->wfactor * c*0.5, y + c * 0.5);

      if (cap->down || (cap->hovered && kb->down))
        ctx_rgba (ctx, 0,0,0, fade);
      else
        ctx_rgba (ctx, 1,0,0, fade);

      ctx_text (ctx, label);

      x += cap->wfactor * c;
    }
  }
  ctx_restore (ctx);
}

static int malloc_trim_cb (Ctx *ctx, void *data)
{
#if GNU_C
  malloc_trim (64*1024);
#endif
  return 1;
}

static void terminal_key_any (CtxEvent *event, void *userdata, void *userdata2)
{
  if (!strcmp (event->string, "resize-event"))
  {
     ensure_layout ();
     dirty++;
  }
  else
  {
    handle_event (ctx, event->string);
  }
}

extern int _ctx_enable_hash_cache;
int enable_terminal_menu = 0;

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

#include <sys/time.h>
#include <time.h>


void draw_panel (Ctx *ctx)
{
  struct tm local_time_res;
  struct timeval tv;
  gettimeofday (&tv, NULL);
  localtime_r (&tv.tv_sec, &local_time_res);

  float titlebar_height = ctx_height (ctx)/40;
  float tab_width = ctx_width (ctx) - titlebar_height * 4 - titlebar_height * 2;

  ctx_save (ctx);
  ctx_rectangle (ctx, 0, 0, ctx_width (ctx), titlebar_height);
  ctx_gray (ctx, 0.0);
  ctx_fill (ctx);
  ctx_font_size (ctx, titlebar_height * 0.9);
  ctx_move_to (ctx, ctx_width (ctx), titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_END);
  ctx_gray (ctx, 0.9);
  char buf[128];
  sprintf (buf, "%02i:%02i:%02i", local_time_res.tm_hour, local_time_res.tm_min, local_time_res.tm_sec);
  ctx_text (ctx, buf);

  int tabs = 0;
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->maximized)
    tabs ++;
  }

  if (tabs)
  tab_width /= tabs;

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
    if (client->maximized)
    {
      ctx_begin_path (ctx);
      draw_titlebar (ctx, client, x, titlebar_height, tab_width, titlebar_height);
    }
      x += tab_width;
  }
  ctx_restore (ctx);
}

void draw_mini_panel (Ctx *ctx)
{
  float titlebar_height = ctx_height (ctx)/40;

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

static void
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

int terminal_main (int argc, char **argv)
{
  execute_self = argv[0];
  float global_scale = 1.0;
  int width = -1;
  int height = -1;
  int cols = -1;
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

  if (ctx_renderer_is_braille (ctx) && font_size <= 0)
  {
    font_size = 11.0;
  }

  if (cols <= 0)
  {
    if (((double)(width))/height < (16.0/9.0))
      cols = 80;
    else
      cols = 160;
  }

  if (font_size < 0)
    font_size = floorf (2 * width / cols /2) * 3;

  const char *command = argv[1];
  if (!command)
    command = vt_find_shell_command();
  client_maximize (add_tab (command, 1));

  if (!active)
    return 1;
  vt_set_ctx (active->vt, ctx);

  ITK *itk = itk_new (ctx);

  signal (SIGCHLD,signal_child);

  ctx_add_timeout (ctx, 1000 * 200, malloc_trim_cb, NULL);

  int sleep_time = 1000;
  int fetched_bytes = 1;

  int print_shape_cache_rate = 0;
  if (getenv ("CTX_DEBUG_SHAPE_CACHE"))
    print_shape_cache_rate = 1;

  while (clients && !ctx_has_quit (ctx))
    {
      CtxList *to_remove = NULL;
      int changes = 0;
      int n_clients = ctx_list_length (clients);
      int follow_mouse = focus_follows_mouse;
      ensure_layout ();

      if (print_shape_cache_rate)
        fprintf (stderr, "\r%f ", ctx_shape_cache_rate);

      CtxClient *client = find_active (ctx_pointer_x (ctx),
                                         ctx_pointer_y (ctx));

      if (follow_mouse || ctx_pointer_is_down (ctx, 0) ||
          ctx_pointer_is_down (ctx, 1) || (active==NULL))
      {
        if (client)
        {
          if (active != client)
          {
            active = client;
            if (follow_mouse == 0 ||
                (ctx_pointer_is_down (ctx, 0) ||
                 ctx_pointer_is_down (ctx, 1)))
            {
              //if (client != clients->data)
              if (!client->maximized)
              {
                ctx_list_remove (&clients, client);
                ctx_list_append (&clients, client);
              }
            }
            changes ++;
          }
        }
      }
      if (active)
        terminal_update_title (active->title);

      itk->scale = 1.0;
      itk->font_size = font_size;

      for (CtxList *l = clients; l; l = l->next)
      {
        CtxClient *client = l->data;
        if (client->vt)
          {
            if (vt_is_done (client->vt))
              ctx_list_prepend (&to_remove, client);
          }
      }
      for (CtxList *l = to_remove; l; l = l->next)
      {
        client_remove (ctx, l->data);
        changes++;
      }
      while (to_remove)
      {
        ctx_list_remove (&to_remove, to_remove->data);
      }

      changes += vt_dirty_count ();
        static float avg_bytespeed = 0.0;

      if (changes || dirty || ctx_is_dirty (ctx))
      {
        dirty = 0;
        itk_reset (itk);
        ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
        itk_style_color (ctx, "wallpaper");
        ctx_fill (ctx);
        draw_vts (ctx);
        ctx_popups (ctx);
        if ((n_clients != 1) || (clients && ((((CtxClient*)clients->data))->maximized == 0)))
          draw_panel (ctx);
        else
          draw_mini_panel (ctx);

        if (enable_terminal_menu)
        {

          float width = ctx_width (ctx);
          float height = ctx_height (ctx);
          itk_panel_start (itk, "ctx terminal", width*0.25,height*0.2, width * 0.5, height * 0.6);
          itk_seperator (itk);

          if (itk_button (itk, "add tab"))
          {
            add_tab (vt_find_shell_command(), 1);
          }
          itk_sameline (itk);
          if (itk_button (itk, "close settings"))
          {
            enable_terminal_menu = 0;
          }
          itk_toggle (itk, "on screen keyboard", &on_screen_keyboard);
          itk_toggle (itk, "focus follows mouse", &focus_follows_mouse);
          itk_ctx_settings (itk);

          if (active)
          {
            itk_labelf (itk, " font size: %f", vt_get_font_size (active->vt));
          }

          itk_panel_end (itk);
          itk_done (itk);
          itk_key_bindings (itk);
        }
        else
        {
          ctx_set_dirty (ctx, 0);
        }
        ctx_osk_draw (ctx);
        ctx_add_key_binding (ctx, "unhandled", NULL, "", terminal_key_any, NULL);
        ctx_flush (ctx);

        //if (bytespeed > 200)
        //if (avg_bytespeed > 8.0 * 1024 * 1024)
        //  sleep_time = 250000;
        //else
        if (avg_bytespeed > 4.0 * 1024 * 1024)
          sleep_time = 100000; /* at more than 4mb second ratelimit to 10 fps */
        else if (avg_bytespeed > 2.0 * 1024 * 1024)
          sleep_time = 33333;
        else if (avg_bytespeed > 1.0 * 1024 * 1024)
          sleep_time = 15000;
        else if (avg_bytespeed > 0.5 * 1024 * 1024)
          sleep_time = 4000;
        else
          sleep_time = 10;
      }
      else
      {
        usleep (sleep_time);
        sleep_time     = 33333 * 0.01 + 0.99 * sleep_time;
      }

      //if (!ctx_is_dirty (ctx))
      {
      CtxEvent *event;
      while ((event = ctx_get_event (ctx)))
      {
      }


      fetched_bytes = 0;
      long time_start = ctx_ticks ();
      {
        /* record amount of time spent - and adjust time of reading for
         * vts?
         */
        long int fractional_sleep = sleep_time / (n_clients?n_clients:1);
        for (CtxList *l = clients; l; l = l->next)
        {
          CtxClient *client = l->data;
          fetched_bytes += vt_poll (client->vt, fractional_sleep);
        }
      }
      long time_end = ctx_ticks ();

        int timed = (time_end-time_start);
        float bytespeed = fetched_bytes / ((timed)/ (1000.0f * 1000.0f));
        avg_bytespeed = bytespeed * 0.2 + avg_bytespeed * 0.8;
      //  fprintf (stderr, "%.2fmb/s %i/%i  %i                    \r", avg_bytespeed/1024/1024, fetched_bytes, timed, sleep_time);
      }

    }

  while (clients)
    client_remove (ctx, clients->data);

  itk_free (itk);
  ctx_free (ctx);

  if (getpid () == 1)
  {
    if (system ("reboot --force")){};
  }

  return 0;
}
