#if CTX_IMPLEMENTATION || CTX_SIMD_BUILD
#if CTX_COMPOSITE 

#include "ctx-split.h"
#define CTX_AA_HALFSTEP2   (CTX_FULL_AA/2)
#define CTX_AA_HALFSTEP    ((CTX_FULL_AA/2)+1)

CTX_INLINE static int ctx_compare_edges (const void *ap, const void *bp)
{
  const CtxSegment *a = (const CtxSegment *) ap;
  const CtxSegment *b = (const CtxSegment *) bp;
  return a->data.s16[1] - b->data.s16[1];
}

CTX_INLINE static int ctx_edge_qsort_partition (CtxSegment *A, int low, int high)
{
  CtxSegment pivot = A[ (high+low) /2];
  int i = low;
  int j = high;
  while (i <= j)
    {
      while (ctx_compare_edges (&A[i], &pivot) < 0) { i ++; }
      while (ctx_compare_edges (&pivot, &A[j]) < 0) { j --; }
      if (i <= j)
        {
          CtxSegment tmp = A[i];
          A[i] = A[j];
          A[j] = tmp;
          i++;
          j--;
        }
    }
  return i;
}

static inline void ctx_edge_qsort (CtxSegment *entries, int low, int high)
{
  int p = ctx_edge_qsort_partition (entries, low, high);
  if (low < p -1 )
    { ctx_edge_qsort (entries, low, p - 1); }
  if (low < high)
    { ctx_edge_qsort (entries, p, high); }
}

static inline void ctx_rasterizer_sort_edges (CtxRasterizer *rasterizer)
{
  ctx_edge_qsort ((CtxSegment*)& (rasterizer->edge_list.entries[0]), 0, rasterizer->edge_list.count-1);
}

static inline void ctx_rasterizer_discard_edges (CtxRasterizer *rasterizer)
{
  int scanline = rasterizer->scanline;
  int next_scanline = scanline + CTX_FULL_AA;
  int limit3 = CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA;
  CtxSegment *segments = &((CtxSegment*)(rasterizer->edge_list.entries))[0];
  int *edges = rasterizer->edges;
  for (unsigned int i = 0; i < rasterizer->active_edges; i++)
    {
      CtxSegment *segment = segments + edges[i];
      int edge_end = segment->data.s16[3]-1;
      if (edge_end < scanline)
        {

          int dx_dy = abs(segment->delta);
          rasterizer->needs_aa3  -= (dx_dy > limit3);
          rasterizer->needs_aa5  -= (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT5);
          rasterizer->needs_aa15 -= (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT15);
          rasterizer->edges[i] = rasterizer->edges[rasterizer->active_edges-1];
          rasterizer->active_edges--;
          i--;
        }
      else if (edge_end < next_scanline)
        rasterizer->ending_edges++;
    }
#if 0
  // perhaps we should - but for 99% of the cases we do not need to, so we skip it
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      int edge_end = ((CtxSegment*)(rasterizer->edge_list.entries))[rasterizer->edges[CTX_MAX_EDGES-1-i]].data.s16[3]-1;
      if (edge_end < scanline + CTX_FULL_AA)
        rasterizer->ending_edges++;
    }
#endif
}

inline static void ctx_rasterizer_increment_edges (CtxRasterizer *rasterizer, int count)
{
  rasterizer->scanline += count;
  CtxSegment *segments = &((CtxSegment*)(rasterizer->edge_list.entries))[0];
  unsigned int active_edges = rasterizer->active_edges;
  unsigned int pending_edges = rasterizer->pending_edges;
  unsigned int pending_base = CTX_MAX_EDGES-pending_edges;
  for (unsigned int i = 0; i < active_edges; i++)
    {
      CtxSegment *segment = segments + rasterizer->edges[i];
      segment->val += segment->delta * count;
    }
  for (unsigned int i = 0; i < pending_edges; i++)
    {
      CtxSegment *segment = segments + rasterizer->edges[pending_base+i];
      segment->val += segment->delta * count;
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
inline static void ctx_edge2_insertion_sort (CtxSegment *segments, int *entries, unsigned int count)
{
  for(unsigned int i=1; i<count; i++)
   {
     int temp = entries[i];
     int j = i-1;
     while (j >= 0 && segments[temp].val - segments[entries[j]].val < 0)
     {
       entries[j+1] = entries[j];
       j--;
     }
     entries[j+1] = temp;
   }
}

inline static int ctx_edge2_compare2 (CtxSegment *segments, int a, int b)
{
  CtxSegment *seg_a = &segments[a];
  CtxSegment *seg_b = &segments[b];
  int minval_a = ctx_mini (seg_a->val - seg_a->delta * CTX_AA_HALFSTEP2, seg_a->val + seg_a->delta * CTX_AA_HALFSTEP);
  int minval_b = ctx_mini (seg_b->val - seg_b->delta * CTX_AA_HALFSTEP2, seg_b->val + seg_b->delta * CTX_AA_HALFSTEP);
  return minval_a - minval_b;
}

inline static void ctx_edge2_insertion_sort2 (CtxSegment *segments, int *entries, unsigned int count)
{
  for(unsigned int i=1; i<count; i++)
   {
     int temp = entries[i];
     int j = i-1;
     while (j >= 0 && ctx_edge2_compare2 (segments, temp, entries[j]) < 0)
     {
       entries[j+1] = entries[j];
       j--;
     }
     entries[j+1] = temp;
   }
}

inline static void ctx_rasterizer_feed_edges (CtxRasterizer *rasterizer, int apply2_sort)
{
  int miny;
  CtxSegment *entries = (CtxSegment*)&rasterizer->edge_list.entries[0];
  rasterizer->horizontal_edges = 0;
  rasterizer->ending_edges = 0;
  for (unsigned int i = 0; i < rasterizer->pending_edges; i++)
    {
      if (entries[rasterizer->edges[CTX_MAX_EDGES-1-i]].data.s16[1] - 1 <= rasterizer->scanline &&
          rasterizer->active_edges < CTX_MAX_EDGES-2)
        {
          unsigned int no = rasterizer->active_edges;
          rasterizer->active_edges++;
          rasterizer->edges[no] = rasterizer->edges[CTX_MAX_EDGES-1-i];
          rasterizer->edges[CTX_MAX_EDGES-1-i] =
            rasterizer->edges[CTX_MAX_EDGES-1-rasterizer->pending_edges + 1];
          rasterizer->pending_edges--;
          i--;
        }
    }
  int limit3 = CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA;
  int scanline = rasterizer->scanline;
  int next_scanline = scanline + CTX_FULL_AA;
  unsigned int edge_pos = rasterizer->edge_pos;
  unsigned int edge_count = rasterizer->edge_list.count;
  int *edges = rasterizer->edges;
  while ((edge_pos < edge_count &&
         (miny=entries[edge_pos].data.s16[1]-1)  <= next_scanline))
    {
      if (rasterizer->active_edges < CTX_MAX_EDGES-2 &&
      entries[edge_pos].data.s16[3]-1 /* (maxy) */  >= scanline)
        {
          int dy = (entries[edge_pos].data.s16[3] - 1 - miny);
          if (dy)
            {
              int yd = scanline - miny;
              unsigned int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              unsigned int index = edges[no] = edge_pos;
              int x0 = entries[index].data.s16[0];
              int x1 = entries[index].data.s16[2];
              int dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
              entries[index].delta = dx_dy;
              entries[index].val = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER +
                                         (yd * dx_dy);

              {
                int abs_dx_dy = abs(dx_dy);
                rasterizer->needs_aa3  += (abs_dx_dy > limit3);
                rasterizer->needs_aa5  += (abs_dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT5);
                rasterizer->needs_aa15 += (abs_dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT15);
              }

              if (miny > scanline &&
                  rasterizer->pending_edges < CTX_MAX_PENDING-1)
              {
                  /* it is a pending edge - we add it to the end of the array
                     and keep a different count for items stored here, like
                     a heap and stack growing against each other
                  */
                    edges[CTX_MAX_EDGES-1-rasterizer->pending_edges] =
                    rasterizer->edges[no];
                    rasterizer->pending_edges++;
                    rasterizer->active_edges--;
              }
            }
          else
            rasterizer->horizontal_edges ++;
        }
      edge_pos++;
    }
    rasterizer->edge_pos = edge_pos;
    ctx_rasterizer_discard_edges (rasterizer);
    if (apply2_sort)
      ctx_edge2_insertion_sort2 ((CtxSegment*)rasterizer->edge_list.entries, rasterizer->edges, rasterizer->active_edges);
    else
      ctx_edge2_insertion_sort ((CtxSegment*)rasterizer->edge_list.entries, rasterizer->edges, rasterizer->active_edges);
}
#undef CTX_CMPSWP

static inline void ctx_coverage_post_process (CtxRasterizer *rasterizer, unsigned int minx, unsigned int maxx, uint8_t *coverage, int *first_col, int *last_col)
{
#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
    float radius = rasterizer->state->gstate.shadow_blur;
    unsigned int dim = 2 * radius + 1;
    if (CTX_UNLIKELY (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM))
      dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
    {
      uint16_t temp[maxx-minx+1];
      memset (temp, 0, sizeof (temp));
      for (unsigned int x = dim/2; x < maxx-minx + 1 - dim/2; x ++)
        for (unsigned int u = 0; u < dim; u ++)
        {
          temp[x] += coverage[minx+x+u-dim/2] * rasterizer->kernel[u] * 256;
        }
      for (unsigned int x = 0; x < maxx-minx + 1; x ++)
        coverage[minx+x] = temp[x] >> 8;
    }
  }
#endif

#if CTX_ENABLE_CLIP
  if (CTX_UNLIKELY(rasterizer->clip_buffer &&  !rasterizer->clip_rectangle))
  {
  int scanline     = rasterizer->scanline - CTX_FULL_AA; // we do the
                                                 // post process after
                                                 // coverage generation icnrement
    /* perhaps not working right for clear? */
    int y = scanline / CTX_FULL_AA;//rasterizer->aa;
    uint8_t *clip_line = &((uint8_t*)(rasterizer->clip_buffer->data))[rasterizer->blit_width*y];
    // XXX SIMD candidate
    for (unsigned int x = minx; x <= maxx; x ++)
    {
#if CTX_1BIT_CLIP
       coverage[x] = (coverage[x] * ((clip_line[x/8]&(1<<(x&8)))?255:0))/255;
#else
       coverage[x] = (255 + coverage[x] * clip_line[x-rasterizer->blit_x])>>8;
#endif
    }
  }
#endif
}

#define CTX_EDGE(no)      entries[edges[no]]
#define CTX_EDGE_YMIN     (segment->data.s16[1]-1)

#define UPDATE_PARITY \
        if (CTX_LIKELY(scanline!=CTX_EDGE_YMIN))\
        { \
          if (is_winding)\
             parity = parity + -1+2*(segment->code == CTX_EDGE_FLIPPED);\
          else\
             parity = 1-parity; \
        }


inline static void
ctx_rasterizer_generate_coverage (CtxRasterizer *rasterizer,
                                  int            minx,
                                  int            maxx,
                                  uint8_t       *coverage,
                                  int            is_winding,
                                  const uint8_t  aa_factor,
                                  const uint8_t  fraction)
{
  CtxSegment *entries      = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int        *edges        = rasterizer->edges;
  int         scanline     = rasterizer->scanline;
  int         active_edges = rasterizer->active_edges;
  int         parity       = 0;
  coverage -= minx;
  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment *segment = &entries[edges[t]];
      UPDATE_PARITY;

      if (parity)
        {
          CtxSegment *next_segment = &entries[edges[t+1]];
          const int x0 = segment->val;
          const int x1 = next_segment->val;
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (CTX_UNLIKELY (first < minx))
          { 
            first = minx;
            graystart=0;
          }
          if (CTX_UNLIKELY (last > maxx))
          {
            last = maxx;
            grayend=255;
          }

          graystart = fraction- (graystart&0xff)/aa_factor;
          grayend   = (grayend & 0xff) / aa_factor;

          if (first < last)
          {
              coverage[first] += graystart;
              for (int x = first + 1; x < last; x++)
                coverage[x]  += fraction;
              coverage[last] += grayend;
          }
          else if (first == last)
            coverage[first] += (graystart-fraction+grayend);
        }
   }
}

inline static void
ctx_rasterizer_generate_coverage_set (CtxRasterizer *rasterizer,
                                      int            minx,
                                      int            maxx,
                                      uint8_t       *coverage,
                                      int            is_winding)
{
  CtxSegment *entries = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int      *edges = rasterizer->edges;
  int scanline     = rasterizer->scanline;
  int active_edges = rasterizer->active_edges;
  int parity = 0;
  coverage -= minx;
  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment *segment = &entries[edges[t]];
      UPDATE_PARITY;

      if (parity)
        {
          CtxSegment *next_segment = &entries[edges[t+1]];
          const int x0        = segment->val;
          const int x1        = next_segment->val;
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (CTX_UNLIKELY (first < minx))
          { 
            first = minx;
            graystart=0;
          }
          if (CTX_UNLIKELY (last > maxx))
          {
            last = maxx;
            grayend=255;
          }

          graystart = (graystart&0xff) ^ 255;
          grayend   = (grayend & 0xff);

          coverage[first] += graystart;
          coverage[last]  += grayend;
          if (first + 1< last)
              memset(&coverage[first+1], 255, last-(first+1));
        }
   }
}


inline static void
ctx_rasterizer_generate_coverage_apply (CtxRasterizer *rasterizer,
                                        int            minx,
                                        int            maxx,
                                        uint8_t* __restrict__ coverage,
                                        int            is_winding,
                                        CtxCovPath     comp)
{
  CtxSegment *entries = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int *edges          = rasterizer->edges;
  int scanline        = rasterizer->scanline;
  const int bpp       = rasterizer->format->bpp;
  int active_edges    = rasterizer->active_edges;
  int parity          = 0;
#if CTX_RASTERIZER_SWITCH_DISPATCH
  uint32_t *src_pixp;
  uint32_t src_pix, si_ga, si_rb, si_ga_full, si_rb_full, si_a;
  if (comp != CTX_COV_PATH_FALLBACK &&
      comp != CTX_COV_PATH_RGBA8_COPY_FRAGMENT &&
      comp != CTX_COV_PATH_RGBA8_OVER_FRAGMENT)
  {
    src_pixp   = ((uint32_t*)rasterizer->color);
    src_pix    = src_pixp[0];
    si_ga      = ((uint32_t*)rasterizer->color)[1];
    si_rb      = ((uint32_t*)rasterizer->color)[2];
    si_ga_full = ((uint32_t*)rasterizer->color)[3];
    si_rb_full = ((uint32_t*)rasterizer->color)[4];
    si_a       = src_pix >> 24;
  }
  else
  {
    src_pix    =
    si_ga      =
    si_rb      =
    si_ga_full =
    si_rb_full =
    si_a       = 0;
    src_pixp = &src_pix;
  }
#endif

  uint8_t *dst = ( (uint8_t *) rasterizer->buf) +
         (rasterizer->blit_stride * (scanline / CTX_FULL_AA));
  int accumulator_x=0;
  uint8_t accumulated = 0;
  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment *segment = &entries[edges[t]];
      UPDATE_PARITY;

       if (parity)
        {
          CtxSegment   *next_segment = &entries[edges[t+1]];
          const int x0        = segment->val;
          const int x1        = next_segment->val;

          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first = graystart >> 8;
          int grayend = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int last = grayend >> 8;

          if (CTX_UNLIKELY(first < minx))
          { 
            graystart = 0;
            first = minx;
          }
          if (CTX_UNLIKELY(last > maxx))
          {
             last = maxx;
             grayend=255;
          }
          graystart = (graystart&0xff) ^ 255;
          grayend = (grayend & 0xff);


          if (accumulated)
          {
            if (accumulator_x == first)
            {
              graystart += accumulated;
            }
            else
            {
              uint32_t* dst_pix = (uint32_t*)(&dst[(accumulator_x*bpp)/8]);
              switch (comp)
              {
#if CTX_RASTERIZER_SWITCH_DISPATCH
                case CTX_COV_PATH_RGBA8_COPY:
                  *dst_pix = ctx_lerp_RGBA8_2(*dst_pix, si_ga, si_rb, accumulated);
                  break;
                case CTX_COV_PATH_RGBA8_OVER:
                  *dst_pix = ctx_over_RGBA8_2(*dst_pix, si_ga, si_rb, si_a, accumulated);
                  break;
#endif
                default:
                  rasterizer->apply_coverage (rasterizer, (uint8_t*)dst_pix, rasterizer->color, accumulator_x, &accumulated, 1);
              }
            }
            accumulated = 0;
          }

          if (first < last)
          {
            switch (comp)
            {
#if CTX_RASTERIZER_SWITCH_DISPATCH
              case CTX_COV_PATH_RGBA8_COPY:
              {
                uint32_t* dst_pix = (uint32_t*)(&dst[(first *bpp)/8]);
                *dst_pix = ctx_lerp_RGBA8_2(*dst_pix, si_ga, si_rb, graystart);

                dst_pix++;
                ctx_span_set_colorb (dst_pix, src_pix, last - first - 1);
              }
              break;
            case CTX_COV_PATH_RGB8_COPY:
            case CTX_COV_PATH_RGBAF_COPY:
            case CTX_COV_PATH_RGB565_COPY:
            case CTX_COV_PATH_RGB332_COPY:
            case CTX_COV_PATH_GRAYA8_COPY:
            case CTX_COV_PATH_GRAYAF_COPY:
            case CTX_COV_PATH_CMYKAF_COPY:
            case CTX_COV_PATH_GRAY8_COPY:
            case CTX_COV_PATH_CMYKA8_COPY:
            case CTX_COV_PATH_CMYK8_COPY:
            {
              uint8_t* dsts = (uint8_t*)(&dst[(first *bpp)/8]);
              uint8_t  startcov = graystart;
              rasterizer->apply_coverage (rasterizer, (uint8_t*)dsts, rasterizer->color, first, &startcov, 1);
              uint8_t* dst_i = (uint8_t*)dsts;
              uint8_t *color = ((uint8_t*)&rasterizer->color_native);
              unsigned int bytes = rasterizer->format->bpp/8;
              dst_i+=bytes;

              unsigned int count = last-(first+1);//  (last - post) - (first+pre) + 1;

              //for (int i = first + pre; i <= last - post; i++)
              if (CTX_LIKELY(count>0))
              switch (bytes)
              {
                case 1:
#if 1
                  memset (dst_i, color[0], count);
#else
                  while (count--)
                  {
                    dst_i[0] = color[0];
                    dst_i++;
                  }
#endif
                  break;
                case 2:
                  {
                    uint16_t val = ((uint16_t*)color)[0];
                    while (count--)
                    {
                      ((uint16_t*)dst_i)[0] = val;
                      dst_i+=2;
                    }
                  }
                  break;
                case 4:
                  {
                    uint32_t val = ((uint32_t*)color)[0];
                    ctx_span_set_colorb ((uint32_t*)dst, val, count);
                  }
                  break;
                case 16:
                  ctx_span_set_color_x4 ((uint32_t*)dst, (uint32_t*)color, count);
                  break;
                case 3:
                 while (count--)
                 {
                   *dst_i ++ = color[0];
                   *dst_i ++ = color[1];
                   *dst_i ++ = color[2];
                 }
                 break;
                case 5:
                 while (count--)
                 {
                   *dst_i ++ = color[0];
                   *dst_i ++ = color[1];
                   *dst_i ++ = color[2];
                   *dst_i ++ = color[3];
                   *dst_i ++ = color[4];
                 }
                 break;
                default:
                 while (count--)
                 {
                   for (unsigned int b = 0; b < bytes; b++)
                     *dst_i++ = color[b];
                 }
                  break;
               }
            }
              break;

              case CTX_COV_PATH_GRAY1_COPY:
              {
                uint8_t* dstp = (uint8_t*)(&dst[(first *bpp)/8]);
                uint8_t *srcp = (uint8_t*)src_pixp;
                uint8_t  startcov = graystart;
                rasterizer->apply_coverage (rasterizer, (uint8_t*)dstp, rasterizer->color, first, &startcov, 1);
                dstp = (uint8_t*)(&dst[((first+1)*bpp)/8]);
                unsigned int count = last - first - 1;
                if (srcp[0]>=127)
                {
                  int x = first + 1;
                  for (unsigned int i = 0; i < count && x & 7; count--)
                  {
                     int bitno = x & 7;
                     *dstp |= (1<<bitno);
                     dstp += (bitno == 7);
                     x++;
                  }

                  for (unsigned int i = 0; i < count && count>8; count-=8)
                  {
                     *dstp = 255;
                     dstp++;
                     x+=8;
                  }

                  for (unsigned int i = 0; i < count; i++)
                  {
                     int bitno = x & 7;
                     *dstp |= (1<<bitno);
                     dstp += (bitno == 7);
                     x++;
                  }
                }
                else
                {
                  unsigned int x = first + 1;
                  for (unsigned int i = 0; i < count && x & 7; count--)
                  {
                     int bitno = x & 7;
                     *dstp &= ~(1<<bitno);
                     dstp += (bitno == 7);
                     x++;
                  }

                  for (unsigned int i = 0; i < count && count>8; count-=8)
                  {
                     *dstp = 0;
                     dstp++;
                     x+=8;
                  }

                  for (unsigned int i = 0; i < count; i++)
                  {
                     int bitno = x & 7;
                     *dstp &= ~(1<<bitno);
                     dstp += (bitno == 7);
                     x++;
                  }

                }
              }
              break;

            case CTX_COV_PATH_RGBA8_OVER:
            {
              uint32_t* dst_pix = (uint32_t*)(&dst[(first *bpp)/8]);
              *dst_pix = ctx_over_RGBA8_2(*dst_pix, si_ga, si_rb, si_a, graystart);
              dst_pix++;
              for (unsigned int i = first + 1; i < (unsigned)last; i++)
              {
                *dst_pix = ctx_over_RGBA8_full_2(*dst_pix, si_ga_full, si_rb_full, si_a);
                dst_pix++;
              }
            }
            break;
            case CTX_COV_PATH_RGBA8_COPY_FRAGMENT:
            {
              float u0 = 0; float v0 = 0;
              float ud = 0; float vd = 0;
              uint8_t gs = graystart;
              ctx_RGBA8_source_copy_normal_fragment (rasterizer, &dst[(first * bpp)/8], NULL, first, &gs, 1);
              ctx_init_uv (rasterizer, first+1, &u0, &v0, &ud, &vd);
              rasterizer->fragment (rasterizer, u0, v0, &dst[((first+1)*bpp)/8], last-first-1, ud, vd);
            }
            break;
              case CTX_COV_PATH_RGBA8_OVER_FRAGMENT:
            {
              uint8_t gs = graystart;
              ctx_RGBA8_source_over_normal_fragment (rasterizer, &dst[(first * bpp)/8], NULL, first, &gs, 1);
              ctx_RGBA8_source_over_normal_full_cov_fragment (rasterizer,
                                                     &dst[((first+1)*bpp)/8], NULL, first + 1, NULL, last-first-1, 1);
            }
            break;
#endif
              default:
            {
              uint8_t opaque[last-first];
              memset (opaque, 255, sizeof (opaque));
              opaque[0] = graystart;
              rasterizer->apply_coverage (rasterizer,
                                             &dst[(first * bpp)/8],
                                             rasterizer->color, first, opaque, last-first);
            }
            }
            accumulated = grayend;
          }
          else if (first == last)
          {
            accumulated = (graystart-(grayend^255));
          }
          accumulator_x = last;
        }
   }

   if (accumulated)
   {
     uint32_t* dst_pix = (uint32_t*)(&dst[(accumulator_x*bpp)/8]);
     switch (comp)
     {
#if CTX_RASTERIZER_SWITCH_DISPATCH
       case CTX_COV_PATH_RGBA8_COPY:
         *dst_pix = ctx_lerp_RGBA8_2(*dst_pix, si_ga, si_rb, accumulated);
         break;
       case CTX_COV_PATH_RGBA8_OVER:
         *dst_pix = ctx_over_RGBA8_2(*dst_pix, si_ga, si_rb, si_a, accumulated);
         break;
#endif
       default:
         rasterizer->apply_coverage (rasterizer, (uint8_t*)dst_pix, rasterizer->color, accumulator_x, &accumulated, 1);
     }
   }
}

inline static int ctx_rasterizer_is_simple (CtxRasterizer *rasterizer)
{
  if (rasterizer->fast_aa == 0 ||
      rasterizer->ending_edges ||
      rasterizer->pending_edges)
   return 0;
  int *edges  = rasterizer->edges;
  CtxSegment *segments = &((CtxSegment*)(rasterizer->edge_list.entries))[0];

  int active_edges = rasterizer->active_edges;
  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment *segment0 = segments + edges[t];
      CtxSegment *segment1 = segments + edges[t+1];
      const int delta0    = segment0->delta;
      const int delta1    = segment1->delta;
      const int x0        = segment0->val;
      const int x1        = segment1->val;
      int x0_end   = x0 + delta0 * CTX_AA_HALFSTEP;
      int x1_end   = x1 + delta1 * CTX_AA_HALFSTEP;
      int x0_start = x0 - delta0 * CTX_AA_HALFSTEP2;
      int x1_start = x1 - delta1 * CTX_AA_HALFSTEP2;
      if (x1_end < x0_end   ||
          x1_start < x0_end ||
          x1_end < x0_start
         )
         return 0;
    }
  return 1;
}


inline static void
ctx_rasterizer_generate_coverage_set2 (CtxRasterizer *rasterizer,
                                         int            minx,
                                         int            maxx,
                                         uint8_t       *coverage,
                                         int            is_winding)
{
  CtxSegment *entries = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int *edges  = rasterizer->edges;
  int scanline        = rasterizer->scanline;
  int active_edges    = rasterizer->active_edges;
  int parity        = 0;

  coverage -= minx;

  const int minx_ = minx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;
  const int maxx_ = maxx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;

  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment   *segment = &entries[edges[t]];
      UPDATE_PARITY;

       if (parity)
        {
          CtxSegment   *next_segment = &entries[edges[t+1]];
          const int x0        = segment->val;
          const int x1        = next_segment->val;
          const int delta0    = segment->delta;
          const int delta1    = next_segment->delta;

          int x0_start = x0 - delta0 * CTX_AA_HALFSTEP2;
          int x1_start = x1 - delta1 * CTX_AA_HALFSTEP2;
          int x0_end   = x0 + delta0 * CTX_AA_HALFSTEP;
          int x1_end   = x1 + delta1 * CTX_AA_HALFSTEP;

          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (CTX_UNLIKELY (first < minx))
          { 
            first = minx;
            graystart=0;
          }
          if (CTX_UNLIKELY (last > maxx))
          {
            last = maxx;
            grayend=255;
          }
          graystart = (graystart&0xff) ^ 255;
          grayend   = (grayend & 0xff);

          if (first < last)
          {
            int pre = 1;
            int post = 1;

            if (abs(delta0) < CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA)
            {
              coverage[first] += graystart;
            }
            else
            {
              unsigned int u0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x0_start, x0_end)));
              unsigned int u1 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x0_start, x0_end)));

              int us = u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);
              int count = 0;

              int mod = ((u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255) *
                         (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255);
              int sum = ((u1-u0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV)/255);

              int recip = 65536/sum;
              for (unsigned int u = u0; u < u1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
              {
                coverage[us + count] = ((u - u0 + mod) * recip)>>16;
                count++;
              }
              pre = (us+count-1)-first+1;
            }
  
            if (abs(delta1) < CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA)
            {
               coverage[last] += grayend;
            }
            else
            {
              unsigned int u0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x1_start, x1_end)));
              unsigned int u1 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x1_start, x1_end)));

              int us = u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);
              int count = 0;
              int mod = ((((u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255)+64) *
                    (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255));
              int sum = ((u1-u0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV * 1.25)/255);
              int recip = 65536 / sum;
              for (unsigned int u = u0; u < u1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
              {
                coverage[us + count] = (((u - u0 + mod) * recip)>>16) ^ 255;
                count++;
              }
              post = last-us+1;
            }
            for (int i = first + pre; i <= last - post; i++)
              coverage[i] = 255;
          }
          else if (first == last)
          {
            coverage[last]+=(graystart-(grayend^255));
          }
        }
   }
}


inline static void
ctx_rasterizer_generate_coverage_apply2 (CtxRasterizer *rasterizer,
                                         int            minx,
                                         int            maxx,
                                         uint8_t       *coverage,
                                         int            is_winding,
                                         CtxCovPath     comp)
{
  CtxSegment *entries = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int *edges          = rasterizer->edges;
  int  scanline       = rasterizer->scanline;
  const int  bpp      = rasterizer->format->bpp;
  int  active_edges   = rasterizer->active_edges;
  int  parity         = 0;

#if CTX_RASTERIZER_SWITCH_DISPATCH
  uint32_t *src_pixp;
  uint32_t src_pix, si_ga, si_rb, si_ga_full, si_rb_full, si_a;
  if (comp != CTX_COV_PATH_FALLBACK &&
      comp != CTX_COV_PATH_RGBA8_COPY_FRAGMENT &&
      comp != CTX_COV_PATH_RGBA8_OVER_FRAGMENT)
  {
    src_pixp   = ((uint32_t*)rasterizer->color);
    src_pix    = src_pixp[0];
    si_ga      = ((uint32_t*)rasterizer->color)[1];
    si_rb      = ((uint32_t*)rasterizer->color)[2];
    si_ga_full = ((uint32_t*)rasterizer->color)[3];
    si_rb_full = ((uint32_t*)rasterizer->color)[4];
    si_a  = src_pix >> 24;
  }
  else
  {
    src_pix    =
    si_ga      =
    si_rb      =
    si_ga_full =
    si_rb_full =
    si_a  = 0;
    src_pixp = &src_pix;
  }
#endif

  uint8_t *dst = ( (uint8_t *) rasterizer->buf) +
         (rasterizer->blit_stride * (scanline / CTX_FULL_AA));

  coverage -= minx;

  const int minx_ = minx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;
  const int maxx_ = maxx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;

  int accumulated_x0 = 65538;
  int accumulated_x1 = 65536;

  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment   *segment = &entries[edges[t]];
      UPDATE_PARITY;

       if (parity)
        {
          CtxSegment   *next_segment = &entries[edges[t+1]];
          const int x0        = segment->val;
          const int x1        = next_segment->val;
          const int delta0    = segment->delta;
          const int delta1    = next_segment->delta;

          int x0_start = x0 - delta0 * CTX_AA_HALFSTEP2;
          int x1_start = x1 - delta1 * CTX_AA_HALFSTEP2;
          int x0_end   = x0 + delta0 * CTX_AA_HALFSTEP;
          int x1_end   = x1 + delta1 * CTX_AA_HALFSTEP;

          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (CTX_UNLIKELY (first < minx))
          { 
            first = minx;
            graystart=0;
          }
          if (CTX_UNLIKELY (last > maxx))
          {
            last = maxx;
            grayend=255;
          }
          graystart = (graystart&0xff) ^ 255;
          grayend   = (grayend & 0xff);

          if (first < last)
          {
            int pre = 1;
            int post = 1;

          if (abs(delta0) < CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA)
          {
             coverage[first] += graystart;

            accumulated_x1 = first;
            accumulated_x0 = ctx_mini (accumulated_x0, first);
          }
          else
          {
            unsigned int u0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x0_start, x0_end)));
            unsigned int u1 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x0_start, x0_end)));

            int mod = ((u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255) *
                    (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255);
            int sum = ((u1-u0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV)/255);

            int us = u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);
            int count = 0;
            int recip = 65536/ sum;
            for (unsigned int u = u0; u < u1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
            {
              coverage[us + count] = ((u - u0 + mod) * recip)>>16;
              count++;
            }
            pre = (us+count-1)-first+1;

            accumulated_x0 = ctx_mini (accumulated_x0, us);
            accumulated_x1 = us + count - 1;
          }

          if (accumulated_x1-accumulated_x0>=0)
          {
             switch (comp)
             {
#if CTX_RASTERIZER_SWITCH_DISPATCH
                case CTX_COV_PATH_RGBA8_OVER:
                {
                  uint32_t *dst_i = (uint32_t*)&dst[((accumulated_x0) * bpp)/8];
                  for (int i = 0; i < accumulated_x1-accumulated_x0+1; i++)
                    {
                      *dst_i = ctx_over_RGBA8_2 (*dst_i, si_ga, si_rb, si_a, coverage[accumulated_x0+i]);
                      dst_i++;
                    }
                }
                break;

                case CTX_COV_PATH_RGBA8_COPY:
                {
                  uint32_t *dst_i = (uint32_t*)&dst[((accumulated_x0) * bpp)/8];
                  for (int i = 0; i < accumulated_x1-accumulated_x0+1; i++)
                  {
                    *dst_i = ctx_lerp_RGBA8_2 (*dst_i, si_ga, si_rb, coverage[accumulated_x0+i]);
                    dst_i++;
                  }
                }
                  break;
                case CTX_COV_PATH_RGB8_COPY:
                {
                  uint8_t *dst_i = (uint8_t*)&dst[((accumulated_x0) * bpp)/8];
                  uint8_t *srcp = (uint8_t*)src_pixp;
                  for (int i = 0; i < accumulated_x1-accumulated_x0+1; i++)
                  {
                    for (int c = 0; c < 3; c++)
                      dst_i[c] = ctx_lerp_u8 (dst_i[c], srcp[c], coverage[accumulated_x0+i]);
                    dst_i +=3;
                  }
                }
                  break;
#endif
                default:
                rasterizer->apply_coverage (rasterizer,
                          &dst[((accumulated_x0) * bpp)/8],
                          rasterizer->color,
                          accumulated_x0,
                          &coverage[accumulated_x0],
                          accumulated_x1-accumulated_x0+1);
             }
             accumulated_x0 = 65538;
             accumulated_x1 = 65536;
          }

          if (abs(delta1) < CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA)
          {
             coverage[last] += grayend;
             accumulated_x1 = last;
             accumulated_x0 = last;
          }
          else
          {
            unsigned int u0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x1_start, x1_end)));
            unsigned int u1 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x1_start, x1_end)));

            int us = u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);
            int count = 0;

            int mod = ((((u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255) +64) *
                    (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255));
            int sum = ((u1-u0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV * 1.25)/255);

            int recip = 65536/ sum;
            for (unsigned int u = u0; u < u1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
            {
              coverage[us + count] = (((u - u0 + mod)*recip)>>16)^255;
              count++;
            }
            post = last-us+1;

            accumulated_x1 = us + count;
            accumulated_x0 = us;
          }
          switch (comp)
          {
#if CTX_RASTERIZER_SWITCH_DISPATCH
            case CTX_COV_PATH_RGBAF_COPY:
            case CTX_COV_PATH_GRAY8_COPY:
            case CTX_COV_PATH_RGB8_COPY:
            case CTX_COV_PATH_GRAYA8_COPY:
            case CTX_COV_PATH_GRAYAF_COPY:
            case CTX_COV_PATH_CMYKAF_COPY:
            case CTX_COV_PATH_RGB565_COPY:
            case CTX_COV_PATH_RGB332_COPY:
            case CTX_COV_PATH_CMYK8_COPY:
            case CTX_COV_PATH_CMYKA8_COPY:
            {
              uint8_t* dsts = (uint8_t*)(&dst[(first *bpp)/8]);
              uint8_t* dst_i = (uint8_t*)dsts;
              uint8_t* color = ((uint8_t*)&rasterizer->color_native);
              unsigned int bytes = rasterizer->format->bpp/8;
              dst_i+=pre*bytes;

              int scount = (last - post) - (first+pre) + 1;
              unsigned int count = scount;

              //for (int i = first + pre; i <= last - post; i++)
              if (CTX_LIKELY(scount>0))
              switch (bytes)
              {
                case 1:
#if 1
                  memset (dst_i, color[0], count);
#else
                  while (count--)
                  {
                    dst_i[0] = color[0];
                    dst_i++;
                  }
#endif
                  break;
                case 2:
                  {
                    uint16_t val = ((uint16_t*)color)[0];
                    while (count--)
                    {
                      ((uint16_t*)dst_i)[0] = val;
                      dst_i+=2;
                    }
                  }
                  break;
                case 4:
                  {
                    uint32_t val = ((uint32_t*)color)[0];
                    while (count--)
                    {
                      ((uint32_t*)dst_i)[0] = val;
                      dst_i+=4;
                    }
                  }
                  break;
                case 16:
                  ctx_span_set_color_x4 ((uint32_t*)dst, (uint32_t*)color, count);
                  break;
                case 3:
                 while (count--)
                 {
                   *dst_i++ = color[0];
                   *dst_i++ = color[1];
                   *dst_i++ = color[2];
                 }
                 break;
                case 5:
                 while (count--)
                 {
                   *dst_i++ = color[0];
                   *dst_i++ = color[1];
                   *dst_i++ = color[2];
                   *dst_i++ = color[3];
                   *dst_i++ = color[4];
                 }
                 break;
                default:
                 while (count--)
                 {
                   for (unsigned int b = 0; b < bytes; b++)
                     *dst_i++ = color[b];
                 }
                  break;
               }
             }
             break;

            case CTX_COV_PATH_RGBA8_COPY:
            {
              uint32_t* dst_pix = (uint32_t*)(&dst[(first *bpp)/8]);
              dst_pix+=pre;
              ctx_span_set_color (dst_pix, src_pix, last-first-pre-post + 1);
            }
            break;


            case CTX_COV_PATH_RGBA8_OVER:
            {
              uint32_t* dst_pix = (uint32_t*)(&dst[(first *bpp)/8]);
              dst_pix+=pre;
              int scount = (last - post) - (first + pre) + 1;
              if (scount > 0)
              {
                unsigned int count = scount;
                while (count--)
                {
                  *dst_pix = ctx_over_RGBA8_full_2(*dst_pix, si_ga_full, si_rb_full, si_a);
                  dst_pix++;
                }
              }
            }
            break;
            case CTX_COV_PATH_RGBA8_COPY_FRAGMENT:
            {
              int width = last-first-pre-post+1;
              if (width>0)
              {
                float u0 = 0; float v0 = 0;
                float ud = 0; float vd = 0;
                ctx_init_uv (rasterizer, first+pre, &u0, &v0, &ud, &vd);
                rasterizer->fragment (rasterizer, u0, v0, &dst[(first+pre)*bpp/8],
                                      width, ud, vd);
              }
            }
            break;
            case CTX_COV_PATH_RGBA8_OVER_FRAGMENT:
              {
                int width = last-first-pre-post+1;
                if (width>0)
                ctx_RGBA8_source_over_normal_full_cov_fragment (rasterizer,
                               &dst[((first+pre)*bpp)/8],
                               NULL,
                               first + pre,
                               NULL,
                               width, 1);
              }
            break;
#endif
            default:
              {
                int width = last-first-pre-post+1;
                if (width > 0)
                {
                uint8_t opaque[width];
                memset (opaque, 255, sizeof (opaque));
                rasterizer->apply_coverage (rasterizer,
                            &dst[((first + pre) * bpp)/8],
                            rasterizer->color,
                            first + pre,
                            opaque,
                            width);
                }
              }
          }
          }
          else if (first == last)
          {
            coverage[last]+=(graystart-(grayend^255));

            accumulated_x1 = last;
            accumulated_x0 = ctx_mini (accumulated_x0, last);
          }
        }
   }

   if (accumulated_x1-accumulated_x0>=0)
   {
             switch (comp)
             {
#if CTX_RASTERIZER_SWITCH_DISPATCH
                case CTX_COV_PATH_RGBA8_OVER:
                {
                  uint32_t *dst_i = (uint32_t*)&dst[((accumulated_x0) * bpp)/8];
                  for (int i = 0; i < accumulated_x1-accumulated_x0+1; i++)
                    {
                      *dst_i = ctx_over_RGBA8_2 (*dst_i, si_ga, si_rb, si_a, coverage[accumulated_x0+i]);
                      dst_i++;
                    }
                }
                break;
                case CTX_COV_PATH_RGBA8_COPY:
                {
                  uint32_t *dst_i = (uint32_t*)&dst[((accumulated_x0) * bpp)/8];
                  for (int i = 0; i < accumulated_x1-accumulated_x0+1; i++)
                  {
                    *dst_i = ctx_lerp_RGBA8_2 (*dst_i, si_ga, si_rb, coverage[accumulated_x0+i]);
                    dst_i++;
                  }
                }
                  break;
#endif
                default:
                rasterizer->apply_coverage (rasterizer,
                          &dst[((accumulated_x0) * bpp)/8],
                          rasterizer->color,
                          accumulated_x0,
                          &coverage[accumulated_x0],
                          accumulated_x1-accumulated_x0+1);
             }
   }
}

#undef CTX_EDGE_Y0
#undef CTX_EDGE

static inline void
ctx_rasterizer_reset (CtxRasterizer *rasterizer)
{
  rasterizer->has_shape       =   
  rasterizer->has_prev        =   
  rasterizer->edge_list.count =    // ready for new edges
  rasterizer->edge_pos        =   
  rasterizer->scanline        = 0;
  if (CTX_LIKELY(!rasterizer->preserve))
  {
    rasterizer->scan_min      =
    rasterizer->col_min       = 5000;
    rasterizer->scan_max      =
    rasterizer->col_max       = -5000;
  }
  //rasterizer->comp_op       = NULL; // keep comp_op cached 
  //     between rasterizations where rendering attributes are
  //     nonchanging
}

static void
ctx_rasterizer_rasterize_edges2 (CtxRasterizer *rasterizer, const int fill_rule 
#if CTX_SHAPE_CACHE
                                ,CtxShapeEntry *shape
#endif
                               )
{
  rasterizer->pending_edges   =   
  rasterizer->active_edges    =   0;
  //rasterizer->scanline        = 0;
  int       is_winding  = fill_rule == CTX_FILL_RULE_WINDING;
  const CtxCovPath comp = rasterizer->comp;
  const int real_aa     = rasterizer->aa;
  uint8_t  *dst         = ((uint8_t *) rasterizer->buf);
  int       scan_start  = rasterizer->blit_y * CTX_FULL_AA;
  int       scan_end    = scan_start + (rasterizer->blit_height - 1) * CTX_FULL_AA;
  const int blit_width  = rasterizer->blit_width;
  const int blit_max_x  = rasterizer->blit_x + blit_width;
  int       minx        = rasterizer->col_min / CTX_SUBDIV - rasterizer->blit_x;
  int       maxx        = (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV -
                          rasterizer->blit_x;
  const int blit_stride = rasterizer->blit_stride;
  uint8_t   real_fraction = 255/real_aa;

  rasterizer->prev_active_edges = -1;
  if (CTX_UNLIKELY (
#if CTX_SHAPE_CACHE
    !shape &&
#endif
    maxx > blit_max_x - 1))
    { maxx = blit_max_x - 1; }

  minx = ctx_maxi (rasterizer->state->gstate.clip_min_x, minx);
  maxx = ctx_mini (rasterizer->state->gstate.clip_max_x, maxx);
  minx = ctx_maxi (0, minx); // redundant?
  if (CTX_UNLIKELY (minx >= maxx))
    {
      return;
    }
#if CTX_SHAPE_CACHE
  uint8_t _coverage[shape?2:maxx-minx+1];
#else
  uint8_t _coverage[maxx-minx+1];
#endif
  uint8_t *coverage = &_coverage[0];

  int coverage_size;

  rasterizer->scan_min -= (rasterizer->scan_min % CTX_FULL_AA);
#if CTX_SHAPE_CACHE
  if (shape)
    {
      coverage_size = shape->width;
      coverage = &shape->data[0];
      scan_start = rasterizer->scan_min;
      scan_end   = rasterizer->scan_max;
    }
  else
#endif
  {
     coverage_size = sizeof (_coverage);
     if (rasterizer->scan_min > scan_start)
       {
          dst += (rasterizer->blit_stride * (rasterizer->scan_min-scan_start) / CTX_FULL_AA);
          scan_start = rasterizer->scan_min;
       }
      scan_end = ctx_mini (rasterizer->scan_max, scan_end);
  }

  if (CTX_UNLIKELY(rasterizer->state->gstate.clip_min_y * CTX_FULL_AA > scan_start ))
    { 
       dst += (rasterizer->blit_stride * (rasterizer->state->gstate.clip_min_y * CTX_FULL_AA -scan_start) / CTX_FULL_AA);
       scan_start = rasterizer->state->gstate.clip_min_y * CTX_FULL_AA; 
    }
  scan_end = ctx_mini (rasterizer->state->gstate.clip_max_y * CTX_FULL_AA, scan_end);
  if (CTX_UNLIKELY(scan_start > scan_end ||
      (scan_start > (rasterizer->blit_y + (rasterizer->blit_height-1)) * CTX_FULL_AA) ||
      (scan_end < (rasterizer->blit_y) * CTX_FULL_AA)))
  { 
    /* not affecting this rasterizers scanlines */
    return;
  }

  rasterizer->horizontal_edges =
    rasterizer->needs_aa3  =
    rasterizer->needs_aa5  =
    rasterizer->needs_aa15 = 0;

  ctx_rasterizer_sort_edges (rasterizer);
  rasterizer->scanline = scan_start;
  ctx_rasterizer_feed_edges (rasterizer, 0); 

  int avoid_direct = (0 
#if CTX_ENABLE_CLIP
         || rasterizer->clip_buffer
#endif
#if CTX_ENABLE_SHADOW_BLUR
         || rasterizer->in_shadow
#endif
#if CTX_SHAPE_CACHE
         || shape != NULL
#endif
         );

  for (; rasterizer->scanline <= scan_end;)
    {

    if (rasterizer->active_edges == 0 && rasterizer->pending_edges == 0)
    { /* no edges */
      ctx_rasterizer_feed_edges (rasterizer, 0);
      ctx_rasterizer_increment_edges (rasterizer, CTX_FULL_AA);
      dst += blit_stride;
#if CTX_SHAPE_CACHE
      if (shape)
      {
        memset (coverage, 0, coverage_size);
        coverage += shape->width;
      }
#endif
      rasterizer->prev_active_edges = rasterizer->active_edges;
      continue;
    }
    else if (real_aa != 1 && ( (rasterizer->horizontal_edges!=0) 
          || (rasterizer->active_edges != rasterizer->prev_active_edges)
          || (rasterizer->active_edges + rasterizer->pending_edges == rasterizer->ending_edges)
          ))
    { /* needs full AA */
        int increment = CTX_FULL_AA/real_aa;
        memset (coverage, 0, coverage_size);
        for (int i = 0; i < real_aa; i++)
        {
          ctx_rasterizer_feed_edges (rasterizer, 0);
          ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, is_winding, real_aa, real_fraction);
          ctx_rasterizer_increment_edges (rasterizer, increment);
        }
    }
    else if (rasterizer->needs_aa3 == 0)
    {
      if (! avoid_direct)
      { /* can generate with direct rendering to target (we're not using shape cache) */
        ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP2);
        ctx_rasterizer_feed_edges (rasterizer, 0);

        ctx_rasterizer_generate_coverage_apply (rasterizer, minx, maxx, coverage, is_winding, comp);
        ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);

        dst += blit_stride;
        rasterizer->prev_active_edges = rasterizer->active_edges;
        continue;
      }
      else
      { /* cheap fully correct AA, to coverage mask / clipping */
        ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP2);
        ctx_rasterizer_feed_edges (rasterizer, 0);

        memset (coverage, 0, coverage_size);
        ctx_rasterizer_generate_coverage_set (rasterizer, minx, maxx, coverage, is_winding);
        ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);
      }
    }
    else if (ctx_rasterizer_is_simple (rasterizer))
    { /* the scanline transitions does not contain multiple intersections - each aa segment is a linear ramp */
      ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP2);
      ctx_rasterizer_feed_edges (rasterizer, 1);
      memset (coverage, 0, coverage_size);
      if (!avoid_direct)
      {
        ctx_rasterizer_generate_coverage_apply2 (rasterizer, minx, maxx, coverage, is_winding,
                      comp);
        ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);

        dst += blit_stride;
        rasterizer->prev_active_edges = rasterizer->active_edges;
        continue;
      }
      ctx_rasterizer_generate_coverage_set2 (rasterizer, minx, maxx, coverage, is_winding);
      ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);
      if (real_aa == 1)
      {
        for (int x = minx; x <= maxx; x ++)
          coverage[x] = coverage[x] > 127?255:0;
      }
    }
    else
    { /* determine level of oversampling based on lowest steepness edges */
      int aa = 3;
      if (rasterizer->needs_aa5 && real_aa >=5)
      {
         aa = 5;
         if (rasterizer->needs_aa15 && real_aa >=15)
           aa = 15;
      }
      int scanline_increment = 15/aa;

      memset (coverage, 0, coverage_size);
      uint8_t fraction = 255/aa;
      for (int i = 0; i < CTX_FULL_AA; i+= scanline_increment)
      {
        ctx_rasterizer_feed_edges (rasterizer, 0);
        ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, is_winding, aa, fraction);
        ctx_rasterizer_increment_edges (rasterizer, scanline_increment);
      }
    }

  ctx_coverage_post_process (rasterizer, minx, maxx, coverage - minx, NULL, NULL);
#if CTX_SHAPE_CACHE
  if (shape == NULL)
#endif
  {
    rasterizer->apply_coverage (rasterizer,
                         &dst[(minx * rasterizer->format->bpp) /8],
                         rasterizer->color,
                         minx,
                         coverage,
                         maxx-minx+ 1);
  }
#if CTX_SHAPE_CACHE
  else
  {
    coverage += shape->width;
  }
#endif
      dst += blit_stride;
      rasterizer->prev_active_edges = rasterizer->active_edges;
    }

  if (CTX_UNLIKELY(rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_SOURCE_OUT ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_SOURCE_IN ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_DESTINATION_IN ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_DESTINATION_ATOP ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_CLEAR))
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
       rasterizer->apply_coverage (rasterizer,
                                   &dst[ (startx * rasterizer->format->bpp) /8],
                                   rasterizer->color,
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
       rasterizer->apply_coverage (rasterizer,
                                   &dst[ (startx * rasterizer->format->bpp) /8],
                                   rasterizer->color,
                                   0,
                                   nocoverage, minx-startx);
       dst += blit_stride;
     }
     }

     if (endx > maxx)
     {
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_start / CTX_FULL_AA);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       rasterizer->apply_coverage (rasterizer,
                                   &dst[ (maxx * rasterizer->format->bpp) /8],
                                   rasterizer->color,
                                   0,
                                   nocoverage, endx-maxx);

       rasterizer->scanline += CTX_FULL_AA;
       dst += rasterizer->blit_stride;
     }
     }
#if 1
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_end / CTX_FULL_AA);
     // XXX this crashes under valgrind/asan
     if(0)for (rasterizer->scanline = scan_end; rasterizer->scanline/CTX_FULL_AA < gscan_end-1;)
     {
       rasterizer->apply_coverage (rasterizer,
                                   &dst[ (startx * rasterizer->format->bpp) /8],
                                   rasterizer->color,
                                   0,
                                   nocoverage, clipw-1);

       rasterizer->scanline += CTX_FULL_AA;
       dst += blit_stride;
     }
#endif
  }
}


#if CTX_INLINE_FILL_RULE

void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule 
#if CTX_SHAPE_CACHE
                                ,CtxShapeEntry *shape
#endif
                               )
{
  if (fill_rule)
  {
    ctx_rasterizer_rasterize_edges2 (rasterizer, 1
#if CTX_SHAPE_CACHE
                    ,shape
#endif
                    );
  }
  else
  {
    ctx_rasterizer_rasterize_edges2 (rasterizer, 0
#if CTX_SHAPE_CACHE
                    ,shape
#endif
                    );
  }
}
#else

void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule 
#if CTX_SHAPE_CACHE
                                ,CtxShapeEntry *shape
#endif
                               )
{
    ctx_rasterizer_rasterize_edges2 (rasterizer, fill_rule
#if CTX_SHAPE_CACHE
                    ,shape
#endif
                    );
}

#endif



extern CtxPixelFormatInfo *ctx_pixel_formats;
void CTX_SIMD_SUFFIX(ctx_simd_setup)(void)
{
  ctx_pixel_formats         = CTX_SIMD_SUFFIX(ctx_pixel_formats);
  ctx_composite_setup       = CTX_SIMD_SUFFIX(ctx_composite_setup);
  ctx_rasterizer_rasterize_edges = CTX_SIMD_SUFFIX(ctx_rasterizer_rasterize_edges);
#if CTX_FAST_FILL_RECT
  ctx_composite_fill_rect   = CTX_SIMD_SUFFIX(ctx_composite_fill_rect);
  ctx_composite_stroke_rect = CTX_SIMD_SUFFIX(ctx_composite_stroke_rect);
#endif
}


#endif
#endif
#if CTX_IMPLEMENTATION
#if CTX_RASTERIZER


inline static float ctx_fast_hypotf (float x, float y)
{
  if (x < 0) { x = -x; }
  if (y < 0) { y = -y; }
  if (x < y)
    { return 0.96f * y + 0.4f * x; }
  else
    { return 0.96f * x + 0.4f * y; }
}



static void
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

static inline void ctx_rasterizer_update_inner_point (CtxRasterizer *rasterizer, int x, int y)
{
  rasterizer->scan_min = ctx_mini (y, rasterizer->scan_min);
  rasterizer->scan_max = ctx_maxi (y, rasterizer->scan_max);
  rasterizer->col_min = ctx_mini (x, rasterizer->col_min);
  rasterizer->col_max = ctx_maxi (x, rasterizer->col_max);
  rasterizer->inner_x = x;
  rasterizer->inner_y = y;
}

static inline int ctx_rasterizer_add_point (CtxRasterizer *rasterizer, int x1, int y1)
{
  CtxSegment entry = {CTX_EDGE, {{0},}};

  entry.data.s16[0]=rasterizer->inner_x;
  entry.data.s16[1]=rasterizer->inner_y;

  entry.data.s16[2]=x1;
  entry.data.s16[3]=y1;

  ctx_rasterizer_update_inner_point (rasterizer, x1, y1);

  return ctx_edgelist_add_single (&rasterizer->edge_list, (CtxEntry*)&entry);
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

static long ctx_shape_cache_hits   = 0;
static long ctx_shape_cache_misses = 0;


/* this returns the buffer to use for rendering, it always
   succeeds..
 */
static inline CtxShapeEntry *ctx_shape_entry_find (CtxRasterizer *rasterizer, uint32_t hash, int width, int height)
{
  /* use both some high and some low bits  */
  int entry_no = ( (hash >> 10) ^ (hash & 1023) ) % CTX_SHAPE_CACHE_ENTRIES;
  {
    static int i = 0;
    i++;
    if (i>256)
      {
        if (ctx_shape_cache_hits+ctx_shape_cache_misses)
        {
          ctx_shape_cache_rate = 
                0.5 * ctx_shape_cache_rate +
                0.5 * (ctx_shape_cache_hits * 100.0  / (ctx_shape_cache_hits+ctx_shape_cache_misses));
        }
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

  CtxShapeEntry *entry = rasterizer->shape_cache.entries[entry_no];
  if (entry)
    {
      int old_size = sizeof (CtxShapeEntry) + entry->width + entry->height + 1;
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
         rasterizer->shape_cache.size -= old_size;
         rasterizer->shape_cache.size += (old_size-size); // slack/leaked
      }
      else
      {
        rasterizer->shape_cache.entries[entry_no] = NULL;
        rasterizer->shape_cache.size -= entry->width * entry->height;
        rasterizer->shape_cache.size -= sizeof (CtxShapeEntry);
        free (entry);
        entry = NULL;
      }
    }

  if (!entry)
    entry = rasterizer->shape_cache.entries[entry_no] = (CtxShapeEntry *) calloc (size, 1);

  rasterizer->shape_cache.size += size;

  ctx_shape_cache_misses ++;
  entry->hash   = hash;
  entry->width  = width;
  entry->height = height;
  entry->uses = 0;
  return entry;
}

#endif

static uint32_t ctx_rasterizer_poly_to_hash (CtxRasterizer *rasterizer)
{
  int x = 0;
  int y = 0;

  CtxSegment *entry = (CtxSegment*)&rasterizer->edge_list.entries[0];

  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = ox;//(ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += oy; //(oy % CTX_RASTERIZER_AA);
  for (unsigned int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxSegment *entry = &(((CtxSegment*)(rasterizer->edge_list.entries)))[i];
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
#if CTX_SHAPE_CACHE
  int x = 0;
  int y = 0;
#endif
  unsigned int count = rasterizer->edge_list.count;
  if (CTX_UNLIKELY (count == 0))
     return 0;
  CtxSegment *entry = (CtxSegment*)&rasterizer->edge_list.entries[0];
#if CTX_SHAPE_CACHE
#if 1
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
#endif
  uint32_t hash = rasterizer->edge_list.count;
  hash = (ox & CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += (oy & CTX_SUBDIV);
#endif
  //CtxSegment *entry = &(((CtxSegment*)(rasterizer->edge_list.entries)))[0];
  for (unsigned int i = 0; i < count; i++)
    {
#if CTX_SHAPE_CACHE
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
#endif
#if 1
      if (entry->data.s16[3] < entry->data.s16[1])
        {
          *entry = ctx_segment_s16 (CTX_EDGE_FLIPPED,
                            entry->data.s16[2], entry->data.s16[3],
                            entry->data.s16[0], entry->data.s16[1]);
        }
#endif
      entry++;
    }
#if CTX_SHAPE_CACHE
  return hash;
#else
  return 0;
#endif
}

static inline void ctx_rasterizer_finish_shape (CtxRasterizer *rasterizer)
{
  if (rasterizer->has_shape && rasterizer->has_prev)
    {
      ctx_rasterizer_line_to (rasterizer, rasterizer->first_x, rasterizer->first_y);
      rasterizer->has_prev = 0;
    }
}

static inline void ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx = x; float ty = y;
  rasterizer->x        = x;
  rasterizer->y        = y;
  rasterizer->first_x  = x;
  rasterizer->first_y  = y;
  rasterizer->has_prev = -1;
  if (rasterizer->uses_transforms)
    {
      _ctx_user_to_device (rasterizer->state, &tx, &ty);
    }

  tx = (tx - rasterizer->blit_x) * CTX_SUBDIV;
  ty = ty * CTX_FULL_AA;

  ctx_rasterizer_update_inner_point (rasterizer, tx, ty);
}

static inline void
ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  rasterizer->has_shape = 1;
  rasterizer->y         = y;
  rasterizer->x         = x;

  float tx = x;
  float ty = y;
  //float ox = rasterizer->x;
  //float oy = rasterizer->y;
  if (rasterizer->uses_transforms)
    {
      _ctx_user_to_device (rasterizer->state, &tx, &ty);
    }
  tx -= rasterizer->blit_x;
#define MIN_Y -1000
#define MAX_Y 1400

  ty = ctx_maxf (MIN_Y, ty);
  ty = ctx_minf (MAX_Y, ty);
  
  ctx_rasterizer_add_point (rasterizer, tx * CTX_SUBDIV, ty * CTX_FULL_AA);//rasterizer->aa);

  if (CTX_UNLIKELY(rasterizer->has_prev<=0))
    {
      CtxSegment *entry = & ((CtxSegment*)rasterizer->edge_list.entries)[rasterizer->edge_list.count-1];
      entry->code = CTX_NEW_EDGE;
      rasterizer->has_prev = 1;
    }
}


CTX_INLINE static float
ctx_bezier_sample_1d (float x0, float x1, float x2, float x3, float dt)
{
  return ctx_lerpf (
      ctx_lerpf (ctx_lerpf (x0, x1, dt),
                 ctx_lerpf (x1, x2, dt), dt),
      ctx_lerpf (ctx_lerpf (x1, x2, dt),
                 ctx_lerpf (x2, x3, dt), dt), dt);
}

CTX_INLINE static void
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
  float t = (s + e) * 0.5f;
  float x, y;
  float lx, ly, dx, dy;
  ctx_bezier_sample (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);
  lx = ctx_lerpf (sx, ex, t);
  ly = ctx_lerpf (sy, ey, t);
  dx = lx - x;
  dy = ly - y;
  if (iteration < 5 && (dx*dx+dy*dy) > tolerance)
  {
    ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                  sx, sy, x, y, s, t, iteration + 1,
                                  tolerance);
    ctx_rasterizer_line_to (rasterizer, x, y);
    ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                  x, y, ex, ey, t, e, iteration + 1,
                                  tolerance);
  }
}

static void
ctx_rasterizer_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2)
{
  float tolerance = 0.66f/ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
  float ox = rasterizer->state->x;
  float oy = rasterizer->state->y;

  tolerance = tolerance * tolerance;
  ctx_rasterizer_bezier_divide (rasterizer,
                                ox, oy, x0, y0,
                                x1, y1, x2, y2,
                                ox, oy, x2, y2,
                                0.0f, 1.0f, 0, tolerance);
  ctx_rasterizer_line_to (rasterizer, x2, y2);
}

static void
ctx_rasterizer_rel_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  //if (CTX_UNLIKELY(x == 0.f && y == 0.f))
  //{ return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_move_to (rasterizer, x, y);
}

static void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  //if (CTX_UNLIKELY(x== 0.f && y==0.f))
  //  { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_line_to (rasterizer, x, y);
}

static void
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
  ctx_matrix_identity (&source->set_transform);
  ctx_matrix_translate (&source->set_transform, x, y);
}


static void
ctx_rasterizer_define_texture (CtxRasterizer *rasterizer,
                               const char    *eid,
                               int            width,
                               int            height,
                               int            format,
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
  if (!rasterizer->state->gstate.source_fill.texture.buffer->color_managed)
  {
    _ctx_texture_prepare_color_management (rasterizer->state,
    rasterizer->state->gstate.source_fill.texture.buffer);
  }
  _ctx_texture_unlock ();
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



static void
ctx_rasterizer_fill (CtxRasterizer *rasterizer)
{
  unsigned int preserved_count =
          (rasterizer->preserve&&rasterizer->edge_list.count)?
             rasterizer->edge_list.count:1;
  int blit_x = rasterizer->blit_x;
  int blit_y = rasterizer->blit_y;
  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;
#if CTX_SHAPE_CACHE
  int blit_stride = rasterizer->blit_stride;
#endif

  CtxSegment temp[preserved_count]; /* copy of already built up path's poly line
                                       XXX - by building a large enough path
                                       the stack can be smashed!
                                     */
  if (CTX_UNLIKELY(rasterizer->preserve))
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) ); }

#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
  for (unsigned int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxSegment *entry = &((CtxSegment*)rasterizer->edge_list.entries)[i];
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
      rasterizer->scan_min > CTX_FULL_AA * (blit_y + blit_height) ||
      rasterizer->scan_max < CTX_FULL_AA * blit_y ||
      rasterizer->col_min > CTX_SUBDIV * (blit_x + blit_width) ||
      rasterizer->col_max < CTX_SUBDIV * blit_x))
    {
    }
  else
  {
    ctx_composite_setup (rasterizer);

    rasterizer->state->min_x = ctx_mini (rasterizer->state->min_x, rasterizer->col_min / CTX_SUBDIV);
    rasterizer->state->max_x = ctx_maxi (rasterizer->state->min_x, rasterizer->col_max / CTX_SUBDIV);
    rasterizer->state->min_y = ctx_mini (rasterizer->state->min_y, rasterizer->scan_min / CTX_FULL_AA);
    rasterizer->state->max_y = ctx_maxi (rasterizer->state->max_y, rasterizer->scan_max / CTX_FULL_AA);

#if CTX_FAST_FILL_RECT
  if (rasterizer->edge_list.count == 5)
    {
      CtxSegment *entry0 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[0];
      CtxSegment *entry1 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[1];
      CtxSegment *entry2 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[2];
      CtxSegment *entry3 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[3];


      if (
          (!(rasterizer->state->gstate.clipped != 0)) &
          (entry0->data.s16[2] == entry1->data.s16[2]) &
          (entry0->data.s16[3] == entry3->data.s16[3]) &
          (entry1->data.s16[3] == entry2->data.s16[3]) &
          (entry2->data.s16[2] == entry3->data.s16[2])
#if CTX_ENABLE_SHADOW_BLUR
           && !rasterizer->in_shadow
#endif
         )
       {
         float x0 = entry3->data.s16[2] * (1.0f / CTX_SUBDIV);
         float y0 = entry3->data.s16[3] * (1.0f / CTX_FULL_AA);
         float x1 = entry1->data.s16[2] * (1.0f / CTX_SUBDIV);
         float y1 = entry1->data.s16[3] * (1.0f / CTX_FULL_AA);

         if (x1 > x0 && y1 > y0)
         {
           ctx_composite_fill_rect (rasterizer, x0, y0, x1, y1, 255);
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
        int clip_x_min = blit_x;
        int clip_x_max = blit_x + blit_width - 1;
        int clip_y_min = blit_y;
        int clip_y_max = blit_y + blit_height - 1;

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
          rasterizer->scanline = scan_min;
          ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                        , NULL
#endif
                                       );
        }
        else
        {
        rasterizer->scanline = scan_min;
        CtxShapeEntry *shape = ctx_shape_entry_find (rasterizer, hash, width, height); 

        if (shape->uses == 0)
          {
            CtxBuffer *buffer_backup = rasterizer->clip_buffer;
            rasterizer->clip_buffer = NULL;
            ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule, shape);
            rasterizer->clip_buffer = buffer_backup;
          }

        int ewidth = x1 - x0;
        if (ewidth>0)
        {
          rasterizer->scanline = scan_min;
          int bpp = rasterizer->format->bpp;
          if (rasterizer->clip_buffer && !rasterizer->clip_rectangle)
          {
          uint8_t composite[ewidth];
          uint8_t *clip_data = (uint8_t*)rasterizer->clip_buffer->data;
          int shape_width = shape->width;
          for (int y = y0; y < y1; y++)
            {
              if ( (y >= clip_y_min) && (y <= clip_y_max) )
                {
                    for (int x = 0; x < ewidth; x++)
                    {
                      int val = shape->data[shape_width * (int)(y-ymin) + xo + x];
                      // XXX : not valid for 1bit clip buffers
                      val = (val*(clip_data) [
                              ((y-blit_y) * blit_width) + x0 + x])/255;
                      composite[x] = val;
                    }
                    rasterizer->apply_coverage (rasterizer,
                                                 ( (uint8_t *) rasterizer->buf) + (y-blit_y) * blit_stride + ((int) (x0) * bpp)/8,
                                                 rasterizer->color,
                                                 x0, // is 0
                                                 composite,
                                                 ewidth );
                 }
               rasterizer->scanline += CTX_FULL_AA;
            }
          }
          else
          {
          for (int y = y0; y < y1; y++)
            {
              if (CTX_LIKELY((y >= clip_y_min) && (y <= clip_y_max) ))
                {
                    rasterizer->apply_coverage (rasterizer,
                                                 ( (uint8_t *) rasterizer->buf) + (y-blit_y) * blit_stride + (int) ((x0) * bpp)/8, rasterizer->color,
                                                 x0,
                                                 &shape->data[shape->width * (int) (y-ymin) + xo],
                                                 ewidth );
                }
               rasterizer->scanline += CTX_FULL_AA;
            }
          }
         }
        }
      }
    else
#endif
    {
            
    ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                    , NULL
#endif
                                   );
    }
  }
#if CTX_FAST_FILL_RECT
done:
#endif
  if (CTX_UNLIKELY(rasterizer->preserve))
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = preserved_count;
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
    font_size = rasterizer->state->gstate.font_size;

    ch = ctx_term_get_cell_height (rasterizer->backend.ctx);
    cw = ctx_term_get_cell_width (rasterizer->backend.ctx);

    _ctx_user_to_device_distance (rasterizer->state, &tx, &font_size);
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
  _ctx_glyph (rasterizer->backend.ctx, unichar, stroke);
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
    font_size = rasterizer->state->gstate.font_size;
    _ctx_user_to_device_distance (rasterizer->state, &tx, &font_size);
  }
  int   ch = ctx_term_get_cell_height (rasterizer->backend.ctx);
  int   cw = ctx_term_get_cell_width (rasterizer->backend.ctx);

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
    _ctx_text (rasterizer->backend.ctx, string, stroke, 1);
  }
}

void
_ctx_font (Ctx *ctx, const char *name);
static void
ctx_rasterizer_set_font (CtxRasterizer *rasterizer, const char *font_name)
{
  _ctx_font (rasterizer->backend.ctx, font_name);
}

static void
ctx_rasterizer_arc (CtxRasterizer *rasterizer,
                    float          x,
                    float          y,
                    float          radius,
                    float          start_angle,
                    float          end_angle,
                    int            anticlockwise)
{
  int full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS;
  full_segments = radius * CTX_PI * 2 / 4.0;
  if (full_segments > CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS)
    { full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS; }
  if (full_segments < 24) full_segments = 24;
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

static void
ctx_rasterizer_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y)
{
  ctx_rasterizer_curve_to (rasterizer,
                           (cx * 2 + rasterizer->x) / 3.0f, (cy * 2 + rasterizer->y) / 3.0f,
                           (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                           x,                              y);
}

static void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                            float cx, float cy,
                            float x,  float y)
{
  ctx_rasterizer_quad_to (rasterizer, cx + rasterizer->x, cy + rasterizer->y,
                          x  + rasterizer->x, y  + rasterizer->y);
}

static void
ctx_rasterizer_stroke (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxSource source_backup;
  int count = rasterizer->edge_list.count;
  if (count == 0)
    return;
  if (gstate->source_stroke.type != CTX_SOURCE_INHERIT_FILL)
  {
    source_backup = gstate->source_fill;
    gstate->source_fill = rasterizer->state->gstate.source_stroke;
  }
  int preserved = rasterizer->preserve;
  float factor = ctx_matrix_get_scale (&gstate->transform);
  float line_width = gstate->line_width * factor;

  rasterizer->comp_op = NULL;
  ctx_composite_setup (rasterizer);

  CtxSegment temp[count]; /* copy of already built up path's poly line  */
  memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) );

#if CTX_FAST_FILL_RECT
  if (rasterizer->edge_list.count == 5)
    {
      CtxSegment *entry0 = &((CtxSegment*)rasterizer->edge_list.entries)[0];
      CtxSegment *entry1 = &((CtxSegment*)rasterizer->edge_list.entries)[1];
      CtxSegment *entry2 = &((CtxSegment*)rasterizer->edge_list.entries)[2];
      CtxSegment *entry3 = &((CtxSegment*)rasterizer->edge_list.entries)[3];

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

        float x0 = entry3->data.s16[2] * 1.0f / CTX_SUBDIV;
        float y0 = entry3->data.s16[3] * 1.0f / CTX_FULL_AA;
        float x1 = entry1->data.s16[2] * 1.0f / CTX_SUBDIV;
        float y1 = entry1->data.s16[3] * 1.0f / CTX_FULL_AA;

        ctx_composite_stroke_rect (rasterizer, x0, y0, x1, y1, line_width);

        goto done;


       }
    }
#endif
  
    {
    {
      if (line_width < 5.0f)
      {
      factor *= 0.89; /* this hack adjustment makes sharp 1px and 2px strokewidths
      //                 end up sharp without erronious AA; we seem to be off by
      //                 one somewhere else, causing the need for this
      //                 */
      line_width *= 0.89f;
      }
      ctx_rasterizer_reset (rasterizer); /* then start afresh with our stroked shape  */
      CtxMatrix transform_backup = gstate->transform;
      _ctx_matrix_identity (&gstate->transform);
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float half_width_x = line_width/2;
      float half_width_y = half_width_x;

      if (CTX_UNLIKELY(line_width <= 0.0f))
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
              CtxSegment *entry = &temp[i];
              float x, y;
              if (entry->code == CTX_NEW_EDGE)
                {
                  if (CTX_LIKELY(started))
                    {
                      end = i - 1;
                      goto foo;
                    }
                  prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  prev_y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
                  started = 1;
                  start = i;
                }
              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              if (length>0.001f)
                {
                  float recip_length = 1.0/length;
                  dx = dx * recip_length * half_width_x;
                  dy = dy * recip_length * half_width_y;
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
              CtxSegment *entry = &temp[i];
              float x, y, dx, dy;
              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
              dx = x - prev_x;
              dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              float recip_length = 1.0f/length;
              dx = dx * recip_length * half_width_x;
              dy = dy * recip_length * half_width_y;
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
                  y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
                  dx = x - prev_x;
                  dy = y - prev_y;
                  length = ctx_fast_hypotf (dx, dy);
                  recip_length = 1.0f/length;
                  if (CTX_LIKELY(length>0.001f))
                    {
                      dx = dx * recip_length * half_width_x;
                      dy = dy * recip_length * half_width_y;
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
                  CtxSegment *entry = &temp[i];
                  if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x, half_width_y);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
                      ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
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
                  CtxSegment *entry = &temp[i];
                  if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
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
                  CtxSegment *entry = &temp[i];
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
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
  }
#if CTX_FAST_FILL_RECT
done:
#endif
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


static void
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

static void
ctx_rasterizer_clip_apply (CtxRasterizer *rasterizer,
                           CtxSegment    *edges)
{
  unsigned int count = edges[0].data.u32[0];

  int minx = 5000;
  int miny = 5000;
  int maxx = -5000;
  int maxy = -5000;
  int prev_x = 0;
  int prev_y = 0;
  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;

  float coords[6][2];

  for (unsigned int i = 0; i < count; i++)
    {
      CtxSegment *entry = &edges[i+1];
      float x, y;
      if (entry->code == CTX_NEW_EDGE)
        {
          prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
          prev_y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
          if (prev_x < minx) { minx = prev_x; }
          if (prev_y < miny) { miny = prev_y; }
          if (prev_x > maxx) { maxx = prev_x; }
          if (prev_y > maxy) { maxy = prev_y; }
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
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
    if (count == 5)
    {
      if (coords[0][0] == coords[1][0] &&
          coords[0][1] == coords[4][1] &&
          coords[0][1] == coords[3][1] &&
          coords[1][1] == coords[2][1] &&
          coords[3][0] == coords[4][0]
          )
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
#if 0
      else
      {
        printf ("%d,%d %dx%d  0,0:%.2f 0,1:%.2f 1,0:%.2f 11:%.2f 20:%.2f 21:%2.f 30:%.2f 31:%.2f 40:%.2f 41:%.2f\n", minx, miny,
                                       maxx-minx+1, maxy-miny+1
                                       
         ,coords[0][0] ,  coords[0][1]
         ,coords[1][0] ,  coords[1][1]
         ,coords[2][0] ,  coords[2][1]
         ,coords[3][0] ,  coords[3][1]
         ,coords[4][0] ,  coords[4][1]
         );
      }
#endif
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

  for (unsigned int i = 0; i < count; i++)
    {
      CtxSegment *entry = &edges[i+1];
      float x, y;
      if (entry->code == CTX_NEW_EDGE)
        {
          prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
          prev_y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
          ctx_move_to (ctx, prev_x, prev_y);
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
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
    unsigned int count = blit_width * blit_height / 8;
    for (unsigned int i = 0; i < count; i++)
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

static void
ctx_rasterizer_clip (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxSegment temp[count+1]; /* copy of already built up path's poly line  */
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
  ctx_buffer_load_png (&rasterizer->backend.ctx->texture[0], path);
  ctx_rasterizer_set_texture (rasterizer, 0, x, y);
}
#endif


static void
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
  //ctx_rasterizer_rel_line_to (rasterizer, width/2, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

static void
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
static inline float
ctx_gaussian (float x, float mu, float sigma)
{
  float a = ( x- mu) / sigma;
  return ctx_expf (-0.5 * a * a);
}

static inline void
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

static void
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
ctx_rasterizer_process (Ctx *ctx, CtxCommand *command);

#if CTX_COMPOSITING_GROUPS
static void
ctx_rasterizer_start_group (CtxRasterizer *rasterizer) /* add a radius? */
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
  ctx_rasterizer_process (rasterizer->backend.ctx, (CtxCommand*)&save_command);
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

  Ctx *ctx = rasterizer->backend.ctx;

  CtxCompositingMode comp = rasterizer->state->gstate.compositing_mode;
  CtxBlend blend = rasterizer->state->gstate.blend_mode;
  float global_alpha = rasterizer->state->gstate.global_alpha_f;
  // fetch compositing, blending, global alpha
  ctx_rasterizer_process (ctx, (CtxCommand*)&restore_command);
  ctx_rasterizer_process (ctx, (CtxCommand*)&save_command);
  CtxEntry set_state[3]=
  {
    ctx_u32 (CTX_COMPOSITING_MODE, comp,  0),
    ctx_u32 (CTX_BLEND_MODE,       blend, 0),
    ctx_f  (CTX_GLOBAL_ALPHA,     global_alpha, 0.0)
  };
  ctx_rasterizer_process (ctx, (CtxCommand*)&set_state[0]);
  ctx_rasterizer_process (ctx, (CtxCommand*)&set_state[1]);
  ctx_rasterizer_process (ctx, (CtxCommand*)&set_state[2]);
  if (no == 0)
  {
    rasterizer->buf = rasterizer->saved_buf;
  }
  else
  {
    rasterizer->buf = rasterizer->group[no-1]->data;
  }
  // XXX use texture_source ?
   ctx_texture_init (ctx, ".ctx-group", 
                  rasterizer->blit_width, 
                  rasterizer->blit_height,
                                         
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
       ctx_f   (CTX_TEXTURE, rasterizer->blit_x, rasterizer->blit_y), 
       ctx_u32 (CTX_DATA, eid_len, eid_len/9+1),
       ctx_u32 (CTX_CONT, 0,0),
       ctx_u32 (CTX_CONT, 0,0)
      };
     memcpy( (char *) &commands[2].data.u8[0], eid, eid_len);
     ( (char *) (&commands[2].data.u8[0]) ) [eid_len]=0;

     ctx_rasterizer_process (ctx, (CtxCommand*)commands);
  }
  {
    CtxEntry commands[2]=
    {
      ctx_f (CTX_RECTANGLE, rasterizer->blit_x, rasterizer->blit_y),
      ctx_f (CTX_CONT,      rasterizer->blit_width, rasterizer->blit_height)
    };
    ctx_rasterizer_process (ctx, (CtxCommand*)commands);
  }
  {
    CtxEntry commands[1] = { ctx_void (CTX_FILL) };
    ctx_rasterizer_process (ctx, (CtxCommand*)commands);
  }
  //ctx_texture_release (rasterizer->backend.ctx, ".ctx-group");
  ctx_buffer_free (rasterizer->group[no]);
  rasterizer->group[no] = 0;
  ctx_rasterizer_process (ctx, (CtxCommand*)&restore_command);
}
#endif

#if CTX_ENABLE_SHADOW_BLUR
static void
ctx_rasterizer_shadow_stroke (CtxRasterizer *rasterizer)
{
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);
  Ctx *ctx = rasterizer->backend.ctx;

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->backend.ctx, CTX_shadowColor, &color) == 0)
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
  ctx_rasterizer_process (ctx, (CtxCommand*)&save_command);
  {
    int i = 0;
    for (int v = 0; v < dim; v += 1, i++)
      {
        float dy = rasterizer->state->gstate.shadow_offset_y + v - dim/2;
        set_color_command[2].data.f[0] = rasterizer->kernel[i] * rgba[3];
        ctx_rasterizer_process (ctx, (CtxCommand*)&set_color_command[0]);
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
  ctx_rasterizer_process (ctx, (CtxCommand*)&restore_command);
}

static void
ctx_rasterizer_shadow_text (CtxRasterizer *rasterizer, const char *str)
{
  float x = rasterizer->state->x;
  float y = rasterizer->state->y;
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);
  Ctx *ctx = rasterizer->backend.ctx;

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->backend.ctx, CTX_shadowColor, &color) == 0)
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
  ctx_rasterizer_process (ctx, (CtxCommand*)&save_command);

  {
      {
        move_to_command[0].data.f[0] = x;
        move_to_command[0].data.f[1] = y;
        set_color_command[2].data.f[0] = rgba[3];
        ctx_rasterizer_process (ctx, (CtxCommand*)&set_color_command);
        ctx_rasterizer_process (ctx, (CtxCommand*)&move_to_command);
        rasterizer->in_shadow=1;
        ctx_rasterizer_text (rasterizer, str, 0);
        rasterizer->in_shadow=0;
      }
  }
  ctx_rasterizer_process (ctx, (CtxCommand*)&restore_command);
  move_to_command[0].data.f[0] = x;
  move_to_command[0].data.f[1] = y;
  ctx_rasterizer_process (ctx, (CtxCommand*)&move_to_command);
}

static void
ctx_rasterizer_shadow_fill (CtxRasterizer *rasterizer)
{
  CtxColor color;
  Ctx *ctx = rasterizer->backend.ctx;
  CtxEntry save_command = ctx_void(CTX_SAVE);

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->backend.ctx, CTX_shadowColor, &color) == 0)
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
  ctx_rasterizer_process (ctx, (CtxCommand*)&save_command);

  {
    for (int v = 0; v < dim; v ++)
      {
        int i = v;
        float dy = rasterizer->state->gstate.shadow_offset_y + v - dim/2;
        set_color_command[2].data.f[0] = rasterizer->kernel[i] * rgba[3];
        ctx_rasterizer_process (ctx, (CtxCommand*)&set_color_command);
        rasterizer->in_shadow = 1;
        rasterizer->shadow_x = rasterizer->state->gstate.shadow_offset_x;
        rasterizer->shadow_y = dy;
        rasterizer->preserve = 1;
        ctx_rasterizer_fill (rasterizer);
        rasterizer->in_shadow = 0;
      }
  }
  ctx_rasterizer_process (ctx, (CtxCommand*)&restore_command);
}
#endif

static void
ctx_rasterizer_line_dash (CtxRasterizer *rasterizer, unsigned int count, float *dashes)
{
  if (!dashes)
  {
    rasterizer->state->gstate.n_dashes = 0;
    return;
  }
  count = CTX_MIN(count, CTX_PARSER_MAX_ARGS-1);
  rasterizer->state->gstate.n_dashes = count;
  memcpy(&rasterizer->state->gstate.dashes[0], dashes, count * sizeof(float));
  for (unsigned int i = 0; i < count; i ++)
  {
    if (rasterizer->state->gstate.dashes[i] < 0.0001f)
      rasterizer->state->gstate.dashes[i] = 0.0001f; // hang protection
  }
}


static void
ctx_rasterizer_process (Ctx *ctx, CtxCommand *command)
{
  CtxEntry      *entry      = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) ctx->backend;
  CtxState      *state      = rasterizer->state;
  CtxCommand    *c          = (CtxCommand *) entry;
  int            clear_clip = 0;

  ctx_interpret_style (state, entry, NULL);
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
          ctx_set_color (rasterizer->backend.ctx, CTX_shadowColor, color);
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
      case CTX_SOURCE_TRANSFORM:
        ctx_matrix_set (&state->gstate.source_fill.set_transform,
                        ctx_arg_float (0), ctx_arg_float (1),
                        ctx_arg_float (2), ctx_arg_float (3),
                        ctx_arg_float (4), ctx_arg_float (5),
                        ctx_arg_float (6), ctx_arg_float (7),
                        ctx_arg_float (8));
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
          rasterizer->comp_op = NULL;
        }
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_state_gradient_clear_stops (state);
        rasterizer->comp_op = NULL;
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_state_gradient_clear_stops (state);
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
      case CTX_APPLY_TRANSFORM:
      case CTX_TRANSLATE:
      case CTX_IDENTITY:
        rasterizer->uses_transforms = 1;
        /* FALLTHROUGH */
      case CTX_SAVE:
        rasterizer->comp_op = NULL;
        ctx_interpret_transforms (state, entry, NULL);
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
              CtxSegment *edges = (CtxSegment*)&state->stringpool[idx];
              ctx_rasterizer_clip_apply (rasterizer, edges);
            }
          }
        }
        }
        break;
      case CTX_STROKE:
#if CTX_ENABLE_SHADOW_BLUR
        if (state->gstate.shadow_blur > 0.0 &&
            !rasterizer->in_text)
          ctx_rasterizer_shadow_stroke (rasterizer);
#endif
        {
        int count = rasterizer->edge_list.count;
        if (state->gstate.n_dashes)
        {
          int n_dashes = state->gstate.n_dashes;
          float *dashes = state->gstate.dashes;
          float factor = ctx_matrix_get_scale (&state->gstate.transform);

          CtxSegment temp[count]; /* copy of already built up path's poly line  */
          memcpy (temp, rasterizer->edge_list.entries, sizeof (temp));
          int start = 0;
          int end   = 0;
      CtxMatrix transform_backup = state->gstate.transform;
      _ctx_matrix_identity (&state->gstate.transform);
      ctx_rasterizer_reset (rasterizer); /* for dashing we create
                                            a dashed path to stroke */
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float pos = 0.0;

      int   dash_no  = 0.0;
      float dash_lpos = state->gstate.line_dash_offset * factor;
      int   is_down = 0;

          while (start < count)
          {
            int started = 0;
            int i;
            is_down = 0;

            if (!is_down)
            {
              CtxSegment *entry = &temp[0];
              prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
              prev_y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
              ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
              is_down = 1;
            }

            for (i = start; i < count; i++)
            {
              CtxSegment *entry = &temp[i];
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
                  prev_y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
                  started = 1;
                  start = i;
                  is_down = 1;
                  ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
                }

again:

              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
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
        state->gstate.transform = transform_backup;
        }
        ctx_rasterizer_stroke (rasterizer);
        }
        ctx_rasterizer_reset (rasterizer);

        break;
      case CTX_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_TEXT:
        rasterizer->in_text++;
#if CTX_ENABLE_SHADOW_BLUR
        if (state->gstate.shadow_blur > 0.0)
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
        if (state->gstate.shadow_blur > 0.0 &&
            !rasterizer->in_text)
          ctx_rasterizer_shadow_fill (rasterizer);
#endif
        ctx_rasterizer_fill (rasterizer);
        ctx_rasterizer_reset (rasterizer);
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
      case CTX_IMAGE_SMOOTHING:
        rasterizer->comp_op = NULL;
        break;
    }
  ctx_interpret_pos_bare (state, entry, NULL);
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
  if (ctx_backend_is_tiled (ctx))
  {
     CtxTiled *fb = (CtxTiled*)(ctx->backend);
     return fb->antialias;
  }
#endif
  if (ctx_backend_type (ctx) != CTX_BACKEND_RASTERIZER) return CTX_ANTIALIAS_DEFAULT;

  switch (((CtxRasterizer*)(ctx->backend))->aa)
  {
    case 1: return CTX_ANTIALIAS_NONE;
    case 3: return CTX_ANTIALIAS_FAST;
    //case 5: return CTX_ANTIALIAS_GOOD;
    default:
    case 15: return CTX_ANTIALIAS_DEFAULT;
  }
}

static int _ctx_antialias_to_aa (CtxAntialias antialias)
{
  switch (antialias)
  {
    case CTX_ANTIALIAS_NONE: return 1;
    case CTX_ANTIALIAS_FAST: return 3;
    case CTX_ANTIALIAS_GOOD: return 5;
    default:
    case CTX_ANTIALIAS_DEFAULT: return CTX_RASTERIZER_AA;
  }
}

void
ctx_set_antialias (Ctx *ctx, CtxAntialias antialias)
{
#if CTX_EVENTS
  if (ctx_backend_is_tiled (ctx))
  {
     CtxTiled *fb = (CtxTiled*)(ctx->backend);
     fb->antialias = antialias;
     for (int i = 0; i < _ctx_max_threads; i++)
     {
       ctx_set_antialias (fb->host[i], antialias);
     }
     return;
  }
#endif
  if (ctx_backend_type (ctx) != CTX_BACKEND_RASTERIZER) return;

  ((CtxRasterizer*)(ctx->backend))->aa = 
     _ctx_antialias_to_aa (antialias);
  ((CtxRasterizer*)(ctx->backend))->fast_aa = 0;
  if (antialias == CTX_ANTIALIAS_DEFAULT||
      antialias == CTX_ANTIALIAS_FAST)
    ((CtxRasterizer*)(ctx->backend))->fast_aa = 1;
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
#if CTX_SHAPE_CACHE
  memset (rasterizer, 0, sizeof (CtxRasterizer) - sizeof (CtxShapeCache));
#else
  memset (rasterizer, 0, sizeof (CtxRasterizer));
#endif
  CtxBackend *backend = (CtxBackend*)rasterizer;
  backend->process = ctx_rasterizer_process;
  backend->free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  backend->ctx     = ctx;
  rasterizer->edge_list.flags |= CTX_DRAWLIST_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->texture_source = texture_source?texture_source:ctx;

  rasterizer->aa          = _ctx_antialias_to_aa (antialias);
  rasterizer->fast_aa = (antialias == CTX_ANTIALIAS_DEFAULT||antialias == CTX_ANTIALIAS_FAST);
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

  if (pixel_format == CTX_FORMAT_BGRA8)
  {
    pixel_format = CTX_FORMAT_RGBA8;
    rasterizer->swap_red_green = 1;
  }

  rasterizer->format = ctx_pixel_format_info (pixel_format);

  return rasterizer;
}

Ctx *
ctx_new_for_buffer (CtxBuffer *buffer)
{
  Ctx *ctx = _ctx_new_drawlist (buffer->width, buffer->height);
  ctx_set_backend (ctx,
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
  Ctx *ctx = _ctx_new_drawlist (width, height);
  CtxRasterizer *r = ctx_rasterizer_init ( (CtxRasterizer *) ctx_calloc (sizeof (CtxRasterizer), 1),
                                          ctx, NULL, &ctx->state, data, 0, 0, width, height,
                                          stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
  ctx_set_backend (ctx, r);
  if (pixel_format == CTX_FORMAT_GRAY1) // XXX we get some bugs without it..
  {                                     //     something is going amiss with offsets
    ctx_set_antialias (ctx, CTX_ANTIALIAS_NONE);
  }
  return ctx;
}

// ctx_new_for_stream (FILE *stream);

#if 0
CtxRasterizer *ctx_rasterizer_new (void *data, int x, int y, int width, int height,
                                   int stride, CtxPixelFormat pixel_format)
{
  CtxState    *state    = (CtxState *) malloc (sizeof (CtxState) );
  CtxRasterizer *rasterizer = (CtxRasterizer *) malloc (sizeof (CtxBackend) );
  ctx_rasterizer_init (rasterizer, state, data, x, y, width, height,
                       stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
}
#endif

#else

#endif


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


/****  end of engine ****/
