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
 * Ctx does not - yet - contain a minimal default fallback font, so
 * you probably want to also include a font, and perhaps enable
 * the cairo or SDL2 optional renderers, a more complete example
 * could be:
 *
 * #include <cairo.h>
 * #include <SDL.h>
 * #include "ctx-font-regular.h"
 * #define CTX_IMPLEMENTATION
 * #include "ctx.h"
 *
 * The behavior of ctx can be tweaked, and additional features can
 * be enabled with other includes, see further down in the start
 * of this file for details.
 */

#ifndef CTX_H
#define CTX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct _Ctx            Ctx;

/* The pixel formats supported as render targets
 */
enum _CtxPixelFormat
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
  CTX_FORMAT_GRAYAF,
  CTX_FORMAT_GRAY1,
  CTX_FORMAT_GRAY2,
  CTX_FORMAT_GRAY4,
  CTX_FORMAT_CMYK8,
  CTX_FORMAT_CMYKA8,
  CTX_FORMAT_CMYKAF,
  CTX_FORMAT_DEVICEN1,
  CTX_FORMAT_DEVICEN2,
  CTX_FORMAT_DEVICEN3,
  CTX_FORMAT_DEVICEN4,
  CTX_FORMAT_DEVICEN5,
  CTX_FORMAT_DEVICEN6,
  CTX_FORMAT_DEVICEN7,
  CTX_FORMAT_DEVICEN8,
  CTX_FORMAT_DEVICEN9,
  CTX_FORMAT_DEVICEN10,
  CTX_FORMAT_DEVICEN11,
  CTX_FORMAT_DEVICEN12,
  CTX_FORMAT_DEVICEN13,
  CTX_FORMAT_DEVICEN14,
  CTX_FORMAT_DEVICEN15,
  CTX_FORMAT_DEVICEN16
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
 */
Ctx *ctx_new_ui (int width, int height);

/**
 * ctx_new_for_renderstream:
 *
 * Create a new drawing context for a pre-existing renderstream.
 */
Ctx *ctx_new_for_renderstream (void *data, size_t length);


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

#define CTX_LINE_WIDTH_HAIRLINE -1000.0
#define CTX_LINE_WIDTH_ALIASED  -1.0
#define CTX_LINE_WIDTH_FAST     -1.0  /* aliased 1px wide line */
void ctx_miter_limit (Ctx *ctx, float limit);
void ctx_line_width       (Ctx *ctx, float x);
void ctx_apply_transform  (Ctx *ctx, float a,  float b,  // hscale, hskew
                           float c,  float d,  // vskew,  vscale
                           float e,  float f); // htran,  vtran

void  ctx_font_size       (Ctx *ctx, float x);
void  ctx_font            (Ctx *ctx, const char *font);
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
float ctx_get_line_width  (Ctx *ctx);
int   ctx_width           (Ctx *ctx);
int   ctx_height          (Ctx *ctx);
int   ctx_rev             (Ctx *ctx);
float ctx_x               (Ctx *ctx);
float ctx_y               (Ctx *ctx);
void  ctx_current_point   (Ctx *ctx, float *x, float *y);
void  ctx_get_transform   (Ctx *ctx, float *a, float *b,
                           float *c, float *d,
                           float *e, float *f);

CtxGlyph *ctx_glyph_allocate (int n_glyphs);

void gtx_glyph_free       (CtxGlyph *glyphs);

int  ctx_glyph            (Ctx *ctx, uint32_t unichar, int stroke);

void ctx_arc              (Ctx  *ctx,
                           float x, float y,
                           float radius,
                           float angle1, float angle2,
                           int   direction);

void ctx_arc_to           (Ctx *ctx, float x1, float y1,
                           float x2, float y2, float radius);

void ctx_quad_to          (Ctx *ctx, float cx, float cy,
                           float x, float y);

void ctx_arc              (Ctx  *ctx,
                           float x, float y,
                           float radius,
                           float angle1, float angle2,
                           int   direction);

void ctx_arc_to           (Ctx *ctx, float x1, float y1,
                           float x2, float y2, float radius);

void ctx_preserve         (Ctx *ctx);
void ctx_fill             (Ctx *ctx);
void ctx_stroke           (Ctx *ctx);
void ctx_paint            (Ctx *ctx);
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

void ctx_rgba   (Ctx *ctx, float r, float g, float b, float a);
void ctx_rgb    (Ctx *ctx, float r, float g, float b);
void ctx_gray   (Ctx *ctx, float gray);
void ctx_rgba8  (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
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

/*ctx_texture_init:
 *
 * return value: the actual id assigned, if id is out of range - or later
 * when -1 as id will mean auto-assign.
 */
int ctx_texture_init (Ctx *ctx, int id, int width, int height, int bpp,
                      uint8_t *pixels,
                      void (*freefunc) (void *pixels, void *user_data),
                      void *user_data);
int ctx_texture_load        (Ctx *ctx, int id, const char *path);
int ctx_texture_load_memory (Ctx *ctx, int id, const char *data, int length);
void ctx_texture_release    (Ctx *ctx, int id);
void ctx_texture            (Ctx *ctx, int id, float x, float y);

void ctx_image_path (Ctx *ctx, const char *path, float x, float y);

typedef struct _CtxRenderstream CtxRenderstream;
typedef void (*CtxFullCb) (CtxRenderstream *renderstream, void *data);

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
#define CTX_CAIRO 1
#else
#define CTX_CAIRO 0
#endif

#ifdef SDL_h_
#define CTX_SDL 1
#else
#define CTX_SDL 0
#endif

#ifndef CTX_FB
#if CTX_SDL
#define CTX_FB 1
#else
#define CTX_FB 0
#endif
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

/* render the deferred commands of a ctx context to a cairo
 * context
 */
void  ctx_render_cairo  (Ctx *ctx, cairo_t *cr);

/* create a ctx context that directly renders to the specified
 * cairo context
 */
Ctx * ctx_new_for_cairo (cairo_t *cr);
#endif

void ctx_render_stream  (Ctx *ctx, FILE *stream, int formatter);

void ctx_render_ctx     (Ctx *ctx, Ctx *d_ctx);

void ctx_start_move     (Ctx *ctx);


int ctx_add_single      (Ctx *ctx, void *entry);

uint32_t ctx_utf8_to_unichar (const char *input);
int      ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);


typedef enum
{
  CTX_FILL_RULE_EVEN_ODD,
  CTX_FILL_RULE_WINDING
} CtxFillRule;

typedef enum
{
  CTX_COMPOSITE_SOURCE_OVER,
  CTX_COMPOSITE_COPY,
  CTX_COMPOSITE_SOURCE_IN,
  CTX_COMPOSITE_SOURCE_OUT,
  CTX_COMPOSITE_SOURCE_ATOP,
  CTX_COMPOSITE_CLEAR,

  CTX_COMPOSITE_DESTINATION_OVER,
  CTX_COMPOSITE_DESTINATION,
  CTX_COMPOSITE_DESTINATION_IN,
  CTX_COMPOSITE_DESTINATION_OUT,
  CTX_COMPOSITE_DESTINATION_ATOP,
  CTX_COMPOSITE_XOR,
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

struct
_CtxGlyph
{
  uint32_t index;
  float    x;
  float    y;
};

void ctx_text_align           (Ctx *ctx, CtxTextAlign      align);
void ctx_text_baseline        (Ctx *ctx, CtxTextBaseline   baseline);
void ctx_text_direction       (Ctx *ctx, CtxTextDirection  direction);
void ctx_fill_rule            (Ctx *ctx, CtxFillRule       fill_rule);
void ctx_line_cap             (Ctx *ctx, CtxLineCap        cap);
void ctx_line_join            (Ctx *ctx, CtxLineJoin       join);
void ctx_compositing_mode     (Ctx *ctx, CtxCompositingMode mode);
int  ctx_set_renderstream     (Ctx *ctx, void *data, int length);
typedef struct _CtxEntry CtxEntry;
/* we only care about the tight packing for this specific
 * structx as we do indexing across members in arrays of it,
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
const CtxEntry *ctx_get_renderstream (Ctx *ctx);
int  ctx_append_renderstream  (Ctx *ctx, void *data, int length);

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

void ctx_set_renderer (Ctx *ctx,
                       void *renderer);
void *ctx_get_renderer (Ctx *ctx);

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

void _ctx_events_init     (Ctx *ctx);
typedef struct _CtxRectangle CtxRectangle;
struct _CtxRectangle {
  int x;
  int y;
  int width;
  int height;
};


typedef void (*CtxCb) (CtxEvent *event,
                       void     *data,
                       void     *data2);
typedef void (*CtxDestroyNotify) (void *data);

enum _CtxEventType {
  CTX_PRESS          = 1 << 0,
  CTX_MOTION         = 1 << 1,
  CTX_RELEASE        = 1 << 2,
  CTX_ENTER          = 1 << 3,
  CTX_LEAVE          = 1 << 4,
  CTX_TAP            = 1 << 5,
  CTX_TAP_AND_HOLD   = 1 << 6,

  /* NYI: SWIPE, ZOOM ROT_ZOOM, */

  CTX_DRAG_PRESS     = 1 << 7,
  CTX_DRAG_MOTION    = 1 << 8,
  CTX_DRAG_RELEASE   = 1 << 9,
  CTX_KEY_DOWN       = 1 << 10,
  CTX_KEY_UP         = 1 << 11,
  CTX_SCROLL         = 1 << 12,
  CTX_MESSAGE        = 1 << 13,
  CTX_DROP           = 1 << 14,

  /* client should store state - preparing
                                 * for restart
                                 */
  CTX_POINTER  = (CTX_PRESS | CTX_MOTION | CTX_RELEASE | CTX_DROP),
  CTX_TAPS     = (CTX_TAP | CTX_TAP_AND_HOLD),
  CTX_CROSSING = (CTX_ENTER | CTX_LEAVE),
  CTX_DRAG     = (CTX_DRAG_PRESS | CTX_DRAG_MOTION | CTX_DRAG_RELEASE),
  CTX_KEY      = (CTX_KEY_DOWN | CTX_KEY_UP),
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

  int      device_no; /* 0 = left mouse button / virtual focus */
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


  unsigned int unicode; /* only valid for key-events */
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

int ctx_key_press (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time);
int ctx_scrolled  (Ctx *ctx, float x, float y, CtxScrollDirection scroll_direction, uint32_t time);
void ctx_incoming_message (Ctx *ctx, const char *message, long time);
int ctx_pointer_motion    (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_release   (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_press     (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_drop      (Ctx *ctx, float x, float y, int device_no, uint32_t time,
                           char *string);


////////////////////

typedef enum
{
  // items marked with % are currently only for the parser
  // for instance for svg compatibility or simulated/converted color spaces
  // not the serialization/internal render stream
  //
  // unless specified, the arguments expected are 32bit float numbers.
  //
  CTX_FLUSH            = ';',
  CTX_ARC_TO           = 'A', // x1 y1 x2 y2 radius
  CTX_ARC              = 'B', // x y radius angle1 angle2 direction
  CTX_CURVE_TO         = 'C', // cx1 cy1 cx2 cy2 x y

  CTX_STROKE           = 'E', //
  CTX_FILL             = 'F', //
  CTX_RESTORE          = 'G', //
  CTX_HOR_LINE_TO      = 'H', // x

  CTX_ROTATE           = 'J', // radians
  CTX_COLOR            = 'K', // model, c1 c2 c3 ca - has a variable set of
  // arguments.
  CTX_LINE_TO          = 'L', // x y
  CTX_MOVE_TO          = 'M', // x y
  CTX_BEGIN_PATH       = 'N',
  CTX_SCALE            = 'O', // xscale yscale
  CTX_NEW_PAGE         = 'P', // - NYI
  CTX_QUAD_TO          = 'Q', // cx cy x y
  CTX_VIEW_BOX         = 'R', // x y width height
  CTX_SMOOTH_TO        = 'S', // cx cy x y
  CTX_SMOOTHQ_TO       = 'T', // x y
  CTX_RESET            = 'U', //
  CTX_VER_LINE_TO      = 'V', // y
  CTX_APPLY_TRANSFORM  = 'W', // a b c d e f - for set_transform combine with identity
  CTX_EXIT             = 'X', //

  CTX_CLOSE_PATH2      = 'Z', //
  CTX_REL_ARC_TO       = 'a', // x1 y1 x2 y2 radius
  CTX_CLIP             = 'b',
  CTX_REL_CURVE_TO     = 'c', // cx1 cy1 cx2 cy2 x y

  CTX_TRANSLATE        = 'e', // x y
  CTX_LINEAR_GRADIENT  = 'f', // x1 y1 x2 y2
  CTX_SAVE             = 'g',
  CTX_REL_HOR_LINE_TO  = 'h', // x
  CTX_TEXTURE          = 'i',
  CTX_PRESERVE         = 'j', // - make the following fill, stroke or clip leave the path

  CTX_SET_KEY          = 'k', // - used together with another char to identify a key to set
  CTX_REL_LINE_TO      = 'l', // x y
  CTX_REL_MOVE_TO      = 'm', // x y
  CTX_FONT         = 'n', // as used by text parser
  CTX_RADIAL_GRADIENT  = 'o', // x1 y1 radius1 x2 y2 radius2
  CTX_GRADIENT_STOP    = 'p', //   , count depends on current color model
  CTX_REL_QUAD_TO      = 'q', // cx cy x y
  CTX_RECTANGLE        = 'r', // x y width height
  CTX_REL_SMOOTH_TO    = 's', // cx cy x y
  CTX_REL_SMOOTHQ_TO   = 't', // x y
  CTX_TEXT_STROKE      = 'u', // string - utf8 string
  CTX_REL_VER_LINE_TO  = 'v', // y
  CTX_GLYPH            = 'w', // unichar fontsize
  CTX_TEXT             = 'x', // string | kern - utf8 data to shape or horizontal kerning amount
  CTX_IDENTITY         = 'y', //
  CTX_CLOSE_PATH       = 'z', //

  CTX_ROUND_RECTANGLE  = 'Y', // x y width height radius
  CTX_SET              = 'D', // key value - will take over k/K spots?
  CTX_GET              = 'd', // key -
  /* these commands have single byte binary representations,
   * but are two chars in text, values below 9 are used for
   * low integers of enum values. and can thus not be used here
   */
  CTX_SET_DRGB_SPACE       = 21, // hacks integer for now
  CTX_SET_RGB_SPACE        = 22, //
  CTX_SET_CMYK_SPACE       = 23, //
  CTX_SET_DCMYK_SPACE      = 24, //

  /* though expressed as two chars in serialization we have
   * dedicated byte commands for these setters
   */
  CTX_TEXT_ALIGN           = 17, // kt align - u8, default = CTX_TEXT_ALIGN_START
  CTX_TEXT_BASELINE        = 18, // kb baseline - u8, default = CTX_TEXT_ALIGN_ALPHABETIC
  CTX_TEXT_DIRECTION       = 19, // kd
  CTX_MITER_LIMIT          = 20, // km limit - float, default = 0.0
  CTX_GLOBAL_ALPHA         = 26, // ka alpha - default=1.0
  CTX_COMPOSITING_MODE     = 27, // kc mode - u8 , default=0
  CTX_BLEND_MODE           = '$',// kB mode - u8 , default=0
                                 // kb - text baseline
  CTX_FONT_SIZE            = 28, // kf size - float, default=?
  CTX_LINE_JOIN            = 29, // kj join - u8 , default=0
  CTX_LINE_CAP             = 30, // kc cap - u8, default = 0
  CTX_LINE_WIDTH           = 31, // kw width, default = 2.0
  CTX_FILL_RULE            = '!', // kr rule - u8, default = CTX_FILLE_RULE_EVEN_ODD
  CTX_SHADOW_BLUR          = '<', // ks
  CTX_SHADOW_COLOR         = '>', // kC
  CTX_SHADOW_OFFSET_X      = '?', // kx
  CTX_SHADOW_OFFSET_Y      = '&', // ky
  CTX_START_GROUP          = '{',
  CTX_END_GROUP            = '}',

  CTX_FUNCTION             = 25,
  //CTX_ENDFUN = 26,

  // non-alphabetic chars that get filtered out when parsing
  // are used for internal purposes
  //
  // unused:  . , : backslash  #  % ^ { } < > ? & /
  //           i 
  //
  CTX_CONT             = '\0', // - contains args from preceding entry
  CTX_SET_RGBA_U8      = '*', // r g b a - u8
  // NYI
  CTX_BITPIX           = 'I', // x, y, width, height, scale
  CTX_BITPIX_DATA      = 'j', //

  CTX_NOP              = ' ', //
  CTX_NEW_EDGE         = '+', // x0 y0 x1 y1 - s16
  CTX_EDGE             = '|', // x0 y0 x1 y1 - s16
  CTX_EDGE_FLIPPED     = '`', // x0 y0 x1 y1 - s16

  CTX_REPEAT_HISTORY   = ']', //
  CTX_DATA             = '(', // size size-in-entries - u32
  CTX_DATA_REV         = ')', // reverse traversal data marker
  // needed to be able to do backwards
  // traversal
  CTX_DEFINE_GLYPH     = '@', // unichar width - u32
  CTX_KERNING_PAIR     = '[', // glA glB kerning, glA and glB in u16 kerning in s32
  CTX_SET_PIXEL        = '-', // r g b a x y - u8 for rgba, and u16 for x,y

  /* optimizations that reduce the number of entries used,
   * not visible outside the draw-stream compression -
   * these are using values that would mean numbers in an
   * SVG path.
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

int ctx_get_renderstream_count (Ctx *ctx);

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
      float x0;
      float y0;
      uint8_t pad0;
      float x1;
      float y1;
      uint8_t pad1;
      float x2;
      float y2;
      uint8_t pad2;
      float x3;
      float y3;
      uint8_t pad3;
      float x4;
      float y4;
    } c;
    struct
    {
      uint8_t code;
      float a0;
      float a1;
      uint8_t pad0;
      float a2;
      float a3;
      uint8_t pad1;
      float a4;
      float a5;
      uint8_t pad2;
      float a6;
      float a7;
      uint8_t pad3;
      float a8;
      float a9;
    } f;
    struct
    {
      uint8_t code;
      uint32_t a0;
      uint32_t a1;
      uint8_t pad0;
      uint32_t a2;
      uint32_t a3;
      uint8_t pad1;
      uint32_t a4;
      uint32_t a5;
      uint8_t pad2;
      uint32_t a6;
      uint32_t a7;
      uint8_t pad3;
      uint32_t a8;
      uint32_t a9;
    } u32;
    struct
    {
      uint8_t code;
      uint64_t a0;
      uint8_t pad0;
      uint64_t a1;
      uint8_t pad1;
      uint64_t a2;
      uint8_t pad2;
      uint64_t a3;
      uint8_t pad3;
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

typedef struct _CtxImplementation CtxImplementation;
struct _CtxImplementation
{
  void (*process) (void *renderer, CtxCommand *entry);
  void (*flush)   (void *renderer);
  void (*free)    (void *renderer);
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
  CTX_ANTIALIAS_DEFAULT,
  CTX_ANTIALIAS_NONE, //
  CTX_ANTIALIAS_FAST, // aa 3
  CTX_ANTIALIAS_GOOD, // aa 5
  CTX_ANTIALIAS_BEST  // aa 17
};
typedef enum _CtxAntialias CtxAntialias;

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

void
ctx_parser_set_size (CtxParser *parser,
                     int        width,
                     int        height,
                     float      cell_width,
                     float      cell_height);

void ctx_parser_feed_byte (CtxParser *parser, int byte);

void ctx_parser_free (CtxParser *parser);

#ifndef assert
#define assert(a)
#endif

#ifdef __cplusplus
}
#endif
#endif
