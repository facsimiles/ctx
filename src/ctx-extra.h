#ifndef __CTX_EXTRA_H
#define __CTX_EXTRA_H

#if CTX_FORCE_INLINES
#define CTX_INLINE inline __attribute__((always_inline))
#else
#define CTX_INLINE inline
#endif


#define CTX_CLAMP(val,min,max) ((val)<(min)?(min):(val)>(max)?(max):(val))
static CTX_INLINE int   ctx_mini (int a, int b)     { return (a < b) * a + (a >= b) * b; }
static CTX_INLINE float ctx_minf (float a, float b) { return (a < b) * a + (a >= b) * b; }
static CTX_INLINE int   ctx_maxi (int a, int b)     { return (a > b) * a + (a <= b) * b; }
static CTX_INLINE float ctx_maxf (float a, float b) { return (a > b) * a + (a <= b) * b; }
static CTX_INLINE float ctx_clampf (float v, float min, float max) {
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
  CTX_OUTPUT_MODE_UI
} CtxOutputmode;






static inline float ctx_pow2 (float a) { return a * a; }
#if CTX_MATH

static CTX_INLINE float
ctx_fabsf (float x)
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
ctx_invsqrtf (float x)
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

static CTX_INLINE float
ctx_invsqrtf_fast (float x)
{
  union
  {
    float f;
    uint32_t i;
  } u = { x };
  u.i = 0x5f3759df - (u.i >> 1);
  return u.f;
}

CTX_INLINE static float ctx_sqrtf (float a)
{
  return 1.0f/ctx_invsqrtf (a);
}

CTX_INLINE static float ctx_sqrtf_fast (float a)
{
  return 1.0f/ctx_invsqrtf_fast (a);
}

CTX_INLINE static float ctx_hypotf (float a, float b)
{
  return ctx_sqrtf (ctx_pow2 (a)+ctx_pow2 (b) );
}

CTX_INLINE static float ctx_hypotf_fast (float a, float b)
{
  return ctx_sqrtf_fast (ctx_pow2 (a)+ctx_pow2 (b) );
}

CTX_INLINE static float
ctx_sinf (float x)
{
  if (x < -CTX_PI * 2)
    {
      x = -x;
      long ix = x / (CTX_PI * 2);
      x = x - ix * CTX_PI * 2;
      x = -x;
    }
  if (x < -CTX_PI * 1000)
  {
    x = -0.5;
  }
  if (x > CTX_PI * 1000)
  {
          // really large numbers tend to cause practically inifinite
          // loops since the > CTX_PI * 2 seemingly fails
    x = 0.5;
  }
  if (x > CTX_PI * 2)
    { 
      long ix = x / (CTX_PI * 2);
      x = x - (ix * CTX_PI * 2);
    }
  while (x < -CTX_PI)
    { x += CTX_PI * 2; }
  while (x > CTX_PI)
    { x -= CTX_PI * 2; }

  /* source : http://mooooo.ooo/chebyshev-sine-approximation/ */
  float coeffs[]=
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

static CTX_INLINE float ctx_atan2f (float y, float x)
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


static CTX_INLINE float ctx_atanf (float a)
{
  return ctx_atan2f ( (a), 1.0f);
}

static CTX_INLINE float ctx_asinf (float x)
{
  return ctx_atanf ( (x) * (ctx_invsqrtf (1.0f-ctx_pow2 (x) ) ) );
}

static CTX_INLINE float ctx_acosf (float x)
{
  return ctx_atanf ( (ctx_sqrtf (1.0f-ctx_pow2 (x) ) / (x) ) );
}

CTX_INLINE static float ctx_cosf (float a)
{
  return ctx_sinf ( (a) + CTX_PI/2.0f);
}

static CTX_INLINE float ctx_tanf (float a)
{
  return (ctx_cosf (a) /ctx_sinf (a) );
}
static CTX_INLINE float
ctx_floorf (float x)
{
  return (int)x; // XXX
}
static CTX_INLINE float
ctx_expf (float x)
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
static inline float ctx_fabsf (float x)           { return fabsf (x); }
static inline float ctx_floorf (float x)          { return floorf (x); }
static inline float ctx_sinf (float x)            { return sinf (x); }
static inline float ctx_atan2f (float y, float x) { return atan2f (y, x); }
static inline float ctx_hypotf (float a, float b) { return hypotf (a, b); }
static inline float ctx_acosf (float a)           { return acosf (a); }
static inline float ctx_cosf (float a)            { return cosf (a); }
static inline float ctx_tanf (float a)            { return tanf (a); }
static inline float ctx_expf (float p)            { return expf (p); }
static inline float ctx_sqrtf (float a)           { return sqrtf (a); }
#endif

static inline float _ctx_parse_float (const char *str, char **endptr)
{
  return strtod (str, endptr); /* XXX: , vs . problem in some locales */
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
CtxColor *ctx_color_new ();
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
void ctx_rasterizer_colorspace_icc (CtxState      *state,
                                    CtxColorSpace  space_slot,
                                    char          *icc_data,
                                    int            icc_length);


CtxBuffer *ctx_buffer_new_bare (void);

void ctx_buffer_set_data (CtxBuffer *buffer,
                          void *data, int width, int height,
                          int stride,
                          CtxPixelFormat pixel_format,
                          void (*freefunc) (void *pixels, void *user_data),
                          void *user_data);

int _ctx_set_frame (Ctx *ctx, int frame);
int _ctx_frame (Ctx *ctx);


void ctx_exit (Ctx *ctx);
void ctx_list_backends(void);
int ctx_pixel_format_ebpp (CtxPixelFormat format);


#endif
