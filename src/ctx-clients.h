#ifndef __CTX_CLIENTS_H
#define __CTX_CLIENTS_H



struct _CtxClient {
  VT    *vt;        // or NULL when thread

  long       rev;

  CtxList *events;  // we could use this queue also for vt

  Ctx     *ctx;
  char    *title;
  int      x;
  int      y;
  int      width;
  int      height;
  float    opacity;
  CtxClientFlags flags;
#if 0
  int      shaded;
  int      iconified;
  int      maximized;
  int      resizable;
#endif
  int      unmaximized_x;
  int      unmaximized_y;
  int      unmaximized_width;
  int      unmaximized_height;
  int      do_quit;
  long     drawn_rev;
  int      id;
  int      internal; // render a settings window rather than a vt

#if CTX_THREADS
  thrd_t tid;     // and only split code path in processing?
                    // -- why?
#endif
  void (*start_routine)(Ctx *ctx, void *user_data);
  void    *user_data;
  CtxClientFinalize finalize;
  Ctx     *sub_ctx;
  CtxList *ctx_events;


  /* we want to keep variation at the end */
#if CTX_THREADS
  mtx_t    mtx;
#endif
#if CTX_VT_DRAWLIST
  Ctx     *recording;
#endif
};


void ctx_client_lock (CtxClient *client);
void ctx_client_unlock (CtxClient *client);

#endif
