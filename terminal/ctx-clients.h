#ifndef __CTX_CLIENTS_H
#define __CTX_CLIENTS_H

typedef enum CtxClientFlags {
  ITK_CLIENT_UI_RESIZABLE = 1<<0,
  ITK_CLIENT_CAN_LAUNCH   = 1<<1,
  ITK_CLIENT_MAXIMIZED    = 1<<2,
  ITK_CLIENT_ICONIFIED    = 1<<3,
  ITK_CLIENT_SHADED       = 1<<4,
  ITK_CLIENT_TITLEBAR     = 1<<5
} CtxClientFlags;

struct _CtxClient {
  VT    *vt;
  char  *title;
  int    x;
  int    y;
  int    width;
  int    height;
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
#if VT_RECORD
  Ctx   *recording;
#endif
};

typedef struct _CtxClient CtxClient;


extern CtxList *clients;
extern CtxClient *active;
extern CtxClient *active_tab;

static CtxClient *ctx_client_by_id (int id);

int ctx_client_resize (int id, int width, int height);
void ctx_client_maximize (int id);


CtxClient *vt_find_client (VT *vt);
CtxClient *ctx_client_new (const char *commandline,
                       int x, int y, int width, int height,
                       CtxClientFlags flags);
CtxClient *ctx_client_new_argv (const char **argv, int x, int y, int width, int height, CtxClientFlags flags);
int ctx_clients_need_redraw (Ctx *ctx);

extern float ctx_shape_cache_rate;
extern int _ctx_max_threads;



void ctx_client_move (int id, int x, int y);
int ctx_client_resize (int id, int w, int h);
void ctx_client_shade_toggle (int id);
float ctx_client_min_y_pos (Ctx *ctx);
float ctx_client_max_y_pos (Ctx *ctx);



CtxClient *client_by_id (int id);

void ctx_client_remove (Ctx *ctx, CtxClient *client);

int ctx_client_height (int id);

int ctx_client_x (int id);
int ctx_client_y (int id);
void ctx_client_raise_top (int id);
void ctx_client_lower_bottom (int id);
void ctx_client_iconify (int id);
int ctx_client_is_iconified (int id);
void ctx_client_uniconify (int id);
void ctx_client_maximize (int id);
int ctx_client_is_maximized (int id);
void ctx_client_unmaximize (int id);
void ctx_client_maximized_toggle (int id);
void ctx_client_shade (int id);
int ctx_client_is_shaded (int id);
void ctx_client_unshade (int id);
void ctx_client_toggle_maximized (int id);
void ctx_client_shade_toggle (int id);
void ctx_client_move (int id, int x, int y);
int ctx_client_resize (int id, int width, int height);

#endif
