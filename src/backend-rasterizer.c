
#ifndef __clang__
#if CTX_RASTERIZER_O3
#pragma GCC push_options
#pragma GCC optimize("O3")
#endif
#if CTX_RASTERIZER_O2
#pragma GCC push_options
#pragma GCC optimize("O2")
#endif
#endif

#if CTX_IMPLEMENTATION || CTX_SIMD_BUILD
#if CTX_COMPOSITE 

#include "ctx-split.h"
#define CTX_AA_HALFSTEP2   (CTX_FULL_AA/2)
#define CTX_AA_HALFSTEP    ((CTX_FULL_AA/2)+1)

static CTX_INLINE int ctx_compare_edges (const void *ap, const void *bp)
{
  const CtxSegment *a = (const CtxSegment *) ap;
  const CtxSegment *b = (const CtxSegment *) bp;
  return a->data.s16[1] - b->data.s16[1];
}

static inline int ctx_edge_qsort_partition (CtxSegment *A, int low, int high)
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

static CTX_INLINE void ctx_rasterizer_discard_edges (CtxRasterizer *rasterizer)
{
  int scanline = rasterizer->scanline + 1;
  int next_scanline = scanline + CTX_FULL_AA;
  CtxSegment *segments = &((CtxSegment*)(rasterizer->edge_list.entries))[0];
  int *edges = rasterizer->edges;
  int ending_edges = 0;
  unsigned int active_edges = rasterizer->active_edges;
  for (unsigned int i = 0; i < active_edges; i++)
    {
      CtxSegment *segment = segments + edges[i];
      int edge_end = segment->data.s16[3];
      if (edge_end < scanline)
        {
          rasterizer->edges[i] = rasterizer->edges[active_edges-1];
          rasterizer->scan_aa[segment->aa]--;
          active_edges--;
          i--;
        }
      else ending_edges += (edge_end < next_scanline);
    }
  rasterizer->active_edges = active_edges;

  unsigned int pending_edges = rasterizer->pending_edges;
  for (unsigned int i = 0; i < pending_edges; i++)
    {
      int edge_end = ((CtxSegment*)(rasterizer->edge_list.entries))[rasterizer->edges[CTX_MAX_EDGES-1-i]].data.s16[3];
      ending_edges += (edge_end < next_scanline);
    }
  rasterizer->ending_edges = ending_edges;
}

CTX_INLINE static void ctx_rasterizer_increment_edges (CtxRasterizer *rasterizer, int count)
{
  rasterizer->scanline += count;
  CtxSegment *__restrict__ segments = &((CtxSegment*)(rasterizer->edge_list.entries))[0];
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
CTX_INLINE static void ctx_edge2_insertion_sort (CtxSegment *segments, int *__restrict__ entries, unsigned int count)
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

CTX_INLINE static void ctx_rasterizer_feed_edges (CtxRasterizer *rasterizer)
{
  CtxSegment *__restrict__ entries = (CtxSegment*)&rasterizer->edge_list.entries[0];
  int *edges = rasterizer->edges;
  unsigned int pending_edges   = rasterizer->pending_edges;
  int scanline = rasterizer->scanline + 1;
  int active_edges = rasterizer->active_edges;
  for (unsigned int i = 0; i < pending_edges; i++)
    {
      if ((entries[edges[CTX_MAX_EDGES-1-i]].data.s16[1] <= scanline) &
          (active_edges < CTX_MAX_EDGES-2))
        {
          edges[active_edges] = edges[CTX_MAX_EDGES-1-i];
          active_edges++;
          edges[CTX_MAX_EDGES-1-i] =
            edges[CTX_MAX_EDGES-1-pending_edges + 1];
          pending_edges--;
          i--;
        }
    }
    rasterizer->active_edges = active_edges;
    rasterizer->pending_edges = pending_edges;
    ctx_rasterizer_discard_edges (rasterizer);
}

CTX_INLINE static int analyze_scanline (CtxRasterizer *rasterizer, unsigned int active_edges, int pending_edges, int horizontal_edges)
{
  if (rasterizer->edge_list.count <= 5)
     return 0;
  if ((rasterizer->fast_aa == 0) |
      horizontal_edges| // XXX : maybe superfluous?
      rasterizer->ending_edges|
      pending_edges)
  {
    return CTX_RASTERIZER_AA;
  }

  const int *edges  = rasterizer->edges;
  const CtxSegment *segments = &((CtxSegment*)(rasterizer->edge_list.entries))[0];

  int crossings = 0;

  const CtxSegment *segment0 = segments + edges[0];
  const int delta0    = segment0->delta;
  const int x0        = segment0->val;
  int x0_end   = x0 + delta0 * CTX_AA_HALFSTEP;
  int x0_start = x0 - delta0 * CTX_AA_HALFSTEP2;

  unsigned int t = 1;
  for (t = 1; (t < active_edges);t++)
    {
      const CtxSegment *segment1 = segments + edges[t];
      const int delta1           = segment1->delta;
      const int x1               = segment1->val;
      const int x1_end   = x1 + delta1 * CTX_AA_HALFSTEP;
      const int x1_start = x1 - delta1 * CTX_AA_HALFSTEP2;

      crossings |=  ((x1_end < x0_end)   | (x1_start < x0_end) | (x1_end < x0_start));
      x0_end = x1_end;
      x0_start = x1_start;
    }

#if CTX_RASTERIZER_AA > 5
  int aa = ((rasterizer->scan_aa[3]>0) *15) + (rasterizer->scan_aa[3]<=0)*
     ((rasterizer->scan_aa[2]>0) * 5 +
       (rasterizer->scan_aa[2]<=0) * ((rasterizer->scan_aa[1]>0) * 3 + (rasterizer->scan_aa[1]<=0)));
#else
  int aa = (rasterizer->scan_aa[2]>0) * 5 +
       (rasterizer->scan_aa[2]<=0) * ((rasterizer->scan_aa[1]>0) * 3 + (rasterizer->scan_aa[1]<=0));
#endif

  return aa * crossings;
}

inline static int ctx_rasterizer_feed_edges_full (CtxRasterizer *rasterizer)
{
  int miny;
  ctx_rasterizer_feed_edges (rasterizer);
  CtxSegment *__restrict__ entries = (CtxSegment*)&rasterizer->edge_list.entries[0];
  int *edges = rasterizer->edges;
  unsigned int pending_edges   = rasterizer->pending_edges;
  int scanline = rasterizer->scanline + 1;
  unsigned int edge_pos = rasterizer->edge_pos;
  int next_scanline = scanline + CTX_FULL_AA;
  unsigned int edge_count = rasterizer->edge_list.count;
  int active_edges = rasterizer->active_edges;
  int horizontal_edges = 0;
  while ((edge_pos < edge_count &&
         (miny=entries[edge_pos].data.s16[1])  <= next_scanline))
    {
      if ((active_edges < CTX_MAX_EDGES-2) &
        (entries[edge_pos].data.s16[3] /* (maxy) */  >= scanline))
        {
          int dy = (entries[edge_pos].data.s16[3] - miny);
          if (dy)
            {
              int yd = scanline - miny;
              unsigned int index = edges[active_edges] = edge_pos;
              int x0 = entries[index].data.s16[0];
              int x1 = entries[index].data.s16[2];
              int dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
              entries[index].delta = dx_dy;
              entries[index].val = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER +
                                         (yd * dx_dy);

	      dx_dy = abs(dx_dy);

#if CTX_RASTERIZER_AA>5
	      entries[index].aa =
                 (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT15) +
                 (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT5) +
                 (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA);
#else
	      entries[index].aa =
                 (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT5) +
                 (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA);
#endif
	      rasterizer->scan_aa[entries[index].aa]++;

              if ((miny > scanline) &
                  (pending_edges < CTX_MAX_PENDING-1))
              {
                  /* it is a pending edge - we add it to the end of the array
                     and keep a different count for items stored here, like
                     a heap and stack growing against each other
                  */
                    edges[CTX_MAX_EDGES-1-pending_edges] = edges[active_edges];
                    pending_edges++;
                    active_edges--;
              }
              active_edges++;
            }
            else
            horizontal_edges++;
        }
      edge_pos++;
    }

    rasterizer->active_edges = active_edges;
    rasterizer->edge_pos = edge_pos;
    rasterizer->pending_edges = pending_edges;
    rasterizer->horizontal_edges = horizontal_edges;
    if (active_edges + pending_edges == 0)
      return -1;
    return analyze_scanline (rasterizer, active_edges, pending_edges, horizontal_edges);
}

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
  if (CTX_UNLIKELY((rasterizer->clip_buffer!=NULL) &  (!rasterizer->clip_rectangle)))
  {
  int scanline     = rasterizer->scanline - CTX_FULL_AA; // we do the
                                                 // post process after
                                                 // coverage generation icnrement
    /* perhaps not working right for clear? */
    int y = scanline / CTX_FULL_AA;//rasterizer->aa;
    uint8_t *clip_line = &((uint8_t*)(rasterizer->clip_buffer->data))[rasterizer->blit_width*y];
    // XXX SIMD candidate
#if CTX_1BIT_CLIP==0
    int blit_x = rasterizer->blit_x;
#endif
    for (unsigned int x = minx; x <= maxx; x ++)
    {
#if CTX_1BIT_CLIP
       coverage[x] = (coverage[x] * ((clip_line[x/8]&(1<<(x&8)))?255:0))/255;
#else
       coverage[x] = (255 + coverage[x] * clip_line[x-blit_x])>>8;
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

#if 1
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
  int accumulator_x = minx;
  uint8_t accumulated = 0;
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

	  if (accumulated)
	  {
	    if (accumulator_x == first)
	    {
	       graystart += accumulated;
	    } else if (accumulator_x >= minx)
	    {
	       coverage[accumulator_x] = accumulated;
	    }
	    accumulated = 0;
	  }

          if (first < last)
	  {
              coverage[first] += graystart;
              memset(&coverage[first+1], 255, last-(first+1));
	      accumulated = grayend;
	  }
	  else
	  {
	    accumulated = (graystart-(grayend^255));
	  }
	  accumulator_x = last;
        }
   }
  if (accumulated)
  {
    coverage[accumulator_x] = accumulated;
  }
}
#endif

#if 1


inline static void
ctx_rasterizer_generate_coverage_apply (CtxRasterizer *rasterizer,
                                        int            minx,
                                        int            maxx,
               //                       uint8_t* __restrict__ coverage,
                                        int            is_winding,
                                        CtxCovPath     comp)
{
  CtxSegment *entries     = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  uint8_t *rasterizer_src = rasterizer->color;
  int *edges              = rasterizer->edges;
  int scanline            = rasterizer->scanline;
  const int bpp           = rasterizer->format->bpp;
  int active_edges        = rasterizer->active_edges;
  int parity              = 0;
#if CTX_RASTERIZER_SWITCH_DISPATCH
  uint32_t *src_pixp;
  uint32_t src_pix, si_ga, si_rb, si_ga_full, si_rb_full, si_a;
  if ((comp != CTX_COV_PATH_FALLBACK) &
      (comp != CTX_COV_PATH_RGBA8_COPY_FRAGMENT) &
      (comp != CTX_COV_PATH_RGBA8_OVER_FRAGMENT))
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
  int accumulator_x = minx;
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
          grayend   = (grayend&0xff);

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
                  rasterizer->apply_coverage (rasterizer, (uint8_t*)dst_pix, rasterizer_src, accumulator_x, &accumulated, 1);
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
                for (unsigned int i = first + 1; i < (unsigned)last; i++)
                  *dst_pix++=src_pix;
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
              rasterizer->apply_coverage (rasterizer, (uint8_t*)dsts, rasterizer_src, first, &startcov, 1);
              uint8_t* dst_i = (uint8_t*)dsts;
              uint8_t *color = ((uint8_t*)&rasterizer->color_native);
              unsigned int bytes = bpp/8;
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

#if CTX_ENABLE_GRAY4
              case CTX_COV_PATH_GRAY4_COPY:
              {
                uint8_t* dstp = (uint8_t*)(&dst[(first *bpp)/8]);
                uint8_t *srcp = (uint8_t*)src_pixp;
                uint8_t  startcov = graystart;
                rasterizer->apply_coverage (rasterizer, (uint8_t*)dstp, rasterizer_src, first, &startcov, 1);
                dstp = (uint8_t*)(&dst[((first+1)*bpp)/8]);
                unsigned int count = last - first - 1;
                int val = srcp[0]/17;

                uint8_t val_x2 = val + (val << 4);
                {
                  int x = first + 1;
                  for (unsigned int i = 0; (i < count) & (x & 1); count--)
                  {
                     int bitno = x & 1;
                     *dstp &= ~(15<<(bitno*4));
                     *dstp |= (val<<(bitno*4));
                     dstp += (bitno == 1);
                     x++;
                  }

                  for (unsigned int i = 0; (i < count) & (count>2); count-=2, x+=2, dstp++)
                     *dstp = val_x2;

                  for (unsigned int i = 0; i < count; count--)
                  {
                     int bitno = x & 1;
                     *dstp &= ~(15<<(bitno*4));
                     *dstp |= (val<<(bitno*4));
                     dstp += (bitno == 1);
                     x++;
                  }
                }
              }
              break;
#endif

#if CTX_ENABLE_GRAY2
              case CTX_COV_PATH_GRAY2_COPY:
              {
                uint8_t* dstp = (uint8_t*)(&dst[(first *bpp)/8]);
                uint8_t *srcp = (uint8_t*)src_pixp;
                uint8_t  startcov = graystart;
                rasterizer->apply_coverage (rasterizer, (uint8_t*)dstp, rasterizer_src, first, &startcov, 1);
                dstp = (uint8_t*)(&dst[((first+1)*bpp)/8]);
                unsigned int count = last - first - 1;
                int val = srcp[0]/85; 

                uint8_t val_x4 = val + (val << 2) + (val << 4) + (val << 6);
                {
                  int x = first + 1;
                  for (unsigned int i = 0; (i < count) & (x & 3); count--)
                  {
                     int bitno = x & 3;
                     *dstp &= ~(3<<(bitno*2));
                     *dstp |= (val<<(bitno*2));
                     dstp += (bitno == 3);
                     x++;
                  }

                  for (unsigned int i = 0; (i < count) & (count>4); count-=4, x+=4, dstp++)
                     *dstp = val_x4;

                  for (unsigned int i = 0; i < count; count--)
                  {
                     int bitno = x & 3;
                     *dstp &= ~(3<<(bitno*2));
                     *dstp |= (val<<(bitno*2));
                     dstp += (bitno == 3);
                     x++;
                  }
                }
              }
              break;
#endif

#if CTX_ENABLE_GRAY1
              case CTX_COV_PATH_GRAY1_COPY:
              {
                uint8_t* dstp = (uint8_t*)(&dst[(first *bpp)/8]);
                uint8_t *srcp = (uint8_t*)src_pixp;
                uint8_t  startcov = graystart;
                rasterizer->apply_coverage (rasterizer, (uint8_t*)dstp, rasterizer_src, first, &startcov, 1);
                dstp = (uint8_t*)(&dst[((first+1)*bpp)/8]);
                unsigned int count = last - first - 1;
                if (srcp[0]>=127)
                {
                  int x = first + 1;
                  for (unsigned int i = 0; (i < count) & (x & 7); count--)
                  {
                     int bitno = x & 7;
                     *dstp |= (1<<bitno);
                     dstp += (bitno == 7);
                     x++;
                  }

                  for (unsigned int i = 0; (i < count) & (count>8); count-=8)
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
                  for (unsigned int i = 0; (i < count) & ((x & 7)!=0); count--)
                  {
                     int bitno = x & 7;
                     *dstp &= ~(1<<bitno);
                     dstp += (bitno == 7);
                     x++;
                  }

                  for (unsigned int i = 0; (i < count) & (count>8); count-=8)
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
#endif

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
              float w0 = 1; float wd = 0;
              uint8_t gs = graystart;
              ctx_RGBA8_source_copy_normal_fragment (rasterizer, &dst[(first * bpp)/8], NULL, first, &gs, 1);
              ctx_init_uv (rasterizer, first+1, scanline/CTX_FULL_AA,&u0, &v0, &w0, &ud, &vd, &wd);
              rasterizer->fragment (rasterizer, u0, v0, w0, &dst[((first+1)*bpp)/8], last-first-1, ud, vd, wd);
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
#if CTX_STATIC_OPAQUE
              uint8_t *opaque = &rasterizer->opaque[0];
#else
              uint8_t opaque[last-first];
              memset (opaque, 255, sizeof (opaque));
#endif
              opaque[0] = graystart;
              rasterizer->apply_coverage (rasterizer,
                              &dst[(first * bpp)/8],
                              rasterizer_src, first, opaque, last-first);

#if CTX_STATIC_OPAQUE
              opaque[0] = 255;
#endif
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
         rasterizer->apply_coverage (rasterizer, (uint8_t*)dst_pix, rasterizer_src,
                         accumulator_x, &accumulated, 1);
     }
   }
}
#endif


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

            if (segment->aa == 0)
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
                coverage[us + count] += ((u - u0 + mod) * recip)>>16;
                count++;
              }
              pre = (us+count-1)-first+1;
            }
  
            if (next_segment->aa == 0)
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
              int sum = ((u1-u0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV * 5/4)/255);
              int recip = 65536 / sum;
              for (unsigned int u = u0; u < u1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
              {
                coverage[us + count] += (((u - u0 + mod) * recip)>>16) ^ 255;
                count++;
              }
              post = last-us;
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
  uint8_t *rasterizer_src = rasterizer->color;
  int  parity         = 0;

#if CTX_RASTERIZER_SWITCH_DISPATCH
  uint32_t *src_pixp;
  uint32_t src_pix, si_ga, si_rb, si_ga_full, si_rb_full, si_a;
  if ((comp != CTX_COV_PATH_FALLBACK) &
      (comp != CTX_COV_PATH_RGBA8_COPY_FRAGMENT) &
      (comp != CTX_COV_PATH_RGBA8_OVER_FRAGMENT))
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
          graystart = 255-(graystart&0xff);
          grayend   = (grayend & 0xff);

          if (first < last)
          {
            int pre = 1;
            int post = 1;

          if (segment->aa==0)
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
              coverage[us + count] += ((u - u0 + mod) * recip)>>16;
              count++;
            }
            pre = us+count-first;

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
                          rasterizer_src,
                          accumulated_x0,
                          &coverage[accumulated_x0],
                          accumulated_x1-accumulated_x0+1);
             }
             accumulated_x0 = 65538;
             accumulated_x1 = 65536;
          }

          if (next_segment->aa == 0)
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
            int sum = ((u1-u0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV * 5/4)/255);

            int recip = 65536/ sum;
            for (unsigned int u = u0; u < u1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
            {
              coverage[us + count] = (((u - u0 + mod)*recip)>>16)^255;
              count++;
            }
            post = last-us;

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
              unsigned int bytes = bpp/8;
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
                float w0 = 1; float wd = 0;
                ctx_init_uv (rasterizer, first+pre, rasterizer->scanline/CTX_FULL_AA,&u0, &v0, &w0, &ud, &vd, &wd);
                rasterizer->fragment (rasterizer, u0, v0, w0, &dst[(first+pre)*bpp/8],
                                      width, ud, vd, wd);
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
#if CTX_STATIC_OPAQUE
                uint8_t *opaque = &rasterizer->opaque[0];
#else
                uint8_t opaque[width];
                memset (opaque, 255, sizeof (opaque));
#endif
                rasterizer->apply_coverage (rasterizer,
                            &dst[((first + pre) * bpp)/8],
                            rasterizer_src,
                            first + pre,
                            opaque,
                            width);
                }
              }
          }
          }
          else if (first == last)
          {
            coverage[last]+=(graystart-(255-grayend));

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
                          rasterizer_src,
                          accumulated_x0,
                          &coverage[accumulated_x0],
                          accumulated_x1-accumulated_x0+1);
             }
   }
}

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
    rasterizer->col_min       = 50000000;
    rasterizer->scan_max      =
    rasterizer->col_max       = -50000000;
  }
  //rasterizer->comp_op       = NULL; // keep comp_op cached 
  //     between rasterizations where rendering attributes are
  //     nonchanging
}



static void
ctx_rasterizer_rasterize_edges2 (CtxRasterizer *rasterizer, const int fill_rule)
{
  rasterizer->pending_edges   =   
  rasterizer->active_edges    =   0;
  CtxGState     *gstate     = &rasterizer->state->gstate;
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

  uint8_t *rasterizer_src = rasterizer->color;

  if (maxx > blit_max_x - 1)
    { maxx = blit_max_x - 1; }

  minx = ctx_maxi (gstate->clip_min_x, minx);
  maxx = ctx_mini (gstate->clip_max_x, maxx);
  minx *= (minx>0);
 

  int pixs = maxx - minx + 1;
  uint8_t _coverage[pixs];
  uint8_t *coverage = &_coverage[0];

  rasterizer->scan_min -= (rasterizer->scan_min % CTX_FULL_AA);
  {
     if (rasterizer->scan_min > scan_start)
       {
          dst += (rasterizer->blit_stride * (rasterizer->scan_min-scan_start) / CTX_FULL_AA);
          scan_start = rasterizer->scan_min;
       }
      scan_end = ctx_mini (rasterizer->scan_max, scan_end);
  }

  if (CTX_UNLIKELY(gstate->clip_min_y * CTX_FULL_AA > scan_start ))
    { 
       dst += (rasterizer->blit_stride * (gstate->clip_min_y * CTX_FULL_AA -scan_start) / CTX_FULL_AA);
       scan_start = gstate->clip_min_y * CTX_FULL_AA; 
    }
  scan_end = ctx_mini (gstate->clip_max_y * CTX_FULL_AA, scan_end);
  if (CTX_UNLIKELY((minx >= maxx) | (scan_start > scan_end) |
      (scan_start > (rasterizer->blit_y + (rasterizer->blit_height-1)) * CTX_FULL_AA) |
      (scan_end < (rasterizer->blit_y) * CTX_FULL_AA)))
  { 
    /* not affecting this rasterizers scanlines */
    return;
  }

  ctx_edge_qsort ((CtxSegment*)& (rasterizer->edge_list.entries[0]), 0, rasterizer->edge_list.count-1);
  rasterizer->scanline = scan_start;

  int allow_direct = !(0 
#if CTX_ENABLE_CLIP
         | ((rasterizer->clip_buffer!=NULL) & (!rasterizer->clip_rectangle))
#endif
#if CTX_ENABLE_SHADOW_BLUR
         | rasterizer->in_shadow
#endif
         );
  for (; rasterizer->scanline <= scan_end;)
    {
      int aa = ctx_rasterizer_feed_edges_full (rasterizer);
      switch (aa)
      {
        case -1:
        rasterizer->scanline += CTX_FULL_AA;
        dst += blit_stride;
        continue;
        case 0:
        { /* the scanline transitions does not contain multiple intersections - each aa segment is a linear ramp */
          ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP2);
          ctx_rasterizer_feed_edges (rasterizer);
          ctx_edge2_insertion_sort ((CtxSegment*)rasterizer->edge_list.entries, rasterizer->edges, rasterizer->active_edges);
    
          memset (coverage, 0, pixs);
          if (allow_direct)
          {
            ctx_rasterizer_generate_coverage_apply2 (rasterizer, minx, maxx, coverage, is_winding, comp);
            ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);
    
            dst += blit_stride;
            continue;
          }
          ctx_rasterizer_generate_coverage_set2 (rasterizer, minx, maxx, coverage, is_winding);
          ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);
          break;
        }
        case 1:
        {
          ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP2);
          ctx_rasterizer_feed_edges (rasterizer);
          ctx_edge2_insertion_sort ((CtxSegment*)rasterizer->edge_list.entries, rasterizer->edges, rasterizer->active_edges);
    
          if (allow_direct)
          { /* can generate with direct rendering to target (we're not using shape cache) */
    
            ctx_rasterizer_generate_coverage_apply (rasterizer, minx, maxx, is_winding, comp);
            ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);
    
            dst += blit_stride;
            continue;
          }
          else
          { /* cheap fully correct AA, to coverage mask / clipping */
            memset (coverage, 0, pixs);
    
            ctx_rasterizer_generate_coverage_set (rasterizer, minx, maxx, coverage, is_winding);
            ctx_rasterizer_increment_edges (rasterizer, CTX_AA_HALFSTEP);
          }
          break;
        }
        default:
        { /* level of oversampling based on lowest steepness edges */
          if (aa > real_aa) aa = real_aa;
          int scanline_increment = 15/aa;
          memset (coverage, 0, pixs);
          uint8_t fraction = 255/aa;
          for (unsigned int i = 0; i < (unsigned)aa; i++)
          {
            ctx_edge2_insertion_sort ((CtxSegment*)rasterizer->edge_list.entries, rasterizer->edges, rasterizer->active_edges);
            ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, is_winding, aa, fraction);
            ctx_rasterizer_increment_edges (rasterizer, scanline_increment);
            ctx_rasterizer_feed_edges (rasterizer);
          }
        }
      }
  
      ctx_coverage_post_process (rasterizer, minx, maxx, coverage - minx, NULL, NULL);
      rasterizer->apply_coverage (rasterizer,
                      &dst[(minx * rasterizer->format->bpp) /8],
                      rasterizer_src,
                      minx,
                      coverage,
                      pixs);
      dst += blit_stride;
    }

#if CTX_BLENDING_AND_COMPOSITING
  if (CTX_UNLIKELY((gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OUT) |
      (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_IN) |
      (gstate->compositing_mode == CTX_COMPOSITE_DESTINATION_IN) |
      (gstate->compositing_mode == CTX_COMPOSITE_DESTINATION_ATOP) |
      (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)))
  {
     /* fill in the rest of the blitrect when compositing mode permits it */
     uint8_t nocoverage[rasterizer->blit_width];
     int gscan_start = gstate->clip_min_y * CTX_FULL_AA;
     int gscan_end = gstate->clip_max_y * CTX_FULL_AA;
     memset (nocoverage, 0, sizeof(nocoverage));
     int startx   = gstate->clip_min_x;
     int endx     = gstate->clip_max_x;
     int clipw    = endx-startx + 1;
     uint8_t *dst = ( (uint8_t *) rasterizer->buf);

     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (gscan_start / CTX_FULL_AA);
     for (rasterizer->scanline = gscan_start; rasterizer->scanline < scan_start;)
     {
       rasterizer->apply_coverage (rasterizer,
                       &dst[ (startx * rasterizer->format->bpp) /8],
                       rasterizer_src, 0, nocoverage, clipw);
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
                       rasterizer_src,
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
                       rasterizer_src, 0, nocoverage, endx-maxx);

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
                       rasterizer_src,
                       0,
                       nocoverage, clipw-1);

       rasterizer->scanline += CTX_FULL_AA;
       dst += blit_stride;
     }
#endif
  }
#endif
}


#if CTX_INLINE_FILL_RULE
void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule);
#else

void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule);
#endif


#if CTX_INLINE_FILL_RULE
void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule)
{
  if (fill_rule)
  {
    ctx_rasterizer_rasterize_edges2 (rasterizer, 1);
  }
  else
  {
    ctx_rasterizer_rasterize_edges2 (rasterizer, 0);
  }
}
#else

void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule)
{
    ctx_rasterizer_rasterize_edges2 (rasterizer, fill_rule);
}

#endif



extern const CtxPixelFormatInfo *ctx_pixel_formats;
void CTX_SIMD_SUFFIX(ctx_simd_setup)(void);
void CTX_SIMD_SUFFIX(ctx_simd_setup)(void)
{
  ctx_pixel_formats         = CTX_SIMD_SUFFIX(ctx_pixel_formats);
  ctx_composite_setup       = CTX_SIMD_SUFFIX(ctx_composite_setup);
  ctx_rasterizer_rasterize_edges = CTX_SIMD_SUFFIX(ctx_rasterizer_rasterize_edges);
#if CTX_FAST_FILL_RECT
  ctx_composite_fill_rect   = CTX_SIMD_SUFFIX(ctx_composite_fill_rect);
#if CTX_FAST_STROKE_RECT
  ctx_composite_stroke_rect = CTX_SIMD_SUFFIX(ctx_composite_stroke_rect);
#endif
#endif
}


#endif
#endif
#if CTX_IMPLEMENTATION
#if CTX_RASTERIZER

void
ctx_RGBA8_to_RGB565_BS (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
#if CTX_RGB565_ALPHA
      if (rgba[3]==0)
        { pixel[0] = ctx_565_pack (255, 0, 255, 1); }
      else
#endif
        { pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 1); }
      pixel+=1;
      rgba +=4;
    }
}

void
ctx_RGB565_BS_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      ((uint32_t*)(rgba))[0] = ctx_565_unpack_32 (*pixel, 1);
#if CTX_RGB565_ALPHA
      if ((rgba[0]==255) & (rgba[2] == 255) & (rgba[1]==0))
        { rgba[3] = 0; }
      else
        { rgba[3] = 255; }
#endif
      pixel+=1;
      rgba +=4;
    }
}


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
  if (gradient->n_stops < CTX_MAX_GRADIENT_STOPS-1) //we'll keep overwriting the last when out of stops
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
  CtxSegment entry = {CTX_EDGE, {{0,}},0,0};

  entry.data.s16[0]=rasterizer->inner_x;
  entry.data.s16[1]=rasterizer->inner_y;

  entry.data.s16[2]=x1;
  entry.data.s16[3]=y1;

  ctx_rasterizer_update_inner_point (rasterizer, x1, y1);

  return ctx_edgelist_add_single (&rasterizer->edge_list, (CtxEntry*)&entry);
}

static void ctx_rasterizer_poly_to_edges (CtxRasterizer *rasterizer)
{
  unsigned int count = rasterizer->edge_list.count;
  CtxSegment *entry = (CtxSegment*)&rasterizer->edge_list.entries[0];
  for (unsigned int i = 0; i < count; i++)
    {
      if (entry->data.s16[3] < entry->data.s16[1])
        {
          *entry = ctx_segment_s16 (CTX_EDGE_FLIPPED,
                            entry->data.s16[2], entry->data.s16[3],
                            entry->data.s16[0], entry->data.s16[1]);
        }
      entry++;
    }
}

static inline void ctx_rasterizer_finish_shape (CtxRasterizer *rasterizer)
{
  if (rasterizer->has_shape & (rasterizer->has_prev>0))
    {
      ctx_rasterizer_line_to (rasterizer, rasterizer->first_x, rasterizer->first_y);
      rasterizer->has_prev = 0;
    }
}

//#define MIN_Y -100
//#define MAX_Y 3800
//#define MIN_X -100
//#define MAX_X 3600*10

static inline void ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  int tx = 0, ty = 0;

  rasterizer->first_x  =
  rasterizer->x        = x;
  rasterizer->first_y  =
  rasterizer->y        = y;
  rasterizer->has_prev = -1;
  _ctx_user_to_device_prepped (rasterizer->state, x,y, &tx, &ty);

  tx -= rasterizer->blit_x * CTX_SUBDIV;
  ctx_rasterizer_update_inner_point (rasterizer, tx, ty);
}

static inline void
ctx_rasterizer_line_to_fixed (CtxRasterizer *rasterizer, int x, int y)
{
  rasterizer->has_shape = 1;
  //  XXX we avoid doing this for the cases where we initially have fixed point
  //  need a separate call for doing this at end of beziers and arcs
  //if (done)
  //{
  //     rasterizer->y         = y*1.0/CTX_FULL_AA;
  //     rasterizer->x         = x*1.0/CTX_SUBDIV;
  //}
  int tx = 0, ty = 0;
  _ctx_user_to_device_prepped_fixed (rasterizer->state, x, y, &tx, &ty);
  tx -= rasterizer->blit_x * CTX_SUBDIV;

  ctx_rasterizer_add_point (rasterizer, tx, ty);

  if (CTX_UNLIKELY(rasterizer->has_prev<=0))
    {
      CtxSegment *entry = & ((CtxSegment*)rasterizer->edge_list.entries)[rasterizer->edge_list.count-1];
      entry->code = CTX_NEW_EDGE;
      rasterizer->has_prev = 1;
    }
}

static inline void
ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  int tx = 0, ty = 0;
  rasterizer->has_shape = 1;
  rasterizer->y         = y;
  rasterizer->x         = x;

  _ctx_user_to_device_prepped (rasterizer->state, x, y, &tx, &ty);
  tx -= rasterizer->blit_x * CTX_SUBDIV;

  ctx_rasterizer_add_point (rasterizer, tx, ty);

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
  float dx, dy;
  ctx_bezier_sample (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);
  dx = (sx+ex)/2 - x;
  dy = (sy+ey)/2 - y;

  if ((iteration<1) | ((iteration < 6) & (((long)dx*dx+dy*dy) > tolerance)))
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

#define CTX_FIX_SCALE 1024
#define CTX_FIX_SHIFT 10
                      

CTX_INLINE static int
ctx_lerp_fixed (int v0, int v1, int dx)
{
  return v0 + (((v1-v0) * dx) >> CTX_FIX_SHIFT);
}

CTX_INLINE static int
ctx_bezier_sample_1d_fixed (int x0, int x1, int x2, int x3, int dt)
{
  return ctx_lerp_fixed (
      ctx_lerp_fixed (ctx_lerp_fixed (x0, x1, dt),
                 ctx_lerp_fixed (x1, x2, dt), dt),
      ctx_lerp_fixed (ctx_lerp_fixed (x1, x2, dt),
                 ctx_lerp_fixed (x2, x3, dt), dt), dt);
}

CTX_INLINE static void
ctx_bezier_sample_fixed (int x0, int y0,
                         int x1, int y1,
                         int x2, int y2,
                         int x3, int y3,
                         int dt, int *x, int *y)
{
  *x = ctx_bezier_sample_1d_fixed (x0, x1, x2, x3, dt);
  *y = ctx_bezier_sample_1d_fixed (y0, y1, y2, y3, dt);
}

static inline void
ctx_rasterizer_bezier_divide_fixed (CtxRasterizer *rasterizer,
                                    int ox, int oy,
                                    int x0, int y0,
                                    int x1, int y1,
                                    int x2, int y2,
                                    int sx, int sy,
                                    int ex, int ey,
                                    int s,
                                    int e,
                                    int iteration, long int tolerance)
{
  int t = (s + e) / 2;
  int x, y;

  ctx_bezier_sample_fixed (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);

  int dx, dy;
#if 1
  dx = (sx+ex)/2 - x;
  dy = (sy+ey)/2 - y;
#else
  int lx, ly;
  lx = ctx_lerp_fixed (sx, ex, t);
  ly = ctx_lerp_fixed (sy, ey, t);
  dx = lx - x;
  dy = ly - y;
#endif

  if ((iteration < 2) | ((iteration < 6) & (((long)dx*dx+dy*dy) > tolerance)))
  {
    ctx_rasterizer_bezier_divide_fixed (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                  sx, sy, x, y, s, t, iteration+1, tolerance
                                  );
    ctx_rasterizer_line_to_fixed (rasterizer, x, y);
    ctx_rasterizer_bezier_divide_fixed (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                  x, y, ex, ey, t, e, iteration+1, tolerance
                                  );
  }
}

static inline void
ctx_rasterizer_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2)
{
  float ox = rasterizer->state->x;
  float oy = rasterizer->state->y;

#if 1
  ctx_rasterizer_bezier_divide_fixed (rasterizer,
            (int)(ox * CTX_FIX_SCALE), (int)(oy * CTX_FIX_SCALE), (int)(x0 * CTX_FIX_SCALE), (int)(y0 * CTX_FIX_SCALE),
            (int)(x1 * CTX_FIX_SCALE), (int)(y1 * CTX_FIX_SCALE), (int)(x2 * CTX_FIX_SCALE), (int)(y2 * CTX_FIX_SCALE),
            (int)(ox * CTX_FIX_SCALE), (int)(oy * CTX_FIX_SCALE), (int)(x2 * CTX_FIX_SCALE), (int)(y2 * CTX_FIX_SCALE),
            0, CTX_FIX_SCALE, 0, rasterizer->state->gstate.tolerance_fixed);
#else
  ctx_rasterizer_bezier_divide (rasterizer,
                                ox, oy, x0, y0,
                                x1, y1, x2, y2,
                                ox, oy, x2, y2,
                                0.0f, 1.0f, 0, rasterizer->state->gstate.tolerance);
#endif
  ctx_rasterizer_line_to (rasterizer, x2, y2);
}

static inline void
ctx_rasterizer_rel_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  //if (CTX_UNLIKELY(x == 0.f && y == 0.f))
  //{ return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_move_to (rasterizer, x, y);
}

static inline void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  //if (CTX_UNLIKELY(x== 0.f && y==0.f))
  //  { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_line_to (rasterizer, x, y);
}

static inline void
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

  ctx_rasterizer_set_texture (rasterizer, eid, 0.0f, 0.0f);
#if CTX_ENABLE_CM
  if (!rasterizer->state->gstate.source_fill.texture.buffer->color_managed)
  {
    _ctx_texture_prepare_color_management (rasterizer->state,
    rasterizer->state->gstate.source_fill.texture.buffer);
  }
#endif
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
  CtxGState     *gstate     = &rasterizer->state->gstate;
  unsigned int preserved_count =
          (rasterizer->preserve&(rasterizer->edge_list.count!=0))?
             rasterizer->edge_list.count:1;
  int blit_x = rasterizer->blit_x;
  int blit_y = rasterizer->blit_y;
  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;

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
    rasterizer->col_min  += (rasterizer->shadow_x - gstate->shadow_blur * 3 + 1) * CTX_SUBDIV;
    rasterizer->col_max  += (rasterizer->shadow_x + gstate->shadow_blur * 3 + 1) * CTX_SUBDIV;
  }
#endif

  if (CTX_UNLIKELY(ctx_is_transparent (rasterizer, 0) |
      (rasterizer->scan_min > CTX_FULL_AA * (blit_y + blit_height)) |
      (rasterizer->scan_max < CTX_FULL_AA * blit_y) |
      (rasterizer->col_min > CTX_SUBDIV * (blit_x + blit_width)) |
      (rasterizer->col_max < CTX_SUBDIV * blit_x)))
    {
    }
  else
  {
    ctx_composite_setup (rasterizer);

    rasterizer->state->ink_min_x = ctx_mini (rasterizer->state->ink_min_x, rasterizer->col_min / CTX_SUBDIV);
    rasterizer->state->ink_max_x = ctx_maxi (rasterizer->state->ink_min_x, rasterizer->col_max / CTX_SUBDIV);
    rasterizer->state->ink_min_y = ctx_mini (rasterizer->state->ink_min_y, rasterizer->scan_min / CTX_FULL_AA);
    rasterizer->state->ink_max_y = ctx_maxi (rasterizer->state->ink_max_y, rasterizer->scan_max / CTX_FULL_AA);

#if CTX_FAST_FILL_RECT
  if (rasterizer->edge_list.count == 5)
    {
      CtxSegment *entry0 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[0];
      CtxSegment *entry1 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[1];
      CtxSegment *entry2 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[2];
      CtxSegment *entry3 = &(((CtxSegment*)(rasterizer->edge_list.entries)))[3];


      if (
          (!(gstate->clipped != 0)) &
          (entry0->data.s16[2] == entry1->data.s16[2]) &
          (entry0->data.s16[3] == entry3->data.s16[3]) &
          (entry1->data.s16[3] == entry2->data.s16[3]) &
          (entry2->data.s16[2] == entry3->data.s16[2])
#if CTX_ENABLE_SHADOW_BLUR
           & (!rasterizer->in_shadow)
#endif
         )
       {
         float x0 = entry3->data.s16[2] * (1.0f / CTX_SUBDIV);
         float y0 = entry3->data.s16[3] * (1.0f / CTX_FULL_AA);
         float x1 = entry1->data.s16[2] * (1.0f / CTX_SUBDIV);
         float y1 = entry1->data.s16[3] * (1.0f / CTX_FULL_AA);

         if ((x1 > x0) & (y1 > y0))
         {
           ctx_composite_fill_rect (rasterizer, x0, y0, x1, y1, 255);
           goto done;
         }
       }
    }
#endif

    ctx_rasterizer_finish_shape (rasterizer);

    ctx_rasterizer_poly_to_edges (rasterizer);

    {
    ctx_rasterizer_rasterize_edges (rasterizer, gstate->fill_rule);
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
    rasterizer->col_min  -= (rasterizer->shadow_x - gstate->shadow_blur * 3 + 1) * CTX_SUBDIV;
    rasterizer->col_max  -= (rasterizer->shadow_x + gstate->shadow_blur * 3 + 1) * CTX_SUBDIV;
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

#if CTX_BRAILLE_TEXT
static CtxTermGlyph *
ctx_rasterizer_find_term_glyph (CtxRasterizer *rasterizer, int col, int row)
{
    CtxTermGlyph *glyph = NULL;
    
    for (CtxList *l = rasterizer->glyphs; l; l=l->next)
    {
      glyph = l->data;
      if ((glyph->col == col) &
          (glyph->row == row))
      {
        return glyph;
      }
    }

    glyph = ctx_calloc (sizeof (CtxTermGlyph), 1);
    ctx_list_append (&rasterizer->glyphs, glyph);
    glyph->col = col;
    glyph->row = row;
    return glyph;
}
#endif

static int _ctx_glyph (Ctx *ctx, int glyph_id, int stroke);
static void
ctx_rasterizer_glyph (CtxRasterizer *rasterizer, uint32_t unichar, int stroke)
{
  float tx = rasterizer->state->x;
  float ty = rasterizer->state->y - rasterizer->state->gstate.font_size;
  float tx2 = rasterizer->state->x + rasterizer->state->gstate.font_size;
  float ty2 = rasterizer->state->y + rasterizer->state->gstate.font_size;
  _ctx_user_to_device (rasterizer->state, &tx, &ty);
  _ctx_user_to_device (rasterizer->state, &tx2, &ty2);

  if ((tx2 < rasterizer->blit_x) | (ty2 < rasterizer->blit_y)) return;
  if ((tx  > rasterizer->blit_x + rasterizer->blit_width) |
      (ty  > rasterizer->blit_y + rasterizer->blit_height))
          return;

#if CTX_TERM
#if CTX_BRAILLE_TEXT
  float font_size = 0;
  int ch = 1;
  int cw = 1;

  if (rasterizer->term_glyphs)
  {
    float tx = 0;
    font_size = rasterizer->state->gstate.font_size;

    ch = (int)ctx_term_get_cell_height (rasterizer->backend.ctx);
    cw = (int)ctx_term_get_cell_width (rasterizer->backend.ctx);

    _ctx_user_to_device_distance (rasterizer->state, &tx, &font_size);
  }
  if ((rasterizer->term_glyphs!=0) & (!stroke) &
      (fabsf (font_size - ch) < 0.5f))
  {
    float tx = rasterizer->x;
    float ty = rasterizer->y;
    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    int col = (int)(tx / cw + 1);
    int row = (int)(ty / ch + 1);
    CtxTermGlyph *glyph = ctx_rasterizer_find_term_glyph (rasterizer, col, row);

    glyph->unichar = unichar;
    ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color,
                         &glyph->rgba_fg[0]);
  }
  else
#endif
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
#if CTX_TERM
#if CTX_BRAILLE_TEXT
  float font_size = 0;
  if (rasterizer->term_glyphs)
  {
    float tx = 0;
    font_size = rasterizer->state->gstate.font_size;
    _ctx_user_to_device_distance (rasterizer->state, &tx, &font_size);
  }
  int   ch = (int)ctx_term_get_cell_height (rasterizer->backend.ctx);
  int   cw = (int)ctx_term_get_cell_width (rasterizer->backend.ctx);

  if ((rasterizer->term_glyphs!=0) & (!stroke) &
      (fabsf (font_size - ch) < 0.5f))
  {
    float tx = rasterizer->x;
    float ty = rasterizer->y;
    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    int col = (int)(tx / cw + 1);
    int row = (int)(ty / ch + 1);

    for (int i = 0; string[i]; i++, col++)
    {
      CtxTermGlyph *glyph = ctx_rasterizer_find_term_glyph (rasterizer, col, row);

      glyph->unichar = string[i];
      ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color,
                      glyph->rgba_fg);
    }
  }
  else
#endif
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
  float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
  int full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS;
  full_segments = (int)(factor * radius * CTX_PI * 2 / 4.0f);
  if (full_segments > CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS)
    { full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS; }
  if (full_segments < 42) full_segments = 42;
  float step = CTX_PI*2.0f/full_segments;
  int steps;

  if (end_angle < -30.0f)
    end_angle = -30.0f;
  if (start_angle < -30.0f)
    start_angle = -30.0f;
  if (end_angle > 30.0f)
    end_angle = 30.0f;
  if (start_angle > 30.0f)
    start_angle = 30.0f;

  if (radius <= 0.0001f)
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
      if (anticlockwise)
      steps = (int)((start_angle - end_angle) / (CTX_PI*2) * full_segments);
      else
      steps = (int)((end_angle - start_angle) / (CTX_PI*2) * full_segments);
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
          if (first & (!rasterizer->has_prev))
            { ctx_rasterizer_move_to (rasterizer, xv, yv); }
          else
            { ctx_rasterizer_line_to (rasterizer, xv, yv); }
          first = 0;
        }
    }
  ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                          y + ctx_sinf (end_angle) * radius);
}

static inline void
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

static inline void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                            float cx, float cy,
                            float x,  float y)
{
  ctx_rasterizer_quad_to (rasterizer, cx + rasterizer->x, cy + rasterizer->y,
                          x  + rasterizer->x, y  + rasterizer->y);
}

static void
ctx_rasterizer_rectangle_reverse (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  float width,
                                  float height);

#if CTX_STROKE_1PX

static void
ctx_rasterizer_pset (CtxRasterizer *rasterizer, int x, int y, uint8_t cov)
{
  if ((x <= 0) | (y < 0) | (x >= rasterizer->blit_width) |
      (y >= rasterizer->blit_height))
    { return; }
  uint8_t fg_color[4];
  ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color,
fg_color);

  int blit_stride = rasterizer->blit_stride;
  int pitch = rasterizer->format->bpp / 8;

  uint8_t *dst = ( (uint8_t *) rasterizer->buf) + y * blit_stride + x * pitch;
  rasterizer->apply_coverage (rasterizer, dst, rasterizer->color, x, &cov, 1);
}


static inline void
ctx_rasterizer_stroke_1px_segment (CtxRasterizer *rasterizer,
                                   float x0, float y0,
                                   float x1, float y1)
{
  void (*apply_coverage)(CtxRasterizer *r, uint8_t *dst, uint8_t *src,
                         int x, uint8_t *coverage, unsigned int count) =
      rasterizer->apply_coverage;
  uint8_t *rasterizer_src = rasterizer->color;
  int pitch = rasterizer->format->bpp / 8;
  int blit_stride = rasterizer->blit_stride;

  //x1 += 0.5f;
  y1 += 0.5f;
  //x0 += 0.5f;
  y0 += 0.5f;

  float dxf = (x1 - x0);
  float dyf = (y1 - y0);
  int tx = (int)((x0)* 65536);
  int ty = (int)((y0)* 65536);

  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;

  if (dxf*dxf>dyf*dyf)
  {
    int length = abs((int)dxf);
    int dy = (int)((dyf * 65536)/(length));
    int x = tx >> 16;

    if (dxf < 0.0f)
    {
      ty = (int)((y1)* 65536);
      x = (int)x1; 
      dy *= -1;
    }
    int i = 0;
    int sblit_height = blit_height << 16;

    for (; (i < length) & (x < 0); ++i, ++x, ty += dy);
    for (; (i < length) & (x < blit_width) & ((ty<0) | (ty>=sblit_height+1))
         ; ++i, ++x, ty += dy);

    for (; i < length && x < blit_width && (ty<65536 || (ty>=sblit_height))
         ; ++i, ++x, ty += dy)
    {
      int y = ty>>16;
      int ypos = (ty >> 8) & 0xff;

      ctx_rasterizer_pset (rasterizer, x, y-1, 255-ypos);
      ctx_rasterizer_pset (rasterizer, x, y, ypos);
    }

      {
       for (; (i < length) & (x < blit_width) & ((ty>65536) & (ty<sblit_height))
            ; ++i, ++x, ty += dy)
       {
         uint8_t *dst = ( (uint8_t *) rasterizer->buf)
                        + ((ty>>16)-1) * blit_stride + x * pitch;
         uint8_t ypos = (ty >> 8) & 0xff;
         uint8_t rcov=255-ypos;
         apply_coverage (rasterizer, dst, rasterizer_src, x, &rcov, 1);
         dst += blit_stride;
         apply_coverage (rasterizer, dst, rasterizer_src, x, &ypos, 1);
       }
      }

    {
      int y = ty>>16;
      int ypos = (ty >> 8) & 0xff;
      ctx_rasterizer_pset (rasterizer, x, y-1, 255-ypos);
      ctx_rasterizer_pset (rasterizer, x, y, ypos);
    }

  }
  else
  {
    int length = abs((int)dyf);
    int dx = (int)((dxf * 65536)/(length));
    int y = ty >> 16;

    if (dyf < 0.0f)
    {
      tx = (int)((x1)* 65536);
      y = (int)y1; 
      dx *= -1;
    }
    int i = 0;

    int sblit_width = blit_width << 16;

    for (; (i < length) & (y < 0); ++i, ++y, tx += dx);

    for (; (i < length) & (y < blit_height) & ((tx<0) | (tx>=sblit_width+1))
         ; ++i, ++y, tx += dx);
    for (; (i < length) & (y < blit_height) & ((tx<65536) | (tx>=sblit_width))
         ; ++i, ++y, tx += dx)
    {
      int x = tx>>16;
      int xpos = (tx >> 8) & 0xff;
      ctx_rasterizer_pset (rasterizer, x-1, y, 255-xpos);
      ctx_rasterizer_pset (rasterizer, x, y, xpos);
    }

      {
       for (; (i < length) & (y < blit_height) & ((tx>65536) & (tx<sblit_width))
            ; ++i, ++y, tx += dx)
       {
         int x = tx>>16;
         uint8_t *dst = ( (uint8_t *) rasterizer->buf)
                       + y * blit_stride + (x-1) * pitch;
         int xpos = (tx >> 8) & 0xff;
         uint8_t cov[2]={255-xpos, xpos};
         apply_coverage (rasterizer, dst, rasterizer_src, x, cov, 2);
       }
      }
    //for (; i <= length; ++i, ++y, tx += dx)
    { // better do one too many than one too little
      int x = tx>>16;
      int xpos = (tx >> 8) & 0xff;
      ctx_rasterizer_pset (rasterizer, x-1, y, 255-xpos);
      ctx_rasterizer_pset (rasterizer, x, y, xpos);
    }
  }
}

static inline void
ctx_rasterizer_stroke_1px (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxSegment *temp = (CtxSegment*)rasterizer->edge_list.entries;
  float prev_x = 0.0f;
  float prev_y = 0.0f;
  int start = 0;
  int end = 0;

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
              if (started)
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
          
          ctx_rasterizer_stroke_1px_segment (rasterizer, prev_x, prev_y, x, y);
          prev_x = x;
          prev_y = y;
        }
      end = i-1;
foo:
      start = end+1;
    }
  ctx_rasterizer_reset (rasterizer);
}

#endif

#define CTX_MIN_STROKE_LEN  0.2f

static void
ctx_rasterizer_stroke (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxSource source_backup;
  int count = rasterizer->edge_list.count;
  if (count == 0)
    return;
  int preserved = rasterizer->preserve;
  float factor = ctx_matrix_get_scale (&gstate->transform);
  float line_width = gstate->line_width * factor;
  if (gstate->source_stroke.type != CTX_SOURCE_INHERIT_FILL)
  {
    source_backup = gstate->source_fill;
    gstate->source_fill = gstate->source_stroke;
  }

  rasterizer->comp_op = NULL;
  ctx_composite_setup (rasterizer);

#if CTX_STROKE_1PX
  if ((gstate->line_width * factor <= 0.0f) &
      (gstate->line_width * factor > -10.0f) &
      (rasterizer->format->bpp >= 8))
  {
    ctx_rasterizer_stroke_1px (rasterizer);
    if (preserved)
    {
      rasterizer->preserve = 0;
    }
    else
    {
      rasterizer->edge_list.count = 0;
    }
    if (gstate->source_stroke.type != CTX_SOURCE_INHERIT_FILL)
      gstate->source_fill = source_backup;

    return;
  }
#endif

  CtxSegment temp[count]; /* copy of already built up path's poly line  */
  memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) );
#if CTX_FAST_FILL_RECT
#if CTX_FAST_STROKE_RECT
  if (rasterizer->edge_list.count == 5)
    {
      CtxSegment *entry0 = &((CtxSegment*)rasterizer->edge_list.entries)[0];
      CtxSegment *entry1 = &((CtxSegment*)rasterizer->edge_list.entries)[1];
      CtxSegment *entry2 = &((CtxSegment*)rasterizer->edge_list.entries)[2];
      CtxSegment *entry3 = &((CtxSegment*)rasterizer->edge_list.entries)[3];

      if (!rasterizer->state->gstate.clipped &
          (entry0->data.s16[2] == entry1->data.s16[2]) &
          (entry0->data.s16[3] == entry3->data.s16[3]) &
          (entry1->data.s16[3] == entry2->data.s16[3]) &
          (entry2->data.s16[2] == entry3->data.s16[2])
#if CTX_ENABLE_SHADOW_BLUR
           & !rasterizer->in_shadow
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
#endif
  
    {
    {
      if (line_width < 5.0f)
      {
#if 1
      factor *= 0.95f; /* this hack adjustment makes sharp 1px and 2px strokewidths
      //                 end up sharp without erronious AA; we seem to be off by
      //                 one somewhere else, causing the need for this
      //                 */
      line_width *= 0.95f;
#endif
      }
      ctx_rasterizer_reset (rasterizer); /* then start afresh with our stroked shape  */
      CtxMatrix transform_backup = gstate->transform;
      _ctx_matrix_identity (&gstate->transform);
      _ctx_transform_prime (rasterizer->state);
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float half_width_x = line_width/2;
      float half_width_y = half_width_x;

      if (CTX_UNLIKELY(line_width <= 0.0f))
        { // makes negative width be 1px in user-space; hairline
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
              y = entry->data.s16[3] * 1.0f/ CTX_FULL_AA;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              if (length>CTX_MIN_STROKE_LEN)
                {
                  float recip_length = 1.0f/length;
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
              if (CTX_LIKELY(length>CTX_MIN_STROKE_LEN))
                {
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  // XXX possible miter line-to
             //   ctx_rasterizer_line_to (rasterizer, prev_x-dy+10, prev_y+dx+10);
                  ctx_rasterizer_line_to (rasterizer, x-dy,      y+dx);
              	  prev_x = x;
                  prev_y = y;
                }
              if (CTX_UNLIKELY(entry->code == CTX_NEW_EDGE))
                {
                  x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
                  dx = x - prev_x;
                  dy = y - prev_y;
                  length = ctx_fast_hypotf (dx, dy);
                  recip_length = 1.0f/length;
                  if (CTX_LIKELY(length>CTX_MIN_STROKE_LEN))
                    {
                      dx = dx * recip_length * half_width_x;
                      dy = dy * recip_length * half_width_y;
                      ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                      ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
                    }
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
                          ctx_rasterizer_rectangle_reverse (rasterizer, x - half_width_x, y - half_width_y, half_width_x, half_width_y);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / CTX_FULL_AA;
                      ctx_rasterizer_rectangle_reverse (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
                  has_prev = 1;
                }
              ctx_rasterizer_rectangle_reverse (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
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
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*2, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
                  has_prev = 1;
                }
              ctx_rasterizer_move_to (rasterizer, x, y);
              ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*2, 0, 1);
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
      _ctx_transform_prime (rasterizer->state);
    }
  }
#if CTX_FAST_FILL_RECT
#if CTX_FAST_STROKE_RECT
  done:
#endif
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
  CtxGState *gstate = &rasterizer->state->gstate;
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
   ctx_buffer_destroy (rasterizer->clip_buffer);
  rasterizer->clip_buffer = NULL;
#endif
  gstate->clip_min_x = rasterizer->blit_x;
  gstate->clip_min_y = rasterizer->blit_y;

  gstate->clip_max_x = rasterizer->blit_x + rasterizer->blit_width - 1;
  gstate->clip_max_y = rasterizer->blit_y + rasterizer->blit_height - 1;
}

static void
ctx_rasterizer_clip_apply (CtxRasterizer *rasterizer,
                           CtxSegment    *edges)
{
  unsigned int count = edges[0].data.u32[0];
  CtxGState *gstate = &rasterizer->state->gstate;

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
          prev_x = entry->data.s16[0] / CTX_SUBDIV;
          prev_y = entry->data.s16[1] / CTX_FULL_AA;
          if (prev_x < minx) { minx = prev_x; }
          if (prev_y < miny) { miny = prev_y; }
          if (prev_x > maxx) { maxx = prev_x; }
          if (prev_y > maxy) { maxy = prev_y; }
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / CTX_FULL_AA;
      if (x < minx) { minx = (int)x; }
      if (y < miny) { miny = (int)y; }
      if (x > maxx) { maxx = (int)x; }
      if (y > maxy) { maxy = (int)y; }

      if (i < 6)
      {
        coords[i][0] = x;
        coords[i][1] = y;
      }
    }

#if CTX_ENABLE_CLIP

  if (((rasterizer->clip_rectangle==1) | (!rasterizer->clip_buffer))
      )
  {
    if (count == 5)
    {
      if ((coords[0][0] == coords[1][0]) &
          (coords[0][1] == coords[4][1]) &
          (coords[0][1] == coords[3][1]) &
          (coords[1][1] == coords[2][1]) &
          (coords[3][0] == coords[4][0])
          )
      {
#if 0
        printf ("%d,%d %dx%d\n", minx, miny,
                                       maxx-minx+1, maxy-miny+1);
#endif

         gstate->clip_min_x =
            ctx_maxi (minx, gstate->clip_min_x);
         gstate->clip_min_y =
            ctx_maxi (miny, gstate->clip_min_y);
         gstate->clip_max_x =
            ctx_mini (maxx, gstate->clip_max_x);
         gstate->clip_max_y =
            ctx_mini (maxy, gstate->clip_max_y);

         rasterizer->clip_rectangle = 1;

#if 0
         if (!rasterizer->clip_buffer)
           rasterizer->clip_buffer = ctx_buffer_new (blit_width,
                                                     blit_height,
                                                     CTX_CLIP_FORMAT);

         memset (rasterizer->clip_buffer->data, 0, blit_width * blit_height);
         int i = 0;
         for (int y = gstate->clip_min_y;
                  y <= gstate->clip_max_y;
                  y++)
         for (int x = gstate->clip_min_x;
                  x <= gstate->clip_max_x;
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

  if ((minx == maxx) | (miny == maxy)) // XXX : reset hack
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

  float prev_x = 0;
  float prev_y = 0;

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
    ctx_destroy (ctx);
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
    for (; (i < count) & maybe_rect & (!next_stage); i++)
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
    for (; (i < count) & (!next_stage) & maybe_rect; i++)
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
    for (; (i < count) & maybe_rect & (!next_stage); i++)
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
    for (; (i < count) & maybe_rect & (!next_stage); i++)
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
   ctx_buffer_destroy (clip_buffer);
#else
  if (coords[0][0]){};
#endif
  
  gstate->clip_min_x = ctx_maxi (minx,
                                         gstate->clip_min_x);
  gstate->clip_min_y = ctx_maxi (miny,
                                         gstate->clip_min_y);
  gstate->clip_max_x = ctx_mini (maxx,
                                         gstate->clip_max_x);
  gstate->clip_max_y = ctx_mini (maxy,
                                         gstate->clip_max_y);
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
      ctx_state_set_blob (rasterizer->state, SQZ_clip, (uint8_t*)temp, sizeof(temp));
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
ctx_rasterizer_rectangle_reverse (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  float width,
                                  float height)
{
  ctx_rasterizer_move_to (rasterizer, x, y);
  ctx_rasterizer_rel_line_to (rasterizer, 0, height);
  ctx_rasterizer_rel_line_to (rasterizer, width, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, -height);
  ctx_rasterizer_rel_line_to (rasterizer, -width, 0);
  //ctx_rasterizer_rel_line_to (rasterizer, width/2, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

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
  return ctx_expf (-0.5f * a * a);
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
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  CtxEntry save_command = ctx_void(CTX_SAVE);
  int no = 0;
  for (no = 0; rasterizer->group[no] && no < CTX_GROUP_MAX; no++);
  no--;

  if (no < 0)
    return;

  Ctx *ctx = rasterizer->backend.ctx;

  CtxCompositingMode comp = gstate->compositing_mode;
  CtxBlend blend = gstate->blend_mode;
  CtxExtend extend = gstate->extend;
  float global_alpha = gstate->global_alpha_f;
  // fetch compositing, blending, global alpha
  ctx_rasterizer_process (ctx, (CtxCommand*)&restore_command);
  ctx_rasterizer_process (ctx, (CtxCommand*)&save_command);
  CtxEntry set_state[4]=
  {
    ctx_u32 (CTX_COMPOSITING_MODE, comp,  0),
    ctx_u32 (CTX_BLEND_MODE,       blend, 0),
    ctx_u32 (CTX_EXTEND,          extend, 0),
    ctx_f  (CTX_GLOBAL_ALPHA,     global_alpha, 0.0)
  };
  ctx_rasterizer_process (ctx, (CtxCommand*)&set_state[0]);
  ctx_rasterizer_process (ctx, (CtxCommand*)&set_state[1]);
  ctx_rasterizer_process (ctx, (CtxCommand*)&set_state[2]);
  ctx_rasterizer_process (ctx, (CtxCommand*)&set_state[3]);
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
     int   eid_len = ctx_strlen (eid);

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
  ctx_buffer_destroy (rasterizer->group[no]);
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
  if (ctx_get_color (rasterizer->backend.ctx, SQZ_shadowColor, &color) == 0)
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
  //ctx_free (kernel);
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
  if (ctx_get_color (rasterizer->backend.ctx, SQZ_shadowColor, &color) == 0)
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
  if (ctx_get_color (rasterizer->backend.ctx, SQZ_shadowColor, &color) == 0)
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
  count = CTX_MIN(count, CTX_MAX_DASHES);
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
          ctx_set_color (rasterizer->backend.ctx, SQZ_shadowColor, color);
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
        if (ctx->bail) break;
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_LINE_TO:
        if (ctx->bail) break;
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_MOVE_TO:
        if (ctx->bail) break;
        ctx_rasterizer_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_MOVE_TO:
        if (ctx->bail) break;
        ctx_rasterizer_rel_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_CURVE_TO:
        if (ctx->bail) break;
        ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
                                 c->c.x1, c->c.y1,
                                 c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        if (ctx->bail) break;
        ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
                                     c->c.x1, c->c.y1,
                                     c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        if (ctx->bail) break;
        ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        if (ctx->bail) break;
        ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_ARC:
        if (ctx->bail) break;
        ctx_rasterizer_arc (rasterizer, c->arc.x, c->arc.y, c->arc.radius, c->arc.angle1, c->arc.angle2, (int)c->arc.direction);
        break;
      case CTX_RECTANGLE:
        if (ctx->bail) break;
        ctx_rasterizer_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                  c->rectangle.width, c->rectangle.height);
        break;
      case CTX_ROUND_RECTANGLE:
        if (ctx->bail) break;
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
        rasterizer->gradient_cache_valid = 0;
        rasterizer->comp_op = NULL;
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_state_gradient_clear_stops (state);
        rasterizer->gradient_cache_valid = 0;
        rasterizer->comp_op = NULL;
        break;
#endif
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_COLOR:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
      case CTX_EXTEND:
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
        for (unsigned int i = state->gstate_no?state->gstate_stack[state->gstate_no-1].keydb_pos:0;
             i < state->gstate.keydb_pos; i++)
        {
          if (state->keydb[i].key == SQZ_clip)
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
        /* FALLTHROUGH */
      case CTX_SAVE:
        rasterizer->comp_op = NULL;
        ctx_interpret_transforms (state, entry, NULL);
        if (clear_clip)
        {
          ctx_rasterizer_clip_reset (rasterizer);
          for (unsigned int i = state->gstate_no?state->gstate_stack[state->gstate_no-1].keydb_pos:0;
             i < state->gstate.keydb_pos; i++)
        {
          if (state->keydb[i].key == SQZ_clip)
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
          if (rasterizer->edge_list.count == 0)break;
#if CTX_ENABLE_SHADOW_BLUR
        if ((state->gstate.shadow_blur > 0.0f) & (!rasterizer->in_text))
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
      _ctx_transform_prime (state);
      ctx_rasterizer_reset (rasterizer); /* for dashing we create
                                            a dashed path to stroke */
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      //float pos = 0.0;

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
                //pos += length;
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
        _ctx_transform_prime (state);
        }
        ctx_rasterizer_stroke (rasterizer);
        }
        ctx_rasterizer_reset (rasterizer);

        break;
      case CTX_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_TEXT:
        if (ctx->bail)
        {
          _ctx_text (rasterizer->backend.ctx, ctx_arg_string(), 0, 0);
          break;
        }

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
        if (ctx->bail) break;
        {
        uint32_t unichar = entry[0].data.u32[0];
        uint32_t stroke = unichar &  ((uint32_t)1<<31);
        if (stroke) unichar -= stroke;
        ctx_rasterizer_glyph (rasterizer, entry[0].data.u32[0], stroke);
        }
        break;
      case CTX_PAINT:
        // XXX simplify this with a special case
        ctx_rasterizer_rectangle (rasterizer, -1000.0, -1000.0, 11000, 11000);
        ctx_rasterizer_fill (rasterizer);
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_FILL:
        if (!ctx->bail)
        {
          if (rasterizer->edge_list.count == 0)break;
#if CTX_ENABLE_SHADOW_BLUR
        if ((state->gstate.shadow_blur > 0.0f) & (!rasterizer->in_text))
          ctx_rasterizer_shadow_fill (rasterizer);
#endif
        ctx_rasterizer_fill (rasterizer);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_START_FRAME:
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


//static CtxFont *ctx_fonts;
void
ctx_rasterizer_deinit (CtxRasterizer *rasterizer)
{
  //rasterizer->fonts = ctx_fonts;
  ctx_drawlist_deinit (&rasterizer->edge_list);
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
  {
    ctx_buffer_destroy (rasterizer->clip_buffer);
    rasterizer->clip_buffer = NULL;
  }
#endif
}

void
ctx_rasterizer_destroy (CtxRasterizer *rasterizer)
{
  ctx_rasterizer_deinit (rasterizer);
  ctx_free (rasterizer);
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
#if CTX_TERMINAL_EVENTS
  if (ctx_backend_is_tiled (ctx))
  {
     CtxTiled *fb = (CtxTiled*)(ctx->backend);
     fb->antialias = antialias;
#if CTX_THREADS
     for (int i = 0; i < _ctx_max_threads; i++)
#else
     int i = 0;
#endif
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
  if ((antialias == CTX_ANTIALIAS_DEFAULT)|
      (antialias == CTX_ANTIALIAS_FAST))
    ((CtxRasterizer*)(ctx->backend))->fast_aa = 1;
}

CtxRasterizer *
ctx_rasterizer_init (CtxRasterizer *rasterizer, Ctx *ctx, Ctx *texture_source, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format, CtxAntialias antialias)
{
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
    ctx_buffer_destroy (rasterizer->clip_buffer);
#endif
  if (rasterizer->edge_list.size)
    ctx_drawlist_deinit (&rasterizer->edge_list);
  memset (rasterizer, 0, sizeof (CtxRasterizer));
  CtxBackend *backend = (CtxBackend*)rasterizer;
  backend->type = CTX_BACKEND_RASTERIZER;
  backend->process = ctx_rasterizer_process;
  backend->destroy = (CtxDestroyNotify)ctx_rasterizer_destroy;
  backend->ctx     = ctx;
  rasterizer->edge_list.flags |= CTX_DRAWLIST_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->texture_source = texture_source?texture_source:ctx;

  rasterizer->aa          = _ctx_antialias_to_aa (antialias);
  rasterizer->fast_aa = ((antialias == CTX_ANTIALIAS_DEFAULT)|(antialias == CTX_ANTIALIAS_FAST));
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

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
  rasterizer->gradient_cache_elements = CTX_GRADIENT_CACHE_ELEMENTS;
  rasterizer->gradient_cache_valid = 0;
#endif
#endif

#if CTX_STATIC_OPAQUE
  memset (rasterizer->opaque, 255, sizeof (rasterizer->opaque));
#endif

  return rasterizer;
}

void
ctx_rasterizer_reinit (Ctx *ctx,
                       void *fb,
                       int x,
                       int y,
                       int width,
                       int height,
                       int stride,
                       CtxPixelFormat pixel_format)
{
  CtxBackend *backend = (CtxBackend*)ctx_get_backend (ctx);
  CtxRasterizer *rasterizer = (CtxRasterizer*)backend;
  if (!backend) return;
#if 0
  // this is a more proper reinit than the below, which should be a lot faster..
  ctx_rasterizer_init (rasterizer, ctx, rasterizer->texture_source, &ctx->state, fb, x, y, width, height, stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
#else

  ctx_state_init (rasterizer->state);
  rasterizer->buf         = fb;
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
  rasterizer->gradient_cache_valid = 0;

  if (pixel_format == CTX_FORMAT_BGRA8)
  {
    pixel_format = CTX_FORMAT_RGBA8;
    rasterizer->swap_red_green = 1;
  }

  rasterizer->format = ctx_pixel_format_info (pixel_format);
#endif
}

Ctx *
ctx_new_for_buffer (CtxBuffer *buffer)
{
  Ctx *ctx = _ctx_new_drawlist (buffer->width, buffer->height);
  ctx_set_backend (ctx,
                    ctx_rasterizer_init ( (CtxRasterizer *) ctx_calloc (sizeof (CtxRasterizer), 1),
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
  CtxState    *state    = (CtxState *) ctx_malloc (sizeof (CtxState) );
  CtxRasterizer *rasterizer = (CtxRasterizer *) ctx_malloc (sizeof (CtxBackend) );
  ctx_rasterizer_init (rasterizer, state, data, x, y, width, height,
                       stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
}
#endif

#else

#endif


static void
ctx_state_gradient_clear_stops (CtxState *state)
{
  state->gradient.n_stops = 0;
}


#ifndef __clang__
#if CTX_RASTERIZER_O3
#pragma GCC pop_options
#endif
#if CTX_RASTERIZER_O2
#pragma GCC pop_options
#endif
#endif
/****  end of engine ****/
