#include "ctx-split.h"

#ifndef __clang__
#if CTX_COMPOSITE_O3
#pragma GCC push_options
#pragma GCC optimize("O3")
#endif
#if CTX_COMPOSITE_O2
#pragma GCC push_options
#pragma GCC optimize("O2")
#endif
#endif

#if CTX_COMPOSITE

#define CTX_FULL_AA 15
#define CTX_REFERENCE 0


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



CTX_INLINE static void
ctx_RGBA8_associate_alpha (uint8_t *u8)
{
#if 1
  uint32_t val = *((uint32_t*)(u8));
  uint32_t a = u8[3];
  uint32_t g = (((val & CTX_RGBA8_G_MASK) * a) >> 8) & CTX_RGBA8_G_MASK;
  uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * a) >> 8) & CTX_RGBA8_RB_MASK;
  *((uint32_t*)(u8)) = g|rb|(a << CTX_RGBA8_A_SHIFT);
#else
  uint32_t a = u8[3];
  u8[0] = (u8[0] * a + 255) >> 8;
  u8[1] = (u8[1] * a + 255) >> 8;
  u8[2] = (u8[2] * a + 255) >> 8;
#endif
}

inline static void
ctx_RGBA8_associate_global_alpha (uint8_t *u8, uint8_t global_alpha)
{
  uint32_t val = *((uint32_t*)(u8));
  uint32_t a = (u8[3] * global_alpha + 255) >> 8;
  uint32_t g = (((val & CTX_RGBA8_G_MASK) * a) >> 8) & CTX_RGBA8_G_MASK;
  uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * a) >> 8) & CTX_RGBA8_RB_MASK;
  *((uint32_t*)(u8)) = g|rb|(a << CTX_RGBA8_A_SHIFT);
}

inline static uint32_t
ctx_RGBA8_associate_global_alpha_u32 (uint32_t val, uint8_t global_alpha)
{
  uint32_t a = ((val>>24) * global_alpha + 255) >> 8;
  uint32_t g = (((val & CTX_RGBA8_G_MASK) * a) >> 8) & CTX_RGBA8_G_MASK;
  uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * a) >> 8) & CTX_RGBA8_RB_MASK;
  return  g|rb|(a << CTX_RGBA8_A_SHIFT);
}

// mixes global alpha in with existing global alpha
inline static uint32_t
ctx_RGBA8_mul_alpha_u32(uint32_t val, uint8_t global_alpha)
{
  uint32_t a = ((val>>24) * global_alpha + 255) >> 8;
  uint32_t g = (((val & CTX_RGBA8_G_MASK) * global_alpha) >> 8) & CTX_RGBA8_G_MASK;
  uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * global_alpha) >> 8) & CTX_RGBA8_RB_MASK;
  return  g|rb|(a << CTX_RGBA8_A_SHIFT);
}

CTX_INLINE static void
ctx_RGBA8_associate_alpha_probably_opaque (uint8_t *u8)
{
  uint32_t a = u8[3];//val>>24;//u8[3];
  if (CTX_UNLIKELY(a!=255))
  {
    u8[0] = (u8[0] * a + 255) >> 8;
    u8[1] = (u8[1] * a + 255) >> 8;
    u8[2] = (u8[2] * a + 255) >> 8;
  }
}

CTX_INLINE static uint32_t ctx_bi_RGBA8 (uint32_t isrc00, uint32_t isrc01, uint32_t isrc10, uint32_t isrc11, uint8_t dx, uint8_t dy)
{
#if 0
#if 0
  uint8_t ret[4];
  uint8_t *src00 = (uint8_t*)&isrc00;
  uint8_t *src10 = (uint8_t*)&isrc10;
  uint8_t *src01 = (uint8_t*)&isrc01;
  uint8_t *src11 = (uint8_t*)&isrc11;
  for (int c = 0; c < 4; c++)
  {
    ret[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                         ctx_lerp_u8 (src10[c], src11[c], dx), dy);
  }
  return  ((uint32_t*)&ret[0])[0];
#else
  return ctx_lerp_RGBA8 (ctx_lerp_RGBA8 (isrc00, isrc01, dx),
                         ctx_lerp_RGBA8 (isrc10, isrc11, dx), dy);
#endif
#else
  uint32_t s0_ga, s0_rb, s1_ga, s1_rb;
  ctx_lerp_RGBA8_split (isrc00, isrc01, dx, &s0_ga, &s0_rb);
  ctx_lerp_RGBA8_split (isrc10, isrc11, dx, &s1_ga, &s1_rb);
  return ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, dy);
#endif
}

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE

inline static int ctx_grad_index (CtxRasterizer *rasterizer, float v)
{
  int ret = (int)(v * (rasterizer->gradient_cache_elements - 1) + 0.5f);
  ret = ctx_maxi (0, ret);
  ret = ctx_mini (rasterizer->gradient_cache_elements-1, ret);
  return ret;
}

inline static int ctx_grad_index_i (CtxRasterizer *rasterizer, int v)
{
  v = v >> 8;
  return ctx_maxi (0, ctx_mini (rasterizer->gradient_cache_elements-1, v));
}

//static void
//ctx_gradient_cache_reset (void)
//{
//  ctx_gradient_cache_valid = 0;
//}
#endif


CTX_INLINE static void
_ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
  float v = x;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  CtxGradient *g = &rasterizer->state->gradient;
  v *= (v>0);
  if (v > 1) { v = 1; }

  if (g->n_stops == 0)
    {
      rgba[0] = rgba[1] = rgba[2] = (int)(v * 255);
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
      int dx = (int)((v - stop->pos) * 255 / (next_stop->pos - stop->pos));
      ((uint32_t*)rgba)[0] = ctx_lerp_RGBA8 (((uint32_t*)stop_rgba)[0],
                                             ((uint32_t*)next_rgba)[0], dx);
      rgba[3]=(rgba[3]*global_alpha_u8+255)>>8;
      if (rasterizer->swap_red_green)
      {
         uint8_t tmp = rgba[0];
         rgba[0] = rgba[2];
         rgba[2] = tmp;
      }
      ctx_RGBA8_associate_alpha (rgba);
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_rgba8 (rasterizer->state, color, rgba);
  if (rasterizer->swap_red_green)
  {
    uint8_t tmp = rgba[0];
    rgba[0] = rgba[2];
    rgba[2] = tmp;
  }
  rgba[3]=(rgba[3]*global_alpha_u8+255)>>8;
  ctx_RGBA8_associate_alpha (rgba);
}

#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer);
#endif

CTX_INLINE static void
ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
#if CTX_GRADIENT_CACHE
  *((uint32_t*)rgba) = *((uint32_t*)(&rasterizer->gradient_cache_u8[ctx_grad_index(rasterizer, x)][0]));
#else
 _ctx_fragment_gradient_1d_RGBA8 (rasterizer, x, y, rgba);
#endif
}
#endif

CTX_INLINE static void
ctx_u8_associate_alpha (int components, uint8_t *u8)
{
  for (int c = 0; c < components-1; c++)
    u8[c] = (u8[c] * u8[components-1] + 255)>>8;
}

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer)
{
  // XXX : todo  make the number of element dynamic depending on length of gradient
  // in device coordinates.

  if (rasterizer->gradient_cache_valid)
    return;
  

  {
    CtxSource *source = &rasterizer->state->gstate.source_fill;
    float length = 100;
    if (source->type == CTX_SOURCE_LINEAR_GRADIENT)
    {
       length = source->linear_gradient.length;
    }
    else
    if (source->type == CTX_SOURCE_RADIAL_GRADIENT)
    {
       length = ctx_maxf (source->radial_gradient.r1, source->radial_gradient.r0);
    }
  //  length = CTX_GRADIENT_CACHE_ELEMENTS;
  {
     float u = length; float v = length;
     const CtxMatrix *m = &rasterizer->state->gstate.transform;
     //CtxMatrix *transform = &source->transform;
     //
     //  combine with above source transform?
     _ctx_matrix_apply_transform (m, &u, &v);
     length = ctx_maxf (u, v);
  }
  
    rasterizer->gradient_cache_elements = ctx_mini ((int)length, CTX_GRADIENT_CACHE_ELEMENTS);
  }

  for (int u = 0; u < rasterizer->gradient_cache_elements; u++)
  {
    float v = u / (rasterizer->gradient_cache_elements - 1.0f);
    _ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0f, &rasterizer->gradient_cache_u8[u][0]);
    //*((uint32_t*)(&rasterizer->gradient_cache_u8_a[u][0]))= *((uint32_t*)(&rasterizer->gradient_cache_u8[u][0]));
    //memcpy(&rasterizer->gradient_cache_u8_a[u][0], &rasterizer->gradient_cache_u8[u][0], 4);
    //ctx_RGBA8_associate_alpha (&rasterizer->gradient_cache_u8_a[u][0]);
  }
  rasterizer->gradient_cache_valid = 1;
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
      rgba[0] = rgba[1] = rgba[2] = (int)(v * 255);
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
      int dx = (int)((v - stop->pos) * 255 / (next_stop->pos - stop->pos));
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
  float global_alpha = rasterizer->state->gstate.global_alpha_f;
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
      int dx = (int)((v - stop->pos) / (next_stop->pos - stop->pos));
      for (int c = 0; c < 4; c++)
        { rgba[c] = ctx_lerpf (stop_rgba[c], next_rgba[c], dx); }
      rgba[3] *= global_alpha;
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_rgba (rasterizer->state, color, rgba);
  rgba[3] *= global_alpha;
}
#endif

static void
ctx_fragment_image_RGBA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dw)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  uint8_t is_assoc = (buffer->format->pixel_format == CTX_FORMAT_RGBA8 ||
                      buffer->format->pixel_format == CTX_FORMAT_BGRA8);

  int width = buffer->width;
  int height = buffer->height;
  for (int i = 0; i < count; i ++)
  {

  int u = (int)x;
  int v = (int)y;
  if ( (u < 0) | (v < 0) | (u >= width) | (v >= height))
      *((uint32_t*)(rgba)) = 0;
  else
    {
      int bpp = buffer->format->bpp/8;
      if (rasterizer->state->gstate.image_smoothing)
      {
        uint8_t *src00 = (uint8_t *) buffer->data;
        src00 += v * buffer->stride + u * bpp;
        uint8_t *src01 = src00;
        if ( u + 1 < width)
        {
          src01 = src00 + bpp;
        }
        uint8_t *src11 = src01;
        uint8_t *src10 = src00;
        if ( v + 1 < height)
        {
          src10 = src00 + buffer->stride;
          src11 = src01 + buffer->stride;
        }
        float dx = (x-(int)(x)) * 255.9f;
        float dy = (y-(int)(y)) * 255.9f;
        uint8_t dxb = (uint8_t)dx;
        uint8_t dyb = (uint8_t)dy;
  
        switch (bpp)
        {
          case 1:
            rgba[0] = rgba[1] = rgba[2] = ctx_lerp_u8 (ctx_lerp_u8 (src00[0], src01[0], dxb),
                                   ctx_lerp_u8 (src10[0], src11[0], dxb), dyb);
            rgba[3] = global_alpha_u8;
            break;
          case 2:
            rgba[0] = rgba[1] = rgba[2] = ctx_lerp_u8 (ctx_lerp_u8 (src00[0], src01[0], dxb),
                                   ctx_lerp_u8 (src10[0], src11[0], dxb), dyb);
            rgba[3] = ctx_lerp_u8 (ctx_lerp_u8 (src00[1], src01[1], dxb),
                                   ctx_lerp_u8 (src10[1], src11[1], dxb), dyb);
            rgba[3] = (rgba[3] * global_alpha_u8) / 255;
            break;
          case 3:
            for (int c = 0; c < bpp; c++)
              { rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dxb),
                                       ctx_lerp_u8 (src10[c], src11[c], dxb), dyb);
                      
              }
            rgba[3]=global_alpha_u8;
            break;
          break;
          case 4:
            if (is_assoc)
            {
              if (global_alpha_u8==255) {
                for (int c = 0; c < bpp; c++)
                  rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dxb),
                                          ctx_lerp_u8 (src10[c], src11[c], dxb), dyb);
              }
              else
                for (int c = 0; c < bpp; c++)
                  rgba[c] = (ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dxb),
                                          ctx_lerp_u8 (src10[c], src11[c], dxb), dyb) * global_alpha_u8) / 255;
            }
            else
            {
              for (int c = 0; c < bpp; c++)
              { rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dxb),
                                       ctx_lerp_u8 (src10[c], src11[c], dxb), dyb);
                      
              }
              rgba[3] = (rgba[3] * global_alpha_u8) / 255;
            }
        }
      }
      else
      {
      uint8_t *src = (uint8_t *) buffer->data;
      src += v * buffer->stride + u * bpp;
      switch (bpp)
        {
          case 1:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[0]; }
            rgba[3] = global_alpha_u8;
            break;
          case 2:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[0]; }
            rgba[3] = src[1];
            rgba[3] = (rgba[3] * global_alpha_u8) / 255;
            break;
          case 3:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[c]; }
            rgba[3] = global_alpha_u8;
            break;
          case 4:
            if (is_assoc)
            {
              if (global_alpha_u8==255)
                for (int c = 0; c < 4; c++)
                  rgba[c] = src[c];
              else
                for (int c = 0; c < 4; c++)
                  rgba[c] = (src[c] * global_alpha_u8)/255;
            }
            else
            {
              for (int c = 0; c < 4; c++)
                { rgba[c] = src[c]; }
              rgba[3] = (rgba[3] * global_alpha_u8) / 255;
            }
            break;
        }

      }
      if (rasterizer->swap_red_green)
      {
        uint8_t tmp = rgba[0];
        rgba[0] = rgba[2];
        rgba[2] = tmp;
      }
    }
    if (!is_assoc)
      ctx_RGBA8_associate_alpha_probably_opaque (rgba);
    rgba += 4;
    x += dx;
    y += dy;
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

#if 0
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
#endif

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
  if (ralpha != 0.0f) ralpha = 1.0f/ralpha;

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


static inline void ctx_swap_red_green_u8 (void *data)
{
  uint8_t *rgba = (uint8_t*)data;
  uint8_t tmp = rgba[0];
  rgba[0] = rgba[2];
  rgba[2] = tmp;
}

static void
ctx_fragment_swap_red_green_u8 (void *out, int count)
{
  uint8_t *rgba = (uint8_t*)out;
  for (int x = 0; x < count; x++)
  {
    ctx_swap_red_green_u8 (rgba);
    rgba += 4;
  }
}

/**** rgb8 ***/

static void
ctx_fragment_image_rgb8_RGBA8_box (CtxRasterizer *rasterizer,
                                   float x, float y, float z,
                                   void *out, int count, float dx, float dy, float dz)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  int width = buffer->width;
  int height = buffer->height;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
  int dim = (int)((1.0f / factor) / 3);

  int i = 0;

  for (; i < count && (x - dim< 0 || y - dim < 0 || x + dim >= height || y + dim >= height); i++)
  {
    *((uint32_t*)(rgba))=0;
    rgba += 4;
    x += dx;
    y += dy;
  }

  for (; i < count && !(
       x - dim < 0 || y - dim < 0 ||
       x + dim >= width ||
       y + dim >= height); i++)
  {

  int u = (int)x;
  int v = (int)y;
    {
      int bpp = 3;
      rgba[3]=global_alpha_u8; // gets lost
          uint64_t sum[4]={0,0,0,0};
          int count = 0;

          {
            for (int ov = - dim; ov <= dim; ov++)
            {
              uint8_t *src = (uint8_t *) buffer->data + bpp * ((v+ov) * width + (u - dim));
              for (int ou = - dim; ou <= dim; ou++)
              {
                for (int c = 0; c < bpp; c++)
                  sum[c] += src[c];
                count ++;
                src += bpp;
              }

            }
          }

          int recip = 65536/count;
          for (int c = 0; c < bpp; c++)
            rgba[c] = sum[c] * recip >> 16;
          ctx_RGBA8_associate_alpha_probably_opaque (rgba);
    }
    rgba += 4;
    x += dx;
    y += dy;
  }

  for (; i < count; i++)
  {
    *((uint32_t*)(rgba))= 0;
    rgba += 4;
  }
}

#define CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(frag) \
static void \
frag##_swap_red_green (CtxRasterizer *rasterizer,\
                       float x, float y, float z,\
                       void *out, int count, float dx, float dy, float dz)\
{\
  frag (rasterizer, x, y, z, out, count, dx, dy, dz);\
  ctx_fragment_swap_red_green_u8 (out, count);\
}



static inline void
ctx_RGBA8_apply_global_alpha_and_associate (CtxRasterizer *rasterizer,
                                         uint8_t *buf, int count)
{
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  uint8_t *rgba = (uint8_t *) buf;
  if (global_alpha_u8 != 255)
  {
    for (int i = 0; i < count; i++)
    {
      ctx_RGBA8_associate_global_alpha (rgba, global_alpha_u8);
      rgba += 4;
    }
  }
  else
  {
    for (int i = 0; i < count; i++)
    {
      ctx_RGBA8_associate_alpha_probably_opaque (rgba);
      rgba += 4;
    }
  }
}

#if CTX_FRAGMENT_SPECIALIZE

static void
ctx_fragment_image_rgb8_RGBA8_nearest (CtxRasterizer *rasterizer,
                                       float x, float y, float z,
                                       void *out, int scount,
                                       float dx, float dy, float dz);
static inline void
ctx_fragment_image_rgb8_RGBA8_bi (CtxRasterizer *rasterizer,
                                  float x, float y, float z,
                                  void *out, int scount,
                                  float dx, float dy, float dz)
{
  ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer,
                                         x, y, z,
                                         out, scount,
                                         dx, dy, dz);
  return;
}

static void
ctx_fragment_image_rgb8_RGBA8_nearest (CtxRasterizer *rasterizer,
                                       float x, float y, float z,
                                       void *out, int scount,
                                       float dx, float dy, float dz)
{
  unsigned int count = scount;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  const int bwidth = buffer->width;
  const int bheight = buffer->height;
  unsigned int i = 0;
  uint8_t *data = ((uint8_t*)buffer->data);

  int yi_delta = (int)(dy * 65536);
  int xi_delta = (int)(dx * 65536);
  int zi_delta = (int)(dz * 65536);
  int32_t yi = (int)(y * 65536);
  int32_t xi = (int)(x * 65536);
  int32_t zi = (int)(z * 65536);
  {
    int32_t u1 = xi + xi_delta* (count-1);
    int32_t v1 = yi + yi_delta* (count-1);
    int32_t z1 = zi + zi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      float z_recip = (z1!=0) * (1.0f/z1);
      if ((u1*z_recip) <0 ||
          (v1*z_recip) <0 ||
          (u1*z_recip) >= (bwidth) - 1 ||
          (v1*z_recip) >= (bheight) - 1)
      {
        *edst-- = 0;
        count --;
        u1 -= xi_delta;
        v1 -= yi_delta;
        z1 -= zi_delta;
      }
      else break;
    }
  }

  for (i= 0; i < count; i ++)
  {
    float z_recip = (zi!=0) * (1.0f/zi);
    int u = (int)(xi * z_recip);
    int v = (int)(yi * z_recip);
    if ( u  <= 0 || v  <= 0 || u+1 >= bwidth-1 || v+1 >= bheight-1)
    {
      *((uint32_t*)(rgba))= 0;
    }
    else
      break;
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;
  }

  while (i < count)
  {
    float z_recip = (zi!=0) * (1.0f/zi);
    int u = (int)(xi * z_recip);
    int v = (int)(yi * z_recip);
    for (unsigned int c = 0; c < 3; c++)
      rgba[c] = data[(bwidth *v +u)*3+c];
    rgba[3] = global_alpha_u8;
    ctx_RGBA8_associate_alpha_probably_opaque (rgba);
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;
    i++;
  }
}



CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgb8_RGBA8_box)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgb8_RGBA8_bi)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgb8_RGBA8_nearest)


static inline void
ctx_fragment_image_rgb8_RGBA8 (CtxRasterizer *rasterizer,
                               float x,
                               float y,
                               float z,
                               void *out, int count, float dx, float dy, float dz)
{
  if (rasterizer->swap_red_green)
  {
    if (rasterizer->state->gstate.image_smoothing)
    {
      float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
      if (factor <= 0.50f)
        ctx_fragment_image_rgb8_RGBA8_box_swap_red_green (rasterizer,x,y,z,out,count,dx,dy,dz);
  #if CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
      else if ((factor > 0.99f) & (factor < 1.01f))
        ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (rasterizer,x,y,z,
                                                            out,count,dx,dy,dz);
  #endif
      else
        ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green (rasterizer,x,y,z,
                                                         out,count, dx, dy, dz);
    }
    else
    {
      ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (rasterizer,x,y,z,
                                                            out,count,dx,dy,dz);
    }
  }
  else
  {
    if (rasterizer->state->gstate.image_smoothing)
    {
      float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
      if (factor <= 0.50f)
        ctx_fragment_image_rgb8_RGBA8_box (rasterizer,x,y,z,out,
                                           count,dx,dy,dz);
  #if CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
      else if ((factor > 0.99f) & (factor < 1.01f))
        ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer, x, y, z, out, count, dx, dy, dz);
  #endif
      else
        ctx_fragment_image_rgb8_RGBA8_bi (rasterizer,x,y,z,out,count,dx,dy,dz);
    }
    else
    {
        ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer,x,y,z,out,
                                               count,dx,dy, dz);
    }
  }
}


/************** rgba8 */

static void
ctx_fragment_image_rgba8_RGBA8_box (CtxRasterizer *rasterizer,
                                    float x, float y, float z,
                                    void *out, int count, float dx, float dy, float dz)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  int width = buffer->width;
  int height = buffer->height;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
  int dim = (int)((1.0f / factor) / 3);

  int i = 0;

  for (; i < count && (x - dim< 0 || y - dim < 0 || x + dim >= height || y + dim >= height); i++)
  {
    *((uint32_t*)(rgba))=0;
    rgba += 4;
    x += dx;
    y += dy;
  }

  for (; i < count && !(
       x - dim < 0 || y - dim < 0 ||
       x + dim >= width ||
       y + dim >= height); i++)
  {

  int u = (int)x;
  int v = (int)y;
    {
      int bpp = 4;
          uint64_t sum[4]={0,0,0,0};
          int count = 0;

          {
            for (int ov = - dim; ov <= dim; ov++)
            {
              uint8_t *src = (uint8_t *) buffer->data + bpp * ((v+ov) * width + (u - dim));
              for (int ou = - dim; ou <= dim; ou++)
              {
                for (int c = 0; c < bpp; c++)
                  sum[c] += src[c];
                count ++;
                src += bpp;
              }

            }
          }

          int recip = 65536/count;
          for (int c = 0; c < bpp; c++)
            rgba[c] = sum[c] * recip >> 16;
          rgba[3]=rgba[3]*global_alpha_u8/255; // gets lost
          ctx_RGBA8_associate_alpha_probably_opaque (rgba);
    }
    rgba += 4;
    x += dx;
    y += dy;
  }


  for (; i < count; i++)
  {
    *((uint32_t*)(rgba))= 0;
    rgba += 4;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}


static void
ctx_fragment_image_rgba8_RGBA8_nearest_copy (CtxRasterizer *rasterizer,
                                             float x, float y, float z,
                                             void *out, int scount, float dx, float dy, float dz)
{
  unsigned int count = scount;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = 
     g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  uint32_t *dst = (uint32_t*)out;
#if 0
  for (int i = 0; i < scount; i++)
          dst[i] = (255<<24)+128;
  return;
#endif
  int bwidth  = buffer->width;
  int bheight = buffer->height;
  int u = (int)x;
  int v = (int)y;

  uint32_t *src = ((uint32_t*)buffer->data) + bwidth * v + u;
  if (CTX_UNLIKELY(!((v >= 0) & (v < bheight))))
  {
    for (unsigned i = 0 ; i < count; i++)
      *dst++ = 0;
    return;
  }

#if 1
  int pre = ctx_mini(ctx_maxi(-u,0), count);
  memset (dst, 0, pre);
  dst +=pre;
  count-=pre;
  src+=pre;
  u+=pre;
#else
  while (count && !(u >= 0))
  {
    *dst++ = 0;
    src ++;
    u++;
    count--;
  }
#endif

  int limit = ctx_mini (count, bwidth - u);
  if (limit>0)
  {
    memcpy (dst, src, limit * 4);
    dst += limit;
  }
  memset (dst, 0, count - limit);
}

static void
ctx_fragment_image_rgba8sepA_RGBA8_nearest_copy (CtxRasterizer *rasterizer,
                                                 float x, float y, float z,
                                                 void *out, int scount, float dx, float dy, float dz)
{
  ctx_fragment_image_rgba8_RGBA8_nearest_copy (rasterizer, x, y, z, out, scount, dx, dy, dz);
  ctx_RGBA8_apply_global_alpha_and_associate (rasterizer, (uint8_t*)out, scount);
}

static void
ctx_fragment_image_rgba8_RGBA8_nearest_copy_repeat (CtxRasterizer *rasterizer,
                                                    float x, float y, float z,
                                                    void *out, int count, float dx, float dy, float dz)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = 
     g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  uint32_t *dst = (uint32_t*)out;
  int bwidth  = buffer->width;
  int bheight = buffer->height;
  int u = (int)x;
  int v = (int)y;
  if (v < 0) v += bheight * 8192;
  if (u < 0) u += bwidth * 8192;
  v %= bheight;
  u %= bwidth;

  uint32_t *src = ((uint32_t*)buffer->data) + bwidth * v;

  while (count)
  {
     int chunk = ctx_mini (bwidth - u, count);
     memcpy (dst, src + u, chunk * 4);
     dst += chunk;
     count -= chunk;
     u = (u + chunk) % bwidth;
  }
}

static CTX_INLINE int
_ctx_coords_restrict (CtxExtend extend,
                      int *u, int *v,
                      int bwidth, int bheight)
{
  switch (extend)
  {
    case CTX_EXTEND_REPEAT:
      if(u)
      {
         while (*u < 0) *u += bwidth * 4096;   // XXX need better way to do this
         *u  %= bwidth;
      }
      if(v)
      {
        while (*v < 0) *v += bheight * 4096;
        *v  %= bheight;
      }
      return 1;
    case CTX_EXTEND_REFLECT:
      if (u)
      {
      while (*u < 0) *u += bwidth * 4096;   // XXX need better way to do this
      *u  %= (bwidth*2);

      *u = (*u>=bwidth) * (bwidth*2 - *u) +
           (*u<bwidth) * *u;
      }

      if (v)
      {
      while (*v < 0) *v += bheight * 4096;
      *v  %= (bheight*2);
      *v = (*v>=bheight) * (bheight*2 - *v) +
           (*v<bheight) * *v;
      }

      return 1;
    case CTX_EXTEND_PAD:
      if (u)*u = ctx_mini (ctx_maxi (*u, 0), bwidth-1);
      if (v)*v = ctx_mini (ctx_maxi (*v, 0), bheight-1);
      return 1;
    case CTX_EXTEND_NONE:
      if (u && ((*u < 0) | (*u >= bwidth))) return 0;
      if (v && ((*v < 0) | (*v >= bheight))) return 0;
      return 1;
  }
  return 0;
}

static void
ctx_fragment_image_rgba8_RGBA8_nearest_affine (CtxRasterizer *rasterizer,
                                               float x, float y, float z,
                                               void *out, int scount, float dx, float dy, float dz)
{
  unsigned int count = scount;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  CtxExtend extend = rasterizer->state->gstate.extend;
  const int bwidth = buffer->width;
  const int bheight = buffer->height;
  unsigned int i = 0;
  uint32_t *data = ((uint32_t*)buffer->data);

  int yi_delta = (int)(dy * 65536);
  int xi_delta = (int)(dx * 65536);
  int32_t yi = (int)(y * 65536);
  int32_t xi = (int)(x * 65536);
  switch (extend){
          case CTX_EXTEND_NONE:
                  {

    int32_t u1 = xi + xi_delta* (count-1);
    int32_t v1 = yi + yi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      if (((u1>>16) <0) |
          ((v1>>16) <0) |
          ((u1>>16) >= (bwidth) - 1) |
          ((v1>>16) >= (bheight) - 1))
      {
        *edst-- = 0;
        count --;
        u1 -= xi_delta;
        v1 -= yi_delta;
      }
      else break;
    }

  for (i= 0; i < count; i ++)
  {
    int u = xi >> 16;
    int v = yi >> 16;
    if ((u  <= 0) | (v  <= 0) | (u+1 >= bwidth-1) | (v+1 >= bheight-1))
    {
      *((uint32_t*)(rgba))= 0;
    }
    else break;
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;
  }

  if (global_alpha_u8 == 255)
  while (i < count)
  {
    int u = xi >> 16;
    int v = yi >> 16;
    ((uint32_t*)(&rgba[0]))[0] = data[bwidth *v +u];
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;
    i++;
  }
  else
  while (i < count)
  {
    int u = xi >> 16;
    int v = yi >> 16;
    ((uint32_t*)(&rgba[0]))[0] =
      ctx_RGBA8_mul_alpha_u32 (data[bwidth *v +u], global_alpha_u8);
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;
    i++;
  }
                  }
  break;
          default:
  if (global_alpha_u8 == 255)
    while (i < count)
    {
      int u = xi >> 16;
      int v = yi >> 16;
      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      ((uint32_t*)(&rgba[0]))[0] = data[bwidth *v +u];
      xi += xi_delta;
      yi += yi_delta;
      rgba += 4;
      i++;
    }
   else
    while (i < count)
    {
      int u = xi >> 16;
      int v = yi >> 16;
      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      ((uint32_t*)(&rgba[0]))[0] =
        ctx_RGBA8_mul_alpha_u32 (data[bwidth *v +u], global_alpha_u8);
      xi += xi_delta;
      yi += yi_delta;
      rgba += 4;
      i++;
    }
   break;
  }
}


static void
ctx_fragment_image_rgba8_RGBA8_nearest_scale (CtxRasterizer *rasterizer,
                                              float x, float y, float z,
                                              void *out, int scount, float dx, float dy, float dz)
{
  unsigned int count = scount;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  CtxBuffer *buffer = NULL;
  CtxExtend  extend = rasterizer->state->gstate.extend;
  uint32_t *src = NULL;
  buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  int ideltax = (int)(dx * 65536);
  uint32_t *dst = (uint32_t*)out;
  int bwidth  = buffer->width;
  int bheight = buffer->height;
  int bbheight = bheight << 16;
  int bbwidth  = bwidth << 16;
//  x += 0.5f;
//  y += 0.5f;

  src = (uint32_t*)buffer->data;
  //if (!src){ fprintf (stderr, "eeek bailing in nearest fragment\n"); return;};

  {
    unsigned int i = 0;
    int32_t ix = (int)(x * 65536);
    int32_t iy = (int)(y * 65536);

    if (extend == CTX_EXTEND_NONE)
    {
    int32_t u1 = ix + ideltax * (count-1);
    int32_t v1 = iy;
    uint32_t *edst = ((uint32_t*)out)+count - 1;
    for (; i < count; )
    {
      if ((u1 <0) | (v1 < 0) | (u1 >= bbwidth) | (v1 >= bbheight))
      {
        *edst-- = 0;
        count --;
        u1 -= ideltax;
      }
      else break;
    }

    for (i = 0; i < count; i ++)
    {
      if ((ix < 0) | (iy < 0) | (ix >= bbwidth)  | (iy >= bbheight))
      {
        *dst++ = 0;
        x += dx;
        ix += ideltax;
      }
      else break;
    }

      int v = iy >> 16;
      int u = ix >> 16;
      int o = (v)*bwidth;
      if (global_alpha_u8==255)
        for (; i < count; i ++)
        {
          u = ix >> 16;
          *dst++ = src[o + (u)];
          ix += ideltax;
        }
      else
        for (; i < count; i ++)
        {
          u = ix >> 16;
          *dst++ = ctx_RGBA8_mul_alpha_u32 (src[o + (u)], global_alpha_u8);
          ix += ideltax;
        }
    }
    else
    {

      int v = iy >> 16;
      int u = ix >> 16;
      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      int o = (v)*bwidth;
      if (global_alpha_u8==255)
      for (; i < count; i ++)
      {
        u = ix >> 16;
        _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
        *dst++ = src[o + (u)];
        ix += ideltax;
      }
      else
      {
        u = ix >> 16;
        _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
        *dst++ = ctx_RGBA8_mul_alpha_u32 (src[o + (u)], global_alpha_u8);
        ix += ideltax;
      }
    }
  }
}

static void
ctx_fragment_image_rgba8_RGBA8_nearest_generic (CtxRasterizer *rasterizer,
                                                float x, float y, float z,
                                                void *out, int scount, float dx, float dy, float dz)
{
  unsigned int count = scount;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  CtxExtend extend = rasterizer->state->gstate.extend;
  const int bwidth = buffer->width;
  const int bheight = buffer->height;
  unsigned int i = 0;
  uint32_t *data = ((uint32_t*)buffer->data);

  int yi_delta = (int)(dy * 65536);
  int xi_delta = (int)(dx * 65536);
  int zi_delta = (int)(dz * 65536);
  int32_t yi = (int)(y * 65536);
  int32_t xi = (int)(x * 65536);
  int32_t zi = (int)(z * 65536);
  switch (extend){
          case CTX_EXTEND_NONE:
                  {

    int32_t u1 = xi + xi_delta* (count-1);
    int32_t v1 = yi + yi_delta* (count-1);
    int32_t z1 = zi + zi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      float z_recip = (z1!=0) * (1.0f/z1);

      if (((u1*z_recip) <0) |
          ((v1*z_recip) <0) |
          ((u1*z_recip) >= (bwidth) - 1) |
          ((v1*z_recip) >= (bheight) - 1))
      {
        *edst-- = 0;
        count --;
        u1 -= xi_delta;
        v1 -= yi_delta;
        z1 -= zi_delta;
      }
      else break;
    }

  for (i= 0; i < count; i ++)
  {
    float z_recip = (zi!=0) * (1.0f/zi);
    int u = (int)(xi * z_recip);
    int v = (int)(yi * z_recip);
    if ( (u <= 0) | (v  <= 0) | (u+1 >= bwidth-1) | (v+1 >= bheight-1))
    {
      *((uint32_t*)(rgba))= 0;
    }
    else
      break;
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;
  }

  if (global_alpha_u8!=255)
  while (i < count)
  {
    float z_recip = (zi!=0) * (1.0f/zi);
    int u = (int)(xi * z_recip);
    int v = (int)(yi * z_recip);
    ((uint32_t*)(&rgba[0]))[0] =
      ctx_RGBA8_mul_alpha_u32 (data[bwidth *v +u], global_alpha_u8);
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;
    i++;
  }
  else
  while (i < count)
  {
    float z_recip = (zi!=0) * (1.0f/zi);
    int u = (int)(xi * z_recip);
    int v = (int)(yi * z_recip);
    ((uint32_t*)(&rgba[0]))[0] = data[bwidth *v +u];
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;
    i++;
  }
                  }
  break;
  default:
    if (global_alpha_u8!=255)
    while (i < count)
    {
      float z_recip = (zi!=0) * (1.0f/zi);
      int u = (int)(xi * z_recip);
      int v = (int)(yi * z_recip);
      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      ((uint32_t*)(&rgba[0]))[0] =
        ctx_RGBA8_mul_alpha_u32 (data[bwidth *v +u], global_alpha_u8);
      xi += xi_delta;
      yi += yi_delta;
      zi += zi_delta;
      rgba += 4;
      i++;
    }
    else
    while (i < count)
    {
      float z_recip = (zi!=0) * (1.0f/zi);
      int u = (int)(xi * z_recip);
      int v = (int)(yi * z_recip);
      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      ((uint32_t*)(&rgba[0]))[0] = data[bwidth *v +u];
      xi += xi_delta;
      yi += yi_delta;
      zi += zi_delta;
      rgba += 4;
      i++;
    }
    break;
  }
}

static void
ctx_fragment_image_rgba8_RGBA8_nearest (CtxRasterizer *rasterizer,
                                   float x, float y, float z,
                                   void *out, int icount, float dx, float dy, float dz)
{
  unsigned int count = icount;
  CtxExtend extend = rasterizer->state->gstate.extend;
  if ((z == 1.0f) & (dz == 0.0f)) // this also catches other constant z!
  {
    if ((dy == 0.0f) & (dx == 1.0f) & (extend == CTX_EXTEND_NONE))
      ctx_fragment_image_rgba8_RGBA8_nearest_copy (rasterizer, x, y, z, out, count, dx, dy, dz);
    else
      ctx_fragment_image_rgba8_RGBA8_nearest_affine (rasterizer, x, y, z, out, count, dx, dy, dz);
  }
  else
  {
    ctx_fragment_image_rgba8_RGBA8_nearest_generic (rasterizer, x, y, z, out, count, dx, dy, dz);
  }
}



static inline void
ctx_fragment_image_rgba8_RGBA8_bi_scale_with_alpha (CtxRasterizer *rasterizer,
                                                    float x, float y, float z,
                                                    void *out, int scount, float dx, float dy, float dz, uint8_t global_alpha_u8)
{
    uint32_t count = scount;
    x -= 0.5f;
    y -= 0.5f;
    uint8_t *rgba = (uint8_t *) out;
    CtxSource *g = &rasterizer->state->gstate.source_fill;
    CtxExtend  extend = rasterizer->state->gstate.extend;
    CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
    const int bwidth = buffer->width;
    const int bheight = buffer->height;
    unsigned int i = 0;

    if (!extend)
    {
    if (!((y >= 0) & (y < bheight)))
    {
      uint32_t *dst = (uint32_t*)rgba;
      for (i = 0 ; i < count; i++)
        *dst++ = 0;
      return;
    }
    }

    //x+=1; // XXX off by one somewhere? ,, needed for alignment with nearest

    int32_t yi = (int)(y * 65536);
    int32_t xi = (int)(x * 65536);

    int xi_delta = (int)(dx * 65536);

    if (!extend)
    {
    int32_t u1 = xi + xi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      if ((u1 <0) | (u1 +65536 >= (bwidth<<16)))
    {
      *edst-- = 0;
      count --;
      u1 -= xi_delta;
    }
    else break;
  }
    for (i= 0; i < count; i ++)
    {
      int u = xi >> 16;
      if ((u < 0) | (u >= bwidth-1))
      {
        *((uint32_t*)(rgba))= 0;
        xi += xi_delta;
        rgba += 4;
      }
      else
        break;
    }
    }

 
  int v = yi >> 16;


  int dv = (yi >> 8) & 0xff;

  int u = xi >> 16;

  int v1 = v+1;

  _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
  _ctx_coords_restrict (extend, NULL, &v1, bwidth, bheight);

  uint32_t *data = ((uint32_t*)buffer->data) + bwidth * v;
  uint32_t *ndata = data + bwidth * !((!extend) & (v1 > bheight-1));

  if (extend)
  {
    if (xi_delta == 65536)
    {
      uint32_t *src0 = data, *src1 = ndata;
      uint32_t s1_ga = 0, s1_rb = 0;
      int du = (xi >> 8) & 0xff;

      src0 = data + u;
      src1 = ndata + u;
      ctx_lerp_RGBA8_split (src0[0],src1[0], dv, &s1_ga, &s1_rb);
  
      for (; i < count; i ++)
      {
        uint32_t s0_ga = s1_ga;
        uint32_t s0_rb = s1_rb; 
        _ctx_coords_restrict (extend, &u, NULL, bwidth, bheight);
        ctx_lerp_RGBA8_split (src0[1],src1[1], dv, &s1_ga, &s1_rb);
        ((uint32_t*)(&rgba[0]))[0] = 
          ctx_RGBA8_mul_alpha_u32 (
                  ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, du), global_alpha_u8);
        rgba += 4;
        u++;
        src0 ++;
        src1 ++;
      }
    }
    else
    {
      uint32_t s0_ga = 0, s1_ga = 0, s0_rb = 0, s1_rb = 0;
      int prev_u = -1000;
      for (; (i < count); i++)
      {
        if (prev_u != u)
        {
          if (prev_u == u-1)
          {
            s0_ga = s1_ga;
            s0_rb = s1_rb;
            ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          }
          else
          {
            ctx_lerp_RGBA8_split (data[u],ndata[u], dv, &s0_ga, &s0_rb);
            ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          }
          prev_u = u;
        }
        ((uint32_t*)(&rgba[0]))[0] = 
          ctx_RGBA8_mul_alpha_u32 (
                  ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, (xi>>8)), global_alpha_u8);
        rgba += 4;
        u = (xi+=xi_delta) >> 16;
        _ctx_coords_restrict (extend, &u, NULL, bwidth, bheight);
      }
    }
  }
  else
  {
    if (xi_delta == 65536)
    {
      uint32_t *src0 = data, *src1 = ndata;
      uint32_t s1_ga = 0, s1_rb = 0;
      int du = (xi >> 8) & 0xff;
  
      src0 = data + u;
      src1 = ndata + u;
      ctx_lerp_RGBA8_split (src0[0],src1[0], dv, &s1_ga, &s1_rb);
  
      for (; i < count; i ++)
      {
        uint32_t s0_ga = s1_ga;
        uint32_t s0_rb = s1_rb;
        ctx_lerp_RGBA8_split (src0[1],src1[1], dv, &s1_ga, &s1_rb);
        ((uint32_t*)(&rgba[0]))[0] = 
          ctx_RGBA8_mul_alpha_u32 (
                  ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, du), global_alpha_u8);
        rgba += 4;
        u++;
        src0 ++;
        src1 ++;
      }
    }
    else
    {
      uint32_t s0_ga = 0, s1_ga = 0, s0_rb = 0, s1_rb = 0;
      int prev_u = -1000;
      for (; (i < count); i++)
      {
        if (prev_u != u)
        {
          if (prev_u == u-1)
          {
            s0_ga = s1_ga;
            s0_rb = s1_rb;
            ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          }
          else
          {
            ctx_lerp_RGBA8_split (data[u],ndata[u], dv, &s0_ga, &s0_rb);
            ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          }
          prev_u = u;
        }
        ((uint32_t*)(&rgba[0]))[0] = 
          ctx_RGBA8_mul_alpha_u32 (
                  ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, (xi>>8)), global_alpha_u8);
        rgba += 4;
        u = (xi+=xi_delta) >> 16;
      }
    }
  }
}

static inline void
ctx_fragment_image_rgba8_RGBA8_bi_scale (CtxRasterizer *rasterizer,
                                         float x, float y, float z,
                                         void *out, int scount, float dx, float dy, float dz)
{
    uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
    if (global_alpha_u8 != 255)
    {
      ctx_fragment_image_rgba8_RGBA8_bi_scale_with_alpha (rasterizer,
                                         x, y, z,
                                         out, scount, dx, dy, dz, global_alpha_u8);
      return;
    }
    uint32_t count = scount;
    x -= 0.5f;
    y -= 0.5f;
    uint8_t *rgba = (uint8_t *) out;
    CtxSource *g = &rasterizer->state->gstate.source_fill;
    CtxExtend  extend = rasterizer->state->gstate.extend;
    CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
    const int bwidth = buffer->width;
    const int bheight = buffer->height;
    unsigned int i = 0;

    if (!extend)
    {
    if (!((y >= 0) & (y < bheight)))
    {
      uint32_t *dst = (uint32_t*)rgba;
      for (i = 0 ; i < count; i++)
        *dst++ = 0;
      return;
    }
    }

    //x+=1; // XXX off by one somewhere? ,, needed for alignment with nearest

    int32_t yi = (int)(y * 65536);
    int32_t xi = (int)(x * 65536);

    int xi_delta = (int)(dx * 65536);

    if (!extend)
    {
    int32_t u1 = xi + xi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      if ((u1 <0) | (u1 +65536 >= (bwidth<<16)))
    {
      *edst-- = 0;
      count --;
      u1 -= xi_delta;
    }
    else break;
  }
    for (i= 0; i < count; i ++)
    {
      int u = xi >> 16;
      if ((u < 0) | (u >= bwidth-1))
      {
        *((uint32_t*)(rgba))= 0;
        xi += xi_delta;
        rgba += 4;
      }
      else
        break;
    }
    }

 
  int v = yi >> 16;


  int dv = (yi >> 8) & 0xff;

  int u = xi >> 16;

  int v1 = v+1;

  _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
  _ctx_coords_restrict (extend, NULL, &v1, bwidth, bheight);

  uint32_t *data = ((uint32_t*)buffer->data) + bwidth * v;
  uint32_t *ndata = data + bwidth * !((!extend) & (v1 > bheight-1));

  if (extend)
  {
    if (xi_delta == 65536)
    {
      uint32_t *src0 = data, *src1 = ndata;
      uint32_t s1_ga = 0, s1_rb = 0;
      int du = (xi >> 8) & 0xff;

      src0 = data + u;
      src1 = ndata + u;
      ctx_lerp_RGBA8_split (src0[0],src1[0], dv, &s1_ga, &s1_rb);
  
      for (; i < count; i ++)
      {
        uint32_t s0_ga = s1_ga;
        uint32_t s0_rb = s1_rb; 
        _ctx_coords_restrict (extend, &u, NULL, bwidth, bheight);
        ctx_lerp_RGBA8_split (src0[1],src1[1], dv, &s1_ga, &s1_rb);
        ((uint32_t*)(&rgba[0]))[0] = ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, du);
        rgba += 4;
        u++;
        src0 ++;
        src1 ++;
      }
    }
    else
    {
      uint32_t s0_ga = 0, s1_ga = 0, s0_rb = 0, s1_rb = 0;
      int prev_u = -1000;
      for (; (i < count); i++)
      {
        if (prev_u == u-1)
        {
          s0_ga = s1_ga;
          s0_rb = s1_rb;
          ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          prev_u = u;
        }
        else if (prev_u != u)
        {
          ctx_lerp_RGBA8_split (data[u],ndata[u], dv, &s0_ga, &s0_rb);
          ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          prev_u = u;
        }
        ((uint32_t*)(&rgba[0]))[0] = ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, (xi>>8));
        rgba += 4;
        u = (xi+=xi_delta) >> 16;
        _ctx_coords_restrict (extend, &u, NULL, bwidth, bheight);
      }
    }
  }
  else
  {
    if (xi_delta == 65536)
    {
      uint32_t *src0 = data, *src1 = ndata;
      uint32_t s1_ga = 0, s1_rb = 0;
      int du = (xi >> 8) & 0xff;
  
      src0 = data + u;
      src1 = ndata + u;
      ctx_lerp_RGBA8_split (src0[0],src1[0], dv, &s1_ga, &s1_rb);
  
      for (; i < count; i ++)
      {
        uint32_t s0_ga = s1_ga;
        uint32_t s0_rb = s1_rb;
        ctx_lerp_RGBA8_split (src0[1],src1[1], dv, &s1_ga, &s1_rb);
        ((uint32_t*)(&rgba[0]))[0] = ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, du);
        rgba += 4;
        u++;
        src0 ++;
        src1 ++;
      }
    }
    else
    {
      uint32_t s0_ga = 0, s1_ga = 0, s0_rb = 0, s1_rb = 0;
      int prev_u = -1000;
      for (; (i < count); i++)
      {
        if (prev_u == u-1)
        {
          s0_ga = s1_ga;
          s0_rb = s1_rb;
          ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          prev_u++;
        }
        else if (prev_u != u)
        {
          ctx_lerp_RGBA8_split (data[u],ndata[u], dv, &s0_ga, &s0_rb);
          ctx_lerp_RGBA8_split (data[u+1],ndata[u+1], dv, &s1_ga, &s1_rb);
          prev_u = u;
        }
        ((uint32_t*)(&rgba[0]))[0] = ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, (xi>>8));
        rgba += 4;
        u = (xi+=xi_delta) >> 16;
      }
    }
  }
}

static inline void
ctx_fragment_image_rgba8_RGBA8_bi_affine_with_alpha (CtxRasterizer *rasterizer,
                                          float x, float y, float z,
                                          void *out, int scount,
                                          float dx, float dy, float dz, uint8_t global_alpha_u8)
{
        x-=0.5f;
        y-=0.5f;
  uint32_t count = scount;
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  CtxExtend extend = rasterizer->state->gstate.extend;
  const int bwidth = buffer->width;
  const int bheight = buffer->height;
  unsigned int i = 0;
  uint32_t *data = ((uint32_t*)buffer->data);

  int yi_delta = (int)(dy * 65536);
  int xi_delta = (int)(dx * 65536);
  int32_t yi = (int)(y * 65536);
  int32_t xi = (int)(x * 65536);

  if (extend == CTX_EXTEND_NONE)
  {
    int32_t u1 = xi + xi_delta* (count-1);
    int32_t v1 = yi + yi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      if (((u1>>16) <0) |
          ((v1>>16) <0) |
          ((u1>>16) >= (bwidth) - 1) |
          ((v1>>16) >= (bheight) - 1))
      {
        *edst-- = 0;
        count --;
        u1 -= xi_delta;
        v1 -= yi_delta;
      }
      else break;
    }

  for (i= 0; i < count; i ++)
  {
    int u = xi >> 16;
    int v = yi >> 16;
    if ((u <= 0) | (v <= 0) | (u+1 >= bwidth-1) | (v+1 >= bheight-1))
    {
      *((uint32_t*)(rgba))= 0;
    }
    else
      break;
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;
  }
  }

  uint32_t *src00=data;
  uint32_t *src01=data;
  uint32_t *src10=data;
  uint32_t *src11=data;

  while (i < count)
  {
    int du = xi >> 8;
    int u = du >> 8;
    int dv = yi >> 8;
    int v = dv >> 8;
    if (CTX_UNLIKELY((u < 0) | (v < 0) | (u+1 >= bwidth) | (v+1 >=bheight))) // default to next sample down and to right
    {
      int u1 = u + 1;
      int v1 = v + 1;

      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      _ctx_coords_restrict (extend, &u1, &v1, bwidth, bheight);

      src00 = data  + bwidth * v + u;
      src01 = data  + bwidth * v + u1;
      src10 = data  + bwidth * v1 + u;
      src11 = data  + bwidth * v1 + u1;
    }
    else 
    {
      src00 = data  + bwidth * v + u;
      src01 = src00 + 1;
      src10 = src00 + bwidth;
      src11 = src01 + bwidth;
    }
    ((uint32_t*)(&rgba[0]))[0] = ctx_RGBA8_mul_alpha_u32 ( ctx_bi_RGBA8 (*src00,*src01,*src10,*src11, du,dv), global_alpha_u8);
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;

    i++;
  }
}

static inline void
ctx_fragment_image_rgba8_RGBA8_bi_affine (CtxRasterizer *rasterizer,
                                          float x, float y, float z,
                                          void *out, int scount,
                                          float dx, float dy, float dz)
{
    uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
    if (global_alpha_u8 != 255)
    {
      ctx_fragment_image_rgba8_RGBA8_bi_affine_with_alpha (rasterizer,
                                         x, y, z,
                                         out, scount, dx, dy, dz, global_alpha_u8);
      return;
    }
        x-=0.5f;
        y-=0.5f;
  uint32_t count = scount;
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  CtxExtend extend = rasterizer->state->gstate.extend;
  const int bwidth = buffer->width;
  const int bheight = buffer->height;
  unsigned int i = 0;
  uint32_t *data = ((uint32_t*)buffer->data);

  int yi_delta = (int)(dy * 65536);
  int xi_delta = (int)(dx * 65536);
  int32_t yi = (int)(y * 65536);
  int32_t xi = (int)(x * 65536);

  if (extend == CTX_EXTEND_NONE)
  {
    int32_t u1 = xi + xi_delta* (count-1);
    int32_t v1 = yi + yi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      if (((u1>>16) <0) |
          ((v1>>16) <0) |
          ((u1>>16) >= (bwidth) - 1) |
          ((v1>>16) >= (bheight) - 1))
      {
        *edst-- = 0;
        count --;
        u1 -= xi_delta;
        v1 -= yi_delta;
      }
      else break;
    }

  for (i= 0; i < count; i ++)
  {
    int u = xi >> 16;
    int v = yi >> 16;
    if ((u <= 0) | (v <= 0) | (u+1 >= bwidth-1) | (v+1 >= bheight-1))
    {
      *((uint32_t*)(rgba))= 0;
    }
    else
      break;
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;
  }
  }

  uint32_t *src00=data;
  uint32_t *src01=data;
  uint32_t *src10=data;
  uint32_t *src11=data;

  while (i < count)
  {
    int du = xi >> 8;
    int u = du >> 8;
    int dv = yi >> 8;
    int v = dv >> 8;
    if (CTX_UNLIKELY((u < 0) | (v < 0) | (u+1 >= bwidth) | (v+1 >=bheight))) // default to next sample down and to right
    {
      int u1 = u + 1;
      int v1 = v + 1;

      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      _ctx_coords_restrict (extend, &u1, &v1, bwidth, bheight);

      src00 = data  + bwidth * v + u;
      src01 = data  + bwidth * v + u1;
      src10 = data  + bwidth * v1 + u;
      src11 = data  + bwidth * v1 + u1;
    }
    else 
    {
      src00 = data  + bwidth * v + u;
      src01 = src00 + 1;
      src10 = src00 + bwidth;
      src11 = src01 + bwidth;
    }
    ((uint32_t*)(&rgba[0]))[0] = ctx_bi_RGBA8 (*src00,*src01,*src10,*src11, du,dv);
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;

    i++;
  }
}


static inline void
ctx_fragment_image_rgba8_RGBA8_bi_generic (CtxRasterizer *rasterizer,
                                           float x, float y, float z,
                                           void *out, int scount,
                                           float dx, float dy, float dz)
{
        x-=0.5f;
        y-=0.5f;
  uint32_t count = scount;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  CtxExtend extend = rasterizer->state->gstate.extend;
  const int bwidth = buffer->width;
  const int bheight = buffer->height;
  unsigned int i = 0;
  uint32_t *data = ((uint32_t*)buffer->data);

  int yi_delta = (int)(dy * 65536);
  int xi_delta = (int)(dx * 65536);
  int zi_delta = (int)(dz * 65536);
  int32_t yi = (int)(y * 65536);
  int32_t xi = (int)(x * 65536);
  int32_t zi = (int)(z * 65536);
  if (extend == CTX_EXTEND_NONE) {
    int32_t u1 = xi + xi_delta* (count-1);
    int32_t v1 = yi + yi_delta* (count-1);
    int32_t z1 = zi + zi_delta* (count-1);
    uint32_t *edst = ((uint32_t*)out)+(count-1);
    for (; i < count; )
    {
      float z_recip = (z1!=0) * (1.0f/z1);
      if ((u1*z_recip) <0 ||
          (v1*z_recip) <0 ||
          (u1*z_recip) >= (bwidth) - 1 ||
          (v1*z_recip) >= (bheight) - 1)
      {
        *edst-- = 0;
        count --;
        u1 -= xi_delta;
        v1 -= yi_delta;
        z1 -= zi_delta;
      }
      else break;
    }

  for (i= 0; i < count; i ++)
  {
    float z_recip = (zi!=0) * (1.0f/zi);
    int u = (int)(xi * z_recip);
    int v = (int)(yi * z_recip);
    if ((u <= 0) | (v <= 0) | (u+1 >= bwidth-1) | (v+1 >= bheight-1))
    {
      *((uint32_t*)(rgba))= 0;
    }
    else
      break;
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;
  }
  }

  uint32_t *src00=data;
  uint32_t *src01=data;
  uint32_t *src10=data;
  uint32_t *src11=data;

  if (global_alpha_u8==255)
  while (i < count)
  {
    float zr = (zi!=0)*(1.0f/zi) * 256;
    int du = (int)(xi * zr);
    int u = du >> 8;
    int dv = (int)(yi * zr);
    int v = dv >> 8;
    if (CTX_UNLIKELY((u < 0) | (v < 0) | (u+1 >= bwidth) | (v+1 >=bheight))) // default to next sample down and to right
    {
      int u1 = u + 1;
      int v1 = v + 1;

      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      _ctx_coords_restrict (extend, &u1, &v1, bwidth, bheight);

      src00 = data  + bwidth * v + u;
      src01 = data  + bwidth * v + u1;
      src10 = data  + bwidth * v1 + u;
      src11 = data  + bwidth * v1 + u1;
    }
    else 
    {
      src00 = data  + bwidth * v + u;
      src01 = src00 + 1;
      src10 = src00 + bwidth;
      src11 = src01 + bwidth;
    }
    ((uint32_t*)(&rgba[0]))[0] = ctx_bi_RGBA8 (*src00,*src01,*src10,*src11, du,dv);
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;

    i++;
  }
  else
  while (i < count)
  {
    float zr = (zi!=0)*(1.0f/zi) * 256;
    int du = (int)(xi * zr);
    int u = du >> 8;
    int dv = (int)(yi * zr);
    int v = dv >> 8;
    if (CTX_UNLIKELY((u < 0) | (v < 0) | (u+1 >= bwidth) | (v+1 >=bheight))) // default to next sample down and to right
    {
      int u1 = u + 1;
      int v1 = v + 1;

      _ctx_coords_restrict (extend, &u, &v, bwidth, bheight);
      _ctx_coords_restrict (extend, &u1, &v1, bwidth, bheight);

      src00 = data  + bwidth * v + u;
      src01 = data  + bwidth * v + u1;
      src10 = data  + bwidth * v1 + u;
      src11 = data  + bwidth * v1 + u1;
    }
    else 
    {
      src00 = data  + bwidth * v + u;
      src01 = src00 + 1;
      src10 = src00 + bwidth;
      src11 = src01 + bwidth;
    }
    ((uint32_t*)(&rgba[0]))[0] =
        ctx_RGBA8_mul_alpha_u32 (
            ctx_bi_RGBA8 (*src00,*src01,*src10,*src11, du,dv), global_alpha_u8);
    xi += xi_delta;
    yi += yi_delta;
    zi += zi_delta;
    rgba += 4;

    i++;
  }
}


static void
ctx_fragment_image_rgba8_RGBA8_bi (CtxRasterizer *rasterizer,
                                   float x, float y, float z,
                                   void *out, int icount, float dx, float dy, float dz)
{
  unsigned int count = icount;
  if ((dy == 0.0f) & (dx > 0.0f) & (z==1.0f) & (dz==0.0f))
  {
    ctx_fragment_image_rgba8_RGBA8_bi_scale (rasterizer, x, y, z, out, count, dx, dy, dz);
  }
  else if ((z == 1.0f) & (dz == 0.0f))
    ctx_fragment_image_rgba8_RGBA8_bi_affine (rasterizer, x, y, z, out, count, dx, dy, dz);
  else
  {
    ctx_fragment_image_rgba8_RGBA8_bi_generic (rasterizer, x, y, z, out, count, dx, dy, dz);
  }
}
#endif

#define ctx_clamp_byte(val) \
  val *= val > 0;\
  val = (val > 255) * 255 + (val <= 255) * val

#if CTX_YUV_LUTS
static const int16_t ctx_y_to_cy[256]={
-19,-18,-17,-16,-14,-13,-12,-11,-10,-9,-7,-6,-5,-4,-3,
-2,0,1,2,3,4,5,6,8,9,10,11,12,13,15,
16,17,18,19,20,22,23,24,25,26,27,29,30,31,32,
33,34,36,37,38,39,40,41,43,44,45,46,47,48,50,
51,52,53,54,55,57,58,59,60,61,62,64,65,66,67,
68,69,71,72,73,74,75,76,78,79,80,81,82,83,84,
86,87,88,89,90,91,93,94,95,96,97,98,100,101,102,
103,104,105,107,108,109,110,111,112,114,115,116,117,118,119,
121,122,123,124,125,126,128,129,130,131,132,133,135,136,137,
138,139,140,142,143,144,145,146,147,149,150,151,152,153,154,
156,157,158,159,160,161,163,164,165,166,167,168,169,171,172,
173,174,175,176,178,179,180,181,182,183,185,186,187,188,189,
190,192,193,194,195,196,197,199,200,201,202,203,204,206,207,
208,209,210,211,213,214,215,216,217,218,220,221,222,223,224,
225,227,228,229,230,231,232,234,235,236,237,238,239,241,242,
243,244,245,246,248,249,250,251,252,253,254,256,257,258,259,
260,261,263,264,265,266,267,268,270,271,272,273,274,275,277,
278};
static const int16_t ctx_u_to_cb[256]={
-259,-257,-255,-253,-251,-249,-247,-245,-243,-241,-239,-237,-234,-232,-230,
-228,-226,-224,-222,-220,-218,-216,-214,-212,-210,-208,-206,-204,-202,-200,
-198,-196,-194,-192,-190,-188,-186,-184,-182,-180,-178,-176,-174,-172,-170,
-168,-166,-164,-162,-160,-158,-156,-154,-152,-150,-148,-146,-144,-142,-140,
-138,-136,-134,-132,-130,-128,-126,-124,-122,-120,-117,-115,-113,-111,-109,
-107,-105,-103,-101,-99,-97,-95,-93,-91,-89,-87,-85,-83,-81,-79,
-77,-75,-73,-71,-69,-67,-65,-63,-61,-59,-57,-55,-53,-51,-49,
-47,-45,-43,-41,-39,-37,-35,-33,-31,-29,-27,-25,-23,-21,-19,
-17,-15,-13,-11,-9,-7,-5,-3,0,2,4,6,8,10,12,
14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,
44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,
74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,
104,106,108,110,112,114,116,119,121,123,125,127,129,131,133,
135,137,139,141,143,145,147,149,151,153,155,157,159,161,163,
165,167,169,171,173,175,177,179,181,183,185,187,189,191,193,
195,197,199,201,203,205,207,209,211,213,215,217,219,221,223,
225,227,229,231,233,236,238,240,242,244,246,248,250,252,254,
256};
static const int16_t ctx_v_to_cr[256]={
-205,-203,-202,-200,-198,-197,-195,-194,-192,-190,-189,-187,-186,-184,-182,
-181,-179,-178,-176,-174,-173,-171,-170,-168,-166,-165,-163,-162,-160,-159,
-157,-155,-154,-152,-151,-149,-147,-146,-144,-143,-141,-139,-138,-136,-135,
-133,-131,-130,-128,-127,-125,-123,-122,-120,-119,-117,-115,-114,-112,-111,
-109,-107,-106,-104,-103,-101,-99,-98,-96,-95,-93,-91,-90,-88,-87,
-85,-83,-82,-80,-79,-77,-76,-74,-72,-71,-69,-68,-66,-64,-63,
-61,-60,-58,-56,-55,-53,-52,-50,-48,-47,-45,-44,-42,-40,-39,
-37,-36,-34,-32,-31,-29,-28,-26,-24,-23,-21,-20,-18,-16,-15,
-13,-12,-10,-8,-7,-5,-4,-2,0,1,3,4,6,7,9,
11,12,14,15,17,19,20,22,23,25,27,28,30,31,33,
35,36,38,39,41,43,44,46,47,49,51,52,54,55,57,
59,60,62,63,65,67,68,70,71,73,75,76,78,79,81,
82,84,86,87,89,90,92,94,95,97,98,100,102,103,105,
106,108,110,111,113,114,116,118,119,121,122,124,126,127,129,
130,132,134,135,137,138,140,142,143,145,146,148,150,151,153,
154,156,158,159,161,162,164,165,167,169,170,172,173,175,177,
178,180,181,183,185,186,188,189,191,193,194,196,197,199,201,
202};

#endif

static inline uint32_t ctx_yuv_to_rgba32 (uint8_t y, uint8_t u, uint8_t v)
{
#if CTX_YUV_LUTS
  int cy  = ctx_y_to_cy[y];
  int red = cy + ctx_v_to_cr[v];
  int green = cy - (((u-128) * 25674 + (v-128) * 53278) >> 16);
  int blue = cy + ctx_u_to_cb[u];
#else
  int cy  = ((y - 16) * 76309) >> 16;
  int cr  = (v - 128);
  int cb  = (u - 128);
  int red = cy + ((cr * 104597) >> 16);
  int green = cy - ((cb * 25674 + cr * 53278) >> 16);
  int blue = cy + ((cb * 132201) >> 16);
#endif
  ctx_clamp_byte (red);
  ctx_clamp_byte (green);
  ctx_clamp_byte (blue);
  return red |
  (green << 8) |
  (blue << 16) |
  (0xff << 24);
}

static void
ctx_fragment_image_yuv420_RGBA8_nearest (CtxRasterizer *rasterizer,
                                         float x, float y, float z,
                                         void *out, int count, float dx, float dy, float dz)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer;
  if (buffer->color_managed)
    buffer = buffer->color_managed;
  uint8_t *src = (uint8_t *) buffer->data;
  int bwidth  = buffer->width;
  int bheight = buffer->height;
  int bwidth_div_2  = bwidth/2;
  int bheight_div_2  = bheight/2;
  x += 0.5f;
  y += 0.5f;

#if CTX_DITHER
  int bits = rasterizer->format->bpp;
  int scan = rasterizer->scanline / CTX_FULL_AA;
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green  = rasterizer->format->dither_green;
#endif

  if (!src)
          return;

  {
    int i = 0;

    float  u1 = x + dx * (count-1);
    float  v1 = y + dy * (count-1);
    uint32_t *edst = ((uint32_t*)out)+count - 1;
    for (; i < count; )
    {
      if ((u1 <0) | (v1 < 0) | (u1 >= bwidth) | (v1 >= bheight))
      {
        *edst-- = 0;
        count --;
        u1 -= dx;
        v1 -= dy;
      }
      else break;
    }

    for (; i < count; i ++)
    {
      int u = (int)x;
      int v = (int)y;
      if ((u < 0) | (v < 0) | (u >= bwidth) | (v >= bheight))
      {
        *((uint32_t*)(rgba))= 0;
      }
      else
      {
        break;
      }
      x += dx;
      y += dy;
      rgba += 4;
    }

    uint32_t u_offset = bheight * bwidth;
    uint32_t v_offset = u_offset + bheight_div_2 * bwidth_div_2;

    if (rasterizer->swap_red_green)
    {
      v_offset = bheight * bwidth;
      u_offset = v_offset + bheight_div_2 * bwidth_div_2;
    }

    // XXX this is incorrect- but fixes some bug!
    int ix = 65536;//x * 65536;
    int iy = (int)(y * 65536);

    int ideltax = (int)(dx * 65536);
    int ideltay = (int)(dy * 65536);

    if (ideltay == 0)
    {
      int u = ix >> 16;
      int v = iy >> 16;

      uint32_t y  = v * bwidth;
      uint32_t uv = (v / 2) * bwidth_div_2;

      if ((v >= 0) & (v < bheight))
      {
#if CTX_DITHER
       if (bits < 24)
       {
         while (i < count)// && u >= 0 && u+1 < bwidth)
         {
           *((uint32_t*)(rgba))= ctx_yuv_to_rgba32 (src[y+u],
                        src[u_offset+uv+u/2], src[v_offset+uv+u/2]);

           ctx_dither_rgba_u8 (rgba, i, scan, dither_red_blue, dither_green);
           ix += ideltax;
           rgba += 4;
           u = ix >> 16;
           i++;
         }
        }
        else
#endif
        while (i < count)// && u >= 0 && u+1 < bwidth)
        {
          *((uint32_t*)(rgba))= ctx_yuv_to_rgba32 (src[y+u],
                          src[u_offset+uv+u/2], src[v_offset+uv+u/2]);
  
          ix += ideltax;
          rgba += 4;
          u = ix >> 16;
          i++;
        }
      }
    }
    else
    {
      int u = ix >> 16;
      int v = iy >> 16;

#if CTX_DITHER
       if (bits < 24)
       {
         while (i < count)// && u >= 0 && v >= 0 && u < bwidth && v < bheight)
         {
           uint32_t y  = v * bwidth + u;
           uint32_t uv = (v / 2) * bwidth_div_2 + (u / 2);

           *((uint32_t*)(rgba))= ctx_yuv_to_rgba32 (src[y],
                        src[u_offset+uv], src[v_offset+uv]);

           ctx_dither_rgba_u8 (rgba, i, scan, dither_red_blue, dither_green);
           ix += ideltax;
           iy += ideltay;
           rgba += 4;
           u = ix >> 16;
           v = iy >> 16;
           i++;
         }
       } else
#endif
       while (i < count)// && u >= 0 && v >= 0 && u < bwidth && v < bheight)
       {
          uint32_t y  = v * bwidth + u;
          uint32_t uv = (v / 2) * bwidth_div_2 + (u / 2);

          *((uint32_t*)(rgba))= ctx_yuv_to_rgba32 (src[y],
                        src[u_offset+uv], src[v_offset+uv]);

          ix += ideltax;
          iy += ideltay;
          rgba += 4;
          u = ix >> 16;
          v = iy >> 16;
          i++;
       }
    }

    for (; i < count; i++)
    {
      *((uint32_t*)(rgba))= 0;
      rgba += 4;
    }
  }

  if (rasterizer->state->gstate.global_alpha_u8 != 255)
    ctx_RGBA8_apply_global_alpha_and_associate (rasterizer, (uint8_t*)out, count);
}

#if CTX_FRAGMENT_SPECIALIZE

CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_box)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_bi)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_nearest)

CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_nearest_copy)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_nearest_copy_repeat)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_nearest_scale)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_nearest_affine)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_nearest_generic)

CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_bi_scale)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_bi_affine)
CTX_DECLARE_SWAP_RED_GREEN_FRAGMENT(ctx_fragment_image_rgba8_RGBA8_bi_generic)

static inline void
ctx_fragment_image_rgba8_RGBA8 (CtxRasterizer *rasterizer,
                                float x, float y, float z,
                                void *out, int count, float dx, float dy, float dz)
{
  if (rasterizer->state->gstate.image_smoothing)
  {
    float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
    if (factor <= 0.50f)
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_box_swap_red_green (rasterizer, x, y, z, out, count, dx, dy, dz);
      else
        ctx_fragment_image_rgba8_RGBA8_box (rasterizer, x, y, z, out, count, dx, dy, dz);
    }
#if CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
    else if ((factor > 0.99f) & (factor < 1.01f))
    {
      // XXX: also verify translate == 0 for this fast path to be valid
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (rasterizer, x, y, z, out, count, dx, dy, dz);
      else
        ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, z, out, count, dx, dy, dz);
    }
#endif
    else
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_bi_swap_red_green (rasterizer, x, y, z, out, count, dx, dy, dz);
      else
        ctx_fragment_image_rgba8_RGBA8_bi (rasterizer, x, y, z, out, count, dx, dy, dz);
    }
  }
  else
  {
    if (rasterizer->swap_red_green)
      ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (rasterizer, x, y, z, out, count, dx, dy, dz);
    else
      ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, z, out, count, dx, dy, dz);
  }
  //ctx_fragment_swap_red_green_u8 (out, count);
#if 0
#if CTX_DITHER
  uint8_t *rgba = (uint8_t*)out;
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
#endif
}
#endif

static void
ctx_fragment_image_gray1_RGBA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer;
  for (int i = 0; i < count; i ++)
  {
  int u = (int)x;
  int v = (int)y;
  if ( (u < 0) | (v < 0) |
       (u >= buffer->width) |
       (v >= buffer->height))
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
            { rgba[c] = 255;
            }//g->texture.rgba[c];
            //}
        }
    }

    rgba += 4;
    x += dx;
    y += dy;
  }
}

#if CTX_GRADIENTS
static void
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
#if CTX_DITHER
  int scan = rasterizer->scanline / CTX_FULL_AA;
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green  = rasterizer->format->dither_green;
  int ox = (int)x;
#endif
  for (int i = 0; i <  count; i ++)
  {
    float v = (ctx_hypotf_fast (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
#if CTX_GRADIENT_CACHE
    uint32_t *rgbap = (uint32_t*)&rasterizer->gradient_cache_u8[ctx_grad_index(rasterizer, v)][0];
    *((uint32_t*)rgba) = *rgbap;
#else
    ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, rgba);
#endif
#
#if CTX_DITHER
    ctx_dither_rgba_u8 (rgba, ox+i, scan, dither_red_blue, dither_green);
#endif
    rgba += 4;
    x += dx;
    y += dy;
  }
}

static void
ctx_fragment_linear_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
#if 0
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i <  count; i ++)
  {
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
#if CTX_GRADIENT_CACHE
  uint32_t*rgbap = ((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(v)][0]));
  *((uint32_t*)rgba) = *rgbap;
#else
  _ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 1.0, rgba);
#endif
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x+i, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
    rgba += 4;
    x += dx;
    y += dy;
  }
#else
  uint8_t *rgba = (uint8_t *) out;

  CtxSource *g = &rasterizer->state->gstate.source_fill;
  float u0 = x; float v0 = y;
  float ud = dx; float vd = dy;
  float linear_gradient_rdelta = g->linear_gradient.rdelta;
  float linear_gradient_length = g->linear_gradient.length;
  float linear_gradient_length_recip = 1.0f/linear_gradient_length;
  float linear_gradient_dx = g->linear_gradient.dx *linear_gradient_length_recip * linear_gradient_rdelta;
  float linear_gradient_dy = g->linear_gradient.dy *linear_gradient_length_recip * linear_gradient_rdelta;
  float linear_gradient_start = g->linear_gradient.start * linear_gradient_rdelta;

#if CTX_DITHER
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green = rasterizer->format->dither_green;
  int scan = rasterizer->scanline / CTX_FULL_AA;
  int ox = (int)x;
#endif

  u0 *= linear_gradient_dx;
  v0 *= linear_gradient_dy;
  ud *= linear_gradient_dx;
  vd *= linear_gradient_dy;

#if CTX_GRADIENT_CACHE
  int vv = (int)(((u0 + v0) - linear_gradient_start) * (rasterizer->gradient_cache_elements-1) * 256);
  int ud_plus_vd = (int)((ud + vd) * (rasterizer->gradient_cache_elements-1) * 256);
#else
  float vv = ((u0 + v0) - linear_gradient_start);
  float ud_plus_vd = (ud + vd);
#endif

  for (int i = 0; i < count ; i++)
  {
#if CTX_GRADIENT_CACHE
  uint32_t*rgbap = ((uint32_t*)(&rasterizer->gradient_cache_u8[ctx_grad_index_i (rasterizer, vv)][0]));
  *((uint32_t*)rgba) = *rgbap;
#else
  _ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, rgba);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (rgba, ox+i, scan, dither_red_blue, dither_green);
#endif
    rgba+= 4;
    vv += ud_plus_vd;
  }
#endif
}

#endif

static void
ctx_fragment_color_RGBA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  uint8_t *rgba_out = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  ctx_color_get_rgba8 (rasterizer->state, &g->color, rgba_out);
  ctx_RGBA8_associate_alpha (rgba_out);
  if (rasterizer->swap_red_green)
  {
    int tmp = rgba_out[0];
    rgba_out[0] = rgba_out[2];
    rgba_out[2] = tmp;
  }
  for (int i = 1; i < count; i++, rgba_out+=4)
    memcpy (rgba_out + count * 4, rgba_out, 4);
}
#if CTX_ENABLE_FLOAT

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i++)
  {
    float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                  g->linear_gradient.length) -
                g->linear_gradient.start) * (g->linear_gradient.rdelta);
    ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0f, rgba);
    x += dx;
    y += dy;
    rgba += 4;
  }
}

static void
ctx_fragment_radial_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i++)
  {
  float v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
        v = (v - g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 0.0f, rgba);
    x+=dx;
    y+=dy;
    rgba +=4;
  }
}
#endif


static void
ctx_fragment_color_RGBAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  float *rgba = (float *) out;
  float  in[4];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  ctx_color_get_rgba (rasterizer->state, &g->color, in);
  for (int c = 0; c < 3; c++)
    in[c] *= in[3];
  while (count--)
  {
    for (int c = 0; c < 4; c++)
      rgba[c] = in[c];
    rgba += 4;
  }
}


static void ctx_fragment_image_RGBAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  float *outf = (float *) out;
  uint8_t rgba[4 * count];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  switch (buffer->format->bpp)
    {
#if CTX_FRAGMENT_SPECIALIZE
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz); break;
#endif
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz);       break;
    }
  for (int c = 0; c < 4 * count; c ++) { outf[c] = ctx_u8_to_float (rgba[c]); }
}

static CtxFragment ctx_rasterizer_get_fragment_RGBAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:         return ctx_fragment_image_RGBAF;
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_RGBAF;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_RGBAF;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_RGBAF;
#endif
    }
  return ctx_fragment_color_RGBAF;
}
#endif


static inline int
ctx_matrix_no_perspective (CtxMatrix *matrix)
{
  if (fabsf(matrix->m[2][0]) >0.001f) return 0;
  if (fabsf(matrix->m[2][1]) >0.001f) return 0;
  if (fabsf(matrix->m[2][2] - 1.0f)>0.001f) return 0;
  return 1;
}

/* for multiples of 90 degree rotations, we return no rotation */
static inline int
ctx_matrix_no_skew_or_rotate (CtxMatrix *matrix)
{
  if (fabsf(matrix->m[0][1]) >0.001f) return 0;
  if (fabsf(matrix->m[1][0]) >0.001f) return 0;
  return ctx_matrix_no_perspective (matrix);
}


static CtxFragment ctx_rasterizer_get_fragment_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:
      {
        CtxBuffer *buffer = g->texture.buffer;
        if (buffer)
          buffer = buffer->color_managed?buffer->color_managed:buffer;
        if (!buffer || !buffer->format)
          return ctx_fragment_color_RGBA8;

        if (buffer->format->pixel_format == CTX_FORMAT_YUV420)
        {
          return ctx_fragment_image_yuv420_RGBA8_nearest;
        }
        else
#if CTX_FRAGMENT_SPECIALIZE
        switch (buffer->format->bpp)
          {
            case 1: return ctx_fragment_image_gray1_RGBA8;
#if 1
            case 24: 
              {
                if (gstate->image_smoothing)
                {
                  float factor = ctx_matrix_get_scale (&gstate->transform);
                          //fprintf (stderr, "{%.3f}", factor);
                  if (factor < 0.5f)
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgb8_RGBA8_box_swap_red_green;
                    return ctx_fragment_image_rgb8_RGBA8_box;
                  }
#if CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
                  else if ((factor > 0.99f) & (factor < 1.01f))
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green;
                    return ctx_fragment_image_rgb8_RGBA8_nearest;
                  }
#endif
                  else
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green;
                    return ctx_fragment_image_rgb8_RGBA8_bi;
                  }
                }
                else
                {
                  if (rasterizer->swap_red_green)
                    return ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green;
                  return ctx_fragment_image_rgb8_RGBA8_nearest;
                }
              }
              break;
#endif
            case 32:
              {
                CtxMatrix *transform = &gstate->source_fill.transform;
                CtxExtend extend = rasterizer->state->gstate.extend;
                if (gstate->image_smoothing)
                {
                  float factor = ctx_matrix_get_scale (&gstate->transform);
                          //fprintf (stderr, "[%.3f]", factor);
                  if (factor < 0.5f)
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgba8_RGBA8_box_swap_red_green;
                    return ctx_fragment_image_rgba8_RGBA8_box;
                  }
#if CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
                  else if ((factor > 0.99f) & (factor < 1.01f) & (extend == CTX_EXTEND_NONE))
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgba8_RGBA8_nearest_copy_swap_red_green;
                    return ctx_fragment_image_rgba8_RGBA8_nearest_copy;
                  }
#endif
                  else
                  {
                    if (rasterizer->swap_red_green)
                    {
                      if (ctx_matrix_no_perspective (transform))
                      {
                        if (ctx_matrix_no_skew_or_rotate (transform))
                        {
                          if ((int)(ctx_fabsf (transform->m[0][0] - 1.0f) < 0.001f) &
                              (ctx_fabsf (transform->m[1][1] - 1.0f) < 0.001f) &
                              (ctx_fmod1f (transform->m[0][2]) < 0.001f) &
                              (ctx_fmod1f (transform->m[1][2]) < 0.001f))
                          {
                            if (extend == CTX_EXTEND_NONE)
                              return ctx_fragment_image_rgba8_RGBA8_nearest_copy_swap_red_green;
                            else if (extend == CTX_EXTEND_REPEAT)
                              return ctx_fragment_image_rgba8_RGBA8_nearest_copy_repeat_swap_red_green;
                          }
                          return ctx_fragment_image_rgba8_RGBA8_bi_scale_swap_red_green;
                        }
                        return ctx_fragment_image_rgba8_RGBA8_bi_affine_swap_red_green;
                      }
                      return ctx_fragment_image_rgba8_RGBA8_bi_generic_swap_red_green;
                    }

                    if (ctx_matrix_no_perspective (transform))
                    {
                      if (ctx_matrix_no_skew_or_rotate (transform))
                      {
                        if ((int)(ctx_fabsf (transform->m[0][0] - 1.0f) < 0.001f) &
                            (ctx_fabsf (transform->m[1][1] - 1.0f) < 0.001f) &
                            (ctx_fmod1f (transform->m[0][2]) < 0.001f) &
                            (ctx_fmod1f (transform->m[1][2]) < 0.001f))
                        {
                          if (extend == CTX_EXTEND_NONE)
                            return ctx_fragment_image_rgba8_RGBA8_nearest_copy;
                          else if (extend == CTX_EXTEND_REPEAT)
                            return ctx_fragment_image_rgba8_RGBA8_nearest_copy_repeat;
                        }
                        return ctx_fragment_image_rgba8_RGBA8_bi_scale;
                      }
                      return ctx_fragment_image_rgba8_RGBA8_bi_affine;
                    }
                    return ctx_fragment_image_rgba8_RGBA8_bi_generic;
                  }
                }
                else
                {
                  if (rasterizer->swap_red_green)
                  {
                    if (ctx_matrix_no_perspective (transform))
                    {
                      if (ctx_matrix_no_skew_or_rotate (transform))
                      {
                        if ((int)(ctx_fabsf (transform->m[0][0] - 1.0f) < 0.001f) &
                            (ctx_fabsf (transform->m[1][1] - 1.0f) < 0.001f))
                        {
                           return ctx_fragment_image_rgba8_RGBA8_nearest_copy_swap_red_green;
                         if (extend == CTX_EXTEND_NONE)
                           return ctx_fragment_image_rgba8_RGBA8_nearest_copy_swap_red_green;
                         else if (extend == CTX_EXTEND_REPEAT)
                           return ctx_fragment_image_rgba8_RGBA8_nearest_copy_repeat_swap_red_green;
                        }
                        return ctx_fragment_image_rgba8_RGBA8_nearest_scale_swap_red_green;
                      }
                      return ctx_fragment_image_rgba8_RGBA8_nearest_affine_swap_red_green;
                    }
                    return ctx_fragment_image_rgba8_RGBA8_nearest_generic_swap_red_green;
                  }
                  if (ctx_matrix_no_perspective (transform))
                  {
                    if (ctx_matrix_no_skew_or_rotate (transform))
                    {
                      if ((int)(ctx_fabsf (transform->m[0][0] - 1.0f) < 0.001f) &
                          (ctx_fabsf (transform->m[1][1] - 1.0f) < 0.001f))
                      {
                         if (extend == CTX_EXTEND_NONE)
                           return ctx_fragment_image_rgba8_RGBA8_nearest_copy;
                         else if (extend == CTX_EXTEND_REPEAT)
                           return ctx_fragment_image_rgba8_RGBA8_nearest_copy_repeat;
                      }
                      return ctx_fragment_image_rgba8_RGBA8_nearest_scale;
                    }
                    return ctx_fragment_image_rgba8_RGBA8_nearest_affine;
                  }
                  return ctx_fragment_image_rgba8_RGBA8_nearest_generic;
                }
              }
            default: return ctx_fragment_image_RGBA8;
          }
#else
          return ctx_fragment_image_RGBA8;
#endif
      }

      case CTX_SOURCE_COLOR:           return ctx_fragment_color_RGBA8;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_RGBA8;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_RGBA8;
#endif
    }
  return ctx_fragment_color_RGBA8;
}

static inline void
ctx_init_uv (CtxRasterizer *rasterizer,
             int x0,
             int y0,
             float *u0, float *v0, float *w0, float *ud, float *vd, float *wd)
             //float *u0, float *v0, float *w0, float *ud, float *vd, float *wd)
{
  CtxMatrix *transform = &rasterizer->state->gstate.source_fill.transform;
  *u0 = transform->m[0][0] * (x0 + 0.0f) +
        transform->m[0][1] * (y0 + 0.0f) +
        transform->m[0][2];
  *v0 = transform->m[1][0] * (x0 + 0.0f) +
        transform->m[1][1] * (y0 + 0.0f) +
        transform->m[1][2];
  *w0 = transform->m[2][0] * (x0 + 0.0f) +
        transform->m[2][1] * (y0 + 0.0f) +
        transform->m[2][2];
  *ud = transform->m[0][0];
  *vd = transform->m[1][0];
  *wd = transform->m[2][0];
}

static inline void
ctx_u8_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  if (CTX_UNLIKELY(rasterizer->fragment))
    {
      float u0 = 0; float v0 = 0;
      float ud = 0; float vd = 0;
      float w0 = 1; float wd = 0;
      ctx_init_uv (rasterizer, x0, rasterizer->scanline/CTX_FULL_AA, &u0, &v0, &w0, &ud, &vd, &wd);
      while (count--)
      {
        uint8_t cov = *coverage;
        if (CTX_UNLIKELY(cov == 0))
        {
          u0+=ud;
          v0+=vd;
        }
        else
        {
          rasterizer->fragment (rasterizer, u0, v0, w0, src, 1, ud, vd, wd);
          u0+=ud;
          v0+=vd;
          if (cov == 255)
          {
            for (int c = 0; c < components; c++)
              dst[c] = src[c];
          }
          else
          {
            uint8_t rcov = 255 - cov;
            for (int c = 0; c < components; c++)
              { dst[c] = (src[c]*cov + dst[c]*rcov)/255; }
          }
        }
        dst += components;
        coverage ++;
      }
      return;
    }

  while (count--)
  {
    uint8_t cov = *coverage;
    uint8_t rcov = 255-cov;
    for (int c = 0; c < components; c++)
      { dst[c] = (src[c]*cov+dst[c]*rcov)/255; }
    dst += components;
    coverage ++;
  }
}

static void
ctx_u8_clear_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  while (count--)
  {
    uint8_t cov = *coverage;
    for (int c = 0; c < components; c++)
      { dst[c] = (dst[c] * (256-cov)) >> 8; }
    coverage ++;
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
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
       break;\
     default:\
     case CTX_COMPOSITE_CLEAR:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
  }\
}

static inline void
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

  while (count--)
  {
    uint8_t cov = *coverage++;
    for (int c = 0; c < components; c++)
      dst[c] =  ((((tsrc[c] * cov)) + (dst[c] * (((((255+(tsrc[components-1] * cov))>>8))^255 ))))>>8);
    dst+=components;
  }
}

static inline void
ctx_u8_source_copy_normal_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  while (count--)
  {
    for (int c = 0; c < components; c++)
      dst[c] =  ctx_lerp_u8(dst[c],src[c],coverage[0]);
    coverage ++;
    dst+=components;
  }
}

static inline void
ctx_RGBA8_source_over_normal_buf (CTX_COMPOSITE_ARGUMENTS, uint8_t *tsrc)
{
  while (count--)
  {
     uint32_t si_ga = ((*((uint32_t*)tsrc)) & 0xff00ff00) >> 8;
     uint32_t si_rb = (*((uint32_t*)tsrc)) & 0x00ff00ff;
//   uint32_t di_ga = ((*((uint32_t*)dst)) & 0xff00ff00) >> 8;
//   uint32_t di_rb = (*((uint32_t*)dst)) & 0x00ff00ff;
     uint32_t si_a  = si_ga >> 16;
     uint32_t cov = *coverage;
     uint32_t racov = (255-((255+si_a*cov)>>8));
     *((uint32_t*)(dst)) =

     (((si_rb*cov+0xff00ff+(((*((uint32_t*)(dst)))&0x00ff00ff)*racov))>>8)&0x00ff00ff)|
     ((si_ga*cov+0xff00ff+((((*((uint32_t*)(dst)))&0xff00ff00)>>8)*racov))&0xff00ff00);

     coverage ++;
     tsrc += 4;
     dst  += 4;
  }
}

static CTX_INLINE void
ctx_RGBA8_source_over_normal_full_cov_buf (CTX_COMPOSITE_ARGUMENTS, uint8_t *tsrc)
{
  uint32_t *ttsrc = (uint32_t*)tsrc;
  uint32_t *ddst  = (uint32_t*)dst;
  while (count--)
  {
     uint32_t si_ga = ((*ttsrc) & 0xff00ff00) >> 8;
     uint32_t si_rb = (*ttsrc++) & 0x00ff00ff;
     uint32_t si_a  = si_ga >> 16;
     uint32_t racov = si_a^255;
     *(ddst) =
     (((si_rb*255+0xff00ff+(((*ddst)&0x00ff00ff)*racov))>>8)&0x00ff00ff)|
     ((si_ga*255+0xff00ff+((((*ddst)&0xff00ff00)>>8)*racov))&0xff00ff00);
     ddst++;
  }
}

static inline void
ctx_RGBA8_source_copy_normal_buf (CTX_COMPOSITE_ARGUMENTS, uint8_t *__restrict__ tsrc)
{
  uint32_t *ttsrc = (uint32_t*)tsrc;
  uint32_t *ddst  = (uint32_t*)dst;
  while (count--)
  {
    *ddst=ctx_lerp_RGBA8 (*ddst, *(ttsrc++), *(coverage++));
    ddst++;
  }
}

static inline void
ctx_RGBA8_source_over_normal_fragment (CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  float w0 = 1; float wd = 0;
  ctx_init_uv (rasterizer, x0, rasterizer->scanline/CTX_FULL_AA, &u0, &v0, &w0, &ud, &vd, &wd);
  uint8_t _tsrc[4 * (count)];
  rasterizer->fragment (rasterizer, u0, v0, w0, &_tsrc[0], count, ud, vd, wd);
  ctx_RGBA8_source_over_normal_buf (rasterizer,
                       dst, src, x0, coverage, count, &_tsrc[0]);
}

static inline void
ctx_RGBA8_source_over_normal_full_cov_fragment (CTX_COMPOSITE_ARGUMENTS, int scanlines)
{
  CtxMatrix *transform = &rasterizer->state->gstate.source_fill.transform;
  int scan = rasterizer->scanline /CTX_FULL_AA;
  CtxFragment fragment = rasterizer->fragment;

  if (CTX_LIKELY(ctx_matrix_no_perspective (transform)))
  {
    float u0, v0, ud, vd, w0, wd;
    uint8_t _tsrc[4 * count];
    ctx_init_uv (rasterizer, x0, scan, &u0, &v0, &w0, &ud, &vd, &wd);
    for (int y = 0; y < scanlines; y++)
    {
      fragment (rasterizer, u0, v0, w0, &_tsrc[0], count, ud, vd, wd);
      ctx_RGBA8_source_over_normal_full_cov_buf (rasterizer,
                          dst, src, x0, coverage, count, &_tsrc[0]);
      u0 -= vd;
      v0 += ud;
      dst += rasterizer->blit_stride;
    }
  }
  else
  {
    uint8_t _tsrc[4 * count];
    for (int y = 0; y < scanlines; y++)
    {
      float u0, v0, ud, vd, w0, wd;
      ctx_init_uv (rasterizer, x0, scan+y, &u0, &v0, &w0, &ud, &vd, &wd);
      fragment (rasterizer, u0, v0, w0, &_tsrc[0], count, ud, vd, wd);
      ctx_RGBA8_source_over_normal_full_cov_buf (rasterizer,
                          dst, src, x0, coverage, count, &_tsrc[0]);
      dst += rasterizer->blit_stride;
    }
  }
}

static inline void
ctx_RGBA8_source_copy_normal_fragment (CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  float w0 = 1; float wd = 0;
  ctx_init_uv (rasterizer, x0, rasterizer->scanline/CTX_FULL_AA, &u0, &v0, &w0, &ud, &vd, &wd);
  uint8_t _tsrc[4 * (count)];
  rasterizer->fragment (rasterizer, u0, v0, w0, &_tsrc[0], count, ud, vd, wd);
  ctx_RGBA8_source_copy_normal_buf (rasterizer,
                       dst, src, x0, coverage, count, &_tsrc[0]);
}


static void
ctx_RGBA8_source_over_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
#if CTX_REFERENCE
  ctx_u8_source_over_normal_color (4, rasterizer, dst, src, x0, coverage, count);
#else
  uint32_t si_ga = ((uint32_t*)rasterizer->color)[1];
  uint32_t si_rb = ((uint32_t*)rasterizer->color)[2];
  uint32_t si_a  = si_ga >> 16;

  while (count--)
  {
     uint32_t cov   = *coverage++;
     uint32_t rcov  = (((255+si_a * cov)>>8))^255;
     uint32_t di    = *((uint32_t*)dst);
     uint32_t di_ga = ((di & 0xff00ff00) >> 8);
     uint32_t di_rb = (di & 0x00ff00ff);
     *((uint32_t*)(dst)) =
     (((si_rb * cov + 0xff00ff + di_rb * rcov) & 0xff00ff00) >> 8)  |
      ((si_ga * cov + 0xff00ff + di_ga * rcov) & 0xff00ff00);
     dst+=4;
  }
#endif
}

static void
ctx_RGBA8_source_copy_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
#if CTX_REFERENCE
  ctx_u8_source_copy_normal_color (4, rasterizer, dst, src, x0, coverage, count);
#else
  uint32_t si_ga = ((uint32_t*)rasterizer->color)[1];
  uint32_t si_rb = ((uint32_t*)rasterizer->color)[2];

  while (count--)
  {
     uint32_t cov   = *coverage++;
     uint32_t di    = *((uint32_t*)dst);
     uint32_t di_ga = (di & 0xff00ff00);
     uint32_t di_rb = (di & 0x00ff00ff);

     uint32_t d_rb  = si_rb - di_rb;
     uint32_t d_ga  = si_ga - (di_ga>>8);

     *((uint32_t*)(dst)) =

     (((di_rb + ((d_rb * cov)>>8)) & 0x00ff00ff))  |
      ((di_ga + ((d_ga * cov)      & 0xff00ff00)));
     dst +=4;
  }
#endif
}

static void
ctx_RGBA8_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_clear_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_u8_blend_normal (int components, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended, int count)
{
  for (int j = 0; j < count; j++)
  {
  switch (components)
  {
     case 3:
       ((uint8_t*)(blended))[2] = ((uint8_t*)(src))[2];
       *((uint16_t*)(blended)) = *((uint16_t*)(src));
       break;
     case 2:
       *((uint16_t*)(blended)) = *((uint16_t*)(src));
       break;
     case 5:
       *((uint32_t*)(blended)) = *((uint32_t*)(src));
       ((uint8_t*)(blended))[4] = ((uint8_t*)(src))[4];
       break;
     case 4:
       *((uint32_t*)(blended)) = *((uint32_t*)(src));
       break;
     default:
       {
        for (int i = 0; i<components;i++)
           blended[i] = src[i];
       }
       break;
  }
    blended+=components;
    src+=components;
  }
}

/* branchless 8bit add that maxes out at 255 */
static inline uint8_t ctx_sadd8(uint8_t a, uint8_t b)
{
  uint16_t s = (uint16_t)a+b;
  return -(s>>8) | (uint8_t)s;
}

#if CTX_BLENDING_AND_COMPOSITING

#define ctx_u8_blend_define(name, CODE) \
static inline void \
ctx_u8_blend_##name (int components, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended, int count)\
{\
  for (int j = 0; j < count; j++) { \
  uint8_t *s=src; uint8_t b[components];\
  ctx_u8_deassociate_alpha (components, dst, b);\
    CODE;\
  blended[components-1] = src[components-1];\
  ctx_u8_associate_alpha (components, blended);\
  src += components;\
  dst += components;\
  blended += components;\
  }\
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
      d = (int)(ctx_sqrtf(b[c]/255.0f) * 255.4f);
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
            return (int)(CTX_CSS_RGB_TO_LUMINANCE(c));
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
            return (int)(CTX_CSS_RGB_TO_LUMINANCE(c));
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

  if ((n < 0) & (l!=n))
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if ((x > 255) & (x!=l))
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
ctx_u8_blend (int components, CtxBlend blend, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended, int count)
{
#if CTX_BLENDING_AND_COMPOSITING
  switch (blend)
  {
    case CTX_BLEND_NORMAL:      ctx_u8_blend_normal      (components, dst, src, blended, count); break;
    case CTX_BLEND_MULTIPLY:    ctx_u8_blend_multiply    (components, dst, src, blended, count); break;
    case CTX_BLEND_SCREEN:      ctx_u8_blend_screen      (components, dst, src, blended, count); break;
    case CTX_BLEND_OVERLAY:     ctx_u8_blend_overlay     (components, dst, src, blended, count); break;
    case CTX_BLEND_DARKEN:      ctx_u8_blend_darken      (components, dst, src, blended, count); break;
    case CTX_BLEND_LIGHTEN:     ctx_u8_blend_lighten     (components, dst, src, blended, count); break;
    case CTX_BLEND_COLOR_DODGE: ctx_u8_blend_color_dodge (components, dst, src, blended, count); break;
    case CTX_BLEND_COLOR_BURN:  ctx_u8_blend_color_burn  (components, dst, src, blended, count); break;
    case CTX_BLEND_HARD_LIGHT:  ctx_u8_blend_hard_light  (components, dst, src, blended, count); break;
    case CTX_BLEND_SOFT_LIGHT:  ctx_u8_blend_soft_light  (components, dst, src, blended, count); break;
    case CTX_BLEND_DIFFERENCE:  ctx_u8_blend_difference  (components, dst, src, blended, count); break;
    case CTX_BLEND_EXCLUSION:   ctx_u8_blend_exclusion   (components, dst, src, blended, count); break;
    case CTX_BLEND_COLOR:       ctx_u8_blend_color       (components, dst, src, blended, count); break;
    case CTX_BLEND_HUE:         ctx_u8_blend_hue         (components, dst, src, blended, count); break;
    case CTX_BLEND_SATURATION:  ctx_u8_blend_saturation  (components, dst, src, blended, count); break;
    case CTX_BLEND_LUMINOSITY:  ctx_u8_blend_luminosity  (components, dst, src, blended, count); break;
    case CTX_BLEND_ADDITION:    ctx_u8_blend_addition    (components, dst, src, blended, count); break;
    case CTX_BLEND_DIVIDE:      ctx_u8_blend_divide      (components, dst, src, blended, count); break;
    case CTX_BLEND_SUBTRACT:    ctx_u8_blend_subtract    (components, dst, src, blended, count); break;
  }
#else
  switch (blend)
  {
    default:                    ctx_u8_blend_normal      (components, dst, src, blended, count); break;
  }

#endif
}

CTX_INLINE static void
__ctx_u8_porter_duff (CtxRasterizer         *rasterizer,
                     int                    components,
                     uint8_t *              dst,
                     uint8_t *              src,
                     int                    x0,
                     uint8_t * __restrict__ coverage,
                     int                    count,
                     CtxCompositingMode     compositing_mode,
                     CtxFragment            fragment,
                     CtxBlend               blend)
{
  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  CtxGState *gstate = &rasterizer->state->gstate;
  uint8_t global_alpha_u8 = gstate->global_alpha_u8;
  uint8_t tsrc[components * count];
  int src_step = 0;

  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
  {
    src = &tsrc[0];
    memcpy (src, rasterizer->color, 4);
    if (blend != CTX_BLEND_NORMAL)
      ctx_u8_blend (components, blend, dst, src, src, 1);
  }
  else
  {
    float u0 = 0; float v0 = 0;
    float ud = 0; float vd = 0;
    float w0 = 1; float wd = 0;
    src = &tsrc[0];

    ctx_init_uv (rasterizer, x0, rasterizer->scanline/CTX_FULL_AA, &u0, &v0, &w0, &ud, &vd, &wd);
    fragment (rasterizer, u0, v0, w0, src, count, ud, vd, wd);
    if (blend != CTX_BLEND_NORMAL)
      ctx_u8_blend (components, blend, dst, src, src, count);
    src_step = components;
  }

  while (count--)
  {
    uint32_t cov = *coverage;

    if (CTX_UNLIKELY(global_alpha_u8 != 255))
      cov = (cov * global_alpha_u8 + 255) >> 8;

    uint8_t csrc[components];
    for (int c = 0; c < components; c++)
      csrc[c] = (src[c] * cov + 255) >> 8;

    for (int c = 0; c < components; c++)
    {
      uint32_t res = 0;
#if 1
      switch (f_s)
      {
        case CTX_PORTER_DUFF_0:             break;
        case CTX_PORTER_DUFF_1:             res += (csrc[c] ); break;
        case CTX_PORTER_DUFF_ALPHA:         res += (csrc[c] * dst[components-1] + 255) >> 8; break;
        case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (csrc[c] * (256-dst[components-1])) >> 8; break;
      }
      switch (f_d)
      {
        case CTX_PORTER_DUFF_0: break;
        case CTX_PORTER_DUFF_1:             res += dst[c]; break;
        case CTX_PORTER_DUFF_ALPHA:         res += (dst[c] * csrc[components-1] + 255) >> 8; break;
        case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (dst[c] * (256-csrc[components-1])) >> 8; break;
      }
#else
      switch (f_s)
      {
        case CTX_PORTER_DUFF_0:             break;
        case CTX_PORTER_DUFF_1:             res += (csrc[c] ); break;
        case CTX_PORTER_DUFF_ALPHA:         res += (csrc[c] * dst[components-1])/255; break;
        case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (csrc[c] * (255-dst[components-1]))/255; break;
      }
      switch (f_d)
      {
        case CTX_PORTER_DUFF_0: break;
        case CTX_PORTER_DUFF_1:             res += dst[c]; break;
        case CTX_PORTER_DUFF_ALPHA:         res += (dst[c] * csrc[components-1])/255; break;
        case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (dst[c] * (255-csrc[components-1]))/255; break;
      }
#endif
      dst[c] = res;
    }
    coverage ++;
    src+=src_step;
    dst+=components;
  }
}

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
  __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count, compositing_mode, fragment, blend);
}

#define _ctx_u8_porter_duffs(comp_format, components, source, fragment, blend) \
   switch (rasterizer->state->gstate.compositing_mode) \
   { \
     case CTX_COMPOSITE_SOURCE_ATOP: \
      __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count, \
        CTX_COMPOSITE_SOURCE_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_ATOP:\
      __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_IN:\
      __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_IN, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION:\
      __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OVER:\
      __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OVER:\
      __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_XOR:\
      __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_XOR, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OUT:\
       __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OUT:\
       __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_IN:\
       __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_IN, fragment, blend);\
       break;\
     case CTX_COMPOSITE_COPY:\
       __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_COPY, fragment, blend);\
       break;\
     case CTX_COMPOSITE_CLEAR:\
       __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
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

ctx_u8_porter_duff(RGBA8, 4,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)
//ctx_u8_porter_duff(comp_name, components,color_##blend_name,  NULL, blend_mode)


#if CTX_INLINED_NORMAL_RGBA8

ctx_u8_porter_duff(RGBA8, 4,color,   rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_GRADIENTS
ctx_u8_porter_duff(RGBA8, 4,linear_gradient, ctx_fragment_linear_gradient_RGBA8, rasterizer->state->gstate.blend_mode)
ctx_u8_porter_duff(RGBA8, 4,radial_gradient, ctx_fragment_radial_gradient_RGBA8, rasterizer->state->gstate.blend_mode)
#endif
ctx_u8_porter_duff(RGBA8, 4,image,           ctx_fragment_image_RGBA8,           rasterizer->state->gstate.blend_mode)
#endif


static inline void
ctx_RGBA8_nop (CTX_COMPOSITE_ARGUMENTS)
{
}


static inline void
ctx_setup_native_color (CtxRasterizer *rasterizer)
{
  if (rasterizer->state->gstate.source_fill.type == CTX_SOURCE_COLOR)
  {
    rasterizer->format->from_comp (rasterizer, 0,
      &rasterizer->color[0],
      &rasterizer->color_native,
      1);
  }
}

static void
ctx_setup_apply_coverage (CtxRasterizer *rasterizer)
{
  rasterizer->apply_coverage = rasterizer->format->apply_coverage ?
                               rasterizer->format->apply_coverage :
                               rasterizer->comp_op;
}

static void
ctx_setup_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components       = 4;
  rasterizer->fragment = ctx_rasterizer_get_fragment_RGBA8 (rasterizer);
  rasterizer->comp_op  = ctx_RGBA8_porter_duff_generic;
  rasterizer->comp = CTX_COV_PATH_FALLBACK;

  int blend_mode       = gstate->blend_mode;
  int compositing_mode = gstate->compositing_mode;

  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      ctx_fragment_color_RGBA8 (rasterizer, 0,0, 1,rasterizer->color, 1, 0,0,0);
      if (gstate->global_alpha_u8 != 255)
      {
        for (int c = 0; c < 4; c ++)
          rasterizer->color[c] = (rasterizer->color[c] * gstate->global_alpha_u8 + 255)>>8;
      }
      uint32_t src_pix    = ((uint32_t*)rasterizer->color)[0];
      uint32_t si_ga      = (src_pix & 0xff00ff00) >> 8;
      uint32_t si_rb      = src_pix & 0x00ff00ff;
      uint32_t si_ga_full = si_ga * 255;
      uint32_t si_rb_full = si_rb * 255;
//      uint32_t si_a       = si_ga >> 16;

      ((uint32_t*)rasterizer->color)[1] = si_ga;
      ((uint32_t*)rasterizer->color)[2] = si_rb;
      ((uint32_t*)rasterizer->color)[3] = si_ga_full;
      ((uint32_t*)rasterizer->color)[4] = si_rb_full;
    }

#if CTX_INLINED_NORMAL_RGBA8
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_RGBA8_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_RGBA8_copy_normal;
          if (gstate->source_fill.type == CTX_SOURCE_COLOR)
            rasterizer->comp = CTX_COV_PATH_RGBA8_COPY;

        }
        else if (gstate->global_alpha_u8 == 0)
        {
          rasterizer->comp_op = ctx_RGBA8_nop;
        }
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              rasterizer->comp_op = ctx_RGBA8_source_over_normal_color;
              if ( ((float*)rasterizer->color)[3] >= 0.999f)
                rasterizer->comp = CTX_COV_PATH_RGBA8_COPY;
            }
            else
            {
              rasterizer->comp_op = ctx_RGBAF_porter_duff_color_normal;
            }
            break;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_linear_gradient_normal;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_radial_gradient_normal;
            break;
#endif
          case CTX_SOURCE_TEXTURE:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_image_normal;
            break;
          default:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_color;
            break;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_linear_gradient;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_radial_gradient;
            break;
#endif
          case CTX_SOURCE_TEXTURE:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_image;
            break;
          default:
            rasterizer->comp_op = ctx_RGBA8_porter_duff_generic;
            break;
        }
        break;
    }

#else

  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {

      if (blend_mode == CTX_BLEND_NORMAL)
      {
        if(compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_RGBA8_source_copy_normal_color;
          rasterizer->comp = CTX_COV_PATH_RGBA8_COPY;
        }
        else if (compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
        {
          if (rasterizer->color[components-1] == 255)
          {
            rasterizer->comp_op = ctx_RGBA8_source_copy_normal_color;
            rasterizer->comp = CTX_COV_PATH_RGBA8_COPY;
          }
          else
          {
            rasterizer->comp_op = ctx_RGBA8_source_over_normal_color;
            rasterizer->comp = CTX_COV_PATH_RGBA8_OVER;
          }
        }
      }
      else if (compositing_mode == CTX_COMPOSITE_CLEAR)
      {
        rasterizer->comp_op = ctx_RGBA8_clear_normal;
      }
  }
  else if (blend_mode == CTX_BLEND_NORMAL)
  {
    if(compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
    {
       rasterizer->comp_op = ctx_RGBA8_source_over_normal_fragment;
       rasterizer->comp = CTX_COV_PATH_RGBA8_OVER_FRAGMENT;
    }
    else if (compositing_mode == CTX_COMPOSITE_COPY)
    {
       rasterizer->comp_op = ctx_RGBA8_source_copy_normal_fragment;
       rasterizer->comp = CTX_COV_PATH_RGBA8_COPY_FRAGMENT;
    }
  }
#endif
  ctx_setup_apply_coverage (rasterizer);
}


static inline void
ctx_setup_RGB (CtxRasterizer *rasterizer)
{
  ctx_setup_RGBA8 (rasterizer);
  ctx_setup_native_color (rasterizer);

  rasterizer->comp = CTX_COV_PATH_FALLBACK;
}



#if CTX_ENABLE_RGB332
static void
ctx_setup_RGB332 (CtxRasterizer *rasterizer)
{
  ctx_setup_RGBA8 (rasterizer);
  ctx_setup_native_color (rasterizer);

  if (rasterizer->comp == CTX_COV_PATH_RGBA8_COPY)
    rasterizer->comp = CTX_COV_PATH_RGB332_COPY;
  else
    rasterizer->comp = CTX_COV_PATH_FALLBACK;
}
#endif

#if CTX_ENABLE_RGB565
static void
ctx_setup_RGB565 (CtxRasterizer *rasterizer)
{
  ctx_setup_RGBA8 (rasterizer);
  ctx_setup_native_color (rasterizer);

  if (rasterizer->comp == CTX_COV_PATH_RGBA8_COPY)
    rasterizer->comp = CTX_COV_PATH_RGB565_COPY;
  else
    rasterizer->comp = CTX_COV_PATH_FALLBACK;
}
#endif

#if CTX_ENABLE_RGB8
static void
ctx_setup_RGB8 (CtxRasterizer *rasterizer)
{
  ctx_setup_RGBA8 (rasterizer);
  ctx_setup_native_color (rasterizer);

  if (rasterizer->comp == CTX_COV_PATH_RGBA8_COPY)
    rasterizer->comp = CTX_COV_PATH_RGB8_COPY;
  else
    rasterizer->comp = CTX_COV_PATH_FALLBACK;
}
#endif

static inline void
ctx_composite_convert (CTX_COMPOSITE_ARGUMENTS)
{
  uint8_t pixels[count * rasterizer->format->ebpp];
  rasterizer->format->to_comp (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  rasterizer->format->from_comp (rasterizer, x0, &pixels[0], dst, count);
}

#if CTX_ENABLE_FLOAT
static inline void
ctx_float_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  float w0 = 1; float wd = 0;

  ctx_init_uv (rasterizer, x0, rasterizer->scanline/CTX_FULL_AA, &u0, &v0, &w0, &ud, &vd, &wd);

  while (count--)
  {
    uint8_t cov = *coverage;
    float covf = ctx_u8_to_float (cov);
    for (int c = 0; c < components; c++)
      dstf[c] = dstf[c]*(1.0f-covf) + srcf[c]*covf;
    dstf += components;
    coverage ++;
  }
}

static inline void
ctx_float_clear_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  while (count--)
  {
#if 0
    uint8_t cov = *coverage;
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
      float ralpha = 1.0f - ctx_u8_to_float (cov);
      for (int c = 0; c < components; c++)
        { dstf[c] = (dstf[c] * ralpha); }
    }
    coverage ++;
#endif
    dstf += components;
  }
}


static inline void
ctx_float_source_over_normal_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;
  while (count--)
  {
    uint8_t cov = *coverage;
    float fcov = ctx_u8_to_float (cov);
    float ralpha = 1.0f - fcov * srcf[components-1];
    for (int c = 0; c < components; c++)
      dstf[c] = srcf[c]*fcov + dstf[c] * ralpha;
    coverage ++;
    dstf+= components;
  }
}

static inline void
ctx_float_source_copy_normal_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;

  while (count--)
  {
    uint8_t cov = *coverage;
    float fcov = ctx_u8_to_float (cov);
    float ralpha = 1.0f - fcov;
    for (int c = 0; c < components; c++)
      dstf[c] = (srcf[c]*fcov + dstf[c] * ralpha);
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

  if ((n < 0.0f) & (l != n))
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if ((x > 1.0f) & (x != l))
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
static inline void \
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

  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  float   global_alpha_f = rasterizer->state->gstate.global_alpha_f;
  
  if (rasterizer->state->gstate.source_fill.type == CTX_SOURCE_COLOR)
  {
    float tsrc[components];

    while (count--)
    {
      uint8_t cov = *coverage;
#if 1
      if (
        CTX_UNLIKELY((compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 1.0f)||
        (cov == 0 && (compositing_mode == CTX_COMPOSITE_SOURCE_OVER ||
        compositing_mode == CTX_COMPOSITE_XOR               ||
        compositing_mode == CTX_COMPOSITE_DESTINATION_OUT   ||
        compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      
        ))))
      {
        coverage ++;
        dstf+=components;
        continue;
      }
#endif
      memcpy (tsrc, rasterizer->color, sizeof(tsrc));

      if (blend != CTX_BLEND_NORMAL)
        ctx_float_blend (components, blend, dstf, tsrc, tsrc);
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
        float res;
        /* these switches and this whole function is written to be
         * inlined when compiled when the enum values passed in are
         * constants.
         */
        switch (f_s)
        {
          case CTX_PORTER_DUFF_0: res = 0.0f; break;
          case CTX_PORTER_DUFF_1:             res = (tsrc[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         res = (tsrc[c] *       dstf[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res = (tsrc[c] * (1.0f-dstf[components-1])); break;
        }
        switch (f_d)
        {
          case CTX_PORTER_DUFF_0: dstf[c] = res; break;
          case CTX_PORTER_DUFF_1:             dstf[c] = res + (dstf[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         dstf[c] = res + (dstf[c] *       tsrc[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: dstf[c] = res + (dstf[c] * (1.0f-tsrc[components-1])); break;
        }
      }
      coverage ++;
      dstf     +=components;
    }
  }
  else
  {
    float tsrc[components];
    float u0 = 0; float v0 = 0;
    float ud = 0; float vd = 0;
    float w0 = 1; float wd = 0;
    for (int c = 0; c < components; c++) tsrc[c] = 0.0f;
    ctx_init_uv (rasterizer, x0, rasterizer->scanline/CTX_FULL_AA, &u0, &v0, &w0, &ud, &vd, &wd);

    while (count--)
    {
      uint8_t cov = *coverage;
#if 1
      if (
        CTX_UNLIKELY((compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 1.0f)||
        (cov == 0 && (compositing_mode == CTX_COMPOSITE_SOURCE_OVER ||
        compositing_mode == CTX_COMPOSITE_XOR               ||
        compositing_mode == CTX_COMPOSITE_DESTINATION_OUT   ||
        compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      
        ))))
      {
        u0 += ud;
        v0 += vd;
        coverage ++;
        dstf+=components;
        continue;
      }
#endif

      fragment (rasterizer, u0, v0, w0, tsrc, 1, ud, vd, wd);
      if (blend != CTX_BLEND_NORMAL)
        ctx_float_blend (components, blend, dstf, tsrc, tsrc);
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
        float res;
        /* these switches and this whole function is written to be
         * inlined when compiled when the enum values passed in are
         * constants.
         */
        switch (f_s)
        {
          case CTX_PORTER_DUFF_0: res = 0.0f; break;
          case CTX_PORTER_DUFF_1:             res = (tsrc[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         res = (tsrc[c] *       dstf[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res = (tsrc[c] * (1.0f-dstf[components-1])); break;
        }
        switch (f_d)
        {
          case CTX_PORTER_DUFF_0: dstf[c] = res; break;
          case CTX_PORTER_DUFF_1:             dstf[c] = res + (dstf[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         dstf[c] = res + (dstf[c] *       tsrc[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: dstf[c] = res + (dstf[c] * (1.0f-tsrc[components-1])); break;
        }
      }
      coverage ++;
      dstf     +=components;
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
#endif

#if CTX_ENABLE_RGBAF

ctx_float_porter_duff(RGBAF, 4,color,   rasterizer->fragment, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(RGBAF, 4,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
#if CTX_GRADIENTS
ctx_float_porter_duff(RGBAF, 4,linear_gradient, ctx_fragment_linear_gradient_RGBAF, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(RGBAF, 4,radial_gradient, ctx_fragment_radial_gradient_RGBAF, rasterizer->state->gstate.blend_mode)
#endif
ctx_float_porter_duff(RGBAF, 4,image,           ctx_fragment_image_RGBAF,           rasterizer->state->gstate.blend_mode)


#if CTX_GRADIENTS
#define ctx_float_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_float_porter_duff(comp_name, components,color_##blend_name,            rasterizer->fragment,                               blend_mode)\
ctx_float_porter_duff(comp_name, components,generic_##blend_name,          rasterizer->fragment,               blend_mode)\
ctx_float_porter_duff(comp_name, components,linear_gradient_##blend_name,  ctx_fragment_linear_gradient_RGBA8, blend_mode)\
ctx_float_porter_duff(comp_name, components,radial_gradient_##blend_name,  ctx_fragment_radial_gradient_RGBA8, blend_mode)\
ctx_float_porter_duff(comp_name, components,image_##blend_name,            ctx_fragment_image_RGBAF,           blend_mode)
#else
#define ctx_float_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_float_porter_duff(comp_name, components,color_##blend_name,            rasterizer->fragment,                               blend_mode)\
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

#if 1
static void
ctx_RGBAF_source_over_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_over_normal_color (4, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif
#endif

static void
ctx_setup_RGBAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 4;
  rasterizer->fragment = ctx_rasterizer_get_fragment_RGBAF (rasterizer);
  rasterizer->comp = CTX_COV_PATH_FALLBACK;
#if 1
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_RGBAF_porter_duff_color;
      ctx_fragment_color_RGBAF (rasterizer, 0,0,1, rasterizer->color, 1, 0,0,0);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;

      if (rasterizer->format->from_comp)
        rasterizer->format->from_comp (rasterizer, 0,
          &rasterizer->color[0],
          &rasterizer->color_native,
          1);
    }
  else
#endif
  {
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
          if (gstate->source_fill.type == CTX_SOURCE_COLOR)
            rasterizer->comp = CTX_COV_PATH_RGBAF_COPY;

        }
        else if (gstate->global_alpha_u8 == 0)
        {
          rasterizer->comp_op = ctx_RGBA8_nop;
        }
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              rasterizer->comp_op = ctx_RGBAF_source_over_normal_color;
              if ( ((float*)rasterizer->color)[3] >= 0.999f)
                rasterizer->comp = CTX_COV_PATH_RGBAF_COPY;
            }
            else
            {
              rasterizer->comp_op = ctx_RGBAF_porter_duff_color_normal;
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
          case CTX_SOURCE_TEXTURE:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_image_normal;
            break;
          default:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_color;
            break;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_linear_gradient;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_radial_gradient;
            break;
#endif
          case CTX_SOURCE_TEXTURE:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_image;
            break;
          default:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
  ctx_setup_apply_coverage (rasterizer);
}

#endif
#if CTX_ENABLE_GRAYAF

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  float rgba[4];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0 ; i < count; i++)
  {
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0f, rgba);
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgba);
  ((float*)out)[1] = rgba[3];
     out = ((float*)(out)) + 2;
     x += dx;
     y += dy;
  }
}

static void
ctx_fragment_radial_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  float rgba[4];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i ++)
  {
  float v = 0.0f;
  if ((g->radial_gradient.r1-g->radial_gradient.r0) > 0.0f)
    {
      v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
      v = (v - g->radial_gradient.r0) / (g->radial_gradient.rdelta);
    }
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 0.0, rgba);
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgba);
  ((float*)out)[1] = rgba[3];
     out = ((float*)(out)) + 2;
     x += dx;
     y += dy;
  }
}
#endif

static void
ctx_fragment_color_GRAYAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i++)
  {
     ctx_color_get_graya (rasterizer->state, &g->color, (float*)out);
     out = ((float*)(out)) + 2;
     x += dx;
     y += dy;
  }
}

static void ctx_fragment_image_GRAYAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  uint8_t rgba[4*count];
  float rgbaf[4*count];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  switch (buffer->format->bpp)
    {
#if CTX_FRAGMENT_SPECIALIZE
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz); break;
#endif
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz);       break;
    }
  for (int c = 0; c < 2 * count; c ++) { 
    rgbaf[c] = ctx_u8_to_float (rgba[c]);
    ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgbaf);
    ((float*)out)[1] = rgbaf[3];
    out = ((float*)out) + 2;
  }
}

static CtxFragment ctx_rasterizer_get_fragment_GRAYAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:           return ctx_fragment_image_GRAYAF;
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_GRAYAF;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_GRAYAF;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_GRAYAF;
#endif
    }
  return ctx_fragment_color_GRAYAF;
}

ctx_float_porter_duff(GRAYAF, 2,color,   rasterizer->fragment, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(GRAYAF, 2,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
ctx_float_porter_duff(GRAYAF, 2,color_normal,   rasterizer->fragment, CTX_BLEND_NORMAL)
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
ctx_GRAYAF_source_copy_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_copy_normal_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

static void
ctx_setup_GRAYAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 2;
  rasterizer->fragment = ctx_rasterizer_get_fragment_GRAYAF (rasterizer);
  rasterizer->comp = CTX_COV_PATH_FALLBACK;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_GRAYAF_porter_duff_color;
      ctx_color_get_rgba (rasterizer->state, &gstate->source_fill.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;

      if (rasterizer->format->from_comp)
        rasterizer->format->from_comp (rasterizer, 0,
          &rasterizer->color[0],
          &rasterizer->color_native,
          1);
    }
  else
  {
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
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = ctx_RGBA8_nop;
#if 1
              else //if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = ctx_GRAYAF_source_copy_normal_color;
#endif
              //else
          //      rasterizer->comp_op = ctx_GRAYAF_porter_duff_color_normal;
            }
            else
            {
              rasterizer->comp_op = ctx_GRAYAF_porter_duff_color_normal;
            }
            break;
          default:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_color;
            break;
          default:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
  ctx_setup_apply_coverage (rasterizer);
}

#endif
#if CTX_ENABLE_GRAYF

static void
ctx_composite_GRAYF (CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;

  float temp[count*2];
  for (unsigned int i = 0; i < count; i++)
  {
    temp[i*2] = dstf[i];
    temp[i*2+1] = 1.0f;
  }
  rasterizer->comp_op (rasterizer, (uint8_t*)temp, rasterizer->color, x0, coverage, count);
  for (unsigned int i = 0; i < count; i++)
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
static inline void
ctx_composite_direct (CTX_COMPOSITE_ARGUMENTS)
{
  // for better performance, this could be done without a pre/post conversion,
  // by swapping R and B of source instead... as long as it is a color instead
  // of gradient or image
  //
  //
  rasterizer->comp_op (rasterizer, dst, rasterizer->color, x0, coverage, count);
}

#if CTX_ENABLE_CMYKAF

static void
ctx_fragment_other_CMYKAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  float *cmyka = (float*)out;
  float _rgba[4 * count];
  float *rgba = &_rgba[0];
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:
        ctx_fragment_image_RGBAF (rasterizer, x, y, z, rgba, count, dx, dy, dz);
        break;
      case CTX_SOURCE_COLOR:
        ctx_fragment_color_RGBAF (rasterizer, x, y, z, rgba, count, dx, dy, dz);
        break;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT:
        ctx_fragment_linear_gradient_RGBAF (rasterizer, x, y, z, rgba, count, dx, dy, dz);
        break;
      case CTX_SOURCE_RADIAL_GRADIENT:
        ctx_fragment_radial_gradient_RGBAF (rasterizer, x, y, z, rgba, count, dx, dy, dz);
        break;
#endif
      default:
        rgba[0]=rgba[1]=rgba[2]=rgba[3]=0.0f;
        break;
    }
  for (int i = 0; i < count; i++)
  {
    cmyka[4]=rgba[3];
    ctx_rgb_to_cmyk (rgba[0], rgba[1], rgba[2], &cmyka[0], &cmyka[1], &cmyka[2], &cmyka[3]);
    cmyka += 5;
    rgba += 4;
  }
}

static void
ctx_fragment_color_CMYKAF (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  float *cmyka = (float*)out;
  float cmyka_in[5];
  ctx_color_get_cmyka (rasterizer->state, &gstate->source_fill.color, cmyka_in);
  for (int i = 0; i < count; i++)
  {
    for (int c = 0; c < 4; c ++)
    {
      cmyka[c] = (1.0f - cmyka_in[c]);
    }
    cmyka[4] = cmyka_in[4];
    cmyka += 5;
  }
}

static CtxFragment ctx_rasterizer_get_fragment_CMYKAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_COLOR:
        return ctx_fragment_color_CMYKAF;
    }
  return ctx_fragment_other_CMYKAF;
}

ctx_float_porter_duff (CMYKAF, 5,color,           rasterizer->fragment, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff (CMYKAF, 5,generic,         rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
ctx_float_porter_duff (CMYKAF, 5,color_normal,            rasterizer->fragment, CTX_BLEND_NORMAL)
ctx_float_porter_duff (CMYKAF, 5,generic_normal,          rasterizer->fragment, CTX_BLEND_NORMAL)

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
ctx_CMYKAF_source_copy_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_copy_normal_color (5, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

static void
ctx_setup_CMYKAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 5;
  rasterizer->fragment = ctx_rasterizer_get_fragment_CMYKAF (rasterizer);
  rasterizer->comp = CTX_COV_PATH_FALLBACK;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_CMYKAF_porter_duff_color;
      rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
      ctx_color_get_cmyka (rasterizer->state, &gstate->source_fill.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        ((float*)rasterizer->color)[components-1] *= gstate->global_alpha_f;

      if (rasterizer->format->from_comp)
        rasterizer->format->from_comp (rasterizer, 0,
          &rasterizer->color[0],
          &rasterizer->color_native,
          1);
    }
  else
  {
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
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = ctx_RGBA8_nop;
              else if (((float*)rasterizer->color)[components-1] == 1.0f)
              {
                rasterizer->comp_op = ctx_CMYKAF_source_copy_normal_color;
                rasterizer->comp = CTX_COV_PATH_CMYKAF_COPY;
              }
              else
                rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
            }
            else
            {
              rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
            }
            break;
          default:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_color;
            break;
          default:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
            break;
        }
        break;
    }
#else

    if (gstate->blend_mode == CTX_BLEND_NORMAL &&
        gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp = CTX_COV_PATH_CMYKAF_COPY;
        }
        else if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER &&
                 rasterizer->color[components-1] == 255)
        {
          rasterizer->comp = CTX_COV_PATH_CMYKAF_COPY;
        }
    }
#endif
  ctx_setup_apply_coverage (rasterizer);
}

static void
ctx_setup_CMYKA8 (CtxRasterizer *rasterizer)
{
  ctx_setup_CMYKAF (rasterizer);

  if (rasterizer->comp == CTX_COV_PATH_CMYKAF_COPY)
    rasterizer->comp = CTX_COV_PATH_CMYKA8_COPY;
}

static void
ctx_setup_CMYK8 (CtxRasterizer *rasterizer)
{
  ctx_setup_CMYKAF (rasterizer);
  if (rasterizer->comp == CTX_COV_PATH_CMYKAF_COPY)
    rasterizer->comp = CTX_COV_PATH_CMYK8_COPY;
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
      if ((a != 0) & (a != 255))
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
      if ((a != 0.0f) & (a != 1.0f))
        {
          float recip = 1.0f/a;
          c *= recip;
          m *= recip;
          y *= recip;
          k *= recip;
        }
      c = 1.0f - c;
      m = 1.0f - m;
      y = 1.0f - y;
      k = 1.0f - k;
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
ctx_GRAY1_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *graya, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int bitno = x&7;
      if ((bitno == 0) & (count >= 7))
      {
        if (*pixel == 0)
        {
          for (int i = 0; i < 8; i++)
          {
            *graya++ = 0; *graya++ = 255;
          }
          x+=8; count-=7; pixel++;
          continue;
        }
        else if (*pixel == 0xff)
        {
          for (int i = 0; i < 8 * 2; i++)
          {
            *graya++ = 255;
          }
          x+=8; count-=7; pixel++;
          continue;
        }
      }
      *graya++ = 255 * ((*pixel) & (1<<bitno));
      *graya++ = 255;
      pixel+= (bitno ==7);
      x++;
    }
}

inline static void
ctx_GRAYA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int gray = rgba[0];
      int bitno = x&7;
      if (gray >= 128)
        *pixel |= (1<<bitno);
      else
        *pixel &= (~ (1<<bitno));
      pixel+= (bitno==7);
      x++;
      rgba +=2;
    }
}

#else

inline static void
ctx_GRAY1_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  uint32_t *dst = (uint32_t*)rgba;
  while (count--)
    {
      int bitno = x&7;

      if ((bitno == 0) & (count >=7))
      {
        /* special case some bit patterns when decoding */
        if (*pixel == 0)
        {
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          x+=8; count-=7; pixel++;
          continue;
        }
        else if (*pixel == 0xff)
        {
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          x+=8; count-=7; pixel++;
          continue;
        }
        else if (*pixel == 0x0f)
        {
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          x+=8; count-=7; pixel++;
          continue;
        }
        else if (*pixel == 0xfc)
        {
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          x+=8; count-=7; pixel++;
          continue;
        }
        else if (*pixel == 0x3f)
        {
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          x+=8; count-=7; pixel++;
          continue;
        }
      }
      *dst++=0xff000000 + 0x00ffffff * ((*pixel & (1<< bitno ) )!=0);
      pixel += (bitno ==7);
      x++;
    }
}

inline static void
ctx_RGBA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int gray = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      int bitno = x&7;
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/aa, 0, 127);
      if (gray >= 128)
        *pixel |= (1<< bitno);
      else
        *pixel &= (~ (1<< bitno));
      pixel+= (bitno ==7);
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
      uint8_t val = (((*pixel) >> ( (x&3) <<1)) & 3) * 85;
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
      val = ctx_sadd8 (val, 40) >> 6;
      *pixel = (*pixel & (~ (3 << ( (x&3) <<1) ) ))
                      | ( (val << ( (x&3) <<1) ) );
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
  uint32_t *dst = (uint32_t*)rgba;
  while (count--)
    {
      int bitno = x & 3;
      if ((bitno == 0) & (count >=3))
      {
        /* special case some bit patterns when decoding */
        if (*pixel == 0)
        {
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          x+=4; count-=3; pixel++;
          continue;
        }
        else if (*pixel == 0xff)
        {
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          x+=4; count-=3; pixel++;
          continue;
        }
        else if (*pixel == 0x55)
        {
          *dst++ = 0xff555555;
          *dst++ = 0xff555555;
          *dst++ = 0xff555555;
          *dst++ = 0xff555555;
          x+=4; count-=3; pixel++;
          continue;
        }
        else if (*pixel == 0xaa)
        {
          *dst++ = 0xffaaaaaa;
          *dst++ = 0xffaaaaaa;
          *dst++ = 0xffaaaaaa;
          *dst++ = 0xffaaaaaa;
          x+=4; count-=3; pixel++;
          continue;
        }
        else if (*pixel == 0x0f)
        {
          *dst++ = 0xff000000;
          *dst++ = 0xff000000;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          x+=4; count-=3; pixel++;
          continue;
        }
        else if (*pixel == 0xfc)
        {
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xff000000;
          x+=4; count-=3; pixel++;
          continue;
        }
        else if (*pixel == 0x3f)
        {
          *dst++ = 0xff000000;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          *dst++ = 0xffffffff;
          x+=4; count-=3; pixel++;
          continue;
        }
      }
      {
        uint8_t val = (((*pixel) >> ( (bitno) <<1)) & 3) * 85;
        *dst = val + val * 256u + val * 256u * 256u + 255u * 256u * 256u * 256u;
        if (bitno==3)
          { pixel+=1; }
        x++;
        dst++;
      }
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
      *pixel = (*pixel & (~ (3 << ((x&3) <<1) ) ))
                      | ( (val << ((x&3) <<1) ) );
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
  for (int i = 0; i < count; i ++)
    {
      pixel[i] = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba + i * 4);
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
ctx_fragment_linear_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
        uint8_t *dst = (uint8_t*)out;
#if CTX_DITHER
  int scan = rasterizer->scanline / CTX_FULL_AA;
  int ox = (int)x;
#endif
  for (int i = 0; i < count;i ++)
  {
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  {
    uint8_t rgba[4];
    ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 1.0f, rgba);
    ctx_rgba_to_graya_u8 (rasterizer->state, rgba, dst);
   
  }

#if CTX_DITHER
  ctx_dither_graya_u8 ((uint8_t*)dst, ox + i, scan, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
  dst += 2;
  x += dx;
  y += dy;
  }
}

static void
ctx_fragment_radial_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  uint8_t *dst = (uint8_t*)out;
#if CTX_DITHER
  int scan = rasterizer->scanline / CTX_FULL_AA;
  int ox = (int)x;
#endif

  for (int i = 0; i < count;i ++)
  {
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  float v = (ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  {
    uint8_t rgba[4];
    ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 1.0, rgba);
    ctx_rgba_to_graya_u8 (rasterizer->state, rgba, dst);
  }
#if CTX_DITHER
  ctx_dither_graya_u8 ((uint8_t*)dst, ox+i, scan, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
  dst += 2;
  x += dx;
  y += dy;
  }
}
#endif

static void
ctx_fragment_color_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  uint16_t *dst = (uint16_t*)out;
  uint16_t pix;
  ctx_color_get_graya_u8 (rasterizer->state, &g->color, (uint8_t*)&pix);
  for (int i = 0; i <count; i++)
  {
    dst[i]=pix;
  }
}

static void ctx_fragment_image_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz)
{
  uint8_t rgba[4*count];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  switch (buffer->format->bpp)
    {
#if CTX_FRAGMENT_SPECIALIZE
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz); break;
#endif
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, z, rgba, count, dx, dy, dz);       break;
    }
  for (int i = 0; i < count; i++)
    ctx_rgba_to_graya_u8 (rasterizer->state, &rgba[i*4], &((uint8_t*)out)[i*2]);
}

static CtxFragment ctx_rasterizer_get_fragment_GRAYA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:           return ctx_fragment_image_GRAYA8;
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_GRAYA8;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_GRAYA8;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_GRAYA8;
#endif
    }
  return ctx_fragment_color_GRAYA8;
}

ctx_u8_porter_duff(GRAYA8, 2,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
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
#if 1
  ctx_u8_source_over_normal_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
#else
  uint8_t tsrc[5];
  *((uint32_t*)tsrc) = *((uint32_t*)src);

  while (count--)
  {
    uint32_t cov = *coverage++;
    uint32_t common =(((((255+(tsrc[1] * cov))>>8))^255 ));
    dst[0] =  ((((tsrc[0] * cov)) + (dst[0] * common ))>>8);
    dst[1] =  ((((tsrc[1] * cov)) + (dst[1] * common ))>>8);
    dst+=2;
  }
#endif
}

static void
ctx_GRAYA8_source_copy_normal_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_source_copy_normal_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

inline static int
ctx_is_opaque_color (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (gstate->global_alpha_u8 != 255)
    return 0;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
  {
    uint8_t ga[2];
    ctx_color_get_graya_u8 (rasterizer->state, &gstate->source_fill.color, ga);
    return ga[1] == 255;
  }
  return 0;
}

static void
ctx_setup_GRAYA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 2;
  rasterizer->fragment = ctx_rasterizer_get_fragment_GRAYA8 (rasterizer);
  rasterizer->comp_op  = ctx_GRAYA8_porter_duff_generic;
  rasterizer->comp = CTX_COV_PATH_FALLBACK;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      ctx_fragment_color_GRAYA8 (rasterizer, 0,0, 1,rasterizer->color, 1, 0,0,0);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          rasterizer->color[c] = (rasterizer->color[c] * gstate->global_alpha_u8)/255;

      if (rasterizer->format->from_comp)
        rasterizer->format->from_comp (rasterizer, 0,
          &rasterizer->color[0],
          &rasterizer->color_native,
          1);
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
          rasterizer->comp = CTX_COV_PATH_GRAYA8_COPY;
        }
        else if (gstate->global_alpha_u8 == 0)
          rasterizer->comp_op = ctx_RGBA8_nop;
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (rasterizer->color[components-1] == 0)
                rasterizer->comp_op = ctx_RGBA8_nop;
              else if (rasterizer->color[components-1] == 255)
              {
                rasterizer->comp_op = ctx_GRAYA8_source_copy_normal_color;
                rasterizer->comp = CTX_COV_PATH_GRAYA8_COPY;
              }
              else
                rasterizer->comp_op = ctx_GRAYA8_source_over_normal_color;
            }
            else
            {
              rasterizer->comp_op = ctx_GRAYA8_porter_duff_generic_normal;
            }
            break;
          default:
            rasterizer->comp_op = ctx_GRAYA8_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        rasterizer->comp_op = ctx_GRAYA8_porter_duff_generic;
        break;
    }
#else
    if ((gstate->blend_mode == CTX_BLEND_NORMAL) &
        (gstate->source_fill.type == CTX_SOURCE_COLOR))
    {
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp = CTX_COV_PATH_GRAYA8_COPY;
        }
        else if ((gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER) &
                 (rasterizer->color[components-1] == 255))
        {
          rasterizer->comp = CTX_COV_PATH_GRAYA8_COPY;
        }
    }
#endif
  ctx_setup_apply_coverage (rasterizer);
}

#if CTX_ENABLE_GRAY4
static void
ctx_setup_GRAY4 (CtxRasterizer *rasterizer)
{
  ctx_setup_GRAYA8 (rasterizer);
  if (rasterizer->comp == CTX_COV_PATH_GRAYA8_COPY)
    rasterizer->comp = CTX_COV_PATH_GRAY4_COPY;
  else
  rasterizer->comp = CTX_COV_PATH_FALLBACK;
}
#endif

#if CTX_ENABLE_GRAY2
static void
ctx_setup_GRAY2 (CtxRasterizer *rasterizer)
{
  ctx_setup_GRAYA8 (rasterizer);
  if (rasterizer->comp == CTX_COV_PATH_GRAYA8_COPY)
    rasterizer->comp = CTX_COV_PATH_GRAY2_COPY;
  else
    rasterizer->comp = CTX_COV_PATH_FALLBACK;
}
#endif

#if CTX_ENABLE_GRAY1
static void
ctx_setup_GRAY1 (CtxRasterizer *rasterizer)
{
  ctx_setup_GRAYA8 (rasterizer);
  if (rasterizer->comp == CTX_COV_PATH_GRAYA8_COPY)
    rasterizer->comp = CTX_COV_PATH_GRAY1_COPY;
  else
    rasterizer->comp = CTX_COV_PATH_FALLBACK;
}
#endif

static void
ctx_setup_GRAY8 (CtxRasterizer *rasterizer)
{
  ctx_setup_GRAYA8 (rasterizer);
  if (rasterizer->comp == CTX_COV_PATH_GRAYA8_COPY)
    rasterizer->comp = CTX_COV_PATH_GRAY8_COPY;
  else
    rasterizer->comp = CTX_COV_PATH_FALLBACK;
}

#endif

#endif

inline static void
ctx_332_unpack (uint8_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue)
{
  *green = (((pixel >> 2) & 7)*255)/7;
  *red   = (((pixel >> 5) & 7)*255)/7;
  *blue  = ((((pixel & 3) << 1) | ((pixel >> 2) & 1))*255)/7;
}

static inline uint8_t
ctx_332_pack (uint8_t red,
              uint8_t green,
              uint8_t blue)
{
  return ((ctx_sadd8(red,15) >> 5) << 5)
        |((ctx_sadd8(green,15) >> 5) << 2)
        |(ctx_sadd8(blue,15) >> 6);
}
#if CTX_ENABLE_RGB332

static inline uint8_t
ctx_888_to_332 (uint32_t in)
{
  uint8_t *rgb=(uint8_t*)(&in);
  return ctx_332_pack (rgb[0],rgb[1],rgb[2]);
}

static inline uint32_t
ctx_332_to_888 (uint8_t in)
{
  uint32_t ret = 0;
  uint8_t *rgba=(uint8_t*)&ret;
  ctx_332_unpack (in,
                  &rgba[0],
                  &rgba[1],
                  &rgba[2]);
  rgba[3] = 255;
  return ret;
}

static inline void
ctx_RGB332_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      ctx_332_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2]);
#if CTX_RGB332_ALPHA
      if ((rgba[0]==255) & (rgba[2] == 255) & (rgba[1]==0))
        { rgba[3] = 0; }
      else
#endif
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
#if CTX_RGB332_ALPHA
      if (rgba[3]==0)
        { pixel[0] = ctx_332_pack (255, 0, 255); }
      else
#endif
        { pixel[0] = ctx_332_pack (rgba[0], rgba[1], rgba[2]); }
      pixel+=1;
      rgba +=4;
    }
}

static void
ctx_composite_RGB332 (CTX_COMPOSITE_ARGUMENTS)
{
#if 1
  if (CTX_LIKELY(rasterizer->comp_op == ctx_RGBA8_source_over_normal_color))
  {
    uint32_t si_ga = ((uint32_t*)rasterizer->color)[1];
    uint32_t si_rb = ((uint32_t*)rasterizer->color)[2];
    uint32_t si_a  = si_ga >> 16;

    while (count--)
    {
      uint32_t cov   = *coverage++;
      uint32_t rcov  = (((255+si_a * cov)>>8))^255;
      uint32_t di    = ctx_332_to_888 (*((uint8_t*)dst));
      uint32_t di_ga = ((di & 0xff00ff00) >> 8);
      uint32_t di_rb = (di & 0x00ff00ff);
      *((uint8_t*)(dst)) =
      ctx_888_to_332((((si_rb * cov + 0xff00ff + di_rb * rcov) & 0xff00ff00) >> 8)  |
       ((si_ga * cov + 0xff00ff + di_ga * rcov) & 0xff00ff00));
       dst+=1;
    }
    return;
  }
#endif
  uint8_t pixels[count * 4];
  ctx_RGB332_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_RGBA8_to_RGB332 (rasterizer, x0, &pixels[0], dst, count);
}

#endif
static inline uint16_t
ctx_565_pack (uint8_t  red,
              uint8_t  green,
              uint8_t  blue,
              const int      byteswap)
{
#if 0
  // is this extra precision warranted?
  // for 332 it gives more pure white..
  // it might be the case also for generic 565
  red = ctx_sadd8 (red, 4);
  green = ctx_sadd8 (green, 3);
  blue = ctx_sadd8 (blue, 4);
#endif

  uint32_t c = (red >> 3) << 11;
  c |= (green >> 2) << 5;
  c |= blue >> 3;
  if (byteswap)
    { return (c>>8) | (c<<8); } /* swap bytes */
  return c;
}

#if CTX_ENABLE_RGB565 | CTX_ENABLE_RGB565_BYTESWAPPED

static inline void
ctx_565_unpack (const uint16_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue,
                const int byteswap)
{
  uint16_t byteswapped;
  if (byteswap)
    { byteswapped = (pixel>>8) | (pixel<<8); }
  else
    { byteswapped  = pixel; }
  uint8_t b  =  (byteswapped & 31) <<3;
  uint8_t g  = ( (byteswapped>>5) & 63) <<2;
  uint8_t r  = ( (byteswapped>>11) & 31) <<3;

#if 0
  *blue  = (b > 248) * 255 + (b <= 248) * b;
  *green = (g > 248) * 255 + (g <= 248) * g;
  *red   = (r > 248) * 255 + (r <= 248) * r;
#else
  *blue = b;
  *green = g;
  *red = r;
#endif
}

static inline uint32_t
ctx_565_unpack_32 (const uint16_t pixel,
                   const int byteswap)
{
  uint16_t byteswapped;
  if (byteswap)
    { byteswapped = (pixel>>8) | (pixel<<8); }
  else
    { byteswapped  = pixel; }
  uint32_t b   = (byteswapped & 31) <<3;
  uint32_t g = ( (byteswapped>>5) & 63) <<2;
  uint32_t r   = ( (byteswapped>>11) & 31) <<3;
#if 0
  b = (b > 248) * 255 + (b <= 248) * b;
  g = (g > 248) * 255 + (g <= 248) * g;
  r = (r > 248) * 255 + (r <= 248) * r;
#endif

  return r +  (g << 8) + (b << 16) + (0xff << 24);
}


static inline uint16_t
ctx_888_to_565 (uint32_t in, int byteswap)
{
  uint8_t *rgb=(uint8_t*)(&in);
  return ctx_565_pack (rgb[0],rgb[1],rgb[2], byteswap);
}

static inline uint32_t
ctx_565_to_888 (uint16_t in, int byteswap)
{
  uint32_t ret = 0;
  uint8_t *rgba=(uint8_t*)&ret;
  ctx_565_unpack (in,
                  &rgba[0],
                  &rgba[1],
                  &rgba[2],
                  byteswap);
  //rgba[3]=255;
  return ret;
}

#endif
#if CTX_ENABLE_RGB565


static inline void
ctx_RGB565_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      // XXX : checking the raw value for alpha before unpack will be faster
      ((uint32_t*)(rgba))[0] = ctx_565_unpack_32 (*pixel, 0);
#if CTX_RGB565_ALPHA
      if ((rgba[0]==255) & (rgba[2] == 255) & (rgba[1]==0))
        { rgba[3] = 0; }
#endif
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
#if CTX_RGB565_ALPHA
      if (rgba[3]==0)
        { pixel[0] = ctx_565_pack (255, 0, 255, 0); }
      else
#endif
        { pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 0); }
      pixel+=1;
      rgba +=4;
    }
}

static void
ctx_RGBA8_source_over_normal_color (CTX_COMPOSITE_ARGUMENTS);
static void
ctx_RGBA8_source_copy_normal_color (CTX_COMPOSITE_ARGUMENTS);

static void
ctx_composite_RGB565 (CTX_COMPOSITE_ARGUMENTS)
{
#if 0 // code is OK - but less code is better
  if (rasterizer->comp_op == ctx_RGBA8_source_over_normal_color)
  {
    uint32_t si_ga = ((uint32_t*)rasterizer->color)[1];
    uint32_t si_rb = ((uint32_t*)rasterizer->color)[2];
    uint32_t si_a  = si_ga >> 16;
    while (count--)
    {
        uint32_t cov   = *coverage++;
        uint32_t rcov  = (((255+si_a * cov)>>8))^255;
        uint32_t di    = ctx_565_to_888 (*((uint16_t*)dst), 0);
        uint32_t di_ga = ((di & 0xff00ff00) >> 8);
        uint32_t di_rb = (di & 0x00ff00ff);
        *((uint16_t*)(dst)) =
        ctx_888_to_565((((si_rb * cov + 0xff00ff + di_rb * rcov) & 0xff00ff00) >> 8)  |
         ((si_ga * cov + 0xff00ff + di_ga * rcov) & 0xff00ff00), 0);
         dst+=2;
    }
    return;
  }
  if (rasterizer->comp_op == ctx_RGBA8_source_copy_normal_color)
  {
    uint32_t si_ga = ((uint32_t*)rasterizer->color)[1];
    uint32_t si_rb = ((uint32_t*)rasterizer->color)[2];
    uint32_t si_a  = si_ga >> 16;
    while (count--)
    {
        uint32_t cov   = *coverage++;
        uint32_t rcov  = cov^255;
        uint32_t di    = ctx_565_to_888 (*((uint16_t*)dst), 0);
        uint32_t di_ga = ((di & 0xff00ff00) >> 8);
        uint32_t di_rb = (di & 0x00ff00ff);
        *((uint16_t*)(dst)) =
        ctx_888_to_565((((si_rb * cov + 0xff00ff + di_rb * rcov) & 0xff00ff00) >> 8)  |
         ((si_ga * cov + 0xff00ff + di_ga * rcov) & 0xff00ff00), 0);
         dst+=2;
    }
    return;
  }
#endif

  uint8_t pixels[count * 4];
  ctx_RGB565_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_RGBA8_to_RGB565 (rasterizer, x0, &pixels[0], dst, count);
}
#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED

void
ctx_RGB565_BS_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count);
void
ctx_RGBA8_to_RGB565_BS (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count);

static void
ctx_composite_RGB565_BS (CTX_COMPOSITE_ARGUMENTS)
{
#if 0 // code is OK - but not faster - at least no on risc-V
  if ((rasterizer->comp_op == ctx_RGBA8_source_over_normal_color)
      ||(rasterizer->comp_op == ctx_RGBA8_source_copy_normal_color))
  {
    uint32_t si_ga = ((uint32_t*)rasterizer->color)[1];
    uint32_t si_rb = ((uint32_t*)rasterizer->color)[2];
    uint32_t si_a  = si_ga >> 16;
    while (count--)
    {
      uint32_t cov   = *coverage++;
      uint32_t rcov  = (((255+si_a * cov)>>8))^255;
      uint32_t di    = ctx_565_to_888 (*((uint16_t*)dst), 1);
      uint32_t di_ga = ((di & 0xff00ff00) >> 8);
      uint32_t di_rb = (di & 0x00ff00ff);
      *((uint16_t*)(dst)) =
      ctx_888_to_565((((si_rb * cov + 0xff00ff + di_rb * rcov) & 0xff00ff00) >> 8)  |
       ((si_ga * cov + 0xff00ff + di_ga * rcov) & 0xff00ff00), 1);
       dst+=2;
    }
    return;
  }
#endif
#if 1
  if (rasterizer->comp_op == ctx_RGBA8_source_copy_normal_color)
  {
    uint32_t si_ga = ((uint32_t*)rasterizer->color)[1];
    uint32_t si_rb = ((uint32_t*)rasterizer->color)[2];
    while (count--)
    {
        uint8_t cov   = *coverage++;
        uint8_t rcov  = cov^255;
        uint32_t di    = ctx_565_to_888 (*((uint16_t*)dst), 1);
        uint32_t di_ga = ((di & 0xff00ff00) >> 8);
        uint32_t di_rb = (di & 0x00ff00ff);
        *((uint16_t*)(dst)) =
        ctx_888_to_565((((si_rb * cov + 0xff00ff + di_rb * rcov) & 0xff00ff00) >> 8)  |
         ((si_ga * cov + 0xff00ff + di_ga * rcov) & 0xff00ff00), 1);
         dst+=2;
    }
    return;
  }
#endif

  uint8_t pixels[count * 4];
  ctx_RGB565_BS_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_RGBA8_to_RGB565_BS (rasterizer, x0, &pixels[0], dst, count);
}
#endif


static inline uint32_t
ctx_over_RGBA8 (uint32_t dst, uint32_t src, uint32_t cov)
{
  uint32_t si_ga = (src & 0xff00ff00) >> 8;
  uint32_t si_rb = src & 0x00ff00ff;
  uint32_t si_a  = si_ga >> 16;
  uint32_t rcov  = ((255+si_a * cov)>>8)^255;
  uint32_t di_ga = ( dst & 0xff00ff00) >> 8;
  uint32_t di_rb = dst & 0x00ff00ff;
  return
     ((((si_rb * cov) + 0xff00ff + (di_rb * rcov)) & 0xff00ff00) >> 8)  |
      (((si_ga * cov) + 0xff00ff + (di_ga * rcov)) & 0xff00ff00);
}


static inline uint32_t
ctx_over_RGBA8_full (uint32_t dst, uint32_t src)
{
  uint32_t si_ga = (src & 0xff00ff00) >> 8;
  uint32_t si_rb = src & 0x00ff00ff;
  uint32_t si_a  = si_ga >> 16;
  uint32_t rcov  = si_a^255;
  uint32_t di_ga = (dst & 0xff00ff00) >> 8;
  uint32_t di_rb = dst & 0x00ff00ff;
  return
     ((((si_rb * 255) + 0xff00ff + (di_rb * rcov)) & 0xff00ff00) >> 8)  |
      (((si_ga * 255) + 0xff00ff + (di_ga * rcov)) & 0xff00ff00);
}

static inline uint32_t
ctx_over_RGBA8_2 (uint32_t dst, uint32_t si_ga, uint32_t si_rb, uint32_t si_a, uint32_t cov)
{
  uint32_t rcov  = ((si_a * cov)/255)^255;
  uint32_t di_ga = (dst & 0xff00ff00) >> 8;
  uint32_t di_rb = dst & 0x00ff00ff;
  return
     ((((si_rb * cov) + 0xff00ff + (di_rb * rcov)) & 0xff00ff00) >> 8)  |
      (((si_ga * cov) + 0xff00ff + (di_ga * rcov)) & 0xff00ff00);
}

static inline uint32_t
ctx_over_RGBA8_full_2 (uint32_t dst, uint32_t si_ga_full, uint32_t si_rb_full, uint32_t si_a)
{
  uint32_t rcov = si_a^255;
  uint32_t di_ga = ( dst & 0xff00ff00) >> 8;
  uint32_t di_rb = dst & 0x00ff00ff;
  return
     ((((si_rb_full) + (di_rb * rcov)) & 0xff00ff00) >> 8)  |
      (((si_ga_full) + (di_ga * rcov)) & 0xff00ff00);
}

static inline void ctx_span_set_color (uint32_t *dst_pix, uint32_t val, int count)
{
  if (count>0)
  while(count--)
    *dst_pix++=val;
}

static inline void ctx_span_set_colorb  (uint32_t *dst_pix, uint32_t val, int count)
{
  while(count--)
    *dst_pix++=val;
}

static inline void ctx_span_set_colorbu (uint32_t *dst_pix, uint32_t val, unsigned int count)
{
  while(count--)
    *dst_pix++=val;
}

static inline void ctx_span_set_color_x4 (uint32_t *dst_pix, uint32_t *val, int count)
{
  if (count>0)
  while(count--)
  {
    *dst_pix++=val[0];
    *dst_pix++=val[1];
    *dst_pix++=val[2];
    *dst_pix++=val[3];
  }
}

#if CTX_FAST_FILL_RECT

#if 1

static inline void ctx_RGBA8_image_rgba8_RGBA8_nearest_fill_rect_copy (CtxRasterizer *rasterizer, int x0, int y0, int x1, int y1, int copy)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  float w0 = 1; float wd = 0;
  ctx_init_uv (rasterizer, x0, rasterizer->scanline/CTX_FULL_AA,&u0, &v0, &w0, &ud, &vd, &wd);

  uint32_t *dst = ( (uint32_t *) rasterizer->buf);
  int blit_stride = rasterizer->blit_stride/4;
  dst += (y0 - rasterizer->blit_y) * blit_stride;
  dst += (x0);

  unsigned int width = x1-x0+1;
  unsigned int height = y1-y0+1;

  //CtxSource *g = &rasterizer->state->gstate.source_fill;
  //CtxBuffer *buffer = g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;

  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = 
     g->texture.buffer->color_managed?g->texture.buffer->color_managed:g->texture.buffer;
  int bwidth  = buffer->width;
  int bheight = buffer->height;
  int u = x0;// + 0.5f;
  int v = y0;// + 0.5f;

  uint32_t *src = ((uint32_t*)buffer->data) + bwidth * v + u;

  int pre = ctx_mini(ctx_maxi(-u,0), width);

  width-=pre;
  u+=pre;

  int core = ctx_mini (width, bwidth - u);

  if (copy)
  {
    if (core>0)
    {
      uint32_t *t_dst = dst;
      for (unsigned int y = 0; y < height; y++)
      {
         if (CTX_LIKELY((v >= 0 && v < bheight)))
         {
           memcpy (t_dst, src + pre, core * 4);
         }
         v++;
         src += bwidth;
         t_dst += blit_stride;
      }
    }
  }
  else
  {
    if (core>0)
    {
      uint32_t *t_dst = dst;
      for (unsigned int y = 0; y < height; y++)
      {
         if (CTX_LIKELY((v >= 0 && v < bheight)))
         {
           ctx_RGBA8_source_over_normal_full_cov_buf (rasterizer,
               (uint8_t*)t_dst, NULL, x0+pre, NULL, core, (uint8_t*)src);
         }
         v++;
         src += bwidth;
         t_dst += blit_stride;
      }
    }
  }
}
#endif


static void
ctx_composite_fill_rect_aligned (CtxRasterizer *rasterizer,
                                 int            x0,
                                 int            y0,
                                 int            x1,
                                 int            y1,
                                 uint8_t        cov)
{
  int blit_x = rasterizer->blit_x;
  int blit_y = rasterizer->blit_y;
  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;
  int blit_stride = rasterizer->blit_stride;

  x0 = ctx_maxi (x0, blit_x);
  x1 = ctx_mini (x1, blit_x + blit_width - 1);
  y0 = ctx_maxi (y0, blit_y);
  y1 = ctx_mini (y1, blit_y + blit_height - 1);

  int width = x1 - x0 + 1;
  int height= y1 - y0 + 1;
  //
  if (CTX_UNLIKELY ((width <=0) | (height <= 0)))
    return;

  CtxCovPath comp = rasterizer->comp;
  uint8_t *dst;

  // this could be done here, but is not used
  // by a couple of the cases
#define INIT_ENV do {\
  rasterizer->scanline = y0 * CTX_FULL_AA; \
  dst = ( (uint8_t *) rasterizer->buf); \
  dst += (y0 - blit_y) * blit_stride; \
  dst += (x0 * rasterizer->format->bpp)/8;}while(0);

//if (CTX_UNLIKELY(width <=0 || height <= 0))
//  return;
  if (cov == 255)
  {
    switch (comp)
    {
    case CTX_COV_PATH_RGBA8_COPY:
    {
      uint32_t color;
      memcpy (&color, (uint32_t*)rasterizer->color, sizeof (color));
      INIT_ENV;
      if (CTX_UNLIKELY(width == 1))
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          uint32_t *dst_i = (uint32_t*)&dst[0];
          *dst_i = color;
          dst += blit_stride;
        }
      }
      else
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
#if 0
          uint32_t *dst_pix = (uint32_t*)&dst[0];
          int count = width;
          while(count--)
            *dst_pix++=color;
#else
          ctx_span_set_colorbu ((uint32_t*)&dst[0], color, width);
#endif
          dst += blit_stride;
        }
      }
      return;
    }
    case CTX_COV_PATH_RGBAF_COPY:
    case CTX_COV_PATH_GRAY8_COPY:
    case CTX_COV_PATH_GRAYA8_COPY:
    case CTX_COV_PATH_GRAYAF_COPY:
    case CTX_COV_PATH_CMYKAF_COPY:
    case CTX_COV_PATH_RGB565_COPY:
    case CTX_COV_PATH_RGB332_COPY:
    case CTX_COV_PATH_RGB8_COPY:
    case CTX_COV_PATH_CMYK8_COPY:
    case CTX_COV_PATH_CMYKA8_COPY:
    {
      uint8_t *color = (uint8_t*)&rasterizer->color_native;
      unsigned int bytes = rasterizer->format->bpp/8;
      INIT_ENV;

      switch (bytes)
      {
        case 1:
          {
          uint8_t col = *color;
          if (width == 1)
          for (unsigned int y = y0; y <= (unsigned)y1; y++)
          {
            *dst = col;
            dst += blit_stride;
          }
          else
          for (unsigned int y = y0; y <= (unsigned)y1; y++)
          {
#if 0
            uint8_t *dst_i = (uint8_t*)&dst[0];
            for (int x = 0; x < width; x++) *dst_i++ = col;
#else
            memset (dst, col, width);
#endif
            dst += blit_stride;
          }
          }
          break;
        case 2:
          {
            uint16_t val = ((uint16_t*)color)[0];
            for (unsigned int y = y0; y <= (unsigned)y1; y++)
            {
              uint16_t *dst_i = (uint16_t*)&dst[0];
              for (int x = 0; x < width; x++)
                 *dst_i++ = val;
              dst += blit_stride;
            }
          }
          break;
        case 3:
          for (unsigned int y = y0; y <= (unsigned)y1; y++)
          {
            uint8_t *dst_i = (uint8_t*)&dst[0];
            for (int x = 0; x < width; x++)
                for (unsigned int b = 0; b < 3; b++) *dst_i++ = color[b];
            dst += blit_stride;
          }
          break;
        case 4:
          {
            uint32_t val = ((uint32_t*)color)[0];
            if (width == 1)
            for (unsigned int y = y0; y <= (unsigned)y1; y++)
            {
              *((uint32_t*)&dst[0]) = val;
              dst += blit_stride;
            }
            else
            for (unsigned int y = y0; y <= (unsigned)y1; y++)
            {
              //uint32_t *dst_i = (uint32_t*)&dst[0];
              ctx_span_set_colorbu ((uint32_t*)&dst[0], val, width);
              dst += blit_stride;
            }
          }
          break;
        case 5:
          for (unsigned int y = y0; y <= (unsigned)y1; y++)
          {
            uint8_t *dst_i = (uint8_t*)&dst[0];
            for (int x = 0; x < width; x++)
               for (unsigned int b = 0; b < 5; b++) *dst_i++ = color[b];
            dst += blit_stride;
          }
          break;
        case 16:
          for (unsigned int y = y0; y <= (unsigned)y1; y++)
          {
            uint8_t *dst_i = (uint8_t*)&dst[0];
            for (int x = 0; x < width; x++)for (unsigned int b = 0; b < 16; b++) *dst_i++ = color[b];
            dst += blit_stride;
          }
          break;
        default:
          for (unsigned int y = y0; y <= (unsigned)y1; y++)
          {
            uint8_t *dst_i = (uint8_t*)&dst[0];
            for (int x = 0; x < width; x++)
              for (unsigned int b = 0; b < bytes; b++)
                *dst_i++ = color[b];
            dst += blit_stride;
          }
      }
      return;
    }
    case CTX_COV_PATH_RGBA8_OVER:
    {
      uint32_t si_ga_full = ((uint32_t*)rasterizer->color)[3];
      uint32_t si_rb_full = ((uint32_t*)rasterizer->color)[4];
      uint32_t si_a  = rasterizer->color[3];
      INIT_ENV;

      if (CTX_UNLIKELY(width == 1))
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          ((uint32_t*)(dst))[0] = ctx_over_RGBA8_full_2 (
             ((uint32_t*)(dst))[0], si_ga_full, si_rb_full, si_a);
          dst += blit_stride;
        }
      }
      else
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          uint32_t *dst_i = (uint32_t*)&dst[0];
          for (int i = 0; i < width; i++)
          {
            dst_i[i] = ctx_over_RGBA8_full_2 (dst_i[i], si_ga_full, si_rb_full, si_a);
          }
          dst += blit_stride;
        }
      }
      return;
    }
    case CTX_COV_PATH_RGBA8_COPY_FRAGMENT:
    {
      CtxFragment fragment = rasterizer->fragment;
      CtxMatrix *transform = &rasterizer->state->gstate.source_fill.transform;
      //CtxExtend extend = rasterizer->state->gstate.extend;
      INIT_ENV;

#if 0
      if (fragment == ctx_fragment_image_rgba8_RGBA8_nearest_copy)
      {
        ctx_RGBA8_image_rgba8_RGBA8_nearest_fill_rect_copy (rasterizer, x0, y0, x1, y1, 1);
        return;
      }
      else
#endif
#if 0
      if (fragment == ctx_fragment_image_rgba8_RGBA8_bi_scale)
      {
        ctx_RGBA8_image_rgba8_RGBA8_bi_scaled_fill_rect (rasterizer, x0, y0, x1,
y1, 1);
        return;
      }
#endif

      if (CTX_LIKELY(ctx_matrix_no_perspective (transform)))
      {
        int scan = rasterizer->scanline/CTX_FULL_AA;
        float u0, v0, ud, vd, w0, wd;
        ctx_init_uv (rasterizer, x0, scan, &u0, &v0, &w0, &ud, &vd, &wd);
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          fragment (rasterizer, u0, v0, w0, &dst[0], width, ud, vd, wd);
          u0 -= vd;
          v0 += ud;
          dst += blit_stride;
        }
      }
      else
      {
        int scan = rasterizer->scanline/CTX_FULL_AA;
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          float u0, v0, ud, vd, w0, wd;
          ctx_init_uv (rasterizer, x0, scan + y-y0, &u0, &v0, &w0, &ud, &vd, &wd);
          fragment (rasterizer, u0, v0, w0, &dst[0], width, ud, vd, wd);
          dst += blit_stride;
        }
      }
      return;
    }
    case CTX_COV_PATH_RGBA8_OVER_FRAGMENT:
    {
      //CtxFragment fragment = rasterizer->fragment;
      //CtxExtend extend = rasterizer->state->gstate.extend;
#if 0
      if (fragment == ctx_fragment_image_rgba8_RGBA8_nearest_copy)
      {
        ctx_RGBA8_image_rgba8_RGBA8_nearest_fill_rect_copy (rasterizer, x0, y0, x1, y1, 0);
        return;
      }
      else
#endif
#if 0
      if (fragment == ctx_fragment_image_rgba8_RGBA8_bi_scale)
      {
        ctx_RGBA8_image_rgba8_RGBA8_bi_scaled_fill_rect (rasterizer, x0, y0, x1,
y1, 0);
        return;
      }
#endif
      INIT_ENV;
      ctx_RGBA8_source_over_normal_full_cov_fragment (rasterizer,
                         &dst[0], NULL, x0, NULL, width, y1-y0+1);
      return;
    }
    break;
    default:
    break;
    }
  }
  else
  {
    switch (comp)
    {
    case CTX_COV_PATH_RGBA8_COPY:
    {
      uint32_t color;
      memcpy (&color, (uint32_t*)rasterizer->color, sizeof (color));
      INIT_ENV;
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          uint32_t *dst_i = (uint32_t*)&dst[0];
          for (unsigned int i = 0; i < (unsigned)width; i++)
          {
            dst_i[i] = ctx_lerp_RGBA8 (dst_i[i], color, cov);
          }
          dst += blit_stride;
        }
        return;
      }
    }
    case CTX_COV_PATH_RGBAF_COPY:
    {
      float *color = ((float*)rasterizer->color);
      float covf = cov / 255.0f;
      INIT_ENV;
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          float *dst_f = (float*)&dst[0];
          for (unsigned int i = 0; i < (unsigned)width; i++)
          {
            for (unsigned int c = 0; c < 4; c++)
              dst_f[i*4+c] = ctx_lerpf (dst_f[i*4+c], color[c], covf);
          }
          dst += blit_stride;
        }
        return;
      }
    }
    case CTX_COV_PATH_RGBA8_OVER:
    {
      uint32_t color;
      memcpy (&color, (uint32_t*)rasterizer->color, sizeof (color));
      INIT_ENV;
      if (width == 1)
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          uint32_t *dst_i = (uint32_t*)&dst[0];
          *dst_i = ctx_over_RGBA8 (*dst_i, color, cov);
          dst += blit_stride;
        }
      }
      else
      {
        for (unsigned int y = y0; y <= (unsigned)y1; y++)
        {
          uint32_t *dst_i = (uint32_t*)&dst[0];
          for (unsigned int i = 0; i < (unsigned)width; i++)
          {
            dst_i[i] = ctx_over_RGBA8 (dst_i[i], color, cov);
          }
          dst += blit_stride;
        }
      }
      return;
    }
    break;
    default:
    break;
    }
  }

  INIT_ENV;
#undef INIT_ENV


  /* fallback */
  {
    uint8_t coverage[width];
    memset (coverage, cov, sizeof (coverage) );
    uint8_t *rasterizer_src = rasterizer->color;
    void (*apply_coverage)(CtxRasterizer *r, uint8_t *dst, uint8_t *src,
                         int x, uint8_t *coverage, unsigned int count) =
      rasterizer->apply_coverage;

    for (unsigned int y = y0; y <= (unsigned)y1; y++)
    {
      apply_coverage (rasterizer, &dst[0], rasterizer_src, x0, coverage, width);
      rasterizer->scanline += CTX_FULL_AA;
      dst += blit_stride;
    }
  }
}

void
CTX_SIMD_SUFFIX (ctx_composite_fill_rect) (CtxRasterizer *rasterizer,
                          float          x0,
                          float          y0,
                          float          x1,
                          float          y1,
                          uint8_t        cov);

void
CTX_SIMD_SUFFIX (ctx_composite_fill_rect) (CtxRasterizer *rasterizer,
                          float          x0,
                          float          y0,
                          float          x1,
                          float          y1,
                          uint8_t        cov)
{
  if(((int)(ctx_fmod1f (x0) < 0.01f) | (ctx_fmod1f(x0) > 0.99f)) &
     ((int)(ctx_fmod1f (y0) < 0.01f) | (ctx_fmod1f(y0) > 0.99f)) &
     ((int)(ctx_fmod1f (x1) < 0.01f) | (ctx_fmod1f(x1) > 0.99f)) &
     ((int)(ctx_fmod1f (y1) < 0.01f) | (ctx_fmod1f(y1) > 0.99f)))
  {
    /* best-case scenario axis aligned rectangle */
    ctx_composite_fill_rect_aligned (rasterizer, (int)x0, (int)y0, (int)(x1-1), (int)(y1-1), 255);
    return;
  }

  int blit_x = rasterizer->blit_x;
  int blit_y = rasterizer->blit_y;
  int blit_stride = rasterizer->blit_stride;
  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;
  uint8_t *rasterizer_src = rasterizer->color;
  void (*apply_coverage)(CtxRasterizer *r, uint8_t *dst, uint8_t *src,
                       int x, uint8_t *coverage, unsigned int count) =
    rasterizer->apply_coverage;

  x0 = ctx_maxf (x0, blit_x);
  y0 = ctx_maxf (y0, blit_y);
  x1 = ctx_minf (x1, blit_x + blit_width);
  y1 = ctx_minf (y1, blit_y + blit_height);

  uint8_t left = (int)(255-ctx_fmod1f (x0) * 255);
  uint8_t top  = (int)(255-ctx_fmod1f (y0) * 255);
  uint8_t right  = (int)(ctx_fmod1f (x1) * 255);
  uint8_t bottom = (int)(ctx_fmod1f (y1) * 255);

  x0 = ctx_floorf (x0);
  y0 = ctx_floorf (y0);
  x1 = ctx_floorf (x1+7/8.0f);
  y1 = ctx_floorf (y1+15/15.0f);

  int has_top    = (top < 255);
  int has_bottom = (bottom <255);
  int has_right  = (right >0);
  int has_left   = (left >0);

  if (x1 >= blit_x + blit_width) has_right = 0;
  if (y1 >= blit_y + blit_height) has_bottom = 0;

  int width = (int)(x1 - x0);

  if ((width >0))
  {
     uint8_t *dst = ( (uint8_t *) rasterizer->buf);
     uint8_t coverage[width+2];
     uint32_t x0i = (int)x0+has_left;
     uint32_t x1i = (int)x1-has_right;
     uint32_t y0i = (int)y0+has_top;
     uint32_t y1i = (int)y1-has_bottom;
     dst += (((int)y0) - blit_y) * blit_stride;
     dst += ((int)x0) * rasterizer->format->bpp/8;

     if (has_top)
     {
       int i = 0;
       if (has_left)
       {
         coverage[i++] = (top * left + 255) >> 8;
       }
       for (unsigned int x = x0i; x < x1i; x++)
         coverage[i++] = top;
       if (has_right)
         coverage[i++]= (top * right + 255) >> 8;

       apply_coverage (rasterizer, dst, rasterizer_src, (int)x0, coverage, width);
       dst += blit_stride;
     }

  if (y1-y0-has_top-has_bottom > 0)
  {
    if (has_left)
      ctx_composite_fill_rect_aligned (rasterizer, (int)x0, y0i,
                                                   (int)x0, y1i-1, left);
    if (has_right)
      ctx_composite_fill_rect_aligned (rasterizer, (int)x1-1, y0i,
                                                   (int)x1-1, y1i-1, right);

    if (width - has_left - has_right > 0)
      ctx_composite_fill_rect_aligned (rasterizer, x0i,y0i,
                                          x1i-1,y1i-1,255);

    dst += blit_stride * (y1i-y0i);
  }
    if (has_bottom)
    {
      int i = 0;
      if (has_left)
        coverage[i++] = (bottom * left + 255) >> 8;
      for (unsigned int x = x0i; x < x1i; x++)
        coverage[i++] = bottom;
      coverage[i++]= (bottom * right + 255) >> 8;

      apply_coverage (rasterizer,dst, rasterizer_src, (int)x0, coverage, width);
    }
  }
}

#if CTX_FAST_STROKE_RECT
void
CTX_SIMD_SUFFIX(ctx_composite_stroke_rect) (CtxRasterizer *rasterizer,
                           float          x0,
                           float          y0,
                           float          x1,
                           float          y1,
                           float          line_width);

void
CTX_SIMD_SUFFIX(ctx_composite_stroke_rect) (CtxRasterizer *rasterizer,
                           float          x0,
                           float          y0,
                           float          x1,
                           float          y1,
                           float          line_width)
{
      float lwmod = ctx_fmod1f (line_width);
      int lw = (int)ctx_floorf (line_width + 0.5f);
      int is_compat_even = (lw % 2 == 0) && (lwmod < 0.1f); // only even linewidths implemented properly
      int is_compat_odd = (lw % 2 == 1) && (lwmod < 0.1f); // only even linewidths implemented properly

      float off_x = 0;
      float off_y = 0;

      if (is_compat_odd)
      {
        off_x = 0.5f;
        off_y = (CTX_FULL_AA/2)*1.0f / (CTX_FULL_AA);
      }

      if((is_compat_odd | is_compat_even) &

     (((int)(ctx_fmod1f (x0-off_x) < 0.01f) | (ctx_fmod1f(x0-off_x) > 0.99f)) &
     ((int)(ctx_fmod1f (y0-off_y) < 0.01f) | (ctx_fmod1f(y0-off_y) > 0.99f)) &
     ((int)(ctx_fmod1f (x1-off_x) < 0.01f) | (ctx_fmod1f(x1-off_x) > 0.99f)) &
     ((int)(ctx_fmod1f (y1-off_y) < 0.01f) | (ctx_fmod1f(y1-off_y) > 0.99f))))


      {
        int bw = lw/2+1;
        int bwb = lw/2;

        if (is_compat_even)
        {
          bw = lw/2;
        }
        /* top */
        ctx_composite_fill_rect_aligned (rasterizer,
                                         (int)x0-bwb, (int)y0-bwb,
                                         (int)x1+bw-1, (int)y0+bw-1, 255);
        /* bottom */
        ctx_composite_fill_rect_aligned (rasterizer,
                                         (int)x0-bwb, (int)y1-bwb,
                                         (int)x1-bwb-1, (int)y1+bw-1, 255);

        /* left */
        ctx_composite_fill_rect_aligned (rasterizer,
                                         (int)x0-bwb, (int)y0+1,
                                         (int)x0+bw-1, (int)y1-bwb, 255);
        /* right */
        ctx_composite_fill_rect_aligned (rasterizer,
                                         (int)x1-bwb, (int)y0+1,
                                         (int)x1+bw-1, (int)y1+bw-1, 255);
      }
      else
      {
        float hw = line_width/2;


        /* top */
        ctx_composite_fill_rect (rasterizer,
                                 x0+hw, y0-hw,
                                 x1-hw, y0+hw, 255);
        /* bottom */
        ctx_composite_fill_rect (rasterizer,
                                 x0+hw, y1-hw,
                                 x1-hw, y1+hw, 255);

        /* left */
        ctx_composite_fill_rect (rasterizer,
                                 x0-hw, y0+hw,
                                 x0+hw, y1-hw, 255);
        /* right */

        ctx_composite_fill_rect (rasterizer,
                                 x1-hw, y0+hw,
                                 x1+hw, y1-hw, 255);

        /* corners */

        ctx_composite_fill_rect (rasterizer,
                                 x0-hw, y0-hw,
                                 x0+hw, y0+hw, 255);
        ctx_composite_fill_rect (rasterizer,
                                 x1-hw, y1-hw,
                                 x1+hw, y1+hw, 255);
        ctx_composite_fill_rect (rasterizer,
                                 x1-hw, y0-hw,
                                 x1+hw, y0+hw, 255);
        ctx_composite_fill_rect (rasterizer,
                                 x0-hw, y1-hw,
                                 x0+hw, y1+hw, 255);
      }
}
#endif
#endif



static void
CTX_SIMD_SUFFIX (ctx_composite_setup) (CtxRasterizer *rasterizer)
{
  if (CTX_UNLIKELY (rasterizer->comp_op==NULL))
  {
#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
  switch (rasterizer->state->gstate.source_fill.type)
  {
    case CTX_SOURCE_LINEAR_GRADIENT:
    case CTX_SOURCE_RADIAL_GRADIENT:
      ctx_gradient_cache_prime (rasterizer);
      break;
    case CTX_SOURCE_TEXTURE:

      _ctx_matrix_multiply (&rasterizer->state->gstate.source_fill.transform,
                            &rasterizer->state->gstate.transform,
                            &rasterizer->state->gstate.source_fill.set_transform
                            );
#if 0
      rasterizer->state->gstate.source_fill.transform_inv =
                           rasterizer->state->gstate.source_fill.transform;
#endif
      ctx_matrix_invert (&rasterizer->state->gstate.source_fill.transform);

#if 0
      if (!rasterizer->state->gstate.source_fill.texture.buffer->color_managed)
      {
        _ctx_texture_prepare_color_management (rasterizer->state,
        rasterizer->state->gstate.source_fill.texture.buffer);
      }
#endif
      break;
  }
#endif
#endif
  }
    rasterizer->format->setup (rasterizer);

}


const CtxPixelFormatInfo CTX_SIMD_SUFFIX(ctx_pixel_formats)[]=
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
    ctx_RGB8_to_RGBA8, ctx_RGBA8_to_RGB8, ctx_composite_convert, ctx_setup_RGB8,
  },
#endif
#if CTX_ENABLE_GRAY1
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY1, 1, 1, 2, 1, 1, CTX_FORMAT_GRAYA8,
    ctx_GRAY1_to_GRAYA8, ctx_GRAYA8_to_GRAY1, ctx_composite_convert, ctx_setup_GRAY1,
#else
    CTX_FORMAT_GRAY1, 1, 1, 4, 1, 1, CTX_FORMAT_RGBA8,
    ctx_GRAY1_to_RGBA8, ctx_RGBA8_to_GRAY1, ctx_composite_convert, ctx_setup_RGB,
#endif
  },
#endif
#if CTX_ENABLE_GRAY2
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY2, 1, 2, 2, 4, 4, CTX_FORMAT_GRAYA8,
    ctx_GRAY2_to_GRAYA8, ctx_GRAYA8_to_GRAY2, ctx_composite_convert, ctx_setup_GRAY2,
#else
    CTX_FORMAT_GRAY2, 1, 2, 4, 4, 4, CTX_FORMAT_RGBA8,
    ctx_GRAY2_to_RGBA8, ctx_RGBA8_to_GRAY2, ctx_composite_convert, ctx_setup_RGB,
#endif
  },
#endif
#if CTX_ENABLE_GRAY4
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY4, 1, 4, 2, 16, 16, CTX_FORMAT_GRAYA8,
    ctx_GRAY4_to_GRAYA8, ctx_GRAYA8_to_GRAY4, ctx_composite_convert, ctx_setup_GRAY4,
#else
    CTX_FORMAT_GRAY4, 1, 4, 4, 16, 16, CTX_FORMAT_GRAYA8,
    ctx_GRAY4_to_RGBA8, ctx_RGBA8_to_GRAY4, ctx_composite_convert, ctx_setup_RGB,
#endif
  },
#endif
#if CTX_ENABLE_GRAY8
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY8, 1, 8, 2, 0, 0, CTX_FORMAT_GRAYA8,
    ctx_GRAY8_to_GRAYA8, ctx_GRAYA8_to_GRAY8, ctx_composite_convert, ctx_setup_GRAY8,
#else
    CTX_FORMAT_GRAY8, 1, 8, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_GRAY8_to_RGBA8, ctx_RGBA8_to_GRAY8, ctx_composite_convert, ctx_setup_RGB,
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
    ctx_GRAYA8_to_RGBA8, ctx_RGBA8_to_GRAYA8, ctx_composite_convert, ctx_setup_RGB,
#endif
  },
#endif
#if CTX_ENABLE_RGB332
  {
    CTX_FORMAT_RGB332, 3, 8, 4, 12, 12, CTX_FORMAT_RGBA8,
    ctx_RGB332_to_RGBA8,  ctx_RGBA8_to_RGB332,
    ctx_composite_RGB332, ctx_setup_RGB332,
  },
#endif
#if CTX_ENABLE_RGB565
  {
    CTX_FORMAT_RGB565, 3, 16, 4, 16, 32, CTX_FORMAT_RGBA8,
    ctx_RGB565_to_RGBA8,  ctx_RGBA8_to_RGB565,
    ctx_composite_RGB565, ctx_setup_RGB565,
  },
#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED
  {
    CTX_FORMAT_RGB565_BYTESWAPPED, 3, 16, 4, 16, 32, CTX_FORMAT_RGBA8,
    ctx_RGB565_BS_to_RGBA8,
    ctx_RGBA8_to_RGB565_BS,
    ctx_composite_RGB565_BS, ctx_setup_RGB565,
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
    NULL, NULL, ctx_composite_CMYKA8, ctx_setup_CMYKA8,
  },
#endif
#if CTX_ENABLE_CMYK8
  {
    CTX_FORMAT_CMYK8, 5, 32, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, ctx_composite_CMYK8, ctx_setup_CMYK8,
  },
#endif
#if CTX_ENABLE_YUV420
  {
    CTX_FORMAT_YUV420, 1, 8, 4, 0, 0, CTX_FORMAT_RGBA8,
    NULL, NULL, ctx_composite_convert, ctx_setup_RGB,
  },
#endif
  {
    CTX_FORMAT_NONE, 0, 0, 0, 0, 0, (CtxPixelFormat)0, NULL, NULL, NULL, NULL,
  }
};

#endif // CTX_COMPOSITE

#ifndef __clang__
#if CTX_COMPOSITE_O3
#pragma GCC pop_options
#endif
#if CTX_COMPOSITE_O2
#pragma GCC pop_options
#endif
#endif

#endif // CTX_IMPLEMENTATION

