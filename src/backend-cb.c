
// the callback backend, 
//
// only set_pixels need implementing, 
//
//bugs:
// lowfi modes only work in 16bpp scanout mode
// random lockups, use mutexes?
//


void ctx_cb_set_flags (Ctx *ctx, int flags)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;

#if CTX_CB_ENABLE_LOW_FI
  if (flags & CTX_FLAG_GRAY2)
    flags |= CTX_FLAG_LOWFI;
  if (flags & CTX_FLAG_GRAY4)
    flags |= CTX_FLAG_LOWFI;
  if (flags & CTX_FLAG_GRAY8)
    flags |= CTX_FLAG_LOWFI;
  if (flags & CTX_FLAG_RGB332)
    flags |= CTX_FLAG_LOWFI;

  if (flags & CTX_FLAG_LOWFI)
    flags |= CTX_FLAG_HASH_CACHE;
#endif
  backend_cb->config.flags = flags;
}

int ctx_cb_get_flags (Ctx *ctx)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  return backend_cb->config.flags;
}

static inline uint16_t ctx_rgb332_to_rgb565 (uint8_t rgb, int byteswap)
{
   uint8_t red, green, blue;
   ctx_332_unpack (rgb, &red, &green, &blue);
   return ctx_565_pack (red, green, blue, byteswap);
}

void
ctx_cb_set_memory_budget (Ctx *ctx, int memory_budget)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  backend_cb->config.memory_budget = memory_budget;
  if (backend_cb->fb)
  {
    ctx_free (backend_cb->fb);
    backend_cb->fb = NULL;
  }
}

#define CTX_MEMDEBUG 0  // by setting this to 1 we get reports about
                        // scratch buffer overflows into the 1kb buffer
                        // area
                        //
#if CTX_MEMDEBUG
#define CTX_SCRATCH_PAD  512

static void
ctx_memdebug (CtxCbBackend *cb_backend, int line_no)
{
  int started = 0;
  int last = 0;
  int first = 0;
  if (!cb_backend->fb)
    return;
  for (int i = cb_backend->config.memory_budget/2; i < cb_backend->config.memory_budget/2 + CTX_SCRATCH_PAD/2;i++)
  {
    if (cb_backend->fb[i] != 42)
    {
      if (!started)
      {
        first = i;
        started = 1;
      }
      last = i;
      cb_backend->fb[i] = 42;
    }
  }
  if (started)
  fprintf (stderr, "%i scratch overreach - first wrong byte at buf + %i last: %i\n",
                  line_no,
                  first - cb_backend->config.memory_budget/2,
                                                                          last - cb_backend->config.memory_budget/2);
}
                        
#define CTX_VERIFY_MEM()  do{ctx_memdebug(backend_cb, __LINE__);}while(0)
#else
#define CTX_SCRATCH_PAD   0
#define CTX_VERIFY_MEM()  do{}while(0)
#endif

static int ctx_render_cb (CtxCbBackend *backend_cb, 
                          int x0, int y0,
                          int x1, int y1,
                          uint32_t active_mask)
{
  Ctx *ctx           = backend_cb->ctx;
  int flags          = backend_cb->config.flags;
  int memory_budget  = backend_cb->config.memory_budget;
  uint16_t *fb;
  CtxPixelFormat format = backend_cb->config.format;
  int bpp            = ctx_pixel_format_bits_per_pixel (format) / 8;
  int abort          = 0;
  Ctx *rctx = flags&CTX_FLAG_DOUBLE_BUFFER?backend_cb->drawlist_copy:ctx;
  
  int width          = x1 - x0 + 1;
  int height         = y1 - y0 + 1;
#if CTX_CB_ENABLE_LOW_FI
  int byteswap;
  byteswap = (format == CTX_FORMAT_RGB565_BYTESWAPPED);
#endif

  if (!backend_cb->fb)
  {
    backend_cb->fb = (uint16_t*)ctx_malloc (memory_budget + CTX_SCRATCH_PAD);
#if CTX_MEMDEBUG
    for (int i = memory_budget/2; i < memory_budget/2 + CTX_SCRATCH_PAD/2;i++)
      backend_cb->fb[i] = 42;
#endif
  }
  fb = backend_cb->fb;

  void (*set_pixels) (Ctx *ctx, void *user_data, 
                      int x, int y, int w, int h, void *buf) =
    backend_cb->config.set_pixels;

#if CTX_CB_ENABLE_LOW_FI
  if (flags & CTX_FLAG_LOWFI)
  {
    int scale_factor  = 1;
    int small_width   = width / scale_factor;
    int small_height  = height / scale_factor;

    int tbpp = bpp * 8;
    CtxPixelFormat tformat = format;
      if   (flags & CTX_FLAG_GRAY2)
      {
        tformat = CTX_FORMAT_GRAY2;
        tbpp = 2;
      }
      else if (flags & CTX_FLAG_GRAY4)
      {
        tformat = CTX_FORMAT_GRAY4;
        tbpp = 4;
      }
      else if (flags & CTX_FLAG_GRAY8)
      {
        tformat = CTX_FORMAT_GRAY8;
        tbpp = 8;
      }
      else
      if (flags & (CTX_FLAG_RGB332))
      {
        tbpp = 8;
        tformat = CTX_FORMAT_RGB332;
      }
    int small_stride = (small_width * tbpp + 7) / 8;
    int min_scanlines = 4;

    while (memory_budget - (small_height * small_stride) < width * bpp * min_scanlines)
    {
      scale_factor ++;
      small_width   = width / scale_factor;
      small_height  = height / scale_factor;
      min_scanlines = scale_factor * 2;
      small_stride  = (small_width * tbpp + 7) / 8;
    }

    int render_height = (memory_budget - (small_height * small_stride)) /
                        (width * bpp);

    const uint8_t *fb_u8 = (uint8_t*)fb;
    uint16_t *scaled = (uint16_t*)&fb_u8[small_height*small_stride];

    memset(fb, 0, small_stride * small_height);
    CtxRasterizer *r = ctx_rasterizer_init ((CtxRasterizer*)&backend_cb->rasterizer,
                rctx, NULL, &rctx->state, fb, 0, 0, small_width, small_height,
                small_stride, tformat, CTX_ANTIALIAS_DEFAULT);
    ((CtxBackend*)r)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;
    ctx_push_backend (rctx, r);

    ctx_scale (rctx, 1.0f/scale_factor, 1.0f/scale_factor);
    ctx_translate (rctx, -1.0f * x0, -1.0f * y0);
    if (active_mask)
      ctx_render_ctx_masked (rctx, rctx, active_mask);
    else
      ctx_render_ctx (rctx, rctx);
    ctx_pop_backend (rctx);

    if (backend_cb->config.update_fb && (flags & CTX_FLAG_INTRA_UPDATE))
       backend_cb->config.update_fb (ctx, backend_cb->backend.user_data);

    int yo = 0;
    do
    {
      render_height = ctx_mini (render_height, y1-y0+1);
      int off = 0;
      for (int y = 0; y < render_height; y++)
      {
        int sbase = (small_stride * ((yo+y)/scale_factor));
        off = y * width;
        switch (tformat)
        {
          case CTX_FORMAT_GRAY1:
            {
              int sx = 0;
              for (int x = 0; x < width;)
              {
                int     soff = sbase + ((sx)/8);
                uint8_t bits = fb_u8[soff];
                uint16_t val = (bits & (1<<(sx&7)))?0xffff:0;
                sx++;

                for (int i = 0; i < scale_factor && x < width; i++, x++)
                  scaled[off++]  = val;
              }
            }
            break;
          case CTX_FORMAT_GRAY2:
            {
              int sx = 0;
              for (int x = 0; x < width;)
              {
                int     soff = sbase + ((sx)/4);
                uint8_t bits = fb_u8[soff];
                uint8_t g    = 85 * ((bits >> (2*(sx&3)))&3);
                uint16_t val = ctx_565_pack (g, g, g, byteswap);
                sx++;

                for (int i = 0; i < scale_factor && x < width; i++, x++)
                  scaled[off++]  = val;
              }
            }
            break;
          case CTX_FORMAT_GRAY4:
            {
              int sx = 0;
              for (int x = 0; x < width;)
              {
                int     soff = sbase + ((sx)/2);
                uint8_t bits = fb_u8[soff];
                uint8_t g    = 17 * ((bits >> (4*(sx&1)))&15);
                uint16_t val = ctx_565_pack (g, g, g, byteswap);
                sx++;

                for (int i = 0; i < scale_factor && x < width; i++, x++)
                  scaled[off++]  = val;
              }
            }


            break;
          case CTX_FORMAT_GRAY8:
            {
              int sx = 0;
              for (int x = 0; x < width;)
              {
                uint8_t g   = fb_u8[sbase + (sx++)];
                uint16_t val = ctx_565_pack (g, g, g, byteswap);
                for (int i = 0; i < scale_factor && x < width; i++, x++)
                  scaled[off++]  = val;
              }
            }
            break;
          case CTX_FORMAT_RGB332:
            {
              int sx = 0;
              for (int x = 0; x < width;)
              {
                uint16_t val = ctx_rgb332_to_rgb565 (
                   fb_u8[sbase + (sx++)], byteswap);
                for (int i = 0; i < scale_factor && x < width; i++, x++)
                  scaled[off++]  = val;
              }
            }
            break;
          default:
          case CTX_FORMAT_RGB565:
          case CTX_FORMAT_RGB565_BYTESWAPPED:
            {
              int sx = 0;
              for (int x = 0; x < width;)
              {
                uint16_t val = fb[sbase/2+(sx++)];
                for (int i = 0; i < scale_factor && x < width; i++, x++)
                  scaled[off++]  = val;
              }
            }
            break;
        }
        for (int ty = 1; ty < scale_factor && y + 1< render_height; ty++)
        {
           memcpy (&scaled[off], &scaled[off-width], 2 * width);
           off += width;
           y++;
        }
      }
      set_pixels (ctx, backend_cb->backend.user_data,
                  x0, y0, width, render_height, (uint16_t*)scaled);
      y0 += render_height;
      yo += render_height;
    } while (y0 < y1);

    if (backend_cb->config.update_fb && (flags & CTX_FLAG_INTRA_UPDATE))
      backend_cb->config.update_fb (ctx, backend_cb->backend.user_data);
    // abort does not happen for low-res update
  }
  else
#endif
  {
    int render_height = height;
    if (width * render_height > memory_budget / bpp)
    {
       render_height = memory_budget / width / bpp;
    }
    CtxRasterizer *r = ctx_rasterizer_init((CtxRasterizer*)&backend_cb->rasterizer,
                         rctx, NULL, &rctx->state, fb, 0, 0, width, height,
                         width * bpp, format, CTX_ANTIALIAS_DEFAULT);
    ((CtxBackend*)r)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;
    ctx_push_backend (rctx, r);

    int do_intra = (((flags & CTX_FLAG_INTRA_UPDATE) != 0) && backend_cb->config.update_fb);
    int keep_data = ((flags & CTX_FLAG_KEEP_DATA) != 0);
    void *set_pixels_user_data = backend_cb->backend.user_data;
    do
    {
      render_height = ctx_mini (render_height, y1-y0+1);
      ctx_rasterizer_init (r, rctx, NULL, &rctx->state, fb, 0, 0, width,
                   render_height, width * bpp, format, CTX_ANTIALIAS_DEFAULT);
      ((CtxBackend*)r)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;

      if (!keep_data)
        memset (fb, 0, width * bpp * render_height);

      ctx_translate (rctx, -1.0f * x0, -1.0f * y0);
      if (active_mask)
        ctx_render_ctx_masked (rctx, rctx, active_mask);
      else
        ctx_render_ctx (rctx, rctx);

      set_pixels (ctx, set_pixels_user_data, 
                  x0, y0, width, render_height, (uint16_t*)fb);

      if (do_intra)
        abort = backend_cb->config.update_fb (ctx, backend_cb->backend.user_data);

      y0 += render_height;
    } while (y0 < y1 && !abort);
    ctx_pop_backend (rctx);    
  }
  return abort;
}


/* XXX: todo replace this with a single function that writes
 * to pointers, like path_extent
 */
static int
ctx_cb_x0 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return cb_backend->min_col * (ctx_width (ctx)/CTX_HASH_COLS);
}

static int
ctx_cb_x1 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return (cb_backend->max_col+1) * (ctx_width (ctx)/CTX_HASH_COLS)-1;
}

static int
ctx_cb_y0 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return cb_backend->min_row * (ctx_height (ctx)/CTX_HASH_ROWS);
}

static int
ctx_cb_y1 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return (cb_backend->max_row+1) * (ctx_height (ctx)/CTX_HASH_ROWS)-1;
}

void
ctx_cb_extent (Ctx *ctx, float *x0, float *y0, float *x1, float *y1)
{
  if (x0) *x0 = ctx_cb_x0 (ctx);
  if (y0) *y0 = ctx_cb_y0 (ctx);
  if (x1) *x1 = ctx_cb_x1 (ctx);
  if (y1) *y1 = ctx_cb_y1 (ctx);
}

static void
ctx_cb_start_frame (Ctx *ctx)
{
#if CTX_EVENTS
  // ctx_handle_events (ctx);
#endif
}

static void
ctx_cb_render_frame (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;

  int width  = ctx_width (ctx);
  int height = ctx_height (ctx);

  int tile_width = width / CTX_HASH_COLS;
  int tile_height = height / CTX_HASH_ROWS;
#if CTX_CB_ENABLE_LOW_FI
  if (cb_backend->config.flags & (CTX_FLAG_GRAY2|CTX_FLAG_GRAY4|CTX_FLAG_GRAY8|CTX_FLAG_RGB332))
      cb_backend->config.flags|=CTX_FLAG_LOWFI;
#endif

  Ctx *rctx = cb_backend->config.flags&CTX_FLAG_DOUBLE_BUFFER?cb_backend->drawlist_copy:ctx;

  if (cb_backend->config.flags & CTX_FLAG_HASH_CACHE)
  {
    CtxState    *state = &rctx->state;

    CtxPixelFormat format = cb_backend->config.format;
    int bpp               = ctx_pixel_format_bits_per_pixel (format) / 8;
    int tile_dim          = tile_width * tile_height * bpp;

    CtxRasterizer *rasterizer = (CtxRasterizer*)&cb_backend->rasterizer;
    ctx_hasher_init (rasterizer, rctx, state, width, height, CTX_HASH_COLS, CTX_HASH_ROWS, &rctx->drawlist);
    ((CtxBackend*)rasterizer)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;

    ctx_push_backend (rctx, rasterizer);

    int dirty_tiles = 0;
    ctx_render_ctx (rctx, rctx);

    cb_backend->max_col = -100;
    cb_backend->min_col = 100;
    cb_backend->max_row = -100;
    cb_backend->min_row = 100;

    uint32_t active_mask = 0;
    uint32_t *hashes = ((CtxHasher*)(rctx->backend))->hashes;
    int tile_no =0;
    int low_res_tiles = 0;

      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++, tile_no++)
        {
          uint32_t new_hash = hashes[tile_no];
          if (new_hash &&
              new_hash != cb_backend->hashes[tile_no])
          {
            dirty_tiles++;
            cb_backend->max_col = ctx_maxi (cb_backend->max_col, col);
            cb_backend->max_row = ctx_maxi (cb_backend->max_row, row);
            cb_backend->min_col = ctx_mini (cb_backend->min_col, col);
            cb_backend->min_row = ctx_mini (cb_backend->min_row, row);
          }
          else
          {
            low_res_tiles += cb_backend->res[tile_no];
          }
        }


      int in_low_res = 0;
      int old_flags = cb_backend->config.flags;
#if CTX_CB_ENABLE_LOW_FI
      if (cb_backend->config.flags & CTX_FLAG_LOWFI)
      {
        in_low_res = 1; // default to assume we're in low res
        if (dirty_tiles == 0 && low_res_tiles !=0) // no dirty and got low_res_tiles
        {
            cb_backend->max_col = -100;
            cb_backend->min_col = 100;
            cb_backend->max_row = -100;
            cb_backend->min_row = 100;
            tile_no = 0;
            for (int row = 0; row < CTX_HASH_ROWS; row++)
              for (int col = 0; col < CTX_HASH_COLS; col++, tile_no++)
              {
                if (cb_backend->res[tile_no])
              {
                cb_backend->max_col = ctx_maxi (cb_backend->max_col, col);
                cb_backend->max_row = ctx_maxi (cb_backend->max_row, row);
                cb_backend->min_col = ctx_mini (cb_backend->min_col, col);
                cb_backend->min_row = ctx_mini (cb_backend->min_row, row);
                dirty_tiles++;
              }
              }

            active_mask = 0;
            for (int row = cb_backend->min_row; row <= cb_backend->max_row; row++)
            for (int col = cb_backend->min_col; col <= cb_backend->max_col; col++)
            {
              tile_no = row * CTX_HASH_COLS + col;
              int tile_no = 0;
              active_mask |= (1<<tile_no);
            }
            if ((cb_backend->config.flags & CTX_FLAG_STAY_LOW) == 0)
              cb_backend->config.flags &= ~CTX_FLAG_LOWFI;
            in_low_res = 0;
        }
        else if (dirty_tiles)
        {
            int memory = (cb_backend->max_col-cb_backend->min_col+1)*
                          (cb_backend->max_row-cb_backend->min_row+1)*tile_dim;
            if (memory < cb_backend->config.memory_budget && 0)
            {
              in_low_res = 0;
              if ((cb_backend->config.flags & CTX_FLAG_STAY_LOW) == 0)
                cb_backend->config.flags &= ~CTX_FLAG_LOWFI;
            }
        }
      }
#endif

      ctx_pop_backend (rctx); // done with hasher
      if (dirty_tiles)
      {
         int x0 = cb_backend->min_col     * tile_width;
         int y0 = cb_backend->min_row     * tile_height;
         int x1 = (cb_backend->max_col+1) * tile_width - 1;
         int y1 = (cb_backend->max_row+1) * tile_height - 1;
#if 0
         if (cb_backend->config.flags & CTX_FLAG_DAMAGE_CONTROL)
         {
           ctx_save (ctx);
           ctx_rectangle (ctx, x0, y0, x1-x0+1, y1-y0+1);
           ctx_rgba (ctx, 1,0,0,0.5);
           ctx_line_width (ctx, 4.0);
           ctx_stroke (ctx);
           ctx_restore (ctx);
         }
#endif

         int width = x1 - x0 + 1;
         int height = y1 - y0 + 1;
         int abort = 0;
         int abortable = 1;

         if (dirty_tiles <= 4 && low_res_tiles <= 4)
         {
           in_low_res = 0;
           abortable = 0;
         }

         if (in_low_res)
         {
             abort = ctx_render_cb (cb_backend, x0, y0, x1, y1, active_mask);
             for (int row = cb_backend->min_row; row <= cb_backend->max_row; row++)
               for (int col = cb_backend->min_col; col <= cb_backend->max_col; col++)
               {
                 int tile_no = row * CTX_HASH_COLS + col;
                 //if (abort)
                 //{
                   //cb_backend->res[tile_no]=0;
                   //cb_backend->hashes[tile_no]= 23;
                 //}
                 //else
                 {
                   cb_backend->hashes[tile_no]= hashes[tile_no];
                   cb_backend->res[tile_no]=in_low_res;
                 }
               }
         }
         else // full res
         {
           tile_no = 0;

           if (width * height * bpp <= cb_backend->config.memory_budget)
           {
             // we have enough memory to render all in one go
             active_mask = 0;
             for (int row = cb_backend->min_row; row <= cb_backend->max_row; row++)
               for (int col = cb_backend->min_col; col <= cb_backend->max_col; col++)
               {
                 int tile_no = row * CTX_HASH_COLS + col;
                 cb_backend->res[tile_no]=0;
                 cb_backend->hashes[tile_no] = hashes[tile_no];
                 active_mask |= (1<<tile_no);
               }
              abort = ctx_render_cb (cb_backend, x0, y0, x1, y1, active_mask);
              if (!abortable)
                abort = 0;
           }
           else
           {
              // render row-by-row (no merging of rows)

           for (int row = 0; row < CTX_HASH_ROWS; row++)
           {
             for (int col = 0; col < CTX_HASH_COLS; col++)
               if (!abort)
             {
               tile_no = row * CTX_HASH_COLS + col;
               active_mask = 1<<tile_no;
               uint32_t new_hash = hashes[tile_no];
               int used_tiles = 1;

               if ((new_hash != cb_backend->hashes[tile_no]) ||
                   cb_backend->res[tile_no])
               {
                    int tx0 = col * tile_width;
                    int ty0 = row * tile_height;
                    int tx1 = tx0 +  tile_width-1;
                    int ty1 = ty0 +  tile_height-1;

#if 1
             int max_tiles = (cb_backend->config.memory_budget / tile_dim);
                    int cont = 1;
                    /* merge horizontal adjecant dirty tiles */
                    if (used_tiles < max_tiles && col + 1 < CTX_HASH_COLS) do {
                      uint32_t next_new_hash = hashes[tile_no+used_tiles];
                      if ((next_new_hash != cb_backend->hashes[tile_no+used_tiles]) ||
                        cb_backend->res[tile_no+used_tiles])
                      {
                        active_mask |= (1 << (tile_no+used_tiles));
                        used_tiles ++;
                        tx1 += (ctx_width (rctx)/CTX_HASH_COLS);
                      }
                      else
                      {
                        cont = 0;
                      }
                    } while (used_tiles < max_tiles && cont && col + used_tiles < CTX_HASH_COLS);
#endif


                    abort = ctx_render_cb (cb_backend, tx0, ty0, tx1, ty1, active_mask);
                    {
                      for (int i = 0; i < used_tiles; i ++)
                      {
                        cb_backend->res[tile_no + i]=0;
                        cb_backend->hashes[tile_no + i] = hashes[tile_no+i];
                      }
                    }
                    if (!abortable)
                       abort = 0;
                    col += used_tiles - 1;
                  }
               }
           }
           }

             }
           }
      cb_backend->config.flags = old_flags;
  }
  else
  {
    ctx_render_cb (cb_backend, 0, 0, ctx_width(rctx)-1, ctx_height(rctx)-1, 0);
  }
  if (cb_backend->config.update_fb)
    cb_backend->config.update_fb (ctx, cb_backend->backend.user_data);
}


#if CTX_PICO
#include "pico/multicore.h"
#define CTX_MB()  do{ __dmb(); __dsb(); __isb(); } while(0)
#else
#define CTX_MB()  do{ ; } while(0)
#endif

#if CTX_PICO | CTX_THREADS

#if CTX_PICO
void *core1_arg = NULL;
static void
ctx_cb_render_thread ()
#else
static void
ctx_cb_render_thread (CtxCbBackend *cb_backend)
#endif
{
#if CTX_PICO
  multicore_fifo_push_blocking (42);
  CtxCbBackend *cb_backend = core1_arg;
#endif
  Ctx *ctx = cb_backend->backend.ctx;

  if (cb_backend->config.renderer_init &&
      cb_backend->config.renderer_init (ctx, cb_backend->backend.user_data))
    return;

  cb_backend->rendering = 0;

  while (cb_backend->rendering >= 0){
    CTX_MB();
    while (cb_backend->rendering == 0)
    {
      CTX_MB();
      if (cb_backend->config.renderer_idle)
        cb_backend->config.renderer_idle (ctx, cb_backend->backend.user_data);
#if 0
      int start = ctx_ms (ctx);
      while (ctx_ms (ctx) - start < 2) {};
#else
      usleep (2 * 1000);
#endif
    }

      CTX_MB();
    if (cb_backend->rendering == 1)
    {
      ctx_cb_render_frame (ctx);
      cb_backend->rendering = 0;
      CTX_MB();
    }
  }

  if (cb_backend->config.renderer_stop)
     cb_backend->config.renderer_stop (ctx, cb_backend->backend.user_data);
}
#endif

static void
ctx_cb_end_frame (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  static int64_t prev_time = 0;
  int64_t cur_time = ctx_ticks () / 1000;

  if (cb_backend->config.flags & CTX_FLAG_SHOW_FPS)
  {
  
    if (prev_time)
    {
      char buf[22];
      float  ms = ((cur_time-prev_time));
      float fps = 1000.0f/ms;
      static float dfps = 0.0f;
      dfps = dfps * 0.9f + fps * 0.1f;
      sprintf (buf, "%2.1ffps", (double)dfps);

      if (cb_backend->config.windowtitle)
      {
#if CTX_EVENTS
	ctx_windowtitle (ctx, buf);
#endif
      }
      else
      {
        float em = ctx_height (ctx) * 0.08f;
        float y = em;
        ctx_save (ctx);
        ctx_font_size (ctx, em);
        ctx_rectangle (ctx, ctx_width(ctx)/2-(em*2), 0, em *4, em * 1.1f);
        ctx_rgba (ctx, 0, 0, 0, 0.7f);
        ctx_fill (ctx);
  
        ctx_rgba (ctx, 1, 1, 0, 1);

        ctx_move_to (ctx, ctx_width (ctx) * 0.5f, y);
        ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
        ctx_text (ctx, buf);
        ctx_reset_path (ctx);
        ctx_restore(ctx);
      }
    }
    prev_time = cur_time;
  }

  if (cb_backend->config.flags & CTX_FLAG_POINTER)
  {
    ctx_save(ctx);
    ctx_translate (ctx, ctx_pointer_x(ctx), ctx_pointer_y(ctx));
    ctx_move_to (ctx, 0,0);
    ctx_line_to (ctx, 30, 40);
    ctx_line_to (ctx, 10, 45);
    ctx_rgba (ctx, 0, 0, 0, 0.5);
    ctx_close_path (ctx);
    ctx_preserve (ctx);
    ctx_fill (ctx);
    ctx_rgba (ctx, 1, 1, 1, 0.5);
    ctx_line_width (ctx, 2.0);
    ctx_stroke (ctx);
    ctx_restore(ctx);
  }

  if (cb_backend->config.flags & CTX_FLAG_DOUBLE_BUFFER)
  {
    CTX_MB();
    while (cb_backend->rendering)
    {
#if CTX_EVENTS
      ctx_handle_events (ctx);
#endif
      CTX_MB();
    }

    CtxDrawlist temp = ctx->drawlist;
    ctx->drawlist = cb_backend->drawlist_copy->drawlist;
    cb_backend->drawlist_copy->drawlist = temp;
    ctx_set_size (cb_backend->drawlist_copy, ctx_width (ctx), ctx_height (ctx));

    cb_backend->rendering = 1;
    cb_backend->frame_no ++;
    CTX_MB();
  }
  else
  {
    ctx_cb_render_frame (ctx);
  }
#if CTX_EVENTS
  ctx_handle_events (ctx);
#endif
}

static void ctx_cb_destroy (void *data)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)data;
  if (cb_backend->config.flags & CTX_FLAG_DOUBLE_BUFFER)
  {
    cb_backend->rendering = -1;
#if CTX_EVENTS
    int start = ctx_ms (cb_backend->backend.ctx);
    while (ctx_ms (cb_backend->backend.ctx) - start < 250) {};
#else
    usleep (1000 * 1000 * 10);
#endif
    ctx_destroy (cb_backend->drawlist_copy);
  }
  else
  {
    if (cb_backend->config.renderer_stop)
      cb_backend->config.renderer_stop (cb_backend->backend.ctx, cb_backend->backend.user_data);
  }
  // XXX leaking ->fb if it was dynamically allocated
  free (data);
}

static void ctx_cb_consume_events (Ctx *ctx)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  backend_cb->config.consume_events (ctx, backend_cb->backend.user_data);
}

static void ctx_cb_windowtitle (Ctx *ctx, const char *utf8)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  backend_cb->config.windowtitle (ctx, backend_cb->backend.user_data, utf8);
}

static void ctx_cb_set_clipboard (Ctx *ctx, const char *utf8)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  backend_cb->config.set_clipboard (ctx, backend_cb->backend.user_data, utf8);
}

static char *ctx_cb_get_clipboard (Ctx *ctx)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  return backend_cb->config.get_clipboard (ctx, backend_cb->backend.user_data);
}


Ctx *ctx_new_cb (int width, int height, CtxCbConfig *config)
{
  Ctx          *ctx        = ctx_new_drawlist (width, height);
  CtxBackend   *backend    = (CtxBackend*)ctx_calloc (1, sizeof (CtxCbBackend));
  CtxCbBackend *cb_backend = (CtxCbBackend*)backend;

  backend->start_frame = ctx_cb_start_frame;
  backend->end_frame   = ctx_cb_end_frame;
  backend->destroy     = ctx_cb_destroy;
  backend->ctx         = ctx;
  backend->user_data   = config->user_data;

  cb_backend->config = *config;
  cb_backend->fb     = (uint16_t*)config->scratch_fb;

  ctx_set_backend (ctx, backend);

  ctx_cb_set_flags (ctx, config->flags);
  if (getenv ("CTX_SHOW_FPS"))
     cb_backend->config.flags |= CTX_FLAG_SHOW_FPS;
  cb_backend->ctx = ctx;

  if (config->consume_events)
    backend->consume_events = ctx_cb_consume_events;
  if (config->windowtitle)
    backend->set_windowtitle = ctx_cb_windowtitle;

  if (config->get_clipboard)
    backend->get_clipboard = ctx_cb_get_clipboard;
  if (config->set_clipboard)
    backend->set_clipboard = ctx_cb_set_clipboard;

  if (!config->scratch_fb)
  {
    cb_backend->config.memory_budget = 0;
    ctx_cb_set_memory_budget (ctx, config->memory_budget);
  }

  //if (config->renderer_init)
  //  cb_backend->config.flags |= CTX_FLAG_DOUBLE_BUFFER;

#if CTX_THREADS | CTX_PICO
  if (cb_backend->config.flags & CTX_FLAG_DOUBLE_BUFFER)
  {
    cb_backend->drawlist_copy = ctx_new_drawlist (width, height); // TODO : keep size in sync
    cb_backend->rendering = -1;

#if CTX_PICO
    core1_arg = cb_backend;
    CTX_MB();
    multicore_launch_core1(ctx_cb_render_thread);
    multicore_fifo_pop_blocking ();
#else
    thrd_t tid;
    thrd_create(&tid, (void*)ctx_cb_render_thread, (void*) cb_backend);
#endif

    if (cb_backend->config.renderer_init)
    {
       int n = 250;
       while (cb_backend->rendering == -1 && n-- > 0){
#if CTX_EVENTS
          int start = ctx_ms (ctx);
	  while (ctx_ms (ctx) - start < 20) {};
#else
	  usleep(20 * 1000);
#endif
       }
       if (cb_backend->rendering == -1)
       {
	 ctx_destroy (ctx);
	 return NULL;
       }
    } else
    {
       cb_backend->rendering = 0;
    }
  }
  else
#endif
  {
    if (cb_backend->config.renderer_init)
      {
	 if (cb_backend->config.renderer_init (ctx, cb_backend->backend.user_data))
	 {
	   ctx_destroy (ctx);
	   return NULL;
	 }
      }
  }

#if CTX_EVENTS
  //ctx_get_event (ctx);
#endif
  return ctx;
}


/* this is a compatibility function to make porting to new ctx quicker
 * first check that things work with this and then
 * implement new API
 */
Ctx *ctx_new_cb_old (int width, int height, CtxPixelFormat format,
                     void (*set_pixels) (Ctx *ctx, void *user_data, 
                                     int x, int y, int w, int h, void *buf),
                     void *set_pixels_user_data,
                     int (*update_fb) (Ctx *ctx, void *user_data),
                     void *update_fb_user_data,
                     int   memory_budget,
                     void *scratch_fb,
                     int   flags)
{
  CtxCbConfig config = {
    .format        = format,
    .flags         = flags,
    .memory_budget = memory_budget,
    .update_fb     = update_fb,
    .set_pixels    = set_pixels,
    .scratch_fb    = scratch_fb,
    .user_data     = set_pixels_user_data
  };
  if (update_fb_user_data && update_fb_user_data != set_pixels_user_data)
  {
    assert(0);exit(0);
  }
  return ctx_new_cb (width, height, &config);
}
