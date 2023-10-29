#include "ctx-split.h"

#if CTX_TILED
static inline int
ctx_tiled_threads_done (CtxTiled *tiled)
{
  int sum = 0;
  for (int i = 0; i < _ctx_max_threads; i++)
  {
     if (tiled->rendered_frame[i] == tiled->render_frame)
       sum ++;
  }
  return sum;
}

int _ctx_damage_control = 0;

void ctx_tiled_destroy (CtxTiled *tiled)
{
  tiled->quit = 1;
  mtx_lock (&tiled->mtx);
  cnd_broadcast (&tiled->cond);
  mtx_unlock (&tiled->mtx);

  while (tiled->thread_quit < _ctx_max_threads)
    usleep (1000);

  if (tiled->pixels)
  {
    ctx_free (tiled->pixels);
    tiled->pixels = NULL;
    for (int i = 0 ; i < _ctx_max_threads; i++)
    {
      if (tiled->host[i])
        ctx_destroy (tiled->host[i]);
      tiled->host[i]=NULL;
    }
    ctx_destroy (tiled->ctx_copy);
  }

  // leak?
}
static unsigned char *sdl_icc = NULL;
static long sdl_icc_length = 0;

static void ctx_tiled_end_frame (Ctx *ctx)
{
  CtxTiled *tiled = (CtxTiled*)ctx->backend;
  mtx_lock (&tiled->mtx);
  if (tiled->shown_frame == tiled->render_frame)
  {
    int dirty_tiles = 0;
    ctx_set_drawlist (tiled->ctx_copy, &tiled->backend.ctx->drawlist.entries[0],
                                           tiled->backend.ctx->drawlist.count * 9);
    if (_ctx_enable_hash_cache)
    {
      Ctx *hasher = ctx_hasher_new (tiled->width, tiled->height,
                        CTX_HASH_COLS, CTX_HASH_ROWS, &tiled->ctx_copy->drawlist);
      ctx_render_ctx (tiled->ctx_copy, hasher);

      for (int row = 0; row < CTX_HASH_ROWS; row++)
      {
        for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          uint32_t new_hash = ctx_hasher_get_hash (hasher, col, row);
          if (new_hash && new_hash != tiled->hashes[(row * CTX_HASH_COLS + col)])
          {
            tiled->hashes[(row * CTX_HASH_COLS +  col)] = new_hash;
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
            dirty_tiles++;
          }
          else
          {
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = -1;
          }
        }
      }

      ctx_destroy (hasher);
    }
    else
    {
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++)
          {
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
            dirty_tiles++;
          }
    }
    int dirty_no = 0;
    if (dirty_tiles)
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        if (tiled->tile_affinity[row * CTX_HASH_COLS + col] != -1)
        {
          tiled->tile_affinity[row * CTX_HASH_COLS + col] = dirty_no * (_ctx_max_threads) / dirty_tiles;
          dirty_no++;
          if (col > tiled->max_col) tiled->max_col = col;
          if (col < tiled->min_col) tiled->min_col = col;
          if (row > tiled->max_row) tiled->max_row = row;
          if (row < tiled->min_row) tiled->min_row = row;
        }
      }

    if (_ctx_damage_control)
    {
      for (int i = 0; i < tiled->width * tiled->height; i++)
      {
        tiled->pixels[i*4+2]  = (tiled->pixels[i*4+2] + 255)/2;
      }
    }

    tiled->render_frame = ++tiled->frame;

#if 0

          //if (tiled->tile_affinity[hno]==no)
          {
            int x0 = ((tiled->width)/CTX_HASH_COLS) * 0;
            int y0 = ((tiled->height)/CTX_HASH_ROWS) * 0;
            int width = tiled->width / CTX_HASH_COLS;
            int height = tiled->height / CTX_HASH_ROWS;
            Ctx *host = tiled->host[0];

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->backend;
            int swap_red_green = ((CtxRasterizer*)(host->backend))->swap_red_green;
            ctx_rasterizer_init (rasterizer,
                                 host, tiled->backend.ctx, &host->state,
                                 &tiled->pixels[tiled->width * 4 * y0 + x0 * 4],
                                 0, 0, 1, 1,
                                 tiled->width*4, CTX_FORMAT_BGRA8,
                                 tiled->antialias);
            ((CtxRasterizer*)(host->backend))->swap_red_green = swap_red_green;
            if (sdl_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, sdl_icc, sdl_icc_length);

            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (tiled->ctx_copy, host);
          }
#endif
    cnd_broadcast (&tiled->cond);
  }
  else
  {
    fprintf (stderr, "{drip}");
  }
  mtx_unlock (&tiled->mtx);
  ctx_drawlist_clear (ctx);
  ctx_handle_events (ctx);
}

static
void ctx_tiled_render_fun (void **data)
{
  int      no = (size_t)data[0];
  CtxTiled *tiled = data[1];

  while (!tiled->quit)
  {
    Ctx *host = tiled->host[no];

    mtx_lock (&tiled->mtx);
    cnd_wait(&tiled->cond, &tiled->mtx);
    mtx_unlock (&tiled->mtx);

    if (tiled->render_frame != tiled->rendered_frame[no])
    {
      int hno = 0;
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++, hno++)
        {
          if (tiled->tile_affinity[hno]==no)
          {
            int x0 = ((tiled->width)/CTX_HASH_COLS) * col;
            int y0 = ((tiled->height)/CTX_HASH_ROWS) * row;
            int width = tiled->width / CTX_HASH_COLS;
            int height = tiled->height / CTX_HASH_ROWS;

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->backend;

            int active_mask = 1 << hno;

#if CTX_TILED_MERGE_HORIZONTAL_NEIGHBORS
            while (col + 1 < CTX_HASH_COLS &&
                   tiled->tile_affinity[hno+1] == no)
            {
              width += tiled->width / CTX_HASH_COLS;
              col++;
              hno++;
              active_mask |= 1 << hno;
            }
#endif
            int swap_red_green = rasterizer->swap_red_green;
            ctx_rasterizer_reinit (host,
                                 &tiled->pixels[tiled->width * 4 * y0 + x0 * 4],
                                 0, 0, width, height,
                                 tiled->width*4, CTX_FORMAT_BGRA8);
            ((CtxRasterizer*)(host->backend))->swap_red_green = swap_red_green;
            if (sdl_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, sdl_icc, sdl_icc_length);

            ctx_translate (host, -x0, -y0);
            ctx_render_ctx_masked (tiled->ctx_copy, host, active_mask);

          }
        }
      tiled->rendered_frame[no] = tiled->render_frame;
    }
  }
  tiled->thread_quit++; // need atomic?
}


static int       ctx_tiled_cursor_drawn   = 0;
static int       ctx_tiled_cursor_drawn_x = 0;
static int       ctx_tiled_cursor_drawn_y = 0;
static CtxCursor ctx_tiled_cursor_drawn_shape = 0;


#define CTX_FB_HIDE_CURSOR_FRAMES 200

static int ctx_tiled_cursor_same_pos = CTX_FB_HIDE_CURSOR_FRAMES;

static inline int ctx_is_in_cursor (int x, int y, int size, CtxCursor shape)
{
  switch (shape)
  {
    case CTX_CURSOR_ARROW:
      if (x > ((size * 4)-y*4)) return 0;
      if (x < y && x > y / 16)
        return 1;
      return 0;

    case CTX_CURSOR_RESIZE_SE:
    case CTX_CURSOR_RESIZE_NW:
    case CTX_CURSOR_RESIZE_SW:
    case CTX_CURSOR_RESIZE_NE:
      {
        float theta = -45.0f/180 * (float)(M_PI);
        float cos_theta;
        float sin_theta;

        if ((shape == CTX_CURSOR_RESIZE_SW) ||
            (shape == CTX_CURSOR_RESIZE_NE))
        {
          theta = -theta;
          cos_theta = ctx_cosf (theta);
          sin_theta = ctx_sinf (theta);
        }
        else
        {
          cos_theta = ctx_cosf (theta);
          sin_theta = ctx_sinf (theta);
        }
        int rot_x = (int)(x * cos_theta - y * sin_theta);
        int rot_y = (int)(y * cos_theta + x * sin_theta);
        x = rot_x;
        y = rot_y;
      }
      /*FALLTHROUGH*/
    case CTX_CURSOR_RESIZE_W:
    case CTX_CURSOR_RESIZE_E:
    case CTX_CURSOR_RESIZE_ALL:
      if (abs (x) < size/2 && abs (y) < size/2)
      {
        if (abs(y) < size/10)
        {
          return 1;
        }
      }
      if ((abs (x) - size/ (shape == CTX_CURSOR_RESIZE_ALL?2:2.7)) >= 0)
      {
        if (abs(y) < (size/2.8)-(abs(x) - (size/2)))
          return 1;
      }
      if (shape != CTX_CURSOR_RESIZE_ALL)
        break;
      /* FALLTHROUGH */
    case CTX_CURSOR_RESIZE_S:
    case CTX_CURSOR_RESIZE_N:
      if (abs (y) < size/2 && abs (x) < size/2)
      {
        if (abs(x) < size/10)
        {
          return 1;
        }
      }
      if ((abs (y) - size/ (shape == CTX_CURSOR_RESIZE_ALL?2:2.7)) >= 0)
      {
        if (abs(x) < (size/2.8)-(abs(y) - (size/2)))
          return 1;
      }
      break;
#if 0
    case CTX_CURSOR_RESIZE_ALL:
      if (abs (x) < size/2 && abs (y) < size/2)
      {
        if (abs (x) < size/10 || abs(y) < size/10)
          return 1;
      }
      break;
#endif
    default:
      return (x ^ y) & 1;
  }
  return 0;
}

static void ctx_tiled_undraw_cursor (CtxTiled *tiled)
{
    int cursor_size = ctx_height (tiled->backend.ctx) / 28;

    if (ctx_tiled_cursor_drawn)
    {
      int no = 0;
      int startx = -cursor_size;
      int starty = -cursor_size;
      if (ctx_tiled_cursor_drawn_shape == CTX_CURSOR_ARROW)
        startx = starty = 0;

      for (int y = starty; y < cursor_size; y++)
      for (int x = startx; x < cursor_size; x++, no+=4)
      {
        if (x + ctx_tiled_cursor_drawn_x < tiled->width && y + ctx_tiled_cursor_drawn_y < tiled->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, ctx_tiled_cursor_drawn_shape))
          {
            int o = ((ctx_tiled_cursor_drawn_y + y) * tiled->width + (ctx_tiled_cursor_drawn_x + x)) * 4;
            tiled->fb[o+0]^=0x88;
            tiled->fb[o+1]^=0x88;
            tiled->fb[o+2]^=0x88;
          }
        }
      }

    ctx_tiled_cursor_drawn = 0;
    }
}

static inline void ctx_tiled_draw_cursor (CtxTiled *tiled)
{
    int cursor_x = (int)ctx_pointer_x (tiled->backend.ctx);
    int cursor_y = (int)ctx_pointer_y (tiled->backend.ctx);
    int cursor_size = ctx_height (tiled->backend.ctx) / 28;
    CtxCursor cursor_shape = tiled->backend.ctx->cursor;
    int no = 0;

    if (cursor_x == ctx_tiled_cursor_drawn_x &&
        cursor_y == ctx_tiled_cursor_drawn_y &&
        cursor_shape == ctx_tiled_cursor_drawn_shape)
      ctx_tiled_cursor_same_pos ++;
    else
      ctx_tiled_cursor_same_pos = 0;

    if (ctx_tiled_cursor_same_pos >= CTX_FB_HIDE_CURSOR_FRAMES)
    {
      if (ctx_tiled_cursor_drawn)
        ctx_tiled_undraw_cursor (tiled);
      return;
    }

    /* no need to flicker when stationary, motion flicker can also be removed
     * by combining the previous and next position masks when a motion has
     * occured..
     */
    if (ctx_tiled_cursor_same_pos && ctx_tiled_cursor_drawn)
      return;

    ctx_tiled_undraw_cursor (tiled);

    no = 0;

    int startx = -cursor_size;
    int starty = -cursor_size;

    if (cursor_shape == CTX_CURSOR_ARROW)
      startx = starty = 0;

    for (int y = starty; y < cursor_size; y++)
      for (int x = startx; x < cursor_size; x++, no+=4)
      {
        if (x + cursor_x < tiled->width && y + cursor_y < tiled->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, cursor_shape))
          {
            int o = ((cursor_y + y) * tiled->width + (cursor_x + x)) * 4;
            tiled->fb[o+0]^=0x88;
            tiled->fb[o+1]^=0x88;
            tiled->fb[o+2]^=0x88;
          }
        }
      }
    ctx_tiled_cursor_drawn = 1;
    ctx_tiled_cursor_drawn_x = cursor_x;
    ctx_tiled_cursor_drawn_y = cursor_y;
    ctx_tiled_cursor_drawn_shape = cursor_shape;
}

#endif




