#include "ctx-split.h"

#if CTX_COMPOSITE

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

CTX_INLINE static void
ctx_RGBA8_associate_alpha_probably_opaque (uint8_t *u8)
{
  uint32_t val = *((uint32_t*)(u8));
  uint32_t a = val>>24;//u8[3];
  //if (CTX_UNLIKELY(a==0))
  //   *((uint32_t*)(u8)) = 0;
  if (CTX_UNLIKELY(a!=255))
  {
     uint32_t g = (((val & CTX_RGBA8_G_MASK) * a) >> 8) & CTX_RGBA8_G_MASK;
     uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * a) >> 8) & CTX_RGBA8_RB_MASK;
     *((uint32_t*)(u8)) = g|rb|(a << CTX_RGBA8_A_SHIFT);
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
static uint8_t ctx_gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];
extern int ctx_gradient_cache_valid;


inline static int ctx_grad_index (float v)
{
  int ret = v * (CTX_GRADIENT_CACHE_ELEMENTS - 1) + 0.5f;
  ret = ctx_maxi (0, ret);
  ret = ctx_mini (CTX_GRADIENT_CACHE_ELEMENTS-1, ret);
  return ret;
}

inline static int ctx_grad_index_i (int v)
{
  v = v >> 8;
  return ctx_maxi (0, ctx_mini (CTX_GRADIENT_CACHE_ELEMENTS-1, v));
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
#if 1
      ((uint32_t*)rgba)[0] = ctx_lerp_RGBA8 (((uint32_t*)stop_rgba)[0],
                                             ((uint32_t*)next_rgba)[0], dx);
#else
      for (int c = 0; c < 4; c++)
        { rgba[c] = ctx_lerp_u8 (stop_rgba[c], next_rgba[c], dx); }
#endif
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
  *((uint32_t*)rgba) = *((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(x)][0]));
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
  if (ctx_gradient_cache_valid)
    return;
  for (int u = 0; u < CTX_GRADIENT_CACHE_ELEMENTS; u++)
  {
    float v = u / (CTX_GRADIENT_CACHE_ELEMENTS - 1.0f);
    _ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0f, &ctx_gradient_cache_u8[u][0]);
    //*((uint32_t*)(&ctx_gradient_cache_u8_a[u][0]))= *((uint32_t*)(&ctx_gradient_cache_u8[u][0]));
    //memcpy(&ctx_gradient_cache_u8_a[u][0], &ctx_gradient_cache_u8[u][0], 4);
    //ctx_RGBA8_associate_alpha (&ctx_gradient_cache_u8_a[u][0]);
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
ctx_fragment_image_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;

  for (int i = 0; i < count; i ++)
  {

  int u = x;
  int v = y;
  int width = buffer->width;
  int height = buffer->height;
  if ( u < 0 || v < 0 ||
       u >= width ||
       v >= height)
    {
      *((uint32_t*)(rgba)) = 0;
    }
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
      float dx = (x-(int)(x)) * 255.9;
      float dy = (y-(int)(y)) * 255.9;

      switch (bpp)
      {
      case 1:
        rgba[0] = rgba[1] = rgba[2] = ctx_lerp_u8 (ctx_lerp_u8 (src00[0], src01[0], dx),
                               ctx_lerp_u8 (src10[0], src11[0], dx), dy);
        rgba[3] = global_alpha_u8;
        break;
      case 2:
        rgba[0] = rgba[1] = rgba[2] = ctx_lerp_u8 (ctx_lerp_u8 (src00[0], src01[0], dx),
                               ctx_lerp_u8 (src10[0], src11[0], dx), dy);
        rgba[3] = ctx_lerp_u8 (ctx_lerp_u8 (src00[1], src01[1], dx),
                               ctx_lerp_u8 (src10[1], src11[1], dx), dy);
        rgba[3] = (rgba[3] * global_alpha_u8) / 255;
        break;
      case 3:
      for (int c = 0; c < bpp; c++)
        { rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                                 ctx_lerp_u8 (src10[c], src11[c], dx), dy);
                
        }
        rgba[3]=global_alpha_u8;
        break;
      break;
      case 4:
      for (int c = 0; c < bpp; c++)
        { rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                                 ctx_lerp_u8 (src10[c], src11[c], dx), dy);
                
        }
        rgba[3] = (rgba[3] * global_alpha_u8) / 255;
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
            for (int c = 0; c < 4; c++)
              { rgba[c] = src[c]; }
            rgba[3] = (rgba[3] * global_alpha_u8) / 255;
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
    ctx_RGBA8_associate_alpha_probably_opaque (rgba); // XXX: really?
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
                                   float x,
                                   float y,
                                   void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  int width = buffer->width;
  int height = buffer->height;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;

  for (int i = 0; i < count; i++)
  {

  int u = x;
  int v = y;
  if ( u < 0 || v < 0 ||
       u >= width ||
       v >= height)
    {
      *((uint32_t*)(rgba))= 0;
    }
  else
    {
      int bpp = 3;
      rgba[3]=global_alpha_u8;
      float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
          int dim = (1.0 / factor) / 2;
          uint64_t sum[4]={0,0,0,0};
          int count = 0;
          for (int ou = - dim; ou < dim; ou++)
          for (int ov = - dim; ov < dim; ov++)
          {
            uint8_t *src = (uint8_t *) buffer->data;

            if (v+ov >= 0 && u+ou >=0 && u + ou < width && v + ov < height)
            {
              int o = (v+ov) * width + (u + ou);
              src += o * bpp;

              for (int c = 0; c < bpp; c++)
                sum[c] += src[c];
              count ++;
            }
          }
          if (count)
          {
            int recip = 65536/count;
            for (int c = 0; c < bpp; c++)
              rgba[c] = sum[c] * recip >> 16;
          }
          ctx_RGBA8_associate_alpha_probably_opaque (rgba);
    }
    rgba += 4;
    x += dx;
    y += dy;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_rgb8_RGBA8_box_swap_red_green (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgb8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
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
      rgba[3] = (rgba[3] * global_alpha_u8) / 255;
      ctx_RGBA8_associate_alpha_probably_opaque (rgba);
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
ctx_fragment_image_rgb8_RGBA8_bi (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;

  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  int width = buffer->width;
  int height = buffer->height;

  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  for (int i = 0; i < count; i++)
  {

  int u = x;
  int v = y;
  if ( u < 0 || v < 0 ||
       u >= width ||
       v >= height)
    {
      *((uint32_t*)(rgba))= 0;
    }
  else
    {
      int bpp = 3;
      uint8_t *src00 = (uint8_t *) buffer->data;
      int stride = buffer->stride;
      src00 += v * stride + u * bpp;
      uint8_t *src01 = src00;
      if ( u + 1 < width)
      {
        src01 = src00 + bpp;
      }
      uint8_t *src11 = src01;
      uint8_t *src10 = src00;
      if ( v + 1 < height)
      {
        src10 = src00 + stride;
        src11 = src01 + stride;
      }
      float dx = (x-(int)(x)) * 255.9f;
      float dy = (y-(int)(y)) * 255.9f;
      for (int c = 0; c < bpp; c++)
      {
        rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                               ctx_lerp_u8 (src10[c], src11[c], dx), dy);
      }
      rgba[3] = global_alpha_u8;
      ctx_RGBA8_associate_alpha_probably_opaque (rgba);
    }
    x += dx;
    y += dy;
    rgba += 4;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgb8_RGBA8_bi (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static CTX_INLINE void
ctx_fragment_image_rgb8_RGBA8_nearest (CtxRasterizer *rasterizer,
                                       float x,
                                       float y,
                                       void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer;
  if (buffer->color_managed)
   buffer = buffer->color_managed;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  uint8_t *rgba = (uint8_t *) out;
  uint8_t *src = (uint8_t *) buffer->data;
  int bwidth = buffer->width;
  int bheight = buffer->height;
  int stride = buffer->stride;

  x += 0.5f;
  y += 0.5f;

  if (CTX_UNLIKELY (dy == 0.0f && dx > 0.999f && dx < 1.001f))
  {
    int v = y;
    int u = x;
  
    if (v < buffer->height && v > 0)
    {
      int o = v * stride + u * 3;
      int i;
      for (i = 0; i < count && u < bwidth && u <0; i++)
      {
        *((uint32_t*)(rgba))= 0;
        rgba += 4;
        o += 3;
        u+=1;
      }

      for (; i < count && u < bwidth; i++)
      {
        rgba[0] = src[o];
        rgba[1] = src[o+1];
        rgba[2] = src[o+2]; 
        rgba[3]=global_alpha_u8;
        rgba += 4;
        o += 3;
        u+=1;
      }
      for (; i < count; i++)
      {
        *((uint32_t*)(rgba))= 0;
        rgba += 4;
      }
    }
    else
    {
      for (int i = 0; i < count; i++)
      {
        *((uint32_t*)(rgba))= 0;
        rgba+=4;
      }
    }
  }
  else
  {
    int u = x;
    int v = y;
    int i;
    for (i = 0; i < count && u < bwidth && u <0; i++)
    {
      u = x;
      v = y;;
      *((uint32_t*)(rgba))= 0;
      x += dx;
      y += dy;
      rgba += 4;
    }
    for (; i < count && u < bwidth; i++)
    {
      u = x;
      v = y;
    if (CTX_UNLIKELY(v < 0 || v >= bheight))
      {
        *((uint32_t*)(rgba))= 0;
      }
    else
      {
        int o = v * stride + u * 3;
        rgba[0] = src[o];
        rgba[1] = src[o+1];
        rgba[2] = src[o+2]; 
        rgba[3] = global_alpha_u8;
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
#if CTX_DITHER
  //ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
  //                    rasterizer->format->dither_green);
#endif
}


static CTX_INLINE void
ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (CtxRasterizer *rasterizer,
                                                      float x,
                                                      float y,
                                                      void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgb8_RGBA8 (CtxRasterizer *rasterizer,
                               float x,
                               float y,
                               void *out, int count, float dx, float dy)
{
  if (rasterizer->state->gstate.image_smoothing)
  {
    float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
    if (factor <= 0.50f)
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgb8_RGBA8_box_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgb8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
    }
#if CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
    else if (factor > 0.99f && factor < 1.01f)
    {
      // XXX missing translate test
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
    }
#endif
    else
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgb8_RGBA8_bi (rasterizer, x, y, out, count, dx, dy);
    }
  }
  else
  {
    if (rasterizer->swap_red_green)
      ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
    else
      ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  }
#if CTX_DITHER
  {
  uint8_t *rgba = (uint8_t*)out;
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
  }
#endif
}


/************** rgba8 */

static void
ctx_fragment_image_rgba8_RGBA8_box (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;

  for (int i = 0; i < count; i ++)
  {

  int u = x;
  int v = y;
  if ( u < 0 || v < 0 ||
       u >= buffer->width ||
       v >= buffer->height)
    {
      *((uint32_t*)(rgba))= 0;
    }
  else
    {
      int bpp = 4;
      float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
          int dim = (1.0 / factor) / 2;
          uint64_t sum[4]={0,0,0,0};
          int count = 0;
          int width = buffer->width;
          int height = buffer->height;
          for (int ou = - dim; ou < dim; ou++)
          for (int ov = - dim; ov < dim; ov++)
          {

            if (v+ov >= 0 && u+ou >=0 && u + ou < width && v + ov < height)
            {
              int o = (v+ov) * width + (u + ou);
              uint8_t *src = (uint8_t *) buffer->data + o * bpp;

              for (int c = 0; c < bpp; c++)
                sum[c] += src[c];
              count ++;
            }
          }
          if (count)
          {
            int recip = 65536/count;
            for (int c = 0; c < bpp; c++)
              rgba[c] = sum[c]*recip>>16;
          }
          rgba[3] = (rgba[3] * global_alpha_u8)/255;
          ctx_RGBA8_associate_alpha_probably_opaque (rgba);
    }
    rgba += 4;
    x += dx;
    y += dy;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}


static void
ctx_fragment_image_rgba8_RGBA8_nearest (CtxRasterizer *rasterizer,
                                        float x,
                                        float y,
                                        void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer;
  if (buffer->color_managed)
    buffer = buffer->color_managed;
  int ideltax = dx * 65536;
  int ideltay = dy * 65536;
  uint32_t *src = (uint32_t *) buffer->data;
  uint32_t *dst = (uint32_t*)out;
  int bwidth  = buffer->width;
  int bheight = buffer->height;
  x += 0.5f;
  y += 0.5f;

#if 1
  if (CTX_UNLIKELY(ideltay == 0 && ideltax == 65536))
  {
    int i = 0;
    int u = x;
    int v = y;
    if (!(v >= 0 && v < bheight))
    {
      for (i = 0 ; i < count; i++)
        *dst++ = 0;
      return;
    }
    src += bwidth * v + u;
    while (count && !(u >= 0))
    {
      *dst++ = 0;
      src ++;
      u++;
      count--;
    }
    int limit = ctx_mini (count, bwidth - u);
    if (limit>0)
    {
      memcpy (dst, src, limit * 4);
      dst += limit;
      i = limit;
    }
    for (;i < count; i++)
      *dst++ = 0;
    return;
  }
#endif

  {
    int i = 0;
    float u1 = x + dx * (count-1);
    float v1 = y + dy * (count-1);
    uint32_t *edst = ((uint32_t*)out)+count;
    for (; i < count; )
    {
      if ((u1 < 0.0f || v1 < 0.0f || u1 >= bwidth || v1 >= bheight))
      {
        *edst-- = 0;
        count --;
        u1 -= dx;
        v1 -= dy;
      }
      else break;
    }


    for (i = 0; i < count; i ++)
    {
      if ((x < 0.0f || y < 0.0f || x >= bwidth || y >= bheight))
      {
        *dst = 0;
        dst++;
        x += dx;
        y += dy;
      }
      else break;
    }


    uint32_t ix = x * 65536;
    uint32_t iy = y * 65536;

    for (; i < count; i ++)
    {
      *dst = src[(iy>>16) * bwidth + (ix>>16)];
      ix += ideltax;
      iy += ideltay;
      dst++;
    }
  }
  ctx_RGBA8_apply_global_alpha_and_associate (rasterizer, (uint8_t*)out, count);
}

static void
ctx_fragment_image_rgba8_RGBA8_bi (CtxRasterizer *rasterizer,
                                   float x,
                                   float y,
                                   void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  float ox = (x-(int)(x));
  float oy = (y-(int)(y));

  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  const int bwidth = buffer->width;
  const int bheight = buffer->height;
  int i = 0;

  if (dy == 0.0f && dx > 0.0f)
  {
    if (!(y >= 0 && y < bheight))
    {
      uint32_t *dst = (uint32_t*)rgba;
      for (i = 0 ; i < count; i++)
        *dst++ = 0;
      return;
    }

    if ((dx > 0.99f && dx < 1.01f && 
         ox < 0.01 && oy < 0.01))
    {
      /* TODO: this could have been rigged up in composite_setup */
      ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer,
                                   x, y, out, count, dx, dy);
      return;
    }
    x+=1; // XXX off by one somewhere? ,, needed for alignment with nearest

    uint32_t *data = ((uint32_t*)buffer->data);
    uint32_t yi = y * 65536;
    uint32_t xi = x * 65536;
    int xi_delta = dx * 65536;

    for (i= 0; i < count; i ++)
    {
      int u = xi >> 16;
      if ( u  < 0 || u >= bwidth-1)
      {
        *((uint32_t*)(rgba))= 0;
        xi += xi_delta;
        rgba += 4;
      }
      else
        break;
    }

  int loaded = -4;
  uint32_t s0_ga = 0, s0_rb = 0, s1_ga = 0, s1_rb = 0;
 
  int v = yi >> 16;
  data += bwidth * v;
  int dv = (yi >> 8) & 0xff;

  int u = xi >> 16;

  uint32_t *ndata = data;
  if (v < bheight-1) ndata += bwidth;

  uint32_t *src0 = data, *src1 = ndata;


  if (xi_delta == 65536 && u < bwidth -1)
  {
    int du = (xi >> 8) & 0xff;

    src0 = data + u;
    src1 = ndata + u;
    ctx_lerp_RGBA8_split (src0[0],src1[0], dv, &s1_ga, &s1_rb);

    int limit = bwidth-u;
    limit = ctx_mini(count,limit);

    for (; i < limit; i ++)
    {
      s0_ga = s1_ga;
      s0_rb = s1_rb;
      ctx_lerp_RGBA8_split (src0[1],src1[1], dv, &s1_ga, &s1_rb);
      ((uint32_t*)(&rgba[0]))[0] = 
      ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, du);
      rgba += 4;
      u++;
      src0 ++;
      src1 ++;
    }
  }
  else
  {

  for (; i < count; i ++)
  {
    if (CTX_UNLIKELY(u >= bwidth-1))
    {
      break;
    }
    else if (CTX_LIKELY(loaded + 1 == u))
    {
      s0_ga = s1_ga;
      s0_rb = s1_rb;
      ctx_lerp_RGBA8_split (src0[1],src1[1], dv, &s1_ga, &s1_rb);
      src0 ++;
      src1 ++;
    }
    else if (loaded != u)
    {
      src0 = data + u;
      src1 = ndata + u;
      ctx_lerp_RGBA8_split (src0[0],src1[0], dv, &s0_ga, &s0_rb);
      ctx_lerp_RGBA8_split (src0[1],src1[1], dv, &s1_ga, &s1_rb);
    }
    loaded = u;
    ((uint32_t*)(&rgba[0]))[0] = 
      ctx_lerp_RGBA8_merge (s0_ga, s0_rb, s1_ga, s1_rb, ((xi>>8)&0xff));
    xi += xi_delta;
    rgba += 4;
    u = xi >> 16;
  }
  }

  }
  else // //
  {
    uint32_t *data = ((uint32_t*)buffer->data);
    for (i= 0; i < count; i ++)
    {
      int u = x;
      int v = y;
      int ut = x + 1.5;
      int vt = y + 1.5;
      if ( ut  <= 0 || vt  <= 0 || u >= buffer->width || v >= buffer->height)
      {
        *((uint32_t*)(rgba))= 0;
      }
      else
        break;
      x += dx;
      y += dy;
      rgba += 4;
    }

  uint32_t yi = y * 65536;
  uint32_t xi = x * 65536;

  int yi_delta = dy * 65536;
  int xi_delta = dx * 65536;

  int loaded = -4;
  uint32_t *src00=data;
  uint32_t *src01=data;
  uint32_t *src10=data;
  uint32_t *src11=data;

  int u = xi >> 16;
  int v = yi >> 16;
  int offset = bwidth * v + u;

  for (; i < count; i ++)
  {
  if (CTX_UNLIKELY(
       u >= buffer->width ||
       v  <= -65536 ||
       u  <= -65536 ||
       v >= buffer->height))
    {
      break;
    }
#if 1
  else if (CTX_UNLIKELY(u < 0 || v < 0)) // default to next sample down and to right
  {
      int got_prev_pix = (u >= 0);
      int got_prev_row = (v>=0);
      src11 = data  + offset + bwidth + 1;
      src10 = src11 - got_prev_pix;
      src01 = src11 - bwidth * got_prev_row;
      src00 = src10 - bwidth * got_prev_row;
  }
#endif
#if 1
  else if (loaded + 1 == offset)
  {
      src00++;
      src01++;
      src10++;
      src11++;
  }
#endif
  else if (loaded != offset)
  {
      int next_row = ( v + 1 < bheight) * bwidth;
      int next_pix = (u + 1 < bwidth);
      src00 = data  + offset;
      src01 = src00 + next_pix;
      src10 = src00 + next_row;
      src11 = src01 + next_row;
  }
    loaded = offset;
    ((uint32_t*)(&rgba[0]))[0] = ctx_bi_RGBA8 (*src00,*src01,*src10,*src11, (xi>>8),(yi>>8)); // the argument type does the & 0xff
    xi += xi_delta;
    yi += yi_delta;
    rgba += 4;

    u = xi >> 16;
    v = yi >> 16;
    offset = bwidth * v + u;
  }
  }

  for (; i < count; i ++)
  {
    *((uint32_t*)(rgba))= 0;
    rgba += 4;
  }

  ctx_RGBA8_apply_global_alpha_and_associate (rasterizer, (uint8_t*)out, count);
}
#endif

#define ctx_clampi(val,min,max) \
     ctx_mini (ctx_maxi ((val), (min)), (max))

static inline uint32_t ctx_yuv_to_rgba32 (uint8_t y, uint8_t u, uint8_t v)
{
  int cy  = ((y - 16) * 76309) >> 16;
  int cr  = (v - 128);
  int cb  = (u - 128);
  int red = cy + ((cr * 104597) >> 16);
  int green = cy - ((cb * 25674 + cr * 53278) >> 16);
  int blue = cy + ((cb * 132201) >> 16);
  return  ctx_clampi (red, 0, 255) |
          (ctx_clampi (green, 0, 255) << 8) |
          (ctx_clampi (blue, 0, 255) << 16) |
          (0xff << 24);
}

static void
ctx_fragment_image_yuv420_RGBA8_nearest (CtxRasterizer *rasterizer,
                                         float x,
                                         float y,
                                         void *out, int count, float dx, float dy)
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

  {
    int i = 0;

    for (; i < count; i ++)
    {
      int u = x;
      int v = y;
      if ((u < 0 || v < 0 || u >= bwidth || v >= bheight))
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
    int iy = y * 65536;

    int ideltax = dx * 65536;
    int ideltay = dy * 65536;

    for (; i < count; i ++)
    {
      int u = ix >> 16;
      int v = iy >> 16;
      if (CTX_LIKELY(u >= 0 && v >= 0 && u < bwidth && v < bheight))
      {
        uint32_t y  = v * bwidth + u;
        uint32_t uv = (v / 2) * bwidth_div_2 + (u / 2);

        *((uint32_t*)(rgba))= ctx_yuv_to_rgba32 (src[y],
                        src[u_offset+uv], src[v_offset+uv]);
        //ctx_RGBA8_associate_alpha_probably_opaque (rgba);
#if CTX_DITHER
       ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                           rasterizer->format->dither_green);
#endif
      }
      else
      {
        break;
      }
      ix += ideltax;
      iy += ideltay;
      rgba += 4;
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

static void
ctx_fragment_image_rgba8_RGBA8_box_swap_red_green (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgba8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgba8_RGBA8_bi_swap_red_green (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgba8_RGBA8_bi (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgba8_RGBA8 (CtxRasterizer *rasterizer,
                                float x,
                                float y,
                                void *out, int count, float dx, float dy)
{
  if (rasterizer->state->gstate.image_smoothing)
  {
    float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
    if (factor <= 0.50f)
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_box_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgba8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
    }
#if CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
    else if (factor > 0.99f && factor < 1.01f)
    {
      // XXX: also verify translate == 0 for this fast path to be valid
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
    }
#endif
    else
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_bi_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgba8_RGBA8_bi (rasterizer, x, y, out, count, dx, dy);
    }
  }
  else
  {
    if (rasterizer->swap_red_green)
      ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
    else
      ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  }
  //ctx_fragment_swap_red_green_u8 (out, count);
#if CTX_DITHER
  uint8_t *rgba = (uint8_t*)out;
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}
#endif

static void
ctx_fragment_image_gray1_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);
  for (int i = 0; i < count; i ++)
  {
  int u = x;
  int v = y;
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
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i <  count; i ++)
  {
    float v = (ctx_hypotf_fast (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
#if CTX_GRADIENT_CACHE
    uint32_t *rgbap = (uint32_t*)&ctx_gradient_cache_u8[ctx_grad_index(v)][0];
    *((uint32_t*)rgba) = *rgbap;
#else
    ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, rgba);
#endif
#if CTX_DITHER
    ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                        rasterizer->format->dither_green);
#endif
    rgba += 4;
    x += dx;
    y += dy;
  }
}

static void
ctx_fragment_linear_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
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
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
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
#endif

  u0 *= linear_gradient_dx;
  v0 *= linear_gradient_dy;
  ud *= linear_gradient_dx;
  vd *= linear_gradient_dy;

#if CTX_GRADIENT_CACHE
  int vv = ((u0 + v0) - linear_gradient_start) * (CTX_GRADIENT_CACHE_ELEMENTS-1) * 256;
  int ud_plus_vd = (ud + vd) * (CTX_GRADIENT_CACHE_ELEMENTS-1) * 256;
#else
  float vv = ((u0 + v0) - linear_gradient_start);
  float ud_plus_vd = (ud + vd);
#endif

  for (int x = 0; x < count ; x++)
  {
#if CTX_GRADIENT_CACHE
  uint32_t*rgbap = ((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index_i (vv)][0]));
  *((uint32_t*)rgba) = *rgbap;
#else
  _ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, rgba);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (rgba, u0, v0, dither_red_blue, dither_green);
#endif
    rgba+= 4;
    vv += ud_plus_vd;
  }
#endif
}

#endif

static void
ctx_fragment_color_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba_out = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  _ctx_color_get_rgba8 (rasterizer->state, &g->color, rgba_out);
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
ctx_fragment_linear_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
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
ctx_fragment_radial_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
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
ctx_fragment_color_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *rgba = (float *) out;
  for (int i = 0; i < count; i++)
  {
    CtxSource *g = &rasterizer->state->gstate.source_fill;
    ctx_color_get_rgba (rasterizer->state, &g->color, rgba);
    for (int c = 0; c < 3; c++)
      rgba[c] *= rgba[3];
    rgba += 4;
  }
}

static void ctx_fragment_image_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *outf = (float *) out;
  uint8_t rgba[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (buffer->format->bpp)
    {
#if CTX_FRAGMENT_SPECIALIZE
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
#endif
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);       break;
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

static CtxFragment ctx_rasterizer_get_fragment_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:
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
            case 1:  return ctx_fragment_image_gray1_RGBA8;
#if 0
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
                  else if (factor > 0.99f && factor < 1.01f)
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
                  else if (factor > 0.99f && factor < 1.01f)
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green;
                    return ctx_fragment_image_rgba8_RGBA8_nearest;
                  }
#endif

                  else
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgba8_RGBA8_bi_swap_red_green;
                    return ctx_fragment_image_rgba8_RGBA8_bi;
                  }
                }
                else
                {
                  if (rasterizer->swap_red_green)
                    return ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green;
                  return ctx_fragment_image_rgba8_RGBA8_nearest;
                }
              }
            default: return ctx_fragment_image_RGBA8;
          }
#else
          return ctx_fragment_image_RGBA8;
#endif

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
  *v0 = rasterizer->scanline / 15;//rasterizer->aa;
  float u1 = *u0 + count;
  float v1 = *v0;

  _ctx_matrix_apply_transform (&gstate->source_fill.transform, u0, v0);
  _ctx_matrix_apply_transform (&gstate->source_fill.transform, &u1, &v1);

  *ud = (u1-*u0) / (count);
  *vd = (v1-*v0) / (count);
}


static void
ctx_u8_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  if (CTX_UNLIKELY(rasterizer->fragment))
    {
      float u0 = 0; float v0 = 0;
      float ud = 0; float vd = 0;
      ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
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
          rasterizer->fragment (rasterizer, u0, v0, src, 1, ud, vd);
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

  while (count--)
  {
    for (int c = 0; c < components; c++)
      //dst[c] =  ((tsrc[c] * *coverage)>>8) + (dst[c] * (((65536)-(tsrc[components-1] * *coverage)))>>16);
      dst[c] =  ((((tsrc[c] * *coverage)) + (dst[c] * (((255)-(((255+(tsrc[components-1] * *coverage))>>8))))))>>8);
    coverage ++;
    dst+=components;
  }
}

static void
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

static inline void
ctx_RGBA8_source_over_normal_full_cov_buf (CTX_COMPOSITE_ARGUMENTS, uint8_t *tsrc)
{
  while (count--)
  {
     uint32_t si_ga = ((*((uint32_t*)tsrc)) & 0xff00ff00) >> 8;
     uint32_t si_rb = (*((uint32_t*)tsrc)) & 0x00ff00ff;
     uint32_t si_a  = si_ga >> 16;
     uint32_t racov = (255-si_a);
     *((uint32_t*)(dst)) =
     (((si_rb*255+0xff00ff+(((*((uint32_t*)(dst)))&0x00ff00ff)*racov))>>8)&0x00ff00ff)|
     ((si_ga*255+0xff00ff+((((*((uint32_t*)(dst)))&0xff00ff00)>>8)*racov))&0xff00ff00);
     tsrc += 4;
     dst  += 4;
  }
}

static void
ctx_RGBA8_source_copy_normal_buf (CTX_COMPOSITE_ARGUMENTS, uint8_t *tsrc)
{
  while (count--)
  {
    ((uint32_t*)dst)[0]=ctx_lerp_RGBA8 (((uint32_t*)dst)[0],
                                        ((uint32_t*)tsrc)[0], coverage[0]);
    coverage ++;
    tsrc += 4;
    dst  += 4;
  }
}

static void
ctx_RGBA8_source_over_normal_fragment (CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
  uint8_t _tsrc[4 * (count)];
  rasterizer->fragment (rasterizer, u0, v0, &_tsrc[0], count, ud, vd);
  ctx_RGBA8_source_over_normal_buf (rasterizer,
                       dst, src, x0, coverage, count, &_tsrc[0]);
}

static inline void
ctx_RGBA8_source_over_normal_full_cov_fragment (CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
  uint8_t _tsrc[4 * (count)];
  rasterizer->fragment (rasterizer, u0, v0, &_tsrc[0], count, ud, vd);
  ctx_RGBA8_source_over_normal_full_cov_buf (rasterizer,
                       dst, src, x0, coverage, count, &_tsrc[0]);
}

static void
ctx_RGBA8_source_copy_normal_fragment (CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
  uint8_t _tsrc[4 * (count)];
  rasterizer->fragment (rasterizer, u0, v0, &_tsrc[0], count, ud, vd);
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
static void \
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

  if (n < 0 && l!=n)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if (x > 255 && x!=l)
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
    fragment (rasterizer, 0, 0, src, 1, 0, 0);
    if (blend != CTX_BLEND_NORMAL)
      ctx_u8_blend (components, blend, dst, src, src, 1);
  }
  else
  {
    float u0 = 0; float v0 = 0;
    float ud = 0; float vd = 0;
    src = &tsrc[0];

    ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
    fragment (rasterizer, u0, v0, src, count, ud, vd);
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

ctx_u8_porter_duff(RGBA8, 4,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)
//ctx_u8_porter_duff(comp_name, components,color_##blend_name,  NULL, blend_mode)

static void
ctx_RGBA8_nop (CTX_COMPOSITE_ARGUMENTS)
{
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
      ctx_fragment_color_RGBA8 (rasterizer, 0,0, rasterizer->color, 1, 0,0);
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

      if (blend_mode == CTX_BLEND_NORMAL &&
           compositing_mode == CTX_COMPOSITE_COPY)
      {
        rasterizer->comp_op = ctx_RGBA8_source_copy_normal_color;
        rasterizer->comp = CTX_COV_PATH_COPY;
      }
      else if (blend_mode == CTX_BLEND_NORMAL &&
          compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
     {
       if (rasterizer->color[components-1] == 255)
       {
        rasterizer->comp_op = ctx_RGBA8_source_copy_normal_color;
        rasterizer->comp = CTX_COV_PATH_COPY;
       }
       else
       {
        rasterizer->comp_op = ctx_RGBA8_source_over_normal_color;
        rasterizer->comp = CTX_COV_PATH_OVER;
       }
     }
     else if (compositing_mode == CTX_COMPOSITE_CLEAR)
     {
       rasterizer->comp_op = ctx_RGBA8_clear_normal;
     }
  }
  else if (blend_mode == CTX_BLEND_NORMAL &&
           compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
  {
     rasterizer->comp_op = ctx_RGBA8_source_over_normal_fragment;
     rasterizer->comp = CTX_COV_PATH_OVER_FRAGMENT;
  }
  else if (blend_mode == CTX_BLEND_NORMAL &&
           compositing_mode == CTX_COMPOSITE_COPY)
  {
     rasterizer->comp_op = ctx_RGBA8_source_copy_normal_fragment;
     rasterizer->comp = CTX_COV_PATH_COPY_FRAGMENT;
  }
}

static void
ctx_setup_RGB565 (CtxRasterizer *rasterizer)
{
  ctx_setup_RGBA8 (rasterizer);
  rasterizer->comp = CTX_COV_PATH_FALLBACK;
}

static void
ctx_composite_convert (CTX_COMPOSITE_ARGUMENTS)
{
  uint8_t pixels[count * rasterizer->format->ebpp];
  rasterizer->format->to_comp (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  rasterizer->format->from_comp (rasterizer, x0, &pixels[0], dst, count);
}

#if CTX_ENABLE_FLOAT
static void
ctx_float_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;

  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);

  while (count--)
  {
    uint8_t cov = *coverage;
    float covf = ctx_u8_to_float (cov);
    for (int c = 0; c < components; c++)
      dstf[c] = dstf[c]*(1.0-covf) + srcf[c]*covf;
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
      float ralpha = 1.0 - ctx_u8_to_float (cov);
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
    for (int c = 0; c < components-1; c++)
      dstf[c] = (srcf[c]*fcov + dstf[c] * ralpha);
    coverage ++;
    dstf+= components;
  }
}

static void
ctx_float_source_copy_normal_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;

  while (count--)
  {
    uint8_t cov = *coverage;
    float fcov = ctx_u8_to_float (cov);
    float ralpha = 1.0f - fcov;
    for (int c = 0; c < components-1; c++)
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

  if (n < 0.0f && l != n)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if (x > 1.0f && x != l)
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

  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  float   global_alpha_f = rasterizer->state->gstate.global_alpha_f;
  
  {
    float tsrc[components];
    float u0 = 0; float v0 = 0;
    float ud = 0; float vd = 0;

    ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);

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

      fragment (rasterizer, u0, v0, tsrc, 1, ud, vd);
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

#if 0
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
      ctx_fragment_color_RGBAF (rasterizer, 0,0, rasterizer->color, 1, 0,0);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;
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
        }
        else if (gstate->global_alpha_u8 == 0)
        {
          rasterizer->comp_op = ctx_RGBA8_nop;
        }
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            //if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            //{
            //  rasterizer->comp_op = ctx_RGBAF_source_over_normal_color;
           // }
           // else
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
            //rasterizer->fragment = NULL;
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
}

#endif
#if CTX_ENABLE_GRAYAF

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float rgba[4];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0 ; i < count; i++)
  {
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0, rgba);
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgba);
  ((float*)out)[1] = rgba[3];
     out = ((float*)(out)) + 2;
     x += dx;
     y += dy;
  }
}

static void
ctx_fragment_radial_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
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
ctx_fragment_color_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
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

static void ctx_fragment_image_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t rgba[4];
  float rgbaf[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (buffer->format->bpp)
    {
#if CTX_FRAGMENT_SPECIALIZE
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
#endif
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);       break;
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
  //  rasterizer->fragment = NULL;
      ctx_color_get_rgba (rasterizer->state, &gstate->source_fill.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;
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
//            rasterizer->fragment = NULL;
            }
            else
            {
              rasterizer->comp_op = ctx_GRAYAF_porter_duff_color_normal;
//            rasterizer->fragment = NULL;
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
//          rasterizer->fragment = NULL;
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
ctx_fragment_other_CMYKAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *cmyka = (float*)out;
  float _rgba[4 * count];
  float *rgba = &_rgba[0];
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:
        ctx_fragment_image_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
        break;
      case CTX_SOURCE_COLOR:
        ctx_fragment_color_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
        break;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT:
        ctx_fragment_linear_gradient_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
        break;
      case CTX_SOURCE_RADIAL_GRADIENT:
        ctx_fragment_radial_gradient_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
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
ctx_fragment_color_CMYKAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
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
 //     rasterizer->fragment = NULL;
      ctx_color_get_cmyka (rasterizer->state, &gstate->source_fill.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        ((float*)rasterizer->color)[components-1] *= gstate->global_alpha_f;
    }
  else
  {
    rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
  }

#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_CMYKAF_clear_normal;
#if 1
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
#if 1
              else //if (((float*)rasterizer->color)[components-1] == 1.0f)
                rasterizer->comp_op = ctx_CMYKAF_source_copy_normal_color;
   //           else
   //             rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
#endif
            }
            else
            {
              rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
   //         rasterizer->fragment = NULL;
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
    //      rasterizer->fragment = NULL;
            break;
          default:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
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
      rgba[0] = 255 * (*pixel & (1<< (x&7) ) );
      rgba[1] = 255;
      pixel+= ( (x&7) ==7);
      x++;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  *pixel = 0;
  while (count--)
    {
      int gray = rgba[0];
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/aa, 0, 127);
      if (gray >= 127)
        {
          *pixel = *pixel | ((1<< (x&7) ) * (gray >= 127));
        }
#if 0
      else
      {
          *pixel = *pixel & (~ (1<< (x&7) ) );
      }
#endif
      if ( (x&7) ==7)
        { pixel+=1;
          if(count>0)*pixel = 0;
        }
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
      *((uint32_t*)(rgba))=0xff000000 + 0x00ffffff * ((*pixel & (1<< (x&7) ) )!=0);
      pixel+= ( (x&7) ==7);
      x++;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  *pixel = 0;
  while (count--)
    {
      int gray = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/aa, 0, 127);
      if (gray <= 127)
        {
          //*pixel = *pixel & (~ (1<< (x&7) ) );
        }
      else
        {
          *pixel = *pixel | (1<< (x&7) );
        }
      if ( (x&7) ==7)
        { pixel+=1;
          if(count>0)*pixel = 0;
        }
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
ctx_fragment_linear_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
        uint8_t *dst = (uint8_t*)out;
  for (int i = 0; i < count;i ++)
  {
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  {
    uint8_t rgba[4];
    ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 1.0, rgba);
    ctx_rgba_to_graya_u8 (rasterizer->state, rgba, dst);
   
  }


#if CTX_DITHER
  ctx_dither_graya_u8 ((uint8_t*)dst, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
  dst += 2;
  x += dx;
  y += dy;
  }
}

#if 0
static void
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
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
ctx_fragment_radial_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
        uint8_t *dst = (uint8_t*)out;
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
  ctx_dither_graya_u8 ((uint8_t*)dst, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
  dst += 2;
  x += dx;
  y += dy;
  }
}
#endif

static void
ctx_fragment_color_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  uint16_t *dst = (uint16_t*)out;
  uint16_t pix;
  ctx_color_get_graya_u8 (rasterizer->state, &g->color, (void*)&pix);
  for (int i = 0; i <count; i++)
  {
    dst[i]=pix;
  }
}

static void ctx_fragment_image_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t rgba[4*count];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (buffer->format->bpp)
    {
#if CTX_FRAGMENT_SPECIALIZE
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
#endif
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);       break;
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

//ctx_u8_porter_duff(GRAYA8, 2,color,   rasterizer->fragment, rasterizer->state->gstate.blend_mode)
ctx_u8_porter_duff(GRAYA8, 2,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
//ctx_u8_porter_duff(GRAYA8, 2,color_normal,   rasterizer->fragment, CTX_BLEND_NORMAL)
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
      ctx_fragment_color_GRAYA8 (rasterizer, 0,0, rasterizer->color, 1, 0,0);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          rasterizer->color[c] = (rasterizer->color[c] * gstate->global_alpha_u8)/255;
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
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (rasterizer->color[components-1] == 0)
                rasterizer->comp_op = ctx_RGBA8_nop;
              else if (rasterizer->color[components-1] == 255)
                rasterizer->comp_op = ctx_GRAYA8_source_copy_normal_color;
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
#if CTX_RGB332_ALPHA
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
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

#endif
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
  *blue   =  (byteswapped & 31) <<3;
  *green = ( (byteswapped>>5) & 63) <<2;
  *red   = ( (byteswapped>>11) & 31) <<3;
#if 0
  if (*blue > 248) { *blue = 255; }
  if (*green > 248) { *green = 255; }
  if (*red > 248) { *red = 255; }
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
  uint8_t blue   = (byteswapped & 31) <<3;
  uint8_t green = ( (byteswapped>>5) & 63) <<2;
  uint8_t red   = ( (byteswapped>>11) & 31) <<3;
#if 0
  if (*blue > 248) { *blue = 255; }
  if (*green > 248) { *green = 255; }
  if (*red > 248) { *red = 255; }
#endif
  return red +  (green << 8) + (blue << 16) + (0xff << 24);
}

static inline uint16_t
ctx_565_pack (const uint8_t  red,
              const uint8_t  green,
              const uint8_t  blue,
              const int      byteswap)
{
  uint32_t c = (red >> 3) << 11;
  c |= (green >> 2) << 5;
  c |= blue >> 3;
  if (byteswap)
    { return (c>>8) | (c<<8); } /* swap bytes */
  return c;
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
      ((uint32_t*)(rgba))[0] = ctx_565_unpack_32 (*pixel, 0);
#if CTX_RGB565_ALPHA
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
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
#if 0
  if (CTX_LIKELY(rasterizer->comp_op == ctx_RGBA8_source_over_normal_color))
  {
     int byteswap = 0;
     uint32_t si    = *((uint32_t*)(src));
     uint16_t si_16 = ctx_888_to_565 (si, byteswap);
     uint32_t sval  = (si_16 & ( (31 << 11 ) | 31));
     uint32_t sg = (si_16 & (63 << 5)) >> 5;
     uint32_t si_a = si >> (24 + 3);
     while (count--)
     {
        uint32_t di_16 = *((uint16_t*)(dst));
        uint32_t cov = (*coverage) >> 3;
        uint32_t racov = (32-((31+si_a*cov)>>5));
        uint32_t dval = (di_16 & ( (31 << 11 ) | 31));
        uint32_t dg = (di_16 >> 5) & 63; // faster outside than
                                         // remerged as part of dval
        *((uint16_t*)(dst)) =
                ((               
                  (((sval * cov) + (dval * racov)) >> 5)
                 ) & ((31 << 11 )|31)) |
                  ((((sg * cov) + (dg * racov)) & 63) );

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

static inline void
ctx_RGB565_BS_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      //ctx_565_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2], 1);
      ((uint32_t*)(rgba))[0] = ctx_565_unpack_32 (*pixel, 1);
#if CTX_RGB565_ALPHA
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
        { rgba[3] = 0; }
      else
        { rgba[3] = 255; }
#endif
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

static void
ctx_composite_RGB565_BS (CTX_COMPOSITE_ARGUMENTS)
{
  uint8_t pixels[count * 4];
  ctx_RGB565_BS_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_RGBA8_to_RGB565_BS (rasterizer, x0, &pixels[0], dst, count);
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
    ctx_composite_RGB565, ctx_setup_RGB565,
  },
#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED
  {
    CTX_FORMAT_RGB565_BYTESWAPPED, 3, 16, 4, 32, 64, CTX_FORMAT_RGBA8,
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
    NULL, NULL, ctx_composite_CMYKA8, ctx_setup_CMYKAF,
  },
#endif
#if CTX_ENABLE_CMYK8
  {
    CTX_FORMAT_CMYK8, 5, 32, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, ctx_composite_CMYK8, ctx_setup_CMYKAF,
  },
#endif
#if CTX_ENABLE_YUV420
  {
    CTX_FORMAT_YUV420, 1, 8, 4, 0, 0, CTX_FORMAT_RGBA8,
    NULL, NULL, ctx_composite_convert, ctx_setup_RGBA8,
  },
#endif
  {
    CTX_FORMAT_NONE
  }
};


CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format)
{
  for (unsigned int i = 0; ctx_pixel_formats[i].pixel_format; i++)
    {
      if (ctx_pixel_formats[i].pixel_format == format)
        {
          return &ctx_pixel_formats[i];
        }
    }
  return NULL;
}

#endif
