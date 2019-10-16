/* ctx - tiny footprint 2d vector rasterizer context

  microcontrollers tested on:
    linux, ESP32 and MAX32666 (dual core Cortex-M4F at 96 MHz)

  Features:
    Pixel-formats:
      GRAY1    1bit b/w            - width must be multiple of 8
      GRAY2    4 level grayscale   - width must be multiple of 4
      GRAY4    16 level grayscale  - width must be multiple of 2
      GRAY8    256 level grayscale
      GRAYA8   256 level grayscale, with 256 level alpha
      RGBA8
      BGRA8
      RGB565
      RGB565 byteswapped,
      RGBAF    32bit floating point
      GRAYF

    Configurable can be compiled with a reduced feature-set for smaller
    binary size on embedded platforms.

    HTML5 Canvas|PDF|SVG drawing API and capabilities:
      (rel)move_to, line_to, curve_to, quad_to, arc_to
      scale, rotate, translate, save, restore, arc, rectangle, fill,
      line-width, stroke, end-caps, linear and radial gradients,
      using PNGs as patterns, text.

    Two font-backends, internal - with no overhead, and ttf from in-memory
    representation using stb_truetype, both driven with the same UTF8 API.

    Negative-width lines get drawn ultrafast 1px wide with bresenham, useful
    for scopes.

    Compact draw-list/command-stream representation, for low-RAM storage, and
    network transmission.

    Implements own math and strtod functions using only single precision float.

    Dithering of gradients/images to avoid banding in RGB565

  known bugs (not missing feature):
    refpack is currently disabled unstable due to packing across CONTs
    avoidance of scale/rotate/translate is buggy when journal is full
    sources are not dealy correctly with under transforms
*/


#ifndef CTX_H
#define CTX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifndef CTX_RASTERIZER  // set to 0 before including ctx to disable renderer code
#define CTX_RASTERIZER 1
#endif

#ifndef CTX_RASTERIZER_AA
#define CTX_RASTERIZER_AA      5
#endif
#ifndef CTX_RASTERIZER_AA2
#define CTX_RASTERIZER_AA2     2
#endif

#ifndef CTX_RASTERIZER_AUTOHINT
#define CTX_RASTERIZER_AUTOHINT   1 // should be made dynamic, only works
                                    // without forced AA
#endif

#ifndef CTX_RASTERIZER_FORCE_AA
#define CTX_RASTERIZER_FORCE_AA 0
#endif
#define CTX_RASTERIZER_AA_SLOPE_LIMIT  256

#define CTX_SUBDIV            8  // changing this changes font-file-format

#define CTX_BAKE_FONT_SIZE   80
#define CTX_BITPACK           1
#define CTX_REFPACK           0

#ifndef CTX_MATH
#define CTX_MATH              1  // use internal fast math for sqrt,sin,cos,atan2f etc.
#endif

#define ctx_log(fmt, ...)
//#define ctx_log(str, a...) fprintf(stderr, str, ##a)

#ifndef CTX_MAX_JOURNAL_SIZE
#define CTX_MAX_JOURNAL_SIZE   24096
#endif

#if CTX_BITPACK
   // should be possible to cut out for more minimal builds
#ifndef CTX_BITPACK_PACKER
#define CTX_BITPACK_PACKER 1
#endif
#endif

#ifndef CTX_DITHER
#define CTX_DITHER 1
#endif

#ifndef CTX_LIMIT_FORMATS

#define CTX_ENABLE_GRAY8                1
#define CTX_ENABLE_GRAYA8               1
#define CTX_ENABLE_RGB8                 1
#define CTX_ENABLE_RGBA8                1
#define CTX_ENABLE_BGRA8                1
#define CTX_ENABLE_RGB565               1
#define CTX_ENABLE_RGB565_BYTESWAPPED   1
#define CTX_ENABLE_RGBAF                1
#define CTX_ENABLE_GRAYF                1
#define CTX_ENABLE_GRAY1                1
#define CTX_ENABLE_GRAY2                1
#define CTX_ENABLE_GRAY4                1

#endif

#define CTX_RASTERIZER_EDGE_MULTIPLIER    1024


#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
  #ifndef CTX_FONT_ENGINE_STB
    #define CTX_FONT_ENGINE_STB      1
  #endif
#else
  #define CTX_FONT_ENGINE_STB        0
#endif

#if CTX_FONT_sgi
  #define CTX_FONT_ENGINE_MONOBITMAP 1
#endif

#if CTX_FONT_regular || CTX_FONT_mono || CTX_FONT_bold \
  || CTX_FONT_italic || CTX_FONT_sans || CTX_FONT_serif
#ifndef CTX_FONT_ENGINE_CTX
  #define CTX_FONT_ENGINE_CTX        1
#endif
#endif

#ifdef CAIRO_H
#define CTX_CAIRO 1
#else
#define CTX_CAIRO 0
#endif

#define CTX_PI               3.141592653589793f
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS  64 

#ifndef CTX_MAX_FONTS
#define CTX_MAX_FONTS        3
#endif
#define CTX_MAX_STATES       16
#define CTX_MAX_EDGES        257
#define CTX_MAX_GRADIENTS    4
#define CTX_MAX_TEXTURES     4
#define CTX_MAX_PENDING      128

#ifndef CTX_EXTRAS
#define CTX_EXTRAS  0
#endif

#define CTX_ASSERT 0

#if CTX_ASSERT==1
#define ctx_assert(a)  if(!(a)){fprintf(stderr,"%s:%i assertion failed\n", __FUNCTION__, __LINE__);  }
#else
#define ctx_assert(a)
#endif

typedef enum
{
  CTX_FORMAT_GRAY8,
  CTX_FORMAT_GRAYA8,
  CTX_FORMAT_RGB8,
  CTX_FORMAT_RGBA8,
  CTX_FORMAT_BGRA8,
  CTX_FORMAT_RGB565,
  CTX_FORMAT_RGB565_BYTESWAPPED,
  CTX_FORMAT_RGBAF,
  CTX_FORMAT_GRAYF,
  CTX_FORMAT_GRAY1,
  CTX_FORMAT_GRAY2,
  CTX_FORMAT_GRAY4
} CtxPixelFormat;

typedef struct _Ctx Ctx;

Ctx *ctx_new                 (void);
Ctx *ctx_new_for_framebuffer (void *data,
                              int width, int height, int stride,
                              CtxPixelFormat pixel_format);
Ctx *ctx_new_for_journal     (void *data, size_t length);
void ctx_free                (Ctx *ctx);

/* creates a new context that is 1000 units wide, this
 * context gets mapped to the width of the terminal
 * rendering the output.
 */
void ctx_new_stdout    (void);

/* blits the contents of a bare context
 */
void ctx_blit          (Ctx *ctx,
                        void *data, int x, int y,
                        int width, int height, int stride,
                        CtxPixelFormat pixel_format);

/* clears and resets a context */
void ctx_clear          (Ctx *ctx);

void ctx_fill           (Ctx *ctx);
void ctx_stroke         (Ctx *ctx);
void ctx_new_path       (Ctx *ctx);
void ctx_save           (Ctx *ctx);
void ctx_restore        (Ctx *ctx);
void ctx_clip           (Ctx *ctx);
void ctx_rotate         (Ctx *ctx, float x);
void ctx_set_line_width (Ctx *ctx, float x);

#define CTX_LINE_WIDTH_HAIRLINE     -1000.0
#define CTX_LINE_WIDTH_ALIASED      -1.0

#define CTX_LINE_WIDTH_FAST         -1.0  /* aliased 1px wide line,
                                             could be replaced with fast
                                             1px wide dab based stroker*/

void ctx_set_font_size  (Ctx *ctx, float x);
void ctx_set_font       (Ctx *ctx, const char *font);
void ctx_scale          (Ctx *ctx, float x, float y);
void ctx_translate      (Ctx *ctx, float x, float y);
void ctx_line_to        (Ctx *ctx, float x, float y);
void ctx_move_to        (Ctx *ctx, float x, float y);
void ctx_curve_to       (Ctx *ctx, float cx0, float cy0,
                         float cx1, float cy1,
                         float x, float y);
void ctx_quad_to        (Ctx *ctx, float cx, float cy,
                         float x, float y);
void ctx_rectangle      (Ctx *ctx,
                         float x0, float y0,
                         float w, float h);
void ctx_rel_line_to    (Ctx *ctx, float x, float y);
void ctx_rel_move_to    (Ctx *ctx, float x, float y);
void ctx_rel_curve_to   (Ctx *ctx,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2);
void ctx_rel_quad_to    (Ctx *ctx, float cx, float cy,
                         float x, float y);
void ctx_close_path     (Ctx *ctx);

void ctx_set_rgba_u8    (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void ctx_set_rgba       (Ctx *ctx, float   r, float   g, float   b, float   a);
void ctx_set_rgb        (Ctx *ctx, float   r, float   g, float   b);
void ctx_set_gray       (Ctx *ctx, float   gray);

void ctx_current_point  (Ctx *ctx, float *x, float *y);
void ctx_arc            (Ctx  *ctx,
                         float x, float y,
                         float radius,
                         float angle1, float angle2,
                         int   direction);
void ctx_arc_to         (Ctx *ctx, float x1, float y1,
                                   float x2, float y2, float radius);
void ctx_set_global_alpha   (Ctx *ctx, float global_alpha);

//void ctx_set_gradient_no     (Ctx *ctx, int no);
void ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1);
void ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0,
                                        float x1, float y1, float r1);
void ctx_gradient_clear_stops (Ctx *ctx);
void ctx_gradient_add_stop    (Ctx *ctx, float pos, float r, float g, float b, float a);

void ctx_gradient_add_stop_u8 (Ctx *ctx, float pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void ctx_image_path (Ctx *ctx, const char *path, float x, float y);
// XXX - this should decode png/jpg/gif header, and make width/height
//       available to drawing code

typedef enum
{
  CTX_FILL_RULE_EVEN_ODD,
  CTX_FILL_RULE_WINDING
} CtxFillRule;

typedef enum
{
  CTX_COMPOSITE_SOURCE_OVER
} CtxCompositing;

typedef enum
{
  CTX_BLEND_NORMAL
} CtxBlend;

typedef enum
{
  CTX_CAP_NONE,
  CTX_CAP_ROUND,
  CTX_CAP_SQUARE
} CtxLineCap;

typedef enum
{
  CTX_JOIN_BEVEL,
  CTX_JOIN_ROUND,
  CTX_JOIN_MITER
} CtxLineJoin;

void ctx_set_fill_rule (Ctx *ctx, CtxFillRule fill_rule);

void ctx_set_line_cap (Ctx *ctx, CtxLineCap cap);

int ctx_set_journal (Ctx *ctx, void *data, int length);


/* these are only needed for clients renderin text, as all text gets
 * converted to paths.
 */

void  ctx_text         (Ctx        *ctx,
                        const char *string);
void  ctx_text_stroke  (Ctx        *ctx,
                        const char *string);
/* return the width of provided string if it had been rendered */
float ctx_text_width   (Ctx        *ctx,
                        const char *string);


int   ctx_load_font_ttf (Ctx *ctx, const char *name, const uint8_t *ttf_contents);
int   ctx_load_font_ttf_file (Ctx *ctx, const char *name, const char *path);


typedef enum
{
  CTX_CLIP            = '#',
  CTX_NEW_EDGE        = '0',
  CTX_EDGE            = '|',
  CTX_EDGE_FLIPPED    = '`',
  CTX_REPEAT_HISTORY  = 'h', //

  CTX_CLEAR           = '\\',

  CTX_BLIT_RECT       = '%',
  CTX_NOP             = ' ',

  CTX_FILL_RULE       = 'f',
  CTX_GLOBAL_ALPHA    = 'O',

  CTX_SET_RGBA        = 'r', // u8
  CTX_LOAD_IMAGE      = '6',
  CTX_LINEAR_GRADIENT = '1',
  CTX_RADIAL_GRADIENT = '2',
  CTX_GRADIENT_NO     = '3',
  CTX_GRADIENT_CLEAR  = '4',
  CTX_GRADIENT_STOP   = '5',

  CTX_LINE_WIDTH      = 'w',
  CTX_LINE_CAP        = 'P',
  CTX_LINE_JOIN       = 'J',

  CTX_FONT_SIZE       = 'Z',


  CTX_STATE           = 'G', // graphics state follows in CTX_DATA

  CTX_CONT            = ';',
  CTX_DATA            = 'd', // size,  size-in-entries
  CTX_DATA_REV        = 'D', // reverse traversal data marker

  CTX_NEW_PATH        = 'p',
  CTX_CLOSE_PATH      = 'z',
  CTX_RECTANGLE       = '[',
  CTX_MOVE_TO         = 'M', // float x, y
  CTX_LINE_TO         = 'L', // float x, y
  CTX_CURVE_TO        = 'C', // float x, y, followed by two ; with rest of coords
  CTX_QUAD_TO         = 'Q',
  CTX_REL_MOVE_TO     = 'm', // float x, y
  CTX_REL_LINE_TO     = 'l', // float x, y
  CTX_REL_CURVE_TO    = 'c', // float x, y, followed by two ; with rest of coords
  CTX_REL_QUAD_TO     = 'q',
  CTX_ARC             = 'A',

  CTX_IDENTITY        = 'i',
  CTX_TRANSLATE       = 'T', // float, float
  CTX_ROTATE          = 'R', // float
  CTX_SCALE           = 'S', // float, float

  CTX_SAVE            = '(',
  CTX_RESTORE         = ')',

  CTX_FILL            = 'F',
  CTX_STROKE          = 's',
  CTX_PAINT           = '+',

  CTX_KERNING_PAIR              = 'K',
  CTX_DEFINE_GLYPH              = '@',
  CTX_GLYPH                     = 'g', // unichar, x, y
  CTX_TEXT                      = 't', // x, y - followed by "" in CTX_DATA

  /* optimizations that reduce the number of entries used,
   * not visible outside the draw-stream compression
   */
#if CTX_BITPACK
  CTX_REL_LINE_TO_X4            = '_',
  CTX_REL_LINE_TO_REL_CURVE_TO  = '~',
  CTX_REL_CURVE_TO_REL_LINE_TO  = '&',
  CTX_REL_CURVE_TO_REL_MOVE_TO  = '?',
  CTX_REL_LINE_TO_X2            = '"',
  CTX_MOVE_TO_REL_LINE_TO       = '/',
  CTX_REL_LINE_TO_REL_MOVE_TO   = '^',
  CTX_FILL_MOVE_TO              = 'e',
  CTX_REL_QUAD_TO_REL_QUAD_TO   = 'U',
  CTX_REL_QUAD_TO_S16           = 'V',
#endif

  CTX_FLUSH                     = 0,
  CTX_EXIT                      = 'X',
} CtxCode;

#ifdef __cplusplus
  }
#endif
#endif

#ifdef CTX_IMPLEMENTATION

#define CTX_MAX(a,b) ((a)>(b)?(a):(b))
#define CTX_MIN(a,b) ((a)<(b)?(a):(b))
#define CTX_CLAMP(val,min,max) ((val)<(min)?(min):(val)>(max)?(max):(val))

#define ctx_pow2(a) ((a)*(a))

#if CTX_MATH

static inline float
ctx_floorf (float x)
{
  return ((int) x);
}

static inline float
ctx_roundf (float x)
{
  return ((int) x) + 0.5f;
}

static inline float
ctx_fabsf (float x)
{
  return (x < 0.0f) ? -x : x;
}

static inline float
ctx_invsqrtf(float x)
{
   void *foo = &x;
   float xhalf = 0.5f * x;
   int i=*(int*)foo;
   void *bar = &i;
   i = 0x5f3759df - (i >> 1);
   x = *(float*)bar;
   x *= (1.5f - xhalf * x * x);
   x *= (1.5f - xhalf * x * x); //repeating Newton-Raphson step for higher precision
   return x;
}

static inline float
ctx_sinf (float x) {
  /* source : http://mooooo.ooo/chebyshev-sine-approximation/ */
  while (x < -CTX_PI)
    x += CTX_PI * 2;
  while (x > CTX_PI)
    x -= CTX_PI * 2;
  float coeffs[]={
        -0.10132118f,           // x
         0.0066208798f,         // x^3
        -0.00017350505f,        // x^5
         0.0000025222919f,      // x^7
        -0.000000023317787f,    // x^9
         0.00000000013291342f}; // x^11
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

static inline float ctx_fmodf(float a, float b)
{
  float val = a / b;
  return val - ((int)val);
}

static float ctx_atan2f (float y, float x)
{
  float atan, z;
  if ( x == 0.0f )
  {
    if ( y > 0.0f )
      return CTX_PI/2;
    if ( y == 0.0f )
      return 0.0f;
    return -CTX_PI/2;
  }
  z = y/x;
  if ( ctx_fabsf( z ) < 1.0f )
  {
    atan = z/(1.0f + 0.28f*z*z);
    if ( x < 0.0f )
    {
      if ( y < 0.0f )
        return atan - CTX_PI;
      return atan + CTX_PI;
    }
  }
  else
  {
    atan = CTX_PI/2 - z/(z*z + 0.28f);
    if ( y < 0.0f ) return atan - CTX_PI;
  }
  return atan;
}

static inline float ctx_atanf (float x)
{
  return ctx_atan2f (x, 1.0f);
  //
  //return CTX_PI/4 * x - x * (ctx_fabsf(x) - 1.0f) * (0.2447f + 0.0663f * ctx_fabsf(x));
}

#define roundf(x)   ctx_roundf(x)
#define fmodf(a)    ctx_fmodf(a)
#define sqrtf(a)    (1.0f/ctx_invsqrtf(a))
#define atanf(a)    ctx_atanf(a)

#define asinf(x)    ctx_atanf((x)/(ctx_sqrtf(1.0f-ctx_pow2(x))))
#define acosf(x)    ctx_atanf((sqrtf(1.0f-ctx_pow2(x))/(x)))
#define atan2f(a,b) ctx_atan2f((a), (b))
#define sinf(a)     ctx_sinf(a)
#define cosf(a)     ctx_sinf((a) + CTX_PI/2)
#define tanf(a)     (cosf(a)/sinf(a))

#define hypotf(a,b) sqrtf(ctx_pow2(a)+ctx_pow2(b))


#else

#include <math.h>

#endif

static inline float ctx_fast_hypotf (float x, float y)
{
  if (x<0) x = -x;
  if (y<0) y = -y;

  if (x < y)
    return 0.96f * y + 0.4f * x;
  else
    return 0.96f * x + 0.4f * y;
}


typedef struct _CtxEntry CtxEntry;

struct __attribute__ ((packed)) _CtxEntry
{
  uint8_t code;
  union {
    float    f[2];
    uint8_t  u8[8];
    int8_t   s8[8];
    uint16_t u16[4];
    int16_t  s16[4];
    uint32_t u32[2];
    int32_t  s32[2];
  } data;
};

#define ctx_arg_float(no) entry[no/2].data.f[no%2]
#define ctx_arg_u32(no)   entry[no/2].data.u32[no%2]
#define ctx_arg_s32(no)   entry[no/2].data.s32[no%2]
#define ctx_arg_u16(no)   entry[no/4].data.u32[no%4]
#define ctx_arg_s16(no)   entry[no/4].data.s32[no%4]
#define ctx_arg_u8(no)    entry[no/8].data.u8[no%8]
#define ctx_arg_s8(no)    entry[no/8].data.s8[no%8]

#if CTX_EXTRAS

char *ctx_commands[]={
"#clip", "|edge", "!fill_edges", "%blit_rect", "rfg", "Ggstate", ";cont", "ddata", "Lline_to", "Mmove_to", "Ccurve_to", "lrel_line_to", "mrel_move_to", "crel_curve_to", "Ttranslate", "Rrotate", "Sscale", "(save", ")restore", "Ffill", "[rectangle", "sstroke", "hhistory", "ttext", "wlinewidth", "Zfontsize", "pnew_path", "zclose_path", "iidentity", "_rel_line_to_x4", "~rel_line_to_rel_curve_to", "&rel_curve_to_rel_line_to", "?rel_curve_rel_move_to", "\"rel_line_to_x2", "/move_to_rel_line_to", "^rel_line_to_rel_move_to", "`edge_flipped", "\\clear", "efill_move_to", " nop", "0new_edge", "Aarc", "Oglobal_alpha", "Qquad_to", "qrel_quad_to", "Urel_quad_to_rel_quad_to", "Vrel_quad_to_s16", "Kkerning", "Pline_cap", "Ffill_rule", "/linear_gradient", "Xexit", "6load_image", "+paint", NULL
};

#endif

typedef struct _CtxRenderer CtxRenderer;
typedef struct _CtxGState CtxGState;
typedef struct _CtxState CtxState;
typedef struct _CtxMatrix CtxMatrix;
struct _CtxMatrix
{
  float m[3][2]; // use 3x3 matrix or fixed point instead?
};

typedef struct _CtxSource CtxSource;

typedef struct _CtxGradientStop CtxGradientStop;

struct _CtxGradientStop
{
  float   pos;      // use integer instead?
  uint8_t rgba[4];
};

enum {
  CTX_SOURCE_COLOR = 0,
  CTX_SOURCE_IMAGE,
  CTX_SOURCE_LINEAR_GRADIENT,
  CTX_SOURCE_RADIAL_GRADIENT,
};

typedef struct _CtxPixelFormatInfo CtxPixelFormatInfo;

typedef struct _CtxBuffer CtxBuffer;

struct _CtxBuffer
{
  void               *data;
  int                 width;
  int                 height;
  int                 stride;
  CtxPixelFormatInfo *format;
  int                 free_buf;
};

void ctx_user_to_device      (CtxState *state, float *x, float *y);

typedef struct _CtxGradient CtxGradient;
struct _CtxGradient
{
  CtxGradientStop stops[16];
  int n_stops;
};

struct _CtxSource
{
  CtxMatrix  transform;
  uint8_t    global_alpha;
  uint8_t    gradient_no;
  int type;
  union {
    struct {
      uint8_t rgba[4];
    } color;
    struct {
      uint8_t rgba[4]; // shares data with set color
      float x0;
      float y0;
      CtxBuffer *buffer;
    } image;
    struct {
      float x0;
      float y0;
      float x1;
      float y1;
      float dx;
      float dy;
      float start;
      float end;
      float length;
    } linear_gradient;
    struct {
      float x0;
      float y0;
      float x1;
      float y1;
      float r0;
      float r1;
    } radial_gradient;
  };
};

struct _CtxGState {
  CtxMatrix    transform;
  CtxSource    source;
  float        line_width;
  float        font_size;
  float        line_spacing;

  /* bitfield-pack all the small state-parts */
  //CtxCompositing compositing_mode:4;
  //CtxBlend             blend_mode:4;
  CtxLineCap             line_cap:2;
  CtxLineJoin           line_join:2;
  CtxFillRule           fill_rule:1;
  unsigned int               font:4;
  unsigned int               bold:1;
  unsigned int             italic:1;
};

typedef struct _CtxJournal CtxJournal;

typedef enum {
  CTX_SOURCE_OVER = 1,
  CTX_DESTINATION_OVER,
  CTX_COPY,
} CtxCompositingMode;

typedef enum {
  CTX_TRANSFORMATION_NONE         = 0,
  CTX_TRANSFORMATION_SCREEN_SPACE = 1,
  CTX_TRANSFORMATION_RELATIVE     = 2,
#if CTX_BITPACK
  CTX_TRANSFORMATION_BITPACK      = 4,
#endif
  CTX_TRANSFORMATION_REFPACK      = 8,
} CtxTransformation;

#define CTX_JOURNAL_DOESNT_OWN_ENTRIES   64

struct _CtxJournal
{
  /* should have a list of iterator state initializer. pos, depth and
     history_pos init info for every 1024 entries, will be most needed
     with wire-protocol and reuse of data from preceding frame.
   */
  CtxEntry *entries;     /* we need to use realloc */
  int       size;
  int       count;
  int       flags; // BITPACK and REFPACK - to be used on resize
  int       bitpack_pos;
};

struct _CtxState {
  CtxGState gstate;
  CtxGState gstate_stack[CTX_MAX_STATES];
  int       gstate_no;
  float     x;
  float     y;
  float     path_start_x;
  float     path_start_y;
  int       has_moved;
  CtxGradient gradient[CTX_MAX_GRADIENTS];
};

struct _Ctx {
  CtxJournal        journal;
  CtxState          state;
  int               transformation;
#if CTX_RASTERIZER
  CtxRenderer      *renderer;
#endif
};

typedef struct _CtxFont CtxFont;

struct _CtxFont
{
  const char *name;
  int type; // 0 ctx    1 stb    2 monobitmap
  union {
    struct { CtxEntry *data; int length; int first_kern;
             /* we've got ~110 bytes to fill to cover as
                much data as stbtt_fontinfo */
             int16_t glyph_pos[26]; // for a..z
           } ctx;
#if CTX_FONT_ENGINE_STB
    struct { stbtt_fontinfo ttf_info; } stb;
#endif
    struct { int start; int end; int gw; int gh; const uint8_t *data;} monobitmap;
  };
};


static CtxFont ctx_fonts[CTX_MAX_FONTS];
static int     ctx_font_count = 0;



void ctx_process (Ctx *ctx, CtxEntry *entry);

static inline void
ctx_matrix_identity (CtxMatrix *matrix)
{
  matrix->m[0][0] = 1.0f; matrix->m[0][1] = 0.0f;
  matrix->m[1][0] = 0.0f; matrix->m[1][1] = 1.0f;
  matrix->m[2][0] = 0.0f; matrix->m[2][1] = 0.0f;
}

static inline void
ctx_matrix_multiply (CtxMatrix       *result,
                     const CtxMatrix *t,
                     const CtxMatrix *s)
{
  CtxMatrix r;
  r.m[0][0] = t->m[0][0] * s->m[0][0] + t->m[0][1] * s->m[1][0];
  r.m[0][1] = t->m[0][0] * s->m[0][1] + t->m[0][1] * s->m[1][1];
  r.m[1][0] = t->m[1][0] * s->m[0][0] + t->m[1][1] * s->m[1][0];
  r.m[1][1] = t->m[1][0] * s->m[0][1] + t->m[1][1] * s->m[1][1];
  r.m[2][0] = t->m[2][0] * s->m[0][0] + t->m[2][1] * s->m[1][0] + s->m[2][0];
  r.m[2][1] = t->m[2][0] * s->m[0][1] + t->m[2][1] * s->m[1][1] + s->m[2][1];
  *result = r;
}

static inline void
ctx_matrix_translate (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = 1.0f; transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f; transform.m[1][1] = 1.0f;
  transform.m[2][0] = x;    transform.m[2][1] = y;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static inline void
ctx_matrix_scale (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = x;    transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f; transform.m[1][1] = y;
  transform.m[2][0] = 0.0f; transform.m[2][1] = 0.0f;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static inline void
ctx_matrix_rotate (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_sin = sinf (angle);
  float val_cos = cosf (angle);
  transform.m[0][0] =  val_cos; transform.m[0][1] = val_sin;
  transform.m[1][0] = -val_sin; transform.m[1][1] = val_cos;
  transform.m[2][0] =     0.0f; transform.m[2][1] = 0.0f;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static inline void
ctx_matrix_skew_x (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = tanf (angle);
  transform.m[0][0] =    1.0f; transform.m[0][1] = 0.0f;
  transform.m[1][0] = val_tan; transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f; transform.m[2][1] = 0.0f;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static inline void
ctx_matrix_skew_y (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = tanf (angle);
  transform.m[0][0] =    1.0f; transform.m[0][1] = val_tan;
  transform.m[1][0] =    0.0f; transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f; transform.m[2][1] = 0.0f;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static inline int
ctx_conts_for_entry (CtxEntry *entry)
{
  switch (entry->code)
  {
    case CTX_DATA:
      return entry->data.u32[1];

    case CTX_LINEAR_GRADIENT:
      return 1;

    case CTX_RADIAL_GRADIENT:
    case CTX_ARC:
    case CTX_CURVE_TO:
    case CTX_REL_CURVE_TO:
      return 2;
    case CTX_RECTANGLE:
    case CTX_REL_QUAD_TO:
    case CTX_QUAD_TO:
      return 1;
    default:
      return 0;
  }
}

typedef struct CtxIterator
{
  int         pos;
  int         in_history;
  int         history_pos;
  int         history[2]; // XXX: the offset to the history entry would be better
  CtxJournal *journal;
  int         end_pos;
  int         flags;

  int         bitpack_pos;
  int         bitpack_length; // if non 0 bitpack is active
  CtxEntry    bitpack_command[6];
} CtxIterator;


/* the iterator - should decode bitpacked data as well -
 * making the renderers simpler, possibly do unpacking
 * all the way to absolute coordinates.. unless mixed
 * relative/not are wanted.
 */

enum _CtxIteratorFlag
{
  CTX_ITERATOR_FLAT           = 0,
  CTX_ITERATOR_EXPAND_REFPACK = 1,
  CTX_ITERATOR_EXPAND_BITPACK = 2,
  CTX_ITERATOR_DEFAULTS       = CTX_ITERATOR_EXPAND_REFPACK |
                                CTX_ITERATOR_EXPAND_BITPACK
};
typedef enum _CtxIteratorFlag CtxIteratorFlag;

static void
ctx_iterator_init (CtxIterator *iterator,
                   CtxJournal  *journal,
                   int          start_pos,
                   int          flags)
{
  iterator->journal        = journal;
  iterator->flags          = flags;
  iterator->bitpack_pos    = 0;
  iterator->bitpack_length = 0;
  iterator->history_pos    = 0;
  iterator->history[0]     = 0;
  iterator->history[1]     = 0;
  iterator->pos            = start_pos;
  iterator->end_pos        = journal->count;
  iterator->in_history     = -1; // -1 is a marker used for first run
  memset (iterator->bitpack_command, 0, sizeof (iterator->bitpack_command));
}

static CtxEntry *_ctx_iterator_next (CtxIterator *iterator)
{
  int expand_refpack = iterator->flags & CTX_ITERATOR_EXPAND_REFPACK;

  int ret = iterator->pos;
  CtxEntry *entry = &iterator->journal->entries[ret];
  if (iterator->pos >= iterator->end_pos)
    return NULL;
  if (expand_refpack == 0)
  {
    if (iterator->in_history == 0)
      iterator->pos += (ctx_conts_for_entry (entry) + 1);
    iterator->in_history = 0;
    if (iterator->pos >= iterator->end_pos)
      return NULL;
    return &iterator->journal->entries[iterator->pos];
  }

  if (iterator->in_history > 0)
  {
    if (iterator->history_pos < iterator->history[1])
    {
      int ret = iterator->history[0] + iterator->history_pos;
      CtxEntry *hentry = &iterator->journal->entries[ret];
      iterator->history_pos += (ctx_conts_for_entry (hentry) + 1);

      if (iterator->history_pos >= iterator->history[1])
      {
        iterator->in_history = 0;
        iterator->pos += (ctx_conts_for_entry (entry) + 1);
      }
      return &iterator->journal->entries[ret];
    }
  }

  if (entry->code == CTX_REPEAT_HISTORY)
  {
    iterator->in_history = 1;
    iterator->history[0] = entry->data.u32[0];
    iterator->history[1] = entry->data.u32[1];
    CtxEntry *hentry = &iterator->journal->entries[iterator->history[0]];
    iterator->history_pos = 0;
    iterator->history_pos += (ctx_conts_for_entry (hentry) + 1);
    return hentry;
  }
  else
  {
    iterator->pos += (ctx_conts_for_entry (entry) + 1);
    iterator->in_history = 0;
  }

  if (ret >= iterator->end_pos)
    return NULL;
  return &iterator->journal->entries[ret];
}

#if CTX_BITPACK
static void
ctx_iterator_expand_s8_args (CtxIterator *iterator, CtxEntry *entry)
{
  int no = 0;
  for (int cno = 0; cno < 4; cno++)
    for (int d = 0; d < 2; d++, no++)
      iterator->bitpack_command[cno].data.f[d] =
         entry->data.s8[no] * 1.0f / CTX_SUBDIV;

  iterator->bitpack_command[0].code = CTX_CONT;
  iterator->bitpack_command[1].code = CTX_CONT;
  iterator->bitpack_command[2].code = CTX_CONT;
  iterator->bitpack_command[3].code = CTX_CONT;
  iterator->bitpack_length = 4;
  iterator->bitpack_pos = 0;
}

static void
ctx_iterator_expand_s16_args (CtxIterator *iterator, CtxEntry *entry)
{
  int no = 0;
  for (int cno = 0; cno < 2; cno++)
    for (int d = 0; d < 2; d++, no++)
      iterator->bitpack_command[cno].data.f[d] = entry->data.s16[no] * 1.0f /
         CTX_SUBDIV;
  iterator->bitpack_command[0].code = CTX_CONT;
  iterator->bitpack_command[1].code = CTX_CONT;
  iterator->bitpack_length = 2;
  iterator->bitpack_pos = 0;
}
#endif

static CtxEntry *
ctx_iterator_next (CtxIterator *iterator)
{
  CtxEntry *ret;
#if CTX_BITPACK
  int expand_bitpack = iterator->flags & CTX_ITERATOR_EXPAND_BITPACK;
  again:
  if (iterator->bitpack_length)
  {
    ret = &iterator->bitpack_command[iterator->bitpack_pos];
    iterator->bitpack_pos += (ctx_conts_for_entry (ret) + 1);
    if (iterator->bitpack_pos >= iterator->bitpack_length)
      {
        iterator->bitpack_length = 0;
      }

    return ret;
  }
#endif
  ret = _ctx_iterator_next (iterator);

#if CTX_BITPACK
  if (ret && expand_bitpack)
  switch (ret->code)
  {
    case CTX_REL_CURVE_TO_REL_LINE_TO:
      ctx_iterator_expand_s8_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_CURVE_TO;
      iterator->bitpack_command[1].code = CTX_CONT;
      iterator->bitpack_command[2].code = CTX_CONT;
      iterator->bitpack_command[3].code = CTX_REL_LINE_TO;

      // 0.0 here is a common optimization - so check for it
      if (ret->data.s8[6]== 0 && ret->data.s8[7] == 0)
        iterator->bitpack_length = 3;
      goto again;

    case CTX_REL_LINE_TO_REL_CURVE_TO:
      ctx_iterator_expand_s8_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
      iterator->bitpack_command[1].code = CTX_REL_CURVE_TO;
      goto again;

    case CTX_REL_CURVE_TO_REL_MOVE_TO:
      ctx_iterator_expand_s8_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_CURVE_TO;
      iterator->bitpack_command[3].code = CTX_REL_MOVE_TO;
      goto again;

    case CTX_REL_LINE_TO_X4:
      ctx_iterator_expand_s8_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
      iterator->bitpack_command[1].code = CTX_REL_LINE_TO;
      iterator->bitpack_command[2].code = CTX_REL_LINE_TO;
      iterator->bitpack_command[3].code = CTX_REL_LINE_TO;
      goto again;

    case CTX_REL_QUAD_TO_S16:
      ctx_iterator_expand_s16_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_QUAD_TO;
      goto again;

    case CTX_REL_QUAD_TO_REL_QUAD_TO:
      ctx_iterator_expand_s8_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_QUAD_TO;
      iterator->bitpack_command[2].code = CTX_REL_QUAD_TO;
      goto again;

    case CTX_REL_LINE_TO_X2:
      ctx_iterator_expand_s16_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
      iterator->bitpack_command[1].code = CTX_REL_LINE_TO;
      goto again;

    case CTX_REL_LINE_TO_REL_MOVE_TO:
      ctx_iterator_expand_s16_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
      iterator->bitpack_command[1].code = CTX_REL_MOVE_TO;
      goto again;

    case CTX_MOVE_TO_REL_LINE_TO:
      ctx_iterator_expand_s16_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_MOVE_TO;
      iterator->bitpack_command[1].code = CTX_REL_MOVE_TO;
      goto again;

    case CTX_FILL_MOVE_TO:
      iterator->bitpack_command[1] = *ret;
      iterator->bitpack_command[0].code = CTX_FILL;
      iterator->bitpack_command[1].code = CTX_MOVE_TO;
      iterator->bitpack_pos = 0;
      iterator->bitpack_length = 2;
      goto again;

    case CTX_LINEAR_GRADIENT:
    case CTX_QUAD_TO:
    case CTX_REL_QUAD_TO:
      iterator->bitpack_command[0] = ret[0];
      iterator->bitpack_command[1] = ret[1];
      iterator->bitpack_pos = 0;
      iterator->bitpack_length = 2;
      goto again;

    case CTX_ARC:
    case CTX_RADIAL_GRADIENT:
    case CTX_CURVE_TO:
    case CTX_REL_CURVE_TO:
      iterator->bitpack_command[0] = ret[0];
      iterator->bitpack_command[1] = ret[1];
      iterator->bitpack_command[2] = ret[2];
      iterator->bitpack_pos = 0;
      iterator->bitpack_length = 3;
      goto again;

    case CTX_LOAD_IMAGE:
      iterator->bitpack_command[0] = ret[0];
      iterator->bitpack_command[1] = ret[1];
      iterator->bitpack_command[2] = ret[2];
      iterator->bitpack_pos = 0;
      iterator->bitpack_length = 3;
      goto again;


    default:
      iterator->bitpack_command[0] = *ret;
      iterator->bitpack_pos = 0;
      iterator->bitpack_length = 1;
      goto again;
  }
#endif

  return ret;
}

static void
ctx_gstate_push (CtxState *state)
{
  if (state->gstate_no +1 >= CTX_MAX_STATES)
    return;
  state->gstate_stack[state->gstate_no] = state->gstate;
  state->gstate_no++;
}

static void
ctx_gstate_pop (CtxState *state)
{
  if (state->gstate_no <= 0)
    return;
  state->gstate = state->gstate_stack[state->gstate_no-1];
  state->gstate_no--;
}

static inline void ctx_interpret_style (CtxState *state, CtxEntry *entry, void *data);
static inline void ctx_interpret_transforms (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_pos (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data);

static void ctx_journal_refpack (CtxJournal *journal);
static void
ctx_journal_resize (CtxJournal *journal, int desired_size)
{
  int new_size = desired_size;

  ctx_journal_refpack (journal);
  if (new_size < journal->size)
    return;

  if (new_size < 100)
    new_size = 100;
  if (new_size < journal->count)
    new_size = journal->count + 4;

  if (new_size >= CTX_MAX_JOURNAL_SIZE)
    new_size = CTX_MAX_JOURNAL_SIZE;
  if (journal->size == CTX_MAX_JOURNAL_SIZE)
    return;
  ctx_log ("growing journal to %i\n", new_size);
  if (journal->size)
    journal->entries = (CtxEntry*)realloc (journal->entries, sizeof (CtxEntry) * new_size);
  else
    journal->entries = (CtxEntry*)malloc (sizeof (CtxEntry) * new_size);
  journal->size = new_size;
}

int
ctx_journal_add (CtxJournal *journal, CtxCode code)
{
  int ret = journal->count;
  if (ret + 1 >= journal->size)
  {
    ctx_journal_resize (journal, journal->size + 128 + 128);
    ret = journal->count;
  }
  if (ret >= journal->size)
    return -1;
  journal->entries[ret].code = code;
  journal->count++;
  return ret;
}

int
ctx_journal_add_single (CtxJournal *journal, CtxEntry *entry)
{
  int ret = journal->count;
  if (ret + 1 >= journal->size)
  {
    ctx_journal_resize (journal, journal->size * 1.2 + 128);
    ret = journal->count;
  }
  journal->entries[journal->count] = *entry;
  ret = journal->count;
  journal->count++;
  return ret;
}

int
ctx_journal_add_ (CtxJournal *journal, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int ret = 0;
  for (int i = 0; i < length; i ++)
  {
    ret = ctx_journal_add_single (journal, &entry[i]);
  }
  return ret;
}

int ctx_set_journal (Ctx *ctx, void *data, int length)
{
  CtxEntry *entries = (CtxEntry*)data;
  if (length % sizeof (CtxEntry))
  {
    //ctx_log("err\n");
    return -1;
  }
  ctx->journal.count = 0;
  for (int i = 0; i < length / sizeof(CtxEntry); i++)
  {
    ctx_journal_add_single (&ctx->journal, &entries[i]);
  }

  return 0;
}

int
ctx_add_data (Ctx *ctx, void *data, int length)
{
  if (length % sizeof (CtxEntry))
  {
    //ctx_log("err\n");
    return -1;
  }
  /* some more input verification might be in order.. like
   * verify that it is well-formed up to length?
   *
   * also - it would be very useful to stop processing
   * upon flush - and do journal resizing.
   */
  return ctx_journal_add_ (&ctx->journal, (CtxEntry*)data);
}

int ctx_journal_add_u32 (CtxJournal *journal, CtxCode code, uint32_t u32[2])
{
  int no = ctx_journal_add (journal, code);
  if (no < 0)
    return -1;
  memcpy (journal->entries[no].data.u32, &u32[0], sizeof(uint32_t) * 2);
  return no;
}

int ctx_journal_add_data (CtxJournal *journal, const void *data, int length)
{
  int ret = ctx_journal_add (journal, CTX_DATA);
  if (!data) return -1;
  int length_in_blocks;
  if (length <= 0) length = strlen ((char*)data) + 1;

  length_in_blocks = length / sizeof (CtxEntry);
  length_in_blocks += (length % sizeof (CtxEntry))?1:0;

  if (journal->count + length_in_blocks + 4 > journal->size)
    ctx_journal_resize (journal, journal->count * 1.2 + length_in_blocks + 32);

  if (journal->count >= journal->size)
    return -1;
  journal->count += length_in_blocks;

  journal->entries[ret].data.u32[0] = length;
  journal->entries[ret].data.u32[1] = length_in_blocks;

  memcpy (&journal->entries[ret+1], data, length);
  {int reverse = ctx_journal_add (journal, CTX_DATA_REV);
   journal->entries[reverse].data.u32[0] = length;
   journal->entries[reverse].data.u32[1] = length_in_blocks;
   /* this reverse marker is needed to be able to do efficient
      front to back traversal
    */
  }

  return ret;
}

static inline CtxEntry
ctx_void (CtxCode code)
{
  CtxEntry command;
  command.code = code;
  command.data.u32[0] = 0;
  command.data.u32[1] = 0;
  return command;
}

static inline CtxEntry
ctx_f (CtxCode code, float x, float y)
{
  CtxEntry command = ctx_void (code);
  command.data.f[0] = x;
  command.data.f[1] = y;
  return command;
}

static inline CtxEntry
ctx_u32 (CtxCode code, uint32_t x, uint32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.u32[0] = x;
  command.data.u32[1] = y;
  return command;
}

static inline CtxEntry
ctx_s32 (CtxCode code, int32_t x, int32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.s32[0] = x;
  command.data.s32[1] = y;
  return command;
}

static inline CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1)
{
  CtxEntry command = ctx_void (code);
  command.data.s16[0] = x0;
  command.data.s16[1] = y0;
  command.data.s16[2] = x1;
  command.data.s16[3] = y1;
  return command;
}

static inline CtxEntry
ctx_u8 (CtxCode code,
   uint8_t a, uint8_t b, uint8_t c, uint8_t d,
   uint8_t e, uint8_t f, uint8_t g, uint8_t h)
{
  CtxEntry command = ctx_void (code);
  command.data.u8[0] = a; command.data.u8[1] = b;
  command.data.u8[2] = c; command.data.u8[3] = d;
  command.data.u8[4] = e; command.data.u8[5] = f;
  command.data.u8[6] = g; command.data.u8[7] = h;
  return command;
}

void
ctx_close_path (Ctx *ctx)
{
  if  (ctx->state.path_start_x != ctx->state.x ||
       ctx->state.path_start_y != ctx->state.y)
    ctx_line_to (ctx, ctx->state.path_start_x, ctx->state.path_start_y);
}

static inline CtxEntry ctx_void (CtxCode code);

#if 0
void
ctx_cmd (Ctx *ctx, CtxCode code)
{
  CtxEntry command = ctx_void (code);
  ctx_process (ctx, &command);
}

void
ctx_cmd_f (Ctx *ctx, CtxCode code, float arg)
{
  CtxEntry command = ctx_f(code, arg, 0.0);
  ctx_process (ctx, &command);
}
#endif

void
ctx_cmd_ff (Ctx *ctx, CtxCode code, float arg1, float arg2)
{
  CtxEntry command = ctx_f(code, arg1, arg2);
  ctx_process (ctx, &command);
}

#define CTX_PROCESS_VOID(cmd) do {\
  CtxEntry command = ctx_void (cmd); \
  ctx_process (ctx, &command);}while(0) \

#define CTX_PROCESS_F1(cmd, x) do {\
  CtxEntry command = ctx_f(cmd, x, 0);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U32(cmd, x, y) do {\
  CtxEntry command = ctx_u32(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_F(cmd, x, y) do {\
  CtxEntry command = ctx_f(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

void
ctx_image_path (Ctx *ctx, const char *path, float x, float y)
{
  int pathlen = strlen (path);
  CtxEntry commands[1 + 2 + pathlen/8];
  memset (commands, 0, sizeof (commands));
  commands[0] = ctx_f(CTX_LOAD_IMAGE, x, y);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = pathlen;
  commands[1].data.u32[1] = pathlen/9+1;
  strcpy ((char*)&commands[2].data.u8[0], path);
  ctx_process (ctx, commands);
}

void ctx_paint(Ctx *ctx) {
  CTX_PROCESS_VOID(CTX_PAINT);
}

void ctx_fill (Ctx *ctx) {
  CTX_PROCESS_VOID(CTX_FILL);
}
void ctx_stroke (Ctx *ctx) {
  CTX_PROCESS_VOID(CTX_STROKE);
}

static void ctx_state_init (CtxState *state);
void ctx_clear (Ctx *ctx) {
  CTX_PROCESS_VOID(CTX_CLEAR);
#if CTX_RASTERIZER
  if (ctx->renderer == NULL)
#endif
  {
    ctx->journal.count = 0;
    ctx->journal.bitpack_pos = 0;
  }
  ctx_state_init (&ctx->state);
}

void ctx_new_path (Ctx *ctx)
{
  CTX_PROCESS_VOID(CTX_NEW_PATH);
}

void ctx_clip (Ctx *ctx)
{
  CTX_PROCESS_VOID(CTX_CLIP);
}

void ctx_save (Ctx *ctx)
{
  CTX_PROCESS_VOID(CTX_SAVE);
}
void ctx_restore (Ctx *ctx)
{
  CTX_PROCESS_VOID(CTX_RESTORE);
}

void ctx_set_line_width (Ctx *ctx, float x) {

  /* XXX : ugly hack to normalize the width dependent on the current
           transform, this does not really belong here */
  x = x * CTX_MAX(CTX_MAX(ctx->state.gstate.transform.m[0][0],
                          ctx->state.gstate.transform.m[0][1]),
                  CTX_MAX(ctx->state.gstate.transform.m[1][0],
                          ctx->state.gstate.transform.m[1][1]));

  CTX_PROCESS_F1(CTX_LINE_WIDTH, x);
}

void ctx_set_global_alpha (Ctx *ctx, float global_alpha)
{
  CTX_PROCESS_F1(CTX_GLOBAL_ALPHA, global_alpha);
}

void ctx_set_font_size (Ctx *ctx, float x) {
  CTX_PROCESS_F1(CTX_FONT_SIZE, x);
}

static int _ctx_resolve_font (const char *name)
{
  for (int i = 0; i < ctx_font_count; i ++)
  {
    if (!strcmp (ctx_fonts[i].name, name))
      return i;
  }
  for (int i = 0; i < ctx_font_count; i ++)
  {
    if (strstr (ctx_fonts[i].name, name))
      return i;
  }
  return -1;
}

static int ctx_resolve_font (const char *name)
{
  int ret = _ctx_resolve_font (name);
  if (ret >= 0)
    return ret;
  if (!strcmp (name, "regular"))
  {
    int ret = _ctx_resolve_font ("sans");
    if (ret >= 0) return ret;
    ret = _ctx_resolve_font ("serif");
    if (ret >= 0) return ret;
  }
  return 0;
}

void ctx_set_font (Ctx *ctx, const char *name)
{
  ctx->state.gstate.font = ctx_resolve_font (name);
}

void ctx_rotate (Ctx *ctx, float x){
  CTX_PROCESS_F1(CTX_ROTATE, x);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    ctx->journal.count--;
}

void ctx_scale (Ctx *ctx, float x, float y) {
  CTX_PROCESS_F (CTX_SCALE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    ctx->journal.count--;
}

void ctx_translate (Ctx *ctx, float x, float y) {
  CTX_PROCESS_F (CTX_TRANSLATE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    ctx->journal.count--;
}

void ctx_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    CTX_PROCESS_F (CTX_MOVE_TO, x, y);
  else
    CTX_PROCESS_F (CTX_LINE_TO, x, y);
}

void ctx_move_to (Ctx *ctx, float x, float y)
{
  CTX_PROCESS_F (CTX_MOVE_TO,x,y);
}

void ctx_curve_to (Ctx *ctx, float x0, float y0,
                   float x1, float y1,
                   float x2, float y2)
{
  CtxEntry command[3]={
     ctx_f (CTX_CURVE_TO, x0, y0),
     ctx_f (CTX_CONT,     x1, y1),
     ctx_f (CTX_CONT,     x2, y2)};

  ctx_process (ctx, command);
}

void ctx_rectangle (Ctx *ctx,
                    float x0, float y0,
                    float w, float h)
{
  ctx_move_to (ctx, x0, y0);
  ctx_rel_line_to (ctx, w, 0);
  ctx_rel_line_to (ctx, 0, h);
  ctx_rel_line_to (ctx, -w, 0);
  ctx_close_path (ctx);
}

static int ctx_is_digit (int ch)
{
  if (ch >= '0' && ch <='9')
    return 1;
  return 0;
}

static float
ctx_strtof (const char *str, char **endptr)
{
  /* not a full strtod replacement, but good enough for what we need,
     not relying on double math in libraries, and */
  float ret = 0.0f;
  int got_point = 0;
  float divisor = 1.0f;
  float sign = 1.0f;

  while (*str == ' ' || *str == '\t' || *str =='\n')
    str++;

  while (ctx_is_digit (*str) || *str == '.' || *str == '-' || *str == '+')
  {
    if (ctx_is_digit (*str))
    {
      if (!got_point)
      {
        ret *= 10.0f;
        ret += (*str - '0');
      }
      else
      {
         divisor *= 0.1f;
         ret += ((*str) - '0') * divisor;
      }
    }
    else if (*str == '.')
    {
      got_point = 1;

    }
    else if (*str == '-')
    {
      sign = -1.0f;
    }
    str++;
  }
  if (endptr)
    *endptr = (char *)str;
  return ret * sign;
}

void ctx_svg_path (Ctx *ctx, const char *str)
{
  char  command = 'm';
  const char *s;
  int numbers = 0;
  float number[12];

  if (!str)
    return;
  ctx_move_to (ctx, 0, 0);

  s = str;
again:
  numbers = 0;

  for (; *s; s++)
  {
    switch (*s)
    {
      case 'z':
      case 'Z':
        ctx_close_path (ctx);
        break;
      case 'm':
      case 'a':
      case 'M':
      case 'c':
      case 'C':
      case 'l':
      case 'L':
         command = *s;
         break;
      case '-':case '.':case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7': case '8': case '9':
      if (*s == '-')
        number[numbers] = ctx_strtof (s, (char**)&s);
      else
      {
        number[numbers] = ctx_strtof (s, (char**)&s);
        s--;
      }
      if (numbers < 11)
        numbers++;

      switch (command)
      {
        case 'a':
          /* fallthrough */
        case 'A':
          if (numbers == 7)
          {
            /// XXX: NYI
            s++;
            goto again;
          }
          /* fallthrough */
        case 'm':
          if (numbers == 2)
          {
            ctx_rel_move_to (ctx, number[0], number[1]);
            s++;
            command = 'l'; // the default after movetos
            goto again;
          }
          break;
        case 'l':
          if (numbers == 2)
          {
            ctx_rel_line_to (ctx, number[0], number[1]);
            s++;
            goto again;
          }
          break;
        case 'c':
          if (numbers == 6)
          {
            ctx_rel_curve_to (ctx, number[0], number[1],
                                   number[2], number[3],
                                   number[4], number[5]);
            s++;
            goto again;
          }
          break;
        case 'M':
          if (numbers == 2)
          {
            ctx_move_to (ctx, number[0], number[1]);
            s++;
            command = 'L'; // the default after movetos
            goto again;
          }
          break;
        case 'L':
          if (numbers == 2)
          {
            ctx_line_to (ctx, number[0], number[1]);
            s++;
            goto again;
          }
          break;
        case 'Q':
          if (numbers == 4)
          {
            ctx_quad_to (ctx, number[0], number[1], number[2], number[3]);
            s++;
            goto again;
          }
          break;
        case 'q':
          if (numbers == 4)
          {
            ctx_rel_quad_to (ctx, number[0], number[1], number[2], number[3]);
            s++;
            goto again;
          }
          break;
        case 'C':
          if (numbers == 6)
          {
            ctx_curve_to (ctx, number[0], number[1],
                                number[2], number[3],
                                number[4], number[5]);
            s++;
            goto again;
          }
          break;
        default:
          ctx_log ("uninterpreted svg path command _%c", *s);
          break;
      }
      break;
      default:
        break;
    }
  }
}



void ctx_rel_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    return;
  CTX_PROCESS_F (CTX_REL_LINE_TO,x,y);
}

void ctx_rel_move_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
  {
    CTX_PROCESS_F (CTX_MOVE_TO,x,y);
    return;
  }
  CTX_PROCESS_F (CTX_REL_MOVE_TO,x,y);
}

void ctx_set_line_cap (Ctx *ctx, CtxLineCap cap)
{
  CtxEntry command = ctx_u8 (CTX_LINE_CAP, cap, 0, 0, 0, 0, 0, 0, 0);
  ctx_process (ctx, &command);
}

void ctx_set_fill_rule (Ctx *ctx, CtxFillRule fill_rule)
{
  CtxEntry command = ctx_u8 (CTX_FILL_RULE, fill_rule, 0, 0, 0, 0, 0, 0, 0);
  ctx_process (ctx, &command);
}

void ctx_set_line_join (Ctx *ctx, CtxLineJoin join)
{
  CtxEntry command = ctx_u8 (CTX_LINE_JOIN, join, 0, 0, 0, 0, 0, 0, 0);
  ctx_process (ctx, &command);
}

void
ctx_rel_curve_to (Ctx *ctx,
                  float x0, float y0,
                  float x1, float y1,
                  float x2, float y2)
{
  if (!ctx->state.has_moved)
    return;

  CtxEntry command[3]={
     ctx_f (CTX_REL_CURVE_TO, x0, y0),
     ctx_f (CTX_CONT, x1, y1),
     ctx_f (CTX_CONT, x2, y2)};

  ctx_process (ctx, command);
}

void
ctx_rel_quad_to (Ctx *ctx,
                 float cx, float cy,
                 float x,  float y)
{
  if (!ctx->state.has_moved)
    return;

  CtxEntry command[2]={
     ctx_f (CTX_REL_QUAD_TO, cx, cy),
     ctx_f (CTX_CONT, x, y)};

  ctx_process (ctx, command);
}

void
ctx_quad_to (Ctx *ctx,
             float cx, float cy,
             float x,  float y)
{
  if (!ctx->state.has_moved)
    return;

  CtxEntry command[2]={
     ctx_f (CTX_QUAD_TO, cx, cy),
     ctx_f (CTX_CONT, x, y)};

  ctx_process (ctx, command);
}

void ctx_arc (Ctx  *ctx,
              float x0, float y0,
              float radius,
              float angle1, float angle2,
              int   direction)
{
  CtxEntry command[3]={
     ctx_f (CTX_ARC, x0, y0),
     ctx_f (CTX_CONT, radius, angle1),
     ctx_f (CTX_CONT, angle2, direction)};
  ctx_process (ctx, command);
}

static inline int ctx_coords_equal (float x1, float y1, float x2, float y2, float tol)
{
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
}

static inline float
ctx_point_seg_dist_sq(float x, float y,
                      float vx, float vy, float wx, float wy)
{
  float l2 = ctx_pow2(vx-wx) + ctx_pow2(vy-wy);
  if (l2 < 0.0001)
    return ctx_pow2(x-vx) + ctx_pow2(y-vy);

  float t = ((x - vx) * (wx - vx) + (y - vy) * (wy - vy)) / l2;
  t = CTX_MAX(0, CTX_MIN(1, t));
  float ix = vx + t * (wx - vx);
  float iy = vy + t * (wy - vy);

  return ctx_pow2(x-ix) + ctx_pow2(y-iy);
}

static inline void
ctx_normalize (float *x, float* y)
{
  float length = hypotf((*x), (*y));
  if (length > 1e-6f)
  {
    float r = 1.0f / length;
    *x *= r;
    *y *= r;
  }
}

void
ctx_arc_to (Ctx *ctx, float x1, float y1, float x2, float y2, float radius)
{
  /* from nanovg */
  float x0 = ctx->state.x;
  float y0 = ctx->state.y;
  float dx0,dy0, dx1,dy1, a, d, cx,cy, a0,a1;
  int dir;

  if (!ctx->state.has_moved)
    return;

  // Handle degenerate cases.
  if (ctx_coords_equal (x0,y0, x1,y1, 0.5f) ||
      ctx_coords_equal (x1,y1, x2,y2, 0.5f) ||
      ctx_point_seg_dist_sq (x1,y1, x0,y0, x2,y2) < 0.5 ||
      radius < 0.5) {
        ctx_line_to (ctx, x1,y1);
        return;
      }

  // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
  dx0 = x0-x1;
  dy0 = y0-y1;
  dx1 = x2-x1;
  dy1 = y2-y1;
  ctx_normalize(&dx0,&dy0);
  ctx_normalize(&dx1,&dy1);
  a = acosf(dx0*dx1 + dy0*dy1);
  d = radius / tanf(a/2.0f);

  if (d > 10000.0f) {
    ctx_line_to (ctx, x1, y1);
    return;
  }
  if ((dx1*dy0 - dx0*dy1) > 0.0f) {
    cx = x1 + dx0*d + dy0*radius;
    cy = y1 + dy0*d + -dx0*radius;
    a0 = atan2f(dx0, -dy0);
    a1 = atan2f(-dx1, dy1);
    dir = 0;
  } else {
    cx = x1 + dx0*d + -dy0*radius;
    cy = y1 + dy0*d + dx0*radius;
    a0 = atan2f(-dx0, dy0);
    a1 = atan2f(dx1, -dy1);
    dir = 1;
   }
   ctx_arc (ctx, cx, cy, radius, a0, a1, dir);
}

void
ctx_rel_arc_to (Ctx *ctx, float x1, float y1, float x2, float y2, float radius)
{
  x1 += ctx->state.x;
  y1 += ctx->state.y;
  x2 += ctx->state.x;
  y2 += ctx->state.y;
  ctx_arc_to (ctx, x1, y1, x2, y2, radius);
}

void
ctx_set_rgba_u8 (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_RGBA, r, g, b, a, 0, 0, 0, 0);
  ctx_process (ctx, &command);
}

void ctx_set_rgba (Ctx *ctx, float r, float g, float b, float a)
{
  int ir = r * 255;
  int ig = g * 255;
  int ib = b * 255;
  int ia = a * 255;
  ir = CTX_CLAMP(ir, 0,255);
  ig = CTX_CLAMP(ig, 0,255);
  ib = CTX_CLAMP(ib, 0,255);
  ia = CTX_CLAMP(ia, 0,255);
  ctx_set_rgba_u8 (ctx, ir, ig, ib, ia);
}

void ctx_set_rgb (Ctx *ctx, float   r, float   g, float   b)
{
  ctx_set_rgba (ctx, r, g, b, 1.0f);
}

void ctx_set_gray (Ctx *ctx, float gray)
{
  /* XXX we'd expect set gray of 0.5 to set 50 gray! */
  ctx_set_rgba (ctx, gray, gray, gray, 1.0f);
}

void
ctx_set_gradient_no (Ctx *ctx, int no)
{
  CtxEntry entry = ctx_s32 (CTX_GRADIENT_NO, no, 0);
  ctx_process (ctx, &entry);
}

void
ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1)
{
  CtxEntry command[2]={
     ctx_f (CTX_LINEAR_GRADIENT, x0, y0),
     ctx_f (CTX_CONT, x1, y1)};
  ctx_process (ctx, command);
}

void
ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0, float x1, float y1, float r1)
{
  CtxEntry command[3]={
     ctx_f (CTX_RADIAL_GRADIENT, x0, y0),
     ctx_f (CTX_CONT, r0, x1),
     ctx_f (CTX_CONT, y1, r1)};
  ctx_process (ctx, command);
}

void
ctx_gradient_clear_stops(Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_GRADIENT_CLEAR);
}

void ctx_gradient_add_stop_u8
(Ctx *ctx, float pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry entry = ctx_f (CTX_GRADIENT_STOP, pos, 0);
  entry.data.u8[4+0] = r;
  entry.data.u8[4+1] = g;
  entry.data.u8[4+2] = b;
  entry.data.u8[4+3] = a;
  ctx_process (ctx, &entry);
}

void ctx_gradient_add_stop_float
(Ctx *ctx, float pos, float r, float g, float b, float a)
{
  int ir = r * 255;
  int ig = g * 255;
  int ib = b * 255;
  int ia = a * 255;
  ir = CTX_CLAMP(ir, 0,255);
  ig = CTX_CLAMP(ig, 0,255);
  ib = CTX_CLAMP(ib, 0,255);
  ia = CTX_CLAMP(ia, 0,255);
  ctx_gradient_add_stop_u8 (ctx, pos, ir, ig, ib, ia);
}

#include <stdio.h>
#include <unistd.h>

void
ctx_flush (Ctx *ctx)
{
#if 0
  //printf (" \e[?2222h");

  ctx_journal_refpack (&ctx->journal);
  for (int i = 0; i < ctx->journal.count - 1; i++)
  {
    CtxEntry *entry = &ctx->journal.entries[i];
    fwrite (entry, 9, 1, stdout);
#if 0
    uint8_t  *buf = (void*)entry;
    for (int j = 0; j < 9; j++)
      printf ("%c", buf[j]);
#endif
  }
  printf ("Xx.Xx.Xx.");
  fflush (NULL);
#endif
  ctx->journal.count = 0;
  ctx_state_init (&ctx->state);
}

void
ctx_exit (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_EXIT);
}

static inline void
ctx_interpret_style (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
  {
    case CTX_LINE_WIDTH:
      state->gstate.line_width = ctx_arg_float(0);
      break;
    case CTX_LINE_CAP:
      state->gstate.line_cap = (CtxLineCap)ctx_arg_u8(0);
      break;
    case CTX_FILL_RULE:
      state->gstate.fill_rule = (CtxFillRule)ctx_arg_u8(0);
      break;
    case CTX_LINE_JOIN:
      state->gstate.line_join = (CtxLineJoin)ctx_arg_u8(0);
      break;
    case CTX_GLOBAL_ALPHA:
      state->gstate.source.global_alpha = CTX_CLAMP(ctx_arg_float(0)*255.0,0, 255);
      break;
    case CTX_FONT_SIZE:
      state->gstate.font_size = ctx_arg_float(0);
      break;
    case CTX_SET_RGBA:
      //ctx_source_deinit (&state->gstate.source);
      state->gstate.source.type = CTX_SOURCE_COLOR;
      for (int i = 0; i < 4; i ++)
        state->gstate.source.color.rgba[i] = ctx_arg_u8(i);
      break;
    case CTX_LINEAR_GRADIENT:
      {
        float x0 = ctx_arg_float(0);
        float y0 = ctx_arg_float(1);
        float x1 = ctx_arg_float(2);
        float y1 = ctx_arg_float(3);
        float dx, dy, length, start, end;

        ctx_user_to_device (state, &x0, &y0);
        ctx_user_to_device (state, &x1, &y1);

        length = hypotf (x1-x0,y1-y0);
        dx = (x1-x0) / length;
        dy = (y1-y0) / length;
        start = (x0 * dx + y0 * dy) / length;
        end =   (x1 * dx + y1 * dy) / length;

        state->gstate.source.linear_gradient.length = length;
        state->gstate.source.linear_gradient.dx = dx;
        state->gstate.source.linear_gradient.dy = dy;
        state->gstate.source.linear_gradient.start = start;
        state->gstate.source.linear_gradient.end = end;
        state->gstate.source.type = CTX_SOURCE_LINEAR_GRADIENT;
        ctx_matrix_identity (&state->gstate.source.transform);
      }
      break;

    case CTX_RADIAL_GRADIENT:
    {
        float x0 = ctx_arg_float(0);
        float y0 = ctx_arg_float(1);
        float r0 = ctx_arg_float(2);
        float x1 = ctx_arg_float(3);
        float y1 = ctx_arg_float(4);
        float r1 = ctx_arg_float(5);
        float t;
        ctx_user_to_device (state, &x0, &y0);
        ctx_user_to_device (state, &x1, &y1);
        t = 0.0f;
        ctx_user_to_device (state, &r0, &t);
        t = 0.0f;
        ctx_user_to_device (state, &r1, &t);

        state->gstate.source.radial_gradient.x0 = x0;
        state->gstate.source.radial_gradient.y0 = y0;
        state->gstate.source.radial_gradient.r0 = r0;
        state->gstate.source.radial_gradient.x1 = x1;
        state->gstate.source.radial_gradient.y1 = y1;
        state->gstate.source.radial_gradient.r1 = r1;
        state->gstate.source.type = CTX_SOURCE_RADIAL_GRADIENT;
        ctx_matrix_identity (&state->gstate.source.transform);
      }
      break;
    case CTX_GRADIENT_NO:
      state->gstate.source.gradient_no = ctx_arg_u8(0);
      break;
  }
}

static inline void
ctx_interpret_transforms (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
  {
    case CTX_SAVE:
      ctx_gstate_push (state);
      break;

    case CTX_RESTORE:
      ctx_gstate_pop (state);
      break;

    case CTX_IDENTITY:
      ctx_matrix_identity (&state->gstate.transform);
      break;

    case CTX_TRANSLATE:
      ctx_matrix_translate (&state->gstate.transform,
                            ctx_arg_float(0), ctx_arg_float(1));
      break;
    case CTX_SCALE:
      ctx_matrix_scale (&state->gstate.transform,
                        ctx_arg_float(0), ctx_arg_float(1));
      break;
    case CTX_ROTATE:
      ctx_matrix_rotate (&state->gstate.transform, ctx_arg_float(0));
      break;
  }
}

static inline void
_ctx_user_to_device (CtxState *state, float *x, float *y)
{
  CtxMatrix m = state->gstate.transform;
  float x_in = *x;
  float y_in = *y;
  *x = ((x_in * m.m[0][0]) + (y_in * m.m[1][0]) + m.m[2][0]);
  *y = ((y_in * m.m[1][1]) + (x_in * m.m[0][1]) + m.m[2][1]);
}

void
ctx_user_to_device (CtxState *state, float *x, float *y)
{
  _ctx_user_to_device (state, x, y);
}

#if CTX_BITPACK_PACKER

static float
find_max_dev (CtxEntry *entry, int nentrys)
{
  float max_dev = 0.0;
  for (int c = 0; c < nentrys; c++)
    {
      for (int d = 0; d < 2; d++)
      {
        if (entry[c].data.f[d] > max_dev)
          max_dev = entry[c].data.f[d];
        if (entry[c].data.f[d] < -max_dev)
          max_dev = -entry[c].data.f[d];
      }
    }
  return max_dev;
}

static void
pack_s8_args (CtxEntry *entry, int8_t *args, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      args[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV;
}

static void
pack_s16_args (CtxEntry *entry, int16_t *args, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      args[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV;
}


static void
ctx_journal_remove_tiny_curves (CtxJournal *journal, int start_pos)
{
  CtxIterator iterator;
  if ((journal->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    return;

  ctx_iterator_init (&iterator, journal, start_pos, CTX_ITERATOR_FLAT);
  iterator.end_pos = journal->count - 5;
  CtxEntry *command = NULL;
  while ((command = ctx_iterator_next(&iterator)))
  {
    /* things smaller than this have probably been scaled down
       beyond recognition, bailing on the geometry here makes it
       pack better and is less rendering work
     */
    if (command[0].code == CTX_REL_CURVE_TO)
    {
      float max_dev = find_max_dev (command, 3);
      if (max_dev < 1.0)
      {
        command[0].code = CTX_REL_LINE_TO;
        command[0].data.f[0] = command[2].data.f[0];
        command[0].data.f[1] = command[2].data.f[1];
        command[1].code = CTX_NOP;
        command[2].code = CTX_NOP;
      }
    }
  }
}

static void
ctx_journal_bitpack (CtxJournal *journal, int start_pos)
{
  int i = 0;

  if ((journal->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    return;
  ctx_journal_remove_tiny_curves (journal, journal->bitpack_pos);

  i = journal->bitpack_pos;

  if (start_pos > i)
    i = start_pos;

  while (i < journal->count - 4) /* the -4 is to avoid looking past
                                    initialized data we're not ready
                                    to bitpack yet*/
  {
    CtxEntry *entry = &journal->entries[i];

#if 1
    if (entry[0].code == CTX_REL_LINE_TO)
    {
      if (entry[1].code == CTX_REL_LINE_TO &&
          entry[2].code == CTX_REL_LINE_TO &&
          entry[3].code == CTX_REL_LINE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          int8_t args[8];
          pack_s8_args (entry, args, 4);
          entry[0].code = CTX_REL_LINE_TO_X4;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
      else if (entry[1].code == CTX_REL_CURVE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          int8_t args[8];
          pack_s8_args (entry, args, 4);
          entry[0].code = CTX_REL_LINE_TO_REL_CURVE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
      else if (entry[1].code == CTX_REL_LINE_TO &&
               entry[2].code == CTX_REL_LINE_TO &&
               entry[3].code == CTX_REL_LINE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          int8_t args[8];
          pack_s8_args (entry, args, 4);
          entry[0].code = CTX_REL_LINE_TO_X4;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
      else if (entry[1].code == CTX_REL_MOVE_TO)
      {
        float max_dev = find_max_dev (entry, 2);
        if (max_dev < 31000 / CTX_SUBDIV)
        {
          int16_t args[8];
          pack_s16_args (entry, args, 2);
          entry[0].code = CTX_REL_LINE_TO_REL_MOVE_TO;
          entry[1].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
      else if (entry[1].code == CTX_REL_LINE_TO)
      {
        float max_dev = find_max_dev (entry, 2);
        if (max_dev < 31000 / CTX_SUBDIV)
        {
          int16_t args[4];
          pack_s16_args (entry, args, 2);
          entry[0].code = CTX_REL_LINE_TO_X2;
          entry[1].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
    }
#endif
#if 1
    else if (entry[0].code == CTX_REL_CURVE_TO)
    {
      if (entry[3].code == CTX_REL_LINE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          int8_t args[8];
          pack_s8_args (entry, args, 4);
          entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
      else if (entry[3].code == CTX_REL_MOVE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          int8_t args[8];
          pack_s8_args (entry, args, 4);
          entry[0].code = CTX_REL_CURVE_TO_REL_MOVE_TO;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
        }
      }
      else
      {
        float max_dev = find_max_dev (entry, 3);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          int8_t args[8];
          pack_s8_args (entry, args, 3);
          args[6] = args[7] = 0;
          entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
    }
#endif
#if 1
    else if (entry[0].code == CTX_REL_QUAD_TO)
    {
      if (entry[2].code == CTX_REL_QUAD_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          int8_t args[8];
          pack_s8_args (entry, args, 4);
          entry[0].code = CTX_REL_QUAD_TO_REL_QUAD_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
          memcpy (&ctx_arg_u8(0), &args[0], 8);
        }
      }
      else
      {
        float max_dev = find_max_dev (entry, 2);
        if (max_dev < 3100 / CTX_SUBDIV)
        {
          int16_t args[4];
          pack_s16_args (entry, args, 2);
          entry[0].code = CTX_REL_QUAD_TO_S16;
          entry[1].code = CTX_NOP;
          memcpy (&entry[0].data.u16[0], &args[0], 8);
        }
      }
    }
#endif
#if 1
    else if (entry[0].code == CTX_FILL &&
             entry[1].code == CTX_MOVE_TO)
    {
        entry[0] = entry[1];
        entry[0].code = CTX_FILL_MOVE_TO;
        entry[1].code = CTX_NOP;
    }
#endif

    i += (ctx_conts_for_entry (entry) + 1);
  }

  int source = journal->bitpack_pos;
  int target = journal->bitpack_pos;
  int removed = 0;

  /* remove all nops that have been inserted as part of shortenings
   */
  while (source < journal->count)
  {
    CtxEntry *sentry = &journal->entries[source];
    CtxEntry *tentry = &journal->entries[target];

    while (sentry->code == CTX_NOP && source < journal->count)
    {
      source++;
      sentry = &journal->entries[source];
      removed++;
    }
    if (sentry != tentry)
      *tentry = *sentry;

    source ++;
    target ++;
  }
  journal->count -= removed;

  journal->bitpack_pos = journal->count - 4;
}

#endif

static inline int
ctx_entries_equal (const CtxEntry *a, const CtxEntry *b)
{
  if (!a || !b) return 0;
  if (a->code == CTX_REPEAT_HISTORY)
    return 0;
  return (a->code == b->code &&
          a->data.u32[0] == b->data.u32[0] &&
          a->data.u32[1] == b->data.u32[1]);
}

#if CTX_BITPACK_PACKER|CTX_REFPACK
static int
ctx_last_history (CtxJournal *journal)
{
  int last_history = 0;
  int i = 0;
  while (i < journal->count)
  {
    CtxEntry *entry = &journal->entries[i];
    if (entry->code == CTX_REPEAT_HISTORY)
    {
      last_history = i;
    }
    i += (ctx_conts_for_entry (entry) + 1);
  }
  return last_history;
}
#endif


#if CTX_REFPACK

static void
ctx_journal_remove (CtxJournal *journal, int pos, int count)
{
  if (count <= 0)
    return;
  for (int i = pos; i < journal->count - count; i++)
  {
    journal->entries[i] = journal->entries[i+count];
  }
  for (int i = journal->count - count; i < journal->count; i++)
  {
    journal->entries[i].code = CTX_CONT;
    journal->entries[i].data.f[0] = 0;
    journal->entries[i].data.f[1] = 0;
  }
  journal->count = journal->count - count;
}



/* find first match of input in dicitonary equal to or larger than
 * minimum_length
 */
static int
ctx_journal_dedup_search (CtxJournal *dictionary, int d_start, int d_end,
                          CtxJournal *input,      int i_start, int i_end,
                          int *match_start,
                          int *match_length,
                          int *input_pos,
                          int  minimum_length)
{
#if 1
  int m = d_end - d_start + 1;
  int n = i_end - i_start + 1;
  int result = 0;
  int end;
  int endpos;

  int len[2][n];
  int currRow = 0;

  for (int i = 0; i <= m; i++) {
    CtxEntry *ientry = &dictionary->entries[d_start + i - 1];
    for (int j= 0; j <= n; j++) {
      CtxEntry *jentry = &input->entries[i_start + j - 1];
      if (i == 0 || j == 0) {
        len[currRow][j] = 0;
      }
      else if (ctx_entries_equal (jentry, ientry))
      {
        len[currRow][j] = len[1 - currRow][j - 1] + 1;
        if (len[currRow][j] > result) {

          // XXX : ensure we're not splittinga CONT

          result = len[currRow][j];
          end = i + d_start - result ;
          endpos = j + i_start - result ;
          if (result >= minimum_length)
            goto done; // re-check the hit might be even better!
        }
      }
      else {
        len[currRow][j] = 0;
      }
    }
    currRow = 1 - currRow;
  }

  done:
  if (result >= minimum_length)
  {
    while (ctx_entries_equal (&dictionary->entries[end+result],
                              &input->entries[endpos+result]))
      result++;

  if (end + result >= d_end)
   {
     //ctx_log( "happens\n");
     result = d_end - end - 1;
   }

    *match_start  = end;
    *match_length = result;
    *input_pos    = endpos;
    return 1;
  }

  return 0;
#else
  int i = i_start;
  int longest = 0;
  int longest_length = 0;
  int longest_pos = 0;
  while (i < input->count)
  {
    CtxEntry *ientry = &input->entries[i];
    for (int j = 0; j < i - longest_length; )
    {
       CtxEntry *jentry = &dictionary->entries[j];
       int matches = 0;
       int mismatch = 0;

       for (int offset = 0; (!mismatch) && j+offset < i; offset++)
       {
         if (jentry[offset].code != CTX_REPEAT_HISTORY &&
             ctx_entries_equal (&jentry[offset], &ientry[offset]))
          {
            matches++;
          }
         else
          {
            mismatch = 1;
          }
       }
       if (matches > longest_length)
       {
         longest_pos = i;
         longest_length = matches;
         longest = j;
       }
       if (longest_length >= minimum_length)
         goto done;
       j += (ctx_conts_for_command (jcommand) + 1);
    }
    i += (ctx_conts_for_command (icommand) + 1);
  }

  done:
  if (longest_length >= minimum_length)
  {
    *match_start  = longest;
    *match_length = longest_length;
    *input_pos = longest_pos;

    return 1;
  }
  return 0;
#endif
}

#endif

static void
ctx_journal_refpack (CtxJournal *journal)
{
#if CTX_BITPACK_PACKER|CTX_REFPACK
  int last_history;
  last_history = ctx_last_history (journal);
#endif
#if CTX_BITPACK_PACKER
  ctx_journal_bitpack (journal, last_history);
#endif
#if CTX_REFPACK
  int length_threshold = 4;

  if ((journal->flags & CTX_TRANSFORMATION_REFPACK) == 0)
    return;

  if (!last_history)
  {
    last_history = 8;
    if (last_history > journal->count)
      last_history = journal->count / 2;
  }

  int completed = last_history;

  while (completed < journal->count)
  {
    int default_search_window = 128;
    int search_window = default_search_window;

    int match_start;
    int match_length=0;
    int match_input_pos;

    if ((journal->count - completed) < search_window)
      search_window = journal->count - completed;

    while (ctx_journal_dedup_search (
           journal, 0, completed-1,
           journal, completed+1, completed+search_window - 2,
           &match_start, &match_length, &match_input_pos,
           length_threshold))
    {
      CtxEntry *ientry = &journal->entries[match_input_pos];
      ientry->code = CTX_REPEAT_HISTORY;
      ientry->data.u32[0]= match_start;
      ientry->data.u32[1]= match_length;
      ctx_journal_remove (journal, match_input_pos+1, match_length-1);
    }

    completed += default_search_window;
  }
#endif
}


static void
ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data)
{
  float start_x = state->x;
  float start_y = state->y;
  int had_moved = state->has_moved;

  switch (entry->code)
  {
    case CTX_CLEAR:
       ctx_state_init (state);
       break;
    case CTX_CLIP:
    case CTX_FILL:
    case CTX_STROKE:
    case CTX_NEW_PATH:
      state->has_moved = 0;
      break;
    case CTX_MOVE_TO:

    case CTX_LINE_TO:
      { float x = ctx_arg_float(0);
        float y = ctx_arg_float(1);
        state->x = x;
        state->y = y;
        if (!state->has_moved || entry->code == CTX_MOVE_TO)
        {
          state->path_start_x = state->x;
          state->path_start_y = state->y;
          state->has_moved = 1;
        }

        if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
        {
          _ctx_user_to_device (state, &x, &y);
          ctx_arg_float(0) = x;
          ctx_arg_float(1) = y;
        }
     }
     break;

    case CTX_ARC:
      state->x = ctx_arg_float (0) + cosf (ctx_arg_float (4)) * ctx_arg_float (2);
      state->y = ctx_arg_float (1) + sinf (ctx_arg_float (4)) * ctx_arg_float (2);

      if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
      {
        float x = ctx_arg_float (0);
        float y = ctx_arg_float (1);
        float r = ctx_arg_float (2);
        _ctx_user_to_device (state, &x, &y);
        ctx_arg_float(0) = x;
        ctx_arg_float(1) = y;
        y = 0;
        _ctx_user_to_device (state, &r, &y);
        ctx_arg_float(2) = r;
      }

      break;


    case CTX_CURVE_TO:
      if (!state->has_moved) // bit ifft for curveto
      {
        state->path_start_x = state->x;
        state->path_start_y = state->y;
        state->has_moved = 1;
      }
      state->x = ctx_arg_float (4);
      state->y = ctx_arg_float (5);

      if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
      {
        for (int c = 0; c < 3; c ++)
        {
          float x = entry[c].data.f[0];
          float y = entry[c].data.f[1];
          _ctx_user_to_device (state, &x, &y);
          entry[c].data.f[0] = x;
          entry[c].data.f[1] = y;
        }
      }
      break;


    case CTX_QUAD_TO:
      if (!state->has_moved) // bit ifft for curveto
      {
        state->path_start_x = state->x;
        state->path_start_y = state->y;
        state->has_moved = 1;
      }
      state->x = ctx_arg_float (2);
      state->y = ctx_arg_float (3);

      if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
      {
        for (int c = 0; c < 2; c ++)
        {
          float x = entry[c].data.f[0];
          float y = entry[c].data.f[1];
          _ctx_user_to_device (state, &x, &y);
          entry[c].data.f[0] = x;
          entry[c].data.f[1] = y;
        }
      }
      break;


    case CTX_REL_MOVE_TO:
    case CTX_REL_LINE_TO:
      state->x += ctx_arg_float(0);
      state->y += ctx_arg_float(1);

      if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
      {
        for (int c = 0; c < 1; c ++)
        {
          float x = state->x;
          float y = state->y;
          _ctx_user_to_device (state, &x, &y);
          entry[c].data.f[0] = x;
          entry[c].data.f[1] = y;
        }
        if (entry->code == CTX_REL_MOVE_TO)
          entry->code = CTX_MOVE_TO;
        else
          entry->code = CTX_LINE_TO;
      }
      break;
    case CTX_REL_CURVE_TO:
      {
        float nx = state->x + ctx_arg_float (4);
        float ny = state->y + ctx_arg_float (5);

        if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
        {
          for (int c = 0; c < 3; c ++)
          {
            float x = state->x + entry[c].data.f[0];
            float y = state->y + entry[c].data.f[1];
            ctx_user_to_device (state, &x, &y);
            entry[c].data.f[0] = x;
            entry[c].data.f[1] = y;
          }
          entry->code = CTX_CURVE_TO;
        }
        state->x = nx;
        state->y = ny;
      }
      break;

    case CTX_REL_QUAD_TO:
      {
        float nx = state->x + ctx_arg_float (2);
        float ny = state->y + ctx_arg_float (3);

        if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
        {
          for (int c = 0; c < 2; c ++)
          {
            float x = state->x + entry[c].data.f[0];
            float y = state->y + entry[c].data.f[1];
            ctx_user_to_device (state, &x, &y);
            entry[c].data.f[0] = x;
            entry[c].data.f[1] = y;
          }
          entry->code = CTX_QUAD_TO;
        }
        state->x = nx;
        state->y = ny;
      }
      break;
  }

  if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_RELATIVE))
  {
    int components = 0;
    ctx_user_to_device (state, &start_x, &start_y);
    switch (entry->code)
    {
      case CTX_MOVE_TO:
        if (had_moved) components = 1;
        break;
      case CTX_LINE_TO:
        components = 1;
      break;
      case CTX_CURVE_TO:
        components = 3;
      break;
      case CTX_QUAD_TO:
        components = 2;
      break;
    }
    if (components)
    {
      for (int c = 0; c < components; c++)
      {
        entry[c].data.f[0] -= start_x;
        entry[c].data.f[1] -= start_y;
      }
      switch (entry->code)
      {
        case CTX_MOVE_TO:
          entry[0].code = CTX_REL_MOVE_TO;
          break;
        case CTX_LINE_TO:
          entry[0].code = CTX_REL_LINE_TO;
          break;
        break;
        case CTX_CURVE_TO:
          entry[0].code = CTX_REL_CURVE_TO;
        break;
        case CTX_QUAD_TO:
          entry[0].code = CTX_REL_QUAD_TO;
        break;
      }
    }
  }
}

static void
ctx_interpret_pos_bare (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
  {
    case CTX_CLEAR:
       ctx_state_init (state);
       break;
    case CTX_CLIP:
    case CTX_NEW_PATH:
    case CTX_FILL:
    case CTX_STROKE:
      state->has_moved = 0;
      break;
    case CTX_MOVE_TO:
    case CTX_LINE_TO:
      { float x = ctx_arg_float(0);
        float y = ctx_arg_float(1);
        state->x = x;
        state->y = y;
        if (!state->has_moved)
        {
          state->path_start_x = state->x;
          state->path_start_y = state->y;
          state->has_moved = 1;
        }
     }
     break;
    case CTX_CURVE_TO:
      state->x = ctx_arg_float (4);
      state->y = ctx_arg_float (5);

      if (!state->has_moved)
      {
        state->path_start_x = state->x;
        state->path_start_y = state->y;
        state->has_moved = 1;
      }
      break;
    case CTX_QUAD_TO:
      state->x = ctx_arg_float (2);
      state->y = ctx_arg_float (3);

      if (!state->has_moved)
      {
        state->path_start_x = state->x;
        state->path_start_y = state->y;
        state->has_moved = 1;
      }
      break;
    case CTX_ARC:
      state->x = ctx_arg_float (0) + cosf (ctx_arg_float (4)) * ctx_arg_float (2);
      state->y = ctx_arg_float (1) + sinf (ctx_arg_float (4)) * ctx_arg_float (2);
      break;

    case CTX_REL_MOVE_TO:
    case CTX_REL_LINE_TO:
      state->x += ctx_arg_float(0);
      state->y += ctx_arg_float(1);
      break;
    case CTX_REL_CURVE_TO:
      state->x += ctx_arg_float(4);
      state->y += ctx_arg_float(5);
      break;
    case CTX_REL_QUAD_TO:
      state->x += ctx_arg_float(2);
      state->y += ctx_arg_float(3);
      break;
  }
}

static void
ctx_interpret_pos (CtxState *state, CtxEntry *entry, void *data)
{
  if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) ||
      (((Ctx*)(data))->transformation & CTX_TRANSFORMATION_RELATIVE))
  {
     ctx_interpret_pos_transform (state, entry, data);
     return;
  }
  ctx_interpret_pos_bare (state, entry, data);
}

static void
ctx_state_init (CtxState *state)
{
  memset (state, 0, sizeof (CtxState)); // XXX: directly setting the
                                        // actually involved bits will be
                                        // faster
  state->gstate.source.global_alpha = 255;
  state->gstate.font_size    = 12;
  state->gstate.line_spacing = 1.0;
  state->gstate.line_width = 2.0;
  ctx_matrix_identity (&state->gstate.transform);
}

static void
ctx_init (Ctx *ctx)
{
  ctx_state_init (&ctx->state);

#if 1
  ctx->transformation |= (CtxTransformation)CTX_TRANSFORMATION_SCREEN_SPACE;
  ctx->transformation |= (CtxTransformation)CTX_TRANSFORMATION_RELATIVE;
#if CTX_BITPACK
  ctx->journal.flags  |= CTX_TRANSFORMATION_BITPACK;
#endif
  ctx->journal.flags  |= CTX_TRANSFORMATION_REFPACK;
#endif
}


static void ctx_setup ();

Ctx *
ctx_new (void)
{
  ctx_setup ();
  Ctx *ctx = (Ctx*)malloc (sizeof (Ctx));
  memset (ctx, 0, sizeof(Ctx));
  ctx_init (ctx);

  return ctx;
}

void
ctx_journal_deinit (CtxJournal *journal)
{
  if (journal->entries && !(journal->flags & CTX_JOURNAL_DOESNT_OWN_ENTRIES))
    free (journal->entries);
  journal->entries = NULL;
}

#if CTX_RASTERIZER
static void ctx_renderer_deinit (CtxRenderer *renderer);
#endif

static void ctx_deinit (Ctx *ctx)
{
#if CTX_RASTERIZER
  if (ctx->renderer)
  {
    ctx_renderer_deinit (ctx->renderer);
    free (ctx->renderer);
    ctx->renderer = NULL;
  }
#endif
  ctx_journal_deinit (&ctx->journal);
}

void ctx_free (Ctx *ctx)
{
  if (!ctx)
    return;
  ctx_deinit (ctx);
  free (ctx);
}

Ctx *ctx_new_for_journal     (void *data, size_t length)
{
  Ctx *ctx = ctx_new ();
  ctx->journal.flags |= CTX_JOURNAL_DOESNT_OWN_ENTRIES;
  ctx->journal.entries = (CtxEntry*)data;
  ctx->journal.count   = length / sizeof (CtxEntry);
  return ctx;
}

#if CTX_RASTERIZER
////////////////////////////////////

typedef struct CtxEdge {
  int32_t  x;     /* the center-line intersection      */
  int32_t  dx;
  uint16_t index;
#if 0             // part of experiments to improve rasterizer
  int32_t  minx;
  int32_t  maxx;
#endif
} CtxEdge;



struct _CtxPixelFormatInfo
{
  CtxPixelFormat pixel_format;
  uint8_t        components; /* number of components */
  uint8_t        bpp; /* bits  per pixel - for doing offset computations
                         along with rowstride found elsewhere, if 0 it indicates
                         1/8  */
  uint8_t        ebpp; /*effective bytes per pixel - for doing offset
                         computations, for formats that get converted, the
                         ebpp of the working space applied */
  uint8_t        dither_red_blue;
  uint8_t        dither_green;
  void     (*to_rgba8)(CtxRenderer *renderer,
                       int x, const void *buf, uint8_t *rgba, int count);
  void     (*from_rgba8)(CtxRenderer *renderer,
                         int x, void *buf, const uint8_t *rgba, int count);

  int      (*crunch)(CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage,
                     int count);
};

struct _CtxRenderer {
  /* these should be initialized and used as the bounds for rendering into the
     buffer as well XXX: not yet in use, and when in use will only be
     correct for axis aligned clips - proper rasterization of a clipping path
     would be yet another refinement on top.
   */
  int      clip_min_x;
  int      clip_min_y;
  int      clip_max_x;
  int      clip_max_y;

  CtxEdge  lingering[CTX_MAX_EDGES];
  int      lingering_edges;  // previous half scanline

  CtxEdge  edges[CTX_MAX_EDGES];
  int      active_edges;
  int      pending_edges;    // next half scanline

  int      edge_pos;         // where we're at in iterating all edges

  int      scanline;
  int      scan_min;
  int      scan_max;
  int      needs_aa;         // count of how many edges implies antialiasing

  CtxJournal edge_list;
  CtxState  *state;

  void      *buf;
  float      x;
  float      y;

  float      first_x;
  float      first_y;
  int        has_shape;
  int        has_prev;

  int        uses_transforms;

  int        min_x;
  int        max_x;

  int        blit_x;
  int        blit_y;
  int        blit_width;
  int        blit_height;
  int        blit_stride;

  CtxBuffer  texture[CTX_MAX_TEXTURES];

  CtxPixelFormatInfo *format;
};

static CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format);

CtxBuffer *ctx_buffer_new (void)
{
  CtxBuffer *buffer = (CtxBuffer*)malloc (sizeof (CtxBuffer));
  memset (buffer, 0, sizeof (CtxBuffer));
  return buffer;
}

void ctx_buffer_set_data (CtxBuffer *buffer,
                          void *data, int width, int height,
                          int stride,
                          CtxPixelFormat pixel_format,
                          int free_buf)
{
  if (buffer->free_buf)
    free (buffer->data);
  buffer->data = data;
  buffer->width = width;
  buffer->height = height;
  buffer->stride = stride;
  buffer->format = ctx_pixel_format_info (pixel_format);
  buffer->free_buf = free_buf;
}

CtxBuffer *ctx_buffer_new_for_data (void *data, int width, int height,
                                    int stride,
                                    CtxPixelFormat pixel_format,
                                    int free_buf)
{
  CtxBuffer *buffer = ctx_buffer_new ();
  ctx_buffer_set_data (buffer, data, width, height, stride, pixel_format, free_buf);
  return buffer;
}

void ctx_buffer_deinit (CtxBuffer *buffer)
{
  if (buffer->free_buf)
    free (buffer->data);
  buffer->data = NULL;
  buffer->free_buf = 0;
}

void ctx_buffer_free (CtxBuffer *buffer)
{
  ctx_buffer_deinit (buffer);
  free (buffer);
}

/* load the png into the buffer */
int ctx_buffer_load_png (CtxBuffer *buffer,
                          const char *path)
{
  ctx_buffer_deinit (buffer);

#ifdef UPNG_H
  upng_t *upng = upng_new_from_file (path);
  int components;
  if (upng == NULL)
    return -1;
  upng_header (upng);
  upng_decode (upng);
  components = upng_get_components (upng);
  buffer->width = upng_get_width (upng);
  buffer->height = upng_get_height (upng);
  buffer->data = upng_steal_buffer (upng);
  upng_free (upng);
  buffer->stride = buffer->width * components;
  switch (components)
  {
    case 1:
      buffer->format = ctx_pixel_format_info (CTX_FORMAT_GRAY8);
      break;
    case 2:
      buffer->format = ctx_pixel_format_info (CTX_FORMAT_GRAYA8);
      break;
    case 3:
      buffer->format = ctx_pixel_format_info (CTX_FORMAT_RGB8);
      break;
    case 4:
      buffer->format = ctx_pixel_format_info (CTX_FORMAT_RGBA8);
      break;
  }
  buffer->free_buf = 1;
  return 0;
#else
  return -1;
#endif
}

static void
ctx_renderer_set_gradient_no (CtxRenderer *renderer, int no)
{
  if (no < 0 || no >= CTX_MAX_GRADIENTS) no = 0;
  renderer->state->gstate.source.gradient_no = no;
}

static void
ctx_renderer_linear_gradient (CtxRenderer *renderer,
                              float x0, float y0,
                              float x1, float y1)
{
  CtxSource *source = &renderer->state->gstate.source;

  source->linear_gradient.x0 = x0;
  source->linear_gradient.y0 = y0;
  source->linear_gradient.x1 = x1;
  source->linear_gradient.y1 = y1;
}

static void
ctx_renderer_radial_gradient (CtxRenderer *renderer,
                              float x0, float y0,
                              float r0, float x1,
                              float y1, float r1)
{
  CtxSource *source = &renderer->state->gstate.source;
  source->radial_gradient.x0 = x0;
  source->radial_gradient.y0 = y0;
  source->radial_gradient.x1 = x1;
  source->radial_gradient.y1 = y1;
  source->radial_gradient.r0 = r0;
  source->radial_gradient.r1 = r1;
}

static void
ctx_renderer_gradient_clear_stops(CtxRenderer *renderer)
{
  renderer->state->gradient[renderer->state->gstate.source.gradient_no].n_stops = 0;
}

static void
ctx_renderer_gradient_add_stop (CtxRenderer *renderer, float pos, uint8_t *rgba)
{
  CtxGradient *gradient = &renderer->state->gradient[renderer->state->gstate.source.gradient_no];

  CtxGradientStop *stop = &gradient->stops[gradient->n_stops];
  stop->pos = pos;
  stop->rgba[0] = rgba[0];
  stop->rgba[1] = rgba[1];
  stop->rgba[2] = rgba[2];
  stop->rgba[3] = rgba[3];
  if (gradient->n_stops < 15)//we'll keep overwriting the last when out of stops
    gradient->n_stops++;
}

static inline int ctx_renderer_add_edge (CtxRenderer *renderer, int x0, int y0, int x1, int y1)
{
  int16_t args[4];
  int max_x = renderer->blit_x + renderer->blit_width;
  if (y1 < renderer->scan_min) renderer->scan_min = y1;
  if (y1 > renderer->scan_max) renderer->scan_max = y1;
  if (x0 > max_x && x1 > max_x) return -1;
  args[0]=x0;
  args[1]=y0;
  args[2]=x1;
  args[3]=y1;

  return ctx_journal_add_u32 (&renderer->edge_list, CTX_EDGE, (uint32_t*)args);
}

static void ctx_renderer_poly_to_edges (CtxRenderer *renderer)
{
  int16_t x = 0;
  int16_t y = 0;
  for (int i = 0; i < renderer->edge_list.count; i++)
  {
    CtxEntry *entry = &renderer->edge_list.entries[i];
    if (entry->code == CTX_NEW_EDGE)
    {
      entry->code = CTX_EDGE;
    }
    else
    {
      entry->data.s16[0] = x;
      entry->data.s16[1] = y;
    }
    x = entry->data.s16[2];
    y = entry->data.s16[3];

    if (entry->data.s16[3] < entry->data.s16[1])
    {
       *entry = ctx_s16(CTX_EDGE_FLIPPED,
                        entry->data.s16[2], entry->data.s16[3],
                        entry->data.s16[0], entry->data.s16[1]);
    }
  }
}

static inline void ctx_renderer_line_to (CtxRenderer *renderer, float x, float y);

static void ctx_renderer_finish_shape (CtxRenderer *renderer)
{
  if (renderer->has_shape && renderer->has_prev)
  {
    ctx_renderer_line_to (renderer, renderer->first_x, renderer->first_y);
    renderer->has_prev = 0;
  }
}

static inline void ctx_renderer_move_to (CtxRenderer *renderer, float x, float y)
{
  renderer->x        = x;
  renderer->y        = y;
  renderer->first_x  = x;
  renderer->first_y  = y;
  renderer->has_prev = 0;
}

static inline void ctx_renderer_line_to (CtxRenderer *renderer, float x, float y)
{
  float tx = x;
  float ty = y;
  float ox = renderer->x;
  float oy = renderer->y;

  if (renderer->uses_transforms)
  {
    ctx_user_to_device (renderer->state, &tx, &ty);
  }
  tx -= renderer->blit_x;
  ctx_renderer_add_edge (renderer, 0, 0, tx * CTX_SUBDIV, ty * CTX_RASTERIZER_AA);
  if (!renderer->has_prev)
  {
    if (renderer->uses_transforms)
      ctx_user_to_device (renderer->state, &ox, &oy);
    ox -= renderer->blit_x;
    renderer->edge_list.entries[renderer->edge_list.count-1].data.s16[0] = ox * CTX_SUBDIV;
    renderer->edge_list.entries[renderer->edge_list.count-1].data.s16[1] = oy * CTX_RASTERIZER_AA;
    renderer->edge_list.entries[renderer->edge_list.count-1].code = CTX_NEW_EDGE;
    renderer->has_prev = 1;
  }
  renderer->has_shape = 1;
  renderer->y = y;
  renderer->x = x;
}

static inline float
ctx_lerpf (float v0, float v1, float dx)
{
  return v0 + (v1-v0) * dx;
}

static inline float
ctx_bezier_sample_1d (float x0, float x1, float x2, float x3, float dt)
{
  float ab   = ctx_lerpf (x0, x1, dt);
  float bc   = ctx_lerpf (x1, x2, dt);
  float cd   = ctx_lerpf (x2, x3, dt);
  float abbc = ctx_lerpf (ab, bc, dt);
  float bccd = ctx_lerpf (bc, cd, dt);
  return ctx_lerpf (abbc, bccd, dt);
}

static inline void
ctx_bezier_sample (float x0, float y0,
                   float x1, float y1,
                   float x2, float y2,
                   float x3, float y3,
                   float dt, float *x, float *y)
{
  *x = ctx_bezier_sample_1d (x0, x1, x2, x3, dt);
  *y = ctx_bezier_sample_1d (y0, y1, y2, y3, dt);
}

/* XXX : pass the constant bits as a vector or pointed to struct? */
static inline void
ctx_renderer_bezier_divide (CtxRenderer *renderer,
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
    return;

  float t = (s + e) * 0.5f;
  float x, y, lx, ly, dx, dy;
  ctx_bezier_sample (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);

  if (iteration)
  {
    lx = ctx_lerpf (sx, ex, t);
    ly = ctx_lerpf (sy, ey, t);

    dx = lx - x;
    dy = ly - y;
    if ((dx*dx+dy*dy) < tolerance)
      /* bailing - because for the mid-point straight line difference is
         tiny */
      return;

    dx = sx - ex;
    dy = ey - ey;
    if ((dx*dx+dy*dy) < tolerance)
      /* bailing on tiny segments */
      return;
  }

  ctx_renderer_bezier_divide (renderer, ox, oy, x0, y0, x1, y1, x2, y2,
                                        sx, sy, x, y, s, t, iteration + 1,
                                        tolerance);
  ctx_renderer_line_to (renderer, x, y);
  ctx_renderer_bezier_divide (renderer, ox, oy, x0, y0, x1, y1, x2, y2,
                                        x, y, ex, ey, t, e, iteration + 1,
                                        tolerance);
}

static inline void
ctx_renderer_curve_to (CtxRenderer *renderer,
                       float x0, float y0,
                       float x1, float y1,
                       float x2, float y2)
{
  float ox = renderer->x;
  float oy = renderer->y;

  float tolerance =
     ctx_pow2(renderer->state->gstate.transform.m[0][0]) +
     ctx_pow2(renderer->state->gstate.transform.m[1][1]);
  tolerance = 1.0f/tolerance;

  ox = renderer->state->x;
  oy = renderer->state->y;

  float maxx = CTX_MAX(x1,x2);
  maxx = CTX_MAX(maxx, ox);
  maxx = CTX_MAX(maxx, x0);
  float maxy = CTX_MAX(y1,y2);
  maxy = CTX_MAX(maxy, oy);
  maxy = CTX_MAX(maxy, y0);
  float minx = CTX_MIN(x1,x2);
  minx = CTX_MIN(minx, ox);
  minx = CTX_MIN(minx, x0);
  float miny = CTX_MIN(y1,y2);
  miny = CTX_MIN(miny, oy);
  miny = CTX_MIN(miny, y0);

  if (tolerance == 1.0f &&
      (
      (minx > renderer->blit_x + renderer->blit_width) ||
      (miny > renderer->blit_y + renderer->blit_height) ||
      (maxx < renderer->blit_x) ||
      (maxy < renderer->blit_y)))
  {
    // tolerance==1.0 is most likely screen-space -
    // skip subdivides for things outside
  }
  else
  {
    ctx_renderer_bezier_divide (renderer,
                                ox, oy, x0, y0,
                                x1, y1, x2, y2,
                                ox, oy, x2, y2,
                                0.0, 1.0, 0, tolerance);
  }
  ctx_renderer_line_to (renderer, x2, y2);
}

static inline void
ctx_renderer_rel_move_to (CtxRenderer *renderer, float x, float y)
{
  if (x == 0.f && y == 0.f)
    return;
  x += renderer->x;
  y += renderer->y;
  ctx_renderer_move_to (renderer, x, y);
}

static inline void
ctx_renderer_rel_line_to (CtxRenderer *renderer, float x, float y)
{
  if (x== 0.f && y==0.f)
    return;
  x += renderer->x;
  y += renderer->y;
  ctx_renderer_line_to (renderer, x, y);
}

static inline void
ctx_renderer_rel_curve_to (CtxRenderer *renderer,
                            float x0, float y0, float x1, float y1, float x2, float y2)
{
  x0 += renderer->x;
  y0 += renderer->y;

  x1 += renderer->x;
  y1 += renderer->y;

  x2 += renderer->x;
  y2 += renderer->y;

  ctx_renderer_curve_to (renderer, x0, y0, x1, y1, x2, y2);
}


static int ctx_compare_edges (const void *ap, const void *bp)
{
  const CtxEntry *a = (const CtxEntry*)ap;
  const CtxEntry *b = (const CtxEntry*)bp;
  int ycompare = a->data.s16[1] - b->data.s16[1];

  if (ycompare)
   return ycompare;

  int xcompare = a->data.s16[0] - b->data.s16[0];

  return xcompare;
}

static inline void ctx_renderer_sort_edges (CtxRenderer *renderer)
{
  qsort (&renderer->edge_list.entries[0], renderer->edge_list.count,
         sizeof (CtxEntry), ctx_compare_edges);
}


static inline void ctx_renderer_discard_edges (CtxRenderer *renderer)
{
  for (int i = 0; i < renderer->active_edges; i++)
  {
    if (renderer->edge_list.entries[renderer->edges[i].index].data.s16[3] < renderer->scanline
)
    {
       if (renderer->lingering_edges + 1 < CTX_MAX_EDGES)
       {
         renderer->lingering[renderer->lingering_edges] =
           renderer->edges[i];
         renderer->lingering_edges++;
       }

      renderer->edges[i] = renderer->edges[renderer->active_edges-1];
      renderer->active_edges--;
      i--;
    }
  }
  for (int i = 0; i < renderer->lingering_edges; i++)
  {
    if (renderer->edge_list.entries[renderer->lingering[i].index].data.s16[3] < renderer->scanline - CTX_RASTERIZER_AA2)
    {
      if (renderer->lingering[i].dx > CTX_RASTERIZER_AA_SLOPE_LIMIT ||
          renderer->lingering[i].dx < -CTX_RASTERIZER_AA_SLOPE_LIMIT)
          renderer->needs_aa --;

      renderer->lingering[i] = renderer->lingering[renderer->lingering_edges-1];
      renderer->lingering_edges--;

      i--;

    }
  }
}

static inline void ctx_renderer_increment_edges (CtxRenderer *renderer, int count)
{
  for (int i = 0; i < renderer->lingering_edges; i++)
  {
     renderer->lingering[i].x += renderer->lingering[i].dx * count;
#if 0
     renderer->lingering[i].minx += renderer->lingering[i].dx * count;
     renderer->lingering[i].maxx += renderer->lingering[i].dx * count;
#endif
  }

  for (int i = 0; i < renderer->active_edges; i++)
  {
     renderer->edges[i].x += renderer->edges[i].dx * count;
#if 0
     renderer->edges[i].minx += renderer->edges[i].dx * count;
     renderer->edges[i].maxx += renderer->edges[i].dx * count;
#endif
  }
  for (int i = 0; i < renderer->pending_edges; i++)
  {
     renderer->edges[CTX_MAX_EDGES-1-i].x += renderer->edges[CTX_MAX_EDGES-1-i].dx * count;
#if 0
     renderer->edges[CTX_MAX_EDGES-1-i].minx += renderer->edges[CTX_MAX_EDGES-1-i].dx * count;
     renderer->edges[CTX_MAX_EDGES-1-i].maxx += renderer->edges[CTX_MAX_EDGES-1-i].dx * count;
#endif
  }
}

/* feeds up to renderer->scanline,
   keeps a pending buffer of edges - that encompass
   the full coming scanline - for adaptive AA,
   feed until the start of the scanline and check for need for aa
   in all of pending + active edges, then
   again feed_edges until middle of scanline if doing non-AA
   or directly render when doing AA
*/
static inline void ctx_renderer_feed_edges (CtxRenderer *renderer)
{
  int miny;
  for (int i = 0; i < renderer->pending_edges; i++)
  {
     if (renderer->edge_list.entries[renderer->edges[CTX_MAX_EDGES-1-i].index].data.s16[1] <= renderer->scanline)
     {
       if (renderer->active_edges < CTX_MAX_EDGES-2)
       {
         int no = renderer->active_edges;
         renderer->active_edges++;
         renderer->edges[no] = renderer->edges[CTX_MAX_EDGES-1-i];
         renderer->edges[CTX_MAX_EDGES-1-i] =
         renderer->edges[CTX_MAX_EDGES-1-renderer->pending_edges + 1];
         renderer->pending_edges--;
         i--;
       }
     }
  }
  while (renderer->edge_pos < renderer->edge_list.count &&
         (miny=renderer->edge_list.entries[renderer->edge_pos].data.s16[1]) <= renderer->scanline)
  {
    if (renderer->active_edges < CTX_MAX_EDGES-2)
    {
      int dy = (renderer->edge_list.entries[renderer->edge_pos].data.s16[3] -
                miny);
      if (dy) /* skipping horizontal edges */
      {
        int yd = renderer->scanline - miny;
        int no = renderer->active_edges;
        renderer->active_edges++;

        renderer->edges[no].index = renderer->edge_pos;

        int x0 = renderer->edge_list.entries[renderer->edges[no].index].data.s16[0];
        int x1 = renderer->edge_list.entries[renderer->edges[no].index].data.s16[2];
        renderer->edges[no].x = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER;

        int dx_dy;
      //  if (dy)
        dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
      //  else
      //  dx_dy = 0;

        renderer->edges[no].dx = dx_dy;
        renderer->edges[no].x += (yd * dx_dy);

        // XXX : even better minx and maxx can
        //       be derived using y0 and y1 for scaling dx_dy
        //       when ydelta to these are smaller than
        //       ydelta to scanline
#if 0
        if (dx_dy < 0)
        {
          renderer->edges[no].minx =
            renderer->edges[no].x + dx_dy/2;
          renderer->edges[no].maxx =
            renderer->edges[no].x - dx_dy/2;
        }
        else
        {
          renderer->edges[no].minx =
            renderer->edges[no].x - dx_dy/2;
          renderer->edges[no].maxx =
            renderer->edges[no].x + dx_dy/2;
        }
#endif

        if (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT ||
            dx_dy < -CTX_RASTERIZER_AA_SLOPE_LIMIT)
          renderer->needs_aa ++;

        if (! (miny <= renderer->scanline))
        {
          /* it is a pending edge - we add it to the end of the array
             and keep a different count for items stored here, similar
             to how heap and stack grows against each other
          */
          renderer->edges[CTX_MAX_EDGES-1-renderer->pending_edges] =
            renderer->edges[no];
          renderer->active_edges--;
          renderer->pending_edges++;
        }
      }
    }
    renderer->edge_pos++;
  }
}

static void ctx_renderer_sort_active_edges (CtxRenderer *renderer)
{
  int sorted = 0;
  while (!sorted)
  {
    sorted = 1;
    for (int i = 0; i < renderer->active_edges-1; i++)
    {
      CtxEdge *a = &renderer->edges[i];
      CtxEdge *b = &renderer->edges[i+1];
      if (a->x > b->x)
        {
          CtxEdge tmp = *b;
          *b = *a;
          *a = tmp;
          sorted = 0;
        }
    }
  }
  sorted = 0;
#if 0
  while (!sorted)
  {
    sorted = 1;
    for (int i = 0; i < renderer->pending_edges-1; i++)
    {
      CtxEdge *a = &renderer->edges[CTX_MAX_EDGES-1-i];
      CtxEdge *b = &renderer->edges[CTX_MAX_EDGES-1-(i+1)];
      if (a->x > b->x)
        {
          CtxEdge tmp = *b;
          *b = *a;
          *a = tmp;
          sorted = 0;
        }
    }
  }
  sorted = 0;
  while (!sorted)
  {
    sorted = 1;
    for (int i = 0; i < renderer->lingering_edges-1; i++)
    {
      CtxEdge *a = &renderer->lingering[i];
      CtxEdge *b = &renderer->lingering[i+1];
      if (a->x > b->x)
        {
          CtxEdge tmp = *b;
          *b = *a;
          *a = tmp;
          sorted = 0;
        }
    }
  }
#endif
}

static inline uint8_t ctx_lerp_u8 (uint8_t v0, uint8_t v1, uint8_t dx)
{
  return (((((v0)<<8) + (dx) * ((v1) - (v0))))>>8);
}

static void
ctx_sample_gradient_1d_u8 (CtxRenderer *renderer, float v, uint8_t *rgba)
{
  /* caching a 512 long gradient - and sampling with nearest neighbor
     will be much faster.. */
  CtxGradient *g = &renderer->state->gradient[renderer->state->gstate.source.gradient_no];

  if (v < 0) v = 0;
  if (v > 1) v = 1;

  if (g->n_stops == 0)
  {
    rgba[0] = rgba[1] = rgba[2] = v * 255;
    rgba[3] = 255;
    return;
  }

  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];

  for (int s = 0; s < g->n_stops; s++)
  {
    stop      = &g->stops[s];
    next_stop = &g->stops[s+1];
    if (s + 1 >= g->n_stops) next_stop = NULL;
    if (v >= stop->pos && next_stop && v < next_stop->pos)
      break;
    stop = NULL;
    next_stop = NULL;
  }
  if (stop == NULL && next_stop)
  {
    for (int c = 0; c < 4; c++)
      rgba[c] = next_stop->rgba[c];
  }
  else if (stop && next_stop == NULL)
  {
    for (int c = 0; c < 4; c++)
      rgba[c] = stop->rgba[c];
  }
  else if (stop && next_stop)
  {
    int dx = (v - stop->pos) * 255 / (next_stop->pos - stop->pos);
    for (int c = 0; c < 4; c++)
      rgba[c] = ctx_lerp_u8 (stop->rgba[c], next_stop->rgba[c], dx);
  }
  else
  {
    for (int c = 0; c < 4; c++)
      rgba[c] = g->stops[g->n_stops-1].rgba[c];
  }
}


static void
ctx_sample_source_u8_image (CtxRenderer *renderer, float x, float y, uint8_t *rgba)
{
  CtxSource *g = &renderer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (renderer);
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
    int bpp = buffer->format->bpp/8;
    uint8_t *src = (uint8_t*)buffer->data;
    src += v * buffer->stride + u * bpp;
    switch (bpp)
    {
      case 1:
        for (int c = 0; c < 3; c++)
          rgba[c] = src[0];
        rgba[3] = 255;
      break;
      case 2:
        for (int c = 0; c < 3; c++)
          rgba[c] = src[0];
        rgba[3] = src[1];
      break;
      case 3:
        for (int c = 0; c < 3; c++)
          rgba[c] = src[c];
        rgba[3] = 255;
      break;
      case 4:
        for (int c = 0; c < 4; c++)
          rgba[c] = src[c];
      break;
    }
  }
}

#if CTX_DITHER
static inline int ctx_dither_mask_a (int x, int y, int c, int divisor)
{
  /* https://pippin.gimp.org/a_dither/ */
  return (((((x + c * 67) + y * 236) * 119) & 255 )-127) / divisor;
}

static inline void ctx_dither_rgba_u8 (uint8_t *rgba, int x, int y, int dither_red_blue, int dither_green)
{
  if (dither_red_blue == 0)
    return;
  for (int c = 0; c < 3; c ++)
  {
    int val = rgba[c] + ctx_dither_mask_a (x, y, 0, c==1?dither_green:dither_red_blue);
    rgba[c] = CTX_CLAMP (val, 0, 255);
  }
}
#endif

static void
ctx_sample_source_u8_image_rgba (CtxRenderer *renderer, float x, float y, uint8_t *rgba)
{
  CtxSource *g = &renderer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (renderer);
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
    uint8_t *src = (uint8_t*)buffer->data;
    src += v * buffer->stride + u * bpp;
    for (int c = 0; c < 4; c++)
      rgba[c] = src[c];
  }
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, renderer->format->dither_red_blue,
                                  renderer->format->dither_green);
#endif
}

static void
ctx_sample_source_u8_image_1bit (CtxRenderer *renderer, float x, float y, uint8_t *rgba)
{
  CtxSource *g = &renderer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (renderer);
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
    uint8_t *src = (uint8_t*)buffer->data;
    src += v * buffer->stride + u / 8;
    if (*src & (1<< (u & 7)))
    {
      rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
    }
    else
    {
      for (int c = 0; c < 4; c++)
        rgba[c] = g->image.rgba[c];
    }
  }
}

static void
ctx_sample_source_u8_image_rgb (CtxRenderer *renderer, float x, float y, uint8_t *rgba)
{
  CtxSource *g = &renderer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (renderer);
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
    int bpp = 3;
    uint8_t *src = (uint8_t*)buffer->data;
    src += v * buffer->stride + u * bpp;
    for (int c = 0; c < 3; c++)
      rgba[c] = src[c];
    rgba[3] = 255;
  }
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, renderer->format->dither_red_blue,
                                  renderer->format->dither_green);
#endif
}


static void
ctx_sample_source_u8_radial_gradient (CtxRenderer *renderer, float x, float y, uint8_t *rgba)
{
  CtxSource *g = &renderer->state->gstate.source;
  float v = 0.0f;
  if (g->radial_gradient.r0 == 0.0f ||
      (g->radial_gradient.r1-g->radial_gradient.r0) < 0.0f)
  {
  }
  else
  {
    v = hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
    v = (v - g->radial_gradient.r0) /
        (g->radial_gradient.r1 - g->radial_gradient.r0);
  }
  ctx_sample_gradient_1d_u8 (renderer, v, rgba);

#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, renderer->format->dither_red_blue,
                                  renderer->format->dither_green);
#endif
}


static void
ctx_sample_source_u8_linear_gradient (CtxRenderer *renderer, float x, float y, uint8_t *rgba)
{
  CtxSource *g = &renderer->state->gstate.source;
  float v = (((g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
            g->linear_gradient.length) -
            g->linear_gradient.start) /
                (g->linear_gradient.end - g->linear_gradient.start);
  ctx_sample_gradient_1d_u8 (renderer, v, rgba);

#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, renderer->format->dither_red_blue,
                                  renderer->format->dither_green);
#endif
}

static void
ctx_sample_source_u8_color (CtxRenderer *renderer, float x, float y, uint8_t *rgba)
{
  CtxSource *g = &renderer->state->gstate.source;
  rgba[0] = g->color.rgba[0];
  rgba[1] = g->color.rgba[1];
  rgba[2] = g->color.rgba[2];
  rgba[3] = g->color.rgba[3];
}

typedef void (*CtxSourceU8)(CtxRenderer *renderer, float x, float y, uint8_t *rgba);

static CtxSourceU8 ctx_renderer_get_source_u8 (CtxRenderer *renderer)
{
  CtxGState *gstate = &renderer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (gstate->source.type)
  {
    case CTX_SOURCE_IMAGE:
      switch (buffer->format->bpp)
      {
        case 1:
          return ctx_sample_source_u8_image_1bit;
        //case 2:
        //  return ctx_sample_source_u8_image_2bit;
        case 24:
          return ctx_sample_source_u8_image_rgb;
        case 32:
          return ctx_sample_source_u8_image_rgba;
        default:
          return ctx_sample_source_u8_image;
      }
    case CTX_SOURCE_COLOR:
      return ctx_sample_source_u8_color;
    case CTX_SOURCE_LINEAR_GRADIENT:
      return ctx_sample_source_u8_linear_gradient;
    case CTX_SOURCE_RADIAL_GRADIENT:
      return ctx_sample_source_u8_radial_gradient;
  }
  return ctx_sample_source_u8_color;
}

static int
ctx_b2f_over_RGBA8 (CtxRenderer *renderer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  CtxGState *gstate = &renderer->state->gstate;
  /*
   */
  uint8_t color[4];

  if (gstate->source.type != CTX_SOURCE_COLOR)
  {
    CtxSourceU8 source = ctx_renderer_get_source_u8 (renderer);
    float y = renderer->scanline / CTX_RASTERIZER_AA;
    for (int x = 0; x < count; x++)
    {
      uint8_t cov = coverage[x];
      if (cov)
      {
        source (renderer, x0 + x, y, &color[0]);
        if (color[3])
        {
          color[3] = (color[3] * gstate->source.global_alpha)>>8;
          color[0] = (color[0] * color[3])>>8;
          color[1] = (color[1] * color[3])>>8;
          color[2] = (color[2] * color[3])>>8;

          uint8_t ralpha = 255 - ((cov * color[3]) >> 8);
          for (int c = 0; c < 4; c++)
            dst[c] = (color[c]*cov + dst[c] * ralpha) >> 8;
        }
      }
      dst+=4;
    }
    return count;
  }

  color[3] = (gstate->source.color.rgba[3] * gstate->source.global_alpha)>>8;
  color[0] = gstate->source.color.rgba[0];
  color[1] = gstate->source.color.rgba[1];
  color[2] = gstate->source.color.rgba[2];

  if (color[3] == 255)
  {
    for (int x = 0; x < count; x++)
    {
      int cov = coverage[x];
      if (cov == 255)
      {
        dst[0] = color[0];
        dst[1] = color[1];
        dst[2] = color[2];
        dst[3] = color[3];
      }
      else if (cov)
      {
        int ralpha = 255 - ((cov * color[3]) >> 8);
        for (int c = 0; c < 4; c++)
          dst[c] = (color[c]*cov + dst[c] * ralpha) >> 8;
      }
      dst+=4;
    }
    return count;
  }

  color[0] = (color[0] * color[3])>>8;
  color[1] = (color[1] * color[3])>>8;
  color[2] = (color[2] * color[3])>>8;

  {
    for (int x = 0; x < count; x++)
    {
      uint8_t cov = coverage[x];
      if (cov)
      {
        uint8_t ralpha = 255 - ((cov * color[3]) >> 8);
        for (int c = 0; c < 4; c++)
          dst[c] = (color[c]*cov + dst[c] * ralpha) >> 8;
      }
      dst+=4;
    }
  }
  return count;
}


static int
ctx_b2f_over_RGBA8_convert (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  renderer->format->to_rgba8 (renderer, x, dst, &pixels[0], count);
  ret = ctx_b2f_over_RGBA8 (renderer, x, &pixels[0], coverage, count);
  renderer->format->from_rgba8 (renderer, x, dst, &pixels[0], count);
  return ret;
}

#if CTX_ENABLE_BGRA8

static inline void
ctx_swap_red_green (uint8_t *rgba)
{
  uint32_t *buf = (uint32_t*) rgba;
  uint32_t orig = *buf;
  uint32_t green_alpha = (orig & 0xff00ff00);
  uint32_t red_blue    = (orig & 0x00ff00ff);
  uint32_t red         = red_blue << 16;
  uint32_t blue        = red_blue >> 16;
  *buf = green_alpha | red | blue;
}

static inline void
ctx_decode_pixels_BGRA8(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  uint32_t *srci = (uint32_t*)buf;
  uint32_t *dsti = (uint32_t*)rgba;

  while (count--)
  {
    uint32_t val = *srci++;
    ctx_swap_red_green ((uint8_t*)&val);
    *dsti++      = val;
  }
}

static inline void
ctx_encode_pixels_BGRA8 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  ctx_decode_pixels_BGRA8(renderer, x, rgba, (uint8_t*)buf, count);
}

static int
ctx_b2f_over_BGRA8 (CtxRenderer *renderer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  CtxGState *gstate = &renderer->state->gstate;
  uint8_t color[4];

  if (gstate->source.type != CTX_SOURCE_COLOR)
  {
    float y = renderer->scanline / CTX_RASTERIZER_AA;
    CtxSourceU8 source = ctx_renderer_get_source_u8 (renderer);
    for (int x = 0; x < count; x++)
    {
      uint8_t cov = coverage[x];
      if (cov)
      {
        uint8_t scolor[4];
        source (renderer, x0 + x, y, &scolor[0]);

        if (scolor[3])
        {
          color[3] = (scolor[3] * gstate->source.global_alpha)>>8;
          color[2] = (scolor[0] * color[3])>>8;
          color[1] = (scolor[1] * color[3])>>8;
          color[0] = (scolor[2] * color[3])>>8;

          uint8_t ralpha = 255 - ((cov * color[3]) >> 8);
          for (int c = 0; c < 4; c++)
            dst[c] = (color[c]*cov + dst[c] * ralpha) >> 8;
        }
      }
      dst+=4;
    }
    return count;
  }

  color[3] = (gstate->source.color.rgba[3] * gstate->source.global_alpha)>>8;
  color[0] = gstate->source.color.rgba[2];
  color[1] = gstate->source.color.rgba[1];
  color[2] = gstate->source.color.rgba[0];

  if (color[3] == 255)
  {
    for (int x = 0; x < count; x++)
    {
      int cov = coverage[x];
      if (cov == 255)
      {
        dst[0] = color[0];
        dst[1] = color[1];
        dst[2] = color[2];
        dst[3] = color[3];
      }
      else if (cov)
      {
        int ralpha = 255 - ((cov * color[3]) >> 8);
        for (int c = 0; c < 4; c++)
          dst[c] = (color[c]*cov + dst[c] * ralpha) >> 8;
      }
      dst+=4;
    }
    return count;
  }

  color[0] = (color[0] * color[3])>>8;
  color[1] = (color[1] * color[3])>>8;
  color[2] = (color[2] * color[3])>>8;

  {
    for (int x = 0; x < count; x++)
    {
      uint8_t cov = coverage[x];
      if (cov)
      {
        uint8_t ralpha = 255 - ((cov * color[3]) >> 8);
        for (int c = 0; c < 4; c++)
          dst[c] = (color[c]*cov + dst[c] * ralpha) >> 8;
      }
      dst+=4;
    }
  }
  return count;
}

#endif


#if CTX_ENABLE_GRAYF
static int
ctx_gray_float_b2f_over (CtxRenderer *renderer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  float *dst_f = (float*)dst;
  float y = renderer->scanline / CTX_RASTERIZER_AA;
  const uint8_t *color = renderer->state->gstate.source.color.rgba;
  float gray = (color[0]+color[1]+color[2])/3.0/255.0;
  float alpha = color[3]/255.0 + renderer->state->gstate.source.global_alpha/255.0;

  CtxSourceU8 source = ctx_renderer_get_source_u8 (renderer);
  if (source == ctx_sample_source_u8_color) source = NULL;

  for (int x = 0; x < count; x++)
  {
    int cov = coverage[x];
    if (cov != 0)
    {
      if (source)
      {
        uint8_t scolor[4];
        source (renderer, x0 + x, y, &scolor[0]);

        gray = ((scolor[0]+scolor[1]+scolor[2])/3.0)/255.0;
        alpha = scolor[3]/255.0 + renderer->state->gstate.source.global_alpha/255.0;
      }

      float ralpha = 1.0f - alpha * cov/255.0;
      dst_f[0] = gray * cov/255.0 + dst_f[0] * ralpha;
    }
    dst_f+=1;
  }
  return count;
}
#endif


#if CTX_ENABLE_RGBAF
static int
ctx_associated_float_b2f_over (CtxRenderer *renderer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  int   components = 4;  // this makes it mostly adapted to become
                         // a generalized floating point ver..
                         // for components == 1-4 assume RGB color source
                         // for more components - use alternate generic
                         // source setting, which permits operation with
                         // non RGB color models.
  float *dst_f = (float*)dst;
  float y = renderer->scanline / CTX_RASTERIZER_AA;
  const uint8_t *color = renderer->state->gstate.source.color.rgba;
  float color_f[components];
  for (int c = 0; c < components; c++)
    color_f[c]=color[c]/255.0f;  // XXX ; lacks gamma
  color_f[components-1] *= (renderer->state->gstate.source.color.rgba[3]/255.0f);
  for (int c = 0; c < components-1; c++)
    color_f[c] *= color_f[components-1];

  CtxSourceU8 source = ctx_renderer_get_source_u8 (renderer);
  if (source == ctx_sample_source_u8_color) source = NULL;

  for (int x = 0; x < count; x++)
  {

    float cov    = coverage[x]/255.0f;
    if (cov != 0.0f)
    {
      if (source)
      {
        uint8_t scolor[4];
        source (renderer, x0 + x, y, &scolor[0]);
        for (int c = 0; c < components; c++)
          color_f[c]=scolor[c]/255.0f;  // XXX ; lacks gamma

        color_f[3] *= (scolor[components-1] * renderer->state->gstate.source.global_alpha/255.0f);
        for (int c = 0; c < components-1; c++)
          color_f[c] *= color_f[components-1];
      }
      float ralpha = 1.0f - color_f[components-1] * cov;
      for (int c = 0; c < components; c++)
        dst_f[c] = color_f[c] * cov + dst_f[c] * ralpha;
    }
    dst_f += components;
  }
  return count;
}
#endif

static inline int
ctx_renderer_apply_coverage (CtxRenderer *renderer,
                             uint8_t     *dst,
                             int          x,
                             uint8_t     *coverage,
                             int          count)
{
  return renderer->format->crunch (renderer, x, dst, coverage, count);
}

static void
ctx_renderer_fill_active_edges (CtxRenderer *renderer,
                                void        *scanline_start,
                                uint8_t     *coverage,
                                int          winding,
                                int          aa)
{
  int scanline     = renderer->scanline;
  int active_edges = renderer->active_edges;
  int blit_min     = renderer->blit_x;
  int blit_max     = renderer->blit_width - 1;
  int parity = 0;

#define CTX_EDGE(no)      renderer->edge_list.entries[renderer->edges[no].index]
#define CTX_EDGE_YMIN(no) CTX_EDGE(no).data.s16[1]
#define CTX_EDGE_YMAX(no) CTX_EDGE(no).data.s16[3]
#define CTX_EDGE_SLOPE(no) renderer->edges[no].dx
#define CTX_EDGE_X(no)     (renderer->edges[no].x)

  {
    int x = CTX_EDGE_X(0) / CTX_SUBDIV / CTX_RASTERIZER_EDGE_MULTIPLIER ;

    if (x < blit_min) x = blit_min;

    if (x < renderer->min_x)
      renderer->min_x = x;
  }

  for (int t = 0; t < active_edges -1;)
  {
    int ymin = CTX_EDGE_YMIN(t);
    int next_t = t + 1;

    if (scanline != ymin)
    {
      if (winding)
        parity += ((CTX_EDGE(t).code == CTX_EDGE_FLIPPED)?1:-1);
      else
        parity = 1 - parity;
    }

    if (parity)
    {
      int x0 = CTX_EDGE_X(t) / CTX_SUBDIV ;
      int x1 = CTX_EDGE_X(next_t) / CTX_SUBDIV ;

      if ((x0 < x1))
      {
        int first = x0 / CTX_RASTERIZER_EDGE_MULTIPLIER;
        int last  = x1 / CTX_RASTERIZER_EDGE_MULTIPLIER;
        if (first < 0)
          first = 0;
        if (first > blit_max)
          first = blit_max;
        if (last > blit_max)
          last = blit_max;
        if (last < 0)
          last = 0;

        int graystart = 255-((x0 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff);
        int grayend   = (x1 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff;

        if (aa)
        {
          if ((first!=last) && graystart)
          {
            int cov = coverage[first] + graystart / CTX_RASTERIZER_AA;
            coverage[first] = CTX_MIN(cov,255);
            first++;
          }
          for (int x = first; x < last; x++)
          {
            int cov = coverage[x] + 255 / CTX_RASTERIZER_AA;
            coverage[x] = CTX_MIN(cov,255);
          }
          if (grayend) {
            int cov = coverage[last] + grayend / CTX_RASTERIZER_AA;
            coverage[last] = CTX_MIN(cov,255);
          }
        }
        else
        {
          if ((first!=last) && graystart)
          {
            int cov = coverage[first] + graystart;
            coverage[first] = CTX_MIN(cov,255);
            first++;
          }
          for (int x = first; x < last; x++)
          {
            coverage[x] = 255;
          }
          if (grayend)
          {
            int cov = coverage[last] + grayend;
            coverage[last] = CTX_MIN(cov,255);
          }
        }
        if (blit_min + last + 1> renderer->max_x) renderer->max_x = blit_min + last + 1;
      }
   }
   t = next_t;
 }
}

#undef CTX_EDGE_Y0
#undef CTX_EDGE

static void
ctx_renderer_reset (CtxRenderer *renderer)
{
  renderer->active_edges = 0;
  renderer->pending_edges = 0;
  renderer->has_shape = 0;
  renderer->has_prev = 0;
  renderer->edge_list.count = 0; // ready for new edges
  renderer->edge_pos = 0;
  renderer->needs_aa = 0;
  renderer->scanline = 0;
  renderer->scan_min = 5000;
  renderer->scan_max = -5000;
}

static inline void
ctx_renderer_rasterize_edges (CtxRenderer *renderer, int winding)
{
  uint8_t *dst = ((uint8_t*)renderer->buf);
  int scan_start = renderer->blit_y * CTX_RASTERIZER_AA;
  int scan_end =   scan_start + renderer->blit_height * CTX_RASTERIZER_AA;
  int blit_width = renderer->blit_width;
  int blit_max_x = renderer->blit_x + blit_width;

    if (scan_end < renderer->blit_y * CTX_SUBDIV ||
        scan_start > (renderer->blit_y + renderer->blit_height) * CTX_SUBDIV)
    {
      ctx_renderer_reset (renderer);
      return;
    }

  renderer->scan_min -= (renderer->scan_min % CTX_RASTERIZER_AA);

  if (renderer->scan_min > scan_start)
  {
    dst += (renderer->blit_stride * (renderer->scan_min/CTX_RASTERIZER_AA-scan_start/CTX_RASTERIZER_AA));
    scan_start = renderer->scan_min;
  }
  if (renderer->scan_max < scan_end) scan_end = renderer->scan_max;

  ctx_renderer_sort_edges (renderer);

  for (renderer->scanline = scan_start; renderer->scanline < scan_end;)
  {
    renderer->min_x = blit_width;
    renderer->max_x = 0;

    uint8_t coverage[blit_width];
    for (int i = 0; i < blit_width/4; i++)
    {
      ((int32_t*)coverage)[i]=0;
    }

    ctx_renderer_feed_edges (renderer);
    ctx_renderer_discard_edges (renderer);


#if CTX_RASTERIZER_FORCE_AA==1
    renderer->needs_aa = 1;
#endif
    if (renderer->needs_aa         // due to slopes of active edges
#if CTX_RASTERIZER_AUTOHINT==0
   || renderer->lingering_edges    // or due to edges ...
   || renderer->pending_edges      //   ... that start or end within scanline
#endif
     )
    {
      for (int i = 0; i < CTX_RASTERIZER_AA; i++)
      {
        ctx_renderer_sort_active_edges (renderer);
        ctx_renderer_fill_active_edges (renderer, dst, coverage, winding, 1);
        renderer->scanline ++;
        ctx_renderer_increment_edges (renderer, 1);
        if (i!=CTX_RASTERIZER_AA-1) {
          ctx_renderer_feed_edges (renderer);
          ctx_renderer_discard_edges (renderer);
        }
      }
    }
    else
    {
      renderer->scanline += CTX_RASTERIZER_AA2 + 1;
      ctx_renderer_increment_edges (renderer, CTX_RASTERIZER_AA2 + 1);
      ctx_renderer_feed_edges (renderer);
      ctx_renderer_discard_edges (renderer);
      ctx_renderer_sort_active_edges (renderer);
      ctx_renderer_fill_active_edges (renderer, dst, coverage, winding, 0);
      renderer->scanline += CTX_RASTERIZER_AA2;
      ctx_renderer_increment_edges (renderer, CTX_RASTERIZER_AA2);
    }

    {
      int minx = renderer->min_x - renderer->blit_x;
      int maxx = renderer->max_x - renderer->blit_x;
      if (maxx > blit_max_x - 1)
        maxx = blit_max_x - 1;
      if (minx < maxx)
      {
        ctx_renderer_apply_coverage (renderer,
                                     &dst[(minx) * renderer->format->bpp/8],
                                     minx,
                                     &coverage[minx], maxx-minx);
      }
    }
    dst += renderer->blit_stride;
  }
  ctx_renderer_reset (renderer);
}

static inline void
ctx_renderer_fill (CtxRenderer *renderer)
{
  if (renderer->scan_min / CTX_RASTERIZER_AA > renderer->blit_y + renderer->blit_height ||
      renderer->scan_max / CTX_RASTERIZER_AA < renderer->blit_y)
  {
    ctx_renderer_reset (renderer);
    return;
  }

  ctx_renderer_finish_shape (renderer);
  ctx_renderer_poly_to_edges (renderer);
  ctx_renderer_rasterize_edges (renderer, renderer->state->gstate.fill_rule);
}


static void
ctx_renderer_arc (CtxRenderer *renderer,
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
    full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS;

  float step = CTX_PI*2.0/full_segments;
  int steps;

  if ((!anticlockwise && (end_angle - start_angle >= CTX_PI*2)) ||
     ((anticlockwise && (start_angle - end_angle >= CTX_PI*2))))
  {
    end_angle = start_angle;
    steps = full_segments - 1;
  }
  else
  {
    if (anticlockwise)
    {
      steps = full_segments - (end_angle - start_angle) / (CTX_PI*2) * full_segments;
    }
    else
      steps = (end_angle - start_angle) / (CTX_PI*2) * full_segments;
  }

  if (anticlockwise) step = step * -1;

  int first = 1;

  if (steps == 0 || (anticlockwise && steps == full_segments))
  {
    ctx_renderer_line_to (renderer, x + cosf (end_angle) * radius,
                                    y + sinf (end_angle) * radius);
  }
  else
  {

    for (float angle = start_angle, i = 0; i < steps; angle += step, i++)
    {
      float xv = x + cosf (angle) * radius;
      float yv = y + sinf (angle) * radius;
      if (first && !renderer->has_prev)
        ctx_renderer_move_to (renderer, xv, yv);
      else
        ctx_renderer_line_to (renderer, xv, yv);
      first = 0;
    }

    ctx_renderer_line_to (renderer, x + cosf (end_angle) * radius,
                          y + sinf (end_angle) * radius);
  }
}

static void
ctx_renderer_quad_to (CtxRenderer *renderer,
                      float        cx,
                      float        cy,
                      float        x,
                      float        y)
{
  ctx_renderer_curve_to (renderer,
    (cx * 2 + renderer->x) / 3.0f, (cy * 2 + renderer->y) / 3.0f,
    (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
    x,                              y);
}

static inline void
ctx_renderer_rel_quad_to (CtxRenderer *renderer,
                          float cx, float cy, float x, float y)
{
  ctx_renderer_quad_to (renderer, cx + renderer->x, cy + renderer->y,
                                  x  + renderer->x, y  + renderer->y);
}

#define LENGTH_OVERSAMPLE 4
static void
ctx_renderer_pset (CtxRenderer *renderer, int x, int y)
{
     // XXX - we avoid rendering here x==0 - to keep with
     //  an off-by one elsewhere
  if (x <= 0 || y < 0 || x >= renderer->blit_width ||
                         y >= renderer->blit_height)
    return;
  uint8_t *fg_color = renderer->state->gstate.source.color.rgba;
  uint8_t pixel[4];
  uint8_t *dst = ((uint8_t*)renderer->buf);
  dst += y * renderer->blit_stride;
  dst += x * renderer->format->bpp / 8;

  renderer->format->to_rgba8 (renderer, x, dst, &pixel[0], 1);
  for (int c = 0; c < 3; c++)
  {
    pixel[c] = fg_color[c];
  }
  renderer->format->from_rgba8 (renderer, x, dst, &pixel[0], 1);
}

static void
ctx_renderer_stroke_1px (CtxRenderer *renderer)
{
  int count = renderer->edge_list.count;
  CtxEntry *temp = renderer->edge_list.entries;

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
        prev_y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;
        started = 1;
        start = i;
      }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;
      int dx = x - prev_x;
      int dy = y - prev_y;

      int length = CTX_MAX(abs(dx), abs(dy));

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
          ctx_renderer_pset (renderer, tx/256, ty/256);
          tx += dx;
          ty += dy;
          ctx_renderer_pset (renderer, tx/256, ty/256);
        }
      }

      prev_x = x;
      prev_y = y;
    }
    end = i-1;
foo:
    start = end+1;
  }

  ctx_renderer_reset (renderer);
}

static void
ctx_renderer_stroke (CtxRenderer *renderer)
{
  if (renderer->state->gstate.line_width <= 0.0f &&
      renderer->state->gstate.line_width > -10.0f)
  {
    ctx_renderer_stroke_1px (renderer);
    return;
  }

  int count = renderer->edge_list.count;
  CtxEntry temp[count]; /* copy of already built up path's poly line  */
  memcpy (temp, renderer->edge_list.entries, sizeof (temp));
  ctx_renderer_reset (renderer); /* then start afresh with our stroked shape  */

  CtxMatrix transform_backup = renderer->state->gstate.transform;

  ctx_matrix_identity (&renderer->state->gstate.transform);

  float prev_x = 0.0f;
  float prev_y = 0.0f;
  float half_width_x = renderer->state->gstate.line_width/2;
  float half_width_y = renderer->state->gstate.line_width/2;
  if (renderer->state->gstate.line_width <= 0.0f)
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
        prev_y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;
        started = 1;
        start = i;
      }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;
      float dx = x - prev_x;
      float dy = y - prev_y;

      float length = ctx_fast_hypotf (dx, dy);

      if (length>0.001f)
      {
        dx = dx/length * half_width_x;
        dy = dy/length * half_width_y;

        if (entry->code == CTX_NEW_EDGE)
        {
           ctx_renderer_finish_shape (renderer);
           ctx_renderer_move_to (renderer, prev_x+dy, prev_y-dx);
        }
        ctx_renderer_line_to (renderer, prev_x-dy, prev_y+dx);
        // XXX possible miter line-to
        ctx_renderer_line_to (renderer, x-dy, y+dx);
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
      y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;

      dx = x - prev_x;
      dy = y - prev_y;

      float length = ctx_fast_hypotf(dx, dy);

      dx = dx/length * half_width_x;
      dy = dy/length * half_width_y;

      if (length>0.001f)
      {
        ctx_renderer_line_to (renderer, prev_x-dy, prev_y+dx);
        // XXX possible miter line-to 
        ctx_renderer_line_to (renderer, x-dy,      y+dx);
      }

      prev_x = x;
      prev_y = y;

      if (entry->code == CTX_NEW_EDGE)
      {
         x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
         y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;

         dx = x - prev_x;
         dy = y - prev_y;
         length = ctx_fast_hypotf(dx, dy);
         if (length>0.001f)
         {

           dx = dx/length * half_width_x;
           dy = dy/length * half_width_y;

           ctx_renderer_line_to (renderer, prev_x-dy, prev_y+dx);
           ctx_renderer_line_to (renderer, x-dy, y+dx);
         }
      }

      if ((prev_x != x) && (prev_y != y))
      {
        prev_x = x;
        prev_y = y;
      }
    }

    start = end+1;
  }
  ctx_renderer_finish_shape (renderer);

  switch(renderer->state->gstate.line_cap)
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
             ctx_renderer_arc (renderer, x, y, half_width_x, 3.1415*3, 0, 1);
             ctx_renderer_finish_shape (renderer);
           }

           x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
           y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;

           ctx_renderer_arc (renderer, x, y, half_width_x, 3.1415*3, 0, 1);
           ctx_renderer_finish_shape (renderer);
         }

         x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
         y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;
         has_prev = 1;
       }
       ctx_renderer_move_to (renderer, x, y);
       ctx_renderer_arc (renderer, x, y, half_width_x, 3.1415*3, 0, 1);
       ctx_renderer_finish_shape (renderer);
      break;
    }
  }

  switch(renderer->state->gstate.line_join)
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
         y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;

         if (entry[1].code == CTX_EDGE)
         {
           ctx_renderer_arc (renderer, x, y, half_width_x, 3.1415*3, 0, 1);
           ctx_renderer_finish_shape (renderer);
         }
       }
      break;
    }
  }

  ctx_renderer_poly_to_edges    (renderer);
  ctx_renderer_rasterize_edges (renderer, 1);

  renderer->state->gstate.transform = transform_backup;
}

static void
ctx_renderer_clip (CtxRenderer *renderer)
{
  // XXX render to temporary 8bit gray, convert to RLE mask

  int count = renderer->edge_list.count;

  float minx = 5000.0f;
  float miny = 5000.0f;
  float maxx = -5000.0f;
  float maxy = -5000.0f;
  float prev_x = 0.0f;
  float prev_y = 0.0f;

  for (int i = 0; i < count; i++)
  {
    CtxEntry *entry = &renderer->edge_list.entries[i];
    float x, y;
    if (entry->code == CTX_NEW_EDGE)
    {
      prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
      prev_y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;
    }
    x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
    y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;

    if (x < minx) minx = x;
    if (prev_x < minx) minx = prev_x;
    if (y < miny) miny = y;
    if (prev_y < miny) miny = prev_y;
    if (x > maxx) maxx = x;
    if (prev_x > maxx) maxx = prev_x;
    if (y > maxy) maxy = y;
    if (prev_y > maxy) maxy = prev_y;

    prev_x = x;
    prev_y = y;
  }
}

static void
ctx_renderer_load_image (CtxRenderer *renderer, const char *path,
                         float x, float y)
{
  // decode PNG, put it in image is slot 1,
  // magic width height stride format data
  ctx_buffer_load_png (&renderer->texture[0], path);
  renderer->state->gstate.source.type = CTX_SOURCE_IMAGE;
  renderer->state->gstate.source.image.buffer = &renderer->texture[0];
  renderer->state->gstate.source.image.x0 = x;
  renderer->state->gstate.source.image.y0 = y;
}

static void
ctx_renderer_process (CtxRenderer *renderer, CtxEntry *entry)
{
  //fprintf (stderr, "%c(%.1f %.1f %i)", entry->code, renderer->x,renderer->y, renderer->has_prev);
  switch (entry->code)
  {
    case CTX_LINE_TO:
      ctx_renderer_line_to (renderer, ctx_arg_float(0), ctx_arg_float(1));
      break;

    case CTX_REL_LINE_TO:
      ctx_renderer_rel_line_to (renderer, ctx_arg_float(0), ctx_arg_float(1));
      break;

    case CTX_MOVE_TO:
      ctx_renderer_move_to (renderer, ctx_arg_float(0), ctx_arg_float(1));
      break;

    case CTX_REL_MOVE_TO:
      ctx_renderer_rel_move_to (renderer, ctx_arg_float(0), ctx_arg_float(1));
      break;

    case CTX_CURVE_TO:
      ctx_renderer_curve_to (renderer, ctx_arg_float(0), ctx_arg_float(1),
                                       ctx_arg_float(2), ctx_arg_float(3),
                                       ctx_arg_float(4), ctx_arg_float(5));
      break;

    case CTX_REL_CURVE_TO:
      ctx_renderer_rel_curve_to (renderer,
                                 ctx_arg_float(0), ctx_arg_float(1),
                                 ctx_arg_float(2), ctx_arg_float(3),
                                 ctx_arg_float(4), ctx_arg_float(5));
      break;

    case CTX_QUAD_TO:
      ctx_renderer_quad_to (renderer, ctx_arg_float(0), ctx_arg_float(1),
                                      ctx_arg_float(2), ctx_arg_float(3));
      break;

    case CTX_REL_QUAD_TO:
      ctx_renderer_rel_quad_to (renderer,
                                ctx_arg_float(0), ctx_arg_float(1),
                                ctx_arg_float(2), ctx_arg_float(3));
      break;

    case CTX_ARC:
      ctx_renderer_arc (renderer,
                        ctx_arg_float(0), ctx_arg_float(1),
                        ctx_arg_float(2), ctx_arg_float(3),
                        ctx_arg_float(4), ctx_arg_float(5));
      break;

    case CTX_RECTANGLE:
      ctx_renderer_move_to (renderer, ctx_arg_float(0), ctx_arg_float(1));
      ctx_renderer_rel_line_to (renderer, ctx_arg_float(2), 0);
      ctx_renderer_rel_line_to (renderer, 0, ctx_arg_float(3));
      ctx_renderer_rel_line_to (renderer, -ctx_arg_float(2), 0);
      break;

    case CTX_BLIT_RECT:
      ctx_renderer_move_to (renderer, ctx_arg_s16(0), ctx_arg_s16(1));
      ctx_renderer_rel_line_to (renderer, ctx_arg_s16(2), 0);
      ctx_renderer_rel_line_to (renderer, 0, ctx_arg_s16(3));
      ctx_renderer_rel_line_to (renderer, -ctx_arg_s16(2), 0);
      ctx_renderer_fill (renderer);
      break;
    case CTX_LOAD_IMAGE:
      ctx_renderer_load_image (renderer,
                    (char*)&entry[2].data.u8[0],
                    ctx_arg_float(0), ctx_arg_float(1));
      break;

    case CTX_GRADIENT_NO:
      ctx_renderer_set_gradient_no (renderer, entry[0].data.u8[0]);
      break;

    case CTX_GRADIENT_CLEAR:
      ctx_renderer_gradient_clear_stops (renderer);
      break;

    case CTX_GRADIENT_STOP:
      ctx_renderer_gradient_add_stop (renderer, ctx_arg_float(0),
                                               &ctx_arg_u8(4));
      break;

    case CTX_LINEAR_GRADIENT:
      ctx_renderer_linear_gradient (renderer,
                           ctx_arg_float(0), ctx_arg_float(1),
                           ctx_arg_float(2), ctx_arg_float(3));
      break;
    case CTX_RADIAL_GRADIENT:
      ctx_renderer_radial_gradient (renderer,
                           ctx_arg_float(0), ctx_arg_float(1), ctx_arg_float(2),
                           ctx_arg_float(3), ctx_arg_float(4), ctx_arg_float(5));
      break;

    case CTX_ROTATE:
    case CTX_SCALE:
    case CTX_TRANSLATE:
    case CTX_SAVE:
    case CTX_RESTORE:
      renderer->uses_transforms = 1;
      ctx_interpret_transforms (renderer->state, entry, NULL);
      break;

    case CTX_STROKE:
      ctx_renderer_stroke (renderer);
      break;

    case CTX_FILL:
      ctx_renderer_fill (renderer);
      break;

    case CTX_NEW_PATH:
      ctx_renderer_reset (renderer);
      break;

    case CTX_CLIP:
      ctx_renderer_clip (renderer);
      break;

    case CTX_CLOSE_PATH:
      ctx_renderer_finish_shape (renderer);
      break;
  }
  ctx_interpret_pos_bare (renderer->state, entry, NULL);
  ctx_interpret_style (renderer->state, entry, NULL);
}


#if CTX_ENABLE_RGB8

static inline void
ctx_decode_pixels_RGB8(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (const uint8_t*)buf;
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

static inline void
ctx_encode_pixels_RGB8 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint8_t *pixel = (uint8_t*)buf;
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
static inline void
ctx_decode_pixels_GRAY1(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    if (*pixel & (x%8))
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
    if ((x%8)==7)
      pixel+=1;
    x++;
    rgba +=4;
  }
}

static inline void
ctx_encode_pixels_GRAY1 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    int gray = (rgba[0]+rgba[1]+rgba[2])/3 ;

    //gray += ctx_dither_mask_a (x, renderer->scanline/CTX_RASTERIZER_AA, 0, 127);

    if (gray < 127)
    {
      *pixel = *pixel & (~(1<< (x%8)));
    }
    else
    {
      *pixel = *pixel | (1<< (x%8));
    }
    if ((x%8)==7)
      pixel+=1;
    x++;
    rgba +=4;
  }
}

static int
ctx_b2f_over_GRAY1 (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_decode_pixels_GRAY1 (renderer, x, dst, &pixels[0], count);
  ret = ctx_b2f_over_RGBA8 (renderer, x, &pixels[0], coverage, count);
  ctx_encode_pixels_GRAY1 (renderer, x, dst, &pixels[0], count);
  return ret;
}


#endif


#if CTX_ENABLE_GRAY2
static inline void
ctx_decode_pixels_GRAY2(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    int val = (*pixel & (3 << ((x % 4)*2)) ) >> ((x%4)*2);
    val <<= 6;
    rgba[0] = val;
    rgba[1] = val;
    rgba[2] = val;
    rgba[3] = 255;
    if ((x%4)==3)
      pixel+=1;
    x++;
    rgba +=4;
  }
}

static inline void
ctx_encode_pixels_GRAY2 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    int val = (rgba[0]+rgba[1]+rgba[2])/3 ;
    val >>= 6;
    *pixel = *pixel & (~(3 << ((x%4)*2)));
    *pixel = *pixel | ((val << ((x%4)*2)));
    if ((x%4)==3)
      pixel+=1;
    x++;
    rgba +=4;
  }
}

static int
ctx_b2f_over_GRAY2 (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_decode_pixels_GRAY2 (renderer, x, dst, &pixels[0], count);
  ret = ctx_b2f_over_RGBA8 (renderer, x, &pixels[0], coverage, count);
  ctx_encode_pixels_GRAY2 (renderer, x, dst, &pixels[0], count);
  return ret;
}


#endif


#if CTX_ENABLE_GRAY4
static inline void
ctx_decode_pixels_GRAY4(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    int val = (*pixel & (15 << ((x % 2)*4)) ) >> ((x%2)*4);
    val <<= 4;
    rgba[0] = val;
    rgba[1] = val;
    rgba[2] = val;
    rgba[3] = 255;
    if ((x%2)==1)
      pixel+=1;
    x++;
    rgba +=4;
  }
}

static inline void
ctx_encode_pixels_GRAY4 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    int val = (rgba[0]+rgba[1]+rgba[2])/3 ;
    val >>= 4;
    *pixel = *pixel & (~(15 << ((x%2)*4)));
    *pixel = *pixel | ((val << ((x%2)*4)));
    if ((x%2)==1)
      pixel+=1;
    x++;
    rgba +=4;
  }
}

static int
ctx_b2f_over_GRAY4 (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_decode_pixels_GRAY4 (renderer, x, dst, &pixels[0], count);
  ret = ctx_b2f_over_RGBA8 (renderer, x, &pixels[0], coverage, count);
  ctx_encode_pixels_GRAY4 (renderer, x, dst, &pixels[0], count);
  return ret;
}


#endif


#if CTX_ENABLE_GRAY8
static inline void
ctx_decode_pixels_GRAY8(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t*)buf;
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

static inline void
ctx_encode_pixels_GRAY8 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    pixel[0] = (rgba[0]+rgba[1]+rgba[2])/3;
    pixel+=1;
    rgba +=4;
  }
}
#endif

#if CTX_ENABLE_GRAYA8
static inline void
ctx_decode_pixels_GRAYA8(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (const uint8_t*)buf;
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

static inline void
ctx_encode_pixels_GRAYA8 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    pixel[0] = (rgba[0]+rgba[1]+rgba[2])/3;
    pixel[1] = rgba[3];
    pixel+=2;
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
    byteswapped = (pixel>>8)|(pixel<<8);
  else
    byteswapped  = pixel;
  *blue   = (byteswapped & 31)<<3;
  *green = ((byteswapped>>5) & 63)<<2;
  *red   = ((byteswapped>>11) & 31)<<3;
  if (*blue > 248) *blue = 255;
  if (*green > 248) *green = 255;
  if (*red > 248) *red = 255;
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
    return (c>>8)|(c<<8); /* swap bytes */
  return c;
}
#endif


#if CTX_ENABLE_RGB565
static inline void
ctx_decode_pixels_RGB565(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t*)buf;
  while (count--)
  {
    ctx_565_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2], 0);
    if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
      rgba[3] = 0;
    else
      rgba[3] = 255;
    pixel+=1;
    rgba +=4;
  }
}

static inline void
ctx_encode_pixels_RGB565 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint16_t *pixel = (uint16_t*)buf;
  while (count--)
  {
    if (rgba[3]==0)
      pixel[0] = ctx_565_pack (255, 0, 255, 0);
    else
      pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 0);
    pixel+=1;
    rgba +=4;
  }
}

static int
ctx_b2f_over_RGB565 (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_decode_pixels_RGB565 (renderer, x, dst, &pixels[0], count);
  ret = ctx_b2f_over_RGBA8 (renderer, x, &pixels[0], coverage, count);
  ctx_encode_pixels_RGB565 (renderer, x, dst, &pixels[0], count);
  return ret;
}


#endif

#if CTX_ENABLE_RGB565_BYTESWAPPED
static inline void
ctx_decode_pixels_RGB565_BS(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t*)buf;
  while (count--)
  {
    ctx_565_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2], 1);
    if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
      rgba[3] = 0;
    else
      rgba[3] = 255;
    pixel+=1;
    rgba +=4;
  }
}

static inline void
ctx_encode_pixels_RGB565_BS (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint16_t *pixel = (uint16_t*)buf;
  while (count--)
  {
    if (rgba[3]==0)
      pixel[0] = ctx_565_pack (255, 0, 255, 1);
    else
      pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 1);
    pixel+=1;
    rgba +=4;
  }
}


static int
ctx_b2f_over_RGB565_BS (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_decode_pixels_RGB565_BS (renderer, x, dst, &pixels[0], count);
  ret = ctx_b2f_over_RGBA8 (renderer, x, &pixels[0], coverage, count);
  ctx_encode_pixels_RGB565_BS (renderer, x, dst, &pixels[0], count);
  return ret;
}


#endif

static void
ctx_renderer_deinit (CtxRenderer *renderer)
{
  ctx_journal_deinit (&renderer->edge_list);
}

static CtxPixelFormatInfo ctx_pixel_formats[]=
{
  // format      ,comp,bitperpx, effective_bytes_per_pixel
  {CTX_FORMAT_RGBA8, 4, 32, 4, 0, 0,
  NULL, NULL, ctx_b2f_over_RGBA8 },
#if CTX_ENABLE_BGRA8
  {CTX_FORMAT_BGRA8, 4, 32, 4, 0, 0,
   ctx_decode_pixels_BGRA8, ctx_encode_pixels_BGRA8, ctx_b2f_over_BGRA8 },
#endif
#if CTX_ENABLE_GRAYF
  {CTX_FORMAT_GRAYF, 1, 32, 4, 0, 0,
   NULL, NULL, ctx_gray_float_b2f_over },
#endif
#if CTX_ENABLE_RGBAF
  {CTX_FORMAT_RGBAF, 4, 128, 4 * 4, 0, 0,
   NULL, NULL, ctx_associated_float_b2f_over },
#endif
#if CTX_ENABLE_RGB8
  {CTX_FORMAT_RGB8, 3, 24, 4, 0, 0,
   ctx_decode_pixels_RGB8, ctx_encode_pixels_RGB8, ctx_b2f_over_RGBA8_convert},
#endif
#if CTX_ENABLE_GRAY1
  {CTX_FORMAT_GRAY1, 1, 1, 4, 1, 1,
   ctx_decode_pixels_GRAY1, ctx_encode_pixels_GRAY1, ctx_b2f_over_GRAY1},
#endif
#if CTX_ENABLE_GRAY2
  {CTX_FORMAT_GRAY2, 1, 2, 4, 4, 4,
   ctx_decode_pixels_GRAY2, ctx_encode_pixels_GRAY2, ctx_b2f_over_GRAY2},
#endif
#if CTX_ENABLE_GRAY4
  {CTX_FORMAT_GRAY4, 1, 4, 4, 16, 16,
   ctx_decode_pixels_GRAY4, ctx_encode_pixels_GRAY4, ctx_b2f_over_GRAY4},
#endif
#if CTX_ENABLE_GRAY8
  {CTX_FORMAT_GRAY8, 1, 8, 4, 0, 0,
   ctx_decode_pixels_GRAY8, ctx_encode_pixels_GRAY8, ctx_b2f_over_RGBA8_convert},
#endif
#if CTX_ENABLE_GRAYA8
  {CTX_FORMAT_GRAYA8, 2, 16, 4, 0, 0,
   ctx_decode_pixels_GRAYA8, ctx_encode_pixels_GRAYA8, ctx_b2f_over_RGBA8_convert},
#endif
#if CTX_ENABLE_RGB565
  {CTX_FORMAT_RGB565, 3, 16, 4, 32, 64,
   ctx_decode_pixels_RGB565, ctx_encode_pixels_RGB565,
   ctx_b2f_over_RGB565},
#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED
  {CTX_FORMAT_RGB565_BYTESWAPPED, 3, 16, 4, 32, 64,
   ctx_decode_pixels_RGB565_BS,
   ctx_encode_pixels_RGB565_BS,
   ctx_b2f_over_RGB565_BS},
#endif
};

static CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format)
{
  for (int i = 0; i < sizeof (ctx_pixel_formats)/sizeof(ctx_pixel_formats[0]);i++)
  {
    if (ctx_pixel_formats[i].pixel_format == format)
    {
      return &ctx_pixel_formats[i];
    }
  }
  return NULL;
}

static void
ctx_renderer_init (CtxRenderer *renderer, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format)
{
  memset (renderer, 0, sizeof (CtxRenderer));
  renderer->edge_pos    = 0;
  renderer->state       = state;
  ctx_state_init (renderer->state);
  renderer->buf         = data;
  renderer->blit_x      = x;
  renderer->blit_y      = y;
  renderer->blit_width  = width;
  renderer->blit_height = height;
  renderer->blit_stride = stride;
  renderer->scan_min    = 5000;
  renderer->scan_max    = -5000;
  renderer->format = ctx_pixel_format_info (pixel_format);
}

Ctx *
ctx_new_for_buffer (CtxBuffer *buffer)
{
  Ctx *ctx = ctx_new ();

  ctx->renderer = (CtxRenderer*)malloc (sizeof (CtxRenderer));
  ctx_renderer_init (ctx->renderer, &ctx->state,
                     buffer->data, 0, 0, buffer->width, buffer->height,
                     buffer->stride, buffer->format->pixel_format);
  return ctx;
}

Ctx *
ctx_new_for_framebuffer (void *data, int width, int height,
                         int stride,
                         CtxPixelFormat pixel_format)
{
  Ctx *ctx = ctx_new ();
  ctx->renderer = (CtxRenderer*)malloc (sizeof (CtxRenderer));
  ctx_renderer_init (ctx->renderer, &ctx->state,
                     data, 0, 0, width, height, stride, pixel_format);
  return ctx;
}

void
ctx_blit (Ctx *ctx, void *data, int x, int y, int width, int height,
          int stride, CtxPixelFormat pixel_format)
{
  CtxRenderer renderer;
  CtxState    state;
  CtxIterator iterator;
  CtxEntry   *entry;

  ctx_renderer_init (&renderer, &state, data, x, y, width, height,
                     stride, pixel_format);

  ctx_iterator_init (&iterator, &ctx->journal, 0, CTX_ITERATOR_EXPAND_REFPACK|
                                                  CTX_ITERATOR_EXPAND_BITPACK);
  while ((entry = ctx_iterator_next(&iterator)))
    ctx_renderer_process (&renderer, entry);

  ctx_renderer_deinit (&renderer);
}
#endif


void
ctx_current_point (Ctx *ctx, float *x, float *y)
{
  if (!ctx)
    return;

#if CTX_RASTERIZER
  if (ctx->renderer)
  {
    if (x) *x = ctx->renderer->x;
    if (y) *y = ctx->renderer->y;
    return;
  }
#endif

  if (x) *x = ctx->state.x;
  if (y) *y = ctx->state.y;
}

void
ctx_process (Ctx *ctx, CtxEntry *entry)
{
#if CTX_RASTERIZER
  if (ctx->renderer)
  {
    ctx_renderer_process (ctx->renderer, entry);
  }
  else
#endif
  {
    /* these functions might alter the code and coordinates of
       command that in the end gets added to the journal
     */
    ctx_interpret_style (&ctx->state, entry, ctx);
    ctx_interpret_transforms (&ctx->state, entry, ctx);
    ctx_interpret_pos (&ctx->state, entry, ctx);
    ctx_journal_add_ (&ctx->journal, entry);

    if (entry->code == CTX_LOAD_IMAGE)
    {
      /* the image command and its data is submitted as one unit,
       */
      ctx_journal_add_ (&ctx->journal, entry+1);
      ctx_journal_add_ (&ctx->journal, entry+2);
    }
  }
}

/****  end of engine ****/

#if CTX_EXTRAS

static inline int ctx_str_to_command (const char *commandline)
{
  int command = 0;
  for (int i = 0; ctx_commands[i]; i++)
    if (!strncmp (commandline, ctx_commands[i]+1, strlen (ctx_commands[i]+1)))
    {
      return ctx_commands[i][0];
    }
  return command;
}

static inline const char *ctx_command_name (int command)
{
  for (int i = 0; ctx_commands[i]; i++)
    if (ctx_commands[i][0] == command)
      return ctx_commands[i]+1;
  return NULL;
}

typedef enum {
  CTX_S8,
  CTX_U8,
  CTX_U16,
  CTX_S16,
  CTX_U32,
  CTX_S32,
  CTX_FLOAT,
  CTX_VOID,
} CtxDataType;


static inline CtxDataType
ctx_datatype_for_code (CtxCode code)
{
  switch (code)
  {
#if CTX_BITPACK
    case CTX_REL_LINE_TO_X2:
    case CTX_MOVE_TO_REL_LINE_TO:
    case CTX_REL_LINE_TO_REL_MOVE_TO:
    case CTX_REL_QUAD_TO_S16:
#endif
    case CTX_EDGE:
    case CTX_NEW_EDGE:
    case CTX_EDGE_FLIPPED:
    case CTX_BLIT_RECT:
      return CTX_S16;
    case CTX_RECTANGLE:
    case CTX_LINE_TO:
    case CTX_REL_LINE_TO:
    case CTX_MOVE_TO:
    case CTX_QUAD_TO:
    case CTX_REL_QUAD_TO:
    case CTX_REL_MOVE_TO:
    case CTX_CURVE_TO:
    case CTX_GLOBAL_ALPHA:
    case CTX_REL_CURVE_TO:
    case CTX_ROTATE:
    case CTX_SCALE:
    case CTX_TRANSLATE:
    case CTX_TEXT:
    case CTX_CONT:
    case CTX_LINE_WIDTH:
    case CTX_FONT_SIZE:
    case CTX_ARC:
    case CTX_LOAD_IMAGE:
    case CTX_LINEAR_GRADIENT:
    case CTX_RADIAL_GRADIENT:
    case CTX_GRADIENT_STOP:
#if CTX_BITPACK
    case CTX_FILL_MOVE_TO:
#endif
      return CTX_FLOAT;
    case CTX_LINE_CAP:
    case CTX_FILL_RULE:
    case CTX_LINE_JOIN:
    case CTX_SET_RGBA:
    case CTX_GRADIENT_NO:
      return CTX_U8;
#if CTX_BITPACK
    case CTX_REL_LINE_TO_X4:
    case CTX_REL_LINE_TO_REL_CURVE_TO:
    case CTX_REL_CURVE_TO_REL_LINE_TO:
    case CTX_REL_CURVE_TO_REL_MOVE_TO:
    case CTX_REL_QUAD_TO_REL_QUAD_TO:
      return CTX_S8;
#endif
    case CTX_DATA:
    case CTX_DATA_REV:
    case CTX_DEFINE_GLYPH:
    case CTX_GLYPH:
      return CTX_U32;
    case CTX_KERNING_PAIR:
      return CTX_U16;
    case CTX_FLUSH:
    case CTX_FILL:
    case CTX_CLIP:
    case CTX_STATE:
    case CTX_SAVE:
    case CTX_RESTORE:
    case CTX_NEW_PATH:
    case CTX_CLOSE_PATH:
    case CTX_IDENTITY:
    case CTX_CLEAR:
    case CTX_STROKE:
    case CTX_NOP:
    case CTX_GRADIENT_CLEAR:
    case CTX_EXIT:
      return CTX_VOID;

    case CTX_REPEAT_HISTORY:
      return CTX_U32;

    //case CTX_SETPNG:
  }
 return 0;
}

int
ctx_cmd_str (Ctx *ctx, const char *commandline)
{
  uint32_t args_u32[2] = {0,};
  const char *rest;
  int code = 0;
  rest = strchr (commandline, ' ');

  if (rest && commandline[1] == ' ')
  {
    code = commandline[0];
  }
  else
  {
    code = ctx_str_to_command (commandline);
    if (!code)
    {
      // unknown command
      return -1;
    }
  }

  while (rest && *rest == ' ') rest++;

  switch (ctx_datatype_for_code (code))
  {
    case CTX_FLOAT:
      {
        float *args = (void*)&args_u32[0];
        args[0] = strtof (rest, (void*)&rest);
        while (rest && *rest == ' ') rest++;
        args[1] = strtof (rest, (void*)&rest);
      }
      break;
    case CTX_U32:
      {
        uint32_t *args = (void*)&args_u32[0];
        for (int i = 0; i < 2; i ++)
        {
          if (rest)
          {
            while (rest && *rest == ' ') rest++;
            args[i] = atoi (rest);
            while (rest && (*rest >= '0') && (*rest <= '9')) rest++;
          }
        }
      }
      break;
    case CTX_S32:
      {
        int32_t *args = (void*)&args_u32[0];
        for (int i = 0; i < 2; i ++)
        {
          if (rest)
          {
            while (rest && *rest == ' ') rest++;
            args[i] = atoi (rest);
            while (rest && (*rest >= '0') && (*rest <= '9')) rest++;
          }
        }
      }
      break;
    case CTX_S16:
      {
        int16_t *args = (void*)&args_u32[0];
        for (int i = 0; i < 4; i ++)
        {
          if (rest)
          {
            while (rest && *rest == ' ') rest++;
            args[i] = atoi (rest);
            while (rest && (*rest >= '0') && (*rest <= '9')) rest++;
          }
        }
      }
      break;
    case CTX_U16:
      {
        uint16_t *args = (void*)&args_u32[0];
        for (int i = 0; i < 4; i ++)
        {
          if (rest)
          {
            while (rest && *rest == ' ') rest++;
            args[i] = atoi (rest);
            while (rest && (*rest >= '0') && (*rest <= '9')) rest++;
          }
        }
      }
      break;
    case CTX_U8:
      {
        uint8_t *args = (void*)&args_u32[0];
        for (int i = 0; i < 8; i ++)
        {
          if (rest)
          {
            while (rest && *rest == ' ') rest++;
            args[i] = atoi (rest);
            while (rest && (*rest >= '0') && (*rest <= '9')) rest++;
          }
        }
      }
      break;
    case CTX_S8:
      {
        int8_t *args = (void*)&args_u32[0];
        for (int i = 0; i < 8; i ++)
        {
          if (rest)
          {
            while (rest && *rest == ' ') rest++;
            args[i] = atoi (rest);
            while (rest && (*rest >= '0') && (*rest <= '9')) rest++;
          }
        }
      }
      break;
    case CTX_VOID:
      break;
  }
  return ctx_journal_add_u32 (&ctx->journal, code, (void*)args_u32);
}

static inline void
ctx_entry_print (CtxState *state, CtxEntry *entry, void *data)
{
  //if(ctx_command_name (entry->code))
  //  ctx_log( "%s", ctx_command_name (entry->code));
  //else
    ctx_log( "[%i / %c]", entry->code, entry->code);

  switch (ctx_datatype_for_code (entry->code))
  {
    case CTX_VOID:
      break;
    case CTX_FLOAT:
     for (int n = 0; n < 2; n++)
       ctx_log( " %f", entry->data.f[n]);
     break;
    case CTX_U32:
     for (int n = 0; n < 2; n++)
       ctx_log( " %lu", (long unsigned int)entry->data.u32[n]);
     break;
    case CTX_S32:
     for (int n = 0; n < 2; n++)
       ctx_log( " %d", (int)entry->data.s32[n]);
     break;
    case CTX_U16:
     for (int n = 0; n < 4; n++)
       ctx_log( " %d", entry->data.u16[n]);
     break;
    case CTX_S16:
     for (int n = 0; n < 4; n++)
       ctx_log( " %d", entry->data.s16[n]);
     break;
    case CTX_U8:
     for (int n = 0; n < 8; n++)
       ctx_log( " %d", entry->data.u8[n]);
     break;
    case CTX_S8:
     for (int n = 0; n < 8; n++)
       ctx_log( " %i", entry->data.s8[n]);
     break;
  }
  ctx_log( "\n");
}

static inline void
ctx_do_cb (Ctx *ctx, int do_history,
           void (*cb)(CtxState *state, CtxEntry *entry, void *data),
           void *data)
{
  CtxIterator iterator;
  ctx_iterator_init (&iterator, &ctx->journal, 0, CTX_ITERATOR_EXPAND_REFPACK|
                                                  CTX_ITERATOR_EXPAND_BITPACK);
  CtxEntry *entry;
  while ((entry = ctx_iterator_next(&iterator)))
  {
    cb (&ctx->state, entry, data);
  }
}
#endif

#if CTX_FONT_ENGINE_STB

static int
file_get_contents (const char     *path,
                   unsigned char **contents,
                   long           *length)
{
  FILE *file;
  long  size;
  long  remaining;
  char *buffer;

  file = fopen (path, "rb");

  if (!file)
    return -1;

  fseek (file, 0, SEEK_END);
  size = remaining = ftell (file);
  if (length)
    *length =size;
  rewind (file);
  buffer = malloc(size + 8);

  if (!buffer)
    {
      fclose(file);
      return -1;
    }

  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      free (buffer);
      return -1;
    }
  fclose (file);
  *contents = (void*)buffer;
  buffer[size] = 0;
  return 0;
}

int
ctx_load_font_ttf (Ctx *ctx, const char *name, const uint8_t *ttf_contents)
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    return -1;
  ctx_fonts[ctx_font_count].type = 1;
  ctx_fonts[ctx_font_count].name = strdup (name);
  if (!stbtt_InitFont(&ctx_fonts[ctx_font_count].stb.ttf_info, ttf_contents, 0))
    {
      ctx_log( "Font init failed\n");
      return -1;
    }
  ctx_font_count ++;
  return ctx_font_count-1;
}

int
ctx_load_font_ttf_file (Ctx *ctx, const char *name, const char *path)
{
  uint8_t *contents = NULL;
  long length = 0;
  file_get_contents (path, &contents, &length);
  if (!contents)
  {
    ctx_log( "File load failed\n");
    return -1;
  }
  return ctx_load_font_ttf (ctx, name, contents);
}

#endif


static inline int ctx_utf8_len (const unsigned char first_byte)
{
  if      ((first_byte & 0x80) == 0)
    return 1; /* ASCII */
  else if ((first_byte & 0xE0) == 0xC0)
    return 2;
  else if ((first_byte & 0xF0) == 0xE0)
    return 3;
  else if ((first_byte & 0xF8) == 0xF0)
    return 4;
  return 1;
}

static inline const char *ctx_utf8_skip (const char *s, int utf8_length)
{
   int count;
   if (!s)
     return NULL;
   for (count = 0; *s; s++)
   {
     if ((*s & 0xC0) != 0x80)
       count++;
     if (count == utf8_length + 1)
       return s;
   }
   return s;
}

static inline int ctx_utf8_strlen (const char *s)
{
   int count;
   if (!s)
     return 0;
   for (count = 0; *s; s++)
     if ((*s & 0xC0) != 0x80)
       count++;
   return count;
}

int
ctx_unichar_to_utf8 (uint32_t  ch,
                     uint8_t  *dest)
{
/* http://www.cprogramming.com/tutorial/utf8.c  */
/*  Basic UTF-8 manipulation routines
  by Jeff Bezanson
  placed in the public domain Fall 2005 ... */
    if (ch < 0x80) {
        dest[0] = (char)ch;
        return 1;
    }
    if (ch < 0x800) {
        dest[0] = (ch>>6) | 0xC0;
        dest[1] = (ch & 0x3F) | 0x80;
        return 2;
    }
    if (ch < 0x10000) {
        dest[0] = (ch>>12) | 0xE0;
        dest[1] = ((ch>>6) & 0x3F) | 0x80;
        dest[2] = (ch & 0x3F) | 0x80;
        return 3;
    }
    if (ch < 0x110000) {
        dest[0] = (ch>>18) | 0xF0;
        dest[1] = ((ch>>12) & 0x3F) | 0x80;
        dest[2] = ((ch>>6) & 0x3F) | 0x80;
        dest[3] = (ch & 0x3F) | 0x80;
        return 4;
    }
    return 0;
}

uint32_t
ctx_utf8_to_unichar (const char *input)
{
  const uint8_t *utf8 = (const uint8_t*)input;
  uint8_t c = utf8[0];
  if ((c & 0x80) == 0)
    return c;
  else if ((c & 0xE0) == 0xC0)
    return ((utf8[0] & 0x1F) << 6) |
            (utf8[1] & 0x3F);
  else if ((c & 0xF0) == 0xE0)
    return ((utf8[0] & 0xF)  << 12)|
           ((utf8[1] & 0x3F) << 6) |
            (utf8[2] & 0x3F);
  else if ((c & 0xF8) == 0xF0)
    return ((utf8[0] & 0x7)  << 18)|
           ((utf8[1] & 0x3F) << 12)|
           ((utf8[2] & 0x3F) << 6) |
            (utf8[3] & 0x3F);
  else if ((c & 0xFC) == 0xF8)
    return ((utf8[0] & 0x3)  << 24)|
           ((utf8[1] & 0x3F) << 18)|
           ((utf8[2] & 0x3F) << 12)|
           ((utf8[3] & 0x3F) << 6) |
            (utf8[4] & 0x3F);
  else if ((c & 0xFE) == 0xFC)
    return ((utf8[0] & 0x1)  << 30)|
           ((utf8[1] & 0x3F) << 24)|
           ((utf8[2] & 0x3F) << 18)|
           ((utf8[3] & 0x3F) << 12)|
           ((utf8[4] & 0x3F) << 6) |
            (utf8[5] & 0x3F);
  return 0;
}

#if CTX_FONT_ENGINE_STB

static inline float
ctx_glyph_width_stb (Ctx *ctx, stbtt_fontinfo *ttf_info, int unichar)
{
  float font_size = ctx->state.gstate.font_size;
  float scale = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int advance, lsb;
  stbtt_GetCodepointHMetrics (ttf_info, unichar, &advance, &lsb);
  return advance * scale;
}

static inline float
ctx_glyph_kern_stb (Ctx *ctx, stbtt_fontinfo *ttf_info, int unicharA, int unicharB)
{
  float font_size = ctx->state.gstate.font_size;
  float scale = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  return stbtt_GetCodepointKernAdvance (ttf_info, unicharA, unicharB) * scale;
}

static inline int
ctx_glyph_stb (Ctx *ctx, stbtt_fontinfo *ttf_info, uint32_t unichar, int stroke)
{
  if (stbtt_FindGlyphIndex (ttf_info, unichar)==0)
    return -1;

  float font_size = ctx->state.gstate.font_size;
  int   baseline = ctx->state.y;
  float origin_x = ctx->state.x;
  float origin_y = baseline;
  float scale    = stbtt_ScaleForPixelHeight (ttf_info, font_size);;

  stbtt_vertex *vertices = NULL;
  int num_verts;

  num_verts = stbtt_GetCodepointShape (ttf_info, unichar, &vertices);

  for (int i = 0; i < num_verts; i++)
  {
    stbtt_vertex *vertex = &vertices[i];
    switch (vertex->type)
      {
       case STBTT_vmove:
         ctx_move_to (ctx,
            origin_x + vertex->x * scale, origin_y - vertex->y * scale);
         break;
       case STBTT_vline:
         ctx_line_to (ctx,
            origin_x + vertex->x * scale, origin_y - vertex->y * scale);
         break;
       case STBTT_vcubic:
         ctx_curve_to (ctx,
            origin_x + vertex->cx  * scale, origin_y - vertex->cy  * scale,
            origin_x + vertex->cx1 * scale, origin_y - vertex->cy1 * scale,
            origin_x + vertex->x   * scale, origin_y - vertex->y   * scale);
         break;
       case STBTT_vcurve:
         ctx_quad_to (ctx,
            origin_x + vertex->cx  * scale, origin_y - vertex->cy  * scale,
            origin_x + vertex->x   * scale, origin_y - vertex->y   * scale);
          break;
     }
   }
   stbtt_FreeShape (ttf_info, vertices);
   if (stroke)
     ctx_stroke (ctx);
   else
     ctx_fill (ctx);
   return 0;
}
#endif

#if CTX_FONT_ENGINE_CTX

static inline float
ctx_glyph_kern_ctx (Ctx *ctx, CtxFont *font, int unicharA, int unicharB)
{
  float font_size = ctx->state.gstate.font_size;
  for (int i = font->ctx.first_kern; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry*)&font->ctx.data[i];
    if (entry->code == CTX_KERNING_PAIR)
    {
      if (font->ctx.first_kern == 0) font->ctx.first_kern = i;
      if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
        return entry->data.s32[2] / 255.0 * font_size / CTX_BAKE_FONT_SIZE;
    }
  }

  return 0;
}

static inline float
ctx_glyph_width_ctx (Ctx *ctx, CtxFont *font, int unichar)
{
  CtxState *state = &ctx->state;
  float font_size = state->gstate.font_size;
  int start = 0;
  if (unichar >= 'a' && unichar <= 'z')
  {
    start = font->ctx.glyph_pos[unichar-'a'];
  }
  for (int i = start; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry*)&font->ctx.data[i];
    if (entry->code == '@')
      if (entry->data.u32[0] == unichar)
        return entry->data.u32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE;
  }
  return 0.0;
}


static inline int
ctx_glyph_ctx (Ctx *ctx, CtxFont *font, uint32_t unichar, int stroke)
{
  CtxState *state = &ctx->state;
  CtxIterator iterator;
  CtxJournal  journal = {(CtxEntry*)font->ctx.data,
                         font->ctx.length,
                         font->ctx.length};
  float origin_x = state->x;
  float origin_y = state->y;

#if 0
  if (ctx->renderer)
  {
    origin_x = ctx->renderer->x;
    origin_y = ctx->renderer->y;
  }
  ctx_current_point (ctx, &origin_x, &origin_y);
#endif

  int in_glyph = 0;
  float font_size = state->gstate.font_size;
  int start = 0;
  if (unichar >= 'a' && unichar <= 'z')
  {
    start = font->ctx.glyph_pos[unichar-'a'];
  }
  ctx_iterator_init (&iterator, &journal, start, CTX_ITERATOR_EXPAND_BITPACK);

  CtxEntry *entry;

  while ((entry = ctx_iterator_next (&iterator)))
  {
    if (in_glyph)
    {
      if (entry->code == '@')
      {
        if (stroke)
          ctx_stroke (ctx);
        else
          ctx_fill (ctx);
        ctx_restore (ctx);
        return 0;
      }
      ctx_process (ctx, entry);
    }
    else if (entry->code == '@' && entry->data.u32[0] == unichar)
    {
      in_glyph = 1;
      if (unichar >= 'a' && unichar <= 'z')
      {
        font->ctx.glyph_pos[unichar-'a'] = iterator.pos;
      }
      ctx_save (ctx);
      ctx_translate (ctx, origin_x, origin_y);
      ctx_new_path (ctx);
      ctx_move_to (ctx, 0, 0);
      ctx_scale (ctx, font_size / CTX_BAKE_FONT_SIZE,
                      font_size / CTX_BAKE_FONT_SIZE);
    }
  }
  ctx_assert (0);
  return -1;
}

int
ctx_load_font_ctx (const char *name, const void *data, int length)
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    return -1;
  ctx_fonts[ctx_font_count].type = 0;
  ctx_fonts[ctx_font_count].name = strdup (name);
  ctx_fonts[ctx_font_count].ctx.data = (CtxEntry*)data;
  ctx_fonts[ctx_font_count].ctx.length = length / sizeof (CtxEntry);
  ctx_font_count++;
  return ctx_font_count-1;
}


#endif
#if CTX_FONT_ENGINE_MONOBITMAP

static inline float
ctx_glyph_kern_monobitmap (Ctx *ctx, CtxFont *font, int unicharA, int unicharB)
{
  return 0.0;
}

static inline float
ctx_glyph_width_monobitmap (Ctx *ctx, CtxFont *font, int unichar)
{
  return 0.0;
}

static inline int
ctx_glyph_monobitmap (Ctx *ctx, CtxFont *font, uint32_t unichar, int stroke)
{
#if 0
  CtxState *state = &ctx->state;
  CtxIterator iterator;
  CtxJournal  journal = {(CtxEntry*)font->ctx.data,
                         font->ctx.length,
                         font->ctx.length};
  float origin_x = state->x;
  float origin_y = state->y;

#if 1
  if (ctx->renderer)
  {
    origin_x = ctx->renderer->x;
    origin_y = ctx->renderer->y;
  }
  ctx_current_point (ctx, &origin_x, &origin_y);
#endif

  int in_glyph = 0;
  float font_size = state->gstate.font_size;
  int start = 0;
  if (unichar >= 'a' && unichar <= 'z')
  {
    start = font->ctx.glyph_pos[unichar-'a'];
  }
  ctx_iterator_init (&iterator, &journal, start, CTX_ITERATOR_EXPAND_BITPACK);

  CtxEntry *entry;

  while ((entry = ctx_iterator_next (&iterator)))
  {
    if (in_glyph)
    {
      if (entry->code == '@')
      {
        if (stroke)
          ctx_stroke (ctx);
        else
          ctx_fill (ctx);
        ctx_restore (ctx);
        return 0;
      }
      ctx_process (ctx, entry);
    }
    else if (entry->code == '@' && entry->data.u32[0] == unichar)
    {
      in_glyph = 1;
      if (unichar >= 'a' && unichar <= 'z')
      {
        font->ctx.glyph_pos[unichar-'a'] = iterator.pos;
      }
      ctx_save (ctx);
      ctx_translate (ctx, origin_x, origin_y);
      ctx_new_path (ctx);
      ctx_move_to (ctx, 0, 0);
      ctx_scale (ctx, font_size / CTX_BAKE_FONT_SIZE,
                      font_size / CTX_BAKE_FONT_SIZE);
    }
  }
  ctx_assert (0);
#endif
  return -1;
}


int
ctx_load_font_monobitmap (const char *name, int start, int end,
                          int glyphwidth, int glyphheight,
                          const uint8_t *data)
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    return -1;
  ctx_fonts[ctx_font_count].type = 2;
  ctx_fonts[ctx_font_count].name = strdup (name);
  ctx_fonts[ctx_font_count].monobitmap.data = data;
  ctx_fonts[ctx_font_count].monobitmap.gw = glyphwidth;
  ctx_fonts[ctx_font_count].monobitmap.gh = glyphheight;
  ctx_fonts[ctx_font_count].monobitmap.start = start;
  ctx_fonts[ctx_font_count].monobitmap.end = end;
  ctx_font_count++;
  return ctx_font_count-1;
}
#endif

static inline int
ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxState *state = &ctx->state;
  switch (ctx_fonts[state->gstate.font].type)
  {
#if CTX_FONT_ENGINE_CTX
    case 0:
      return ctx_glyph_ctx (ctx, &ctx_fonts[state->gstate.font], unichar, stroke);
#endif
#if CTX_FONT_ENGINE_STB
    case 1:
      return ctx_glyph_stb (ctx, &ctx_fonts[state->gstate.font].stb.ttf_info, unichar, stroke);
#endif
#if CTX_FONT_ENGINE_MONOBITMAP
    case 2:
      return ctx_glyph_monobitmap (ctx, &ctx_fonts[state->gstate.font], unichar, stroke);
#endif
  }
  return -1;
}

static inline float
ctx_glyph_width (Ctx *ctx, int unichar)
{
  CtxState *state = &ctx->state;
  switch (ctx_fonts[state->gstate.font].type)
  {
#if CTX_FONT_ENGINE_CTX
    case 0:
      return ctx_glyph_width_ctx (ctx, &ctx_fonts[state->gstate.font], unichar);
#endif
#if CTX_FONT_ENGINE_STB
    case 1:
      return ctx_glyph_width_stb (ctx, &ctx_fonts[state->gstate.font].stb.ttf_info, unichar);
#endif
#if CTX_FONT_ENGINE_MONOBITMAP
    case 2:
      return ctx_glyph_width_monobitmap (ctx, &ctx_fonts[state->gstate.font], unichar);
#endif
  }
  return 0.0;
}

static inline float
ctx_glyph_kern (Ctx *ctx, int unicharA, int unicharB)
{
  CtxState *state = &ctx->state;
  switch (ctx_fonts[state->gstate.font].type)
  {
#if CTX_FONT_ENGINE_CTX
    case 0:
      return ctx_glyph_kern_ctx (ctx, &ctx_fonts[state->gstate.font], unicharA, unicharB);
#endif
#if CTX_FONT_ENGINE_STB
    case 1:
      return ctx_glyph_kern_stb (ctx, &ctx_fonts[state->gstate.font].stb.ttf_info, unicharA, unicharB);
#endif
#if CTX_FONT_ENGINE_MONOBITMAP
    case 2:
      return ctx_glyph_kern_monobitmap (ctx, &ctx_fonts[state->gstate.font], unicharA, unicharB);
#endif
  }
  return 0.0;
}

float
ctx_text_width (Ctx        *ctx,
                const char *string)
{
  float sum = 0.0;
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    {
      sum += ctx_glyph_width (ctx, ctx_utf8_to_unichar (utf8));
    }
  return sum;
}

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke)
{
  CtxState *state = &ctx->state;
  float x = ctx->state.x;
  float y = ctx->state.y;
  float x0 = x;
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    {
      if (*utf8 == '\n')
      {
        y += ctx->state.gstate.font_size * state->gstate.line_spacing;
        x = x0;
        ctx_move_to (ctx, x, y);
      }
      else
      {
        uint32_t unichar = ctx_utf8_to_unichar (utf8);
        ctx_move_to (ctx, x, y);
        ctx_glyph (ctx, unichar, stroke);
        const char *next_utf8 = ctx_utf8_skip (utf8, 1);
        if (next_utf8)
        {
          x += ctx_glyph_width (ctx, unichar);
          x += ctx_glyph_kern (ctx, unichar, ctx_utf8_to_unichar (next_utf8));
        }
      }
    }
}

void
ctx_text (Ctx        *ctx,
           const char *string)
{
  _ctx_text (ctx, string, 0);
}


void
ctx_text_stroke (Ctx        *ctx,
                 const char *string)
{
  _ctx_text (ctx, string, 1);
}

#if CTX_CAIRO

void
ctx_render_cairo (Ctx *ctx, cairo_t *cr)
{
  CtxIterator iterator;
  CtxState    state;
  CtxEntry   *entry;

  cairo_pattern_t *pat = NULL;
  cairo_surface_t *image = NULL;

  ctx_state_init (&state);
  ctx_iterator_init (&iterator, &ctx->journal, 0, CTX_ITERATOR_EXPAND_REFPACK|
                                                  CTX_ITERATOR_EXPAND_BITPACK);

  while ((entry = ctx_iterator_next (&iterator)))
  {
    //command_print (NULL, command, &ctx->journal);
    //if (cairo_status (cr)) return;
    switch (entry->code)
    {
      case CTX_LINE_TO:
        cairo_line_to (cr, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_REL_LINE_TO:
        cairo_rel_line_to (cr, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_MOVE_TO:
        cairo_move_to (cr, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_REL_MOVE_TO:
        cairo_rel_move_to (cr, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_CURVE_TO:
        cairo_curve_to (cr, ctx_arg_float(0), ctx_arg_float(1),
                            ctx_arg_float(2), ctx_arg_float(3),
                            ctx_arg_float(4), ctx_arg_float(5));
        break;

      case CTX_REL_CURVE_TO:
        cairo_rel_curve_to (cr,ctx_arg_float(0), ctx_arg_float(1),
                               ctx_arg_float(2), ctx_arg_float(3),
                               ctx_arg_float(4), ctx_arg_float(5));
        break;

      case CTX_QUAD_TO:
        {
          double x0, y0;
          cairo_get_current_point (cr, &x0, &y0);
          float cx = ctx_arg_float(0);
          float cy = ctx_arg_float(1);
          float  x = ctx_arg_float(2);
          float  y = ctx_arg_float(3);
          cairo_curve_to (cr,
            (cx * 2 + x0) / 3.0f, (cy * 2 + y0) / 3.0f,
            (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
            x,                              y);
        }
        break;
      case CTX_REL_QUAD_TO:
        {
          double x0, y0;
          cairo_get_current_point (cr, &x0, &y0);

          float cx = ctx_arg_float(0) + x0;
          float cy = ctx_arg_float(1) + y0;
          float  x = ctx_arg_float(2) + x0;
          float  y = ctx_arg_float(3) + y0;
          cairo_curve_to (cr,
            (cx * 2 + x0) / 3.0f, (cy * 2 + y0) / 3.0f,
            (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
            x,                              y);
        }
        break;

      /* rotate/scale/translate does not occur in fully minified data stream */
      case CTX_ROTATE:
        cairo_rotate (cr, ctx_arg_float(0));
        break;

      case CTX_SCALE:
        cairo_scale (cr, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_TRANSLATE:
        cairo_translate (cr, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_LINE_WIDTH:
        cairo_set_line_width (cr, ctx_arg_float(0));
        break;

      case CTX_ARC:
        cairo_arc (cr, ctx_arg_float(0), ctx_arg_float(1),
                       ctx_arg_float(2), ctx_arg_float(3),
                       ctx_arg_float(4));
        break;


      case CTX_SET_RGBA:
        cairo_set_source_rgba (cr, ctx_arg_u8(0)/255.0,
                                   ctx_arg_u8(1)/255.0,
                                   ctx_arg_u8(2)/255.0,
                                   ctx_arg_u8(3)/255.0);
        break;

      case CTX_RECTANGLE:
        cairo_rectangle (cr,ctx_arg_float(0),
                            ctx_arg_float(1),
                            ctx_arg_float(2),
                            ctx_arg_float(3));
        break;

      case CTX_BLIT_RECT:
        cairo_rectangle (cr,ctx_arg_s16(0)/CTX_SUBDIV,
                            ctx_arg_s16(1)/CTX_SUBDIV,
                            ctx_arg_s16(2)/CTX_SUBDIV,
                            ctx_arg_s16(3)/CTX_SUBDIV);
        cairo_fill (cr);
        break;

      case CTX_FILL:
        cairo_fill (cr);
        break;
      case CTX_STROKE:
        cairo_stroke (cr);
        break;

      case CTX_IDENTITY:
        cairo_identity_matrix (cr);
        break;

      case CTX_CLIP:
        cairo_clip (cr);
        break;

      case CTX_NEW_PATH:
        cairo_new_path (cr);
        break;

      case CTX_CLOSE_PATH:
        cairo_close_path (cr);
        break;

      case CTX_SAVE:
        cairo_save (cr);
        break;

      case CTX_RESTORE:
        cairo_restore (cr);
        break;

      case CTX_FONT_SIZE:
        cairo_set_font_size (cr, ctx_arg_float(0));
        break;

      case CTX_LINE_CAP:
        {
        int cairo_val = CAIRO_LINE_CAP_SQUARE;
        switch (ctx_arg_u8(0))
        {
          case CTX_CAP_ROUND: cairo_val = CAIRO_LINE_CAP_ROUND; break;
          case CTX_CAP_SQUARE: cairo_val = CAIRO_LINE_CAP_SQUARE; break;
          case CTX_CAP_NONE: cairo_val = CAIRO_LINE_CAP_BUTT; break;
        }
          cairo_set_line_cap (cr, cairo_val);
        }
        break;

      case CTX_LINE_JOIN:
        {
          int cairo_val = CAIRO_LINE_JOIN_ROUND;
          switch (ctx_arg_u8(0))
          {
            case CTX_JOIN_ROUND: cairo_val = CAIRO_LINE_JOIN_ROUND; break;
            case CTX_JOIN_BEVEL: cairo_val = CAIRO_LINE_JOIN_BEVEL; break;
            case CTX_JOIN_MITER: cairo_val = CAIRO_LINE_JOIN_MITER; break;
          }
          cairo_set_line_cap (cr, cairo_val);
        }
        break;
      case CTX_LINEAR_GRADIENT:
        {
          if (pat)
          {
            cairo_pattern_destroy (pat);
            pat = NULL;
          }
          pat = cairo_pattern_create_linear (ctx_arg_float(0), ctx_arg_float(1),
                                             ctx_arg_float(2), ctx_arg_float(3));
          cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 0, 1);
          cairo_pattern_add_color_stop_rgba (pat, 1, 1, 1, 1, 1);
          cairo_set_source (cr, pat);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          if (pat)
          {
            cairo_pattern_destroy (pat);
            pat = NULL;
          }
          pat = cairo_pattern_create_radial (ctx_arg_float(0), ctx_arg_float(1),
                                             ctx_arg_float(2), ctx_arg_float(3),
                                             ctx_arg_float(4), ctx_arg_float(5));
          cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 0, 1);
          cairo_pattern_add_color_stop_rgba (pat, 1, 1, 1, 1, 1);
          cairo_set_source (cr, pat);
        }
        break;
      case CTX_LOAD_IMAGE:
        {
          if (image)
          {
            cairo_surface_destroy (image);
            image = NULL;
          }

          if (pat)
          {
            cairo_pattern_destroy (pat);
            pat = NULL;
          }
          image = cairo_image_surface_create_from_png ((char*)&entry[2].data.u8[0]);
          cairo_set_source_surface (cr, image, ctx_arg_float(0), ctx_arg_float(1));
        }
        break;
      case CTX_TEXT:
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
      case CTX_STATE:
      case CTX_REPEAT_HISTORY:
        break;
    }
  }
  if (pat)
    cairo_pattern_destroy (pat);
  if (image)
  {
    cairo_surface_destroy (image);
    image = NULL;
  }
}
#endif


static void ctx_setup ()
{
  static int initialized = 0;
  if (initialized) return;
  initialized = 1;
#if CTX_FONT_ENGINE_CTX
  ctx_font_count = 0; // oddly - this is needed in arduino
#if CTX_FONT_regular
  ctx_load_font_ctx ("regular", ctx_font_regular,
                        sizeof (ctx_font_regular));
#endif
#if CTX_FONT_mono
  ctx_load_font_ctx ("mono", ctx_font_mono,
                        sizeof (ctx_font_mono));
#endif
#if CTX_FONT_bold
  ctx_load_font_ctx ("bold", ctx_font_bold,
                        sizeof (ctx_font_bold));
#endif
#if CTX_FONT_italic
  ctx_load_font_ctx ("italic", ctx_font_italic,
                        sizeof (ctx_font_italic));
#endif
#if CTX_FONT_sans
  ctx_load_font_ctx ("sans", ctx_font_sans,
                        sizeof (ctx_font_sans));
#endif
#if CTX_FONT_serif
  ctx_load_font_ctx ("serif", ctx_font_serif,
                        sizeof (ctx_font_serif));
#endif
#if CTX_FONT_symbol
  ctx_load_font_ctx ("symbol", ctx_font_symbol,
                        sizeof (ctx_font_symbol));
#endif
#if CTX_FONT_emoji
  ctx_load_font_ctx ("emoji", ctx_font_emoji,
                        sizeof (ctx_font_emoji));
#endif
#endif
#if CTX_FONT_sgi
  ctx_load_font_monobitmap ("bitmap", ' ', '~', 8, 13, &sgi_font[0][0]);
#endif
}

#endif
