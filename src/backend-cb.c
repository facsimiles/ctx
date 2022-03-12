

typedef struct CtxCbBackend
{
  CtxBackend     backend;
  CtxPixelFormat format;
  int            flags;
  uint16_t      *fb;

  void (*set_pixels) (Ctx *ctx, void *user_data, 
                      int x, int y, int w, int h, void *buf, int buf_size);
  void   *set_pixels_user_data;
  void (*update_fb) (Ctx *ctx, void *user_data);
  void   *update_fb_user_data;

  int     min_col; // hasher cols and rows
  int     min_row; // hasher cols and rows
  int     max_col; // hasher cols and rows
  int     max_row; // hasher cols and rows
  uint32_t hashes[CTX_HASH_ROWS * CTX_HASH_COLS];
  int     memory_budget;
  Ctx    *renderer;
} CtxCbBackend;

void ctx_cb_set_flags (Ctx *ctx, int flags)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  backend_cb->flags = flags;
}

int ctx_cb_get_flags (Ctx *ctx)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  return backend_cb->flags;
}

static void ctx_render_cb (Ctx *ctx, 
                            int x0, int y0,
                            int x1, int y1,
                            uint32_t active_mask)
{
  CtxCbBackend *backend_cb = (CtxCbBackend*)ctx->backend;
  int flags                = backend_cb->flags;
  int memory_budget        = backend_cb->memory_budget;
  int width                = x1 - x0 + 1;
  int height               = y1 - y0 + 1;
  uint16_t *fb;
  CtxPixelFormat           format = backend_cb->format;
  int bpp                  = ctx_pixel_format_bits_per_pixel (format)/ 8;

  int chunk_size = 8; /* wanting chunks of 16 scanlines at a
                         time to go out seems to give good
                         spi bandwidth use */
  while (chunk_size * width * 2 > memory_budget/2)
  {
    chunk_size/=2;
  }
 
  if (!backend_cb->fb)
    backend_cb->fb = (uint16_t*)ctx_malloc (memory_budget);
  fb = backend_cb->fb;

  if (flags & CTX_FLAG_RGB332)
  {
#if 0
    int render_height = height;
    memory_budget -= chunk_size * width * 2;

    if (width * render_height > memory_budget)
    {
       render_height = memory_budget / width;
    }
    do
    {

    render_height = ctx_mini (render_height, y1-y0);
    if ((flags & CTX_FLAG_KEEP_DATA) == 0)
      memset (fb, 0, width * render_height);
    Ctx *renderer = ctx_new_for_framebuffer (fb,
       width, render_height, width,
       CTX_FORMAT_RGB332);

    ctx_translate (renderer, -1.0 * x0, -1.0 * y0);
    if (active_mask)
      ctx_render_ctx_masked (ctx, renderer, active_mask);
    else
      ctx_render_ctx (ctx, renderer);
    ctx_destroy (renderer);

    uint8_t *temp = ((uint8_t*)fb)+memory_budget;
    uint8_t *src = (uint8_t*)fb;

    for (int y = y0; y < y0 + render_height; y+=chunk_size)
    {
      uint16_t *dst = (uint16_t*)temp;
      float h = ctx_mini (chunk_size, y1-y);
      for (int i = 0; i < width * h; i++)
      {
        int val = *src++;
        uint8_t r, g, b;
        //ctx_332_unpack (val, &r, &g, &b);
        r=g=b=val;
        *dst++ = ctx_565_pack (r, g, b, 1);
      }
      backend_cb->set_pixels (ctx, backend_cb->set_pixels_user_data, 
                              x0, y, width, h, (uint16_t*)temp,
                              width * h * 2);
    }
      y0 += render_height;
    } while (y0 < y1);
#else
     int render_height = height;
     memory_budget -= chunk_size * width * 2;
     if (width * render_height > memory_budget)
     {
       render_height = memory_budget / width;
     }
    do
    {

    render_height = ctx_mini (render_height, y1-y0);
    memset (fb, 0, width * render_height);
    Ctx *renderer = ctx_new_for_framebuffer (fb,
       width, render_height, width,
       CTX_FORMAT_GRAY8);

    ctx_translate (renderer, -1.0 * x0, -1.0 * y0);
    if (active_mask)
      ctx_render_ctx_masked (ctx, renderer, active_mask);
    else
      ctx_render_ctx (ctx, renderer);
    ctx_destroy (renderer);

    uint8_t *temp = ((uint8_t*)fb)+memory_budget;
    uint8_t *src = (uint8_t*)fb;

    for (int y = y0; y < y0 + render_height; y+=chunk_size)
    {
      uint16_t *dst = (uint16_t*)temp;
      float h = ctx_mini (chunk_size, y1-y);
      for (int i = 0; i < width * h; i++)
      {
        int val = *src++;
        *dst++ = ctx_565_pack (val, val, val, 1);
      }
      backend_cb->set_pixels (ctx, backend_cb->set_pixels_user_data, 
                              x0, y, width, h, (uint16_t*)temp,
                              width * h * 2);
    }
      y0 += render_height;
    } while (y0 < y1);
#endif
  }
  else if (flags & CTX_FLAG_GRAY)
  {
     int render_height = height;
     memory_budget -= chunk_size * width * 2;
     if (width * render_height > memory_budget)
     {
       render_height = memory_budget / width;
     }
    do
    {

    render_height = ctx_mini (render_height, y1-y0);
    memset (fb, 0, width * render_height);
    Ctx *renderer = ctx_new_for_framebuffer (fb,
       width, render_height, width,
       CTX_FORMAT_GRAY8);

    ctx_translate (renderer, -1.0 * x0, -1.0 * y0);
    if (active_mask)
      ctx_render_ctx_masked (ctx, renderer, active_mask);
    else
      ctx_render_ctx (ctx, renderer);
    ctx_destroy (renderer);

    uint8_t *temp = ((uint8_t*)fb)+memory_budget;
    uint8_t *src = (uint8_t*)fb;

    for (int y = y0; y < y0 + render_height; y+=chunk_size)
    {
      uint16_t *dst = (uint16_t*)temp;
      float h = ctx_mini (chunk_size, y1-y);
      for (int i = 0; i < width * h; i++)
      {
        int val = *src++;
        *dst++ = ctx_565_pack (val, val, val, 1);
      }
      backend_cb->set_pixels (ctx, backend_cb->set_pixels_user_data, 
                              x0, y, width, h, (uint16_t*)temp,
                              width * h * 2);
    }
      y0 += render_height;
    } while (y0 < y1);
  }
  else
  {
    int render_height = height;
    if (width * render_height > memory_budget / bpp)
    {
       render_height = memory_budget / width / bpp;
    }

    do
    {
      render_height = ctx_mini (render_height, y1-y0);
      if ((flags & CTX_FLAG_KEEP_DATA) == 0)
      memset (fb, 0, width * bpp * render_height);
      Ctx *renderer = ctx_new_for_framebuffer (fb, width, render_height, width * bpp,
            format);
      ctx_translate (renderer, -1.0 * x0, -1.0 * y0);
      ctx_render_ctx (ctx, renderer);

      if (backend_cb->update_fb && (flags & CTX_FLAG_INTRA_UPDATE))
        backend_cb->update_fb (ctx, backend_cb->update_fb_user_data);

      backend_cb->set_pixels (ctx, backend_cb->set_pixels_user_data, 
                              x0, y0, width, render_height, (uint16_t*)fb,
                              width * render_height * bpp);

      if (backend_cb->update_fb && (flags & CTX_FLAG_INTRA_UPDATE))
        backend_cb->update_fb (ctx, backend_cb->update_fb_user_data);

      ctx_destroy (renderer);    

      y0 += render_height;
    } while (y0 < y1);
  }
#if 1
  if (flags & CTX_FLAG_CYCLE_BUF)
  {
    ctx_free (fb);
    backend_cb->fb = NULL;
  }
#endif
}

CTX_EXPORT int
ctx_cb_x0 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return cb_backend->min_col * (ctx_width (ctx)/CTX_HASH_COLS);
}

CTX_EXPORT int
ctx_cb_x1 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return (cb_backend->max_col+1) * (ctx_width (ctx)/CTX_HASH_COLS)-1;
}

CTX_EXPORT int
ctx_cb_y0 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return cb_backend->min_row * (ctx_height (ctx)/CTX_HASH_ROWS);
}

CTX_EXPORT int
ctx_cb_y1 (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  return (cb_backend->max_row+1) * (ctx_height (ctx)/CTX_HASH_ROWS)-1;
}

static void
ctx_cb_end_frame (Ctx *ctx)
{
  CtxCbBackend *cb_backend = (CtxCbBackend*)ctx->backend;
  static int64_t prev_time = 0;
  int64_t cur_time = ctx_ticks () / 1000;

  if (cb_backend->flags & CTX_FLAG_SHOW_FPS)
  {
   
  float em = ctx_height (ctx) * 0.08;
  float y = em;
  ctx_font_size (ctx, em);
  ctx_rectangle (ctx, ctx_width(ctx)-(em*4), 0, em *4, em * 1.1);
  ctx_rgba (ctx, 0, 0, 0, 0.7);
  ctx_fill (ctx);

  ctx_rgba (ctx, 1, 1, 0, 1);

  if (prev_time)
  {
    char buf[22];
    float fps = 1.0f/((cur_time-prev_time)/1000.0f);
    ctx_move_to (ctx, ctx_width (ctx) - (em * 3.8), y);
    sprintf (buf, "%2.1f fps", fps);
    ctx_text (ctx, buf);
    ctx_begin_path (ctx);
  }
  prev_time = cur_time;
  }


  if (cb_backend->flags & CTX_FLAG_HASH_CACHE)
  {
    Ctx *hasher = ctx_hasher_new (ctx_width (ctx), ctx_height (ctx),
                                  CTX_HASH_COLS, CTX_HASH_ROWS, &ctx->drawlist);
    int dirty_tiles = 0;
    ctx_render_ctx (ctx, hasher);

    cb_backend->max_col = -100;
    cb_backend->min_col = 100;
    cb_backend->max_row = -100;
    cb_backend->min_row = 100;

    uint32_t active_mask = 0;
    int tile_no =0;
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          uint32_t new_hash = ctx_hasher_get_hash (hasher, col, row);
          if (new_hash &&
              new_hash != cb_backend->hashes[(row * CTX_HASH_COLS + col)])
          {
            cb_backend->hashes[(row * CTX_HASH_COLS +  col)]= new_hash;
            dirty_tiles++;

            cb_backend->max_col = ctx_maxi (cb_backend->max_col, col);
            cb_backend->max_row = ctx_maxi (cb_backend->max_row, row);
            cb_backend->min_col = ctx_mini (cb_backend->min_col, col);
            cb_backend->min_row = ctx_mini (cb_backend->min_row, row);
          }

          active_mask |= (1<<tile_no);
          tile_no++;
        }
      ctx_free (((CtxHasher*)(hasher->backend))->hashes);
      ctx_destroy (hasher);

      if (dirty_tiles)
      {
         int x0 = cb_backend->min_col * (ctx_width (ctx)/CTX_HASH_COLS);
         int x1 = (cb_backend->max_col+1) * (ctx_width (ctx)/CTX_HASH_COLS)-1;
         int y0 = cb_backend->min_row * (ctx_height (ctx)/CTX_HASH_ROWS);
         int y1 = (cb_backend->max_row+1) * (ctx_height (ctx)/CTX_HASH_ROWS)-1;

         if (cb_backend->flags & CTX_FLAG_DAMAGE_CONTROL)
         {
#if 1
         ctx_save (ctx);
         ctx_rectangle (ctx, x0, y0, x1-x0+1, y1-y0+1);
         ctx_rgba (ctx, 1,0,0,0.5);
         ctx_line_width (ctx, 4.0);
         ctx_stroke (ctx);
         ctx_restore (ctx);
#endif
         }
#if 0
         //ctx_move_to (ctx, (x0+x1)/2, (y0+y1)/2);
         //char buf[44];
         //sprintf (buf, "%ix%i", ctx_width(ctx), ctx_height(ctx));
         //ctx_text (ctx, buf);

         //ctx_rgba (ctx, 0,1,0,0.5);
         //ctx_rectangle (ctx, 0, 0, ctx_width(ctx)/2, ctx_height(ctx)/2);
         //ctx_fill (ctx);
#endif
         int width = x1 - x0 + 1;
         int height = y1 - y0 + 1;
         if ( (cb_backend->flags & CTX_FLAG_AUTO_RGB332) &&
              ((width) * height * 2 > cb_backend->memory_budget))
         {
           cb_backend->flags |= CTX_FLAG_RGB332;
           ctx_render_cb (ctx, x0, y0, x1, y1, active_mask);
           cb_backend->flags -= CTX_FLAG_RGB332;
         }
         else
         {
           ctx_render_cb (ctx, x0, y0, x1, y1, active_mask);
         }
      }
  }
  else
  {
    ctx_render_cb (ctx, 0, 0, ctx_width(ctx)-1, ctx_height(ctx)-1, 0);
  }
  if (cb_backend->update_fb)
    cb_backend->update_fb (ctx, cb_backend->update_fb_user_data);
}

Ctx *ctx_new_cb (int width, int height, CtxPixelFormat format,
                 void (*set_pixels) (Ctx *ctx, void *user_data, 
                                     int x, int y, int w, int h, void *buf,
                                     int buf_size),
                 void *set_pixels_user_data,
                 void (*update_fb) (Ctx *ctx, void *user_data),
                 void *update_fb_user_data,
                 int   memory_budget,
                 void *scratch_fb,
                 int   flags)
{
  Ctx *ctx                   = ctx_new_drawlist (width, height);
  CtxBackend    *backend     = (CtxBackend*)ctx_calloc (sizeof (CtxCbBackend), 1);
  CtxCbBackend  *cb_backend  = (CtxCbBackend*)backend;
  backend->end_frame         = ctx_cb_end_frame;
  cb_backend->format         = format;
  cb_backend->fb             = (uint16_t*)scratch_fb;
  cb_backend->flags          = flags;
  cb_backend->set_pixels     = set_pixels;
  cb_backend->update_fb      = update_fb;
  cb_backend->set_pixels_user_data = set_pixels_user_data;
  cb_backend->update_fb_user_data   = update_fb_user_data;
  cb_backend->memory_budget  = memory_budget;
  ctx_set_backend (ctx, backend);
  if (!scratch_fb)
    cb_backend->fb = (uint16_t*)ctx_malloc (memory_budget);
  return ctx;
}

#if CTX_TFT_ESPI

static void ctx_tft_set_pixels (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf, int buf_size)
{
  TFT_eSPI *tft = (TFT_eSPI*)user_data;
  tft->pushRect (x, y, w, h, (uint16_t*)buf);
}

Ctx *ctx_new_tft (TFT_eSPI *tft,
                  int memory_budget,
                  void *scratch_fb,
                  int flags)
{
  return ctx_new_cb (tft->width(), tft->height(), 
                     CTX_FORMAT_RGB565_BYTESWAPPED,
                     ctx_tft_set_pixels,
                     tft,
                     NULL,
                     NULL,
                     memory_budget,
                     scratch_fb,
                     flags);
}
#endif
