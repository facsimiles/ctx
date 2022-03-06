#include "ctx-split.h"

#if CTX_EVENTS
#if CTX_HEADLESS

#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

static char *ctx_fb_clipboard = NULL;
static void ctx_headless_set_clipboard (Ctx *ctx, const char *text)
{
  if (ctx_fb_clipboard)
    free (ctx_fb_clipboard);
  ctx_fb_clipboard = NULL;
  if (text)
  {
    ctx_fb_clipboard = strdup (text);
  }
}

static char *ctx_headless_get_clipboard (Ctx *ctx)
{
  if (ctx_fb_clipboard) return strdup (ctx_fb_clipboard);
  return strdup ("");
}

static int ctx_headless_get_mice_fd (Ctx *ctx)
{
  //CtxHeadless *fb = (void*)ctx->backend;
  return _ctx_mice_fd;
}

typedef struct _CtxHeadless CtxHeadless;
struct _CtxHeadless
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
   cnd_t        cond;
   mtx_t        mtx;
   int          tty;
};

#if UINTPTR_MAX == 0xffFFffFF
  #define fbdrmuint_t uint32_t
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
  #define fbdrmuint_t uint64_t
#endif

static void ctx_headless_show_frame (CtxHeadless *fb, int block)
{
  CtxTiled *tiled = (void*)fb;
  if (tiled->shown_frame == tiled->render_frame)
  {
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
       }
       else
       {
         tiled->min_row = 100;
         tiled->max_row = 0;
         tiled->min_col = 100;
         tiled->max_col = 0;
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
    }
    tiled->shown_frame = tiled->render_frame;
  }
}

void ctx_headless_consume_events (Ctx *ctx)
{
  CtxHeadless *fb = (void*)ctx->backend;
  ctx_headless_show_frame (fb, 0);
  event_check_pending (&fb->tiled);
}

inline static void ctx_headless_start_frame (Ctx *ctx)
{
  ctx_headless_show_frame ((CtxHeadless*)ctx->backend, 1);
}

void ctx_headless_free (CtxHeadless *fb)
{
  CtxTiled *tiled=(CtxTiled*)fb;

  if (tiled->fb)
  {
  free (tiled->fb); // it is not the tiled renderers responsibilty,
                    // since it might not be allocated this way
  tiled->fb = NULL;
  ctx_babl_exit (); // we do this together with the fb,
                    // which makes it happen only once
                    // even if the headless_free is called
                    // twice
  }
  //munmap (tiled->fb, fb->fb_mapped_size);
  //close (fb->fb_fd);
  //if (system("stty sane")){};
  ctx_tiled_free ((CtxTiled*)fb);
  //free (fb);
}

//static unsigned char *fb_icc = NULL;
//static long fb_icc_length = 0;

static CtxHeadless *ctx_headless = NULL;


Ctx *ctx_new_headless (int width, int height)
{
  ctx_babl_init ();
  if (width < 0 || height < 0)
  {
    width = 1920;
    height = 780;
  }
#if CTX_RASTERIZER
  CtxHeadless *fb = calloc (sizeof (CtxHeadless), 1);
  CtxBackend *backend = (CtxBackend*)fb;
  CtxTiled *tiled     = (CtxTiled*)fb;
  ctx_headless = fb;

  tiled->width = width;
  tiled->height = height;

  fb->fb_bits        = 32;
  fb->fb_bpp         = 4;
  fb->fb_mapped_size = width * height * 4;
#endif

  tiled->fb = calloc (fb->fb_mapped_size, 1);
  if (!tiled->fb)
    return NULL;
  tiled->pixels = calloc (fb->fb_mapped_size, 1);
  tiled->show_frame = (void*)ctx_headless_show_frame;


 // ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);
 //
 // not to be done for headless, we want sRGB thumbs - at least not device specific
 // perhaps rec2020 or similar?

  backend->ctx = _ctx_new_drawlist (width, height);
  backend->end_frame = ctx_tiled_end_frame;
  backend->process = (void*)ctx_drawlist_process;
  backend->start_frame = ctx_headless_start_frame;
  backend->free  = (void*)ctx_headless_free;
  backend->set_clipboard = ctx_headless_set_clipboard;
  backend->get_clipboard = ctx_headless_get_clipboard;
  backend->consume_events = ctx_headless_consume_events;

  tiled->ctx_copy = ctx_new (width, height, "drawlist");
  tiled->width    = width;
  tiled->height   = height;

  ctx_set_backend (backend->ctx, fb);
  ctx_set_backend (tiled->ctx_copy, fb);
  ctx_set_texture_cache (tiled->ctx_copy, backend->ctx);

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

  tiled->vt_active = 1;

  return backend->ctx;
}
#endif
#endif
