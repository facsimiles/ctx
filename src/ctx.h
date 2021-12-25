/* 
 * ctx.h is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * ctx.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ctx; if not, see <https://www.gnu.org/licenses/>.
 *
 * 2012, 2015, 2019, 2020 Øyvind Kolås <pippin@gimp.org>
 *
 * ctx is a single header 2d vector graphics processing framework.
 *
 * To use ctx in a project, do the following:
 *
 * #define CTX_IMPLEMENTATION
 * #include "ctx.h"
 *
 * Ctx contains a minimal default fallback font with only ascii, so
 * you probably want to also include a font, and perhaps enable
 * the cairo or SDL2 optional backends, a more complete example
 * could be:
 *
 * #include <cairo.h>
 * #include <SDL.h>
 * #include "ctx-font-regular.h"
 * #define CTX_IMPLEMENTATION
 * #include "ctx.h"
 *
 * The behavior of ctx can be tweaked, and features can be configured, enabled
 * or disabled with other #defines, see further down in the start of this file
 * for details.
 */

#ifndef CTX_H
#define CTX_H

#ifdef __cplusplus
extern "C" {
#endif

#if !__COSMOPOLITAN__
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#if CTX_EVENTS
#include <sys/select.h> // XXX if events?
#endif
#endif

typedef struct _Ctx            Ctx;

/* The pixel formats supported as render targets
 */
enum _CtxPixelFormat
{
  CTX_FORMAT_NONE=0,
  CTX_FORMAT_GRAY8,  // 1  - these enum values are not coincidence
  CTX_FORMAT_GRAYA8, // 2  -
  CTX_FORMAT_RGB8,   // 3  -
  CTX_FORMAT_RGBA8,  // 4  -
  CTX_FORMAT_BGRA8,  // 5
  CTX_FORMAT_RGB565, // 6
  CTX_FORMAT_RGB565_BYTESWAPPED, // 7
  CTX_FORMAT_RGB332, // 8
  CTX_FORMAT_RGBAF,  // 9
  CTX_FORMAT_GRAYF,  // 10
  CTX_FORMAT_GRAYAF, // 11
  CTX_FORMAT_GRAY1,  //12 MONO
  CTX_FORMAT_GRAY2,  //13 DUO
  CTX_FORMAT_GRAY4,  //14
  CTX_FORMAT_CMYK8,  //15
  CTX_FORMAT_CMYKA8, //16 
  CTX_FORMAT_CMYKAF, //17
  CTX_FORMAT_YUV420, //18
  CTX_FORMAT_RGBA8_SEPARATE_ALPHA, // 19
};
typedef enum   _CtxPixelFormat CtxPixelFormat;

typedef struct _CtxGlyph       CtxGlyph;

/**
 * ctx_new:
 *
 * Create a new drawing context, this context has no pixels but
 * accumulates commands and can be played back on other ctx
 * render contexts.
 */
Ctx *ctx_new (void);

/**
 * ctx_new_for_framebuffer:
 *
 * Create a new drawing context for a framebuffer, rendering happens
 * immediately.
 */
Ctx *ctx_new_for_framebuffer (void *data,
                              int   width,
                              int   height,
                              int   stride,
                              CtxPixelFormat pixel_format);
/**
 * ctx_new_ui:
 *
 * Create a new interactive ctx context, might depend on additional
 * integration.
 *
 * The values for backend are as for the environment variable,
 * NULL for auto.
 */
Ctx *ctx_new_ui (int width, int height, const char *backend);

/**
 * ctx_new_for_drawlist:
 *
 * Create a new drawing context for a pre-existing drawlist.
 */
Ctx *ctx_new_for_drawlist (void *data, size_t length);


/**
 * ctx_dirty_rect:
 *
 * Query the dirtied bounding box of drawing commands thus far.
 */
void  ctx_dirty_rect      (Ctx *ctx, int *x, int *y, int *width, int *height);

/**
 * ctx_free:
 * @ctx: a ctx context
 */
void ctx_free (Ctx *ctx);

/* clears and resets a context */
void ctx_reset          (Ctx *ctx);
void ctx_begin_path     (Ctx *ctx);
void ctx_save           (Ctx *ctx);
void ctx_restore        (Ctx *ctx);
void ctx_start_group    (Ctx *ctx);
void ctx_end_group      (Ctx *ctx);
void ctx_clip           (Ctx *ctx);
void ctx_identity       (Ctx *ctx);
void ctx_rotate         (Ctx *ctx, float x);

void ctx_image_smoothing     (Ctx *ctx, int enabled);
int  ctx_get_image_smoothing (Ctx *ctx);

#define CTX_LINE_WIDTH_HAIRLINE -1000.0
#define CTX_LINE_WIDTH_ALIASED  -1.0
#define CTX_LINE_WIDTH_FAST     -1.0  /* aliased 1px wide line */
void ctx_miter_limit (Ctx *ctx, float limit);
float ctx_get_miter_limit (Ctx *ctx);
void ctx_line_width       (Ctx *ctx, float x);
void ctx_line_dash_offset (Ctx *ctx, float line_dash);
float ctx_get_line_dash_offset (Ctx *ctx);
void ctx_apply_transform  (Ctx *ctx, float a,  float b,  // hscale, hskew
                                     float c,  float d,  // vskew,  vscale
                                     float e,  float f); // htran,  vtran
void ctx_set_transform    (Ctx *ctx, float a, float b, float c, float d, float e, float f);
void  ctx_line_dash       (Ctx *ctx, float *dashes, int count);
void  ctx_font_size       (Ctx *ctx, float x);
void  ctx_font            (Ctx *ctx, const char *font);
void  ctx_font_family     (Ctx *ctx, const char *font_family);
void  ctx_scale           (Ctx *ctx, float x, float y);
void  ctx_translate       (Ctx *ctx, float x, float y);
void  ctx_line_to         (Ctx *ctx, float x, float y);
void  ctx_move_to         (Ctx *ctx, float x, float y);
void  ctx_curve_to        (Ctx *ctx, float cx0, float cy0,
                           float cx1, float cy1,
                           float x, float y);
void  ctx_quad_to         (Ctx *ctx, float cx, float cy,
                           float x, float y);
void  ctx_arc             (Ctx  *ctx,
                           float x, float y,
                           float radius,
                           float angle1, float angle2,
                           int   direction);
void  ctx_arc_to          (Ctx *ctx, float x1, float y1,
                           float x2, float y2, float radius);
void  ctx_rel_arc_to      (Ctx *ctx, float x1, float y1,
                           float x2, float y2, float radius);
void  ctx_rectangle       (Ctx *ctx,
                           float x0, float y0,
                           float w, float h);
void  ctx_round_rectangle (Ctx *ctx,
                           float x0, float y0,
                           float w, float h,
                           float radius);
void  ctx_rel_line_to     (Ctx *ctx,
                           float x, float y);
void  ctx_rel_move_to     (Ctx *ctx,
                           float x, float y);
void  ctx_rel_curve_to    (Ctx *ctx,
                           float x0, float y0,
                           float x1, float y1,
                           float x2, float y2);
void  ctx_rel_quad_to     (Ctx *ctx,
                           float cx, float cy,
                           float x, float y);
void  ctx_close_path      (Ctx *ctx);
float ctx_get_font_size   (Ctx *ctx);
const char *ctx_get_font  (Ctx *ctx);
float ctx_get_line_width  (Ctx *ctx);
int   ctx_width           (Ctx *ctx);
int   ctx_height          (Ctx *ctx);
float ctx_x               (Ctx *ctx);
float ctx_y               (Ctx *ctx);
void  ctx_current_point   (Ctx *ctx, float *x, float *y);
void  ctx_get_transform   (Ctx *ctx, float *a, float *b,
                           float *c, float *d,
                           float *e, float *f);

CtxGlyph *ctx_glyph_allocate (int n_glyphs);

void gtx_glyph_free       (CtxGlyph *glyphs);

int  ctx_glyph            (Ctx *ctx, uint32_t unichar, int stroke);

void ctx_preserve         (Ctx *ctx);
void ctx_fill             (Ctx *ctx);
void ctx_stroke           (Ctx *ctx);

void ctx_parse            (Ctx *ctx, const char *string);

void ctx_shadow_rgba      (Ctx *ctx, float r, float g, float b, float a);
void ctx_shadow_blur      (Ctx *ctx, float x);
void ctx_shadow_offset_x  (Ctx *ctx, float x);
void ctx_shadow_offset_y  (Ctx *ctx, float y);
void ctx_view_box         (Ctx *ctx,
                           float x0, float y0,
                           float w, float h);
void
ctx_set_pixel_u8          (Ctx *ctx, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void  ctx_global_alpha     (Ctx *ctx, float global_alpha);
float ctx_get_global_alpha (Ctx *ctx);

void ctx_named_source (Ctx *ctx, const char *name);
// followed by a color, gradient or pattern definition

void ctx_stroke_source  (Ctx *ctx); // next source definition is for stroking

void ctx_rgba_stroke   (Ctx *ctx, float r, float g, float b, float a);
void ctx_rgb_stroke    (Ctx *ctx, float r, float g, float b);
void ctx_rgba8_stroke  (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void ctx_gray_stroke   (Ctx *ctx, float gray);
void ctx_drgba_stroke  (Ctx *ctx, float r, float g, float b, float a);
void ctx_cmyka_stroke  (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_cmyk_stroke   (Ctx *ctx, float c, float m, float y, float k);
void ctx_dcmyka_stroke (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_dcmyk_stroke  (Ctx *ctx, float c, float m, float y, float k);



void ctx_rgba   (Ctx *ctx, float r, float g, float b, float a);
void ctx_rgb    (Ctx *ctx, float r, float g, float b);
void ctx_rgba8  (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void ctx_gray   (Ctx *ctx, float gray);
void ctx_drgba  (Ctx *ctx, float r, float g, float b, float a);
void ctx_cmyka  (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_cmyk   (Ctx *ctx, float c, float m, float y, float k);
void ctx_dcmyka (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_dcmyk  (Ctx *ctx, float c, float m, float y, float k);

/* there is also getters for colors, by first setting a color in one format and getting
 * it with another color conversions can be done
 */

void ctx_get_rgba   (Ctx *ctx, float *rgba);
void ctx_get_graya  (Ctx *ctx, float *ya);
void ctx_get_drgba  (Ctx *ctx, float *drgba);
void ctx_get_cmyka  (Ctx *ctx, float *cmyka);
void ctx_get_dcmyka (Ctx *ctx, float *dcmyka);
int  ctx_in_fill    (Ctx *ctx, float x, float y);
int  ctx_in_stroke  (Ctx *ctx, float x, float y);

void ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1);
void ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0,
                          float x1, float y1, float r1);
/* XXX should be ctx_gradient_add_stop_rgba */
void ctx_gradient_add_stop (Ctx *ctx, float pos, float r, float g, float b, float a);

void ctx_gradient_add_stop_u8 (Ctx *ctx, float pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a);


/*
 *
 */
void ctx_define_texture (Ctx *ctx,
                         const char *eid,
                         int         width,
                         int         height,
                         int         stride,
                         int         format,
                         void       *data,
                         char       *ret_eid);

void
ctx_source_transform (Ctx *ctx, float a, float b,  // hscale, hskew
                      float c, float d,  // vskew,  vscale
                      float e, float f);  // htran,  vtran
typedef struct _CtxMatrix     CtxMatrix;
void
ctx_source_transform_matrix (Ctx *ctx, CtxMatrix *matrix);

void
ctx_get_image_data (Ctx *ctx, int sx, int sy, int sw, int sh,
                    CtxPixelFormat format, int dst_stride,
                    uint8_t *dst_data);

void
ctx_put_image_data (Ctx *ctx, int w, int h, int stride, int format,
                    uint8_t *data,
                    int ox, int oy,
                    int dirtyX, int dirtyY,
                    int dirtyWidth, int dirtyHeight);


/* loads an image file from disk into texture, returning pixel width, height
 * and eid, the eid is based on the path; not the contents - avoiding doing
 * sha1 checksum of contents. The width and height of the image is returned
 * along with the used eid, width height or eid can be NULL if we
 * do not care about their values.
 */
void ctx_texture_load (Ctx        *ctx,
                       const char *path,
                       int        *width,
                       int        *height,
                       char       *eid);

/* sets the paint source to be a texture by eid
 */
void ctx_texture              (Ctx *ctx, const char *eid, float x, float y);

void ctx_draw_texture         (Ctx *ctx, const char *eid, float x, float y, float w, float h);

void ctx_draw_texture_clipped (Ctx *ctx, const char *eid, float x, float y, float w, float h, float sx, float sy, float swidth, float sheight);

void ctx_draw_image           (Ctx *ctx, const char *path, float x, float y, float w, float h);

void ctx_draw_image_clipped   (Ctx *ctx, const char *path, float x, float y, float w, float h, float sx, float sy, float swidth, float sheight);

/* used by the render threads of fb and sdl backends.
 */
void ctx_set_texture_source (Ctx *ctx, Ctx *texture_source);
/* used when sharing cache state of eids between clients
 */
void ctx_set_texture_cache (Ctx *ctx, Ctx *texture_cache);

typedef struct _CtxDrawlist CtxDrawlist;
typedef void (*CtxFullCb) (CtxDrawlist *drawlist, void *data);

int ctx_pixel_format_bits_per_pixel (CtxPixelFormat format); // bits per pixel
int ctx_pixel_format_get_stride (CtxPixelFormat format, int width);
int ctx_pixel_format_components (CtxPixelFormat format);

void _ctx_set_store_clear (Ctx *ctx);
void _ctx_set_transformation (Ctx *ctx, int transformation);

Ctx *ctx_hasher_new (int width, int height, int cols, int rows);
uint8_t *ctx_hasher_get_hash (Ctx *ctx, int col, int row);

int ctx_utf8_strlen (const char *s);

#ifdef _BABL_H
#define CTX_BABL 1
#else
#define CTX_BABL 0
#endif

/* If cairo.h is included before ctx.h add cairo integration code
 */
#ifdef CAIRO_H
#ifndef CTX_CAIRO
#define CTX_CAIRO 1
#endif
#endif

#ifndef CTX_SDL
#ifdef SDL_h_
#define CTX_SDL 1
#else
#define CTX_SDL 0
#endif
#endif

#ifndef CTX_HEADLESS
#define CTX_HEADLESS 0
#endif

#ifndef CTX_FB
#define CTX_FB 0
#endif

#ifndef CTX_KMS
#define CTX_KMS 0
#endif

#if CTX_SDL
#define ctx_mutex_t            SDL_mutex
#define ctx_create_mutex()     SDL_CreateMutex()
#define ctx_lock_mutex(a)      SDL_LockMutex(a)
#define ctx_unlock_mutex(a)    SDL_UnlockMutex(a)
#else
#define ctx_mutex_t           int
#define ctx_create_mutex()    NULL
#define ctx_lock_mutex(a)   
#define ctx_unlock_mutex(a)  
#endif

#if CTX_CAIRO
#ifndef CAIRO_H
typedef struct _cairo_t cairo_t;
#endif

/* render the deferred commands of a ctx context to a cairo
 * context
 */
void  ctx_render_cairo  (Ctx *ctx, cairo_t *cr);

/* create a ctx context that directly renders to the specified
 * cairo context
 */
Ctx * ctx_new_for_cairo (cairo_t *cr);
#endif

/* free with free() */
char *ctx_render_string (Ctx *ctx, int longform, int *retlen);

void ctx_render_stream  (Ctx *ctx, FILE *stream, int formatter);

void ctx_render_ctx     (Ctx *ctx, Ctx *d_ctx);
void ctx_render_ctx_textures (Ctx *ctx, Ctx *d_ctx); /* cycles through all
                                                        used texture eids
                                                      */

void ctx_start_move     (Ctx *ctx);


int ctx_add_single      (Ctx *ctx, void *entry);

uint32_t ctx_utf8_to_unichar (const char *input);
int      ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);


typedef enum
{
  CTX_FILL_RULE_WINDING = 0,
  CTX_FILL_RULE_EVEN_ODD
} CtxFillRule;

typedef enum
{
#if 0
  CTX_COMPOSITE_SOURCE_OVER      = 0,
  CTX_COMPOSITE_COPY             = 32,
  CTX_COMPOSITE_SOURCE_IN        = 64,
  CTX_COMPOSITE_SOURCE_OUT       = 96,
  CTX_COMPOSITE_SOURCE_ATOP      = 128,
  CTX_COMPOSITE_CLEAR            = 160,

  CTX_COMPOSITE_DESTINATION_OVER = 192,
  CTX_COMPOSITE_DESTINATION      = 224,
  CTX_COMPOSITE_DESTINATION_IN   = 256,
  CTX_COMPOSITE_DESTINATION_OUT  = 288,
  CTX_COMPOSITE_DESTINATION_ATOP = 320,
  CTX_COMPOSITE_XOR              = 352,

  CTX_COMPOSITE_ALL              = (32+64+128+256)
#else
  CTX_COMPOSITE_SOURCE_OVER      =0,
  CTX_COMPOSITE_COPY             ,
  CTX_COMPOSITE_SOURCE_IN        ,
  CTX_COMPOSITE_SOURCE_OUT       ,
  CTX_COMPOSITE_SOURCE_ATOP      ,
  CTX_COMPOSITE_CLEAR            ,

  CTX_COMPOSITE_DESTINATION_OVER ,
  CTX_COMPOSITE_DESTINATION      ,
  CTX_COMPOSITE_DESTINATION_IN   ,
  CTX_COMPOSITE_DESTINATION_OUT  ,
  CTX_COMPOSITE_DESTINATION_ATOP ,
  CTX_COMPOSITE_XOR              ,
#endif
} CtxCompositingMode;

typedef enum
{
  CTX_BLEND_NORMAL,
  CTX_BLEND_MULTIPLY,
  CTX_BLEND_SCREEN,
  CTX_BLEND_OVERLAY,
  CTX_BLEND_DARKEN,
  CTX_BLEND_LIGHTEN,
  CTX_BLEND_COLOR_DODGE,
  CTX_BLEND_COLOR_BURN,
  CTX_BLEND_HARD_LIGHT,
  CTX_BLEND_SOFT_LIGHT,
  CTX_BLEND_DIFFERENCE,
  CTX_BLEND_EXCLUSION,
  CTX_BLEND_HUE, 
  CTX_BLEND_SATURATION, 
  CTX_BLEND_COLOR, 
  CTX_BLEND_LUMINOSITY,  // 15
  CTX_BLEND_DIVIDE,
  CTX_BLEND_ADDITION,
  CTX_BLEND_SUBTRACT,    // 18
} CtxBlend;

void ctx_blend_mode (Ctx *ctx, CtxBlend mode);

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
  CTX_TEXT_ALIGN_START = 0,  // in mrg these didnt exist
  CTX_TEXT_ALIGN_END,        // but left/right did
  CTX_TEXT_ALIGN_JUSTIFY, // not handled in ctx
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

struct
_CtxGlyph
{
  uint32_t index;
  float    x;
  float    y;
};

CtxTextAlign       ctx_get_text_align (Ctx *ctx);
CtxTextBaseline    ctx_get_text_baseline (Ctx *ctx);
CtxTextDirection   ctx_get_text_direction (Ctx *ctx);
CtxFillRule        ctx_get_fill_rule (Ctx *ctx);
CtxLineCap         ctx_get_line_cap (Ctx *ctx);
CtxLineJoin        ctx_get_line_join (Ctx *ctx);
CtxCompositingMode ctx_get_compositing_mode (Ctx *ctx);
CtxBlend           ctx_get_blend_mode (Ctx *ctx);

void ctx_gradient_add_stop_string (Ctx *ctx, float pos, const char *color);

void ctx_text_align           (Ctx *ctx, CtxTextAlign      align);
void ctx_text_baseline        (Ctx *ctx, CtxTextBaseline   baseline);
void ctx_text_direction       (Ctx *ctx, CtxTextDirection  direction);
void ctx_fill_rule            (Ctx *ctx, CtxFillRule       fill_rule);
void ctx_line_cap             (Ctx *ctx, CtxLineCap        cap);
void ctx_line_join            (Ctx *ctx, CtxLineJoin       join);
void ctx_compositing_mode     (Ctx *ctx, CtxCompositingMode mode);
int  ctx_set_drawlist     (Ctx *ctx, void *data, int length);
typedef struct _CtxEntry CtxEntry;
/* we only care about the tight packing for this specific
 * struct as we do indexing across members in arrays of it,
 * to make sure its size becomes 9bytes -
 * the pack pragma is also sufficient on recent gcc versions
 */
#pragma pack(push,1)
struct
  _CtxEntry
{
  uint8_t code;
  union
  {
    float    f[2];
    uint8_t  u8[8];
    int8_t   s8[8];
    uint16_t u16[4];
    int16_t  s16[4];
    uint32_t u32[2];
    int32_t  s32[2];
    uint64_t u64[1]; // unused
  } data; // 9bytes long, we're favoring compactness and correctness
  // over performance. By sacrificing float precision, zeroing
  // first 8bit of f[0] would permit 8bytes long and better
  // aglinment and cacheline behavior.
};
#pragma pack(pop)
const CtxEntry *ctx_get_drawlist (Ctx *ctx);
int  ctx_append_drawlist  (Ctx *ctx, void *data, int length);

/* these are only needed for clients rendering text, as all text gets
 * converted to paths.
 */
void  ctx_glyphs        (Ctx        *ctx,
                         CtxGlyph   *glyphs,
                         int         n_glyphs);

void  ctx_glyphs_stroke (Ctx       *ctx,
                         CtxGlyph   *glyphs,
                         int         n_glyphs);

void  ctx_text          (Ctx        *ctx,
                         const char *string);
void  ctx_text_stroke   (Ctx        *ctx,
                         const char *string);

void  ctx_fill_text     (Ctx        *ctx,
                         const char *string,
                         float       x,
                         float       y);

void  ctx_stroke_text   (Ctx        *ctx,
                         const char *string,
                         float       x,
                         float       y);

/* returns the total horizontal advance if string had been rendered */
float ctx_text_width    (Ctx        *ctx,
                         const char *string);

float ctx_glyph_width   (Ctx *ctx, int unichar);

int   ctx_load_font_ttf (const char *name, const void *ttf_contents, int length);



enum _CtxModifierState
{
  CTX_MODIFIER_STATE_SHIFT   = (1<<0),
  CTX_MODIFIER_STATE_CONTROL = (1<<1),
  CTX_MODIFIER_STATE_ALT     = (1<<2),
  CTX_MODIFIER_STATE_BUTTON1 = (1<<3),
  CTX_MODIFIER_STATE_BUTTON2 = (1<<4),
  CTX_MODIFIER_STATE_BUTTON3 = (1<<5),
  CTX_MODIFIER_STATE_DRAG    = (1<<6), // pointer button is down (0 or any)
};
typedef enum _CtxModifierState CtxModifierState;

enum _CtxScrollDirection
{
  CTX_SCROLL_DIRECTION_UP,
  CTX_SCROLL_DIRECTION_DOWN,
  CTX_SCROLL_DIRECTION_LEFT,
  CTX_SCROLL_DIRECTION_RIGHT
};
typedef enum _CtxScrollDirection CtxScrollDirection;

typedef struct _CtxEvent CtxEvent;

void ctx_set_backend (Ctx *ctx,
                       void *backend);
void *ctx_get_backend (Ctx *ctx);

/* the following API is only available when CTX_EVENTS is defined to 1
 *
 * it provides the ability to register callbacks with the current path
 * that get delivered with transformed coordinates.
 */
int ctx_is_dirty (Ctx *ctx);
void ctx_set_dirty (Ctx *ctx, int dirty);
float ctx_get_float (Ctx *ctx, uint32_t hash);
void ctx_set_float (Ctx *ctx, uint32_t hash, float value);

unsigned long ctx_ticks (void);
void ctx_flush (Ctx *ctx);

void ctx_set_clipboard (Ctx *ctx, const char *text);
char *ctx_get_clipboard (Ctx *ctx);

void _ctx_events_init     (Ctx *ctx);
typedef struct _CtxIntRectangle CtxIntRectangle;
struct _CtxIntRectangle {
  int x;
  int y;
  int width;
  int height;
};

void ctx_quit (Ctx *ctx);
int  ctx_has_quit (Ctx *ctx);

typedef void (*CtxCb) (CtxEvent *event,
                       void     *data,
                       void     *data2);
typedef void (*CtxDestroyNotify) (void *data);

enum _CtxEventType {
  CTX_PRESS        = 1 << 0,
  CTX_MOTION       = 1 << 1,
  CTX_RELEASE      = 1 << 2,
  CTX_ENTER        = 1 << 3,
  CTX_LEAVE        = 1 << 4,
  CTX_TAP          = 1 << 5,
  CTX_TAP_AND_HOLD = 1 << 6,

  /* NYI: SWIPE, ZOOM ROT_ZOOM, */

  CTX_DRAG_PRESS   = 1 << 7,
  CTX_DRAG_MOTION  = 1 << 8,
  CTX_DRAG_RELEASE = 1 << 9,
  CTX_KEY_PRESS    = 1 << 10,
  CTX_KEY_DOWN     = 1 << 11,
  CTX_KEY_UP       = 1 << 12,
  CTX_SCROLL       = 1 << 13,
  CTX_MESSAGE      = 1 << 14,
  CTX_DROP         = 1 << 15,

  CTX_SET_CURSOR   = 1 << 16, // used internally

  /* client should store state - preparing
                                 * for restart
                                 */
  CTX_POINTER  = (CTX_PRESS | CTX_MOTION | CTX_RELEASE | CTX_DROP),
  CTX_TAPS     = (CTX_TAP | CTX_TAP_AND_HOLD),
  CTX_CROSSING = (CTX_ENTER | CTX_LEAVE),
  CTX_DRAG     = (CTX_DRAG_PRESS | CTX_DRAG_MOTION | CTX_DRAG_RELEASE),
  CTX_KEY      = (CTX_KEY_DOWN | CTX_KEY_UP | CTX_KEY_PRESS),
  CTX_MISC     = (CTX_MESSAGE),
  CTX_ANY      = (CTX_POINTER | CTX_DRAG | CTX_CROSSING | CTX_KEY | CTX_MISC | CTX_TAPS),
};
typedef enum _CtxEventType CtxEventType;

#define CTX_CLICK   CTX_PRESS   // SHOULD HAVE MORE LOGIC

struct _CtxEvent {
  CtxEventType  type;
  uint32_t time;
  Ctx     *ctx;
  int stop_propagate; /* when set - propagation is stopped */

  CtxModifierState state;

  int     device_no; /* 0 = left mouse button / virtual focus */
                     /* 1 = middle mouse button */
                     /* 2 = right mouse button */
                     /* 3 = first multi-touch .. (NYI) */

  float   device_x; /* untransformed (device) coordinates  */
  float   device_y;

  /* coordinates; and deltas for motion/drag events in user-coordinates: */
  float   x;
  float   y;
  float   start_x; /* start-coordinates (press) event for drag, */
  float   start_y; /*    untransformed coordinates */
  float   prev_x;  /* previous events coordinates */
  float   prev_y;
  float   delta_x; /* x - prev_x, redundant - but often useful */
  float   delta_y; /* y - prev_y, redundant - ..  */


  unsigned int unicode; /* only valid for key-events, re-use as keycode? */
  const char *string;   /* as key can be "up" "down" "space" "backspace" "a" "b" "ø" etc .. */
                        /* this is also where the message is delivered for
                         * MESSAGE events
                         *
                         * and the data for drop events are delivered
                         */
  CtxScrollDirection scroll_direction;


  // would be nice to add the bounding box of the hit-area causing
  // the event, making for instance scissored enter/leave repaint easier.
};

// layer-event "layer"  motion x y device_no 

void ctx_add_key_binding_full (Ctx *ctx,
                               const char *key,
                               const char *action,
                               const char *label,
                               CtxCb       cb,
                               void       *cb_data,
                               CtxDestroyNotify destroy_notify,
                               void       *destroy_data);
void ctx_add_key_binding (Ctx *ctx,
                          const char *key,
                          const char *action,
                          const char *label,
                          CtxCb cb,
                          void  *cb_data);
typedef struct CtxBinding {
  char *nick;
  char *command;
  char *label;
  CtxCb cb;
  void *cb_data;
  CtxDestroyNotify destroy_notify;
  void  *destroy_data;
} CtxBinding;
CtxBinding *ctx_get_bindings (Ctx *ctx);
void  ctx_clear_bindings     (Ctx *ctx);
void  ctx_remove_idle        (Ctx *ctx, int handle);
int   ctx_add_timeout_full   (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                              void (*destroy_notify)(void *destroy_data), void *destroy_data);
int   ctx_add_timeout        (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data);
int   ctx_add_idle_full      (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                              void (*destroy_notify)(void *destroy_data), void *destroy_data);
int   ctx_add_idle           (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data);


void ctx_add_hit_region (Ctx *ctx, const char *id);

void ctx_listen_full (Ctx     *ctx,
                      float    x,
                      float    y,
                      float    width,
                      float    height,
                      CtxEventType  types,
                      CtxCb    cb,
                      void    *data1,
                      void    *data2,
                      void   (*finalize)(void *listen_data, void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data);
void  ctx_event_stop_propagate (CtxEvent *event);
void  ctx_listen               (Ctx          *ctx,
                                CtxEventType  types,
                                CtxCb         cb,
                                void*         data1,
                                void*         data2);
void  ctx_listen_with_finalize (Ctx          *ctx,
                                CtxEventType  types,
                                CtxCb         cb,
                                void*         data1,
                                void*         data2,
                      void   (*finalize)(void *listen_data, void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data);

void ctx_init (int *argc, char ***argv); // is a no-op but could launch
                                         // terminal
CtxEvent *ctx_get_event (Ctx *ctx);
int       ctx_has_event (Ctx *ctx, int timeout);
void      ctx_get_event_fds (Ctx *ctx, int *fd, int *count);

int   ctx_pointer_is_down (Ctx *ctx, int no);
float ctx_pointer_x (Ctx *ctx);
float ctx_pointer_y (Ctx *ctx);
void  ctx_freeze (Ctx *ctx);
void  ctx_thaw   (Ctx *ctx);
int   ctx_events_frozen (Ctx *ctx);
void  ctx_events_clear_items (Ctx *ctx);
int   ctx_events_width (Ctx *ctx);
int   ctx_events_height (Ctx *ctx);

/* The following functions drive the event delivery, registered callbacks
 * are called in response to these being called.
 */

int ctx_key_down  (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time);
int ctx_key_up    (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time);
int ctx_key_press (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time);


int ctx_scrolled  (Ctx *ctx, float x, float y, CtxScrollDirection scroll_direction, uint32_t time);
void ctx_incoming_message (Ctx *ctx, const char *message, long time);
int ctx_pointer_motion    (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_release   (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_press     (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_drop      (Ctx *ctx, float x, float y, int device_no, uint32_t time,
                           char *string);

typedef enum
{
  CTX_CONT             = '\0', // - contains args from preceding entry
  CTX_NOP              = ' ', //
  CTX_DATA             = '(', // size size-in-entries - u32
  CTX_DATA_REV         = ')', // reverse traversal data marker
  CTX_SET_RGBA_U8      = '*', // r g b a - u8
  CTX_NEW_EDGE         = '+', // x0 y0 x1 y1 - s16
  // set pixel might want a shorter ascii form? or keep it an embedded
  // only option?
  CTX_SET_PIXEL        = '-', // 8bit "fast-path" r g b a x y - u8 for rgba, and u16 for x,y
  /* optimizations that reduce the number of entries used,
   * not visible outside the drawlist compression, thus
   * using entries that cannot be used directly as commands
   * since they would be interpreted as numbers - if values>127
   * then the embedded font data is harder to escape.
   */
  CTX_REL_LINE_TO_X4            = '0', // x1 y1 x2 y2 x3 y3 x4 y4   -- s8
  CTX_REL_LINE_TO_REL_CURVE_TO  = '1', // x1 y1 cx1 cy1 cx2 cy2 x y -- s8
  CTX_REL_CURVE_TO_REL_LINE_TO  = '2', // cx1 cy1 cx2 cy2 x y x1 y1 -- s8
  CTX_REL_CURVE_TO_REL_MOVE_TO  = '3', // cx1 cy1 cx2 cy2 x y x1 y1 -- s8
  CTX_REL_LINE_TO_X2            = '4', // x1 y1 x2 y2 -- s16
  CTX_MOVE_TO_REL_LINE_TO       = '5', // x1 y1 x2 y2 -- s16
  CTX_REL_LINE_TO_REL_MOVE_TO   = '6', // x1 y1 x2 y2 -- s16
  CTX_FILL_MOVE_TO              = '7', // x y
  CTX_REL_QUAD_TO_REL_QUAD_TO   = '8', // cx1 x1 cy1 y1 cx1 x2 cy1 y1 -- s8
  CTX_REL_QUAD_TO_S16           = '9', // cx1 cy1 x y                 - s16
  // expand with: . : 
  CTX_FLUSH            = ';',

  CTX_DEFINE_GLYPH     = '@', // unichar width - u32
  CTX_ARC_TO           = 'A', // x1 y1 x2 y2 radius
  CTX_ARC              = 'B', // x y radius angle1 angle2 direction
  CTX_CURVE_TO         = 'C', // cx1 cy1 cx2 cy2 x y
  CTX_STROKE           = 'E', //
  CTX_FILL             = 'F', //
  CTX_RESTORE          = 'G', //
  CTX_HOR_LINE_TO      = 'H', // x
  CTX_DEFINE_TEXTURE   = 'I', // "eid" width height format "data"
  CTX_ROTATE           = 'J', // radians
  CTX_COLOR            = 'K', // model, c1 c2 c3 ca - has a variable set of
  // arguments.
  CTX_LINE_TO          = 'L', // x y
  CTX_MOVE_TO          = 'M', // x y
  CTX_BEGIN_PATH       = 'N', //
  CTX_SCALE            = 'O', // xscale yscale
  CTX_NEW_PAGE         = 'P', // - NYI - optional page-size
  CTX_QUAD_TO          = 'Q', // cx cy x y
  CTX_VIEW_BOX         = 'R', // x y width height
  CTX_SMOOTH_TO        = 'S', // cx cy x y
  CTX_SMOOTHQ_TO       = 'T', // x y
  CTX_RESET            = 'U', //
  CTX_VER_LINE_TO      = 'V', // y
  CTX_APPLY_TRANSFORM  = 'W', // a b c d e f - for set_transform combine with identity
  CTX_EXIT             = 'X', //
  CTX_ROUND_RECTANGLE  = 'Y', // x y width height radius

  CTX_CLOSE_PATH2      = 'Z', //
  CTX_STROKE_SOURCE    = '_', // next source definition applies to strokes
  CTX_KERNING_PAIR     = '[', // glA glB kerning, glA and glB in u16 kerning in s32
  CTX_COLOR_SPACE      = ']', // IccSlot  data  data_len,
                         //    data can be a string with a name,
                         //    icc data or perhaps our own serialization
                         //    of profile data
  CTX_REL_ARC_TO       = 'a', // x1 y1 x2 y2 radius
  CTX_CLIP             = 'b',
  CTX_REL_CURVE_TO     = 'c', // cx1 cy1 cx2 cy2 x y
  CTX_LINE_DASH        = 'd', // dashlen0 [dashlen1 ...]
  CTX_TRANSLATE        = 'e', // x y
  CTX_LINEAR_GRADIENT  = 'f', // x1 y1 x2 y2
  CTX_SAVE             = 'g',
  CTX_REL_HOR_LINE_TO  = 'h', // x
  CTX_TEXTURE          = 'i',
  CTX_PRESERVE         = 'j', // 
  CTX_SET_KEY          = 'k', // - used together with another char to identify
                              //   a key to set
  CTX_REL_LINE_TO      = 'l', // x y
  CTX_REL_MOVE_TO      = 'm', // x y
  CTX_FONT             = 'n', // as used by text parser
  CTX_RADIAL_GRADIENT  = 'o', // x1 y1 radius1 x2 y2 radius2
  CTX_GRADIENT_STOP    = 'p', // argument count depends on current color model
  CTX_REL_QUAD_TO      = 'q', // cx cy x y
  CTX_RECTANGLE        = 'r', // x y width height
  CTX_REL_SMOOTH_TO    = 's', // cx cy x y
  CTX_REL_SMOOTHQ_TO   = 't', // x y
  CTX_STROKE_TEXT      = 'u', // string - utf8 string
  CTX_REL_VER_LINE_TO  = 'v', // y
  CTX_GLYPH            = 'w', // unichar fontsize
  CTX_TEXT             = 'x', // string | kern - utf8 data to shape or horizontal kerning amount
  CTX_IDENTITY         = 'y', //
  CTX_CLOSE_PATH       = 'z', //
  CTX_START_GROUP      = '{',
  CTX_END_GROUP        = '}',
  CTX_SOURCE_TRANSFORM = '`',

  CTX_EDGE             = '&',                        // will not occur in commandstream
  CTX_EDGE_FLIPPED     = '^', // x0 y0 x1 y1 - s16   // thus these use reserved entries as code

  /* though expressed as two chars in serialization we have
   * dedicated byte commands for the setters to keep the dispatch
   * simpler. There is no need for these to be human readable thus we go >128
   *
   * unused:        !&<=>?:.=/\`,
   * reserved:      '"&   #  %^@
   */


  CTX_FILL_RULE        = 128, // kr rule - u8, default = CTX_FILLE_RULE_EVEN_ODD
  CTX_BLEND_MODE       = 129, // kB mode - u8 , default=0

  CTX_MITER_LIMIT      = 130, // km limit - float, default = 0.0

  CTX_LINE_JOIN        = 131, // kj join - u8 , default=0
  CTX_LINE_CAP         = 132, // kc cap - u8, default = 0
  CTX_LINE_WIDTH       = 133, // kw width, default = 2.0
  CTX_GLOBAL_ALPHA     = 134, // ka alpha - default=1.0
  CTX_COMPOSITING_MODE = 135, // kc mode - u8 , default=0

  CTX_FONT_SIZE        = 136, // kf size - float, default=?
  CTX_TEXT_ALIGN       = 137, // kt align - u8, default = CTX_TEXT_ALIGN_START
  CTX_TEXT_BASELINE    = 138, // kb baseline - u8, default = CTX_TEXT_ALIGN_ALPHABETIC
  CTX_TEXT_DIRECTION   = 139, // kd

  CTX_SHADOW_BLUR      = 140, // ks
  CTX_SHADOW_COLOR     = 141, // kC
  CTX_SHADOW_OFFSET_X  = 142, // kx
  CTX_SHADOW_OFFSET_Y  = 143, // ky
  CTX_IMAGE_SMOOTHING  = 144, // kS
  CTX_LINE_DASH_OFFSET = 145, // kD lineDashOffset

  // items marked with % are currently only for the parser
  // for instance for svg compatibility or simulated/converted color spaces
  // not the serialization/internal render stream
  //
  CTX_STROKE_RECT      = 200, // strokeRect - only exist in long form
  CTX_FILL_RECT        = 201, // fillRect   - only exist in long form
} CtxCode;


#pragma pack(push,1)

typedef struct _CtxCommand CtxCommand;
typedef struct _CtxIterator CtxIterator;

CtxIterator *
ctx_current_path (Ctx *ctx);
void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2);

#define CTX_ASSERT               0

#if CTX_ASSERT==1
#define ctx_assert(a)  if(!(a)){fprintf(stderr,"%s:%i assertion failed\n", __FUNCTION__, __LINE__);  }
#else
#define ctx_assert(a)
#endif

int ctx_get_drawlist_count (Ctx *ctx);

struct
  _CtxCommand
{
  union
  {
    uint8_t  code;
    CtxEntry entry;
    struct
    {
      uint8_t code;
      float scalex;
      float scaley;
    } scale;
    struct
    {
      uint8_t code;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t cont;
      uint8_t data[8]; /* ... and continues */
    } data;
    struct
    {
      uint8_t code;
      uint32_t stringlen;
      uint32_t blocklen;
    } data_rev;
    struct
    {
      uint8_t code;
      float pad;
      float pad2;
      uint8_t code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t code_cont;
      uint8_t utf8[8]; /* .. and continues */
    } text;
    struct
    {
      uint8_t  code;
      uint32_t key_hash;
      float    pad;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } set;
    struct
    {
      uint8_t  code;
      uint32_t pad0;
      float    pad1;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } get;
    struct {
      uint8_t  code;
      uint32_t count; /* better than byte_len in code, but needs to then be set   */
      float    pad1;
      uint8_t  code_data;
      uint32_t byte_len;
      uint32_t blocklen;
      uint8_t  code_cont;
      float    data[2]; /* .. and - possibly continues */
    } line_dash;
    struct {
      uint8_t  code;
      uint32_t space_slot;
      float    pad1;
      uint8_t  code_data;
      uint32_t data_len;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  data[8]; /* .. and continues */
    } colorspace;
    struct
    {
      uint8_t  code;
      float    x;
      float    y;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      char     eid[8]; /* .. and continues */
    } texture;
    struct
    {
      uint8_t  code;
      uint32_t width;
      uint32_t height;
      uint8_t  code_cont0;
      uint16_t format;
      uint16_t pad0;
      uint32_t pad1;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont1;
      char     eid[8]; /* .. and continues */
      // followed by - in variable offset code_Data, data_len, datablock_len, cont, pixeldata
    } define_texture;
    struct
    {
      uint8_t  code;
      float    pad;
      float    pad2;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } text_stroke;
    struct
    {
      uint8_t  code;
      float    pad;
      float    pad2;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } set_font;
    struct
    {
      uint8_t code;
      float model;
      float r;
      uint8_t pad1;
      float g;
      float b;
      uint8_t pad2;
      float a;
    } rgba;
    struct
    {
      uint8_t code;
      float model;
      float c;
      uint8_t pad1;
      float m;
      float y;
      uint8_t pad2;
      float k;
      float a;
    } cmyka;
    struct
    {
      uint8_t code;
      float model;
      float g;
      uint8_t pad1;
      float a;
    } graya;

    struct
    {
      uint8_t code;
      float model;
      float c0;
      uint8_t pad1;
      float c1;
      float c2;
      uint8_t pad2;
      float c3;
      float c4;
      uint8_t pad3;
      float c5;
      float c6;
      uint8_t pad4;
      float c7;
      float c8;
      uint8_t pad5;
      float c9;
      float c10;
    } set_color;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } rel_move_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } rel_line_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } line_to;
    struct
    {
      uint8_t code;
      float cx1;
      float cy1;
      uint8_t pad0;
      float cx2;
      float cy2;
      uint8_t pad1;
      float x;
      float y;
    } rel_curve_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } move_to;
    struct
    {
      uint8_t code;
      float cx1;
      float cy1;
      uint8_t pad0;
      float cx2;
      float cy2;
      uint8_t pad1;
      float x;
      float y;
    } curve_to;
    struct
    {
      uint8_t code;
      float x1;
      float y1;
      uint8_t pad0;
      float r1;
      float x2;
      uint8_t pad1;
      float y2;
      float r2;
    } radial_gradient;
    struct
    {
      uint8_t code;
      float x1;
      float y1;
      uint8_t pad0;
      float x2;
      float y2;
    } linear_gradient;
    struct
    {
      uint8_t code;
      float x;
      float y;
      uint8_t pad0;
      float width;
      float height;
      uint8_t pad1;
      float radius;
    } rectangle;
    struct {
      uint8_t code;
      float x;
      float y;
      uint8_t pad0;
      float width;
      float height;
    } view_box;

    struct
    {
      uint8_t code;
      uint16_t glyph_before;
      uint16_t glyph_after;
       int32_t amount;
    } kern;

    struct
    {
      uint8_t code;
      uint32_t glyph;
      uint32_t advance; // * 256
    } define_glyph;

    struct
    {
      uint8_t code;
      uint8_t rgba[4];
      uint16_t x;
      uint16_t y;
    } set_pixel;
    struct
    {
      uint8_t code;
      float cx;
      float cy;
      uint8_t pad0;
      float x;
      float y;
    } quad_to;
    struct
    {
      uint8_t code;
      float cx;
      float cy;
      uint8_t pad0;
      float x;
      float y;
    } rel_quad_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
      uint8_t pad0;
      float radius;
      float angle1;
      uint8_t pad1;
      float angle2;
      float direction;
    }
    arc;
    struct
    {
      uint8_t code;
      float x1;
      float y1;
      uint8_t pad0;
      float x2;
      float y2;
      uint8_t pad1;
      float radius;
    }
    arc_to;
    /* some format specific generic accesors:  */
    struct
    {
      uint8_t code;
      float   x0;
      float   y0;
      uint8_t pad0;
      float   x1;
      float   y1;
      uint8_t pad1;
      float   x2;
      float   y2;
      uint8_t pad2;
      float   x3;
      float   y3;
      uint8_t pad3;
      float   x4;
      float   y4;
    } c;
    struct
    {
      uint8_t code;
      float   a0;
      float   a1;
      uint8_t pad0;
      float   a2;
      float   a3;
      uint8_t pad1;
      float   a4;
      float   a5;
      uint8_t pad2;
      float   a6;
      float   a7;
      uint8_t pad3;
      float   a8;
      float   a9;
    } f;
    struct
    {
      uint8_t  code;
      uint32_t a0;
      uint32_t a1;
      uint8_t  pad0;
      uint32_t a2;
      uint32_t a3;
      uint8_t  pad1;
      uint32_t a4;
      uint32_t a5;
      uint8_t  pad2;
      uint32_t a6;
      uint32_t a7;
      uint8_t  pad3;
      uint32_t a8;
      uint32_t a9;
    } u32;
    struct
    {
      uint8_t  code;
      uint64_t a0;
      uint8_t  pad0;
      uint64_t a1;
      uint8_t  pad1;
      uint64_t a2;
      uint8_t  pad2;
      uint64_t a3;
      uint8_t  pad3;
      uint64_t a4;
    } u64;
    struct
    {
      uint8_t code;
      int32_t a0;
      int32_t a1;
      uint8_t pad0;
      int32_t a2;
      int32_t a3;
      uint8_t pad1;
      int32_t a4;
      int32_t a5;
      uint8_t pad2;
      int32_t a6;
      int32_t a7;
      uint8_t pad3;
      int32_t a8;
      int32_t a9;
    } s32;
    struct
    {
      uint8_t code;
      int16_t a0;
      int16_t a1;
      int16_t a2;
      int16_t a3;
      uint8_t pad0;
      int16_t a4;
      int16_t a5;
      int16_t a6;
      int16_t a7;
      uint8_t pad1;
      int16_t a8;
      int16_t a9;
      int16_t a10;
      int16_t a11;
      uint8_t pad2;
      int16_t a12;
      int16_t a13;
      int16_t a14;
      int16_t a15;
      uint8_t pad3;
      int16_t a16;
      int16_t a17;
      int16_t a18;
      int16_t a19;
    } s16;
    struct
    {
      uint8_t code;
      uint16_t a0;
      uint16_t a1;
      uint16_t a2;
      uint16_t a3;
      uint8_t pad0;
      uint16_t a4;
      uint16_t a5;
      uint16_t a6;
      uint16_t a7;
      uint8_t pad1;
      uint16_t a8;
      uint16_t a9;
      uint16_t a10;
      uint16_t a11;
      uint8_t pad2;
      uint16_t a12;
      uint16_t a13;
      uint16_t a14;
      uint16_t a15;
      uint8_t pad3;
      uint16_t a16;
      uint16_t a17;
      uint16_t a18;
      uint16_t a19;
    } u16;
    struct
    {
      uint8_t code;
      uint8_t a0;
      uint8_t a1;
      uint8_t a2;
      uint8_t a3;
      uint8_t a4;
      uint8_t a5;
      uint8_t a6;
      uint8_t a7;
      uint8_t pad0;
      uint8_t a8;
      uint8_t a9;
      uint8_t a10;
      uint8_t a11;
      uint8_t a12;
      uint8_t a13;
      uint8_t a14;
      uint8_t a15;
      uint8_t pad1;
      uint8_t a16;
      uint8_t a17;
      uint8_t a18;
      uint8_t a19;
      uint8_t a20;
      uint8_t a21;
      uint8_t a22;
      uint8_t a23;
    } u8;
    struct
    {
      uint8_t code;
      int8_t a0;
      int8_t a1;
      int8_t a2;
      int8_t a3;
      int8_t a4;
      int8_t a5;
      int8_t a6;
      int8_t a7;
      uint8_t pad0;
      int8_t a8;
      int8_t a9;
      int8_t a10;
      int8_t a11;
      int8_t a12;
      int8_t a13;
      int8_t a14;
      int8_t a15;
      uint8_t pad1;
      int8_t a16;
      int8_t a17;
      int8_t a18;
      int8_t a19;
      int8_t a20;
      int8_t a21;
      int8_t a22;
      int8_t a23;
    } s8;
  };
  CtxEntry next_entry; // also pads size of CtxCommand slightly.
};

typedef struct _CtxBackend CtxBackend;
void ctx_windowtitle (Ctx *ctx, const char *text);
struct _CtxBackend
{
  Ctx  *ctx;
  const char *backend;
  void (*process)        (void *backend, CtxCommand *entry);
  void (*reset)          (void *backend);
  void (*flush)          (void *backend);

  char *(*get_clipboard)   (void *ctxctx);
  void (*set_clipboard)    (void *ctxctx, const char *text);
  void (*set_windowtitle)  (void *ctxctx, const char *text);

  char *(*get_event)      (void *backend, int timout_ms);
  int   (*has_event)      (void *backend, int timout_ms);

  void        (*consume_events) (void *ctx);
  void        (*get_event_fds)  (void *ctx);

  void (*free)           (void *backend);
};

CtxCommand *ctx_iterator_next (CtxIterator *iterator);

#define ctx_arg_string()  ((char*)&entry[2].data.u8[0])


/* The above should be public API
 */

#pragma pack(pop)

/* access macros for nth argument of a given type when packed into
 * an CtxEntry pointer in current code context
 */
#define ctx_arg_float(no) entry[(no)>>1].data.f[(no)&1]
#define ctx_arg_u64(no)   entry[(no)].data.u64[0]
#define ctx_arg_u32(no)   entry[(no)>>1].data.u32[(no)&1]
#define ctx_arg_s32(no)   entry[(no)>>1].data.s32[(no)&1]
#define ctx_arg_u16(no)   entry[(no)>>2].data.u16[(no)&3]
#define ctx_arg_s16(no)   entry[(no)>>2].data.s16[(no)&3]
#define ctx_arg_u8(no)    entry[(no)>>3].data.u8[(no)&7]
#define ctx_arg_s8(no)    entry[(no)>>3].data.s8[(no)&7]
#define ctx_arg_string()  ((char*)&entry[2].data.u8[0])

typedef enum
{
  CTX_GRAY           = 1,
  CTX_RGB            = 3,
  CTX_DRGB           = 4,
  CTX_CMYK           = 5,
  CTX_DCMYK          = 6,
  CTX_LAB            = 7,
  CTX_LCH            = 8,
  CTX_GRAYA          = 101,
  CTX_RGBA           = 103,
  CTX_DRGBA          = 104,
  CTX_CMYKA          = 105,
  CTX_DCMYKA         = 106,
  CTX_LABA           = 107,
  CTX_LCHA           = 108,
  CTX_GRAYA_A        = 201,
  CTX_RGBA_A         = 203,
  CTX_RGBA_A_DEVICE  = 204,
  CTX_CMYKA_A        = 205,
  CTX_DCMYKA_A       = 206,
  // RGB  device and  RGB  ?
} CtxColorModel;

enum _CtxAntialias
{
  CTX_ANTIALIAS_DEFAULT, //
  CTX_ANTIALIAS_NONE, // non-antialiased
  CTX_ANTIALIAS_FAST, // aa 3    // deprected or is default equal to this now?
  CTX_ANTIALIAS_GOOD, // aa 5    // this should perhaps still be 5?
};
typedef enum _CtxAntialias CtxAntialias;

enum _CtxCursor
{
  CTX_CURSOR_UNSET,
  CTX_CURSOR_NONE,
  CTX_CURSOR_ARROW,
  CTX_CURSOR_IBEAM,
  CTX_CURSOR_WAIT,
  CTX_CURSOR_HAND,
  CTX_CURSOR_CROSSHAIR,
  CTX_CURSOR_RESIZE_ALL,
  CTX_CURSOR_RESIZE_N,
  CTX_CURSOR_RESIZE_S,
  CTX_CURSOR_RESIZE_E,
  CTX_CURSOR_RESIZE_NE,
  CTX_CURSOR_RESIZE_SE,
  CTX_CURSOR_RESIZE_W,
  CTX_CURSOR_RESIZE_NW,
  CTX_CURSOR_RESIZE_SW,
  CTX_CURSOR_MOVE
};
typedef enum _CtxCursor CtxCursor;

/* to be used immediately after a ctx_listen or ctx_listen_full causing the
 * cursor to change when hovering the listen area.
 */
void ctx_listen_set_cursor (Ctx      *ctx,
                            CtxCursor cursor);

/* lower level cursor setting that is independent of ctx event handling
 */
void         ctx_set_cursor (Ctx *ctx, CtxCursor cursor);
CtxCursor    ctx_get_cursor (Ctx *ctx);
void         ctx_set_antialias (Ctx *ctx, CtxAntialias antialias);
CtxAntialias ctx_get_antialias (Ctx *ctx);
void         ctx_set_render_threads   (Ctx *ctx, int n_threads);
int          ctx_get_render_threads   (Ctx *ctx);

void         ctx_set_hash_cache (Ctx *ctx, int enable_hash_cache);
int          ctx_get_hash_cache (Ctx *ctx);


typedef struct _CtxParser CtxParser;
  CtxParser *ctx_parser_new (
  Ctx       *ctx,
  int        width,
  int        height,
  float      cell_width,
  float      cell_height,
  int        cursor_x,
  int        cursor_y,
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
  void  *prop_data,
  void (*exit) (void *exit_data),
  void *exit_data);


enum _CtxColorSpace
{
  CTX_COLOR_SPACE_DEVICE_RGB,
  CTX_COLOR_SPACE_DEVICE_CMYK,
  CTX_COLOR_SPACE_USER_RGB,
  CTX_COLOR_SPACE_USER_CMYK,
  CTX_COLOR_SPACE_TEXTURE
};
typedef enum _CtxColorSpace CtxColorSpace;

/* sets the color space for a slot, the space is either a string of
 * "sRGB" "rec2020" .. etc or an icc profile.
 *
 * The slots device_rgb and device_cmyk is mostly to be handled outside drawing 
 * code, and user_rgb and user_cmyk is to be used. With no user_cmyk set
 * user_cmyk == device_cmyk.
 *
 * The set profiles follows the graphics state.
 */
void ctx_colorspace (Ctx           *ctx,
                     CtxColorSpace  space_slot,
                     unsigned char *data,
                     int            data_length);

void
ctx_parser_set_size (CtxParser *parser,
                     int        width,
                     int        height,
                     float      cell_width,
                     float      cell_height);

void ctx_parser_feed_bytes (CtxParser *parser, const char *data, int count);

int
ctx_get_contents (const char     *path,
                   unsigned char **contents,
                   long           *length);
int
ctx_get_contents2 (const char     *path,
                   unsigned char **contents,
                   long           *length,
                   long            max_len);

void ctx_parser_free (CtxParser *parser);
typedef struct _CtxSHA1 CtxSHA1;

void
ctx_bin2base64 (const void *bin,
                int         bin_length,
                char       *ascii);
int
ctx_base642bin (const char    *ascii,
                int           *length,
                unsigned char *bin);


struct
  _CtxMatrix
{
  float m[3][2];
};

void ctx_apply_matrix (Ctx *ctx, CtxMatrix *matrix);
void ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y);
void ctx_matrix_invert (CtxMatrix *m);
void ctx_matrix_identity (CtxMatrix *matrix);
void ctx_matrix_scale (CtxMatrix *matrix, float x, float y);
void ctx_matrix_rotate (CtxMatrix *matrix, float angle);
void ctx_matrix_multiply (CtxMatrix       *result,
                          const CtxMatrix *t,
                          const CtxMatrix *s);


/* we already have the start of the file available which disambiguates some
 * of our important supported formats, give preference to magic, then extension
 * then text plain vs binary.
 */
const char *ctx_guess_media_type (const char *path, const char *content, int len);

/* get media-type, with preference towards using extension of path and
 * not reading the data at all.
 */
const char *ctx_path_get_media_type (const char *path);

typedef enum {
  CTX_MEDIA_TYPE_NONE=0,
  CTX_MEDIA_TYPE_TEXT,
  CTX_MEDIA_TYPE_IMAGE,
  CTX_MEDIA_TYPE_VIDEO,
  CTX_MEDIA_TYPE_AUDIO,
  CTX_MEDIA_TYPE_INODE,
  CTX_MEDIA_TYPE_APPLICATION,
} CtxMediaTypeClass;

CtxMediaTypeClass ctx_media_type_class (const char *media_type);


float ctx_term_get_cell_width (Ctx *ctx);
float ctx_term_get_cell_height (Ctx *ctx);





#if 1 // CTX_VT

typedef struct _VT VT;
void vt_feed_keystring    (VT *vt, CtxEvent *event, const char *str);
void vt_paste             (VT *vt, const char *str);
char *vt_get_selection    (VT *vt);
long vt_rev               (VT *vt);
int  vt_has_blink         (VT *vt);
int ctx_vt_had_alt_screen (VT *vt);

int  ctx_clients_need_redraw   (Ctx *ctx);
void ctx_clients_handle_events (Ctx *ctx);


typedef struct _CtxBuffer CtxBuffer;
CtxBuffer *ctx_buffer_new_for_data (void *data, int width, int height,
                                    int stride,
                                    CtxPixelFormat pixel_format,
                                    void (*freefunc) (void *pixels, void *user_data),
                                    void *user_data);

typedef enum CtxBackendType {
  CTX_BACKEND_NONE,
  CTX_BACKEND_CTX,
  CTX_BACKEND_HEADLESS,
  CTX_BACKEND_TERM,
  CTX_BACKEND_FB,
  CTX_BACKEND_KMS,
  CTX_BACKEND_TERMIMG,
  CTX_BACKEND_CAIRO,
  CTX_BACKEND_SDL,
} CtxBackendType;

CtxBackendType ctx_backend_type (Ctx *ctx);

static inline int ctx_backend_is_tiled (Ctx *ctx)
{
  switch (ctx_backend_type (ctx))
  {
    case CTX_BACKEND_FB:
    case CTX_BACKEND_SDL:
    case CTX_BACKEND_KMS:
    case CTX_BACKEND_HEADLESS:
      return 1;
    default:
      return 0;
  }
}

#endif


#ifndef CTX_CODEC_CHAR
//#define CTX_CODEC_CHAR '\035'
//#define CTX_CODEC_CHAR 'a'
#define CTX_CODEC_CHAR '\020' // datalink escape
//#define CTX_CODEC_CHAR '^'
#endif

#ifndef assert
#define assert(a)
#endif

#ifdef __cplusplus
}
#endif
#endif
