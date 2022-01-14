
#if CTX_TFT_ESPI

#ifndef CTX_TFT_ESPI_MEMORY_BUDGET
#define CTX_TFT_ESPI_MEMORY_BUDGET  (320*280)
#endif


typedef struct CtxTftBackend
{
  CtxBackend   backend;
  TFT_eSPI    *tft;
  int          flags;
  uint16_t    *fb;

  int     min_col; // hasher cols and rows
  int     min_row; // hasher cols and rows
  int     max_col; // hasher cols and rows
  int     max_row; // hasher cols and rows
  uint8_t hashes[CTX_HASH_ROWS * CTX_HASH_COLS * 20];
  uint8_t state[CTX_HASH_ROWS * CTX_HASH_COLS];
} CtxTftBackend;

static void ctx_render_tft (Ctx *ctx, 
                            int x0, int y0, int x1, int y1)
{
  CtxTftBackend *backend_tft = (CtxTftBackend*)ctx->backend;
  TFT_eSPI *tft = backend_tft->tft;
  int flags = backend_tft->flags;
  int memory_budget = CTX_TFT_ESPI_MEMORY_BUDGET;
  int width  = x1 - x0 + 1;
  int height = y1 - y0 + 1;
  uint16_t *fb;

  int chunk_size = 16; /* wanting chunks of 16 scanlines at a
                          time to go out seems to give good
                          spi bandwidth use */
  while (chunk_size * width * 2 > memory_budget/2)
  {
    chunk_size/=2;
  }
 
  if (!backend_tft->fb)
    backend_tft->fb = (uint16_t*)malloc (memory_budget);
  fb = backend_tft->fb;

  if (flags & CTX_TFT_332)
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
       CTX_FORMAT_RGB332);

    ctx_translate (renderer, -1.0 * x0, -1.0 * y0);
    ctx_render_ctx (ctx, renderer);
    ctx_free (renderer);

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
        ctx_332_unpack (val, &r, &g, &b);
        *dst++ = ctx_565_pack (r, g, b, 1);
      }
      tft->pushRect(x0, y, width, h, (uint16_t*)temp);
    }
      y0 += render_height;
    } while (y0 < y1);
  }
  else if (flags & CTX_TFT_GRAY)
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
    ctx_render_ctx (ctx, renderer);
    ctx_free (renderer);

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
      tft->pushRect(x0, y, width, h, (uint16_t*)temp);
    }
      y0 += render_height;
    } while (y0 < y1);
  }
  else
  {
    int render_height = height;
    if (width * render_height > memory_budget / 2)
    {
       render_height = memory_budget / width / 2;
    }

    do
    {
      render_height = ctx_mini (render_height, y1-y0);
      memset (fb, 0, width * 2 * render_height);
      Ctx *renderer = ctx_new_for_framebuffer (fb, width, render_height, width * 2,
            CTX_FORMAT_RGB565_BYTESWAPPED);
      ctx_translate (renderer, -1.0 * x0, -1.0 * y0);
      ctx_render_ctx (ctx, renderer);
      tft->pushRect(x0, y0, width, render_height, fb);
      ctx_free (renderer);    

      y0 += render_height;
    } while (y0 < y1);
  }
  if (flags & CTX_TFT_CYCLE_BUF)
  {
    free (fb);
    backend_tft->fb = NULL;
  }
}


static void
ctx_tft_flush (Ctx *ctx)
{
  CtxTftBackend *tft_backend = (CtxTftBackend*)ctx->backend;
  static int64_t prev_time = 0;
  int64_t cur_time = ctx_ticks () / 1000;

  if (tft_backend->flags & CTX_TFT_SHOW_FPS)
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


  if (tft_backend->flags & CTX_TFT_HASH_CACHE)
  {
    Ctx *hasher = ctx_hasher_new (ctx_width (ctx), ctx_height (ctx),
                                  CTX_HASH_COLS, CTX_HASH_ROWS);
    int dirty_tiles = 0;
    ctx_render_ctx (ctx, hasher);

    tft_backend->max_col = -100;
    tft_backend->min_col = 100;
    tft_backend->max_row = -100;
    tft_backend->min_row = 100;

      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          uint8_t *new_hash = ctx_hasher_get_hash (hasher, col, row);
          if (new_hash && memcmp (new_hash, &tft_backend->hashes[(row * CTX_HASH_COLS + col) *  20], 20))
          {
            memcpy (&tft_backend->hashes[(row * CTX_HASH_COLS +  col)*20], new_hash, 20);
            dirty_tiles++;

            tft_backend->max_col = ctx_maxi (tft_backend->max_col, col);
            tft_backend->max_row = ctx_maxi (tft_backend->max_row, row);
            tft_backend->min_col = ctx_mini (tft_backend->min_col, col);
            tft_backend->min_row = ctx_mini (tft_backend->min_row, row);
          }
        }
      free (((CtxHasher*)(hasher->backend))->hashes);
      ctx_free (hasher);


      if (dirty_tiles)
      {
         int x0 = tft_backend->min_col * (ctx_width (ctx)/CTX_HASH_COLS);
         int x1 = (tft_backend->max_col+1) * (ctx_width (ctx)/CTX_HASH_COLS)-1;
         int y0 = tft_backend->min_row * (ctx_height (ctx)/CTX_HASH_ROWS);
         int y1 = (tft_backend->max_row+1) * (ctx_height (ctx)/CTX_HASH_ROWS)-1;
#if 0
         ctx_rectangle (ctx, x0, y0, x1-x0+1, y1-y0+1);
         ctx_rgba (ctx, 1,0,0,0.5);
         ctx_line_width (ctx, 4.0);
         ctx_stroke (ctx);

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
         if ( (tft_backend->flags & CTX_TFT_AUTO_332) &&
              ((width) * height * 2 > CTX_TFT_ESPI_MEMORY_BUDGET))
         {
           tft_backend->flags |= CTX_TFT_332;
           ctx_render_tft (ctx, x0, y0, x1, y1);
           tft_backend->flags -= CTX_TFT_332;
         }
         else
         {
           ctx_render_tft (ctx, x0, y0, x1, y1);
         }
      }
  }
  else
  {
    ctx_render_tft (ctx, 0, 0, ctx_width(ctx)-1, ctx_height(ctx)-1);
  }
}

Ctx *ctx_new_tft (TFT_eSPI *tft, int flags)
{
  Ctx *ctx = ctx_new_drawlist (tft->width(), tft->height());
  CtxBackend *backend = (CtxBackend*)calloc (sizeof (CtxTftBackend), 1);
  CtxTftBackend *tft_backend = (CtxTftBackend*)backend;
  tft_backend->tft = tft;
  tft_backend->flags = flags;
  backend->flush = ctx_tft_flush;
  ctx_set_backend (ctx, backend);
  return ctx;
}

#endif
