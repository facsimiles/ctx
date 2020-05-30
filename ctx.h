/* ctx - tiny footprint 2d vector rasterizer context
 *
 * Copyright (c) 2019, 2020 Øyvind Kolås <pippin@gimp.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies. Funding
 * of past and future development is welcome, for further information see
 * https://pippin.gimp.org/funding/
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
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

typedef enum
{
  CTX_FORMAT_GRAY8,
  CTX_FORMAT_GRAYA8,
  CTX_FORMAT_RGB8,
  CTX_FORMAT_RGBA8,
  CTX_FORMAT_BGRA8,
  CTX_FORMAT_RGB565,
  CTX_FORMAT_RGB565_BYTESWAPPED,
  CTX_FORMAT_RGB332,
  CTX_FORMAT_RGBAF,
  CTX_FORMAT_GRAYF,
  CTX_FORMAT_GRAY1,
  CTX_FORMAT_GRAY2,
  CTX_FORMAT_GRAY4,
  CTX_FORMAT_CMYK8,
  CTX_FORMAT_CMYKA8,
  CTX_FORMAT_CMYKAF
} CtxPixelFormat;

typedef struct _Ctx Ctx;

struct _CtxGlyph
{
  uint32_t index;
  float    x;
  float    y;
};

typedef struct _CtxGlyph CtxGlyph;

CtxGlyph *ctx_glyph_allocate (int n_glyphs);
void      gtx_glyph_free     (CtxGlyph *glyphs);

/**
 * ctx_new:
 *
 * Create a new drawing context, without an associated target frame buffer,
 * use ctx_blit to render the built up renderstream to a framebuffer.
 */
Ctx *ctx_new                 (void);

/**
 * ctx_new_for_framebuffer:
 *
 * Create a new drawing context for a framebuffer, rendering happens
 * immediately.
 */
Ctx *ctx_new_for_framebuffer (void *data,
                              int width, int height, int stride,
                              CtxPixelFormat pixel_format);

/**
 * ctx_new_for_renderstream:
 *
 * Create a new drawing context for a pre-existing renderstream.
 */
Ctx *ctx_new_for_renderstream (void *data, size_t length);
void ctx_free                  (Ctx *ctx);

/* blits the contents of a bare context
 */
void ctx_blit          (Ctx *ctx,
                        void *data, int x, int y,
                        int width, int height, int stride,
                        CtxPixelFormat pixel_format);

/* clears and resets a context */
void ctx_clear          (Ctx *ctx);

void ctx_new_path       (Ctx *ctx);
void ctx_save           (Ctx *ctx);
void ctx_restore        (Ctx *ctx);
void ctx_clip           (Ctx *ctx);
void ctx_identity_matrix (Ctx *ctx);
void ctx_rotate         (Ctx *ctx, float x);
void ctx_set_line_width (Ctx *ctx, float x);
void ctx_set_transform  (Ctx *ctx, float a, float b,  // hscale, hskew
                                   float c, float d,  // vskew,  vscale
                                   float e, float f); // htran,  vtran
void ctx_get_transform  (Ctx *ctx, float *a, float *b,
                                   float *c, float *d,
                                   float *e, float *f);

#define CTX_LINE_WIDTH_HAIRLINE -1000.0
#define CTX_LINE_WIDTH_ALIASED  -1.0

#define CTX_LINE_WIDTH_FAST     -1.0  /* aliased 1px wide line,
                                         could be replaced with fast
                                         1px wide dab based stroker*/

void  ctx_dirty_rect     (Ctx *ctx, int *x, int *y, int *width, int *height);

void  ctx_set_font_size  (Ctx *ctx, float x);
void  ctx_set_font       (Ctx *ctx, const char *font);
void  ctx_scale          (Ctx *ctx, float x, float y);
void  ctx_translate      (Ctx *ctx, float x, float y);
void  ctx_line_to        (Ctx *ctx, float x, float y);
void  ctx_move_to        (Ctx *ctx, float x, float y);
void  ctx_curve_to       (Ctx *ctx, float cx0, float cy0,
                          float cx1, float cy1,
                          float x, float y);
void  ctx_quad_to        (Ctx *ctx, float cx, float cy,
                          float x, float y);
void  ctx_rectangle      (Ctx *ctx,
                          float x0, float y0,
                          float w, float h);
void  ctx_rel_line_to    (Ctx *ctx, float x, float y);
void  ctx_rel_move_to    (Ctx *ctx, float x, float y);
void  ctx_rel_curve_to   (Ctx *ctx,
                          float x0, float y0,
                          float x1, float y1,
                          float x2, float y2);
void  ctx_rel_quad_to    (Ctx *ctx, float cx, float cy,
                          float x, float y);
void  ctx_close_path     (Ctx *ctx);

float ctx_get_font_size  (Ctx *ctx);

//void ctx_set_rgba_stroke_u8 (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void ctx_set_rgba_u8    (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
//void ctx_set_rgba_stroke (Ctx *ctx, float  r, float   g, float   b, float   a);
void ctx_set_rgba       (Ctx *ctx, float r, float g, float b, float a);
void ctx_set_rgb        (Ctx *ctx, float r, float g, float b);
void ctx_set_gray       (Ctx *ctx, float gray);

void ctx_set_cmyk       (Ctx *ctx, float c, float m, float y, float k);
void ctx_set_cmyka      (Ctx *ctx, float c, float m, float y, float k, float a);

void ctx_get_rgba       (Ctx *ctx, float *rgba);
void ctx_get_cmyka      (Ctx *ctx, float *cmyka);
void ctx_get_graya      (Ctx *ctx, float *ya);


void ctx_current_point  (Ctx *ctx, float *x, float *y);
float ctx_x             (Ctx *ctx);
float ctx_y             (Ctx *ctx);

int  ctx_glyph          (Ctx *ctx, uint32_t unichar, int stroke);
void ctx_arc            (Ctx  *ctx,
                         float x, float y,
                         float radius,
                         float angle1, float angle2,
                         int   direction);
void ctx_arc_to         (Ctx *ctx, float x1, float y1,
                                   float x2, float y2, float radius);
void ctx_set_global_alpha (Ctx *ctx, float global_alpha);
float ctx_get_global_alpha (Ctx *ctx);

void ctx_fill           (Ctx *ctx);
void ctx_stroke         (Ctx *ctx);
void ctx_paint          (Ctx *ctx);
void
ctx_set_pixel_u8 (Ctx *ctx, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1);
void ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0,
                                    float x1, float y1, float r1);
void ctx_gradient_clear_stops (Ctx *ctx);
void ctx_gradient_add_stop    (Ctx *ctx, float pos, float r, float g, float b, float a);

void ctx_gradient_add_stop_u8 (Ctx *ctx, float pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/*ctx_texture_init:
 *
 * return value: the actual id assigned, if id is out of range - or later
 * when -1 as id will mean auto-assign.
 */
int ctx_texture_init (Ctx *ctx, int id, int width, int height, int bpp,
                      uint8_t *pixels,
                      void (*freefunc)(void *pixels, void *user_data),
                      void *user_data);
int ctx_texture_load        (Ctx *ctx, int id, const char *path);
int ctx_texture_load_memory (Ctx *ctx, int id, const char *data, int length);

void ctx_texture_release (Ctx *ctx, int id);

void ctx_texture (Ctx *ctx, int id, float x, float y);

void ctx_image_path (Ctx *ctx, const char *path, float x, float y);

typedef struct _CtxRenderstream CtxRenderstream;
typedef void (*CtxFullCb) (CtxRenderstream *renderstream, void *data);

void _ctx_set_store_clear (Ctx *ctx);
void _ctx_set_transformation (Ctx *ctx, int transformation);
void ctx_set_full_cb (Ctx *ctx, CtxFullCb cb, void *data);

/* If cairo.h is included before ctx.h add cairo integration code
 */
#ifdef CAIRO_H
#define CTX_CAIRO 1
#else
#define CTX_CAIRO 0
#endif

#if CTX_CAIRO
void
ctx_render_cairo (Ctx *ctx, cairo_t *cr);
#endif

void ctx_render_stream (Ctx *ctx, FILE *stream, int formatter);

void ctx_render_ctx (Ctx *ctx, Ctx *d_ctx);

int
ctx_add_single (Ctx *ctx, void *entry);

uint32_t
ctx_utf8_to_unichar (const char *input);
int ctx_unichar_to_utf8 (uint32_t  ch,
                         uint8_t  *dest);

typedef enum
{
  CTX_FILL_RULE_EVEN_ODD,
  CTX_FILL_RULE_WINDING
} CtxFillRule;

typedef enum
{
  CTX_COMPOSITE_SOURCE_OVER,
  CTX_COMPOSITE_SOURCE_COPY
} CtxCompositingMode;

typedef enum
{
  CTX_BLEND_NORMAL
} CtxBlend;

typedef enum
{
  CTX_JOIN_BEVEL = 0,
  CTX_JOIN_ROUND = 1,
  CTX_JOIN_MITER = 2
} CtxLineJoin;

typedef enum
{
  CTX_CAP_NONE   = 0,
  CTX_CAP_ROUND  = 1,
  CTX_CAP_SQUARE = 2
} CtxLineCap;

typedef enum
{
  CTX_TEXT_BASELINE_ALPHABETIC = 0,
  CTX_TEXT_BASELINE_TOP,
  CTX_TEXT_BASELINE_HANGING,
  CTX_TEXT_BASELINE_MIDDLE,
  CTX_TEXT_BASELINE_IDEOGRAPHIC,
  CTX_TEXT_BASELINE_BOTTOM
} CtxTextBaseline;

typedef enum
{
  CTX_TEXT_ALIGN_START = 0,
  CTX_TEXT_ALIGN_END,
  CTX_TEXT_ALIGN_CENTER,
  CTX_TEXT_ALIGN_LEFT,
  CTX_TEXT_ALIGN_RIGHT
} CtxTextAlign;

typedef enum
{
  CTX_TEXT_DIRECTION_INHERIT = 0,
  CTX_TEXT_DIRECTION_LTR,
  CTX_TEXT_DIRECTION_RTL
} CtxTextDirection;

void ctx_set_fill_rule        (Ctx *ctx, CtxFillRule fill_rule);
void ctx_set_line_cap         (Ctx *ctx, CtxLineCap cap);
void ctx_set_line_join        (Ctx *ctx, CtxLineJoin join);
void ctx_set_compositing_mode (Ctx *ctx, CtxCompositingMode mode);
int ctx_set_renderstream      (Ctx *ctx, void *data, int length);
int ctx_append_renderstream   (Ctx *ctx, void *data, int length);


/* these are only needed for clients renderin text, as all text gets
 * converted to paths.
 */

void  ctx_glyphs       (Ctx        *ctx,
                        CtxGlyph   *glyphs,
                        int         n_glyphs);
void  ctx_glyphs_stroke (Ctx       *ctx,
                        CtxGlyph   *glyphs,
                        int         n_glyphs);
void  ctx_text         (Ctx        *ctx,
                        const char *string);
void  ctx_text_stroke  (Ctx        *ctx,
                        const char *string);
/* return the width of provided string if it had been rendered */
float ctx_text_width   (Ctx        *ctx,
                        const char *string);
float ctx_glyph_width (Ctx *ctx, int unichar);

int   ctx_load_font_ttf (const char *name, const void *ttf_contents, int length);


/* definitions that determine which features are included and their settings,
 * for particular platforms - in particular microcontrollers ctx might need
 * tuning for different quality/performance/resource constraints.
 *
 * the way to configure ctx is to set these defines, before both including it
 * as a header and in the file where CTX_IMPLEMENTATION is set to include the
 * implementation for different featureset and runtime settings.
 *
 */

#ifndef CTX_RASTERIZER  // set to 0 before to disable renderer code, useful for clients that only
                        // build journals.
#define CTX_RASTERIZER   1
#endif

/* experimental feature, not fully working - where text rendering happens
 * closer to rasterizer. Positions are screwed up in playback  */
#ifndef CTX_BACKEND_TEXT
#define CTX_BACKEND_TEXT 1
#endif

/* vertical level of supersampling at full/forced AA.
 *
 * 1 is none, 2 is faster, 3 is fast 5 is good 15 is best for 8bit  51 is
 *
 *
 * valid values:
 * 1 2 3 5 15 17 51 85
 */
#ifndef CTX_RASTERIZER_AA
#define CTX_RASTERIZER_AA       5
#endif

#define CTX_RASTERIZER_AA2     (CTX_RASTERIZER_AA/2)
#define CTX_RASTERIZER_AA3     (CTX_RASTERIZER_AA/2+CTX_RASTERIZER_AA%2)


/* force full antialising */
#ifndef CTX_RASTERIZER_FORCE_AA
#define CTX_RASTERIZER_FORCE_AA  0
#endif

/* when AA is not forced, the slope below which full AA get enabled.
 */
#ifndef CTX_RASTERIZER_AA_SLOPE_LIMIT
#define CTX_RASTERIZER_AA_SLOPE_LIMIT    512
#endif

/* subpixel-aa coordinates used in BITPACKing of renderstream
 */
#define CTX_SUBDIV            8 // changing this changes font-file-format

// 8    12 68 40 24
// 16   12 68 40 24
/* scale-factor for font outlines prior to bit quantization by CTX_SUBDIV
 */
#define CTX_BAKE_FONT_SIZE   160

/* pack some linetos/curvetos/movetos into denser renderstream indstructions,
 * permitting more vectors to be stored in the same space.
 */
#ifndef CTX_BITPACK
#define CTX_BITPACK           1
#endif

/* reference-packing, look for recurring patterns in renderstream and encode
 * subsequent references as references to prior occurences. (currently slow
 * and/or broken, thus disabled by default.)
 */
#ifndef CTX_REFPACK
#define CTX_REFPACK           0
#endif

/* whether we have a shape-cache where we keep pre-rasterized bitmaps of commonly
 * occuring small shapes.
 */
#ifndef CTX_SHAPE_CACHE
#define CTX_SHAPE_CACHE    0
#endif

/* size (in pixels, w*h) that we cache rasterization for
 */
#ifndef CTX_SHAPE_CACHE_DIM
#define CTX_SHAPE_CACHE_DIM      (16*16)
#endif

#ifndef CTX_SHAPE_CACHE_MAX_DIM
#define CTX_SHAPE_CACHE_MAX_DIM  32
#endif

/* maximum number of entries in shape cache
 */
#ifndef CTX_SHAPE_CACHE_ENTRIES
#define CTX_SHAPE_CACHE_ENTRIES  160
#endif

/* implement a chache for gradient rendering; that cuts down the
 * per-pixel cost for complex gradients
 */
#ifndef CTX_GRADIENT_CACHE
#define CTX_GRADIENT_CACHE 1
#endif

#ifndef CTX_FONTS_FROM_FILE
#define CTX_FONTS_FROM_FILE 1
#endif

#ifndef CTX_FORMATTER 
#define CTX_FORMATTER 0
#endif

#ifndef CTX_PARSER
#define CTX_PARSER 0
#endif

/* when ctx_math is defined, which it is by default, we use ctx' own
 * implementations of math functions, instead of relying on math.h
 * the possible inlining gives us a slight speed-gain, and on
 * embedded platforms guarantees that we do not do double precision
 * math.
 */
#ifndef CTX_MATH
#define CTX_MATH           1  // use internal fast math for sqrt,sin,cos,atan2f etc.
#endif

#define ctx_log(fmt, ...)
//#define ctx_log(str, a...) fprintf(stderr, str, ##a)

/* the initial journal size - for both rasterizer
 * edgelist and renderstram.
 */
#ifndef CTX_MIN_JOURNAL_SIZE
#define CTX_MIN_JOURNAL_SIZE   128
#endif

/* The maximum size we permit the renderstream to grow to,
 * the memory used is this number * 9, where 9 is sizeof(CtxEntry)
 */
#ifndef CTX_MAX_JOURNAL_SIZE
#define CTX_MAX_JOURNAL_SIZE   1024*10
#endif

#ifndef CTX_RENDERSTREAM_STATIC
#define CTX_RENDERSTREAM_STATIC 0
#endif

#ifndef CTX_MIN_EDGE_LIST_SIZE
#define CTX_MIN_EDGE_LIST_SIZE   128
#endif

/* The maximum size we permit the renderstream to grow to
 */
#ifndef CTX_MAX_EDGE_LIST_SIZE
#define CTX_MAX_EDGE_LIST_SIZE   1024
#endif

/* whether we dither or not for gradients
 */
#ifndef CTX_DITHER
#define CTX_DITHER 1
#endif

#ifndef CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK  1
#endif

#if CTX_ENABLE_CMYK
#ifndef CTX_ENABLE_CMYKAF
#define CTX_ENABLE_CMYKAF 1
#endif
#endif

/* by default ctx includes all pixel formats, on microcontrollers
 * it can be useful to slim down code and runtime size by only
 * defining the used formats, set CTX_LIMIT_FORMATS to 1, and
 * manually add CTX_ENABLE_ flags for each of them.
 */
#ifndef CTX_LIMIT_FORMATS

#define CTX_ENABLE_GRAY8                1
#define CTX_ENABLE_GRAYA8               1
#define CTX_ENABLE_RGB8                 1
#define CTX_ENABLE_RGBA8                1
#define CTX_ENABLE_BGRA8                1
#define CTX_ENABLE_RGB332               1
#define CTX_ENABLE_RGB565               1
#define CTX_ENABLE_RGB565_BYTESWAPPED   1
#define CTX_ENABLE_RGBAF                1
#define CTX_ENABLE_GRAYF                1
#define CTX_ENABLE_GRAY1                1
#define CTX_ENABLE_GRAY2                1
#define CTX_ENABLE_GRAY4                1
#define CTX_ENABLE_CMYK8                1
#define CTX_ENABLE_CMYKA8               1
#define CTX_ENABLE_CMYKAF               1

#endif

#define CTX_RASTERIZER_EDGE_MULTIPLIER  1024

/* by including ctx-font-regular.h, or ctx-font-mono.h the
 * built-in fonts using ctx renderstream encoding is enabled
 */
#if CTX_FONT_regular || CTX_FONT_mono || CTX_FONT_bold \
  || CTX_FONT_italic || CTX_FONT_sans || CTX_FONT_serif
#ifndef CTX_FONT_ENGINE_CTX
  #define CTX_FONT_ENGINE_CTX        1
#endif
#endif


/* If stb_strutype.h is included before ctx.h add integration code for runtime loading
 * of opentype fonts.
 */
#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
  #ifndef CTX_FONT_ENGINE_STB
    #define CTX_FONT_ENGINE_STB      1
  #endif
#else
  #define CTX_FONT_ENGINE_STB        0
#endif

/* force add format if we have shape cache */
#if CTX_SHAPE_CACHE
#ifdef CTX_ENABLE_GRAY8
#undef CTX_ENABLE_GRAY8
#endif
#define CTX_ENABLE_GRAY8  1
#endif

/* include the bitpack packer, can be opted out of to decrease code size
 */
#ifndef CTX_BITPACK_PACKER
#define CTX_BITPACK_PACKER 1
#endif

/* enable RGBA8 intermediate format for
 *the indirectly implemented pixel-formats.
 */
#if CTX_ENABLE_GRAY1 | CTX_ENABLE_GRAY2 | CTX_ENABLE_GRAY4 | CTX_ENABLE_RGB565 | CTX_ENABLE_RGB565_BYTESWAPPED | CTX_ENABLE_RGB8 | CTX_ENABLE_RGB332

#ifdef CTX_ENABLE_RGBA8
#undef CTX_ENABLE_RGBA8
#endif
#define CTX_ENABLE_RGBA8  1

#endif


#define CTX_PI               3.141592653589793f
#ifndef CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS  100
#endif

#ifndef CTX_MAX_FONTS
#define CTX_MAX_FONTS        3
#endif

#ifndef CTX_MAX_STATES
#define CTX_MAX_STATES       10
#endif

#ifndef CTX_MAX_EDGES
#define CTX_MAX_EDGES        257
#endif

#ifndef CTX_MAX_LINGERING_EDGES
#define CTX_MAX_LINGERING_EDGES 32
#endif

#ifndef CTX_MAX_GRADIENTS
#define CTX_MAX_GRADIENTS    1
#endif

#ifndef CTX_MAX_TEXTURES
#define CTX_MAX_TEXTURES     16
#endif

#ifndef CTX_MAX_PENDING
#define CTX_MAX_PENDING      128
#endif

#ifndef CTX_FULL_CB
#define CTX_FULL_CB 0
#endif

#ifndef CTX_RENDER_CTX
#define CTX_RENDER_CTX       1
#endif

#define CTX_ASSERT 0

#if CTX_ASSERT==1
#define ctx_assert(a)  if(!(a)){fprintf(stderr,"%s:%i assertion failed\n", __FUNCTION__, __LINE__);  }
#else
#define ctx_assert(a)
#endif


#if CTX_FONTS_FROM_FILE
int   ctx_load_font_ttf_file (const char *name, const char *path);
#endif

int ctx_get_renderstream_count (Ctx *ctx);

typedef enum
{

  // items marked with % are currently only for the parser
  // for instance for svg compatibility or simulated/converted color spaces
  // not the serialization/internal render stream
  CTX_FLUSH            = 0,
  CTX_ARC_TO           = 'A', // SVG %
  CTX_ARC              = 'B',
  CTX_CURVE_TO         = 'C', // SVG float x, y, followed by two ; with rest of coords
  CTX_RESTORE          = 'D',
  CTX_STROKE           = 'E',
  CTX_FILL             = 'F',
  CTX_HOR_LINE_TO      = 'H', // SVG %
  CTX_ROTATE           = 'J', // float
  CTX_SET_COLOR        = 'K', //              [ byte, entries       ] 
  CTX_LINE_TO          = 'L', // SVG float x, y
  CTX_MOVE_TO          = 'M', // SVG float x, y
  CTX_SCALE            = 'O', // float, float
  CTX_NEW_PAGE         = 'P', // NYI
  CTX_QUAD_TO          = 'Q', // SVG
  CTX_MEDIA_BOX        = 'R', //
  CTX_SMOOTH_TO        = 'S', // SVG %
  CTX_SMOOTHQ_TO       = 'T', // SVG %
  CTX_CLEAR            = 'U',
  CTX_VER_LINE_TO      = 'V', // SVG %
  CTX_SET_TRANSFORM    = 'W',
  CTX_EXIT             = 'X',
  // Z - SVG?
  CTX_REL_ARC_TO       = 'a', // SVG %
  CTX_CLIP             = 'b',
  CTX_REL_CURVE_TO     = 'c', // SVG 
  CTX_SAVE             = 'd',
  CTX_TRANSLATE        = 'e', // float, float
  CTX_LINEAR_GRADIENT  = 'f',
  CTX_GLYPH            = 'g', // unichar, fontsize
  CTX_REL_HOR_LINE_TO  = 'h', // SVG %
  CTX_TEXTURE          = 'i',
  CTX_REL_LINE_TO      = 'l', // SVG
  CTX_REL_MOVE_TO      = 'm', // SVG
  CTX_SET_FONT         = 'n', // as used by text parser
  CTX_RADIAL_GRADIENT  = 'o',
  CTX_GRADIENT_STOP    = 'p',
  CTX_REL_QUAD_TO      = 'q',
  CTX_RECTANGLE        = 'r',
  CTX_REL_SMOOTH_TO    = 's', // SVG
  CTX_REL_SMOOTHQ_TO   = 't', // SVG
  CTX_TEXT_STROKE      = 'u', //
  CTX_REL_VER_LINE_TO  = 'v',
  CTX_SET_PARAM        = 'w', 
  CTX_TEXT             = 'x', // x, y - followed by "" in CTX_DATA
  CTX_IDENTITY         = 'y', // < can be achieved with set_transform
  CTX_CLOSE_PATH       = 'z',
  CTX_NEW_PATH         = 'N',

  /* these commands have single byte binary representations,
   * but are two chars in text, values below 9 are used for
   * low integers of enum values. and can thus not be used here
   */
  CTX_SET_GLOBAL_ALPHA     = 10, // wa
  CTX_SET_COMPOSITING_MODE = 11, // wc
  CTX_SET_FONT_SIZE        = 12, // wf
  CTX_SET_LINE_JOIN        = 13, // wj
  CTX_SET_LINE_CAP         = 14, // wc
  CTX_SET_LINE_WIDTH       = 15, // ww
  CTX_SET_FILL_RULE        = 16, // wr
  CTX_SET_TEXT_ALIGN       = 17, // wt
  CTX_SET_TEXT_BASELINE    = 18, // wb
  CTX_SET_TEXT_DIRECTION   = 19, // wd
  CTX_SET_MITER_LIMIT      = 20, // wm

  CTX_DEFUN  = 21,
  CTX_ENDFUN = 22,

  // non-alphabetic chars that get filtered out when parsing
  // are used for internal purposes
  //
  // unused:  . , : backslash ! # $ % ^ { } < > ? &
  CTX_SET_RGBA_U8      = '*', // u8
  CTX_GRADIENT_CLEAR   = '/',
  CTX_BITPIX           = 'I', // x, y, width, height, scale
  CTX_BITPIX_DATA      = 'j', //

  CTX_NOP              = ' ',
  CTX_NEW_EDGE         = '+',
  CTX_EDGE             = '|',
  CTX_EDGE_FLIPPED     = '`',

  CTX_REPEAT_HISTORY   = ']', //
  CTX_CONT             = ';',
  CTX_DATA             = '(', // size,  size-in-entries
  CTX_DATA_REV         = ')', // reverse traversal data marker
  CTX_DEFINE_GLYPH     = '@',
  CTX_KERNING_PAIR     = '[',
  CTX_SET_PIXEL        = '-',

  /* optimizations that reduce the number of entries used,
   * not visible outside the draw-stream compression -
   * these are using values that would mean numbers in an
   * SVG path.
   */
#if CTX_BITPACK
  CTX_REL_LINE_TO_X4            = '0',
  CTX_REL_LINE_TO_REL_CURVE_TO  = '1',
  CTX_REL_CURVE_TO_REL_LINE_TO  = '2',
  CTX_REL_CURVE_TO_REL_MOVE_TO  = '3',
  CTX_REL_LINE_TO_X2            = '4',
  CTX_MOVE_TO_REL_LINE_TO       = '5',
  CTX_REL_LINE_TO_REL_MOVE_TO   = '6',
  CTX_FILL_MOVE_TO              = '7',
  CTX_REL_QUAD_TO_REL_QUAD_TO   = '8',
  CTX_REL_QUAD_TO_S16           = '9',
#endif
} CtxCode;

typedef enum {
  CTX_GRAY = 1,
  CTX_RGB  = 3,
  CTX_CMYK = 4,
  CTX_LAB  = 5,
  CTX_LCH  = 6,

  CTX_GRAYA   = 101,
  CTX_GRAYA_A = 201,
  CTX_RGBA    = 103,
  CTX_RGBA_A  = 203,
  CTX_CMYKA   = 104,
  CTX_CMYKA_A = 204,
  CTX_LABA    = 105,
  CTX_LCHA    = 106,

  // RGB  device and  RGB  ?
  // 
  //
  //
} CtxColorModel;

typedef struct _CtxEntry CtxEntry;

/* we only care about the tight packing for this specific
 * struct, to make sure its size becomes 9bytes -
 * the pack pragma is also sufficient on recent gcc versions
 */
#pragma pack(push,1)

struct _CtxEntry
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
    uint64_t u64[1];
  } data;
};

#pragma pack(pop)


#ifdef __cplusplus
  }
#endif
#endif

typedef struct _CtxParser CtxParser;
CtxParser *ctx_parser_new (
  Ctx       *ctx,
  int        width,
  int        height,
  float      cell_width,
  float      cell_height,
  int        cursor_x,
  int        cursor_y,
  void (*exit)(void *exit_data),
  void *exit_data);
void ctx_parser_free (CtxParser *parser);
void ctx_parser_feed_byte (CtxParser *parser, int byte);

#define CTX_CLAMP(val,min,max) ((val)<(min)?(min):(val)>(max)?(max):(val))

#ifdef CTX_IMPLEMENTATION


static void
ctx_memset (void *ptr, uint8_t val, int length)
{
  uint8_t *p = ptr;
  for (int i = 0; i < length; i ++)
    p[i] = val;
}

#define ctx_pow2(a) ((a)*(a))
static float
ctx_minf (float a, float b)
{
  if (a < b)
    return a;
  return b;
}

static float
ctx_maxf (float a, float b)
{
  if (a > b)
    return a;
  return b;
}
#if CTX_MATH
  /* a tiny self contained c math library containing what we need,
   * defined in terms of sqrt(x) sinf and atan2f
   */

static float
ctx_fabsf (float x)
{
  union {
    float f;
    uint32_t i;
  } u = { x };
  u.i &= 0x7fffffff;
  return u.f;
}

static float
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

static float
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
    if (x < 0.0f)
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

#define sqrtf(a)     (1.0f/ctx_invsqrtf(a))
#define hypotf(a,b)  sqrtf(ctx_pow2(a)+ctx_pow2(b))
#define sinf(a)      ctx_sinf(a)
#define fabsf(a)     ctx_fabsf(a)
#define atan2f(a,b)  ctx_atan2f((a), (b))

/* define more trig based on having sqrt, sin and atan2 */
#define atanf(a)     atan2f((a), 1.0f)
#define asinf(x)     atanf((x)*(ctxinv_sqrtf(1.0f-ctx_pow2(x))))
#define acosf(x)     atanf((sqrtf(1.0f-ctx_pow2(x))/(x)))
#define cosf(a)      sinf((a) + CTX_PI/2.0f)
#define tanf(a)      (cosf(a)/sinf(a))

#else

#include <math.h>

#endif

static float ctx_fast_hypotf (float x, float y)
{
  if (x < 0) x = -x;
  if (y < 0) y = -y;

  if (x < y)
    return 0.96f * y + 0.4f * x;
  else
    return 0.96f * x + 0.4f * y;
}

#define ctx_arg_float(no) entry[(no)>>1].data.f[(no)&1]
#define ctx_arg_u32(no)   entry[(no)>>1].data.u32[(no)&1]
#define ctx_arg_s32(no)   entry[(no)>>1].data.s32[(no)&1]
#define ctx_arg_u16(no)   entry[(no)>>2].data.u16[(no)&3]
#define ctx_arg_s16(no)   entry[(no)>>2].data.s16[(no)&3]
#define ctx_arg_u8(no)    entry[(no)>>3].data.u8[(no)&7]
#define ctx_arg_s8(no)    entry[(no)>>3].data.s8[(no)&7]
#define ctx_arg_string()  ((char*)&entry[2].data.u8[0])

typedef struct _CtxRenderer CtxRenderer;
typedef struct _CtxGState   CtxGState;
typedef struct _CtxState    CtxState;
typedef struct _CtxMatrix   CtxMatrix;
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
  void (*free_func) (void *pixels, void *user_data);
  void               *user_data;
};

void ctx_user_to_device          (CtxState *state, float *x, float *y);
void ctx_user_to_device_distance (CtxState *state, float *x, float *y);

typedef struct _CtxGradient CtxGradient;
struct _CtxGradient
{
  CtxGradientStop stops[16];
  int n_stops;
};

#define CTX_VALID_RGBA_U8   (1<<0)
#define CTX_VALID_RGBA      (1<<1)
#if CTX_ENABLE_CMYK
#define CTX_VALID_CMYKA_U8  (1<<2)
#define CTX_VALID_CMYKA     (1<<3)
#endif
#define CTX_VALID_GRAYA     (1<<4)
#define CTX_VALID_GRAYA_U8  (1<<5)
#define CTX_VALID_LABA      ((1<<6) | CTX_VALID_GRAYA)

//_ctx_target_space (ctx, icc);
//_ctx_space (ctx);


typedef struct _CtxColor CtxColor;
struct _CtxColor
{
  uint8_t rgba[4];
  uint8_t original; // the bitmask of the originally set color
  uint8_t valid;    // bitmask of which members contain valid
                    // values, gets denser populated as more
                    // formats are requested from a set color.
  float   red;
  float   green;
  float   blue;
  float   alpha;
  float   l;        // luminance and gray
#if CTX_ENABLE_LAB  // NYI
  float   a;
  float   b;
#endif

  uint8_t l_u8;
#if CTX_ENABLE_CMYK
  uint8_t cmyka[5];
  float   cyan;
  float   magenta;
  float   yellow;
  float   key;
#endif 

#if CTX_ENABLE_CM
  void   *space;   // a babl_space when not direct
  // cmyk values are presumed to always be in
  // ICC space and the color values set are not
  // influenced by color management. RGB values
  // however are.
  float   cm_red;
  float   cm_green;
  float   cm_blue;
#endif
};

static void ctx_color_set_rgba_u8 (CtxColor *color, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  color->original = color->valid = CTX_VALID_RGBA_U8;
  color->rgba[0] = r;
  color->rgba[1] = g;
  color->rgba[2] = b;
  color->rgba[3] = a;
}

#if 0
static void ctx_color_set_rgba_u8_ (CtxColor *color, const uint8_t *in)
{
  ctx_color_set_rgba_u8 (color, in[0], in[1], in[2], in[3]);
}
#endif

static void ctx_color_set_graya (CtxColor *color, float gray, float alpha)
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

static void ctx_color_set_rgba (CtxColor *color, float r, float g, float b, float a)
{
  color->original = color->valid = CTX_VALID_RGBA;
  color->red   = r;
  color->green = g;
  color->blue  = b;
  color->alpha = a;
}

#if 0
static void ctx_color_set_rgba_ (CtxColor *color, const float *in)
{
  ctx_color_set_rgba (color, in[0], in[1], in[2], in[3]);
}
#endif

#if CTX_ENABLE_CMYK
static void ctx_color_set_cmyka (CtxColor *color, float c, float m, float y, float k, float a)
{
  color->original = color->valid = CTX_VALID_CMYKA;
  color->cyan    = c;
  color->magenta = m;
  color->yellow  = y;
  color->key     = k;
  color->alpha   = a;
}

#if 0
static void ctx_color_set_cmyka_ (CtxColor *color, const float *in)
{
  ctx_color_set_cmyka (color, in[0], in[1], in[2], in[3], in[4]);
}
#endif

#endif

static void ctx_color_get_rgba (CtxColor *color, float *out)
{
  if (!(color->valid & CTX_VALID_RGBA))
  {
    if (color->valid & CTX_VALID_RGBA_U8)
    {
      color->red    = color->rgba[0]/255.0f;
      color->green  = color->rgba[1]/255.0f;
      color->blue   = color->rgba[2]/255.0f;
      color->alpha  = color->rgba[3]/255.0f;
    }
#if CTX_ENABLE_CMYK
    else if (color->valid & CTX_VALID_CMYKA)
    {
      color->red    = (1.0f-color->cyan) * (1.0-color->key);
      color->green  = (1.0f-color->magenta) * (1.0-color->key);
      color->blue   = (1.0f-color->yellow) * (1.0-color->key);
    }
#endif
    else if (color->valid & CTX_VALID_GRAYA)
    {
      color->red    = 
      color->green  =
      color->blue   = color->l;
    }
    color->valid |= CTX_VALID_RGBA;
  }
  out[0] = color->red;
  out[1] = color->green;
  out[2] = color->blue;
  out[3] = color->alpha;
}

static void ctx_color_get_graya (CtxColor *color, float *out)
{
  if (!(color->valid & CTX_VALID_GRAYA))
  {
    float rgba[4];
    ctx_color_get_rgba (color, rgba);
    color->l = (rgba[0] + rgba[1] + rgba[2])/3.0f; // XXX
    color->valid |= CTX_VALID_GRAYA;
  }
  out[0] = color->l;
  out[1] = color->alpha;
}

#if CTX_ENABLE_CMYK
static void ctx_color_get_cmyka (CtxColor *color, float *out)
{
  if (!(color->valid & CTX_VALID_CMYKA))
  {
    if (color->valid & CTX_VALID_GRAYA)
    {
      color->cyan = color->magenta = color->yellow = 0.0;
      color->key = color->l;
    }
    else
    {
      float rgba[4];
      float key;
      ctx_color_get_rgba (color, rgba);
      color->cyan = 1.0f - rgba[0];
      color->magenta = 1.0f - rgba[1];
      color->yellow = 1.0f - rgba[2];
      key = ctx_minf(color->cyan, ctx_minf(color->yellow, color->magenta));
      color->key = key;
      if (color->key < 1.0f)
      {
        color->cyan = (color->cyan - key) / (1.0f - key);
        color->magenta = (color->magenta - key) / (1.0f - key);
        color->yellow = (color->yellow - key) / (1.0f - key);
      }
      else
      {
        color->cyan = color->magenta = color->yellow = 0.0f;
      }
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
static void ctx_color_get_cmyka_u8 (CtxColor *color, uint8_t *out)
{
  if (!(color->valid & CTX_VALID_CMYKA_U8))
  {
    float cmyka[5];
    ctx_color_get_cmyka (color, cmyka);
    for (int i = 0; i < 5; i ++)
      color->cmyka[i] = cmyka[i] * 255.99f;
    color->valid |= CTX_VALID_CMYKA_U8;
  }
  out[0] = color->cmyka[0];
  out[1] = color->cmyka[1];
  out[2] = color->cmyka[2];
  out[3] = color->cmyka[3];
}
#endif


#endif

static void ctx_color_get_rgba_u8 (CtxColor *color, uint8_t *out)
{
  if (!(color->valid & CTX_VALID_RGBA_U8))
  {
    float rgba[4];
    ctx_color_get_rgba (color, rgba);
    for (int i = 0; i < 4; i ++)
      color->rgba[i] = rgba[i] * 255.99f;
    color->valid |= CTX_VALID_RGBA_U8;
  }
  out[0] = color->rgba[0];
  out[1] = color->rgba[1];
  out[2] = color->rgba[2];
  out[3] = color->rgba[3];
}

#if 0
static void ctx_color_get_graya_u8 (CtxColor *color, uint8_t *out)
{
  if (!(color->valid & CTX_VALID_GRAYA_U8))
  {
    float graya[2];
    ctx_color_get_graya (color, graya);
    color->l_u8 = graya[0] * 255.99f;
    color->rgba[3] = graya[1] * 255.99f;
    color->valid |= CTX_VALID_GRAYA_U8;
  }
  out[0] = color->l_u8;
  out[1] = color->rgba[3];
}
#endif


struct _CtxSource
{
  int type;
  CtxMatrix  transform;
  union {
    CtxColor color;
    struct {
      uint8_t rgba[4]; // shares data with set color
      uint8_t pad;
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
  CtxMatrix     transform;
  //CtxSource   source_stroke;
  CtxSource     source;
  CtxColorModel color_model;
  uint8_t       global_alpha_u8;
  float         global_alpha_f;
  float         line_width;
  float         miter_limit;
  float         font_size;
  float         line_spacing;

  float         shadow_blur;
  float         shadow_x;
  float         shadow_y;
  //CtxSource   shadow_source;
  //CtxColor    shadow_color;
  int16_t       clip_min_x;
  int16_t       clip_min_y;
  int16_t       clip_max_x;
  int16_t       clip_max_y;

  /* bitfield-pack small state-parts */
  CtxCompositingMode  compositing_mode:2;
  CtxBlend                  blend_mode:3;
  CtxLineCap                  line_cap:2;
  CtxLineJoin                line_join:2;
  CtxFillRule                fill_rule:1;
  CtxTextBaseline        text_baseline:3;
  CtxTextAlign              text_align:3;
  CtxTextDirection      text_direction:2;
  unsigned int                    font:6;
  unsigned int                    bold:1;
  unsigned int                  italic:1;
};

typedef enum {
  CTX_TRANSFORMATION_NONE         = 0,
  CTX_TRANSFORMATION_SCREEN_SPACE = 1,
  CTX_TRANSFORMATION_RELATIVE     = 2,
#if CTX_BITPACK
  CTX_TRANSFORMATION_BITPACK      = 4,
#endif
  CTX_TRANSFORMATION_REFPACK      = 8,
  CTX_TRANSFORMATION_STORE_CLEAR  = 16,
} CtxTransformation;

#define CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES   64
#define CTX_RENDERSTREAM_EDGE_LIST            128

struct _CtxRenderstream
{
  /* should have a list of iterator state initializer. pos, depth and
     history_pos init info for every 1024 entries, will be most needed
     with wire-protocol and reuse of data from preceding frame.
   */
  CtxEntry *entries;     /* we need to use realloc */
  int       size;
  int       count;
  uint32_t  flags; // BITPACK and REFPACK - to be used on resize
  int       bitpack_pos;
#if CTX_FULL_CB
  int       full_cb_entries;
  CtxFullCb full_cb;
  void     *full_cb_data;
#endif
};

struct _CtxState {
  int         has_moved:1;
  float       x;
  float       y;
  int         min_x;
  int         min_y;
  int         max_x;
  int         max_y;
  CtxGradient gradient; /* we keep only one gradient,
                           this goes icky with multiple
                           restores - it should really be part of
                           graphics state.. XXX */

  int16_t     gstate_no;
  CtxGState   gstate;
  CtxGState   gstate_stack[CTX_MAX_STATES];//at end, so can be made dynamic
};

#if CTX_RASTERIZER

typedef struct CtxEdge {
  int32_t  x;     /* the center-line intersection      */
  int32_t  dx;
  uint16_t index;
} CtxEdge;

struct _CtxRenderer {
  /* these should be initialized and used as the bounds for rendering into the
     buffer as well XXX: not yet in use, and when in use will only be
     correct for axis aligned clips - proper rasterization of a clipping path
     would be yet another refinement on top.
   */
  //CtxBuffer *clip_buffer; // NYI

  CtxEdge  lingering[CTX_MAX_LINGERING_EDGES];
  int      lingering_edges;  // previous half scanline

  CtxEdge  edges[CTX_MAX_EDGES];
  int      active_edges;
  int      pending_edges;    // next half scanline

  int      edge_pos;         // where we're at in iterating all edges

  int      scanline;
  int      scan_min;
  int      scan_max;
  int      col_min;
  int      col_max;

  CtxRenderstream edge_list;
  CtxState  *state;
  Ctx       *ctx;

  void      *buf;
  float      x;
  float      y;

  float      first_x;
  float      first_y;
  int        has_shape;
  int        has_prev;

  int8_t     needs_aa; // count of how many edges implies antialiasing
  int        uses_transforms:1;

  int16_t    blit_x;
  int16_t    blit_y;
  int16_t    blit_width;
  int16_t    blit_height;
  int16_t    blit_stride;

  CtxPixelFormatInfo *format;
};

struct _CtxPixelFormatInfo
{
  CtxPixelFormat pixel_format;
  uint8_t        components:4; /* number of components */
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

#endif

struct _Ctx {
#if CTX_RASTERIZER
  CtxRenderer      *renderer;
#endif
  CtxRenderstream   renderstream;
  CtxState          state;
  int               transformation;
  CtxBuffer         texture[CTX_MAX_TEXTURES];
};

typedef struct _CtxFont       CtxFont;
typedef struct _CtxFontEngine CtxFontEngine;

struct _CtxFontEngine
{
#if CTX_FONTS_FROM_FILE
   int   (*load_file)   (const char *name, const char *path);
#endif
   int   (*load_memory) (const char *name, const void *data, int length);
   int   (*glyph)       (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke);
   float (*glyph_width) (CtxFont *font, Ctx *ctx, uint32_t unichar);
   float (*glyph_kern)  (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB);
};

struct _CtxFont
{
  CtxFontEngine *engine;
  const char *name;
  int type; // 0 ctx    1 stb    2 monobitmap
  union {
    struct { CtxEntry *data; int length; int first_kern;
             /* we've got ~110 bytes to fill to cover as
                much data as stbtt_fontinfo */
             //int16_t glyph_pos[26]; // for a..z
             int       glyphs; // number of glyphs
             uint32_t *index;
           } ctx;
#if CTX_FONT_ENGINE_STB
    struct { stbtt_fontinfo ttf_info;
             int cache_index;
             int cache_unichar;
           } stb;
#endif
    struct { int start; int end; int gw; int gh; const uint8_t *data;} monobitmap;
  };
};

static CtxFont ctx_fonts[CTX_MAX_FONTS];
static int     ctx_font_count = 0;

void ctx_dirty_rect (Ctx *ctx, int *x, int *y, int *width, int *height)
{
  if ((ctx->state.min_x > ctx->state.max_x) ||
      (ctx->state.min_y > ctx->state.max_y))
  {
    if (x) *x = 0;
    if (y) *y = 0;
    if (width) *width = 0;
    if (height) *height = 0;
    return;
  }
  if (ctx->state.min_x < 0)
    ctx->state.min_x = 0;
  if (ctx->state.min_y < 0)
    ctx->state.min_y = 0;


  if (x) *x = ctx->state.min_x;
  if (y) *y = ctx->state.min_y;
  if (width) *width = ctx->state.max_x - ctx->state.min_x;
  if (height) *height = ctx->state.max_y - ctx->state.min_y;

  //fprintf (stderr, "%i %i %ix%i\n", *x, *y, *width, *height);
}

void ctx_process (Ctx *ctx, CtxEntry *entry);

static void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f)
{
  matrix->m[0][0] = a; matrix->m[0][1] = b;
  matrix->m[1][0] = c; matrix->m[1][1] = d;
  matrix->m[2][0] = e; matrix->m[2][1] = f;
}

static void
ctx_matrix_identity (CtxMatrix *matrix)
{
  matrix->m[0][0] = 1.0f; matrix->m[0][1] = 0.0f;
  matrix->m[1][0] = 0.0f; matrix->m[1][1] = 1.0f;
  matrix->m[2][0] = 0.0f; matrix->m[2][1] = 0.0f;
}

static void
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

static void
ctx_matrix_translate (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = 1.0f; transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f; transform.m[1][1] = 1.0f;
  transform.m[2][0] = x;    transform.m[2][1] = y;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
ctx_matrix_scale (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = x;    transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f; transform.m[1][1] = y;
  transform.m[2][0] = 0.0f; transform.m[2][1] = 0.0f;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
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

#if 0
static void
ctx_matrix_skew_x (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = tanf (angle);
  transform.m[0][0] =    1.0f; transform.m[0][1] = 0.0f;
  transform.m[1][0] = val_tan; transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f; transform.m[2][1] = 0.0f;

  ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
ctx_matrix_skew_y (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = tanf (angle);
  transform.m[0][0] =    1.0f; transform.m[0][1] = val_tan;
  transform.m[1][0] =    0.0f; transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f; transform.m[2][1] = 0.0f;

  ctx_matrix_multiply (matrix, &transform, matrix);
}
#endif

void
ctx_get_rgba (Ctx *ctx, float *rgba)
{
  ctx_color_get_rgba (&ctx->state.gstate.source.color, rgba);
}
#if CTX_ENABLE_CMYK
void
ctx_get_cmyka (Ctx *ctx, float *cmyka)
{
  ctx_color_get_cmyka (&ctx->state.gstate.source.color, cmyka);
}
#endif
void
ctx_get_graya (Ctx *ctx, float *ya)
{
  ctx_color_get_graya (&ctx->state.gstate.source.color, ya);
}

static int
ctx_conts_for_entry (CtxEntry *entry)
{
  switch (entry->code)
  //switch ((CtxCode)(cmd))  //  XXX  should be exhaustive
  {
    case CTX_DATA:
      return entry->data.u32[1];
    case CTX_LINEAR_GRADIENT:
      return 1;
    case CTX_RADIAL_GRADIENT:
    case CTX_ARC:
    case CTX_CURVE_TO:
    case CTX_REL_CURVE_TO:
    case CTX_SET_TRANSFORM:
    case CTX_SET_COLOR:
      return 2;
    case CTX_RECTANGLE:
    case CTX_REL_QUAD_TO:
    case CTX_QUAD_TO:
    case CTX_TEXTURE:
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
  CtxRenderstream *renderstream;
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
                   CtxRenderstream  *renderstream,
                   int          start_pos,
                   int          flags)
{
  iterator->renderstream   = renderstream;
  iterator->flags          = flags;
  iterator->bitpack_pos    = 0;
  iterator->bitpack_length = 0;
  iterator->history_pos    = 0;
  iterator->history[0]     = 0;
  iterator->history[1]     = 0;
  iterator->pos            = start_pos;
  iterator->end_pos        = renderstream->count;
  iterator->in_history     = -1; // -1 is a marker used for first run
  ctx_memset (iterator->bitpack_command, 0, sizeof (iterator->bitpack_command));
}

static CtxEntry *_ctx_iterator_next (CtxIterator *iterator)
{
#if CTX_REFPACK
  int expand_refpack = iterator->flags & CTX_ITERATOR_EXPAND_REFPACK;

  int ret = iterator->pos;
  CtxEntry *entry = &iterator->renderstream->entries[ret];
  if (iterator->pos >= iterator->end_pos)
    return NULL;
  if (expand_refpack == 0)
  {
    if (iterator->in_history == 0)
      iterator->pos += (ctx_conts_for_entry (entry) + 1);
    iterator->in_history = 0;
    if (iterator->pos >= iterator->end_pos)
      return NULL;
    return &iterator->renderstream->entries[iterator->pos];
  }

  if (iterator->in_history > 0)
  {
    if (iterator->history_pos < iterator->history[1])
    {
      int ret = iterator->history[0] + iterator->history_pos;
      CtxEntry *hentry = &iterator->renderstream->entries[ret];
      iterator->history_pos += (ctx_conts_for_entry (hentry) + 1);

      if (iterator->history_pos >= iterator->history[1])
      {
        iterator->in_history = 0;
        iterator->pos += (ctx_conts_for_entry (entry) + 1);
      }
      return &iterator->renderstream->entries[ret];
    }
  }

  if (entry->code == CTX_REPEAT_HISTORY)
  {
    iterator->in_history = 1;
    iterator->history[0] = entry->data.u32[0];
    iterator->history[1] = entry->data.u32[1];
    CtxEntry *hentry = &iterator->renderstream->entries[iterator->history[0]];
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
  return &iterator->renderstream->entries[ret];
#else
  int ret = iterator->pos;
  CtxEntry *entry = &iterator->renderstream->entries[ret];
  if (ret >= iterator->end_pos)
    return NULL;
  {
    if (iterator->in_history == 0)
      iterator->pos += (ctx_conts_for_entry (entry) + 1);
    iterator->in_history = 0;
    if (iterator->pos >= iterator->end_pos)
      return NULL;
    return &iterator->renderstream->entries[iterator->pos];
  }
#endif
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

  iterator->bitpack_command[0].code = 
  iterator->bitpack_command[1].code =
  iterator->bitpack_command[2].code =
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
  iterator->bitpack_pos    = 0;
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
  //switch ((CtxCode)(ret->code))
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
      iterator->bitpack_command[0].code =
      iterator->bitpack_command[1].code =
      iterator->bitpack_command[2].code =
      iterator->bitpack_command[3].code = CTX_REL_LINE_TO;
      goto again;

    case CTX_REL_QUAD_TO_S16:
      ctx_iterator_expand_s16_args (iterator, ret);
      iterator->bitpack_command[0].code = CTX_REL_QUAD_TO;
      goto again;

    case CTX_REL_QUAD_TO_REL_QUAD_TO:
      ctx_iterator_expand_s8_args (iterator, ret);
      iterator->bitpack_command[0].code =
      iterator->bitpack_command[2].code = CTX_REL_QUAD_TO;
      goto again;

    case CTX_REL_LINE_TO_X2:
      ctx_iterator_expand_s16_args (iterator, ret);
      iterator->bitpack_command[0].code =
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
    case CTX_TEXTURE:
      iterator->bitpack_command[0] = ret[0];
      iterator->bitpack_command[1] = ret[1];
      iterator->bitpack_pos = 0;
      iterator->bitpack_length = 2;
      goto again;

    case CTX_ARC:
    case CTX_SET_COLOR:
    case CTX_RADIAL_GRADIENT:
    case CTX_CURVE_TO:
    case CTX_REL_CURVE_TO:
    case CTX_SET_TRANSFORM:
      iterator->bitpack_command[0] = ret[0];
      iterator->bitpack_command[1] = ret[1];
      iterator->bitpack_command[2] = ret[2];
      iterator->bitpack_pos = 0;
      iterator->bitpack_length = 3;
      goto again;

    case CTX_TEXT:
    case CTX_TEXT_STROKE:
    case CTX_SET_FONT:
      iterator->bitpack_length = 0;
      return ret;

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
  if (state->gstate_no + 1 >= CTX_MAX_STATES)
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

static void ctx_interpret_style (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_transforms (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_pos (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data);


static void ctx_renderstream_refpack (CtxRenderstream *renderstream);
static void
ctx_renderstream_resize (CtxRenderstream *renderstream, int desired_size)
{
#if CTX_RENDERSTREAM_STATIC
  if (renderstream->flags & CTX_RENDERSTREAM_EDGE_LIST)
  {
    static CtxEntry sbuf[CTX_MAX_EDGE_LIST_SIZE];
    renderstream->entries = &sbuf[0];
    renderstream->size = CTX_MAX_EDGE_LIST_SIZE;
  }
  else
  {
    static CtxEntry sbuf[CTX_MAX_JOURNAL_SIZE];
    renderstream->entries = &sbuf[0];
    renderstream->size = CTX_MAX_JOURNAL_SIZE;
    ctx_renderstream_refpack (renderstream);
  }
#else
  int new_size = desired_size;

  int min_size = CTX_MIN_JOURNAL_SIZE;
  int max_size = CTX_MAX_JOURNAL_SIZE;

  if (renderstream->flags & CTX_RENDERSTREAM_EDGE_LIST)
  {
    min_size = CTX_MIN_EDGE_LIST_SIZE;
    max_size = CTX_MAX_EDGE_LIST_SIZE;
  }
  else
  {
    ctx_renderstream_refpack (renderstream);
  }

  if (new_size < renderstream->size)
    return;
  if (renderstream->size == max_size)
    return;

  if (new_size < min_size)
    new_size = min_size;
  if (new_size < renderstream->count)
    new_size = renderstream->count + 4;

  if (new_size >= max_size)
    new_size = max_size;
  if (new_size != min_size)
  {
    //ctx_log ("growing renderstream %p to %d\n", renderstream, new_size);
  }
  if (renderstream->entries)
  {
    //printf ("grow %p to %d from %d\n", renderstream, new_size, renderstream->size);
    CtxEntry * ne =  (CtxEntry*)malloc (sizeof (CtxEntry) * new_size);
    memcpy (ne, renderstream->entries, renderstream->size * sizeof (CtxEntry));
    free (renderstream->entries);
    renderstream->entries = ne;
    //renderstream->entries = (CtxEntry*)malloc (renderstream->entries, sizeof (CtxEntry) * new_size);
  }
  else
  {
    //printf ("allocating for %p %d\n", renderstream, new_size);
    renderstream->entries = (CtxEntry*)malloc (sizeof (CtxEntry) * new_size);
  }
  renderstream->size = new_size;
  //printf ("renderstream %p is %d\n", renderstream, renderstream->size);
  #endif
}

static int
ctx_renderstream_add_single (CtxRenderstream *renderstream, CtxEntry *entry)
{
  int max_size = CTX_MAX_JOURNAL_SIZE;
  int ret = renderstream->count;

  if (renderstream->flags & CTX_RENDERSTREAM_EDGE_LIST)
  {
    max_size = CTX_MAX_EDGE_LIST_SIZE;
  }

  if (renderstream->flags & CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES)
  {
    return ret;
  }

  if (ret + 8 >= renderstream->size - 20)
  {
    ctx_renderstream_resize (renderstream, renderstream->size * 2);
  }

  if (renderstream->count >= max_size - 20)
  {
#if CTX_FULL_CB
    if (renderstream->full_cb)
    {
      if (ctx_conts_for_entry (entry)==0 &&
          entry->code != CTX_CONT)
      {
         renderstream->full_cb (renderstream, renderstream->full_cb_data);
         /* the full_cb is responsible for setting renderstream->count=0 */
      }
    }
    else
    {
       return 0;
    }
#else
       return 0;
#endif
  }

  renderstream->entries[renderstream->count] = *entry;
  ret = renderstream->count;
  renderstream->count++;
  return ret;
}

int
ctx_add_single (Ctx *ctx, void *entry)
{
  return ctx_renderstream_add_single (&ctx->renderstream, (CtxEntry*)entry);
}

int
ctx_renderstream_add_entry (CtxRenderstream *renderstream, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int ret = 0;
  for (int i = 0; i < length; i ++)
  {
    ret = ctx_renderstream_add_single (renderstream, &entry[i]);
  }
  return ret;
}

int ctx_set_renderstream (Ctx *ctx, void *data, int length)
{
  CtxEntry *entries = (CtxEntry*)data;
  if (length % sizeof (CtxEntry))
  {
    //ctx_log("err\n");
    return -1;
  }
  ctx->renderstream.count = 0;
  for (unsigned int i = 0; i < length / sizeof(CtxEntry); i++)
  {
    ctx_renderstream_add_single (&ctx->renderstream, &entries[i]);
  }

  return 0;
}

int ctx_get_renderstream_count (Ctx *ctx)
{
  return ctx->renderstream.count;
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
   * upon flush - and do renderstream resizing.
   */
  return ctx_renderstream_add_entry (&ctx->renderstream, (CtxEntry*)data);
}

static int ctx_renderstream_add_u32 (CtxRenderstream *renderstream, CtxCode code, uint32_t u32[2])
{
  CtxEntry entry = {code, {{0},}};
  entry.data.u32[0] = u32[0];
  entry.data.u32[1] = u32[1];
  return ctx_renderstream_add_single (renderstream, &entry);
}

int ctx_renderstream_add_data (CtxRenderstream *renderstream, const void *data, int length)
{
  CtxEntry entry = {CTX_DATA, {{0},}};
  entry.data.u32[0] = 0;
  entry.data.u32[1] = 0;
  int ret = ctx_renderstream_add_single (renderstream, &entry);

  if (!data) return -1;
  int length_in_blocks;
  if (length <= 0) length = strlen ((char*)data) + 1;

  length_in_blocks = length / sizeof (CtxEntry);
  length_in_blocks += (length % sizeof (CtxEntry))?1:0;

  if (renderstream->count + length_in_blocks + 4 > renderstream->size)
    ctx_renderstream_resize (renderstream, renderstream->count * 1.2 + length_in_blocks + 32);

  if (renderstream->count >= renderstream->size)
    return -1;
  renderstream->count += length_in_blocks;

  renderstream->entries[ret].data.u32[0] = length;
  renderstream->entries[ret].data.u32[1] = length_in_blocks;

  memcpy (&renderstream->entries[ret+1], data, length);
  {//int reverse = ctx_renderstream_add (renderstream, CTX_DATA_REV);
   CtxEntry entry = {CTX_DATA_REV, {{0},}};
   entry.data.u32[0] = length;
   entry.data.u32[1] = length_in_blocks;
   ctx_renderstream_add_single (renderstream, &entry);
   /* this reverse marker exist to enable more efficient
      front to back traversal
    */
  }

  return ret;
}

static CtxEntry
ctx_void (CtxCode code)
{
  CtxEntry command;
  command.code = code;
  command.data.u32[0] = 0;
  command.data.u32[1] = 0;
  return command;
}

static CtxEntry
ctx_f (CtxCode code, float x, float y)
{
  CtxEntry command = ctx_void (code);
  command.data.f[0] = x;
  command.data.f[1] = y;
  return command;
}

static CtxEntry
ctx_u32 (CtxCode code, uint32_t x, uint32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.u32[0] = x;
  command.data.u32[1] = y;
  return command;
}

#if 0
static CtxEntry
ctx_s32 (CtxCode code, int32_t x, int32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.s32[0] = x;
  command.data.s32[1] = y;
  return command;
}
#endif

static CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1)
{
  CtxEntry command = ctx_void (code);
  command.data.s16[0] = x0;
  command.data.s16[1] = y0;
  command.data.s16[2] = x1;
  command.data.s16[3] = y1;
  return command;
}

static CtxEntry
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

#define CTX_PROCESS_VOID(cmd) do {\
  CtxEntry command = ctx_void (cmd); \
  ctx_process (ctx, &command);}while(0) \

#define CTX_PROCESS_F(cmd, x, y) do {\
  CtxEntry command = ctx_f(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_F1(cmd, x) do {\
  CtxEntry command = ctx_f(cmd, x, 0);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U32(cmd, x, y) do {\
  CtxEntry command = ctx_u32(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U8(cmd, x) do {\
  CtxEntry command = ctx_u8(cmd, x,0,0,0,0,0,0,0);\
  ctx_process (ctx, &command);}while(0)


void
ctx_close_path (Ctx *ctx)
{
  CTX_PROCESS_VOID(CTX_CLOSE_PATH);
}


void ctx_texture (Ctx *ctx, int id, float x, float y)
{
  CtxEntry commands[2];
  if (id < 0) return;
  commands[0] = ctx_u32(CTX_TEXTURE, id, 0);
  commands[1] = ctx_f (CTX_CONT, x, y);
  ctx_process (ctx, commands);
}

void
ctx_image_path (Ctx *ctx, const char *path, float x, float y)
{
  int id = ctx_texture_load (ctx, -1, path);
  ctx_texture (ctx, id, x, y);
}

void ctx_fill (Ctx *ctx) {
  CTX_PROCESS_VOID(CTX_FILL);
}
void ctx_stroke (Ctx *ctx) {
  CTX_PROCESS_VOID(CTX_STROKE);
}

static void ctx_state_init (CtxState *state);

static void ctx_empty (Ctx *ctx)
{
#if CTX_RASTERIZER
  if (ctx->renderer == NULL)
#endif
  {
    ctx->renderstream.count = 0;
    ctx->renderstream.bitpack_pos = 0;
  }
}

void _ctx_set_store_clear (Ctx *ctx)
{
  ctx->transformation |= CTX_TRANSFORMATION_STORE_CLEAR;
}

void ctx_clear (Ctx *ctx) {
  CTX_PROCESS_VOID(CTX_CLEAR);
  if (ctx->transformation & CTX_TRANSFORMATION_STORE_CLEAR)
    return;
  ctx_empty (ctx);
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
  x = x * ctx_maxf(ctx_maxf(fabsf(ctx->state.gstate.transform.m[0][0]),
                          fabsf(ctx->state.gstate.transform.m[0][1])),
                  ctx_maxf(fabsf(ctx->state.gstate.transform.m[1][0]),
                          fabsf(ctx->state.gstate.transform.m[1][1])));
  CTX_PROCESS_F1(CTX_SET_LINE_WIDTH, x);
}

void
ctx_set_global_alpha (Ctx *ctx, float global_alpha)
{
  CTX_PROCESS_F1(CTX_SET_GLOBAL_ALPHA, global_alpha);
}

float
ctx_get_global_alpha (Ctx *ctx)
{
 return ctx->state.gstate.global_alpha_f;
}

void
ctx_set_font_size (Ctx *ctx, float x) {
  CTX_PROCESS_F1(CTX_SET_FONT_SIZE, x);
}

void
ctx_set_miter_limit (Ctx *ctx, float limit) {
  CTX_PROCESS_F1(CTX_SET_MITER_LIMIT, limit);
}

float ctx_get_font_size  (Ctx *ctx)
{
  return ctx->state.gstate.font_size;
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

#if CTX_FULL_CB
void ctx_set_full_cb (Ctx *ctx, CtxFullCb cb, void *data)
{
  ctx->renderstream.full_cb = cb;
  ctx->renderstream.full_cb_data = data;
}
#endif

void
_ctx_set_font (Ctx *ctx, const char *name)
{
  ctx->state.gstate.font = ctx_resolve_font (name);
}

void
ctx_set_font (Ctx *ctx, const char *name)
{
#if CTX_BACKEND_TEXT
  int namelen = strlen (name);
  CtxEntry commands[1 + 2 + namelen/8];
  ctx_memset (commands, 0, sizeof (commands));
  commands[0] = ctx_f(CTX_SET_FONT, 0, 0);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = namelen;
  commands[1].data.u32[1] = namelen/9+1;
  strcpy ((char*)&commands[2].data.u8[0], name);
  ctx_process (ctx, commands);
#else
  _ctx_set_font (ctx, name);
#endif
}

void
ctx_identity_matrix (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_IDENTITY);
}

void
ctx_set_transform (Ctx *ctx, float a, float b,  // hscale, hskew
                             float c, float d,  // vskew,  vscale
                             float e, float f)  // htran,  vtran
{
  CtxEntry command[3]={
     ctx_f (CTX_SET_TRANSFORM, a, b),
     ctx_f (CTX_CONT,          c, d),
     ctx_f (CTX_CONT,          e, f)};
  ctx_process (ctx, command);
}

void
ctx_get_transform  (Ctx *ctx, float *a, float *b,
                              float *c, float *d,
                              float *e, float *f)
{
  if (a) *a = ctx->state.gstate.transform.m[0][0];
  if (b) *b = ctx->state.gstate.transform.m[0][1];
  if (c) *c = ctx->state.gstate.transform.m[1][0];
  if (d) *d = ctx->state.gstate.transform.m[1][1];
  if (e) *e = ctx->state.gstate.transform.m[2][0];
  if (f) *f = ctx->state.gstate.transform.m[2][1];
}

void ctx_rotate (Ctx *ctx, float x){
  CTX_PROCESS_F1(CTX_ROTATE, x);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    ctx->renderstream.count--;
}

void ctx_scale (Ctx *ctx, float x, float y) {
  CTX_PROCESS_F (CTX_SCALE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    ctx->renderstream.count--;
}

void ctx_translate (Ctx *ctx, float x, float y) {
  CTX_PROCESS_F (CTX_TRANSLATE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    ctx->renderstream.count--;
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
#if 0
  CtxEntry command[3]={
     ctx_f (CTX_RECTANGLE, x0, y0),
     ctx_f (CTX_CONT,      w, h)};

  ctx_process (ctx, command);
#else
  ctx_move_to (ctx, x0, y0);
  ctx_rel_line_to (ctx, w, 0);
  ctx_rel_line_to (ctx, 0, h);
  ctx_rel_line_to (ctx, -w, 0);
  ctx_close_path (ctx);
#endif
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

void ctx_set_line_cap (Ctx *ctx, CtxLineCap cap) {
  CTX_PROCESS_U8 (CTX_SET_LINE_CAP, cap);
}
void ctx_set_fill_rule (Ctx *ctx, CtxFillRule fill_rule) {
  CTX_PROCESS_U8 (CTX_SET_FILL_RULE, fill_rule);
}
void ctx_set_line_join (Ctx *ctx, CtxLineJoin join) {
  CTX_PROCESS_U8 (CTX_SET_LINE_JOIN, join);
}
void ctx_set_compositing_mode (Ctx *ctx, CtxCompositingMode mode) {
  CTX_PROCESS_U8 (CTX_SET_COMPOSITING_MODE, mode);
}
void ctx_set_text_align (Ctx *ctx, CtxTextAlign text_align) {
  CTX_PROCESS_U8 (CTX_SET_TEXT_ALIGN, text_align);
}
void ctx_set_text_baseline (Ctx *ctx, CtxTextBaseline text_baseline) {
  CTX_PROCESS_U8 (CTX_SET_TEXT_BASELINE, text_baseline);
}
void ctx_set_text_direction (Ctx *ctx, CtxTextDirection text_direction) {
  CTX_PROCESS_U8 (CTX_SET_TEXT_DIRECTION, text_direction);
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

static int ctx_coords_equal (float x1, float y1, float x2, float y2, float tol)
{
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
}

static float
ctx_point_seg_dist_sq(float x, float y,
                      float vx, float vy, float wx, float wy)
{
  float l2 = ctx_pow2(vx-wx) + ctx_pow2(vy-wy);
  if (l2 < 0.0001)
    return ctx_pow2(x-vx) + ctx_pow2(y-vy);

  float t = ((x - vx) * (wx - vx) + (y - vy) * (wy - vy)) / l2;
  t = ctx_maxf(0, ctx_minf(1, t));
  float ix = vx + t * (wx - vx);
  float iy = vy + t * (wy - vy);

  return ctx_pow2(x-ix) + ctx_pow2(y-iy);
}

static void
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
#if 0
  // Handle degenerate cases.
  if (ctx_coords_equal (x0,y0, x1,y1, 0.5f) ||
      ctx_coords_equal (x1,y1, x2,y2, 0.5f) ||
      ctx_point_seg_dist_sq (x1,y1, x0,y0, x2,y2) < 0.5 ||
      radius < 0.5) {
        ctx_line_to (ctx, x1,y1);
        return;
      }
#endif

  // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
  dx0 = x0-x1;
  dy0 = y0-y1;
  dx1 = x2-x1;
  dy1 = y2-y1;
  ctx_normalize(&dx0,&dy0);
  ctx_normalize(&dx1,&dy1);
  a = acosf(dx0*dx1 + dy0*dy1);
  d = radius / tanf(a/2.0f);
#if 1
  if (d > 10000.0f) {
    ctx_line_to (ctx, x1, y1);
    return;
  }
#endif
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

#if 0
void
ctx_set_rgba_stroke_u8 (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_RGBA_STROKE, r, g, b, a, 0, 0, 0, 0);

  // XXX turn it into a no-op if the color matches color
  //     in state

  ctx_process (ctx, &command);
}
#endif

void
ctx_set_rgba_u8 (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_RGBA_U8, r, g, b, a, 0, 0, 0, 0);

  // XXX turn it into a no-op if the color matches color
  //     in state

  ctx_process (ctx, &command);
}

void
ctx_set_pixel_u8 (Ctx *ctx, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_PIXEL, r, g, b, a, 0, 0, 0, 0);
  command.data.u16[2]=x;
  command.data.u16[3]=y;
  ctx_process (ctx, &command);
}

void ctx_set_rgba (Ctx *ctx, float r, float g, float b, float a)
{
  CtxEntry command[3]={
     ctx_f (CTX_SET_COLOR, CTX_RGBA, 0.0f),
     ctx_f (CTX_CONT, r, g),
     ctx_f (CTX_CONT, b, a)};
  ctx_process (ctx, command);
}

void ctx_set_rgb (Ctx *ctx, float   r, float   g, float   b)
{
  ctx_set_rgba (ctx, r, g, b, 1.0f);
}

void ctx_set_gray (Ctx *ctx, float gray)
{
  CtxEntry command[3]={
     ctx_f (CTX_SET_COLOR, CTX_GRAY, 0.0f),
     ctx_f (CTX_CONT, gray, 1.0f),
     ctx_f (CTX_CONT, 0.0f, 0.0f)};
  ctx_process (ctx, command);
}

#if CTX_ENABLE_CMYK
void ctx_set_cmyk (Ctx *ctx, float c, float m, float y, float k)
{
  CtxEntry command[3]={
     ctx_f (CTX_SET_COLOR, CTX_CMYKA, 1.0f),
     ctx_f (CTX_CONT, c, m),
     ctx_f (CTX_CONT, y, k)};
  ctx_process (ctx, command);
}

void ctx_set_cmyka      (Ctx *ctx, float c, float m, float y, float k, float a)
{
  CtxEntry command[3]={
     ctx_f (CTX_SET_COLOR, CTX_CMYKA, a),
     ctx_f (CTX_CONT, c, m),
     ctx_f (CTX_CONT, y, k)};
  ctx_process (ctx, command);
}
#endif

#if 0
void ctx_set_rgba_stroke (Ctx *ctx, float r, float g, float b, float a)
{
  int ir = r * 255;
  int ig = g * 255;
  int ib = b * 255;
  int ia = a * 255;
  ir = CTX_CLAMP(ir, 0,255);
  ig = CTX_CLAMP(ig, 0,255);
  ib = CTX_CLAMP(ib, 0,255);
  ia = CTX_CLAMP(ia, 0,255);
  ctx_set_rgba_stroke_u8 (ctx, ir, ig, ib, ia);
}

void ctx_set_rgb_stroke (Ctx *ctx, float   r, float   g, float   b)
{
  ctx_set_rgba_stroke (ctx, r, g, b, 1.0f);
}

void ctx_set_gray_stroke (Ctx *ctx, float gray)
{
  ctx_set_rgba_stroke (ctx, gray, gray, gray, 1.0f);
}

#endif

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

void ctx_gradient_add_stop
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

void
ctx_exit (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_EXIT);
}

//////////////////

#include <stdio.h>
#include <unistd.h>

void
ctx_flush (Ctx *ctx)
{
#if 0
  //printf (" \e[?2222h");

  ctx_renderstream_refpack (&ctx->renderstream);
  for (int i = 0; i < ctx->renderstream.count - 1; i++)
  {
    CtxEntry *entry = &ctx->renderstream.entries[i];
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

  ctx->renderstream.count = 0;
  ctx_state_init (&ctx->state);
}

static void
ctx_matrix_inverse (CtxMatrix *m)
{
  CtxMatrix t = *m;
  float invdet, det = m->m[0][0] * m->m[1][1] -
                      m->m[1][0] * m->m[0][1];
  if (det > -0.0000001f && det < 0.0000001f)
  {
    m->m[0][0] = m->m[0][1] =
    m->m[1][0] = m->m[1][1] =
    m->m[2][0] = m->m[2][1] = 0.0;
    return;
  }
  invdet = 1.0f / det;
  m->m[0][0] = t.m[1][1] * invdet;
  m->m[1][0] = -t.m[1][0] * invdet;
  m->m[2][0] = (t.m[1][0] * t.m[2][1] - t.m[1][1] * t.m[2][0]) * invdet;
  m->m[0][1] = -t.m[0][1] * invdet;
  m->m[1][1] = t.m[0][0] * invdet;
  m->m[2][1] = (t.m[0][1] * t.m[2][0] - t.m[0][0] * t.m[2][1]) * invdet ;
}


static void
ctx_interpret_style (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
  {
    case CTX_SET_LINE_WIDTH:
      state->gstate.line_width = ctx_arg_float(0);
      break;
    case CTX_SET_LINE_CAP:
      state->gstate.line_cap = (CtxLineCap)ctx_arg_u8(0);
      break;
    case CTX_SET_FILL_RULE:
      state->gstate.fill_rule = (CtxFillRule)ctx_arg_u8(0);
      break;
    case CTX_SET_LINE_JOIN:
      state->gstate.line_join = (CtxLineJoin)ctx_arg_u8(0);
      break;
    case CTX_SET_COMPOSITING_MODE:
      state->gstate.compositing_mode = (CtxCompositingMode)ctx_arg_u8(0);
      break;
    case CTX_SET_TEXT_ALIGN:
      state->gstate.text_align = (CtxTextAlign)ctx_arg_u8(0);
      break;
    case CTX_SET_TEXT_BASELINE:
      state->gstate.text_baseline = (CtxTextBaseline)ctx_arg_u8(0);
      break;
    case CTX_SET_TEXT_DIRECTION:
      state->gstate.text_direction = (CtxTextDirection)ctx_arg_u8(0);
      break;
    case CTX_SET_GLOBAL_ALPHA:
      state->gstate.global_alpha_u8 = CTX_CLAMP(ctx_arg_float(0)*255.0,0, 255);
      state->gstate.global_alpha_f = ctx_arg_float(0);
      break;
    case CTX_SET_FONT_SIZE:
      state->gstate.font_size = ctx_arg_float(0);
      break;
    case CTX_SET_MITER_LIMIT:
      state->gstate.miter_limit = ctx_arg_float(0);
      break;
    case CTX_SET_COLOR:
      {
        CtxColor *color = &state->gstate.source.color;
        state->gstate.source.type = CTX_SOURCE_COLOR;
        switch ((int)ctx_arg_float(0)){
          case CTX_RGB:
            ctx_color_set_rgba (color, ctx_arg_float(2), ctx_arg_float(3), ctx_arg_float(4), 1.0f);
          break;
        case CTX_RGBA:
            ctx_color_set_rgba (color, ctx_arg_float(2), ctx_arg_float(3), ctx_arg_float(4), ctx_arg_float(5));
          break;
#if CTX_ENABLE_CMYK
        case CTX_CMYKA:
            ctx_color_set_cmyka (color, ctx_arg_float(2), ctx_arg_float(3), ctx_arg_float(4), ctx_arg_float(5), ctx_arg_float(1));
        break;
          case CTX_CMYK:
            ctx_color_set_cmyka (color, ctx_arg_float(2), ctx_arg_float(3), ctx_arg_float(4), ctx_arg_float(5), 1.0);
          break;
#endif
        case CTX_GRAYA:
          ctx_color_set_graya (color, ctx_arg_float(2), ctx_arg_float(3));
          break;
        case CTX_GRAY:
          ctx_color_set_graya (color, ctx_arg_float(2), 1.0f);
          break;
      }
      }
      break;

    case CTX_SET_RGBA_U8:
      //ctx_source_deinit (&state->gstate.source);
      state->gstate.source.type = CTX_SOURCE_COLOR;
      ctx_color_set_rgba_u8 (&state->gstate.source.color,
                             ctx_arg_u8(0),
                             ctx_arg_u8(1),
                             ctx_arg_u8(2),
                             ctx_arg_u8(3));
      //for (int i = 0; i < 4; i ++)
      //  state->gstate.source.color.rgba[i] = ctx_arg_u8(i);
      break;
#if 0
    case CTX_SET_RGBA_STROKE:
      //ctx_source_deinit (&state->gstate.source);
      state->gstate.source_stroke = state->gstate.source;
      state->gstate.source_stroke.type = CTX_SOURCE_COLOR;
      for (int i = 0; i < 4; i ++)
        state->gstate.source_stroke.color.rgba[i] = ctx_arg_u8(i);
      break;
#endif
    //case CTX_TEXTURE:
    //  state->gstate.source.type = CTX_SOURCE_
    //  break;
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
        state->gstate.source.transform = state->gstate.transform;
        ctx_matrix_inverse (&state->gstate.source.transform);
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

        state->gstate.source.radial_gradient.x0 = x0;
        state->gstate.source.radial_gradient.y0 = y0;
        state->gstate.source.radial_gradient.r0 = r0;
        state->gstate.source.radial_gradient.x1 = x1;
        state->gstate.source.radial_gradient.y1 = y1;
        state->gstate.source.radial_gradient.r1 = r1;
        state->gstate.source.type = CTX_SOURCE_RADIAL_GRADIENT;
        //ctx_matrix_identity (&state->gstate.source.transform);
        state->gstate.source.transform = state->gstate.transform;
        ctx_matrix_inverse (&state->gstate.source.transform);
      }
      break;
  }
}

static void
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
    case CTX_SET_TRANSFORM:
      ctx_matrix_set (&state->gstate.transform,
                        ctx_arg_float(0), ctx_arg_float(1),
                        ctx_arg_float(2), ctx_arg_float(3),
                        ctx_arg_float(4), ctx_arg_float(5));
      break;
  }
}

static void
ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y)
{
  float x_in = *x;
  float y_in = *y;
  *x = ((x_in * m->m[0][0]) + (y_in * m->m[1][0]) + m->m[2][0]);
  *y = ((y_in * m->m[1][1]) + (x_in * m->m[0][1]) + m->m[2][1]);
}

void
ctx_user_to_device (CtxState *state, float *x, float *y)
{
  ctx_matrix_apply_transform (&state->gstate.transform, x, y);
}

void
ctx_user_to_device_distance (CtxState *state, float *x, float *y)
{
  const CtxMatrix *m = &state->gstate.transform;
  ctx_matrix_apply_transform (m, x, y);
  *x -= m->m[2][0];
  *y -= m->m[2][1];
}

#if CTX_BITPACK_PACKER

#if CTX_BITPACK
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
pack_s8_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      entry[0].data.s8[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV;
}

static void
pack_s16_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      entry[0].data.s16[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV;
}
#endif

#if CTX_BITPACK
static void
ctx_renderstream_remove_tiny_curves (CtxRenderstream *renderstream, int start_pos)
{
  CtxIterator iterator;
  if ((renderstream->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    return;

  ctx_iterator_init (&iterator, renderstream, start_pos, CTX_ITERATOR_FLAT);
  iterator.end_pos = renderstream->count - 5;
  CtxEntry *command = NULL;
  while ((command = ctx_iterator_next(&iterator)))
  {
    /* things smaller than this have probably been scaled down
       beyond recognition, bailing for both better packing and less rasterization work
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
#endif

static void
ctx_renderstream_bitpack (CtxRenderstream *renderstream, int start_pos)
{
#if CTX_BITPACK
  int i = 0;

  if ((renderstream->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    return;
  ctx_renderstream_remove_tiny_curves (renderstream, renderstream->bitpack_pos);

  i = renderstream->bitpack_pos;

  if (start_pos > i)
    i = start_pos;

  while (i < renderstream->count - 4) /* the -4 is to avoid looking past
                                    initialized data we're not ready
                                    to bitpack yet*/
  {
    CtxEntry *entry = &renderstream->entries[i];

    if (entry[0].code == CTX_SET_RGBA_U8 &&
        entry[1].code == CTX_MOVE_TO &&
        entry[2].code == CTX_REL_LINE_TO &&
        entry[3].code == CTX_REL_LINE_TO &&
        entry[4].code == CTX_REL_LINE_TO &&
        entry[5].code == CTX_REL_LINE_TO &&
        entry[6].code == CTX_FILL &&
        fabsf (entry[2].data.f[0] - 1.0f) < 0.02f &&
        fabsf (entry[3].data.f[1] - 1.0f) < 0.02f)
    {
        entry[0].code = CTX_SET_PIXEL;
        entry[0].data.u16[2] = entry[1].data.f[0];
        entry[0].data.u16[3] = entry[1].data.f[1];
        entry[1].code = CTX_NOP;
        entry[2].code = CTX_NOP;
        entry[3].code = CTX_NOP;
        entry[4].code = CTX_NOP;
        entry[5].code = CTX_NOP;
        entry[6].code = CTX_NOP;
    }
#if 1
    else if (entry[0].code == CTX_REL_LINE_TO)
    {
      if (entry[1].code == CTX_REL_LINE_TO &&
          entry[2].code == CTX_REL_LINE_TO &&
          entry[3].code == CTX_REL_LINE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          pack_s8_args (entry, 4);
          entry[0].code = CTX_REL_LINE_TO_X4;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
        }
      }
      else if (entry[1].code == CTX_REL_CURVE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          pack_s8_args (entry, 4);
          entry[0].code = CTX_REL_LINE_TO_REL_CURVE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
        }
      }
      else if (entry[1].code == CTX_REL_LINE_TO &&
               entry[2].code == CTX_REL_LINE_TO &&
               entry[3].code == CTX_REL_LINE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          pack_s8_args (entry, 4);
          entry[0].code = CTX_REL_LINE_TO_X4;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
        }
      }
      else if (entry[1].code == CTX_REL_MOVE_TO)
      {
        float max_dev = find_max_dev (entry, 2);
        if (max_dev < 31000 / CTX_SUBDIV)
        {
          pack_s16_args (entry, 2);
          entry[0].code = CTX_REL_LINE_TO_REL_MOVE_TO;
          entry[1].code = CTX_NOP;
        }
      }
      else if (entry[1].code == CTX_REL_LINE_TO)
      {
        float max_dev = find_max_dev (entry, 2);
        if (max_dev < 31000 / CTX_SUBDIV)
        {
          pack_s16_args (entry, 2);
          entry[0].code = CTX_REL_LINE_TO_X2;
          entry[1].code = CTX_NOP;
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
          pack_s8_args (entry, 4);
          entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
        }
      }
      else if (entry[3].code == CTX_REL_MOVE_TO)
      {
        float max_dev = find_max_dev (entry, 4);
        if (max_dev < 114 / CTX_SUBDIV)
        {
          pack_s8_args (entry, 4);
          entry[0].code = CTX_REL_CURVE_TO_REL_MOVE_TO;
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
          pack_s8_args (entry, 3);
          ctx_arg_s8(6) =
          ctx_arg_s8(7) = 0;
          entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
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
          pack_s8_args (entry, 4);
          entry[0].code = CTX_REL_QUAD_TO_REL_QUAD_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
        }
      }
      else
      {
        float max_dev = find_max_dev (entry, 2);
        if (max_dev < 3100 / CTX_SUBDIV)
        {
          pack_s16_args (entry, 2);
          entry[0].code = CTX_REL_QUAD_TO_S16;
          entry[1].code = CTX_NOP;
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
#if 1
    else if (entry[0].code == CTX_MOVE_TO &&
             entry[1].code == CTX_MOVE_TO &&
             entry[2].code == CTX_MOVE_TO)
    {
        entry[0] = entry[2];
        entry[0].code = CTX_MOVE_TO;
        entry[1].code = CTX_NOP;
        entry[2].code = CTX_NOP;
    }
#endif
#if 1
    else if ((entry[0].code == CTX_MOVE_TO &&
              entry[1].code == CTX_MOVE_TO) ||
            (entry[0].code == CTX_REL_MOVE_TO &&
             entry[1].code == CTX_MOVE_TO))
    {
        entry[0] = entry[1];
        entry[0].code = CTX_MOVE_TO;
        entry[1].code = CTX_NOP;
    }
#endif

    i += (ctx_conts_for_entry (entry) + 1);
  }

  int source = renderstream->bitpack_pos;
  int target = renderstream->bitpack_pos;
  int removed = 0;
  /* remove nops that have been inserted as part of shortenings
   */
  while (source < renderstream->count)
  {
    CtxEntry *sentry = &renderstream->entries[source];
    CtxEntry *tentry = &renderstream->entries[target];

    while (sentry->code == CTX_NOP && source < renderstream->count)
    {
      source++;
      sentry = &renderstream->entries[source];
      removed++;
    }
    if (sentry != tentry)
      *tentry = *sentry;

    source ++;
    target ++;
  }
  renderstream->count -= removed;

  renderstream->bitpack_pos = renderstream->count;
#endif
}

#endif

#if CTX_REFPACK
static int
ctx_entries_equal (const CtxEntry *a, const CtxEntry *b)
{
  if (!a || !b) return 0;
  if (a->code == CTX_REPEAT_HISTORY)
    return 0;
  return (a->code == b->code &&
          a->data.u32[0] == b->data.u32[0] &&
          a->data.u32[1] == b->data.u32[1]);
}
#endif

#if CTX_BITPACK_PACKER|CTX_REFPACK
static int
ctx_last_history (CtxRenderstream *renderstream)
{
  int last_history = 0;
  int i = 0;
  while (i < renderstream->count)
  {
    CtxEntry *entry = &renderstream->entries[i];
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
ctx_renderstream_remove (CtxRenderstream *renderstream, int pos, int count)
{
  if (count <= 0)
    return;
  for (int i = pos; i < renderstream->count - count; i++)
  {
    renderstream->entries[i] = renderstream->entries[i+count];
  }
  for (int i = renderstream->count - count; i < renderstream->count; i++)
  {
    renderstream->entries[i].code = CTX_CONT;
    renderstream->entries[i].data.f[0] = 0;
    renderstream->entries[i].data.f[1] = 0;
  }
  renderstream->count = renderstream->count - count;
}



/* find first match of input in dicitonary equal to or larger than
 * minimum_length
 */
static int
ctx_renderstream_dedup_search (CtxRenderstream *dictionary,
                          int d_start, int d_end,
                          CtxRenderstream *input,
                          int i_start, int i_end,
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
    if (dictionary->entries[end+result + 1].code == CTX_CONT) 
    {
      return 0;
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
ctx_renderstream_refpack (CtxRenderstream *renderstream)
{
#if CTX_BITPACK_PACKER|CTX_REFPACK
  int last_history;
  last_history = ctx_last_history (renderstream);
#else
  if (renderstream){};
#endif
#if CTX_BITPACK_PACKER
    ctx_renderstream_bitpack (renderstream, last_history);
#endif
#if CTX_REFPACK
  int length_threshold = 4;

  if ((renderstream->flags & CTX_TRANSFORMATION_REFPACK) == 0)
    return;

  if (!last_history)
  {
    last_history = 8;
    if (last_history > renderstream->count)
      last_history = renderstream->count / 2;
  }

  int completed = last_history;

  while (completed < renderstream->count)
  {
    int default_search_window = 512;
    int search_window = default_search_window;

    int match_start;
    int match_length=0;
    int match_input_pos;

    if ((renderstream->count - completed) < search_window)
      search_window = renderstream->count - completed;

    while (ctx_renderstream_dedup_search (
           renderstream, 0, completed-1,
           renderstream, completed+1, completed+search_window - 2,
           &match_start, &match_length, &match_input_pos,
           length_threshold))
    {
      CtxEntry *ientry = &renderstream->entries[match_input_pos];
      ientry->code = CTX_REPEAT_HISTORY;
      ientry->data.u32[0]= match_start;
      ientry->data.u32[1]= match_length;
      ctx_renderstream_remove (renderstream, match_input_pos+1, match_length-1);
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
          state->has_moved = 1;
        }

        if ((((Ctx*)(data))->transformation & CTX_TRANSFORMATION_SCREEN_SPACE))
        {
          ctx_user_to_device (state, &x, &y);
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
        ctx_user_to_device (state, &x, &y);
        ctx_arg_float(0) = x;
        ctx_arg_float(1) = y;
        y = 0;
        ctx_user_to_device_distance (state, &r, &y);
        ctx_arg_float(2) = r;
      }

      break;

    case CTX_LINEAR_GRADIENT:
      {
        float x = ctx_arg_float (0);
        float y = ctx_arg_float (1);
        ctx_user_to_device (state, &x, &y);
        ctx_arg_float(0) = x;
        ctx_arg_float(1) = y;

        x = ctx_arg_float (2);
        y = ctx_arg_float (3);
        ctx_user_to_device (state, &x, &y);
        ctx_arg_float(2) = x;
        ctx_arg_float(3) = y;
      }
      break;

    case CTX_RADIAL_GRADIENT:
      {
        float x = ctx_arg_float (0);
        float y = ctx_arg_float (1);
        float r = ctx_arg_float (2);
        ctx_user_to_device (state, &x, &y);
        ctx_arg_float(0) = x;
        ctx_arg_float(1) = y;
        y = 0;
        ctx_user_to_device_distance (state, &r, &y);
        ctx_arg_float(2) = r;

        x = ctx_arg_float (3);
        y = ctx_arg_float (4);
        r = ctx_arg_float (5);
        ctx_user_to_device (state, &x, &y);
        ctx_arg_float(3) = x;
        ctx_arg_float(4) = y;
        y = 0;
        ctx_user_to_device_distance (state, &r, &y);
        ctx_arg_float(5) = r;
      }
      break;

    case CTX_CURVE_TO:
      if (!state->has_moved) // bit ifft for curveto
      {
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
          ctx_user_to_device (state, &x, &y);
          entry[c].data.f[0] = x;
          entry[c].data.f[1] = y;
        }
      }
      break;

    case CTX_QUAD_TO:
      if (!state->has_moved) // bit ifft for curveto
      {
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
          ctx_user_to_device (state, &x, &y);
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
          ctx_user_to_device (state, &x, &y);
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
          state->has_moved = 1;
        }
     }
     break;
    case CTX_CURVE_TO:
      state->x = ctx_arg_float (4);
      state->y = ctx_arg_float (5);

      if (!state->has_moved)
      {
        state->has_moved = 1;
      }
      break;
    case CTX_QUAD_TO:
      state->x = ctx_arg_float (2);
      state->y = ctx_arg_float (3);

      if (!state->has_moved)
      {
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
  ctx_memset (state, 0, sizeof (CtxState));
  state->gstate.global_alpha_u8 = 255;
  state->gstate.global_alpha_f = 1.0;
  state->gstate.font_size    = 12;
  state->gstate.line_spacing = 1.0;
  state->gstate.line_width   = 2.0;
  state->min_x               = 8192;
  state->min_y               = 8192;
  state->max_x               = -8192;
  state->max_y               = -8192;
  ctx_matrix_identity (&state->gstate.transform);
}

void _ctx_set_transformation (Ctx *ctx, int transformation)
{
  ctx->transformation = transformation;
}

static void
ctx_init (Ctx *ctx)
{
  ctx_state_init (&ctx->state);
#if 1
  ctx->transformation |= (CtxTransformation)CTX_TRANSFORMATION_SCREEN_SPACE;
  ctx->transformation |= (CtxTransformation)CTX_TRANSFORMATION_RELATIVE;
#if CTX_BITPACK
  ctx->renderstream.flags |= CTX_TRANSFORMATION_BITPACK;
#endif
#if CTX_REFPACK
  ctx->renderstream.flags |= CTX_TRANSFORMATION_REFPACK;
#endif
#endif
}

static void ctx_setup ();

#if CTX_RENDERSTREAM_STATIC
static Ctx ctx_state;
#endif

Ctx *
ctx_new (void)
{
  ctx_setup ();
#if CTX_RENDERSTREAM_STATIC
  Ctx *ctx = &ctx_state;
#else
  Ctx *ctx = (Ctx*)malloc (sizeof (Ctx));
#endif
  ctx_memset (ctx, 0, sizeof(Ctx));
  ctx_init (ctx);

  return ctx;
}

void
ctx_renderstream_deinit (CtxRenderstream *renderstream)
{
#if CTX_RENDERSTREAM_STATIC==0
  if (renderstream->entries && !(renderstream->flags & CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES))
    free (renderstream->entries);
#endif
  renderstream->entries = NULL;
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
  ctx_renderstream_deinit (&ctx->renderstream);
}

void ctx_free (Ctx *ctx)
{
  if (!ctx)
    return;
  ctx_deinit (ctx);
#if CTX_RENDERSTREAM_STATIC==0
  free (ctx);
#endif
}

Ctx *ctx_new_for_renderstream (void *data, size_t length)
{
  Ctx *ctx = ctx_new ();
  ctx->renderstream.flags   |= CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES;
  ctx->renderstream.entries  = (CtxEntry*)data;
  ctx->renderstream.count    = length / sizeof (CtxEntry);
  return ctx;
}

#if CTX_RASTERIZER
////////////////////////////////////


static CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format);

CtxBuffer *ctx_buffer_new (void)
{
  CtxBuffer *buffer = (CtxBuffer*)malloc (sizeof (CtxBuffer));
  ctx_memset (buffer, 0, sizeof (CtxBuffer));
  return buffer;
}

void ctx_buffer_set_data (CtxBuffer *buffer,
                          void *data, int width, int height,
                          int stride,
                          CtxPixelFormat pixel_format,
                       void (*freefunc)(void *pixels, void *user_data),
                       void *user_data)
{
  if (buffer->free_func)
    buffer->free_func (buffer->data, buffer->user_data);
  buffer->data = data;
  buffer->width = width;
  buffer->height = height;
  buffer->stride = stride;
  buffer->format = ctx_pixel_format_info (pixel_format);
  buffer->free_func = freefunc;
  buffer->user_data = user_data;
}

CtxBuffer *ctx_buffer_new_for_data (void *data, int width, int height,
                                    int stride,
                                    CtxPixelFormat pixel_format,
                       void (*freefunc)(void *pixels, void *user_data),
                       void *user_data)
{
  CtxBuffer *buffer = ctx_buffer_new ();
  ctx_buffer_set_data (buffer, data, width, height, stride, pixel_format,
                       freefunc, user_data);
  return buffer;
}

void ctx_buffer_deinit (CtxBuffer *buffer)
{
  if (buffer->free_func)
    buffer->free_func (buffer->data, buffer->user_data);
  buffer->data = NULL;
  buffer->free_func = NULL;
  buffer->user_data  = NULL;
}

void ctx_buffer_free (CtxBuffer *buffer)
{
  ctx_buffer_deinit (buffer);
  free (buffer);
}

/* load the png into the buffer */
static int ctx_buffer_load_png (CtxBuffer *buffer,
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
  buffer->free_func = (void*)free;
  buffer->user_data = NULL;
  return 0;
#else
  if (path){};
  return -1;
#endif
}

void ctx_texture_release (Ctx *ctx, int id)
{
  if (id < 0 || id >= CTX_MAX_TEXTURES)
     return;
  ctx_buffer_deinit (&ctx->texture[id]);
}

static int ctx_allocate_texture_id (Ctx *ctx, int id)
{
  if (id < 0 || id >= CTX_MAX_TEXTURES)
  {
    for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
      if (ctx->texture[i].data == NULL)
        return i;
    return -1; // eeek
  }
  return id;
}

/* load the png into the buffer */
static int ctx_buffer_load_memory (CtxBuffer *buffer,
                                   const char *data,
                                   int length)
{
  ctx_buffer_deinit (buffer);

#ifdef UPNG_H
  upng_t *upng = upng_new_from_bytes (data, length);
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
  buffer->free_func = (void*)free;
  buffer->user_data = NULL;
  return 0;
#else
  if (data && length){};
  return -1;
#endif
}

int ctx_texture_load_memory (Ctx *ctx, int id, const char *data, int length)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    return id;
  if (ctx_buffer_load_memory (&ctx->texture[id], data, length))
  {
    return -1;
  }
  return id;
}

int ctx_texture_load (Ctx *ctx, int id, const char *path)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    return id;
  if (ctx_buffer_load_png (&ctx->texture[id], path))
  {
    return -1;
  }
  return id;
}

int ctx_texture_init (Ctx *ctx, int id, int width, int height, int bpp,
                       uint8_t *pixels,
                       void (*freefunc)(void *pixels, void *user_data),
                       void *user_data)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    return id;
  ctx_buffer_deinit (&ctx->texture[id]);
  ctx_buffer_set_data (&ctx->texture[id], 
     pixels, width, height, width * (bpp/8), bpp==32?CTX_FORMAT_RGBA8:CTX_FORMAT_RGB8, freefunc, user_data);
  return id;
}

void
ctx_gradient_clear_stops (Ctx *ctx)
{
   CTX_PROCESS_VOID (CTX_GRADIENT_CLEAR);
}



#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_reset (void);
#endif

static void
ctx_renderer_gradient_clear_stops(CtxRenderer *renderer)
{
  renderer->state->gradient.n_stops = 0;
}

static void
ctx_renderer_gradient_add_stop (CtxRenderer *renderer, float pos, uint8_t *rgba)
{
  CtxGradient *gradient = &renderer->state->gradient;

  CtxGradientStop *stop = &gradient->stops[gradient->n_stops];
  stop->pos = pos;
  stop->rgba[0] = rgba[0];
  stop->rgba[1] = rgba[1];
  stop->rgba[2] = rgba[2];
  stop->rgba[3] = rgba[3];
  if (gradient->n_stops < 15)//we'll keep overwriting the last when out of stops
    gradient->n_stops++;

}

static int ctx_renderer_add_point (CtxRenderer *renderer, int x1, int y1)
{
  int16_t args[4];

  if (y1 < renderer->scan_min)
    renderer->scan_min = y1;
  if (y1 > renderer->scan_max)
    renderer->scan_max = y1;

  if (x1 < renderer->col_min)
    renderer->col_min = x1;
  if (x1 > renderer->col_max)
    renderer->col_max = x1;

  args[0]=0;
  args[1]=0;
  args[2]=x1;
  args[3]=y1;

  return ctx_renderstream_add_u32 (&renderer->edge_list, CTX_EDGE, (uint32_t*)args);
}

float ctx_shape_cache_rate = 0.0;
#if CTX_SHAPE_CACHE

static uint32_t ctx_shape_time = 0;

/* to get better cache usage-  */

struct _CtxShapeEntry {
  uint32_t hash;
  uint16_t width;
  uint16_t height;
  uint32_t refs;
  uint32_t age;   // time last used
  uint32_t uses;  // instrumented for longer keep-alive
  uint8_t  data[];
};

typedef struct _CtxShapeEntry CtxShapeEntry;


#define CTX_SHAPE_CACHE_PRIME1   13
#define CTX_SHAPE_CACHE_PRIME2   112
#define CTX_SHAPE_CACHE_PRIME3   79
#define CTX_SHAPE_CACHE_PRIME4   79

// this needs a max-size
// and a more agressive freeing when
// size is about to be exceeded

struct _CtxShapeCache {
  CtxShapeEntry *entries[CTX_SHAPE_CACHE_ENTRIES];
  long size;
};

typedef struct _CtxShapeCache CtxShapeCache;

static CtxShapeCache ctx_cache = {{NULL,}, 0};

static long hits = 0;
static long misses = 0;


/* this returns the buffer to use for rendering, it always
   succeeds..
 */
static CtxShapeEntry *ctx_shape_entry_find (uint32_t hash, int width, int height, uint32_t time) {
  int entry_no = ((hash >> 10) ^ (hash & 1023)) % CTX_SHAPE_CACHE_ENTRIES;
  int i;

    {
      static int i = 0;
      i++;
      if (i>512)
      {
        ctx_shape_cache_rate = hits * 100.0  / (hits+misses);
        i = 0; hits = 0; misses = 0;
      }
    }

  i = entry_no;
  if (ctx_cache.entries[i])
  {
    if (ctx_cache.entries[i]->hash == hash &&
        ctx_cache.entries[i]->width == width &&
        ctx_cache.entries[i]->height == height)
      {
        ctx_cache.entries[i]->refs++;
        ctx_cache.entries[i]->age = time;
        if (ctx_cache.entries[i]->uses < 1<<30)
          ctx_cache.entries[i]->uses++;
        hits ++;
        return ctx_cache.entries[i];
      }
#if 0
    else if (i < CTX_SHAPE_CACHE_ENTRIES-2)
    {
      if (ctx_cache.entries[i+1])
      {
       if (ctx_cache.entries[i+1]->hash == hash &&
           ctx_cache.entries[i+1]->width == width &&
           ctx_cache.entries[i+1]->height == height)
       {
         ctx_cache.entries[i+1]->refs++;
         ctx_cache.entries[i+1]->age = time;
         if (ctx_cache.entries[i+1]->uses < 1<<30)
           ctx_cache.entries[i+1]->uses++;
         hits ++;
         return ctx_cache.entries[i+1];
       }
       else if (i < CTX_SHAPE_CACHE_ENTRIES-3)
       {
      if (ctx_cache.entries[i+2])
      {
       if (ctx_cache.entries[i+2]->hash == hash &&
           ctx_cache.entries[i+2]->width == width &&
           ctx_cache.entries[i+2]->height == height)
       {
         ctx_cache.entries[i+2]->refs++;
         ctx_cache.entries[i+2]->age = time;
         if (ctx_cache.entries[i+2]->uses < 1<<30)
           ctx_cache.entries[i+2]->uses++;
         hits ++;
         return ctx_cache.entries[i+2];
       }
      }
      else
      {
        i+=2;
      }


       }
      }
      else
      {
        i++;
      }
    }
#endif
  }
  misses ++;

// XXX : this 1 one is needed  to silence:
// ==90718== Invalid write of size 1
// ==90718==    at 0x1189EF: ctx_renderer_generate_coverage (ctx.h:4786)
// ==90718==    by 0x118E57: ctx_renderer_rasterize_edges (ctx.h:4907)
//
  int size = sizeof(CtxShapeEntry) + width * height + 1;
  CtxShapeEntry *new_entry = (CtxShapeEntry*)malloc (size);
  new_entry->refs = 1;
  if (ctx_cache.entries[i])
  {
  
    CtxShapeEntry *entry = ctx_cache.entries[i];
    while (entry->refs){};
    ctx_cache.entries[i] = new_entry;
    ctx_cache.size -= entry->width * entry->height;
    ctx_cache.size -= sizeof (CtxShapeEntry);
    free (entry);
  }
  else
  {
    ctx_cache.entries[i] = new_entry;
  }

  ctx_cache.size += size;

  ctx_cache.entries[i]->age = time;
  ctx_cache.entries[i]->hash=hash;
  ctx_cache.entries[i]->width=width;
  ctx_cache.entries[i]->height=height;
  ctx_cache.entries[i]->uses = 0;
  return ctx_cache.entries[i];
}

static void ctx_shape_entry_release (CtxShapeEntry *entry)
{
  entry->refs--;
}
#endif

static uint32_t ctx_renderer_poly_to_edges (CtxRenderer *renderer)
{

  int16_t x = 0;
  int16_t y = 0;

#if CTX_SHAPE_CACHE
  CtxEntry *entry = &renderer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = renderer->edge_list.count;
  hash = (ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += (oy % CTX_RASTERIZER_AA);
#endif

  for (int i = 0; i < renderer->edge_list.count; i++)
  {
    CtxEntry *entry = &renderer->edge_list.entries[i];
    if (entry->code == CTX_NEW_EDGE)
    {
      entry->code = CTX_EDGE;
#if CTX_SHAPE_CACHE
      hash *= CTX_SHAPE_CACHE_PRIME2;
#endif
    }
    else
    {
      entry->data.s16[0] = x;
      entry->data.s16[1] = y;
    }
    x = entry->data.s16[2];
    y = entry->data.s16[3];

#if CTX_SHAPE_CACHE
    int dx = x-ox;
    int dy = y-oy;
    ox = x;
    oy = y;

    hash *= CTX_SHAPE_CACHE_PRIME3;
    hash += dx;
    hash *= CTX_SHAPE_CACHE_PRIME4;
    hash += dy;
#endif

    if (entry->data.s16[3] < entry->data.s16[1])
    {
       *entry = ctx_s16(CTX_EDGE_FLIPPED,
                        entry->data.s16[2], entry->data.s16[3],
                        entry->data.s16[0], entry->data.s16[1]);
    }
  }
#if CTX_SHAPE_CACHE
  return hash;
#else
  return 0;
#endif
}

static void ctx_renderer_line_to (CtxRenderer *renderer, float x, float y);

static void ctx_renderer_finish_shape (CtxRenderer *renderer)
{
  if (renderer->has_shape && renderer->has_prev)
  {
    ctx_renderer_line_to (renderer, renderer->first_x, renderer->first_y);
    renderer->has_prev = 0;
  }
}

static void ctx_renderer_move_to (CtxRenderer *renderer, float x, float y)
{
  renderer->x        = x;
  renderer->y        = y;
  renderer->first_x  = x;
  renderer->first_y  = y;
  renderer->has_prev = -1;
}

static void ctx_renderer_line_to (CtxRenderer *renderer, float x, float y)
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
  ctx_renderer_add_point (renderer, tx * CTX_SUBDIV, ty * CTX_RASTERIZER_AA);
  if (renderer->has_prev<=0)
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
  renderer->y         = y;
  renderer->x         = x;
}

static float
ctx_lerpf (float v0, float v1, float dx)
{
  return v0 + (v1-v0) * dx;
}

static float
ctx_bezier_sample_1d (float x0, float x1, float x2, float x3, float dt)
{
  float ab   = ctx_lerpf (x0, x1, dt);
  float bc   = ctx_lerpf (x1, x2, dt);
  float cd   = ctx_lerpf (x2, x3, dt);
  float abbc = ctx_lerpf (ab, bc, dt);
  float bccd = ctx_lerpf (bc, cd, dt);
  return ctx_lerpf (abbc, bccd, dt);
}

static void
ctx_bezier_sample (float x0, float y0,
                   float x1, float y1,
                   float x2, float y2,
                   float x3, float y3,
                   float dt, float *x, float *y)
{
  *x = ctx_bezier_sample_1d (x0, x1, x2, x3, dt);
  *y = ctx_bezier_sample_1d (y0, y1, y2, y3, dt);
}

static void
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

static void
ctx_renderer_curve_to (CtxRenderer *renderer,
                       float x0, float y0,
                       float x1, float y1,
                       float x2, float y2)
{
  float tolerance =
     ctx_pow2(renderer->state->gstate.transform.m[0][0]) +
     ctx_pow2(renderer->state->gstate.transform.m[1][1]);

  float ox = renderer->x;
  float oy = renderer->y;

  ox = renderer->state->x;
  oy = renderer->state->y;

  tolerance = 1.0f/tolerance;
#if 0 // skipping this to preserve hashes
  float maxx = ctx_maxf(x1,x2);
  maxx = ctx_maxf(maxx, ox);
  maxx = ctx_maxf(maxx, x0);
  float maxy = ctx_maxf(y1,y2);
  maxy = ctx_maxf(maxy, oy);
  maxy = ctx_maxf(maxy, y0);
  float minx = ctx_minf(x1,x2);
  minx = ctx_minf(minx, ox);
  minx = ctx_minf(minx, x0);
  float miny = ctx_minf(y1,y2);
  miny = ctx_minf(miny, oy);
  miny = ctx_minf(miny, y0);

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
#endif
  {
    ctx_renderer_bezier_divide (renderer,
                                ox, oy, x0, y0,
                                x1, y1, x2, y2,
                                ox, oy, x2, y2,
                                0.0f, 1.0f, 0.0f, tolerance);
  }
  ctx_renderer_line_to (renderer, x2, y2);
}

static void
ctx_renderer_rel_move_to (CtxRenderer *renderer, float x, float y)
{
  if (x == 0.f && y == 0.f)
    return;
  x += renderer->x;
  y += renderer->y;
  ctx_renderer_move_to (renderer, x, y);
}

static void
ctx_renderer_rel_line_to (CtxRenderer *renderer, float x, float y)
{
  if (x== 0.f && y==0.f)
    return;
  x += renderer->x;
  y += renderer->y;
  ctx_renderer_line_to (renderer, x, y);
}

static void
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

static void ctx_renderer_sort_edges (CtxRenderer *renderer)
{
  qsort (&renderer->edge_list.entries[0], renderer->edge_list.count,
         sizeof (CtxEntry), ctx_compare_edges);
}

static void ctx_renderer_discard_edges (CtxRenderer *renderer)
{
  for (int i = 0; i < renderer->active_edges; i++)
  {
    if (renderer->edge_list.entries[renderer->edges[i].index].data.s16[3] < renderer->scanline
)
    {
       if (renderer->lingering_edges + 1 < CTX_MAX_LINGERING_EDGES)
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

static void ctx_renderer_increment_edges (CtxRenderer *renderer, int count)
{
  for (int i = 0; i < renderer->lingering_edges; i++)
  {
     renderer->lingering[i].x += renderer->lingering[i].dx * count;
  }

  for (int i = 0; i < renderer->active_edges; i++)
  {
     renderer->edges[i].x += renderer->edges[i].dx * count;
  }
  for (int i = 0; i < renderer->pending_edges; i++)
  {
     renderer->edges[CTX_MAX_EDGES-1-i].x += renderer->edges[CTX_MAX_EDGES-1-i].dx * count;
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
static void ctx_renderer_feed_edges (CtxRenderer *renderer)
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
             and keep a different count for items stored here, like
             a heap and stack growing against each other
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

static uint8_t ctx_lerp_u8 (uint8_t v0, uint8_t v1, uint8_t dx)
{
  return (((((v0)<<8) + (dx) * ((v1) - (v0))))>>8);
}

#if CTX_GRADIENT_CACHE

#define CTX_GRADIENT_CACHE_ELEMENTS 128

static uint8_t ctx_gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];

static void
ctx_gradient_cache_reset (void)
{
  for (int i = 0; i < CTX_GRADIENT_CACHE_ELEMENTS; i++)
  {
    ctx_gradient_cache_u8[i][0] = 255;
    ctx_gradient_cache_u8[i][1] = 2;
    ctx_gradient_cache_u8[i][2] = 255;
    ctx_gradient_cache_u8[i][3] = 13;
  }
}

#endif

static void
ctx_sample_gradient_1d_u8 (CtxRenderer *renderer, float v, uint8_t *rgba)
{
  /* caching a 512 long gradient - and sampling with nearest neighbor
     will be much faster.. */
  CtxGradient *g = &renderer->state->gradient;

  if (v < 0) v = 0;
  if (v > 1) v = 1;

#if CTX_GRADIENT_CACHE
  int cache_no = v * (CTX_GRADIENT_CACHE_ELEMENTS-1.0f);
  uint8_t *cache_entry = &ctx_gradient_cache_u8[cache_no][0];

  if (!(//cache_entry[0] == 255 &&
        //cache_entry[1] == 2   &&
        //cache_entry[2] == 255 &&
        cache_entry[3] == 13))
  {
    rgba[0] = cache_entry[0];
    rgba[1] = cache_entry[1];
    rgba[2] = cache_entry[2];
    rgba[3] = cache_entry[3];
    return;
  }
#endif

  if (g->n_stops == 0)
  {
    rgba[0] = rgba[1] = rgba[2] = v * 255;
    rgba[3] = 255;
    return;
  }

#if CTX_GRADIENT_CACHE
  /* force first and last cached entries to be end points */
  if (cache_no == 0) v = 0.0f;
  else if (cache_no == CTX_GRADIENT_CACHE_ELEMENTS-1) v = 1.0f;
#endif

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
#if CTX_GRADIENT_CACHE
  cache_entry[0] = rgba[0];
  cache_entry[1] = rgba[1];
  cache_entry[2] = rgba[2];
  cache_entry[3] = rgba[3];
#endif
}


static void
ctx_sample_source_rgba_u8_image (CtxRenderer *renderer, float x, float y, void *out)
{
  uint8_t *rgba = out;
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
ctx_sample_source_rgba_u8_image_rgba (CtxRenderer *renderer, float x, float y, void *out)
{
  uint8_t *rgba = out;
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
ctx_sample_source_rgba_u8_image_1bit (CtxRenderer *renderer, float x, float y, void *out)
{
  uint8_t *rgba = out;
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
ctx_sample_source_rgba_u8_image_rgb (CtxRenderer *renderer, float x, float y, void *out)
{
  uint8_t *rgba = out;
  CtxSource *g = &renderer->state->gstate.source;
  CtxBuffer *buffer = g->image.buffer;
  ctx_assert (renderer);
  ctx_assert (g);
  ctx_assert (buffer);
  int u = x - g->image.x0;
  int v = y - g->image.y0;

  if ( (u < 0) || (v < 0) ||
       (u >= buffer->width-1) ||
       (v >= buffer->height-1))
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
ctx_sample_source_rgba_u8_radial_gradient (CtxRenderer *renderer, float x, float y, void *out)
{
  uint8_t *rgba = out;
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
ctx_sample_source_rgba_u8_linear_gradient (CtxRenderer *renderer, float x, float y, void *out)
{
  uint8_t *rgba = out;
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
ctx_sample_source_rgba_u8_color (CtxRenderer *renderer, float x, float y, void *out)
{
  uint8_t *rgba = out;
  CtxSource *g = &renderer->state->gstate.source;
  ctx_color_get_rgba_u8 (&g->color, rgba);
}

typedef void (*CtxSourceU8)(CtxRenderer *renderer, float x, float y, void *out);

static CtxSourceU8 ctx_renderer_get_source_rgba_u8 (CtxRenderer *renderer)
{
  CtxGState *gstate = &renderer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (gstate->source.type)
  {
    case CTX_SOURCE_IMAGE:
      switch (buffer->format->bpp)
      {
        case 1:
          return ctx_sample_source_rgba_u8_image_1bit;
        //case 2:
        //  return ctx_sample_source_rgba_u8_image_2bit;
        case 24:
          return ctx_sample_source_rgba_u8_image_rgb;
        case 32:
          return ctx_sample_source_rgba_u8_image_rgba;
        default:
          return ctx_sample_source_rgba_u8_image;
      }
    case CTX_SOURCE_COLOR:
      return ctx_sample_source_rgba_u8_color;
    case CTX_SOURCE_LINEAR_GRADIENT:
      return ctx_sample_source_rgba_u8_linear_gradient;
    case CTX_SOURCE_RADIAL_GRADIENT:
      return ctx_sample_source_rgba_u8_radial_gradient;
  }
  return ctx_sample_source_rgba_u8_color;
}


#define MASK_ALPHA       (0xff << 24)
#define MASK_GREEN_ALPHA ((0xff << 8)|MASK_ALPHA)
#define MASK_RED_BLUE    ((0xff << 16) | (0xff))

static void ctx_over_RGBA8 (uint8_t * restrict dst, uint8_t * restrict src , uint8_t cov)
{
  uint8_t ralpha = 255 - ((cov * src[3]) >> 8);
  for (int c = 0; c < 4; c++)
    dst[c] = (src[c]*cov + dst[c] * ralpha) >> 8;
}

static int
ctx_b2f_over_RGBA8 (CtxRenderer *renderer, int x0, uint8_t * restrict dst, uint8_t *restrict coverage, int count)
{
  CtxGState *gstate = &renderer->state->gstate;

  uint8_t color[4];

  if (gstate->source.type != CTX_SOURCE_COLOR)
  {
    CtxSourceU8 source = ctx_renderer_get_source_rgba_u8 (renderer);
    float y = renderer->scanline / CTX_RASTERIZER_AA;
    for (int x = 0; x < count; x++)
    {
      uint8_t cov = coverage[x];
      if (cov)
      {
        float u = x0 + x;
        float v = y;
        ctx_matrix_apply_transform (&gstate->source.transform, &u, &v);

        source (renderer, u, v, &color[0]);
        if (color[3])
        {
#if 0
          if ((gstate->source.global_alpha != 255) ||
              (color[3]!=255))
          {
            color[3] = (color[3] * gstate->source.global_alpha)>>8;
            color[0] = (color[0] * color[3])>>8;
            color[1] = (color[1] * color[3])>>8;
            color[2] = (color[2] * color[3])>>8;
          }
#endif
          ctx_over_RGBA8 (dst, color, cov);
        }
      }
      dst += 4;
    }
    return count;
  }
  ctx_color_get_rgba_u8 (&gstate->source.color, color);
  color[3] = (color[3] * gstate->global_alpha_u8)>>8;

  if (color[3] == 255)
  {
    for (int x = 0; x < count; x++)
    {
      int cov = *coverage;
      if (cov)
      {
        if (cov >= 255)
        {
          *((uint32_t*)dst) = *((uint32_t*)color);
        }
        else
        {
          ctx_over_RGBA8 (dst, color, cov);
        }
      }
      coverage++;
      dst += 4;
    }
    return count;
  }

  color[0] = (color[0] * color[3])>>8;
  color[1] = (color[1] * color[3])>>8;
  color[2] = (color[2] * color[3])>>8;

  for (int x = 0; x < count; x++)
  {
    uint8_t cov = *coverage;;
    if (cov)
    {
      ctx_over_RGBA8 (dst, color, cov);
    }
    dst += 4;
    coverage++;
  }
  return count;
}

static int
ctx_b2f_over_RGBA8_convert (CtxRenderer *renderer, int x, uint8_t *restrict dst, uint8_t *restrict coverage, int count)
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

static void
ctx_decode_pixels_BGRA8(CtxRenderer *renderer, int x, const void *restrict buf, uint8_t *restrict rgba, int count)
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

static void
ctx_encode_pixels_BGRA8 (CtxRenderer *renderer, int x, void *restrict buf, const uint8_t *restrict rgba, int count)
{
  ctx_decode_pixels_BGRA8(renderer, x, rgba, (uint8_t*)buf, count);
}

static int
ctx_b2f_over_BGRA8 (CtxRenderer *renderer, int x0, uint8_t *restrict dst, uint8_t *restrict coverage, int count)
{
  CtxGState *gstate = &renderer->state->gstate;

  uint8_t color[4];

  if (gstate->source.type != CTX_SOURCE_COLOR)
  {
    CtxSourceU8 source = ctx_renderer_get_source_rgba_u8 (renderer);
    float y = renderer->scanline / CTX_RASTERIZER_AA;
    for (int x = 0; x < count; x++)
    {
      uint8_t cov = coverage[x];
      if (cov)
      {
        float u = x0 + x;
        float v = y;
        ctx_matrix_apply_transform (&gstate->source.transform, &u, &v);
        source (renderer, u, v, &color[0]);
        if (color[3])
        {
          ctx_swap_red_green (color);
          if ((gstate->global_alpha_u8 != 255 )||
              (color[3]!=255))
          {
            color[3] = (color[3] * gstate->global_alpha_u8)>>8;
            color[0] = (color[0] * color[3])>>8;
            color[1] = (color[1] * color[3])>>8;
            color[2] = (color[2] * color[3])>>8;
          }
          ctx_over_RGBA8 (dst, color, cov);
        }
      }
      dst += 4;
    }
    return count;
  }

  ctx_color_get_rgba_u8 (&gstate->source.color, color);
  if (gstate->global_alpha_u8 != 255)
  {
    color[3] = (color[3] * gstate->global_alpha_u8)>>8;
  }
  ctx_swap_red_green (color);

  if (color[3] >= 255)
  {
    for (int x = 0; x < count; x++)
    {
      int cov = *coverage;;
      if (cov)
      {
        if (cov >= 240)
        {
          *((uint32_t*)dst) = *((uint32_t*)color);
        }
        else
        {
          ctx_over_RGBA8 (dst, color, cov);
        }
      }
      coverage++;
      dst += 4;
    }
    return count;
  }

  color[0] = (color[0] * color[3])>>8;
  color[1] = (color[1] * color[3])>>8;
  color[2] = (color[2] * color[3])>>8;

  {
    for (int x = 0; x < count; x++)
    {
      uint8_t cov = *coverage;;
      if (cov)
      {
        ctx_over_RGBA8 (dst, color, cov);

#undef MASK_GREEN
#undef MASK_GREEN_ALPHA
#undef MASK_RED_BLUE
      }
      dst += 4;
      coverage++;
    }
  }
  return count;
}

#endif


#if CTX_ENABLE_GRAYF
static int
ctx_gray_float_b2f_over (CtxRenderer *renderer, int x0, uint8_t *restrict dst, uint8_t *restrict coverage, int count)
{
  float *dst_f = (float*)dst;
  float y = renderer->scanline / CTX_RASTERIZER_AA;
  float graya[2];
 
  ctx_color_get_graya (&renderer->state->gstate.source.color, graya);

  float gray = graya[0];
  float alpha = graya[1];

  CtxSourceU8 source = ctx_renderer_get_source_rgba_u8 (renderer);
  if (source == ctx_sample_source_rgba_u8_color) source = NULL;

  for (int x = 0; x < count; x++)
  {
    int cov = coverage[x];
    if (cov != 0)
    {
      if (source)
      {
        uint8_t scolor[4];
        float u = x0 + x;
        float v = y;
        ctx_matrix_apply_transform (&renderer->state->gstate.source.transform, &u, &v);
        source (renderer, u, v, &scolor[0]);
        // XXX - we need generic sources for gray and cmyk
        gray = ((scolor[0]+scolor[1]+scolor[2])/3.0)/255.0; 
        alpha = scolor[3]/255.0 + renderer->state->gstate.global_alpha_u8/255.0;
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
ctx_associated_rgba_float_b2f_over (CtxRenderer *renderer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
          int components = 4;  // this makes it mostly adapted to become
                               // a generalized floating point ver..
                               // for components == 1-4 assume RGB color source
                               // for more components - use alternate generic
                               // source setting, which permits operation with
                               // non RGB color models.
          float *dst_f = (float*)dst;
          float y = renderer->scanline / CTX_RASTERIZER_AA;

          CtxSourceU8 source = ctx_renderer_get_source_rgba_u8 (renderer);
          float color_f[components];

  if (source == ctx_sample_source_rgba_u8_color)
  {
    uint8_t scolor[4];
    ctx_sample_source_rgba_u8_color (renderer, x0, y, &scolor[0]);
    for (int c = 0; c < components; c++)
      color_f[c]=scolor[c]/255.0f;  // XXX color

    color_f[components-1] *= renderer->state->gstate.global_alpha_f;
    for (int c = 0; c < components-1; c++)
      color_f[c] *= color_f[components-1];
    source = NULL;
  }

  {
    for (int x = 0; x < count; x++)
    {
      float cov = coverage[x]/255.0f;
      if (cov != 0.0f)
      {
        if (source)
        {
          uint8_t scolor[4];
          source (renderer, x0 + x, y, &scolor[0]);
          for (int c = 0; c < components; c++)
            color_f[c]=scolor[c]/255.0f;  // XXX ; lacks gamma handling

          color_f[components-1] *= renderer->state->gstate.global_alpha_f;
          for (int c = 0; c < components-1; c++)
            color_f[c] *= color_f[components-1];
        }

        float ralpha = 1.0f - color_f[components-1] * cov;
        for (int c = 0; c < components; c++)
          dst_f[c] = color_f[c] * cov + dst_f[c] * ralpha;
      }
      dst_f += components;
    }
  }
  return count;
}
#endif

#if CTX_ENABLE_CMYKAF

static void
ctx_sample_source_cmyka_f_color (CtxRenderer *renderer, float x, float y, void *out)
{
  float *cmyka = out;
  // XXX : only solid color implemented for now
  ctx_color_get_cmyka (&renderer->state->gstate.source.color, out);
  for (int i = 0; i < 4; i ++)
    cmyka[i] *= cmyka[4];
}

static CtxSourceU8 ctx_renderer_get_source_cmykaf (CtxRenderer *renderer)
{
  return ctx_sample_source_cmyka_f_color;
}

static int
ctx_associated_cmyka_float_b2f_over (CtxRenderer *renderer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  int components = 5;
  float *dst_f = (float*)dst;
  float y = renderer->scanline / CTX_RASTERIZER_AA;
  CtxSourceU8 source = ctx_renderer_get_source_cmykaf (renderer);
  float color_f[components];
  if (source == ctx_sample_source_cmyka_f_color)
  {
    source (renderer, x0, y, color_f);
    color_f[components-1] *= renderer->state->gstate.global_alpha_f;
    for (int c = 0; c < components-1; c++)
      color_f[c] *= color_f[components-1];
    source = NULL;
  }

  for (int x = 0; x < count; x++)
  {
    float cov = coverage[x]/255.0f;
    if (cov != 0.0f)
    {
      if (source)
      {
        source (renderer, x0 + x, y, color_f);
        color_f[components-1] *= renderer->state->gstate.global_alpha_f;
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

#if CTX_ENABLE_CMYKA8

static void
ctx_CMYKA8_to_CMYKAF (CtxRenderer *renderer, uint8_t *src, float *dst, int count)
{
  for (int i = 0; i < count; i ++)
  {
     dst[0] = (255-src[0])/255.0f;
     dst[1] = (255-src[1])/255.0f;
     dst[2] = (255-src[2])/255.0f;
     dst[3] = (255-src[3])/255.0f;
     dst[4] = src[1];

     for (int c = 0; c < 4; c++)
       dst[c] *= dst[4];
     src += 5;
     dst += 5;
  }
}
static void
ctx_CMYKAF_to_CMYKA8 (CtxRenderer *renderer, float *src, uint8_t *dst, int count)
{
  for (int i = 0; i < count; i ++)
  {
     float c = src[0]; float m = src[1]; float y = src[2]; float k = src[3];
     float a = src[4];

     if (a != 0.0f && a != 1.0f)
     {
        float recip = 1.0f/a;
        c *= recip; m *= recip; y *= recip; k *= recip;
     }
     c = 1.0 - c; m = 1.0 - m; y = 1.0 - y; k = 1.0 - k;
     dst[0] = c * 255.99;
     dst[1] = m * 255.99;
     dst[2] = y * 255.99;
     dst[3] = k * 255.99;
     dst[4] = a * 255.99;

     src += 5;
     dst += 5;
  }
}

static int
ctx_b2f_over_CMYKA8 (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  float pixels[count * 5];

  ctx_CMYKA8_to_CMYKAF (renderer, dst, &pixels[0], count);
  ret = ctx_associated_cmyka_float_b2f_over (renderer, x, (uint8_t*)&pixels[0], coverage, count);
  ctx_CMYKAF_to_CMYKA8 (renderer, &pixels[0], dst, count);
  return ret;
}
#endif

#if CTX_ENABLE_CMYK8

static void
ctx_CMYK8_to_CMYKAF (CtxRenderer *renderer, uint8_t *src, float *dst, int count)
{
  for (int i = 0; i < count; i ++)
  {
     dst[0] = (255-src[0])/255.0f;
     dst[1] = (255-src[1])/255.0f;
     dst[2] = (255-src[2])/255.0f;
     dst[3] = (255-src[3])/255.0f;
     dst[4] = 1.0f;
     src += 4;
     dst += 5;
  }
}
static void
ctx_CMYKAF_to_CMYK8 (CtxRenderer *renderer, float *src, uint8_t *dst, int count)
{
  for (int i = 0; i < count; i ++)
  {
     float c = src[0]; float m = src[1]; float y = src[2]; float k = src[3];
     float a = src[4];

     if (a != 0.0f && a != 1.0f)
     {
        float recip = 1.0f/a;
        c *= recip; m *= recip; y *= recip; k *= recip;
     }
     c = 1.0 - c; m = 1.0 - m; y = 1.0 - y; k = 1.0 - k;

     dst[0] = c * 255.99;
     dst[1] = m * 255.99;
     dst[2] = y * 255.99;
     dst[3] = k * 255.99;

     src += 5;
     dst += 4;
  }
}

static int
ctx_b2f_over_CMYK8 (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  float pixels[count * 5];

  ctx_CMYK8_to_CMYKAF (renderer, dst, &pixels[0], count);
  ret = ctx_associated_cmyka_float_b2f_over (renderer, x, (uint8_t*)&pixels[0], coverage, count);
  ctx_CMYKAF_to_CMYK8 (renderer, &pixels[0], dst, count);
  return ret;
}
#endif

#endif

static int
ctx_renderer_apply_coverage (CtxRenderer *renderer,
                             uint8_t     *dst,
                             int          x,
                             uint8_t     *coverage,
                             int          count)
{
  if (x + count >= renderer->blit_x + renderer->blit_width)
  {
    count = renderer->blit_x + renderer->blit_width - x - 1;
  }
  return renderer->format->crunch (renderer, x, dst, coverage, count);
}

static void
ctx_renderer_generate_coverage (CtxRenderer *renderer,
                                int          minx,
                                int          maxx,
                                uint8_t     *coverage,
                                int          winding,
                                int          aa)
{
  int scanline     = renderer->scanline;
  int active_edges = renderer->active_edges;
  int parity = 0;

  coverage -= minx;

#define CTX_EDGE(no)      renderer->edge_list.entries[renderer->edges[no].index]
#define CTX_EDGE_YMIN(no) CTX_EDGE(no).data.s16[1]
#define CTX_EDGE_YMAX(no) CTX_EDGE(no).data.s16[3]
#define CTX_EDGE_SLOPE(no) renderer->edges[no].dx
#define CTX_EDGE_X(no)     (renderer->edges[no].x)

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
      int x0 = CTX_EDGE_X(t)      / CTX_SUBDIV ;
      int x1 = CTX_EDGE_X(next_t) / CTX_SUBDIV ;

      if ((x0 < x1))
      {
        int first = x0 / CTX_RASTERIZER_EDGE_MULTIPLIER;
        int last  = x1 / CTX_RASTERIZER_EDGE_MULTIPLIER;
#if 0
        if (first < 0)
          first = 0;
        if (first >= maxx-minx)
          first = maxx-minx;
        if (last < 0)
          last = 0;
#endif
        if (first < minx)
          first = minx;
        if (last >= maxx)
          last = maxx;
        if (first > last)
          return;

        int graystart = 255-((x0 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff);
        int grayend   = (x1 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff;

        if (aa)
        {
          if ((first!=last) && graystart)
          {
            int cov = coverage[first] + graystart / CTX_RASTERIZER_AA;
            coverage[first] = ctx_minf(cov,255);
            first++;
          }
          for (int x = first; x < last; x++)
          {
            int cov = coverage[x] + 255 / CTX_RASTERIZER_AA;
            coverage[x] = ctx_minf(cov,255);
          }
          if (grayend) {
            int cov = coverage[last] + grayend / CTX_RASTERIZER_AA;
            coverage[last] = ctx_minf(cov,255);
          }
        }
        else
        {
          if ((first!=last) && graystart)
          {
            int cov = coverage[first] + graystart;
            coverage[first] = ctx_minf(cov,255);
            first++;
          }

          for (int x = first; x < last; x++)
          {
            coverage[x] = 255;
          }
          if (grayend)
          {
            int cov = coverage[last] + grayend;
            coverage[last] = ctx_minf(cov,255);
          }
        }

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
  renderer->lingering_edges = 0;
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
  renderer->col_min = 5000;
  renderer->col_max = -5000;
}

static void
ctx_renderer_rasterize_edges (CtxRenderer *renderer, int winding
#if CTX_SHAPE_CACHE
                              ,CtxShapeEntry *shape
#endif
                              )
{
  uint8_t *dst = ((uint8_t*)renderer->buf);
  int scan_start = renderer->blit_y * CTX_RASTERIZER_AA;
  int scan_end   = scan_start + renderer->blit_height * CTX_RASTERIZER_AA;
  int blit_width = renderer->blit_width;
  int blit_max_x = renderer->blit_x + blit_width;

  int minx = renderer->col_min / CTX_SUBDIV - renderer->blit_x;
  int maxx = (renderer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV - renderer->blit_x;
#if 1
  if (
#if CTX_SHAPE_CACHE
                   !shape && 
#endif
                  
                  maxx > blit_max_x - 1)
    maxx = blit_max_x - 1;
#endif

#if 1
    if (renderer->state->gstate.clip_min_x>
        minx)
         minx = renderer->state->gstate.clip_min_x;
    if (renderer->state->gstate.clip_max_x <
        maxx)
         maxx = renderer->state->gstate.clip_max_x;
#endif

  if (minx < 0)
    minx = 0;
  if (minx >= maxx)
  {
          ctx_renderer_reset (renderer);
          return;
  }

#if CTX_SHAPE_CACHE
  uint8_t _coverage[shape?2:maxx-minx+1];
#else
  uint8_t _coverage[maxx-minx+1];
#endif
  uint8_t *coverage = &_coverage[0];
#if CTX_SHAPE_CACHE
  if (shape)
  {
    coverage = &shape->data[0];
  }
#endif
  ctx_assert (coverage);

  renderer->scan_min -= (renderer->scan_min % CTX_RASTERIZER_AA);

#if CTX_SHAPE_CACHE
  if (shape)
  {
    scan_start = renderer->scan_min;
    scan_end   = renderer->scan_max;
  }
  else
#endif
  {
    if (renderer->scan_min > scan_start)
    {
      dst += (renderer->blit_stride * (renderer->scan_min-scan_start)/CTX_RASTERIZER_AA);
      scan_start = renderer->scan_min;
    }
    if (renderer->scan_max < scan_end)
      scan_end = renderer->scan_max;
  }

  if (renderer->state->gstate.clip_min_y * CTX_RASTERIZER_AA > scan_start )
    scan_start = renderer->state->gstate.clip_min_y * CTX_RASTERIZER_AA;
  if (renderer->state->gstate.clip_max_y *  CTX_RASTERIZER_AA < scan_end)
    scan_end = renderer->state->gstate.clip_max_y * CTX_RASTERIZER_AA;

  ctx_renderer_sort_edges (renderer);

  if (scan_start > scan_end) return;

  for (renderer->scanline = scan_start; renderer->scanline < scan_end;)
  {
    ctx_memset (coverage, 0,
#if CTX_SHAPE_CACHE
                    shape?shape->width:
#endif
                    sizeof(_coverage));

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
        ctx_renderer_generate_coverage (renderer, minx, maxx, coverage, winding, 1);
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
#if 1
      renderer->scanline += CTX_RASTERIZER_AA3;
      ctx_renderer_increment_edges (renderer, CTX_RASTERIZER_AA3);
      ctx_renderer_feed_edges (renderer);
      ctx_renderer_discard_edges (renderer);
      ctx_renderer_sort_active_edges (renderer);
      ctx_renderer_generate_coverage (renderer, minx, maxx, coverage, winding, 0);
      renderer->scanline += CTX_RASTERIZER_AA2;
      ctx_renderer_increment_edges (renderer, CTX_RASTERIZER_AA2);
#else
      renderer->scanline += CTX_RASTERIZER_AA;

#endif
    }

    if (maxx>minx)
    {
#if CTX_SHAPE_CACHE
      if (shape == NULL)
#endif
      {
        ctx_renderer_apply_coverage (renderer,
                                     &dst[(minx * renderer->format->bpp)/8],
                                     minx,
                                     coverage, maxx-minx);
      }
    }
#if CTX_SHAPE_CACHE
      if (shape)
      {
        coverage += shape->width;
      }
#endif
    dst += renderer->blit_stride;
  }
  ctx_renderer_reset (renderer);
}


static inline int
ctx_renderer_fill_rect (CtxRenderer *renderer,
                        int          x0,
                        int          y0,
                        int          x1,
                        int          y1)
{
  if (x0>x1 || y0>y1) return 1; // XXX : maybe this only happens under
                                //       memory corruption
  if (x1 % CTX_SUBDIV ||
      x0 % CTX_SUBDIV ||
      y1 % CTX_RASTERIZER_AA ||
      y0 % CTX_RASTERIZER_AA)
    return 0;

  x1 /= CTX_SUBDIV;
  x0 /= CTX_SUBDIV;
  y1 /= CTX_RASTERIZER_AA;
  y0 /= CTX_RASTERIZER_AA;

  uint8_t coverage[x1-x0 + 1];
  uint8_t *dst = ((uint8_t*)renderer->buf);
  ctx_memset (coverage, 0xff, sizeof (coverage));

  if (x0 < renderer->blit_x)
    x0 = renderer->blit_x;
  if (y0 < renderer->blit_y)
    y0 = renderer->blit_y;
  if (y1 > renderer->blit_y + renderer->blit_height)
    y1 = renderer->blit_y + renderer->blit_height;
  if (x1 > renderer->blit_x + renderer->blit_width)
    x1 = renderer->blit_x + renderer->blit_width;

  dst += (y0 - renderer->blit_y) * renderer->blit_stride;

  int width = x1 - x0 + 1;
  if (width > 0)
  {
    renderer->scanline = y0 * CTX_RASTERIZER_AA;
    for (int y = y0; y < y1; y++)
    {
      renderer->scanline += CTX_RASTERIZER_AA;
      ctx_renderer_apply_coverage (renderer,
                                   &dst[(x0) * renderer->format->bpp/8],
                                   x0,
                                   coverage, width);
      dst += renderer->blit_stride;
    }
  }
  return 1;
}

static inline void
ctx_renderer_fill (CtxRenderer *renderer)
{
#if 1
  if (renderer->scan_min / CTX_RASTERIZER_AA > renderer->blit_y + renderer->blit_height ||
      renderer->scan_max / CTX_RASTERIZER_AA < renderer->blit_y)
  {
    ctx_renderer_reset (renderer);
    return;
  }
#endif
#if 1
  if (renderer->col_min / CTX_SUBDIV > renderer->blit_x + renderer->blit_width ||
      renderer->col_max / CTX_SUBDIV < renderer->blit_x)
  {
    ctx_renderer_reset (renderer);
    return;
  }
#endif

  if (renderer->state->min_x > renderer->col_min / CTX_SUBDIV)
    renderer->state->min_x = renderer->col_min / CTX_SUBDIV;
  if (renderer->state->max_x < renderer->col_max / CTX_SUBDIV)
    renderer->state->max_x = renderer->col_max / CTX_SUBDIV;

  if (renderer->state->min_y > renderer->scan_min / CTX_RASTERIZER_AA)
    renderer->state->min_y = renderer->scan_min / CTX_RASTERIZER_AA;
  if (renderer->state->max_y < renderer->scan_max / CTX_RASTERIZER_AA)
    renderer->state->max_y = renderer->scan_max / CTX_RASTERIZER_AA;

  if (renderer->edge_list.count == 4)
  {
    CtxEntry *entry0 = &renderer->edge_list.entries[0];
    CtxEntry *entry1 = &renderer->edge_list.entries[1];
    CtxEntry *entry2 = &renderer->edge_list.entries[2];
    CtxEntry *entry3 = &renderer->edge_list.entries[3];

    if ((entry0->data.s16[2] == entry1->data.s16[2]) &&
        (entry0->data.s16[3] == entry3->data.s16[3]) &&
        (entry1->data.s16[3] == entry2->data.s16[3]) &&
        (entry2->data.s16[2] == entry3->data.s16[2])
        )
    {
      /* XXX ; also check that there is no subpixel bits.. */

      if (ctx_renderer_fill_rect (renderer,
                              entry3->data.s16[2],
                              entry3->data.s16[3],
                              entry1->data.s16[2],
                              entry1->data.s16[3]))
      {
        ctx_renderer_reset (renderer);
        return;
      }
    }
  }

  ctx_renderer_finish_shape (renderer);
#if CTX_SHAPE_CACHE


  uint32_t hash = ctx_renderer_poly_to_edges (renderer);
  int width = (renderer->col_max + (CTX_SUBDIV-1)) / CTX_SUBDIV - renderer->col_min/CTX_SUBDIV;
  int height = (renderer->scan_max + (CTX_RASTERIZER_AA-1)) / CTX_RASTERIZER_AA - renderer->scan_min / CTX_RASTERIZER_AA;

  if (width * height < CTX_SHAPE_CACHE_DIM && width >=1 && height >= 1 
        && width < CTX_SHAPE_CACHE_MAX_DIM 
        && height < CTX_SHAPE_CACHE_MAX_DIM)
  {
    int scan_min = renderer->scan_min;
    int col_min = renderer->col_min;
    CtxShapeEntry *shape = ctx_shape_entry_find (hash, width, height, ctx_shape_time++);
    if (shape->uses == 0)
    {
       ctx_renderer_rasterize_edges (renderer, renderer->state->gstate.fill_rule, shape);
    }

    scan_min -= (scan_min % CTX_RASTERIZER_AA);
    renderer->scanline = scan_min;

    int y0 = renderer->scanline / CTX_RASTERIZER_AA;
    int y1 = y0 + shape->height;
    int x0 = col_min / CTX_SUBDIV;

    int ymin = y0;
    int x1 = x0 + shape->width;
    int clip_x_min = renderer->blit_x;
    int clip_x_max = renderer->blit_x + renderer->blit_width - 1;
    int clip_y_min = renderer->blit_y;
    int clip_y_max = renderer->blit_y + renderer->blit_height - 1;

#if 0
    if (renderer->state->gstate.clip_min_x>
        clip_x_min)
            clip_x_min = renderer->state->gstate.clip_min_x;
    if (renderer->state->gstate.clip_max_x <
        clip_x_max)
            clip_x_max = renderer->state->gstate.clip_max_x;
#endif
#if 0
    if (renderer->state->gstate.clip_min_y>
        clip_y_min)
            clip_y_min = renderer->state->gstate.clip_min_y;
    if (renderer->state->gstate.clip_max_y <
        clip_y_max)
            clip_y_max = renderer->state->gstate.clip_max_y;
#endif

    if (x1 >= clip_x_max)
      x1 = clip_x_max;

    int xo = 0;
    if (x0 < clip_x_min)
    {
      xo = clip_x_min - x0;
      x0 = clip_x_min;
    }

    int ewidth = x1 - x0;

    if (ewidth>0)
    for (int y = y0; y < y1; y++)
    {
    if ((y >= clip_y_min) && (y <= clip_y_max))
    {
      ctx_renderer_apply_coverage (renderer,
                                 ((uint8_t*)renderer->buf) + (y-renderer->blit_y) * renderer->blit_stride + (int)(x0) * renderer->format->bpp/8,
                             x0,
                             &shape->data[shape->width * (int)(y-ymin) + xo],
                             ewidth );
    }
    }

    if (shape->uses != 0)
    {
      ctx_renderer_reset (renderer);
    }
    ctx_shape_entry_release (shape);
    return;
  }
  else
#else
  ctx_renderer_poly_to_edges (renderer);
#endif
  {
  //fprintf (stderr, "%i %i\n", width, height);
    ctx_renderer_rasterize_edges (renderer, renderer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                    , NULL
#endif
                    );
  }
}

static int _ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke);
static inline void
ctx_renderer_glyph (CtxRenderer *renderer, uint32_t unichar, int stroke)
{
  _ctx_glyph (renderer->ctx, unichar, stroke);
}

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible);
static inline void
ctx_renderer_text (CtxRenderer *renderer, const char *string, int stroke)
{
  _ctx_text (renderer->ctx, string, stroke, 1);
}

void
_ctx_set_font (Ctx *ctx, const char *name);
static inline void
ctx_renderer_set_font (CtxRenderer *renderer, const char *font_name)
{
  _ctx_set_font (renderer->ctx, font_name);
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

  if (end_angle == start_angle)
  {
//  if (renderer->has_prev!=0)
    ctx_renderer_line_to (renderer, x + cosf (end_angle) * radius,
                          y + sinf (end_angle) * radius);
//    else
//    ctx_renderer_move_to (renderer, x + cosf (end_angle) * radius,
//                          y + sinf (end_angle) * radius);

    return;
  }

#if 0
  if ((!anticlockwise && (end_angle - start_angle >= CTX_PI*2)) ||
     ((anticlockwise && (start_angle - end_angle >= CTX_PI*2))))
  {
    end_angle = start_angle;
    steps = full_segments - 1;
  }
  else
#endif
  {
    steps = (end_angle - start_angle) / (CTX_PI*2) * full_segments;
    //if (steps < 0) steps += full_segments;
    if (anticlockwise)
      steps = full_segments - steps;;
  }

  if (anticlockwise) step = step * -1;

  int first = 1;

  if (steps == 0 || steps==full_segments -1  || (anticlockwise && steps == full_segments))
  {
      float xv = x + cosf (start_angle) * radius;
      float yv = y + sinf (start_angle) * radius;
      if (!renderer->has_prev)
        ctx_renderer_move_to (renderer, xv, yv);
      
      first = 0;
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

  }
    ctx_renderer_line_to (renderer, x + cosf (end_angle) * radius,
                          y + sinf (end_angle) * radius);
}

static void
ctx_renderer_quad_to (CtxRenderer *renderer,
                      float        cx,
                      float        cy,
                      float        x,
                      float        y)
{
  /* XXX : it is probably cheaper/faster to do quad interpolation directly -
   *       though it will increase the code-size
   */
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

#define LENGTH_OVERSAMPLE 1
static void
ctx_renderer_pset (CtxRenderer *renderer, int x, int y, uint8_t cov)
{
     // XXX - we avoid rendering here x==0 - to keep with
     //  an off-by one elsewhere
  if (x <= 0 || y < 0 || x >= renderer->blit_width ||
                         y >= renderer->blit_height)
    return;
  uint8_t fg_color[4];
  ctx_color_get_rgba_u8 (&renderer->state->gstate.source.color, fg_color);

  uint8_t pixel[4];
  uint8_t *dst = ((uint8_t*)renderer->buf);
  dst += y * renderer->blit_stride;
  dst += x * renderer->format->bpp / 8;

  if (!renderer->format->to_rgba8 ||
      !renderer->format->from_rgba8)
    return;

  if (cov == 255)
  {
    for (int c = 0; c < 4; c++)
    {
      pixel[c] = fg_color[c];
    }
  }
  else
  {
    renderer->format->to_rgba8 (renderer, x, dst, &pixel[0], 1);
    for (int c = 0; c < 4; c++)
    {
      pixel[c] = ctx_lerp_u8 (pixel[c], fg_color[c], cov);
    }
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

      int length = ctx_maxf(abs(dx), abs(dy));

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
          ctx_renderer_pset (renderer, tx/256, ty/256, 255);
          tx += dx;
          ty += dy;
          ctx_renderer_pset (renderer, tx/256, ty/256, 255);
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
  //CtxSource source_backup = renderer->state->gstate.source;
  //renderer->state->gstate.source = renderer->state->gstate.source_stroke;

  if (renderer->state->gstate.line_width <= 0.0f &&
      renderer->state->gstate.line_width > -10.0f)
  {
    ctx_renderer_stroke_1px (renderer);
    //renderer->state->gstate.source = source_backup;
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
           dx = dx / length * half_width_x;
           dy = dy / length * half_width_y;

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
             ctx_renderer_arc (renderer, x, y, half_width_x, CTX_PI*3, 0, 1);
             ctx_renderer_finish_shape (renderer);
           }

           x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
           y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;

           ctx_renderer_arc (renderer, x, y, half_width_x, CTX_PI*3, 0, 1);
           ctx_renderer_finish_shape (renderer);
         }

         x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
         y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;
         has_prev = 1;
       }
       ctx_renderer_move_to (renderer, x, y);
       ctx_renderer_arc (renderer, x, y, half_width_x, CTX_PI*3, 0, 1);
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
           ctx_renderer_arc (renderer, x, y, half_width_x, CTX_PI*3, 0, 1);
           ctx_renderer_finish_shape (renderer);
         }
       }
      break;
    }
  }

#if 0
  ctx_renderer_poly_to_edges    (renderer);
  ctx_renderer_rasterize_edges (renderer, 1, NULL);
#else
  CtxFillRule rule_backup = renderer->state->gstate.fill_rule;
  renderer->state->gstate.fill_rule = CTX_FILL_RULE_WINDING; 

  ctx_renderer_fill (renderer);

  renderer->state->gstate.fill_rule = rule_backup;
#endif
  //renderer->state->gstate.source = source_backup;
  renderer->state->gstate.transform = transform_backup;
}

static void
ctx_renderer_clip (CtxRenderer *renderer)
{
  // XXX
  //  keep a rectangular clip/scissors rect
  //  optionally keep a 1 2 4 8 or f graybuffer
  //////

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
      if (prev_x < minx) minx = prev_x;
      if (prev_y < miny) miny = prev_y;
      if (prev_x > maxx) maxx = prev_x;
      if (prev_y > maxy) maxy = prev_y;
    }
    x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
    y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;

    if (x < minx) minx = x;
    if (y < miny) miny = y;
    if (x > maxx) maxx = x;
    if (y > maxy) maxy = y;
  }

  if (minx > renderer->state->gstate.clip_min_x)
    renderer->state->gstate.clip_min_x = minx;
  if (miny > renderer->state->gstate.clip_min_y)
    renderer->state->gstate.clip_min_y = miny;
  if (maxx < renderer->state->gstate.clip_max_x)
    renderer->state->gstate.clip_max_x = maxx;
  if (maxy < renderer->state->gstate.clip_max_y)
    renderer->state->gstate.clip_max_y = maxy;
  ctx_renderer_reset (renderer);
}

static void
ctx_renderer_set_texture (CtxRenderer *renderer,
                          int   no,
                          float x,
                          float y)
{
  if (no < 0 || no >= CTX_MAX_TEXTURES) no = 0;

  if (renderer->ctx->texture[no].data == NULL)
  {
    ctx_log ("failed setting texture %i\n", no);
    return;
  }
  renderer->state->gstate.source.type = CTX_SOURCE_IMAGE;
  renderer->state->gstate.source.image.buffer = &renderer->ctx->texture[no];

  //ctx_user_to_device (renderer->state, &x, &y);

  renderer->state->gstate.source.image.x0 = 0;
  renderer->state->gstate.source.image.y0 = 0;

  renderer->state->gstate.source.transform = renderer->state->gstate.transform;
  ctx_matrix_translate (&renderer->state->gstate.source.transform, x, y);
  ctx_matrix_inverse (&renderer->state->gstate.source.transform);
}

#if 0
static void
ctx_renderer_load_image (CtxRenderer *renderer,
                         const char  *path,
                         float x,
                         float y)
{
  // decode PNG, put it in image is slot 1,
  // magic width height stride format data
  ctx_buffer_load_png (&renderer->ctx->texture[0], path);
  ctx_renderer_set_texture (renderer, 0, x, y);
}
#endif

static void
ctx_renderer_set_pixel (CtxRenderer *renderer,
                        uint16_t x,
                        uint16_t y,
                        uint8_t r,
                        uint8_t g,
                        uint8_t b,
                        uint8_t a)
{
  renderer->state->gstate.source.type = CTX_SOURCE_COLOR;
  ctx_color_set_rgba_u8 (&renderer->state->gstate.source.color, r, g, b, a);

#if 1
  ctx_renderer_pset (renderer, x, y, 255);
#else
  ctx_renderer_move_to (renderer, x, y);
  ctx_renderer_rel_line_to (renderer, 1, 0);
  ctx_renderer_rel_line_to (renderer, 0, 1);
  ctx_renderer_rel_line_to (renderer, -1, 0);
  ctx_renderer_fill (renderer);
#endif
}

static void
ctx_renderer_rectangle (CtxRenderer *renderer,
                        float x,
                        float y,
                        float width,
                        float height)
{
  ctx_renderer_move_to (renderer, x, y);
  ctx_renderer_rel_line_to (renderer, width, 0);
  ctx_renderer_rel_line_to (renderer, 0, height);
  //ctx_renderer_rel_line_to (renderer, -width, 0);
  ctx_renderer_finish_shape (renderer);
}

static void
ctx_renderer_process (Ctx *ctx, CtxEntry *entry)
{
  CtxRenderer *renderer = ctx->renderer;
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
      ctx_renderer_rectangle (renderer, ctx_arg_float(0), ctx_arg_float(1),
                                        ctx_arg_float(2), ctx_arg_float(3));
      break;

    case CTX_SET_PIXEL:
      ctx_renderer_set_pixel (renderer, ctx_arg_u16 (2), ctx_arg_u16 (3),
        ctx_arg_u8(0), ctx_arg_u8(1), ctx_arg_u8(2), ctx_arg_u8(3));

      break;

    case CTX_TEXTURE:
      ctx_renderer_set_texture (renderer, ctx_arg_u32(0),
                                          ctx_arg_float(2), ctx_arg_float(3));
      break;

#if 0
    case CTX_LOAD_IMAGE:
      ctx_renderer_load_image (renderer, ctx_arg_string(),
                    ctx_arg_float(0), ctx_arg_float(1));
      break;
#endif

    case CTX_GRADIENT_CLEAR:
      ctx_renderer_gradient_clear_stops (renderer);
      break;
    case CTX_GRADIENT_STOP:
      ctx_renderer_gradient_add_stop (renderer, ctx_arg_float(0),
                                               &ctx_arg_u8(4));
      break;

    case CTX_LINEAR_GRADIENT:
#if CTX_GRADIENT_CACHE
      ctx_gradient_cache_reset();
#endif
      break;
    case CTX_RADIAL_GRADIENT:
#if CTX_GRADIENT_CACHE
      ctx_gradient_cache_reset();
#endif
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

    case CTX_SET_FONT:

      ctx_renderer_set_font (renderer, ctx_arg_string());
      break;

    case CTX_TEXT:
      ctx_renderer_text (renderer, ctx_arg_string(), 0);
      break;

    case CTX_TEXT_STROKE:
      ctx_renderer_text (renderer, ctx_arg_string(), 1);
      break;

    case CTX_GLYPH:
      ctx_renderer_glyph (renderer, entry[0].data.u32[0], entry[0].data.u8[4]);
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
    if (*pixel & (1<<(x&7)))
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
    if ((x&7)==7)
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
      *pixel = *pixel & (~(1<< (x&7)));
    }
    else
    {
      *pixel = *pixel | (1<< (x&7));
    }
    if ((x&7)==7)
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
    int val = (*pixel & (3 << ((x & 3)<<1)) ) >> ((x&3)<<1);
    val <<= 6;
    rgba[0] = val;
    rgba[1] = val;
    rgba[2] = val;
    rgba[3] = 255;
    if ((x&3)==3)
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
    *pixel = *pixel & (~(3 << ((x&3)<<1)));
    *pixel = *pixel | ((val << ((x&3)<<1)));
    if ((x&3)==3)
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
    int val = (*pixel & (15 << ((x & 1)<<2)) ) >> ((x&1)<<2);
    val <<= 4;
    rgba[0] = val;
    rgba[1] = val;
    rgba[2] = val;
    rgba[3] = 255;
    if ((x&1)==1)
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
    *pixel = *pixel & (~(15 << ((x&1)<<2)));
    *pixel = *pixel | ((val << ((x&1)<<2)));
    if ((x&1)==1)
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
         // for internal uses... using only green would work
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

#if CTX_ENABLE_RGB332
static inline void
ctx_332_unpack (uint8_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue)
{
  *blue   = (pixel & 3)<<6;
  *green = ((pixel >> 2) & 7)<<5;
  *red   = ((pixel >> 5) & 7)<<5;
  if (*blue > 223)  *blue  = 255;
  if (*green > 223) *green = 255;
  if (*red > 223)   *red   = 255;
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
ctx_decode_pixels_RGB332(CtxRenderer *renderer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    ctx_332_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2]);
    if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
      rgba[3] = 0;
    else
      rgba[3] = 255;
    pixel+=1;
    rgba +=4;
  }
}

static inline void
ctx_encode_pixels_RGB332 (CtxRenderer *renderer, int x, void *buf, const uint8_t *rgba, int count)
{
  uint8_t *pixel = (uint8_t*)buf;
  while (count--)
  {
    if (rgba[3]==0)
      pixel[0] = ctx_332_pack (255, 0, 255);
    else
      pixel[0] = ctx_332_pack (rgba[0], rgba[1], rgba[2]);
    pixel+=1;
    rgba +=4;
  }
}

static int
ctx_b2f_over_RGB332 (CtxRenderer *renderer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_decode_pixels_RGB332 (renderer, x, dst, &pixels[0], count);
  ret = ctx_b2f_over_RGBA8 (renderer, x, &pixels[0], coverage, count);
  ctx_encode_pixels_RGB332 (renderer, x, dst, &pixels[0], count);
  return ret;
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
  ctx_renderstream_deinit (&renderer->edge_list);
}

static CtxPixelFormatInfo ctx_pixel_formats[]=
{
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
   NULL, NULL, ctx_associated_rgba_float_b2f_over },
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
#if CTX_ENABLE_RGB332
  {CTX_FORMAT_RGB332, 3, 8, 4, 10, 12,
   ctx_decode_pixels_RGB332, ctx_encode_pixels_RGB332,
   ctx_b2f_over_RGB332},
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
#if CTX_ENABLE_CMYKAF
  {CTX_FORMAT_CMYKAF, 5, 160, 4 * 5, 0, 0,
   NULL, NULL, ctx_associated_cmyka_float_b2f_over },
#endif
#if CTX_ENABLE_CMYKA8
  {CTX_FORMAT_CMYKA8, 5, 40, 4 * 5, 0, 0,
   NULL, NULL, ctx_b2f_over_CMYKA8},
#endif
#if CTX_ENABLE_CMYK8
  {CTX_FORMAT_CMYK8, 5, 32, 4 * 5, 0, 0,
   NULL, NULL, ctx_b2f_over_CMYK8},
#endif
};

static CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format)
{
  for (unsigned int i = 0; i < sizeof (ctx_pixel_formats)/sizeof(ctx_pixel_formats[0]);i++)
  {
    if (ctx_pixel_formats[i].pixel_format == format)
    {
      return &ctx_pixel_formats[i];
    }
  }
  return NULL;
}

static void
ctx_renderer_init (CtxRenderer *renderer, Ctx *ctx, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format)
{
  ctx_memset (renderer, 0, sizeof (CtxRenderer));
  renderer->edge_list.flags |= CTX_RENDERSTREAM_EDGE_LIST;
  renderer->state       = state;
  renderer->ctx         = ctx;
  ctx_state_init (renderer->state);
  renderer->buf         = data;
  renderer->blit_x      = x;
  renderer->blit_y      = y;
  renderer->blit_width  = width;
  renderer->blit_height = height;

  renderer->state->gstate.clip_min_x  = x;
  renderer->state->gstate.clip_min_y  = y;
  renderer->state->gstate.clip_max_x  = x + width - 1;
  renderer->state->gstate.clip_max_y  = y + height - 1;

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
  ctx_renderer_init (ctx->renderer, ctx, &ctx->state,
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
  ctx_renderer_init (ctx->renderer, ctx, &ctx->state,
                     data, 0, 0, width, height, stride, pixel_format);
  return ctx;
}

// ctx_new_for_stream (FILE *stream);

#if 0
CtxRenderer *ctx_renderer_new (void *data, int x, int y, int width, int height,
          int stride, CtxPixelFormat pixel_format)
{
  CtxState    *state    = (CtxState*)malloc (sizeof (CtxState));
  CtxRenderer *renderer = (CtxRenderer*)malloc (sizeof (CtxRenderer));
  ctx_renderer_init (renderer, state, data, x, y, width, height,
                     stride, pixel_format);
  
}
#endif


/* add an or-able value to pixelformat to indicate vflip+hflip
 */
void
ctx_blit (Ctx *ctx, void *data, int x, int y, int width, int height,
          int stride, CtxPixelFormat pixel_format)
{
  CtxIterator iterator;
  CtxEntry   *entry;
//  CtxState    *state    = (CtxState*)malloc (sizeof (CtxState));
  CtxRenderer *renderer = (CtxRenderer*)malloc (sizeof (CtxRenderer));

  ctx_renderer_init (renderer, ctx, &ctx->state, data, x, y, width, height,
                     stride, pixel_format);

  ctx_iterator_init (&iterator, &ctx->renderstream, 0, CTX_ITERATOR_EXPAND_REFPACK|
                                                  CTX_ITERATOR_EXPAND_BITPACK);


  /* this is not re-entrant, a different way of permitting
   * renderer to use itself as ctx target is needed via some hook
   * in ctx_process()
   */
  ctx->renderer = renderer;
  while ((entry = ctx_iterator_next(&iterator)))
    ctx_renderer_process (ctx, entry);
  ctx->renderer = NULL;

  ctx_renderer_deinit (renderer);
  free (renderer);
  //free (state);
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

float ctx_x (Ctx *ctx)
{
  float x = 0, y = 0;
  ctx_current_point (ctx, &x, &y);
  return x;
}

float ctx_y (Ctx *ctx)
{
  float x = 0, y = 0;
  ctx_current_point (ctx, &x, &y);
  return y;
}


void
ctx_process (Ctx *ctx, CtxEntry *entry)
{
#if CTX_RASTERIZER
  if (ctx->renderer)
  {
    ctx_renderer_process (ctx, entry);
  }
  else
#endif
  {
    /* these functions might alter the code and coordinates of
       command that in the end gets added to the renderstream
     */
    ctx_interpret_style (&ctx->state, entry, ctx);
    ctx_interpret_transforms (&ctx->state, entry, ctx);
    ctx_interpret_pos (&ctx->state, entry, ctx);

    ctx_renderstream_add_entry (&ctx->renderstream, entry);

#if 1
    if (entry->code == CTX_TEXT ||
        entry->code == CTX_TEXT_STROKE ||
        entry->code == CTX_SET_FONT)
    {
      /* the image command and its data is submitted as one unit,
       */
      ctx_renderstream_add_entry (&ctx->renderstream, entry+1);
      ctx_renderstream_add_entry (&ctx->renderstream, entry+2);
    }
#endif
  }
}


/****  end of engine ****/

#if CTX_FONTS_FROM_FILE
static int
_ctx_file_get_contents (const char     *path,
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

static const char *ctx_utf8_skip (const char *s, int utf8_length)
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

//  XXX  :  unused
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
static float
ctx_glyph_width_stb (CtxFont *font, Ctx *ctx, uint32_t unichar);
static float
ctx_glyph_kern_stb (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB);
static int
ctx_glyph_stb (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke);

CtxFontEngine ctx_font_engine_stb = {
#if CTX_FONTS_FROM_FILE
   ctx_load_font_ttf_file,
#endif
   ctx_load_font_ttf,
   ctx_glyph_stb,
   ctx_glyph_width_stb,
   ctx_glyph_kern_stb,
};


int
ctx_load_font_ttf (const char *name, const void *ttf_contents, int length)
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    return -1;
  ctx_fonts[ctx_font_count].type = 1;
  ctx_fonts[ctx_font_count].name = (char*)malloc (strlen (name) + 1);
  strcpy ((char*)ctx_fonts[ctx_font_count].name, name);
  if (!stbtt_InitFont(&ctx_fonts[ctx_font_count].stb.ttf_info, ttf_contents, 0))
    {
      ctx_log( "Font init failed\n");
      return -1;
    }
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_stb;
  ctx_font_count ++;
  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ttf_file (const char *name, const char *path)
{
  uint8_t *contents = NULL;
  long length = 0;
  _ctx_file_get_contents (path, &contents, &length);
  if (!contents)
  {
    ctx_log( "File load failed\n");
    return -1;
  }
  return ctx_load_font_ttf (name, contents, length);
}
#endif

static int
ctx_glyph_stb_find (CtxFont *font, int unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  int index = font->stb.cache_index;
  if (font->stb.cache_unichar == unichar)
  {
    return index;
  }
  font->stb.cache_unichar = 0;
  index = font->stb.cache_index = stbtt_FindGlyphIndex (ttf_info, unichar);
  font->stb.cache_unichar = unichar;
  return index;
}

static float
ctx_glyph_width_stb (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  float font_size = ctx->state.gstate.font_size;
  float scale = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int advance, lsb;
  int glyph = ctx_glyph_stb_find (font, unichar);
  if (glyph==0)
    return 0.0f;
  stbtt_GetGlyphHMetrics (ttf_info, glyph, &advance, &lsb);
  return (advance * scale);
}

static float
ctx_glyph_kern_stb (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  float font_size = ctx->state.gstate.font_size;
  float scale = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int glyphA = ctx_glyph_stb_find (font, unicharA);
  int glyphB = ctx_glyph_stb_find (font, unicharB);
  return stbtt_GetGlyphKernAdvance (ttf_info, glyphA, glyphB) * scale;
}

static int
ctx_glyph_stb (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  int glyph = ctx_glyph_stb_find (font, unichar);
  if (glyph==0)
    return -1;

  float font_size = ctx->state.gstate.font_size;
  int   baseline = ctx->state.y;
  float origin_x = ctx->state.x;
  float origin_y = baseline;
  float scale    = stbtt_ScaleForPixelHeight (ttf_info, font_size);;

  stbtt_vertex *vertices = NULL;
  int num_verts = stbtt_GetGlyphShape (ttf_info, glyph, &vertices);

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
   {
     ctx_stroke (ctx);
   }
   else
     ctx_fill (ctx);
   return 0;
}
#endif

#if CTX_FONT_ENGINE_CTX

static float
ctx_glyph_kern_ctx (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  float font_size = ctx->state.gstate.font_size;
  if (font->ctx.first_kern == -1)
    return 0.0;

  for (int i = font->ctx.first_kern; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry*)&font->ctx.data[i];
    if (entry->code == CTX_KERNING_PAIR)
    {
      if (font->ctx.first_kern == 0) font->ctx.first_kern = i;
      if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
        return entry->data.s32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE;
    }
  }
  if (font->ctx.first_kern == 0)
    font->ctx.first_kern = -1;

  return 0;
}
#if 0
static int ctx_glyph_find (Ctx *ctx, CtxFont *font, uint32_t unichar)
{
  for (int i = 0; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry*)&font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
      return i;
  }
  return 0;
}
#endif

static int ctx_font_find_glyph (CtxFont *font, uint32_t glyph)
{
  for (int i = 0; i < font->ctx.glyphs; i++)
  {
    if (font->ctx.index[i * 2] == glyph)
      return font->ctx.index[i * 2 + 1];
  }
  return -1;
}

static float
ctx_glyph_width_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  CtxState *state = &ctx->state;
  float font_size = state->gstate.font_size;
  int   start     = ctx_font_find_glyph (font, unichar);

  if (start < 0)
    return 0.0;  // XXX : fallback

  for (int i = start; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry*)&font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH)
      if (entry->data.u32[0] == (unsigned)unichar)
        return (entry->data.u32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE);
  }
  return 0.0;
}

static int
ctx_glyph_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxState *state = &ctx->state;
  CtxIterator iterator;
  CtxRenderstream  renderstream = {(CtxEntry*)font->ctx.data,
                                              font->ctx.length,
                                              font->ctx.length, 0, 0
#if CTX_FULL_CB
                                              ,0,0,0
#endif
  };

  float origin_x = state->x;
  float origin_y = state->y;

  ctx_current_point (ctx, &origin_x, &origin_y);

  int in_glyph = 0;
  float font_size = state->gstate.font_size;
  int start = 0;
  start = ctx_font_find_glyph (font, unichar);
  if (start < 0)
    return -1;  // XXX : fallback

  ctx_iterator_init (&iterator, &renderstream, start, CTX_ITERATOR_EXPAND_BITPACK);

  CtxEntry *entry;

  while ((entry = ctx_iterator_next (&iterator)))
  {
    if (in_glyph)
    {
      if (entry->code == CTX_DEFINE_GLYPH)
      {
done:
        if (stroke)
          ctx_stroke (ctx);
        else
          ctx_fill (ctx);
        ctx_restore (ctx);
        return 0;
      }
      ctx_process (ctx, entry);
    }
    else if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
    {
      in_glyph = 1;
      ctx_save (ctx);
      ctx_translate (ctx, origin_x, origin_y);
      ctx_new_path (ctx);
      ctx_move_to (ctx, 0, 0);
      ctx_scale (ctx, font_size / CTX_BAKE_FONT_SIZE,
                      font_size / CTX_BAKE_FONT_SIZE);
    }
  }
goto done; // for the last glyph in a font
  return -1;
}

uint32_t ctx_glyph_no (Ctx *ctx, int no)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  if (no < 0 || no >= font->ctx.glyphs)
    return 0;
  return font->ctx.index[no*2];
}

static void ctx_font_init_ctx (CtxFont *font)
{
  int glyph_count = 0;
  for (int i = 0; i < font->ctx.length; i++)
  {
    CtxEntry *entry = &font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH)
      glyph_count ++;
  }
  font->ctx.glyphs = glyph_count;
#if CTX_RENDERSTREAM_STATIC
  static uint32_t idx[512];
  font->ctx.index = &idx[0];//(uint32_t*)malloc (sizeof (uint32_t) * 2 * glyph_count);
#else
  font->ctx.index = (uint32_t*)malloc (sizeof (uint32_t) * 2 * glyph_count);
#endif
  int no = 0;
  for (int i = 0; i < font->ctx.length; i++)
  {
    CtxEntry *entry = &font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH)
    {
      font->ctx.index[no*2]   = entry->data.u32[0];
      font->ctx.index[no*2+1] = i;
      no++;
    }
  }
}

int
ctx_load_font_ctx (const char *name, const void *data, int length);
#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path);
#endif

CtxFontEngine ctx_font_engine_ctx = {
#if CTX_FONTS_FROM_FILE
   ctx_load_font_ctx_file,
#endif
   ctx_load_font_ctx,
   ctx_glyph_ctx,
   ctx_glyph_width_ctx,
   ctx_glyph_kern_ctx,
};

int
ctx_load_font_ctx (const char *name, const void *data, int length)
{
  if (length % sizeof(CtxEntry))
    return -1;
  if (ctx_font_count >= CTX_MAX_FONTS)
    return -1;
  ctx_fonts[ctx_font_count].type = 0;
  ctx_fonts[ctx_font_count].name = name;
  ctx_fonts[ctx_font_count].ctx.data = (CtxEntry*)data;
  ctx_fonts[ctx_font_count].ctx.length = length / sizeof (CtxEntry);
  ctx_font_init_ctx (&ctx_fonts[ctx_font_count]);
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_ctx;
  ctx_font_count++;
  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path)
{
  uint8_t *contents = NULL;
  long length = 0;
  _ctx_file_get_contents (path, &contents, &length);
  if (!contents)
  {
    ctx_log( "File load failed\n");
    return -1;
  }
  return ctx_load_font_ctx (name, contents, length);
}
#endif
#endif


int
_ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  return font->engine->glyph (font, ctx, unichar, stroke);
}

int
ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
#if CTX_BACKEND_TEXT
  CtxEntry commands[1];
  ctx_memset (commands, 0, sizeof (commands));
  commands[0] = ctx_u32(CTX_GLYPH, unichar, 0);
  commands[0].data.u8[4] = stroke;
  ctx_process (ctx, commands);
  return 0; // XXX is return value used?
#else
  return _ctx_glyph (ctx, unichar, stroke);
#endif
}

float
ctx_glyph_width (Ctx *ctx, int unichar)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  return font->engine->glyph_width (font, ctx, unichar);
}

static float
ctx_glyph_kern (Ctx *ctx, int unicharA, int unicharB)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  return font->engine->glyph_kern (font, ctx, unicharA, unicharB);
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
_ctx_glyphs (Ctx     *ctx,
            CtxGlyph *glyphs,
            int       n_glyphs,
            int       stroke)
{
  for (int i = 0; i < n_glyphs; i++)
    {
      {
        uint32_t unichar = glyphs[i].index;
        ctx_move_to (ctx, glyphs[i].x, glyphs[i].y);
        ctx_glyph (ctx, unichar, stroke);
      }
    }
}

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible)
{
  CtxState *state = &ctx->state;
  float x = ctx->state.x;
  switch (state->gstate.text_align)
  {
    case CTX_TEXT_ALIGN_START:
    case CTX_TEXT_ALIGN_LEFT:
      break;
    case CTX_TEXT_ALIGN_CENTER:
      x -= ctx_text_width (ctx, string)/2;
      break;
    case CTX_TEXT_ALIGN_END:
    case CTX_TEXT_ALIGN_RIGHT:
      x -= ctx_text_width (ctx, string);
      break;
  }
  float y = ctx->state.y;
  float baseline_offset = 0.0f;
  switch (state->gstate.text_baseline)
  {
    case CTX_TEXT_BASELINE_HANGING:
            /* XXX : crude */
      baseline_offset = ctx->state.gstate.font_size * 0.55;
      break;
    case CTX_TEXT_BASELINE_TOP:
            /* XXX : crude */
      baseline_offset = ctx->state.gstate.font_size * 0.7;
      break;
    case CTX_TEXT_BASELINE_BOTTOM:
      baseline_offset = -ctx->state.gstate.font_size * 0.1;
      break;
    case CTX_TEXT_BASELINE_ALPHABETIC:
    case CTX_TEXT_BASELINE_IDEOGRAPHIC:
      baseline_offset = 0.0f;
      break;
    case CTX_TEXT_BASELINE_MIDDLE:
      baseline_offset = ctx->state.gstate.font_size * 0.25;
      break;
  }
  float x0 = x;
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1))
    {
      if (*utf8 == '\n')
      {
        y += ctx->state.gstate.font_size * state->gstate.line_spacing;
        x = x0; 
        if (visible)
          ctx_move_to (ctx, x, y);
      }
      else
      {
        uint32_t unichar = ctx_utf8_to_unichar (utf8);
        if (visible)
        {
          ctx_move_to (ctx, x, y + baseline_offset);
          _ctx_glyph (ctx, unichar, stroke);
        }
        const char *next_utf8 = ctx_utf8_skip (utf8, 1);
        if (next_utf8)
        {
          x += ctx_glyph_width (ctx, unichar);
          x += ctx_glyph_kern (ctx, unichar, ctx_utf8_to_unichar (next_utf8));
        }
        if (visible)
          ctx_move_to (ctx, x, y);
      }
    }
  if (!visible)
    ctx_move_to (ctx, x, y);
}


CtxGlyph *
ctx_glyph_allocate (int n_glyphs)
{
  return (CtxGlyph*)malloc (sizeof (CtxGlyph) * n_glyphs);
}
void
gtx_glyph_free     (CtxGlyph *glyphs)
{
  free (glyphs);
}

void
ctx_glyphs (Ctx        *ctx,
            CtxGlyph   *glyphs,
            int         n_glyphs)
{
  _ctx_glyphs (ctx, glyphs, n_glyphs, 0);
}

void
ctx_glyphs_stroke (Ctx        *ctx,
                   CtxGlyph   *glyphs,
                   int         n_glyphs)
{
  _ctx_glyphs (ctx, glyphs, n_glyphs, 1);
}


void
ctx_text (Ctx        *ctx,
          const char *string)
{
#if CTX_BACKEND_TEXT
  int stringlen = strlen (string);
  CtxEntry commands[1 + 2 + stringlen/8];
  ctx_memset (commands, 0, sizeof (commands));
  commands[0] = ctx_u8(CTX_TEXT, 0, 0,0,0,0,0,0,0);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = stringlen;
  commands[1].data.u32[1] = stringlen/9+1;
  strcpy ((char*)&commands[2].data.u8[0], string);
  ((char*)(&commands[2].data.u8[0]))[stringlen]=0;
  ctx_process (ctx, commands);
  _ctx_text (ctx, string, 0, 0);
#else
  _ctx_text (ctx, string, 0, 1);
#endif
}

void
ctx_text_stroke (Ctx        *ctx,
                 const char *string)
{
#if CTX_BACKEND_TEXT
  int stringlen = strlen (string);
  CtxEntry commands[1 + 2 + stringlen/8];
  ctx_memset (commands, 0, sizeof (commands));
  commands[0] = ctx_u8(CTX_TEXT_STROKE, 1, 0,0,0,0,0,0,0);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = stringlen;
  commands[1].data.u32[1] = stringlen/9+1;
  strcpy ((char*)&commands[2].data.u8[0], string);
  ((char*)(&commands[2].data.u8[0]))[stringlen]=0;
  ctx_process (ctx, commands);
  _ctx_text (ctx, string, 1, 0);
#else
  _ctx_text (ctx, string, 1, 1);
#endif
}

static void ctx_setup ()
{
  static int initialized = 0;
  if (initialized) return;
  initialized = 1;

#if CTX_FONT_ENGINE_CTX
  ctx_font_count = 0; // oddly - this is needed in arduino
#if CTX_FONT_regular
  ctx_load_font_ctx ("sans-ctx", ctx_font_regular, sizeof (ctx_font_regular));
#endif
#if CTX_FONT_mono
  ctx_load_font_ctx ("mono-ctx", ctx_font_mono, sizeof (ctx_font_mono));
#endif
#if CTX_FONT_bold
  ctx_load_font_ctx ("bold-ctx", ctx_font_bold, sizeof (ctx_font_bold));
#endif
#if CTX_FONT_italic
  ctx_load_font_ctx ("italic-ctx", ctx_font_italic, sizeof (ctx_font_italic));
#endif
#if CTX_FONT_sans
  ctx_load_font_ctx ("sans-ctx", ctx_font_sans, sizeof (ctx_font_sans));
#endif
#if CTX_FONT_serif
  ctx_load_font_ctx ("serif-ctx", ctx_font_serif, sizeof (ctx_font_serif));
#endif
#if CTX_FONT_symbol
  ctx_load_font_ctx ("symbol-ctx", ctx_font_symbol, sizeof (ctx_font_symbol));
#endif
#if CTX_FONT_emoji
  ctx_load_font_ctx ("emoji-ctx", ctx_font_emoji, sizeof (ctx_font_emoji));
#endif
#endif
#if CTX_FONT_sgi
  ctx_load_font_monobitmap ("bitmap", ' ', '~', 8, 13, &sgi_font[0][0]);
#endif
#if DEJAVU_SANS_MONO
  ctx_load_font_ttf ("mono-DejaVuSansMono", ttf_DejaVuSansMono_ttf, ttf_DejaVuSansMono_ttf_len);
#endif
#if NOTO_EMOJI_REGULAR
  ctx_load_font_ttf ("sans-NotoEmoji_Regular", ttf_NotoEmoji_Regular_ttf, ttf_NotoEmoji_Regular_ttf_len);
#endif
#if ROBOTO_LIGHT
  ctx_load_font_ttf ("sans-Roboto_Light", ttf_Roboto_Light_ttf, ttf_Roboto_Light_ttf_len);
#endif
#if ROBOTO_REGULAR
  ctx_load_font_ttf ("sans-Roboto_Regular", ttf_Roboto_Regular_ttf, ttf_Roboto_Regular_ttf_len);
#endif
#if ROBOTO_BOLD
  ctx_load_font_ttf ("sans-Roboto_Bold", ttf_Roboto_Bold_ttf, ttf_Roboto_Bold_ttf_len);
#endif
#if DEJAVU_SANS
  ctx_load_font_ttf ("sans-DejaVuSans", ttf_DejaVuSans_ttf, ttf_DejaVuSans_ttf_len);
#endif
#if VERA
  ctx_load_font_ttf ("sans-Vera", ttf_Vera_ttf, ttf_Vera_ttf_len);
#endif
#if UNSCII_16
  ctx_load_font_ttf ("mono-unscii16", ttf_unscii_16_ttf, ttf_unscii_16_ttf_len);
#endif

#if XA000_MONO
  ctx_load_font_ttf ("mono-0xA000", ttf_0xA000_Mono_ttf, ttf_0xA000_Mono_ttf_len);
#endif
}

#if CTX_CAIRO

void
ctx_render_cairo (Ctx *ctx, cairo_t *cr)
{
  CtxIterator iterator;
  CtxEntry   *entry;

  cairo_pattern_t *pat = NULL;
  cairo_surface_t *image = NULL;

  ctx_iterator_init (&iterator, &ctx->renderstream, 0, CTX_ITERATOR_EXPAND_REFPACK|
                                                  CTX_ITERATOR_EXPAND_BITPACK);

  while ((entry = ctx_iterator_next (&iterator)))
  {
    //if (cairo_status (cr)) return;
    switch (entry->code)
    //switch ((CtxCode)(entry->code))
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

      case CTX_SET_LINE_WIDTH:
        cairo_set_line_width (cr, ctx_arg_float(0));
        break;

      case CTX_ARC:
        if (ctx_arg_float(5) == 0)
          cairo_arc (cr, ctx_arg_float(0), ctx_arg_float(1),
                         ctx_arg_float(2), ctx_arg_float(3),
                         ctx_arg_float(4));
        else
          cairo_arc_negative (cr, ctx_arg_float(0), ctx_arg_float(1),
                              ctx_arg_float(2), ctx_arg_float(3),
                              ctx_arg_float(4));
        break;


      case CTX_SET_RGBA_U8:
        cairo_set_source_rgba (cr, ctx_arg_u8(0)/255.0,
                                   ctx_arg_u8(1)/255.0,
                                   ctx_arg_u8(2)/255.0,
                                   ctx_arg_u8(3)/255.0);
        break;

#if 0
      case CTX_SET_RGBA_STROKE: // XXX : we need to maintain
                                //       state for the two kinds
        cairo_set_source_rgba (cr, ctx_arg_u8(0)/255.0,
                                   ctx_arg_u8(1)/255.0,
                                   ctx_arg_u8(2)/255.0,
                                   ctx_arg_u8(3)/255.0);
        break;
#endif

      case CTX_RECTANGLE:
        cairo_rectangle (cr,ctx_arg_float(0),
                            ctx_arg_float(1),
                            ctx_arg_float(2),
                            ctx_arg_float(3));
        break;
      case CTX_SET_PIXEL:
        cairo_set_source_rgba (cr,
                      ctx_arg_u8(0)/255.0f,
                      ctx_arg_u8(1)/255.0f,
                      ctx_arg_u8(2)/255.0f,
                      ctx_arg_u8(3)/255.0f);
        cairo_rectangle (cr, ctx_arg_u16(2), ctx_arg_u16(3), 1, 1);
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

      case CTX_SET_FONT_SIZE:
        cairo_set_font_size (cr, ctx_arg_float(0));
        break;
      case CTX_SET_MITER_LIMIT:
        cairo_set_miter_limit (cr, ctx_arg_float(0));
        break;

      case CTX_SET_LINE_CAP:
        {
          int cairo_val = CAIRO_LINE_CAP_SQUARE;
          switch (ctx_arg_u8(0))
          {
            case CTX_CAP_ROUND:  cairo_val = CAIRO_LINE_CAP_ROUND;  break;
            case CTX_CAP_SQUARE: cairo_val = CAIRO_LINE_CAP_SQUARE; break;
            case CTX_CAP_NONE:   cairo_val = CAIRO_LINE_CAP_BUTT;   break;
          }
          cairo_set_line_cap (cr, cairo_val);
        }
        break;

      case CTX_SET_COMPOSITING_MODE:
        {
          int cairo_val = CAIRO_OPERATOR_OVER;
          switch (ctx_arg_u8(0))
          {
            case CTX_COMPOSITE_SOURCE_OVER: cairo_val = CAIRO_OPERATOR_OVER; break;
            case CTX_COMPOSITE_SOURCE_COPY: cairo_val = CAIRO_OPERATOR_SOURCE; break;
          }
          cairo_set_operator (cr, cairo_val);
        }

      case CTX_SET_LINE_JOIN:
        {
          int cairo_val = CAIRO_LINE_JOIN_ROUND;
          switch (ctx_arg_u8(0))
          {
            case CTX_JOIN_ROUND: cairo_val = CAIRO_LINE_JOIN_ROUND; break;
            case CTX_JOIN_BEVEL: cairo_val = CAIRO_LINE_JOIN_BEVEL; break;
            case CTX_JOIN_MITER: cairo_val = CAIRO_LINE_JOIN_MITER; break;
          }
          cairo_set_line_join (cr, cairo_val);
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
          cairo_set_source (cr, pat);
        }
        break;

      case CTX_GRADIENT_STOP:
        cairo_pattern_add_color_stop_rgba (pat,
          ctx_arg_float(0),
          ctx_arg_u8(4)/255.0,
          ctx_arg_u8(5)/255.0,
          ctx_arg_u8(6)/255.0,
          ctx_arg_u8(7)/255.0);

        break;
        // XXX  implement TEXTURE
#if 0
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
          image = cairo_image_surface_create_from_png (ctx_arg_string());
          cairo_set_source_surface (cr, image, ctx_arg_float(0), ctx_arg_float(1));
        }
        break;
#endif
      case CTX_TEXT:
        /* XXX: implement some linebreaking/wrap behavior here */
        cairo_show_text (cr, ctx_arg_string ());
        break;

      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
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

#if CTX_RENDER_CTX 

void
ctx_render_ctx (Ctx *ctx, Ctx *d_ctx)
{
  CtxIterator iterator;
  CtxEntry   *entry;

  ctx_iterator_init (&iterator, &ctx->renderstream, 0, CTX_ITERATOR_EXPAND_REFPACK|
                     CTX_ITERATOR_EXPAND_BITPACK);

  while ((entry = ctx_iterator_next (&iterator)))
  {
    //switch ((CtxCode)(entry->code))
    switch (entry->code)
    {
      case CTX_ARC_TO:
        ctx_arc_to (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                           ctx_arg_float(1), ctx_arg_float(2),
                           ctx_arg_float(3));
        break;
      case CTX_REL_ARC_TO:
        ctx_rel_arc_to (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                               ctx_arg_float(1), ctx_arg_float(2),
                               ctx_arg_float(3));
        break;
      case CTX_LINE_TO:
        ctx_line_to (d_ctx, ctx_arg_float(0), ctx_arg_float(1));
        break;
      case CTX_REL_LINE_TO:
        ctx_rel_line_to (d_ctx, ctx_arg_float(0), ctx_arg_float(1));
        break;
      case CTX_MOVE_TO:
        ctx_move_to (d_ctx, ctx_arg_float(0), ctx_arg_float(1));
        break;
      case CTX_REL_MOVE_TO:
        ctx_rel_move_to (d_ctx, ctx_arg_float(0), ctx_arg_float(1));
        break;
      case CTX_CURVE_TO:
        ctx_curve_to (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                             ctx_arg_float(2), ctx_arg_float(3),
                             ctx_arg_float(4), ctx_arg_float(5));
        break;
      case CTX_REL_CURVE_TO:
        ctx_rel_curve_to (d_ctx,ctx_arg_float(0), ctx_arg_float(1),
                                ctx_arg_float(2), ctx_arg_float(3),
                                ctx_arg_float(4), ctx_arg_float(5));
        break;
      case CTX_QUAD_TO:
        {
          float cx = ctx_arg_float(0);
          float cy = ctx_arg_float(1);
          float  x = ctx_arg_float(2);
          float  y = ctx_arg_float(3);
          ctx_quad_to (d_ctx, cx, cy, x, y);
        }
        break;

      case CTX_REL_QUAD_TO:
        {
          float cx = ctx_arg_float(0);
          float cy = ctx_arg_float(1);
          float  x = ctx_arg_float(2);
          float  y = ctx_arg_float(3);
          ctx_rel_quad_to (d_ctx, cx, cy, x, y);
        }
        break;

      case CTX_ROTATE:
        ctx_rotate (d_ctx, ctx_arg_float(0));
        break;
      case CTX_SET_GLOBAL_ALPHA:
        ctx_set_global_alpha (d_ctx, ctx_arg_float(0));
        break;

      case CTX_SCALE:
        ctx_scale (d_ctx, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_TRANSLATE:
        ctx_translate (d_ctx, ctx_arg_float(0), ctx_arg_float(1));
        break;

      case CTX_SET_LINE_WIDTH:
        ctx_set_line_width (d_ctx, ctx_arg_float(0));
        break;

      case CTX_ARC:
        ctx_arc (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                        ctx_arg_float(2), ctx_arg_float(3),
                        ctx_arg_float(4), ctx_arg_float(5));
        break;

      case CTX_SET_RGBA_U8:
        ctx_set_rgba (d_ctx, ctx_arg_u8(0)/255.0,
                             ctx_arg_u8(1)/255.0,
                             ctx_arg_u8(2)/255.0,
                             ctx_arg_u8(3)/255.0);
        break;


      case CTX_SET_COLOR:
        switch ((int)ctx_arg_float(0))
        {
          case CTX_RGBA:
           ctx_set_rgba (d_ctx, ctx_arg_float(2),
                                ctx_arg_float(3),
                                ctx_arg_float(4),
                                ctx_arg_float(5));
           break;
          case CTX_GRAY:
           ctx_set_gray (d_ctx, ctx_arg_float(2));
           break;
          case CTX_GRAYA:
           ctx_set_rgba (d_ctx, ctx_arg_float(2),
                                ctx_arg_float(2),
                                ctx_arg_float(2),
                                ctx_arg_float(3));
           break;
          case CTX_RGB:
           ctx_set_rgb (d_ctx, ctx_arg_float(2),
                               ctx_arg_float(3),
                               ctx_arg_float(4));
           break;
#if CTX_ENABLE_CMYK
          case CTX_CMYKA:
           ctx_set_cmyka (d_ctx, ctx_arg_float(2),
                                 ctx_arg_float(3),
                                 ctx_arg_float(4),
                                 ctx_arg_float(5),
                                 ctx_arg_float(1));
           break;
          case CTX_CMYK:
           ctx_set_cmyka (d_ctx, ctx_arg_float(2),
                                 ctx_arg_float(3),
                                 ctx_arg_float(4),
                                 ctx_arg_float(5),
                                 1.0f);
           break;
#endif
        }
        break;

#if 0
      case CTX_SET_RGBA_STROKE:
        ctx_set_rgba_stroke (d_ctx, ctx_arg_u8(0)/255.0,
                                    ctx_arg_u8(1)/255.0,
                                    ctx_arg_u8(2)/255.0,
                                    ctx_arg_u8(3)/255.0);
        break;
#endif

      case CTX_SET_PIXEL:
         ctx_set_pixel_u8 (d_ctx,
                      ctx_arg_u16(2), ctx_arg_u16(3),
                      ctx_arg_u8(0),ctx_arg_u8(1),ctx_arg_u8(2),ctx_arg_u8(3));
      break;

      case CTX_RECTANGLE:
        ctx_rectangle (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                              ctx_arg_float(2), ctx_arg_float(3));
        break;

      case CTX_FILL:    ctx_fill (d_ctx);   break;
      case CTX_STROKE:  ctx_stroke (d_ctx); break;
      case CTX_CLEAR:   ctx_clear (d_ctx);  break;
      case CTX_NEW_PAGE:
        //ctx_new_page (d_ctx);
        break;
      case CTX_SET_TRANSFORM:
        ctx_set_transform (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                           ctx_arg_float(2), ctx_arg_float(3),
                           ctx_arg_float(4), ctx_arg_float(5));
        break;

      case CTX_IDENTITY:      ctx_identity_matrix (d_ctx);                break;
      case CTX_CLIP:          ctx_clip (d_ctx);                           break;                  
      case CTX_NEW_PATH:      ctx_new_path (d_ctx);                       break;
      case CTX_CLOSE_PATH:    ctx_close_path (d_ctx);                     break;
      case CTX_SAVE:          ctx_save (d_ctx);                           break;
      case CTX_RESTORE:       ctx_restore (d_ctx);                        break;
      case CTX_SET_FONT_SIZE: ctx_set_font_size (d_ctx, ctx_arg_float(0));break;
      case CTX_SET_MITER_LIMIT: ctx_set_miter_limit (d_ctx, ctx_arg_float(0));break;
      case CTX_SET_FILL_RULE:
        ctx_set_fill_rule (d_ctx, (CtxFillRule)ctx_arg_u8(0));            break;
      case CTX_SET_LINE_CAP:
        ctx_set_line_cap (d_ctx, (CtxLineCap)ctx_arg_u8(0));              break;
      case CTX_SET_LINE_JOIN:
        ctx_set_line_join (d_ctx, (CtxLineJoin)ctx_arg_u8(0));            break;
      case CTX_SET_COMPOSITING_MODE:
        ctx_set_compositing_mode (d_ctx, (CtxCompositingMode)ctx_arg_u8(0));
        break;
      case CTX_SET_TEXT_ALIGN:
        ctx_set_text_align (d_ctx, (CtxTextAlign)ctx_arg_u8(0)); break;
      case CTX_SET_TEXT_BASELINE:
        ctx_set_text_baseline (d_ctx, (CtxTextBaseline)ctx_arg_u8(0)); break;
      case CTX_SET_TEXT_DIRECTION:
        ctx_set_text_direction (d_ctx, (CtxTextDirection)ctx_arg_u8(0)); break;
      case CTX_LINEAR_GRADIENT:
        ctx_linear_gradient (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                                    ctx_arg_float(2), ctx_arg_float(3));
        ctx_gradient_clear_stops (d_ctx);
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_radial_gradient (d_ctx, ctx_arg_float(0), ctx_arg_float(1),
                                    ctx_arg_float(2), ctx_arg_float(3),
                                    ctx_arg_float(4), ctx_arg_float(5));
        ctx_gradient_clear_stops (d_ctx);
        break;

      case CTX_GRADIENT_STOP:
        ctx_gradient_add_stop (d_ctx,
          ctx_arg_float(0),
          ctx_arg_u8(4)/255.0,
          ctx_arg_u8(5)/255.0,
          ctx_arg_u8(6)/255.0,
          ctx_arg_u8(7)/255.0);
        break;

      case CTX_SET_FONT:    ctx_set_font (d_ctx, ctx_arg_string ());    break;
      case CTX_TEXT:        ctx_text (d_ctx, ctx_arg_string ());        break;
      case CTX_TEXT_STROKE: ctx_text_stroke (d_ctx, ctx_arg_string ()); break;
      case CTX_GLYPH:       ctx_glyph (d_ctx, ctx_arg_u32(0), ctx_arg_u8(4));
         break;
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
      case CTX_REPEAT_HISTORY:
        break;
    }
  }
}
#endif


#if CTX_FORMATTER

typedef enum CtxFormatter {
  CTX_FORMATTER_COMPACT=0,
  CTX_FORMATTER_VERBOSE
} CtxFormatter;

static void _ctx_print_endcmd (FILE *stream, int formatter)
{
  if (formatter)
  {
    fwrite (");\n", 3, 1, stream);
  }
}

static void _ctx_indent (FILE *stream, int level)
{
    for (int i = 0; i < level; i++)
      fwrite ("  ", 1, 2, stream);
}

static void _ctx_print_name (FILE *stream, int code, int formatter, int *indent)
{
#define CTX_VERBOSE_NAMES 1
#if CTX_VERBOSE_NAMES
  if (formatter)
  {
    const char *name = NULL;
    _ctx_indent (stream, *indent);
    //switch ((CtxCode)code)
    switch (code)
    {
      case CTX_SET_PARAM:            name="set_param";break;
      case CTX_SET_COLOR:            name="set_color";break;
      case CTX_DEFINE_GLYPH:         name="define_glyph";break;
      case CTX_SET_PIXEL:            name="set_pixel";break;
      case CTX_SET_GLOBAL_ALPHA:     name="set_global_alpha";break;
      case CTX_TEXT:                 name="text";break;
      case CTX_TEXT_STROKE:          name="text_stroke";break;
      case CTX_SAVE:                 name="save";break;
      case CTX_RESTORE:              name="restore";break;
      case CTX_RECTANGLE:            name="rectangle";break;
      case CTX_LINEAR_GRADIENT:      name="linear_gradient";break;
      case CTX_RADIAL_GRADIENT:      name="radial_gradient";break;
      case CTX_GRADIENT_STOP:        name="gradient_add_stop";break;
      case CTX_MEDIA_BOX:            name="media_box";break;
      case CTX_MOVE_TO:              name="move_to";break;
      case CTX_LINE_TO:              name="line_to";break;
      case CTX_NEW_PATH:             name="new_path";break;
      case CTX_REL_MOVE_TO:          name="rel_move_to";break;
      case CTX_REL_LINE_TO:          name="rel_line_to";break;
      case CTX_FILL:                 name="fill";break;
      case CTX_EXIT:                 name="exit";break;
      case CTX_SET_TRANSFORM:        name="set_transform";break;
      case CTX_REL_ARC_TO:           name="rel_arc_to";break;
      case CTX_GLYPH:                name="glyph";break;
      case CTX_TEXTURE:              name="texture";break;
      case CTX_IDENTITY:             name="identity";break;
      case CTX_CLOSE_PATH:           name="close_path";break;
      case CTX_FLUSH:                name="flush";break;
      case CTX_CLEAR:                name="clear";break;
      case CTX_SET_FONT:             name="set_font";break;
      case CTX_STROKE:               name="stroke";break;
      case CTX_CLIP:                 name="clip";break;
      case CTX_ARC:                  name="arc";break;
      case CTX_SCALE:                name="scale";break;
      case CTX_TRANSLATE:            name="translate";break;
      case CTX_ROTATE:               name="rotate";break;
      case CTX_ARC_TO:               name="arc_to";break;
      case CTX_CURVE_TO:             name="curve_to";break;
      case CTX_REL_CURVE_TO:         name="rel_curve_to";break;
      case CTX_REL_QUAD_TO:          name="rel_quad_to";break;
      case CTX_QUAD_TO:              name="quad_to";break;
      case CTX_SMOOTH_TO:            name="smooth_to";break;
      case CTX_REL_SMOOTH_TO:        name="rel_smooth_to";break;
      case CTX_SMOOTHQ_TO:           name="smoothq_to";break;
      case CTX_REL_SMOOTHQ_TO:       name="rel_smoothq_to";break;
      case CTX_NEW_PAGE:             name="new_page";break;
      case CTX_HOR_LINE_TO:          name="hor_line_to";break;
      case CTX_VER_LINE_TO:          name="ver_line_to";break;
      case CTX_REL_HOR_LINE_TO:      name="rel_hor_line_to";break;
      case CTX_REL_VER_LINE_TO:      name="rel_ver_line_to";break;
      case CTX_SET_COMPOSITING_MODE: name="set_compositing_mode";break;
      case CTX_SET_TEXT_ALIGN:       name="set_text_align";break;
      case CTX_SET_TEXT_BASELINE:    name="set_text_baseline";break;
      case CTX_SET_TEXT_DIRECTION:   name="set_text_direction";break;
      case CTX_SET_FONT_SIZE:        name="set_font_size";break;
      case CTX_SET_MITER_LIMIT:      name="set_miter_limit";break;
      case CTX_SET_LINE_JOIN:        name="set_line_join";break;
      case CTX_SET_LINE_CAP:         name="set_line_cap";break;
      case CTX_SET_LINE_WIDTH:       name="set_line_width";break;
      case CTX_SET_FILL_RULE:        name="set_fill_rule";break;
    }
    if (name)
    {
      fwrite (name, 1, strlen((char*)name), stream);
      fwrite (" (", 1, 2, stream);

      if (code == CTX_SAVE)
        (*indent)++;
      else if (code == CTX_RESTORE)
        (*indent)--;
      return;
    }
  }

#endif
  {
  uint8_t name[3];
  name[0]=CTX_SET_PARAM;
  name[2]='\0';
  switch (code)
  {
    case CTX_SET_GLOBAL_ALPHA:     name[1]='a';break;
    case CTX_SET_COMPOSITING_MODE: name[1]='m';break;
    case CTX_SET_TEXT_ALIGN:       name[1]='t';break;
    case CTX_SET_TEXT_BASELINE:    name[1]='b';break;
    case CTX_SET_TEXT_DIRECTION:   name[1]='d';break;
    case CTX_SET_FONT_SIZE:        name[1]='f';break;
    case CTX_SET_MITER_LIMIT:      name[1]='l';break;
    case CTX_SET_LINE_JOIN:        name[1]='j';break;
    case CTX_SET_LINE_CAP:         name[1]='c';break;
    case CTX_SET_LINE_WIDTH:       name[1]='w';break;
    case CTX_SET_FILL_RULE:        name[1]='r';break;
    default:
     name[0] = code; name[1] = 0;
    break;
  }
  fwrite (name, 1, strlen((char*)name), stream);
   if (formatter)
    fwrite (" (", 1, 2, stream);
  }

}

static void
ctx_print_entry_enum (FILE *stream, int formatter, int *indent, CtxEntry *entry, int args)
{
  _ctx_print_name (stream, entry->code, formatter, indent);


  for (int i = 0; i <  args; i ++)
  {
    int val = ctx_arg_u8(i);
    if (i>0)
      fwrite (" ", 1, 1, stream);
#if CTX_VERBOSE_NAMES
    if (formatter)
    {
      const char *str = NULL;
      switch (entry->code)
      {
        case CTX_SET_TEXT_BASELINE:
          switch (val)
          {
            case CTX_TEXT_BASELINE_ALPHABETIC:  str = "alphabetic";   break;
            case CTX_TEXT_BASELINE_TOP:         str = "top";  break;
            case CTX_TEXT_BASELINE_BOTTOM:      str = "bottom";  break;
            case CTX_TEXT_BASELINE_HANGING:     str = "hanging";    break;
            case CTX_TEXT_BASELINE_MIDDLE:      str = "middle"; break;
            case CTX_TEXT_BASELINE_IDEOGRAPHIC: str = "ideographic"; break;
          }
          break;
        case CTX_SET_TEXT_ALIGN:
          switch (val)
          {
            case CTX_TEXT_ALIGN_LEFT:   str = "left";   break;
            case CTX_TEXT_ALIGN_RIGHT:  str = "right";  break;
            case CTX_TEXT_ALIGN_START:  str = "start";  break;
            case CTX_TEXT_ALIGN_END:    str = "end";    break;
            case CTX_TEXT_ALIGN_CENTER: str = "center"; break;
          }
          break;
        case CTX_SET_LINE_CAP:
          switch (val)
          {
            case CTX_CAP_NONE:     str = "none";   break;
            case CTX_CAP_ROUND:    str = "round";  break;
            case CTX_CAP_SQUARE:   str = "square"; break;
          }
          break;
        case CTX_SET_LINE_JOIN:
          switch (val)
          {
            case CTX_JOIN_MITER:    str = "miter"; break;
            case CTX_JOIN_ROUND:    str = "round"; break;
            case CTX_JOIN_BEVEL:    str = "bevel"; break;
          }
          break;
        case CTX_SET_FILL_RULE:
          switch (val)
          {
            case CTX_FILL_RULE_WINDING: str = "winding"; break;
            case CTX_FILL_RULE_EVEN_ODD: str = "evenodd"; break;
          }
          break;
      }
      if (str)
        fprintf (stream, "%s", str);
      else
        fprintf (stream, "%i", val);
    }
    else
#endif
    {
      fprintf (stream, "%i", val);
    }
  }
  _ctx_print_endcmd (stream, formatter);
}

static void
ctx_print_escaped_string (FILE *stream, const char *string)
{
  if (!string | !stream) return;
  for (int i = 0; string[i]; i++)
  {
    switch (string[i])
    {
      case '"':
        fwrite ("\\\"", 1, 2, stream);
        break;
      case '\\':
        fwrite ("\\\\", 1, 2, stream);
        break;
      case '\n':
        fwrite ("\\n", 1, 2, stream);
        break;
      default:
        fwrite (&string[i], 1, 1, stream);
    }
  }
}

static void
ctx_print_float (FILE *stream, float val)
{
    char temp[128];
    sprintf (temp, "%0.5f", val);
    int j;
    for (j = 0; temp[j]; j++)
      if (j == ',') temp[j] = '.';
    j--;
    if (j>0)
    while (temp[j] == '0')
    {
      temp[j]=0;j--;
    }
    if (temp[j]=='.')
      temp[j]='\0';
    fwrite (temp, 1, strlen(temp), stream);
}

static void
ctx_print_entry (FILE *stream, int formatter, int *indent, CtxEntry *entry, int args)
{
  _ctx_print_name (stream, entry->code, formatter, indent);

  for (int i = 0; i <  args; i ++)
  {
    float val = ctx_arg_float (i);
    if (i>0 && val >= 0.0f)
    {
      switch (formatter)
      {
        case CTX_FORMATTER_VERBOSE:
          fwrite (", ", 2, 1, stream);
          break;
        case CTX_FORMATTER_COMPACT:
          if (val >= 0.0f)
            fwrite (" ", 1, 1, stream);
          break;
        default:
          fwrite (" ", 1, 1, stream);
          break;
      }
    }
    ctx_print_float (stream, val);
  }
  _ctx_print_endcmd (stream, formatter);
}

void
ctx_render_stream (Ctx *ctx, FILE *stream, int formatter)
{
  int indent = 0;
  CtxIterator iterator;
  CtxEntry   *entry;

  ctx_iterator_init (&iterator, &ctx->renderstream, 0,
                     CTX_ITERATOR_EXPAND_REFPACK|
                     CTX_ITERATOR_EXPAND_BITPACK);

  while ((entry = ctx_iterator_next (&iterator)))
  {
    switch (entry->code)
    //switch ((CtxCode)(entry->code))
    {
      case CTX_LINE_TO:
      case CTX_REL_LINE_TO:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_MOVE_TO:
      case CTX_REL_MOVE_TO:
        ctx_print_entry (stream, formatter, &indent, entry, 2);
        break;

      case CTX_ARC_TO:
        ctx_print_entry (stream, formatter, &indent, entry, 5);
        break;

      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_RADIAL_GRADIENT:
      case CTX_SET_TRANSFORM:
        ctx_print_entry (stream, formatter, &indent, entry, 6);
        break;

      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_REL_QUAD_TO:
      case CTX_LINEAR_GRADIENT:
      case CTX_MEDIA_BOX:
        ctx_print_entry (stream, formatter, &indent, entry, 4);
        break;

      case CTX_SET_FONT_SIZE:
      case CTX_SET_MITER_LIMIT:
      case CTX_ROTATE:
      case CTX_SET_LINE_WIDTH:
      case CTX_VER_LINE_TO:
      case CTX_HOR_LINE_TO:
        ctx_print_entry (stream, formatter, &indent, entry, 1);
        break;

      case CTX_SET_RGBA_U8:
        if (formatter)
        {
          _ctx_indent (stream, indent);
          fwrite ("rgba (", 6, 1, stream);
        }
        else
        {
          fwrite ("rgba (", 5, 1, stream);
        }
        for (int c = 0; c < 4; c++)
        {
          if (c)
          {
            if (formatter == CTX_FORMATTER_VERBOSE)
              fwrite (", ", 2, 1, stream);
            else
              fwrite (" ", 1, 1, stream);
          }
          ctx_print_float (stream, ctx_arg_u8(c)/255.0);
        }
        _ctx_print_endcmd (stream, formatter);
        break;

#if 0
      case CTX_SET_RGBA_STROKE:
        ctx_set_rgba_stroke (d_ctx, ctx_arg_u8(0)/255.0,
                                    ctx_arg_u8(1)/255.0,
                                    ctx_arg_u8(2)/255.0,
                                    ctx_arg_u8(3)/255.0);
        break;
#endif

      case CTX_SET_PIXEL:
#if 0
         ctx_set_pixel_u8 (d_ctx,
                      ctx_arg_u16(2), ctx_arg_u16(3),
                      ctx_arg_u8(0),
                      ctx_arg_u8(1),
                      ctx_arg_u8(2),
                      ctx_arg_u8(3));
#endif
      break;


      case CTX_FILL:
      case CTX_CLEAR:
      case CTX_STROKE:
      case CTX_IDENTITY:
      case CTX_CLIP:
      case CTX_NEW_PATH:
      case CTX_CLOSE_PATH:
      case CTX_SAVE:
      case CTX_RESTORE:
        ctx_print_entry (stream, formatter, &indent, entry, 0);
        break;

      case CTX_SET_TEXT_ALIGN:
      case CTX_SET_TEXT_BASELINE:
      case CTX_SET_TEXT_DIRECTION:
      case CTX_SET_FILL_RULE:
      case CTX_SET_LINE_CAP:
      case CTX_SET_LINE_JOIN:
      case CTX_SET_COMPOSITING_MODE:
        ctx_print_entry_enum (stream, formatter, &indent, entry, 1);
        break;

      case CTX_GRADIENT_STOP:
        _ctx_print_name (stream, entry->code, formatter, &indent);
        for (int c = 0; c < 4; c++)
        {
          if (c)
            fwrite ("  ", 1, 1, stream);
          ctx_print_float (stream, ctx_arg_u8(4+c)/255.0);
        }
        _ctx_print_endcmd (stream, formatter);
        break;

      case CTX_TEXT:
      case CTX_TEXT_STROKE:
      case CTX_SET_FONT:

        _ctx_print_name (stream, entry->code, formatter, &indent);
        fprintf (stream, "\"");
        ctx_print_escaped_string (stream, ctx_arg_string());
        fprintf (stream, "\"");
        _ctx_print_endcmd (stream, formatter);
        break;

      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
      case CTX_REPEAT_HISTORY:
        break;
    }
  }
  fprintf (stream, "\n");
}

#endif

/* the parser comes in the end, nothing in ctx knows about the parser  */

#if CTX_PARSER

/* ctx parser, */

struct _CtxParser {
  Ctx       *ctx;
  int        state;
  uint8_t    holding[1024];
  int        line; /*  for error reporting */
  int        col;  /*  for error reporting */
  int        pos;
  float      numbers[12]; /* used by svg parser */
  int        n_numbers;
  int        decimal;
  char       command;
  int        n_args;
  float      pcx;
  float      pcy;
  int        color_components;
  int        color_model; // 1 gray 3 rgb 4 cmyk
  float      left_margin; // set by last user provided move_to
                          // before text, used by newlines

  int        width;   // <- maybe should be float
  int        height;
  float      cell_width;
  float      cell_height;
  int        cursor_x; // <- leaking in from terminal
  int        cursor_y;

  void (*exit)(void *exit_data);
  void *exit_data;
};

static CtxParser *
ctx_parser_init (CtxParser *parser,
  Ctx       *ctx,
  int        width,
  int        height,
  float      cell_width,
  float      cell_height,
  int        cursor_x,
  int        cursor_y,
  void (*exit)(void *exit_data),
  void *exit_data
          )
{
  ctx_memset (parser, 0, sizeof (CtxParser));
  parser->line             = 1;
  parser->ctx              = ctx;
  parser->cell_width       = cell_width;
  parser->cell_height      = cell_height;
  parser->cursor_x         = cursor_x;
  parser->cursor_y         = cursor_y;
  parser->width            = width;
  parser->height           = height;
  parser->exit             = exit;
  parser->exit_data        = exit_data;
  parser->color_model      = CTX_RGBA;
  parser->color_components = 4;
  parser->command          = 'm';
  return parser;
}

CtxParser *ctx_parser_new (
  Ctx       *ctx,
  int        width,
  int        height,
  float      cell_width,
  float      cell_height,
  int        cursor_x,
  int        cursor_y,
  void (*exit)(void *exit_data),
  void *exit_data)
{
  return ctx_parser_init (malloc (sizeof(CtxParser)),
             ctx,
             width, height,
             cell_width, cell_height,
             cursor_x, cursor_y,
             exit, exit_data);
}
void ctx_parser_free (CtxParser *parser)
{
  free (parser);
}

static int ctx_arguments_for_code (CtxCode code)
{
  switch (code)
  {
  case CTX_SAVE:
  case CTX_IDENTITY:
  case CTX_CLOSE_PATH:
  case CTX_NEW_PATH:
  case CTX_CLEAR:
  case CTX_FLUSH:
  case CTX_RESTORE:
  case CTX_STROKE:
  case CTX_FILL:
  case CTX_NEW_PAGE:
  case CTX_CLIP:
  case CTX_EXIT:
    return 0;
  case CTX_SET_GLOBAL_ALPHA:
  case CTX_SET_COMPOSITING_MODE:
  case CTX_SET_FONT_SIZE:
  case CTX_SET_LINE_JOIN:
  case CTX_SET_LINE_CAP:
  case CTX_SET_LINE_WIDTH:
  case CTX_SET_FILL_RULE:
  case CTX_SET_TEXT_ALIGN:
  case CTX_SET_TEXT_BASELINE:
  case CTX_SET_TEXT_DIRECTION:
  case CTX_SET_MITER_LIMIT:
  case CTX_REL_VER_LINE_TO:
  case CTX_REL_HOR_LINE_TO:
  case CTX_HOR_LINE_TO:
  case CTX_VER_LINE_TO:
  case CTX_SET_FONT:
  case CTX_ROTATE:
    return 1;
  case CTX_TRANSLATE:
  case CTX_REL_SMOOTHQ_TO:
  case CTX_LINE_TO:
  case CTX_MOVE_TO:
  case CTX_SCALE:
  case CTX_REL_LINE_TO:
  case CTX_REL_MOVE_TO:
  case CTX_SMOOTHQ_TO:
  case CTX_GLYPH: // glyph and is_stroke
    return 2;
  case CTX_LINEAR_GRADIENT:
  case CTX_REL_QUAD_TO:
  case CTX_QUAD_TO:
  case CTX_RECTANGLE:
  case CTX_REL_SMOOTH_TO:
  case CTX_MEDIA_BOX:
  case CTX_SMOOTH_TO:
    return 4;
  case CTX_ARC_TO:
  case CTX_REL_ARC_TO:
    return 5;
  case CTX_ARC:
  case CTX_CURVE_TO:
  case CTX_REL_CURVE_TO:
  case CTX_SET_TRANSFORM:
  case CTX_RADIAL_GRADIENT:
    return 6;
  case CTX_TEXT_STROKE:
  case CTX_TEXT: // special case
    return 100; /* 100 is a special value,
                   which means string|number accepted
                 */

  //case CTX_SET_PARAM:
  case CTX_SET_COLOR:
    return 200;  /* 200 means number of components */
  case CTX_GRADIENT_STOP:
    return 201;  /* 201 means number of components+1 */

  case CTX_DEFUN: /* special interpretation   */
    return 300;
    default:
#if 0
  case CTX_TEXTURE:
  CTX_SET_RGBA_U8
  CTX_GRADIENT_CLEAR
  CTX_BITPIX
  CTX_BITPIX_DATA
  CTX_NOP
  CTX_NEW_EDGE
  CTX_EDGE
  CTX_EDGE_FLIPPED
  CTX_REPEAT_HISTORY
  CTX_CONT
  CTX_DATA
  CTX_DATA_REV
  CTX_DEFINE_GLYPH
  CTX_KERNING_PAIR
  CTX_SET_PIXEL
  CTX_REL_LINE_TO_X4
  CTX_REL_LINE_TO_REL_CURVE_TO
  CTX_REL_CURVE_TO_REL_LINE_TO
  CTX_REL_CURVE_TO_REL_MOVE_TO
  CTX_REL_LINE_TO_X2
  CTX_MOVE_TO_REL_LINE_TO
  CTX_REL_LINE_TO_REL_MOVE_TO
  CTX_FILL_MOVE_TO
  CTX_REL_QUAD_TO_REL_QUAD_TO
  CTX_REL_QUAD_TO_S16
#endif
    return 0;
  }
}

static int ctx_parser_set_command (CtxParser *parser, CtxCode code)
{
  parser->n_args = ctx_arguments_for_code (code);
  if (parser->n_args >= 200)
  {
    parser->n_args = (parser->n_args % 100) + parser->color_components;
  }
#if 0
  else if (parser->n_args >= 100)
  {
    parser->n_args = (parser->n_args % 100);
  }
#endif
  return code;
}

static void ctx_parser_set_color_model (CtxParser *parser, int color_model);

static int ctx_parser_resolve_command (CtxParser *parser, const uint8_t*str)
{
  uint32_t ret = str[0];

  if (str[0] && str[1])
  {
    uint32_t str_hash = 0;
  #define LOWER(a) ( (((a)>='A')&&((a)<='Z') )?(a)+32:(a))  // this causes collisions, not sure if we want
  //#define LOWER(a) (a)
    /* trim ctx_ and CTX_ prefix */
    if ((str[0] == 'c' && str[1] == 't' && str[2] == 'x' && str[3] == '_')||
        (str[0] == 'C' && str[1] == 'T' && str[2] == 'X' && str[3] == '_'))
    {
      str += 4;
    }
    if ((str[0] == 's' && str[1] == 'e' && str[2] == 't' && str[3] == '_'))
      str += 4;

    /* hash string to number */
    {
      int multiplier = 1;
      for (int i = 0; str[i] && i < 12; i++)
      {
        str_hash = str_hash + LOWER(str[i]) * multiplier;
        multiplier *= 11;
      }
    }

/* And let the compiler determine numbers for the parser
 * directly, if the hash causes collisions in the vocabulary
 * we can tweak the hash until it doesn't, for now multiplying
 * by 11 works.
 */
#define STR(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) (\
          (((uint32_t)LOWER(a0))+ \
          (((uint32_t)LOWER(a1))*11)+ \
          (((uint32_t)LOWER(a2))*11*11)+ \
          (((uint32_t)LOWER(a3))*11*11*11)+ \
          (((uint32_t)LOWER(a4))*11*11*11*11) + \
          (((uint32_t)LOWER(a5))*11*11*11*11*11) + \
          (((uint32_t)LOWER(a6))*11*11*11*11*11*11) + \
          (((uint32_t)LOWER(a7))*11*11*11*11*11*11*11) + \
          (((uint32_t)LOWER(a8))*11*11*11*11*11*11*11*11) + \
          (((uint32_t)LOWER(a9))*11*11*11*11*11*11*11*11*11) + \
          (((uint32_t)LOWER(a10))*11*11*11*11*11*11*11*11*11*11) + \
          (((uint32_t)LOWER(a11))*11*11*11*11*11*11*11*11*11*11*11)))

  switch (str_hash)
  {
#define CTX_ENABLE_DEFUN 1
#if CTX_ENABLE_DEFUN
    case STR('d','e','f','u','n',0,0,0,0,0,0,0): ret = CTX_DEFUN; break;
    case STR('e','n','d','f','u','n',0,0,0,0,0,0): ret = CTX_ENDFUN; break;
#endif

    /* first a list of mappings to one_char hashes, handled in a
     * separate fast path switch without hashing
     */
    case STR('a','r','c','_','t','o',0,0,0,0,0,0):             ret = CTX_ARC_TO; break;
    case STR('a','r','c',0,0,0,0,0,0,0,0,0):                   ret = CTX_ARC; break;
    case STR('c','u','r','v','e','_','t','o',0,0,0,0):         ret = CTX_CURVE_TO; break;
    case STR('r','e','s','t','o','r','e',0,0,0,0,0):           ret = CTX_RESTORE; break;
    case STR('s','t','r','o','k','e',0,0,0,0,0,0):             ret = CTX_STROKE; break;
    case STR('f','i','l','l',0,0,0,0,0,0,0,0):                 ret = CTX_FILL; break;
    case STR('h','o','r','_','l','i','n','e','_','t','o',0):   ret = CTX_HOR_LINE_TO; break;
    case STR('r','o','t','a','t','e',0,0,0,0,0,0):             ret = CTX_ROTATE; break;
    case STR('c','o','l','o','r',0,0,0,0,0,0,0):               ret = CTX_SET_COLOR; break;
    case STR('l','i','n','e','_','t','o',0,0,0,0,0):           ret = CTX_LINE_TO; break;
    case STR('m','o','v','e','_','t','o',0,0,0,0,0):           ret = CTX_MOVE_TO; break;
    case STR('s','c','a','l','e',0,0,0,0,0,0,0):               ret = CTX_SCALE; break;
    case STR('n','e','w','_','p','a','g','e',0,0,0,0):         ret = CTX_NEW_PAGE; break;
    case STR('q','u','a','d','_','t','o',0,0,0,0,0):           ret = CTX_QUAD_TO; break;
    case STR('m','e','d','i','a','_','b','o','x',0,0,0):       ret = CTX_MEDIA_BOX; break;
    case STR('s','m','o','o','t','h','_','t','o',0,0,0):       ret = CTX_SMOOTH_TO; break;
    case STR('s','m','o','o','t','h','_','q','u','a','d','_'): ret = CTX_SMOOTHQ_TO; break;
    case STR('c','l','e','a','r',0,0,0,0,0,0,0):               ret = CTX_CLEAR; break;
    case STR('v','e','r','_','l','i','n','e','_','t','o',0):   ret = CTX_VER_LINE_TO; break;
    case STR('e','x','i','t',0,0,0,0,0,0,0,0):
    case STR('d','o','n','e',0,0,0,0,0,0,0,0):                 ret = CTX_EXIT; break;
    case STR('c','l','o','s','e','_','p','a','t','h',0,0):     ret =  CTX_CLOSE_PATH; break;
    case STR('n','e','w','_','p','a','t','h',0,0,0,0): 
    case STR('b','e','g','i','n','_','p','a','t','h',0,0):     ret =  CTX_NEW_PATH; break;

    case STR('r','e','l','_','a','r','c','_','t','o',0,0):     ret = CTX_REL_ARC_TO; break;
    case STR('c','l','i','p',0,0,0,0,0,0,0,0):                 ret = CTX_CLIP; break;
    case STR('r','e','l','_','c','u','r','v','e','_','t','o'): ret = CTX_REL_CURVE_TO; break;
    case STR('s','a','v','e',0,0,0,0,0,0,0,0):                 ret = CTX_SAVE; break;
    case STR('t','r','a','n','s','l','a','t','e',0,0,0):       ret = CTX_TRANSLATE; break;
    case STR('l','i','n','e','a','r','_','g','r','a','d','i'): ret = CTX_LINEAR_GRADIENT; break;
    case STR('r','e','l','_','h','o','r','_','l','i','n','e'): ret = CTX_REL_HOR_LINE_TO; break;


    case STR('r','e','l','_','l','i','n','e','_','t','o',0):   ret = CTX_REL_LINE_TO; break;
    case STR('r','e','l','_','m','o','v','e','_','t','o',0):   ret = CTX_REL_MOVE_TO; break;
    case STR('f','o','n','t',0,0,0,0,0,0,0,0):                 ret = CTX_SET_FONT; break;
    case STR('r','a','d','i','a','l','_','g','r','a','d','i'): ret = CTX_RADIAL_GRADIENT;  break;
    case STR('g','r','a','d','i','e','n','t','_','a','d','d'):
    case STR('a','d','d','_','s','t','o','p',0,0,0,0):         ret = CTX_GRADIENT_STOP; break;
    case STR('r','e','l','_','q','u','a','d','_','t','o',0):   ret = CTX_REL_QUAD_TO; break;
    case STR('r','e','c','t','a','n','g','l','e',0,0,0):
    case STR('r','e','c','t',0,0,0,0,'e',0,0,0):               ret = CTX_RECTANGLE; break;
    case STR('r','e','l','_','s','m','o','o','t','h','_','t'): ret = CTX_REL_SMOOTH_TO; break;
    case STR('r','e','l','_','s','m','o','o','t','h','_','q'): ret = CTX_REL_SMOOTHQ_TO; break;
    case STR('t','e','x','t','_','s','t','r','o','k','e', 0):  ret = CTX_TEXT_STROKE; break;
    case STR('r','e','l','_','v','e','r','_','l','i','n','e'): ret = CTX_REL_VER_LINE_TO; break;
    case STR('t','e','x','t',0,0,0,0,0,0,0,0):                 ret = CTX_TEXT; break;
    case STR('i','d','e','n','t','i','t','y',0,0,0,0):         ret = CTX_IDENTITY; break;
    case STR('t','r','a','n','s','f','o','r','m',0,0,0):       ret = CTX_SET_TRANSFORM; break;
                                                           // XXX: make it apply instead of set

    case STR(CTX_SET_PARAM,'m',0,0,0,0,0,0,0,0,0,0):
    case STR('c','o','m','p','o','s','i','t','i','n','g','m'): 
      return ctx_parser_set_command (parser, CTX_SET_COMPOSITING_MODE);

    case STR(CTX_SET_PARAM,'r',0,0,0,0,0,0,0,0,0,0):
    case STR('f','i','l','l','_','r','u','l','e',0,0,0): 
      return ctx_parser_set_command (parser, CTX_SET_FILL_RULE);

    case STR(CTX_SET_PARAM,'f',0,0,0,0,0,0,0,0,0,0):
    case STR('f','o','n','t','_','s','i','z','e',0,0,0):
      return ctx_parser_set_command (parser, CTX_SET_FONT_SIZE);

    case STR(CTX_SET_PARAM,'l',0,0,0,0,0,0,0,0,0,0):
    case STR('m','i','t','e','r','_','l','i','m','i','t',0):
      return ctx_parser_set_command (parser, CTX_SET_MITER_LIMIT);

    case STR(CTX_SET_PARAM,'t',0,0,0,0,0,0,0,0,0,0):
    case STR('t','e','x','t','_','a','l','i','g','n',0, 0):
      return ctx_parser_set_command (parser, CTX_SET_TEXT_ALIGN);

    case STR(CTX_SET_PARAM,'b',0,0,0,0,0,0,0,0,0,0):
    case STR('t','e','x','t','_','b','a','s','e','l','i','n'):
      return ctx_parser_set_command (parser, CTX_SET_TEXT_BASELINE);

    case STR(CTX_SET_PARAM,'d',0,0,0,0,0,0,0,0,0,0):
    case STR('t','e','x','t','_','d','i','r','e','c','t','i'):
      return ctx_parser_set_command (parser, CTX_SET_TEXT_DIRECTION);

    case STR(CTX_SET_PARAM,'j',0,0,0,0,0,0,0,0,0,0):
    case STR('j','o','i','n',0,0,0,0,0,0,0,0):
    case STR('l','i','n','e','_','j','o','i','n',0,0,0):
      return ctx_parser_set_command (parser, CTX_SET_LINE_JOIN);

    case STR(CTX_SET_PARAM,'c',0,0,0,0,0,0,0,0,0,0):
    case STR('c','a','p',0,0,0,0,0,0,0,0,0):
    case STR('l','i','n','e','_','c','a','p',0,0,0,0):
      return ctx_parser_set_command (parser, CTX_SET_LINE_CAP);

    case STR(CTX_SET_PARAM,'w',0,0,0,0,0,0,0,0,0,0):
    case STR('l','i','n','e','_','w','i','d','t','h',0,0):
      return ctx_parser_set_command (parser, CTX_SET_LINE_WIDTH);

    case STR(CTX_SET_PARAM,'a',0,0,0,0,0,0,0,0,0,0):
    case STR('g','l','o','b','a','l','_','a','l','p','h','a'):
      return ctx_parser_set_command (parser, CTX_SET_GLOBAL_ALPHA);

    /* strings are handled directly here,
     * instead of in the one-char handler, using return instead of break
     */
    case STR('g','r','a','y',0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_GRAY);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('g','r','a','y','a',0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_GRAYA);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('r','g','b',0,0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_RGB);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('r','g','b','a',0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_RGBA);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('c','m','y','k',0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_CMYK);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('l','a','b',0,0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_LAB);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('l','a','b','a',0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_LABA);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('l','c','h',0,0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_LCH);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('l','c','h','a',0,0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, CTX_LCHA);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    case STR('c','m','y','k','a',0,0,0,0,0,0,0):
      ctx_parser_set_color_model (parser, 104);
      return ctx_parser_set_command (parser, CTX_SET_COLOR);

    /* words in all caps map mapping to low integer/enum constants
    */
    case STR('w','i','n','d','i','n', 'g', 0, 0, 0, 0, 0):  return CTX_FILL_RULE_WINDING;
    case STR('e','v','e','n','o','d', 'd', 0, 0, 0, 0, 0):  return CTX_FILL_RULE_EVEN_ODD;
    case STR('e','v','e','n','_','o','d', 'd', 0, 0, 0, 0):  return CTX_FILL_RULE_EVEN_ODD;

    case STR('b','e','v','e','l',0, 0, 0, 0, 0, 0, 0):     return CTX_JOIN_BEVEL;
    case STR('r','o','u','n','d',0, 0, 0, 0, 0, 0, 0):     return CTX_JOIN_ROUND;
    case STR('m','i','t','e','r',0, 0, 0, 0, 0, 0, 0):     return CTX_JOIN_MITER;
    case STR('n','o','n','e', 0 ,0, 0, 0, 0, 0, 0, 0):      return CTX_CAP_NONE;
    case STR('s','q','u','a','r','e', 0, 0, 0, 0, 0, 0):   return CTX_CAP_SQUARE;

    case STR('s','t','a','r','t',0, 0, 0, 0, 0, 0, 0):
      return CTX_TEXT_ALIGN_START;
    case STR('e','n','d',0,0,0, 0, 0, 0, 0, 0, 0):
      return CTX_TEXT_ALIGN_END;
    case STR('l','e','f','t',0,0, 0, 0, 0, 0, 0, 0):
      return CTX_TEXT_ALIGN_LEFT;
    case STR('r','i','g','h','t',0, 0, 0, 0, 0, 0, 0):
      return CTX_TEXT_ALIGN_RIGHT;
    case STR('c','e','n','t','e','r', 0, 0, 0, 0, 0, 0):
      return CTX_TEXT_ALIGN_CENTER;

    case STR('t','o','p',0,0,0, 0, 0, 0, 0, 0, 0):
      return CTX_TEXT_BASELINE_TOP;
    case STR('b','o','t','t','o','m', 0, 0, 0, 0, 0, 0):
      return CTX_TEXT_BASELINE_BOTTOM;
    case STR('m','i','d','d','l', 'e',0, 0, 0, 0, 0, 0):
      return CTX_TEXT_BASELINE_MIDDLE;
    case STR('a','l','p','h','a', 'b','e', 't', 'i', 'c', 0, 0):
      return CTX_TEXT_BASELINE_ALPHABETIC;
    case STR('h','a','n','g','i', 'n','g', 0, 0, 0, 0, 0):
      return CTX_TEXT_BASELINE_HANGING;
    case STR('i','d','e','o','g','r','a','p','h','i','c', 0):
      return CTX_TEXT_BASELINE_IDEOGRAPHIC;

#undef STR
#undef LOWER
    default:
      ret = str_hash;
  }
  }

  return ctx_parser_set_command (parser, ret);
}

enum {
  CTX_PARSER_NEUTRAL = 0,
  CTX_PARSER_NUMBER,
  CTX_PARSER_NEGATIVE_NUMBER,
  CTX_PARSER_WORD,
  CTX_PARSER_COMMENT,
  CTX_PARSER_STRING_APOS,
  CTX_PARSER_STRING_QUOT,
  CTX_PARSER_STRING_APOS_ESCAPED,
  CTX_PARSER_STRING_QUOT_ESCAPED,
} CTX_STATE;

static void ctx_parser_set_color_model (CtxParser *parser, int color_model)
{
  parser->color_model      = color_model;
  parser->color_components = color_model % 100;
  if (parser->color_model >  99)
    parser->color_components++;
}

static void ctx_parser_get_color_rgba (CtxParser *parser, int offset, float *red, float *green, float *blue, float *alpha)
{
  /* this is the function that fetches colors from the input, 
   * we should here transform to the context's color-space
   *
   */

  *alpha = 1.0;
  switch (parser->color_model)
  {
    case CTX_GRAYA:
      *alpha = parser->numbers[offset + 1];
    case CTX_GRAY:
      *red = *green = *blue = parser->numbers[offset + 0];
      break;
    default:
    case CTX_LABA: // NYI - needs RGB profile
    case CTX_LCHA: // NYI - needs RGB profile
    case CTX_RGBA:
      *alpha = parser->numbers[offset + 3];
    case CTX_LAB: // NYI
    case CTX_LCH: // NYI
    case CTX_RGB:
      *red = parser->numbers[offset + 0];
      *green = parser->numbers[offset + 1];
      *blue = parser->numbers[offset + 2];
    break;

    case CTX_CMYKA:
      *alpha = parser->numbers[offset + 4];
    case CTX_CMYK:
      /* should use profile instead  */
      *red = (1.0-parser->numbers[offset + 0]) *
               (1.0 - parser->numbers[offset + 3]);
      *green = (1.0-parser->numbers[offset + 1]) *
                 (1.0 - parser->numbers[offset + 3]);
      *blue = (1.0-parser->numbers[offset + 2]) *
                 (1.0 - parser->numbers[offset + 3]);
    break;
  }
}

static void ctx_parser_dispatch_command (CtxParser *parser)
{
  CtxCode cmd = parser->command;
  Ctx *ctx = parser->ctx;

  if (parser->n_args != 100 &&
      parser->n_args != parser->n_numbers)
  {
    ctx_log ("ctx:%i:%i %c got %i instead of %i args\n",
      parser->line, parser->col,
      cmd, parser->n_numbers, parser->n_args);
  }
#define arg(a)  (parser->numbers[a])

  parser->command = CTX_NOP;
  switch (cmd)
  {
    default: break; // to silence warnings about missing ones
    case CTX_FILL:    ctx_fill (ctx);    break;
    case CTX_SAVE:    ctx_save (ctx);    break;
    case CTX_STROKE:  ctx_stroke (ctx);  break;
    case CTX_RESTORE: ctx_restore (ctx); break;

    case CTX_SET_COLOR:
      {
        switch (parser->color_model)
        {
          case CTX_GRAY:
            ctx_set_gray (ctx, arg(0));
            break;
          case CTX_GRAYA:
            ctx_set_rgba (ctx, arg(0), arg(0), arg(0), arg(1));
            break;
          case CTX_RGB:
            ctx_set_rgb (ctx, arg(0), arg(1), arg(2)); break;
#if CTX_ENABLE_CMYK
          case CTX_CMYK:
            ctx_set_cmyk (ctx, arg(0), arg(1), arg(2), arg(3)); break;
          case CTX_CMYKA:
            ctx_set_cmyka (ctx, arg(0), arg(1), arg(2), arg(3), arg(4)); break;
#else
          /* when there is no cmyk support at all in renderer
           * do a naive mapping to RGB on input.
           */
          case CTX_CMYK:
          case CTX_CMYKA:
            {
              float c = arg(0);
              float m = arg(1);
              float y = arg(2);
              float k = arg(3);
              float r = (1.0f-c) * (1.0f-k);
              float g = (1.0f-m) * (1.0f-k);
              float b = (1.0f-y) * (1.0f-k);
              float a = 1.0f;
              if (parser->color_model == CTX_CMYKA)
                a = arg(4);
              ctx_set_rgba (ctx, r, g, b, a);
            }
            break;
#endif
          case CTX_RGBA:
            ctx_set_rgba (ctx, arg(0), arg(1), arg(2), arg(3));
            break;
        }
      }
      break;
    case CTX_ARC_TO: 
      ctx_arc_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4));
      break;
    case CTX_REL_ARC_TO:
      ctx_rel_arc_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4));
      break;
    case CTX_REL_SMOOTH_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          float ax = 2 * ctx_x (ctx) - cx;
          float ay = 2 * ctx_y (ctx) - cy;
          ctx_curve_to (ctx, ax, ay, arg(0) +  cx, arg(1) + cy,
                             arg(2) + cx, arg(3) + cy);
          parser->pcx = arg(0) + cx;
          parser->pcy = arg(1) + cy;
        }
        break;
    case CTX_SMOOTH_TO:
        {
          float ax = 2 * ctx_x (ctx) - parser->pcx;
          float ay = 2 * ctx_y (ctx) - parser->pcy;
          ctx_curve_to (ctx, ax, ay, arg(0), arg(1),
                             arg(2), arg(3));
          parser->pcx = arg(0);
          parser->pcx = arg(1);
        }
        break;

    case CTX_SMOOTHQ_TO:
        ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0), arg(1));
        break;
    case CTX_REL_SMOOTHQ_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          parser->pcx = 2 * ctx_x (ctx) - parser->pcx;
          parser->pcy = 2 * ctx_y (ctx) - parser->pcy;
          ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0) +  cx, arg(1) + cy);
        }
        break;
    case CTX_VER_LINE_TO: ctx_line_to (ctx, ctx_x (ctx), arg(0)); parser->command = CTX_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
    case CTX_HOR_LINE_TO:
        ctx_line_to (ctx, arg(0), ctx_y(ctx)); parser->command = CTX_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
    case CTX_REL_HOR_LINE_TO: ctx_rel_line_to (ctx, arg(0), 0.0f); parser->command = CTX_REL_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
    case CTX_REL_VER_LINE_TO: ctx_rel_line_to (ctx, 0.0f, arg(0)); parser->command = CTX_REL_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
    case CTX_ARC:
        ctx_arc (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5));
        break;
    case CTX_SET_TRANSFORM:
        ctx_set_transform (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5));
        break;
    case CTX_CURVE_TO:
        ctx_curve_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5));
        parser->pcx = arg(2);
        parser->pcy = arg(3);
        parser->command = CTX_CURVE_TO;
        break;
    case CTX_REL_CURVE_TO:
        parser->pcx = arg(2) + ctx_x (ctx);
        parser->pcy = arg(3) + ctx_y (ctx);
                        
        ctx_rel_curve_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5));
        parser->command = CTX_REL_CURVE_TO;
        break;
    case CTX_LINE_TO:
        ctx_line_to (ctx, arg(0), arg(1));
        parser->command = CTX_LINE_TO;
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        break;
    case CTX_MOVE_TO:
        ctx_move_to (ctx, arg(0), arg(1));
        parser->command = CTX_LINE_TO;
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        parser->left_margin = parser->pcx;
        break;
    case CTX_SET_FONT_SIZE:
        ctx_set_font_size (ctx, arg(0));
        break;
    case CTX_SET_MITER_LIMIT:
        ctx_set_miter_limit (ctx, arg(0));
        break;
    case CTX_SCALE:
        ctx_scale (ctx, arg(0), arg(1));
        break;
    case CTX_QUAD_TO:
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        ctx_quad_to (ctx, arg(0), arg(1), arg(2), arg(3));
        parser->command = CTX_QUAD_TO;
        break;
    case CTX_REL_QUAD_TO: 
        parser->pcx = arg(0) + ctx_x (ctx);
        parser->pcy = arg(1) + ctx_y (ctx);
        ctx_rel_quad_to (ctx, arg(0), arg(1), arg(2), arg(3));
        parser->command = CTX_REL_QUAD_TO;
        break;
    case CTX_CLIP:
        ctx_clip (ctx);
        break;
    case CTX_TRANSLATE:
        ctx_translate (ctx, arg(0), arg(1));
        break;
    case CTX_ROTATE:
        ctx_rotate (ctx, arg(0));
        break;
    case CTX_SET_FONT:
        ctx_set_font (ctx, (char*)parser->holding);
        break;
    case CTX_TEXT_STROKE:
    case CTX_TEXT:
        if (parser->n_numbers == 1)
          ctx_rel_move_to (ctx, -parser->numbers[0], 0.0);  //  XXX : scale by font(size)
        else
        {
          if (strchr (parser->holding, '\n'))
          {
            /* XXX : instead of a strdup, having a linebuffer would be better
             */
            char *copy = strdup ((char*)parser->holding);
            char *c;
            for (c = copy; c; )
            {
              char *next_nl = strchr (c, '\n');
              if (next_nl)
              {
                *next_nl = 0;
              }

              /* do our own layouting on a per-word basis?, to get justified
               * margins? then we'd want explict margins rather than the
               * implicit ones from move_to's .. making move_to work within
               * margins.
               */
              if (cmd == CTX_TEXT_STROKE)
                ctx_text_stroke (ctx, c);
              else
                ctx_text (ctx, c);

              if (next_nl)
              {
                ctx_move_to (ctx, parser->left_margin, ctx_y (ctx) + 
                                  ctx_get_font_size (ctx));
                c = next_nl + 1;
              }
              else
              {
                c = NULL;
              }
            }
            free (copy);
          }
          else
          {
            if (cmd == CTX_TEXT_STROKE)
              ctx_text_stroke (ctx, parser->holding);
            else
              ctx_text (ctx, parser->holding);
          }
        }
        if (cmd == CTX_TEXT_STROKE)
          parser->command = CTX_TEXT_STROKE;
        else
          parser->command = CTX_TEXT;
        break;
    case CTX_REL_LINE_TO:
        ctx_rel_line_to (ctx , arg(0), arg(1));
        parser->pcx += arg(0);
        parser->pcy += arg(1);
        break;
    case CTX_REL_MOVE_TO:
        ctx_rel_move_to (ctx , arg(0), arg(1));
        parser->pcx += arg(0);
        parser->pcy += arg(1);
        parser->left_margin = ctx_x (ctx);
        break;
    case CTX_SET_LINE_WIDTH:
        ctx_set_line_width (ctx, arg(0));
        break;
    case CTX_SET_LINE_JOIN:
        ctx_set_line_join (ctx, arg(0));
        break;
    case CTX_SET_LINE_CAP:
        ctx_set_line_cap (ctx, arg(0));
        break;
    case CTX_SET_COMPOSITING_MODE:
        ctx_set_compositing_mode (ctx, arg(0));
        break;
    case CTX_SET_FILL_RULE:
        ctx_set_fill_rule (ctx, arg(0));
        break;
    case CTX_SET_TEXT_ALIGN:
        ctx_set_text_align (ctx, arg(0));
        break;
    case CTX_SET_TEXT_BASELINE:
        ctx_set_text_baseline (ctx, arg(0));
        break;
    case CTX_SET_TEXT_DIRECTION:
        ctx_set_text_direction (ctx, arg(0));
        break;
    case CTX_IDENTITY:
        ctx_identity_matrix (ctx);
        break;
    case CTX_RECTANGLE:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3));
        break;
    case CTX_LINEAR_GRADIENT:
       ctx_linear_gradient (ctx, arg(0), arg(1), arg(2), arg(3));
       break;
    case CTX_RADIAL_GRADIENT:
        ctx_radial_gradient (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5));
      break;
    case CTX_GRADIENT_STOP:
      {
        float red, green, blue, alpha;
        ctx_parser_get_color_rgba (parser, 1, &red, &green, &blue, &alpha);

        ctx_gradient_add_stop (ctx, arg(0), red, green, blue, alpha);
      }
      break;
    case CTX_SET_GLOBAL_ALPHA:
      ctx_set_global_alpha (ctx, arg(0));
      break;
    case CTX_NEW_PATH:
       ctx_new_path (ctx);
       break;
    case CTX_CLOSE_PATH:
       ctx_close_path (ctx);
       break;
    case CTX_EXIT:
       if (parser->exit)
         parser->exit (parser->exit_data);
       break;
    case CTX_CLEAR:
       ctx_clear (ctx);
       ctx_translate (ctx,
                     (parser->cursor_x-1) * parser->cell_width * 1.0,
                     (parser->cursor_y-1) * parser->cell_height * 1.0);
       break;
  }
#undef arg
  parser->n_numbers = 0;
}

static void ctx_parser_holding_append (CtxParser *parser, int byte)
{
  parser->holding[parser->pos++]=byte;
  if (parser->pos > (int)sizeof(parser->holding)-2)
    parser->pos = sizeof(parser->holding)-2;
  parser->holding[parser->pos]=0;
}

static void ctx_parser_transform_percent (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  int big   = parser->width;
  int small = parser->height;
  if (big < small)
  {
    small = parser->width;
    big   = parser->height;
  }

  switch (code)
  {
    case CTX_RADIAL_GRADIENT:
    case CTX_ARC:
      switch (arg_no)
      {
        case 0:
        case 3:
          *value *= (parser->width/100.0);
          break;
        case 1:
        case 4:
          *value *= (parser->height/100.0);
          break;
        case 2:
        case 5:
          *value *= small/100.0;
          break;
      }

      break;
    case CTX_SET_FONT_SIZE:
    case CTX_SET_MITER_LIMIT:
    case CTX_SET_LINE_WIDTH:
      {
        *value *= (small/100.0);
      }
      break;

    case CTX_ARC_TO:
    case CTX_REL_ARC_TO:
      if (arg_no > 3)
      {
        *value *= (small/100.0);
      }
      else
      {
        if (arg_no % 2 == 0)
          *value  *= ((parser->width)/100.0);
        else
          *value *= ((parser->height)/100.0);
      }
      break;

    default: // even means x coord
      if (arg_no % 2 == 0)
        *value  *= ((parser->width)/100.0);
      else
        *value *= ((parser->height)/100.0);
      break;

  }
}

static void ctx_parser_transform_cell (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  float small = parser->cell_width;
  if (small > parser->cell_height)
    small = parser->cell_height;

  switch (code)
  {

    case CTX_RADIAL_GRADIENT:
    case CTX_ARC:
      switch (arg_no)
      {
        case 0: case 3:
          *value *= parser->cell_width;
          break;
        case 1: case 4:
          *value *= parser->cell_height;
          break;
        case 2: case 5:
          *value *= small; // use height?
          break;
      }

      break;
    case CTX_SET_MITER_LIMIT:
    case CTX_SET_FONT_SIZE:
    case CTX_SET_LINE_WIDTH:
      {
        *value *= parser->cell_height;
      }
      break;

    case CTX_ARC_TO:
    case CTX_REL_ARC_TO:
      if (arg_no > 3)
      {
        *value *= small;
      }
      else
      {
        *value *= (arg_no%2==0)?parser->cell_width:parser->cell_height;
      }
      break;

    case CTX_RECTANGLE:
      if (arg_no % 2 == 0)
        *value *= parser->cell_width;
      else
      {
         if (!(arg_no > 1))
           parser->numbers[parser->n_numbers] --; // XXX ?
        *value *= parser->cell_height;
      }
      break;
    default: // even means x coord odd means y coord
      *value *= (arg_no%2==0)?parser->cell_width:parser->cell_height;
      break;
  }
}

static void ctx_parser_word_done (CtxParser *parser)
{
  parser->holding[parser->pos]=0;
  int command = ctx_parser_resolve_command (parser, parser->holding);

  if (command >= 0 && command < 10) // special case low enum values
  {
    parser->numbers[parser->n_numbers] = command;
    parser->state = CTX_PARSER_NUMBER;
    ctx_parser_feed_byte (parser, ',');
  }
  else if (command > 0)
  {
     parser->command = command;
     if (parser->n_args == 0)
     {
       ctx_parser_dispatch_command (parser);
     }
  }
  else
  {
    /* interpret char by char */
    uint8_t buf[16]=" ";
    for (int i = 0; parser->pos && parser->holding[i] > ' '; i++)
    {
       buf[0] = parser->holding[i];
       parser->command = ctx_parser_resolve_command (parser, buf);
       if (parser->command > 0)
       {
         if (parser->n_args == 0)
         {
           ctx_parser_dispatch_command (parser);
         }
       }
       else
       {
         ctx_log ("unhandled command '%c'\n", buf[0]);
       }
    }
  }
  parser->n_numbers = 0;
}

static void ctx_parser_string_done (CtxParser *parser)
{
  ctx_parser_dispatch_command (parser);
}

void ctx_parser_feed_byte (CtxParser *parser, int byte)
{
  switch (byte)
  {
    case '\n': parser->col=0; parser->line++; break;
    default: parser->col++;
  }
  switch (parser->state)
  {
    case CTX_PARSER_NEUTRAL:
      switch (byte)
      {
         case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
         case 8: case 11: case 12: case 14: case 15: case 16: case 17:
         case 18: case 19: case 20: case 21: case 22: case 23: case 24:
         case 25: case 26: case 27: case 28: case 29: case 30: case 31:
            break;
         case ' ':case '\t':case '\r':case '\n':case ';':case ',':case '(':case ')':
         case '{':case '}':case '=':
            break;
         case '#':
            parser->state = CTX_PARSER_COMMENT;
            break;
         case '\'':
            parser->state = CTX_PARSER_STRING_APOS;
            parser->pos = 0;
            parser->holding[0] = 0;
            break;
         case '"':
            parser->state = CTX_PARSER_STRING_QUOT;
            parser->pos = 0;
            parser->holding[0] = 0;
            break;
         case '-':
            parser->state = CTX_PARSER_NEGATIVE_NUMBER;
            parser->numbers[parser->n_numbers] = 0;
            parser->decimal = 0;
            break;
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            parser->state = CTX_PARSER_NUMBER;
            parser->numbers[parser->n_numbers] = 0;
            parser->numbers[parser->n_numbers] += (byte - '0');
            parser->decimal = 0;
            break;
         case '.':
            parser->state = CTX_PARSER_NUMBER;
            parser->numbers[parser->n_numbers] = 0;
            parser->decimal = 1;
            break;
         default:
            parser->state = CTX_PARSER_WORD;
            parser->pos = 0;
            ctx_parser_holding_append (parser, byte);
            break;
      }
      break;
    case CTX_PARSER_NUMBER:
    case CTX_PARSER_NEGATIVE_NUMBER:
      {
        int new_neg = 0;
        switch (byte)
        {
           case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
           case 8: case 11: case 12: case 14: case 15: case 16: case 17:
           case 18: case 19: case 20: case 21: case 22: case 23: case 24:
           case 25: case 26: case 27: case 28: case 29: case 30: case 31:
              parser->state = CTX_PARSER_NEUTRAL;
              break;
           case ' ':case '\t':case '\r':case '\n':case ';':case ',':case '(':case ')':
           case '{':case '}':case '=':
              if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                parser->numbers[parser->n_numbers] *= -1;
    
              parser->state = CTX_PARSER_NEUTRAL;
              break;
           case '#':
              parser->state = CTX_PARSER_COMMENT;
              break;
           case '-':
              parser->state = CTX_PARSER_NEGATIVE_NUMBER;
              new_neg = 1;
              parser->numbers[parser->n_numbers+1] = 0;
              parser->decimal = 0;
              break;
           case '.': 
              //if (parser->decimal) // TODO permit .13.32.43 to equivalent to .12 .32 .43
              parser->decimal = 1;
              break;
           case '0': case '1': case '2': case '3': case '4':
           case '5': case '6': case '7': case '8': case '9':
              if (parser->decimal)
              {
                parser->decimal *= 10;
                parser->numbers[parser->n_numbers] += (byte - '0') / (1.0 * parser->decimal);
              }
              else
              {
                parser->numbers[parser->n_numbers] *= 10;
                parser->numbers[parser->n_numbers] += (byte - '0');
              }
              break;
           case '@': // cells
              if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                parser->numbers[parser->n_numbers] *= -1;

              ctx_parser_transform_cell (parser, parser->command, parser->n_numbers, &parser->numbers[parser->n_numbers]);

              parser->state = CTX_PARSER_NEUTRAL;
          break;
           case '%': // percent of width/height
              if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                parser->numbers[parser->n_numbers] *= -1;

              ctx_parser_transform_percent (parser, parser->command, parser->n_numbers, &parser->numbers[parser->n_numbers]);

              parser->state = CTX_PARSER_NEUTRAL;
              break;
           default:
              if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                parser->numbers[parser->n_numbers] *= -1;
              parser->state = CTX_PARSER_WORD;
              parser->pos = 0;
              ctx_parser_holding_append (parser, byte);
              break;
        }
        if ((parser->state != CTX_PARSER_NUMBER &&
             parser->state != CTX_PARSER_NEGATIVE_NUMBER) || new_neg)
        {
          parser->n_numbers ++;
          if (parser->n_numbers == parser->n_args || parser->n_args == 100)
          {
            ctx_parser_dispatch_command (parser);
          }
    
          if (parser->n_numbers > 10)
            parser->n_numbers = 10;
        }
      }
      break;

    case CTX_PARSER_WORD:
      switch (byte)
      {
         case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
         case 8: case 11: case 12: case 14: case 15: case 16: case 17:
         case 18: case 19: case 20: case 21: case 22: case 23: case 24:
         case 25: case 26: case 27: case 28: case 29: case 30: case 31:
         case ' ':case '\t':case '\r':case '\n':case ';':case ',':case '(':case ')':case '=':
         case '{':case '}':
            parser->state = CTX_PARSER_NEUTRAL;
            break;
         case '#':
            parser->state = CTX_PARSER_COMMENT;
            break;
         case '-':
            parser->state = CTX_PARSER_NEGATIVE_NUMBER;
            parser->numbers[parser->n_numbers] = 0;
            parser->decimal = 0;
            break;
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            parser->state = CTX_PARSER_NUMBER;
            parser->numbers[parser->n_numbers] = 0;
            parser->numbers[parser->n_numbers] += (byte - '0');
            parser->decimal = 0;
            break;
         case '.':
            parser->state = CTX_PARSER_NUMBER;
            parser->numbers[parser->n_numbers] = 0;
            parser->decimal = 1;
            break;
         default:
            ctx_parser_holding_append (parser, byte);
            break;
      }
      if (parser->state != CTX_PARSER_WORD)
      {
        ctx_parser_word_done (parser);
      }
      break;

    case CTX_PARSER_STRING_APOS:
      switch (byte)
      {
         case '\\':
            parser->state = CTX_PARSER_STRING_APOS_ESCAPED;
            break;
         case '\'':
            parser->state = CTX_PARSER_NEUTRAL;
            ctx_parser_string_done (parser);
            break;
         default:
            ctx_parser_holding_append (parser, byte);
            break;
      }
      break;
    case CTX_PARSER_STRING_APOS_ESCAPED:
      switch (byte)
      {
         case '0': byte = '\0'; break;
         case 'b': byte = '\b'; break;
         case 'f': byte = '\f'; break;
         case 'n': byte = '\n'; break;
         case 'r': byte = '\r'; break;
         case 't': byte = '\t'; break;
         case 'v': byte = '\v'; break;
         default: break;
      }
      ctx_parser_holding_append (parser, byte);

      parser->state = CTX_PARSER_STRING_APOS;
      break;
    case CTX_PARSER_STRING_QUOT_ESCAPED:
      switch (byte)
      {
         case '0': byte = '\0'; break;
         case 'b': byte = '\b'; break;
         case 'f': byte = '\f'; break;
         case 'n': byte = '\n'; break;
         case 'r': byte = '\r'; break;
         case 't': byte = '\t'; break;
         case 'v': byte = '\v'; break;
         default: break;
      }
      ctx_parser_holding_append (parser, byte);
      parser->state = CTX_PARSER_STRING_QUOT;
      break;

    case CTX_PARSER_STRING_QUOT:
      switch (byte)
      {
         case '\\':
            parser->state = CTX_PARSER_STRING_QUOT_ESCAPED;
            break;
         case '"':
            parser->state = CTX_PARSER_NEUTRAL;
            ctx_parser_string_done (parser);
            break;
         default:
            ctx_parser_holding_append (parser, byte);
            break;
      }
      break;
    case CTX_PARSER_COMMENT:
      switch (byte)
      {
        case '\r':
        case '\n':
          parser->state = CTX_PARSER_NEUTRAL;
        default:
          break;
      }
      break;
  }
}
#endif

#if CTX_MATH

#undef sqrtf
#undef atanf
#undef asinf
#undef acosf
#undef atan2f
#undef sinf
#undef fabsf
#undef cosf
#undef tanf
#undef hypotf

#else

#include <math.h>

#endif

#endif
