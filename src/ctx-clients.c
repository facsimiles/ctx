#endif

float ctx_target_fps = 100.0; /* this might end up being the resolution of our
                                 idle callback firing
                               */

#if CTX_VT


#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

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
#include <math.h>
#include <sys/time.h>
#include <time.h>
#endif

#define VT_RECORD 0
extern Ctx *ctx;
#define flag_is_set(a, f) (((a) & (f))!=0)
#define flag_set(a, f)    ((a) |= (f));
#define flag_unset(a, f)  ((a) &= ~(f));

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

void terminal_update_title    (const char *title);
void ctx_sdl_set_fullscreen   (Ctx *ctx, int val);
int  ctx_sdl_get_fullscreen   (Ctx *ctx);
static int ctx_fetched_bytes = 1;

CtxClient *vt_get_client (VT *vt);

int vt_set_prop (VT *vt, uint32_t key_hash, const char *val)
{
#if 1
  switch (key_hash)
  {
    case CTX_title:  
     {
       CtxClient *client = vt_get_client (vt);
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
  CtxClient *client = ctx_client_by_id (ct->id);
  uint32_t val_hash = ctx_strhash (val);
  if (!client)
    return 0;

  if (key_hash == ctx_strhash("start_move"))
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
    case CTX_title:  ctx_client_set_title (ct->id, val); break;
    case CTX_x:      client->x = fval; break;
    case CTX_y:      client->y = fval; break;
    case CTX_width:  ctx_client_resize (ct->id, fval, client->height); break;
    case CTX_height: ctx_client_resize (ct->id, client->width, fval); break;
    case CTX_action:
      switch (val_hash)
      {
        case CTX_maximize:     ctx_client_maximize (client); break;
        case CTX_unmaximize:   ctx_client_unmaximize (client); break;
        case CTX_lower:        ctx_client_lower (client); break;
        case CTX_lower_bottom: ctx_client_lower_bottom (client);  break;
        case CTX_raise:        ctx_client_raise (client); break;
        case CTX_raise_top:    ctx_client_raise_top (client); break;
      }
      break;
  }
  ct->rev++;
#endif
  return 0;
}

static float _ctx_font_size = 10.0;


int ctx_client_resize (Ctx *ctx, int id, int width, int height);
void ctx_client_maximize (Ctx *ctx, int id);

CtxClient *vt_get_client (VT *vt)
{
  for (CtxList *l = ctx_clients (vt->root_ctx); l; l =l->next)
  {
    CtxClient *client = l->data;
    if (client->vt == vt)
            return client;
  }
  return NULL;
}

static void ctx_client_init (Ctx *ctx, CtxClient *client, int x, int y, int width, int height, float font_size,
                             CtxClientFlags flags, void *user_data, CtxClientFinalize finalize)
{
  static int global_id = 0;


  if (font_size <= 0.0) font_size = ctx_get_font_size (ctx);
  if (ctx_backend_type (ctx) == CTX_BACKEND_TERM)
  {
    font_size = 3;
  }
  client->id = ++global_id; // starting at 1 is nicer, then we can use 0 for none
  client->x = x;
  client->y = y;
  client->flags = flags;
  client->ctx = ctx;
  client->width = width;
  client->height = height;
  client->user_data = user_data;
  client->finalize = finalize;
  client->opacity = 1.0f;

      //fprintf (stderr, "client new:%f\n", font_size);
#if CTX_THREADS
  mtx_init (&client->mtx, mtx_plain);
#endif
}

CtxClient *ctx_client_new (Ctx *ctx,
                           const char *commandline,
                           int x, int y, int width, int height,
                           float font_size,
                           CtxClientFlags flags,
                           void *user_data,
                           CtxClientFinalize finalize)
{
  CtxClient *client = calloc (sizeof (CtxClient), 1);
  ctx_list_append (&ctx->events.clients, client);
  ctx_client_init (ctx, client, x, y, width, height, font_size, flags, user_data, finalize);
  float line_spacing = 2.0f;
  client->vt = vt_new (commandline, width, height, font_size,line_spacing, client->id, (flags & ITK_CLIENT_CAN_LAUNCH)!=0);
  client->vt->client = client;
  vt_set_ctx (client->vt, ctx);
  return client;
}

CtxClient *ctx_client_new_argv (Ctx *ctx, char **argv, int x, int y, int width, int height, float font_size, CtxClientFlags flags, void *user_data, CtxClientFinalize finalize)
{

  CtxClient *client = calloc (sizeof (CtxClient), 1);
  ctx_client_init (ctx, client, x, y, width, height, font_size, flags, user_data, finalize);
  ctx_list_append (&ctx->events.clients, client);

  float line_spacing = 2.0f;
  client->vt = vt_new_argv (argv, width, height, font_size,line_spacing, client->id, (flags & ITK_CLIENT_CAN_LAUNCH)!=0);
  client->vt->client = client;
  vt_set_ctx (client->vt, ctx);
  return client;
}

static void *launch_client_thread (void *data)
{
  CtxClient *client = data;

  client->sub_ctx = ctx_new_ui (client->width, client->height,
                                "headless");

  client->start_routine (client->sub_ctx, client->user_data);

  fprintf (stderr, "%s: cleanup\n", __FUNCTION__);
  ctx_free (client->sub_ctx);
  return NULL;
}

CtxClient *ctx_client_new_thread (Ctx *ctx, void (*start_routine)(Ctx *ctx, void *user_data),
                                  int x, int y, int width, int height, float font_size, CtxClientFlags flags, void *user_data, CtxClientFinalize finalize)
{
  CtxClient *client = calloc (sizeof (CtxClient), 1);
  ctx_client_init (ctx, client, x, y, width, height, font_size, flags, user_data, finalize);

  ctx_list_append (&ctx->events.clients, client);


  client->start_routine = start_routine;
  thrd_create (&client->tid, launch_client_thread, client);
  //float line_spacing = 2.0f;
  //client->vt = vt_new_thread (start_routine, userdata, width, height, font_size,line_spacing, client->id, (flags & ITK_CLIENT_CAN_LAUNCH)!=0);
  //vt_set_ctx (client->vt, ctx);
  if (client->vt)
    client->vt->client = client;
  return client;
}

extern float ctx_shape_cache_rate;
extern int _ctx_max_threads;

static int focus_follows_mouse = 0;

static CtxClient *find_active (Ctx *ctx, int x, int y)
{
  CtxClient *ret = NULL;
  float titlebar_height = _ctx_font_size;
  int resize_border = titlebar_height/2;

  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
     CtxClient *c = l->data;
     if ((c->flags & ITK_CLIENT_MAXIMIZED) && c == ctx->events.active_tab)
     if (x > c->x - resize_border && x < c->x+c->width + resize_border &&
         y > c->y - titlebar_height && y < c->y+c->height + resize_border)
     {
       ret = c;
     }
  }

  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
     CtxClient *c = l->data;
     if (!(c->flags &  ITK_CLIENT_MAXIMIZED))
     if (x > c->x - resize_border && x < c->x+c->width + resize_border &&
         y > c->y - titlebar_height && y < c->y+c->height + resize_border)
     {
       ret = c;
     }
  }
  return ret;
}

int id_to_no (Ctx *ctx, int id)
{
  CtxList *l;
  int no = 0;

  for (l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->id == id)
      return no;
    no++;
  }
  return -1;
}

void ctx_client_move (Ctx *ctx, int id, int x, int y);
int ctx_client_resize (Ctx *ctx, int id, int w, int h);
void ctx_client_shade_toggle (Ctx *ctx, int id);
float ctx_client_min_y_pos (Ctx *ctx);
float ctx_client_max_y_pos (Ctx *ctx);

static void ctx_clients_ensure_layout (Ctx *ctx)
{
  CtxList *clients = ctx_clients (ctx);
  int n_clients = ctx_list_length (clients);
  if (n_clients == 1)
  {
    CtxClient *client = clients->data;
    if (client->flags & ITK_CLIENT_MAXIMIZED)
    {
      ctx_client_move (ctx, client->id, 0, 0);
      ctx_client_resize (ctx, client->id, ctx_width (ctx), ctx_height(ctx));
      if (ctx->events.active_tab == NULL)
        ctx->events.active_tab = client;
    }
  }
  else
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->flags & ITK_CLIENT_MAXIMIZED)
    {
      ctx_client_move (ctx, client->id, 0, ctx_client_min_y_pos (ctx));
      ctx_client_resize (ctx, client->id, ctx_width (ctx), ctx_height(ctx) -
                      ctx_client_min_y_pos (ctx) / 2);   // /2 to counter the double titlebar of non-maximized
      if (ctx->events.active_tab == NULL)
        ctx->events.active_tab = client;
    }
  }
}

CtxClient *ctx_client_by_id (Ctx *ctx, int id)
{
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->id == id)
      return client;
  }
  return NULL;
}

void ctx_client_remove (Ctx *ctx, CtxClient *client)
{
  ctx_client_lock (client);
  if (!client->internal)
  {

    if (client->vt)
      vt_destroy (client->vt);
  }

  if (client->title)
    free (client->title);

#if VT_RECORD
  if (client->recording)
    ctx_free (client->recording);
#endif
  if (client->finalize)
     client->finalize (client, client->user_data);

  ctx_list_remove (&ctx->events.clients, client);

  if (client == ctx->events.active_tab)
  {
    ctx->events.active_tab = NULL;
  }

  if (ctx)
  if (client == ctx->events.active)
  {
    ctx->events.active = find_active (ctx, ctx_pointer_x (ctx), ctx_pointer_y (ctx));
    if (!ctx->events.active)
    {
      if (ctx->events.clients)
        ctx->events.active = ctx->events.clients->data;
    }
  }

  ctx_client_unlock (client);
  free (client);
}

void ctx_client_remove_by_id (Ctx *ctx, int id)
{
  CtxClient *client = ctx_client_by_id (ctx, id);
  if (client)
    ctx_client_remove (ctx, client);
}

int ctx_client_height (Ctx *ctx, int id)
{
  CtxClient *client = ctx_client_by_id (ctx, id);
  if (!client) return 0;
  return client->height;
}

int ctx_client_x (Ctx *ctx, int id)
{
  CtxClient *client = ctx_client_by_id (ctx, id);
  if (!client) return 0;
  return client->x;
}

int ctx_client_y (Ctx *ctx, int id)
{
  CtxClient *client = ctx_client_by_id (ctx, id);
  if (!client) return 0;
  return client->y;
}

void ctx_client_raise_top (Ctx *ctx, int id)
{
  CtxClient *client = ctx_client_by_id (ctx, id);
  if (!client) return;
  ctx_list_remove (&ctx->events.clients, client);
  ctx_list_append (&ctx->events.clients, client);
  ctx_queue_draw (ctx);
}

void ctx_client_lower_bottom (Ctx *ctx, int id)
{
  CtxClient *client = ctx_client_by_id (ctx, id);
  if (!client) return;
  ctx_list_remove (&ctx->events.clients, client);
  ctx_list_prepend (&ctx->events.clients, client);
  ctx_queue_draw (ctx);
}


void ctx_client_iconify (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   client->flags |= ITK_CLIENT_ICONIFIED;
   ctx_queue_draw (ctx);
}

int ctx_client_is_iconified (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return -1;
   return (client->flags & ITK_CLIENT_ICONIFIED) != 0;
}

void ctx_client_uniconify (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   client->flags &= ~ITK_CLIENT_ICONIFIED;
   ctx_queue_draw (ctx);
}

void ctx_client_maximize (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   if (!(client->flags &  ITK_CLIENT_MAXIMIZED))
   {
     client->flags |= ITK_CLIENT_MAXIMIZED;
     client->unmaximized_x = client->x;
     client->unmaximized_y = client->y;
     client->unmaximized_width  = client->width;
     client->unmaximized_height = client->height;
     ctx_client_move (ctx, id, 0, ctx_client_min_y_pos (client->ctx));
   }

   // enforce_layout does the size
   //client_resize (ctx, id, ctx_width (ctx), ctx_height(ctx) - ctx_client_min_y_pos (ctx));
   
   ctx->events.active = ctx->events.active_tab = client;
   ctx_queue_draw (ctx);
}

int ctx_client_is_maximized (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return -1;
   return (client->flags & ITK_CLIENT_MAXIMIZED) != 0;
}

void ctx_client_unmaximize (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   if ((client->flags & ITK_CLIENT_MAXIMIZED) == 0)
     return;
   client->flags &= ~ITK_CLIENT_MAXIMIZED;
   ctx_client_resize (ctx, id, client->unmaximized_width, client->unmaximized_height);
   ctx_client_move (ctx, id, client->unmaximized_x, client->unmaximized_y);
   ctx->events.active_tab = NULL;
   ctx_queue_draw (ctx);
}

void ctx_client_maximized_toggle (Ctx *ctx, int id)
{
  if (ctx_client_is_maximized (ctx, id))
    ctx_client_unmaximize (ctx, id);
  else
    ctx_client_maximize (ctx, id);
}


void ctx_client_shade (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   client->flags |= ITK_CLIENT_SHADED;
   ctx_queue_draw (ctx);
}

int ctx_client_is_shaded (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return -1;
   return (client->flags & ITK_CLIENT_SHADED) != 0;
}

void ctx_client_unshade (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   client->flags &= ~ITK_CLIENT_SHADED;
   ctx_queue_draw (ctx);
}

void ctx_client_toggle_maximized (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   if (ctx_client_is_maximized (ctx, id))
     ctx_client_unmaximize (ctx, id);
   else
     ctx_client_maximize (ctx, id);
}

void ctx_client_shade_toggle (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client) return;
   if (ctx_client_is_shaded (ctx, id))
    ctx_client_shade (ctx, id);
   else
    ctx_client_unshade (ctx, id);
}

void ctx_client_move (Ctx *ctx, int id, int x, int y)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (client && (client->x != x || client->y != y))
   {
     client->x = x;
     client->y = y;
     ctx_client_rev_inc (client);
     ctx_queue_draw (ctx);
   }
}

void ctx_client_set_font_size (Ctx *ctx, int id, float font_size)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (client->vt)
   {
     if (vt_get_font_size (client->vt) != font_size)
       vt_set_font_size (client->vt, font_size);
     ctx_queue_draw (ctx);
   }
}

float ctx_client_get_font_size (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (client->vt)
     return vt_get_font_size (client->vt);
   return 14.0;
}

void ctx_client_set_opacity (Ctx *ctx, int id, float opacity)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client)
     return;
   if (opacity > 0.98) opacity = 1.0f;
   client->opacity = opacity;
   ctx_queue_draw (ctx);
}

float ctx_client_get_opacity (Ctx *ctx, int id)
{
   CtxClient *client = ctx_client_by_id (ctx, id);
   if (!client)
     return 1.0f;
   return client->opacity;
}

int ctx_client_resize (Ctx *ctx, int id, int width, int height)
{
   CtxClient *client = ctx_client_by_id (ctx, id);

   if (client && ((height != client->height) || (width != client->width) ))
   {
     client->width  = width;
     client->height = height;
     if (client->vt)
       vt_set_px_size (client->vt, width, height);
     ctx_queue_draw (ctx);
     return 1;
   }
   return 0;
}

static void ctx_client_titlebar_drag (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  if (event->type == CTX_DRAG_RELEASE)
  {
    static int prev_drag_end_time = 0;
    if (event->time - prev_drag_end_time < 500)
    {
      //client_shade_toggle (ctx, client->id);
      ctx_client_maximized_toggle (event->ctx, client->id);
    }
    prev_drag_end_time = event->time;
  }

  float new_x = client->x +  event->delta_x;
  float new_y = client->y +  event->delta_y;

  float snap_threshold = 8;

  if (ctx_backend_type (event->ctx) == CTX_BACKEND_TERM)
     snap_threshold = 1;

  if (new_y < ctx_client_min_y_pos (event->ctx)) new_y = ctx_client_min_y_pos (event->ctx);
  if (new_y > ctx_client_max_y_pos (event->ctx)) new_y = ctx_client_max_y_pos (event->ctx);

  if (fabs (new_x - 0) < snap_threshold) new_x = 0.0;
  if (fabs (ctx_width (event->ctx) - (new_x + client->width)) < snap_threshold)
       new_x = ctx_width (event->ctx) - client->width;

  ctx_client_move (event->ctx, client->id, new_x, new_y);

  event->stop_propagate = 1;
}

static float min_win_dim = 32;

static void ctx_client_resize_se (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  int new_w = client->width + event->delta_x;
  int new_h = client->height + event->delta_y;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  ctx_client_resize (event->ctx, client->id, new_w, new_h);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void ctx_client_resize_e (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  int new_w = client->width + event->delta_x;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  ctx_client_resize (event->ctx, client->id, new_w, client->height);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void ctx_client_resize_s (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  int new_h = client->height + event->delta_y;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  ctx_client_resize (event->ctx, client->id, client->width, new_h);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void ctx_client_resize_n (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  float new_y = client->y +  event->delta_y;
  int new_h = client->height - event->delta_y;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  ctx_client_resize (event->ctx, client->id, client->width, new_h);
  ctx_client_move (event->ctx, client->id, client->x, new_y);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void ctx_client_resize_ne (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  float new_y = client->y +  event->delta_y;
  int new_h = client->height - event->delta_y;
  int new_w = client->width + event->delta_x;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  ctx_client_resize (event->ctx, client->id, new_w, new_h);
  ctx_client_move (event->ctx, client->id, client->x, new_y);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void ctx_client_resize_sw (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  float new_x = client->x +  event->delta_x;
  int new_w = client->width - event->delta_x;
  int new_h = client->height + event->delta_y;

  if (new_h <= min_win_dim) new_h = min_win_dim;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  ctx_client_resize (event->ctx, client->id, new_w, new_h);
  ctx_client_move (event->ctx, client->id, new_x, client->y);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void ctx_client_resize_nw (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  float new_x = client->x +  event->delta_x;
  float new_y = client->y +  event->delta_y;
  int new_w = client->width - event->delta_x;
  int new_h = client->height - event->delta_y;
  if (new_h <= min_win_dim) new_h = min_win_dim;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  ctx_client_resize (event->ctx, client->id, new_w, new_h);
  ctx_client_move (event->ctx, client->id, new_x, new_y);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void ctx_client_resize_w (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;

  float new_x = client->x +  event->delta_x;
  int new_w = client->width - event->delta_x;
  if (new_w <= min_win_dim) new_w = min_win_dim;
  ctx_client_resize (event->ctx, client->id, new_w, client->height);
  ctx_client_move (event->ctx, client->id, new_x, client->y);
  ctx_client_rev_inc (client);
  ctx_queue_draw (event->ctx);

  event->stop_propagate = 1;
}

static void ctx_client_close (CtxEvent *event, void *data, void *data2)
{
  //Ctx *ctx = event->ctx;
  CtxClient *client = data;

 // client->do_quit = 1;
  
  ctx_client_remove (event->ctx, client);

  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

/********************/
void vt_use_images (VT *vt, Ctx *ctx);
float _ctx_green = 0.5;

static void ctx_client_draw (Ctx *ctx, CtxClient *client, float x, float y)
{
    if (client->tid)
    {
#if 0
      ctx_save (ctx);
      ctx_translate (ctx, x, y);
      int width = client->width;
      int height = client->height;
      itk_panel_start (itk, "", 0, 0, width, height);
      //itk_seperator (itk);
#if 0
      if (itk_button (itk, "add tab"))
      {
        add_tab (vt_find_shell_command(), 1);
      }
#endif
      //itk_sameline (itk);
      //itk_toggle (itk, "on screen keyboard", &on_screen_keyboard);
      //itk_toggle (itk, "focus follows mouse", &focus_follows_mouse);
      itk_slider_float (itk, "CTX_GREEN", &_ctx_green, 0.0, 1.0, 0.5);
      itk_ctx_settings (itk);
      itk_itk_settings (itk);
      itk_panel_end (itk);
      itk_done (itk);
      //itk_key_bindings (itk);
      ctx_restore (ctx);
#endif
    }
    else
    {
       ctx_client_lock (client);

       int found = 0;
       for (CtxList *l2 = ctx_clients (ctx); l2; l2 = l2->next)
         if (l2->data == client) found = 1;
       if (found)
       {

      int rev = ctx_client_rev (client);
#if VT_RECORD
      if (client->drawn_rev != rev)
      {
        if (!client->recording)
          client->recording = ctx_new ();
        else
          ctx_reset (client->recording);
        vt_draw (client->vt, client->recording, 0.0, 0.0);
      }

      if (client->recording)
      {
        ctx_save (ctx);
        ctx_translate (ctx, x, y);
        if (client->opacity != 1.0f)
        {
          ctx_global_alpha (ctx, client->opacity);
        }
        ctx_render_ctx (client->recording, ctx);
        vt_register_events (client->vt, ctx, 0.0, 0.0);
        ctx_restore (ctx);
      }
#else
      if (client->opacity != 1.0)
      {
        ctx_save (ctx);
        ctx_global_alpha (ctx, client->opacity);
      }
      vt_draw (client->vt, ctx, x, y);
      if (client->opacity != 1.0)
      {
        ctx_restore (ctx);
      }
      ctx_client_register_events (client, ctx, x, y);
#endif
      client->drawn_rev = rev;
      ctx_client_unlock (client);
      }
    }
}

static void ctx_client_use_images (Ctx *ctx, CtxClient *client)
{
  if (!client->internal)
  {
      uint32_t rev = ctx_client_rev (client);
#if VT_RECORD
      if (client->drawn_rev != rev)
      {
        if (!client->recording)
          client->recording = ctx_new ();
        else
          ctx_reset (client->recording);
        vt_draw (client->vt, client->recording, 0.0, 0.0);
      }

      if (client->recording)
      {
        ctx_save (ctx);
        if (client->opacity != 1.0f)
        {
          ctx_global_alpha (ctx, client->opacity);
        }
        ctx_render_ctx_textures (client->recording, ctx);
        ctx_restore (ctx);
      }
#else
    if (client->vt)vt_use_images (client->vt, ctx);
#endif
    client->drawn_rev = rev;
  }
}

void ctx_client_lock (CtxClient *client)
{
#if CTX_THREADS
    mtx_lock (&client->mtx);
#endif
}

void ctx_client_unlock (CtxClient *client)
{
#if CTX_THREADS
    mtx_unlock (&client->mtx);
#endif
}


CtxEvent *ctx_event_copy (CtxEvent *event)
{
  CtxEvent *copy = calloc (1, sizeof (CtxEvent));
  *copy = *event;
  if (copy->string) {
    copy->string = strdup (copy->string);
    copy->owns_string = 1;
  }
  return copy;
}

#if 0
void ctx_client_handle_event (Ctx *ctx, CtxEvent *ctx_event, const char *event)
{
  if (!ctx->events.active)
    return;
  if (ctx->events.active->internal)
    return;
  VT *vt = ctx->events.active->vt;
  CtxClient *client = vt_get_client (vt);

  ctx_client_lock (client);

  if (!strcmp (event, "F11"))
  {
#if CTX_SDL
    if (ctx_backend_is_sdl (ctx))
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
           ((ctx_backend_is_fb (ctx) || ctx_backend_is_term (ctx) || ctx_backend_is_kms (ctx))
           &&   !strcmp (event, "control-t") ))
  {
    //XXX add_tab (vt_find_shell_command(), 1);
  }
  else if (!strcmp (event, "shift-control-n") )
    {
      pid_t pid;
      if ( (pid=fork() ) ==0)
        {
          unsetenv ("CTX_VERSION");
         // execlp (execute_self, execute_self, NULL);
          exit (0);
        }
    }

#if 0
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
#endif
  else if (!strcmp (event, "shift-control-q") )
    {
      ctx_quit (ctx);
    }
  else if (!strcmp (event, "shift-control-w") )
    {
      ctx->events.active->do_quit = 1;
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
  ctx_client_unlock (client);
}
#endif

static int ctx_clients_dirty_count (Ctx *ctx)
{
  int changes = 0;
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if ((client->drawn_rev != ctx_client_rev (client) ) ||
        vt_has_blink (client->vt))
      changes++;
  }
  return changes;
}

static void ctx_client_titlebar_drag_maximized (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  Ctx *ctx = event->ctx;
  ctx->events.active = ctx->events.active_tab = client;
  if (event->type == CTX_DRAG_RELEASE)
  {
    static int prev_drag_end_time = 0;
    if (event->time - prev_drag_end_time < 500)
    {
      //client_shade_toggle (ctx, client->id);
      ctx_client_unmaximize (ctx, client->id);
      ctx_client_raise_top (ctx, client->id);
      ctx->events.active_tab = NULL;
    }
    prev_drag_end_time = event->time;
  }
  ctx_queue_draw (event->ctx);
  ctx_client_rev_inc (client);
  event->stop_propagate = 1;
}

float ctx_client_min_y_pos (Ctx *ctx)
{
  return _ctx_font_size * 2; // a titlebar and a panel
}

float ctx_client_max_y_pos (Ctx *ctx)
{
  return ctx_height (ctx);
}

void ctx_client_titlebar_draw (Ctx *ctx, CtxClient *client,
                               float x, float y, float width, float titlebar_height)
{
#if 0
  ctx_move_to (ctx, x, y + height * 0.8);
  if (client == ctx->events.active)
    ctx_rgba (ctx, 1, 1,0.4, 1.0);
  else
    ctx_rgba (ctx, 1, 1,1, 0.8);
  ctx_text (ctx, client->title);
#else
  ctx_rectangle (ctx, x, y - titlebar_height,
                 width, titlebar_height);
  if (client == ctx->events.active)
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
  //ctx_font_size (ctx, itk->font_size);//titlebar_height);// * 0.85);

  if (client == ctx->events.active &&
      (flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED) || y != titlebar_height))
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
  if (client == ctx->events.active)
    itk_style_color (ctx, "titlebar-focused-close");
  else
    itk_style_color (ctx, "titlebar-close");
  ctx_text (ctx, "X");

  ctx_move_to (ctx, x +  width/2, y - titlebar_height * 0.22);
  if (client == ctx->events.active)
    itk_style_color (ctx, "titlebar-focused-fg");
  else
    itk_style_color (ctx, "titlebar-fg");

  ctx_save (ctx);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  if (client->title)
    ctx_text (ctx, client->title);
  else
    ctx_text (ctx, "untitled");
  ctx_restore (ctx);
#endif
}

#if 0
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
#endif

int ctx_clients_draw (Ctx *ctx, int layer2)
{
  CtxList *clients = ctx_clients (ctx);
  _ctx_font_size = ctx_get_font_size (ctx);
  float titlebar_height = _ctx_font_size;
  int n_clients         = ctx_list_length (clients);

  if (ctx->events.active && flag_is_set(ctx->events.active->flags, ITK_CLIENT_MAXIMIZED) && n_clients == 1)
  {
    ctx_client_draw (ctx, ctx->events.active, 0, 0);
    return 0;
  }
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED))
    {
      if (client == ctx->events.active_tab)
      {
        ctx_client_draw (ctx, client, 0, titlebar_height);
      }
      else
      {
        ctx_client_use_images (ctx, client);
      }
    }
  }

  {
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    VT *vt = client->vt;

    if (layer2)
    {
      if (!flag_is_set (client->flags, ITK_CLIENT_LAYER2))
        continue;
    }
    else
    {
      if (flag_is_set (client->flags, ITK_CLIENT_LAYER2))
        continue;
    }

    if (vt && !flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED))
    {
      if (flag_is_set(client->flags, ITK_CLIENT_SHADED))
      {
        ctx_client_use_images (ctx, client);
      }
      else
      {
        ctx_client_draw (ctx, client, client->x, client->y);

      // resize regions
      if (client == ctx->events.active &&
         !flag_is_set(client->flags, ITK_CLIENT_SHADED) &&
         !flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED) &&
         flag_is_set(client->flags, ITK_CLIENT_UI_RESIZABLE))
      {
        itk_style_color (ctx, "titlebar-focused-bg");

        ctx_rectangle (ctx,
                       client->x,
                       client->y - titlebar_height * 2,
                       client->width, titlebar_height);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_n, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_N);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x,
                       client->y + client->height - titlebar_height,
                       client->width, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_s, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_S);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x + client->width,
                       client->y - titlebar_height,
                       titlebar_height, client->height + titlebar_height);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_e, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_E);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x - titlebar_height,
                       client->y - titlebar_height,
                       titlebar_height, client->height + titlebar_height);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_w, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_W);
        ctx_begin_path (ctx); 

        ctx_rectangle (ctx,
                       client->x + client->width - titlebar_height,
                       client->y - titlebar_height * 2,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_ne, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_NE);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x - titlebar_height,
                       client->y - titlebar_height * 2,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_nw, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_NW);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x - titlebar_height,
                       client->y + client->height - titlebar_height,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_sw, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_SW);
        ctx_begin_path (ctx);

        ctx_rectangle (ctx,
                       client->x + client->width - titlebar_height,
                       client->y + client->height - titlebar_height,
                       titlebar_height * 2, titlebar_height * 2);
        ctx_listen (ctx, CTX_DRAG, ctx_client_resize_se, client, NULL);
        ctx_listen_set_cursor (ctx, CTX_CURSOR_RESIZE_SE);
        ctx_begin_path (ctx);

      }

      }

      if (client->flags & ITK_CLIENT_TITLEBAR)
        ctx_client_titlebar_draw (ctx, client, client->x, client->y, client->width, titlebar_height);
    }
  }
  }
  return 0;
}

extern int _ctx_enable_hash_cache;

void vt_audio_task (VT *vt, int click);

int ctx_input_pending (Ctx *ctx, int timeout);
int ctx_clients_active (Ctx *ctx)
{
  if (ctx->events.active) return ctx->events.active->id;
  return -1;
}

int ctx_clients_need_redraw (Ctx *ctx)
{
  int changes = 0;
  int follow_mouse = focus_follows_mouse;
      CtxList *to_remove = NULL;
  ctx_clients_ensure_layout (ctx);

//  if (print_shape_cache_rate)
//    fprintf (stderr, "\r%f ", ctx_shape_cache_rate);

   CtxClient *client = find_active (ctx, ctx_pointer_x (ctx),
                                    ctx_pointer_y (ctx));

   if (follow_mouse || ctx_pointer_is_down (ctx, 0) ||
       ctx_pointer_is_down (ctx, 1) || (ctx->events.active==NULL))
   {
        if (client)
        {
          if (ctx->events.active != client)
          {
            ctx->events.active = client;
            if (follow_mouse == 0 ||
                (ctx_pointer_is_down (ctx, 0) ||
                 ctx_pointer_is_down (ctx, 1)))
            {
              //if (client != clients->data)
       #if 1
              if ((client->flags & ITK_CLIENT_MAXIMIZED)==0)
              {
                ctx_list_remove (&ctx->events.clients, client);
                ctx_list_append (&ctx->events.clients, client);
              }
#endif
            }
            changes ++;
          }
        }
   }

   for (CtxList *l = ctx_clients (ctx); l; l = l->next)
   {
     CtxClient *client = l->data;
     if (client->vt)
       {
         if (vt_is_done (client->vt))
         {
           if ((client->flags & ITK_CLIENT_KEEP_ALIVE))
           {
             client->flags |= ITK_CLIENT_FINISHED;
           }
           else
           {
             ctx_list_prepend (&to_remove, client);
           }
         }
       }
   }
   while (to_remove)
   {
     changes++;
     ctx_client_remove (ctx, to_remove->data);
     ctx_list_remove (&to_remove, to_remove->data);
   }

   changes += ctx_clients_dirty_count (ctx);
   return changes != 0;
}
float ctx_avg_bytespeed = 0.0;

int ctx_clients_tab_to_id (Ctx *ctx, int tab_no)
{
  CtxList *clients = ctx_clients (ctx);
  int no = 0;
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (flag_is_set(client->flags, ITK_CLIENT_MAXIMIZED))
    {
      if (no == tab_no)
        return client->id;
      no++;
    }
  }
  return -1;
}

CtxList *ctx_clients (Ctx *ctx)
{
  return ctx?ctx->events.clients:NULL;
}

#endif /* CTX_VT */

int ctx_clients_handle_events (Ctx *ctx)
{
  //int n_clients = ctx_list_length (clients);
  int pending_data = 0;
#if CTX_VT
  long time_start = ctx_ticks ();
#endif
  int sleep_time = 1000000/ctx_target_fps;
  pending_data += ctx_input_pending (ctx, sleep_time);

#if CTX_VT
  CtxList *clients = ctx_clients (ctx);
  if (!clients)
    return pending_data != 0;
  ctx_fetched_bytes = 0;
  if (pending_data)
  {
    if (!pending_data)pending_data = 1;
    /* record amount of time spent - and adjust time of reading for
     * vts?
     */
    //long int fractional_sleep = sleep_time / pending_data;
    long int fractional_sleep = sleep_time * 0.75;
    for (CtxList *l = clients; l; l = l->next)
    {
      CtxClient *client = l->data;
      ctx_client_lock (client);
      int found = 0;
      for (CtxList *l2 = clients; l2; l2 = l2->next)
        if (l2->data == client) found = 1;
      if (!found)
        goto done;
      
      ctx_fetched_bytes += vt_poll (client->vt, fractional_sleep);
      //ctx_fetched_bytes += vt_poll (client->vt, sleep_time); //fractional_sleep);
      ctx_client_unlock (client);
    }
done:
    if(0){
    }
  }
  else
  {
    for (CtxList *l = clients; l; l = l->next)
    {
      CtxClient *client = l->data;
      vt_audio_task (client->vt, 0);
    }
  }

  //int got_events = 0;

  //while (ctx_get_event (ctx)) { }
#if 0
  if (changes /*|| pending_data */)
  {
    ctx_target_fps *= 1.6;
    if (ctx_target_fps > 60) ctx_target_fps = 60;
  }
  else
  {
    ctx_target_fps = ctx_target_fps * 0.95 + 30.0 * 0.05;

    // 20fps is the lowest where sun 8bit ulaw 8khz works reliably
  }

  if (ctx_avg_bytespeed > 1024 * 1024) ctx_target_fps = 10.0;

  if (_ctx_green < 0.4)
    ctx_target_fps = 120.0;
  else if (_ctx_green > 0.6)
    ctx_target_fps = 25.0;

  //ctx_target_fps = 30.0;
#else
  ctx_target_fps = 100.0; // need to be higher than vsync rate to hit vsync
#endif

  long time_end = ctx_ticks ();

  int timed = (time_end-time_start);
  float bytespeed = ctx_fetched_bytes / ((timed)/ (1000.0f * 1000.0f));

  ctx_avg_bytespeed = bytespeed * 0.2 + ctx_avg_bytespeed * 0.8;
#if 0
  fprintf (stderr, "%.2fmb/s %i/%i  %.2f                    \r", ctx_avg_bytespeed/1024/1024, ctx_fetched_bytes, timed, ctx_target_fps);
#endif




#endif
  return 0;
}

void ctx_client_rev_inc (CtxClient *client)
{
  if (client) client->rev++;
}
long ctx_client_rev (CtxClient *client)
{
  return client?client->rev:0;
}
