#ifndef __CTX_CLIENTS_H
#define __CTX_CLIENTS_H

typedef enum CtxClientFlags {
  ITK_CLIENT_UI_RESIZABLE = 1<<0,
  ITK_CLIENT_CAN_LAUNCH   = 1<<1,
  ITK_CLIENT_MAXIMIZED    = 1<<2,
  ITK_CLIENT_ICONIFIED    = 1<<3,
  ITK_CLIENT_SHADED       = 1<<4,
  ITK_CLIENT_TITLEBAR     = 1<<5,
  ITK_CLIENT_LAYER2       = 1<<6,  // used for having a second set
                                   // to draw - useful for splitting
                                   // scrolled and HUD items
                                   // with HUD being LAYER2
                                  
  ITK_CLIENT_KEEP_ALIVE   = 1<<7,  // do not automatically
  ITK_CLIENT_FINISHED     = 1<<8,  // do not automatically
                                   // remove after process quits
  ITK_CLIENT_PRELOAD      = 1<<9
} CtxClientFlags;

typedef struct _CtxClient CtxClient;
typedef void (*CtxClientFinalize)(CtxClient *client, void *user_data);

struct _CtxClient {
  VT    *vt;
  Ctx   *ctx;
  char  *title;
  int    x;
  int    y;
  int    width;
  int    height;
  float  opacity;
  CtxClientFlags flags;
#if 0
  int    shaded;
  int    iconified;
  int    maximized;
  int    resizable;
#endif
  int    unmaximized_x;
  int    unmaximized_y;
  int    unmaximized_width;
  int    unmaximized_height;
  int    do_quit;
  long   drawn_rev;
  int    id;
  int    internal; // render a settings window rather than a vt
  void  *user_data;
  CtxClientFinalize finalize;
#if CTX_THREADS
  mtx_t  mtx;
#endif
#if VT_RECORD
  Ctx   *recording;
#endif
};

int   ctx_client_resize        (Ctx *ctx, int id, int width, int height);
void  ctx_client_set_font_size (Ctx *ctx, int id, float font_size);
float ctx_client_get_font_size (Ctx *ctx, int id);
void  ctx_client_maximize      (Ctx *ctx, int id);


CtxClient *vt_get_client (VT *vt);
CtxClient *ctx_client_new (Ctx *ctx,
                           const char *commandline,
                           int x, int y, int width, int height,
                           float font_size,
                           CtxClientFlags flags,
                           void *user_data,
                           CtxClientFinalize client_finalize);

CtxClient *ctx_client_new_argv (Ctx *ctx, char **argv, int x, int y, int width, int height, float font_size, CtxClientFlags flags, void *user_data,
                CtxClientFinalize client_finalize);
int ctx_clients_need_redraw (Ctx *ctx);

extern float ctx_shape_cache_rate;
extern int _ctx_max_threads;

void  ctx_client_move         (Ctx *ctx, int id, int x, int y);
void  ctx_client_shade_toggle (Ctx *ctx, int id);
float ctx_client_min_y_pos    (Ctx *ctx);
float ctx_client_max_y_pos    (Ctx *ctx);

int   ctx_clients_active      (Ctx *ctx);

CtxClient *ctx_client_by_id (Ctx *ctx, int id);

int ctx_clients_draw (Ctx *ctx, int layer2);

void ctx_client_remove (Ctx *ctx, CtxClient *client);

int ctx_client_height (Ctx *ctx, int id);

int  ctx_client_x (Ctx *ctx, int id);
int  ctx_client_y (Ctx *ctx, int id);
void ctx_client_raise_top (Ctx *ctx, int id);
void ctx_client_lower_bottom (Ctx *ctx, int id);
void ctx_client_iconify (Ctx *ctx, int id);
int  ctx_client_is_iconified (Ctx *ctx, int id);
void ctx_client_uniconify (Ctx *ctx, int id);
void ctx_client_maximize (Ctx *ctx, int id);
int  ctx_client_is_maximized (Ctx *ctx, int id);
void ctx_client_unmaximize (Ctx *ctx, int id);
void ctx_client_maximized_toggle (Ctx *ctx, int id);
void ctx_client_shade (Ctx *ctx, int id);
int  ctx_client_is_shaded (Ctx *ctx, int id);
void ctx_client_unshade (Ctx *ctx, int id);
void ctx_client_toggle_maximized (Ctx *ctx, int id);
void ctx_client_shade_toggle (Ctx *ctx, int id);
void ctx_client_move (Ctx *ctx, int id, int x, int y);
int  ctx_client_resize (Ctx *ctx, int id, int width, int height);
void ctx_client_set_opacity (Ctx *ctx, int id, float opacity);
float ctx_client_get_opacity (Ctx *ctx, int id);

#endif
