#include "ctx-split.h"

#if CTX_EVENTS

#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#if CTX_HEADLESS
static int ctx_headless_get_mice_fd (Ctx *ctx)
{
  //CtxHeadless *fb = (void*)ctx->renderer;
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
   int          tty;
   cnd_t        cond;
   mtx_t        mtx;
};

#if UINTPTR_MAX == 0xffFFffFF
  #define fbdrmuint_t uint32_t
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
  #define fbdrmuint_t uint64_t
#endif


static void ctx_headless_flip (CtxHeadless *fb)
{
}

static void ctx_headless_show_frame (CtxHeadless *fb, int block)
{
  CtxTiled *tiled = (void*)fb;
  if (tiled->shown_frame == tiled->render_frame)
  {
    if (block == 0) // consume event call
    {
//    ctx_tiled_draw_cursor (tiled);
      ctx_headless_flip (fb);
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
 //       ctx_tiled_undraw_cursor (tiled);
       }
       else
       {

      tiled->min_row = 100;
      tiled->max_row = 0;
      tiled->min_col = 100;
      tiled->max_col = 0;
//   ctx_tiled_undraw_cursor (tiled);
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
 // ctx_tiled_cursor_drawn = 0;
 // ctx_tiled_draw_cursor (tiled);
    ctx_headless_flip (fb);
    tiled->shown_frame = tiled->render_frame;
  }
}

int ctx_headless_consume_events (Ctx *ctx)
{
  CtxHeadless *fb = (void*)ctx->renderer;
  ctx_headless_show_frame (fb, 0);
  event_check_pending (&fb->tiled);
  return 0;
}

inline static void ctx_headless_reset (CtxHeadless *fb)
{
  ctx_headless_show_frame (fb, 1);
}

inline static void ctx_headless_flush (CtxHeadless *fb)
{
  ctx_tiled_flush ((CtxTiled*)fb);
}

void ctx_headless_free (CtxHeadless *fb)
{
  CtxTiled*tiled=(CtxTiled*)fb;

  munmap (tiled->fb, fb->fb_mapped_size);
  close (fb->fb_fd);
  if (system("stty sane")){};
  ctx_tiled_free ((CtxTiled*)fb);
  //free (fb);
#if CTX_BABL
  babl_exit ();
#endif
}

//static unsigned char *fb_icc = NULL;
//static long fb_icc_length = 0;

int ctx_renderer_is_headless (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_headless_free)
          return 1;
  return 0;
}

static CtxHeadless *ctx_headless = NULL;


Ctx *ctx_new_headless (int width, int height)
{
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
  ctx_headless_events = 1;

#if CTX_BABL
  babl_init ();
#endif

  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);

  backend->ctx    = ctx_new ();
  tiled->ctx_copy = ctx_new ();
  tiled->width    = width;
  tiled->height   = height;

  ctx_set_renderer (backend->ctx, fb);
  ctx_set_renderer (tiled->ctx_copy, fb);
  ctx_set_texture_cache (tiled->ctx_copy, backend->ctx);

  ctx_set_size (backend->ctx, width, height);
  ctx_set_size (tiled->ctx_copy, width, height);

  backend->flush = (void*)ctx_headless_flush;
  backend->reset = (void*)ctx_headless_reset;
  backend->free  = (void*)ctx_headless_free;
  backend->set_clipboard = (void*)ctx_fb_set_clipboard;
  backend->get_clipboard = (void*)ctx_fb_get_clipboard;

  for (int i = 0; i < _ctx_max_threads; i++)
  {
    tiled->host[i] = ctx_new_for_framebuffer (tiled->pixels,
                   tiled->width/CTX_HASH_COLS, tiled->height/CTX_HASH_ROWS,
                   tiled->width * 4, CTX_FORMAT_BGRA8); // this format
                                  // is overriden in  thread
    ((CtxRasterizer*)(tiled->host[i]->renderer))->swap_red_green = 1;
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

  //ctx_flush (tiled->ctx);
  tiled->vt_active = 1;

  return backend->ctx;
}
#else

int ctx_renderer_is_headless (Ctx *ctx)
{
  return 0;
}
#endif
#endif
