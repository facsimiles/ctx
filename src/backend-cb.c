

typedef struct CtxCbBackend
{
  CtxBackend     backend;
  CtxPixelFormat format;
  int            flags;
  int     memory_budget;
  int     min_col; // hasher cols and rows
  int     min_row; // hasher cols and rows
  int     max_col; // hasher cols and rows
  int     max_row; // hasher cols and rows
  uint16_t      *fb;
  Ctx           *ctx;

  void (*set_pixels) (Ctx *ctx, void *user_data, 
                      int x, int y, int w, int h, void *buf, int buf_size);
  void   *set_pixels_user_data;
  int  (*update_fb) (Ctx *ctx, void *user_data);
  void   *update_fb_user_data;

  uint32_t hashes[CTX_HASH_ROWS * CTX_HASH_COLS];
  uint8_t res[CTX_HASH_ROWS * CTX_HASH_COLS]; // when non-0 we have non-full res rendered

  CtxHasher     rasterizer;
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
  backend_cb->memory_budget = memory_budget;
  if (backend_cb->fb)
    ctx_free (backend_cb->fb);
  backend_cb->fb = NULL;
}

static int ctx_render_cb (CtxCbBackend *backend_cb, 
                          int x0, int y0,
                          int x1, int y1,
                          uint32_t active_mask)
{
  Ctx *ctx           = backend_cb->ctx;
  int flags          = backend_cb->flags;
  int memory_budget  = backend_cb->memory_budget;
  int width          = x1 - x0 + 1;
  int height         = y1 - y0 + 1;
  uint16_t *fb;
  CtxPixelFormat format = backend_cb->format;
  int bpp            = ctx_pixel_format_bits_per_pixel (format) / 8;
  int abort          = 0;

  if (!backend_cb->fb)
    backend_cb->fb = (uint16_t*)ctx_malloc (memory_budget);
  fb = backend_cb->fb;

  if (flags & CTX_FLAG_LOWRES)
  {
    int scale_factor  = 1;
    int small_width   = width / scale_factor;
    int small_height  = height / scale_factor;

    int tbpp = bpp * 8;
    CtxPixelFormat tformat = format;
    if (flags & (CTX_FLAG_GRAY2|CTX_FLAG_GRAY4|CTX_FLAG_GRAY8))
    {
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
      else {
        tformat = CTX_FORMAT_GRAY4;
        tbpp = 4;
      }
    }
    else
    {
      if (flags & (CTX_FLAG_RGB332))
      {
        tbpp = 8;
        tformat = CTX_FORMAT_RGB332;
      }
    }
    int small_stride = (small_width * tbpp + 7) / 8;
    int min_scanlines = 4;

    while (memory_budget - (small_height * small_stride) < width * bpp * min_scanlines)
    {
      scale_factor ++;
      small_width   = width / scale_factor;
      small_height  = height / scale_factor;
      min_scanlines = scale_factor;
      small_stride  = (small_width * tbpp + 7) / 8;
    }

    int render_height = (memory_budget - (small_height * small_stride)) /
                        (width * bpp);
    uint8_t *gray_fb = (uint8_t*)fb;
    uint16_t *scaled = (uint16_t*)&gray_fb[small_height*small_stride];

    memset(fb, 0, small_stride * small_height);
    CtxRasterizer *r = ctx_rasterizer_init (&backend_cb->rasterizer,
                ctx, NULL, &ctx->state, fb, 0, 0, small_width, small_height,
                small_stride, tformat, CTX_ANTIALIAS_DEFAULT);
    ((CtxBackend*)r)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;
    ctx_push_backend (ctx, r);

    ctx_scale (ctx, 1.0f/scale_factor, 1.0f/scale_factor);
    ctx_translate (ctx, -1.0 * x0, -1.0 * y0);
    if (active_mask)
      ctx_render_ctx_masked (ctx, ctx, active_mask);
    else
      ctx_render_ctx (ctx, ctx);
    ctx_pop_backend (ctx);

    if (backend_cb->update_fb && (flags & CTX_FLAG_INTRA_UPDATE))
      backend_cb->update_fb (ctx, backend_cb->update_fb_user_data);
    int yo = 0;
    do
    {
      render_height = ctx_mini (render_height, y1-y0+1);
      int off = 0;
   
      if (flags & (CTX_FLAG_GRAY2|CTX_FLAG_GRAY4|CTX_FLAG_GRAY8))
      {
        const uint8_t *gray_fb = (uint8_t*)fb;
        if (tbpp == 1)
        {
          for (int y = 0; y < render_height; y++)
          {
            int sbase = (small_stride * ((yo+y)/scale_factor));
            for (int x = 0; x < width/scale_factor; x++)
            {
               int     soff = sbase + ((x)/8);
               uint8_t bits = gray_fb[soff];
               uint16_t val = (bits & (1<<(x&7)))?0xffff:0;
               for (int i = 0; i < scale_factor; i++)
                 scaled[off++]  = val;
            }
#if 1
            for (int ty = 1; ty < scale_factor; ty++)
            {
               for (int x = 0; x < width; x++)
               {
                 scaled[off] = scaled[off - width];
                 off++;
               }
               y++;
            }
#endif
          }
        }
        else if (tbpp == 2)
        {
          for (int y = 0; y < render_height; y++)
          {
            int sbase = (small_stride * ((yo+y)/scale_factor));
            for (int x = 0; x < width/scale_factor; x++)
            {
               int     soff = sbase + ((x)/4);
               uint8_t bits = gray_fb[soff];
               uint8_t g    = 85 * ((bits >> (2*(x&3)))&3);
               uint16_t val = ctx_565_pack (g, g, g, 1);
               for (int i = 0; i < scale_factor; i++)
                 scaled[off++]  = val;
            }
            for (int ty = 1; ty < scale_factor; ty++)
            {
               for (int x = 0; x < width; x++)
               {
                 scaled[off] = scaled[off - width];
                 off++;
               }
               y++;
            }
          }
        }
        else if (tbpp == 4)
        {
          for (int y = 0; y < render_height; y++)
          {
            int sbase = (small_stride * ((yo+y)/scale_factor));
            for (int x = 0; x < width/scale_factor; x++)
            {
               int     soff = sbase + ((x)/2);
               uint8_t bits = gray_fb[soff];
               uint8_t g    = 17 * ((bits >> (4*(x&1)))&15);
               uint16_t val = ctx_565_pack (g, g, g, 1);
               for (int i = 0; i < scale_factor; i++)
                 scaled[off++]  = val;
            }
            for (int ty = 1; ty < scale_factor; ty++)
            {
               for (int x = 0; x < width; x++)
               {
                 scaled[off] = scaled[off - width];
                 off++;
               }
               y++;
            }
          }
        }
        else
        {
          for (int y = 0; y < render_height; y++)
          {
            int sbase = (small_stride * ((yo+y)/scale_factor));
            for (int x = 0; x < width/scale_factor; x++)
            {
               uint8_t g   = gray_fb[sbase + (x)];
               uint16_t val = ctx_565_pack (g, g, g, 1);
               for (int i = 0; i < scale_factor; i++)
                 scaled[off++]  = val;
            }
            for (int ty = 1; ty < scale_factor; ty++)
            {
               for (int x = 0; x < width; x++)
               {
                 scaled[off] = scaled[off - width];
                 off++;
               }
               y++;
            }
          }
        }
      }
      else
      {
        if (tbpp == 8)
        {
          uint8_t *fb_u8 = (uint8_t*)fb;
          for (int y = 0; y < render_height; y++)
          {
            for (int x = 0; x < width/scale_factor; x++)
            {
               uint16_t val = ctx_rgb332_to_rgb565 (
                   fb_u8[small_width * ((yo+y)/scale_factor) + (x)], 1);
               for (int i = 0; i < scale_factor; i++)
                 scaled[off++]  = val;
            }
            for (int ty = 1; ty < scale_factor; ty++)
            {
               for (int x = 0; x < width; x++)
               {
                 scaled[off] = scaled[off - width];
                 off++;
               }
               y++;
            }
          }
        }
        else
        {
          for (int y = 0; y < render_height; y++)
          {
            for (int x = 0; x < width/scale_factor; x++)
            {
               uint16_t val = fb[small_width * ((yo+y)/scale_factor) + (x)];
               for (int i = 0; i < scale_factor; i++)
                 scaled[off++]  = val;
            }
            for (int ty = 1; ty < scale_factor; ty++)
            {
               for (int x = 0; x < width; x++)
               {
                 scaled[off] = scaled[off - width];
                 off++;
               }
               y++;
            }
          }
        }
      }
      backend_cb->set_pixels (ctx, backend_cb->set_pixels_user_data, 
                              x0, y0, width, render_height, (uint16_t*)scaled,
                              width * render_height * bpp);
      y0 += render_height;
      yo += render_height;
    } while (y0 < y1);

    if (backend_cb->update_fb && (flags & CTX_FLAG_INTRA_UPDATE))
      backend_cb->update_fb (ctx, backend_cb->update_fb_user_data);
    // abort does not happen for low-res update
  }
  else
  {
    int render_height = height;
    if (width * render_height > memory_budget / bpp)
    {
       render_height = memory_budget / width / bpp;
    }
    CtxRasterizer *r = ctx_rasterizer_init(&backend_cb->rasterizer,
                         ctx, NULL, &ctx->state, fb, 0, 0, width, height,
                         width * bpp, format, CTX_ANTIALIAS_DEFAULT);
    ((CtxBackend*)r)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;
    ctx_push_backend (ctx, r);

    do
    {
      render_height = ctx_mini (render_height, y1-y0+1);
      ctx_rasterizer_init (r, ctx, NULL, &ctx->state, fb, 0, 0, width,
                   render_height, width * bpp, format, CTX_ANTIALIAS_DEFAULT);
    ((CtxBackend*)r)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;

      if ((flags & CTX_FLAG_KEEP_DATA) == 0)
        memset (fb, 0, width * bpp * render_height);

      ctx_translate (ctx, -1.0 * x0, -1.0 * y0);
      if (active_mask)
        ctx_render_ctx_masked (ctx, ctx, active_mask);
      else
        ctx_render_ctx (ctx, ctx);

      backend_cb->set_pixels (ctx, backend_cb->set_pixels_user_data, 
                              x0, y0, width, render_height, (uint16_t*)fb,
                              width * render_height * bpp);

      if (backend_cb->update_fb && (flags & CTX_FLAG_INTRA_UPDATE))
        abort = backend_cb->update_fb (ctx, backend_cb->update_fb_user_data);

      y0 += render_height;
    } while (y0 < y1 && !abort);
    ctx_pop_backend (ctx);    
  }
  return abort;
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
  if (cb_backend->flags & (CTX_FLAG_GRAY2|CTX_FLAG_GRAY4|CTX_FLAG_GRAY8|CTX_FLAG_RGB332))
      cb_backend->flags|=CTX_FLAG_LOWRES;

  if (cb_backend->flags & CTX_FLAG_SHOW_FPS)
  {
   
  float em = ctx_height (ctx) * 0.08;
  float y = em;
  ctx_font_size (ctx, em);
  ctx_rectangle (ctx, ctx_width(ctx)-(em*4), 0, em *4, em * 1.1f);
  ctx_rgba (ctx, 0, 0, 0, 0.7f);
  ctx_fill (ctx);

  ctx_rgba (ctx, 1, 1, 0, 1);

  if (prev_time)
  {
    char buf[22];
    float fps = 1.0f/((cur_time-prev_time)/1000.0f);
    ctx_move_to (ctx, ctx_width (ctx) - (em * 3.8f), y);
    sprintf (buf, "%2.1f fps", (double)fps);
    ctx_text (ctx, buf);
    ctx_begin_path (ctx);
  }
  prev_time = cur_time;
  }


  if (cb_backend->flags & CTX_FLAG_HASH_CACHE)
  {
    CtxState    *state = &ctx->state;

    CtxPixelFormat format = cb_backend->format;
    int bpp               = ctx_pixel_format_bits_per_pixel (format) / 8;
    int tile_dim          = (ctx_width (ctx)/CTX_HASH_COLS) *
                            (ctx_height (ctx)/CTX_HASH_ROWS) * bpp;

    CtxRasterizer *rasterizer = &cb_backend->rasterizer;
    ctx_hasher_init (rasterizer, ctx, state, ctx_width(ctx), ctx_height(ctx), CTX_HASH_COLS, CTX_HASH_ROWS, &ctx->drawlist);
    ((CtxBackend*)rasterizer)->destroy = (CtxDestroyNotify)ctx_rasterizer_deinit;

    ctx_push_backend (ctx, rasterizer);

    int dirty_tiles = 0;
    ctx_render_ctx (ctx, ctx);

    cb_backend->max_col = -100;
    cb_backend->min_col = 100;
    cb_backend->max_row = -100;
    cb_backend->min_row = 100;

    uint32_t active_mask = 0;
    uint32_t *hashes = ((CtxHasher*)(ctx->backend))->hashes;
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
      int old_flags = cb_backend->flags;
      if (cb_backend->flags & CTX_FLAG_LOWRES)
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
            if ((cb_backend->flags & CTX_FLAG_REDUCED) == 0)
              cb_backend->flags &= ~CTX_FLAG_LOWRES;
            in_low_res = 0;
        }
        else if (dirty_tiles)
        {
            int memory = (cb_backend->max_col-cb_backend->min_col+1)*
                          (cb_backend->max_row-cb_backend->min_row+1)*tile_dim;
            if (memory < cb_backend->memory_budget && 0)
            {
              in_low_res = 0;
              if ((cb_backend->flags & CTX_FLAG_REDUCED) == 0)
                cb_backend->flags &= ~CTX_FLAG_LOWRES;
            }
        }
      }
      ctx_pop_backend (ctx);
      if (dirty_tiles)
      {
         int x0 = cb_backend->min_col     * (ctx_width (ctx)/CTX_HASH_COLS);
         int y0 = cb_backend->min_row     * (ctx_height (ctx)/CTX_HASH_ROWS);
         int x1 = (cb_backend->max_col+1) * (ctx_width (ctx)/CTX_HASH_COLS)-1;
         int y1 = (cb_backend->max_row+1) * (ctx_height (ctx)/CTX_HASH_ROWS);
         if (cb_backend->flags & CTX_FLAG_DAMAGE_CONTROL)
         {
           ctx_save (ctx);
           ctx_rectangle (ctx, x0, y0, x1-x0+1, y1-y0+1);
           ctx_rgba (ctx, 1,0,0,0.5);
           ctx_line_width (ctx, 4.0);
           ctx_stroke (ctx);
           ctx_restore (ctx);
         }
         //int width = x1 - x0 + 1;
         //int height = y1 - y0 + 1;
         int abort = 0;

         if (in_low_res)
         {
             abort = ctx_render_cb (cb_backend, x0, y0, x1, y1, active_mask);
             for (int row = cb_backend->min_row; row <= cb_backend->max_row; row++)
               for (int col = cb_backend->min_col; col <= cb_backend->max_col; col++)
               {
                 int tile_no = row * CTX_HASH_COLS + col;
                 if (abort)
                 {
                   //cb_backend->res[tile_no]=0;
                   //cb_backend->hashes[tile_no]= 23;
                 }
                 else
                 {
                   cb_backend->hashes[tile_no]= hashes[tile_no];
                   cb_backend->res[tile_no]=in_low_res;
                 }
               }
         }
         else // full res
         {
           tile_no = 0;
           int max_tiles = (cb_backend->memory_budget / tile_dim);

           for (int row = 0; row < CTX_HASH_ROWS; row++)
           {
             for (int col = 0; col < CTX_HASH_COLS; col++)
               if (!abort)
             {
               tile_no = row * CTX_HASH_COLS + col;
               uint32_t new_hash = hashes[tile_no];
               int used_tiles = 1;
               active_mask = 0;
               active_mask = 1<<tile_no;

               if ((new_hash != cb_backend->hashes[tile_no]) ||
                   cb_backend->res[tile_no])
               {
                    int x0 = col * (ctx_width (ctx)/CTX_HASH_COLS);
                    int x1 = (col+1) * (ctx_width (ctx)/CTX_HASH_COLS)-1;
                    int y0 = row * (ctx_height (ctx)/CTX_HASH_ROWS);
                    int y1 = (row+1) * (ctx_height (ctx)/CTX_HASH_ROWS);

#if 0
                    int cont = 1;
                    /* merge horizontal adjecant dirty tiles */
                    if (used_tiles < max_tiles && col + 1 < CTX_HASH_COLS) do {
                      uint32_t next_new_hash = hashes[tile_no+used_tiles];
                      if ((next_new_hash != cb_backend->hashes[tile_no+used_tiles]) ||
                        cb_backend->res[tile_no+used_tiles])
                      {
                        active_mask |= (1 << (tile_no+used_tiles));
                        used_tiles ++;
                        x1 += (ctx_width (ctx)/CTX_HASH_COLS);
                      }
                      else
                      {
                        cont = 0;
                      }
                    } while (used_tiles < max_tiles && cont && col + used_tiles < CTX_HASH_COLS);
#endif


                    abort = ctx_render_cb (cb_backend, x0, y0, x1, y1, active_mask);
                    if (!abort)
                    {
                      for (int i = 0; i < used_tiles; i ++)
                      {
                        cb_backend->res[tile_no + i]=0;
                        cb_backend->hashes[tile_no + i] = hashes[tile_no+i];
                      }
                    }
                    col += used_tiles - 1;
                  }
               }
           }
             }
           }
      cb_backend->flags = old_flags;
  }
  else
  {
    ctx_render_cb (cb_backend, 0, 0, ctx_width(ctx)-1, ctx_height(ctx)-1, 0);
  }
  if (cb_backend->update_fb)
    cb_backend->update_fb (ctx, cb_backend->update_fb_user_data);
}

Ctx *ctx_new_cb (int width, int height, CtxPixelFormat format,
                 void (*set_pixels) (Ctx *ctx, void *user_data, 
                                     int x, int y, int w, int h, void *buf,
                                     int buf_size),
                 void *set_pixels_user_data,
                 int (*update_fb) (Ctx *ctx, void *user_data),
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
  cb_backend->ctx = ctx;
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
