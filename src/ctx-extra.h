#ifndef __CTX_EXTRA_H
#define __CTX_EXTRA_H

#if CTX_FORCE_INLINES
#define CTX_INLINE inline __attribute__((always_inline))
#else
#define CTX_INLINE inline
#endif


#define CTX_CLAMP(val,min,max) ((val)<(min)?(min):(val)>(max)?(max):(val))
//static CTX_INLINE int   ctx_mini (const int a, const int b)     { if (a < b) return a; return b; }
//static CTX_INLINE int   ctx_maxi (const int a, const int b)     { if (a > b) return a; return b; }
static CTX_INLINE int   ctx_mini (const int a, const int b)     {
return (a<b)*a+(a>=b)*b;
	//if (a < b) return a; return b; 
}
static CTX_INLINE int   ctx_maxi (const int a, const int b)     {
return (a>b)*a+(a<=b)*b;
	//if (a > b) return a; return b; 
}
static CTX_INLINE float ctx_minf (const float a, const float b) { if (a < b) return a; return b; }
static CTX_INLINE float ctx_maxf (const float a, const float b) { if (a > b) return a; return b; }
static CTX_INLINE float ctx_clampf (const float v, const float min, const float max) {
       return CTX_CLAMP(v,min,max);
}


typedef enum CtxOutputmode
{
  CTX_OUTPUT_MODE_QUARTER,
  CTX_OUTPUT_MODE_BRAILLE,
  CTX_OUTPUT_MODE_SIXELS,
  CTX_OUTPUT_MODE_GRAYS,
  CTX_OUTPUT_MODE_CTX,
  CTX_OUTPUT_MODE_CTX_COMPACT,
  CTX_OUTPUT_MODE_CTX_FILE,
  CTX_OUTPUT_MODE_CTX_COMPACT_FILE,
  CTX_OUTPUT_MODE_UI
} CtxOutputmode;

static CTX_INLINE float ctx_pow2 (const float a) { return a * a; }
#if CTX_MATH

static CTX_INLINE float
ctx_fabsf (const float x)
{
  union
  {
    float f;
    uint32_t i;
  } u = { x };
  u.i &= 0x7fffffff;
  return u.f;
}

static CTX_INLINE float
ctx_invsqrtf (const float x)
{
  union
  {
    float f;
    uint32_t i;
  } u = { x };
  u.i = 0x5f3759df - (u.i >> 1);
  u.f *= (1.5f - 0.5f * x * u.f * u.f);
  u.f *= (1.5f - 0.5f * x * u.f * u.f); //repeating Newton-Raphson step for higher precision
  return u.f;
}


CTX_INLINE static float ctx_sqrtf (const float a)
{
  return 1.0f/ctx_invsqrtf (a);
}

CTX_INLINE static float ctx_hypotf (const float a, const float b)
{
  return ctx_sqrtf (ctx_pow2 (a)+ctx_pow2 (b) );
}

CTX_INLINE static float
ctx_sinf (float x)
{
  if (x < -CTX_PI * 2)
    {
      x = -x;
      long ix = (long)(x / (CTX_PI * 2));
      x = x - ix * CTX_PI * 2;
      x = -x;
    }
  if (x < -CTX_PI * 1000)
  {
    x = -0.5f;
  }
  if (x > CTX_PI * 1000)
  {
    // really large numbers tend to cause practically inifinite
    // loops since the > CTX_PI * 2 seemingly fails
    x = 0.5f;
  }
  if (x > CTX_PI * 2)
    { 
      long ix = (long)(x / (CTX_PI * 2));
      x = x - (ix * CTX_PI * 2);
    }
  while (x < -CTX_PI)
    { x += CTX_PI * 2; }
  while (x > CTX_PI)
    { x -= CTX_PI * 2; }

  /* source : http://mooooo.ooo/chebyshev-sine-approximation/ */
  const float coeffs[]=
  {
    -0.10132118f,           // x
      0.0066208798f,         // x^3
      -0.00017350505f,        // x^5
      0.0000025222919f,      // x^7
      -0.000000023317787f,    // x^9
      0.00000000013291342f
    }; // x^11
  float x2 = x*x;
  float p11 = coeffs[5];
  float p9  = p11*x2 + coeffs[4];
  float p7  = p9*x2  + coeffs[3];
  float p5  = p7*x2  + coeffs[2];
  float p3  = p5*x2  + coeffs[1];
  float p1  = p3*x2  + coeffs[0];
  return (x - CTX_PI + 0.00000008742278f) *
         (x + CTX_PI - 0.00000008742278f) * p1 * x;
}

static CTX_INLINE float ctx_atan2f (const float y, const float x)
{
  float atan, z;
  if ( x == 0.0f )
    {
      if ( y > 0.0f )
        { return CTX_PI/2; }
      if ( y == 0.0f )
        { return 0.0f; }
      return -CTX_PI/2;
    }
  z = y/x;
  if ( ctx_fabsf ( z ) < 1.0f )
    {
      atan = z/ (1.0f + 0.28f*z*z);
      if (x < 0.0f)
        {
          if ( y < 0.0f )
            { return atan - CTX_PI; }
          return atan + CTX_PI;
        }
    }
  else
    {
      atan = CTX_PI/2 - z/ (z*z + 0.28f);
      if ( y < 0.0f ) { return atan - CTX_PI; }
    }
  return atan;
}


static CTX_INLINE float ctx_atanf (const float a)
{
  return ctx_atan2f (a, 1.0f);
}

static CTX_INLINE float ctx_asinf (const float x)
{
  return ctx_atanf ( x * ctx_invsqrtf (1.0f-ctx_pow2 (x) ));
}

static CTX_INLINE float ctx_acosf (const float x)
{
  return ctx_atanf ( ctx_sqrtf (1.0f-ctx_pow2 (x) ) / (x) );
}

CTX_INLINE static float ctx_cosf (const float a)
{
  return ctx_sinf ( (a) + CTX_PI/2.0f);
}

static CTX_INLINE float ctx_tanf (const float a)
{
  return (ctx_cosf (a) / ctx_sinf (a) );
}
static CTX_INLINE float
ctx_floorf (const float x)
{
  return (int)x; // XXX
}
static CTX_INLINE float
ctx_expf (const float x)
{
  union { uint32_t i; float f; } v =
    {  (uint32_t)( (1 << 23) * (x + 183.1395965f)) };
  return v.f;
}

/* define more trig based on having sqrt, sin and atan2 */

#else
#if !__COSMOPOLITAN__
#include <math.h>
#endif
static CTX_INLINE float ctx_fabsf (const float x)           { return fabsf (x); }
static CTX_INLINE float ctx_floorf (const float x)          { return floorf (x); }
static CTX_INLINE float ctx_asinf (const float x)           { return asinf (x); }
static CTX_INLINE float ctx_sinf (const float x)            { return sinf (x); }
static CTX_INLINE float ctx_atan2f (const float y, float x) { return atan2f (y, x); }
static CTX_INLINE float ctx_hypotf (const float a, float b) { return hypotf (a, b); }
static CTX_INLINE float ctx_acosf (const float a)           { return acosf (a); }
static CTX_INLINE float ctx_cosf (const float a)            { return cosf (a); }
static CTX_INLINE float ctx_tanf (const float a)            { return tanf (a); }
static CTX_INLINE float ctx_expf (const float p)            { return expf (p); }
static CTX_INLINE float ctx_sqrtf (const float a)           { return sqrtf (a); }
static CTX_INLINE float ctx_atanf (const float a)           { return atanf (a); }
#endif

static CTX_INLINE float
ctx_invsqrtf_fast (const float x)
{
  union
  {
    float f;
    uint32_t i;
  } u = { x };
  u.i = 0x5f3759df - (u.i >> 1);
  return u.f;
}
CTX_INLINE static float ctx_sqrtf_fast (const float a)
{
  return 1.0f/ctx_invsqrtf_fast (a);
}
CTX_INLINE static float ctx_hypotf_fast (const float a, const float b)
{
  return ctx_sqrtf_fast (ctx_pow2 (a)+ctx_pow2 (b) );
}


static CTX_INLINE float ctx_atan2f_rest (
  const float x, const float y_recip)
{
  float atan, z = x * y_recip;
  if ( ctx_fabsf ( z ) < 1.0f )
    {
      atan = z/ (1.0f + 0.28f*z*z);
      if (y_recip < 0.0f)
        {
          if ( x < 0.0f )
            { return atan - CTX_PI; }
          return atan + CTX_PI;
        }
    }
  else
    {
      atan = CTX_PI/2 - z/ (z*z + 0.28f);
      if ( x < 0.0f ) { return atan - CTX_PI; }
    }
  return atan;
}


static inline float _ctx_parse_float (const char *str, char **endptr)
{
  return strtof (str, endptr); /* XXX: , vs . problem in some locales */
}

const char *ctx_get_string (Ctx *ctx, uint32_t hash);
void ctx_set_string (Ctx *ctx, uint32_t hash, const char *value);
typedef struct _CtxColor CtxColor;

void
ctx_matrix_translate (CtxMatrix *matrix, float x, float y);


void ctx_get_matrix (Ctx *ctx, CtxMatrix *matrix);
void ctx_set_matrix (Ctx *ctx, CtxMatrix *matrix);
int _ctx_is_rasterizer (Ctx *ctx);

int ctx_color (Ctx *ctx, const char *string);
typedef struct _CtxState CtxState;
CtxColor *ctx_color_new (void);
CtxState *ctx_get_state (Ctx *ctx);
void ctx_color_get_rgba (CtxState *state, CtxColor *color, float *out);
void ctx_color_set_rgba (CtxState *state, CtxColor *color, float r, float g, float b, float a);
void ctx_color_free (CtxColor *color);
void ctx_set_color (Ctx *ctx, uint32_t hash, CtxColor *color);
int  ctx_get_color (Ctx *ctx, uint32_t hash, CtxColor *color);
int  ctx_color_set_from_string (Ctx *ctx, CtxColor *color, const char *string);

int ctx_color_is_transparent (CtxColor *color);
int ctx_utf8_len (const unsigned char first_byte);

void ctx_user_to_device          (Ctx *ctx, float *x, float *y);
void ctx_user_to_device_distance (Ctx *ctx, float *x, float *y);


void ctx_device_to_user          (Ctx *ctx, float *x, float *y);
void ctx_device_to_user_distance (Ctx *ctx, float *x, float *y);

const char *ctx_utf8_skip (const char *s, int utf8_length);
int ctx_is_set_now (Ctx *ctx, uint32_t hash);
void ctx_set_size (Ctx *ctx, int width, int height);

static inline float ctx_matrix_get_scale (CtxMatrix *matrix)
{
   return ctx_maxf (ctx_maxf (ctx_fabsf (matrix->m[0][0]),
                         ctx_fabsf (matrix->m[0][1]) ),
               ctx_maxf (ctx_fabsf (matrix->m[1][0]),
                         ctx_fabsf (matrix->m[1][1]) ) );
}

#if CTX_GET_CONTENTS
int
_ctx_file_get_contents (const char     *path,
                        unsigned char **contents,
                        long           *length);
#endif

#if CTX_FONTS_FROM_FILE
int   ctx_load_font_ttf_file (const char *name, const char *path);
#endif

#if CTX_BABL
void ctx_rasterizer_colorspace_babl (CtxState      *state,
                                     CtxColorSpace  space_slot,
                                     const Babl    *space);
#endif
void ctx_rasterizer_colorspace_icc (CtxState            *state,
                                    CtxColorSpace        space_slot,
                                    const unsigned char *icc_data,
                                    int                  icc_length);


CtxBuffer *ctx_buffer_new_bare (void);

void ctx_buffer_set_data (CtxBuffer *buffer,
                          void *data, int width, int height,
                          int stride,
                          CtxPixelFormat pixel_format,
                          void (*freefunc) (void *pixels, void *user_data),
                          void *user_data);

int ctx_textureclock (Ctx *ctx);


void ctx_done_frame (Ctx *ctx);
void ctx_list_backends(void);
int ctx_pixel_format_ebpp (CtxPixelFormat format);


#endif
