#include "ctx-split.h"

#if CTX_FB

/**/

typedef struct _CtxFbCb CtxFbCb;
struct _CtxFbCb
{
   Ctx          *ctx;

   int           key_balance;
   int           key_repeat;
   int lctrl;
   int lalt;
   int rctrl;


   int           width;
   int           height;


   uint8_t*      fb;

   char         *fb_path;
   int           fb_fd;

   int           fb_bits;
   int           fb_bpp;
   int           fb_mapped_size;

   int vt;
   int tty;


   int vt_active;

#if __linux__
   struct       fb_var_screeninfo vinfo;
   struct       fb_fix_screeninfo finfo;
#endif


#if 0
   char         *title;
   const char   *prev_title;
   int           clipboard_requested;
   char         *clipboard;
   char         *clipboard_pasted;
#endif

   CtxCursor shown_cursor;

   int     is_kms;
   CtxKMS  kms;
};

static void fb_cb_set_pixels (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf)
{
  CtxFbCb *fb = (CtxFbCb*)user_data;

  if (fb->vt_active == 0)
	  return;

  int bpp = fb->fb_bpp;
  int ws = w * bpp;
  int stride = fb->width * bpp;

  uint8_t *src = (uint8_t*)buf;
  uint8_t *dst = fb->fb + stride * y + x * bpp;
  for (int scan = 0; scan < h; scan++)
  {
    memcpy (dst, src, ws);
    src += ws;
    dst += stride;
  }
  //Fb_Rect r = {x, y, w, h};
  //Fb_UpdateTexture (((CtxFbCb*)user_data)->texture, &r, buf, w * 4);

}

static void fb_cb_renderer_idle (Ctx *ctx, void *user_data)
{
//CtxFbCb *fb = (CtxFbCb*)user_data;
}

void *ctx_fbkms_new (CtxKMS *fb, int *width, int *height);
void ctx_fbkms_flip (CtxKMS *fb);
void ctx_fbkms_close (CtxKMS *fb);

static int fb_cb_frame_done (Ctx *ctx, void *user_data)
{
  CtxFbCb *fb = (CtxFbCb*)user_data;

  if (fb->is_kms)
  {
     ctx_fbkms_flip (&fb->kms);
  }
  else
  {
#ifdef __linux__
  // doing the following... and fps drops
  //__u32 dummy = 0;  
  //ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);  
    ioctl (fb->fb_fd, FBIOPAN_DISPLAY, &fb->vinfo);  
#endif
  }

  fb_cb_renderer_idle (ctx, user_data);

  return 0;
}

static void fb_cb_consume_events (Ctx *ctx, void *user_data)
{
  CtxFbCb *fb = (CtxFbCb*)user_data;
  CtxCbBackend *cb = ctx_get_backend (ctx);
  for (int i = 0; i < cb->evsource_count; i++)
  {
    while (evsource_has_event (cb->evsource[i]))
    {
      char *event = evsource_get_event (cb->evsource[i]);
      if (event)
      {
        if (fb->vt_active)
        {
          ctx_key_press (ctx, 0, event, 0); // we deliver all events as key-press, the key_press handler   disambiguates
        }
        ctx_free (event);
      }
    }
  }
}


static void ctx_fb_cb_get_event_fds (Ctx *ctx, int *fd, int *count)
{
  int mice_fd = _ctx_mice_fd;
  fd[0] = STDIN_FILENO;
  if (mice_fd)
  {
    fd[1] = mice_fd;
    *count = 2;
  }
  else
  {
    *count = 1;
  }
}

static void fb_cb_renderer_stop (Ctx *ctx, void *user_data)
{
  CtxFbCb *fb = (CtxFbCb*)user_data;

  if (fb->is_kms)
  {
     ctx_fbkms_flip (&fb->kms);
  }
  else
  {

#if CTX_FB_KDSETMODE
#ifdef __linux__
  ioctl (0, KDSETMODE, KD_TEXT);  
#endif
#endif
#ifdef __NetBSD__
  {  
   int mode = WSDISPLAYIO_MODE_EMUL;  
   ioctl (fb->fb_fd, WSDISPLAYIO_SMODE, &mode);  
  }  
#endif
  munmap (fb->fb, fb->fb_mapped_size);
  close (fb->fb_fd);  
  if (system("stty sane")){};
  }

}

#ifdef __linux__
static void fb_cb_vt_switch_cb (int sig)
{

  CtxFbCb  *fb = (void*)ctx_fb;
  CtxBackend *backend = ctx_get_backend(fb->ctx);
  CtxCbBackend *cb = (CtxCbBackend*)backend;
  if (sig == SIGUSR1)
  {
    ioctl (0, VT_RELDISP, 1);
    fb->vt_active = 0;
#if CTX_FB_KDSETMODE
#ifdef __linux__
    ioctl (0, KDSETMODE, KD_TEXT);  
#endif
#endif
  }
  else
  {
    ioctl (0, VT_RELDISP, VT_ACKACQ);
    fb->vt_active = 1;
#if CTX_FB_KDSETMODE
#ifdef __linux__
    ioctl (0, KDSETMODE, KD_GRAPHICS);
#endif
#endif
    ctx_queue_draw (fb->ctx);
    for (int i =0; i<CTX_HASH_COLS * CTX_HASH_ROWS; i++)
      cb->hashes[i] = 0;
  }
}
#endif



static int fb_cb_renderer_init (Ctx *ctx, void *user_data)
{
  CtxFbCb *fb = (CtxFbCb*)user_data;
  CtxCbBackend *cb = ctx_get_backend (ctx);

  ctx_fb = (CtxFb*)fb;

  int kms_w = 0; int kms_h = 0;
  uint8_t *base = NULL;
  if (0 && (base = ctx_fbkms_new(&fb->kms, &kms_w, &kms_h)))
  {
     fb->fb = base;
     fb->width = kms_w;
     fb->height = kms_h;
     fb->is_kms = 1;
     fb->fb_bits = 32;
     fb->fb_bpp = 4;
     fb->fb_mapped_size = fb->width * 4 * fb->height;
  }
  else
    {  
#ifdef __linux__
  const char *dev_path = "/dev/fb0";  
#endif
#ifdef __NetBSD__
  const char *dev_path = "/dev/ttyE0";  
#endif
#ifdef __OpenBSD__
  const char *dev_path = "/dev/ttyC0";  
#endif
  fb->fb_fd = open (dev_path, O_RDWR);  
  if (fb->fb_fd > 0)  
    fb->fb_path = ctx_strdup (dev_path);  
  else  
  {  
#ifdef __linux__
    fb->fb_fd = open ("/dev/graphics/fb0", O_RDWR);  
    if (fb->fb_fd > 0)  
    {  
      fb->fb_path = ctx_strdup ("/dev/graphics/fb0");  
    }  
    else  
#endif
    {  
      ctx_free (fb);  
      return -1;  
    }  
  }  

#ifdef __linux__
  if (ioctl(fb->fb_fd, FBIOGET_FSCREENINFO, &fb->finfo))
    {
      fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      ctx_free (fb->fb_path);
      ctx_free (fb);
      return -1;
    }

   if (ioctl(fb->fb_fd, FBIOGET_VSCREENINFO, &fb->vinfo))
     {
       fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      ctx_free (fb->fb_path);
      ctx_free (fb);
      return -1;
     }

  fb->width = fb->vinfo.xres;
  fb->height = fb->vinfo.yres;

  fb->fb_bits = fb->vinfo.bits_per_pixel;

  if (fb->fb_bits == 16)
    fb->fb_bits =
      fb->vinfo.red.length +
      fb->vinfo.green.length +
      fb->vinfo.blue.length;
   else if (fb->fb_bits == 8)
  {
    unsigned short red[256],  green[256],  blue[256];
  //  unsigned short original_red[256];
  //  unsigned short original_green[256];
  //  unsigned short original_blue[256];
    struct fb_cmap cmap = {0, 256, red, green, blue, NULL};
  //  struct fb_cmap original_cmap = {0, 256, original_red, original_green, original_blue, NULL};
    int i;

    /* do we really need to restore it ? */
   // if (ioctl (fb->fb_fd, FBIOPUTCMAP, &original_cmap) == -1)
   // {
   //   fprintf (stderr, "palette initialization problem %i\n", __LINE__);
   // }

    for (i = 0; i < 256; i++)
    {
      red[i]   = ((( i >> 5) & 0x7) << 5) << 8;
      green[i] = ((( i >> 2) & 0x7) << 5) << 8;
      blue[i]  = ((( i >> 0) & 0x3) << 6) << 8;
    }

        if (ioctl (fb->fb_fd, FBIOPUTCMAP, &cmap) == -1)
    {
      fprintf (stderr, "palette initialization problem %i\n", __LINE__);
    }
  }

  fb->fb_bpp = fb->vinfo.bits_per_pixel / 8;
  fb->fb_mapped_size = fb->finfo.smem_len;
#endif

#ifdef __NetBSD__
  struct wsdisplay_fbinfo finfo;

  int mode = WSDISPLAYIO_MODE_DUMBFB;
  //int mode = WSDISPLAYIO_MODE_MAPPED;
  if (ioctl (fb->fb_fd, WSDISPLAYIO_SMODE, &mode)) {
    return -1;
  }
  if (ioctl (fb->fb_fd, WSDISPLAYIO_GINFO, &finfo)) {
    fprintf (stderr, "ioctl: WSIDSPLAYIO_GINFO failed\n");
    return -1;
  }

  fb->width = finfo.width;
  fb->height = finfo.height;
  fb->fb_bits = finfo.depth;
  fb->fb_bpp = (fb->fb_bits + 1) / 8;
  fb->fb_mapped_size = fb->width * fb->height * fb->fb_bpp;


  if (fb->fb_bits == 8)
  {
    uint8_t red[256],  green[256],  blue[256];
    struct wsdisplay_cmap cmap;
    cmap.red = red;
    cmap.green = green;
    cmap.blue = blue;
    cmap.count = 256;
    cmap.index = 0;
    for (int i = 0; i < 256; i++)
    {
      red[i]   = ((( i >> 5) & 0x7) << 5);
      green[i] = ((( i >> 2) & 0x7) << 5);
      blue[i]  = ((( i >> 0) & 0x3) << 6);
    }

    ioctl (fb->fb_fd, WSDISPLAYIO_PUTCMAP, &cmap);
  }
#endif


  fb->fb = mmap (NULL, fb->fb_mapped_size, PROT_READ|PROT_WRITE, MAP_SHARED, fb->fb_fd, 0);
  }

  if (!fb->fb)
  {
    fprintf (stderr, "failed opening fb\n");
    return -1;
  }

  switch (fb->fb_bits)
  {
    case 32: cb->config.format = CTX_FORMAT_BGRA8; break;
    case 24: cb->config.format = CTX_FORMAT_RGB8; break;
    case 16: cb->config.format = CTX_FORMAT_RGB565; break;
    case 8: cb->config.format = CTX_FORMAT_RGB332; break;
  }

#if CTX_BABL
  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);  
#endif


  ctx_set_size (ctx, fb->width, fb->height);
#ifdef __linux__

  if (fb->is_kms == 0)
  {
    signal (SIGUSR1, fb_cb_vt_switch_cb);
    signal (SIGUSR2, fb_cb_vt_switch_cb);

    struct vt_stat st;
    if (ioctl (0, VT_GETSTATE, &st) == -1)
    {
      ctx_log ("VT_GET_MODE failed\n");
      return -1;
    }

    fb->vt = st.v_active;
    struct vt_mode mode;
    mode.mode   = VT_PROCESS;
    mode.relsig = SIGUSR1;
    mode.acqsig = SIGUSR2;
    if (ioctl (0, VT_SETMODE, &mode) < 0)
    {
      fprintf (stderr, "VT_SET_MODE on vt %i failed\n", fb->vt);
      return -1;
    }
#if CTX_FB_KDSETMODE
#ifdef __linux__
    ioctl (0, KDSETMODE, KD_GRAPHICS);
#endif
#endif
  }
#endif

  return 0;
}


Ctx *ctx_new_fb_cb (int width, int height)
{
  CtxFbCb *fb = (CtxFbCb*)ctx_calloc (1, sizeof (CtxFbCb));


  CtxCbConfig config = {
    .format         = CTX_FORMAT_RGBA8,
    .flags          = 0
	            | CTX_FLAG_HASH_CACHE
	            | CTX_FLAG_DOUBLE_BUFFER
		    | CTX_FLAG_POINTER
	           ,
    .memory_budget  = 1920*1200*4,
    .renderer_init  = fb_cb_renderer_init,
    .renderer_idle  = fb_cb_renderer_idle,
    .set_pixels     = fb_cb_set_pixels,
    .update_fb      = fb_cb_frame_done,
    .renderer_stop  = fb_cb_renderer_stop,
    .consume_events = fb_cb_consume_events,
    //.get_clipboard  = fb_cb_get_clipboard,
    //.set_clipboard  = fb_cb_set_clipboard,
    
    .user_data      = fb,
  };

  Ctx *ctx = ctx_new_cb (width, height, &config);
  if (!ctx)
    return NULL;
  fb->ctx = ctx;

  CtxCbBackend *cb = ctx_get_backend (ctx);
  EvSource *kb = NULL;

  CtxBackend *backend = (void*)cb;
  backend->get_event_fds = ctx_fb_cb_get_event_fds;

#if CTX_RAW_KB_EVENTS
  if (!kb) kb = evsource_kb_raw_new ();
#endif
  if (!kb) kb = evsource_kb_term_new ();
  if (kb)
  {
    cb->evsource[cb->evsource_count++] = kb;
    kb->priv = fb->ctx;
  }
  EvSource *mice  = NULL;
  mice = evsource_linux_ts_new ();
#if CTX_PTY
  if (!mice)
    mice = evsource_mice_new ();
#endif
  if (mice)
  {
    cb->evsource[cb->evsource_count++] = mice;
    mice->priv = fb->ctx;
  }
  fb->vt_active = 1;


  return fb->ctx;
}

#endif
