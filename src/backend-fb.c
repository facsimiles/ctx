#include "ctx-split.h"

#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif


#if CTX_KMS || CTX_FB

static int ctx_fb_get_mice_fd (Ctx *ctx)
{
  //CtxFb *fb = (void*)ctx->backend;
  return _ctx_mice_fd;
}

static void ctx_fb_get_event_fds (Ctx *ctx, int *fd, int *count)
{
  int mice_fd = ctx_fb_get_mice_fd (ctx);
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
#endif

#if CTX_FB

#ifdef __linux__
  #include <linux/fb.h>
  #include <linux/vt.h>
  #include <linux/kd.h>
#endif

#ifdef __NetBSD__
  typedef uint8_t unchar;
  typedef uint8_t u_char;
  typedef uint16_t ushort;
  typedef uint32_t u_int;
  typedef uint64_t u_long;
  #include <sys/param.h>
  #include <dev/wscons/wsdisplay_usl_io.h>
  #include <dev/wscons/wsconsio.h>
  #include <dev/wscons/wsksymdef.h>
#endif

  #include <sys/mman.h>

typedef struct _CtxFb CtxFb;
struct _CtxFb
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
   int          vt;
   int          tty;
   cnd_t        cond;
   mtx_t        mtx;
#if __linux__
   struct       fb_var_screeninfo vinfo;
   struct       fb_fix_screeninfo finfo;
#endif
};

#if UINTPTR_MAX == 0xffFFffFF
  #define fbdrmuint_t uint32_t
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
  #define fbdrmuint_t uint64_t
#endif


static void ctx_fb_flip (CtxFb *fb)
{
#ifdef __linux__
  ioctl (fb->fb_fd, FBIOPAN_DISPLAY, &fb->vinfo);
#endif
}

static void ctx_fb_show_frame (CtxFb *fb, int block)
{
  CtxTiled *tiled = (void*)fb;
  if (tiled->shown_frame == tiled->render_frame)
  {
    if (block == 0) // consume event call
    {
      ctx_tiled_draw_cursor (tiled);
      ctx_fb_flip (fb);
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
      if (count > 2000)
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
#ifdef __linux__
           __u32 dummy = 0;
          ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
#endif
          ctx_tiled_undraw_cursor (tiled);
       }
       else
       {

      tiled->min_row = 100;
      tiled->max_row = 0;
      tiled->min_col = 100;
      tiled->max_col = 0;
#ifdef __linux__
    {
     __u32 dummy = 0;
     ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
    }
#endif
     ctx_tiled_undraw_cursor (tiled);
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
    ctx_tiled_draw_cursor (tiled);
    ctx_fb_flip (fb);
    tiled->shown_frame = tiled->render_frame;
  }
}

void ctx_fb_consume_events (Ctx *ctx)
{
  CtxFb *fb = (void*)ctx->backend;
  ctx_fb_show_frame (fb, 0);
  event_check_pending (&fb->tiled);
}

inline static void ctx_fb_reset (Ctx *ctx)
{
  ctx_fb_show_frame ((CtxFb*)ctx->backend, 1);
}

void ctx_fb_free (CtxFb *fb)
{
  CtxTiled*tiled=(CtxTiled*)fb;

//#ifdef __linux__
  ioctl (0, KDSETMODE, KD_TEXT);
//#endif
#ifdef __NetBSD__
  {
   int mode = WSDISPLAYIO_MODE_EMUL;
   ioctl (fb->fb_fd, WSDISPLAYIO_SMODE, &mode);
  }
#endif
  munmap (tiled->fb, fb->fb_mapped_size);
  close (fb->fb_fd);
  if (system("stty sane")){};
  ctx_tiled_free ((CtxTiled*)fb);
  //free (fb);
  ctx_babl_exit ();
}

//static unsigned char *fb_icc = NULL;
//static long fb_icc_length = 0;

static CtxFb *ctx_fb = NULL;
#ifdef __linux__
static void fb_vt_switch_cb (int sig)
{
  CtxTiled *tiled = (void*)ctx_fb;
  CtxBackend *backend = (void*)ctx_fb;
  if (sig == SIGUSR1)
  {
    ioctl (0, VT_RELDISP, 1);
    tiled->vt_active = 0;
    ioctl (0, KDSETMODE, KD_TEXT);
  }
  else
  {
    ioctl (0, VT_RELDISP, VT_ACKACQ);
    tiled->vt_active = 1;
    // queue draw
    tiled->render_frame = ++tiled->frame;
    ioctl (0, KDSETMODE, KD_GRAPHICS);
    {
      backend->ctx->dirty=1;

      for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        tiled->hashes[(row * CTX_HASH_COLS + col) *  20] += 1;
      }
    }
  }
}
#endif


Ctx *ctx_new_fb (int width, int height)
{
#if CTX_RASTERIZER
  CtxFb *fb = calloc (sizeof (CtxFb), 1);
  CtxTiled *tiled = (void*)fb;
  CtxBackend *backend = (void*)fb;
  ctx_fb = fb;
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
    fb->fb_path = strdup (dev_path);
  else
  {
#ifdef __linux__
    fb->fb_fd = open ("/dev/graphics/fb0", O_RDWR);
    if (fb->fb_fd > 0)
    {
      fb->fb_path = strdup ("/dev/graphics/fb0");
    }
    else
#endif
    {
      free (fb);
      return NULL;
    }
  }

#ifdef __linux__
  if (ioctl(fb->fb_fd, FBIOGET_FSCREENINFO, &fb->finfo))
    {
      fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      free (fb->fb_path);
      free (fb);
      return NULL;
    }

   if (ioctl(fb->fb_fd, FBIOGET_VSCREENINFO, &fb->vinfo))
     {
       fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      free (fb->fb_path);
      free (fb);
      return NULL;
     }
  ioctl (0, KDSETMODE, KD_GRAPHICS);

//fprintf (stderr, "%s\n", fb->fb_path);
  width = tiled->width = fb->vinfo.xres;
  height = tiled->height = fb->vinfo.yres;

  fb->fb_bits = fb->vinfo.bits_per_pixel;
//fprintf (stderr, "fb bits: %i\n", fb->fb_bits);

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
    return NULL;
  }
  if (ioctl (fb->fb_fd, WSDISPLAYIO_GINFO, &finfo)) {
    fprintf (stderr, "ioctl: WSIDSPLAYIO_GINFO failed\n");
    return NULL;
  }

  width = tiled->width = finfo.width;
  height = tiled->height = finfo.height;
  fb->fb_bits = finfo.depth;
  fb->fb_bpp = (fb->fb_bits + 1) / 8;
  fb->fb_mapped_size = width * height * fb->fb_bpp;


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

                                              
  tiled->fb = mmap (NULL, fb->fb_mapped_size, PROT_READ|PROT_WRITE, MAP_SHARED, fb->fb_fd, 0);
  }
  if (!tiled->fb)
    return NULL;
  tiled->pixels = calloc (fb->fb_mapped_size, 1);
  tiled->show_frame = (void*)ctx_fb_show_frame;

  ctx_babl_init ();

  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);

  backend->ctx    = _ctx_new_drawlist (width, height);
  tiled->ctx_copy = _ctx_new_drawlist (width, height);
  tiled->width    = width;
  tiled->height   = height;

  ctx_set_backend (backend->ctx, fb);
  ctx_set_backend (tiled->ctx_copy, fb);
  ctx_set_texture_cache (tiled->ctx_copy, backend->ctx);


  backend->flush = ctx_tiled_flush;
  backend->process = (void*)ctx_drawlist_process;

  backend->reset = ctx_fb_reset;
  backend->free  = (void*)ctx_fb_free;
  backend->set_clipboard = ctx_headless_set_clipboard;
  backend->get_clipboard = ctx_headless_get_clipboard;
  backend->consume_events = ctx_fb_consume_events;
  backend->get_event_fds = ctx_fb_get_event_fds;

  ctx_set_size (backend->ctx, width, height);
  ctx_set_size (tiled->ctx_copy, width, height);

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
  signal (SIGUSR1, fb_vt_switch_cb);
  signal (SIGUSR2, fb_vt_switch_cb);

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
