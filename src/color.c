#ifndef __CTX_COLOR
#define __CTX_COLOR
#include "ctx-split.h"

CTX_STATIC int ctx_color_model_get_components (CtxColorModel model)
{
  switch (model)
    {
      case CTX_GRAY:
        return 1;
      case CTX_GRAYA:
      case CTX_GRAYA_A:
        return 1;
      case CTX_RGB:
      case CTX_LAB:
      case CTX_LCH:
      case CTX_DRGB:
        return 3;
      case CTX_CMYK:
      case CTX_DCMYK:
      case CTX_LABA:
      case CTX_LCHA:
      case CTX_RGBA:
      case CTX_DRGBA:
      case CTX_RGBA_A:
      case CTX_RGBA_A_DEVICE:
        return 4;
      case CTX_DCMYKA:
      case CTX_CMYKA:
      case CTX_CMYKA_A:
      case CTX_DCMYKA_A:
        return 5;
    }
  return 0;
}

#if 0

inline static float ctx_u8_to_float (uint8_t val_u8)
{
  float val_f = val_u8 / 255.0;
  return val_f;
}
#else
float ctx_u8_float[256];

#endif

CtxColor *ctx_color_new ()
{
  CtxColor *color = (CtxColor*)calloc (sizeof (CtxColor), 1);
  return color;
}

int ctx_color_is_transparent (CtxColor *color)
{
  return color->alpha <= 0.001f;
}


void ctx_color_free (CtxColor *color)
{
  free (color);
}

CTX_STATIC void ctx_color_set_RGBA8 (CtxState *state, CtxColor *color, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  color->original = color->valid = CTX_VALID_RGBA_U8;
  color->rgba[0] = r;
  color->rgba[1] = g;
  color->rgba[2] = b;
  color->rgba[3] = a;
#if CTX_ENABLE_CM
  color->space = state->gstate.device_space;
#endif
}

#if 0
static void ctx_color_set_RGBA8_ (CtxColor *color, const uint8_t *in)
{
  ctx_color_set_RGBA8 (color, in[0], in[1], in[2], in[3]);
}
#endif

CTX_STATIC void ctx_color_set_graya (CtxState *state, CtxColor *color, float gray, float alpha)
{
  color->original = color->valid = CTX_VALID_GRAYA;
  color->l = gray;
  color->alpha = alpha;
}
#if 0
static void ctx_color_set_graya_ (CtxColor *color, const float *in)
{
  return ctx_color_set_graya (color, in[0], in[1]);
}
#endif

void ctx_color_set_rgba (CtxState *state, CtxColor *color, float r, float g, float b, float a)
{
#if CTX_ENABLE_CM
  color->original = color->valid = CTX_VALID_RGBA;
  color->red      = r;
  color->green    = g;
  color->blue     = b;
  color->space    = state->gstate.rgb_space;
#else
  color->original     = color->valid = CTX_VALID_RGBA_DEVICE;
  color->device_red   = r;
  color->device_green = g;
  color->device_blue  = b;
#endif
  color->alpha        = a;
}

CTX_STATIC void ctx_color_set_drgba (CtxState *state, CtxColor *color, float r, float g, float b, float a)
{
#if CTX_ENABLE_CM
  color->original     = color->valid = CTX_VALID_RGBA_DEVICE;
  color->device_red   = r;
  color->device_green = g;
  color->device_blue  = b;
  color->alpha        = a;
  color->space        = state->gstate.device_space;
#else
  ctx_color_set_rgba (state, color, r, g, b, a);
#endif
}

#if 0
static void ctx_color_set_rgba_ (CtxState *state, CtxColor *color, const float *in)
{
  ctx_color_set_rgba (color, in[0], in[1], in[2], in[3]);
}
#endif

/* the baseline conversions we have whether CMYK support is enabled or not,
 * providing an effort at right rendering
 */
static void ctx_cmyk_to_rgb (float c, float m, float y, float k, float *r, float *g, float *b)
{
  *r = (1.0f-c) * (1.0f-k);
  *g = (1.0f-m) * (1.0f-k);
  *b = (1.0f-y) * (1.0f-k);
}

// XXX needs state
void ctx_rgb_to_cmyk (float r, float g, float b,
              float *c_out, float *m_out, float *y_out, float *k_out)
{
  float c = 1.0f - r;
  float m = 1.0f - g;
  float y = 1.0f - b;
  float k = ctx_minf (c, ctx_minf (y, m) );
  if (k < 1.0f)
    {
      c = (c - k) / (1.0f - k);
      m = (m - k) / (1.0f - k);
      y = (y - k) / (1.0f - k);
    }
  else
    {
      c = m = y = 0.0f;
    }
  *c_out = c;
  *m_out = m;
  *y_out = y;
  *k_out = k;
}

#if CTX_ENABLE_CMYK
CTX_STATIC void ctx_color_set_cmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a)
{
  color->original = color->valid = CTX_VALID_CMYKA;
  color->cyan     = c;
  color->magenta  = m;
  color->yellow   = y;
  color->key      = k;
  color->alpha    = a;
#if CTX_ENABLE_CM
  color->space    = state->gstate.cmyk_space;
#endif
}

CTX_STATIC void ctx_color_set_dcmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a)
{
  color->original       = color->valid = CTX_VALID_DCMYKA;
  color->device_cyan    = c;
  color->device_magenta = m;
  color->device_yellow  = y;
  color->device_key     = k;
  color->alpha          = a;
#if CTX_ENABLE_CM
  color->space = state->gstate.device_space;
#endif
}

#endif

#if CTX_ENABLE_CM

static void ctx_rgb_user_to_device (CtxState *state, float rin, float gin, float bin,
                                    float *rout, float *gout, float *bout)
{
  /* babl plug-in point */
  *rout = rin ;
  *gout = gin ;
  *bout = bin ;
}

static void ctx_rgb_device_to_user (CtxState *state, float rin, float gin, float bin,
                                    float *rout, float *gout, float *bout)
{
  /* babl plug-in point */
  *rout = rin ;
  *gout = gin ;
  *bout = bin ;
}
#endif

static void ctx_color_get_drgba (CtxState *state, CtxColor *color, float *out)
{
  if (! (color->valid & CTX_VALID_RGBA_DEVICE) )
    {
#if CTX_ENABLE_CM
      if (color->valid & CTX_VALID_RGBA)
        {
          ctx_rgb_user_to_device (state, color->red, color->green, color->blue,
                                  & (color->device_red), & (color->device_green), & (color->device_blue) );
        }
      else
#endif
        if (color->valid & CTX_VALID_RGBA_U8)
          {
            color->device_red   = ctx_u8_to_float (color->rgba[0]);
            color->device_green = ctx_u8_to_float (color->rgba[1]);
            color->device_blue  = ctx_u8_to_float (color->rgba[2]);
            color->alpha        = ctx_u8_to_float (color->rgba[3]);
          }
#if CTX_ENABLE_CMYK
        else if (color->valid & CTX_VALID_CMYKA)
          {
            ctx_cmyk_to_rgb (color->cyan, color->magenta, color->yellow, color->key,
                             &color->device_red,
                             &color->device_green,
                             &color->device_blue);
          }
#endif
        else if (color->valid & CTX_VALID_GRAYA)
          {
            color->device_red   =
              color->device_green =
                color->device_blue  = color->l;
          }
      color->valid |= CTX_VALID_RGBA_DEVICE;
    }
  out[0] = color->device_red;
  out[1] = color->device_green;
  out[2] = color->device_blue;
  out[3] = color->alpha;
}

void ctx_color_get_rgba (CtxState *state, CtxColor *color, float *out)
{
#if CTX_ENABLE_CM
  if (! (color->valid & CTX_VALID_RGBA) )
    {
      ctx_color_get_drgba (state, color, out);
      if (color->valid & CTX_VALID_RGBA_DEVICE)
        {
          ctx_rgb_device_to_user (state, color->device_red, color->device_green, color->device_blue,
                                  & (color->red), & (color->green), & (color->blue) );
        }
      color->valid |= CTX_VALID_RGBA;
    }
  out[0] = color->red;
  out[1] = color->green;
  out[2] = color->blue;
  out[3] = color->alpha;
#else
  ctx_color_get_drgba (state, color, out);
#endif
}


float ctx_float_color_rgb_to_gray (CtxState *state, const float *rgb)
{
        // XXX todo replace with correct according to primaries
  return CTX_CSS_RGB_TO_LUMINANCE(rgb);
}
uint8_t ctx_u8_color_rgb_to_gray (CtxState *state, const uint8_t *rgb)
{
        // XXX todo replace with correct according to primaries
  return CTX_CSS_RGB_TO_LUMINANCE(rgb);
}

void ctx_color_get_graya (CtxState *state, CtxColor *color, float *out)
{
  if (! (color->valid & CTX_VALID_GRAYA) )
    {
      float rgba[4];
      ctx_color_get_drgba (state, color, rgba);
      color->l = ctx_float_color_rgb_to_gray (state, rgba);
      color->valid |= CTX_VALID_GRAYA;
    }
  out[0] = color->l;
  out[1] = color->alpha;
}

#if CTX_ENABLE_CMYK
void ctx_color_get_cmyka (CtxState *state, CtxColor *color, float *out)
{
  if (! (color->valid & CTX_VALID_CMYKA) )
    {
      if (color->valid & CTX_VALID_GRAYA)
        {
          color->cyan = color->magenta = color->yellow = 0.0;
          color->key = color->l;
        }
      else
        {
          float rgba[4];
          ctx_color_get_rgba (state, color, rgba);
          ctx_rgb_to_cmyk (rgba[0], rgba[1], rgba[2],
                           &color->cyan, &color->magenta, &color->yellow, &color->key);
          color->alpha = rgba[3];
        }
      color->valid |= CTX_VALID_CMYKA;
    }
  out[0] = color->cyan;
  out[1] = color->magenta;
  out[2] = color->yellow;
  out[3] = color->key;
  out[4] = color->alpha;
}

#if 0
static void ctx_color_get_cmyka_u8 (CtxState *state, CtxColor *color, uint8_t *out)
{
  if (! (color->valid & CTX_VALID_CMYKA_U8) )
    {
      float cmyka[5];
      ctx_color_get_cmyka (color, cmyka);
      for (int i = 0; i < 5; i ++)
        { color->cmyka[i] = ctx_float_to_u8 (cmyka[i]); }
      color->valid |= CTX_VALID_CMYKA_U8;
    }
  out[0] = color->cmyka[0];
  out[1] = color->cmyka[1];
  out[2] = color->cmyka[2];
  out[3] = color->cmyka[3];
}
#endif
#endif

void
ctx_color_get_rgba8 (CtxState *state, CtxColor *color, uint8_t *out)
{
  if (! (color->valid & CTX_VALID_RGBA_U8) )
    {
      float rgba[4];
      ctx_color_get_drgba (state, color, rgba);
      for (int i = 0; i < 4; i ++)
        { color->rgba[i] = ctx_float_to_u8 (rgba[i]); }
      color->valid |= CTX_VALID_RGBA_U8;
    }
  out[0] = color->rgba[0];
  out[1] = color->rgba[1];
  out[2] = color->rgba[2];
  out[3] = color->rgba[3];
}

void ctx_color_get_graya_u8 (CtxState *state, CtxColor *color, uint8_t *out)
{
  if (! (color->valid & CTX_VALID_GRAYA_U8) )
    {
      float graya[2];
      ctx_color_get_graya (state, color, graya);
      color->l_u8 = ctx_float_to_u8 (graya[0]);
      color->rgba[3] = ctx_float_to_u8 (graya[1]);
      color->valid |= CTX_VALID_GRAYA_U8;
    }
  out[0] = color->l_u8;
  out[1] = color->rgba[3];
}


void
ctx_get_rgba (Ctx *ctx, float *rgba)
{
  ctx_color_get_rgba (& (ctx->state), &ctx->state.gstate.source.color, rgba);
}

void
ctx_get_drgba (Ctx *ctx, float *rgba)
{
  ctx_color_get_drgba (& (ctx->state), &ctx->state.gstate.source.color, rgba);
}

int ctx_in_fill (Ctx *ctx, float x, float y)
{
  float x1, y1, x2, y2;
  ctx_path_extents (ctx, &x1, &y1, &x2, &y2);

  if (x1 <= x && x <= x2 && // XXX - just bounding box for now
      y1 <= y && y <= y2)   //
    return 1;
  return 0;
}


#if CTX_ENABLE_CMYK
void
ctx_get_cmyka (Ctx *ctx, float *cmyka)
{
  ctx_color_get_cmyka (& (ctx->state), &ctx->state.gstate.source.color, cmyka);
}
#endif
void
ctx_get_graya (Ctx *ctx, float *ya)
{
  ctx_color_get_graya (& (ctx->state), &ctx->state.gstate.source.color, ya);
}

#if CTX_ENABLE_CM
void
ctx_set_drgb_space (Ctx *ctx, int device_space)
{
  CTX_PROCESS_U8 (CTX_SET_DRGB_SPACE, device_space);
}

void
ctx_set_dcmyk_space (Ctx *ctx, int device_space)
{
  CTX_PROCESS_U8 (CTX_SET_DCMYK_SPACE, device_space);
}

void
ctx_rgb_space (Ctx *ctx, int device_space)
{
  CTX_PROCESS_U8 (CTX_SET_RGB_SPACE, device_space);
}

void
ctx_set_cmyk_space (Ctx *ctx, int device_space)
{
  CTX_PROCESS_U8 (CTX_SET_CMYK_SPACE, device_space);
}
#endif


void ctx_drgba (Ctx *ctx, float r, float g, float b, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_DRGBA, r),
    ctx_f (CTX_CONT, g, b),
    ctx_f (CTX_CONT, a, 0)
  };
  ctx_process (ctx, command);
}

void ctx_rgba (Ctx *ctx, float r, float g, float b, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_RGBA, r),
    ctx_f (CTX_CONT, g, b),
    ctx_f (CTX_CONT, a, 0)
  };
  float rgba[4];
  ctx_color_get_rgba (&ctx->state, &ctx->state.gstate.source.color, rgba);
  if (rgba[0] == r && rgba[1] == g && rgba[2] == b && rgba[3] == a)
     return;
  ctx_process (ctx, command);
}

void ctx_rgb (Ctx *ctx, float   r, float   g, float   b)
{
  ctx_rgba (ctx, r, g, b, 1.0f);
}

void ctx_gray (Ctx *ctx, float gray)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_GRAY, gray),
    ctx_f (CTX_CONT, 1.0f, 0.0f),
    ctx_f (CTX_CONT, 0.0f, 0.0f)
  };
  ctx_process (ctx, command);
}

#if CTX_ENABLE_CMYK
void ctx_cmyk (Ctx *ctx, float c, float m, float y, float k)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_CMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, 1.0f)
  };
  ctx_process (ctx, command);
}

void ctx_cmyka      (Ctx *ctx, float c, float m, float y, float k, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_CMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, a)
  };
  ctx_process (ctx, command);
}

void ctx_dcmyk (Ctx *ctx, float c, float m, float y, float k)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_DCMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, 1.0f)
  };
  ctx_process (ctx, command);
}

void ctx_dcmyka (Ctx *ctx, float c, float m, float y, float k, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_DCMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, a)
  };
  ctx_process (ctx, command);
}

#endif

/* XXX: missing CSS1:
 *
 *   EM { color: rgb(110%, 0%, 0%) }  // clipped to 100% 
 *
 *
 *   :first-letter
 *   :first-list
 *   :link :visited :active
 *
 */

typedef struct ColorDef {
  uint32_t name;
  float r;
  float g;
  float b;
  float a;
} ColorDef;

#define CTX_silver 	CTX_STRH('s','i','l','v','e','r',0,0,0,0,0,0,0,0)
#define CTX_fuchsia 	CTX_STRH('f','u','c','h','s','i','a',0,0,0,0,0,0,0)
#define CTX_gray 	CTX_STRH('g','r','a','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_yellow 	CTX_STRH('y','e','l','l','o','w',0,0,0,0,0,0,0,0)
#define CTX_white 	CTX_STRH('w','h','i','t','e',0,0,0,0,0,0,0,0,0)
#define CTX_maroon 	CTX_STRH('m','a','r','o','o','n',0,0,0,0,0,0,0,0)
#define CTX_magenta 	CTX_STRH('m','a','g','e','n','t','a',0,0,0,0,0,0,0)
#define CTX_blue 	CTX_STRH('b','l','u','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_green 	CTX_STRH('g','r','e','e','n',0,0,0,0,0,0,0,0,0)
#define CTX_red 	CTX_STRH('r','e','d',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_purple 	CTX_STRH('p','u','r','p','l','e',0,0,0,0,0,0,0,0)
#define CTX_olive 	CTX_STRH('o','l','i','v','e',0,0,0,0,0,0,0,0,0)
#define CTX_teal        CTX_STRH('t','e','a','l',0,0,0,0,0,0,0,0,0,0)
#define CTX_black 	CTX_STRH('b','l','a','c','k',0,0,0,0,0,0,0,0,0)
#define CTX_cyan 	CTX_STRH('c','y','a','n',0,0,0,0,0,0,0,0,0,0)
#define CTX_navy 	CTX_STRH('n','a','v','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_lime 	CTX_STRH('l','i','m','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_aqua 	CTX_STRH('a','q','u','a',0,0,0,0,0,0,0,0,0,0)
#define CTX_transparent CTX_STRH('t','r','a','n','s','p','a','r','e','n','t',0,0,0)

static ColorDef colors[]={
  {CTX_black,    0, 0, 0, 1},
  {CTX_red,      1, 0, 0, 1},
  {CTX_green,    0, 1, 0, 1},
  {CTX_yellow,   1, 1, 0, 1},
  {CTX_blue,     0, 0, 1, 1},
  {CTX_fuchsia,  1, 0, 1, 1},
  {CTX_cyan,     0, 1, 1, 1},
  {CTX_white,    1, 1, 1, 1},
  {CTX_silver,   0.75294, 0.75294, 0.75294, 1},
  {CTX_gray,     0.50196, 0.50196, 0.50196, 1},
  {CTX_magenta,  0.50196, 0, 0.50196, 1},
  {CTX_maroon,   0.50196, 0, 0, 1},
  {CTX_purple,   0.50196, 0, 0.50196, 1},
  {CTX_green,    0, 0.50196, 0, 1},
  {CTX_lime,     0, 1, 0, 1},
  {CTX_olive,    0.50196, 0.50196, 0, 1},
  {CTX_navy,     0, 0,      0.50196, 1},
  {CTX_teal,     0, 0.50196, 0.50196, 1},
  {CTX_aqua,     0, 1, 1, 1},
  {CTX_transparent, 0, 0, 0, 0},
  {CTX_none,     0, 0, 0, 0},
};

static int xdigit_value(const char xdigit)
{
  if (xdigit >= '0' && xdigit <= '9')
   return xdigit - '0';
  switch (xdigit)
  {
    case 'A':case 'a': return 10;
    case 'B':case 'b': return 11;
    case 'C':case 'c': return 12;
    case 'D':case 'd': return 13;
    case 'E':case 'e': return 14;
    case 'F':case 'f': return 15;
  }
  return 0;
}

static int
ctx_color_parse_rgb (CtxState *ctxstate, CtxColor *color, const char *color_string)
{
  float dcolor[4] = {0,0,0,1};
  while (*color_string && *color_string != '(')
    color_string++;
  if (*color_string) color_string++;

  {
    int n_floats = 0;
    char *p =    (char*)color_string;
    char *prev = (char*)NULL;
    for (; p && n_floats < 4 && p != prev && *p; )
    {
      float val;
      prev = p;
      val = _ctx_parse_float (p, &p);
      if (p != prev)
      {
        if (n_floats < 3)
          dcolor[n_floats++] = val/255.0;
        else
          dcolor[n_floats++] = val;

        while (*p == ' ' || *p == ',')
        {
          p++;
          prev++;
        }
      }
    }
  }
  ctx_color_set_rgba (ctxstate, color, dcolor[0], dcolor[1],dcolor[2],dcolor[3]);
  return 0;
}

static int ctx_isxdigit (uint8_t ch)
{
  if (ch >= '0' && ch <= '9') return 1;
  if (ch >= 'a' && ch <= 'f') return 1;
  if (ch >= 'A' && ch <= 'F') return 1;
  return 0;
}

static int
mrg_color_parse_hex (CtxState *ctxstate, CtxColor *color, const char *color_string)
{
  float dcolor[4]={0,0,0,1};
  int string_length = strlen (color_string);
  int i;
  dcolor[3] = 1.0;

  if (string_length == 7 ||  /* #rrggbb   */
      string_length == 9)    /* #rrggbbaa */
    {
      int num_iterations = (string_length - 1) / 2;
  
      for (i = 0; i < num_iterations; ++i)
        {
          if (ctx_isxdigit (color_string[2 * i + 1]) &&
              ctx_isxdigit (color_string[2 * i + 2]))
            {
              dcolor[i] = (xdigit_value (color_string[2 * i + 1]) << 4 |
                           xdigit_value (color_string[2 * i + 2])) / 255.f;
            }
          else
            {
              return 0;
            }
        }
      /* Successful #rrggbb(aa) parsing! */
      ctx_color_set_rgba (ctxstate, color, dcolor[0], dcolor[1],dcolor[2],dcolor[3]);
      return 1;
    }
  else if (string_length == 4 ||  /* #rgb  */
           string_length == 5)    /* #rgba */
    {
      int num_iterations = string_length - 1;
      for (i = 0; i < num_iterations; ++i)
        {
          if (ctx_isxdigit (color_string[i + 1]))
            {
              dcolor[i] = (xdigit_value (color_string[i + 1]) << 4 |
                           xdigit_value (color_string[i + 1])) / 255.f;
            }
          else
            {
              return 0;
            }
        }
      ctx_color_set_rgba (ctxstate, color, dcolor[0], dcolor[1],dcolor[2],dcolor[3]);
      /* Successful #rgb(a) parsing! */
      return 0;
    }
  /* String was of unsupported length. */
  return 1;
}

#define CTX_currentColor 	CTX_STRH('c','u','r','r','e','n','t','C','o','l','o','r',0,0)

int ctx_color_set_from_string (Ctx *ctx, CtxColor *color, const char *string)
{
  int i;
  uint32_t hash = ctx_strhash (string, 0);
//  ctx_color_set_rgba (&(ctx->state), color, 0.4,0.1,0.9,1.0);
//  return 0;
    //rgba[0], rgba[1], rgba[2], rgba[3]);

  if (hash == CTX_currentColor)
  {
    float rgba[4];
    CtxColor ccolor;
    ctx_get_color (ctx, CTX_color, &ccolor);
    ctx_color_get_rgba (&(ctx->state), &ccolor, rgba);
    ctx_color_set_rgba (&(ctx->state), color, rgba[0], rgba[1], rgba[2], rgba[3]);
    return 0;
  }

  for (i = (sizeof(colors)/sizeof(colors[0]))-1; i>=0; i--)
  {
    if (hash == colors[i].name)
    {
      ctx_color_set_rgba (&(ctx->state), color,
       colors[i].r, colors[i].g, colors[i].b, colors[i].a);
      return 0;
    }
  }

  if (string[0] == '#')
    mrg_color_parse_hex (&(ctx->state), color, string);
  else if (string[0] == 'r' &&
      string[1] == 'g' &&
      string[2] == 'b'
      )
    ctx_color_parse_rgb (&(ctx->state), color, string);

  return 0;
}

int ctx_color (Ctx *ctx, const char *string)
{
  CtxColor color;
  ctx_color_set_from_string (ctx, &color, string);
  float rgba[4];
  ctx_color_get_rgba (&(ctx->state), &color, rgba);
  ctx_rgba (ctx, rgba[0],rgba[1],rgba[2],rgba[3]);
  return 0;
}

void
ctx_rgba8 (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_RGBA_U8, r, g, b, a, 0, 0, 0, 0);

  uint8_t rgba[4];
  ctx_color_get_rgba8 (&ctx->state, &ctx->state.gstate.source.color, rgba);
  if (rgba[0] == r && rgba[1] == g && rgba[2] == b && rgba[3] == a)
     return;

  ctx_process (ctx, &command);
}

#endif 
