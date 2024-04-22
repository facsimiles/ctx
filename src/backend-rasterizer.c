

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
#define CTX_AA_HALFSTEP    ((CTX_FULL_AA/2)+1)
#define CTX_AA_HALFSTEP2   (CTX_FULL_AA/2)


#define CTX_MAGIC_OFFSET  1 // without this we get scanline glitches

static inline void ctx_rasterizer_discard_edges (CtxRasterizer *rasterizer)
{
  int scanline = rasterizer->scanline + CTX_MAGIC_OFFSET;
  int next_scanline = scanline + CTX_FULL_AA;
  CtxSegment *segments = &((CtxSegment*)(rasterizer->edge_list.entries))[0];
  int *edges = rasterizer->edges;
  int ending_edges = 0;
  unsigned int active_edges = rasterizer->active_edges;
  for (unsigned int i = 0; i < active_edges; i++)
    {
      CtxSegment *segment = segments + edges[i];
      int edge_end = segment->y1;
      if (edge_end < scanline)
        {
#if 0
	  for (unsigned int j = i; j < active_edges -1; j++)
            rasterizer->edges[j] = rasterizer->edges[j+1];
#else
	  rasterizer->edges[i] = rasterizer->edges[active_edges-1];
#endif
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
      int edge_end = ((CtxSegment*)(rasterizer->edge_list.entries))[rasterizer->edges[CTX_MAX_EDGES-1-i]].y1;
      ending_edges += (edge_end < next_scanline);
    }
  rasterizer->ending_edges = ending_edges;
}

CTX_INLINE static void ctx_rasterizer_increment_edges (CtxRasterizer *rasterizer, int count)
{
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

CTX_INLINE static void ctx_rasterizer_sort_active_edges (CtxRasterizer *rasterizer)
{
  CtxSegment *segments= (CtxSegment*)rasterizer->edge_list.entries;
  int *entries = rasterizer->edges;
  unsigned int count = rasterizer->active_edges;

  for(unsigned int i=1; i<count; i++)
   {
     int temp = entries[i];
     int tv = segments[temp].val;
     int j = i-1;
     while (j >= 0 && tv - segments[entries[j]].val < 0)
     {
       entries[j+1] = entries[j];
       j--;
     }
     entries[j+1] = temp;
   }
}

CTX_INLINE static void ctx_rasterizer_feed_pending_edges (CtxRasterizer *rasterizer)
{
  CtxSegment *__restrict__ entries = (CtxSegment*)&rasterizer->edge_list.entries[0];
  int *edges = rasterizer->edges;
  unsigned int pending_edges   = rasterizer->pending_edges;
  int scanline = rasterizer->scanline + CTX_MAGIC_OFFSET;
  int active_edges = rasterizer->active_edges;
  for (unsigned int i = 0; i < pending_edges; i++)
    {
      if ((entries[edges[CTX_MAX_EDGES-1-i]].y0 <= scanline) &
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

// makes us up-to date with ready to render rasterizer->scanline
inline static int ctx_rasterizer_feed_edges_full (CtxRasterizer *rasterizer,
		                                  int with_shadow,
						  float blur_radius)
{
  int miny;
  const int max_vaa = rasterizer->aa;
  ctx_rasterizer_feed_pending_edges (rasterizer);
  CtxSegment *__restrict__ entries = (CtxSegment*)&rasterizer->edge_list.entries[0];
  int *edges = rasterizer->edges;
  unsigned int pending_edges   = rasterizer->pending_edges;
  int scanline = rasterizer->scanline + CTX_MAGIC_OFFSET;

  int active_edges = rasterizer->active_edges;
  int horizontal_edges = 0;

  if (with_shadow)
  {
  int shadow_active_edges = rasterizer->shadow_active_edges;
  int *edges = rasterizer->shadow_edges;
  int blur_scanline_start = scanline - CTX_FULL_AA * (int)blur_radius;
  int next_scanline = scanline + CTX_FULL_AA * (int)blur_radius;
  unsigned int edge_pos = rasterizer->shadow_edge_pos;
  unsigned int edge_count = rasterizer->edge_list.count;
  for (int i = 0; i < shadow_active_edges;i++)
  {
    if (entries[edges[i]].y1 < blur_scanline_start)
    {
       edges[i]=edges[shadow_active_edges-1];
       shadow_active_edges--;
       i--;
    }
  }

  while ((edge_pos < edge_count &&
         (miny=entries[edge_pos].y0)  <= next_scanline))
  {
      int y1 = entries[edge_pos].y1;
      if ((shadow_active_edges < CTX_MAX_EDGES-2) &
        (y1 >= blur_scanline_start))
        {
          edges[shadow_active_edges++] = edge_pos;
        }
      edge_pos++;
  }
  rasterizer->shadow_edge_pos     = edge_pos;
  rasterizer->shadow_active_edges = shadow_active_edges;
  }


#if CTX_SCANBIN
   int scan = scanline / CTX_FULL_AA;
   int count = rasterizer->scan_bin_count[scan];
   if (count)
   for (int i = 0; i < count; i++)
   {
       int edge_pos = rasterizer->scan_bins[scan][i];
       miny = entries[edge_pos].y0;
#else
  int next_scanline = scanline + CTX_FULL_AA;
  unsigned int edge_pos = rasterizer->edge_pos;
  unsigned int edge_count = rasterizer->edge_list.count;
  while ((edge_pos < edge_count &&
         (miny=entries[edge_pos].y0)  <= next_scanline))
  {
#endif
      int y1 = entries[edge_pos].y1;
      if ((active_edges < CTX_MAX_EDGES-2) &
        (y1 >= scanline))
        {
          int dy = (y1 - miny);
          if (dy)
            {
              int yd = (scanline + CTX_AA_HALFSTEP2) - miny;
              unsigned int index = edges[active_edges] = edge_pos;
              int x0 = entries[index].x0;
              int x1 = entries[index].x1;
              int dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
              entries[index].delta = dx_dy;
              entries[index].val = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER + (yd * dx_dy);

              {
	        dx_dy = abs(dx_dy);

#if 0
#define CTX_RASTERIZER_AA_SLOPE_LIMIT3           ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*1.3)/CTX_SUBDIV/15/1024)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT5           ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*14)/CTX_SUBDIV/15/1024)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT15          ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*15)/CTX_SUBDIV/15/1024)
#else
#if 0
#define CTX_RASTERIZER_AA_SLOPE_LIMIT3           ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER)/CTX_SUBDIV/15/1024)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT5           ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*3)/CTX_SUBDIV/15/1024)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT15          ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*5)/CTX_SUBDIV/15/1024)
#else
#define CTX_RASTERIZER_AA_SLOPE_LIMIT3           ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*0.95)/CTX_SUBDIV/15/1024)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT5           ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*6.5)/CTX_SUBDIV/15/1024)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT15          ((65536*CTX_RASTERIZER_EDGE_MULTIPLIER*10.5)/CTX_SUBDIV/15/1024)
#endif
#endif

		int aa = 0;
		if (max_vaa > 5)
		aa = (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT3) 
	           +  (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT5) 
	           +  (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT15);
		else
		aa = (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT3) 
	           +  (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT5) * (max_vaa>3);
		
                rasterizer->scan_aa[aa]++;
	        entries[index].aa = aa;
	      }

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
	    {
	      horizontal_edges++;
	    }
        }
#if CTX_SCANBIN
#else
      edge_pos++;
#endif
  }
#if CTX_SCANBIN==0
    rasterizer->edge_pos         = edge_pos;
#endif
    rasterizer->active_edges     = active_edges;
    rasterizer->pending_edges    = pending_edges;
    if (active_edges + pending_edges == 0)
      return -1;

    if (rasterizer->ending_edges|pending_edges|horizontal_edges)
    {
      const unsigned int *scan_aa = rasterizer->scan_aa;
      int aa = scan_aa[3]?15:scan_aa[2]?5:3;
      return aa;
      //return ctx_mini(aa, rasterizer->aa);
    }
    return 0;
}

static inline void ctx_coverage_post_process (CtxRasterizer *rasterizer, const unsigned int minx, const unsigned int maxx, uint8_t *coverage, int *first_col, int *last_col)
{
#if CTX_ENABLE_CLIP
  if (CTX_UNLIKELY((rasterizer->clip_buffer!=NULL) &  (!rasterizer->clip_rectangle)))
  {
  int scanline     = rasterizer->scanline - CTX_FULL_AA; // we do the
                                                 // post process after
                                                 // coverage generation icnrement
    /* perhaps not working right for clear? */
    int y = scanline / CTX_FULL_AA;
    uint8_t *clip_line = &((uint8_t*)(rasterizer->clip_buffer->data))[rasterizer->blit_width*y];
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

#define UPDATE_PARITY \
        if (scanline!=segment->y0-1)\
        { \
          if (is_winding)\
             parity = parity + -1+2*(segment->code == CTX_EDGE_FLIPPED);\
          else\
             parity = 1-parity; \
        }


CTX_INLINE static void
ctx_rasterizer_generate_coverage (CtxRasterizer *rasterizer,
                                  int            minx,
                                  int            maxx,
                                  uint8_t       *coverage,
                                  int            is_winding,
                                  const uint8_t  aa_factor,
                                  const uint8_t  fraction,
				  int *ret_c0,
				  int *ret_c1
				  )
{
  CtxSegment *entries      = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int        *edges        = rasterizer->edges;
  int         scanline     = rasterizer->scanline;
  int         active_edges = rasterizer->active_edges;
  int         parity       = 0;
  int         c0 = *ret_c0;
  int         c1 = *ret_c1;
  coverage -= minx;
  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment *segment = &entries[edges[t]];
      UPDATE_PARITY;

      if (parity)
        {
          CtxSegment *next_segment = &entries[edges[t+1]];
          const int x0  = segment->val;
          const int x1  = next_segment->val;
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (first < minx)
          { 
            first = minx;
            graystart=0;
          }
          if (last > maxx)
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
	  c0 = ctx_mini(first, c0);
	  c1 = ctx_maxi(last, c1);
        }
   }
  *ret_c0 = c0;
  *ret_c1 = c1;
}

static inline float ctx_p_line_sq_dist (float x, float y, float x1, float y1, float x2, float y2) {
  float A = x - x1;
  float B = y - y1;
  float C = x2 - x1;
  float D = y2 - y1;

  float dot = A * C + B * D;
  float len_sq = C * C + D * D;
  float param = -1.0f;
  if (len_sq != 0.0f) //in case of 0 length line
      param = dot / len_sq;

  float xx, yy;

  if (param < 0.0f) {
    xx = x1;
    yy = y1;
  }
  else if (param > 1.0f) {
    xx = x2;
    yy = y2;
  }
  else {
    xx = x1 + param * C;
    yy = y1 + param * D;
  }

  float dx = x - xx;
  float dy = y - yy;
  return dx * dx + dy * dy;
}

static inline float dist_to_edge_sq (int u, int v, CtxSegment *__restrict__ entries, int edge_no, float blur_rad)
{
  CtxSegment *segment = &entries[edge_no];
  float y0 = segment->y0;
  float y1 = segment->y1;

  float x0 = segment->x0 * (1.0f * CTX_FULL_AA / CTX_SUBDIV );
  float x1 = segment->x1 * (1.0f * CTX_FULL_AA / CTX_SUBDIV );
  return ctx_p_line_sq_dist (u, v, x0, y0, x1, y1);
}

static inline float dist_to_edge (int u, int v, CtxSegment *__restrict__ entries, int edge_no, float blur_rad)
{
  return ctx_sqrtf_fast (dist_to_edge_sq(u,v,entries,edge_no,blur_rad));
}

static inline float smin_exp( float a, float b, float k )
{
    k *= 1.0;
    float r = exp2(-a/k) + exp2(-b/k);
    return -k*log2(r);
}

static inline float smin_cubic( float a, float b, float k )
{
  k *= 4.0f;
  float h = k-ctx_fabsf(a-b);
  h = (h * (h>0))/k;
  return ctx_minf(a,b) - h*h*k*0.25f;
}

static CTX_INLINE float ctx_sdf_f (CtxSegment *entries, int u, int v, int parity, int edge_count, float blur, int *edges)
{
  float min_dist = 2048 * 2048 * 15 * 15;

  for (int j = 0; j < edge_count; j++)
  {
#if CTX_RASTERIZER_BLUR_FUDGE
     float sq_dist = dist_to_edge(u, v, entries, edges[j], blur);
     min_dist = smin_cubic(min_dist,sq_dist, blur/2);
#else
     float sq_dist = dist_to_edge_sq(u, v, entries, edges[j], blur);
     min_dist = ctx_minf(min_dist, sq_dist);
#endif
  }

#if CTX_RASTERIZER_BLUR_FUDGE==0
  min_dist = ctx_sqrtf_fast (min_dist);
#endif

  if (parity)
    return min_dist;
  else
    return -min_dist;
}
static inline float ctx_erf2(float x)
{
  #define CTX_2_SQRTPI     1.12837916709551257390  /* 2/sqrt(pi) */
  x = x * CTX_2_SQRTPI;
  float xx = x * x;
  x = x + (0.24295f + (0.03395f + 0.0104f * xx) * xx) * (x * xx);
  return x * ctx_invsqrtf_fast (1.0 + x * x);
}
static inline float ctx_erf(float x)
{
  int s = (x>=0.0f) + (-1 * (x<0.0f));
  float a = x * s;
  x = 1.0f + (0.278393f + (0.230389f + 0.078108f * (a * a)) * a) * a;
  x *= x;
  return s - s / (x * x);
}

static inline uint8_t gaussian_approximation(float x) {
   x = ctx_erf(x);
   x+= 0.5f;
   if (x > 1.0f) return 255;
   if (x < 0.0f) return 0;
  return x * 255.0f;
}


inline static void
ctx_rasterizer_generate_sdf (CtxRasterizer *rasterizer,
                                       const int      minx,
                                       const int      maxx,
                                       uint8_t       *coverage,
                                       const int      is_winding,
				       float          blur)
{
  CtxSegment *entries = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int *edges  = rasterizer->edges;
  int active_edges    = rasterizer->active_edges;
  int *shadow_edges  = rasterizer->shadow_edges;
  int shadow_active_edges    = rasterizer->shadow_active_edges;
  int scanline        = rasterizer->scanline;
  int parity        = 0;
  float inv_blur = 1.0/(blur * CTX_FULL_AA);
  const int skip_len = blur / 2 + 1;
  // how far ahead we jump looking for
			  // same alpha runs - speeding up solid/blank and

  coverage -= minx;


  int c0 = maxx;
  int c1 = minx;

  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment   *segment = &entries[edges[t]];
      UPDATE_PARITY;

      CtxSegment   *next_segment = &entries[edges[t+1]];
      int x0        = segment->val;
      const int x1        = next_segment->val;

      int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
      int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
      int first     = graystart >> 8;
      int last      = grayend   >> 8;

      if (first < minx)
        first = minx;
      if (last > maxx)
        last = maxx;

      if (first <= last)
      {
        int u = x0 * 15 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);

#if 0
#define COMPUTE_SDF(u,v) \
          blur_map[ctx_sdf (entries, (u), (v), parity,\
    		      shadow_active_edges, blur, shadow_edges)]
#else
#define COMPUTE_SDF(u,v) \
	(gaussian_approximation(ctx_sdf_f(entries,(u),(v), parity, shadow_active_edges, blur, shadow_edges) * inv_blur))
#endif

        int i;
        int prev = -1;
        for (i = first; i <= last; i++)
        {
          coverage[i] = COMPUTE_SDF(u,scanline);

          if (prev == coverage[i])
          {
    	    if (last-i > skip_len
               	&& COMPUTE_SDF(u+15*skip_len, scanline) == prev
                && COMPUTE_SDF(u+15*skip_len/2, scanline) == prev)
    	    {
    	      for (int j = 1; j < skip_len; j++)
    	        coverage[i+j] = prev;
    	      u += 15 * skip_len;
    	      i += (skip_len-1);
    	      continue;
    	    }
          }
          prev = coverage[i];
          u += 15;
        }
      }
      c0 = ctx_mini (c0, first);
      c1 = ctx_maxi (c1, last);
   }

  parity = 0;
   
  {
     int i = minx;

  int prev = -1;
  for (; i < c0; i++)
  {
     coverage[i] = COMPUTE_SDF(i*15, scanline);
     if (c0-i > skip_len &&
         COMPUTE_SDF((i+skip_len)*15, scanline) == prev)
     {
	for (int j = 1; j < skip_len; j++)
	  coverage[i+j] = prev;
	i += (skip_len-1);
	continue;
     }
  }
  prev = -1;
  for (int i = c1+1; i < maxx; i++)
  {
     coverage[i] = COMPUTE_SDF(i*15, scanline);
     if (maxx-i > skip_len && COMPUTE_SDF((i+skip_len)*15, scanline) == prev)
     {
	for (int j = 1; j < skip_len; j++)
	  coverage[i+j] = prev;
	i += (skip_len-1);
	continue;
     }
  }
  }
}


inline static void
ctx_rasterizer_generate_coverage_grads (CtxRasterizer *rasterizer,
                                            const int      minx,
                                            const int      maxx,
                                            uint8_t       *coverage,
                                            const int      is_winding,
				 	    int           *c0_ret,
				 	    int           *c1_ret)
{
  CtxSegment *entries = (CtxSegment*)(&rasterizer->edge_list.entries[0]);
  int *edges  = rasterizer->edges;
  int scanline        = rasterizer->scanline;
  int active_edges    = rasterizer->active_edges;
  int parity        = 0;

  coverage -= minx;

  const int minx_ = minx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;
  const int maxx_ = maxx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;

  int c0 = maxx;
  int c1 = minx;

  for (int t = 0; t < active_edges -1;t++)
    {
      CtxSegment   *segment = &entries[edges[t]];
      UPDATE_PARITY;

       if (parity)
        {
          CtxSegment   *next_segment = &entries[edges[t+1]];
          const int x0        = segment->val;
          const int x1        = next_segment->val;

          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart >> 8;
          int last      = grayend   >> 8;

          if (first < minx)
          { 
            first = minx;
            graystart=0;
          }
          if (last > maxx)
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
	      c0 = ctx_mini(first, c0);
            }
            else
            {
              const int delta0    = segment->delta;
              int x0_start = x0 - delta0 * CTX_AA_HALFSTEP2;
              int x0_end   = x0 + delta0 * CTX_AA_HALFSTEP;
              unsigned int u0x0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x0_start, x0_end)));
              unsigned int u1x0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x0_start, x0_end)));

              int us = u0x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);

              int mod = ((u0x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255) *
                         (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255);
              int sum = ((u1x0-u0x0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV)/255);

              int recip = (65535)/sum;
	      int a = mod * recip;
	      recip *= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV;
	      c0 = ctx_mini(us, c0);
              for (unsigned int u = u0x0; u < u1x0; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
              {
                coverage[us ++] += a>>16;
		a += recip;
              }
              pre = (us-1)-first+1;
            }
  
            if (next_segment->aa == 0)
            {
               coverage[last] += grayend;
	       c1 = ctx_maxi(last, c1);
            }
            else
            {
              const int delta1    = next_segment->delta;
              int x1_start = x1 - delta1 * CTX_AA_HALFSTEP2;
              int x1_end   = x1 + delta1 * CTX_AA_HALFSTEP;
              unsigned int u0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x1_start, x1_end)));
              unsigned int u1 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x1_start, x1_end)));

              int us = u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);
              int mod = ((((u0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255)) *
                    (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255));
              int sum = ((u1-u0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV)/255);
              int recip = (65535) / sum;
	      int a = (65536 * 255) - mod * recip;
	      recip *= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV;
              post = last-us;
              for (unsigned int u = u0; u < u1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)
              {
                coverage[us ++] += (a>>16);
		a -= recip;
              }
	      c1 = ctx_maxi(us, c1);
            }
	    last-=post;
            for (int i = first + pre; i <= last; i++)
              coverage[i] = 255;
          }
          else if (first == last)
          {
            coverage[last]+=(graystart-(grayend^255));
	    c0 = ctx_mini(first, c0);
	    c1 = ctx_maxi(last, c1);
          }
        }
   }

  *c0_ret = c0;
  *c1_ret = c1;
}

#define CTX_RASTERIZER_MAX_EMPTIES  16
#define CTX_RASTERIZER_MAX_SOLID    16

inline static void
ctx_rasterizer_apply_grads_generic (CtxRasterizer *rasterizer,
                                                     const int      minx,
                                                     const int      maxx,
                                                     uint8_t       *coverage,
                                                     const int      is_winding,
			                             ctx_apply_coverage_fun apply_coverage)
{
#define CTX_APPLY_GRAD_A \
  CtxSegment *entries = (CtxSegment*)(&rasterizer->edge_list.entries[0]);\
  int *edges  = rasterizer->edges;\
  uint8_t *rasterizer_src = rasterizer->color;\
  int scanline        = rasterizer->scanline;\
  unsigned int active_edges    = rasterizer->active_edges - 1;\
  int parity        = 0;\
\
  uint8_t *dst = ( (uint8_t *) rasterizer->buf) +\
         (rasterizer->blit_stride * (scanline / CTX_FULL_AA));\
  coverage -= minx;\
\
  const int minx_ = minx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;\
  const int maxx_ = maxx * CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV;\
\
  int cov_min = maxx;\
  int cov_max = minx;\
  const int  bpp      = rasterizer->format->bpp;
  CTX_APPLY_GRAD_A

#define CTX_APPLY_GRAD_B(empty_factor, solid_factor) \
  for (unsigned int t = 0; t < active_edges;t++) \
    { \
      CtxSegment   *segment = &entries[edges[t]]; \
      UPDATE_PARITY; \
\
       if (parity)\
        {\
          CtxSegment   *next_segment = &entries[edges[t+1]]; \
          const int x0        = segment->val; \
          const int x1        = next_segment->val;\
\
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256); \
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256); \
          int first     = graystart >> 8; \
          int last      = grayend   >> 8; \
 \
          if (CTX_UNLIKELY (first < minx)) \
          {  \
            first = minx; \
            graystart=0; \
          } \
          if (CTX_UNLIKELY (last > maxx)) \
          { \
            last = maxx; \
            grayend=255; \
          } \
          graystart = (graystart&0xff) ^ 255; \
          grayend   = (grayend & 0xff); \
\
          if (first < last)\
          {\
            const int delta1 = next_segment->delta; \
            int x1_start = x1 - delta1 * CTX_AA_HALFSTEP2; \
            int x1_end   = x1 + delta1 * CTX_AA_HALFSTEP; \
            unsigned int u0x1 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x1_start, x1_end)));\
\
            unsigned int pre = 1;\
            unsigned int post = 1;\
\
	    if (first - cov_max > CTX_RASTERIZER_MAX_EMPTIES * empty_factor)\
	    {\
                 if (cov_max>=cov_min)\
                 {\
                   apply_coverage (cov_max-cov_min+1, &dst[((cov_min) * bpp)/8], rasterizer_src,\
				   &coverage[cov_min], rasterizer, cov_min);\
                   cov_min = maxx;\
                   cov_max = minx;\
                 }\
	    }\
\
            if (segment->aa == 0)\
            {\
              coverage[first] += graystart;\
 	      cov_min = ctx_mini (cov_min, first);\
 	      cov_max = ctx_maxi (cov_max, first);\
            }\
            else\
            {\
              const int delta0    = segment->delta; \
              int x0_start = x0 - delta0 * CTX_AA_HALFSTEP2; \
              int x0_end   = x0 + delta0 * CTX_AA_HALFSTEP; \
              unsigned int u0x0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_mini (x0_start, x0_end)));\
              unsigned int u1x0 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x0_start, x0_end)));\
\
              int us = u0x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);\
              int mod = ((u0x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255) *\
                         (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255);\
              int sum = ((u1x0-u0x0+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV)/255);\
\
              int recip = (65535)/sum;\
	      int a = mod * recip;\
	      recip *= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV;\
\
 	      cov_min = ctx_mini (cov_min, us);\
              for (unsigned int u = u0x0; u < u1x0; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)\
              {\
                coverage[us ++] += a>>16;\
		a += recip;\
              }\
	      cov_max = us;\
\
              pre = (us-1)-first+1;\
            }\
            if (next_segment->aa != 0) \
	    { \
              post = last - u0x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV); \
	    }\
	    {\
	       int width = (last-post) - (first+pre) + 1;\
	       if (width > CTX_RASTERIZER_MAX_SOLID * solid_factor)\
	       {\
                 if (cov_max>=cov_min)\
                 {\
                   apply_coverage (cov_max-cov_min+1, &dst[((cov_min) * bpp)/8], rasterizer_src,\
				   &coverage[cov_min], rasterizer, cov_min);\
                   cov_min = maxx;\
                   cov_max = minx;\
                 }
  CTX_APPLY_GRAD_B(1, 1)
                       {
#if CTX_STATIC_OPAQUE
                       uint8_t *opaque = &rasterizer->opaque[0];
#else
                       uint8_t opaque[width];
                       memset (opaque, 255, sizeof (opaque));
#endif
                       apply_coverage (width,
                                   &dst[((first + pre) * bpp)/8],
                                   rasterizer_src,
                                   opaque,
				   rasterizer,
                                   first + pre);
                       }
#define CTX_APPLY_GRAD_C \
                 }\
	       else\
	       {\
                 for (int i = 0; i < width; i++)\
                   coverage[first + pre + i] = 255;\
 	         cov_min = ctx_mini (cov_min, first + pre);\
 	         cov_max = first + pre + width;\
	       }\
	    }\
  \
            if (next_segment->aa == 0)\
            {\
               coverage[last] += grayend;\
 	       cov_min = ctx_mini (cov_min, last);\
	       cov_max = last;\
            }\
            else\
            {\
              unsigned int u1x1 = ctx_mini (maxx_, ctx_maxi (minx_, ctx_maxi (x1_start, x1_end)));\
              int us = u0x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV);\
              int mod = ((((u0x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256) % 256)^255)) *\
                    (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/255));\
              int sum = ((u1x1-u0x1+CTX_RASTERIZER_EDGE_MULTIPLIER * CTX_SUBDIV)/255);\
              int recip = (65535) / sum;\
	      int a = (65536 * 255) - mod * recip;\
	      recip *= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV;\
\
 	      cov_min = ctx_mini (cov_min, us);\
              for (unsigned int u = u0x1; u < u1x1; u+= CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV)\
              {\
                coverage[us ++] += (a>>16);\
		a -= recip;\
              }\
	      cov_max = us;\
            }\
          }\
          else if (first == last)\
          {\
            coverage[last]+=(graystart-(grayend^255)); \
	    cov_min = ctx_mini (cov_min, first); \
	    cov_max = last;\
          }\
        }\
   }\
  if (cov_max>=cov_min)\
     apply_coverage (cov_max-cov_min+1, &dst[(cov_min*bpp)/8], rasterizer_src, \
		     &coverage[cov_min], rasterizer, cov_min);
  CTX_APPLY_GRAD_C
}

inline static void
ctx_rasterizer_apply_grads_RGBA8_copy_normal_color (CtxRasterizer *rasterizer,
                                                                     const int      minx,
                                                                     const int      maxx,
                                                                     uint8_t       *coverage,
                                                                     const int      is_winding,
			                                             ctx_apply_coverage_fun apply_coverage)
{
  CTX_APPLY_GRAD_A
  uint32_t src_pix = ((uint32_t*)rasterizer_src)[0];
  CTX_APPLY_GRAD_B(1, 1)
  ctx_span_set_color ((uint32_t*)(&dst[(first+pre) *4]), src_pix, width);
  CTX_APPLY_GRAD_C
}

inline static void
ctx_rasterizer_apply_grads_RGBA8_over_normal_color (CtxRasterizer *rasterizer,
                                                                     const int      minx,
                                                                     const int      maxx,
                                                                     uint8_t       *coverage,
                                                                     const int      is_winding,
			                                             ctx_apply_coverage_fun apply_coverage)
{
  CTX_APPLY_GRAD_A
  uint32_t si_ga_full, si_rb_full, si_ga, si_a;
  si_ga = ((uint32_t*)rasterizer_src)[1];
  si_ga_full = ((uint32_t*)rasterizer_src)[3];
  si_rb_full = ((uint32_t*)rasterizer_src)[4];
  si_a  = si_ga >> 16;
  CTX_APPLY_GRAD_B(1, 1)
  uint32_t* dst_pix = (uint32_t*)(&dst[(first+pre) *4]);
  unsigned int count = width;
  while (count--)
  {
    *dst_pix = ctx_over_RGBA8_full_2(*dst_pix, si_ga_full, si_rb_full, si_a);
    dst_pix++;
  }
  CTX_APPLY_GRAD_C
}

inline static void
ctx_rasterizer_apply_grads_copy_normal_color (CtxRasterizer *rasterizer,
                                                                     const int      minx,
                                                                     const int      maxx,
                                                                     uint8_t       *coverage,
                                                                     const int      is_winding,
                                                                     const CtxCovPath comp,
			                                             ctx_apply_coverage_fun apply_coverage)
{
  CTX_APPLY_GRAD_A
  unsigned int bytes = bpp/8;

  CTX_APPLY_GRAD_B(1, 1)

  uint8_t* dst_i = (uint8_t*)(&dst[(first+pre) * bytes]);
  uint8_t* color = ((uint8_t*)&rasterizer->color_native);
  switch (bytes)
  {
    case 16:
       ctx_span_set_color_x4 ((uint32_t*)dst_i, (uint32_t*)color, width);
       break;
    case 4:
      ctx_span_set_color ((uint32_t*)(&dst[(first+pre) *4]), ((uint32_t*)color)[0], width);
      break;
    case 2:
    {
      uint16_t val = ((uint16_t*)color)[0];
      while (width--)
      {
         ((uint16_t*)dst_i)[0] = val;
         dst_i+=2;
      }
    }
      break;
   default:
   while (width--)
   {
     for (unsigned int b = 0; b < bytes; b++)
       *dst_i++ = color[b];
   }
   break;

  }
  CTX_APPLY_GRAD_C
}

inline static void
ctx_rasterizer_apply_grads_RGBA8_copy_fragment (CtxRasterizer *rasterizer,
                                                                 const int      minx,
                                                                 const int      maxx,
                                                                 uint8_t       *coverage,
                                                                 const int      is_winding,
			                                         ctx_apply_coverage_fun apply_coverage)
{
  CTX_APPLY_GRAD_A
  CTX_APPLY_GRAD_B(1, 1)
                   {
                       float u0 = 0; float v0 = 0;
                       float ud = 0; float vd = 0;
                       float w0 = 1; float wd = 0;
                       ctx_init_uv (rasterizer, first+pre, scanline/CTX_FULL_AA,&u0, &v0, &w0, &ud, &vd, &wd);
                       rasterizer->fragment (rasterizer, u0, v0, w0, &dst[(first+pre)*4],
                                             width, ud, vd, wd);
                   }
  CTX_APPLY_GRAD_C
}

inline static void
ctx_rasterizer_apply_grads_RGBA8_over_fragment (CtxRasterizer *rasterizer,
                                                                 const int      minx,
                                                                 const int      maxx,
                                                                 uint8_t       *coverage,
                                                                 const int      is_winding,
			                                         ctx_apply_coverage_fun apply_coverage)
{
  CTX_APPLY_GRAD_A
  CTX_APPLY_GRAD_B(1, 1)
	       ctx_RGBA8_source_over_normal_full_cov_fragment (
		    width,
		     &dst[(first+pre)*4],
		     NULL,
		     NULL,
		     rasterizer,
		     first + pre,
		    1);
  CTX_APPLY_GRAD_C
}

#undef CTX_APPLY_GRAD_A
#undef CTX_APPLY_GRAD_B
#undef CTX_APPLY_GRAD_C


inline static void
ctx_rasterizer_apply_grads (CtxRasterizer *rasterizer,
                                             const int      minx,
                                             const int      maxx,
                                             uint8_t       *coverage,
                                             const int      is_winding,
                                             const CtxCovPath comp,
			                     ctx_apply_coverage_fun apply_coverage)
{
  if (rasterizer->active_edges < 2) return;
  switch (comp)
  {
#if CTX_RASTERIZER_SWITCH_DISPATCH
    case CTX_COV_PATH_RGBA8_OVER:
       ctx_rasterizer_apply_grads_RGBA8_over_normal_color (rasterizer, minx, maxx, coverage, is_winding, apply_coverage);
       break;
    case CTX_COV_PATH_RGBA8_COPY:
       ctx_rasterizer_apply_grads_RGBA8_copy_normal_color (rasterizer, minx, maxx, coverage, is_winding, apply_coverage);
       break;
    case CTX_COV_PATH_RGB565_COPY:
    case CTX_COV_PATH_RGBAF_COPY:
    case CTX_COV_PATH_RGB332_COPY:
    case CTX_COV_PATH_GRAY8_COPY:
    case CTX_COV_PATH_RGB8_COPY:
    case CTX_COV_PATH_GRAYA8_COPY:
    case CTX_COV_PATH_GRAYAF_COPY:
    case CTX_COV_PATH_CMYKAF_COPY:
    case CTX_COV_PATH_CMYK8_COPY:
    case CTX_COV_PATH_CMYKA8_COPY:
       ctx_rasterizer_apply_grads_copy_normal_color (rasterizer, minx, maxx, coverage, is_winding, comp, apply_coverage);
       break;
    case CTX_COV_PATH_RGBA8_COPY_FRAGMENT:
       ctx_rasterizer_apply_grads_RGBA8_copy_fragment (rasterizer, minx, maxx, coverage, is_winding, apply_coverage);
       break;
    case CTX_COV_PATH_RGBA8_OVER_FRAGMENT:
       ctx_rasterizer_apply_grads_RGBA8_over_fragment (rasterizer, minx, maxx, coverage, is_winding, apply_coverage);
       break;
#endif
     default:
	ctx_rasterizer_apply_grads_generic (rasterizer, minx, maxx, coverage, is_winding, apply_coverage);
  }
}

static inline void
ctx_rasterizer_reset_soft (CtxRasterizer *rasterizer)
{
#if CTX_SCANBIN==0
  rasterizer->edge_pos        =   
#endif
  rasterizer->shadow_edge_pos =   
  rasterizer->scanline        = 0;
  //rasterizer->comp_op       = NULL; // keep comp_op cached 
  //     between rasterizations where rendering attributes are
  //     nonchanging
}


static inline void
ctx_rasterizer_reset (CtxRasterizer *rasterizer)
{
  ctx_rasterizer_reset_soft (rasterizer);
  rasterizer->first_edge = -1;
  rasterizer->has_prev        =   
  rasterizer->edge_list.count =    // ready for new edges
  rasterizer->edge_list.count =    // ready for new edges
#if CTX_SCANBIN==0
  rasterizer->edge_pos        =   
#endif
  rasterizer->shadow_edge_pos =   
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

#if CTX_SCANBIN==0
static CTX_INLINE int ctx_compare_edge (const void *ap, int by0)
{
  return ((const CtxSegment *) ap)->y0 - by0;
}

static CTX_INLINE int ctx_edge_qsort_partition (CtxSegment *A, int low, int high)
{
  int pivot_y0 = A[ (high+low) /2].y0;
  int i = low;
  int j = high;
  while (i <= j)
    {
      while (ctx_compare_edge (&A[i], pivot_y0) < 0) { i ++; }
      while (ctx_compare_edge (&A[j], pivot_y0) > 0) { j --; }
      if (i <= j)
        {
          CtxSegment tmp = A[i];
          A[i++] = A[j];
          A[j--] = tmp;
        }
    }
  return i;
}

static void ctx_edge_qsortb (CtxSegment *entries, int low, int high)
{
  do {
    int p = ctx_edge_qsort_partition (entries, low, high);
    if (low < p - 1)
      ctx_edge_qsortb (entries, low, p - 1);
    if (low >= high)
      return;
    low = p;
  } while (1);
}

static CTX_INLINE void ctx_edge_qsort (CtxSegment *entries, int low, int high)
{
  do {
    int p = ctx_edge_qsort_partition (entries, low, high);
    if (low < p - 1)
      ctx_edge_qsortb (entries, low, p - 1);
    if (low >= high)
      return;
    low = p;
  } while (1);
}

static CTX_INLINE void ctx_sort_edges (CtxRasterizer *rasterizer)
{
  ctx_edge_qsort ((CtxSegment*)& (rasterizer->edge_list.entries[0]), 0, rasterizer->edge_list.count-1);
}
#endif

static void
ctx_rasterizer_rasterize_edges2 (CtxRasterizer *rasterizer, const int fill_rule, const int allow_direct)
{
  rasterizer->pending_edges   =   
  rasterizer->active_edges    =   0;
  CtxGState     *gstate     = &rasterizer->state->gstate;
  const int      is_winding  = fill_rule == CTX_FILL_RULE_WINDING;
  const CtxCovPath comp = rasterizer->comp;
  uint8_t  *dst         = ((uint8_t *) rasterizer->buf);
  int       scan_start  = rasterizer->blit_y * CTX_FULL_AA;
  int       scan_end    = scan_start + (rasterizer->blit_height - 1) * CTX_FULL_AA;
  const int blit_width  = rasterizer->blit_width;
  const int blit_max_x  = rasterizer->blit_x + blit_width;
  int       minx        = rasterizer->col_min / CTX_SUBDIV - rasterizer->blit_x;
  int       maxx        = (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV -
                          rasterizer->blit_x;
  const int bpp = rasterizer->format->bpp;
  const int blit_stride = rasterizer->blit_stride;

  uint8_t *rasterizer_src = rasterizer->color;

  if (maxx > blit_max_x - 1)
    { maxx = blit_max_x - 1; }

  minx = ctx_maxi (gstate->clip_min_x, minx);
  maxx = ctx_mini (gstate->clip_max_x, maxx);
  minx *= (minx>0);
 
  int pixs = maxx - minx + 1;
  uint8_t _coverage[pixs+16]; // XXX this might hide some valid asan warnings
  uint8_t *coverage = &_coverage[0];
  ctx_apply_coverage_fun apply_coverage = rasterizer->apply_coverage;

  rasterizer->scan_min -= (rasterizer->scan_min % CTX_FULL_AA);
  {
     if (rasterizer->scan_min > scan_start)
       {
          dst += (blit_stride * (rasterizer->scan_min-scan_start) / CTX_FULL_AA);
          scan_start = rasterizer->scan_min;
       }
      scan_end = ctx_mini (rasterizer->scan_max, scan_end);
  }

  if (CTX_UNLIKELY(gstate->clip_min_y * CTX_FULL_AA > scan_start ))
    { 
       dst += (blit_stride * (gstate->clip_min_y * CTX_FULL_AA -scan_start) / CTX_FULL_AA);
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
  rasterizer->scan_aa[1]=
  rasterizer->scan_aa[2]=
  rasterizer->scan_aa[3]=0;

#if CTX_SCANBIN
  int ss = scan_start/CTX_FULL_AA;
  int se = scan_end/CTX_FULL_AA;
  if (ss < 0)ss =0;
  if (se >= CTX_MAX_SCANLINES) se = CTX_MAX_SCANLINES-1;

  for (int i = ss; i < se; i++)
    rasterizer->scan_bin_count[i]=0;

  for (unsigned int i = 0; i < rasterizer->edge_list.count; i++)
  {
    CtxSegment *segment = & ((CtxSegment*)rasterizer->edge_list.entries)[i];
    int scan = (segment->y0-CTX_FULL_AA+2) / CTX_FULL_AA;
    if (scan < ss) scan = ss;
    if (scan < se)
      rasterizer->scan_bins[scan][rasterizer->scan_bin_count[scan]++]=i;
  }
#else
  ctx_sort_edges (rasterizer);
#endif

  rasterizer->scanline = scan_start;

  while (rasterizer->scanline <= scan_end)
    {
      int c0 = minx;
      int c1 = maxx;
      int aa = ctx_rasterizer_feed_edges_full (rasterizer, 0, 0.0f);
      switch (aa)
      {
        case -1: /* no edges */
          rasterizer->scanline += CTX_FULL_AA;
          dst += blit_stride;
          continue;
        case 0: /* the scanline transitions does not contain multiple intersections - each aa segment is a linear ramp */
        case 1: /* level-1 aa is good enough - use same case for less iteration of edges */
        { 
          rasterizer->scanline += CTX_AA_HALFSTEP2;
          ctx_rasterizer_feed_pending_edges (rasterizer);
	  ctx_rasterizer_sort_active_edges (rasterizer);
    
          memset (coverage, 0, pixs);
          if (allow_direct)
          {
            ctx_rasterizer_apply_grads (rasterizer, minx, maxx, coverage, is_winding, comp, apply_coverage);
            rasterizer->scanline += CTX_AA_HALFSTEP;
            ctx_rasterizer_increment_edges (rasterizer, CTX_FULL_AA);
    
            dst += blit_stride;
            continue;
          }
          ctx_rasterizer_generate_coverage_grads (rasterizer, minx, maxx, coverage, is_winding, &c0, &c1);
          rasterizer->scanline += CTX_AA_HALFSTEP;
          ctx_rasterizer_increment_edges (rasterizer, CTX_FULL_AA);
          break;
        }
#if 1
        case 3:
        { /* level of oversampling based on lowest steepness edges */
	  const int raa=3;
          ctx_rasterizer_increment_edges (rasterizer, -CTX_AA_HALFSTEP2);
          memset (coverage, 0, pixs);
          const int scanline_increment = 15/raa;
          const uint8_t fraction = 255/raa;

	  c0 = maxx;
	  c1 = minx;
          for (int i = 1; i <= raa; i++)
          {
            ctx_rasterizer_sort_active_edges (rasterizer);
            ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, is_winding, raa, fraction, &c0, &c1);
            rasterizer->scanline += scanline_increment;
            ctx_rasterizer_increment_edges (rasterizer, scanline_increment + CTX_AA_HALFSTEP2 * (i==raa));
            ctx_rasterizer_feed_pending_edges (rasterizer);
          }
        }
	break;
        case 5:
        { /* level of oversampling based on lowest steepness edges */
	  const int raa=5;
          ctx_rasterizer_increment_edges (rasterizer, -CTX_AA_HALFSTEP2);
          memset (coverage, 0, pixs);
          const int scanline_increment = 15/raa;
          const uint8_t fraction = 255/raa;

	  c0 = maxx;
	  c1 = minx;
          for (int i = 1; i <= raa; i++)
          {
            ctx_rasterizer_sort_active_edges (rasterizer);
            ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, is_winding, raa, fraction, &c0, &c1);
            rasterizer->scanline += scanline_increment;
            ctx_rasterizer_increment_edges (rasterizer, scanline_increment + CTX_AA_HALFSTEP2 * (i==raa));
            ctx_rasterizer_feed_pending_edges (rasterizer);
          }
        }
	break;
        case 15:
        { /* level of oversampling based on lowest steepness edges */
	  const int raa=15;
          ctx_rasterizer_increment_edges (rasterizer, -CTX_AA_HALFSTEP2);
          memset (coverage, 0, pixs);
          const int scanline_increment = 15/raa;
          const uint8_t fraction = 255/raa;

	  c0 = maxx;
	  c1 = minx;
          for (int i = 1; i <= raa; i++)
          {
            ctx_rasterizer_sort_active_edges (rasterizer);
            ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, is_winding, raa, fraction, &c0, &c1);
            rasterizer->scanline += scanline_increment;
            ctx_rasterizer_increment_edges (rasterizer, scanline_increment + CTX_AA_HALFSTEP2 * (i==raa));
            ctx_rasterizer_feed_pending_edges (rasterizer);
          }
        }
	break;
#else
        default:
        { /* level of oversampling based on lowest steepness edges */
	  const int raa=aa;
          ctx_rasterizer_increment_edges (rasterizer, -CTX_AA_HALFSTEP2);
          memset (coverage, 0, pixs);
          const int scanline_increment = 15/raa;
          const uint8_t fraction = 255/raa;

	  c0 = maxx;
	  c1 = minx;
          for (int i = 1; i <= raa; i++)
          {
            ctx_rasterizer_sort_active_edges (rasterizer);
            ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, is_winding, raa, fraction, &c0, &c1);
            rasterizer->scanline += scanline_increment;
            ctx_rasterizer_increment_edges (rasterizer, scanline_increment + CTX_AA_HALFSTEP2 * (i==raa));
            ctx_rasterizer_feed_pending_edges (rasterizer);
          }
        }
#endif
      }
  
      if (c1 >= c0)
      {
        ctx_coverage_post_process (rasterizer, c0, c1, coverage - minx, NULL, NULL);
        apply_coverage (c1-c0+1,
                        &dst[(c0 * bpp) /8],
                        rasterizer_src,
                        coverage + (c0-minx),
                        rasterizer, c0);
      }
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
     //int gscan_end = gstate->clip_max_y * CTX_FULL_AA;
     memset (nocoverage, 0, sizeof(nocoverage));
     int startx   = gstate->clip_min_x;
     int endx     = gstate->clip_max_x;
     int clipw    = endx-startx + 1;
     uint8_t *dst = ( (uint8_t *) rasterizer->buf);

     dst = (uint8_t*)(rasterizer->buf) + blit_stride * (gscan_start / CTX_FULL_AA);
     for (rasterizer->scanline = gscan_start; rasterizer->scanline < scan_start;)
     {
       apply_coverage (clipw,
                       &dst[ (startx * rasterizer->format->bpp) /8],
                       rasterizer_src, nocoverage, rasterizer, 0);
       rasterizer->scanline += CTX_FULL_AA;
       dst += blit_stride;
     }

     if (0)//(minx > startx) & (minx<maxx))
     {
     dst = (uint8_t*)(rasterizer->buf) + blit_stride * (scan_start / CTX_FULL_AA);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       apply_coverage (minx-startx,
                       &dst[ (startx * rasterizer->format->bpp) /8],
                       rasterizer_src,
                       nocoverage, rasterizer, 0);
       dst += blit_stride;
     }
     }

     if (endx > maxx)
     {
     dst = (uint8_t*)(rasterizer->buf) + blit_stride * (scan_start / CTX_FULL_AA);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       apply_coverage (endx-maxx,
                       &dst[ (maxx * rasterizer->format->bpp) /8],
                       rasterizer_src, nocoverage, rasterizer, 0);

       rasterizer->scanline += CTX_FULL_AA;
       dst += blit_stride;
     }
     }
#if 0
     dst = (uint8_t*)(rasterizer->buf) + blit_stride * (scan_end / CTX_FULL_AA);
     for (rasterizer->scanline = scan_end; rasterizer->scanline < gscan_end;)
     {
       apply_coverage (clipw-1,
                       &dst[ (startx * rasterizer->format->bpp) /8],
                       rasterizer_src,
                       nocoverage, rasterizer, 0);

       rasterizer->scanline += CTX_FULL_AA;
       dst += blit_stride;
     }
#endif
  }
#endif
}

static void
ctx_rasterizer_rasterize_edges3 (CtxRasterizer *rasterizer, const int fill_rule)
{
  rasterizer->pending_edges   =   
  rasterizer->active_edges    =   0;
  rasterizer->shadow_active_edges =   0;
  CtxGState *gstate     = &rasterizer->state->gstate;
  float blur_radius = gstate->shadow_blur *
	   ctx_matrix_get_scale (&gstate->transform)* 2;
  //fprintf (stderr, "%f\n", gstate->shadow_blur); 
  const int  is_winding = fill_rule == CTX_FILL_RULE_WINDING;
  uint8_t  *dst         = ((uint8_t *) rasterizer->buf);

  int       scan_start  = rasterizer->blit_y * CTX_FULL_AA;
  int       scan_end    = scan_start + (rasterizer->blit_height - 1) * CTX_FULL_AA;
  const int blit_width  = rasterizer->blit_width;
  const int blit_max_x  = rasterizer->blit_x + blit_width;
  int       minx        = rasterizer->col_min / CTX_SUBDIV - rasterizer->blit_x;
  int       maxx        = (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV -
                          rasterizer->blit_x;
  const int bpp = rasterizer->format->bpp;
  const int blit_stride = rasterizer->blit_stride;

  uint8_t *rasterizer_src = rasterizer->color;

  if (maxx > blit_max_x - 1)
    { maxx = blit_max_x - 1; }

  minx = ctx_maxi (gstate->clip_min_x, minx);
  maxx = ctx_mini (gstate->clip_max_x, maxx);
  minx *= (minx>0);
 
  int pixs = maxx - minx + 1;
  uint8_t _coverage[pixs+16]; // XXX this might hide some valid asan warnings
  uint8_t *coverage = &_coverage[0];
  ctx_apply_coverage_fun apply_coverage = rasterizer->apply_coverage;

  int br = ((int)blur_radius) * CTX_FULL_AA;
  rasterizer->scan_min -= (rasterizer->scan_min % CTX_FULL_AA);
  {
     if (rasterizer->scan_min > scan_start)
       {
          dst += (blit_stride * (rasterizer->scan_min-scan_start) / CTX_FULL_AA);
          scan_start = rasterizer->scan_min;
       }
      scan_end = ctx_mini (rasterizer->scan_max, scan_end);
  }

  if (CTX_UNLIKELY(gstate->clip_min_y * CTX_FULL_AA > scan_start ))
    { 
       dst += (blit_stride * (gstate->clip_min_y * CTX_FULL_AA -scan_start) / CTX_FULL_AA);
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
  rasterizer->scan_aa[1]=
  rasterizer->scan_aa[2]=
  rasterizer->scan_aa[3]=0;

#if CTX_SCANBIN
  int ss = scan_start/CTX_FULL_AA;
  int se = scan_end/CTX_FULL_AA;
  if (ss < 0)ss =0;
  if (se >= CTX_MAX_SCANLINES) se = CTX_MAX_SCANLINES-1;

  for (int i = ss; i < se; i++)
    rasterizer->scan_bin_count[i]=0;

  for (unsigned int i = 0; i < rasterizer->edge_list.count; i++)
  {
    CtxSegment *segment = & ((CtxSegment*)rasterizer->edge_list.entries)[i];
    int scan = (segment->y0-CTX_FULL_AA+2) / CTX_FULL_AA;
    if (scan < ss) scan = ss;
    if (scan < se)
      rasterizer->scan_bins[scan][rasterizer->scan_bin_count[scan]++]=i;
  }
#else
  ctx_sort_edges (rasterizer);
#endif

  rasterizer->scanline = scan_start;

  while (rasterizer->scanline <= scan_end)
    {
      int c0 = minx;
      int c1 = maxx;
        ctx_rasterizer_feed_edges_full (rasterizer, 1, blur_radius);
        { 
          rasterizer->scanline += CTX_AA_HALFSTEP2;
          ctx_rasterizer_feed_pending_edges (rasterizer);
    
          memset (coverage, 0, pixs);
	  ctx_rasterizer_sort_active_edges (rasterizer);
          ctx_rasterizer_generate_sdf (rasterizer, minx, maxx, coverage, is_winding, blur_radius);
          rasterizer->scanline += CTX_AA_HALFSTEP;
          ctx_rasterizer_increment_edges (rasterizer, CTX_FULL_AA);
        }
  
      {
        ctx_coverage_post_process (rasterizer, c0, c1, coverage - minx, NULL, NULL);
        apply_coverage (c1-c0+1,
                        &dst[(c0 * bpp) /8],
                        rasterizer_src,
                        coverage + (c0-minx),
                        rasterizer, c0);
      }
      dst += blit_stride;
    }
}



#if CTX_INLINE_FILL_RULE
void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule);
#else

void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule);
#endif


#if CTX_INLINE_FILL_RULE

// this can shave 1-2% percent off execution time, at the penalty of increased code size
void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule)
{
#if CTX_RASTERIZER_ALLOW_DIRECT
  int allow_direct = !(0 
#if CTX_ENABLE_CLIP
         | ((rasterizer->clip_buffer!=NULL) & (!rasterizer->clip_rectangle))
#endif
#if CTX_ENABLE_SHADOW_BLUR
         | rasterizer->in_shadow
#endif
         );
#else
  const int allow_direct = 0;  // temporarily disabled
			       // we seem to overrrun our scans
#endif
    if (rasterizer->in_shadow)
    {
      if (fill_rule) ctx_rasterizer_rasterize_edges3 (rasterizer, 1);
      else           ctx_rasterizer_rasterize_edges3 (rasterizer, 0);
      return;
    }

#if 1
    if (allow_direct)
    {
      if (fill_rule) ctx_rasterizer_rasterize_edges2 (rasterizer, 1, 1);
      else           ctx_rasterizer_rasterize_edges2 (rasterizer, 0, 1);
    }
    else
    {
      if (fill_rule) ctx_rasterizer_rasterize_edges2 (rasterizer, 1, 0);
      else           ctx_rasterizer_rasterize_edges2 (rasterizer, 0, 0);
    }
#else
#endif
}
#else

void
CTX_SIMD_SUFFIX (ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule)
{
  int allow_direct = !(0 
#if CTX_ENABLE_CLIP
         | ((rasterizer->clip_buffer!=NULL) & (!rasterizer->clip_rectangle))
#endif
         );
  if (rasterizer->in_shadow)
    ctx_rasterizer_rasterize_edges3 (rasterizer, fill_rule);
  else
    ctx_rasterizer_rasterize_edges2 (rasterizer, fill_rule, allow_direct);
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

static CTX_INLINE int ctx_rasterizer_add_point (CtxRasterizer *rasterizer, int x1, int y1)
{
  CtxSegment entry = {{CTX_EDGE, 0, 0, 0, 0, 0}};
  x1 -= rasterizer->blit_x * CTX_SUBDIV;

  entry.x0=rasterizer->inner_x;
  entry.y0=rasterizer->inner_y;

  entry.x1=x1;
  entry.y1=y1;

  ctx_rasterizer_update_inner_point (rasterizer, x1, y1);

  int ret = ctx_edgelist_add_single (&rasterizer->edge_list, (CtxEntry*)&entry);
  if (CTX_UNLIKELY(rasterizer->has_prev<=0))
    {
      CtxSegment *segment = & ((CtxSegment*)rasterizer->edge_list.entries)[rasterizer->edge_list.count-1];
      segment->code = CTX_NEW_EDGE;
      rasterizer->has_prev = 1;
      rasterizer->first_edge = rasterizer->edge_list.count-1;
    }
  return ret;
}

static inline void ctx_rasterizer_poly_to_edges (CtxRasterizer *rasterizer)
{
  unsigned int count = rasterizer->edge_list.count;
  CtxSegment *segment = (CtxSegment*)&rasterizer->edge_list.entries[0];
  int skipped = 0;
  for (unsigned int i = 0; i < count; i++)
    {
      if (segment[skipped].code == CTX_CLOSE_EDGE)
	skipped ++;
      else
      {
        if (segment[skipped].y1 < segment[skipped].y0)
        {
          segment[0] = ctx_segment_s16 (CTX_EDGE_FLIPPED,
                            segment[skipped].x1, segment[skipped].y1,
                            segment[skipped].x0, segment[skipped].y0);
        }
        else
        {
	  segment[0] = segment[skipped];
        }
        segment++;
      }
    }
  rasterizer->edge_list.count = count - skipped;
}

static inline void
ctx_rasterizer_close_path (CtxRasterizer *rasterizer)
{
  int x0 = rasterizer->inner_x;
  int y0 = rasterizer->inner_y;
  if ((rasterizer->has_prev > 0) & (rasterizer->first_edge>=0))
    {

      if (rasterizer->first_edge>=0)
      {
        CtxSegment *segment = & ((CtxSegment*)rasterizer->edge_list.entries)[rasterizer->first_edge];
	if (segment->code == CTX_NEW_EDGE)
	{
          CtxSegment entry = {{CTX_EDGE, 0, 0, 0, 0, 0}};
          int x1 = segment->x0;
	  int y1 = segment->y0;
          entry.x0=x0;
          entry.y0=y0;
          entry.x1=x1;
          entry.y1=y1;
          ctx_rasterizer_update_inner_point (rasterizer, x1, y1);
          rasterizer->has_prev = 0;
	  rasterizer->first_edge = -1;
          ctx_edgelist_add_single (&rasterizer->edge_list, (CtxEntry*)&entry);
   entry = *segment;
   entry.code = CTX_CLOSE_EDGE;

          ctx_edgelist_add_single (&rasterizer->edge_list, (CtxEntry*)&entry);
	  // shorten to half length?
          return;
	}
      }
    }
}

//#define MIN_Y -100
//#define MAX_Y 3800
//#define MIN_X -100
//#define MAX_X 3600*10

static inline void ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  int tx = 0, ty = 0;

  rasterizer->x        = x;
  rasterizer->y        = y;
  rasterizer->first_edge = rasterizer->edge_list.count - 1; // ?
  rasterizer->has_prev = -1;
  _ctx_user_to_device_prepped (rasterizer->state, x,y, &tx, &ty);

  tx -= rasterizer->blit_x * CTX_SUBDIV;
  ctx_rasterizer_update_inner_point (rasterizer, tx, ty);
}

static CTX_INLINE void
ctx_rasterizer_line_to_fixed (CtxRasterizer *rasterizer, int x, int y)
{
  int tx = 0, ty = 0;
  _ctx_user_to_device_prepped_fixed (rasterizer->state, x, y, &tx, &ty);
  ctx_rasterizer_add_point (rasterizer, tx, ty);
}

static CTX_INLINE void
ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  int tx = 0, ty = 0;
  rasterizer->y         = y;
  rasterizer->x         = x;

  _ctx_user_to_device_prepped (rasterizer->state, x, y, &tx, &ty);
  ctx_rasterizer_add_point (rasterizer, tx, ty);
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

  if ((iteration<2) | ((iteration < 6) & (dx*dx+dy*dy > tolerance)))
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
  return v0 + (((v1-v0) * dx + ((1<<CTX_FIX_SHIFT)-1)) >> CTX_FIX_SHIFT);
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

typedef struct CtxFixedBezier
{
  int x0; int y0;
  int x1; int y1;
  int x2; int y2;
  int x3; int y3;
} CtxFixedBezier;

CTX_INLINE static void
ctx_bezier_sample_fixed (const CtxFixedBezier *b,
                         int dt, int *x, int *y)
{
  *x = ctx_bezier_sample_1d_fixed (b->x0, b->x1, b->x2, b->x3, dt);
  *y = ctx_bezier_sample_1d_fixed (b->y0, b->y1, b->y2, b->y3, dt);
}

static inline void
ctx_rasterizer_bezier_divide_fixed (CtxRasterizer *rasterizer,
				    const CtxFixedBezier *b,
                                    int sx, int sy,
                                    int ex, int ey,
                                    int s,
                                    int e,
                                    int iteration, long int tolerance)
{
  int t = (s + e) / 2;
  int x, y;

  ctx_bezier_sample_fixed (b, t, &x, &y);

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
    ctx_rasterizer_bezier_divide_fixed (rasterizer, b,
                                  sx, sy, x, y, s, t, iteration+1, tolerance
                                  );
    ctx_rasterizer_line_to_fixed (rasterizer, x, y);
    ctx_rasterizer_bezier_divide_fixed (rasterizer, b,
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


#if CTX_RASTERIZER_BEZIER_FIXED_POINT
  CtxFixedBezier b = {
            (int)(ox * CTX_FIX_SCALE), (int)(oy * CTX_FIX_SCALE), (int)(x0 * CTX_FIX_SCALE), (int)(y0 * CTX_FIX_SCALE),
            (int)(x1 * CTX_FIX_SCALE), (int)(y1 * CTX_FIX_SCALE), (int)(x2 * CTX_FIX_SCALE), (int)(y2 * CTX_FIX_SCALE)
  };
  ctx_rasterizer_bezier_divide_fixed (rasterizer, &b,
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

static CTX_INLINE int ctx_perpdot(int ax,int ay,int bx, int by)
{ return (ax*by)-(ay*bx);
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
  int preserved = 0;
  if (rasterizer->preserve)
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (CtxSegment)*preserved_count );
      preserved = 1;
    }

#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
    float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
  for (unsigned int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxSegment *segment = &((CtxSegment*)rasterizer->edge_list.entries)[i];
      segment->x0 += rasterizer->state->gstate.shadow_offset_x * CTX_SUBDIV * factor;
      segment->y0 += rasterizer->state->gstate.shadow_offset_y * CTX_FULL_AA * factor;
      segment->x1 += rasterizer->state->gstate.shadow_offset_x * CTX_SUBDIV * factor;
      segment->y1 += rasterizer->state->gstate.shadow_offset_y * CTX_FULL_AA * factor;
    }
    rasterizer->scan_min += ((rasterizer->state->gstate.shadow_offset_y - gstate->shadow_blur)*factor +1) * CTX_FULL_AA;
    rasterizer->scan_max += ((rasterizer->state->gstate.shadow_offset_y + gstate->shadow_blur)*factor +1) * CTX_FULL_AA;
    rasterizer->col_min  += ((rasterizer->state->gstate.shadow_offset_x - gstate->shadow_blur)*factor + 1) * CTX_SUBDIV;
    rasterizer->col_max  += ((rasterizer->state->gstate.shadow_offset_x + gstate->shadow_blur)*factor + 1) * CTX_SUBDIV;
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
          (entry0->x1 == entry1->x1) &
          (entry0->y1 == entry3->y1) &
          (entry1->y1 == entry2->y1) &
          (entry2->x1 == entry3->x1)
#if CTX_ENABLE_SHADOW_BLUR
           & (!rasterizer->in_shadow)
#endif
         )
       {
         float x0 = entry3->x1 * (1.0f / CTX_SUBDIV);
         float y0 = entry3->y1 * (1.0f / CTX_FULL_AA);
         float x1 = entry1->x1 * (1.0f / CTX_SUBDIV);
         float y1 = entry1->y1 * (1.0f / CTX_FULL_AA);

         if ((x1 > x0) & (y1 > y0))
         {
           ctx_composite_fill_rect (rasterizer, x0, y0, x1, y1, 255);
           goto done;
         }
       }
    }
#endif


    ctx_rasterizer_close_path (rasterizer);
    ctx_rasterizer_poly_to_edges (rasterizer);

    ctx_rasterizer_rasterize_edges (rasterizer, gstate->fill_rule);
  }
#if CTX_FAST_FILL_RECT
done:
#endif
  if (preserved)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (CtxSegment)*preserved_count );
      rasterizer->edge_list.count = preserved_count;
    }
#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
    float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
    rasterizer->scan_min -= ((rasterizer->state->gstate.shadow_offset_y - gstate->shadow_blur) *factor+ 1) * CTX_FULL_AA;
    rasterizer->scan_max -= ((rasterizer->state->gstate.shadow_offset_y + gstate->shadow_blur) *factor+ 1) * CTX_FULL_AA;
    rasterizer->col_min  -= ((rasterizer->state->gstate.shadow_offset_x - gstate->shadow_blur) *factor+ 1) * CTX_SUBDIV;
    rasterizer->col_max  -= ((rasterizer->state->gstate.shadow_offset_x + gstate->shadow_blur) *factor+ 1) * CTX_SUBDIV;
  }
#endif
  rasterizer->preserve = 0;
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

  if ((tx2 < rasterizer->blit_x) | (ty2 < rasterizer->blit_y))
    return;
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
  if (full_segments < 24) full_segments = 24;
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
  rasterizer->apply_coverage (1, dst, rasterizer->color, &cov, rasterizer, x);
}


static inline void
ctx_rasterizer_stroke_1px_segment (CtxRasterizer *rasterizer,
                                   float x0, float y0,
                                   float x1, float y1)
{
  ctx_apply_coverage_fun apply_coverage = rasterizer->apply_coverage;
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
         apply_coverage (1, dst, rasterizer_src, &rcov, rasterizer, x);
         dst += blit_stride;
         apply_coverage (1, dst, rasterizer_src, &ypos, rasterizer, x);
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
         apply_coverage (2, dst, rasterizer_src, cov, rasterizer, x);
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
          CtxSegment *segment = &temp[i];
          float x, y;
          if (segment->code == CTX_NEW_EDGE)
            {
              if (started)
                {
                  end = i - 1;
                  goto foo;
                }
              prev_x = segment->x0 * 1.0f / CTX_SUBDIV;
              prev_y = segment->y0 * 1.0f / CTX_FULL_AA;
              started = 1;
              start = i;
            }
          x = segment->x1 * 1.0f / CTX_SUBDIV;
          y = segment->y1 * 1.0f / CTX_FULL_AA;
          
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
          (entry0->x1 == entry1->x1) &
          (entry0->y1 == entry3->y1) &
          (entry1->y1 == entry2->y1) &
          (entry2->x1 == entry3->x1)
#if CTX_ENABLE_SHADOW_BLUR
           & !rasterizer->in_shadow
#endif
         )
       {
        float x0 = entry3->x1 * 1.0f / CTX_SUBDIV;
        float y0 = entry3->y1 * 1.0f / CTX_FULL_AA;
        float x1 = entry1->x1 * 1.0f / CTX_SUBDIV;
        float y1 = entry1->y1 * 1.0f / CTX_FULL_AA;

        ctx_composite_stroke_rect (rasterizer, x0, y0, x1, y1, line_width);

        goto done;
       }
    }
#endif
#endif
  
    {
    {
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
              CtxSegment *segment= &temp[i];
              float x, y;
              if (segment->code == CTX_NEW_EDGE)
                {
                  if (CTX_LIKELY(started))
                    {
                      end = i - 1;
                      goto foo;
                    }
                  prev_x = segment->x0 * 1.0f / CTX_SUBDIV;
                  prev_y = segment->y0 * 1.0f / CTX_FULL_AA;
                  started = 1;
                  start = i;
                }
              x = segment->x1 * 1.0f / CTX_SUBDIV;
              y = segment->y1 * 1.0f/ CTX_FULL_AA;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_hypotf (dx, dy);
              if ((length>CTX_MIN_STROKE_LEN) | (segment->code == CTX_NEW_EDGE))
                {
                  float recip_length = 1.0f/length;
                  dx = dx * recip_length * half_width_x;
                  dy = dy * recip_length * half_width_y;
                  if (segment->code == CTX_NEW_EDGE)
                    {
                      ctx_rasterizer_close_path (rasterizer);
                      ctx_rasterizer_move_to (rasterizer, prev_x+dy, prev_y-dx);
                    }
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  
                  ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
                }
                  prev_x = x;
                  prev_y = y;
            }
          end = i-1;
foo:
          for (int i = end; i >= start; i--)
            {
              CtxSegment *segment = &temp[i];
              float x, y, dx, dy;
              x = segment->x1 * 1.0f / CTX_SUBDIV;
              y = segment->y1 * 1.0f / CTX_FULL_AA;
              dx = x - prev_x;
              dy = y - prev_y;
              float length = ctx_hypotf (dx, dy);
              if (length>CTX_MIN_STROKE_LEN)
                {
                  float recip_length = 1.0f/length;
                  dx = dx * recip_length * half_width_x;
                  dy = dy * recip_length * half_width_y;
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);

                  // XXX possible miter line-to
             //   ctx_rasterizer_line_to (rasterizer, prev_x-dy+10, prev_y+dx+10);
                  ctx_rasterizer_line_to (rasterizer, x-dy,      y+dx);
              	  prev_x = x;
                  prev_y = y;
                }
              if (CTX_UNLIKELY(segment->code == CTX_NEW_EDGE))
                {
                  x = segment->x0 * 1.0f / CTX_SUBDIV;
                  y = segment->y0 * 1.0f / CTX_FULL_AA;
                  dx = x - prev_x;
                  dy = y - prev_y;
                  length = ctx_hypotf (dx, dy);
                  if (CTX_LIKELY(length>CTX_MIN_STROKE_LEN))
                    {
                      float recip_length = 1.0f/length;
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
      ctx_rasterizer_close_path (rasterizer);
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
                  CtxSegment *segment = &temp[i];
                  if (CTX_UNLIKELY(segment->code == CTX_NEW_EDGE))
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_rectangle_reverse (rasterizer, x - half_width_x, y - half_width_y, half_width_x, half_width_y);
                          ctx_rasterizer_close_path (rasterizer);
                        }
                      x = segment->x0 * 1.0f / CTX_SUBDIV;
                      y = segment->y0 * 1.0f / CTX_FULL_AA;
                      ctx_rasterizer_rectangle_reverse (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
                      ctx_rasterizer_close_path (rasterizer);
                    }
                  x = segment->x1 * 1.0f / CTX_SUBDIV;
                  y = segment->y1 * 1.0f / CTX_FULL_AA;
                  has_prev = 1;
                }
              ctx_rasterizer_rectangle_reverse (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
              ctx_rasterizer_close_path (rasterizer);
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
                  CtxSegment *segment = &temp[i];
                  if (CTX_UNLIKELY(segment->code == CTX_NEW_EDGE))
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                          ctx_rasterizer_close_path (rasterizer);
                        }
                      x = segment->x0 * 1.0f / CTX_SUBDIV;
                      y = segment->y0 * 1.0f / CTX_FULL_AA;
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*2, 0, 1);
                      ctx_rasterizer_close_path (rasterizer);
                    }
                  x = segment->x1 * 1.0f / CTX_SUBDIV;
                  y = segment->y1 * 1.0f / CTX_FULL_AA;
                  has_prev = 1;
                }
              ctx_rasterizer_move_to (rasterizer, x, y);
              ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*2, 0, 1);
              ctx_rasterizer_close_path (rasterizer);
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
                  CtxSegment *segment = &temp[i];
                  x = segment->x1 * 1.0f / CTX_SUBDIV;
                  y = segment->y1 * 1.0f / CTX_FULL_AA;
                  if (CTX_UNLIKELY(segment[1].code == CTX_EDGE))
                    {
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*2, 0, 1);
                      ctx_rasterizer_close_path (rasterizer);
                    }
                }
              break;
            }
        }
      CtxFillRule rule_backup = gstate->fill_rule;
      gstate->fill_rule = CTX_FILL_RULE_WINDING;
      rasterizer->preserve = 0; // so fill isn't tripped
      int aa = rasterizer->aa;
      rasterizer->aa = 3 + (aa>5)*2;
      ctx_rasterizer_fill (rasterizer);
      rasterizer->aa = aa;
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
  unsigned int count = edges[0].u32[0];
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
      CtxSegment *segment = &edges[i+1];
      float x, y;
      if (segment->code == CTX_NEW_EDGE)
        {
          prev_x = segment->x0 / CTX_SUBDIV;
          prev_y = segment->y0 / CTX_FULL_AA;
          if (prev_x < minx) { minx = prev_x; }
          if (prev_y < miny) { miny = prev_y; }
          if (prev_x > maxx) { maxx = prev_x; }
          if (prev_y > maxy) { maxy = prev_y; }
        }
      x = segment->x1 * 1.0f / CTX_SUBDIV;
      y = segment->y1 * 1.0f / CTX_FULL_AA;
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
          (coords[0][1] == coords[3][1]) &
          (coords[1][1] == coords[2][1]) &
          (coords[2][0] == coords[3][0])
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
    return;
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
      CtxSegment *segment = &edges[i+1];
      float x, y;
      if (segment->code == CTX_NEW_EDGE)
        {
          prev_x = segment->x0 * 1.0f / CTX_SUBDIV;
          prev_y = segment->y0 * 1.0f / CTX_FULL_AA;
          ctx_move_to (ctx, prev_x, prev_y);
        }
      x = segment->x1 * 1.0f / CTX_SUBDIV;
      y = segment->y1 * 1.0f / CTX_FULL_AA;
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
      temp[0].u32[0] = count;
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
  ctx_rasterizer_close_path (rasterizer);
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
  ctx_rasterizer_close_path (rasterizer);
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
  ctx_rasterizer_rectangle (rasterizer, x, y, 1.0f, 1.0f);
  ctx_rasterizer_fill (rasterizer);
#endif
}

static void
ctx_rasterizer_round_rectangle (CtxRasterizer *rasterizer, float x, float y, float width, float height, float corner_radius)
{
  float aspect  = 1.0f;
  float radius  = corner_radius / aspect;
  float degrees = CTX_PI / 180.0f;

  if (radius > width*0.5f) radius = width/2;
  if (radius > height*0.5f) radius = height/2;

  ctx_rasterizer_close_path (rasterizer);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + radius, radius, 180 * degrees, 270 * degrees, 0);

  ctx_rasterizer_close_path (rasterizer);
}

static void
ctx_rasterizer_process (Ctx *ctx, const CtxCommand *command);

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
    ctx_f (CTX_CONT, rgba[3], 0.0f)
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  ctx_rasterizer_process (ctx, (CtxCommand*)&save_command);
    ctx_rasterizer_process (ctx, (CtxCommand*)&set_color_command[0]);
#if CTX_ENABLE_SHADOW_BLUR
    rasterizer->in_shadow = 1;
#endif
    rasterizer->preserve = 1;
    ctx_rasterizer_stroke (rasterizer);
#if CTX_ENABLE_SHADOW_BLUR
    rasterizer->in_shadow = 0;
#endif
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
    ctx_f (CTX_CONT, rgba[3], 1.0f)
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  ctx_rasterizer_process (ctx, (CtxCommand*)&save_command);

  ctx_rasterizer_process (ctx, (CtxCommand*)&set_color_command);
  rasterizer->preserve = 1;
  rasterizer->in_shadow = 1;
  ctx_rasterizer_fill (rasterizer);
  ctx_rasterizer_reset_soft (rasterizer);
  rasterizer->in_shadow = 0;
  ctx_rasterizer_process (ctx, (CtxCommand*)&restore_command);
}
#endif

static void
ctx_rasterizer_line_dash (CtxRasterizer *rasterizer, unsigned int count, const float *dashes)
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
ctx_rasterizer_process (Ctx *ctx, const CtxCommand *c)
{
  const CtxEntry *entry      = &c->entry;
  CtxRasterizer  *rasterizer = (CtxRasterizer *) ctx->backend;
  CtxState       *state      = rasterizer->state;
  int             clear_clip = 0;

  switch (c->code)
    {
      case CTX_LINE_HEIGHT:
      case CTX_WRAP_LEFT:
      case CTX_WRAP_RIGHT:
      case CTX_LINE_DASH_OFFSET:
      case CTX_STROKE_POS:
      case CTX_FEATHER:
      case CTX_LINE_WIDTH:
      case CTX_SHADOW_BLUR:
      case CTX_SHADOW_OFFSET_X:
      case CTX_SHADOW_OFFSET_Y:
      case CTX_LINE_CAP:
      case CTX_FILL_RULE:
      case CTX_LINE_JOIN:
      case CTX_TEXT_ALIGN:
      case CTX_TEXT_BASELINE:
      case CTX_TEXT_DIRECTION:
      case CTX_GLOBAL_ALPHA:
      case CTX_FONT_SIZE:
      case CTX_MITER_LIMIT:
      case CTX_COLOR_SPACE:
      case CTX_STROKE_SOURCE:
        ctx_interpret_style (state, entry, NULL);
	break;
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
      case CTX_CONIC_GRADIENT:
      case CTX_LINEAR_GRADIENT:
      case CTX_RADIAL_GRADIENT:
        ctx_interpret_style (state, entry, NULL);
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
      case CTX_SET_RGBA_U8:
        ctx_interpret_style (state, entry, NULL);
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
              CtxSegment *segment = &temp[0];
              prev_x = segment->x0 * 1.0f / CTX_SUBDIV;
              prev_y = segment->y0 * 1.0f / CTX_FULL_AA;
              ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
              is_down = 1;
            }

            for (i = start; i < count; i++)
            {
              CtxSegment *segment = &temp[i];
              float x, y;
              if (segment->code == CTX_NEW_EDGE)
                {
                  if (started)
                    {
                      end = i - 1;
                      dash_no = 0;
                      dash_lpos = 0.0;
                      goto foo;
                    }
                  prev_x = segment->x0 * 1.0f / CTX_SUBDIV;
                  prev_y = segment->y0 * 1.0f / CTX_FULL_AA;
                  started = 1;
                  start = i;
                  is_down = 1;
                  ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
                }

again:

              x = segment->x1 * 1.0f / CTX_SUBDIV;
              y = segment->y1 * 1.0f / CTX_FULL_AA;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_hypotf (dx, dy);

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
        ctx_interpret_style (state, entry, NULL);
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
	  int preserve = rasterizer->preserve;
#if CTX_ENABLE_SHADOW_BLUR
        if ((state->gstate.shadow_blur > 0.0f) & (!rasterizer->in_text))
	{
          ctx_rasterizer_shadow_fill (rasterizer);
	}
#endif
        ctx_rasterizer_fill (rasterizer);
	if (preserve)
          ctx_rasterizer_reset_soft (rasterizer);
	else
          ctx_rasterizer_reset (rasterizer);
        }
        break;
      case CTX_START_FRAME:
      case CTX_BEGIN_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLIP:
        ctx_rasterizer_clip (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_close_path (rasterizer);
        break;
      case CTX_IMAGE_SMOOTHING:
        ctx_interpret_style (state, entry, NULL);
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
    case 0: 
    case 1:
	return CTX_ANTIALIAS_NONE;
    case 3:
	return CTX_ANTIALIAS_FAST;
    case 5:
	return CTX_ANTIALIAS_GOOD;
    default:
    case 15:
	return CTX_ANTIALIAS_FULL;
  }
}

static int _ctx_antialias_to_aa (CtxAntialias antialias)
{
  switch (antialias)
  {
    case CTX_ANTIALIAS_NONE: return 1;
    case CTX_ANTIALIAS_FAST: return 3;
    case CTX_ANTIALIAS_GOOD: return 5;
    case CTX_ANTIALIAS_FULL: return 15;
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
