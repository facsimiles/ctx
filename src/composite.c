#include "ctx-split.h"

#if CTX_COMPOSITE


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


inline static int ctx_grad_index (float v)
{
  int ret = v * (CTX_GRADIENT_CACHE_ELEMENTS - 1.0f) + 0.5f;
  if (ret >= CTX_GRADIENT_CACHE_ELEMENTS)
    return CTX_GRADIENT_CACHE_ELEMENTS - 1;
  if (ret >= 0 && ret < CTX_GRADIENT_CACHE_ELEMENTS)
    return ret;
  return 0;
}

extern uint8_t ctx_gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];
extern uint8_t ctx_gradient_cache_u8_a[CTX_GRADIENT_CACHE_ELEMENTS][4];
extern int ctx_gradient_cache_valid;

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
      for (int c = 0; c < 4; c++)
        { rgba[c] = ctx_lerp_u8 (stop_rgba[c], next_rgba[c], dx); }
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
}

#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer);
#endif

CTX_INLINE static void
ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
#if CTX_GRADIENT_CACHE
  uint32_t* rgbap = ((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(x)][0]));
  *((uint32_t*)rgba) = *rgbap;
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
    //*((uint32_t*)(&ctx_gradient_cache_u8_a[u][0]))= *((uint32_t*)(&ctx_gradient_cache_u8[u][0]));
    memcpy(&ctx_gradient_cache_u8_a[u][0], &ctx_gradient_cache_u8[u][0], 4);
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
      if (rasterizer->swap_red_green)
      {
        uint8_t tmp = rgba[0];
        rgba[0] = rgba[2];
        rgba[2] = tmp;
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

  if (rasterizer->swap_red_green)
  {
    uint8_t tmp = rgba[0];
    rgba[0] = rgba[2];
    rgba[2] = tmp;
  }
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
  if (rasterizer->swap_red_green)
  {
    uint8_t tmp = rgba[0];
    rgba[0] = rgba[2];
    rgba[2] = tmp;
  }
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
  uint32_t *rgbap = (uint32_t*)&ctx_gradient_cache_u8[ctx_grad_index(v)][0];
  *((uint32_t*)rgba) = *rgbap;
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
  uint32_t*rgbap = ((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(v)][0]));
  *((uint32_t*)rgba) = *rgbap;
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
  if (rasterizer->swap_red_green)
  {
    int tmp = rgba[0];
    rgba[0] = rgba[2];
    rgba[2] = tmp;
  }
}
#if CTX_ENABLE_FLOAT

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
#endif

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
           //__attribute__ ((fallthrough));
        switch (components)
        {
          case 1: dst[0] = 0; break;
          case 3: dst[2] = 0;
           /* FALLTHROUGH */
          case 2: *((uint16_t*)(dst)) = 0; break;
          case 5: dst[4] = 0;
           /* FALLTHROUGH */
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
      uint32_t *tsrci = (uint32_t*)tsrc;
#if CTX_GRADIENT_CACHE
      uint32_t *cachei = ((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
      *tsrci = *cachei;
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    uint32_t si = *tsrci;
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

    uint32_t *dsti = ((uint32_t*)(dst));
          uint32_t di = *dsti;
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


    uint32_t *sip = ((uint32_t*)(tsrc));
    uint32_t si = *sip;
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

          uint32_t *dip = ((uint32_t*)(dst));
          uint32_t di = *dip;
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
      uint32_t *tsrcp = (uint32_t*)tsrc;
      uint32_t *cp = ((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
      *tsrcp = *cp;
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    uint32_t *sip = ((uint32_t*)(tsrc));
    uint32_t si = *sip;
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

          uint32_t *dip = ((uint32_t*)(dst));
          uint32_t di = *dip;
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

    uint32_t *sip = ((uint32_t*)(tsrc));
    uint32_t si = *sip;
      int si_a = si >> CTX_RGBA8_A_SHIFT;

    uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
    uint32_t si_rb = si & CTX_RGBA8_RB_MASK;

          uint32_t *dip = ((uint32_t*)(dst));
          uint32_t di = *dip;
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
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_color) (CTX_COMPOSITE_ARGUMENTS)
{
#if 0
  ctx_u8_source_over_normal_color (4, rasterizer, dst, src, clip, x0, coverage, count);
  return;
#endif
  {
    uint8_t tsrc[4];
    memcpy (tsrc, src, 4);
    ctx_RGBA8_associate_alpha (tsrc);
    uint8_t a = src[3];
    int x = 0;

#if CTX_AVX2
    if ((size_t)(dst) & 31)
#endif
    {
      uint32_t *sip = ((uint32_t*)(tsrc));
      uint32_t si = *sip;
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
        uint32_t *dip = ((uint32_t*)(dst));
        uint32_t di = *dip;
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
      uint32_t *sip = ((uint32_t*)(tsrc));
      uint32_t si = *sip;
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
          uint32_t *dip = ((uint32_t*)(dst));
          uint32_t di = *dip;
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
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_copy_normal) (CTX_COMPOSITE_ARGUMENTS)
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
      /* FALLTHROUGH */
     case 2:
       *((uint16_t*)(blended)) = *((uint16_t*)(src));
       ctx_u8_associate_alpha (components, blended);
       break;
     case 5:
       ((uint8_t*)(blended))[4] = ((uint8_t*)(src))[4];
       /* FALLTHROUGH */
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
CTX_COMPOSITE_SUFFIX(ctx_##comp_format##_porter_duff_##source) (CTX_COMPOSITE_ARGUMENTS) \
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
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop) (CTX_COMPOSITE_ARGUMENTS)
{
}

static void
ctx_setup_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 4;
  rasterizer->fragment = ctx_rasterizer_get_fragment_RGBA8 (rasterizer);
  rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_generic);
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
      if (rasterizer->swap_red_green)
      {
        uint8_t *rgba = &rasterizer->color[0];
        uint8_t tmp = rgba[0];
        rgba[0] = rgba[2];
        rgba[2] = tmp;
      }

      switch (gstate->blend_mode)
      {
        case CTX_BLEND_NORMAL:
          if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
          {
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_copy_normal);
            return;
          }
          else if (gstate->global_alpha_u8 == 0)
          {
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
          }
          else if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
          {
             if (rasterizer->color[components-1] == 0)
                 rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
             else
                 rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_color);
         }
         break;
      default:
         rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_color);
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
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_linear_gradient_normal);
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_radial_gradient_normal);
            break;
#endif
          case CTX_SOURCE_IMAGE:
            {
               CtxSource *g = &rasterizer->state->gstate.source;
               switch (g->image.buffer->format->bpp)
               {
                 case 32:
                   rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_image_rgba8_normal);
                   break;
                 case 24:
                   rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_image_rgb8_normal);
                 break;
                 default:
                   rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_generic_normal);
                   break;
               }
            }
            break;
          default:
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_generic_normal);
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

#if CTX_ENABLE_FLOAT
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
#endif

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
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        }
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)(rasterizer->color))[components-1] == 0.0f)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
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
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
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
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
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
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        else
        switch (gstate->source.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (rasterizer->color[components-1] == 0)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
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

CtxPixelFormatInfo CTX_COMPOSITE_SUFFIX(ctx_pixel_formats)[]=
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
  {
    CTX_FORMAT_NONE
  }
};


void
CTX_COMPOSITE_SUFFIX(ctx_compositor_setup) (CtxRasterizer *rasterizer)
{
  if (rasterizer->format->setup)
  {
    // event if _default is used we get to work
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

#endif
