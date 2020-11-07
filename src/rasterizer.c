#include "ctx-split.h"

#if CTX_RASTERIZER

CTX_STATIC void
ctx_rasterizer_gradient_add_stop (CtxRasterizer *rasterizer, float pos, float *rgba)
{
  CtxGradient *gradient = &rasterizer->state->gradient;
  CtxGradientStop *stop = &gradient->stops[gradient->n_stops];
  stop->pos = pos;
  ctx_color_set_rgba (rasterizer->state, & (stop->color), rgba[0], rgba[1], rgba[2], rgba[3]);
  if (gradient->n_stops < 15) //we'll keep overwriting the last when out of stops
    { gradient->n_stops++; }
}

static int ctx_rasterizer_add_point (CtxRasterizer *rasterizer, int x1, int y1)
{
  int16_t args[4];
  if (y1 < rasterizer->scan_min)
    { rasterizer->scan_min = y1; }
  if (y1 > rasterizer->scan_max)
    { rasterizer->scan_max = y1; }
  if (x1 < rasterizer->col_min)
    { rasterizer->col_min = x1; }
  if (x1 > rasterizer->col_max)
    { rasterizer->col_max = x1; }
  args[0]=0;
  args[1]=0;
  args[2]=x1;
  args[3]=y1;
  return ctx_renderstream_add_u32 (&rasterizer->edge_list, CTX_EDGE, (uint32_t *) args);
}

#define CTX_SHAPE_CACHE_PRIME1   7853
#define CTX_SHAPE_CACHE_PRIME2   4129
#define CTX_SHAPE_CACHE_PRIME3   3371
#define CTX_SHAPE_CACHE_PRIME4   4221

float ctx_shape_cache_rate = 0.0;
#if CTX_SHAPE_CACHE

static uint32_t ctx_shape_time = 0;

/* to get better cache usage-  */

struct _CtxShapeEntry
{
  uint32_t hash;
  uint16_t width;
  uint16_t height;
  uint32_t refs;
  uint32_t age;   // time last used
  uint32_t uses;  // instrumented for longer keep-alive
  uint8_t  data[];
};

typedef struct _CtxShapeEntry CtxShapeEntry;


// this needs a max-size
// and a more agressive freeing when
// size is about to be exceeded

struct _CtxShapeCache
{
  CtxShapeEntry *entries[CTX_SHAPE_CACHE_ENTRIES];
  long size;
};

typedef struct _CtxShapeCache CtxShapeCache;

static ctx_mutex_t  *ctx_shape_cache_mutex;
static CtxShapeCache ctx_cache = {{NULL,}, 0};

static long hits = 0;
static long misses = 0;


/* this returns the buffer to use for rendering, it always
   succeeds..
 */
static CtxShapeEntry *ctx_shape_entry_find (uint32_t hash, int width, int height, uint32_t time)
{
  int entry_no = ( (hash >> 10) ^ (hash & 1023) ) % CTX_SHAPE_CACHE_ENTRIES;
  int i;
  {
    static int i = 0;
    i++;
    if (i>512)
      {
        ctx_shape_cache_rate = hits * 100.0  / (hits+misses);
        i = 0;
        hits = 0;
        misses = 0;
      }
  }
  i = entry_no;
  if (ctx_cache.entries[i])
    {
      if (ctx_cache.entries[i]->hash == hash &&
          ctx_cache.entries[i]->width == width &&
          ctx_cache.entries[i]->height == height)
        {
          ctx_cache.entries[i]->refs++;
          ctx_cache.entries[i]->age = time;
          if (ctx_cache.entries[i]->uses < 1<<30)
            { ctx_cache.entries[i]->uses++; }
          hits ++;
          return ctx_cache.entries[i];
        }
#if 0
      else if (i < CTX_SHAPE_CACHE_ENTRIES-2)
        {
          if (ctx_cache.entries[i+1])
            {
              if (ctx_cache.entries[i+1]->hash == hash &&
                  ctx_cache.entries[i+1]->width == width &&
                  ctx_cache.entries[i+1]->height == height)
                {
                  ctx_cache.entries[i+1]->refs++;
                  ctx_cache.entries[i+1]->age = time;
                  if (ctx_cache.entries[i+1]->uses < 1<<30)
                    { ctx_cache.entries[i+1]->uses++; }
                  hits ++;
                  return ctx_cache.entries[i+1];
                }
              else if (i < CTX_SHAPE_CACHE_ENTRIES-3)
                {
                  if (ctx_cache.entries[i+2])
                    {
                      if (ctx_cache.entries[i+2]->hash == hash &&
                          ctx_cache.entries[i+2]->width == width &&
                          ctx_cache.entries[i+2]->height == height)
                        {
                          ctx_cache.entries[i+2]->refs++;
                          ctx_cache.entries[i+2]->age = time;
                          if (ctx_cache.entries[i+2]->uses < 1<<30)
                            { ctx_cache.entries[i+2]->uses++; }
                          hits ++;
                          return ctx_cache.entries[i+2];
                        }
                    }
                  else
                    {
                      i+=2;
                    }
                }
            }
          else
            {
              i++;
            }
        }
#endif
    }

  if (!ctx_shape_cache_mutex)
  {
    // racy - but worst that happens is visual glitches
    ctx_shape_cache_mutex = ctx_create_mutex ();
  }
  // lock shape cache mutex
  ctx_lock_mutex (ctx_shape_cache_mutex);

  misses ++;
// XXX : this 1 one is needed  to silence a false positive:
// ==90718== Invalid write of size 1
// ==90718==    at 0x1189EF: ctx_rasterizer_generate_coverage (ctx.h:4786)
// ==90718==    by 0x118E57: ctx_rasterizer_rasterize_edges (ctx.h:4907)
//
  int size = sizeof (CtxShapeEntry) + width * height + 1;
  CtxShapeEntry *new_entry = (CtxShapeEntry *) malloc (size);
  new_entry->refs = 1;
  if (ctx_cache.entries[i])
    {
      CtxShapeEntry *entry = ctx_cache.entries[i];
      while (entry->refs) {};
      ctx_cache.entries[i] = new_entry;
      ctx_cache.size -= entry->width * entry->height;
      ctx_cache.size -= sizeof (CtxShapeEntry);
      free (entry);
    }
  else
    {
      ctx_cache.entries[i] = new_entry;
    }
  ctx_cache.size += size;
  ctx_cache.entries[i]->age = time;
  ctx_cache.entries[i]->hash=hash;
  ctx_cache.entries[i]->width=width;
  ctx_cache.entries[i]->height=height;
  ctx_cache.entries[i]->uses = 0;
  ctx_unlock_mutex (ctx_shape_cache_mutex);
  return ctx_cache.entries[i];
}

static void ctx_shape_entry_release (CtxShapeEntry *entry)
{
  entry->refs--;
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
  int aa = rasterizer->aa;
  CtxEntry *entry = &rasterizer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = (ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += (oy % aa);
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

CTX_STATIC void ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y);

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
  float tx; float ty;
  int aa = rasterizer->aa;
  rasterizer->x        = x;
  rasterizer->y        = y;
  rasterizer->first_x  = x;
  rasterizer->first_y  = y;
  rasterizer->has_prev = -1;

  tx = (x - rasterizer->blit_x) * CTX_SUBDIV;
  ty = y * aa;
  if (ty < rasterizer->scan_min)
    { rasterizer->scan_min = ty; }
  if (ty > rasterizer->scan_max)
    { rasterizer->scan_max = ty; }
  if (tx < rasterizer->col_min)
    { rasterizer->col_min = tx; }
  if (tx > rasterizer->col_max)
    { rasterizer->col_max = tx; }
}

CTX_STATIC void ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx = x;
  float ty = y;
  float ox = rasterizer->x;
  float oy = rasterizer->y;
  if (rasterizer->uses_transforms)
    {
      _ctx_user_to_device (rasterizer->state, &tx, &ty);
    }
  tx -= rasterizer->blit_x;
#define MIN_Y -1000
#define MAX_Y 1400


  if (ty < MIN_Y) ty = MIN_Y;
  if (ty > MAX_Y) ty = MAX_Y;
  ctx_rasterizer_add_point (rasterizer, tx * CTX_SUBDIV, ty * rasterizer->aa);
  if (rasterizer->has_prev<=0)
    {
      if (rasterizer->uses_transforms)
      {
        // storing transformed would save some processing for a tiny
        // amount of runtime RAM XXX
        _ctx_user_to_device (rasterizer->state, &ox, &oy);
      }
      ox -= rasterizer->blit_x;

  if (oy < MIN_Y) oy = MIN_Y;
  if (oy > MAX_Y) oy = MAX_Y;

      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].data.s16[0] = ox * CTX_SUBDIV;
      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].data.s16[1] = oy * rasterizer->aa;
      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].code = CTX_NEW_EDGE;
      rasterizer->has_prev = 1;
    }
  rasterizer->has_shape = 1;
  rasterizer->y         = y;
  rasterizer->x         = x;
}

CTX_INLINE static float
ctx_lerpf (float v0, float v1, float dx)
{
  return v0 + (v1-v0) * dx;
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

static void
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
  if (iteration > 8)
    { return; }
  float t = (s + e) * 0.5f;
  float x, y, lx, ly, dx, dy;
  ctx_bezier_sample (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);
  if (iteration)
    {
      lx = ctx_lerpf (sx, ex, t);
      ly = ctx_lerpf (sy, ey, t);
      dx = lx - x;
      dy = ly - y;
      if ( (dx*dx+dy*dy) < tolerance)
        /* bailing - because for the mid-point straight line difference is
           tiny */
        { return; }
      dx = sx - ex;
      dy = ey - ey;
      if ( (dx*dx+dy*dy) < tolerance)
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
  if (tolerance == 1.0f)
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
    if(
        (minx > rasterizer->blit_x + rasterizer->blit_width) ||
        (miny > rasterizer->blit_y + rasterizer->blit_height) ||
        (maxx < rasterizer->blit_x) ||
        (maxy < rasterizer->blit_y) )
    {
      // tolerance==1.0 is most likely screen-space -
      // skip subdivides for things outside
    }
    else
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
  if (x == 0.f && y == 0.f)
    { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_move_to (rasterizer, x, y);
}

CTX_STATIC void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  if (x== 0.f && y==0.f)
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

static void ctx_edge_qsort (CtxEntry *entries, int low, int high)
{
  {
    int p = ctx_edge_qsort_partition (entries, low, high);
    if (low < p -1 )
      { ctx_edge_qsort (entries, low, p - 1); }
    if (low < high)
      { ctx_edge_qsort (entries, p, high); }
  }
}

static void ctx_rasterizer_sort_edges (CtxRasterizer *rasterizer)
{
  if (rasterizer->edge_list.count > 1)
    {
      ctx_edge_qsort (& (rasterizer->edge_list.entries[0]), 0, rasterizer->edge_list.count-1);
    }
}

static void ctx_rasterizer_discard_edges (CtxRasterizer *rasterizer)
{
#if CTX_RASTERIZER_FORCE_AA==0
  rasterizer->ending_edges = 0;
#endif
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      int edge_end =rasterizer->edge_list.entries[rasterizer->edges[i].index].data.s16[3];
      if (edge_end < rasterizer->scanline)
        {
          int dx_dy = rasterizer->edges[i].dx;
          if (abs(dx_dy)> CTX_RASTERIZER_AA_SLOPE_LIMIT)
            { rasterizer->needs_aa --; }
          rasterizer->edges[i] = rasterizer->edges[rasterizer->active_edges-1];
          rasterizer->active_edges--;
          i--;
        }
#if CTX_RASTERIZER_FORCE_AA==0
      else if (edge_end < rasterizer->scanline + rasterizer->aa)
        rasterizer->ending_edges = 1;
#endif
    }
}

static void ctx_rasterizer_increment_edges (CtxRasterizer *rasterizer, int count)
{
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      rasterizer->edges[i].x += rasterizer->edges[i].dx * count;
    }
#if CTX_RASTERIZER_FORCE_AA==0
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      rasterizer->edges[CTX_MAX_EDGES-1-i].x += rasterizer->edges[CTX_MAX_EDGES-1-i].dx * count;
    }
#endif
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
#if CTX_RASTERIZER_FORCE_AA==0
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      if (entries[rasterizer->edges[CTX_MAX_EDGES-1-i].index].data.s16[1] <= rasterizer->scanline)
        {
          if (rasterizer->active_edges < CTX_MAX_EDGES-2)
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
#endif
  while (rasterizer->edge_pos < rasterizer->edge_list.count &&
         (miny=entries[rasterizer->edge_pos].data.s16[1]) <= rasterizer->scanline 
#if CTX_RASTERIZER_FORCE_AA==0
         + rasterizer->aa
#endif
         
         )
    {
      if (rasterizer->active_edges < CTX_MAX_EDGES-2)
        {
          int dy = (entries[rasterizer->edge_pos].data.s16[3] -
                    miny);
          if (dy) /* skipping horizontal edges */
            {
              int yd = rasterizer->scanline - miny;
              int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              rasterizer->edges[no].index = rasterizer->edge_pos;
              int index = rasterizer->edges[no].index;
              int x0 = entries[index].data.s16[0];
              int x1 = entries[index].data.s16[2];
              rasterizer->edges[no].x = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER;
              int dx_dy;
              //  if (dy)
              dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
              //  else
              //  dx_dy = 0;
              rasterizer->edges[no].dx = dx_dy;
              rasterizer->edges[no].x += (yd * dx_dy);
              // XXX : even better minx and maxx can
              //       be derived using y0 and y1 for scaling dx_dy
              //       when ydelta to these are smaller than
              //       ydelta to scanline
#if 0
              if (dx_dy < 0)
                {
                  rasterizer->edges[no].minx =
                    rasterizer->edges[no].x + dx_dy/2;
                  rasterizer->edges[no].maxx =
                    rasterizer->edges[no].x - dx_dy/2;
                }
              else
                {
                  rasterizer->edges[no].minx =
                    rasterizer->edges[no].x - dx_dy/2;
                  rasterizer->edges[no].maxx =
                    rasterizer->edges[no].x + dx_dy/2;
                }
#endif
#if CTX_RASTERIZER_FORCE_AA==0
              if (abs(dx_dy)> CTX_RASTERIZER_AA_SLOPE_LIMIT)
                { rasterizer->needs_aa ++; }

              if ((miny > rasterizer->scanline) )
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
#endif
            }
        }
      rasterizer->edge_pos++;
    }
}

CTX_INLINE static int ctx_compare_edges2 (const void *ap, const void *bp)
{
  const CtxEdge *a = (const CtxEdge *) ap;
  const CtxEdge *b = (const CtxEdge *) bp;
  return a->x - b->x;
}

CTX_INLINE static int ctx_edge2_qsort_partition (CtxEdge *A, int low, int high)
{
  CtxEdge pivot = A[ (high+low) /2];
  int i = low;
  int j = high;
  while (i <= j)
    {
      while (ctx_compare_edges2 (&A[i], &pivot) <0) { i ++; }
      while (ctx_compare_edges2 (&pivot, &A[j]) <0) { j --; }
      if (i <= j)
        {
          CtxEdge tmp = A[i];
          A[i] = A[j];
          A[j] = tmp;
          i++;
          j--;
        }
    }
  return i;
}

static void ctx_edge2_qsort (CtxEdge *entries, int low, int high)
{
  {
    int p = ctx_edge2_qsort_partition (entries, low, high);
    if (low < p -1 )
      { ctx_edge2_qsort (entries, low, p - 1); }
    if (low < high)
      { ctx_edge2_qsort (entries, p, high); }
  }
}


static void ctx_rasterizer_sort_active_edges (CtxRasterizer *rasterizer)
{
  CtxEdge *edges = rasterizer->edges;
  /* we use sort networks for the very frequent cases of few active edges
   * the built in qsort is fast, but sort networks are even faster
   */
  switch (rasterizer->active_edges)
  {
    case 0:
    case 1: break;
#if CTX_BLOATY_FAST_PATHS
    case 2:
#define COMPARE(a,b) \
      if (ctx_compare_edges2 (&edges[a], &edges[b])>0)\
      {\
        CtxEdge tmp = edges[a];\
        edges[a] = edges[b];\
        edges[b] = tmp;\
      }
      COMPARE(0,1);
      break;
    case 3:
      COMPARE(0,1); COMPARE(0,2); COMPARE(1,2);
      break;
    case 4:
      COMPARE(0,1); COMPARE(2,3); COMPARE(0,2); COMPARE(1,3); COMPARE(1,2);
      break;
    case 5:
      COMPARE(1,2); COMPARE(0,2); COMPARE(0,1); COMPARE(3,4); COMPARE(0,3);
      COMPARE(1,4); COMPARE(2,4); COMPARE(1,3); COMPARE(2,3);
      break;
    case 6:
      COMPARE(1,2); COMPARE(0,2); COMPARE(0,1); COMPARE(4,5);
      COMPARE(3,5); COMPARE(3,4); COMPARE(0,3); COMPARE(1,4);
      COMPARE(2,5); COMPARE(2,4); COMPARE(1,3); COMPARE(2,3);
      break;
#endif
    default:
      ctx_edge2_qsort (&edges[0], 0, rasterizer->active_edges-1);
      break;
  }
}

CTX_INLINE static uint8_t ctx_lerp_u8 (uint8_t v0, uint8_t v1, uint8_t dx)
{
#if 0
  return v0 + ((v1-v0) * dx)/255;
#else
  return ( ( ( ( (v0) <<8) + (dx) * ( (v1) - (v0) ) ) ) >>8);
#endif
}

#define CTX_RGBA8_R_SHIFT  0
#define CTX_RGBA8_G_SHIFT  8
#define CTX_RGBA8_B_SHIFT  16
#define CTX_RGBA8_A_SHIFT  24

#define CTX_RGBA8_R_MASK   (0xff << CTX_RGBA8_R_SHIFT)
#define CTX_RGBA8_G_MASK   (0xff << CTX_RGBA8_G_SHIFT)
#define CTX_RGBA8_B_MASK   (0xff << CTX_RGBA8_B_SHIFT)
#define CTX_RGBA8_A_MASK   (0xff << CTX_RGBA8_A_SHIFT)

#define CTX_RGBA8_RB_MASK  (CTX_RGBA8_R_MASK | CTX_RGBA8_B_MASK)
#define CTX_RGBA8_GA_MASK  (CTX_RGBA8_G_MASK | CTX_RGBA8_A_MASK)

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE


#define CTX_GRADIENT_CACHE_ELEMENTS 256

inline static int ctx_grad_index (float v)
{
  int ret = v * (CTX_GRADIENT_CACHE_ELEMENTS - 1.0f) + 0.5f;
  if (ret >= CTX_GRADIENT_CACHE_ELEMENTS)
    return CTX_GRADIENT_CACHE_ELEMENTS - 1;
  if (ret >= 0 && ret < CTX_GRADIENT_CACHE_ELEMENTS)
    return ret;
  return 0;
}

static uint8_t ctx_gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];
static uint8_t ctx_gradient_cache_u8_a[CTX_GRADIENT_CACHE_ELEMENTS][4];
static int ctx_gradient_cache_valid = 0;

void
ctx_gradient_cache_reset (void)
{
  ctx_gradient_cache_valid = 0;
}


#endif
void
ctx_state_gradient_clear_stops (CtxState *state)
{
#if CTX_GRADIENT_CACHE
  ctx_gradient_cache_reset ();
#endif
  state->gradient.n_stops = 0;
}

CTX_INLINE static void
_ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
  float v = x;
  CtxGradient *g = &rasterizer->state->gradient;
  if (v < 0) { v = 0; }
  if (v > 1) { v = 1; }

  if (g->n_stops == 0)
    {
      rgba[0] = rgba[1] = rgba[2] = v * 255;
      rgba[3] = 255;
      return;
    }
  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];
  CtxColor *color;
  for (int s = 0; s < g->n_stops; s++)
    {
      stop      = &g->stops[s];
      next_stop = &g->stops[s+1];
      if (s + 1 >= g->n_stops) { next_stop = NULL; }
      if (v >= stop->pos && next_stop && v < next_stop->pos)
        { break; }
      stop = NULL;
      next_stop = NULL;
    }
  if (stop == NULL && next_stop)
    {
      color = & (next_stop->color);
    }
  else if (stop && next_stop == NULL)
    {
      color = & (stop->color);
    }
  else if (stop && next_stop)
    {
      uint8_t stop_rgba[4];
      uint8_t next_rgba[4];
      ctx_color_get_rgba8 (rasterizer->state, & (stop->color), stop_rgba);
      ctx_color_get_rgba8 (rasterizer->state, & (next_stop->color), next_rgba);
      int dx = (v - stop->pos) * 255 / (next_stop->pos - stop->pos);
      for (int c = 0; c < 4; c++)
        { rgba[c] = ctx_lerp_u8 (stop_rgba[c], next_rgba[c], dx); }
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_rgba8 (rasterizer->state, color, rgba);
}


#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer);
#endif

CTX_INLINE static void
ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
#if CTX_GRADIENT_CACHE
  *((uint32_t*)rgba) = *((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(x)][0]));
#else
 _ctx_fragment_gradient_1d_RGBA8 (rasterizer, x, y, rgba);
#endif
}
#endif


CTX_INLINE static void
ctx_RGBA8_associate_alpha (uint8_t *u8)
{
            {
    uint32_t val = *((uint32_t*)(u8));
    int a = val >> CTX_RGBA8_A_SHIFT;
    if (a!=255)
    {
      if (a)
      {
        uint32_t g = (((val & CTX_RGBA8_G_MASK) * a) >> 8) & CTX_RGBA8_G_MASK;
        uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * a) >> 8) & CTX_RGBA8_RB_MASK;
        *((uint32_t*)(u8)) = g|rb|(a << CTX_RGBA8_A_SHIFT);
      }
      else
      {
        *((uint32_t*)(u8)) = 0;
      }
    }
  }
}


CTX_INLINE static void
ctx_u8_associate_alpha (int components, uint8_t *u8)
{
  switch (u8[components-1])
  {
          case 255:break;
          case 0: 
            for (int c = 0; c < components-1; c++)
             u8[c] = 0;
            break;
          default:
  for (int c = 0; c < components-1; c++)
    u8[c] = (u8[c] * u8[components-1]) /255;
  }
}

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer)
{
  if (ctx_gradient_cache_valid)
    return;
  for (int u = 0; u < CTX_GRADIENT_CACHE_ELEMENTS; u++)
  {
    float v = u / (CTX_GRADIENT_CACHE_ELEMENTS - 1.0f);
    _ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0f, &ctx_gradient_cache_u8[u][0]);
    *((uint32_t*)(&ctx_gradient_cache_u8_a[u][0]))= *((uint32_t*)(&ctx_gradient_cache_u8[u][0]));
    ctx_RGBA8_associate_alpha (&ctx_gradient_cache_u8_a[u][0]);
  }
  ctx_gradient_cache_valid = 1;
}
#endif

CTX_INLINE static void
ctx_fragment_gradient_1d_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
  float v = x;
  CtxGradient *g = &rasterizer->state->gradient;
  if (v < 0) { v = 0; }
  if (v > 1) { v = 1; }
  if (g->n_stops == 0)
    {
      rgba[0] = rgba[1] = rgba[2] = v * 255;
      rgba[1] = 255;
      return;
    }
  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];
  CtxColor *color;
  for (int s = 0; s < g->n_stops; s++)
    {
      stop      = &g->stops[s];
      next_stop = &g->stops[s+1];
      if (s + 1 >= g->n_stops) { next_stop = NULL; }
      if (v >= stop->pos && next_stop && v < next_stop->pos)
        { break; }
      stop = NULL;
      next_stop = NULL;
    }
  if (stop == NULL && next_stop)
    {
      color = & (next_stop->color);
    }
  else if (stop && next_stop == NULL)
    {
      color = & (stop->color);
    }
  else if (stop && next_stop)
    {
      uint8_t stop_rgba[4];
      uint8_t next_rgba[4];
      ctx_color_get_graya_u8 (rasterizer->state, & (stop->color), stop_rgba);
      ctx_color_get_graya_u8 (rasterizer->state, & (next_stop->color), next_rgba);
      int dx = (v - stop->pos) * 255 / (next_stop->pos - stop->pos);
      for (int c = 0; c < 2; c++)
        { rgba[c] = ctx_lerp_u8 (stop_rgba[c], next_rgba[c], dx); }
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_graya_u8 (rasterizer->state, color, rgba);
}

CTX_INLINE static void
ctx_fragment_gradient_1d_RGBAF (CtxRasterizer *rasterizer, float v, float y, float *rgba)
{
  CtxGradient *g = &rasterizer->state->gradient;
  if (v < 0) { v = 0; }
  if (v > 1) { v = 1; }
  if (g->n_stops == 0)
    {
      rgba[0] = rgba[1] = rgba[2] = v;
      rgba[3] = 1.0;
      return;
    }
  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];
  CtxColor *color;
  for (int s = 0; s < g->n_stops; s++)
    {
      stop      = &g->stops[s];
      next_stop = &g->stops[s+1];
      if (s + 1 >= g->n_stops) { next_stop = NULL; }
      if (v >= stop->pos && next_stop && v < next_stop->pos)
        { break; }
      stop = NULL;
      next_stop = NULL;
    }
  if (stop == NULL && next_stop)
    {
      color = & (next_stop->color);
    }
  else if (stop && next_stop == NULL)
    {
      color = & (stop->color);
    }
  else if (stop && next_stop)
    {
      float stop_rgba[4];
      float next_rgba[4];
      ctx_color_get_rgba (rasterizer->state, & (stop->color), stop_rgba);
      ctx_color_get_rgba (rasterizer->state, & (next_stop->color), next_rgba);
      int dx = (v - stop->pos) / (next_stop->pos - stop->pos);
      for (int c = 0; c < 4; c++)
        { rgba[c] = ctx_lerpf (stop_rgba[c], next_rgba[c], dx); }
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_rgba (rasterizer->state, color, rgba);
}
#endif

static void
ctx_fragment_image_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);
  int u = x - g->image.x0;
  int v = y - g->image.y0;
  if ( u < 0 || v < 0 ||
       u >= buffer->width ||
       v >= buffer->height)
    {
      *((uint32_t*)(rgba)) = 0;
    }
  else
    {
      int bpp = buffer->format->bpp/8;
      uint8_t *src = (uint8_t *) buffer->data;
      src += v * buffer->stride + u * bpp;
      switch (bpp)
        {
          case 1:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[0]; }
            rgba[3] = 255;
            break;
          case 2:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[0]; }
            rgba[3] = src[1];
            break;
          case 3:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[c]; }
            rgba[3] = 255;
            break;
          case 4:
            for (int c = 0; c < 4; c++)
              { rgba[c] = src[c]; }
            break;
        }
    }
}

#if CTX_DITHER
static inline int ctx_dither_mask_a (int x, int y, int c, int divisor)
{
  /* https://pippin.gimp.org/a_dither/ */
  return ( ( ( ( (x + c * 67) + y * 236) * 119) & 255 )-127) / divisor;
}

inline static void
ctx_dither_rgba_u8 (uint8_t *rgba, int x, int y, int dither_red_blue, int dither_green)
{
  if (dither_red_blue == 0)
    { return; }
  for (int c = 0; c < 3; c ++)
    {
      int val = rgba[c] + ctx_dither_mask_a (x, y, 0, c==1?dither_green:dither_red_blue);
      rgba[c] = CTX_CLAMP (val, 0, 255);
    }
}

inline static void
ctx_dither_graya_u8 (uint8_t *rgba, int x, int y, int dither_red_blue, int dither_green)
{
  if (dither_red_blue == 0)
    { return; }
  for (int c = 0; c < 1; c ++)
    {
      int val = rgba[c] + ctx_dither_mask_a (x, y, 0, dither_red_blue);
      rgba[c] = CTX_CLAMP (val, 0, 255);
    }
}
#endif

CTX_INLINE static void
ctx_RGBA8_deassociate_alpha (const uint8_t *in, uint8_t *out)
{
    uint32_t val = *((uint32_t*)(in));
    int a = val >> CTX_RGBA8_A_SHIFT;
    if (a)
    {
    if (a ==255)
    {
      *((uint32_t*)(out)) = val;
    } else
    {
      uint32_t g = (((val & CTX_RGBA8_G_MASK) * 255 / a) >> 8) & CTX_RGBA8_G_MASK;
      uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * 255 / a) >> 8) & CTX_RGBA8_RB_MASK;
      *((uint32_t*)(out)) = g|rb|(a << CTX_RGBA8_A_SHIFT);
    }
    }
    else
    {
      *((uint32_t*)(out)) = 0;
    }
}

CTX_INLINE static void
ctx_u8_deassociate_alpha (int components, const uint8_t *in, uint8_t *out)
{
  if (in[components-1])
  {
    if (in[components-1] != 255)
    for (int c = 0; c < components-1; c++)
      out[c] = (in[c] * 255) / in[components-1];
    else
    for (int c = 0; c < components-1; c++)
      out[c] = in[c];
    out[components-1] = in[components-1];
  }
  else
  {
  for (int c = 0; c < components; c++)
    out[c] = 0;
  }
}

CTX_INLINE static void
ctx_float_associate_alpha (int components, float *rgba)
{
  float alpha = rgba[components-1];
  for (int c = 0; c < components-1; c++)
    rgba[c] *= alpha;
}

CTX_INLINE static void
ctx_float_deassociate_alpha (int components, float *rgba, float *dst)
{
  float ralpha = rgba[components-1];
  if (ralpha != 0.0) ralpha = 1.0/ralpha;

  for (int c = 0; c < components-1; c++)
    dst[c] = (rgba[c] * ralpha);
  dst[components-1] = rgba[components-1];
}

CTX_INLINE static void
ctx_RGBAF_associate_alpha (float *rgba)
{
  ctx_float_associate_alpha (4, rgba);
}

CTX_INLINE static void
ctx_RGBAF_deassociate_alpha (float *rgba, float *dst)
{
  ctx_float_deassociate_alpha (4, rgba, dst);
}

static void
ctx_fragment_image_rgba8_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);
  int u = x - g->image.x0;
  int v = y - g->image.y0;
  if ( u < 0 || v < 0 ||
       u >= buffer->width ||
       v >= buffer->height)
    {
      rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
    }
  else
    {
      int bpp = 4;
      uint8_t *src = (uint8_t *) buffer->data;
      src += v * buffer->stride + u * bpp;
      for (int c = 0; c < 4; c++)
        { rgba[c] = src[c]; }
    }
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_gray1_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);
  int u = x - g->image.x0;
  int v = y - g->image.y0;
  if ( u < 0 || v < 0 ||
       u >= buffer->width ||
       v >= buffer->height)
    {
      rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
    }
  else
    {
      uint8_t *src = (uint8_t *) buffer->data;
      src += v * buffer->stride + u / 8;
      if (*src & (1<< (u & 7) ) )
        {
          rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
        }
      else
        {
          for (int c = 0; c < 4; c++)
            { rgba[c] = g->image.rgba[c]; }
        }
    }
}

static void
ctx_fragment_image_rgb8_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);
  int u = x - g->image.x0;
  int v = y - g->image.y0;
  if ( (u < 0) || (v < 0) ||
       (u >= buffer->width-1) ||
       (v >= buffer->height-1) )
    {
      rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
    }
  else
    {
      int bpp = 3;
      uint8_t *src = (uint8_t *) buffer->data;
      src += v * buffer->stride + u * bpp;
      for (int c = 0; c < 3; c++)
        { rgba[c] = src[c]; }
      rgba[3] = 255;
    }
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

#if CTX_GRADIENTS
static void
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = (ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
#if CTX_GRADIENT_CACHE
  *((uint32_t*)rgba) = *((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(v)][0]));
#else
  ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, rgba);
#endif
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_linear_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
#if CTX_GRADIENT_CACHE
  *((uint32_t*)rgba) = *((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(v)][0]));
#else
  _ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 1.0, rgba);
#endif
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

#endif

static void
ctx_fragment_color_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  ctx_color_get_rgba8 (rasterizer->state, &g->color, rgba);
}

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0f, rgba);
}

static void
ctx_fragment_radial_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
        v = (v - g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 0.0f, rgba);
}
#endif


static void
ctx_fragment_color_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  ctx_color_get_rgba (rasterizer->state, &g->color, rgba);
}

static void ctx_fragment_image_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *outf = (float *) out;
  uint8_t rgba[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (buffer->format->bpp)
    {
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba); break;
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba);       break;
    }
  for (int c = 0; c < 4; c ++) { outf[c] = ctx_u8_to_float (rgba[c]); }
}

static CtxFragment ctx_rasterizer_get_fragment_RGBAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_IMAGE:           return ctx_fragment_image_RGBAF;
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_RGBAF;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_RGBAF;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_RGBAF;
#endif
    }
  return ctx_fragment_color_RGBAF;
}

static CtxFragment ctx_rasterizer_get_fragment_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_IMAGE:
        switch (buffer->format->bpp)
          {
            case 1:  return ctx_fragment_image_gray1_RGBA8;
            case 24: return ctx_fragment_image_rgb8_RGBA8;
            case 32: return ctx_fragment_image_rgba8_RGBA8;
            default: return ctx_fragment_image_RGBA8;
          }
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_RGBA8;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_RGBA8;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_RGBA8;
#endif
    }
  return ctx_fragment_color_RGBA8;
}

static void
ctx_init_uv (CtxRasterizer *rasterizer,
             int x0, int count,
             float *u0, float *v0, float *ud, float *vd)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  *u0 = x0;
  *v0 = rasterizer->scanline / rasterizer->aa;
  float u1 = *u0 + count;
  float v1 = *v0;

  ctx_matrix_apply_transform (&gstate->source.transform, u0, v0);
  ctx_matrix_apply_transform (&gstate->source.transform, &u1, &v1);

  *ud = (u1-*u0) / (count);
  *vd = (v1-*v0) / (count);
}

#if 0
static void
ctx_u8_source_over_normal_opaque_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  while (count--)
  {
    int cov = *coverage;
    if (cov)
    {
    if (cov == 255)
    {
        switch (components)
        {
          case 4:
            *((uint32_t*)(dst)) = *((uint32_t*)(src));
            break;
          default:
            for (int c = 0; c < components; c++)
              dst[c] = src[c];
        }
    }
    else
    {
        for (int c = 0; c < components; c++)
          dst[c] = dst[c]+((src[c]-dst[c]) * cov) / 255;
    }
    }
    coverage ++;
    dst+=components;
  }
}
#endif

static void
ctx_u8_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  if (rasterizer->fragment)
    {
      ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
    }

  while (count--)
  {
    int cov = *coverage;
    if (cov == 0)
    {
      for (int c = 0; c < components; c++)
        { dst[c] = 0; }
    }
    else
    {
      if (rasterizer->fragment)
      {
        rasterizer->fragment (rasterizer, u0, v0, src);
        u0+=ud;
        v0+=vd;
      }
    if (cov == 255)
    {
      for (int c = 0; c < components; c++)
        dst[c] = src[c];
    }
    else
    {
      uint8_t ralpha = 255 - cov;
      for (int c = 0; c < components; c++)
        { dst[c] = (src[c]*cov + 0 * ralpha) / 255; }
    }
    }
    dst += components;
    coverage ++;
  }
}

static void
ctx_u8_clear_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  while (count--)
  {
#if 0
    int cov = *coverage;
    if (cov)
    {
      if (cov == 255)
      {
#endif
        switch (components)
        {
          case 1: dst[0] = 0; break;
          case 3: dst[2] = 0;
          case 2: *((uint16_t*)(dst)) = 0; break;
          case 5: dst[4] = 0;
          case 4: *((uint32_t*)(dst)) = 0; break;
          default:
            for (int c = 0; c < components; c ++)
              dst[c] = 0;
            break;
        }
#if 0
      }
      else
      {
        uint8_t ralpha = 255 - cov;
        for (int c = 0; c < components; c++)
          { dst[c] = (dst[c] * ralpha) / 255; }
      }
    }
    coverage ++;
#endif
    dst += components;
  }
}

typedef enum {
  CTX_PORTER_DUFF_0,
  CTX_PORTER_DUFF_1,
  CTX_PORTER_DUFF_ALPHA,
  CTX_PORTER_DUFF_1_MINUS_ALPHA,
} CtxPorterDuffFactor;

#define  \
ctx_porter_duff_factors(mode, foo, bar)\
{\
  switch (mode)\
  {\
     case CTX_COMPOSITE_SOURCE_ATOP:\
        f_s = CTX_PORTER_DUFF_ALPHA;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
      break;\
     case CTX_COMPOSITE_DESTINATION_ATOP:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_ALPHA;\
      break;\
     case CTX_COMPOSITE_DESTINATION_IN:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_ALPHA;\
      break;\
     case CTX_COMPOSITE_DESTINATION:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_1;\
       break;\
     case CTX_COMPOSITE_SOURCE_OVER:\
        f_s = CTX_PORTER_DUFF_1;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
       break;\
     case CTX_COMPOSITE_DESTINATION_OVER:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_1;\
       break;\
     case CTX_COMPOSITE_XOR:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
       break;\
     case CTX_COMPOSITE_DESTINATION_OUT:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
       break;\
     case CTX_COMPOSITE_SOURCE_OUT:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
     case CTX_COMPOSITE_SOURCE_IN:\
        f_s = CTX_PORTER_DUFF_ALPHA;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
     case CTX_COMPOSITE_COPY:\
        f_s = CTX_PORTER_DUFF_1;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
     default:\
     case CTX_COMPOSITE_CLEAR:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
  }\
}

#if 0
static void
ctx_u8_source_over_normal_color (int components,
                                 CtxRasterizer         *rasterizer,
                                 uint8_t * __restrict__ dst,
                                 uint8_t * __restrict__ src,
                                 int                    x0,
                                 uint8_t * __restrict__ coverage,
                                 int                    count)
{
  uint8_t tsrc[5];
  *((uint32_t*)tsrc) = *((uint32_t*)src);
  ctx_u8_associate_alpha (components, tsrc);

    while (count--)
    {
      int cov = *coverage;
      if (cov)
      {
        if (cov == 255)
        {
        for (int c = 0; c < components; c++)
          dst[c] = (tsrc[c]) + (dst[c] * (255-(tsrc[components-1])))/(255);
        }
        else
        {
          for (int c = 0; c < components; c++)
            dst[c] = (tsrc[c] * cov)/255 + (dst[c] * ((255*255)-(tsrc[components-1] * cov)))/(255*255);
         }
      }
      coverage ++;
      dst+=components;
    }
}
#endif

#if CTX_AVX2
#define lo_mask   _mm256_set1_epi32 (0x00FF00FF)
#define hi_mask   _mm256_set1_epi32 (0xFF00FF00)
#define x00ff     _mm256_set1_epi16(255)
#define x0101     _mm256_set1_epi16(0x0101)
#define x0080     _mm256_set1_epi16(0x0080)

#include <stdalign.h>
#endif

#if CTX_GRADIENTS
#if CTX_INLINED_GRADIENTS
static void
ctx_RGBA8_source_over_normal_linear_gradient (CTX_COMPOSITE_ARGUMENTS)
{
  CtxSource *g = &rasterizer->state->gstate.source;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
  float linear_gradient_dx = g->linear_gradient.dx;
  float linear_gradient_dy = g->linear_gradient.dy;
  float linear_gradient_rdelta = g->linear_gradient.rdelta;
  float linear_gradient_start = g->linear_gradient.start;
  float linear_gradient_length = g->linear_gradient.length;
#if CTX_DITHER
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green = rasterizer->format->dither_green;
#endif
#if CTX_AVX2
    alignas(32)
#endif
    uint8_t tsrc[4 * 8];
    //*((uint32_t*)(tsrc)) = *((uint32_t*)(src));
    //ctx_RGBA8_associate_alpha (tsrc);
    //uint8_t a = src[3];
    int x = 0;

#if CTX_AVX2
    if ((size_t)(dst) & 31)
#endif
    {
    {
      for (; (x < count) 
#if CTX_AVX2
                      && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
      {
        int cov = *coverage;
        if (cov)
        {
      float vv = ( ( (linear_gradient_dx * u0 + linear_gradient_dy * v0) / linear_gradient_length) -
            linear_gradient_start) * (linear_gradient_rdelta);
#if CTX_GRADIENT_CACHE
      *((uint32_t*)tsrc) = *((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    uint32_t si = *((uint32_t*)(tsrc));
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

          uint32_t di = *((uint32_t*)(dst));
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst += 4;
        coverage ++;
        u0 += ud;
        v0 += vd;
      }
    }
    }

#if CTX_AVX2
                    
    for (; x <= count-8; x+=8)
    {
      __m256i xcov;
      __m256i x1_minus_cov_mul_a;
     
     if (((uint64_t*)(coverage))[0] == 0)
     {
        u0 += ud * 8;
        v0 += vd * 8;
     }
     else
     {
      int a = 255;
      for (int i = 0; i < 8; i++)
      {
      float vv = ( ( (linear_gradient_dx * u0 + linear_gradient_dy * v0) / linear_gradient_length) -
            linear_gradient_start) * (linear_gradient_rdelta);

#if CTX_GRADIENT_CACHE
      ((uint32_t*)tsrc)[i] = *((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, &tsrc[i*4]);
      ctx_u8_associate_alpha (4, &tsrc[i*4]);
#endif
      if (tsrc[i*4+3]!=255)
        a = 0;
#if CTX_DITHER
      ctx_dither_rgba_u8 (&tsrc[i*4], u0, v0, dither_red_blue, dither_green);
#endif
        u0 += ud;
        v0 += vd;
      }
      __m256i xsrc  = _mm256_load_si256((__m256i*)(tsrc));
      __m256i xsrc_a = _mm256_set_epi16(
            tsrc[7*4+3], tsrc[7*4+3],
            tsrc[6*4+3], tsrc[6*4+3],
            tsrc[5*4+3], tsrc[5*4+3],
            tsrc[4*4+3], tsrc[4*4+3],
            tsrc[3*4+3], tsrc[3*4+3],
            tsrc[2*4+3], tsrc[2*4+3],
            tsrc[1*4+3], tsrc[1*4+3],
            tsrc[0*4+3], tsrc[0*4+3]);

       if (((uint64_t*)(coverage))[0] == 0xffffffffffffffff)
       {
          if (a == 255)
          {
            _mm256_store_si256((__m256i*)dst, xsrc);
            dst += 4 * 8;
            coverage += 8;
            continue;
          }

          xcov = x00ff;
          x1_minus_cov_mul_a = _mm256_sub_epi16(x00ff, xsrc_a);
       }
       else
       {
         xcov  = _mm256_set_epi16((coverage[7]), (coverage[7]),
                                  (coverage[6]), (coverage[6]),
                                  (coverage[5]), (coverage[5]),
                                  (coverage[4]), (coverage[4]),
                                  (coverage[3]), (coverage[3]),
                                  (coverage[2]), (coverage[2]),
                                  (coverage[1]), (coverage[1]),
                                  (coverage[0]), (coverage[0]));
        x1_minus_cov_mul_a = 
           _mm256_sub_epi16(x00ff, _mm256_mulhi_epu16 (
                   _mm256_adds_epu16 (_mm256_mullo_epi16(xcov,
                                      xsrc_a), x0080), x0101));
       }
      __m256i xdst   = _mm256_load_si256((__m256i*)(dst));
      __m256i dst_lo = _mm256_and_si256 (xdst, lo_mask);
      __m256i dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8);
      __m256i src_lo = _mm256_and_si256 (xsrc, lo_mask);
      __m256i src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
        
      dst_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_hi,  x1_minus_cov_mul_a), x0080), x0101);
      dst_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_lo,  x1_minus_cov_mul_a), x0080), x0101);

      src_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_lo, xcov), x0080), x0101);
      src_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_hi,  xcov), x0080), x0101);

      dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
      dst_lo = _mm256_adds_epu16(dst_lo, src_lo);

      _mm256_store_si256((__m256i*)dst,
         _mm256_slli_epi16 (dst_hi, 8) | dst_lo);
     }

      dst += 4 * 8;
      coverage += 8;
    }

    if (x < count)
    {
      for (; (x < count) ; x++)
      {
        int cov = *coverage;
        if (cov)
        {
      float vv = ( ( (linear_gradient_dx * u0 + linear_gradient_dy * v0) / linear_gradient_length) -
            linear_gradient_start) * (linear_gradient_rdelta);
#if CTX_GRADIENT_CACHE
      *((uint32_t*)tsrc) = *((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_u8_associate_alpha (4, tsrc);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    uint32_t si = *((uint32_t*)(tsrc));
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

          uint32_t di = *((uint32_t*)(dst));
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst += 4;
        coverage ++;
        u0 += ud;
        v0 += vd;
      }
    }
#endif
}

static void
ctx_RGBA8_source_over_normal_radial_gradient (CTX_COMPOSITE_ARGUMENTS)
{
  CtxSource *g = &rasterizer->state->gstate.source;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
  float radial_gradient_x0 = g->radial_gradient.x0;
  float radial_gradient_y0 = g->radial_gradient.y0;
  float radial_gradient_r0 = g->radial_gradient.r0;
  float radial_gradient_rdelta = g->radial_gradient.rdelta;
#if CTX_DITHER
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green = rasterizer->format->dither_green;
#endif
#if CTX_AVX2
  alignas(32)
#endif
    uint8_t tsrc[4 * 8];
    int x = 0;

#if CTX_AVX2

    if ((size_t)(dst) & 31)
#endif
    {
    {
      for (; (x < count) 
#if CTX_AVX2
                      && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
      {
        int cov = *coverage;
        if (cov)
        {
      float vv = ctx_hypotf (radial_gradient_x0 - u0, radial_gradient_y0 - v0);
            vv = (vv - radial_gradient_r0) * (radial_gradient_rdelta);
#if CTX_GRADIENT_CACHE
      ((uint32_t*)tsrc)[0] = *((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    uint32_t si = *((uint32_t*)(tsrc));
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

          uint32_t di = *((uint32_t*)(dst));
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst += 4;
        coverage ++;
        u0 += ud;
        v0 += vd;
      }
    }
    }

#if CTX_AVX2
                    
    for (; x <= count-8; x+=8)
    {
      __m256i xcov;
      __m256i x1_minus_cov_mul_a;
     
     if (((uint64_t*)(coverage))[0] == 0)
     {
        u0 += ud * 8;
        v0 += vd * 8;
     }
     else
     {
      int a = 255;
      for (int i = 0; i < 8; i++)
      {
      float vv = ctx_hypotf (radial_gradient_x0 - u0, radial_gradient_y0 - v0);
            vv = (vv - radial_gradient_r0) * (radial_gradient_rdelta);

#if CTX_GRADIENT_CACHE
      ((uint32_t*)tsrc)[i] = *((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, &tsrc[i*4]);
      ctx_RGBA8_associate_alpha (&tsrc[i*4]);
#endif
      if (tsrc[i*4+3]!=255)
        a = 0;
#if CTX_DITHER
      ctx_dither_rgba_u8 (&tsrc[i*4], u0, v0, dither_red_blue, dither_green);
#endif
        u0 += ud;
        v0 += vd;
      }

      __m256i xsrc  = _mm256_load_si256((__m256i*)(tsrc));

      __m256i  xsrc_a = _mm256_set_epi16(
            tsrc[7*4+3], tsrc[7*4+3],
            tsrc[6*4+3], tsrc[6*4+3],
            tsrc[5*4+3], tsrc[5*4+3],
            tsrc[4*4+3], tsrc[4*4+3],
            tsrc[3*4+3], tsrc[3*4+3],
            tsrc[2*4+3], tsrc[2*4+3],
            tsrc[1*4+3], tsrc[1*4+3],
            tsrc[0*4+3], tsrc[0*4+3]);
       if (((uint64_t*)(coverage))[0] == 0xffffffffffffffff)
       {
          if (a == 255)
          {
            _mm256_store_si256((__m256i*)dst, xsrc);
            dst      += 32;
            coverage += 8;
            continue;
          }
          xcov = x00ff;
          x1_minus_cov_mul_a = _mm256_sub_epi16(x00ff, xsrc_a);
       }
       else
       {
         xcov  = _mm256_set_epi16((coverage[7]), (coverage[7]),
                                  (coverage[6]), (coverage[6]),
                                  (coverage[5]), (coverage[5]),
                                  (coverage[4]), (coverage[4]),
                                  (coverage[3]), (coverage[3]),
                                  (coverage[2]), (coverage[2]),
                                  (coverage[1]), (coverage[1]),
                                  (coverage[0]), (coverage[0]));
        x1_minus_cov_mul_a = 
           _mm256_sub_epi16(x00ff, _mm256_mulhi_epu16 (
                   _mm256_adds_epu16 (_mm256_mullo_epi16(xcov,
                                      xsrc_a), x0080), x0101));
       }
      __m256i xdst   = _mm256_load_si256((__m256i*)(dst));
      __m256i dst_lo = _mm256_and_si256 (xdst, lo_mask);
      __m256i dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8);
      __m256i src_lo = _mm256_and_si256 (xsrc, lo_mask);
      __m256i src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
//////////////
      dst_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_hi,  x1_minus_cov_mul_a), x0080), x0101);
      dst_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_lo,  x1_minus_cov_mul_a), x0080), x0101);

      src_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_lo, xcov), x0080), x0101);
      src_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_hi, xcov), x0080), x0101);

      dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
      dst_lo = _mm256_adds_epu16(dst_lo, src_lo);

      _mm256_store_si256((__m256i*)dst,
         _mm256_slli_epi16 (dst_hi, 8) | dst_lo);
     }

      dst += 4 * 8;
      coverage += 8;
    }

    if (x < count)
    {
      for (; (x < count) ; x++)
      {
        int cov = *coverage;
        if (cov)
        {
      float vv = ctx_hypotf (radial_gradient_x0 - u0, radial_gradient_y0 - v0);
            vv = (vv - radial_gradient_r0) * (radial_gradient_rdelta);
#if CTX_GRADIENT_CACHE
        ((uint32_t*)tsrc)[0] = *((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    uint32_t si = *((uint32_t*)(tsrc));
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

          uint32_t di = *((uint32_t*)(dst));
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst += 4;
        coverage ++;
        u0 += ud;
        v0 += vd;
      }
    }
#endif
}
#endif
#endif

static void
ctx_RGBA8_source_over_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
#if 0
  ctx_u8_source_over_normal_color (4, rasterizer, dst, src, clip, x0, coverage, count);
  return;
#endif
  {
    uint8_t tsrc[4];
    *((uint32_t*)(tsrc)) = *((uint32_t*)(src));
    ctx_RGBA8_associate_alpha (tsrc);
    uint8_t a = src[3];
    int x = 0;

#if CTX_AVX2
    if ((size_t)(dst) & 31)
#endif
    {
      uint32_t si = *((uint32_t*)(tsrc));
      uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
      uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
      if (a==255)
      {

      for (; (x < count) 
#if CTX_AVX2
                      && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
    {
      int cov = coverage[0];
      if (cov)
      {
        if (cov == 255)
        {
          *((uint32_t*)(dst)) = si;
        }
        else
        {
        int r_cov = 255-cov;
        uint32_t di = *((uint32_t*)(dst));
        uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
        uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
        *((uint32_t*)(dst)) = 
         (((si_rb * cov + di_rb * r_cov) >> 8) & CTX_RGBA8_RB_MASK) |
         (((si_ga * cov + di_ga * r_cov) >> 8) & CTX_RGBA8_GA_MASK);
        }
      }
      dst += 4;
      coverage ++;
    }
  }
    else
    {
      int si_a = si >> CTX_RGBA8_A_SHIFT;
      for (; (x < count) 
#if CTX_AVX2
                      && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
      {
        int cov = *coverage;
        if (cov)
        {
          uint32_t di = *((uint32_t*)(dst));
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst += 4;
        coverage ++;
      }
    }
    }

#if CTX_AVX2
                    
    __m256i xsrc = _mm256_set1_epi32( *((uint32_t*)tsrc)) ;
    for (; x <= count-8; x+=8)
    {
      __m256i xcov;
      __m256i x1_minus_cov_mul_a;
     
     if (((uint64_t*)(coverage))[0] == 0)
     {
     }
     else
     {
       if (((uint64_t*)(coverage))[0] == 0xffffffffffffffff)
       {
          if (a == 255)
          {
            _mm256_store_si256((__m256i*)dst, xsrc);
            dst += 4 * 8;
            coverage += 8;
            continue;
          }

          xcov = x00ff;
          x1_minus_cov_mul_a = _mm256_set1_epi16(255-a);
       }
       else
       {
         xcov  = _mm256_set_epi16((coverage[7]), (coverage[7]),
                                  (coverage[6]), (coverage[6]),
                                  (coverage[5]), (coverage[5]),
                                  (coverage[4]), (coverage[4]),
                                  (coverage[3]), (coverage[3]),
                                  (coverage[2]), (coverage[2]),
                                  (coverage[1]), (coverage[1]),
                                  (coverage[0]), (coverage[0]));
        x1_minus_cov_mul_a = 
           _mm256_sub_epi16(x00ff, _mm256_mulhi_epu16 (
                   _mm256_adds_epu16 (_mm256_mullo_epi16(xcov,
                                      _mm256_set1_epi16(a)), x0080), x0101));
       }
      __m256i xdst   = _mm256_load_si256((__m256i*)(dst));
      __m256i dst_lo = _mm256_and_si256 (xdst, lo_mask);
      __m256i dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8);
      __m256i src_lo = _mm256_and_si256 (xsrc, lo_mask);
      __m256i src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
        
      dst_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_hi,  x1_minus_cov_mul_a), x0080), x0101);
      dst_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_lo,  x1_minus_cov_mul_a), x0080), x0101);

      src_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_lo, xcov), x0080), x0101);
      src_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_hi,  xcov), x0080), x0101);

      dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
      dst_lo = _mm256_adds_epu16(dst_lo, src_lo);

      _mm256_store_si256((__m256i*)dst, _mm256_slli_epi16 (dst_hi,8)|dst_lo);
     }

      dst += 4 * 8;
      coverage += 8;
    }

    if (x < count)
    {
      uint32_t si = *((uint32_t*)(tsrc));
      uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
      uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
      int si_a = si >> CTX_RGBA8_A_SHIFT;
      for (; x < count; x++)
      {
        int cov = *coverage;
        if (cov)
        {
        if (cov == 255 && (tsrc[3]==255))
        {
          *((uint32_t*)(dst)) = si;
        }
        else
        {
          uint32_t di = *((uint32_t*)(dst));
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        }
        dst += 4;
        coverage ++;
      }
    }
#endif
  }
}

static void
ctx_RGBA8_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_copy_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_RGBA8_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_clear_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_u8_blend_normal (int components, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended)
{
  switch (components)
  {
     case 3:
       ((uint8_t*)(blended))[2] = ((uint8_t*)(src))[2];
     case 2:
       *((uint16_t*)(blended)) = *((uint16_t*)(src));
       ctx_u8_associate_alpha (components, blended);
       break;
     case 5:
       ((uint8_t*)(blended))[4] = ((uint8_t*)(src))[4];
     case 4:
       *((uint32_t*)(blended)) = *((uint32_t*)(src));
       ctx_u8_associate_alpha (components, blended);
       break;
     default:
       {
         uint8_t alpha = src[components-1];
         for (int i = 0; i<components - 1;i++)
           blended[i] = (src[i] * alpha)/255;
         blended[components-1]=alpha;
       }
       break;
  }
}

/* branchless 8bit add that maxes out at 255 */
CTX_INLINE uint8_t ctx_sadd8(uint8_t a, uint8_t b)
{
  uint16_t s = (uint16_t)a+b;
  return -(s>>8) | (uint8_t)s;
}

#if CTX_BLENDING_AND_COMPOSITING

#define ctx_u8_blend_define(name, CODE) \
static void \
ctx_u8_blend_##name (int components, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended)\
{\
  uint8_t *s=src; uint8_t b[components];\
  ctx_u8_deassociate_alpha (components, dst, b);\
    CODE;\
  blended[components-1] = src[components-1];\
  ctx_u8_associate_alpha (components, blended);\
}

#define ctx_u8_blend_define_seperable(name, CODE) \
        ctx_u8_blend_define(name, for (int c = 0; c < components-1; c++) { CODE ;}) \

ctx_u8_blend_define_seperable(multiply,     blended[c] = (b[c] * s[c])/255;)
ctx_u8_blend_define_seperable(screen,       blended[c] = s[c] + b[c] - (s[c] * b[c])/255;)
ctx_u8_blend_define_seperable(overlay,      blended[c] = b[c] < 127 ? (s[c] * b[c])/255 :
                                                         s[c] + b[c] - (s[c] * b[c])/255;)
ctx_u8_blend_define_seperable(darken,       blended[c] = ctx_mini (b[c], s[c]))
ctx_u8_blend_define_seperable(lighten,      blended[c] = ctx_maxi (b[c], s[c]))
ctx_u8_blend_define_seperable(color_dodge,  blended[c] = b[c] == 0 ? 0 :
                                     s[c] == 255 ? 255 : ctx_mini(255, (255 * b[c]) / (255-s[c])))
ctx_u8_blend_define_seperable(color_burn,   blended[c] = b[c] == 1 ? 1 :
                                     s[c] == 0 ? 0 : 255 - ctx_mini(255, (255*(255 - b[c])) / s[c]))
ctx_u8_blend_define_seperable(hard_light,   blended[c] = s[c] < 127 ? (b[c] * s[c])/255 :
                                                          b[c] + s[c] - (b[c] * s[c])/255;)
ctx_u8_blend_define_seperable(difference,   blended[c] = (b[c] - s[c]))
ctx_u8_blend_define_seperable(divide,       blended[c] = s[c]?(255 * b[c]) / s[c]:0)
ctx_u8_blend_define_seperable(addition,     blended[c] = ctx_sadd8 (s[c], b[c]))
ctx_u8_blend_define_seperable(subtract,     blended[c] = ctx_maxi(0, s[c]-b[c]))
ctx_u8_blend_define_seperable(exclusion,    blended[c] = b[c] + s[c] - 2 * (b[c] * s[c]/255))
ctx_u8_blend_define_seperable(soft_light,
  if (s[c] <= 255/2)
  {
    blended[c] = b[c] - (255 - 2 * s[c]) * b[c] * (255 - b[c]) / (255 * 255);
  }
  else
  {
    int d;
    if (b[c] <= 255/4)
      d = (((16 * b[c] - 12 * 255)/255 * b[c] + 4 * 255) * b[c])/255;
    else
      d = ctx_sqrtf(b[c]/255.0) * 255.4;
    blended[c] = (b[c] + (2 * s[c] - 255) * (d - b[c]))/255;
  }
)

static int ctx_int_get_max (int components, int *c)
{
  int max = 0;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] > max) max = c[i];
  }
  return max;
}

static int ctx_int_get_min (int components, int *c)
{
  int min = 400;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] < min) min = c[i];
  }
  return min;
}

static int ctx_int_get_lum (int components, int *c)
{
  switch (components)
  {
    case 3:
    case 4:
            return CTX_CSS_RGB_TO_LUMINANCE(c);
    case 1:
    case 2:
            return c[0];
            break;
    default:
       {
         int sum = 0;
         for (int i = 0; i < components - 1; i ++)
         {
           sum += c[i];
         }
         return sum / (components - 1);
       }
            break;
  }
}

static int ctx_u8_get_lum (int components, uint8_t *c)
{
  switch (components)
  {
    case 3:
    case 4:
            return CTX_CSS_RGB_TO_LUMINANCE(c);
    case 1:
    case 2:
            return c[0];
            break;
    default:
       {
         int sum = 0;
         for (int i = 0; i < components - 1; i ++)
         {
           sum += c[i];
         }
         return sum / (components - 1);
       }
            break;
  }
}
static int ctx_u8_get_sat (int components, uint8_t *c)
{
  switch (components)
  {
    case 3:
    case 4:
            { int r = c[0];
              int g = c[1];
              int b = c[2];
              return ctx_maxi(r, ctx_maxi(g,b)) - ctx_mini(r,ctx_mini(g,b));
            }
            break;
    case 1:
    case 2:
            return 0.0;
            break;
    default:
       {
         int min = 1000;
         int max = -1000;
         for (int i = 0; i < components - 1; i ++)
         {
           if (c[i] < min) min = c[i];
           if (c[i] > max) max = c[i];
         }
         return max-min;
       }
       break;
  }
}

static void ctx_u8_set_lum (int components, uint8_t *c, uint8_t lum)
{
  int d = lum - ctx_u8_get_lum (components, c);
  int tc[components];
  for (int i = 0; i < components - 1; i++)
  {
    tc[i] = c[i] + d;
  }

  int l = ctx_int_get_lum (components, tc);
  int n = ctx_int_get_min (components, tc);
  int x = ctx_int_get_max (components, tc);

  if (n < 0)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if (x > 255)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * (255 - l)) / (x-l));
  }
  for (int i = 0; i < components - 1; i++)
    c[i] = tc[i];
}

static void ctx_u8_set_sat (int components, uint8_t *c, uint8_t sat)
{
  int max = 0, mid = 1, min = 2;
  
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}
  if (c[mid] > c[max]){int t = mid; mid = max; max = t;}
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}

  if (c[max] > c[min])
  {
    c[mid] = ((c[mid]-c[min]) * sat) / (c[max] - c[min]);
    c[max] = sat;
  }
  else
  {
    c[mid] = c[max] = 0;
  }
  c[min] = 0;
}

ctx_u8_blend_define(color,
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_u8_set_lum(components, blended, ctx_u8_get_lum (components, s));
)

ctx_u8_blend_define(hue,
  int in_sat = ctx_u8_get_sat(components, b);
  int in_lum = ctx_u8_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_u8_set_sat(components, blended, in_sat);
  ctx_u8_set_lum(components, blended, in_lum);
)

ctx_u8_blend_define(saturation,
  int in_sat = ctx_u8_get_sat(components, s);
  int in_lum = ctx_u8_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_u8_set_sat(components, blended, in_sat);
  ctx_u8_set_lum(components, blended, in_lum);
)

ctx_u8_blend_define(luminosity,
  int in_lum = ctx_u8_get_lum(components, s);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_u8_set_lum(components, blended, in_lum);
)
#endif

CTX_INLINE static void
ctx_u8_blend (int components, CtxBlend blend, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended)
{
#if CTX_BLENDING_AND_COMPOSITING
  switch (blend)
  {
    case CTX_BLEND_NORMAL:      ctx_u8_blend_normal      (components, dst, src, blended); break;
    case CTX_BLEND_MULTIPLY:    ctx_u8_blend_multiply    (components, dst, src, blended); break;
    case CTX_BLEND_SCREEN:      ctx_u8_blend_screen      (components, dst, src, blended); break;
    case CTX_BLEND_OVERLAY:     ctx_u8_blend_overlay     (components, dst, src, blended); break;
    case CTX_BLEND_DARKEN:      ctx_u8_blend_darken      (components, dst, src, blended); break;
    case CTX_BLEND_LIGHTEN:     ctx_u8_blend_lighten     (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_DODGE: ctx_u8_blend_color_dodge (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_BURN:  ctx_u8_blend_color_burn  (components, dst, src, blended); break;
    case CTX_BLEND_HARD_LIGHT:  ctx_u8_blend_hard_light  (components, dst, src, blended); break;
    case CTX_BLEND_SOFT_LIGHT:  ctx_u8_blend_soft_light  (components, dst, src, blended); break;
    case CTX_BLEND_DIFFERENCE:  ctx_u8_blend_difference  (components, dst, src, blended); break;
    case CTX_BLEND_EXCLUSION:   ctx_u8_blend_exclusion   (components, dst, src, blended); break;
    case CTX_BLEND_COLOR:       ctx_u8_blend_color       (components, dst, src, blended); break;
    case CTX_BLEND_HUE:         ctx_u8_blend_hue         (components, dst, src, blended); break;
    case CTX_BLEND_SATURATION:  ctx_u8_blend_saturation  (components, dst, src, blended); break;
    case CTX_BLEND_LUMINOSITY:  ctx_u8_blend_luminosity  (components, dst, src, blended); break;
    case CTX_BLEND_ADDITION:    ctx_u8_blend_addition    (components, dst, src, blended); break;
    case CTX_BLEND_DIVIDE:      ctx_u8_blend_divide      (components, dst, src, blended); break;
    case CTX_BLEND_SUBTRACT:    ctx_u8_blend_subtract    (components, dst, src, blended); break;
  }
#else
  switch (blend)
  {
    default:                    ctx_u8_blend_normal      (components, dst, src, blended); break;
  }

#endif
}

CTX_INLINE static void
__ctx_u8_porter_duff (CtxRasterizer         *rasterizer,
                     int                    components,
                     uint8_t * __restrict__ dst,
                     uint8_t * __restrict__ src,
                     int                    x0,
                     uint8_t * __restrict__ coverage,
                     int                    count,
                     CtxCompositingMode     compositing_mode,
                     CtxFragment            fragment,
                     CtxBlend               blend)
{
  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;

  {
    uint8_t tsrc[components];
    float u0 = 0; float v0 = 0;
    float ud = 0; float vd = 0;
    if (fragment)
      ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);

 // if (blend == CTX_BLEND_NORMAL)
      ctx_u8_blend (components, blend, dst, src, tsrc);

    while (count--)
    {
      int cov = *coverage;

      if (
        (compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 255)||
        (compositing_mode == CTX_COMPOSITE_SOURCE_OVER      && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_XOR              && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_DESTINATION_OUT  && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      && cov == 0)
        )
      {
        u0 += ud;
        v0 += vd;
        coverage ++;
        dst+=components;
        continue;
      }

      if (fragment)
      {
        fragment (rasterizer, u0, v0, tsrc);
      //if (blend != CTX_BLEND_NORMAL)
          ctx_u8_blend (components, blend, dst, tsrc, tsrc);
      }
      else
      {
      //if (blend != CTX_BLEND_NORMAL)
          ctx_u8_blend (components, blend, dst, src, tsrc);
      }

      u0 += ud;
      v0 += vd;
      if (global_alpha_u8 != 255)
        cov = (cov * global_alpha_u8)/255;

      if (cov != 255)
      for (int c = 0; c < components; c++)
        tsrc[c] = (tsrc[c] * cov)/255;

      for (int c = 0; c < components; c++)
      {
        int res = 0;
        switch (f_s)
        {
          case CTX_PORTER_DUFF_0: break;
          case CTX_PORTER_DUFF_1:             res += (tsrc[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         res += (tsrc[c] *      dst[components-1])/255; break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (tsrc[c] * (255-dst[components-1]))/255; break;
        }
        switch (f_d)
        {
          case CTX_PORTER_DUFF_0: break;
          case CTX_PORTER_DUFF_1:             res += dst[c]; break;
          case CTX_PORTER_DUFF_ALPHA:         res += (dst[c] * tsrc[components-1])/255; break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (dst[c] * (255-tsrc[components-1]))/255; break;
        }
        dst[c] = res;
      }
      coverage ++;
      dst+=components;
    }
  }
}

#if CTX_AVX2
CTX_INLINE static void
ctx_avx2_porter_duff (CtxRasterizer         *rasterizer,
                      int                    components,
                      uint8_t * dst,
                      uint8_t * src,
                      int                    x0,
                      uint8_t * coverage,
                      int                    count,
                      CtxCompositingMode     compositing_mode,
                      CtxFragment            fragment,
                      CtxBlend               blend)
{
  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
//assert ((((size_t)dst) & 31) == 0);
  int n_pix = 32/components;
  uint8_t tsrc[components * n_pix];
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  int x = 0;
  if (fragment)
    ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);

  for (; x < count; x+=n_pix)
  {
    __m256i xdst  = _mm256_loadu_si256((__m256i*)(dst)); 
    __m256i xcov;
    __m256i xsrc;
    __m256i xsrc_a;
    __m256i xdst_a;

    int is_blank = 1;
    int is_full = 0;
    switch (n_pix)
    {
      case 16:
        if (((uint64_t*)(coverage))[0] &&
            ((uint64_t*)(coverage))[1])
           is_blank = 0;
        else if (((uint64_t*)(coverage))[0] == 0xffffffffffffffff &&
                 ((uint64_t*)(coverage))[1] == 0xffffffffffffffff)
           is_full = 1;
        break;
      case 8:
        if (((uint64_t*)(coverage))[0])
           is_blank = 0;
        else if (((uint64_t*)(coverage))[0] == 0xffffffffffffffff)
           is_full = 1;
        break;
      case 4:
        if (((uint32_t*)(coverage))[0])
           is_blank = 0;
        else if (((uint32_t*)(coverage))[0] == 0xffffffff)
           is_full = 1;
        break;
      default:
        break;
    }

#if 1
    if (
      //(compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 255)||
      (compositing_mode == CTX_COMPOSITE_SOURCE_OVER      && is_blank) ||
      (compositing_mode == CTX_COMPOSITE_XOR              && is_blank) ||
      (compositing_mode == CTX_COMPOSITE_DESTINATION_OUT  && is_blank) ||
      (compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      && is_blank)
      )
    {
      u0 += ud * n_pix;
      v0 += vd * n_pix;
      coverage += n_pix;
      dst+=32;
      continue;
    }
#endif

    if (fragment)
    {
      for (int i = 0; i < n_pix; i++)
      {
         fragment (rasterizer, u0, v0, &tsrc[i*components]);
         ctx_u8_associate_alpha (components, &tsrc[i*components]);
         ctx_u8_blend (components, blend,
                       &dst[i*components],
                       &tsrc[i*components],
                       &tsrc[i*components]);
         u0 += ud;
         v0 += vd;
      }
      xsrc = _mm256_loadu_si256((__m256i*)tsrc);
    }
    else
    {
#if 0
      if (blend == CTX_BLEND_NORMAL && components == 4)
        xsrc = _mm256_set1_epi32 (*((uint32_t*)src));
    else
#endif
      {
 //     for (int i = 0; i < n_pix; i++)
 //       for (int c = 0; c < components; c++)
 //         tsrc[i*components+c]=src[c];
#if 1
        uint8_t lsrc[components];
        for (int i = 0; i < components; i ++)
          lsrc[i] = src[i];
  //    ctx_u8_associate_alpha (components, lsrc);
        for (int i = 0; i < n_pix; i++)
          ctx_u8_blend (components, blend,
                        &dst[i*components],
                        lsrc,
                        &tsrc[i*components]);
#endif
        xsrc = _mm256_loadu_si256((__m256i*)tsrc);
      }
    }

    if (is_full)
       xcov = _mm256_set1_epi16(255);
    else
    switch (n_pix)
    {
      case 4: xcov  = _mm256_set_epi16(
               (coverage[3]), (coverage[3]), coverage[3], coverage[3],
               (coverage[2]), (coverage[2]), coverage[2], coverage[2],
               (coverage[1]), (coverage[1]), coverage[1], coverage[1],
               (coverage[0]), (coverage[0]), coverage[0], coverage[0]);
              break;
      case 8: xcov  = _mm256_set_epi16(
               (coverage[7]), (coverage[7]),
               (coverage[6]), (coverage[6]),
               (coverage[5]), (coverage[5]),
               (coverage[4]), (coverage[4]),
               (coverage[3]), (coverage[3]),
               (coverage[2]), (coverage[2]),
               (coverage[1]), (coverage[1]),
               (coverage[0]), (coverage[0]));
              break;
      case 16: xcov  = _mm256_set_epi16(
               (coverage[15]),
               (coverage[14]),
               (coverage[13]),
               (coverage[12]),
               (coverage[11]),
               (coverage[10]),
               (coverage[9]),
               (coverage[8]),
               (coverage[7]),
               (coverage[6]),
               (coverage[5]),
               (coverage[4]),
               (coverage[3]),
               (coverage[2]),
               (coverage[1]),
               (coverage[0]));
              break;
    }
#if 0
    switch (n_pix)
    {
      case 4:
      xsrc_a = _mm256_set_epi16(
            tsrc[3*components+(components-1)], tsrc[3*components+(components-1)],tsrc[3*components+(components-1)], tsrc[3*components+(components-1)],
            tsrc[2*components+(components-1)], tsrc[2*components+(components-1)],tsrc[2*components+(components-1)], tsrc[2*components+(components-1)],
            tsrc[1*components+(components-1)], tsrc[1*components+(components-1)],tsrc[1*components+(components-1)], tsrc[1*components+(components-1)],
            tsrc[0*components+(components-1)], tsrc[0*components+(components-1)],tsrc[0*components+(components-1)], tsrc[0*components+(components-1)]);
      xdst_a = _mm256_set_epi16(
            dst[3*components+(components-1)], dst[3*components+(components-1)],dst[3*components+(components-1)], dst[3*components+(components-1)],
            dst[2*components+(components-1)], dst[2*components+(components-1)],dst[2*components+(components-1)], dst[2*components+(components-1)],
            dst[1*components+(components-1)], dst[1*components+(components-1)],dst[1*components+(components-1)], dst[1*components+(components-1)],
            dst[0*components+(components-1)], dst[0*components+(components-1)],dst[0*components+(components-1)], dst[0*components+(components-1)]);

              break;
      case 8:
      xsrc_a = _mm256_set_epi16(
            tsrc[7*components+(components-1)], tsrc[7*components+(components-1)],
            tsrc[6*components+(components-1)], tsrc[6*components+(components-1)],
            tsrc[5*components+(components-1)], tsrc[5*components+(components-1)],
            tsrc[4*components+(components-1)], tsrc[4*components+(components-1)],
            tsrc[3*components+(components-1)], tsrc[3*components+(components-1)],
            tsrc[2*components+(components-1)], tsrc[2*components+(components-1)],
            tsrc[1*components+(components-1)], tsrc[1*components+(components-1)],
            tsrc[0*components+(components-1)], tsrc[0*components+(components-1)]);
      xdst_a = _mm256_set_epi16(
            dst[7*components+(components-1)], dst[7*components+(components-1)],
            dst[6*components+(components-1)], dst[6*components+(components-1)],
            dst[5*components+(components-1)], dst[5*components+(components-1)],
            dst[4*components+(components-1)], dst[4*components+(components-1)],
            dst[3*components+(components-1)], dst[3*components+(components-1)],
            dst[2*components+(components-1)], dst[2*components+(components-1)],
            dst[1*components+(components-1)], dst[1*components+(components-1)],
            dst[0*components+(components-1)], dst[0*components+(components-1)]);
              break;
      case 16: 
      xsrc_a = _mm256_set_epi16(
            tsrc[15*components+(components-1)],
            tsrc[14*components+(components-1)],
            tsrc[13*components+(components-1)],
            tsrc[12*components+(components-1)],
            tsrc[11*components+(components-1)],
            tsrc[10*components+(components-1)],
            tsrc[9*components+(components-1)],
            tsrc[8*components+(components-1)],
            tsrc[7*components+(components-1)],
            tsrc[6*components+(components-1)],
            tsrc[5*components+(components-1)],
            tsrc[4*components+(components-1)],
            tsrc[3*components+(components-1)],
            tsrc[2*components+(components-1)],
            tsrc[1*components+(components-1)],
            tsrc[0*components+(components-1)]);
      xdst_a = _mm256_set_epi16(
            dst[15*components+(components-1)],
            dst[14*components+(components-1)],
            dst[13*components+(components-1)],
            dst[12*components+(components-1)],
            dst[11*components+(components-1)],
            dst[10*components+(components-1)],
            dst[9*components+(components-1)],
            dst[8*components+(components-1)],
            dst[7*components+(components-1)],
            dst[6*components+(components-1)],
            dst[5*components+(components-1)],
            dst[4*components+(components-1)],
            dst[3*components+(components-1)],
            dst[2*components+(components-1)],
            dst[1*components+(components-1)],
            dst[0*components+(components-1)]);
              break;
    }
#endif

    if (global_alpha_u8 != 255)
    {
      xcov = _mm256_mulhi_epu16(
              _mm256_adds_epu16(
                 _mm256_mullo_epi16(xcov,
                                    _mm256_set1_epi16(global_alpha_u8)),
                 x0080), x0101);
      is_full = 0;
    }


    xsrc_a = _mm256_srli_epi32(xsrc, 24);  // XX 24 is RGB specific
    if (!is_full)
    xsrc_a = _mm256_mulhi_epu16(
              _mm256_adds_epu16(
                 _mm256_mullo_epi16(xsrc_a, xcov),
                 x0080), x0101);
    xsrc_a = xsrc_a | _mm256_slli_epi32(xsrc, 16);

    xdst_a = _mm256_srli_epi32(xdst, 24);
    xdst_a = xdst_a |  _mm256_slli_epi32(xdst, 16);


 //  case CTX_COMPOSITE_SOURCE_OVER:
 //     f_s = CTX_PORTER_DUFF_1;
 //     f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;


    __m256i dst_lo;
    __m256i dst_hi; 
    __m256i src_lo; 
    __m256i src_hi;

    switch (f_s)
    {
      case CTX_PORTER_DUFF_0:
        src_lo = _mm256_set1_epi32(0);
        src_hi = _mm256_set1_epi32(0);
        break;
      case CTX_PORTER_DUFF_1:
        src_lo = _mm256_and_si256 (xsrc, lo_mask); 
        src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);

        //if (!is_full)
        {
          src_lo = _mm256_mulhi_epu16(
                   _mm256_adds_epu16(
                   _mm256_mullo_epi16(src_lo, xcov),
                   x0080), x0101);
          src_hi = _mm256_mulhi_epu16(
                   _mm256_adds_epu16(
                   _mm256_mullo_epi16(src_hi, xcov),
                   x0080), x0101);
        }
        break;
      case CTX_PORTER_DUFF_ALPHA:
        // res += (tsrc[c] *      dst[components-1])/255;
        src_lo = _mm256_and_si256 (xsrc, lo_mask); 
        src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
        if (!is_full)
        {
           src_lo = _mm256_mulhi_epu16(
                 _mm256_adds_epu16(
                 _mm256_mullo_epi16(src_lo, xcov),
                 x0080), x0101);
           src_hi = _mm256_mulhi_epu16(
                    _mm256_adds_epu16(
                    _mm256_mullo_epi16(src_hi, xcov),
                    x0080), x0101);
        }
        src_lo = _mm256_mulhi_epu16 (
                      _mm256_adds_epu16 (_mm256_mullo_epi16(src_lo,
                                         xdst_a), x0080), x0101);
        src_hi = _mm256_mulhi_epu16 (
                      _mm256_adds_epu16 (_mm256_mullo_epi16(src_hi,
                                         xdst_a), x0080), x0101);
        break;
      case CTX_PORTER_DUFF_1_MINUS_ALPHA:
        // res += (tsrc[c] * (255-dst[components-1]))/255;
        src_lo = _mm256_and_si256 (xsrc, lo_mask); 
        src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
  //    if (!is_full)
        {
          src_lo = _mm256_mulhi_epu16(
                        _mm256_adds_epu16(
                        _mm256_mullo_epi16(src_lo, xcov),
                        x0080), x0101);
          src_hi = _mm256_mulhi_epu16(
                        _mm256_adds_epu16(
                        _mm256_mullo_epi16(src_hi, xcov),
                        x0080), x0101);
        }
        src_lo = _mm256_mulhi_epu16 (
                  _mm256_adds_epu16 (_mm256_mullo_epi16(src_lo,
                                     _mm256_sub_epi16(x00ff,xdst_a)), x0080),
                  x0101);
        src_hi = _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(src_hi,
                                    _mm256_sub_epi16(x00ff,xdst_a)), x0080),
                 x0101);
        break;
    }
    switch (f_d)
    {
      case CTX_PORTER_DUFF_0: 
        dst_lo = _mm256_set1_epi32(0);
        dst_hi = _mm256_set1_epi32(0);
        break;
      case CTX_PORTER_DUFF_1:
        dst_lo = _mm256_and_si256 (xdst, lo_mask); 
        dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8); 
        break;
      case CTX_PORTER_DUFF_ALPHA:        
          //res += (dst[c] * tsrc[components-1])/255;
          dst_lo = _mm256_and_si256 (xdst, lo_mask); 
          dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8); 

          dst_lo =
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_lo,
                                    xsrc_a), x0080), x0101);
          dst_hi =
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_hi,
                                    xsrc_a), x0080), x0101);
          break;
      case CTX_PORTER_DUFF_1_MINUS_ALPHA:
          dst_lo = _mm256_and_si256 (xdst, lo_mask); 
          dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8); 
          dst_lo = 
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_lo,
                                    _mm256_sub_epi16(x00ff,xsrc_a)), x0080),
                 x0101);
          dst_hi = 
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_hi,
                                    _mm256_sub_epi16(x00ff,xsrc_a)), x0080),
                 x0101);
          break;
    }

    dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
    dst_lo = _mm256_adds_epu16(dst_lo, src_lo);

#if 0 // to toggle source vs dst
      src_hi = _mm256_slli_epi16 (src_hi, 8);
      _mm256_storeu_si256((__m256i*)dst, _mm256_blendv_epi8(src_lo, src_hi, hi_mask));
#else
      _mm256_storeu_si256((__m256i*)dst, _mm256_slli_epi16 (dst_hi, 8) | dst_lo);
#endif

    coverage += n_pix;
    dst      += 32;
  }
}
#endif

CTX_INLINE static void
_ctx_u8_porter_duff (CtxRasterizer         *rasterizer,
                     int                    components,
                     uint8_t *              dst,
                     uint8_t * __restrict__ src,
                     int                    x0,
                     uint8_t *              coverage,
                     int                    count,
                     CtxCompositingMode     compositing_mode,
                     CtxFragment            fragment,
                     CtxBlend               blend)
{
#if NOT_USABLE_CTX_AVX2
  int pre_count = 0;
  if ((size_t)(dst)&31)
  {
    pre_count = (32-(((size_t)(dst))&31))/components;
  __ctx_u8_porter_duff (rasterizer, components,
     dst, src, x0, coverage, pre_count, compositing_mode, fragment, blend);
    dst += components * pre_count;
    x0 += pre_count;
    coverage += pre_count;
    count -= pre_count;
  }
  if (count < 0)
     return;
  int post_count = (count & 31);
  if (src && 0)
  {
    src[0]/=2;
    src[1]/=2;
    src[2]/=2;
    src[3]/=2;
  }
#if 0
  __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count-post_count, compositing_mode, fragment, blend);
#else
  ctx_avx2_porter_duff (rasterizer, components, dst, src, x0, coverage, count-post_count, compositing_mode, fragment, blend);
#endif
  if (src && 0)
  {
    src[0]*=2;
    src[1]*=2;
    src[2]*=2;
    src[3]*=2;
  }
  if (post_count > 0)
  {
       x0 += (count - post_count);
       dst += components * (count-post_count);
       coverage += (count - post_count);
       __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, post_count, compositing_mode, fragment, blend);
  }
#else
  __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count, compositing_mode, fragment, blend);
#endif
}

#define _ctx_u8_porter_duffs(comp_format, components, source, fragment, blend) \
   switch (rasterizer->state->gstate.compositing_mode) \
   { \
     case CTX_COMPOSITE_SOURCE_ATOP: \
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count, \
        CTX_COMPOSITE_SOURCE_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_ATOP:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_IN:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_IN, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OVER:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OVER:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_XOR:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_XOR, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OUT:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OUT:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_IN:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_IN, fragment, blend);\
       break;\
     case CTX_COMPOSITE_COPY:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_COPY, fragment, blend);\
       break;\
     case CTX_COMPOSITE_CLEAR:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_CLEAR, fragment, blend);\
       break;\
   }

/* generating one function per compositing_mode would be slightly more efficient,
 * but on embedded targets leads to slightly more code bloat,
 * here we trade off a slight amount of performance
 */
#define ctx_u8_porter_duff(comp_format, components, source, fragment, blend) \
static void \
ctx_##comp_format##_porter_duff_##source (CTX_COMPOSITE_ARGUMENTS) \
{ \
  _ctx_u8_porter_duffs(comp_format, components, source, fragment, blend);\
}

ctx_u8_porter_duff(RGBA8, 4,color,   NULL,                 rasterizer->state->gstate.blend_mode)
ctx_u8_porter_duff(RGBA8, 4,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

//ctx_u8_porter_duff(comp_name, components,color_##blend_name,  NULL, blend_mode)

#if CTX_INLINED_NORMAL

#if CTX_GRADIENTS
#define ctx_u8_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_u8_porter_duff(comp_name, components,generic_##blend_name,          rasterizer->fragment,               blend_mode)\
ctx_u8_porter_duff(comp_name, components,linear_gradient_##blend_name,  ctx_fragment_linear_gradient_##comp_name, blend_mode)\
ctx_u8_porter_duff(comp_name, components,radial_gradient_##blend_name,  ctx_fragment_radial_gradient_##comp_name, blend_mode)\
ctx_u8_porter_duff(comp_name, components,image_rgb8_##blend_name, ctx_fragment_image_rgb8_##comp_name,      blend_mode)\
ctx_u8_porter_duff(comp_name, components,image_rgba8_##blend_name,ctx_fragment_image_rgba8_##comp_name,     blend_mode)
ctx_u8_porter_duff_blend(RGBA8, 4, CTX_BLEND_NORMAL, normal)
#else

#define ctx_u8_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_u8_porter_duff(comp_name, components,generic_##blend_name,          rasterizer->fragment,               blend_mode)\
ctx_u8_porter_duff(comp_name, components,image_rgb8_##blend_name, ctx_fragment_image_rgb8_##comp_name,      blend_mode)\
ctx_u8_porter_duff(comp_name, components,image_rgba8_##blend_name,ctx_fragment_image_rgba8_##comp_name,     blend_mode)
ctx_u8_porter_duff_blend(RGBA8, 4, CTX_BLEND_NORMAL, normal)
#endif
#endif


static void
ctx_RGBA8_nop (CTX_COMPOSITE_ARGUMENTS)
{
}

static void
ctx_setup_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 4;
  rasterizer->fragment = ctx_rasterizer_get_fragment_RGBA8 (rasterizer);
  rasterizer->comp_op = ctx_RGBA8_porter_duff_generic;
#if 1
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
  {
    rasterizer->comp_op = ctx_RGBA8_clear_normal;
    return;
  }
#endif
#if CTX_INLINED_GRADIENTS
#if CTX_GRADIENTS
  if (gstate->source.type == CTX_SOURCE_LINEAR_GRADIENT &&
      gstate->blend_mode == CTX_BLEND_NORMAL &&
      gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
  {
     rasterizer->comp_op = ctx_RGBA8_source_over_normal_linear_gradient;
     return;
  }
  if (gstate->source.type == CTX_SOURCE_RADIAL_GRADIENT &&
      gstate->blend_mode == CTX_BLEND_NORMAL &&
      gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
  {
     rasterizer->comp_op = ctx_RGBA8_source_over_normal_radial_gradient;
     return;
  }
#endif
#endif

  if (gstate->source.type == CTX_SOURCE_COLOR)
    {
      ctx_color_get_rgba8 (rasterizer->state, &gstate->source.color, rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        rasterizer->color[components-1] = (rasterizer->color[components-1] * gstate->global_alpha_u8)/255;

      switch (gstate->blend_mode)
      {
        case CTX_BLEND_NORMAL:
          if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
          {
            rasterizer->comp_op = ctx_RGBA8_copy_normal;
            return;
          }
          else if (gstate->global_alpha_u8 == 0)
          {
            rasterizer->comp_op = ctx_RGBA8_nop;
          }
          else if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
          {
             if (rasterizer->color[components-1] == 0)
                 rasterizer->comp_op = ctx_RGBA8_nop;
             else
                 rasterizer->comp_op = ctx_RGBA8_source_over_normal_color;
         }
         break;
      default:
         rasterizer->comp_op = ctx_RGBA8_porter_duff_color;
         break;
    }
    //rasterizer->comp_op = ctx_RGBA8_porter_duff_color; // XXX overide to make all go
                                                       // through generic code path
    rasterizer->fragment = NULL;
    return;
  }

#if CTX_INLINED_NORMAL
    if (gstate->blend_mode == CTX_BLEND_NORMAL)
    {
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            return; // exhaustively handled above;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_linear_gradient_normal;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_radial_gradient_normal;
            break;
#endif
          case CTX_SOURCE_IMAGE:
            {
               CtxSource *g = &rasterizer->state->gstate.source;
               switch (g->image.buffer->format->bpp)
               {
                 case 32:
                   rasterizer->comp_op = ctx_RGBA8_porter_duff_image_rgba8_normal;
                   break;
                 case 24:
                   rasterizer->comp_op = ctx_RGBA8_porter_duff_image_rgb8_normal;
                 break;
                 default:
                   rasterizer->comp_op = ctx_RGBA8_porter_duff_generic_normal;
                   break;
               }
            }
            break;
          default:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_generic_normal;
            break;
        }
        return;
    }
#endif
}

/*
 * we could use this instead of NULL - but then dispatch
 * is slightly slower
 */
inline static void
ctx_composite_direct (CTX_COMPOSITE_ARGUMENTS)
{
  rasterizer->comp_op (rasterizer, dst, rasterizer->color, x0, coverage, count);
}

static void
ctx_composite_convert (CTX_COMPOSITE_ARGUMENTS)
{
  uint8_t pixels[count * rasterizer->format->ebpp];
  rasterizer->format->to_comp (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  rasterizer->format->from_comp (rasterizer, x0, &pixels[0], dst, count);
}


static void
ctx_float_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;

  if (rasterizer->fragment)
    {
      ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
    }

  while (count--)
  {
    int cov = *coverage;
    if (cov == 0)
    {
      for (int c = 0; c < components; c++)
        { dst[c] = 0; }
    }
    else
    {
      if (rasterizer->fragment)
      {
        rasterizer->fragment (rasterizer, u0, v0, src);
        u0+=ud;
        v0+=vd;
      }
    if (cov == 255)
    {
      for (int c = 0; c < components; c++)
        dstf[c] = srcf[c];
    }
    else
    {
      float covf = ctx_u8_to_float (cov);
      for (int c = 0; c < components; c++)
        dstf[c] = srcf[c]*covf;
    }
    }
    dstf += components;
    coverage ++;
  }
}

static void
ctx_float_clear_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  while (count--)
  {
#if 0
    int cov = *coverage;
    if (cov == 0)
    {
    }
    else if (cov == 255)
    {
#endif
      switch (components)
      {
        case 2:
          ((uint64_t*)(dst))[0] = 0;
          break;
        case 4:
          ((uint64_t*)(dst))[0] = 0;
          ((uint64_t*)(dst))[1] = 0;
          break;
        default:
          for (int c = 0; c < components; c++)
            dstf[c] = 0.0f;
      }
#if 0
    }
    else
    {
      float ralpha = 1.0 - ctx_u8_to_float (cov);
      for (int c = 0; c < components; c++)
        { dstf[c] = (dstf[c] * ralpha); }
    }
    coverage ++;
#endif
    dstf += components;
  }
}

static void
ctx_float_source_over_normal_opaque_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;

  while (count--)
  {
    int cov = *coverage;
    if (cov)
    {
      if (cov == 255)
      {
        for (int c = 0; c < components; c++)
          dstf[c] = srcf[c];
      }
      else
      {
        float fcov = ctx_u8_to_float (cov);
        float ralpha = 1.0f - fcov;
        for (int c = 0; c < components-1; c++)
          dstf[c] = (srcf[c]*fcov + dstf[c] * ralpha);
      }
    }
    coverage ++;
    dstf+= components;
  }
}

inline static void
ctx_float_blend_normal (int components, float *dst, float *src, float *blended)
{
  float a = src[components-1];
  for (int c = 0; c <  components - 1; c++)
    blended[c] = src[c] * a;
  blended[components-1]=a;
}

static float ctx_float_get_max (int components, float *c)
{
  float max = -1000.0f;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] > max) max = c[i];
  }
  return max;
}

static float ctx_float_get_min (int components, float *c)
{
  float min = 400.0;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] < min) min = c[i];
  }
  return min;
}

static float ctx_float_get_lum (int components, float *c)
{
  switch (components)
  {
    case 3:
    case 4:
            return CTX_CSS_RGB_TO_LUMINANCE(c);
    case 1:
    case 2:
            return c[0];
            break;
    default:
       {
         float sum = 0;
         for (int i = 0; i < components - 1; i ++)
         {
           sum += c[i];
         }
         return sum / (components - 1);
       }
  }
}

static float ctx_float_get_sat (int components, float *c)
{
  switch (components)
  {
    case 3:
    case 4:
            { float r = c[0];
              float g = c[1];
              float b = c[2];
              return ctx_maxf(r, ctx_maxf(g,b)) - ctx_minf(r,ctx_minf(g,b));
            }
            break;
    case 1:
    case 2: return 0.0;
            break;
    default:
       {
         float min = 1000;
         float max = -1000;
         for (int i = 0; i < components - 1; i ++)
         {
           if (c[i] < min) min = c[i];
           if (c[i] > max) max = c[i];
         }
         return max-min;
       }
  }
}

static void ctx_float_set_lum (int components, float *c, float lum)
{
  float d = lum - ctx_float_get_lum (components, c);
  float tc[components];
  for (int i = 0; i < components - 1; i++)
  {
    tc[i] = c[i] + d;
  }

  float l = ctx_float_get_lum (components, tc);
  float n = ctx_float_get_min (components, tc);
  float x = ctx_float_get_max (components, tc);

  if (n < 0.0f)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if (x > 1.0f)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * (1.0f - l)) / (x-l));
  }
  for (int i = 0; i < components - 1; i++)
    c[i] = tc[i];
}

static void ctx_float_set_sat (int components, float *c, float sat)
{
  int max = 0, mid = 1, min = 2;
  
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}
  if (c[mid] > c[max]){int t = mid; mid = max; max = t;}
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}

  if (c[max] > c[min])
  {
    c[mid] = ((c[mid]-c[min]) * sat) / (c[max] - c[min]);
    c[max] = sat;
  }
  else
  {
    c[mid] = c[max] = 0.0f;
  }
  c[min] = 0.0f;

}

#define ctx_float_blend_define(name, CODE) \
static void \
ctx_float_blend_##name (int components, float * __restrict__ dst, float *src, float *blended)\
{\
  float *s = src; float b[components];\
  ctx_float_deassociate_alpha (components, dst, b);\
    CODE;\
  blended[components-1] = s[components-1];\
  ctx_float_associate_alpha (components, blended);\
}

#define ctx_float_blend_define_seperable(name, CODE) \
        ctx_float_blend_define(name, for (int c = 0; c < components-1; c++) { CODE ;}) \

ctx_float_blend_define_seperable(multiply,    blended[c] = (b[c] * s[c]);)
ctx_float_blend_define_seperable(screen,      blended[c] = b[c] + s[c] - (b[c] * s[c]);)
ctx_float_blend_define_seperable(overlay,     blended[c] = b[c] < 0.5f ? (s[c] * b[c]) :
                                                          s[c] + b[c] - (s[c] * b[c]);)
ctx_float_blend_define_seperable(darken,      blended[c] = ctx_minf (b[c], s[c]))
ctx_float_blend_define_seperable(lighten,     blended[c] = ctx_maxf (b[c], s[c]))
ctx_float_blend_define_seperable(color_dodge, blended[c] = (b[c] == 0.0f) ? 0.0f :
                                     s[c] == 1.0f ? 1.0f : ctx_minf(1.0f, (b[c]) / (1.0f-s[c])))
ctx_float_blend_define_seperable(color_burn,  blended[c] = (b[c] == 1.0f) ? 1.0f :
                                     s[c] == 0.0f ? 0.0f : 1.0f - ctx_minf(1.0f, ((1.0f - b[c])) / s[c]))
ctx_float_blend_define_seperable(hard_light,  blended[c] = s[c] < 0.f ? (b[c] * s[c]) :
                                                          b[c] + s[c] - (b[c] * s[c]);)
ctx_float_blend_define_seperable(difference,  blended[c] = (b[c] - s[c]))

ctx_float_blend_define_seperable(divide,      blended[c] = s[c]?(b[c]) / s[c]:0.0f)
ctx_float_blend_define_seperable(addition,    blended[c] = s[c]+b[c])
ctx_float_blend_define_seperable(subtract,    blended[c] = s[c]-b[c])

ctx_float_blend_define_seperable(exclusion,   blended[c] = b[c] + s[c] - 2.0f * b[c] * s[c])
ctx_float_blend_define_seperable(soft_light,
  if (s[c] <= 0.5f)
  {
    blended[c] = b[c] - (1.0f - 2.0f * s[c]) * b[c] * (1.0f - b[c]);
  }
  else
  {
    int d;
    if (b[c] <= 255/4)
      d = (((16 * b[c] - 12.0f) * b[c] + 4.0f) * b[c]);
    else
      d = ctx_sqrtf(b[c]);
    blended[c] = (b[c] + (2.0f * s[c] - 1.0f) * (d - b[c]));
  }
)


ctx_float_blend_define(color,
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_float_set_lum(components, blended, ctx_float_get_lum (components, s));
)

ctx_float_blend_define(hue,
  float in_sat = ctx_float_get_sat(components, b);
  float in_lum = ctx_float_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_float_set_sat(components, blended, in_sat);
  ctx_float_set_lum(components, blended, in_lum);
)

ctx_float_blend_define(saturation,
  float in_sat = ctx_float_get_sat(components, s);
  float in_lum = ctx_float_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_float_set_sat(components, blended, in_sat);
  ctx_float_set_lum(components, blended, in_lum);
)

ctx_float_blend_define(luminosity,
  float in_lum = ctx_float_get_lum(components, s);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_float_set_lum(components, blended, in_lum);
)

inline static void
ctx_float_blend (int components, CtxBlend blend, float * __restrict__ dst, float *src, float *blended)
{
  switch (blend)
  {
    case CTX_BLEND_NORMAL:      ctx_float_blend_normal      (components, dst, src, blended); break;
    case CTX_BLEND_MULTIPLY:    ctx_float_blend_multiply    (components, dst, src, blended); break;
    case CTX_BLEND_SCREEN:      ctx_float_blend_screen      (components, dst, src, blended); break;
    case CTX_BLEND_OVERLAY:     ctx_float_blend_overlay     (components, dst, src, blended); break;
    case CTX_BLEND_DARKEN:      ctx_float_blend_darken      (components, dst, src, blended); break;
    case CTX_BLEND_LIGHTEN:     ctx_float_blend_lighten     (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_DODGE: ctx_float_blend_color_dodge (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_BURN:  ctx_float_blend_color_burn  (components, dst, src, blended); break;
    case CTX_BLEND_HARD_LIGHT:  ctx_float_blend_hard_light  (components, dst, src, blended); break;
    case CTX_BLEND_SOFT_LIGHT:  ctx_float_blend_soft_light  (components, dst, src, blended); break;
    case CTX_BLEND_DIFFERENCE:  ctx_float_blend_difference  (components, dst, src, blended); break;
    case CTX_BLEND_EXCLUSION:   ctx_float_blend_exclusion   (components, dst, src, blended); break;
    case CTX_BLEND_COLOR:       ctx_float_blend_color       (components, dst, src, blended); break;
    case CTX_BLEND_HUE:         ctx_float_blend_hue         (components, dst, src, blended); break;
    case CTX_BLEND_SATURATION:  ctx_float_blend_saturation  (components, dst, src, blended); break;
    case CTX_BLEND_LUMINOSITY:  ctx_float_blend_luminosity  (components, dst, src, blended); break;
    case CTX_BLEND_ADDITION:    ctx_float_blend_addition    (components, dst, src, blended); break;
    case CTX_BLEND_SUBTRACT:    ctx_float_blend_subtract    (components, dst, src, blended); break;
    case CTX_BLEND_DIVIDE:      ctx_float_blend_divide      (components, dst, src, blended); break;
  }
}

/* this is the grunt working function, when inlined code-path elimination makes
 * it produce efficient code.
 */
CTX_INLINE static void
ctx_float_porter_duff (CtxRasterizer         *rasterizer,
                       int                    components,
                       uint8_t * __restrict__ dst,
                       uint8_t * __restrict__ src,
                       int                    x0,
                       uint8_t * __restrict__ coverage,
                       int                    count,
                       CtxCompositingMode     compositing_mode,
                       CtxFragment            fragment,
                       CtxBlend               blend)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;

  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  float   global_alpha_f = rasterizer->state->gstate.global_alpha_f;
  
  {
    float tsrc[components];
    float u0, v0, ud, vd;
    ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
    if (blend == CTX_BLEND_NORMAL)
      ctx_float_blend (components, blend, dstf, srcf, tsrc);

    while (count--)
    {
      int cov = *coverage;
#if 1
      if (
        (compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 1.0f)||
        (compositing_mode == CTX_COMPOSITE_SOURCE_OVER      && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_XOR              && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_DESTINATION_OUT  && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      && cov == 0)
        )
      {
        u0 += ud;
        v0 += vd;
        coverage ++;
        dstf+=components;
        continue;
      }
#endif

      if (fragment)
      {
        fragment (rasterizer, u0, v0, tsrc);
        if (blend != CTX_BLEND_NORMAL)
          ctx_float_blend (components, blend, dstf, tsrc, tsrc);
      }
      else
      {
        if (blend != CTX_BLEND_NORMAL)
          ctx_float_blend (components, blend, dstf, srcf, tsrc);
      }
      u0 += ud;
      v0 += vd;
      float covf = ctx_u8_to_float (cov);

      if (global_alpha_u8 != 255)
        covf = covf * global_alpha_f;

      if (covf != 1.0f)
      {
        for (int c = 0; c < components; c++)
          tsrc[c] *= covf;
      }

      for (int c = 0; c < components; c++)
      {
        float res = 0.0f;
        /* these switches and this whole function disappear when
         * compiled when the enum values passed in are constants.
         */
        switch (f_s)
        {
          case CTX_PORTER_DUFF_0: break;
          case CTX_PORTER_DUFF_1:             res += (tsrc[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         res += (tsrc[c] *       dstf[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (tsrc[c] * (1.0f-dstf[components-1])); break;
        }
        switch (f_d)
        {
          case CTX_PORTER_DUFF_0: break;
          case CTX_PORTER_DUFF_1:             res += (dstf[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         res += (dstf[c] *       tsrc[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (dstf[c] * (1.0f-tsrc[components-1])); break;
        }
        dstf[c] = res;
      }
      coverage ++;
      dstf+=components;
    }
  }
}

/* generating one function per compositing_mode would be slightly more efficient,
 * but on embedded targets leads to slightly more code bloat,
 * here we trade off a slight amount of performance
 */
#define ctx_float_porter_duff(compformat, components, source, fragment, blend) \
static void \
ctx_##compformat##_porter_duff_##source (CTX_COMPOSITE_ARGUMENTS) \
{ \
   switch (rasterizer->state->gstate.compositing_mode) \
   { \
     case CTX_COMPOSITE_SOURCE_ATOP: \
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count, \
        CTX_COMPOSITE_SOURCE_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_ATOP:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_IN:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_IN, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OVER:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OVER:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_XOR:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_XOR, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OUT:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OUT:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_IN:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_IN, fragment, blend);\
       break;\
     case CTX_COMPOSITE_COPY:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_COPY, fragment, blend);\
       break;\
     case CTX_COMPOSITE_CLEAR:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_CLEAR, fragment, blend);\
       break;\
   }\
}

#if CTX_ENABLE_RGBAF

ctx_float_porter_duff(RGBAF, 4,color,           NULL,                               rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(RGBAF, 4,generic,         rasterizer->fragment,               rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
#if CTX_GRADIENTS
ctx_float_porter_duff(RGBAF, 4,linear_gradient, ctx_fragment_linear_gradient_RGBAF, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(RGBAF, 4,radial_gradient, ctx_fragment_radial_gradient_RGBAF, rasterizer->state->gstate.blend_mode)
#endif
ctx_float_porter_duff(RGBAF, 4,image,           ctx_fragment_image_RGBAF,           rasterizer->state->gstate.blend_mode)


#if CTX_GRADIENTS
#define ctx_float_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_float_porter_duff(comp_name, components,color_##blend_name,            NULL,                               blend_mode)\
ctx_float_porter_duff(comp_name, components,generic_##blend_name,          rasterizer->fragment,               blend_mode)\
ctx_float_porter_duff(comp_name, components,linear_gradient_##blend_name,  ctx_fragment_linear_gradient_RGBA8, blend_mode)\
ctx_float_porter_duff(comp_name, components,radial_gradient_##blend_name,  ctx_fragment_radial_gradient_RGBA8, blend_mode)\
ctx_float_porter_duff(comp_name, components,image_##blend_name,            ctx_fragment_image_RGBAF,           blend_mode)
#else
#define ctx_float_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_float_porter_duff(comp_name, components,color_##blend_name,            NULL,                               blend_mode)\
ctx_float_porter_duff(comp_name, components,generic_##blend_name,          rasterizer->fragment,               blend_mode)\
ctx_float_porter_duff(comp_name, components,image_##blend_name,            ctx_fragment_image_RGBAF,           blend_mode)
#endif

ctx_float_porter_duff_blend(RGBAF, 4, CTX_BLEND_NORMAL, normal)


static void
ctx_RGBAF_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_copy_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_RGBAF_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_clear_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_RGBAF_source_over_normal_opaque_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_over_normal_opaque_color (4, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

static void
ctx_setup_RGBAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 4;
  if (gstate->source.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_RGBAF_porter_duff_color;
      rasterizer->fragment = NULL;
      ctx_color_get_rgba (rasterizer->state, &gstate->source.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;
    }
  else
  {
    rasterizer->fragment = ctx_rasterizer_get_fragment_RGBAF (rasterizer);
    rasterizer->comp_op = ctx_RGBAF_porter_duff_generic;
  }


#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_RGBAF_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_RGBAF_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
        {
          rasterizer->comp_op = ctx_RGBA8_nop;
        }
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)(rasterizer->color))[components-1] == 0.0f)
                rasterizer->comp_op = ctx_RGBA8_nop;
              else if (((float*)(rasterizer->color))[components-1] == 1.0f)
                rasterizer->comp_op = ctx_RGBAF_source_over_normal_opaque_color;
              else
                rasterizer->comp_op = ctx_RGBAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
            }
            else
            {
              rasterizer->comp_op = ctx_RGBAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
            }
            break;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_linear_gradient_normal;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_radial_gradient_normal;
            break;
#endif
          case CTX_SOURCE_IMAGE:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_image_normal;
            break;
          default:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_color;
            rasterizer->fragment = NULL;
            break;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_linear_gradient;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_radial_gradient;
            break;
#endif
          case CTX_SOURCE_IMAGE:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_image;
            break;
          default:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
}

#endif
#if CTX_ENABLE_GRAYAF

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float rgba[4];
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0, rgba);
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgba);
  ((float*)out)[1] = rgba[3];
}

static void
ctx_fragment_radial_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float rgba[4];
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = 0.0f;
  if ((g->radial_gradient.r1-g->radial_gradient.r0) > 0.0f)
    {
      v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
      v = (v - g->radial_gradient.r0) / (g->radial_gradient.rdelta);
    }
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 0.0, rgba);
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgba);
  ((float*)out)[1] = rgba[3];
}
#endif

static void
ctx_fragment_color_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  CtxSource *g = &rasterizer->state->gstate.source;
  ctx_color_get_graya (rasterizer->state, &g->color, (float*)out);
}

static void ctx_fragment_image_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t rgba[4];
  float rgbaf[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (buffer->format->bpp)
    {
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba); break;
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba);       break;
    }
  for (int c = 0; c < 4; c ++) { rgbaf[c] = ctx_u8_to_float (rgba[c]); }
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgbaf);
  ((float*)out)[1] = rgbaf[3];
}

static CtxFragment ctx_rasterizer_get_fragment_GRAYAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_IMAGE:           return ctx_fragment_image_GRAYAF;
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_GRAYAF;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_GRAYAF;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_GRAYAF;
#endif
    }
  return ctx_fragment_color_GRAYAF;
}

ctx_float_porter_duff(GRAYAF, 2,color,   NULL,                 rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(GRAYAF, 2,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL

ctx_float_porter_duff(GRAYAF, 2,color_normal,   NULL,                 CTX_BLEND_NORMAL)
ctx_float_porter_duff(GRAYAF, 2,generic_normal, rasterizer->fragment, CTX_BLEND_NORMAL)

static void
ctx_GRAYAF_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_copy_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_GRAYAF_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_clear_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_GRAYAF_source_over_normal_opaque_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_over_normal_opaque_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

static void
ctx_setup_GRAYAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 2;
  if (gstate->source.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_GRAYAF_porter_duff_color;
      rasterizer->fragment = NULL;
      ctx_color_get_rgba (rasterizer->state, &gstate->source.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;
    }
  else
  {
    rasterizer->fragment = ctx_rasterizer_get_fragment_GRAYAF (rasterizer);
    rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic;
  }

#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_GRAYAF_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_GRAYAF_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
          rasterizer->comp_op = ctx_RGBA8_nop;
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = ctx_RGBA8_nop;
              else if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = ctx_GRAYAF_source_over_normal_opaque_color;
              else
                rasterizer->comp_op = ctx_GRAYAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
            }
            else
            {
              rasterizer->comp_op = ctx_GRAYAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
            }
            break;
          default:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_color;
            rasterizer->fragment = NULL;
            break;
          default:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
}

#endif
#if CTX_ENABLE_GRAYF

static void
ctx_composite_GRAYF (CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;

  float temp[count*2];
  for (int i = 0; i < count; i++)
  {
    temp[i*2] = dstf[i];
    temp[i*2+1] = 1.0f;
  }
  rasterizer->comp_op (rasterizer, (uint8_t*)temp, rasterizer->color, x0, coverage, count);
  for (int i = 0; i < count; i++)
  {
    dstf[i] = temp[i*2];
  }
}

#endif
#if CTX_ENABLE_BGRA8

inline static void
ctx_swap_red_green (uint8_t *rgba)
{
  uint32_t *buf  = (uint32_t *) rgba;
  uint32_t  orig = *buf;
  uint32_t  green_alpha = (orig & 0xff00ff00);
  uint32_t  red_blue    = (orig & 0x00ff00ff);
  uint32_t  red         = red_blue << 16;
  uint32_t  blue        = red_blue >> 16;
  *buf = green_alpha | red | blue;
}

static void
ctx_BGRA8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  uint32_t *srci = (uint32_t *) buf;
  uint32_t *dsti = (uint32_t *) rgba;
  while (count--)
    {
      uint32_t val = *srci++;
      ctx_swap_red_green ( (uint8_t *) &val);
      *dsti++      = val;
    }
}

static void
ctx_RGBA8_to_BGRA8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  ctx_BGRA8_to_RGBA8 (rasterizer, x, rgba, (uint8_t *) buf, count);
}

static void
ctx_composite_BGRA8 (CTX_COMPOSITE_ARGUMENTS)
{
  // for better performance, this could be done without a pre/post conversion,
  // by swapping R and B of source instead... as long as it is a color instead
  // of gradient or image
  //
  //
  uint8_t pixels[count * 4];
  ctx_BGRA8_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_BGRA8_to_RGBA8  (rasterizer, x0, &pixels[0], dst, count);
}

#endif
#if CTX_ENABLE_CMYKAF

static void
ctx_fragment_other_CMYKAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *cmyka = (float*)out;
  float rgba[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_IMAGE:
        ctx_fragment_image_RGBAF (rasterizer, x, y, rgba);
        break;
      case CTX_SOURCE_COLOR:
        ctx_fragment_color_RGBAF (rasterizer, x, y, rgba);
        break;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT:
        ctx_fragment_linear_gradient_RGBAF (rasterizer, x, y, rgba);
        break;
      case CTX_SOURCE_RADIAL_GRADIENT:
        ctx_fragment_radial_gradient_RGBAF (rasterizer, x, y, rgba);
        break;
#endif
      default:
        rgba[0]=rgba[1]=rgba[2]=rgba[3]=0.0f;
        break;
    }
  cmyka[4]=rgba[3];
  ctx_rgb_to_cmyk (rgba[0], rgba[1], rgba[2], &cmyka[0], &cmyka[1], &cmyka[2], &cmyka[3]);
}

static void
ctx_fragment_color_CMYKAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  float *cmyka = (float*)out;
  ctx_color_get_cmyka (rasterizer->state, &gstate->source.color, cmyka);
  // RGBW instead of CMYK
  for (int i = 0; i < 4; i ++)
    {
      cmyka[i] = (1.0f - cmyka[i]);
    }
}

static CtxFragment ctx_rasterizer_get_fragment_CMYKAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_COLOR:
        return ctx_fragment_color_CMYKAF;
    }
  return ctx_fragment_other_CMYKAF;
}

ctx_float_porter_duff (CMYKAF, 5,color,           NULL,                               rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff (CMYKAF, 5,generic,         rasterizer->fragment,               rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL

ctx_float_porter_duff (CMYKAF, 5,color_normal,            NULL,                               CTX_BLEND_NORMAL)
ctx_float_porter_duff (CMYKAF, 5,generic_normal,          rasterizer->fragment,               CTX_BLEND_NORMAL)

static void
ctx_CMYKAF_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_copy_normal (5, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_CMYKAF_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_clear_normal (5, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_CMYKAF_source_over_normal_opaque_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_over_normal_opaque_color (5, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

static void
ctx_setup_CMYKAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 5;
  if (gstate->source.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_CMYKAF_porter_duff_color;
      rasterizer->fragment = NULL;
      ctx_color_get_cmyka (rasterizer->state, &gstate->source.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        ((float*)rasterizer->color)[components-1] *= gstate->global_alpha_f;
    }
  else
  {
    rasterizer->fragment = ctx_rasterizer_get_fragment_CMYKAF (rasterizer);
    rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
  }


#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_CMYKAF_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_CMYKAF_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
          rasterizer->comp_op = ctx_RGBA8_nop;
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = ctx_RGBA8_nop;
              else if (((float*)rasterizer->color)[components-1] == 1.0f)
                rasterizer->comp_op = ctx_CMYKAF_source_over_normal_opaque_color;
              else
                rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
            }
            else
            {
              rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
            }
            break;
          default:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_color;
            rasterizer->fragment = NULL;
            break;
          default:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
}

#endif
#if CTX_ENABLE_CMYKA8

static void
ctx_CMYKA8_to_CMYKAF (CtxRasterizer *rasterizer, uint8_t *src, float *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      for (int c = 0; c < 4; c ++)
        { dst[c] = ctx_u8_to_float ( (255-src[c]) ); }
      dst[4] = ctx_u8_to_float (src[4]);
      for (int c = 0; c < 4; c++)
        { dst[c] *= dst[4]; }
      src += 5;
      dst += 5;
    }
}
static void
ctx_CMYKAF_to_CMYKA8 (CtxRasterizer *rasterizer, float *src, uint8_t *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      int a = ctx_float_to_u8 (src[4]);
      if (a != 0 && a != 255)
      {
        float recip = 1.0f/src[4];
        for (int c = 0; c < 4; c++)
        {
          dst[c] = ctx_float_to_u8 (1.0f - src[c] * recip);
        }
      }
      else
      {
        for (int c = 0; c < 4; c++)
          dst[c] = 255 - ctx_float_to_u8 (src[c]);
      }
      dst[4]=a;

      src += 5;
      dst += 5;
    }
}

static void
ctx_composite_CMYKA8 (CTX_COMPOSITE_ARGUMENTS)
{
  float pixels[count * 5];
  ctx_CMYKA8_to_CMYKAF (rasterizer, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, (uint8_t *) &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_CMYKAF_to_CMYKA8 (rasterizer, &pixels[0], dst, count);
}

#endif
#if CTX_ENABLE_CMYK8

static void
ctx_CMYK8_to_CMYKAF (CtxRasterizer *rasterizer, uint8_t *src, float *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      dst[0] = ctx_u8_to_float (255-src[0]);
      dst[1] = ctx_u8_to_float (255-src[1]);
      dst[2] = ctx_u8_to_float (255-src[2]);
      dst[3] = ctx_u8_to_float (255-src[3]);
      dst[4] = 1.0f;
      src += 4;
      dst += 5;
    }
}
static void
ctx_CMYKAF_to_CMYK8 (CtxRasterizer *rasterizer, float *src, uint8_t *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      float c = src[0];
      float m = src[1];
      float y = src[2];
      float k = src[3];
      float a = src[4];
      if (a != 0.0f && a != 1.0f)
        {
          float recip = 1.0f/a;
          c *= recip;
          m *= recip;
          y *= recip;
          k *= recip;
        }
      c = 1.0 - c;
      m = 1.0 - m;
      y = 1.0 - y;
      k = 1.0 - k;
      dst[0] = ctx_float_to_u8 (c);
      dst[1] = ctx_float_to_u8 (m);
      dst[2] = ctx_float_to_u8 (y);
      dst[3] = ctx_float_to_u8 (k);
      src += 5;
      dst += 4;
    }
}

static void
ctx_composite_CMYK8 (CTX_COMPOSITE_ARGUMENTS)
{
  float pixels[count * 5];
  ctx_CMYK8_to_CMYKAF (rasterizer, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, (uint8_t *) &pixels[0], src, x0, coverage, count);
  ctx_CMYKAF_to_CMYK8 (rasterizer, &pixels[0], dst, count);
}
#endif

static void
ctx_rasterizer_setup (CtxRasterizer *rasterizer)
{
  if (rasterizer->format->setup)
  {
    rasterizer->format->setup (rasterizer);
  }
#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source.type)
  {
    case CTX_SOURCE_LINEAR_GRADIENT:
    case CTX_SOURCE_RADIAL_GRADIENT:
      ctx_gradient_cache_prime (rasterizer);
  }
#endif
#endif
}

inline static void
ctx_rasterizer_apply_coverage (CtxRasterizer *rasterizer,
                               uint8_t * __restrict__ dst,
                               int            x,
                               uint8_t * __restrict__ coverage,
                               int            count)
{
  if (rasterizer->format->apply_coverage)
    rasterizer->format->apply_coverage(rasterizer, dst, rasterizer->color, x, coverage, count);
  else
    rasterizer->comp_op (rasterizer, dst, rasterizer->color, x, coverage, count);
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
#define CTX_EDGE_YMIN(no) CTX_EDGE(no).data.s16[1]
#define CTX_EDGE_X(no)     (rasterizer->edges[no].x)
  for (int t = 0; t < active_edges -1;)
    {
      int ymin = CTX_EDGE_YMIN (t);
      int next_t = t + 1;
      if (scanline != ymin)
        {
          if (winding)
            { parity += ( (CTX_EDGE (t).code == CTX_EDGE_FLIPPED) ?1:-1); }
          else
            { parity = 1 - parity; }
        }
      if (parity)
        {
          int x0 = CTX_EDGE_X (t)      / CTX_SUBDIV ;
          int x1 = CTX_EDGE_X (next_t) / CTX_SUBDIV ;
          int first = x0 / CTX_RASTERIZER_EDGE_MULTIPLIER;
          int last  = x1 / CTX_RASTERIZER_EDGE_MULTIPLIER;

          int graystart = 255 - ( (x0 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff);
          int grayend   = (x1 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff;

          if (first < minx)
            { first = minx;
              graystart=255;
            }
          if (last > maxx)
            { last = maxx;
              grayend=255;
            }
          if (first == last)
          {
            coverage[first] += (graystart-(255-grayend))/ aa_factor;
          }
          else if (first < last)
          {
                  /*
            if (aa_factor == 1)
            {
              coverage[first] += graystart;
              for (int x = first + 1; x < last; x++)
                coverage[x] = 255;
              coverage[last] = grayend;
            }
            else
            */
            {
              coverage[first] += graystart/ aa_factor;
              for (int x = first + 1; x < last; x++)
                coverage[x] += fraction;
              coverage[last]  += grayend/ aa_factor;
            }
          }
        }
      t = next_t;
    }

#if CTX_ENABLE_SHADOW_BLUR
  if (rasterizer->in_shadow)
  {
    float radius = rasterizer->state->gstate.shadow_blur;
    int dim = 2 * radius + 1;
    if (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM)
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
  if (rasterizer->clip_buffer)
  {
    /* perhaps not working right for clear? */
    int y = scanline / rasterizer->aa;
    uint8_t *clip_line = &((uint8_t*)(rasterizer->clip_buffer->data))[rasterizer->blit_width*y];
    // XXX SIMD candidate
    for (int x = minx; x <= maxx; x ++)
    {
#if CTX_1BIT_CLIP
        coverage[x] = (coverage[x] * ((clip_line[x/8]&(1<<(x%8)))?255:0))/255;
#else
        coverage[x] = (coverage[x] * clip_line[x])/255;
#endif
    }
  }
  if (rasterizer->aa == 1)
  {
    for (int x = minx; x <= maxx; x ++)
     coverage[x] = coverage[x] > 127?255:0;
  }
#endif
}

#undef CTX_EDGE_Y0
#undef CTX_EDGE

CTX_STATIC void
ctx_rasterizer_reset (CtxRasterizer *rasterizer)
{
#if CTX_RASTERIZER_FORCE_AA==0
  rasterizer->pending_edges   = 0;
#endif
  rasterizer->active_edges    = 0;
  rasterizer->has_shape       = 0;
  rasterizer->has_prev        = 0;
  rasterizer->edge_list.count = 0; // ready for new edges
  rasterizer->edge_pos        = 0;
  rasterizer->needs_aa        = 0;
  rasterizer->scanline        = 0;
  if (!rasterizer->preserve)
  {
    rasterizer->scan_min      = 5000;
    rasterizer->scan_max      = -5000;
    rasterizer->col_min       = 5000;
    rasterizer->col_max       = -5000;
  }
  //rasterizer->comp_op       = NULL;
}

static void
ctx_rasterizer_rasterize_edges (CtxRasterizer *rasterizer, int winding
#if CTX_SHAPE_CACHE
                                ,CtxShapeEntry *shape
#endif
                               )
{
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  int aa = rasterizer->aa;

  int scan_start = rasterizer->blit_y * aa;
  int scan_end   = scan_start + rasterizer->blit_height * aa;
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

  ctx_rasterizer_setup (rasterizer);

#if CTX_SHAPE_CACHE
  if (shape)
    {
      coverage = &shape->data[0];
    }
#endif
  ctx_assert (coverage);
  rasterizer->scan_min -= (rasterizer->scan_min % aa);
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
          dst += (rasterizer->blit_stride * (rasterizer->scan_min-scan_start) / aa);
          scan_start = rasterizer->scan_min;
        }
      if (rasterizer->scan_max < scan_end)
        { scan_end = rasterizer->scan_max; }
    }
  if (rasterizer->state->gstate.clip_min_y * aa > scan_start )
    { 
       dst += (rasterizer->blit_stride * (rasterizer->state->gstate.clip_min_y * aa -scan_start) / aa);
       scan_start = rasterizer->state->gstate.clip_min_y * aa; 
    }
  if (rasterizer->state->gstate.clip_max_y * aa < scan_end)
    { scan_end = rasterizer->state->gstate.clip_max_y * aa; }
  if (scan_start > scan_end ||
      (scan_start > (rasterizer->blit_y + rasterizer->blit_height) * aa) ||
      (scan_end < (rasterizer->blit_y) * aa))
  { 
    /* not affecting this rasterizers scanlines */
    ctx_rasterizer_reset (rasterizer);
    return;
  }
  ctx_rasterizer_sort_edges (rasterizer);
  if (maxx>minx)
  {
#if CTX_RASTERIZER_FORCE_AA==0
    int halfstep2 = aa/2;
    int halfstep  = aa/2 + 1;
#endif
    rasterizer->needs_aa = 0;
    rasterizer->scanline = scan_start-aa*200;
    ctx_rasterizer_feed_edges (rasterizer);
    ctx_rasterizer_discard_edges (rasterizer);
    ctx_rasterizer_increment_edges (rasterizer, aa * 200);
    rasterizer->scanline = scan_start;
    ctx_rasterizer_feed_edges (rasterizer);
    ctx_rasterizer_discard_edges (rasterizer);

  for (rasterizer->scanline = scan_start; rasterizer->scanline <= scan_end;)
    {
      ctx_memset (coverage, 0,
#if CTX_SHAPE_CACHE
                  shape?shape->width:
#endif
                  sizeof (_coverage) );
#if CTX_RASTERIZER_FORCE_AA==1
      rasterizer->needs_aa = 1;
#endif

#if CTX_RASTERIZER_FORCE_AA==0
      if (rasterizer->needs_aa
        || rasterizer->pending_edges
        || rasterizer->ending_edges
        || rasterizer->force_aa
        || aa == 1
          )
#endif
        {
          for (int i = 0; i < rasterizer->aa; i++)
            {
              ctx_rasterizer_sort_active_edges (rasterizer);
              ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, aa);
              rasterizer->scanline ++;
              ctx_rasterizer_increment_edges (rasterizer, 1);
              ctx_rasterizer_feed_edges (rasterizer);
  ctx_rasterizer_discard_edges (rasterizer);
            }
        }
#if CTX_RASTERIZER_FORCE_AA==0
      else
        {
          ctx_rasterizer_increment_edges (rasterizer, halfstep);
          ctx_rasterizer_sort_active_edges (rasterizer);
          ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, 1);
          ctx_rasterizer_increment_edges (rasterizer, halfstep2);
          rasterizer->scanline += rasterizer->aa;
          ctx_rasterizer_feed_edges (rasterizer);
  ctx_rasterizer_discard_edges (rasterizer);
        }
#endif
        {
#if CTX_SHAPE_CACHE
          if (shape == NULL)
#endif
            {
#if 0
              if (aa==1)
              {
                for (int x = 0; x < maxx-minx; x++)
                  coverage
              }
#endif
              ctx_rasterizer_apply_coverage (rasterizer,
                                             &dst[(minx * rasterizer->format->bpp) /8],
                                             minx,
                                             coverage, maxx-minx + 1);
            }
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
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_COPY ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_DESTINATION_ATOP ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_CLEAR)
  {
     /* fill in the rest of the blitrect when compositing mode permits it */
     uint8_t nocoverage[rasterizer->blit_width];
     //int gscan_start = rasterizer->state->gstate.clip_min_y * aa;
     int gscan_start = rasterizer->state->gstate.clip_min_y * aa;
     int gscan_end = rasterizer->state->gstate.clip_max_y * aa;
     memset (nocoverage, 0, sizeof(nocoverage));
     int startx   = rasterizer->state->gstate.clip_min_x;
     int endx     = rasterizer->state->gstate.clip_max_x;
     int clipw    = endx-startx + 1;
     uint8_t *dst = ( (uint8_t *) rasterizer->buf);

     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (gscan_start / aa);
     for (rasterizer->scanline = gscan_start; rasterizer->scanline < scan_start;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, clipw);
       rasterizer->scanline += aa;
       dst += rasterizer->blit_stride;
     }
     if (minx < startx)
     {
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_start / aa);
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
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_start / aa);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (maxx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, endx-maxx);

       rasterizer->scanline += aa;
       dst += rasterizer->blit_stride;
     }
     }
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_end / aa);
     // XXX valgrind/asan this
     if(0)for (rasterizer->scanline = scan_end; rasterizer->scanline/aa < gscan_end-1;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, clipw-1);

       rasterizer->scanline += aa;
       dst += rasterizer->blit_stride;
     }
  }
  ctx_rasterizer_reset (rasterizer);
}


static int
ctx_rasterizer_fill_rect (CtxRasterizer *rasterizer,
                          int          x0,
                          int          y0,
                          int          x1,
                          int          y1)
{
  int aa = rasterizer->aa;
  if (x0>x1 || y0>y1) { return 1; } // XXX : maybe this only happens under
  //       memory corruption
  if (x1 % CTX_SUBDIV ||
      x0 % CTX_SUBDIV ||
      y1 % aa ||
      y0 % aa)
    { return 0; }
  x1 /= CTX_SUBDIV;
  x0 /= CTX_SUBDIV;
  y1 /= aa;
  y0 /= aa;
  uint8_t coverage[x1-x0 + 1];
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  ctx_rasterizer_setup (rasterizer);
  ctx_memset (coverage, 0xff, sizeof (coverage) );
  if (x0 < rasterizer->blit_x)
    { x0 = rasterizer->blit_x; }
  if (y0 < rasterizer->blit_y)
    { y0 = rasterizer->blit_y; }
  if (y1 > rasterizer->blit_y + rasterizer->blit_height)
    { y1 = rasterizer->blit_y + rasterizer->blit_height; }
  if (x1 > rasterizer->blit_x + rasterizer->blit_width)
    { x1 = rasterizer->blit_x + rasterizer->blit_width; }
  dst += (y0 - rasterizer->blit_y) * rasterizer->blit_stride;
  int width = x1 - x0;
  if (width > 0)
    {
      rasterizer->scanline = y0 * aa;
      for (int y = y0; y < y1; y++)
        {
          rasterizer->scanline += aa;
          ctx_rasterizer_apply_coverage (rasterizer,
                                         &dst[ (x0) * rasterizer->format->bpp/8],
                                         x0,
                                         coverage, width);
          dst += rasterizer->blit_stride;
        }
    }
  return 1;
}

inline static int
ctx_is_transparent (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (gstate->global_alpha_u8 == 0)
    return 1;
  if (gstate->source.type == CTX_SOURCE_COLOR)
  {
    uint8_t ga[2];
    ctx_color_get_graya_u8 (rasterizer->state, &gstate->source.color, ga);
    if (ga[1] == 0)
      return 1;
  }
  return 0;
}

static void
ctx_rasterizer_fill (CtxRasterizer *rasterizer)
{
  int count = rasterizer->preserve?rasterizer->edge_list.count:0;
  int aa = rasterizer->aa;

  CtxEntry temp[count]; /* copy of already built up path's poly line
                          XXX - by building a large enough path
                          the stack can be smashed!
                         */
  if (rasterizer->preserve)
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) ); }

  if (ctx_is_transparent (rasterizer))
  {
     ctx_rasterizer_reset (rasterizer);
     goto done;
  }

#if CTX_ENABLE_SHADOW_BLUR
  if (rasterizer->in_shadow)
  {
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      entry->data.s16[2] += rasterizer->shadow_x * CTX_SUBDIV;
      entry->data.s16[3] += rasterizer->shadow_y * aa;
    }
    rasterizer->scan_min += rasterizer->shadow_y * aa;
    rasterizer->scan_max += rasterizer->shadow_y * aa;
    rasterizer->col_min  += (rasterizer->shadow_x - rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
    rasterizer->col_max  += (rasterizer->shadow_x + rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
  }
#endif

#if 1
  if (rasterizer->scan_min / aa > rasterizer->blit_y + rasterizer->blit_height ||
      rasterizer->scan_max / aa < rasterizer->blit_y)
    {
      ctx_rasterizer_reset (rasterizer);
      goto done;
    }
#endif
#if 1
  if (rasterizer->col_min / CTX_SUBDIV > rasterizer->blit_x + rasterizer->blit_width ||
      rasterizer->col_max / CTX_SUBDIV < rasterizer->blit_x)
    {
      ctx_rasterizer_reset (rasterizer);
      goto done;
    }
#endif
  ctx_rasterizer_setup (rasterizer);


  rasterizer->state->min_x =
    ctx_mini (rasterizer->state->min_x, rasterizer->col_min / CTX_SUBDIV);
  rasterizer->state->max_x =
    ctx_maxi (rasterizer->state->max_x, rasterizer->col_max / CTX_SUBDIV);
  rasterizer->state->min_y =
    ctx_mini (rasterizer->state->min_y, rasterizer->scan_min / aa);
  rasterizer->state->max_y =
    ctx_maxi (rasterizer->state->max_y, rasterizer->scan_max / aa);
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
           (entry2->data.s16[2] == entry3->data.s16[2]) &&
           ((entry3->data.s16[2] & (CTX_SUBDIV-1)) == 0)  &&
           ((entry3->data.s16[3] & (aa-1)) == 0)
#if CTX_ENABLE_SHADOW_BLUR
           && !rasterizer->in_shadow
#endif
         )
        {
          if (ctx_rasterizer_fill_rect (rasterizer,
                                        entry3->data.s16[2],
                                        entry3->data.s16[3],
                                        entry1->data.s16[2],
                                        entry1->data.s16[3]) )
            {
              ctx_rasterizer_reset (rasterizer);
              goto done;
            }
        }
    }
  ctx_rasterizer_finish_shape (rasterizer);


#if CTX_SHAPE_CACHE
  uint32_t hash = ctx_rasterizer_poly_to_edges (rasterizer);
  int width = (rasterizer->col_max + (CTX_SUBDIV-1) ) / CTX_SUBDIV - rasterizer->col_min/CTX_SUBDIV + 1;
  int height = (rasterizer->scan_max + (aa-1) ) / aa - rasterizer->scan_min / aa + 1;
  if (width * height < CTX_SHAPE_CACHE_DIM && width >=1 && height >= 1
      && width < CTX_SHAPE_CACHE_MAX_DIM
      && height < CTX_SHAPE_CACHE_MAX_DIM && !rasterizer->state->gstate.clipped &&
#if CTX_ENABLE_SHADOW_BLUR
      !rasterizer->in_shadow
#endif
      )
    {
      int scan_min = rasterizer->scan_min;
      int col_min = rasterizer->col_min;
      CtxShapeEntry *shape = ctx_shape_entry_find (hash, width, height, ctx_shape_time++);
      if (shape->uses == 0)
        {
          ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule, shape);
        }
      scan_min -= (scan_min % aa);
      rasterizer->scanline = scan_min;
      int y0 = rasterizer->scanline / aa;
      int y1 = y0 + shape->height;
      int x0 = col_min / CTX_SUBDIV;
      int ymin = y0;
      int x1 = x0 + shape->width;
      int clip_x_min = rasterizer->blit_x;
      int clip_x_max = rasterizer->blit_x + rasterizer->blit_width - 1;
      int clip_y_min = rasterizer->blit_y;
      int clip_y_max = rasterizer->blit_y + rasterizer->blit_height - 1;
#if 0
      if (rasterizer->state->gstate.clip_min_x>
          clip_x_min)
        { clip_x_min = rasterizer->state->gstate.clip_min_x; }
      if (rasterizer->state->gstate.clip_max_x <
          clip_x_max)
        { clip_x_max = rasterizer->state->gstate.clip_max_x; }
#endif
#if 0
      if (rasterizer->state->gstate.clip_min_y>
          clip_y_min)
        { clip_y_min = rasterizer->state->gstate.clip_min_y; }
      if (rasterizer->state->gstate.clip_max_y <
          clip_y_max)
        { clip_y_max = rasterizer->state->gstate.clip_max_y; }
#endif
      if (x1 >= clip_x_max)
        { x1 = clip_x_max; }
      int xo = 0;
      if (x0 < clip_x_min)
        {
          xo = clip_x_min - x0;
          x0 = clip_x_min;
        }
      int ewidth = x1 - x0;
      if (ewidth>0)
        for (int y = y0; y < y1; y++)
          {
            if ( (y >= clip_y_min) && (y <= clip_y_max) )
              {
                ctx_rasterizer_apply_coverage (rasterizer,
                                               ( (uint8_t *) rasterizer->buf) + (y-rasterizer->blit_y) * rasterizer->blit_stride + (int) (x0) * rasterizer->format->bpp/8,
                                               x0,
                                               &shape->data[shape->width * (int) (y-ymin) + xo],
                                               ewidth );
              }
          }
      if (shape->uses != 0)
        {
          ctx_rasterizer_reset (rasterizer);
        }
      ctx_shape_entry_release (shape);
      goto done;
    }
  else
#else
  ctx_rasterizer_poly_to_edges (rasterizer);
#endif

    {
      //fprintf (stderr, "%i %i\n", width, height);
      ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                      , NULL
#endif
                                     );
    }

done:
  if (rasterizer->preserve)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
    }
#if CTX_ENABLE_SHADOW_BLUR
  if (rasterizer->in_shadow)
  {
    rasterizer->scan_min -= rasterizer->shadow_y * aa;
    rasterizer->scan_max -= rasterizer->shadow_y * aa;
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
  _ctx_glyph (rasterizer->ctx, unichar, stroke);
}

typedef struct _CtxTermGlyph CtxTermGlyph;

struct _CtxTermGlyph
{
  uint32_t unichar;
  int      col;
  int      row;
  uint8_t  rgba_bg[4];
  uint8_t  rgba_fg[4];
};

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible);
static void
ctx_rasterizer_text (CtxRasterizer *rasterizer, const char *string, int stroke)
{
#if CTX_BRAILLE_TEXT
  if (rasterizer->term_glyphs && !stroke)
  {
    int col = rasterizer->x / 2 + 1;
    int row = rasterizer->y / 4 + 1;
    for (int i = 0; string[i]; i++, col++)
    {
      CtxTermGlyph *glyph = calloc (sizeof (CtxTermGlyph), 1);
      ctx_list_prepend (&rasterizer->glyphs, glyph);
      glyph->unichar = string[i];
      glyph->col = col;
      glyph->row = row;
      ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color,
                      glyph->rgba_fg);
    }
    //_ctx_text (rasterizer->ctx, string, stroke, 1);
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
                    float        x,
                    float        y,
                    float        radius,
                    float        start_angle,
                    float        end_angle,
                    int          anticlockwise)
{
  int full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS;
  full_segments = radius * CTX_PI;
  if (full_segments > CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS)
    { full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS; }
  float step = CTX_PI*2.0/full_segments;
  int steps;
  if (end_angle == start_angle)
    {
//  if (rasterizer->has_prev!=0)
      ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                              y + ctx_sinf (end_angle) * radius);
//    else
//    ctx_rasterizer_move_to (rasterizer, x + ctx_cosf (end_angle) * radius,
//                          y + ctx_sinf (end_angle) * radius);
      return;
    }
#if 0
  if ( (!anticlockwise && (end_angle - start_angle >= CTX_PI*2) ) ||
       ( (anticlockwise && (start_angle - end_angle >= CTX_PI*2) ) ) )
    {
      end_angle = start_angle;
      steps = full_segments - 1;
    }
  else
#endif
    {
      steps = (end_angle - start_angle) / (CTX_PI*2) * full_segments;
      //if (steps < 0) steps += full_segments;
      if (anticlockwise)
        { steps = full_segments - steps; };
    }
  if (anticlockwise) { step = step * -1; }
  int first = 1;
  if (steps == 0 || steps==full_segments -1  || (anticlockwise && steps == full_segments) )
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
   *       though it will increase the code-size
   */
  ctx_rasterizer_curve_to (rasterizer,
                           (cx * 2 + rasterizer->x) / 3.0f, (cy * 2 + rasterizer->y) / 3.0f,
                           (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                           x,                              y);
}

CTX_STATIC void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                            float cx, float cy, float x, float y)
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
  ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color, fg_color);
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
  int aa = rasterizer->aa;
  int start = 0;
  int end = 0;
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
  int count = rasterizer->edge_list.count;
  int preserved = rasterizer->preserve;
  int aa = rasterizer->aa;
  CtxEntry temp[count]; /* copy of already built up path's poly line  */
  memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) );
#if 1
  if (rasterizer->state->gstate.line_width <= 0.0f &&
      rasterizer->state->gstate.line_width > -10.0f)
    {
      ctx_rasterizer_stroke_1px (rasterizer);
    }
  else
#endif
    {
      ctx_rasterizer_reset (rasterizer); /* then start afresh with our stroked shape  */
      CtxMatrix transform_backup = rasterizer->state->gstate.transform;
      ctx_matrix_identity (&rasterizer->state->gstate.transform);
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float half_width_x = rasterizer->state->gstate.line_width/2;
      float half_width_y = rasterizer->state->gstate.line_width/2;
      if (rasterizer->state->gstate.line_width <= 0.0f)
        {
          half_width_x = .5;
          half_width_y = .5;
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
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              if (length>0.001f)
                {
                  dx = dx/length * half_width_x;
                  dy = dy/length * half_width_y;
                  if (entry->code == CTX_NEW_EDGE)
                    {
                      ctx_rasterizer_finish_shape (rasterizer);
                      ctx_rasterizer_move_to (rasterizer, prev_x+dy, prev_y-dx);
                    }
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  // XXX possible miter line-to
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
              if (length>0.001f)
                {
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  // XXX possible miter line-to
                  ctx_rasterizer_line_to (rasterizer, x-dy,      y+dx);
                }
              prev_x = x;
              prev_y = y;
              if (entry->code == CTX_NEW_EDGE)
                {
                  x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[1] * 1.0f / aa;
                  dx = x - prev_x;
                  dy = y - prev_y;
                  length = ctx_fast_hypotf (dx, dy);
                  if (length>0.001f)
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
      switch (rasterizer->state->gstate.line_cap)
        {
          case CTX_CAP_SQUARE: // XXX:NYI
          case CTX_CAP_NONE: /* nothing to do */
            break;
          case CTX_CAP_ROUND:
            {
              float x = 0, y = 0;
              int has_prev = 0;
              for (int i = 0; i < count; i++)
                {
                  CtxEntry *entry = &temp[i];
                  if (entry->code == CTX_NEW_EDGE)
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
      switch (rasterizer->state->gstate.line_join)
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
                  if (entry[1].code == CTX_EDGE)
                    {
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                }
              break;
            }
        }
      CtxFillRule rule_backup = rasterizer->state->gstate.fill_rule;
      rasterizer->state->gstate.fill_rule = CTX_FILL_RULE_WINDING;
      rasterizer->preserve = 0; // so fill isn't tripped
      ctx_rasterizer_fill (rasterizer);
      rasterizer->state->gstate.fill_rule = rule_backup;
      //rasterizer->state->gstate.source = source_backup;
      rasterizer->state->gstate.transform = transform_backup;
    }
  if (preserved)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
}

#if CTX_1BIT_CLIP
#define CTX_CLIP_FORMAT CTX_FORMAT_GRAY1
#else
#define CTX_CLIP_FORMAT CTX_FORMAT_GRAY8
#endif

CTX_STATIC void
ctx_rasterizer_clip (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  int aa = rasterizer->aa;
  CtxEntry temp[count]; /* copy of already built up path's poly line  */
  rasterizer->state->has_clipped=1;
  rasterizer->state->gstate.clipped=1;
  if (rasterizer->preserve)
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) ); }
#if CTX_ENABLE_CLIP
  if (!rasterizer->clip_buffer)
  {
    rasterizer->clip_buffer = ctx_buffer_new (rasterizer->blit_width,
                                              rasterizer->blit_height,
                                              CTX_CLIP_FORMAT);
  }

  // for now only one level of clipping is supported
  {

  int prev_x = 0;
  int prev_y = 0;

    Ctx *ctx = ctx_new_for_framebuffer (rasterizer->clip_buffer->data, rasterizer->blit_width, rasterizer->blit_height,
       rasterizer->blit_width,
       CTX_CLIP_FORMAT);
    memset (rasterizer->clip_buffer->data, 0, rasterizer->blit_width * rasterizer->blit_height);

  for (int i = 0; i < count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
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
#endif
  int minx = 5000;
  int miny = 5000;
  int maxx = -5000;
  int maxy = -5000;
  int prev_x = 0;
  int prev_y = 0;
  for (int i = 0; i < count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
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
    }
  rasterizer->state->gstate.clip_min_x = ctx_maxi (minx,
                                         rasterizer->state->gstate.clip_min_x);
  rasterizer->state->gstate.clip_min_y = ctx_maxi (miny,
                                         rasterizer->state->gstate.clip_min_y);
  rasterizer->state->gstate.clip_max_x = ctx_mini (maxx,
                                         rasterizer->state->gstate.clip_max_x);
  rasterizer->state->gstate.clip_max_y = ctx_mini (maxy,
                                         rasterizer->state->gstate.clip_max_y);
  ctx_rasterizer_reset (rasterizer);
  if (rasterizer->preserve)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
}

static void
ctx_rasterizer_set_texture (CtxRasterizer *rasterizer,
                            int   no,
                            float x,
                            float y)
{
  if (no < 0 || no >= CTX_MAX_TEXTURES) { no = 0; }
  if (rasterizer->texture_source->texture[no].data == NULL)
    {
      ctx_log ("failed setting texture %i\n", no);
      return;
    }
  rasterizer->state->gstate.source.type = CTX_SOURCE_IMAGE;
  rasterizer->state->gstate.source.image.buffer = &rasterizer->texture_source->texture[no];
  //ctx_user_to_device (rasterizer->state, &x, &y);
  rasterizer->state->gstate.source.image.x0 = 0;
  rasterizer->state->gstate.source.image.y0 = 0;
  rasterizer->state->gstate.source.transform = rasterizer->state->gstate.transform;
  ctx_matrix_translate (&rasterizer->state->gstate.source.transform, x, y);
  ctx_matrix_invert (&rasterizer->state->gstate.source.transform);
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

CTX_STATIC void
ctx_rasterizer_set_pixel (CtxRasterizer *rasterizer,
                          uint16_t x,
                          uint16_t y,
                          uint8_t r,
                          uint8_t g,
                          uint8_t b,
                          uint8_t a)
{
  rasterizer->state->gstate.source.type = CTX_SOURCE_COLOR;
  ctx_color_set_RGBA8 (rasterizer->state, &rasterizer->state->gstate.source.color, r, g, b, a);
#if 1
  ctx_rasterizer_pset (rasterizer, x, y, 255);
#else
  ctx_rasterizer_move_to (rasterizer, x, y);
  ctx_rasterizer_rel_line_to (rasterizer, 1, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, 1);
  ctx_rasterizer_rel_line_to (rasterizer, -1, 0);
  ctx_rasterizer_fill (rasterizer);
#endif
}

CTX_STATIC void
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
  ctx_rasterizer_rel_line_to (rasterizer, 0.3, 0);
  ctx_rasterizer_finish_shape (rasterizer);
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

  if (radius > width/2) radius = width/2;
  if (radius > height/2) radius = height/2;

  ctx_rasterizer_finish_shape (rasterizer);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + radius, radius, 180 * degrees, 270 * degrees, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

static void
ctx_rasterizer_process (void *user_data, CtxCommand *command);

static int
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
    ctx_u8 (CTX_COMPOSITING_MODE, comp,  0,0,0,0,0,0,0),
    ctx_u8 (CTX_BLEND_MODE,       blend, 0,0,0,0,0,0,0),
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
  int id = ctx_texture_init (rasterizer->ctx, -1,
                  rasterizer->blit_width,
                  rasterizer->blit_height,
                  rasterizer->format->ebpp * 8,
                  (uint8_t*)rasterizer->group[no]->data,
                  NULL, NULL);
  {
     CtxEntry commands[2] =
      {ctx_u32 (CTX_TEXTURE, id, 0),
       ctx_f  (CTX_CONT, rasterizer->blit_x, rasterizer->blit_y)};
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
  ctx_texture_release (rasterizer->ctx, id);
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
ctx_rasterizer_process (void *user_data, CtxCommand *command)
{
  CtxEntry *entry = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) user_data;
  CtxState *state = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  switch (c->code)
    {
#if CTX_ENABLE_SHADOW_BLUR
      case CTX_SHADOW_COLOR:
        {
          CtxColor  col;
          CtxColor *color = &col;
          //state->gstate.source.type = CTX_SOURCE_COLOR;
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
      case CTX_TEXTURE:
        ctx_rasterizer_set_texture (rasterizer, ctx_arg_u32 (0),
                                    ctx_arg_float (2), ctx_arg_float (3) );
        rasterizer->comp_op = NULL;
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
        break;
#if CTX_COMPOSITING_GROUPS
      case CTX_START_GROUP:
        ctx_rasterizer_start_group (rasterizer);
        break;
      case CTX_END_GROUP:
        ctx_rasterizer_end_group (rasterizer);
        break;
#endif

#if 0
      case CTX_SET:
        ctx_state_set_string (rasterizer->state,
                        c->set.key_hash,
                        c->set.utf8);
        break;
#endif
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_SAVE:
      case CTX_RESTORE:
        rasterizer->comp_op = NULL;
        rasterizer->uses_transforms = 1;
        ctx_interpret_transforms (rasterizer->state, entry, NULL);
        break;
      case CTX_STROKE:
#if CTX_ENABLE_SHADOW_BLUR
        if (rasterizer->state->gstate.shadow_blur > 0.0 &&
            !rasterizer->in_text)
          ctx_rasterizer_shadow_stroke (rasterizer);
#endif
        ctx_rasterizer_stroke (rasterizer);
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
        break;
      case CTX_TEXT_STROKE:
        ctx_rasterizer_text (rasterizer, ctx_arg_string(), 1);
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
  ctx_interpret_style (rasterizer->state, entry, NULL);
  if (command->code == CTX_LINE_WIDTH)
    {
      float x = state->gstate.line_width;
      /* normalize line width according to scaling factor
       */
      x = x * ctx_maxf (ctx_maxf (ctx_fabsf (state->gstate.transform.m[0][0]),
                                  ctx_fabsf (state->gstate.transform.m[0][1]) ),
                        ctx_maxf (ctx_fabsf (state->gstate.transform.m[1][0]),
                                  ctx_fabsf (state->gstate.transform.m[1][1]) ) );
      state->gstate.line_width = x;
    }
}

void
ctx_rasterizer_deinit (CtxRasterizer *rasterizer)
{
  ctx_renderstream_deinit (&rasterizer->edge_list);
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
  {
    ctx_buffer_free (rasterizer->clip_buffer);
    rasterizer->clip_buffer = NULL;
  }
#endif
  free (rasterizer);
}

int ctx_renderer_is_sdl (Ctx *ctx);
int ctx_renderer_is_fb  (Ctx *ctx);

static int _ctx_is_rasterizer (Ctx *ctx);
CtxAntialias ctx_get_antialias (Ctx *ctx)
{
#if CTX_EVENTS
  if (ctx_renderer_is_sdl (ctx) || ctx_renderer_is_fb (ctx))
  {
     CtxThreaded *fb = (CtxThreaded*)(ctx->renderer);
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
    case CTX_ANTIALIAS_DEFAULT: return 15;
    case CTX_ANTIALIAS_BEST: return 17;
  }
}

void
ctx_set_antialias (Ctx *ctx, CtxAntialias antialias)
{
#if CTX_EVENTS
  if (ctx_renderer_is_sdl (ctx) || ctx_renderer_is_fb (ctx))
  {
     CtxThreaded *fb = (CtxThreaded*)(ctx->renderer);
     fb->antialias = antialias;
     for (int i = 0; i < _ctx_threads; i++)
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



#if CTX_ENABLE_RGB8

inline static void
ctx_RGB8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (const uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = pixel[1];
      rgba[2] = pixel[2];
      rgba[3] = 255;
      pixel+=3;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_RGB8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = rgba[0];
      pixel[1] = rgba[1];
      pixel[2] = rgba[2];
      pixel+=3;
      rgba +=4;
    }
}

#endif
#if CTX_ENABLE_GRAY1

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY1_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      if (*pixel & (1<< (x&7) ) )
        {
          rgba[0] = 255;
          rgba[1] = 255;
        }
      else
        {
          rgba[0] = 0;
          rgba[1] = 255;
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int gray = rgba[0];
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/aa, 0, 127);
      if (gray < 127)
        {
          *pixel = *pixel & (~ (1<< (x&7) ) );
        }
      else
        {
          *pixel = *pixel | (1<< (x&7) );
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

#else

inline static void
ctx_GRAY1_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      if (*pixel & (1<< (x&7) ) )
        {
          rgba[0] = 255;
          rgba[1] = 255;
          rgba[2] = 255;
          rgba[3] = 255;
        }
      else
        {
          rgba[0] = 0;
          rgba[1] = 0;
          rgba[2] = 0;
          rgba[3] = 255;
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int gray = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/aa, 0, 127);
      if (gray < 127)
        {
          *pixel = *pixel & (~ (1<< (x&7) ) );
        }
      else
        {
          *pixel = *pixel | (1<< (x&7) );
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}
#endif

#endif
#if CTX_ENABLE_GRAY2

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY2_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (3 << ( (x & 3) <<1) ) ) >> ( (x&3) <<1);
      val <<= 6;
      rgba[0] = val;
      rgba[1] = 255;
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY2 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = rgba[0];
      val >>= 6;
      *pixel = *pixel & (~ (3 << ( (x&3) <<1) ) );
      *pixel = *pixel | ( (val << ( (x&3) <<1) ) );
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}
#else

inline static void
ctx_GRAY2_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (3 << ( (x & 3) <<1) ) ) >> ( (x&3) <<1);
      val <<= 6;
      rgba[0] = val;
      rgba[1] = val;
      rgba[2] = val;
      rgba[3] = 255;
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY2 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      val >>= 6;
      *pixel = *pixel & (~ (3 << ( (x&3) <<1) ) );
      *pixel = *pixel | ( (val << ( (x&3) <<1) ) );
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}
#endif

#endif
#if CTX_ENABLE_GRAY4

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY4_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (15 << ( (x & 1) <<2) ) ) >> ( (x&1) <<2);
      val <<= 4;
      rgba[0] = val;
      rgba[1] = 255;
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY4 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = rgba[0];
      val >>= 4;
      *pixel = *pixel & (~ (15 << ( (x&1) <<2) ) );
      *pixel = *pixel | ( (val << ( (x&1) <<2) ) );
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}
#else
inline static void
ctx_GRAY4_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (15 << ( (x & 1) <<2) ) ) >> ( (x&1) <<2);
      val <<= 4;
      rgba[0] = val;
      rgba[1] = val;
      rgba[2] = val;
      rgba[3] = 255;
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY4 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      val >>= 4;
      *pixel = *pixel & (~ (15 << ( (x&1) <<2) ) );
      *pixel = *pixel | ( (val << ( (x&1) <<2) ) );
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}
#endif

#endif
#if CTX_ENABLE_GRAY8

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY8_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = 255;
      pixel+=1;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = rgba[0];
      pixel+=1;
      rgba +=2;
    }
}
#else
inline static void
ctx_GRAY8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = pixel[0];
      rgba[2] = pixel[0];
      rgba[3] = 255;
      pixel+=1;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      // for internal uses... using only green would work
      pixel+=1;
      rgba +=4;
    }
}
#endif

#endif
#if CTX_ENABLE_GRAYA8

inline static void
ctx_GRAYA8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (const uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = pixel[0];
      rgba[2] = pixel[0];
      rgba[3] = pixel[1];
      pixel+=2;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      pixel[1] = rgba[3];
      pixel+=2;
      rgba +=4;
    }
}

#if CTX_NATIVE_GRAYA8
CTX_INLINE static void ctx_rgba_to_graya_u8 (CtxState *state, uint8_t *in, uint8_t *out)
{
  out[0] = ctx_u8_color_rgb_to_gray (state, in);
  out[1] = in[3];
}

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  ctx_fragment_gradient_1d_GRAYA8 (rasterizer, v, 1.0, (uint8_t*)out);
#if CTX_DITHER
  ctx_dither_graya_u8 ((uint8_t*)out, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

#if 0
static void
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = (ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, rgba);
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}
#endif


static void
ctx_fragment_radial_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = (ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, (uint8_t*)out);
#if CTX_DITHER
  ctx_dither_graya_u8 ((uint8_t*)out, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}
#endif

static void
ctx_fragment_color_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  CtxSource *g = &rasterizer->state->gstate.source;
  ctx_color_get_graya_u8 (rasterizer->state, &g->color, out);
}

static void ctx_fragment_image_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t rgba[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (buffer->format->bpp)
    {
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba); break;
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba);       break;
    }
  ctx_rgba_to_graya_u8 (rasterizer->state, rgba, (uint8_t*)out);
}

static CtxFragment ctx_rasterizer_get_fragment_GRAYA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_IMAGE:           return ctx_fragment_image_GRAYA8;
#if CTX_GRADIENTS
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_GRAYA8;
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_GRAYA8;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_GRAYA8;
#endif
    }
  return ctx_fragment_color_GRAYA8;
}

ctx_u8_porter_duff(GRAYA8, 2,color,   NULL,                 rasterizer->state->gstate.blend_mode)
ctx_u8_porter_duff(GRAYA8, 2,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL

ctx_u8_porter_duff(GRAYA8, 2,color_normal,   NULL,                 CTX_BLEND_NORMAL)
ctx_u8_porter_duff(GRAYA8, 2,generic_normal, rasterizer->fragment, CTX_BLEND_NORMAL)

static void
ctx_GRAYA8_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_copy_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_GRAYA8_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_clear_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_GRAYA8_source_over_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_source_over_normal_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}

static void
ctx_GRAYA8_source_over_normal_opaque_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_source_over_normal_opaque_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

inline static int
ctx_is_opaque_color (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (gstate->global_alpha_u8 != 255)
    return 0;
  if (gstate->source.type == CTX_SOURCE_COLOR)
  {
    uint8_t ga[2];
    ctx_color_get_graya_u8 (rasterizer->state, &gstate->source.color, ga);
    return ga[1] == 255;
  }
  return 0;
}

static void
ctx_setup_GRAYA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 2;
  if (gstate->source.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_GRAYA8_porter_duff_color;
      rasterizer->fragment = NULL;
      ctx_color_get_rgba8 (rasterizer->state, &gstate->source.color, rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          rasterizer->color[c] = (rasterizer->color[c] * gstate->global_alpha_u8)/255;
      rasterizer->color[0] = ctx_u8_color_rgb_to_gray (rasterizer->state, rasterizer->color);
      rasterizer->color[1] = rasterizer->color[3];
    }
  else
  {
    rasterizer->fragment = ctx_rasterizer_get_fragment_GRAYA8 (rasterizer);
    rasterizer->comp_op  = ctx_GRAYA8_porter_duff_generic;
  }

#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_GRAYA8_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_GRAYA8_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
          rasterizer->comp_op = ctx_RGBA8_nop;
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (rasterizer->color[components-1] == 0)
                rasterizer->comp_op = ctx_RGBA8_nop;
              else if (rasterizer->color[components-1] == 255)
                rasterizer->comp_op = ctx_GRAYA8_source_over_normal_opaque_color;
              else
                rasterizer->comp_op = ctx_GRAYA8_source_over_normal_color;
              rasterizer->fragment = NULL;
            }
            else
            {
              rasterizer->comp_op = ctx_GRAYA8_porter_duff_color_normal;
              rasterizer->fragment = NULL;
            }
            break;
          default:
            rasterizer->comp_op = ctx_GRAYA8_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_GRAYA8_porter_duff_color;
            rasterizer->fragment = NULL;
            break;
          default:
            rasterizer->comp_op = ctx_GRAYA8_porter_duff_generic;
            break;
        }
        break;
    }
#endif
}
#endif

#endif
#if CTX_ENABLE_RGB332

inline static void
ctx_332_unpack (uint8_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue)
{
  *blue   = (pixel & 3) <<6;
  *green = ( (pixel >> 2) & 7) <<5;
  *red   = ( (pixel >> 5) & 7) <<5;
  if (*blue > 223)  { *blue  = 255; }
  if (*green > 223) { *green = 255; }
  if (*red > 223)   { *red   = 255; }
}

static inline uint8_t
ctx_332_pack (uint8_t red,
              uint8_t green,
              uint8_t blue)
{
  uint8_t c  = (red >> 5) << 5;
  c |= (green >> 5) << 2;
  c |= (blue >> 6);
  return c;
}

static inline void
ctx_RGB332_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      ctx_332_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2]);
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
        { rgba[3] = 0; }
      else
        { rgba[3] = 255; }
      pixel+=1;
      rgba +=4;
    }
}

static inline void
ctx_RGBA8_to_RGB332 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      if (rgba[3]==0)
        { pixel[0] = ctx_332_pack (255, 0, 255); }
      else
        { pixel[0] = ctx_332_pack (rgba[0], rgba[1], rgba[2]); }
      pixel+=1;
      rgba +=4;
    }
}

#endif
#if CTX_ENABLE_RGB565 | CTX_ENABLE_RGB565_BYTESWAPPED

static inline void
ctx_565_unpack (uint16_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue,
                int      byteswap)
{
  uint16_t byteswapped;
  if (byteswap)
    { byteswapped = (pixel>>8) | (pixel<<8); }
  else
    { byteswapped  = pixel; }
  *blue   = (byteswapped & 31) <<3;
  *green = ( (byteswapped>>5) & 63) <<2;
  *red   = ( (byteswapped>>11) & 31) <<3;
  if (*blue > 248) { *blue = 255; }
  if (*green > 248) { *green = 255; }
  if (*red > 248) { *red = 255; }
}

static inline uint16_t
ctx_565_pack (uint8_t red,
              uint8_t green,
              uint8_t blue,
              int     byteswap)
{
  uint32_t c = (red >> 3) << 11;
  c |= (green >> 2) << 5;
  c |= blue >> 3;
  if (byteswap)
    { return (c>>8) | (c<<8); } /* swap bytes */
  return c;
}

#endif
#if CTX_ENABLE_RGB565

static inline void
ctx_RGB565_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      ctx_565_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2], 0);
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
        { rgba[3] = 0; }
      else
        { rgba[3] = 255; }
      pixel+=1;
      rgba +=4;
    }
}

static inline void
ctx_RGBA8_to_RGB565 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      if (rgba[3]==0)
        { pixel[0] = ctx_565_pack (255, 0, 255, 0); }
      else
        { pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 0); }
      pixel+=1;
      rgba +=4;
    }
}

#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED

static inline void
ctx_RGB565_BS_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      ctx_565_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2], 1);
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
        { rgba[3] = 0; }
      else
        { rgba[3] = 255; }
      pixel+=1;
      rgba +=4;
    }
}

static inline void
ctx_RGBA8_to_RGB565_BS (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      if (rgba[3]==0)
        { pixel[0] = ctx_565_pack (255, 0, 255, 1); }
      else
        { pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 1); }
      pixel+=1;
      rgba +=4;
    }
}

#endif


static CtxPixelFormatInfo ctx_pixel_formats[]=
{
#if CTX_ENABLE_RGBA8
  {
    CTX_FORMAT_RGBA8, 4, 32, 4, 0, 0, CTX_FORMAT_RGBA8,
    NULL, NULL, NULL, ctx_setup_RGBA8
  },
#endif
#if CTX_ENABLE_BGRA8
  {
    CTX_FORMAT_BGRA8, 4, 32, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_BGRA8_to_RGBA8, ctx_RGBA8_to_BGRA8, ctx_composite_BGRA8, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_GRAYF
  {
    CTX_FORMAT_GRAYF, 1, 32, 4 * 2, 0, 0, CTX_FORMAT_GRAYAF,
    NULL, NULL, ctx_composite_GRAYF, ctx_setup_GRAYAF,
  },
#endif
#if CTX_ENABLE_GRAYAF
  {
    CTX_FORMAT_GRAYAF, 2, 64, 4 * 2, 0, 0, CTX_FORMAT_GRAYAF,
    NULL, NULL, NULL, ctx_setup_GRAYAF,
  },
#endif
#if CTX_ENABLE_RGBAF
  {
    CTX_FORMAT_RGBAF, 4, 128, 4 * 4, 0, 0, CTX_FORMAT_RGBAF,
    NULL, NULL, NULL, ctx_setup_RGBAF,
  },
#endif
#if CTX_ENABLE_RGB8
  {
    CTX_FORMAT_RGB8, 3, 24, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_RGB8_to_RGBA8, ctx_RGBA8_to_RGB8, ctx_composite_convert, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_GRAY1
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY1, 1, 1, 2, 1, 1, CTX_FORMAT_GRAYA8,
    ctx_GRAY1_to_GRAYA8, ctx_GRAYA8_to_GRAY1, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY1, 1, 1, 4, 1, 1, CTX_FORMAT_RGBA8,
    ctx_GRAY1_to_RGBA8, ctx_RGBA8_to_GRAY1, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAY2
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY2, 1, 2, 2, 4, 4, CTX_FORMAT_GRAYA8,
    ctx_GRAY2_to_GRAYA8, ctx_GRAYA8_to_GRAY2, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY2, 1, 2, 4, 4, 4, CTX_FORMAT_RGBA8,
    ctx_GRAY2_to_RGBA8, ctx_RGBA8_to_GRAY2, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAY4
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY4, 1, 4, 2, 16, 16, CTX_FORMAT_GRAYA8,
    ctx_GRAY4_to_GRAYA8, ctx_GRAYA8_to_GRAY4, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY4, 1, 4, 4, 16, 16, CTX_FORMAT_GRAYA8,
    ctx_GRAY4_to_RGBA8, ctx_RGBA8_to_GRAY4, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAY8
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY8, 1, 8, 2, 0, 0, CTX_FORMAT_GRAYA8,
    ctx_GRAY8_to_GRAYA8, ctx_GRAYA8_to_GRAY8, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY8, 1, 8, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_GRAY8_to_RGBA8, ctx_RGBA8_to_GRAY8, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAYA8
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAYA8, 2, 16, 2, 0, 0, CTX_FORMAT_GRAYA8,
    ctx_GRAYA8_to_RGBA8, ctx_RGBA8_to_GRAYA8, NULL, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAYA8, 2, 16, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_GRAYA8_to_RGBA8, ctx_RGBA8_to_GRAYA8, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_RGB332
  {
    CTX_FORMAT_RGB332, 3, 8, 4, 10, 12, CTX_FORMAT_RGBA8,
    ctx_RGB332_to_RGBA8, ctx_RGBA8_to_RGB332,
    ctx_composite_convert, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_RGB565
  {
    CTX_FORMAT_RGB565, 3, 16, 4, 32, 64, CTX_FORMAT_RGBA8,
    ctx_RGB565_to_RGBA8, ctx_RGBA8_to_RGB565,
    ctx_composite_convert, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED
  {
    CTX_FORMAT_RGB565_BYTESWAPPED, 3, 16, 4, 32, 64, CTX_FORMAT_RGBA8,
    ctx_RGB565_BS_to_RGBA8,
    ctx_RGBA8_to_RGB565_BS,
    ctx_composite_convert, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_CMYKAF
  {
    CTX_FORMAT_CMYKAF, 5, 160, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, NULL, ctx_setup_CMYKAF,
  },
#endif
#if CTX_ENABLE_CMYKA8
  {
    CTX_FORMAT_CMYKA8, 5, 40, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, ctx_composite_CMYKA8, ctx_setup_CMYKAF,
  },
#endif
#if CTX_ENABLE_CMYK8
  {
    CTX_FORMAT_CMYK8, 5, 32, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, ctx_composite_CMYK8, ctx_setup_CMYKAF,
  },
#endif
};


static CtxRasterizer *
ctx_rasterizer_init (CtxRasterizer *rasterizer, Ctx *ctx, Ctx *texture_source, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format, CtxAntialias antialias)
{
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
    ctx_buffer_free (rasterizer->clip_buffer);
#endif
  if (rasterizer->edge_list.size)
    ctx_renderstream_deinit (&rasterizer->edge_list);

  ctx_memset (rasterizer, 0, sizeof (CtxRasterizer) );
  rasterizer->vfuncs.process = ctx_rasterizer_process;
  rasterizer->vfuncs.free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  rasterizer->edge_list.flags |= CTX_RENDERSTREAM_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->ctx         = ctx;
  rasterizer->texture_source = texture_source?texture_source:ctx;
  rasterizer->aa          = _ctx_antialias_to_aa (antialias);
  rasterizer->force_aa    = 0;
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
  CtxRasterizer *r = ctx_rasterizer_init ( (CtxRasterizer *) calloc (sizeof (CtxRasterizer), 1),
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
ctx_pixel_format_info (CtxPixelFormat format)
{
  for (unsigned int i = 0; i < sizeof (ctx_pixel_formats) /sizeof (ctx_pixel_formats[0]); i++)
    {
      if (ctx_pixel_formats[i].pixel_format == format)
        {
          return &ctx_pixel_formats[i];
        }
    }
  return NULL;
}
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
      case CTX_TEXT_STROKE:
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
        ctx_renderstream_add_entry (&ctx->current_path, entry);
        break;
      default:
        break;
    }
#endif
  if (ctx->renderer && ctx->renderer->process)
    {
      ctx->renderer->process (ctx->renderer, (CtxCommand *) entry);
    }
  else
    {
      /* these functions might alter the code and coordinates of
         command that in the end gets added to the renderstream
       */
      ctx_interpret_style (&ctx->state, entry, ctx);
      ctx_interpret_transforms (&ctx->state, entry, ctx);
      ctx_interpret_pos (&ctx->state, entry, ctx);
      ctx_renderstream_add_entry (&ctx->renderstream, entry);
#if 1
      if (entry->code == CTX_TEXT ||
          entry->code == CTX_SET ||
          entry->code == CTX_TEXT_STROKE ||
          entry->code == CTX_FONT)
        {
          /* the image command and its data is submitted as one unit,
           */
          ctx_renderstream_add_entry (&ctx->renderstream, entry+1);
          ctx_renderstream_add_entry (&ctx->renderstream, entry+2);
        }
#endif
    }
}

/****  end of engine ****/
