#include "ctx-split.h"

#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif



#if CTX_KMS
#ifdef __linux__
  #include <linux/kd.h>
#endif
  //#include <linux/fb.h>
  //#include <linux/vt.h>
  #include <sys/mman.h>
  //#include <threads.h>
  #include <libdrm/drm.h>
  #include <libdrm/drm_mode.h>


typedef struct _CtxKMS CtxKMS;
struct _CtxKMS
{
   CtxTiled tiled;
   int           key_balance;
   int           key_repeat;
   int           lctrl;
   int           lalt;
   int           rctrl;

   int          fb_fd;
   char        *fb_path;
   int          fb_bits;
   int          fb_bpp;
   int          fb_mapped_size;
   //struct       fb_var_screeninfo vinfo;
   //struct       fb_fix_screeninfo finfo;
   int          vt;
   int          tty;
   int          is_kms;
   cnd_t        cond;
   mtx_t        mtx;
   struct drm_mode_crtc crtc;
};


#if UINTPTR_MAX == 0xffFFffFF
  #define fbdrmuint_t uint32_t
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
  #define fbdrmuint_t uint64_t
#endif

void *ctx_fbkms_new (CtxKMS *fb, int *width, int *height)
{
   int got_master = 0;
   fb->fb_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
   if (!fb->fb_fd)
     return NULL;
   static fbdrmuint_t res_conn_buf[20]={0}; // this is static since its contents
                                         // are used by the flip callback
   fbdrmuint_t res_fb_buf[20]={0};
   fbdrmuint_t res_crtc_buf[20]={0};
   fbdrmuint_t res_enc_buf[20]={0};
   struct   drm_mode_card_res res={0};

   if (ioctl(fb->fb_fd, DRM_IOCTL_SET_MASTER, 0))
     goto cleanup;
   got_master = 1;

   if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
     goto cleanup;
   res.fb_id_ptr=(fbdrmuint_t)res_fb_buf;
   res.crtc_id_ptr=(fbdrmuint_t)res_crtc_buf;
   res.connector_id_ptr=(fbdrmuint_t)res_conn_buf;
   res.encoder_id_ptr=(fbdrmuint_t)res_enc_buf;
   if(ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
      goto cleanup;


   unsigned int i;
   for (i=0;i<res.count_connectors;i++)
   {
     struct drm_mode_modeinfo conn_mode_buf[20]={0};
     fbdrmuint_t conn_prop_buf[20]={0},
                     conn_propval_buf[20]={0},
                     conn_enc_buf[20]={0};

     struct drm_mode_get_connector conn={0};

     conn.connector_id=res_conn_buf[i];

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     conn.modes_ptr=(fbdrmuint_t)conn_mode_buf;
     conn.props_ptr=(fbdrmuint_t)conn_prop_buf;
     conn.prop_values_ptr=(fbdrmuint_t)conn_propval_buf;
     conn.encoders_ptr=(fbdrmuint_t)conn_enc_buf;

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     //Check if the connector is OK to use (connected to something)
     if (conn.count_encoders<1 || conn.count_modes<1 || !conn.encoder_id || !conn.connection)
       continue;

//------------------------------------------------------------------------------
//Creating a dumb buffer
//------------------------------------------------------------------------------
     struct drm_mode_create_dumb create_dumb={0};
     struct drm_mode_map_dumb    map_dumb={0};
     struct drm_mode_fb_cmd      cmd_dumb={0};
     create_dumb.width  = conn_mode_buf[0].hdisplay;
     create_dumb.height = conn_mode_buf[0].vdisplay;
     create_dumb.bpp   = 32;
     create_dumb.flags = 0;
     create_dumb.pitch = 0;
     create_dumb.size  = 0;
     create_dumb.handle = 0;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) ||
         !create_dumb.handle)
       goto cleanup;

     cmd_dumb.width =create_dumb.width;
     cmd_dumb.height=create_dumb.height;
     cmd_dumb.bpp   =create_dumb.bpp;
     cmd_dumb.pitch =create_dumb.pitch;
     cmd_dumb.depth =24;
     cmd_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb))
       goto cleanup;

     map_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb))
       goto cleanup;

     void *base = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fb->fb_fd, map_dumb.offset);
     if (!base)
     {
       goto cleanup;
     }
     *width  = create_dumb.width;
     *height = create_dumb.height;

     struct drm_mode_get_encoder enc={0};
     enc.encoder_id=conn.encoder_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETENCODER, &enc))
        goto cleanup;

     fb->crtc.crtc_id=enc.crtc_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCRTC, &fb->crtc))
        goto cleanup;

     fb->crtc.fb_id=cmd_dumb.fb_id;
     fb->crtc.set_connectors_ptr=(fbdrmuint_t)&res_conn_buf[i];
     fb->crtc.count_connectors=1;
     fb->crtc.mode=conn_mode_buf[0];
     fb->crtc.mode_valid=1;
     return base;
   }
cleanup:
   if (got_master)
     ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
   fb->fb_fd = 0;
   return NULL;
}

void ctx_fbkms_flip (CtxKMS *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_MODE_SETCRTC, &fb->crtc);
}

void ctx_fbkms_close (CtxKMS *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
  close (fb->fb_fd);
  fb->fb_fd = 0;
}

static void ctx_kms_flip (CtxKMS *fb)
{
  if (fb->is_kms)
    ctx_fbkms_flip (fb);
#if 0
  else
    ioctl (fb->fb_fd, FBIOPAN_DISPLAY, &fb->vinfo);
#endif
}

inline static uint32_t
ctx_swap_red_green2 (uint32_t orig)
{
  uint32_t  green_alpha = (orig & 0xff00ff00);
  uint32_t  red_blue    = (orig & 0x00ff00ff);
  uint32_t  red         = red_blue << 16;
  uint32_t  blue        = red_blue >> 16;
  return green_alpha | red | blue;
}

static void ctx_kms_show_frame (CtxKMS *fb, int block)
{
  CtxTiled *tiled = (void*)fb;
  if (tiled->shown_frame == tiled->render_frame)
  {
    if (block == 0) // consume event call
    {
      ctx_tiled_draw_cursor ((CtxTiled*)fb);
      ctx_kms_flip (fb);
    }
    return;
  }

  if (block)
  {
    int count = 0;
    while (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
    {
      usleep (500);
      count ++;
      if (count > 500)
      {
        tiled->shown_frame = tiled->render_frame;
        return;
      }
    }
  }
  else
  {
    if (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
      return;
  }

    if (tiled->vt_active)
    {
       int pre_skip = tiled->min_row * tiled->height/CTX_HASH_ROWS * tiled->width;
       int post_skip = (CTX_HASH_ROWS-tiled->max_row-1) * tiled->height/CTX_HASH_ROWS * tiled->width;

       int rows = ((tiled->width * tiled->height) - pre_skip - post_skip)/tiled->width;

       int col_pre_skip = tiled->min_col * tiled->width/CTX_HASH_COLS;
       int col_post_skip = (CTX_HASH_COLS-tiled->max_col-1) * tiled->width/CTX_HASH_COLS;
       if (_ctx_damage_control)
       {
         pre_skip = post_skip = col_pre_skip = col_post_skip = 0;
       }

       if (pre_skip < 0) pre_skip = 0;
       if (post_skip < 0) post_skip = 0;


       if (tiled->min_row == 100){
          pre_skip = 0;
          post_skip = 0;
          // not when kms ?
#if 0
     __u32 dummy = 0;
          ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
#endif
          ctx_tiled_undraw_cursor ((CtxTiled*)fb);
       }
       else
       {

      tiled->min_row = 100;
      tiled->max_row = 0;
      tiled->min_col = 100;
      tiled->max_col = 0;

     // not when kms ?
 #if 0
     __u32 dummy = 0;
     ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
#endif
     ctx_tiled_undraw_cursor ((CtxTiled*)fb);
     switch (fb->fb_bits)
     {
       case 32:
#if 1
         {
           uint8_t *dst = tiled->fb + pre_skip * 4;
           uint8_t *src = tiled->pixels + pre_skip * 4;
           int pre = col_pre_skip * 4;
           int post = col_post_skip * 4;
           int core = tiled->width * 4 - pre - post;
           for (int i = 0; i < rows; i++)
           {
             dst  += pre;
             src  += pre;
             memcpy (dst, src, core);
             src  += core;
             dst  += core;
             dst  += post;
             src  += post;
           }
         }
#else
         { int count = tiled->width * tiled->height;
           const uint32_t *src = (void*)tiled->pixels;
           uint32_t *dst = (void*)tiled->fb;
           count-= pre_skip;
           src+= pre_skip;
           dst+= pre_skip;
           count-= post_skip;
           while (count -- > 0)
           {
             dst[0] = ctx_swap_red_green2 (src[0]);
             src++;
             dst++;
           }
         }
#endif
         break;
         /* XXX  :  note: converting a scanline (or all) to target and
          * then doing a bulk memcpy be faster (at least with som /dev/fbs)  */
       case 24:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = tiled->fb;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 3;
           count-= post_skip;
           while (count -- > 0)
           {
             dst[0] = src[0];
             dst[1] = src[1];
             dst[2] = src[2];
             dst+=3;
             src+=4;
           }
         }
         break;
       case 16:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = tiled->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 2;
           while (count -- > 0)
           {
             int big = ((src[0] >> 3)) +
                ((src[1] >> 2)<<5) +
                ((src[2] >> 3)<<11);
             dst[0] = big & 255;
             dst[1] = big >>  8;
             dst+=2;
             src+=4;
           }
         }
         break;
       case 15:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = tiled->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 2;
           while (count -- > 0)
           {
             int big = ((src[2] >> 3)) +
                       ((src[1] >> 2)<<5) +
                       ((src[0] >> 3)<<10);
             dst[0] = big & 255;
             dst[1] = big >>  8;
             dst+=2;
             src+=4;
           }
         }
         break;
       case 8:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = tiled->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip;
           while (count -- > 0)
           {
             dst[0] = ((src[0] >> 5)) +
                      ((src[1] >> 5)<<3) +
                      ((src[2] >> 6)<<6);
             dst+=1;
             src+=4;
           }
         }
         break;
     }
    }
    ctx_tiled_cursor_drawn = 0;
    ctx_tiled_draw_cursor (&fb->tiled);
    ctx_kms_flip (fb);
    tiled->shown_frame = tiled->render_frame;
  }
}

void ctx_kms_consume_events (Ctx *ctx)
{
  CtxKMS *fb = (void*)ctx->backend;
  ctx_kms_show_frame (fb, 0);
  event_check_pending (&fb->tiled);
}

inline static void ctx_kms_start_frame (Ctx *ctx)
{
  ctx_kms_show_frame ((CtxKMS*)ctx->backend, 1);
}

void ctx_kms_free (CtxKMS *fb)
{
  if (fb->is_kms)
  {
    ctx_fbkms_close (fb);
  }
#ifdef __linux__
  ioctl (0, KDSETMODE, KD_TEXT);
#endif
  if (system("stty sane")){};
  ctx_tiled_free ((CtxTiled*)fb);
  //ctx_free (fb);
  ctx_babl_exit ();
}

//static unsigned char *fb_icc = NULL;
//static long fb_icc_length = 0;

#if 0
static CtxKMS *ctx_fb = NULL;
static void vt_switch_cb (int sig)
{
  CtxTiled *tiled = (void*)ctx_fb;
  if (sig == SIGUSR1)
  {
    if (ctx_fb->is_kms)
      ioctl(ctx_fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
    ioctl (0, VT_RELDISP, 1);
    ctx_fb->vt_active = 0;
#if 0
    ioctl (0, KDSETMODE, KD_TEXT);
#endif
  }
  else
  {
    ioctl (0, VT_RELDISP, VT_ACKACQ);
    ctx_fb->vt_active = 1;
    // queue draw
    tiled->render_frame = ++tiled->frame;
#if 0
    ioctl (0, KDSETMODE, KD_GRAPHICS);
#endif
    if (ctx_fb->is_kms)
    {
      ioctl(ctx_fb->fb_fd, DRM_IOCTL_SET_MASTER, 0);
      ctx_kms_flip (ctx_fb);
    }
    else
    {
      tiled->ctx->dirty=1;

      for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        tiled->hashes[(row * CTX_HASH_COLS + col) *  4] += 1;
      }
    }
  }
}
#endif

static int ctx_kms_get_mice_fd (Ctx *ctx)
{
  //CtxKMS *fb = (void*)ctx->backend;
  return _ctx_mice_fd;
}

Ctx *ctx_new_kms (int width, int height)
{
#if CTX_RASTERIZER
  CtxKMS *fb = ctx_calloc (sizeof (CtxKMS), 1);
  CtxBackend *backend = (CtxBackend*)fb;

  CtxTiled *tiled = (void*)fb;
  tiled->fb = ctx_fbkms_new (fb, &tiled->width, &tiled->height);
  if (tiled->fb)
  {
    fb->is_kms         = 1;
    width              = tiled->width;
    height             = tiled->height;
    /*
       we're ignoring the input width and height ,
       maybe turn them into properties - for
       more generic handling.
     */
    fb->fb_mapped_size = tiled->width * tiled->height * 4;
    fb->fb_bits        = 32;
    fb->fb_bpp         = 4;
  }
  if (!tiled->fb)
    return NULL;
  tiled->pixels = ctx_calloc (fb->fb_mapped_size, 1);

  ctx_babl_init ();

  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);

  backend->ctx = _ctx_new_drawlist (width, height);
  tiled->ctx_copy = _ctx_new_drawlist (width, height);

  tiled->width    = width;
  tiled->height   = height;
  tiled->show_frame = (void*)ctx_kms_show_frame;

  ctx_set_backend (backend->ctx, fb);
  ctx_set_backend (tiled->ctx_copy, fb);
  ctx_set_texture_cache (tiled->ctx_copy, backend->ctx);

  backend->end_frame = ctx_tiled_end_frame;
  backend->start_frame = ctx_kms_start_frame;
  backend->free  = (void*)ctx_kms_free;
  backend->process = (void*)ctx_drawlist_process;
  backend->consume_events = ctx_kms_consume_events;
  backend->get_event_fds = (void*) ctx_fb_get_event_fds;
  backend->set_clipboard = ctx_headless_set_clipboard;
  backend->get_clipboard = ctx_headless_get_clipboard;

  for (int i = 0; i < _ctx_max_threads; i++)
  {
    tiled->host[i] = ctx_new_for_framebuffer (tiled->pixels,
                   tiled->width/CTX_HASH_COLS, tiled->height/CTX_HASH_ROWS,
                   tiled->width * 4, CTX_FORMAT_BGRA8); // this format
                                  // is overriden in  thread
    ((CtxRasterizer*)(tiled->host[i]->backend))->swap_red_green = 1;
    ctx_set_texture_source (tiled->host[i], backend->ctx);
  }

  mtx_init (&tiled->mtx, mtx_plain);
  cnd_init (&tiled->cond);

#define start_thread(no)\
  if(_ctx_max_threads>no){ \
    static void *args[2]={(void*)no, };\
    thrd_t tid;\
    args[1]=fb;\
    thrd_create (&tid, (void*)ctx_tiled_render_fun, args);\
  }
  start_thread(0);
  start_thread(1);
  start_thread(2);
  start_thread(3);
  start_thread(4);
  start_thread(5);
  start_thread(6);
  start_thread(7);
  start_thread(8);
  start_thread(9);
  start_thread(10);
  start_thread(11);
  start_thread(12);
  start_thread(13);
  start_thread(14);
  start_thread(15);
#undef start_thread


  EvSource *kb = evsource_kb_new ();
  if (kb)
  {
    tiled->evsource[tiled->evsource_count++] = kb;
    kb->priv = fb;
  }
  EvSource *mice  = evsource_mice_new ();
  if (mice)
  {
    tiled->evsource[tiled->evsource_count++] = mice;
    mice->priv = fb;
  }

  tiled->vt_active = 1;
#ifdef __linux__
  ioctl(0, KDSETMODE, KD_GRAPHICS);
#endif
  tiled->shown_frame = tiled->render_frame;
#if 0
  signal (SIGUSR1, vt_switch_cb);
  signal (SIGUSR2, vt_switch_cb);

  struct vt_stat st;
  if (ioctl (0, VT_GETSTATE, &st) == -1)
  {
    ctx_log ("VT_GET_MODE on vt %i failed\n", fb->vt);
    return NULL;
  }

  fb->vt = st.v_active;

  struct vt_mode mode;
  mode.mode   = VT_PROCESS;
  mode.relsig = SIGUSR1;
  mode.acqsig = SIGUSR2;
  if (ioctl (0, VT_SETMODE, &mode) < 0)
  {
    ctx_log ("VT_SET_MODE on vt %i failed\n", fb->vt);
    return NULL;
  }
#endif

  return backend->ctx;
#else
  return NULL;
#endif
}
#endif
#endif
