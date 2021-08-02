#include "ctx-split.h"
#if CTX_RASTERIZER

static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer);

static void
_ctx_setup_compositor (CtxRasterizer *rasterizer)
{
  if (CTX_LIKELY (rasterizer->comp_op))
    return;
  rasterizer->format->setup (rasterizer);
#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
  {
    case CTX_SOURCE_LINEAR_GRADIENT:
    case CTX_SOURCE_RADIAL_GRADIENT:
      ctx_gradient_cache_prime (rasterizer);
      break;
    case CTX_SOURCE_TEXTURE:
      if (!rasterizer->state->gstate.source_fill.texture.buffer->color_managed)
        _ctx_texture_prepare_color_management (rasterizer,
        rasterizer->state->gstate.source_fill.texture.buffer);
      break;
  }
#endif
#endif
}

#define CTX_FULL_AA 15
inline static void
ctx_rasterizer_apply_coverage (CtxRasterizer *rasterizer,
                               uint8_t * dst,
                               int       x,
                               uint8_t * coverage,
                               int       count)
{
  if (CTX_UNLIKELY(rasterizer->format->apply_coverage))
    rasterizer->format->apply_coverage(rasterizer, dst, rasterizer->color, x, coverage, count);
  else
    /* it is faster to dispatch in this condition, than using a shared
     * direct trampoline
     */
    rasterizer->comp_op (rasterizer, dst, rasterizer->color, x, coverage, count);
}

CTX_STATIC void
ctx_rasterizer_gradient_add_stop (CtxRasterizer *rasterizer, float pos, float *rgba)
{
  /* FIXME XXX we only have one gradient, but might need separate gradients
   * for fill/stroke !
   * 
   */
  CtxGradient *gradient = &rasterizer->state->gradient;
  CtxGradientStop *stop = &gradient->stops[gradient->n_stops];
  stop->pos = pos;
  ctx_color_set_rgba (rasterizer->state, & (stop->color), rgba[0], rgba[1], rgba[2], rgba[3]);
  if (gradient->n_stops < 15) //we'll keep overwriting the last when out of stops
    { gradient->n_stops++; }
}

static inline int ctx_rasterizer_add_point (CtxRasterizer *rasterizer, int x1, int y1)
{
  CtxEntry entry = {CTX_EDGE, {{0},}};
  if (CTX_UNLIKELY(y1 < rasterizer->scan_min))
    { rasterizer->scan_min = y1; }
  if (CTX_UNLIKELY(y1 > rasterizer->scan_max))
    { rasterizer->scan_max = y1; }
  if (CTX_UNLIKELY(x1 < rasterizer->col_min))
    { rasterizer->col_min = x1; }
  if (CTX_UNLIKELY(x1 > rasterizer->col_max))
    { rasterizer->col_max = x1; }

  entry.data.s16[2]=x1;
  entry.data.s16[3]=y1;
  return ctx_drawlist_add_single (&rasterizer->edge_list, &entry);
}

#if 0
#define CTX_SHAPE_CACHE_PRIME1   7853
#define CTX_SHAPE_CACHE_PRIME2   4129
#define CTX_SHAPE_CACHE_PRIME3   3371
#define CTX_SHAPE_CACHE_PRIME4   4221
#else
#define CTX_SHAPE_CACHE_PRIME1   283
#define CTX_SHAPE_CACHE_PRIME2   599
#define CTX_SHAPE_CACHE_PRIME3   101
#define CTX_SHAPE_CACHE_PRIME4   661
#endif

float ctx_shape_cache_rate = 0.0;
#if CTX_SHAPE_CACHE
int   _ctx_shape_cache_enabled = 1;

//static CtxShapeCache ctx_cache = {{NULL,}, 0};

static long ctx_shape_cache_hits = 0;
static long ctx_shape_cache_misses = 0;


/* this returns the buffer to use for rendering, it always
   succeeds..
 */
static inline CtxShapeEntry *ctx_shape_entry_find (CtxRasterizer *rasterizer, uint32_t hash, int width, int height)
{
  /* use both some high and some low bits  */
  int entry_no = ( (hash >> 10) ^ (hash & 1023) ) % CTX_SHAPE_CACHE_ENTRIES;
  int i;
  {
    static int i = 0;
    i++;
    if (i>1000)
      {
        ctx_shape_cache_rate = ctx_shape_cache_hits * 100.0  / (ctx_shape_cache_hits+ctx_shape_cache_misses);
        i = 0;
        ctx_shape_cache_hits = 0;
        ctx_shape_cache_misses = 0;
      }
  }
// XXX : this 1 one is needed  to silence a false positive:
// ==90718== Invalid write of size 1
// ==90718==    at 0x1189EF: ctx_rasterizer_generate_coverage (ctx.h:4786)
// ==90718==    by 0x118E57: ctx_rasterizer_rasterize_edges (ctx.h:4907)
//
  int size = sizeof (CtxShapeEntry) + width * height + 1;

  i = entry_no;
  if (rasterizer->shape_cache.entries[i])
    {
      CtxShapeEntry *entry = rasterizer->shape_cache.entries[i];
      int old_size = sizeof (CtxShapeEntry) + width + height + 1;
      if (entry->hash == hash &&
          entry->width == width &&
          entry->height == height)
        {
          if (entry->uses < 1<<30)
            { entry->uses++; }
          ctx_shape_cache_hits ++;
          return entry;
        }

      if (old_size >= size)
      {
      }
      else
      {
        rasterizer->shape_cache.entries[i] = NULL;
        rasterizer->shape_cache.size -= entry->width * entry->height;
        rasterizer->shape_cache.size -= sizeof (CtxShapeEntry);
        free (entry);
        rasterizer->shape_cache.entries[i] = (CtxShapeEntry *) calloc (size, 1);
      }
    }
  else
    {
        rasterizer->shape_cache.entries[i] = (CtxShapeEntry *) calloc (size, 1);
    }

  ctx_shape_cache_misses ++;
  rasterizer->shape_cache.size              += size;
  rasterizer->shape_cache.entries[i]->hash   = hash;
  rasterizer->shape_cache.entries[i]->width  = width;
  rasterizer->shape_cache.entries[i]->height = height;
  rasterizer->shape_cache.entries[i]->uses = 0;
  return rasterizer->shape_cache.entries[i];
}

#endif

CTX_STATIC uint32_t ctx_rasterizer_poly_to_hash (CtxRasterizer *rasterizer)
{
  int16_t x = 0;
  int16_t y = 0;

  CtxEntry *entry = &rasterizer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = ox;//(ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += oy; //(oy % CTX_RASTERIZER_AA);
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      x = entry->data.s16[2];
      y = entry->data.s16[3];
      int dx = x-ox;
      int dy = y-oy;
      ox = x;
      oy = y;
      hash *= CTX_SHAPE_CACHE_PRIME3;
      hash += dx;
      hash *= CTX_SHAPE_CACHE_PRIME4;
      hash += dy;
    }
  return hash;
}

static uint32_t ctx_rasterizer_poly_to_edges (CtxRasterizer *rasterizer)
{
  int16_t x = 0;
  int16_t y = 0;
  if (rasterizer->edge_list.count == 0)
     return 0;
#if CTX_SHAPE_CACHE
  CtxEntry *entry = &rasterizer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = (ox & CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += (oy & CTX_SUBDIV);
#endif
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      if (entry->code == CTX_NEW_EDGE)
        {
          entry->code = CTX_EDGE;
#if CTX_SHAPE_CACHE
          hash *= CTX_SHAPE_CACHE_PRIME2;
#endif
        }
      else
        {
          entry->data.s16[0] = x;
          entry->data.s16[1] = y;
        }
      x = entry->data.s16[2];
      y = entry->data.s16[3];
#if CTX_SHAPE_CACHE
      int dx = x-ox;
      int dy = y-oy;
      ox = x;
      oy = y;
      hash *= CTX_SHAPE_CACHE_PRIME3;
      hash += dx;
      hash *= CTX_SHAPE_CACHE_PRIME4;
      hash += dy;
#endif
      if (entry->data.s16[3] < entry->data.s16[1])
        {
          *entry = ctx_s16 (CTX_EDGE_FLIPPED,
                            entry->data.s16[2], entry->data.s16[3],
                            entry->data.s16[0], entry->data.s16[1]);
        }
    }
#if CTX_SHAPE_CACHE
  return hash;
#else
  return 0;
#endif
}

CTX_STATIC void ctx_rasterizer_finish_shape (CtxRasterizer *rasterizer)
{
  if (rasterizer->has_shape && rasterizer->has_prev)
    {
      ctx_rasterizer_line_to (rasterizer, rasterizer->first_x, rasterizer->first_y);
      rasterizer->has_prev = 0;
    }
}

CTX_STATIC void ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx = x; float ty = y;
  int aa = 15;//rasterizer->aa;
  rasterizer->x        = x;
  rasterizer->y        = y;
  rasterizer->first_x  = x;
  rasterizer->first_y  = y;
  rasterizer->has_prev = -1;
  if (CTX_UNLIKELY(rasterizer->uses_transforms))
    {
      _ctx_user_to_device (rasterizer->state, &tx, &ty);
    }

  tx = (tx - rasterizer->blit_x) * CTX_SUBDIV;
  ty = ty * aa;

  if (CTX_UNLIKELY(ty < rasterizer->scan_min))
    { rasterizer->scan_min = ty; }
  if (CTX_UNLIKELY(ty > rasterizer->scan_max))
    { rasterizer->scan_max = ty; }
  if (CTX_UNLIKELY(tx < rasterizer->col_min))
    { rasterizer->col_min = tx; }
  if (CTX_UNLIKELY(tx > rasterizer->col_max))
    { rasterizer->col_max = tx; }
}



static inline void ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx = x;
  float ty = y;
  float ox = rasterizer->x;
  float oy = rasterizer->y;
  if (CTX_UNLIKELY(rasterizer->uses_transforms))
    {
      _ctx_user_to_device (rasterizer->state, &tx, &ty);
    }
  tx -= rasterizer->blit_x;
#define MIN_Y -1000
#define MAX_Y 1400

  if (CTX_UNLIKELY(ty < MIN_Y)) ty = MIN_Y;
  if (CTX_UNLIKELY(ty > MAX_Y)) ty = MAX_Y;
  
  ctx_rasterizer_add_point (rasterizer, tx * CTX_SUBDIV, ty * 15);//rasterizer->aa);

  if (rasterizer->has_prev<=0)
    {
      if (CTX_UNLIKELY(rasterizer->uses_transforms))
      {
        // storing transformed would save some processing for a tiny
        // amount of runtime RAM XXX
        _ctx_user_to_device (rasterizer->state, &ox, &oy);
      }
      ox -= rasterizer->blit_x;

      if (CTX_UNLIKELY(oy < MIN_Y)) oy = MIN_Y;
      if (CTX_UNLIKELY(oy > MAX_Y)) oy = MAX_Y;

      CtxEntry *entry = &rasterizer->edge_list.entries[rasterizer->edge_list.count-1];
      entry->data.s16[0] = ox * CTX_SUBDIV;
      entry->data.s16[1] = oy * 15;
      entry->code = CTX_NEW_EDGE;
      rasterizer->has_prev = 1;
    }
  rasterizer->has_shape = 1;
  rasterizer->y         = y;
  rasterizer->x         = x;
}


CTX_INLINE static float
ctx_bezier_sample_1d (float x0, float x1, float x2, float x3, float dt)
{
  float ab   = ctx_lerpf (x0, x1, dt);
  float bc   = ctx_lerpf (x1, x2, dt);
  float cd   = ctx_lerpf (x2, x3, dt);
  float abbc = ctx_lerpf (ab, bc, dt);
  float bccd = ctx_lerpf (bc, cd, dt);
  return ctx_lerpf (abbc, bccd, dt);
}

inline static void
ctx_bezier_sample (float x0, float y0,
                   float x1, float y1,
                   float x2, float y2,
                   float x3, float y3,
                   float dt, float *x, float *y)
{
  *x = ctx_bezier_sample_1d (x0, x1, x2, x3, dt);
  *y = ctx_bezier_sample_1d (y0, y1, y2, y3, dt);
}

static inline void
ctx_rasterizer_bezier_divide (CtxRasterizer *rasterizer,
                              float ox, float oy,
                              float x0, float y0,
                              float x1, float y1,
                              float x2, float y2,
                              float sx, float sy,
                              float ex, float ey,
                              float s,
                              float e,
                              int   iteration,
                              float tolerance)
{
  if (CTX_UNLIKELY(iteration > 8))
    { return; }
  float t = (s + e) * 0.5f;
  float x, y, lx, ly, dx, dy;
  ctx_bezier_sample (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);
  if (CTX_LIKELY(iteration))
    {
      lx = ctx_lerpf (sx, ex, t);
      ly = ctx_lerpf (sy, ey, t);
      dx = lx - x;
      dy = ly - y;
      if (CTX_UNLIKELY( (dx*dx+dy*dy) < tolerance))
        /* bailing - because for the mid-point straight line difference is
           tiny */
        { return; }
      dx = sx - ex;
      dy = ey - ey;
      if (CTX_UNLIKELY( (dx*dx+dy*dy) < tolerance))
        /* bailing on tiny segments */
        { return; }
    }
  ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                sx, sy, x, y, s, t, iteration + 1,
                                tolerance);
  ctx_rasterizer_line_to (rasterizer, x, y);
  ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                x, y, ex, ey, t, e, iteration + 1,
                                tolerance);
}

CTX_STATIC void
ctx_rasterizer_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2)
{
  float tolerance =
    ctx_pow2 (rasterizer->state->gstate.transform.m[0][0]) +
    ctx_pow2 (rasterizer->state->gstate.transform.m[1][1]);
  float ox = rasterizer->x;
  float oy = rasterizer->y;
  ox = rasterizer->state->x;
  oy = rasterizer->state->y;
  tolerance = 1.0f/tolerance * 2;
#if 1 // skipping this to preserve hash integrity
  if (tolerance == 1.0f || 1)
  {
  float maxx = ctx_maxf (x1,x2);
  maxx = ctx_maxf (maxx, ox);
  maxx = ctx_maxf (maxx, x0);
  float maxy = ctx_maxf (y1,y2);
  maxy = ctx_maxf (maxy, oy);
  maxy = ctx_maxf (maxy, y0);
  float minx = ctx_minf (x1,x2);
  minx = ctx_minf (minx, ox);
  minx = ctx_minf (minx, x0);
  float miny = ctx_minf (y1,y2);
  miny = ctx_minf (miny, oy);
  miny = ctx_minf (miny, y0);
  
  _ctx_user_to_device (rasterizer->state, &minx, &miny);
  _ctx_user_to_device (rasterizer->state, &maxx, &maxy);
#if 1
    if(
        (minx > rasterizer->blit_x + rasterizer->blit_width) ||
        (miny > rasterizer->blit_y + rasterizer->blit_height) ||
        (maxx < rasterizer->blit_x) ||
        (maxy < rasterizer->blit_y) )
    {
    }
    else
#endif
    {
      ctx_rasterizer_bezier_divide (rasterizer,
                                    ox, oy, x0, y0,
                                    x1, y1, x2, y2,
                                    ox, oy, x2, y2,
                                    0.0f, 1.0f, 0.0f, tolerance);
    }
  }
  else
#endif
    {
      ctx_rasterizer_bezier_divide (rasterizer,
                                    ox, oy, x0, y0,
                                    x1, y1, x2, y2,
                                    ox, oy, x2, y2,
                                    0.0f, 1.0f, 0.0f, tolerance);
    }
  ctx_rasterizer_line_to (rasterizer, x2, y2);
}

CTX_STATIC void
ctx_rasterizer_rel_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  if (CTX_UNLIKELY(x == 0.f && y == 0.f))
    { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_move_to (rasterizer, x, y);
}

CTX_STATIC void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  if (CTX_UNLIKELY(x== 0.f && y==0.f))
    { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_line_to (rasterizer, x, y);
}

CTX_STATIC void
ctx_rasterizer_rel_curve_to (CtxRasterizer *rasterizer,
                             float x0, float y0, float x1, float y1, float x2, float y2)
{
  x0 += rasterizer->x;
  y0 += rasterizer->y;
  x1 += rasterizer->x;
  y1 += rasterizer->y;
  x2 += rasterizer->x;
  y2 += rasterizer->y;
  ctx_rasterizer_curve_to (rasterizer, x0, y0, x1, y1, x2, y2);
}


static int
ctx_rasterizer_find_texture (CtxRasterizer *rasterizer,
                             const char *eid)
{
  int no;
  for (no = 0; no < CTX_MAX_TEXTURES; no++)
  {
    if (rasterizer->texture_source->texture[no].data &&
        rasterizer->texture_source->texture[no].eid &&
        !strcmp (rasterizer->texture_source->texture[no].eid, eid))
      return no;
  }
  return -1;
}

static void
ctx_rasterizer_set_texture (CtxRasterizer *rasterizer,
                            const char *eid,
                            float x,
                            float y)
{
  int is_stroke = (rasterizer->state->source != 0);
  CtxSource *source = is_stroke && (rasterizer->state->gstate.source_stroke.type != CTX_SOURCE_INHERIT_FILL)?
                        &rasterizer->state->gstate.source_stroke:
                        &rasterizer->state->gstate.source_fill;
  rasterizer->state->source = 0;

  int no = ctx_rasterizer_find_texture (rasterizer, eid);
  if (no < 0 || no >= CTX_MAX_TEXTURES) { no = 0; }
  if (rasterizer->texture_source->texture[no].data == NULL)
    {
      fprintf (stderr, "ctx tex fail %p %s %i\n", rasterizer->texture_source, eid, no);
      return;
    }
  else
  {
    rasterizer->texture_source->texture[no].frame = rasterizer->texture_source->frame;
  }
  source->type = CTX_SOURCE_TEXTURE;
  source->texture.buffer = &rasterizer->texture_source->texture[no];
  source->texture.x0 = 0;
  source->texture.y0 = 0;
  source->transform = rasterizer->state->gstate.transform;
  ctx_matrix_translate (&source->transform, x, y);
  ctx_matrix_invert (&source->transform);
}


static void ctx_rasterizer_define_texture (CtxRasterizer *rasterizer,
                                           const char *eid,
                                           int width,
                                           int height,
                                           int format,
                                           char unsigned *data)
{
  _ctx_texture_lock (); // we're using the same texture_source from all threads, keeping allocaitons down
                        // need synchronizing (it could be better to do a pre-pass)
  ctx_texture_init (rasterizer->texture_source,
                    eid,
                    width,
                    height,
                    ctx_pixel_format_get_stride ((CtxPixelFormat)format, width),
                    (CtxPixelFormat)format,
#if CTX_ENABLE_CM
                    (void*)rasterizer->state->gstate.texture_space,
#else
                    NULL,
#endif
                    data,
                    ctx_buffer_pixels_free, (void*)23);
                    /*  when userdata for ctx_buffer_pixels_free is 23, texture_init dups the data on
                     *  use
                     */

  ctx_rasterizer_set_texture (rasterizer, eid, 0.0, 0.0);
  _ctx_texture_unlock ();
}


CTX_INLINE static int ctx_compare_edges (const void *ap, const void *bp)
{
  const CtxEntry *a = (const CtxEntry *) ap;
  const CtxEntry *b = (const CtxEntry *) bp;
  int ycompare = a->data.s16[1] - b->data.s16[1];
  if (ycompare)
    { return ycompare; }
  int xcompare = a->data.s16[0] - b->data.s16[0];
  return xcompare;
}

CTX_INLINE static int ctx_edge_qsort_partition (CtxEntry *A, int low, int high)
{
  CtxEntry pivot = A[ (high+low) /2];
  int i = low;
  int j = high;
  while (i <= j)
    {
      while (ctx_compare_edges (&A[i], &pivot) <0) { i ++; }
      while (ctx_compare_edges (&pivot, &A[j]) <0) { j --; }
      if (i <= j)
        {
          CtxEntry tmp = A[i];
          A[i] = A[j];
          A[j] = tmp;
          i++;
          j--;
        }
    }
  return i;
}

static inline void ctx_edge_qsort (CtxEntry *entries, int low, int high)
{
  {
    int p = ctx_edge_qsort_partition (entries, low, high);
    if (low < p -1 )
      { ctx_edge_qsort (entries, low, p - 1); }
    if (low < high)
      { ctx_edge_qsort (entries, p, high); }
  }
}

static CTX_INLINE void ctx_rasterizer_sort_edges (CtxRasterizer *rasterizer)
{
  ctx_edge_qsort (& (rasterizer->edge_list.entries[0]), 0, rasterizer->edge_list.count-1);
}


static inline void ctx_rasterizer_discard_edges (CtxRasterizer *rasterizer)
{
  int scanline = rasterizer->scanline;
  rasterizer->ending_edges = 0;
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      int edge_end =rasterizer->edge_list.entries[rasterizer->edges[i].index].data.s16[3]-1;
      if (CTX_UNLIKELY(edge_end < scanline))
        {
          int dx_dy = abs(rasterizer->edges[i].delta);
          if (CTX_UNLIKELY(dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT3))
            { rasterizer->needs_aa3 --;
              if (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT5)
              { rasterizer->needs_aa5 --;
                if (CTX_UNLIKELY(dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT15))
                  {
                    rasterizer->needs_aa15 --;
                  }
              }
            }
          rasterizer->edges[i] = rasterizer->edges[rasterizer->active_edges-1];
          rasterizer->active_edges--;
          i--;
        }
      else if (CTX_UNLIKELY(edge_end < scanline + CTX_FULL_AA))
        rasterizer->ending_edges = 1; // only used as a flag!
    }
}

inline static void ctx_rasterizer_increment_edges (CtxRasterizer *rasterizer, int count)
{
  rasterizer->scanline += count;
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      rasterizer->edges[i].val += rasterizer->edges[i].delta * count;
    }
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      rasterizer->edges[CTX_MAX_EDGES-1-i].val += rasterizer->edges[CTX_MAX_EDGES-1-i].delta * count;
    }
}

/* feeds up to rasterizer->scanline,
   keeps a pending buffer of edges - that encompass
   the full incoming scanline,
   feed until the start of the scanline and check for need for aa
   in all of pending + active edges, then
   again feed_edges until middle of scanline if doing non-AA
   or directly render when doing AA
*/
inline static void ctx_rasterizer_feed_edges (CtxRasterizer *rasterizer)
{
  int miny;
  CtxEntry *entries = rasterizer->edge_list.entries;
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      if (entries[rasterizer->edges[CTX_MAX_EDGES-1-i].index].data.s16[1] - 1 <= rasterizer->scanline)
        {
          if (CTX_LIKELY(rasterizer->active_edges < CTX_MAX_EDGES-2))
            {
              int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              rasterizer->edges[no] = rasterizer->edges[CTX_MAX_EDGES-1-i];
              rasterizer->edges[CTX_MAX_EDGES-1-i] =
                rasterizer->edges[CTX_MAX_EDGES-1-rasterizer->pending_edges + 1];
              rasterizer->pending_edges--;
              i--;
            }
        }
    }
  int scanline = rasterizer->scanline;
  while (CTX_LIKELY(rasterizer->edge_pos < rasterizer->edge_list.count &&
         (miny=entries[rasterizer->edge_pos].data.s16[1]-1)  <= scanline + 15))
    {
      int maxy=entries[rasterizer->edge_pos].data.s16[3]-1;
      if (CTX_LIKELY(rasterizer->active_edges < CTX_MAX_EDGES-2) &&
          maxy >= scanline)
        {
          int dy = (entries[rasterizer->edge_pos].data.s16[3] - 1 - miny);
          if (dy) /* skipping horizontal edges */
            {
              int yd = scanline - miny;
              int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              rasterizer->edges[no].index = rasterizer->edge_pos;
              int index = rasterizer->edges[no].index;
              int x0 = entries[index].data.s16[0];
              int x1 = entries[index].data.s16[2];
              rasterizer->edges[no].val = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER;
              int dx_dy;
              dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
              rasterizer->edges[no].delta = dx_dy;
              rasterizer->edges[no].val += (yd * dx_dy);

              {
                int abs_dx_dy = abs(dx_dy);
                if (abs_dx_dy> CTX_RASTERIZER_AA_SLOPE_LIMIT3)
                  { rasterizer->needs_aa3 ++;
                    if (abs_dx_dy> CTX_RASTERIZER_AA_SLOPE_LIMIT5)
                    { rasterizer->needs_aa5 ++; 
                      if (CTX_UNLIKELY(abs_dx_dy> CTX_RASTERIZER_AA_SLOPE_LIMIT15))
                      { rasterizer->needs_aa15 ++;
                      }
                    }
                  }
              }

              if ((miny > scanline) )
                {
                  /* it is a pending edge - we add it to the end of the array
                     and keep a different count for items stored here, like
                     a heap and stack growing against each other
                  */
                  if (rasterizer->pending_edges < CTX_MAX_PENDING-1)
                  {
                    rasterizer->edges[CTX_MAX_EDGES-1-rasterizer->pending_edges] =
                    rasterizer->edges[no];
                    rasterizer->pending_edges++;
                    rasterizer->active_edges--;
                  }
                }
            }
        }
      rasterizer->edge_pos++;
    }
    ctx_rasterizer_discard_edges (rasterizer);
}

CTX_INLINE static int ctx_compare_edges2 (const void *ap, const void *bp)
{
  const CtxEdge *a = (const CtxEdge *) ap;
  const CtxEdge *b = (const CtxEdge *) bp;
  return a->val - b->val;
}

CTX_INLINE static void ctx_edge2_insertion_sort (CtxEdge *entries, int count)
{
  for(int i=1; i<count; i++)
   {
     CtxEdge temp = entries[i];
     int j = i-1;
     while (j >= 0 && ctx_compare_edges2 (&temp, &entries[j])<0)
     {
       entries[j+1] = entries[j];
       j--;
     }
     entries[j+1] = temp;
   }
}

static inline void ctx_rasterizer_sort_active_edges (CtxRasterizer *rasterizer)
{
  ctx_edge2_insertion_sort (rasterizer->edges, rasterizer->active_edges);
}

#undef CTX_CMPSWP

static inline void ctx_coverage_post_process (CtxRasterizer *rasterizer, int minx, int maxx, uint8_t *coverage, int *first_col, int *last_col)
{
  int scanline     = rasterizer->scanline;
#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
    float radius = rasterizer->state->gstate.shadow_blur;
    int dim = 2 * radius + 1;
    if (CTX_UNLIKELY (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM))
      dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
    {
      uint16_t temp[maxx-minx+1];
      memset (temp, 0, sizeof (temp));
      for (int x = dim/2; x < maxx-minx + 1 - dim/2; x ++)
        for (int u = 0; u < dim; u ++)
        {
            temp[x] += coverage[minx+x+u-dim/2] * rasterizer->kernel[u] * 256;
        }
      for (int x = 0; x < maxx-minx + 1; x ++)
        coverage[minx+x] = temp[x] >> 8;
    }
  }
#endif

#if CTX_ENABLE_CLIP
  if (CTX_UNLIKELY(rasterizer->clip_buffer &&  !rasterizer->clip_rectangle))
  {
    /* perhaps not working right for clear? */
    int y = scanline / 15;//rasterizer->aa;
    uint8_t *clip_line = &((uint8_t*)(rasterizer->clip_buffer->data))[rasterizer->blit_width*y];
    // XXX SIMD candidate
    for (int x = minx; x <= maxx; x ++)
    {
#if CTX_1BIT_CLIP
        coverage[x] = (coverage[x] * ((clip_line[x/8]&(1<<(x&8)))?255:0))/255;
#else
        coverage[x] = (coverage[x] * clip_line[x])/255;
#endif
    }
  }
  if (CTX_UNLIKELY(rasterizer->aa == 1))
  {
    for (int x = minx; x <= maxx; x ++)
     coverage[x] = coverage[x] > 127?255:0;
  }
  if (CTX_UNLIKELY(rasterizer->in_line_stroke == 1))
  {
    int first = minx;
    int last = maxx;
    for (int x = minx; (x <= maxx) && (coverage[x] == 0); x ++)
      first = x;
    for (int x = maxx; x >= first && coverage[x] == 0; x --)
      last = x;
    if (first == last)
       last ++;
    if (last > maxx)last = maxx;
    *first_col = first; 
    *last_col = last; 
  }
#endif
}

inline static void
ctx_rasterizer_generate_coverage (CtxRasterizer *rasterizer,
                                  int            minx,
                                  int            maxx,
                                  uint8_t       *coverage,
                                  int            winding,
                                  int            aa_factor)
{
  CtxEntry *entries = rasterizer->edge_list.entries;;
  CtxEdge  *edges = rasterizer->edges;
  int scanline     = rasterizer->scanline;
  int active_edges = rasterizer->active_edges;
  int parity = 0;
  int fraction = 255/aa_factor;
  coverage -= minx;
#define CTX_EDGE(no)      entries[edges[no].index]
#define CTX_EDGE_YMIN(no) (CTX_EDGE(no).data.s16[1]-1)
#define CTX_EDGE_X(no)    (edges[no].val)
  for (int t = 0; t < active_edges -1;t++)
    {
      int ymin = CTX_EDGE_YMIN (t);
      if (scanline != ymin)
        {
          if (!winding)
            { parity = 1 - parity; }
          else
            { parity += ( (CTX_EDGE (t).code == CTX_EDGE_FLIPPED) ?1:-1); }
        }

       if (parity)
        {
          int x0 = CTX_EDGE_X (t);
          int x1 = CTX_EDGE_X (t+1);
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (CTX_UNLIKELY(first < minx))
          { 
            first = minx;
            graystart=0;//255;
          }
          if (CTX_UNLIKELY(last > maxx))
          {
            last = maxx;
            grayend=255;
          }

          graystart=fraction- (graystart&0xff)/aa_factor;
          grayend = (grayend & 0xff) / aa_factor;

          if (first < last)
          {
              coverage[first] += graystart;
              for (int x = first + 1; x < last; x++)
                coverage[x] += fraction;
              coverage[last]  += grayend;
          }
          else if (first == last)
            coverage[first] += (graystart-(fraction-grayend));
        }
   }
}

inline static void
ctx_rasterizer_generate_coverage_set (CtxRasterizer *rasterizer,
                                  int            minx,
                                  int            maxx,
                                  uint8_t       *coverage,
                                  int            winding,
                                  int            aa_factor)
{
  CtxEntry *entries = rasterizer->edge_list.entries;;
  CtxEdge  *edges = rasterizer->edges;
  int scanline     = rasterizer->scanline;
  int active_edges = rasterizer->active_edges;
  int parity = 0;
  int fraction = 255/aa_factor;
  coverage -= minx;
#define CTX_EDGE(no)      entries[edges[no].index]
#define CTX_EDGE_YMIN(no) (CTX_EDGE(no).data.s16[1]-1)
#define CTX_EDGE_X(no)    (edges[no].val)
  for (int t = 0; t < active_edges -1;t++)
    {
      int ymin = CTX_EDGE_YMIN (t);
      if (scanline != ymin)
        {
          if (!winding)
            { parity = 1 - parity; }
          else
            { parity += ( (CTX_EDGE (t).code == CTX_EDGE_FLIPPED) ?1:-1); }
        }

       if (parity)
        {
          int x0 = CTX_EDGE_X (t);
          int x1 = CTX_EDGE_X (t+1);
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (CTX_UNLIKELY(first < minx))
          { 
            first = minx;
            graystart=0;
          }
          if (CTX_UNLIKELY(last > maxx))
          {
            last = maxx;
            grayend=255;
          }

          graystart=fraction - (graystart&0xff)/aa_factor;
          grayend = (grayend & 0xff) / aa_factor;

          if (first < last)
          {
              coverage[first] += graystart;
              for (int x = first + 1; x < last; x++)
                coverage[x] = fraction;
              coverage[last]  += grayend;
          }
          else if (CTX_UNLIKELY(first == last))
            coverage[first] += (graystart-(fraction-grayend));
        }
   }
}

#undef CTX_EDGE_Y0
#undef CTX_EDGE

CTX_STATIC void
ctx_rasterizer_reset (CtxRasterizer *rasterizer)
{
  rasterizer->pending_edges   = 0;
  rasterizer->active_edges    = 0;
  rasterizer->has_shape       = 0;
  rasterizer->has_prev        = 0;
  rasterizer->edge_list.count = 0; // ready for new edges
  rasterizer->edge_pos        = 0;
  rasterizer->needs_aa3       = 0;
  rasterizer->needs_aa5       = 0;
  rasterizer->needs_aa15      = 0;
  rasterizer->scanline        = 0;
  if (!rasterizer->preserve)
  {
    rasterizer->scan_min      = 5000;
    rasterizer->scan_max      = -5000;
    rasterizer->col_min       = 5000;
    rasterizer->col_max       = -5000;
  }
  //rasterizer->comp_op       = NULL; // keep comp_op cached 
  //     between rasterizations where rendering attributes are
  //     nonchanging
}

#define CTX_INLINE_FILL_RULE 1


static 
#if CTX_INLINE_FILL_RULE
inline 
#endif
        void
ctx_rasterizer_rasterize_edges (CtxRasterizer *rasterizer, int winding
#if CTX_SHAPE_CACHE
                                ,CtxShapeEntry *shape
#endif
                               )
{
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);

  int real_aa = rasterizer->aa;

  int scan_start = rasterizer->blit_y * CTX_FULL_AA;
  int scan_end   = scan_start + rasterizer->blit_height * CTX_FULL_AA;
  int blit_width = rasterizer->blit_width;
  int blit_max_x = rasterizer->blit_x + blit_width;
  int minx       = rasterizer->col_min / CTX_SUBDIV - rasterizer->blit_x;
  int maxx       = (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV - rasterizer->blit_x;

#if 1
  if (
#if CTX_SHAPE_CACHE
    !shape &&
#endif
    maxx > blit_max_x - 1)
    { maxx = blit_max_x - 1; }
#endif
#if 1
  if (rasterizer->state->gstate.clip_min_x>
      minx)
    { minx = rasterizer->state->gstate.clip_min_x; }
  if (rasterizer->state->gstate.clip_max_x <
      maxx)
    { maxx = rasterizer->state->gstate.clip_max_x; }
#endif
  if (minx < 0)
    { minx = 0; }
  if (minx >= maxx)
    {
      ctx_rasterizer_reset (rasterizer);
      return;
    }
#if CTX_SHAPE_CACHE
  uint8_t _coverage[shape?2:maxx-minx+1];
#else
  uint8_t _coverage[maxx-minx+1];
#endif
  uint8_t *coverage = &_coverage[0];


#if CTX_SHAPE_CACHE
  if (shape)
    {
      coverage = &shape->data[0];
    }
#endif
  ctx_assert (coverage);
  rasterizer->scan_min -= (rasterizer->scan_min % CTX_FULL_AA);
#if CTX_SHAPE_CACHE
  if (shape)
    {
      scan_start = rasterizer->scan_min;
      scan_end   = rasterizer->scan_max;
    }
  else
#endif
    {
      if (rasterizer->scan_min > scan_start)
        {
          dst += (rasterizer->blit_stride * (rasterizer->scan_min-scan_start) / CTX_FULL_AA);
          scan_start = rasterizer->scan_min;
        }
      if (rasterizer->scan_max < scan_end)
        { scan_end = rasterizer->scan_max; }
    }
  if (rasterizer->state->gstate.clip_min_y * CTX_FULL_AA > scan_start )
    { 
       dst += (rasterizer->blit_stride * (rasterizer->state->gstate.clip_min_y * CTX_FULL_AA -scan_start) / CTX_FULL_AA);
       scan_start = rasterizer->state->gstate.clip_min_y * CTX_FULL_AA; 
    }
  if (rasterizer->state->gstate.clip_max_y * CTX_FULL_AA < scan_end)
    { scan_end = rasterizer->state->gstate.clip_max_y * CTX_FULL_AA; }
  if (scan_start > scan_end ||
      (scan_start > (rasterizer->blit_y + rasterizer->blit_height) * CTX_FULL_AA) ||
      (scan_end < (rasterizer->blit_y) * CTX_FULL_AA))
  { 
    /* not affecting this rasterizers scanlines */
    ctx_rasterizer_reset (rasterizer);
    return;
  }

  ctx_rasterizer_sort_edges (rasterizer);
  {
    int halfstep2 = CTX_FULL_AA/2;
    int halfstep  = halfstep2 + 1;
    rasterizer->needs_aa3  = 0;
    rasterizer->needs_aa5  = 0;
    rasterizer->needs_aa15 = 0;
    rasterizer->scanline = scan_start;
    ctx_rasterizer_feed_edges (rasterizer); 

  for (; rasterizer->scanline <= scan_end;)
    {
      int contains_edge_end = rasterizer->pending_edges ||
                              rasterizer->ending_edges;

      // check if all pending edges are scanline aligned at start
      ctx_memset (coverage, 0,
#if CTX_SHAPE_CACHE
                  shape?shape->width:
#endif
                  sizeof (_coverage) );

#if 0
      if (1)
      {
      for (int i = 0; i < CTX_FULL_AA; i+=3)
      {
        ctx_rasterizer_feed_edges (rasterizer);
        ctx_rasterizer_sort_active_edges (rasterizer);
        ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, CTX_FULL_AA/3);
        ctx_rasterizer_increment_edges (rasterizer, 3);
      }
      }
      else 
#endif
    if (contains_edge_end)
    {
        for (int i = 0; i < real_aa; i++)
        {
          ctx_rasterizer_feed_edges (rasterizer);
          ctx_rasterizer_sort_active_edges (rasterizer);
          ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, real_aa);
          ctx_rasterizer_increment_edges (rasterizer, CTX_FULL_AA/real_aa);
        }
#define CTX_VIS_AA 0
#if CTX_VIS_AA
      for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 1.0;
#endif
    }
    else if (!rasterizer->needs_aa3) // if it doesnt need aa3 it doesnt need aa5 or aa15 either
    {
      ctx_rasterizer_increment_edges (rasterizer, halfstep2);
      ctx_rasterizer_feed_edges (rasterizer);

      ctx_rasterizer_sort_active_edges (rasterizer);
      ctx_rasterizer_generate_coverage_set (rasterizer, minx, maxx, coverage, winding, 1);
      ctx_rasterizer_increment_edges (rasterizer, halfstep);
#if CTX_VIS_AA
      for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 0.10;
#endif
    }
#if 1
    else if (rasterizer->needs_aa15)
      {
        for (int i = 0; i < CTX_FULL_AA; i++)
        {
          ctx_rasterizer_feed_edges (rasterizer);
          ctx_rasterizer_sort_active_edges (rasterizer);
          ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, CTX_FULL_AA);
          ctx_rasterizer_increment_edges (rasterizer, 1);
        }
#if CTX_VIS_AA
        for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 0.90;
#endif
    }
#endif
    else if (rasterizer->needs_aa5)
    {
      for (int i = 0; i < CTX_FULL_AA; i+=3)
      {
        ctx_rasterizer_feed_edges (rasterizer);
        ctx_rasterizer_sort_active_edges (rasterizer);
        ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, CTX_FULL_AA/3);
        ctx_rasterizer_increment_edges (rasterizer, 3);
      }
#if CTX_VIS_AA
        for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 0.5;
#endif
    }
    else if (rasterizer->needs_aa3)
    {
      for (int i = 0; i < CTX_FULL_AA; i+=5)
      {
        ctx_rasterizer_feed_edges (rasterizer);
        ctx_rasterizer_sort_active_edges (rasterizer);
        ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, CTX_FULL_AA/5);
        ctx_rasterizer_increment_edges (rasterizer, 5);
      }
#if CTX_VIS_AA
      for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 0.25;
#endif
    }
    else
    {
      ctx_rasterizer_increment_edges (rasterizer, halfstep2);
      ctx_rasterizer_feed_edges (rasterizer);

      ctx_rasterizer_sort_active_edges (rasterizer);
      ctx_rasterizer_generate_coverage_set (rasterizer, minx, maxx, coverage, winding, 1);
      ctx_rasterizer_increment_edges (rasterizer, halfstep);
#if CTX_VIS_AA
      for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 0.30;
#endif
    }
  int first_col = minx;
  int last_col = maxx;

  ctx_coverage_post_process (rasterizer, minx, maxx, coverage - minx,
                  &first_col, &last_col);
  if (CTX_LIKELY(rasterizer->in_line_stroke == 0))
  {
    first_col = minx;
    last_col = maxx;
  }
#if CTX_SHAPE_CACHE
  if (shape == NULL)
#endif
  {
    ctx_rasterizer_apply_coverage (rasterizer,
                         &dst[(first_col * rasterizer->format->bpp) /8],
                         first_col,
                         coverage + (first_col-minx),
                         last_col-first_col+ 1);
  }
#if CTX_SHAPE_CACHE
      if (shape)
        {
          coverage += shape->width;
        }
#endif
      dst += rasterizer->blit_stride;
    }
  }

  if (rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_SOURCE_OUT ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_SOURCE_IN ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_DESTINATION_IN ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_DESTINATION_ATOP ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_CLEAR)
  {
     /* fill in the rest of the blitrect when compositing mode permits it */
     uint8_t nocoverage[rasterizer->blit_width];
     //int gscan_start = rasterizer->state->gstate.clip_min_y * CTX_FULL_AA;
     int gscan_start = rasterizer->state->gstate.clip_min_y * CTX_FULL_AA;
     int gscan_end = rasterizer->state->gstate.clip_max_y * CTX_FULL_AA;
     memset (nocoverage, 0, sizeof(nocoverage));
     int startx   = rasterizer->state->gstate.clip_min_x;
     int endx     = rasterizer->state->gstate.clip_max_x;
     int clipw    = endx-startx + 1;
     uint8_t *dst = ( (uint8_t *) rasterizer->buf);

     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (gscan_start / CTX_FULL_AA);
     for (rasterizer->scanline = gscan_start; rasterizer->scanline < scan_start;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, clipw);
       rasterizer->scanline += CTX_FULL_AA;
       dst += rasterizer->blit_stride;
     }
     if (minx < startx)
     {
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_start / CTX_FULL_AA);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, minx-startx);
       dst += rasterizer->blit_stride;
     }
     }
     if (endx > maxx)
     {
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_start / CTX_FULL_AA);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (maxx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, endx-maxx);

       rasterizer->scanline += CTX_FULL_AA;
       dst += rasterizer->blit_stride;
     }
     }
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_end / CTX_FULL_AA);
     // XXX valgrind/asan this
     if(0)for (rasterizer->scanline = scan_end; rasterizer->scanline/CTX_FULL_AA < gscan_end-1;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, clipw-1);

       rasterizer->scanline += CTX_FULL_AA;
       dst += rasterizer->blit_stride;
     }
  }
  ctx_rasterizer_reset (rasterizer);
}

inline static int
ctx_is_transparent (CtxRasterizer *rasterizer, int stroke)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (gstate->global_alpha_u8 == 0)
    return 1;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
  {
    uint8_t ga[2];
    ctx_color_get_graya_u8 (rasterizer->state, &gstate->source_fill.color, ga);
    if (ga[1] == 0)
      return 1;
  }
  return 0;
}

#define CTX_RECT_FILL 1

#if CTX_RECT_FILL
static int
ctx_rasterizer_fill_rect (CtxRasterizer *rasterizer,
                          int          x0,
                          int          y0,
                          int          x1,
                          int          y1,
                          uint8_t      cov)
{
  if (CTX_UNLIKELY(x0>=x1)) {
          return 1;

     int tmp = x1;
     x1 = x0;
     x0 = tmp;
     tmp = y1;
     y1 = y0;
     y0 = tmp;
  }
#if 0
  if (x1 % CTX_SUBDIV ||
      x0 % CTX_SUBDIV ||
      y1 % CTX_FULL_AA ||
      y0 % CTX_FULL_AA)
    { return 0; }
#endif
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  _ctx_setup_compositor (rasterizer);
  if (CTX_UNLIKELY(x0 < rasterizer->blit_x))
    { x0 = rasterizer->blit_x; }
  if (CTX_UNLIKELY(y0 < rasterizer->blit_y))
    { y0 = rasterizer->blit_y; }
  if (CTX_UNLIKELY(y1 > rasterizer->blit_y + rasterizer->blit_height))
    { y1 = rasterizer->blit_y + rasterizer->blit_height; }
  if (CTX_UNLIKELY(x1 > rasterizer->blit_x + rasterizer->blit_width))
    { x1 = rasterizer->blit_x + rasterizer->blit_width; }
  dst += (y0 - rasterizer->blit_y) * rasterizer->blit_stride;
  int width = x1 - x0;

  if (CTX_LIKELY(width > 0))
    {
      if (rasterizer->format->components == 4 &&
                      cov == 255 &&
          rasterizer->state->gstate.source_fill.type == CTX_SOURCE_COLOR &&
          rasterizer->state->gstate.blend_mode == CTX_BLEND_NORMAL &&
          (
           (rasterizer->color[3]==255 &&
            rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_SOURCE_OVER) ||
          (rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_COPY)  )&&
          rasterizer->format->bpp == 32)
      {
        if (CTX_UNLIKELY(width == 1))
        {
          for (int y = y0; y < y1; y++)
          {
            memcpy (&dst[(x0)*4], rasterizer->color, 4);
            dst += rasterizer->blit_stride;
          }
        }
        else
        {
          uint8_t colorrow[4*(width+1)];
          for (int i = 0; i < width + 1; i++)
            memcpy(&colorrow[i*4], rasterizer->color, 4);
          for (int y = y0; y < y1; y++)
          {
            memcpy (&dst[(x0)*4], colorrow, 4 * (width));
            dst += rasterizer->blit_stride;
          }
        }
      }
      else
      {
        if (CTX_UNLIKELY(width == 1))
        {
          uint8_t coverage[1]={cov};
          for (int y = y0; y < y1; y++)
          {
            ctx_rasterizer_apply_coverage (rasterizer,
                                         &dst[ (x0) * rasterizer->format->bpp/8],
                                         x0,
                                         coverage, 1);
            dst += rasterizer->blit_stride;
          }
        }
        else
        {
        uint8_t coverage[x1-x0 + 1];
        ctx_memset (coverage, cov, sizeof (coverage) );
        rasterizer->scanline = y0 * CTX_FULL_AA;
        for (int y = y0; y < y1; y++)
        {
          rasterizer->scanline += CTX_FULL_AA;
          ctx_rasterizer_apply_coverage (rasterizer,
                                         &dst[ (x0) * rasterizer->format->bpp/8],
                                         x0,
                                         coverage, width);
          dst += rasterizer->blit_stride;
        }
        }
      }
    }
  return 1;
}
#endif

static inline float ctx_fmod1f (float val)
{
  int vali = val;
  return val - vali;
}

static void
ctx_rasterizer_fill (CtxRasterizer *rasterizer)
{
  int count = rasterizer->preserve?rasterizer->edge_list.count:0;

  CtxEntry temp[count]; /* copy of already built up path's poly line
                          XXX - by building a large enough path
                          the stack can be smashed!
                         */
  if (CTX_UNLIKELY(rasterizer->preserve))
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) ); }

#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      entry->data.s16[2] += rasterizer->shadow_x * CTX_SUBDIV;
      entry->data.s16[3] += rasterizer->shadow_y * CTX_FULL_AA;
    }
    rasterizer->scan_min += rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->scan_max += rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->col_min  += (rasterizer->shadow_x - rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
    rasterizer->col_max  += (rasterizer->shadow_x + rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
  }
#endif

  if (CTX_UNLIKELY(ctx_is_transparent (rasterizer, 0) ||
      rasterizer->scan_min / CTX_FULL_AA > rasterizer->blit_y + rasterizer->blit_height ||
      rasterizer->scan_max / CTX_FULL_AA < rasterizer->blit_y ||
      rasterizer->col_min / CTX_SUBDIV > rasterizer->blit_x + rasterizer->blit_width ||
      rasterizer->col_max / CTX_SUBDIV < rasterizer->blit_x))
    {
      ctx_rasterizer_reset (rasterizer);
    }
  else
  {
    _ctx_setup_compositor (rasterizer);

    if (CTX_UNLIKELY ( rasterizer->col_min / CTX_SUBDIV < rasterizer->state->min_x))
       rasterizer->state->min_x = rasterizer->col_min / CTX_SUBDIV;
    if (CTX_UNLIKELY ( rasterizer->col_min / CTX_SUBDIV > rasterizer->state->max_x))
       rasterizer->state->min_x = rasterizer->col_min / CTX_SUBDIV;

    if (CTX_UNLIKELY ( rasterizer->scan_min / CTX_FULL_AA < rasterizer->state->min_y))
       rasterizer->state->min_y = rasterizer->scan_min / CTX_FULL_AA;
    if (CTX_UNLIKELY ( rasterizer->scan_min / CTX_FULL_AA > rasterizer->state->max_y))
       rasterizer->state->max_y = rasterizer->scan_max / CTX_FULL_AA;

#if CTX_RECT_FILL
  if (rasterizer->edge_list.count == 6)
    {
      CtxEntry *entry0 = &rasterizer->edge_list.entries[0];
      CtxEntry *entry1 = &rasterizer->edge_list.entries[1];
      CtxEntry *entry2 = &rasterizer->edge_list.entries[2];
      CtxEntry *entry3 = &rasterizer->edge_list.entries[3];
      if (!rasterizer->state->gstate.clipped &&
          (entry0->data.s16[2] == entry1->data.s16[2]) &&
          (entry0->data.s16[3] == entry3->data.s16[3]) &&
          (entry1->data.s16[3] == entry2->data.s16[3]) &&
          (entry2->data.s16[2] == entry3->data.s16[2])
#if CTX_ENABLE_SHADOW_BLUR
           && !rasterizer->in_shadow
#endif
         )
       {
         if(((entry1->data.s16[2] % (CTX_SUBDIV))  == 0)  &&
            ((entry1->data.s16[3] % (CTX_FULL_AA)) == 0) &&
            ((entry3->data.s16[2] % (CTX_SUBDIV))  == 0)  &&
            ((entry3->data.s16[3] % (CTX_FULL_AA)) == 0))
         {
           /* best-case axis aligned rectangle */
           float x0 = entry3->data.s16[2] * 1.0f / CTX_SUBDIV;
           float y0 = entry3->data.s16[3] * 1.0f / CTX_FULL_AA;
           float x1 = entry1->data.s16[2] * 1.0f / CTX_SUBDIV;
           float y1 = entry1->data.s16[3] * 1.0f / CTX_FULL_AA;

           ctx_rasterizer_fill_rect (rasterizer, x0, y0, x1, y1, 255);
           ctx_rasterizer_reset (rasterizer);
           goto done;
         }
         else if (1)
         {
           float x0 = entry3->data.s16[2] * 1.0f / CTX_SUBDIV;
           float y0 = entry3->data.s16[3] * 1.0f / CTX_FULL_AA;
           float x1 = entry1->data.s16[2] * 1.0f / CTX_SUBDIV;
           float y1 = entry1->data.s16[3] * 1.0f / CTX_FULL_AA;

           if (CTX_UNLIKELY(x0 < rasterizer->blit_x))
             { x0 = rasterizer->blit_x; }
           if (CTX_UNLIKELY(y0 < rasterizer->blit_y))
             { y0 = rasterizer->blit_y; }
           if (CTX_UNLIKELY(y1 > rasterizer->blit_y + rasterizer->blit_height))
             { y1 = rasterizer->blit_y + rasterizer->blit_height; }
           if (CTX_UNLIKELY(x1 > rasterizer->blit_x + rasterizer->blit_width))
             { x1 = rasterizer->blit_x + rasterizer->blit_width; }

           uint8_t left = 255-ctx_fmod1f (x0) * 255;
           uint8_t top  = 255-ctx_fmod1f (y0) * 255;
           uint8_t right  = ctx_fmod1f (x1) * 255;
           uint8_t bottom = ctx_fmod1f (y1) * 255;

           x0 = ctx_floorf (x0);
           y0 = ctx_floorf (y0);
           x1 = ctx_floorf (x1+7/8.0f);
           y1 = ctx_floorf (y1+14/15.0f);

           int has_top    = (top < 255);
           int has_bottom = (bottom <255);
           int has_right  = (right >0);
           int has_left   = (left >0);

           int width = x1 - x0;

           if (CTX_LIKELY(width >0))
           {
              uint8_t *dst = ( (uint8_t *) rasterizer->buf);
              uint8_t coverage[width+2];
              dst += (((int)y0) - rasterizer->blit_y) * rasterizer->blit_stride;
              dst += ((int)x0) * rasterizer->format->bpp/8;

              if (has_top)
              {
                int i = 0;
                if (has_left)
                {
                  coverage[i++] = top * left / 255;
                }
                for (int x = x0 + has_left; x < x1 - has_right; x++)
                  coverage[i++] = top;
                coverage[i++]= top * right / 255;

                  ctx_rasterizer_apply_coverage (rasterizer,dst,
                                                 x0,
                                                 coverage, width);
                 dst += rasterizer->blit_stride;
               }

           if (!(rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_COPY &&
             rasterizer->state->gstate.blend_mode == CTX_BLEND_NORMAL &&
             rasterizer->state->gstate.source_fill.type == CTX_SOURCE_COLOR
             ))
           {
             int i = 0;
             if (has_left)
             {
               coverage[i++] = left;
             }
             for (int x = x0 + has_left; x < x1 - has_right; x++)
               coverage[i++] = 255;
             coverage[i++] = right;

             for (int ty = y0+has_top; ty < y1-has_bottom; ty++)
             {
               ctx_rasterizer_apply_coverage (rasterizer, dst, x0, coverage, width);
               dst += rasterizer->blit_stride;
             }
           }
           else
           {
             if (has_left)
               ctx_rasterizer_fill_rect (rasterizer, x0, y0 + has_top, x0+1, y1 - has_bottom, left);
             if (has_right)
               ctx_rasterizer_fill_rect (rasterizer, x1-1, y0 + has_top, x1, y1 - has_bottom, right);
             x0 += has_left;
             y0 += has_top;
             y1 -= has_bottom;
             x1 -= has_right;
             ctx_rasterizer_fill_rect (rasterizer, x0,y0,x1,y1,255);

             dst += rasterizer->blit_stride * ((((int)y1)-has_bottom) - (((int)y0)+has_top));
           }

           if (has_bottom)
           {
             int i = 0;
             if (has_left)
               coverage[i++] = bottom * left / 255;
             for (int x = x0 + has_left; x < x1 - has_right; x++)
               coverage[i++] = bottom;
             coverage[i++]= bottom * right / 255;

             ctx_rasterizer_apply_coverage (rasterizer,dst, x0, coverage, width);
           }
           }

           ctx_rasterizer_reset (rasterizer);
           goto done;
         }
       }
    }
#endif
    ctx_rasterizer_finish_shape (rasterizer);

    uint32_t hash = ctx_rasterizer_poly_to_edges (rasterizer);
    if (hash){};

#if CTX_SHAPE_CACHE
    int width = (rasterizer->col_max + (CTX_SUBDIV-1) ) / CTX_SUBDIV - rasterizer->col_min/CTX_SUBDIV + 1;
    int height = (rasterizer->scan_max + (CTX_FULL_AA-1) ) / CTX_FULL_AA - rasterizer->scan_min / CTX_FULL_AA + 1;
    if (width * height < CTX_SHAPE_CACHE_DIM && width >=1 && height >= 1
        && width < CTX_SHAPE_CACHE_MAX_DIM
        && height < CTX_SHAPE_CACHE_MAX_DIM 
#if CTX_ENABLE_SHADOW_BLUR
        && !rasterizer->in_shadow
#endif
        )
      {
        int scan_min = rasterizer->scan_min;
        int col_min = rasterizer->col_min;
        scan_min -= (scan_min % CTX_FULL_AA);
        int y0 = scan_min / CTX_FULL_AA;
        int y1 = y0 + height;
        int x0 = col_min / CTX_SUBDIV;
        int ymin = y0;
        int x1 = x0 + width;
        int clip_x_min = rasterizer->blit_x;
        int clip_x_max = rasterizer->blit_x + rasterizer->blit_width - 1;
        int clip_y_min = rasterizer->blit_y;
        int clip_y_max = rasterizer->blit_y + rasterizer->blit_height - 1;

        int dont_cache = 0;
        if (CTX_UNLIKELY(x1 >= clip_x_max))
          { x1 = clip_x_max;
            dont_cache = 1;
          }
        int xo = 0;
        if (CTX_UNLIKELY(x0 < clip_x_min))
          {
            xo = clip_x_min - x0;
            x0 = clip_x_min;
            dont_cache = 1;
          }
        if (CTX_UNLIKELY(y0 < clip_y_min || y1 >= clip_y_max))
          dont_cache = 1;
        if (dont_cache || !_ctx_shape_cache_enabled)
        {
#if CTX_INLINE_FILL_RULE
          if (rasterizer->state->gstate.fill_rule)
          ctx_rasterizer_rasterize_edges (rasterizer, 1
#if CTX_SHAPE_CACHE
                                        , NULL
#endif
                                       );
          else
          ctx_rasterizer_rasterize_edges (rasterizer, 0
#if CTX_SHAPE_CACHE
                                        , NULL
#endif
                                       );
#else
          ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                        , NULL
#endif
                                       );
#endif
        }
        else
        {
        rasterizer->scanline = scan_min;
        CtxShapeEntry *shape = ctx_shape_entry_find (rasterizer, hash, width, height); 

        if (shape->uses == 0)
          {
            CtxBuffer *buffer_backup = rasterizer->clip_buffer;
            rasterizer->clip_buffer = NULL;
#if CTX_INLINE_FILL_RULE
            if(rasterizer->state->gstate.fill_rule)
            ctx_rasterizer_rasterize_edges (rasterizer, 1, shape);
            else
            ctx_rasterizer_rasterize_edges (rasterizer, 0, shape);
#else
            ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule, shape);
#endif
            rasterizer->clip_buffer = buffer_backup;
          }
        rasterizer->scanline = scan_min;

        int ewidth = x1 - x0;
        if (ewidth>0)
        {
          if (rasterizer->clip_buffer && !rasterizer->clip_rectangle)
          {
          uint8_t composite[ewidth];
          for (int y = y0; y < y1; y++)
            {
              if ( (y >= clip_y_min) && (y <= clip_y_max) )
                {
                    for (int x = 0; x < ewidth; x++)
                    {
                      int val = shape->data[shape->width * (int)(y-ymin) + xo + x];
                      // XXX : not valid for 1bit clip buffers
                      val = (val*((uint8_t*)rasterizer->clip_buffer->data) [
                              ((y-rasterizer->blit_y) * rasterizer->blit_width) + x0 + x])/255;
                      composite[x] = val;
                    }
                    ctx_rasterizer_apply_coverage (rasterizer,
                                                 ( (uint8_t *) rasterizer->buf) + (y-rasterizer->blit_y) * rasterizer->blit_stride + (int) (x0) * rasterizer->format->bpp/8,
                                                 x0, // is 0
                                                 composite,
                                                 ewidth );
               rasterizer->scanline += CTX_FULL_AA;
            }
          }
          }
          else
          for (int y = y0; y < y1; y++)
            {
              if (CTX_LIKELY((y >= clip_y_min) && (y <= clip_y_max) ))
                {
                    ctx_rasterizer_apply_coverage (rasterizer,
                                                 ( (uint8_t *) rasterizer->buf) + (y-rasterizer->blit_y) * rasterizer->blit_stride + (int) (x0) * rasterizer->format->bpp/8,
                                                 x0,
                                                 &shape->data[shape->width * (int) (y-ymin) + xo],
                                                 ewidth );
                }
               rasterizer->scanline += CTX_FULL_AA;
            }
        }
        if (shape->uses != 0)
          {
            ctx_rasterizer_reset (rasterizer);
          }
        }
      }
    else
#endif
    {
            
#if CTX_INLINE_FILL_RULE
    if(rasterizer->state->gstate.fill_rule)
    ctx_rasterizer_rasterize_edges (rasterizer, 1
#if CTX_SHAPE_CACHE
                                    , NULL
#endif
                                   );
    else
    ctx_rasterizer_rasterize_edges (rasterizer, 0
#if CTX_SHAPE_CACHE
                                    , NULL
#endif
                                   );
#else
    ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                    , NULL
#endif
                                   );
#endif
    }
  }
done:
  if (CTX_UNLIKELY(rasterizer->preserve))
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
    }
#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
    rasterizer->scan_min -= rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->scan_max -= rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->col_min  -= (rasterizer->shadow_x - rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
    rasterizer->col_max  -= (rasterizer->shadow_x + rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
  }
#endif
  rasterizer->preserve = 0;
}

#if 0
static void
ctx_rasterizer_triangle (CtxRasterizer *rasterizer,
                         int x0, int y0,
                         int x1, int y1,
                         int x2, int y2,
                         int r0, int g0, int b0, int a0,
                         int r1, int g1, int b1, int a1,
                         int r2, int g2, int b2, int a2,
                         int u0, int v0,
                         int u1, int v1)
{

}
#endif


typedef struct _CtxTermGlyph CtxTermGlyph;

struct _CtxTermGlyph
{
  uint32_t unichar;
  int      col;
  int      row;
  uint8_t  rgba_bg[4];
  uint8_t  rgba_fg[4];
};

static int _ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke);
static void
ctx_rasterizer_glyph (CtxRasterizer *rasterizer, uint32_t unichar, int stroke)
{
  float tx = rasterizer->state->x;
  float ty = rasterizer->state->y - rasterizer->state->gstate.font_size;
  float tx2 = rasterizer->state->x + rasterizer->state->gstate.font_size;
  float ty2 = rasterizer->state->y + rasterizer->state->gstate.font_size;
  _ctx_user_to_device (rasterizer->state, &tx, &ty);
  _ctx_user_to_device (rasterizer->state, &tx2, &ty2);

  if (tx2 < rasterizer->blit_x || ty2 < rasterizer->blit_y) return;
  if (tx  > rasterizer->blit_x + rasterizer->blit_width ||
      ty  > rasterizer->blit_y + rasterizer->blit_height)
          return;

#if CTX_BRAILLE_TEXT
  float font_size = 0;
  int ch = 1;
  int cw = 1;

  if (rasterizer->term_glyphs)
  {
    float tx = 0;
    float ty = rasterizer->state->gstate.font_size;
    float txb = 0;
    float tyb = 0;

    ch = ctx_term_get_cell_height (rasterizer->ctx);
    cw = ctx_term_get_cell_width (rasterizer->ctx);

    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    _ctx_user_to_device (rasterizer->state, &txb, &tyb);
    font_size = ty-tyb;
  }
  if (rasterizer->term_glyphs && !stroke &&
      fabs (font_size - ch) < 0.5)
  {
    float tx = rasterizer->x;
    float ty = rasterizer->y;
    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    int col = tx / cw + 1;
    int row = ty / ch + 1;
    CtxTermGlyph *glyph = ctx_calloc (sizeof (CtxTermGlyph), 1);
    ctx_list_append (&rasterizer->glyphs, glyph);
    glyph->unichar = unichar;
    glyph->col = col;
    glyph->row = row;
    ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color,
                         &glyph->rgba_fg[0]);
  }
  else
#endif
  _ctx_glyph (rasterizer->ctx, unichar, stroke);
}


static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible);
static void
ctx_rasterizer_text (CtxRasterizer *rasterizer, const char *string, int stroke)
{
#if CTX_BRAILLE_TEXT
  float font_size = 0;
  if (rasterizer->term_glyphs)
  {
  float tx = 0;
  float ty = rasterizer->state->gstate.font_size;
  _ctx_user_to_device (rasterizer->state, &tx, &ty);
  font_size = ty;
  }
  int   ch = ctx_term_get_cell_height (rasterizer->ctx);
  int   cw = ctx_term_get_cell_width (rasterizer->ctx);

  if (rasterizer->term_glyphs && !stroke &&
      fabs (font_size - ch) < 0.5)
  {
    float tx = rasterizer->x;
    float ty = rasterizer->y;
    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    int col = tx / cw + 1;
    int row = ty / ch + 1;
    for (int i = 0; string[i]; i++, col++)
    {
      CtxTermGlyph *glyph = ctx_calloc (sizeof (CtxTermGlyph), 1);
      ctx_list_prepend (&rasterizer->glyphs, glyph);
      glyph->unichar = string[i];
      glyph->col = col;
      glyph->row = row;
      ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color,
                      glyph->rgba_fg);
    }
  }
  else
#endif
  {
    _ctx_text (rasterizer->ctx, string, stroke, 1);
  }
}

void
_ctx_font (Ctx *ctx, const char *name);
CTX_STATIC void
ctx_rasterizer_set_font (CtxRasterizer *rasterizer, const char *font_name)
{
  _ctx_font (rasterizer->ctx, font_name);
}

CTX_STATIC void
ctx_rasterizer_arc (CtxRasterizer *rasterizer,
                    float          x,
                    float          y,
                    float          radius,
                    float          start_angle,
                    float          end_angle,
                    int            anticlockwise)
{
  int full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS;
  full_segments = radius * CTX_PI * 2;
  if (full_segments > CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS)
    { full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS; }
  float step = CTX_PI*2.0/full_segments;
  int steps;

  if (end_angle < -30.0)
    end_angle = -30.0;
  if (start_angle < -30.0)
    start_angle = -30.0;
  if (end_angle > 30.0)
    end_angle = 30.0;
  if (start_angle > 30.0)
    start_angle = 30.0;

  if (radius <= 0.0001)
          return;

  if (end_angle == start_angle)
          // XXX also detect arcs fully outside render view
    {
    if (rasterizer->has_prev!=0)
      ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                              y + ctx_sinf (end_angle) * radius);
      else
      ctx_rasterizer_move_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                            y + ctx_sinf (end_angle) * radius);
      return;
    }
#if 1
  if ( (!anticlockwise && fabsf((end_angle - start_angle) - CTX_PI*2) < 0.01f)  ||
       ( (anticlockwise && fabsf((start_angle - end_angle) - CTX_PI*2) < 0.01f ) ) 
  ||   (anticlockwise && fabsf((end_angle - start_angle) - CTX_PI*2) < 0.01f)  ||  (!anticlockwise && fabsf((start_angle - end_angle) - CTX_PI*2) < 0.01f )  )
    {
      start_angle = start_angle;
      steps = full_segments - 1;
    }
  else
#endif
    {
      steps = (end_angle - start_angle) / (CTX_PI*2) * full_segments;
      if (anticlockwise)
        { steps = full_segments - steps; };
   // if (steps > full_segments)
   //   steps = full_segments;
    }
  if (anticlockwise) { step = step * -1; }
  int first = 1;
  if (steps == 0 /* || steps==full_segments -1  || (anticlockwise && steps == full_segments) */)
    {
      float xv = x + ctx_cosf (start_angle) * radius;
      float yv = y + ctx_sinf (start_angle) * radius;
      if (!rasterizer->has_prev)
        { ctx_rasterizer_move_to (rasterizer, xv, yv); }
      first = 0;
    }
  else
    {
      for (float angle = start_angle, i = 0; i < steps; angle += step, i++)
        {
          float xv = x + ctx_cosf (angle) * radius;
          float yv = y + ctx_sinf (angle) * radius;
          if (first && !rasterizer->has_prev)
            { ctx_rasterizer_move_to (rasterizer, xv, yv); }
          else
            { ctx_rasterizer_line_to (rasterizer, xv, yv); }
          first = 0;
        }
    }
  ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                          y + ctx_sinf (end_angle) * radius);
}

CTX_STATIC void
ctx_rasterizer_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y)
{
  /* XXX : it is probably cheaper/faster to do quad interpolation directly -
   *       though it will increase the code-size, an
   *       alternative is to turn everything into cubic
   *       and deal with cubics more directly during
   *       rasterization
   */
  ctx_rasterizer_curve_to (rasterizer,
                           (cx * 2 + rasterizer->x) / 3.0f, (cy * 2 + rasterizer->y) / 3.0f,
                           (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                           x,                              y);
}

CTX_STATIC void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                            float cx, float cy,
                            float x,  float y)
{
  ctx_rasterizer_quad_to (rasterizer, cx + rasterizer->x, cy + rasterizer->y,
                          x  + rasterizer->x, y  + rasterizer->y);
}

#define LENGTH_OVERSAMPLE 1
static void
ctx_rasterizer_pset (CtxRasterizer *rasterizer, int x, int y, uint8_t cov)
{
  // XXX - we avoid rendering here x==0 - to keep with
  //  an off-by one elsewhere
  //
  //  XXX onlt works in rgba8 formats
  if (x <= 0 || y < 0 || x >= rasterizer->blit_width ||
      y >= rasterizer->blit_height)
    { return; }
  uint8_t fg_color[4];
  ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, fg_color);
  uint8_t pixel[4];
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  dst += y * rasterizer->blit_stride;
  dst += x * rasterizer->format->bpp / 8;
  if (!rasterizer->format->to_comp ||
      !rasterizer->format->from_comp)
    { return; }
  if (cov == 255)
    {
      for (int c = 0; c < 4; c++)
        {
          pixel[c] = fg_color[c];
        }
    }
  else
    {
      rasterizer->format->to_comp (rasterizer, x, dst, &pixel[0], 1);
      for (int c = 0; c < 4; c++)
        {
          pixel[c] = ctx_lerp_u8 (pixel[c], fg_color[c], cov);
        }
    }
  rasterizer->format->from_comp (rasterizer, x, &pixel[0], dst, 1);
}

static void
ctx_rasterizer_stroke_1px (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxEntry *temp = rasterizer->edge_list.entries;
  float prev_x = 0.0f;
  float prev_y = 0.0f;
  int aa = 15;//rasterizer->aa;
  int start = 0;
  int end = 0;
#if 0
  float factor = ctx_matrix_get_scale (&state->gstate.transform);
#endif

  while (start < count)
    {
      int started = 0;
      int i;
      for (i = start; i < count; i++)
        {
          CtxEntry *entry = &temp[i];
          float x, y;
          if (entry->code == CTX_NEW_EDGE)
            {
              if (started)
                {
                  end = i - 1;
                  goto foo;
                }
              prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
              prev_y = entry->data.s16[1] * 1.0f / aa;
              started = 1;
              start = i;
            }
          x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
          y = entry->data.s16[3] * 1.0f / aa;
          int dx = x - prev_x;
          int dy = y - prev_y;
          int length = ctx_maxf (abs (dx), abs (dy) );
          if (length)
            {
              length *= LENGTH_OVERSAMPLE;
              int len = length;
              int tx = prev_x * 256;
              int ty = prev_y * 256;
              dx *= 256;
              dy *= 256;
              dx /= length;
              dy /= length;
              for (int i = 0; i < len; i++)
                {
                  ctx_rasterizer_pset (rasterizer, tx/256, ty/256, 255);
                  tx += dx;
                  ty += dy;
                  ctx_rasterizer_pset (rasterizer, tx/256, ty/256, 255);
                }
            }
          prev_x = x;
          prev_y = y;
        }
      end = i-1;
foo:
      start = end+1;
    }
  ctx_rasterizer_reset (rasterizer);
}

static void
ctx_rasterizer_stroke (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxSource source_backup = gstate->source_fill;
  if (gstate->source_stroke.type != CTX_SOURCE_INHERIT_FILL)
    gstate->source_fill = rasterizer->state->gstate.source_stroke;
  int count = rasterizer->edge_list.count;
  int preserved = rasterizer->preserve;
  float factor = ctx_matrix_get_scale (&gstate->transform);

  int aa = CTX_FULL_AA;
  CtxEntry temp[count]; /* copy of already built up path's poly line  */
  memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) );
#if 1
  if (CTX_UNLIKELY(gstate->line_width * factor <= 0.0f &&
      gstate->line_width * factor > -10.0f))
    {
      ctx_rasterizer_stroke_1px (rasterizer);
    }
  else
#endif
    {
      factor *= 0.86; /* this hack adjustment makes sharp 1px and 2px strokewidths
                            end up sharp without erronious AA
                       */
      ctx_rasterizer_reset (rasterizer); /* then start afresh with our stroked shape  */
      CtxMatrix transform_backup = gstate->transform;
      ctx_matrix_identity (&gstate->transform);
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float half_width_x = gstate->line_width * factor/2;
      float half_width_y = gstate->line_width * factor/2;
      if (CTX_UNLIKELY(gstate->line_width <= 0.0f))
        { // makes 0 width be hairline
          half_width_x = .5f;
          half_width_y = .5f;
        }
      int start = 0;
      int end   = 0;
      while (start < count)
        {
          int started = 0;
          int i;
          for (i = start; i < count; i++)
            {
              CtxEntry *entry = &temp[i];
              float x, y;
              if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                {
                  if (started)
                    {
                      end = i - 1;
                      goto foo;
                    }
                  prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  prev_y = entry->data.s16[1] * 1.0f / aa;
                  started = 1;
                  start = i;
                }
              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / aa;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              if (CTX_LIKELY(length>0.001f))
                {
                  dx = dx/length * half_width_x;
                  dy = dy/length * half_width_y;
                  if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                    {
                      ctx_rasterizer_finish_shape (rasterizer);
                      ctx_rasterizer_move_to (rasterizer, prev_x+dy, prev_y-dx);
                    }
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  
                  // we need to know the slope of the other side

                  // XXX possible miter line-to
                  //ctx_rasterizer_line_to (rasterizer, prev_x-dy+4, prev_y+dx+10);
                  //ctx_rasterizer_line_to (rasterizer, prev_x-dy+8, prev_y+dx+0);


                  ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
                }
              prev_x = x;
              prev_y = y;
            }
          end = i-1;
foo:
          for (int i = end; i >= start; i--)
            {
              CtxEntry *entry = &temp[i];
              float x, y, dx, dy;
              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / aa;
              dx = x - prev_x;
              dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              dx = dx/length * half_width_x;
              dy = dy/length * half_width_y;
              if (CTX_LIKELY(length>0.001f))
                {
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  // XXX possible miter line-to
             //   ctx_rasterizer_line_to (rasterizer, prev_x-dy+10, prev_y+dx+10);
                  ctx_rasterizer_line_to (rasterizer, x-dy,      y+dx);
                }
              prev_x = x;
              prev_y = y;
              if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                {
                  x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[1] * 1.0f / aa;
                  dx = x - prev_x;
                  dy = y - prev_y;
                  length = ctx_fast_hypotf (dx, dy);
                  if (CTX_LIKELY(length>0.001f))
                    {
                      dx = dx / length * half_width_x;
                      dy = dy / length * half_width_y;
                      ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                      ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
                    }
                }
              if ( (prev_x != x) && (prev_y != y) )
                {
                  prev_x = x;
                  prev_y = y;
                }
            }
          start = end+1;
        }
      ctx_rasterizer_finish_shape (rasterizer);
      switch (gstate->line_cap)
        {
          case CTX_CAP_SQUARE: // XXX: incorrect - if rectangles were in
                               //                  reverse order - rotation would be off
                               //                  better implement correct here
            {
              float x = 0, y = 0;
              int has_prev = 0;
              for (int i = 0; i < count; i++)
                {
                  CtxEntry *entry = &temp[i];
                  if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x, half_width_y);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / aa;
                      ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / aa;
                  has_prev = 1;
                }
              ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
              ctx_rasterizer_finish_shape (rasterizer);
            }
            break;
          case CTX_CAP_NONE: /* nothing to do */
            break;
          case CTX_CAP_ROUND:
            {
              float x = 0, y = 0;
              int has_prev = 0;
              for (int i = 0; i < count; i++)
                {
                  CtxEntry *entry = &temp[i];
                  if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / aa;
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / aa;
                  has_prev = 1;
                }
              ctx_rasterizer_move_to (rasterizer, x, y);
              ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
              ctx_rasterizer_finish_shape (rasterizer);
              break;
            }
        }
      switch (gstate->line_join)
        {
          case CTX_JOIN_BEVEL:
          case CTX_JOIN_MITER:
            break;
          case CTX_JOIN_ROUND:
            {
              float x = 0, y = 0;
              for (int i = 0; i < count-1; i++)
                {
                  CtxEntry *entry = &temp[i];
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / aa;
                  if (CTX_UNLIKELY(entry[1].code == CTX_EDGE))
                    {
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*2, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                }
              break;
            }
        }
      CtxFillRule rule_backup = gstate->fill_rule;
      gstate->fill_rule = CTX_FILL_RULE_WINDING;
      rasterizer->preserve = 0; // so fill isn't tripped
      ctx_rasterizer_fill (rasterizer);
      gstate->fill_rule = rule_backup;
      gstate->transform = transform_backup;
    }
  if (preserved)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
  if (gstate->source_stroke.type != CTX_SOURCE_INHERIT_FILL)
    gstate->source_fill = source_backup;
}

#if CTX_1BIT_CLIP
#define CTX_CLIP_FORMAT CTX_FORMAT_GRAY1
#else
#define CTX_CLIP_FORMAT CTX_FORMAT_GRAY8
#endif


CTX_STATIC void
ctx_rasterizer_clip_reset (CtxRasterizer *rasterizer)
{
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
   ctx_buffer_free (rasterizer->clip_buffer);
  rasterizer->clip_buffer = NULL;
#endif
  rasterizer->state->gstate.clip_min_x = rasterizer->blit_x;
  rasterizer->state->gstate.clip_min_y = rasterizer->blit_y;

  rasterizer->state->gstate.clip_max_x = rasterizer->blit_x + rasterizer->blit_width - 1;
  rasterizer->state->gstate.clip_max_y = rasterizer->blit_y + rasterizer->blit_height - 1;
}

CTX_STATIC void
ctx_rasterizer_clip_apply (CtxRasterizer *rasterizer,
                           CtxEntry      *edges)
{
  int count = edges[0].data.u32[0];

  int minx = 5000;
  int miny = 5000;
  int maxx = -5000;
  int maxy = -5000;
  int prev_x = 0;
  int prev_y = 0;
  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;

  int aa = 15;//rasterizer->aa;
  float coords[6][2];

  for (int i = 0; i < count; i++)
    {
      CtxEntry *entry = &edges[i+1];
      float x, y;
      if (entry->code == CTX_NEW_EDGE)
        {
          prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
          prev_y = entry->data.s16[1] * 1.0f / aa;
          if (prev_x < minx) { minx = prev_x; }
          if (prev_y < miny) { miny = prev_y; }
          if (prev_x > maxx) { maxx = prev_x; }
          if (prev_y > maxy) { maxy = prev_y; }
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / aa;
      if (x < minx) { minx = x; }
      if (y < miny) { miny = y; }
      if (x > maxx) { maxx = x; }
      if (y > maxy) { maxy = y; }

      if (i < 6)
      {
        coords[i][0] = x;
        coords[i][1] = y;
      }
    }

#if CTX_ENABLE_CLIP

  if ((rasterizer->clip_rectangle==1
                          || !rasterizer->clip_buffer)
                  )
  {
    if (count == 6)
    {
      if (coords[3][0] == coords[5][0] &&
          coords[3][1] == coords[5][1])
      {
#if 0
        printf ("%d,%d %dx%d\n", minx, miny,
                                       maxx-minx+1, maxy-miny+1);
#endif

         rasterizer->state->gstate.clip_min_x =
            ctx_maxi (minx, rasterizer->state->gstate.clip_min_x);
         rasterizer->state->gstate.clip_min_y =
            ctx_maxi (miny, rasterizer->state->gstate.clip_min_y);
         rasterizer->state->gstate.clip_max_x =
            ctx_mini (maxx, rasterizer->state->gstate.clip_max_x);
         rasterizer->state->gstate.clip_max_y =
            ctx_mini (maxy, rasterizer->state->gstate.clip_max_y);

         rasterizer->clip_rectangle = 1;

#if 0
         if (!rasterizer->clip_buffer)
           rasterizer->clip_buffer = ctx_buffer_new (blit_width,
                                                     blit_height,
                                                     CTX_CLIP_FORMAT);

         memset (rasterizer->clip_buffer->data, 0, blit_width * blit_height);
         int i = 0;
         for (int y = rasterizer->state->gstate.clip_min_y;
                  y <= rasterizer->state->gstate.clip_max_y;
                  y++)
         for (int x = rasterizer->state->gstate.clip_min_x;
                  x <= rasterizer->state->gstate.clip_max_x;
                  x++, i++)
         {
           ((uint8_t*)(rasterizer->clip_buffer->data))[i] = 255;
         }
#endif

         return;
      }
    }
  }
  rasterizer->clip_rectangle = 0;

  if ((minx == maxx) || (miny == maxy)) // XXX : reset hack
  {
    ctx_rasterizer_clip_reset (rasterizer);
    return;//goto done;
  }

  int we_made_it = 0;
  CtxBuffer *clip_buffer;

  if (!rasterizer->clip_buffer)
  {
    rasterizer->clip_buffer = ctx_buffer_new (blit_width,
                                              blit_height,
                                              CTX_CLIP_FORMAT);
    clip_buffer = rasterizer->clip_buffer;
    we_made_it = 1;
    if (CTX_CLIP_FORMAT == CTX_FORMAT_GRAY1)
      memset (rasterizer->clip_buffer->data, 0, blit_width * blit_height/8);
    else
      memset (rasterizer->clip_buffer->data, 0, blit_width * blit_height);
  }
  else
  {
    clip_buffer = ctx_buffer_new (blit_width, blit_height,
                                  CTX_CLIP_FORMAT);
  }

  {

  int prev_x = 0;
  int prev_y = 0;

    Ctx *ctx = ctx_new_for_framebuffer (clip_buffer->data, blit_width, blit_height,
       blit_width,
       CTX_CLIP_FORMAT);

  for (int i = 0; i < count; i++)
    {
      CtxEntry *entry = &edges[i+1];
      float x, y;
      if (entry->code == CTX_NEW_EDGE)
        {
          prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
          prev_y = entry->data.s16[1] * 1.0f / aa;
          ctx_move_to (ctx, prev_x, prev_y);
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / aa;
      ctx_line_to (ctx, x, y);
    }
    ctx_gray (ctx, 1.0f);
    ctx_fill (ctx);
    ctx_free (ctx);
  }

  int maybe_rect = 1;
  rasterizer->clip_rectangle = 0;

  if (CTX_CLIP_FORMAT == CTX_FORMAT_GRAY1)
  {
    int count = blit_width * blit_height / 8;
    for (int i = 0; i < count; i++)
    {
      ((uint8_t*)rasterizer->clip_buffer->data)[i] =
      (((uint8_t*)rasterizer->clip_buffer->data)[i] &
      ((uint8_t*)clip_buffer->data)[i]);
    }
  }
  else
  {
    int count = blit_width * blit_height;


    int i;
    int x0 = 0;
    int y0 = 0;
    int width = -1;
    int next_stage = 0;
    uint8_t *p_data = (uint8_t*)rasterizer->clip_buffer->data;
    uint8_t *data = (uint8_t*)clip_buffer->data;

    i=0;
    /* find upper left */
    for (; i < count && maybe_rect && !next_stage; i++)
    {
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;
      switch (val)
      {
        case 255:
          x0 = i % blit_width;
          y0 = i / blit_width;
          next_stage = 1;
          break;
        case 0: break;
        default:
          maybe_rect = 0;
          break;
      }
    }

    next_stage = 0;
    /* figure out with */
    for (; i < count && !next_stage && maybe_rect; i++)
    {
      int x = i % blit_width;
      int y = i / blit_width;
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;

      if (y == y0)
      {
        switch (val)
        {
          case 255:
            width = x - x0 + 1;
            break;
          case 0:
            next_stage = 1;
            break;
          default:
            maybe_rect = 0;
            break;
        }
        if (x % blit_width == blit_width - 1) next_stage = 1;
      }
      else next_stage = 1;
    }

    next_stage = 0;
    /* body */
    for (; i < count && maybe_rect && !next_stage; i++)
    {
      int x = i % blit_width;
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;

      if (x < x0)
      {
        if (val != 0){ maybe_rect = 0; next_stage = 1; }
      } else if (x < x0 + width)
      {
        if (val != 255){ if (val != 0) maybe_rect = 0; next_stage = 1; }
      } else {
        if (val != 0){ maybe_rect = 0; next_stage = 1; }
      }
    }

    next_stage = 0;
    /* foot */
    for (; i < count && maybe_rect && !next_stage; i++)
    {
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;

      if (val != 0){ maybe_rect = 0; next_stage = 1; }
    }


    for (; i < count; i++)
    {
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;
    }

    if (maybe_rect)
       rasterizer->clip_rectangle = 1;
  }
  if (!we_made_it)
   ctx_buffer_free (clip_buffer);
#else
  if (coords[0][0]){};
#endif
  
  rasterizer->state->gstate.clip_min_x = ctx_maxi (minx,
                                         rasterizer->state->gstate.clip_min_x);
  rasterizer->state->gstate.clip_min_y = ctx_maxi (miny,
                                         rasterizer->state->gstate.clip_min_y);
  rasterizer->state->gstate.clip_max_x = ctx_mini (maxx,
                                         rasterizer->state->gstate.clip_max_x);
  rasterizer->state->gstate.clip_max_y = ctx_mini (maxy,
                                         rasterizer->state->gstate.clip_max_y);
}

CTX_STATIC void
ctx_rasterizer_clip (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxEntry temp[count+1]; /* copy of already built up path's poly line  */
  rasterizer->state->has_clipped=1;
  rasterizer->state->gstate.clipped=1;
  //if (rasterizer->preserve)
    { memcpy (temp + 1, rasterizer->edge_list.entries, sizeof (temp) - sizeof (temp[0]));
      temp[0].code = CTX_NOP;
      temp[0].data.u32[0] = count;
      ctx_state_set_blob (rasterizer->state, CTX_clip, (uint8_t*)temp, sizeof(temp));
    }
  ctx_rasterizer_clip_apply (rasterizer, temp);
  ctx_rasterizer_reset (rasterizer);
  if (rasterizer->preserve)
    {
      memcpy (rasterizer->edge_list.entries, temp + 1, sizeof (temp) - sizeof(temp[0]));
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
}


#if 0
static void
ctx_rasterizer_load_image (CtxRasterizer *rasterizer,
                           const char  *path,
                           float x,
                           float y)
{
  // decode PNG, put it in image is slot 1,
  // magic width height stride format data
  ctx_buffer_load_png (&rasterizer->ctx->texture[0], path);
  ctx_rasterizer_set_texture (rasterizer, 0, x, y);
}
#endif


CTX_INLINE void
ctx_rasterizer_rectangle (CtxRasterizer *rasterizer,
                          float x,
                          float y,
                          float width,
                          float height)
{
  ctx_rasterizer_move_to (rasterizer, x, y);
  ctx_rasterizer_rel_line_to (rasterizer, width, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, height);
  ctx_rasterizer_rel_line_to (rasterizer, -width, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, -height);
  ctx_rasterizer_rel_line_to (rasterizer, width/2, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

CTX_STATIC void
ctx_rasterizer_set_pixel (CtxRasterizer *rasterizer,
                          uint16_t x,
                          uint16_t y,
                          uint8_t r,
                          uint8_t g,
                          uint8_t b,
                          uint8_t a)
{
  rasterizer->state->gstate.source_fill.type = CTX_SOURCE_COLOR;
  ctx_color_set_RGBA8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, r, g, b, a);
  rasterizer->comp_op = NULL;
#if 0
  // XXX : doesn't take transforms into account - and has
  // received less testing than code paths part of protocol,
  // using rectangle properly will trigger the fillrect fastpath
  ctx_rasterizer_pset (rasterizer, x, y, 255);
#else
  ctx_rasterizer_rectangle (rasterizer, x, y, 1.0, 1.0);
  ctx_rasterizer_fill (rasterizer);
#endif
}

#if CTX_ENABLE_SHADOW_BLUR
static float
ctx_gaussian (float x, float mu, float sigma)
{
  float a = ( x- mu) / sigma;
  return ctx_expf (-0.5 * a * a);
}

static void
ctx_compute_gaussian_kernel (int dim, float radius, float *kernel)
{
  float sigma = radius / 2;
  float sum = 0.0;
  int i = 0;
  //for (int row = 0; row < dim; row ++)
    for (int col = 0; col < dim; col ++, i++)
    {
      float val = //ctx_gaussian (row, radius, sigma) *
                            ctx_gaussian (col, radius, sigma);
      kernel[i] = val;
      sum += val;
    }
  i = 0;
  //for (int row = 0; row < dim; row ++)
    for (int col = 0; col < dim; col ++, i++)
        kernel[i] /= sum;
}
#endif

CTX_STATIC void
ctx_rasterizer_round_rectangle (CtxRasterizer *rasterizer, float x, float y, float width, float height, float corner_radius)
{
  float aspect  = 1.0f;
  float radius  = corner_radius / aspect;
  float degrees = CTX_PI / 180.0f;

  if (radius > width*0.5f) radius = width/2;
  if (radius > height*0.5f) radius = height/2;

  ctx_rasterizer_finish_shape (rasterizer);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + radius, radius, 180 * degrees, 270 * degrees, 0);

  ctx_rasterizer_finish_shape (rasterizer);
}

static void
ctx_rasterizer_process (void *user_data, CtxCommand *command);

int
_ctx_is_rasterizer (Ctx *ctx)
{
  if (ctx->renderer && ctx->renderer->process == ctx_rasterizer_process)
    return 1;
  return 0;
}

#if CTX_COMPOSITING_GROUPS
static void
ctx_rasterizer_start_group (CtxRasterizer *rasterizer)
{
  CtxEntry save_command = ctx_void(CTX_SAVE);
  // allocate buffer, and set it as temporary target
  int no;
  if (rasterizer->group[0] == NULL) // first group
  {
    rasterizer->saved_buf = rasterizer->buf;
  }
  for (no = 0; rasterizer->group[no] && no < CTX_GROUP_MAX; no++);

  if (no >= CTX_GROUP_MAX)
     return;
  rasterizer->group[no] = ctx_buffer_new (rasterizer->blit_width,
                                          rasterizer->blit_height,
                                          rasterizer->format->composite_format);
  rasterizer->buf = rasterizer->group[no]->data;
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);
}

static void
ctx_rasterizer_end_group (CtxRasterizer *rasterizer)
{
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  CtxEntry save_command = ctx_void(CTX_SAVE);
  int no = 0;
  for (no = 0; rasterizer->group[no] && no < CTX_GROUP_MAX; no++);
  no--;

  if (no < 0)
    return;

  CtxCompositingMode comp = rasterizer->state->gstate.compositing_mode;
  CtxBlend blend = rasterizer->state->gstate.blend_mode;
  float global_alpha = rasterizer->state->gstate.global_alpha_f;
  // fetch compositing, blending, global alpha
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);
  CtxEntry set_state[3]=
  {
    ctx_u32 (CTX_COMPOSITING_MODE, comp,  0),
    ctx_u32 (CTX_BLEND_MODE,       blend, 0),
    ctx_f  (CTX_GLOBAL_ALPHA,     global_alpha, 0.0)
  };
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_state[0]);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_state[1]);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_state[2]);
  if (no == 0)
  {
    rasterizer->buf = rasterizer->saved_buf;
  }
  else
  {
    rasterizer->buf = rasterizer->group[no-1]->data;
  }
  // XXX use texture_source ?
   ctx_texture_init (rasterizer->ctx, ".ctx-group", // XXX ? count groups..
                  rasterizer->blit_width,  // or have group based on thread-id?
                  rasterizer->blit_height, // .. this would mean threadsafe
                                           // allocation
                  rasterizer->blit_width * rasterizer->format->bpp/8,
                  rasterizer->format->pixel_format,
                  NULL, // space
                  (uint8_t*)rasterizer->group[no]->data,
                  NULL, NULL);
  {
     const char *eid = ".ctx-group";
     int   eid_len = strlen (eid);

     CtxEntry commands[4] =
      {
       ctx_f  (CTX_TEXTURE, rasterizer->blit_x, rasterizer->blit_y), 
       ctx_u32 (CTX_DATA, eid_len, eid_len/9+1),
       ctx_u32 (CTX_CONT, 0,0),
       ctx_u32 (CTX_CONT, 0,0)
      };
     memcpy( (char *) &commands[2].data.u8[0], eid, eid_len);
     ( (char *) (&commands[2].data.u8[0]) ) [eid_len]=0;

     ctx_rasterizer_process (rasterizer, (CtxCommand*)commands);
  }
  {
    CtxEntry commands[2]=
    {
      ctx_f (CTX_RECTANGLE, rasterizer->blit_x, rasterizer->blit_y),
      ctx_f (CTX_CONT,      rasterizer->blit_width, rasterizer->blit_height)
    };
    ctx_rasterizer_process (rasterizer, (CtxCommand*)commands);
  }
  {
    CtxEntry commands[1]= { ctx_void (CTX_FILL) };
    ctx_rasterizer_process (rasterizer, (CtxCommand*)commands);
  }
  //ctx_texture_release (rasterizer->ctx, ".ctx-group");
  ctx_buffer_free (rasterizer->group[no]);
  rasterizer->group[no] = 0;
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
}
#endif

#if CTX_ENABLE_SHADOW_BLUR
static void
ctx_rasterizer_shadow_stroke (CtxRasterizer *rasterizer)
{
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->ctx, CTX_shadowColor, &color) == 0)
    ctx_color_get_rgba (rasterizer->state, &color, rgba);

  CtxEntry set_color_command [3]=
  {
    ctx_f (CTX_COLOR, CTX_RGBA, rgba[0]),
    ctx_f (CTX_CONT, rgba[1], rgba[2]),
    ctx_f (CTX_CONT, rgba[3], 0)
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  float radius = rasterizer->state->gstate.shadow_blur;
  int dim = 2 * radius + 1;
  if (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM)
    dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
  ctx_compute_gaussian_kernel (dim, radius, rasterizer->kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);
  {
    int i = 0;
    for (int v = 0; v < dim; v += 1, i++)
      {
        float dy = rasterizer->state->gstate.shadow_offset_y + v - dim/2;
        set_color_command[2].data.f[0] = rasterizer->kernel[i] * rgba[3];
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_color_command[0]);
#if CTX_ENABLE_SHADOW_BLUR
        rasterizer->in_shadow = 1;
#endif
        rasterizer->shadow_x = rasterizer->state->gstate.shadow_offset_x;
        rasterizer->shadow_y = dy;
        rasterizer->preserve = 1;
        ctx_rasterizer_stroke (rasterizer);
#if CTX_ENABLE_SHADOW_BLUR
        rasterizer->in_shadow = 0;
#endif
      }
  }
  //free (kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
}

static void
ctx_rasterizer_shadow_text (CtxRasterizer *rasterizer, const char *str)
{
  float x = rasterizer->state->x;
  float y = rasterizer->state->y;
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->ctx, CTX_shadowColor, &color) == 0)
    ctx_color_get_rgba (rasterizer->state, &color, rgba);

  CtxEntry set_color_command [3]=
  {
    ctx_f (CTX_COLOR, CTX_RGBA, rgba[0]),
    ctx_f (CTX_CONT, rgba[1], rgba[2]),
    ctx_f (CTX_CONT, rgba[3], 0)
  };
  CtxEntry move_to_command [1]=
  {
    ctx_f (CTX_MOVE_TO, x, y),
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  float radius = rasterizer->state->gstate.shadow_blur;
  int dim = 2 * radius + 1;
  if (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM)
    dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
  ctx_compute_gaussian_kernel (dim, radius, rasterizer->kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);

  {
      {
        move_to_command[0].data.f[0] = x;
        move_to_command[0].data.f[1] = y;
        set_color_command[2].data.f[0] = rgba[3];
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_color_command);
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&move_to_command);
        rasterizer->in_shadow=1;
        ctx_rasterizer_text (rasterizer, str, 0);
        rasterizer->in_shadow=0;
      }
  }
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
  move_to_command[0].data.f[0] = x;
  move_to_command[0].data.f[1] = y;
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&move_to_command);
}

static void
ctx_rasterizer_shadow_fill (CtxRasterizer *rasterizer)
{
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->ctx, CTX_shadowColor, &color) == 0)
    ctx_color_get_rgba (rasterizer->state, &color, rgba);

  CtxEntry set_color_command [3]=
  {
    ctx_f (CTX_COLOR, CTX_RGBA, rgba[0]),
    ctx_f (CTX_CONT, rgba[1], rgba[2]),
    ctx_f (CTX_CONT, rgba[3], 0)
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  float radius = rasterizer->state->gstate.shadow_blur;
  int dim = 2 * radius + 1;
  if (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM)
    dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
  ctx_compute_gaussian_kernel (dim, radius, rasterizer->kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);

  {
    for (int v = 0; v < dim; v ++)
      {
        int i = v;
        float dy = rasterizer->state->gstate.shadow_offset_y + v - dim/2;
        set_color_command[2].data.f[0] = rasterizer->kernel[i] * rgba[3];
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_color_command);
        rasterizer->in_shadow = 1;
        rasterizer->shadow_x = rasterizer->state->gstate.shadow_offset_x;
        rasterizer->shadow_y = dy;
        rasterizer->preserve = 1;
        ctx_rasterizer_fill (rasterizer);
        rasterizer->in_shadow = 0;
      }
  }
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
}
#endif

static void
ctx_rasterizer_line_dash (CtxRasterizer *rasterizer, int count, float *dashes)
{
  if (!dashes)
  {
    rasterizer->state->gstate.n_dashes = 0;
    return;
  }
  count = CTX_MIN(count, CTX_PARSER_MAX_ARGS-1);
  rasterizer->state->gstate.n_dashes = count;
  memcpy(&rasterizer->state->gstate.dashes[0], dashes, count * sizeof(float));
  for (int i = 0; i < count; i ++)
  {
    if (rasterizer->state->gstate.dashes[i] < 0.0001f)
      rasterizer->state->gstate.dashes[i] = 0.0001f; // hang protection
  }
}


static void
ctx_rasterizer_process (void *user_data, CtxCommand *command)
{
  CtxEntry *entry = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) user_data;
  CtxState *state = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  int clear_clip = 0;
  ctx_interpret_style (rasterizer->state, entry, NULL);
  switch (c->code)
    {
#if CTX_ENABLE_SHADOW_BLUR
      case CTX_SHADOW_COLOR:
        {
          CtxColor  col;
          CtxColor *color = &col;
          //state->gstate.source_fill.type = CTX_SOURCE_COLOR;
          switch ((int)c->rgba.model)
            {
              case CTX_RGB:
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, 1.0f);
                break;
              case CTX_RGBA:
                //ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
              case CTX_DRGBA:
                ctx_color_set_drgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYKA:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_CMYK:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
              case CTX_DCMYKA:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_DCMYK:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
#endif
              case CTX_GRAYA:
                ctx_color_set_graya (state, color, c->graya.g, c->graya.a);
                break;
              case CTX_GRAY:
                ctx_color_set_graya (state, color, c->graya.g, 1.0f);
                break;
            }
          ctx_set_color (rasterizer->ctx, CTX_shadowColor, color);
        }
        break;
#endif
      case CTX_LINE_DASH:
        if (c->line_dash.count)
          {
            ctx_rasterizer_line_dash (rasterizer, c->line_dash.count, c->line_dash.data);
          }
        else
        ctx_rasterizer_line_dash (rasterizer, 0, NULL);
        break;

      case CTX_LINE_TO:
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_LINE_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_MOVE_TO:
        ctx_rasterizer_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rasterizer_rel_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_CURVE_TO:
        ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
                                 c->c.x1, c->c.y1,
                                 c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
                                     c->c.x1, c->c.y1,
                                     c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_ARC:
        ctx_rasterizer_arc (rasterizer, c->arc.x, c->arc.y, c->arc.radius, c->arc.angle1, c->arc.angle2, c->arc.direction);
        break;
      case CTX_RECTANGLE:
        ctx_rasterizer_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                  c->rectangle.width, c->rectangle.height);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_rasterizer_round_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                        c->rectangle.width, c->rectangle.height,
                                        c->rectangle.radius);
        break;
      case CTX_SET_PIXEL:
        ctx_rasterizer_set_pixel (rasterizer, c->set_pixel.x, c->set_pixel.y,
                                  c->set_pixel.rgba[0],
                                  c->set_pixel.rgba[1],
                                  c->set_pixel.rgba[2],
                                  c->set_pixel.rgba[3]);
        break;
      case CTX_DEFINE_TEXTURE:
        {
          uint8_t *pixel_data = ctx_define_texture_pixel_data (entry);
          ctx_rasterizer_define_texture (rasterizer, c->define_texture.eid,
                                         c->define_texture.width, c->define_texture.height,
                                         c->define_texture.format,
                                         pixel_data);
          rasterizer->comp_op = NULL;
          rasterizer->fragment = NULL;
        }
        break;
      case CTX_TEXTURE:
        ctx_rasterizer_set_texture (rasterizer, c->texture.eid,
                                    c->texture.x, c->texture.y);
        rasterizer->comp_op = NULL;
        rasterizer->fragment = NULL;
        break;
#if 0
      case CTX_LOAD_IMAGE:
        ctx_rasterizer_load_image (rasterizer, ctx_arg_string(),
                                   ctx_arg_float (0), ctx_arg_float (1) );
        break;
#endif
#if CTX_GRADIENTS
      case CTX_GRADIENT_STOP:
        {
          float rgba[4]= {ctx_u8_to_float (ctx_arg_u8 (4) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+1) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+2) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+3) )
                         };
          ctx_rasterizer_gradient_add_stop (rasterizer,
                                            ctx_arg_float (0), rgba);
          rasterizer->comp_op = NULL;
        }
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_state_gradient_clear_stops (rasterizer->state);
        rasterizer->comp_op = NULL;
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_state_gradient_clear_stops (rasterizer->state);
        rasterizer->comp_op = NULL;
        break;
#endif
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_COLOR:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
        rasterizer->comp_op = NULL;
        //_ctx_setup_compositor (rasterizer);
        break;
#if CTX_COMPOSITING_GROUPS
      case CTX_START_GROUP:
        ctx_rasterizer_start_group (rasterizer);
        break;
      case CTX_END_GROUP:
        ctx_rasterizer_end_group (rasterizer);
        break;
#endif

      case CTX_RESTORE:
        for (int i = state->gstate_no?state->gstate_stack[state->gstate_no-1].keydb_pos:0;
             i < state->gstate.keydb_pos; i++)
        {
          if (state->keydb[i].key == CTX_clip)
          {
            clear_clip = 1;
          }
        }
        /* FALLTHROUGH */
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_IDENTITY:
      case CTX_SAVE:
        rasterizer->comp_op = NULL;
        rasterizer->uses_transforms = 1;
        ctx_interpret_transforms (rasterizer->state, entry, NULL);
        if (clear_clip)
        {
          ctx_rasterizer_clip_reset (rasterizer);
        for (int i = state->gstate_no?state->gstate_stack[state->gstate_no-1].keydb_pos:0;
             i < state->gstate.keydb_pos; i++)
        {
          if (state->keydb[i].key == CTX_clip)
          {
            int idx = ctx_float_to_string_index (state->keydb[i].value);
            if (idx >=0)
            {
              CtxEntry *edges = (CtxEntry*)&state->stringpool[idx];
              ctx_rasterizer_clip_apply (rasterizer, edges);
            }
          }
        }
        }
        break;
      case CTX_STROKE:
#if CTX_ENABLE_SHADOW_BLUR
        if (rasterizer->state->gstate.shadow_blur > 0.0 &&
            !rasterizer->in_text)
          ctx_rasterizer_shadow_stroke (rasterizer);
#endif
        {
        int count = rasterizer->edge_list.count;
        if (count <= 3)
          rasterizer->in_line_stroke++;
        if (rasterizer->state->gstate.n_dashes)
        {
          int n_dashes = rasterizer->state->gstate.n_dashes;
          float *dashes = rasterizer->state->gstate.dashes;
          float factor = ctx_matrix_get_scale (&state->gstate.transform);

          int aa = 15;//rasterizer->aa;
          CtxEntry temp[count]; /* copy of already built up path's poly line  */
          memcpy (temp, rasterizer->edge_list.entries, sizeof (temp));
          int start = 0;
          int end   = 0;
      CtxMatrix transform_backup = rasterizer->state->gstate.transform;
      ctx_matrix_identity (&rasterizer->state->gstate.transform);
      ctx_rasterizer_reset (rasterizer); /* for dashing we create
                                            a dashed path to stroke */
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float pos = 0.0;

      int   dash_no  = 0.0;
      float dash_lpos = rasterizer->state->gstate.line_dash_offset * factor;
      int   is_down = 0;

          while (start < count)
          {
            int started = 0;
            int i;
            is_down = 0;

            if (!is_down)
            {
              CtxEntry *entry = &temp[0];
              prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
              prev_y = entry->data.s16[1] * 1.0f / aa;
              ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
              is_down = 1;
            }


            for (i = start; i < count; i++)
            {
              CtxEntry *entry = &temp[i];
              float x, y;
              if (entry->code == CTX_NEW_EDGE)
                {
                  if (started)
                    {
                      end = i - 1;
                      dash_no = 0;
                      dash_lpos = 0.0;
                      goto foo;
                    }
                  prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  prev_y = entry->data.s16[1] * 1.0f / aa;
                  started = 1;
                  start = i;
                  is_down = 1;
                  ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
                }

again:

              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / aa;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);

              if (dash_lpos + length >= dashes[dash_no] * factor)
              {
                float p = (dashes[dash_no] * factor - dash_lpos) / length;
                float splitx = x * p + (1.0f - p) * prev_x;
                float splity = y * p + (1.0f - p) * prev_y;
                if (is_down)
                {
                  ctx_rasterizer_line_to (rasterizer, splitx, splity);
                  is_down = 0;
                }
                else
                {
                  ctx_rasterizer_move_to (rasterizer, splitx, splity);
                  is_down = 1;
                }
                prev_x = splitx;
                prev_y = splity;
                dash_no++;
                dash_lpos=0;
                if (dash_no >= n_dashes) dash_no = 0;
                goto again;
              }
              else
              {
                pos += length;
                dash_lpos += length;
                {
                  if (is_down)
                    ctx_rasterizer_line_to (rasterizer, x, y);
                }
              }
              prev_x = x;
              prev_y = y;
            }
          end = i-1;
foo:
          start = end+1;
        }
      rasterizer->state->gstate.transform = transform_backup;
        }
        ctx_rasterizer_stroke (rasterizer);
        if (count <= 3)
          rasterizer->in_line_stroke--;
        }

        break;
      case CTX_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_TEXT:
        rasterizer->in_text++;
#if CTX_ENABLE_SHADOW_BLUR
        if (rasterizer->state->gstate.shadow_blur > 0.0)
          ctx_rasterizer_shadow_text (rasterizer, ctx_arg_string ());
#endif
        ctx_rasterizer_text (rasterizer, ctx_arg_string(), 0);
        rasterizer->in_text--;
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_STROKE_TEXT:
        ctx_rasterizer_text (rasterizer, ctx_arg_string(), 1);
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_GLYPH:
        ctx_rasterizer_glyph (rasterizer, entry[0].data.u32[0], entry[0].data.u8[4]);
        break;
      case CTX_FILL:
#if CTX_ENABLE_SHADOW_BLUR
        if (rasterizer->state->gstate.shadow_blur > 0.0 &&
            !rasterizer->in_text)
          ctx_rasterizer_shadow_fill (rasterizer);
#endif
        ctx_rasterizer_fill (rasterizer);
        break;
      case CTX_RESET:
      case CTX_BEGIN_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLIP:
        ctx_rasterizer_clip (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_finish_shape (rasterizer);
        break;
    }
  ctx_interpret_pos_bare (rasterizer->state, entry, NULL);
}

void
ctx_rasterizer_deinit (CtxRasterizer *rasterizer)
{
  ctx_drawlist_deinit (&rasterizer->edge_list);
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
  {
    ctx_buffer_free (rasterizer->clip_buffer);
    rasterizer->clip_buffer = NULL;
  }
#endif
#if CTX_SHAPE_CACHE
  for (int i = 0; i < CTX_SHAPE_CACHE_ENTRIES; i ++)
    if (rasterizer->shape_cache.entries[i])
    {
      free (rasterizer->shape_cache.entries[i]);
      rasterizer->shape_cache.entries[i] = NULL;
    }

#endif
  free (rasterizer);
}


CtxAntialias ctx_get_antialias (Ctx *ctx)
{
#if CTX_EVENTS
  if (ctx_renderer_is_sdl (ctx) || ctx_renderer_is_fb (ctx))
  {
     CtxTiled *fb = (CtxTiled*)(ctx->renderer);
     return fb->antialias;
  }
#endif
  if (!_ctx_is_rasterizer (ctx)) return CTX_ANTIALIAS_DEFAULT;

  switch (((CtxRasterizer*)(ctx->renderer))->aa)
  {
    case 1: return CTX_ANTIALIAS_NONE;
    case 3: return CTX_ANTIALIAS_FAST;
    case 5: return CTX_ANTIALIAS_GOOD;
    default:
    case 15: return CTX_ANTIALIAS_DEFAULT;
    case 17: return CTX_ANTIALIAS_BEST;
  }
}

int _ctx_antialias_to_aa (CtxAntialias antialias)
{
  switch (antialias)
  {
    case CTX_ANTIALIAS_NONE: return 1;
    case CTX_ANTIALIAS_FAST: return 3;
    case CTX_ANTIALIAS_GOOD: return 5;
    default:
    case CTX_ANTIALIAS_DEFAULT: return CTX_RASTERIZER_AA;
    case CTX_ANTIALIAS_BEST: return 17;
  }
}

void
ctx_set_antialias (Ctx *ctx, CtxAntialias antialias)
{
#if CTX_EVENTS
  if (ctx_renderer_is_sdl (ctx) || ctx_renderer_is_fb (ctx))
  {
     CtxTiled *fb = (CtxTiled*)(ctx->renderer);
     fb->antialias = antialias;
     for (int i = 0; i < _ctx_max_threads; i++)
     {
       ctx_set_antialias (fb->host[i], antialias);
     }
     return;
  }
#endif
  if (!_ctx_is_rasterizer (ctx)) return;

  ((CtxRasterizer*)(ctx->renderer))->aa = 
     _ctx_antialias_to_aa (antialias);
/* vertical level of supersampling at full/forced AA.
 *
 * 1 is none, 3 is fast 5 is good 15 or 17 is best for 8bit
 *
 * valid values:  - for other values we do not add up to 255
 * 3 5 15 17 51
 *
 */
}

CtxRasterizer *
ctx_rasterizer_init (CtxRasterizer *rasterizer, Ctx *ctx, Ctx *texture_source, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format, CtxAntialias antialias)
{
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
    ctx_buffer_free (rasterizer->clip_buffer);
#endif
  if (rasterizer->edge_list.size)
    ctx_drawlist_deinit (&rasterizer->edge_list);

  ctx_memset (rasterizer, 0, sizeof (CtxRasterizer) );
  rasterizer->vfuncs.process = ctx_rasterizer_process;
  rasterizer->vfuncs.free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  rasterizer->edge_list.flags |= CTX_DRAWLIST_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->ctx         = ctx;
  rasterizer->texture_source = texture_source?texture_source:ctx;
  rasterizer->aa          = _ctx_antialias_to_aa (antialias);
  ctx_state_init (rasterizer->state);
  rasterizer->buf         = data;
  rasterizer->blit_x      = x;
  rasterizer->blit_y      = y;
  rasterizer->blit_width  = width;
  rasterizer->blit_height = height;
  rasterizer->state->gstate.clip_min_x  = x;
  rasterizer->state->gstate.clip_min_y  = y;
  rasterizer->state->gstate.clip_max_x  = x + width - 1;
  rasterizer->state->gstate.clip_max_y  = y + height - 1;
  rasterizer->blit_stride = stride;
  rasterizer->scan_min    = 5000;
  rasterizer->scan_max    = -5000;
  rasterizer->format = ctx_pixel_format_info (pixel_format);

  return rasterizer;
}

Ctx *
ctx_new_for_buffer (CtxBuffer *buffer)
{
  Ctx *ctx = ctx_new ();
  ctx_set_renderer (ctx,
                    ctx_rasterizer_init ( (CtxRasterizer *) malloc (sizeof (CtxRasterizer) ),
                                          ctx, NULL, &ctx->state,
                                          buffer->data, 0, 0, buffer->width, buffer->height,
                                          buffer->stride, buffer->format->pixel_format,
                                          CTX_ANTIALIAS_DEFAULT));
  return ctx;
}

Ctx *
ctx_new_for_framebuffer (void *data, int width, int height,
                         int stride,
                         CtxPixelFormat pixel_format)
{
  Ctx *ctx = ctx_new ();
  CtxRasterizer *r = ctx_rasterizer_init ( (CtxRasterizer *) ctx_calloc (sizeof (CtxRasterizer), 1),
                                          ctx, NULL, &ctx->state, data, 0, 0, width, height,
                                          stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
  ctx_set_renderer (ctx, r);
  return ctx;
}

// ctx_new_for_stream (FILE *stream);

#if 0
CtxRasterizer *ctx_rasterizer_new (void *data, int x, int y, int width, int height,
                                   int stride, CtxPixelFormat pixel_format)
{
  CtxState    *state    = (CtxState *) malloc (sizeof (CtxState) );
  CtxRasterizer *rasterizer = (CtxRasterizer *) malloc (sizeof (CtxRenderer) );
  ctx_rasterizer_init (rasterizer, state, data, x, y, width, height,
                       stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
}
#endif

CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format);

#else

CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format)
{
  return NULL;
}
#endif

void
ctx_current_point (Ctx *ctx, float *x, float *y)
{
  if (!ctx)
    { 
      if (x) { *x = 0.0f; }
      if (y) { *y = 0.0f; }
    }
#if CTX_RASTERIZER
  if (ctx->renderer)
    {
      if (x) { *x = ( (CtxRasterizer *) (ctx->renderer) )->x; }
      if (y) { *y = ( (CtxRasterizer *) (ctx->renderer) )->y; }
      return;
    }
#endif
  if (x) { *x = ctx->state.x; }
  if (y) { *y = ctx->state.y; }
}

float ctx_x (Ctx *ctx)
{
  float x = 0, y = 0;
  ctx_current_point (ctx, &x, &y);
  return x;
}

float ctx_y (Ctx *ctx)
{
  float x = 0, y = 0;
  ctx_current_point (ctx, &x, &y);
  return y;
}

void
ctx_process (Ctx *ctx, CtxEntry *entry)
{
#if CTX_CURRENT_PATH
  switch (entry->code)
    {
      case CTX_TEXT:
      case CTX_STROKE_TEXT:
      case CTX_BEGIN_PATH:
        ctx->current_path.count = 0;
        break;
      case CTX_CLIP:
      case CTX_FILL:
      case CTX_STROKE:
              // XXX unless preserve
        ctx->current_path.count = 0;
        break;
      case CTX_CLOSE_PATH:
      case CTX_LINE_TO:
      case CTX_MOVE_TO:
      case CTX_QUAD_TO:
      case CTX_SMOOTH_TO:
      case CTX_SMOOTHQ_TO:
      case CTX_REL_QUAD_TO:
      case CTX_REL_SMOOTH_TO:
      case CTX_REL_SMOOTHQ_TO:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_RECTANGLE:
      case CTX_ROUND_RECTANGLE:
        ctx_drawlist_add_entry (&ctx->current_path, entry);
        break;
      default:
        break;
    }
#endif
  if (CTX_LIKELY(ctx->renderer && ctx->renderer->process))
    {
      ctx->renderer->process (ctx->renderer, (CtxCommand *) entry);
    }
  else
    {
      /* these functions might alter the code and coordinates of
         command that in the end gets added to the drawlist
       */
      ctx_interpret_style (&ctx->state, entry, ctx);
      ctx_interpret_transforms (&ctx->state, entry, ctx);
      ctx_interpret_pos (&ctx->state, entry, ctx);
      ctx_drawlist_add_entry (&ctx->drawlist, entry);
    }
}


int ctx_gradient_cache_valid = 0;

void
ctx_state_gradient_clear_stops (CtxState *state)
{
//#if CTX_GRADIENT_CACHE
//  ctx_gradient_cache_reset ();
//#endif
  ctx_gradient_cache_valid = 0;
  state->gradient.n_stops = 0;
}

uint8_t ctx_gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];
uint8_t ctx_gradient_cache_u8_a[CTX_GRADIENT_CACHE_ELEMENTS][4];

/****  end of engine ****/
