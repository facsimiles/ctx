/* 
 * ctx is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * ctx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ctx; if not, see <https://www.gnu.org/licenses/>.
 *
 * 2002, 2012, 2015, 2019, 2020 Øyvind Kolås <pippin@gimp.org>
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

/* The pixel formats supported as render targets
 */

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
} CtxPixelFormat;

typedef struct _Ctx Ctx;

struct
  _CtxGlyph
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
Ctx *ctx_new (void);


/**
 * ctx_new_for_framebuffer:
 *
 * Create a new drawing context for a framebuffer, rendering happens
 * immediately.
 */
Ctx *ctx_new_for_framebuffer (void *data,
                              int width, int height, int stride,
                              CtxPixelFormat pixel_format);

Ctx *ctx_new_ui (int width, int height);

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
void ctx_reset          (Ctx *ctx);

void ctx_new_path       (Ctx *ctx);
void ctx_save           (Ctx *ctx);
void ctx_restore        (Ctx *ctx);
void ctx_clip           (Ctx *ctx);
void ctx_identity       (Ctx *ctx);
void ctx_rotate         (Ctx *ctx, float x);
void ctx_set_line_width (Ctx *ctx, float x);
void ctx_apply_transform  (Ctx *ctx, float a,  float b,  // hscale, hskew
                           float c,  float d,  // vskew,  vscale
                           float e,  float f); // htran,  vtran


#define CTX_LINE_WIDTH_HAIRLINE -1000.0
#define CTX_LINE_WIDTH_ALIASED  -1.0
#define CTX_LINE_WIDTH_FAST     -1.0  /* aliased 1px wide line */

void  ctx_dirty_rect      (Ctx *ctx, int *x, int *y, int *width, int *height);

void  ctx_set_font_size   (Ctx *ctx, float x);
void  ctx_set_font        (Ctx *ctx, const char *font);
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
void  ctx_rectangle       (Ctx *ctx,
                           float x0, float y0,
                           float w, float h);
void  ctx_rel_line_to     (Ctx *ctx, float x, float y);
void  ctx_rel_move_to     (Ctx *ctx, float x, float y);
void  ctx_rel_curve_to    (Ctx *ctx,
                           float x0, float y0,
                           float x1, float y1,
                           float x2, float y2);
void  ctx_rel_quad_to     (Ctx *ctx, float cx, float cy,
                           float x, float y);

void  ctx_close_path      (Ctx *ctx);



float ctx_get_font_size   (Ctx *ctx);
float ctx_get_line_width  (Ctx *ctx);
float ctx_x               (Ctx *ctx);
int   ctx_width           (Ctx *ctx);
int   ctx_height          (Ctx *ctx);
int   ctx_rev             (Ctx *ctx);
float ctx_y               (Ctx *ctx);
void  ctx_current_point   (Ctx *ctx, float *x, float *y);
void  ctx_get_transform   (Ctx *ctx, float *a, float *b,
                           float *c, float *d,
                           float *e, float *f);

int  ctx_glyph            (Ctx *ctx, uint32_t unichar, int stroke);
void ctx_arc              (Ctx  *ctx,
                           float x, float y,
                           float radius,
                           float angle1, float angle2,
                           int   direction);
void ctx_arc_to           (Ctx *ctx, float x1, float y1,
                           float x2, float y2, float radius);

void ctx_quad_to        (Ctx *ctx, float cx, float cy,
                         float x, float y);
void ctx_arc            (Ctx  *ctx,
                         float x, float y,
                         float radius,
                         float angle1, float angle2,
                         int   direction);
void ctx_arc_to         (Ctx *ctx, float x1, float y1,
                         float x2, float y2, float radius);

void ctx_preserve       (Ctx *ctx);
void ctx_fill           (Ctx *ctx);
void ctx_stroke         (Ctx *ctx);
void ctx_paint          (Ctx *ctx);

void
ctx_set_pixel_u8 (Ctx *ctx, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void  ctx_set_global_alpha (Ctx *ctx, float global_alpha);
float ctx_get_global_alpha (Ctx *ctx);

void ctx_set_rgba   (Ctx *ctx, float r, float g, float b, float a);
void ctx_set_rgb    (Ctx *ctx, float r, float g, float b);
void ctx_set_gray   (Ctx *ctx, float gray);
void ctx_set_rgba8  (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void ctx_set_drgba  (Ctx *ctx, float r, float g, float b, float a);
void ctx_set_cmyka  (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_set_cmyk   (Ctx *ctx, float c, float m, float y, float k);
void ctx_set_dcmyka (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_set_dcmyk  (Ctx *ctx, float c, float m, float y, float k);

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
void ctx_texture_release (Ctx *ctx, int id);
void ctx_texture (Ctx *ctx, int id, float x, float y);

void ctx_image_path (Ctx *ctx, const char *path, float x, float y);

typedef struct _CtxRenderstream CtxRenderstream;
typedef void (*CtxFullCb) (CtxRenderstream *renderstream, void *data);

void _ctx_set_store_clear (Ctx *ctx);
void _ctx_set_transformation (Ctx *ctx, int transformation);

Ctx *ctx_hasher_new (int width, int height, int rows, int cols);
uint64_t ctx_hash_get_hash (Ctx *ctx, int row, int col);

/* If cairo.h is included before ctx.h add cairo integration code
 */
#ifdef CAIRO_H
#define CTX_CAIRO 1
#else
#define CTX_CAIRO 0
#endif

#if CTX_CAIRO
void  ctx_render_cairo  (Ctx *ctx, cairo_t *cr);
Ctx * ctx_new_for_cairo (cairo_t *cr);
#endif

void ctx_render_stream (Ctx *ctx, FILE *stream, int formatter);

void ctx_render_ctx (Ctx *ctx, Ctx *d_ctx);

void ctx_start_move (Ctx *ctx);

int ctx_add_single (Ctx *ctx, void *entry);

uint32_t ctx_utf8_to_unichar (const char *input);
int      ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);

typedef enum
{
  CTX_FILL_RULE_EVEN_ODD,
  CTX_FILL_RULE_WINDING
} CtxFillRule;

typedef enum
{
  CTX_COMPOSITE_SOURCE_OVER, // 0
  CTX_COMPOSITE_COPY,
  CTX_COMPOSITE_CLEAR,
  CTX_COMPOSITE_SOURCE_IN,
  CTX_COMPOSITE_SOURCE_OUT,
  CTX_COMPOSITE_SOURCE_ATOP,
  CTX_COMPOSITE_DESTINATION,
  CTX_COMPOSITE_DESTINATION_OVER,
  CTX_COMPOSITE_DESTINATION_IN, // 8
  CTX_COMPOSITE_DESTINATION_OUT,
  CTX_COMPOSITE_DESTINATION_ATOP,
  CTX_COMPOSITE_XOR, // 11
} CtxCompositingMode;

typedef enum
{
  CTX_BLEND_NORMAL, // 0
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
int  ctx_set_renderstream     (Ctx *ctx, void *data, int length);
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

typedef struct _CtxEntry CtxEntry;

enum _CtxModifierState
{
  CTX_MODIFIER_STATE_SHIFT   = (1<<0),
  CTX_MODIFIER_STATE_CONTROL = (1<<1),
  CTX_MODIFIER_STATE_ALT     = (1<<2),
  CTX_MODIFIER_STATE_BUTTON1 = (1<<3),
  CTX_MODIFIER_STATE_BUTTON2 = (1<<4),
  CTX_MODIFIER_STATE_BUTTON3 = (1<<5)
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

/* the following API is only available when CTX_EVENTS is defined to 1
 *
 * it provides the ability to register callbacks with the current path
 * that get delivered with transformed coordinates.
 */

uint32_t ctx_ms    (Ctx *ctx);
long     ctx_ticks (void);

void _ctx_events_init     (Ctx *ctx);


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
  Ctx     *ctx;
  uint32_t time;

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

  CtxScrollDirection scroll_direction;

  unsigned int unicode; /* only valid for key-events */

  const char *string;   /* as key can be "up" "down" "space" "backspace" "a" "b" "ø" etc .. */
                        /* this is also where the message is delivered for
                         * MESSAGE events
                         *
                         * and the data for drop events are delivered
                         */
  int stop_propagate; /* */

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
void  ctx_clear_bindings   (Ctx *ctx);
void  ctx_remove_idle      (Ctx *ctx, int handle);
int   ctx_add_timeout_full (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                            void (*destroy_notify)(void *destroy_data), void *destroy_data);
int   ctx_add_timeout   (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data);
int   ctx_add_idle_full (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                         void (*destroy_notify)(void *destroy_data), void *destroy_data);
int   ctx_add_idle      (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data);

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

int   ctx_pointer_is_down (Ctx *ctx, int no);
float ctx_pointer_x (Ctx *ctx);
float ctx_pointer_y (Ctx *ctx);
void  ctx_freeze (Ctx *ctx);
void  ctx_thaw   (Ctx *ctx);



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
  CTX_SET_COLOR        = 'K', // model, c1 c2 c3 ca - has a variable set of
  // arguments.
  CTX_LINE_TO          = 'L', // x y
  CTX_MOVE_TO          = 'M', // x y
  CTX_NEW_PATH         = 'N',
  CTX_SCALE            = 'O', // xscale yscale
  CTX_NEW_PAGE         = 'P', // - NYI
  CTX_QUAD_TO          = 'Q', // cx cy x y
  CTX_MEDIA_BOX        = 'R', // x y width height
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
  CTX_SET_FONT         = 'n', // as used by text parser
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
  CTX_SET_TEXT_ALIGN       = 17, // kt align - u8, default = CTX_TEXT_ALIGN_START
  CTX_SET_TEXT_BASELINE    = 18, // kb baseline - u8, default = CTX_TEXT_ALIGN_ALPHABETIC
  CTX_SET_TEXT_DIRECTION   = 19, // kd
  CTX_SET_MITER_LIMIT      = 20, // km limit - float, default = 0.0

  CTX_SET_DRGB_SPACE       = 21, // hacks integer for now
  CTX_SET_RGB_SPACE        = 22, //
  CTX_SET_CMYK_SPACE       = 23, //
  CTX_SET_DCMYK_SPACE      = 24, //

  /* though expressed as two chars in serialization we have
   * dedicated byte commands for these setters
   */
  CTX_SET_GLOBAL_ALPHA     = 26, // ka alpha - default=1.0
  CTX_SET_COMPOSITING_MODE = 27, // kc mode - u8 , default=0
  CTX_SET_BLEND_MODE       = '$',// kb mode - u8 , default=0
  CTX_SET_FONT_SIZE        = 28, // kf size - float, default=?
  CTX_SET_LINE_JOIN        = 29, // kj join - u8 , default=0
  CTX_SET_LINE_CAP         = 30, // kc cap - u8, default = 0
  CTX_SET_LINE_WIDTH       = 31, // kw width, default = 2.0
  CTX_SET_FILL_RULE        = '!', // kr rule - u8, default = CTX_FILLE_RULE_EVEN_ODD

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
typedef struct _CtxCommand CtxCommand;
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
      uint8_t code;
      float pad;
      float pad2;
      uint8_t code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t code_cont;
      uint8_t utf8[8]; /* .. and continues */
    } text_stroke;
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

typedef struct _CtxIterator CtxIterator;
CtxCommand *ctx_iterator_next (CtxIterator *iterator);

CtxIterator *
ctx_current_path (Ctx *ctx);
void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2);


/* definitions that determine which features are included and their settings,
 * for particular platforms - in particular microcontrollers ctx might need
 * tuning for different quality/performance/resource constraints.
 *
 * the way to configure ctx is to set these defines, before both including it
 * as a header and in the file where CTX_IMPLEMENTATION is set to include the
 * implementation for different featureset and runtime settings.
 *
 */

#ifndef CTX_RASTERIZER  // set to 0 before to disable rasterizer code, useful for clients that only
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
#define CTX_SUBDIV             8 // changing this changes font-file-format

// 8    12 68 40 24
// 16   12 68 40 24
/* scale-factor for font outlines prior to bit quantization by CTX_SUBDIV
 *
 * changing this also changes font file format
 */
#define CTX_BAKE_FONT_SIZE    160

/* pack some linetos/curvetos/movetos into denser renderstream indstructions,
 * permitting more vectors to be stored in the same space.
 */
#ifndef CTX_BITPACK
#define CTX_BITPACK           1
#endif

/* whether we have a shape-cache where we keep pre-rasterized bitmaps of commonly
 * occuring small shapes.
 */
#ifndef CTX_SHAPE_CACHE
#define CTX_SHAPE_CACHE       0
#endif

/* size (in pixels, w*h) that we cache rasterization for
 */
#ifndef CTX_SHAPE_CACHE_DIM
#define CTX_SHAPE_CACHE_DIM      (16*16)
#endif

#ifndef CTX_SHAPE_CACHE_MAX_DIM
#define CTX_SHAPE_CACHE_MAX_DIM  32
#endif

#ifndef CTX_PARSER_MAXLEN
#define CTX_PARSER_MAXLEN  1024 // this is the largest text string we support
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
#define CTX_GRADIENT_CACHE  1
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

#ifndef CTX_CURRENT_PATH
#define CTX_CURRENT_PATH 1
#endif

#ifndef CTX_XML
#define CTX_XML 1
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
#define CTX_MIN_JOURNAL_SIZE   10240
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
#define CTX_MAX_EDGE_LIST_SIZE   4096
#endif

//#define CTX_STRINGPOOL_SIZE   6000 // needed for tiger
//#define CTX_STRINGPOOL_SIZE   8000 // needed for debian logo in bw
#define CTX_STRINGPOOL_SIZE   8500 // needed for debian logo in color

/* whether we dither or not for gradients
 */
#ifndef CTX_DITHER
#define CTX_DITHER 1
#endif

#ifndef CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK  1
#endif

#ifndef CTX_ENABLE_CM
#define CTX_ENABLE_CM  1
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

#if CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK8                1
#define CTX_ENABLE_CMYKA8               1
#define CTX_ENABLE_CMYKAF               1
#endif

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

/* enable cmykf which is cmyk intermediate format
 */
#ifdef CTX_ENABLE_CMYK8
#ifdef CTX_ENABLE_CMYKF
#undef CTX_ENABLE_CMYKF
#endif
#define CTX_ENABLE_CMYKF  1
#endif
#ifdef CTX_ENABLE_CMYKA8
#ifdef CTX_ENABLE_CMYKF
#undef CTX_ENABLE_CMYKF
#endif
#define CTX_ENABLE_CMYKF  1
#endif

#ifdef CTX_ENABLE_CMYKF8
#ifdef CTX_ENABLE_CMYK
#undef CTX_ENABLE_CMYK
#endif
#define CTX_ENABLE_CMYK  1
#endif

#define CTX_PI               3.141592653589793f
#ifndef CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS  100
#endif

#ifndef CTX_MAX_FONTS
#define CTX_MAX_FONTS           3
#endif

#ifndef CTX_MAX_STATES
#define CTX_MAX_STATES          10
#endif

#ifndef CTX_MAX_EDGES
#define CTX_MAX_EDGES           257
#endif

#ifndef CTX_MAX_LINGERING_EDGES
#define CTX_MAX_LINGERING_EDGES 32
#endif

#ifndef CTX_MAX_TEXTURES
#define CTX_MAX_TEXTURES        16
#endif

#ifndef CTX_MAX_PENDING
#define CTX_MAX_PENDING         128
#endif

#ifndef CTX_RENDER_CTX
#define CTX_RENDER_CTX       1
#endif

#ifndef CTX_EVENTS
#define CTX_EVENTS           0
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

static inline int ctx_color_model_get_components (CtxColorModel model)
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

#define CTX_CLAMP(val,min,max) ((val)<(min)?(min):(val)>(max)?(max):(val))
static inline int   ctx_mini (int a, int b)     { if (a < b) return a; return b; }
static inline float ctx_minf (float a, float b) { if (a < b) return a; return b; }
static inline int   ctx_maxi (int a, int b)     { if (a > b) return a; return b; }
static inline float ctx_maxf (float a, float b) { if (a > b) return a; return b; }


#ifndef __CTX_LIST__
#define  __CTX_LIST__

#include <stdlib.h>

/* The whole ctx_list implementation is in the header and will be inlined
 * wherever it is used.
 */

typedef struct _CtxList CtxList;
struct _CtxList {
  void *data;
  CtxList *next;
  void (*freefunc)(void *data, void *freefunc_data);
  void *freefunc_data;
};

static inline void ctx_list_prepend_full (CtxList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  CtxList *new_= (CtxList*)calloc (sizeof (CtxList), 1);
  new_->next = *list;
  new_->data=data;
  new_->freefunc=freefunc;
  new_->freefunc_data = freefunc_data;
  *list = new_;
}


static inline int ctx_list_length (CtxList *list)
{
  int length = 0;
  CtxList *l;
  for (l = list; l; l = l->next, length++);
  return length;
}

static inline void ctx_list_prepend (CtxList **list, void *data)
{
  CtxList *new_ = (CtxList*) calloc (sizeof (CtxList), 1);
  new_->next= *list;
  new_->data=data;
  *list = new_;
}

static inline CtxList *ctx_list_nth (CtxList *list, int no)
{
  while (no-- && list)
    { list = list->next; }
  return list;
}


static inline void
ctx_list_insert_before (CtxList **list, CtxList *sibling,
                       void *data)
{
  if (*list == NULL || *list == sibling)
    {
      ctx_list_prepend (list, data);
    }
  else
    {
      CtxList *prev = NULL;
      for (CtxList *l = *list; l; l=l->next)
        {
          if (l == sibling)
            { break; }
          prev = l;
        }
      if (prev)
        {
          CtxList *new_ = (CtxList*)calloc (sizeof (CtxList), 1);
          new_->next = sibling;
          new_->data = data;
          prev->next=new_;
        }
    }
}


static inline void ctx_list_remove (CtxList **list, void *data)
{
  CtxList *iter, *prev = NULL;
  if ((*list)->data == data)
    {
      if ((*list)->freefunc)
        (*list)->freefunc ((*list)->data, (*list)->freefunc_data);
      prev = (*list)->next;
      free (*list);
      *list = prev;
      return;
    }
  for (iter = *list; iter; iter = iter->next)
    if (iter->data == data)
      {
        if (iter->freefunc)
          iter->freefunc (iter->data, iter->freefunc_data);
        prev->next = iter->next;
        free (iter);
        break;
      }
    else
      prev = iter;
}

static inline void ctx_list_free (CtxList **list)
{
  while (*list)
    ctx_list_remove (list, (*list)->data);
}

static inline void
ctx_list_reverse (CtxList **list)
{
  CtxList *new_ = NULL;
  CtxList *l;
  for (l = *list; l; l=l->next)
    ctx_list_prepend (&new_, l->data);
  ctx_list_free (list);
  *list = new_;
}

static inline void *ctx_list_last (CtxList *list)
{
  if (list)
    {
      CtxList *last;
      for (last = list; last->next; last=last->next);
      return last->data;
    }
  return NULL;
}

static inline void ctx_list_append_full (CtxList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  CtxList *new_ = (CtxList*) calloc (sizeof (CtxList), 1);
  new_->data=data;
  new_->freefunc = freefunc;
  new_->freefunc_data = freefunc_data;
  if (*list)
    {
      CtxList *last;
      for (last = *list; last->next; last=last->next);
      last->next = new_;
      return;
    }
  *list = new_;
  return;
}

static inline void ctx_list_append (CtxList **list, void *data)
{
  ctx_list_append_full (list, data, NULL, NULL);
}

static inline void
ctx_list_insert_at (CtxList **list,
                    int       no,
                    void     *data)
{
  if (*list == NULL || no == 0)
    {
      ctx_list_prepend (list, data);
    }
  else
    {
      int pos = 0;
      CtxList *prev = NULL;
      CtxList *sibling = NULL;
      for (CtxList *l = *list; l && pos < no; l=l->next)
        {
          prev = sibling;
          sibling = l;
          pos ++;
        }
      if (prev)
        {
          CtxList *new_ = (CtxList*)calloc (sizeof (CtxList), 1);
          new_->next = sibling;
          new_->data = data;
          prev->next=new_;
          return;
        }
      ctx_list_append (list, data);
    }
}

static CtxList*
ctx_list_merge_sorted (CtxList* list1,
                       CtxList* list2,
    int(*compare)(const void *a, const void *b, void *userdata), void *userdata
)
{
  if (list1 == NULL)
     return(list2);
  else if (list2==NULL)
     return(list1);

  if (compare (list1->data, list2->data, userdata) >= 0)
  {
    list1->next = ctx_list_merge_sorted (list1->next,list2, compare, userdata);
    /*list1->next->prev = list1;
      list1->prev = NULL;*/
    return list1;
  }
  else
  {
    list2->next = ctx_list_merge_sorted (list1,list2->next, compare, userdata);
    /*list2->next->prev = list2;
      list2->prev = NULL;*/
    return list2;
  }
}

static void
ctx_list_split_half (CtxList*  head,
                     CtxList** list1,
                     CtxList** list2)
{
  CtxList* fast;
  CtxList* slow;
  if (head==NULL || head->next==NULL)
  {
    *list1 = head;
    *list2 = NULL;
  }
  else
  {
    slow = head;
    fast = head->next;

    while (fast != NULL)
    {
      fast = fast->next;
      if (fast != NULL)
      {
        slow = slow->next;
        fast = fast->next;
      }
    }

    *list1 = head;
    *list2 = slow->next;
    slow->next = NULL;
  }
}

static inline void ctx_list_sort (CtxList **head,
    int(*compare)(const void *a, const void *b, void *userdata),
    void *userdata)
{
  CtxList* list1;
  CtxList* list2;

  /* Base case -- length 0 or 1 */
  if ((*head == NULL) || ((*head)->next == NULL))
  {
    return;
  }

  ctx_list_split_half (*head, &list1, &list2);
  ctx_list_sort (&list1, compare, userdata);
  ctx_list_sort (&list2, compare, userdata);
  *head = ctx_list_merge_sorted (list1, list2, compare, userdata);
}

#endif
typedef enum CtxOutputmode
{
  CTX_OUTPUT_MODE_QUARTER,
  CTX_OUTPUT_MODE_BRAILLE,
  CTX_OUTPUT_MODE_GRAYS,
  CTX_OUTPUT_MODE_CTX,
  CTX_OUTPUT_MODE_CTX_COMPACT,
  CTX_OUTPUT_MODE_CTX_TERM,
  CTX_OUTPUT_MODE_SIXELS,
} CtxOutputmode;

#define CTX_NORMALIZE(a)            (((a)=='-')?'_':(a))
#define CTX_NORMALIZE_CASEFOLDED(a) (((a)=='-')?'_':((((a)>='A')&&((a)<='Z'))?(a)+32:(a)))


/* We use the preprocessor to compute case invariant hashes
 * of strings directly, if there is collisions in our vocabulary
 * the compiler tells us.
 */
#define CTX_STRH(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a27,a12,a13) (\
          (((uint32_t)CTX_NORMALIZE(a0))+ \
          (((uint32_t)CTX_NORMALIZE(a1))*27)+ \
          (((uint32_t)CTX_NORMALIZE(a2))*27*27)+ \
          (((uint32_t)CTX_NORMALIZE(a3))*27*27*27)+ \
          (((uint32_t)CTX_NORMALIZE(a4))*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a5))*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a6))*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a7))*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a8))*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a9))*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a10))*27*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a27))*27*27*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a12))*27*27*27*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE(a13))*27*27*27*27*27*27*27*27*27*27*27*27*27)))

#define CTX_STRHash(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a27,a12,a13) (\
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a0))+ \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a1))*27)+ \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a2))*27*27)+ \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a3))*27*27*27)+ \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a4))*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a5))*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a6))*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a7))*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a8))*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a9))*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a10))*27*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a27))*27*27*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a12))*27*27*27*27*27*27*27*27*27*27*27*27) + \
          (((uint32_t)CTX_NORMALIZE_CASEFOLDED(a13))*27*27*27*27*27*27*27*27*27*27*27*27*27)))

#if 0
static inline uint32_t ctx_strhash (const char *str, int case_insensitive)
{
  uint32_t str_hash = 0;
  /* hash string to number */
  {
    int multiplier = 1;
    for (int i = 0; str[i] && i < 14; i++)
      {
        if (case_insensitive)
          { str_hash = str_hash + CTX_NORMALIZE_CASEFOLDED (str[i]) * multiplier; }
        else
          { str_hash = str_hash + CTX_NORMALIZE (str[i]) * multiplier; }
        multiplier *= 27;
      }
  }
  return str_hash;
}
#else

static inline uint32_t ctx_strhash (const char *str, int case_insensitive)
{
  uint32_t ret;
  if (!str) return 0;
  int len = strlen (str);
  if (case_insensitive)
    ret =CTX_STRHash(len>=0?str[0]:0,
                       len>=1?str[1]:0,
                       len>=2?str[2]:0,
                       len>=3?str[3]:0,
                       len>=4?str[4]:0,
                       len>=5?str[5]:0,
                       len>=6?str[6]:0,
                       len>=7?str[7]:0,
                       len>=8?str[8]:0,
                       len>=9?str[9]:0,
                       len>=10?str[10]:0,
                       len>=11?str[11]:0,
                       len>=12?str[12]:0,
                       len>=13?str[13]:0);
  else
    ret =CTX_STRH(len>=0?str[0]:0,
                    len>=1?str[1]:0,
                    len>=2?str[2]:0,
                    len>=3?str[3]:0,
                    len>=4?str[4]:0,
                    len>=5?str[5]:0,
                    len>=6?str[6]:0,
                    len>=7?str[7]:0,
                    len>=8?str[8]:0,
                    len>=9?str[9]:0,
                    len>=10?str[10]:0,
                    len>=11?str[11]:0,
                    len>=12?str[12]:0,
                    len>=13?str[13]:0);
                  return ret;
}
#endif

static inline float ctx_pow2 (float a) { return a * a; }
#if CTX_MATH

static inline float
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

static inline float
ctx_invsqrtf (float x)
{
  void *foo = &x;
  float xhalf = 0.5f * x;
  int i=* (int *) foo;
  void *bar = &i;
  i = 0x5f3759df - (i >> 1);
  x = * (float *) bar;
  x *= (1.5f - xhalf * x * x);
  x *= (1.5f - xhalf * x * x); //repeating Newton-Raphson step for higher precision
  return x;
}

static inline float
ctx_sinf (float x)
{
  /* source : http://mooooo.ooo/chebyshev-sine-approximation/ */
  while (x < -CTX_PI)
    { x += CTX_PI * 2; }
  while (x > CTX_PI)
    { x -= CTX_PI * 2; }
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

static inline float ctx_atan2f (float y, float x)
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

static inline float ctx_sqrtf (float a)
{
  return 1.0f/ctx_invsqrtf (a);
}

static inline float ctx_hypotf (float a, float b)
{
  return ctx_sqrtf (ctx_pow2 (a)+ctx_pow2 (b) );
}

static inline float ctx_atanf (float a)
{
  return ctx_atan2f ( (a), 1.0f);
}

static inline float ctx_asinf (float x)
{
  return ctx_atanf ( (x) * (ctx_invsqrtf (1.0f-ctx_pow2 (x) ) ) );
}

static inline float ctx_acosf (float x)
{
  return ctx_atanf ( (ctx_sqrtf (1.0f-ctx_pow2 (x) ) / (x) ) );
}

static inline float ctx_cosf (float a)
{
  return ctx_sinf ( (a) + CTX_PI/2.0f);
}

static inline float ctx_tanf (float a)
{
  return (ctx_cosf (a) /ctx_sinf (a) );
}
static inline float
ctx_floorf (float x)
{
  return (int)x; // XXX
}

/* define more trig based on having sqrt, sin and atan2 */

#else
#include <math.h>
static inline float ctx_fabsf (float x)           { return fabsf (x); }
static inline float ctx_floorf (float x)          { return floorf (x); }
static inline float ctx_sinf (float x)            { return sinf (x); }
static inline float ctx_atan2f (float y, float x) { return atan2f (y, x); }
static inline float ctx_hypotf (float a, float b) { return hypotf (a, b); }
static inline float ctx_acosf (float a)           { return acosf (a); }
static inline float ctx_cosf (float a)            { return cosf (a); }
static inline float ctx_tanf (float a)            { return tanf (a); }
#endif

#ifdef CTX_IMPLEMENTATION

#if 0
static void
ctx_memset (void *ptr, uint8_t val, int length)
{
  uint8_t *p = (uint8_t *) ptr;
  for (int i = 0; i < length; i ++)
    { p[i] = val; }
}
#else
#define ctx_memset memset
#endif


static inline void ctx_strcpy (char *dst, const char *src)
{
  int i = 0;
  for (i = 0; src[i]; i++)
    { dst[i] = src[i]; }
  dst[i] = 0;
}

static char *ctx_strchr (const char *haystack, char needle)
{
  const char *p = haystack;
  while (*p && *p != needle)
    {
      p++;
    }
  if (*p == needle)
    { return (char *) p; }
  return NULL;
}


static float ctx_fast_hypotf (float x, float y)
{
  if (x < 0) { x = -x; }
  if (y < 0) { y = -y; }
  if (x < y)
    { return 0.96f * y + 0.4f * x; }
  else
    { return 0.96f * x + 0.4f * y; }
}


typedef struct _CtxRasterizer CtxRasterizer;
typedef struct _CtxGState     CtxGState;
typedef struct _CtxState      CtxState;
typedef struct _CtxMatrix     CtxMatrix;
struct
  _CtxMatrix
{
  float m[3][2];
};

typedef struct _CtxSource CtxSource;


#define CTX_VALID_RGBA_U8     (1<<0)
#define CTX_VALID_RGBA_DEVICE (1<<1)
#if CTX_ENABLE_CM
#define CTX_VALID_RGBA        (1<<2)
#endif
#if CTX_ENABLE_CMYK
#define CTX_VALID_CMYKA       (1<<3)
#define CTX_VALID_DCMYKA      (1<<4)
#endif
#define CTX_VALID_GRAYA       (1<<5)
#define CTX_VALID_GRAYA_U8    (1<<6)
#define CTX_VALID_LABA        ((1<<7) | CTX_VALID_GRAYA)

//_ctx_target_space (ctx, icc);
//_ctx_space (ctx);

typedef struct _CtxColor CtxColor;
struct _CtxColor
{
  uint8_t magic; // for colors used in keydb, set to a non valid start of
                 // string value.
  uint8_t rgba[4];
  uint8_t l_u8;
  uint8_t original; // the bitmask of the originally set color
  uint8_t valid;    // bitmask of which members contain valid
  // values, gets denser populated as more
  // formats are requested from a set color.
  float   device_red;
  float   device_green;
  float   device_blue;
  float   alpha;
  float   l;        // luminance and gray
#if CTX_ENABLE_LAB  // NYI
  float   a;
  float   b;
#endif
#if CTX_ENABLE_CMYK
  float   device_cyan;
  float   device_magenta;
  float   device_yellow;
  float   device_key;
  float   cyan;
  float   magenta;
  float   yellow;
  float   key;
#endif

#if CTX_ENABLE_CM
  int     space;   // a babl_space when not direct
  // cmyk values are presumed to always be in
  // ICC space and the color values set are not
  // influenced by color management. RGB values
  // however are. will lose prefix
  float   red;
  float   green;
  float   blue;
#endif
};

typedef struct _CtxGradientStop CtxGradientStop;

struct _CtxGradientStop
{
  float   pos;
  CtxColor color;
};

typedef enum _CtxSourceType CtxSourceType;

enum _CtxSourceType
{
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

struct _CtxSource
{
  int type;
  CtxMatrix  transform;
  union
  {
    CtxColor color;
    struct
    {
      uint8_t rgba[4]; // shares data with set color
      uint8_t pad;
      float x0;
      float y0;
      CtxBuffer *buffer;
    } image;
    struct
    {
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
    struct
    {
      float x0;
      float y0;
      float r0;
      float x1;
      float y1;
      float r1;
    } radial_gradient;
  };
};

struct _CtxGState
{
  int           keydb_pos;
  int           stringpool_pos;

  CtxMatrix     transform;
  //CtxSource   source_stroke;
  CtxSource     source;

  float         global_alpha_f;
  float         line_width;
  float         miter_limit;
  float         font_size;

  int16_t       clip_min_x;
  int16_t       clip_min_y;
  int16_t       clip_max_x;
  int16_t       clip_max_y;

#if CTX_ENABLE_CM
  int           device_space;
  int           rgb_space;
  int           cmyk_space;
#endif

  uint8_t       global_alpha_u8;
  CtxColorModel color_model;
  /* bitfield-pack small state-parts */
  CtxCompositingMode  compositing_mode:4;
  CtxBlend                  blend_mode:4;
  CtxLineCap                  line_cap:2;
  CtxLineJoin                line_join:2;
  CtxFillRule                fill_rule:1;
  unsigned int                    font:6;
  unsigned int                    bold:1;
  unsigned int                  italic:1;
};

typedef enum
{
  CTX_TRANSFORMATION_NONE         = 0,
  CTX_TRANSFORMATION_SCREEN_SPACE = 1,
  CTX_TRANSFORMATION_RELATIVE     = 2,
#if CTX_BITPACK
  CTX_TRANSFORMATION_BITPACK      = 4,
#endif
  CTX_TRANSFORMATION_STORE_CLEAR  = 16,
} CtxTransformation;

#define CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES   64
#define CTX_RENDERSTREAM_EDGE_LIST            128
#define CTX_RENDERSTREAM_CURRENT_PATH         512
// BITPACK

struct _CtxRenderstream
{
  CtxEntry *entries;
  int       count;
  int       size;
  uint32_t  flags;
  int       bitpack_pos;  // stream is bitpacked up to this offset
};

CtxRenderstream *ctx_copy_path      (Ctx *ctx);
CtxRenderstream *ctx_copy_path_flat (Ctx *ctx);

#define CTX_MAX_KEYDB 64

// the keydb consists of keys set to floating point values,
// that might also be interpreted as integers for enums.
//
// the hash
typedef struct _CtxKeyDbEntry CtxKeyDbEntry;
struct _CtxKeyDbEntry
{
  uint32_t key;
  float value;
  //union { float f[1]; uint8_t u8[4]; }value;
};

struct _CtxState
{
  int           has_moved:1;
  float         x;
  float         y;
  int           min_x;
  int           min_y;
  int           max_x;
  int           max_y;
  CtxKeyDbEntry keydb[CTX_MAX_KEYDB];
  char          stringpool[CTX_STRINGPOOL_SIZE];
  CtxGradient   gradient; /* we keep only one gradient,
                             this goes icky with multiple
                             restores - it should really be part of
                             graphics state..
                             XXX, with the stringpool gradients
                             can be stored there.
                           */
  int16_t       gstate_no;
  CtxGState     gstate;
  CtxGState     gstate_stack[CTX_MAX_STATES];//at end, so can be made dynamic
};



#define STR CTX_STRH

#define CTX_add_stop        CTX_STRH('a','d','d','_','s','t','o','p',0,0,0,0,0,0)
#define CTX_addStop         CTX_STRH('a','d','d','S','t','o','p',0,0,0,0,0,0,0)
#define CTX_alphabetic   CTX_STRH('a','l','p','h','a','b','e','t','i','c',0,0,0,0)
#define CTX_arc          CTX_STRH('a','r','c',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_arc_to       CTX_STRH('a','r','c','_','t','o',0,0,0,0,0,0,0,0)
#define CTX_arcTo        CTX_STRH('a','r','c','T','o',0,0,0,0,0,0,0,0,0)
#define CTX_begin_path   CTX_STRH('b','e','g','i','n','_','p','a','t','h',0,0,0,0)
#define CTX_beginPath    CTX_STRH('b','e','g','i','n','P','a','t','h',0,0,0,0,0)
#define CTX_bevel        CTX_STRH('b','e','v','e','l',0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_bottom       CTX_STRH('b','o','t','t','o','m',0,0,0,0,0,0,0,0)
#define CTX_cap          CTX_STRH('c','a','p',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_center       CTX_STRH('c','e','n','t','e','r', 0, 0, 0, 0, 0, 0,0,0)
#define CTX_clear        CTX_STRH('c','l','e','a','r',0,0,0,0,0,0,0,0,0)
#define CTX_color        CTX_STRH('c','o','l','o','r',0,0,0,0,0,0,0,0,0)
#define CTX_copy         CTX_STRH('c','o','p','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_clip         CTX_STRH('c','l','i','p',0,0,0,0,0,0,0,0,0,0)
#define CTX_close_path   CTX_STRH('c','l','o','s','e','_','p','a','t','h',0,0,0,0)
#define CTX_closePath    CTX_STRH('c','l','o','s','e','P','a','t','h',0,0,0,0,0)
#define CTX_cmyka        CTX_STRH('c','m','y','k','a',0,0,0,0,0,0,0,0,0)
#define CTX_cmyk         CTX_STRH('c','m','y','k',0,0,0,0,0,0,0,0,0,0)
#define CTX_cmyk_space   CTX_STRH('c','m','y','k','_','s','p','a','c','e',0,0,0,0)
#define CTX_cmykSpace    CTX_STRH('c','m','y','k','S','p','a','c','e',0,0,0,0,0)
#define CTX_color        CTX_STRH('c','o','l','o','r',0,0,0,0,0,0,0,0,0)

#define CTX_blending     CTX_STRH('b','l','e','n','d','i','n','g',0,0,0,0,0,0)
#define CTX_blend        CTX_STRH('b','l','e','n','d',0,0,0,0,0,0,0,0,0)
#define CTX_blending_mode CTX_STRH('b','l','e','n','d','i','n','g','_','m','o','d','e',0)
#define CTX_blendingMode CTX_STRH('b','l','e','n','d','i','n','g','M','o','d','e',0,0)
#define CTX_blend_mode   CTX_STRH('b','l','e','n','d','_','m','o','d','e',0,0,0,0)
#define CTX_blendMode    CTX_STRH('b','l','e','n','d','M','o','d','e',0,0,0,0,0)

#define CTX_composite    CTX_STRH('c','o','m','p','o','s','i','t','i','e',0,0,0,0)
#define CTX_compositing_mode CTX_STRH('c','o','m','p','o','s','i','t','i','n','g','_','m','o')
#define CTX_compositingMode CTX_STRH('c','o','m','p','o','s','i','t','i','n','g','M','o','d')
#define CTX_curve_to     CTX_STRH('c','u','r','v','e','_','t','o',0,0,0,0,0,0)
#define CTX_curveTo      CTX_STRH('c','u','r','v','e','T','o',0,0,0,0,0,0,0)
#define CTX_darken       CTX_STRH('d','a','r','k','e','n',0,0,0,0,0,0,0,0)
#define CTX_dcmyk_space  CTX_STRH('d','c','m','y','k','_','s','p','a','c','e',0,0,0)
#define CTX_dcmykSpace   CTX_STRH('d','c','m','y','k','S','p','a','c','e',0,0,0,0)
#define CTX_destinationIn    CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','I','n',0)
#define CTX_destination_in   CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','_','i','n')
#define CTX_destinationAtop  CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','A','t','o')
#define CTX_destination_atop CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','_','a','t')
#define CTX_destinationOver  CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','O','v','e')
#define CTX_destination_over CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','-','o','v')
#define CTX_destinationOut   CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','O','u','t')
#define CTX_destination_out  CTX_STRH('d','e','s','t','i','n','a','t','i','o','n','_','o','u')
#define CTX_difference       CTX_STRH('d','i','f','f','e','r','e','n','c','e',0,0,0,0)
#define CTX_done         CTX_STRH('d','o','n','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_drgba        CTX_STRH('d','r','g','b','a',0,0,0,0,0,0,0,0,0)
#define CTX_drgb         CTX_STRH('d','r','g','b',0,0,0,0,0,0,0,0,0,0)
#define CTX_drgb_space   CTX_STRH('d','r','g','b','_','s','p','a','c','e',0,0,0,0)
#define CTX_drgbSpace    CTX_STRH('d','r','g','b','S','p','a','c','e',0,0,0,0,0)
#define CTX_end          CTX_STRH('e','n','d',0,0,0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_endfun       CTX_STRH('e','n','d','f','u','n',0,0,0,0,0,0,0,0)
#define CTX_even_odd     CTX_STRH('e','v','e','n','_','o','d', 'd', 0, 0, 0, 0,0,0)
#define CTX_evenOdd      CTX_STRH('e','v','e','n','O','d', 'd', 0, 0, 0, 0, 0,0,0)
#define CTX_exit         CTX_STRH('e','x','i','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_fill         CTX_STRH('f','i','l','l',0,0,0,0,0,0,0,0,0,0)
#define CTX_fill_rule    CTX_STRH('f','i','l','l','_','r','u','l','e',0,0,0,0,0)
#define CTX_fillRule     CTX_STRH('f','i','l','l','R','u','l','e',0,0,0,0,0,0)
#define CTX_flush        CTX_STRH('f','l','u','s','h',0,0,0,0,0,0,0,0,0)
#define CTX_font         CTX_STRH('f','o','n','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_font_size    CTX_STRH('f','o','n','t','_','s','i','z','e',0,0,0,0,0)
#define CTX_fontSize     CTX_STRH('f','o','n','t','S','i','z','e',0,0,0,0,0,0)
#define CTX_function CTX_STRH('f','u','n','c','t','i','o','n',0,0,0,0,0,0)
#define CTX_getkey       CTX_STRH('g','e','t','k','e','y',0,0,0,0,0,0,0,0)
#define CTX_global_alpha CTX_STRH('g','l','o','b','a','l','_','a','l','p','h','a',0,0)
#define CTX_globalAlpha  CTX_STRH('g','l','o','b','a','l','A','l','p','h','a',0,0,0)
#define CTX_gradient_add_stop CTX_STRH('g','r','a','d','i','e','n','t','_','a','d','d','_','s')
#define CTX_gradientAddStop CTX_STRH('g','r','a','d','i','e','n','t','A','d','d','S','t','o')
#define CTX_graya        CTX_STRH('g','r','a','y','a',0,0,0,0,0,0,0,0,0)
#define CTX_gray         CTX_STRH('g','r','a','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_H
#define CTX_hanging      CTX_STRH('h','a','n','g','i','n','g',0,0,0,0,0,0,0)
#define CTX_height       CTX_STRH('h','e','i','g','h','t',0,0,0,0,0,0,0,0)
#define CTX_hor_line_to  CTX_STRH('h','o','r','_','l','i','n','e','_','t','o',0,0,0)
#define CTX_horLineTo    CTX_STRH('h','o','r','L','i','n','e','T','o',0,0,0,0,0)
#define CTX_hue          CTX_STRH('h','u','e',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_identity     CTX_STRH('i','d','e','n','t','i','t','y',0,0,0,0,0,0)
#define CTX_ideographic  CTX_STRH('i','d','e','o','g','r','a','p','h','i','c',0,0,0)
#define CTX_join         CTX_STRH('j','o','i','n',0,0,0,0,0,0,0,0,0,0)
#define CTX_laba         CTX_STRH('l','a','b','a',0,0,0,0,0,0,0,0,0,0)
#define CTX_lab          CTX_STRH('l','a','b',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_lcha         CTX_STRH('l','c','h','a',0,0,0,0,0,0,0,0,0,0)
#define CTX_lch          CTX_STRH('l','c','h',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_left         CTX_STRH('l','e','f','t',0,0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_lighter      CTX_STRH('l','i','g','h','t','e','r',0,0,0,0,0,0,0)
#define CTX_lighten      CTX_STRH('l','i','g','h','t','e','n',0,0,0,0,0,0,0)
#define CTX_linear_gradient CTX_STRH('l','i','n','e','a','r','_','g','r','a','d','i','e','n')
#define CTX_linearGradient CTX_STRH('l','i','n','e','a','r','G','r','a','d','i','e','n','t')
#define CTX_line_cap     CTX_STRH('l','i','n','e','_','c','a','p',0,0,0,0,0,0)
#define CTX_lineCap      CTX_STRH('l','i','n','e','C','a','p',0,0,0,0,0,0,0)
#define CTX_line_height  CTX_STRH('l','i','n','e','_','h','e','i','h','t',0,0,0,0)
#define CTX_line_join    CTX_STRH('l','i','n','e','_','j','o','i','n',0,0,0,0,0)
#define CTX_lineJoin     CTX_STRH('l','i','n','e','J','o','i','n',0,0,0,0,0,0)
#define CTX_line_spacing CTX_STRH('l','i','n','e','_','s','p','a','c','i','n','g',0,0)
#define CTX_line_to      CTX_STRH('l','i','n','e','_','t','o',0,0,0,0,0,0,0)
#define CTX_lineTo       CTX_STRH('l','i','n','e','T','o',0,0,0,0,0,0,0,0)
#define CTX_line_width   CTX_STRH('l','i','n','e','_','w','i','d','t','h',0,0,0,0)
#define CTX_lineWidth    CTX_STRH('l','i','n','e','W','i','d','t','h',0,0,0,0,0)
#define CTX_media_box    CTX_STRH('m','e','d','i','a','_','b','o','x',0,0,0,0,0)
#define CTX_mediaBox     CTX_STRH('m','e','d','i','a','B','o','x',0,0,0,0,0,0)
#define CTX_middle       CTX_STRH('m','i','d','d','l','e',0, 0, 0, 0, 0, 0,0,0)
#define CTX_miter        CTX_STRH('m','i','t','e','r',0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_miter_limit  CTX_STRH('m','i','t','e','r','_','l','i','m','i','t',0,0,0)
#define CTX_miterLimit   CTX_STRH('m','i','t','e','r','L','i','m','i','t',0,0,0,0)
#define CTX_move_to      CTX_STRH('m','o','v','e','_','t','o',0,0,0,0,0,0,0)
#define CTX_moveTo       CTX_STRH('m','o','v','e','T','o',0,0,0,0,0,0,0,0)
#define CTX_multiply     CTX_STRH('m','u','l','t','i','p','l','y',0,0,0,0,0,0)
#define CTX_new_page     CTX_STRH('n','e','w','_','p','a','g','e',0,0,0,0,0,0)
#define CTX_newPage      CTX_STRH('n','e','w','P','a','g','e',0,0,0,0,0,0,0)
#define CTX_new_path     CTX_STRH('n','e','w','_','p','a','t','h',0,0,0,0,0,0)
#define CTX_newPath      CTX_STRH('n','e','w','P','a','t','h',0,0,0,0,0,0,0)
#define CTX_new_state    CTX_STRH('n','e','w','_','s','t','a','t','e',0,0,0,0,0)
#define CTX_none         CTX_STRH('n','o','n','e', 0 ,0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_quad_to      CTX_STRH('q','u','a','d','_','t','o',0,0,0,0,0,0,0)
#define CTX_quadTo       CTX_STRH('q','u','a','d','T','o',0,0,0,0,0,0,0,0)
#define CTX_radial_gradient CTX_STRH('r','a','d','i','a','l','_','g','r','a','d','i','e','n')
#define CTX_radialGradient  CTX_STRH('r','a','d','i','a','l',,'G','r','a','d','i','e','n','t')
#define CTX_rectangle    CTX_STRH('r','e','c','t','a','n','g','l','e',0,0,0,0,0)
#define CTX_rect         CTX_STRH('r','e','c','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_rel_arc_to   CTX_STRH('r','e','l','_','a','r','c','_','t','o',0,0,0,0)
#define CTX_relArcTo     CTX_STRH('r','e','l','A','r','c','T','o',0,0,0,0,0,0)
#define CTX_rel_curve_to CTX_STRH('r','e','l','_','c','u','r','v','e','_','t','o',0,0)
#define CTX_relCurveTo   CTX_STRH('r','e','l','C','u','r','v','e','T','o',0,0,0,0)
#define CTX_rel_hor_line_to CTX_STRH('r','e','l','_','h','o','r','_','l','i','n','e',0,0)
#define CTX_relHorLineTo CTX_STRH('r','e','l','H','o','r','L','i','n','e','T','o',0,0)
#define CTX_rel_line_to  CTX_STRH('r','e','l','_','l','i','n','e','_','t','o',0,0,0)
#define CTX_relLineTo    CTX_STRH('r','e','l','L','i','n','e','T','o',0,0,0,0,0)
#define CTX_rel_move_to  CTX_STRH('r','e','l','_','m','o','v','e','_','t','o',0,0,0)
#define CTX_relMoveTo    CTX_STRH('r','e','l','M','o','v','e','T','o',0,0,0,0,0)
#define CTX_rel_quad_to  CTX_STRH('r','e','l','_','q','u','a','d','_','t','o',0,0,0)
#define CTX_relQuadTo    CTX_STRH('r','e','l','Q','u','a','d','T','o',0,0,0,0,0)
#define CTX_rel_smoothq_to CTX_STRH('r','e','l','_','s','m','o','o','t','h','q','_','t','o')
#define CTX_relSmoothqTo CTX_STRH('r','e','l','S','m','o','o','t','h','q','T','o',0,0)
#define CTX_rel_smooth_to CTX_STRH('r','e','l','_','s','m','o','o','t','h','_','t','o',0)
#define CTX_relSmoothTo  CTX_STRH('r','e','l','S','m','o','o','t','h','T','o',0,0,0)
#define CTX_rel_ver_line_to CTX_STRH('r','e','l','_','v','e','r','_','l','i','n','e',0,0)
#define CTX_relVerLineTo CTX_STRH('r','e','l','V','e','r','L','i','n','e','T','o',0,0)
#define CTX_restore      CTX_STRH('r','e','s','t','o','r','e',0,0,0,0,0,0,0)
#define CTX_reset        CTX_STRH('r','e','s','e','t',0,0,0,0,0,0,0,0,0)
#define CTX_rgba         CTX_STRH('r','g','b','a',0,0,0,0,0,0,0,0,0,0)
#define CTX_rgb          CTX_STRH('r','g','b',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_rgb_space    CTX_STRH('r','g','b','_','s','p','a','c','e',0,0,0,0,0)
#define CTX_rgbSpace     CTX_STRH('r','g','b','S','p','a','c','e',0,0,0,0,0,0)
#define CTX_right        CTX_STRH('r','i','g','h','t',0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_rotate       CTX_STRH('r','o','t','a','t','e',0,0,0,0,0,0,0,0)
#define CTX_round        CTX_STRH('r','o','u','n','d',0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_round_rectangle CTX_STRH('r','o','u','n','d','_','r','e','c','t','a','n','g','l')
#define CTX_roundRectangle  CTX_STRH('r','o','u','n','d','R','e','c','t','a','n','g','l','e')
#define CTX_save         CTX_STRH('s','a','v','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_save         CTX_STRH('s','a','v','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_scale        CTX_STRH('s','c','a','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_screen       CTX_STRH('s','c','r','e','e','n',0,0,0,0,0,0,0,0)
#define CTX_setkey       CTX_STRH('s','e','t','k','e','y',0,0,0,0,0,0,0,0)
#define CTX_smooth_quad_to CTX_STRH('s','m','o','o','t','h','_','q','u','a','d','_','t','o')
#define CTX_smoothQuadTo CTX_STRH('s','m','o','o','t','h','Q','u','a','d','T','o',0,0)
#define CTX_smooth_to    CTX_STRH('s','m','o','o','t','h','_','t','o',0,0,0,0,0)
#define CTX_smoothTo     CTX_STRH('s','m','o','o','t','h','T','o',0,0,0,0,0,0)
#define CTX_sourceIn     CTX_STRH('s','o','u','r','c','e','I','n',0,0,0,0,0,0)
#define CTX_source_in    CTX_STRH('s','o','u','r','c','e','_','i','n',0,0,0,0,0)
#define CTX_sourceAtop   CTX_STRH('s','o','u','r','c','e','A','t','o','p',0,0,0,0)
#define CTX_source_atop  CTX_STRH('s','o','u','r','c','e','_','a','t','o','p',0,0,0)
#define CTX_sourceOut    CTX_STRH('s','o','u','r','c','e','O','u','t',0,0,0,0,0)
#define CTX_source_out   CTX_STRH('s','o','u','r','c','e','_','o','u','t',0,0,0,0)
#define CTX_sourceOver   CTX_STRH('s','o','u','r','c','e','O','v','e','r',0,0,0,0)
#define CTX_source_over  CTX_STRH('s','o','u','r','c','e','_','o','v','e','r',0,0,0)
#define CTX_square       CTX_STRH('s','q','u','a','r','e', 0, 0, 0, 0, 0, 0,0,0)
#define CTX_start        CTX_STRH('s','t','a','r','t',0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_start_move   CTX_STRH('s','t','a','r','t','_','m','o','v','e',0,0,0,0)
#define CTX_stroke       CTX_STRH('s','t','r','o','k','e',0,0,0,0,0,0,0,0)
#define CTX_text_align   CTX_STRH('t','e','x','t','_','a','l','i','g','n',0, 0,0,0)
#define CTX_textAlign    CTX_STRH('t','e','x','t','A','l','i','g','n',0, 0, 0,0,0)
#define CTX_text_baseline CTX_STRH('t','e','x','t','_','b','a','s','e','l','i','n','e',0)
#define CTX_text_baseline CTX_STRH('t','e','x','t','_','b','a','s','e','l','i','n','e',0)
#define CTX_textBaseline CTX_STRH('t','e','x','t','B','a','s','e','l','i','n','e',0,0)
#define CTX_text         CTX_STRH('t','e','x','t',0,0,0,0,0,0,0,0,0,0)
#define CTX_text_direction CTX_STRH('t','e','x','t','_','d','i','r','e','c','t','i','o','n')
#define CTX_textDirection CTX_STRH('t','e','x','t','D','i','r','e','c','t','i','o','n',0)
#define CTX_text_indent  CTX_STRH('t','e','x','t','_','i','n','d','e','n','t', 0,0,0)
#define CTX_text_stroke  CTX_STRH('t','e','x','t','_','s','t','r','o','k','e', 0,0,0)
#define CTX_textStroke   CTX_STRH('t','e','x','t','S','t','r','o','k','e', 0, 0,0,0)
#define CTX_top          CTX_STRH('t','o','p',0,0,0, 0, 0, 0, 0, 0, 0,0,0)
#define CTX_transform    CTX_STRH('t','r','a','n','s','f','o','r','m',0,0,0,0,0)
#define CTX_translate    CTX_STRH('t','r','a','n','s','l','a','t','e',0,0,0,0,0)
#define CTX_verLineTo    CTX_STRH('v','e','r','L','i','n','e','T','o',0,0,0,0,0)
#define CTX_ver_line_to  CTX_STRH('v','e','r','_','l','i','n','e','_','t','o',0,0,0)
#define CTX_width        CTX_STRH('w','i','d','t','h',0,0,0,0,0,0,0,0,0)
#define CTX_winding      CTX_STRH('w','i','n','d','i','n', 'g', 0, 0, 0, 0, 0,0,0)
#define CTX_x            CTX_STRH('x',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_xor          CTX_STRH('x','o','r',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_y            CTX_STRH('y',0,0,0,0,0,0,0,0,0,0,0,0,0)

static float ctx_state_get (CtxState *state, uint32_t hash)
{
  for (int i = state->gstate.keydb_pos-1; i>=0; i--)
    {
      if (state->keydb[i].key == hash)
        { return state->keydb[i].value; }
    }
  return -0.0;
}

static void ctx_state_set (CtxState *state, uint32_t key, float value)
{
  if (key != CTX_new_state)
    {
      if (ctx_state_get (state, key) == value)
        { return; }
      for (int i = state->gstate.keydb_pos-1;
           state->keydb[i].key != CTX_new_state && i >=0;
           i--)
        {
          if (state->keydb[i].key == key)
            {
              state->keydb[i].value = value;
              return;
            }
        }
    }
  if (state->gstate.keydb_pos >= CTX_MAX_KEYDB)
    { return; }
  state->keydb[state->gstate.keydb_pos].key = key;
  state->keydb[state->gstate.keydb_pos].value = value;
  state->gstate.keydb_pos++;
}


#define CTX_KEYDB_STRING_START (-80000.0)
#define CTX_KEYDB_STRING_END   (CTX_KEYDB_STRING_START + CTX_STRINGPOOL_SIZE)

static int ctx_float_is_string (float val)
{
  return val >= CTX_KEYDB_STRING_START && val <= CTX_KEYDB_STRING_END;
}

static int ctx_float_to_string_index (float val)
{
  int idx = -1;
  if (ctx_float_is_string (val))
  {
    idx = val - CTX_KEYDB_STRING_START;
  }
  return idx;
}

static float ctx_string_index_to_float (int index)
{
  return CTX_KEYDB_STRING_START + index;
}

void *ctx_state_get_blob (CtxState *state, uint32_t key)
{
  float stored = ctx_state_get (state, key);
  int idx = ctx_float_to_string_index (stored);
  if (idx >= 0)
  {
     return &state->stringpool[idx];
  }
  // format number as string?
  return NULL;
}

const char *ctx_state_get_string (CtxState *state, uint32_t key)
{
  const char *ret = (char*)ctx_state_get_blob (state, key);
  if (ret && ret[0] == 127)
    return NULL;
  return ret;
}

static int ctx_str_is_number (const char *str)
{
  int got_digit = 0;
  for (int i = 0; str[i]; i++)
  {
    if (str[i] >= '0' && str[i] <= '9')
    {
       got_digit ++;
    }
    else if (str[i] == '.')
    {
    }
    else
      return 0;
  }
  if (got_digit)
    return 1;
  return 0;
}


static void ctx_state_set_blob (CtxState *state, uint32_t key, uint8_t *data, int len)
{
  int idx = state->gstate.stringpool_pos;

  if (idx + len > CTX_STRINGPOOL_SIZE)
  {
    fprintf (stderr, "blowing varpool size [%c%c%c..]\n", data[0],data[1], data[1]?data[2]:0);
#if 0
    for (int i = 0; i< CTX_STRINGPOOL_SIZE; i++)
    {
       if (i==0) fprintf (stderr, "\n%i ", i);
       else      fprintf (stderr, "%c", state->stringpool[i]);
    }
#endif
    return;
  }

  memcpy (&state->stringpool[idx], data, len);
  state->gstate.stringpool_pos+=len;
  state->stringpool[state->gstate.stringpool_pos++]=0;
  ctx_state_set (state, key, ctx_string_index_to_float (idx));
}

static void ctx_state_set_string (CtxState *state, uint32_t key, const char *string)
{
  float old_val = ctx_state_get (state, key);
  int   old_idx = ctx_float_to_string_index (old_val);

  if (old_idx >= 0)
  {
    const char *old_string = ctx_state_get_string (state, key);
    if (old_string && !strcmp (old_string, string))
      return;
  }

  if (ctx_str_is_number (string))
  {
    ctx_state_set (state, key, strtod (string, NULL));
    return;
  }
  // should do same with color
 
  // XXX should special case when the string modified is at the
  //     end of the stringpool.
  ctx_state_set_blob (state, key, (uint8_t*)string, strlen(string));
}

static int ctx_state_get_color (CtxState *state, uint32_t key, CtxColor *color)
{
  CtxColor *stored = (CtxColor*)ctx_state_get_blob (state, key);
  if (stored)
  {
    if (stored->magic == 127)
    {
      *color = *stored;
      return 0;
    }
  }
  return -1;
}

static void ctx_state_set_color (CtxState *state, uint32_t key, CtxColor *color)
{
  CtxColor mod_color;
  CtxColor old_color;
  mod_color = *color;
  mod_color.magic = 127;
  if (ctx_state_get_color (state, key, &old_color)==0)
  {
    if (!memcmp (&mod_color, &old_color, sizeof (mod_color)))
      return;
  }
  ctx_state_set_blob (state, key, (uint8_t*)&mod_color, sizeof (CtxColor));
}

static uint8_t ctx_float_to_u8 (float val_f)
{
  int val_i = val_f * 255.999f;
  if (val_i < 0) { return 0; }
  else if (val_i > 255) { return 255; }
  return val_i;
}

static float ctx_u8_to_float (uint8_t val_u8)
{
  float val_f = val_u8 / 255.0;
  return val_f;
}

static void ctx_color_set_RGBA8 (CtxState *state, CtxColor *color, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
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

static void ctx_color_set_graya (CtxState *state, CtxColor *color, float gray, float alpha)
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

static void ctx_color_set_rgba (CtxState *state, CtxColor *color, float r, float g, float b, float a)
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

static void ctx_color_set_drgba (CtxState *state, CtxColor *color, float r, float g, float b, float a)
{
#if CTX_ENABLE_CM
  color->original     = color->valid = CTX_VALID_RGBA_DEVICE;
  color->device_red   = r;
  color->device_green = g;
  color->device_blue  = b;
  color->alpha        = a;
  color->space = state->gstate.device_space;
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

static void ctx_rgb_to_cmyk (float r, float g, float b,
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
static void ctx_color_set_cmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a)
{
  color->original = color->valid = CTX_VALID_CMYKA;
  color->cyan     = c;
  color->magenta  = m;
  color->yellow   = y;
  color->key      = k;
  color->alpha    = a;
  color->space    = state->gstate.cmyk_space;
}

static void ctx_color_set_dcmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a)
{
  color->original       = color->valid = CTX_VALID_DCMYKA;
  color->device_cyan    = c;
  color->device_magenta = m;
  color->device_yellow  = y;
  color->device_key     = k;
  color->alpha          = a;
  color->space = state->gstate.cmyk_space;
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

static void ctx_color_get_rgba (CtxState *state, CtxColor *color, float *out)
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

static void ctx_color_get_graya (CtxState *state, CtxColor *color, float *out)
{
  if (! (color->valid & CTX_VALID_GRAYA) )
    {
      float rgba[4];
      ctx_color_get_drgba (state, color, rgba);
      color->l = (rgba[0] + rgba[1] + rgba[2]) /3.0f; // XXX
      color->valid |= CTX_VALID_GRAYA;
    }
  out[0] = color->l;
  out[1] = color->alpha;
}

#if CTX_ENABLE_CMYK
static void ctx_color_get_CMYKAF (CtxState *state, CtxColor *color, float *out)
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

static void ctx_color_get_rgba8 (CtxState *state, CtxColor *color, uint8_t *out)
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

#if 0
static void ctx_color_get_graya_u8 (CtxState *state, CtxColor *color, uint8_t *out)
{
  if (! (color->valid & CTX_VALID_GRAYA_U8) )
    {
      float graya[2];
      ctx_color_get_graya (ctx, color, graya);
      color->l_u8 = ctx_float_to_u8 (graya[0]);
      color->rgba[3] = ctx_float_to_u8 (graya[1]);
      color->valid |= CTX_VALID_GRAYA_U8;
    }
  out[0] = color->l_u8;
  out[1] = color->rgba[3];
}
#endif

#if CTX_RASTERIZER

typedef struct CtxEdge
{
  int32_t  x;     /* the center-line intersection      */
  int32_t  dx;
  uint16_t index;
} CtxEdge;

typedef void (*CtxFragment) (CtxRasterizer *rasterizer, float x, float y, void *out);

struct _CtxRasterizer
{
  CtxImplementation vfuncs;
  /* these should be initialized and used as the bounds for rendering into the
     buffer as well XXX: not yet in use, and when in use will only be
     correct for axis aligned clips - proper rasterization of a clipping path
     would be yet another refinement on top.
   */
  //CtxBuffer *clip_buffer; // NYI

  int        lingering_edges;  // previous half scanline
  CtxEdge    lingering[CTX_MAX_LINGERING_EDGES];

  int        active_edges;
  int        pending_edges;    // next half scanline
  int        edge_pos;         // where we're at in iterating all edges
  CtxEdge    edges[CTX_MAX_EDGES];

  int        scanline;
  int        scan_min;
  int        scan_max;
  int        col_min;
  int        col_max;

  CtxRenderstream edge_list;

  CtxState  *state;
  Ctx       *ctx;

  void      *buf;
  float      x;  // < redundant? use state instead?
  float      y;

  float      first_x;
  float      first_y;
  int8_t     needs_aa; // count of how many edges implies antialiasing
  int        has_shape:2;
  int        has_prev:2;
  int        preserve:1;
  int        uses_transforms:1;

  int16_t    blit_x;
  int16_t    blit_y;
  int16_t    blit_width;
  int16_t    blit_height;
  int16_t    blit_stride;

  CtxPixelFormatInfo *format;


  CtxFragment     fragment;
  void (*comp_op)(uint8_t *src, uint8_t *dst, uint8_t *cov, int count, float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer);
  void (*blend_op)(uint8_t *dst, uint8_t *src, uint8_t *blended);

};

typedef struct _CtxRectangle CtxRectangle;
struct _CtxRectangle {
  int x;
  int y;
  int width;
  int height;
};

typedef struct _CtxHasher CtxHasher;
struct _CtxHasher
{
  CtxRasterizer rasterizer;
  int           cols;
  int           rows;
  uint32_t     *hashes;
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

  void         (*to_comp) (CtxRasterizer *r,
                           int x, const void *src, uint8_t *comp, int count);
  void         (*from_comp) (CtxRasterizer *r,
                             int x, const uint8_t *comp, void *dst, int count);
  int          (*crunch) (CtxRasterizer *r, int x, uint8_t *dst, uint8_t *coverage,
                          int count);
  void         (*setup) (CtxRasterizer *r);
};

#endif

struct
  _CtxIterator
{
  int              pos;
  int              in_history;
  CtxRenderstream *renderstream;
  int              end_pos;
  int              flags;

  int              bitpack_pos;
  int              bitpack_length;     // if non 0 bitpack is active
  CtxEntry         bitpack_command[6]; // the command returned to the
  // user if unpacking is needed.
};

#define CTX_MAX_DEVICES 16
#define CTX_MAX_KEYBINDINGS         256

#if CTX_EVENTS 


// YYY include list implementation - since it already is a header+inline online
// implementation?



typedef struct CtxItemCb {
  CtxEventType types;
  CtxCb        cb;
  void*        data1;
  void*        data2;

  void (*finalize) (void *data1, void *data2, void *finalize_data);
  void  *finalize_data;

} CtxItemCb;


#define CTX_MAX_CBS              128

typedef struct CtxItem {
  CtxMatrix inv_matrix;  /* for event coordinate transforms */

  /* bounding box */
  float          x0;
  float          y0;
  float          x1;
  float          y1;

  void *path;
  double          path_hash;

  CtxEventType   types;   /* all cb's ored together */
  CtxItemCb cb[CTX_MAX_CBS];
  int       cb_count;
  int       ref_count;
} CtxItem;


typedef struct _CtxEvents CtxEvents;
struct _CtxEvents
{
  int             frozen;
  int             fullscreen;
  CtxList        *grabs; /* could split the grabs per device in the same way,
                            to make dispatch overhead smaller,. probably
                            not much to win though. */
  CtxItem         *prev[CTX_MAX_DEVICES];
  float            pointer_x[CTX_MAX_DEVICES];
  float            pointer_y[CTX_MAX_DEVICES];
  unsigned char    pointer_down[CTX_MAX_DEVICES];
  CtxEvent         drag_event[CTX_MAX_DEVICES];
  CtxList         *idles;
  CtxList         *events; // for ctx_get_event
  int              ctx_get_event_enabled;
  int              idle_id;
  CtxBinding       bindings[CTX_MAX_KEYBINDINGS]; /*< better as list, uses no mem if unused */
  int              n_bindings;
  int              width;
  int              height;
  CtxList         *items;
  CtxModifierState modifier_state;
  int              tap_delay_min;
  int              tap_delay_max;
  int              tap_delay_hold;
  double           tap_hysteresis;
};


#endif

struct
  _Ctx
{
  CtxImplementation *renderer;
  CtxRenderstream    renderstream;
  CtxState           state;        /**/
  int                transformation;
  CtxBuffer          texture[CTX_MAX_TEXTURES];
  int                rev;
#if CTX_EVENTS 
  CtxEvents          events;
  int                mouse_fd;
  int                mouse_x;
  int                mouse_y;
#endif
#if CTX_CURRENT_PATH
  CtxRenderstream    current_path; // possibly transformed coordinates !
  CtxIterator        current_path_iterator;
#endif
};

#if CTX_EVENTS
int ctx_width (Ctx *ctx)
{
  return ctx->events.width;
}
int ctx_height (Ctx *ctx)
{
  return ctx->events.height;
}

#endif
int ctx_rev (Ctx *ctx)
{
  return ctx->rev;
}

const char *ctx_get_string (Ctx *ctx, uint32_t hash)
{
  return ctx_state_get_string (&ctx->state, hash);
}
float ctx_get_float (Ctx *ctx, uint32_t hash)
{
  return ctx_state_get (&ctx->state, hash);
}
int ctx_get_int (Ctx *ctx, uint32_t hash)
{
  return ctx_state_get (&ctx->state, hash);
}
void ctx_set_float (Ctx *ctx, uint32_t hash, float value)
{
  ctx_state_set (&ctx->state, hash, value);
}
void ctx_set_string (Ctx *ctx, uint32_t hash, const char *value)
{
  ctx_state_set_string (&ctx->state, hash, value);
}
void ctx_set_color (Ctx *ctx, uint32_t hash, CtxColor *color)
{
  ctx_state_set_color (&ctx->state, hash, color);
}
int  ctx_get_color (Ctx *ctx, uint32_t hash, CtxColor *color)
{
  return ctx_state_get_color (&ctx->state, hash, color);
}
int ctx_is_set (Ctx *ctx, uint32_t hash)
{
  return ctx_get_float (ctx, hash) != -0.0f;
}
int ctx_is_set_now (Ctx *ctx, uint32_t hash)
{
  return ctx_is_set (ctx, hash);
}

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
  union
  {
    struct
    {
      CtxEntry *data;
      int length;
      int first_kern;
      /* we've got ~110 bytes to fill to cover as
         much data as stbtt_fontinfo */
      //int16_t glyph_pos[26]; // for a..z
      int       glyphs; // number of glyphs
      uint32_t *index;
    } ctx;
#if CTX_FONT_ENGINE_STB
    struct
    {
      stbtt_fontinfo ttf_info;
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
  if ( (ctx->state.min_x > ctx->state.max_x) ||
       (ctx->state.min_y > ctx->state.max_y) )
    {
      if (x) { *x = 0; }
      if (y) { *y = 0; }
      if (width) { *width = 0; }
      if (height) { *height = 0; }
      return;
    }
  if (ctx->state.min_x < 0)
    { ctx->state.min_x = 0; }
  if (ctx->state.min_y < 0)
    { ctx->state.min_y = 0; }
  if (x) { *x = ctx->state.min_x; }
  if (y) { *y = ctx->state.min_y; }
  if (width) { *width = ctx->state.max_x - ctx->state.min_x; }
  if (height) { *height = ctx->state.max_y - ctx->state.min_y; }
  //fprintf (stderr, "%i %i %ix%i\n", *x, *y, *width, *height);
}

void ctx_process (Ctx *ctx, CtxEntry *entry);

static void // XXX unused
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f)
{
  matrix->m[0][0] = a;
  matrix->m[0][1] = b;
  matrix->m[1][0] = c;
  matrix->m[1][1] = d;
  matrix->m[2][0] = e;
  matrix->m[2][1] = f;
}

static void
ctx_matrix_identity (CtxMatrix *matrix)
{
  matrix->m[0][0] = 1.0f;
  matrix->m[0][1] = 0.0f;
  matrix->m[1][0] = 0.0f;
  matrix->m[1][1] = 1.0f;
  matrix->m[2][0] = 0.0f;
  matrix->m[2][1] = 0.0f;
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
  transform.m[0][0] = 1.0f;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] = x;
  transform.m[2][1] = y;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
ctx_matrix_scale (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = x;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = y;
  transform.m[2][0] = 0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
ctx_matrix_rotate (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_sin = ctx_sinf (angle);
  float val_cos = ctx_cosf (angle);
  transform.m[0][0] =  val_cos;
  transform.m[0][1] = val_sin;
  transform.m[1][0] = -val_sin;
  transform.m[1][1] = val_cos;
  transform.m[2][0] =     0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

#if 0
static void
ctx_matrix_skew_x (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = ctx_tanf (angle);
  transform.m[0][0] =    1.0f;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = val_tan;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
ctx_matrix_skew_y (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = ctx_tanf (angle);
  transform.m[0][0] =    1.0f;
  transform.m[0][1] = val_tan;
  transform.m[1][0] =    0.0f;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}
#endif

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
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_APPLY_TRANSFORM:
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

// expanding arc_to to arc can be the job
// of a layer in front of renderer?
//   doing:
//     rectangle
//     arc
//     ... etc reduction to beziers
//     or even do the reduction to
//     polylines directly here...
//     making the rasterizer able to
//     only do poly-lines? will that be faster?

/* the iterator - should decode bitpacked data as well -
 * making the rasterizers simpler, possibly do unpacking
 * all the way to absolute coordinates.. unless mixed
 * relative/not are wanted.
 */

enum _CtxIteratorFlag
{
  CTX_ITERATOR_FLAT           = 0,
  CTX_ITERATOR_EXPAND_BITPACK = 2,
  CTX_ITERATOR_DEFAULTS       = CTX_ITERATOR_EXPAND_BITPACK
};
typedef enum _CtxIteratorFlag CtxIteratorFlag;

static void
ctx_iterator_init (CtxIterator      *iterator,
                   CtxRenderstream  *renderstream,
                   int               start_pos,
                   int               flags)
{
  iterator->renderstream   = renderstream;
  iterator->flags          = flags;
  iterator->bitpack_pos    = 0;
  iterator->bitpack_length = 0;
  iterator->pos            = start_pos;
  iterator->end_pos        = renderstream->count;
  iterator->in_history     = -1; // -1 is a marker used for first run
  ctx_memset (iterator->bitpack_command, 0, sizeof (iterator->bitpack_command) );
}

static CtxEntry *_ctx_iterator_next (CtxIterator *iterator)
{
  int ret = iterator->pos;
  CtxEntry *entry = &iterator->renderstream->entries[ret];
  if (ret >= iterator->end_pos)
    { return NULL; }
  if (iterator->in_history == 0)
    { iterator->pos += (ctx_conts_for_entry (entry) + 1); }
  iterator->in_history = 0;
  if (iterator->pos >= iterator->end_pos)
    { return NULL; }
  return &iterator->renderstream->entries[iterator->pos];
}

#if CTX_CURRENT_PATH
CtxIterator *
ctx_current_path (Ctx *ctx)
{
  CtxIterator *iterator = &ctx->current_path_iterator;
  ctx_iterator_init (iterator, &ctx->current_path, 0, CTX_ITERATOR_EXPAND_BITPACK);
  return iterator;
}

void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2)
{
  float minx = 50000.0;
  float miny = 50000.0;
  float maxx = -50000.0;
  float maxy = -50000.0;
  float x = 0;
  float y = 0;

  CtxIterator *iterator = ctx_current_path (ctx);
  CtxCommand *command;

  while ((command = ctx_iterator_next (iterator)))
  {
     int got_coord = 0;
     switch (command->code)
     {
        // XXX missing many curve types
        case CTX_LINE_TO:
        case CTX_MOVE_TO:
          x = command->move_to.x;
          y = command->move_to.y;
          got_coord++;
          break;
        case CTX_REL_LINE_TO:
        case CTX_REL_MOVE_TO:
          x += command->move_to.x;
          y += command->move_to.y;
          got_coord++;
          break;
        case CTX_CURVE_TO:
          x = command->curve_to.x;
          y = command->curve_to.y;
          got_coord++;
          break;
        case CTX_REL_CURVE_TO:
          x += command->curve_to.x;
          y += command->curve_to.y;
          got_coord++;
          break;
        case CTX_RECTANGLE:
          x = command->rectangle.x;
          y = command->rectangle.y;
          minx = ctx_minf (minx, x);
          miny = ctx_minf (miny, y);
          maxx = ctx_maxf (maxx, x);
          maxy = ctx_maxf (maxy, y);

          x += command->rectangle.width;
          y += command->rectangle.height;
          got_coord++;
          break;
     }
    if (got_coord)
    {
      minx = ctx_minf (minx, x);
      miny = ctx_minf (miny, y);
      maxx = ctx_maxf (maxx, x);
      maxy = ctx_maxf (maxy, y);
    }
  }
  if (ex1) *ex1 = minx;
  if (ey1) *ey1 = miny;
  if (ex2) *ex2 = maxx;
  if (ey2) *ey2 = maxy;
}


#endif
// 6024x4008
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
  iterator->bitpack_command[0].code =
    iterator->bitpack_command[1].code = CTX_CONT;
  iterator->bitpack_length = 2;
  iterator->bitpack_pos    = 0;
}
#endif

CtxCommand *
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
      return (CtxCommand *) ret;
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
          iterator->bitpack_command[1].code =
          iterator->bitpack_command[2].code = CTX_CONT;
          iterator->bitpack_command[3].code = CTX_REL_LINE_TO;
          // 0.0 here is a common optimization - so check for it
          if (ret->data.s8[6]== 0 && ret->data.s8[7] == 0)
            { iterator->bitpack_length = 3; }
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
        case CTX_RECTANGLE:
          iterator->bitpack_command[0] = ret[0];
          iterator->bitpack_command[1] = ret[1];
          iterator->bitpack_pos = 0;
          iterator->bitpack_length = 2;
          goto again;
        case CTX_ARC:
        case CTX_ARC_TO:
        case CTX_REL_ARC_TO:
        case CTX_SET_COLOR:
        case CTX_RADIAL_GRADIENT:
        case CTX_CURVE_TO:
        case CTX_REL_CURVE_TO:
        case CTX_APPLY_TRANSFORM:
        case CTX_ROUND_RECTANGLE:
          iterator->bitpack_command[0] = ret[0];
          iterator->bitpack_command[1] = ret[1];
          iterator->bitpack_command[2] = ret[2];
          iterator->bitpack_pos = 0;
          iterator->bitpack_length = 3;
          goto again;
        case CTX_TEXT:
        case CTX_TEXT_STROKE:
        case CTX_SET_FONT:
        case CTX_SET:
          iterator->bitpack_length = 0;
          return (CtxCommand *) ret;
        default:
          iterator->bitpack_command[0] = ret[0];
          iterator->bitpack_command[1] = ret[1];
          iterator->bitpack_command[2] = ret[2];
          iterator->bitpack_command[3] = ret[3];
          iterator->bitpack_command[4] = ret[4];
          iterator->bitpack_pos = 0;
          iterator->bitpack_length = 1;
          goto again;
      }
#endif
  return (CtxCommand *) ret;
}

static void
ctx_gstate_push (CtxState *state)
{
  if (state->gstate_no + 1 >= CTX_MAX_STATES)
    { return; }
  state->gstate_stack[state->gstate_no] = state->gstate;
  state->gstate_no++;
  ctx_state_set (state, CTX_new_state, 0.0);
}

static void
ctx_gstate_pop (CtxState *state)
{
  if (state->gstate_no <= 0)
    { return; }
  state->gstate = state->gstate_stack[state->gstate_no-1];
  state->gstate_no--;
}

static void ctx_interpret_style         (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_transforms    (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_pos           (CtxState *state, CtxEntry *entry, void *data);
static void ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data);


static void ctx_renderstream_compact (CtxRenderstream *renderstream);
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
  else if (renderstream->flags & CTX_RENDERSTREAM_CURRENT_PATH)
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
      ctx_renderstream_compact (renderstream);
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
  else if (renderstream->flags & CTX_RENDERSTREAM_CURRENT_PATH)
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else
    {
      ctx_renderstream_compact (renderstream);
    }
  if (new_size < renderstream->size)
    { return; }
  if (renderstream->size == max_size)
    { return; }
  if (new_size < min_size)
    { new_size = min_size; }
  if (new_size < renderstream->count)
    { new_size = renderstream->count + 4; }
  if (new_size >= max_size)
    { new_size = max_size; }
  if (new_size != min_size)
    {
      //ctx_log ("growing renderstream %p to %d\n", renderstream, new_size);
    }
  if (renderstream->entries)
    {
      //printf ("grow %p to %d from %d\n", renderstream, new_size, renderstream->size);
      CtxEntry *ne =  (CtxEntry *) malloc (sizeof (CtxEntry) * new_size);
      memcpy (ne, renderstream->entries, renderstream->size * sizeof (CtxEntry) );
      free (renderstream->entries);
      renderstream->entries = ne;
      //renderstream->entries = (CtxEntry*)malloc (renderstream->entries, sizeof (CtxEntry) * new_size);
    }
  else
    {
      //printf ("allocating for %p %d\n", renderstream, new_size);
      renderstream->entries = (CtxEntry *) malloc (sizeof (CtxEntry) * new_size);
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
  else if (renderstream->flags & CTX_RENDERSTREAM_CURRENT_PATH)
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
      return 0;
    }
  renderstream->entries[renderstream->count] = *entry;
  ret = renderstream->count;
  renderstream->count++;
  return ret;
}

int
ctx_add_single (Ctx *ctx, void *entry)
{
  return ctx_renderstream_add_single (&ctx->renderstream, (CtxEntry *) entry);
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

int ctx_append_renderstream (Ctx *ctx, void *data, int length)
{
  CtxEntry *entries = (CtxEntry *) data;
  if (length % sizeof (CtxEntry) )
    {
      //ctx_log("err\n");
      return -1;
    }
  for (unsigned int i = 0; i < length / sizeof (CtxEntry); i++)
    {
      ctx_renderstream_add_single (&ctx->renderstream, &entries[i]);
    }
  return 0;
}

int ctx_set_renderstream (Ctx *ctx, void *data, int length)
{
  ctx->renderstream.count = 0;
  ctx_append_renderstream (ctx, data, length);
  return 0;
}

int ctx_get_renderstream_count (Ctx *ctx)
{
  return ctx->renderstream.count;
}

int
ctx_add_data (Ctx *ctx, void *data, int length)
{
  if (length % sizeof (CtxEntry) )
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
  return ctx_renderstream_add_entry (&ctx->renderstream, (CtxEntry *) data);
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
  if (!data) { return -1; }
  int length_in_blocks;
  if (length <= 0) { length = strlen ( (char *) data) + 1; }
  length_in_blocks = length / sizeof (CtxEntry);
  length_in_blocks += (length % sizeof (CtxEntry) ) ?1:0;
  if (renderstream->count + length_in_blocks + 4 > renderstream->size)
    { ctx_renderstream_resize (renderstream, renderstream->count * 1.2 + length_in_blocks + 32); }
  if (renderstream->count >= renderstream->size)
    { return -1; }
  renderstream->count += length_in_blocks;
  renderstream->entries[ret].data.u32[0] = length;
  renderstream->entries[ret].data.u32[1] = length_in_blocks;
  memcpy (&renderstream->entries[ret+1], data, length);
  {
    //int reverse = ctx_renderstream_add (renderstream, CTX_DATA_REV);
    CtxEntry entry = {CTX_DATA_REV, {{0},}};
    entry.data.u32[0] = length;
    entry.data.u32[1] = length_in_blocks;
    ctx_renderstream_add_single (renderstream, &entry);
    /* this reverse marker exist to enable more efficient
       front to back traversal, can be ignored in other
       direction, is this needed after string setters as well?
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
  command.data.u8[0] = a;
  command.data.u8[1] = b;
  command.data.u8[2] = c;
  command.data.u8[3] = d;
  command.data.u8[4] = e;
  command.data.u8[5] = f;
  command.data.u8[6] = g;
  command.data.u8[7] = h;
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


static void
ctx_process_cmd_str_with_len (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1, int len)
{
  CtxEntry commands[1 + 2 + len/8];
  ctx_memset (commands, 0, sizeof (commands) );
  commands[0] = ctx_u32 (code, arg0, arg1);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = len;
  commands[1].data.u32[1] = len/9+1;
  memcpy( (char *) &commands[2].data.u8[0], string, len);
  ( (char *) (&commands[2].data.u8[0]) ) [len]=0;
  ctx_process (ctx, commands);
}

static void
ctx_process_cmd_str (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1)
{
  ctx_process_cmd_str_with_len (ctx, code, string, arg0, arg1, strlen (string));
}

void
ctx_close_path (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_CLOSE_PATH);
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

uint8_t *
ctx_get_image_data (Ctx *ctx, int sx, int sy, int sw, int sh, int format, int stride)
{
   // NYI
   return NULL;
}

void
ctx_put_image_data (Ctx *ctx, uint8_t *data, int w, int h, int format, int stride,
                    int dx, int dy, int dirtyX, int dirtyY,
                    int dirtyWidth, int dirtyHeight)
{
   // NYI
}
                    

#if CTX_ENABLE_CMYK
void
ctx_get_cmyka (Ctx *ctx, float *cmyka)
{
  ctx_color_get_CMYKAF (& (ctx->state), &ctx->state.gstate.source.color, cmyka);
}
#endif
void
ctx_get_graya (Ctx *ctx, float *ya)
{
  ctx_color_get_graya (& (ctx->state), &ctx->state.gstate.source.color, ya);
}
//YYY

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
ctx_set_rgb_space (Ctx *ctx, int device_space)
{
  CTX_PROCESS_U8 (CTX_SET_RGB_SPACE, device_space);
}

void
ctx_set_cmyk_space (Ctx *ctx, int device_space)
{
  CTX_PROCESS_U8 (CTX_SET_CMYK_SPACE, device_space);
}
#endif

void ctx_texture (Ctx *ctx, int id, float x, float y)
{
  CtxEntry commands[2];
  if (id < 0) { return; }
  commands[0] = ctx_u32 (CTX_TEXTURE, id, 0);
  commands[1] = ctx_f  (CTX_CONT, x, y);
  ctx_process (ctx, commands);
}

void
ctx_image_path (Ctx *ctx, const char *path, float x, float y)
{
  int id = ctx_texture_load (ctx, -1, path);
  ctx_texture (ctx, id, x, y);
}

void
ctx_set_rgba8 (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_RGBA_U8, r, g, b, a, 0, 0, 0, 0);
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

void ctx_set_drgba (Ctx *ctx, float r, float g, float b, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SET_COLOR, CTX_DRGBA, r),
    ctx_f (CTX_CONT, g, b),
    ctx_f (CTX_CONT, a, 0)
  };
  ctx_process (ctx, command);
}

void ctx_set_rgba (Ctx *ctx, float r, float g, float b, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SET_COLOR, CTX_RGBA, r),
    ctx_f (CTX_CONT, g, b),
    ctx_f (CTX_CONT, a, 0)
  };
  ctx_process (ctx, command);
}

void ctx_set_rgb (Ctx *ctx, float   r, float   g, float   b)
{
  ctx_set_rgba (ctx, r, g, b, 1.0f);
}

void ctx_set_gray (Ctx *ctx, float gray)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SET_COLOR, CTX_GRAY, gray),
    ctx_f (CTX_CONT, 1.0f, 0.0f),
    ctx_f (CTX_CONT, 0.0f, 0.0f)
  };
  ctx_process (ctx, command);
}

#if CTX_ENABLE_CMYK
void ctx_set_cmyk (Ctx *ctx, float c, float m, float y, float k)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SET_COLOR, CTX_CMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, 1.0f)
  };
  ctx_process (ctx, command);
}

void ctx_set_cmyka      (Ctx *ctx, float c, float m, float y, float k, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SET_COLOR, CTX_CMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, a)
  };
  ctx_process (ctx, command);
}

void ctx_set_dcmyk (Ctx *ctx, float c, float m, float y, float k)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SET_COLOR, CTX_DCMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, 1.0f)
  };
  ctx_process (ctx, command);
}

void ctx_set_dcmyka (Ctx *ctx, float c, float m, float y, float k, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SET_COLOR, CTX_DCMYKA, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, a)
  };
  ctx_process (ctx, command);
}

#endif

void
ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1)
{
  CtxEntry command[2]=
  {
    ctx_f (CTX_LINEAR_GRADIENT, x0, y0),
    ctx_f (CTX_CONT,            x1, y1)
  };
  ctx_process (ctx, command);
}

void
ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0, float x1, float y1, float r1)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_RADIAL_GRADIENT, x0, y0),
    ctx_f (CTX_CONT,            r0, x1),
    ctx_f (CTX_CONT,            y1, r1)
  };
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
  ir = CTX_CLAMP (ir, 0,255);
  ig = CTX_CLAMP (ig, 0,255);
  ib = CTX_CLAMP (ib, 0,255);
  ia = CTX_CLAMP (ia, 0,255);
  ctx_gradient_add_stop_u8 (ctx, pos, ir, ig, ib, ia);
}

void ctx_preserve (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_PRESERVE);
}
void ctx_fill (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_FILL);
}
void ctx_stroke (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_STROKE);
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

#if CTX_EVENTS
static void
ctx_collect_events (CtxEvent *event, void *data, void *data2)
{
  Ctx *ctx = data;
  CtxEvent *copy;
  if (event->type == CTX_KEY_DOWN && !strcmp (event->string, "idle"))
          return;
  copy = malloc (sizeof (CtxEvent));
  memcpy (copy, event, sizeof (CtxEvent));
  ctx_list_append_full (&ctx->events.events, copy, (void*)free, NULL);
}
#endif

static void _ctx_bindings_key_down (CtxEvent *event, void *data1, void *data2);

void ctx_reset (Ctx *ctx)
{
  //CTX_PROCESS_VOID (CTX_RESET);
  //if (ctx->transformation & CTX_TRANSFORMATION_STORE_CLEAR)
  //  { return; }
  ctx_empty (ctx);
  ctx_state_init (&ctx->state);
#if CTX_EVENTS
  ctx_list_free (&ctx->events.items);

  if (ctx->events.ctx_get_event_enabled)
  {
    ctx_clear_bindings (ctx);
    ctx_listen_full (ctx, 0, 0, ctx->events.width, ctx->events.height,
                     CTX_PRESS|CTX_RELEASE|CTX_MOTION, ctx_collect_events, ctx, ctx,
                     NULL, NULL);
    ctx_listen_full (ctx, 0, 0, 0,0,
                     CTX_KEY_DOWN, ctx_collect_events, ctx, ctx,
                     NULL, NULL);
    ctx_listen_full (ctx, 0, 0, 0,0,
                     CTX_KEY_UP, ctx_collect_events, ctx, ctx,
                     NULL, NULL);
    ctx_listen_full (ctx, 0,0,0,0,
                     CTX_KEY_DOWN, _ctx_bindings_key_down, ctx, ctx,
                     NULL, NULL);
  }
#endif
}

void ctx_new_path (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_NEW_PATH);
}

void ctx_clip (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_CLIP);
}

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);

void ctx_start_move (Ctx *ctx)
{
  ctx_set (ctx, CTX_start_move, "", 0);
}

void ctx_save (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_SAVE);
}
void ctx_restore (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_RESTORE);
}

void ctx_set_line_width (Ctx *ctx, float x)
{
  if (ctx->state.gstate.line_width != x)
    CTX_PROCESS_F1 (CTX_SET_LINE_WIDTH, x);
}

void
ctx_set_global_alpha (Ctx *ctx, float global_alpha)
{
  if (ctx->state.gstate.global_alpha_f != global_alpha)
    CTX_PROCESS_F1 (CTX_SET_GLOBAL_ALPHA, global_alpha);
}

float
ctx_get_global_alpha (Ctx *ctx)
{
  return ctx->state.gstate.global_alpha_f;
}

void
ctx_set_font_size (Ctx *ctx, float x)
{
  CTX_PROCESS_F1 (CTX_SET_FONT_SIZE, x);
}

void
ctx_set_miter_limit (Ctx *ctx, float limit)
{
  CTX_PROCESS_F1 (CTX_SET_MITER_LIMIT, limit);
}

float ctx_get_font_size  (Ctx *ctx)
{
  return ctx->state.gstate.font_size;
}

float ctx_get_line_width (Ctx *ctx)
{
  return ctx->state.gstate.line_width;
}

static int ctx_strcmp (const char *a, const char *b)
{
  int i;
  for (i = 0; a[i] && b[i]; a++, b++)
    if (a[0] != b[0])
      { return 1; }
  if (a[0] == 0 && b[0] == 0) { return 0; }
  return 1;
}

static int ctx_strncmp (const char *a, const char *b, size_t n)
{
  size_t i;
  for (i = 0; a[i] && b[i] && i < n; a++, b++)
    if (a[0] != b[0])
      { return 1; }
  return 0;
}

static int ctx_strlen (const char *s)
{
  int len = 0;
  for (; *s; s++) { len++; }
  return len;
}

static char *ctx_strstr (const char *h, const char *n)
{
  int needle_len = ctx_strlen (n);
  if (n[0]==0)
    { return (char *) h; }
  while (h)
    {
      h = ctx_strchr (h, n[0]);
      if (!h)
        { return NULL; }
      if (!ctx_strncmp (h, n, needle_len) )
        { return (char *) h; }
      h++;
    }
  return NULL;
}

static int _ctx_resolve_font (const char *name)
{
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (!ctx_strcmp (ctx_fonts[i].name, name) )
        { return i; }
    }
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (ctx_strstr (ctx_fonts[i].name, name) )
        { return i; }
    }
  return -1;
}

static int ctx_resolve_font (const char *name)
{
  int ret = _ctx_resolve_font (name);
  if (ret >= 0)
    { return ret; }
  if (!ctx_strcmp (name, "regular") )
    {
      int ret = _ctx_resolve_font ("sans");
      if (ret >= 0) { return ret; }
      ret = _ctx_resolve_font ("serif");
      if (ret >= 0) { return ret; }
    }
  return 0;
}

void
_ctx_set_font (Ctx *ctx, const char *name)
{
  ctx->state.gstate.font = ctx_resolve_font (name);
}

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len)
{
  if (len <= 0) len = strlen (string);
  ctx_process_cmd_str (ctx, CTX_SET, string, key_hash, len);
}

#include <unistd.h>

const char *
ctx_get (Ctx *ctx, const char *key)
{
  static char retbuf[32];
  int len = 0;
  CTX_PROCESS_U32(CTX_GET, ctx_strhash (key, 0), 0);
  while (read (STDIN_FILENO, &retbuf[len], 1) != -1)
    {
      if(retbuf[len]=='\n')
        break;
      retbuf[++len]=0;
    }
  return retbuf;
}

void
ctx_set_font (Ctx *ctx, const char *name)
{
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_SET_FONT, name, 0, 0);
#else
  _ctx_set_font (ctx, name);
#endif
}

void
ctx_identity (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_IDENTITY);
}

void
ctx_apply_transform (Ctx *ctx, float a, float b,  // hscale, hskew
                     float c, float d,  // vskew,  vscale
                     float e, float f)  // htran,  vtran
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_APPLY_TRANSFORM, a, b),
    ctx_f (CTX_CONT,            c, d),
    ctx_f (CTX_CONT,            e, f)
  };
  ctx_process (ctx, command);
}

void
ctx_get_transform  (Ctx *ctx, float *a, float *b,
                    float *c, float *d,
                    float *e, float *f)
{
  if (a) { *a = ctx->state.gstate.transform.m[0][0]; }
  if (b) { *b = ctx->state.gstate.transform.m[0][1]; }
  if (c) { *c = ctx->state.gstate.transform.m[1][0]; }
  if (d) { *d = ctx->state.gstate.transform.m[1][1]; }
  if (e) { *e = ctx->state.gstate.transform.m[2][0]; }
  if (f) { *f = ctx->state.gstate.transform.m[2][1]; }
}

void ctx_apply_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_apply_transform (ctx,
                       matrix->m[0][0], matrix->m[0][1],
                       matrix->m[1][0], matrix->m[1][1],
                       matrix->m[2][0], matrix->m[2][1]);
}

void ctx_get_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  *matrix = ctx->state.gstate.transform;
}

void ctx_set_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_identity (ctx);
  ctx_apply_matrix (ctx, matrix);
}

void ctx_rotate (Ctx *ctx, float x)
{
  CTX_PROCESS_F1 (CTX_ROTATE, x);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->renderstream.count--; }
}

void ctx_scale (Ctx *ctx, float x, float y)
{
  CTX_PROCESS_F (CTX_SCALE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->renderstream.count--; }
}

void ctx_translate (Ctx *ctx, float x, float y)
{
  CTX_PROCESS_F (CTX_TRANSLATE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->renderstream.count--; }
}

void ctx_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    { CTX_PROCESS_F (CTX_MOVE_TO, x, y); }
  else
    { CTX_PROCESS_F (CTX_LINE_TO, x, y); }
}

void ctx_move_to (Ctx *ctx, float x, float y)
{
  CTX_PROCESS_F (CTX_MOVE_TO,x,y);
}

void ctx_curve_to (Ctx *ctx, float x0, float y0,
                   float x1, float y1,
                   float x2, float y2)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_CURVE_TO, x0, y0),
    ctx_f (CTX_CONT,     x1, y1),
    ctx_f (CTX_CONT,     x2, y2)
  };
  ctx_process (ctx, command);
}

void ctx_round_rectangle (Ctx *ctx,
                          float x0, float y0,
                          float w, float h,
                          float radius)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_RECTANGLE, x0, y0),
    ctx_f (CTX_CONT,      w, h),
    ctx_f (CTX_CONT,      radius, 0)
  };
  ctx_process (ctx, command);
}

void ctx_rectangle (Ctx *ctx,
                    float x0, float y0,
                    float w, float h)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_RECTANGLE, x0, y0),
    ctx_f (CTX_CONT,      w, h)
  };
  ctx_process (ctx, command);
}

void ctx_rel_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    { return; }
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
  if (ctx->state.gstate.line_cap != cap)
    CTX_PROCESS_U8 (CTX_SET_LINE_CAP, cap);
}
void ctx_set_fill_rule (Ctx *ctx, CtxFillRule fill_rule)
{
  if (ctx->state.gstate.fill_rule != fill_rule)
    CTX_PROCESS_U8 (CTX_SET_FILL_RULE, fill_rule);
}
void ctx_set_line_join (Ctx *ctx, CtxLineJoin join)
{
  if (ctx->state.gstate.line_join != join)
    CTX_PROCESS_U8 (CTX_SET_LINE_JOIN, join);
}

void ctx_set_blend_mode (Ctx *ctx, CtxBlend mode)
{
  CTX_PROCESS_U8 (CTX_SET_BLEND_MODE, mode);
}

void ctx_set_compositing_mode (Ctx *ctx, CtxCompositingMode mode)
{
  CTX_PROCESS_U8 (CTX_SET_COMPOSITING_MODE, mode);
}
void ctx_set_text_align (Ctx *ctx, CtxTextAlign text_align)
{
  CTX_PROCESS_U8 (CTX_SET_TEXT_ALIGN, text_align);
}
void ctx_set_text_baseline (Ctx *ctx, CtxTextBaseline text_baseline)
{
  CTX_PROCESS_U8 (CTX_SET_TEXT_BASELINE, text_baseline);
}
void ctx_set_text_direction (Ctx *ctx, CtxTextDirection text_direction)
{
  CTX_PROCESS_U8 (CTX_SET_TEXT_DIRECTION, text_direction);
}

void
ctx_rel_curve_to (Ctx *ctx,
                  float x0, float y0,
                  float x1, float y1,
                  float x2, float y2)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[3]=
  {
    ctx_f (CTX_REL_CURVE_TO, x0, y0),
    ctx_f (CTX_CONT, x1, y1),
    ctx_f (CTX_CONT, x2, y2)
  };
  ctx_process (ctx, command);
}

void
ctx_rel_quad_to (Ctx *ctx,
                 float cx, float cy,
                 float x,  float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[2]=
  {
    ctx_f (CTX_REL_QUAD_TO, cx, cy),
    ctx_f (CTX_CONT, x, y)
  };
  ctx_process (ctx, command);
}

void
ctx_quad_to (Ctx *ctx,
             float cx, float cy,
             float x,  float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[2]=
  {
    ctx_f (CTX_QUAD_TO, cx, cy),
    ctx_f (CTX_CONT, x, y)
  };
  ctx_process (ctx, command);
}

void ctx_arc (Ctx  *ctx,
              float x0, float y0,
              float radius,
              float angle1, float angle2,
              int   direction)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_ARC, x0, y0),
    ctx_f (CTX_CONT, radius, angle1),
    ctx_f (CTX_CONT, angle2, direction)
  };
  ctx_process (ctx, command);
}

static int ctx_coords_equal (float x1, float y1, float x2, float y2, float tol)
{
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
}

static float
ctx_point_seg_dist_sq (float x, float y,
                       float vx, float vy, float wx, float wy)
{
  float l2 = ctx_pow2 (vx-wx) + ctx_pow2 (vy-wy);
  if (l2 < 0.0001)
    { return ctx_pow2 (x-vx) + ctx_pow2 (y-vy); }
  float t = ( (x - vx) * (wx - vx) + (y - vy) * (wy - vy) ) / l2;
  t = ctx_maxf (0, ctx_minf (1, t) );
  float ix = vx + t * (wx - vx);
  float iy = vy + t * (wy - vy);
  return ctx_pow2 (x-ix) + ctx_pow2 (y-iy);
}

static void
ctx_normalize (float *x, float *y)
{
  float length = ctx_hypotf ( (*x), (*y) );
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
  // XXX : should partially move into rasterizer to preserve comand
  //       even if an arc preserves all geometry, just to ensure roundtripping
  //       of data
  /* from nanovg - but not quite working ; uncertain if arc or wrong
   * transfusion is the cause.
   */
  float x0 = ctx->state.x;
  float y0 = ctx->state.y;
  float dx0,dy0, dx1,dy1, a, d, cx,cy, a0,a1;
  int dir;
  if (!ctx->state.has_moved)
    { return; }
  if (1)
    {
      // Handle degenerate cases.
      if (ctx_coords_equal (x0,y0, x1,y1, 0.5f) ||
          ctx_coords_equal (x1,y1, x2,y2, 0.5f) ||
          ctx_point_seg_dist_sq (x1,y1, x0,y0, x2,y2) < 0.5 ||
          radius < 0.5)
        {
          ctx_line_to (ctx, x1,y1);
          return;
        }
    }
  // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
  dx0 = x0-x1;
  dy0 = y0-y1;
  dx1 = x2-x1;
  dy1 = y2-y1;
  ctx_normalize (&dx0,&dy0);
  ctx_normalize (&dx1,&dy1);
  a = ctx_acosf (dx0*dx1 + dy0*dy1);
  d = radius / ctx_tanf (a/2.0f);
#if 0
  if (d > 10000.0f)
    {
      ctx_line_to (ctx, x1, y1);
      return;
    }
#endif
  if ( (dx1*dy0 - dx0*dy1) > 0.0f)
    {
      cx = x1 + dx0*d + dy0*radius;
      cy = y1 + dy0*d + -dx0*radius;
      a0 = ctx_atan2f (dx0, -dy0);
      a1 = ctx_atan2f (-dx1, dy1);
      dir = 0;
    }
  else
    {
      cx = x1 + dx0*d + -dy0*radius;
      cy = y1 + dy0*d + dx0*radius;
      a0 = ctx_atan2f (-dx0, dy0);
      a1 = ctx_atan2f (dx1, -dy1);
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
ctx_exit (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_EXIT);
}

#include <stdio.h>
#include <unistd.h>

void
ctx_flush (Ctx *ctx)
{
  ctx->rev++;
//  CTX_PROCESS_VOID (CTX_FLUSH);
#if 0
  //printf (" \e[?2222h");
  ctx_renderstream_compact (&ctx->renderstream);
  for (int i = 0; i < ctx->renderstream.count - 1; i++)
    {
      CtxEntry *entry = &ctx->renderstream.entries[i];
      fwrite (entry, 9, 1, stdout);
#if 0
      uint8_t  *buf = (void *) entry;
      for (int j = 0; j < 9; j++)
        { printf ("%c", buf[j]); }
#endif
    }
  printf ("Xx.Xx.Xx.");
  fflush (NULL);
#endif
  if (ctx->renderer && ctx->renderer->flush)
    ctx->renderer->flush (ctx->renderer);
  ctx->renderstream.count = 0;
  ctx_state_init (&ctx->state);
}

////////////////////////////////////////

static void
ctx_matrix_invert (CtxMatrix *m)
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
  CtxCommand *c = (CtxCommand *) entry;
  switch (entry->code)
    {
      case CTX_SET_LINE_WIDTH:
        state->gstate.line_width = ctx_arg_float (0);
        break;
      case CTX_SET_LINE_CAP:
        state->gstate.line_cap = (CtxLineCap) ctx_arg_u8 (0);
        break;
      case CTX_SET_FILL_RULE:
        state->gstate.fill_rule = (CtxFillRule) ctx_arg_u8 (0);
        break;
      case CTX_SET_LINE_JOIN:
        state->gstate.line_join = (CtxLineJoin) ctx_arg_u8 (0);
        break;
      case CTX_SET_COMPOSITING_MODE:
        state->gstate.compositing_mode = (CtxCompositingMode) ctx_arg_u8 (0);
        break;
      case CTX_SET_BLEND_MODE:
        state->gstate.blend_mode = (CtxBlend) ctx_arg_u8 (0);
        break;
      case CTX_SET_TEXT_ALIGN:
        ctx_state_set (state, CTX_text_align, ctx_arg_u8 (0) );
        break;
      case CTX_SET_TEXT_BASELINE:
        ctx_state_set (state, CTX_text_baseline, ctx_arg_u8 (0) );
        break;
      case CTX_SET_TEXT_DIRECTION:
        ctx_state_set (state, CTX_text_direction, ctx_arg_u8 (0) );
        break;
      case CTX_SET_GLOBAL_ALPHA:
        state->gstate.global_alpha_u8 = ctx_float_to_u8 (ctx_arg_float (0) );
        state->gstate.global_alpha_f = ctx_arg_float (0);
        break;
      case CTX_SET_FONT_SIZE:
        state->gstate.font_size = ctx_arg_float (0);
        break;
      case CTX_SET_MITER_LIMIT:
        state->gstate.miter_limit = ctx_arg_float (0);
        break;
      case CTX_SET_COLOR:
        {
          CtxColor *color = &state->gstate.source.color;
          state->gstate.source.type = CTX_SOURCE_COLOR;
          switch ( (int) ctx_arg_float (0) )
            {
              case CTX_RGB:
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, 1.0f);
                break;
              case CTX_RGBA:
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
              case CTX_DRGBA:
                ctx_color_set_drgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYKA:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_CMYK:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
              case CTX_DCMYKA:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_DCMYK:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
#endif
              case CTX_GRAYA:
                ctx_color_set_graya (state, color, c->graya.g, c->graya.a);
                break;
              case CTX_GRAY:
                ctx_color_set_graya (state, color, c->graya.g, 1.0f);
                break;
            }
        }
        break;
      case CTX_SET_RGBA_U8:
        //ctx_source_deinit (&state->gstate.source);
        state->gstate.source.type = CTX_SOURCE_COLOR;
        ctx_color_set_RGBA8 (state, &state->gstate.source.color,
                             ctx_arg_u8 (0),
                             ctx_arg_u8 (1),
                             ctx_arg_u8 (2),
                             ctx_arg_u8 (3) );
        //for (int i = 0; i < 4; i ++)
        //  state->gstate.source.color.rgba[i] = ctx_arg_u8(i);
        break;
#if 0
      case CTX_SET_RGBA_STROKE:
        //ctx_source_deinit (&state->gstate.source);
        state->gstate.source_stroke = state->gstate.source;
        state->gstate.source_stroke.type = CTX_SOURCE_COLOR;
        for (int i = 0; i < 4; i ++)
          { state->gstate.source_stroke.color.rgba[i] = ctx_arg_u8 (i); }
        break;
#endif
      //case CTX_TEXTURE:
      //  state->gstate.source.type = CTX_SOURCE_
      //  break;
      case CTX_LINEAR_GRADIENT:
        {
          float x0 = ctx_arg_float (0);
          float y0 = ctx_arg_float (1);
          float x1 = ctx_arg_float (2);
          float y1 = ctx_arg_float (3);
          float dx, dy, length, start, end;
          ctx_user_to_device (state, &x0, &y0);
          ctx_user_to_device (state, &x1, &y1);
          length = ctx_hypotf (x1-x0,y1-y0);
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
          ctx_matrix_invert (&state->gstate.source.transform);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          float x0 = ctx_arg_float (0);
          float y0 = ctx_arg_float (1);
          float r0 = ctx_arg_float (2);
          float x1 = ctx_arg_float (3);
          float y1 = ctx_arg_float (4);
          float r1 = ctx_arg_float (5);
          state->gstate.source.radial_gradient.x0 = x0;
          state->gstate.source.radial_gradient.y0 = y0;
          state->gstate.source.radial_gradient.r0 = r0;
          state->gstate.source.radial_gradient.x1 = x1;
          state->gstate.source.radial_gradient.y1 = y1;
          state->gstate.source.radial_gradient.r1 = r1;
          state->gstate.source.type      = CTX_SOURCE_RADIAL_GRADIENT;
          state->gstate.source.transform = state->gstate.transform;
          ctx_matrix_invert (&state->gstate.source.transform);
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
                              ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_SCALE:
        ctx_matrix_scale (&state->gstate.transform,
                          ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_ROTATE:
        ctx_matrix_rotate (&state->gstate.transform, ctx_arg_float (0) );
        break;
      case CTX_APPLY_TRANSFORM:
        {
          CtxMatrix m;
          ctx_matrix_set (&m,
                          ctx_arg_float (0), ctx_arg_float (1),
                          ctx_arg_float (2), ctx_arg_float (3),
                          ctx_arg_float (4), ctx_arg_float (5) );
          ctx_matrix_multiply (&state->gstate.transform,
                               &state->gstate.transform, &m); // XXX verify order
        }
#if 0
        ctx_matrix_set (&state->gstate.transform,
                        ctx_arg_float (0), ctx_arg_float (1),
                        ctx_arg_float (2), ctx_arg_float (3),
                        ctx_arg_float (4), ctx_arg_float (5) );
#endif
        break;
    }
}

static inline void
ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y)
{
  float x_in = *x;
  float y_in = *y;
  *x = ( (x_in * m->m[0][0]) + (y_in * m->m[1][0]) + m->m[2][0]);
  *y = ( (y_in * m->m[1][1]) + (x_in * m->m[0][1]) + m->m[2][1]);
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
            { max_dev = entry[c].data.f[d]; }
          if (entry[c].data.f[d] < -max_dev)
            { max_dev = -entry[c].data.f[d]; }
        }
    }
  return max_dev;
}

static void
pack_s8_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      { entry[0].data.s8[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV; }
}

static void
pack_s16_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      { entry[0].data.s16[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV; }
}
#endif

#if CTX_BITPACK
static void
ctx_renderstream_remove_tiny_curves (CtxRenderstream *renderstream, int start_pos)
{
  CtxIterator iterator;
  if ( (renderstream->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_iterator_init (&iterator, renderstream, start_pos, CTX_ITERATOR_FLAT);
  iterator.end_pos = renderstream->count - 5;
  CtxCommand *command = NULL;
  while ( (command = ctx_iterator_next (&iterator) ) )
    {
      CtxEntry *entry = &command->entry;
      /* things smaller than this have probably been scaled down
         beyond recognition, bailing for both better packing and less rasterization work
       */
      if (command[0].code == CTX_REL_CURVE_TO)
        {
          float max_dev = find_max_dev (entry, 3);
          if (max_dev < 1.0)
            {
              entry[0].code = CTX_REL_LINE_TO;
              entry[0].data.f[0] = entry[2].data.f[0];
              entry[0].data.f[1] = entry[2].data.f[1];
              entry[1].code = CTX_NOP;
              entry[2].code = CTX_NOP;
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
  if ( (renderstream->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_renderstream_remove_tiny_curves (renderstream, renderstream->bitpack_pos);
  i = renderstream->bitpack_pos;
  if (start_pos > i)
    { i = start_pos; }
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
          ctx_fabsf (entry[2].data.f[0] - 1.0f) < 0.02f &&
          ctx_fabsf (entry[3].data.f[1] - 1.0f) < 0.02f)
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
                  ctx_arg_s8 (6) =
                    ctx_arg_s8 (7) = 0;
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
          entry[0]      = entry[2];
          entry[0].code = CTX_MOVE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
        }
#endif
#if 1
      else if ( (entry[0].code == CTX_MOVE_TO &&
                 entry[1].code == CTX_MOVE_TO) ||
                (entry[0].code == CTX_REL_MOVE_TO &&
                 entry[1].code == CTX_MOVE_TO) )
        {
          entry[0]      = entry[1];
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
        { *tentry = *sentry; }
      source ++;
      target ++;
    }
  renderstream->count -= removed;
  renderstream->bitpack_pos = renderstream->count;
#endif
}

#endif

#if CTX_BITPACK_PACKER
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

static void
ctx_renderstream_compact (CtxRenderstream *renderstream)
{
#if CTX_BITPACK_PACKER
  int last_history;
  last_history = ctx_last_history (renderstream);
#else
  if (renderstream) {};
#endif
#if CTX_BITPACK_PACKER
  ctx_renderstream_bitpack (renderstream, last_history);
#endif
}

/*
 * this transforms the contents of entry according to ctx->transformation
 */
static void
ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data)
{
  CtxCommand *c = (CtxCommand *) entry;
  float start_x = state->x;
  float start_y = state->y;
  int had_moved = state->has_moved;
  switch (entry->code)
    {
      case CTX_MOVE_TO:
      case CTX_LINE_TO:
        {
          float x = c->c.x0;
          float y = c->c.y0;
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              ctx_user_to_device (state, &x, &y);
              ctx_arg_float (0) = x;
              ctx_arg_float (1) = y;
            }
        }
        break;
      case CTX_ARC:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            float temp;
            ctx_user_to_device (state, &c->arc.x, &c->arc.y);
            temp = 0;
            ctx_user_to_device_distance (state, &c->arc.radius, &temp);
          }
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_user_to_device (state, &c->linear_gradient.x1, &c->linear_gradient.y1);
        ctx_user_to_device (state, &c->linear_gradient.x2, &c->linear_gradient.y2);
        break;
      case CTX_RADIAL_GRADIENT:
        {
          float temp;
          ctx_user_to_device (state, &c->radial_gradient.x1, &c->radial_gradient.y1);
          temp = 0;
          ctx_user_to_device_distance (state, &c->radial_gradient.r1, &temp);
          ctx_user_to_device (state, &c->radial_gradient.x2, &c->radial_gradient.y2);
          temp = 0;
          ctx_user_to_device_distance (state, &c->radial_gradient.r2, &temp);
        }
        break;
      case CTX_CURVE_TO:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
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
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
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
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
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
              { entry->code = CTX_MOVE_TO; }
            else
              { entry->code = CTX_LINE_TO; }
          }
        break;
      case CTX_REL_CURVE_TO:
        {
          float nx = state->x + ctx_arg_float (4);
          float ny = state->y + ctx_arg_float (5);
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              for (int c = 0; c < 3; c ++)
                {
                  float x = nx + entry[c].data.f[0];
                  float y = ny + entry[c].data.f[1];
                  ctx_user_to_device (state, &x, &y);
                  entry[c].data.f[0] = x;
                  entry[c].data.f[1] = y;
                }
              entry->code = CTX_CURVE_TO;
            }
        }
        break;
      case CTX_REL_QUAD_TO:
        {
          float nx = state->x + ctx_arg_float (2);
          float ny = state->y + ctx_arg_float (3);
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              for (int c = 0; c < 2; c ++)
                {
                  float x = nx + entry[c].data.f[0];
                  float y = ny + entry[c].data.f[1];
                  ctx_user_to_device (state, &x, &y);
                  entry[c].data.f[0] = x;
                  entry[c].data.f[1] = y;
                }
              entry->code = CTX_QUAD_TO;
            }
        }
        break;
    }
  if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_RELATIVE) )
    {
      int components = 0;
      ctx_user_to_device (state, &start_x, &start_y);
      switch (entry->code)
        {
          case CTX_MOVE_TO:
            if (had_moved) { components = 1; }
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

// for cmm - permit setting resolved babl fishes?
//
// for a pair of profiles, generate atob and btoa fishes
// and set them
//
// can override bridges for
//
// rgb   <> drb
// cmyk  <> drgb
// dcmyk <> cmyk

static void
ctx_interpret_pos_bare (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
    {
      case CTX_RESET:
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
        {
          float x = ctx_arg_float (0);
          float y = ctx_arg_float (1);
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
        state->x = ctx_arg_float (0) + ctx_cosf (ctx_arg_float (4) ) * ctx_arg_float (2);
        state->y = ctx_arg_float (1) + ctx_sinf (ctx_arg_float (4) ) * ctx_arg_float (2);
        break;
      case CTX_REL_MOVE_TO:
      case CTX_REL_LINE_TO:
        state->x += ctx_arg_float (0);
        state->y += ctx_arg_float (1);
        break;
      case CTX_REL_CURVE_TO:
        state->x += ctx_arg_float (4);
        state->y += ctx_arg_float (5);
        break;
      case CTX_REL_QUAD_TO:
        state->x += ctx_arg_float (2);
        state->y += ctx_arg_float (3);
        break;
        // XXX missing some smooths
    }
}

static void
ctx_interpret_pos (CtxState *state, CtxEntry *entry, void *data)
{
  if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) ||
       ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_RELATIVE) )
    {
      ctx_interpret_pos_transform (state, entry, data);
    }
  ctx_interpret_pos_bare (state, entry, data);
}

static void
ctx_state_init (CtxState *state)
{
  ctx_memset (state, 0, sizeof (CtxState) );
  state->gstate.global_alpha_u8 = 255;
  state->gstate.global_alpha_f  = 1.0;
  state->gstate.font_size       = 12;
  state->gstate.line_width      = 2.0;
  ctx_state_set (state, CTX_line_spacing, 1.0f);
  state->min_x                  = 8192;
  state->min_y                  = 8192;
  state->max_x                  = -8192;
  state->max_y                  = -8192;
  ctx_matrix_identity (&state->gstate.transform);
}

void _ctx_set_transformation (Ctx *ctx, int transformation)
{
  ctx->transformation = transformation;
}

static void
_ctx_init (Ctx *ctx)
{
  ctx_state_init (&ctx->state);
  ctx->renderer = NULL;
#if CTX_CURRENT_PATH
  ctx->current_path.flags |= CTX_RENDERSTREAM_CURRENT_PATH;
#endif
  //ctx->transformation |= (CtxTransformation) CTX_TRANSFORMATION_SCREEN_SPACE;
  //ctx->transformation |= (CtxTransformation) CTX_TRANSFORMATION_RELATIVE;
#if CTX_BITPACK
  ctx->renderstream.flags |= CTX_TRANSFORMATION_BITPACK;
#endif
}

static void ctx_setup ();

#if CTX_RENDERSTREAM_STATIC
static Ctx ctx_state;
#endif

void ctx_set_renderer (Ctx  *ctx,
                       void *renderer)
{
  if (ctx->renderer && ctx->renderer->free)
    ctx->renderer->free (ctx->renderer);
  ctx->renderer = (CtxImplementation*)renderer;
}

Ctx *
ctx_new (void)
{
  ctx_setup ();
#if CTX_RENDERSTREAM_STATIC
  Ctx *ctx = &ctx_state;
#else
  Ctx *ctx = (Ctx *) malloc (sizeof (Ctx) );
#endif
  ctx_memset (ctx, 0, sizeof (Ctx) );
  _ctx_init (ctx);
  return ctx;
}

void
ctx_renderstream_deinit (CtxRenderstream *renderstream)
{
#if !CTX_RENDERSTREAM_STATIC
  if (renderstream->entries && ! (renderstream->flags & CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES) )
    { free (renderstream->entries); }
#endif
  renderstream->entries = NULL;
}

#if CTX_RASTERIZER
static void ctx_rasterizer_deinit (CtxRasterizer *rasterizer);
#endif

static void ctx_deinit (Ctx *ctx)
{
  if (ctx->renderer)
    {
      if (ctx->renderer->free)
        ctx->renderer->free (ctx->renderer);
      ctx->renderer    = NULL;
    }
  ctx_renderstream_deinit (&ctx->renderstream);
#if CTX_CURRENT_PATH
  ctx_renderstream_deinit (&ctx->current_path);
#endif
}

void ctx_free (Ctx *ctx)
{
  if (!ctx)
    { return; }
  ctx_deinit (ctx);
#if !CTX_RENDERSTREAM_STATIC
  free (ctx);
#endif
}

Ctx *ctx_new_for_renderstream (void *data, size_t length)
{
  Ctx *ctx = ctx_new ();
  ctx->renderstream.flags   |= CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES;
  ctx->renderstream.entries  = (CtxEntry *) data;
  ctx->renderstream.count    = length / sizeof (CtxEntry);
  return ctx;
}

#if CTX_RASTERIZER
////////////////////////////////////


static CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format);

CtxBuffer *ctx_buffer_new (void)
{
  CtxBuffer *buffer = (CtxBuffer *) malloc (sizeof (CtxBuffer) );
  ctx_memset (buffer, 0, sizeof (CtxBuffer) );
  return buffer;
}

void ctx_buffer_set_data (CtxBuffer *buffer,
                          void *data, int width, int height,
                          int stride,
                          CtxPixelFormat pixel_format,
                          void (*freefunc) (void *pixels, void *user_data),
                          void *user_data)
{
  if (buffer->free_func)
    { buffer->free_func (buffer->data, buffer->user_data); }
  buffer->data      = data;
  buffer->width     = width;
  buffer->height    = height;
  buffer->stride    = stride;
  buffer->format    = ctx_pixel_format_info (pixel_format);
  buffer->free_func = freefunc;
  buffer->user_data = user_data;
}

CtxBuffer *ctx_buffer_new_for_data (void *data, int width, int height,
                                    int stride,
                                    CtxPixelFormat pixel_format,
                                    void (*freefunc) (void *pixels, void *user_data),
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
    { buffer->free_func (buffer->data, buffer->user_data); }
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
    { return -1; }
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
  buffer->free_func = (void *) free;
  buffer->user_data = NULL;
  return 0;
#else
  if (path) {};
  return -1;
#endif
}

void ctx_texture_release (Ctx *ctx, int id)
{
  if (id < 0 || id >= CTX_MAX_TEXTURES)
    { return; }
  ctx_buffer_deinit (&ctx->texture[id]);
}

static int ctx_allocate_texture_id (Ctx *ctx, int id)
{
  if (id < 0 || id >= CTX_MAX_TEXTURES)
    {
      for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
        if (ctx->texture[i].data == NULL)
          { return i; }
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
    { return -1; }
  upng_header (upng);
  upng_decode (upng);
  components     = upng_get_components (upng);
  buffer->width  = upng_get_width (upng);
  buffer->height = upng_get_height (upng);
  buffer->data   = upng_steal_buffer (upng);
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
  buffer->free_func = (void *) free;
  buffer->user_data = NULL;
  return 0;
#else
  if (data && length) {};
  return -1;
#endif
}

int ctx_texture_load_memory (Ctx *ctx, int id, const char *data, int length)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    { return id; }
  if (ctx_buffer_load_memory (&ctx->texture[id], data, length) )
    {
      return -1;
    }
  return id;
}

int ctx_texture_load (Ctx *ctx, int id, const char *path)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    { return id; }
  if (ctx_buffer_load_png (&ctx->texture[id], path) )
    {
      return -1;
    }
  return id;
}

int ctx_texture_init (Ctx *ctx, int id, int width, int height, int bpp,
                      uint8_t *pixels,
                      void (*freefunc) (void *pixels, void *user_data),
                      void *user_data)
{
  id = ctx_allocate_texture_id (ctx, id);
  if (id < 0)
    { return id; }
  ctx_buffer_deinit (&ctx->texture[id]);
  ctx_buffer_set_data (&ctx->texture[id],
                       pixels, width, height, width * (bpp/8), bpp==32?CTX_FORMAT_RGBA8:CTX_FORMAT_RGB8, freefunc, user_data);
  return id;
}

#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_reset (void);
#endif

static void
ctx_state_gradient_clear_stops (CtxState *state)
{
  state->gradient.n_stops = 0;
}

static void
ctx_rasterizer_gradient_add_stop (CtxRasterizer *rasterizer, float pos, float *rgba)
{
  CtxGradient *gradient = &rasterizer->state->gradient;
  CtxGradientStop *stop = &gradient->stops[gradient->n_stops];
  stop->pos = pos;
  ctx_color_set_rgba (rasterizer->state, & (stop->color), rgba[0], rgba[1], rgba[2], rgba[3]);
  if (gradient->n_stops < 15) //we'll keep overwriting the last when out of stops
    { gradient->n_stops++; }
}

static int ctx_rasterizer_add_point (CtxRasterizer *rasterizer, int x1, int y1)
{
  int16_t args[4];
  if (y1 < rasterizer->scan_min)
    { rasterizer->scan_min = y1; }
  if (y1 > rasterizer->scan_max)
    { rasterizer->scan_max = y1; }
  if (x1 < rasterizer->col_min)
    { rasterizer->col_min = x1; }
  if (x1 > rasterizer->col_max)
    { rasterizer->col_max = x1; }
  args[0]=0;
  args[1]=0;
  args[2]=x1;
  args[3]=y1;
  return ctx_renderstream_add_u32 (&rasterizer->edge_list, CTX_EDGE, (uint32_t *) args);
}

#define CTX_SHAPE_CACHE_PRIME1   7853
#define CTX_SHAPE_CACHE_PRIME2   4129
#define CTX_SHAPE_CACHE_PRIME3   3371
#define CTX_SHAPE_CACHE_PRIME4   4221

float ctx_shape_cache_rate = 0.0;
#if CTX_SHAPE_CACHE

static uint32_t ctx_shape_time = 0;

/* to get better cache usage-  */

struct _CtxShapeEntry
{
  uint32_t hash;
  uint16_t width;
  uint16_t height;
  uint32_t refs;
  uint32_t age;   // time last used
  uint32_t uses;  // instrumented for longer keep-alive
  uint8_t  data[];
};

typedef struct _CtxShapeEntry CtxShapeEntry;


// this needs a max-size
// and a more agressive freeing when
// size is about to be exceeded

struct _CtxShapeCache
{
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
static CtxShapeEntry *ctx_shape_entry_find (uint32_t hash, int width, int height, uint32_t time)
{
  int entry_no = ( (hash >> 10) ^ (hash & 1023) ) % CTX_SHAPE_CACHE_ENTRIES;
  int i;
  {
    static int i = 0;
    i++;
    if (i>512)
      {
        ctx_shape_cache_rate = hits * 100.0  / (hits+misses);
        i = 0;
        hits = 0;
        misses = 0;
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
            { ctx_cache.entries[i]->uses++; }
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
                    { ctx_cache.entries[i+1]->uses++; }
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
                            { ctx_cache.entries[i+2]->uses++; }
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
// ==90718==    at 0x1189EF: ctx_rasterizer_generate_coverage (ctx.h:4786)
// ==90718==    by 0x118E57: ctx_rasterizer_rasterize_edges (ctx.h:4907)
//
  int size = sizeof (CtxShapeEntry) + width * height + 1;
  CtxShapeEntry *new_entry = (CtxShapeEntry *) malloc (size);
  new_entry->refs = 1;
  if (ctx_cache.entries[i])
    {
      CtxShapeEntry *entry = ctx_cache.entries[i];
      while (entry->refs) {};
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


static uint32_t ctx_rasterizer_poly_to_hash (CtxRasterizer *rasterizer)
{
  int16_t x = 0;
  int16_t y = 0;
  CtxEntry *entry = &rasterizer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = ox;//(ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += oy; //(oy % CTX_RASTERIZER_AA);
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      x = entry->data.s16[2];
      y = entry->data.s16[3];
      int dx = x-ox;
      int dy = y-oy;
      ox = x;
      oy = y;
      hash *= CTX_SHAPE_CACHE_PRIME3;
      hash += dx;
      hash *= CTX_SHAPE_CACHE_PRIME4;
      hash += dy;
    }
  return hash;
}


static uint32_t ctx_rasterizer_poly_to_edges (CtxRasterizer *rasterizer)
{
  int16_t x = 0;
  int16_t y = 0;
  if (rasterizer->edge_list.count == 0)
     return 0;
#if CTX_SHAPE_CACHE
  CtxEntry *entry = &rasterizer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = (ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += (oy % CTX_RASTERIZER_AA);
#endif
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
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
          *entry = ctx_s16 (CTX_EDGE_FLIPPED,
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

static void ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y);

static void ctx_rasterizer_finish_shape (CtxRasterizer *rasterizer)
{
  if (rasterizer->has_shape && rasterizer->has_prev)
    {
      ctx_rasterizer_line_to (rasterizer, rasterizer->first_x, rasterizer->first_y);
      rasterizer->has_prev = 0;
    }
}

static void ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx; float ty;
  rasterizer->x        = x;
  rasterizer->y        = y;
  rasterizer->first_x  = x;
  rasterizer->first_y  = y;
  rasterizer->has_prev = -1;

  tx = (x - rasterizer->blit_x) * CTX_SUBDIV;
  ty = y * CTX_RASTERIZER_AA;
  if (ty < rasterizer->scan_min)
    { rasterizer->scan_min = ty; }
  if (ty > rasterizer->scan_max)
    { rasterizer->scan_max = ty; }
  if (tx < rasterizer->col_min)
    { rasterizer->col_min = tx; }
  if (tx > rasterizer->col_max)
    { rasterizer->col_max = tx; }
}

static void ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx = x;
  float ty = y;
  float ox = rasterizer->x;
  float oy = rasterizer->y;
  if (rasterizer->uses_transforms)
    {
      ctx_user_to_device (rasterizer->state, &tx, &ty);
    }
  tx -= rasterizer->blit_x;
  ctx_rasterizer_add_point (rasterizer, tx * CTX_SUBDIV, ty * CTX_RASTERIZER_AA);
  if (rasterizer->has_prev<=0)
    {
      if (rasterizer->uses_transforms)
        { ctx_user_to_device (rasterizer->state, &ox, &oy); }
      ox -= rasterizer->blit_x;
      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].data.s16[0] = ox * CTX_SUBDIV;
      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].data.s16[1] = oy * CTX_RASTERIZER_AA;
      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].code = CTX_NEW_EDGE;
      rasterizer->has_prev = 1;
    }
  rasterizer->has_shape = 1;
  rasterizer->y         = y;
  rasterizer->x         = x;
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
ctx_rasterizer_bezier_divide (CtxRasterizer *rasterizer,
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
    { return; }
  float t = (s + e) * 0.5f;
  float x, y, lx, ly, dx, dy;
  ctx_bezier_sample (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);
  if (iteration)
    {
      lx = ctx_lerpf (sx, ex, t);
      ly = ctx_lerpf (sy, ey, t);
      dx = lx - x;
      dy = ly - y;
      if ( (dx*dx+dy*dy) < tolerance)
        /* bailing - because for the mid-point straight line difference is
           tiny */
        { return; }
      dx = sx - ex;
      dy = ey - ey;
      if ( (dx*dx+dy*dy) < tolerance)
        /* bailing on tiny segments */
        { return; }
    }
  ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                sx, sy, x, y, s, t, iteration + 1,
                                tolerance);
  ctx_rasterizer_line_to (rasterizer, x, y);
  ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                x, y, ex, ey, t, e, iteration + 1,
                                tolerance);
}

static void
ctx_rasterizer_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2)
{
  float tolerance =
    ctx_pow2 (rasterizer->state->gstate.transform.m[0][0]) +
    ctx_pow2 (rasterizer->state->gstate.transform.m[1][1]);
  float ox = rasterizer->x;
  float oy = rasterizer->y;
  ox = rasterizer->state->x;
  oy = rasterizer->state->y;
  tolerance = 1.0f/tolerance;
#if 0 // skipping this to preserve hashes
  float maxx = ctx_maxf (x1,x2);
  maxx = ctx_maxf (maxx, ox);
  maxx = ctx_maxf (maxx, x0);
  float maxy = ctx_maxf (y1,y2);
  maxy = ctx_maxf (maxy, oy);
  maxy = ctx_maxf (maxy, y0);
  float minx = ctx_minf (x1,x2);
  minx = ctx_minf (minx, ox);
  minx = ctx_minf (minx, x0);
  float miny = ctx_minf (y1,y2);
  miny = ctx_minf (miny, oy);
  miny = ctx_minf (miny, y0);
  if (tolerance == 1.0f &&
      (
        (minx > rasterizer->blit_x + rasterizer->blit_width) ||
        (miny > rasterizer->blit_y + rasterizer->blit_height) ||
        (maxx < rasterizer->blit_x) ||
        (maxy < rasterizer->blit_y) ) )
    {
      // tolerance==1.0 is most likely screen-space -
      // skip subdivides for things outside
    }
  else
#endif
    {
      ctx_rasterizer_bezier_divide (rasterizer,
                                    ox, oy, x0, y0,
                                    x1, y1, x2, y2,
                                    ox, oy, x2, y2,
                                    0.0f, 1.0f, 0.0f, tolerance);
    }
  ctx_rasterizer_line_to (rasterizer, x2, y2);
}

static void
ctx_rasterizer_rel_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  if (x == 0.f && y == 0.f)
    { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_move_to (rasterizer, x, y);
}

static void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  if (x== 0.f && y==0.f)
    { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_line_to (rasterizer, x, y);
}

static void
ctx_rasterizer_rel_curve_to (CtxRasterizer *rasterizer,
                             float x0, float y0, float x1, float y1, float x2, float y2)
{
  x0 += rasterizer->x;
  y0 += rasterizer->y;
  x1 += rasterizer->x;
  y1 += rasterizer->y;
  x2 += rasterizer->x;
  y2 += rasterizer->y;
  ctx_rasterizer_curve_to (rasterizer, x0, y0, x1, y1, x2, y2);
}

static int ctx_compare_edges (const void *ap, const void *bp)
{
  const CtxEntry *a = (const CtxEntry *) ap;
  const CtxEntry *b = (const CtxEntry *) bp;
  int ycompare = a->data.s16[1] - b->data.s16[1];
  if (ycompare)
    { return ycompare; }
  int xcompare = a->data.s16[0] - b->data.s16[0];
  return xcompare;
}

static int ctx_edge_qsort_partition (CtxEntry *A, int low, int high)
{
  CtxEntry pivot = A[ (high+low) /2];
  int i = low;
  int j = high;
  while (i <= j)
    {
      while (ctx_compare_edges (&A[i], &pivot) <0) { i ++; }
      while (ctx_compare_edges (&pivot, &A[j]) <0) { j --; }
      if (i <= j)
        {
          CtxEntry tmp = A[i];
          A[i] = A[j];
          A[j] = tmp;
          i++;
          j--;
        }
    }
  return i;
}

static void ctx_edge_qsort (CtxEntry *entries, int low, int high)
{
  {
    int p = ctx_edge_qsort_partition (entries, low, high);
    if (low < p -1 )
      { ctx_edge_qsort (entries, low, p - 1); }
    if (low < high)
      { ctx_edge_qsort (entries, p, high); }
  }
}

static void ctx_rasterizer_sort_edges (CtxRasterizer *rasterizer)
{
#if 0
  qsort (&rasterizer->edge_list.entries[0], rasterizer->edge_list.count,
         sizeof (CtxEntry), ctx_compare_edges);
#else
  if (rasterizer->edge_list.count > 1)
    {
      ctx_edge_qsort (& (rasterizer->edge_list.entries[0]), 0, rasterizer->edge_list.count-1);
    }
#endif
}

static void ctx_rasterizer_discard_edges (CtxRasterizer *rasterizer)
{
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      if (rasterizer->edge_list.entries[rasterizer->edges[i].index].data.s16[3] < rasterizer->scanline
         )
        {
          if (rasterizer->lingering_edges + 1 < CTX_MAX_LINGERING_EDGES)
            {
              rasterizer->lingering[rasterizer->lingering_edges] =
                rasterizer->edges[i];
              rasterizer->lingering_edges++;
            }
          rasterizer->edges[i] = rasterizer->edges[rasterizer->active_edges-1];
          rasterizer->active_edges--;
          i--;
        }
    }
  for (int i = 0; i < rasterizer->lingering_edges; i++)
    {
      if (rasterizer->edge_list.entries[rasterizer->lingering[i].index].data.s16[3] < rasterizer->scanline - CTX_RASTERIZER_AA2)
        {
          if (rasterizer->lingering[i].dx > CTX_RASTERIZER_AA_SLOPE_LIMIT ||
              rasterizer->lingering[i].dx < -CTX_RASTERIZER_AA_SLOPE_LIMIT)
            { rasterizer->needs_aa --; }
          rasterizer->lingering[i] = rasterizer->lingering[rasterizer->lingering_edges-1];
          rasterizer->lingering_edges--;
          i--;
        }
    }
}

static void ctx_rasterizer_increment_edges (CtxRasterizer *rasterizer, int count)
{
  for (int i = 0; i < rasterizer->lingering_edges; i++)
    {
      rasterizer->lingering[i].x += rasterizer->lingering[i].dx * count;
    }
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      rasterizer->edges[i].x += rasterizer->edges[i].dx * count;
    }
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      rasterizer->edges[CTX_MAX_EDGES-1-i].x += rasterizer->edges[CTX_MAX_EDGES-1-i].dx * count;
    }
}

/* feeds up to rasterizer->scanline,
   keeps a pending buffer of edges - that encompass
   the full coming scanline - for adaptive AA,
   feed until the start of the scanline and check for need for aa
   in all of pending + active edges, then
   again feed_edges until middle of scanline if doing non-AA
   or directly render when doing AA
*/
static void ctx_rasterizer_feed_edges (CtxRasterizer *rasterizer)
{
  int miny;
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      if (rasterizer->edge_list.entries[rasterizer->edges[CTX_MAX_EDGES-1-i].index].data.s16[1] <= rasterizer->scanline)
        {
          if (rasterizer->active_edges < CTX_MAX_EDGES-2)
            {
              int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              rasterizer->edges[no] = rasterizer->edges[CTX_MAX_EDGES-1-i];
              rasterizer->edges[CTX_MAX_EDGES-1-i] =
                rasterizer->edges[CTX_MAX_EDGES-1-rasterizer->pending_edges + 1];
              rasterizer->pending_edges--;
              i--;
            }
        }
    }
  while (rasterizer->edge_pos < rasterizer->edge_list.count &&
         (miny=rasterizer->edge_list.entries[rasterizer->edge_pos].data.s16[1]) <= rasterizer->scanline)
    {
      if (rasterizer->active_edges < CTX_MAX_EDGES-2)
        {
          int dy = (rasterizer->edge_list.entries[rasterizer->edge_pos].data.s16[3] -
                    miny);
          if (dy) /* skipping horizontal edges */
            {
              int yd = rasterizer->scanline - miny;
              int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              rasterizer->edges[no].index = rasterizer->edge_pos;
              int x0 = rasterizer->edge_list.entries[rasterizer->edges[no].index].data.s16[0];
              int x1 = rasterizer->edge_list.entries[rasterizer->edges[no].index].data.s16[2];
              rasterizer->edges[no].x = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER;
              int dx_dy;
              //  if (dy)
              dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
              //  else
              //  dx_dy = 0;
              rasterizer->edges[no].dx = dx_dy;
              rasterizer->edges[no].x += (yd * dx_dy);
              // XXX : even better minx and maxx can
              //       be derived using y0 and y1 for scaling dx_dy
              //       when ydelta to these are smaller than
              //       ydelta to scanline
#if 0
              if (dx_dy < 0)
                {
                  rasterizer->edges[no].minx =
                    rasterizer->edges[no].x + dx_dy/2;
                  rasterizer->edges[no].maxx =
                    rasterizer->edges[no].x - dx_dy/2;
                }
              else
                {
                  rasterizer->edges[no].minx =
                    rasterizer->edges[no].x - dx_dy/2;
                  rasterizer->edges[no].maxx =
                    rasterizer->edges[no].x + dx_dy/2;
                }
#endif
              if (dx_dy > CTX_RASTERIZER_AA_SLOPE_LIMIT ||
                  dx_dy < -CTX_RASTERIZER_AA_SLOPE_LIMIT)
                { rasterizer->needs_aa ++; }
              if (! (miny <= rasterizer->scanline) )
                {
                  /* it is a pending edge - we add it to the end of the array
                     and keep a different count for items stored here, like
                     a heap and stack growing against each other
                  */
                  rasterizer->edges[CTX_MAX_EDGES-1-rasterizer->pending_edges] =
                    rasterizer->edges[no];
                  rasterizer->active_edges--;
                  rasterizer->pending_edges++;
                }
            }
        }
      rasterizer->edge_pos++;
    }
}

static void ctx_rasterizer_sort_active_edges (CtxRasterizer *rasterizer)
{
  int sorted = 0;
  while (!sorted)
    {
      sorted = 1;
      for (int i = 0; i < rasterizer->active_edges-1; i++)
        {
          CtxEdge *a = &rasterizer->edges[i];
          CtxEdge *b = &rasterizer->edges[i+1];
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
      for (int i = 0; i < rasterizer->pending_edges-1; i++)
        {
          CtxEdge *a = &rasterizer->edges[CTX_MAX_EDGES-1-i];
          CtxEdge *b = &rasterizer->edges[CTX_MAX_EDGES-1- (i+1)];
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
      for (int i = 0; i < rasterizer->lingering_edges-1; i++)
        {
          CtxEdge *a = &rasterizer->lingering[i];
          CtxEdge *b = &rasterizer->lingering[i+1];
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
  return ( ( ( ( (v0) <<8) + (dx) * ( (v1) - (v0) ) ) ) >>8);
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
ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
  float v = x;
  /* caching a 512 long gradient - and sampling with nearest neighbor
     will be much faster.. */
  CtxGradient *g = &rasterizer->state->gradient;
  if (v < 0) { v = 0; }
  if (v > 1) { v = 1; }
#if CTX_GRADIENT_CACHE
  int cache_no = v * (CTX_GRADIENT_CACHE_ELEMENTS-1.0f);
  uint8_t *cache_entry = &ctx_gradient_cache_u8[cache_no][0];
  if (! ( //cache_entry[0] == 255 &&
        //cache_entry[1] == 2   &&
        //cache_entry[2] == 255 &&
        cache_entry[3] == 13) )
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
  if (cache_no == 0) { v = 0.0f; }
  else if (cache_no == CTX_GRADIENT_CACHE_ELEMENTS-1) { v = 1.0f; }
#endif
  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];
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
      ctx_color_get_rgba8 (rasterizer->state, & (next_stop->color), rgba);
    }
  else if (stop && next_stop == NULL)
    {
      ctx_color_get_rgba8 (rasterizer->state, & (stop->color), rgba);
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
    }
  else
    {
      ctx_color_get_rgba8 (rasterizer->state, & (g->stops[g->n_stops-1].color), rgba);
    }
#if CTX_GRADIENT_CACHE
  cache_entry[0] = rgba[0];
  cache_entry[1] = rgba[1];
  cache_entry[2] = rgba[2];
  cache_entry[3] = rgba[3];
#endif
}

static void
ctx_fragment_gradient_1d_RGBAF (CtxRasterizer *rasterizer, float x, float y, float *rgba)
{
  float v = x;
  /* caching a 512 long gradient - and sampling with nearest neighbor
     will be much faster.. */
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
      ctx_color_get_rgba (rasterizer->state, & (next_stop->color), rgba);
    }
  else if (stop && next_stop == NULL)
    {
      ctx_color_get_rgba (rasterizer->state, & (stop->color), rgba);
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
    }
  else
    {
      ctx_color_get_rgba (rasterizer->state, & (g->stops[g->n_stops-1].color), rgba);
    }
}

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
      rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
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
    }
}

#if CTX_DITHER
static inline int ctx_dither_mask_a (int x, int y, int c, int divisor)
{
  /* https://pippin.gimp.org/a_dither/ */
  return ( ( ( ( (x + c * 67) + y * 236) * 119) & 255 )-127) / divisor;
}

static inline void ctx_dither_rgba_u8 (uint8_t *rgba, int x, int y, int dither_red_blue, int dither_green)
{
  if (dither_red_blue == 0)
    { return; }
  for (int c = 0; c < 3; c ++)
    {
      int val = rgba[c] + ctx_dither_mask_a (x, y, 0, c==1?dither_green:dither_red_blue);
      rgba[c] = CTX_CLAMP (val, 0, 255);
    }
}
#endif

static void
ctx_RGBA8_associate_alpha (uint8_t *rgba)
{
  rgba[0] = (rgba[0] * rgba[3]) /255;
  rgba[1] = (rgba[1] * rgba[3]) /255;
  rgba[2] = (rgba[2] * rgba[3]) /255;
}

static void
ctx_RGBAF_associate_alpha (float *rgba)
{
  rgba[0] = (rgba[0] * rgba[3]);
  rgba[1] = (rgba[1] * rgba[3]);
  rgba[2] = (rgba[2] * rgba[3]);
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
    }
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
//ctx_RGBA8_associate_alpha (rgba);
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
    }
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = 0.0f;
  if (g->radial_gradient.r0 == 0.0f ||
      (g->radial_gradient.r1-g->radial_gradient.r0) < 0.0f)
    {
    }
  else
    {
      v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
      v = (v - g->radial_gradient.r0) /
          (g->radial_gradient.r1 - g->radial_gradient.r0);
    }
  ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, rgba);
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
//ctx_RGBA8_associate_alpha (rgba);
}

static void
ctx_fragment_radial_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = 0.0f;
  if (g->radial_gradient.r0 == 0.0f ||
      (g->radial_gradient.r1-g->radial_gradient.r0) < 0.0f)
    {
    }
  else
    {
      v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
      v = (v - g->radial_gradient.r0) /
          (g->radial_gradient.r1 - g->radial_gradient.r0);
    }
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 0.0, rgba);
//ctx_RGBAF_associate_alpha (rgba);
}


static void
ctx_fragment_linear_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) /
            (g->linear_gradient.end - g->linear_gradient.start);
  ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 1.0, rgba);
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
//ctx_RGBA8_associate_alpha (rgba);
}

static void
ctx_fragment_linear_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) /
            (g->linear_gradient.end - g->linear_gradient.start);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0, rgba);
  ctx_RGBAF_associate_alpha (rgba);
}

static void
ctx_fragment_color_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  ctx_color_get_rgba8 (rasterizer->state, &g->color, rgba);
//ctx_RGBA8_associate_alpha (rgba);
}

static void
ctx_fragment_color_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source;
  ctx_color_get_rgba (rasterizer->state, &g->color, rgba);
  ctx_RGBAF_associate_alpha (rgba);
}

static void ctx_fragment_image_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  float *outf = (float *) out;
  uint8_t rgba[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (buffer->format->bpp)
    {
      case 1:
        ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba);
      case 24:
        ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba);
      case 32:
        ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba);
      default:
        ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba);
    }
  for (int c = 0; c < 4; c ++) { outf[c] = ctx_u8_to_float (rgba[c]); }
}

static CtxFragment ctx_rasterizer_get_fragment_RGBAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_IMAGE:
        return ctx_fragment_image_RGBAF;
      case CTX_SOURCE_COLOR:
        return ctx_fragment_color_RGBAF;
      case CTX_SOURCE_LINEAR_GRADIENT:
        return ctx_fragment_linear_gradient_RGBAF;
      case CTX_SOURCE_RADIAL_GRADIENT:
        return ctx_fragment_radial_gradient_RGBAF;
    }
  return ctx_fragment_color_RGBAF;
}

static CtxFragment ctx_rasterizer_get_fragment_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source.image.buffer;
  switch (gstate->source.type)
    {
      case CTX_SOURCE_IMAGE:
        switch (buffer->format->bpp)
          {
            case 1:
              return ctx_fragment_image_gray1_RGBA8;
            case 24:
              return ctx_fragment_image_rgb8_RGBA8;
            case 32:
              return ctx_fragment_image_rgba8_RGBA8;
            default:
              return ctx_fragment_image_RGBA8;
          }
      case CTX_SOURCE_COLOR:
        return ctx_fragment_color_RGBA8;
      case CTX_SOURCE_LINEAR_GRADIENT:
        return ctx_fragment_linear_gradient_RGBA8;
      case CTX_SOURCE_RADIAL_GRADIENT:
        return ctx_fragment_radial_gradient_RGBA8;
    }
  return ctx_fragment_color_RGBA8;
}


#define MASK_ALPHA       (0xff << 24)
#define MASK_GREEN_ALPHA ((0xff << 8)|MASK_ALPHA)
#define MASK_RED_BLUE    ((0xff << 16) | (0xff))

static inline void
ctx_RGBA8_source_over_normal_opaque_color (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  while (count--)
  {
    uint8_t cov = *covp;
    if (cov)
    {
      if (cov != 255)
      {
        uint8_t ralpha = 255 - cov;
        for (int c = 0; c < 4; c++)
          dst[c] = (src[c]*cov + dst[c] * ralpha) / 255;
      }
      else // cov == 255
      {
        *((uint32_t*)(dst)) = *((uint32_t*)(src));
      }
    }
    covp ++;
    dst+=4;
  }
}

static inline void
ctx_RGBA8_source_over_normal_color (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  uint8_t alpha = src[3];
  while (count--)
  {
    uint8_t cov = *covp;
    if (cov)
    {
      if (cov != 255)
      {
        uint8_t ralpha = 255 - ( (cov * alpha) / 255);
        for (int c = 0; c < 4; c++)
          dst[c] = (src[c]*cov + dst[c] * ralpha) / 255;
      }
      else // cov == 255
      {
        if (alpha == 255)
          *((uint32_t*)(dst)) = *((uint32_t*)(src));
        else
         {
           uint8_t ralpha = 255 - alpha;
           for (int c = 0; c < 4; c++)
             dst[c] = src[c] + ((dst[c] * ralpha) / 255);
         }
      }
    }
    covp ++;
    dst+=4;
  }
}

static inline void
ctx_RGBA8_source_over_normal (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, float u, float v, float ud, float vd, CtxRasterizer *rasterizer)
{
  uint8_t alpha = src[3];
  while (count--)
  {
    uint8_t cov = *covp;
    if (cov)
    {
      if (rasterizer->fragment)
      {
        rasterizer->fragment (rasterizer, u, v, src);
        ctx_RGBA8_associate_alpha (src);
        alpha = src[3];
      }
      if (cov != 255)
      {
        uint8_t ralpha = 255 - ( (cov * alpha) / 255);

        for (int c = 0; c < 4; c++)
          dst[c] = (src[c]*cov + dst[c] * ralpha) / 255;
      }
      else // cov == 255
      {
        if (alpha == 255)
          *((uint32_t*)(dst)) = *((uint32_t*)(src));
        else
         {
           uint8_t ralpha = 255 - alpha;
           for (int c = 0; c < 4; c++)
             dst[c] = src[c] + ((dst[c] * ralpha) / 255);
         }
      }
    }
    covp ++;
    u += ud;
    v += vd;
    dst  +=4;
  }
}

static void ctx_RGBA8_blend_normal (uint8_t *dst, uint8_t *src, uint8_t *blended)
{
  for (int c = 0; c < 4; c++)
    blended[c] = src[c];
}

static void ctx_RGBA8_blend_multiply (uint8_t *dst, uint8_t *src, uint8_t *blended)
{
  uint8_t tsrc[4];
  uint8_t tdst[4];
  for (int c = 0; c < 3; c++)
    tsrc[0] = (src[c] * 255) / src[3];
  for (int c = 0; c < 3; c++)
    tdst[0] = (dst[c] * 255) / dst[3];

  for (int c = 0; c < 3; c++)
    blended[c] = (tsrc[c] * tdst[c])/255;
  blended[3] = src[3];

  ctx_RGBA8_associate_alpha (blended);
}


typedef enum {
  CTX_PORTER_DUFF_0,
  CTX_PORTER_DUFF_1,
  CTX_PORTER_DUFF_FOO,
  CTX_PORTER_DUFF_1_MINUS_FOO,
} CtxPorterDuffFactor;

static inline void
ctx_RGBA8_porter_duff (uint8_t *dst, uint8_t *src, uint8_t *covp, int count,
                       CtxPorterDuffFactor f_s, CtxPorterDuffFactor f_d,
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  while (count--)
  {
    uint8_t cov = *covp;
    uint8_t tsrc[4];

    if (rasterizer->fragment)
    {
      rasterizer->fragment (rasterizer, u0, v0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
      u0 += ud;
      v0 += vd;
      rasterizer->blend_op (dst, tsrc, tsrc);
    }
    else
    {
      rasterizer->blend_op (dst, src, tsrc);
    }

    if (cov != 255) for (int c = 0; c < 4; c++)
      tsrc[c] = (tsrc[c] * cov)/255;

    for (int c = 0; c < 4; c++)
    {
      int res = 0;
      switch (f_s)
      {
        case CTX_PORTER_DUFF_0: break;
        case CTX_PORTER_DUFF_1:   res += tsrc[c]; break;
        case CTX_PORTER_DUFF_FOO: res += (tsrc[c] * dst[3])/255; break;
        case CTX_PORTER_DUFF_1_MINUS_FOO: res += (tsrc[c] * (255-dst[3]))/255; break;
      }
      switch (f_d)
      {
        case CTX_PORTER_DUFF_0: break;
        case CTX_PORTER_DUFF_1:   res += dst[c]; break;
        case CTX_PORTER_DUFF_FOO: res += (dst[c] * tsrc[3])/255; break;
        case CTX_PORTER_DUFF_1_MINUS_FOO: res += (dst[c] * (255-tsrc[3]))/255; break;
      }
      if (res > 255) res = 255;
      dst[c] = res;
    }
    covp ++;
    dst+=4;
  }
}

static inline void
ctx_RGBA8_source_atop (uint8_t *dst, uint8_t *src, uint8_t *covp, int count,
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_FOO, CTX_PORTER_DUFF_1_MINUS_FOO,
    u0, v0, ud, vd, rasterizer);
}

static inline void
ctx_RGBA8_destination_atop (uint8_t *dst, uint8_t *src, uint8_t *covp, int count,
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_1_MINUS_FOO, CTX_PORTER_DUFF_FOO,
    u0, v0, ud, vd, rasterizer);
}

static inline void
ctx_RGBA8_source_in (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_FOO, CTX_PORTER_DUFF_0,
    u0, v0, ud, vd, rasterizer);
}

static inline void
ctx_RGBA8_destination_in (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_0, CTX_PORTER_DUFF_FOO,
    u0, v0, ud, vd, rasterizer);
}

static inline void
ctx_RGBA8_destination (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_0, CTX_PORTER_DUFF_1,
    u0, v0, ud, vd, rasterizer);
}

static inline void
ctx_RGBA8_source_over (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_1, CTX_PORTER_DUFF_1_MINUS_FOO,
    u0, v0, ud, vd, rasterizer);
}

static void
ctx_RGBA8_destination_over (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_1_MINUS_FOO, CTX_PORTER_DUFF_1,
    u0, v0, ud, vd, rasterizer);
}

static void
ctx_RGBA8_xor (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_1_MINUS_FOO, CTX_PORTER_DUFF_1_MINUS_FOO,
    u0, v0, ud, vd, rasterizer);
}

static void
ctx_RGBA8_destination_out (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_0, CTX_PORTER_DUFF_1_MINUS_FOO,
    u0, v0, ud, vd, rasterizer);
}

static void
ctx_RGBA8_source_out (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_1_MINUS_FOO, CTX_PORTER_DUFF_0,
    u0, v0, ud, vd, rasterizer);
}

static void
ctx_RGBA8_source (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, 
float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  ctx_RGBA8_porter_duff (dst, src, covp, count,
    CTX_PORTER_DUFF_1, CTX_PORTER_DUFF_0,
    u0, v0, ud, vd, rasterizer);
}

static void ctx_RGBA8_copy_normal (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  while (count--)
  {
    uint8_t cov = *covp;
    if (cov == 0)
    {
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
      for (int c = 0; c < 4; c++)
        dst[c] = src[c];
    }
    else
    {
      uint8_t ralpha = 255 - cov;
      for (int c = 0; c < 4; c++)
        { dst[c] = (src[c]*cov + dst[c] * ralpha) / 255; }
    }
    }
    dst += 4;
    covp ++;
  }
}

static void ctx_RGBA8_clear_normal (uint8_t *dst, uint8_t *src, uint8_t *covp, int count, float u0, float v0, float ud, float vd, CtxRasterizer *rasterizer)
{
  while (count--)
  {
    uint8_t cov = *covp;
    for (int c = 0; c < 4; c++)
      dst[c] = 0;
    if (cov == 0)
    {
    }
    else if (cov == 255)
    {
      *((uint32_t*)(dst)) = 0;
    }
    else
    {
      uint8_t ralpha = 255 - cov;
      for (int c = 0; c < 4; c++)
        { dst[c] = (dst[c] * ralpha) / 255; }
    }
    covp ++;
    dst += 4;
  }
}

static inline void ctx_float_source_over (int components, float *dst, float *src, uint8_t cov)
{
  if (cov == 0) return;
  float fcov = cov/255.0f;

  float ralpha = 1.0f - (fcov) * src[components-1];
  if (ralpha == 0.0f)
  {
    for (int c = 0; c < components; c++)
      dst[c] = src[c];
  }
  else
  for (int c = 0; c < components; c++)
    dst[c] = src[c] * fcov + dst[c] * ralpha;
}

static inline void ctx_float_copy (int components, float *dst, float *src, uint8_t cov)
{
  if (cov == 0) return;
  if (cov == 255)
  {
    for (int c = 0; c < components; c++)
      dst[c] = src[c];
  }
  else
  {
    float fcov = cov/255.0f;
    float ralpha = 1.0f - (fcov);
    for (int c = 0; c < components; c++)
      dst[c] = src[c] * fcov + dst[c] * ralpha;
  }
}

static inline void ctx_float_clear(int components, float *dst, float *src, uint8_t cov)
{
  if (cov == 0) return;
  if (cov == 255)
  {
    for (int c = 0; c < components; c++)
      dst[c] = 0.0f;
  }
  else
  {
    float fcov = cov/255.0f;
    float ralpha = 1.0f - (fcov);
    for (int c = 0; c < components; c++)
      dst[c] = dst[c] * ralpha;
  }
}

static void
ctx_setup_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;

  switch (gstate->blend_mode)
  {
    default:
    case CTX_BLEND_NORMAL:   rasterizer->blend_op = ctx_RGBA8_blend_normal; break;
    case CTX_BLEND_MULTIPLY: rasterizer->blend_op = ctx_RGBA8_blend_multiply; break;
  }

  if (gstate->blend_mode == CTX_BLEND_NORMAL)
  {
    switch (gstate->compositing_mode)
    {
      case CTX_COMPOSITE_SOURCE_OVER:
         if (gstate->source.type == CTX_SOURCE_COLOR)
         {
           uint8_t color[4];
           ctx_color_get_rgba8 (rasterizer->state, &gstate->source.color, color);
           if (color[3] == 255 && gstate->global_alpha_u8 == 255)
             rasterizer->comp_op = ctx_RGBA8_source_over_normal_opaque_color;
           else
             rasterizer->comp_op = ctx_RGBA8_source_over_normal_color;
         }
         else
         {
           rasterizer->comp_op = ctx_RGBA8_source_over_normal;
         }
         break;
      case CTX_COMPOSITE_CLEAR: rasterizer->comp_op = ctx_RGBA8_clear_normal; break;
      case CTX_COMPOSITE_COPY:  rasterizer->comp_op = ctx_RGBA8_copy_normal; break;
      default:
         rasterizer->comp_op = NULL;
         break;
    }
  }

  if (!rasterizer->comp_op)
  {
  switch (gstate->compositing_mode)
  {
     default:
     case CTX_COMPOSITE_SOURCE_OVER: rasterizer->comp_op = ctx_RGBA8_source_over; break;
     case CTX_COMPOSITE_SOURCE_OUT:  rasterizer->comp_op = ctx_RGBA8_source_out; break;
     case CTX_COMPOSITE_SOURCE_ATOP: rasterizer->comp_op = ctx_RGBA8_source_atop; break;
     case CTX_COMPOSITE_SOURCE_IN:   rasterizer->comp_op = ctx_RGBA8_source_in; break;

     case CTX_COMPOSITE_DESTINATION: rasterizer->comp_op = ctx_RGBA8_destination; break;
     case CTX_COMPOSITE_DESTINATION_ATOP: rasterizer->comp_op = ctx_RGBA8_destination_atop; break;
     case CTX_COMPOSITE_DESTINATION_OVER: rasterizer->comp_op = ctx_RGBA8_destination_over; break;
     case CTX_COMPOSITE_DESTINATION_OUT: rasterizer->comp_op = ctx_RGBA8_destination_out; break;
     case CTX_COMPOSITE_DESTINATION_IN: rasterizer->comp_op = ctx_RGBA8_destination_in; break;
     case CTX_COMPOSITE_XOR:   rasterizer->comp_op = ctx_RGBA8_xor;     break;
     case CTX_COMPOSITE_COPY:  rasterizer->comp_op = ctx_RGBA8_source;  break;
     case CTX_COMPOSITE_CLEAR: rasterizer->comp_op = ctx_RGBA8_clear_normal;  break;
  }
  }

  if (gstate->source.type != CTX_SOURCE_COLOR)
    {
      rasterizer->fragment = ctx_rasterizer_get_fragment_RGBA8 (rasterizer);
    }
  else
    {
      rasterizer->fragment = NULL;
    }
}

static int
ctx_composite_RGBA8 (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (!rasterizer->comp_op)
    ctx_setup_RGBA8 (rasterizer);

  if (gstate->source.type != CTX_SOURCE_COLOR)
    {
      float y = rasterizer->scanline / CTX_RASTERIZER_AA;
      float u0 = x0;
      float v0 = y;
      float u1 = u0 + count;
      float v1 = v0;

      ctx_matrix_apply_transform (&gstate->source.transform, &u0, &v0);
      ctx_matrix_apply_transform (&gstate->source.transform, &u1, &v1);

      float ud = (u1-u0)/(count);
      float vd = (v1-v0)/(count);
      uint8_t color[4];
      rasterizer->comp_op (dst, color, coverage, count, u0, v0, ud, vd, rasterizer);
    }
  else
  {
    uint8_t color[4];
    ctx_color_get_rgba8 (rasterizer->state, &gstate->source.color, color);
    if (gstate->global_alpha_u8 != 255)
      color[3] = (color[3] * gstate->global_alpha_u8)/255;
    if (color[3] != 255)
      ctx_RGBA8_associate_alpha (color);
    rasterizer->comp_op (dst, color, coverage, count, 0,0,0,0,rasterizer);
  }
  return count;
}

static int
ctx_composite_RGBA8_convert (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  rasterizer->format->to_comp (rasterizer, x, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x, &pixels[0], coverage, count);
  rasterizer->format->from_comp (rasterizer, x, &pixels[0], dst, count);
  return ret;
}

#if CTX_ENABLE_BGRA8

static inline void
ctx_swap_red_green (uint8_t *rgba)
{
  uint32_t *buf = (uint32_t *) rgba;
  uint32_t orig = *buf;
  uint32_t green_alpha = (orig & 0xff00ff00);
  uint32_t red_blue    = (orig & 0x00ff00ff);
  uint32_t red         = red_blue << 16;
  uint32_t blue        = red_blue >> 16;
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

static int
ctx_composite_BGRA8 (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_BGRA8_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x0, &pixels[0], coverage, count);
  ctx_BGRA8_to_RGBA8  (rasterizer, x0, &pixels[0], dst, count);
  return count;
}

#endif


#if CTX_ENABLE_GRAYF

static int
ctx_GRAYF_composite (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count,
                void (*comp_op)(int components, float *src, float *dst, uint8_t cov))
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 2;
  float *dst_f = (float *) dst;
  float y = rasterizer->scanline / CTX_RASTERIZER_AA;
  float graya[2];
  ctx_color_get_graya (rasterizer->state, &gstate->source.color, graya);
  CtxFragment source = ctx_rasterizer_get_fragment_RGBA8 (rasterizer);

  float u0 = x0;
  float v0 = y;
  float u1 = x0 + count;
  float v1 = y;
  float ud = 0;
  float vd = 0;

  if (source == ctx_fragment_color_RGBA8)
    { 
       source = NULL;
    }
  else
    {
      ctx_matrix_apply_transform (&gstate->source.transform, &u0, &v0);
      ctx_matrix_apply_transform (&gstate->source.transform, &u1, &v1);
      ud = (u1-u0)/(count);
      vd = (v1-v0)/(count);
    }

  for (int x = 0; x < count; x++)
    {
      int cov = coverage[x];
      if (cov != 0)
        {
          if (source)
            {
              uint8_t scolor[4];
              source (rasterizer, u0, v0, &scolor[0]);

              graya[0] = ctx_u8_to_float ( (scolor[0]+scolor[1]+scolor[2]) /3.0);
              graya[1] = ctx_u8_to_float (scolor[3]) * gstate->global_alpha_f;
            }
          float temp_f[2]={dst_f[0], 1.0f};
          comp_op (components, temp_f, graya, cov);
          dst_f[0]=temp_f[0];
        }
      u0 += ud;
      v0 += vd;
      dst_f+=1;
    }
  return count;
}

static int
ctx_composite_GRAYF (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  switch (rasterizer->state->gstate.compositing_mode)
  {
     default:
     case CTX_COMPOSITE_SOURCE_OVER:
       return ctx_GRAYF_composite (rasterizer, x0, dst, coverage, count, ctx_float_source_over);
     case CTX_COMPOSITE_COPY:
       return ctx_GRAYF_composite (rasterizer, x0, dst, coverage, count, ctx_float_copy);
     case CTX_COMPOSITE_CLEAR:
       return ctx_GRAYF_composite (rasterizer, x0, dst, coverage, count, ctx_float_clear);
  }
}

#endif


#if CTX_ENABLE_RGBAF

static int
ctx_RGBAF_composite (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count,
                void (*comp_op)(int components, float *src, float *dst, uint8_t cov))
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 4;
  float *dst_f = (float *) dst;
  float y = rasterizer->scanline / CTX_RASTERIZER_AA;
  CtxFragment fragment = ctx_rasterizer_get_fragment_RGBAF (rasterizer);
  float color_f[components];
  float u0 = x0;
  float v0 = y;
  float u1 = x0 + count;
  float v1 = y;
  float ud = 0;
  float vd = 0;

  if (gstate->source.type == CTX_SOURCE_COLOR)
    {
      ctx_fragment_color_RGBAF (rasterizer, x0, y, color_f);
      if (gstate->global_alpha_f != 1.0f)
        for (int c = 0; c < components; c++)
          { color_f[c] *= gstate->global_alpha_f; }
      fragment = NULL;
    }
  else
    {
      ctx_matrix_apply_transform (&gstate->source.transform, &u0, &v0);
      ctx_matrix_apply_transform (&gstate->source.transform, &u1, &v1);
      ud = (u1-u0)/(count);
      vd = (v1-v0)/(count);
    }

  {
    for (int x = 0; x < count; x++)
      {
        uint8_t cov = coverage[x];
        if (cov != 0)
          {
            if (fragment)
              {
                fragment (rasterizer, u0, v0, color_f);
                if (gstate->global_alpha_f != 1.0f)
                  for (int c = 0; c < components; c++)
                    { color_f[c] *= gstate->global_alpha_f; }
              }
            comp_op (components, color_f, dst_f, cov);
          }
        dst_f += components;
        u0 += ud;
        v0 += vd;
      }
  }
  return count;
}

static int
ctx_composite_RGBAF (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  switch (rasterizer->state->gstate.compositing_mode)
  {
     default:
     case CTX_COMPOSITE_SOURCE_OVER:
       return ctx_RGBAF_composite (rasterizer, x0, dst, coverage, count, ctx_float_source_over);
     case CTX_COMPOSITE_COPY:
       return ctx_RGBAF_composite (rasterizer, x0, dst, coverage, count, ctx_float_copy);
     case CTX_COMPOSITE_CLEAR:
       return ctx_RGBAF_composite (rasterizer, x0, dst, coverage, count, ctx_float_clear);
  }
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
      case CTX_SOURCE_LINEAR_GRADIENT:
        ctx_fragment_linear_gradient_RGBAF (rasterizer, x, y, rgba);
        break;
      case CTX_SOURCE_RADIAL_GRADIENT:
        ctx_fragment_radial_gradient_RGBAF (rasterizer, x, y, rgba);
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
  ctx_color_get_CMYKAF (rasterizer->state, &gstate->source.color, cmyka);
  // RGBW instead of CMYK, and premultiply
  for (int i = 0; i < 4; i ++)
    {
      cmyka[i] = (1.0f - cmyka[i]) * cmyka[4];
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


static int
ctx_CMYKAF_composite (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count,
                void (*comp_op)(int components, float *src, float *dst, uint8_t cov))
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 5;
  float *dst_f = (float *) dst;
  float y = rasterizer->scanline / CTX_RASTERIZER_AA;
  CtxFragment fragment = ctx_rasterizer_get_fragment_CMYKAF (rasterizer);
  float color_f[components];
  float u0 = x0;
  float v0 = y;
  float u1 = x0 + count;
  float v1 = y;
  float ud = 0;
  float vd = 0;

  if (gstate->source.type == CTX_SOURCE_COLOR)
    {
      fragment (rasterizer, 0, 0, color_f);
      for (int c = 0; c < components-1; c++)
        { color_f[c] *= gstate->global_alpha_f; }
      fragment = NULL;
    }
  else
  {
    ctx_matrix_apply_transform (&gstate->source.transform, &u0, &v0);
    ctx_matrix_apply_transform (&gstate->source.transform, &u1, &v1);
    ud = (u1-u0)/(count);
    vd = (v1-v0)/(count);
  }

  for (int x = 0; x < count; x++)
    {
      int cov = coverage[x];
      if (cov)
        {
          if (fragment)
            {
              fragment (rasterizer, u0, v0, color_f);
              for (int c = 0; c < components; c++)
                { color_f[c] *= gstate->global_alpha_f; }
            }
          comp_op (components, color_f, dst_f, cov);
        }
      dst_f += components;
      u0 += ud;
      v0 += vd;
    }
  return count;
}

static int
ctx_composite_CMYKAF (CtxRasterizer *rasterizer, int x0, uint8_t *dst, uint8_t *coverage, int count)
{
  switch (rasterizer->state->gstate.compositing_mode)
  {
     default:
     case CTX_COMPOSITE_SOURCE_OVER:
       return ctx_CMYKAF_composite (rasterizer, x0, dst, coverage, count, ctx_float_source_over);
     case CTX_COMPOSITE_COPY:
       return ctx_CMYKAF_composite (rasterizer, x0, dst, coverage, count, ctx_float_copy);
     case CTX_COMPOSITE_CLEAR:
       return ctx_CMYKAF_composite (rasterizer, x0, dst, coverage, count, ctx_float_clear);
  }
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
      dst[4] = ctx_float_to_u8 (a);
      src += 5;
      dst += 5;
    }
}

static int
ctx_composite_CMYKA8 (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  float pixels[count * 5];
  ctx_CMYKA8_to_CMYKAF (rasterizer, dst, &pixels[0], count);
  ret = ctx_composite_CMYKAF (rasterizer, x, (uint8_t *) &pixels[0], coverage, count);
  ctx_CMYKAF_to_CMYKA8 (rasterizer, &pixels[0], dst, count);
  return ret;
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

static int
ctx_composite_CMYK8 (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  float pixels[count * 5];
  ctx_CMYK8_to_CMYKAF (rasterizer, dst, &pixels[0], count);
  ret = ctx_composite_CMYKAF (rasterizer, x, (uint8_t *) &pixels[0], coverage, count);
  ctx_CMYKAF_to_CMYK8 (rasterizer, &pixels[0], dst, count);
  return ret;
}
#endif

static int
ctx_rasterizer_apply_coverage (CtxRasterizer *rasterizer,
                               uint8_t       *dst,
                               int            x,
                               uint8_t       *coverage,
                               int            count)
{
  if (count >= rasterizer->blit_x + rasterizer->blit_width - x)
    {
      count = rasterizer->blit_x + rasterizer->blit_width - x;
    }
  return rasterizer->format->crunch (rasterizer, x, dst, coverage, count);
}

static void
ctx_rasterizer_generate_coverage (CtxRasterizer *rasterizer,
                                  int            minx,
                                  int            maxx,
                                  uint8_t       *coverage,
                                  int            winding,
                                  int            aa)
{
  int scanline     = rasterizer->scanline;
  int active_edges = rasterizer->active_edges;
  int parity = 0;
  coverage -= minx;
#define CTX_EDGE(no)      rasterizer->edge_list.entries[rasterizer->edges[no].index]
#define CTX_EDGE_YMIN(no) CTX_EDGE(no).data.s16[1]
#define CTX_EDGE_YMAX(no) CTX_EDGE(no).data.s16[3]
#define CTX_EDGE_SLOPE(no) rasterizer->edges[no].dx
#define CTX_EDGE_X(no)     (rasterizer->edges[no].x)
  for (int t = 0; t < active_edges -1;)
    {
      int ymin = CTX_EDGE_YMIN (t);
      int next_t = t + 1;
      if (scanline != ymin)
        {
          if (winding)
            { parity += ( (CTX_EDGE (t).code == CTX_EDGE_FLIPPED) ?1:-1); }
          else
            { parity = 1 - parity; }
        }
      if (parity)
        {
          int x0 = CTX_EDGE_X (t)      / CTX_SUBDIV ;
          int x1 = CTX_EDGE_X (next_t) / CTX_SUBDIV ;
          if ( (x0 < x1) )
            {
              int first = x0 / CTX_RASTERIZER_EDGE_MULTIPLIER;
              int last  = x1 / CTX_RASTERIZER_EDGE_MULTIPLIER;
#if 0
              if (first < 0)
                { first = 0; }
              if (first >= maxx-minx)
                { first = maxx-minx; }
              if (last < 0)
                { last = 0; }
#endif
              if (first < minx)
                { first = minx; }
              if (last >= maxx)
                { last = maxx; }
              if (first > last)
                { return; }
              int graystart = 255- ( (x0 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff);
              int grayend   = (x1 * 256/CTX_RASTERIZER_EDGE_MULTIPLIER) & 0xff;
              if (aa)
                {
                  if ( (first!=last) && graystart)
                    {
                      int cov = coverage[first] + graystart / CTX_RASTERIZER_AA;
                      coverage[first] = ctx_mini (cov,255);
                      first++;
                    }
                  for (int x = first; x < last; x++)
                    {
                      int cov = coverage[x] + 255 / CTX_RASTERIZER_AA;
                      coverage[x] = ctx_mini (cov,255);
                    }
                  if (grayend)
                    {
                      int cov = coverage[last] + grayend / CTX_RASTERIZER_AA;
                      coverage[last] = ctx_mini (cov,255);
                    }
                }
              else
                {
                  if ( (first!=last) && graystart)
                    {
                      int cov = coverage[first] + graystart;
                      coverage[first] = ctx_mini (cov,255);
                      first++;
                    }
                  for (int x = first; x < last; x++)
                    {
                      coverage[x] = 255;
                    }
                  if (grayend)
                    {
                      int cov = coverage[last] + grayend;
                      coverage[last] = ctx_mini (cov,255);
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
ctx_rasterizer_reset (CtxRasterizer *rasterizer)
{
  rasterizer->lingering_edges = 0;
  rasterizer->active_edges    = 0;
  rasterizer->pending_edges   = 0;
  rasterizer->has_shape       = 0;
  rasterizer->has_prev        = 0;
  rasterizer->edge_list.count = 0; // ready for new edges
  rasterizer->edge_pos        = 0;
  rasterizer->needs_aa        = 0;
  rasterizer->scanline        = 0;
  rasterizer->scan_min        = 5000;
  rasterizer->scan_max        = -5000;
  rasterizer->col_min         = 5000;
  rasterizer->col_max         = -5000;
  rasterizer->comp_op         = NULL;
}

static void
ctx_rasterizer_rasterize_edges (CtxRasterizer *rasterizer, int winding
#if CTX_SHAPE_CACHE
                                ,CtxShapeEntry *shape
#endif
                               )
{
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  int scan_start = rasterizer->blit_y * CTX_RASTERIZER_AA;
  int scan_end   = scan_start + rasterizer->blit_height * CTX_RASTERIZER_AA;
  int blit_width = rasterizer->blit_width;
  int blit_max_x = rasterizer->blit_x + blit_width;
  int minx = rasterizer->col_min / CTX_SUBDIV - rasterizer->blit_x;
  int maxx = (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV - rasterizer->blit_x;
#if 1
  if (
#if CTX_SHAPE_CACHE
    !shape &&
#endif
    maxx > blit_max_x - 1)
    { maxx = blit_max_x - 1; }
#endif
#if 1
  if (rasterizer->state->gstate.clip_min_x>
      minx)
    { minx = rasterizer->state->gstate.clip_min_x; }
  if (rasterizer->state->gstate.clip_max_x <
      maxx)
    { maxx = rasterizer->state->gstate.clip_max_x; }
#endif
  if (minx < 0)
    { minx = 0; }
  if (minx >= maxx)
    {
      ctx_rasterizer_reset (rasterizer);
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
  rasterizer->scan_min -= (rasterizer->scan_min % CTX_RASTERIZER_AA);
#if CTX_SHAPE_CACHE
  if (shape)
    {
      scan_start = rasterizer->scan_min;
      scan_end   = rasterizer->scan_max;
    }
  else
#endif
    {
      if (rasterizer->scan_min > scan_start)
        {
          dst += (rasterizer->blit_stride * (rasterizer->scan_min-scan_start) /CTX_RASTERIZER_AA);
          scan_start = rasterizer->scan_min;
        }
      if (rasterizer->scan_max < scan_end)
        { scan_end = rasterizer->scan_max; }
    }
  if (rasterizer->state->gstate.clip_min_y * CTX_RASTERIZER_AA > scan_start )
    { scan_start = rasterizer->state->gstate.clip_min_y * CTX_RASTERIZER_AA; }
  if (rasterizer->state->gstate.clip_max_y *  CTX_RASTERIZER_AA < scan_end)
    { scan_end = rasterizer->state->gstate.clip_max_y * CTX_RASTERIZER_AA; }
  if (scan_start > scan_end ||
      (scan_start > (rasterizer->blit_y + rasterizer->blit_height) * CTX_RASTERIZER_AA) ||
      (scan_end < (rasterizer->blit_y) * CTX_RASTERIZER_AA))
  { 
    ctx_rasterizer_reset (rasterizer);
    return;
  }
  ctx_rasterizer_sort_edges (rasterizer);
  for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
    {
      ctx_memset (coverage, 0,
#if CTX_SHAPE_CACHE
                  shape?shape->width:
#endif
                  sizeof (_coverage) );
      ctx_rasterizer_feed_edges (rasterizer);
      ctx_rasterizer_discard_edges (rasterizer);
#if CTX_RASTERIZER_FORCE_AA==1
      rasterizer->needs_aa = 1;
#endif
      if (rasterizer->needs_aa         // due to slopes of active edges
#if CTX_RASTERIZER_AUTOHINT==0
          || rasterizer->lingering_edges    // or due to edges ...
          || rasterizer->pending_edges      //   ... that start or end within scanline
#endif
         )
        {
          for (int i = 0; i < CTX_RASTERIZER_AA; i++)
            {
              ctx_rasterizer_sort_active_edges (rasterizer);
              ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, 1);
              rasterizer->scanline ++;
              ctx_rasterizer_increment_edges (rasterizer, 1);
              if (i!=CTX_RASTERIZER_AA-1)
                {
                  ctx_rasterizer_feed_edges (rasterizer);
                  ctx_rasterizer_discard_edges (rasterizer);
                }
            }
        }
      else
        {
#if 1
          rasterizer->scanline += CTX_RASTERIZER_AA3;
          ctx_rasterizer_increment_edges (rasterizer, CTX_RASTERIZER_AA3);
          ctx_rasterizer_feed_edges (rasterizer);
          ctx_rasterizer_discard_edges (rasterizer);
          ctx_rasterizer_sort_active_edges (rasterizer);
          ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, 0);
          rasterizer->scanline += CTX_RASTERIZER_AA2;
          ctx_rasterizer_increment_edges (rasterizer, CTX_RASTERIZER_AA2);
#else
          rasterizer->scanline += CTX_RASTERIZER_AA;
#endif
        }
      if (maxx>minx)
        {
#if CTX_SHAPE_CACHE
          if (shape == NULL)
#endif
            {
              ctx_rasterizer_apply_coverage (rasterizer,
                                             &dst[ (minx * rasterizer->format->bpp) /8],
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
      dst += rasterizer->blit_stride;
    }
  ctx_rasterizer_reset (rasterizer);
}


static inline int
ctx_rasterizer_fill_rect (CtxRasterizer *rasterizer,
                          int          x0,
                          int          y0,
                          int          x1,
                          int          y1)
{
  if (x0>x1 || y0>y1) { return 1; } // XXX : maybe this only happens under
  //       memory corruption
  if (x1 % CTX_SUBDIV ||
      x0 % CTX_SUBDIV ||
      y1 % CTX_RASTERIZER_AA ||
      y0 % CTX_RASTERIZER_AA)
    { return 0; }
  x1 /= CTX_SUBDIV;
  x0 /= CTX_SUBDIV;
  y1 /= CTX_RASTERIZER_AA;
  y0 /= CTX_RASTERIZER_AA;
  uint8_t coverage[x1-x0 + 1];
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  ctx_memset (coverage, 0xff, sizeof (coverage) );
  if (x0 < rasterizer->blit_x)
    { x0 = rasterizer->blit_x; }
  if (y0 < rasterizer->blit_y)
    { y0 = rasterizer->blit_y; }
  if (y1 > rasterizer->blit_y + rasterizer->blit_height)
    { y1 = rasterizer->blit_y + rasterizer->blit_height; }
  if (x1 > rasterizer->blit_x + rasterizer->blit_width)
    { x1 = rasterizer->blit_x + rasterizer->blit_width; }
  dst += (y0 - rasterizer->blit_y) * rasterizer->blit_stride;
  int width = x1 - x0 + 1;
  if (width > 0)
    {
      rasterizer->scanline = y0 * CTX_RASTERIZER_AA;
      for (int y = y0; y < y1; y++)
        {
          rasterizer->scanline += CTX_RASTERIZER_AA;
          ctx_rasterizer_apply_coverage (rasterizer,
                                         &dst[ (x0) * rasterizer->format->bpp/8],
                                         x0,
                                         coverage, width);
          dst += rasterizer->blit_stride;
        }
    }
  return 1;
}

static void
ctx_rasterizer_fill (CtxRasterizer *rasterizer)
{
  int count = rasterizer->preserve?rasterizer->edge_list.count:0;
  CtxEntry temp[count]; /* copy of already built up path's poly line
                          XXX - by building a large enough path
                          the stack can be smashed!
                         */
  if (rasterizer->preserve)
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) ); }
#if 1
  if (rasterizer->scan_min / CTX_RASTERIZER_AA > rasterizer->blit_y + rasterizer->blit_height ||
      rasterizer->scan_max / CTX_RASTERIZER_AA < rasterizer->blit_y)
    {
      ctx_rasterizer_reset (rasterizer);
      goto done;
    }
#endif
#if 1
  if (rasterizer->col_min / CTX_SUBDIV > rasterizer->blit_x + rasterizer->blit_width ||
      rasterizer->col_max / CTX_SUBDIV < rasterizer->blit_x)
    {
      ctx_rasterizer_reset (rasterizer);
      goto done;
    }
#endif
  rasterizer->state->min_x =
    ctx_mini (rasterizer->state->min_x, rasterizer->col_min / CTX_SUBDIV);
  rasterizer->state->max_x =
    ctx_maxi (rasterizer->state->max_x, rasterizer->col_max / CTX_SUBDIV);
  rasterizer->state->min_y =
    ctx_mini (rasterizer->state->min_y, rasterizer->scan_min / CTX_RASTERIZER_AA);
  rasterizer->state->max_y =
    ctx_maxi (rasterizer->state->max_y, rasterizer->scan_max / CTX_RASTERIZER_AA);
  if (rasterizer->edge_list.count == 6)
    {
      CtxEntry *entry0 = &rasterizer->edge_list.entries[0];
      CtxEntry *entry1 = &rasterizer->edge_list.entries[1];
      CtxEntry *entry2 = &rasterizer->edge_list.entries[2];
      CtxEntry *entry3 = &rasterizer->edge_list.entries[3];
      if ( (entry0->data.s16[2] == entry1->data.s16[2]) &&
           (entry0->data.s16[3] == entry3->data.s16[3]) &&
           (entry1->data.s16[3] == entry2->data.s16[3]) &&
           (entry2->data.s16[2] == entry3->data.s16[2])
         )
        {
          /* XXX ; also check that there is no subpixel bits.. */
          if (ctx_rasterizer_fill_rect (rasterizer,
                                        entry3->data.s16[2],
                                        entry3->data.s16[3],
                                        entry1->data.s16[2],
                                        entry1->data.s16[3]) )
            {
              ctx_rasterizer_reset (rasterizer);
              goto done;
            }
        }
    }
  ctx_rasterizer_finish_shape (rasterizer);
#if CTX_SHAPE_CACHE
  uint32_t hash = ctx_rasterizer_poly_to_edges (rasterizer);
  int width = (rasterizer->col_max + (CTX_SUBDIV-1) ) / CTX_SUBDIV - rasterizer->col_min/CTX_SUBDIV;
  int height = (rasterizer->scan_max + (CTX_RASTERIZER_AA-1) ) / CTX_RASTERIZER_AA - rasterizer->scan_min / CTX_RASTERIZER_AA;
  if (width * height < CTX_SHAPE_CACHE_DIM && width >=1 && height >= 1
      && width < CTX_SHAPE_CACHE_MAX_DIM
      && height < CTX_SHAPE_CACHE_MAX_DIM)
    {
      int scan_min = rasterizer->scan_min;
      int col_min = rasterizer->col_min;
      CtxShapeEntry *shape = ctx_shape_entry_find (hash, width, height, ctx_shape_time++);
      if (shape->uses == 0)
        {
          ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule, shape);
        }
      scan_min -= (scan_min % CTX_RASTERIZER_AA);
      rasterizer->scanline = scan_min;
      int y0 = rasterizer->scanline / CTX_RASTERIZER_AA;
      int y1 = y0 + shape->height;
      int x0 = col_min / CTX_SUBDIV;
      int ymin = y0;
      int x1 = x0 + shape->width;
      int clip_x_min = rasterizer->blit_x;
      int clip_x_max = rasterizer->blit_x + rasterizer->blit_width - 1;
      int clip_y_min = rasterizer->blit_y;
      int clip_y_max = rasterizer->blit_y + rasterizer->blit_height - 1;
#if 0
      if (rasterizer->state->gstate.clip_min_x>
          clip_x_min)
        { clip_x_min = rasterizer->state->gstate.clip_min_x; }
      if (rasterizer->state->gstate.clip_max_x <
          clip_x_max)
        { clip_x_max = rasterizer->state->gstate.clip_max_x; }
#endif
#if 0
      if (rasterizer->state->gstate.clip_min_y>
          clip_y_min)
        { clip_y_min = rasterizer->state->gstate.clip_min_y; }
      if (rasterizer->state->gstate.clip_max_y <
          clip_y_max)
        { clip_y_max = rasterizer->state->gstate.clip_max_y; }
#endif
      if (x1 >= clip_x_max)
        { x1 = clip_x_max; }
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
            if ( (y >= clip_y_min) && (y <= clip_y_max) )
              {
                ctx_rasterizer_apply_coverage (rasterizer,
                                               ( (uint8_t *) rasterizer->buf) + (y-rasterizer->blit_y) * rasterizer->blit_stride + (int) (x0) * rasterizer->format->bpp/8,
                                               x0,
                                               &shape->data[shape->width * (int) (y-ymin) + xo],
                                               ewidth );
              }
          }
      if (shape->uses != 0)
        {
          ctx_rasterizer_reset (rasterizer);
        }
      ctx_shape_entry_release (shape);
      goto done;
    }
  else
#else
  ctx_rasterizer_poly_to_edges (rasterizer);
#endif
    {
      //fprintf (stderr, "%i %i\n", width, height);
      ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                      , NULL
#endif
                                     );
    }
done:
  if (rasterizer->preserve)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
    }
  rasterizer->preserve = 0;
}

static int _ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke);
static inline void
ctx_rasterizer_glyph (CtxRasterizer *rasterizer, uint32_t unichar, int stroke)
{
  _ctx_glyph (rasterizer->ctx, unichar, stroke);
}

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible);
static inline void
ctx_rasterizer_text (CtxRasterizer *rasterizer, const char *string, int stroke)
{
  _ctx_text (rasterizer->ctx, string, stroke, 1);
}

void
_ctx_set_font (Ctx *ctx, const char *name);
static inline void
ctx_rasterizer_set_font (CtxRasterizer *rasterizer, const char *font_name)
{
  _ctx_set_font (rasterizer->ctx, font_name);
}

static void
ctx_rasterizer_arc (CtxRasterizer *rasterizer,
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
    { full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS; }
  float step = CTX_PI*2.0/full_segments;
  int steps;
  if (end_angle == start_angle)
    {
//  if (rasterizer->has_prev!=0)
      ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                              y + ctx_sinf (end_angle) * radius);
//    else
//    ctx_rasterizer_move_to (rasterizer, x + ctx_cosf (end_angle) * radius,
//                          y + ctx_sinf (end_angle) * radius);
      return;
    }
#if 0
  if ( (!anticlockwise && (end_angle - start_angle >= CTX_PI*2) ) ||
       ( (anticlockwise && (start_angle - end_angle >= CTX_PI*2) ) ) )
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
        { steps = full_segments - steps; };
    }
  if (anticlockwise) { step = step * -1; }
  int first = 1;
  if (steps == 0 || steps==full_segments -1  || (anticlockwise && steps == full_segments) )
    {
      float xv = x + ctx_cosf (start_angle) * radius;
      float yv = y + ctx_sinf (start_angle) * radius;
      if (!rasterizer->has_prev)
        { ctx_rasterizer_move_to (rasterizer, xv, yv); }
      first = 0;
    }
  else
    {
      for (float angle = start_angle, i = 0; i < steps; angle += step, i++)
        {
          float xv = x + ctx_cosf (angle) * radius;
          float yv = y + ctx_sinf (angle) * radius;
          if (first && !rasterizer->has_prev)
            { ctx_rasterizer_move_to (rasterizer, xv, yv); }
          else
            { ctx_rasterizer_line_to (rasterizer, xv, yv); }
          first = 0;
        }
    }
  ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                          y + ctx_sinf (end_angle) * radius);
}

static void
ctx_rasterizer_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y)
{
  /* XXX : it is probably cheaper/faster to do quad interpolation directly -
   *       though it will increase the code-size
   */
  ctx_rasterizer_curve_to (rasterizer,
                           (cx * 2 + rasterizer->x) / 3.0f, (cy * 2 + rasterizer->y) / 3.0f,
                           (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                           x,                              y);
}

static inline void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                            float cx, float cy, float x, float y)
{
  ctx_rasterizer_quad_to (rasterizer, cx + rasterizer->x, cy + rasterizer->y,
                          x  + rasterizer->x, y  + rasterizer->y);
}

#define LENGTH_OVERSAMPLE 1
static void
ctx_rasterizer_pset (CtxRasterizer *rasterizer, int x, int y, uint8_t cov)
{
  // XXX - we avoid rendering here x==0 - to keep with
  //  an off-by one elsewhere
  //
  //  XXX onlt works in rgba8 formats
  if (x <= 0 || y < 0 || x >= rasterizer->blit_width ||
      y >= rasterizer->blit_height)
    { return; }
  uint8_t fg_color[4];
  ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color, fg_color);
  uint8_t pixel[4];
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  dst += y * rasterizer->blit_stride;
  dst += x * rasterizer->format->bpp / 8;
  if (!rasterizer->format->to_comp ||
      !rasterizer->format->from_comp)
    { return; }
  if (cov == 255)
    {
      for (int c = 0; c < 4; c++)
        {
          pixel[c] = fg_color[c];
        }
    }
  else
    {
      rasterizer->format->to_comp (rasterizer, x, dst, &pixel[0], 1);
      for (int c = 0; c < 4; c++)
        {
          pixel[c] = ctx_lerp_u8 (pixel[c], fg_color[c], cov);
        }
    }
  rasterizer->format->from_comp (rasterizer, x, &pixel[0], dst, 1);
}

static void
ctx_rasterizer_stroke_1px (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxEntry *temp = rasterizer->edge_list.entries;
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
          int length = ctx_maxf (abs (dx), abs (dy) );
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
                  ctx_rasterizer_pset (rasterizer, tx/256, ty/256, 255);
                  tx += dx;
                  ty += dy;
                  ctx_rasterizer_pset (rasterizer, tx/256, ty/256, 255);
                }
            }
          prev_x = x;
          prev_y = y;
        }
      end = i-1;
foo:
      start = end+1;
    }
  ctx_rasterizer_reset (rasterizer);
}

static void
ctx_rasterizer_stroke (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  int preserved = rasterizer->preserve;
  CtxEntry temp[count]; /* copy of already built up path's poly line  */
  memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) );
  if (rasterizer->state->gstate.line_width <= 0.0f &&
      rasterizer->state->gstate.line_width > -10.0f)
    {
      ctx_rasterizer_stroke_1px (rasterizer);
    }
  else
    {
      ctx_rasterizer_reset (rasterizer); /* then start afresh with our stroked shape  */
      CtxMatrix transform_backup = rasterizer->state->gstate.transform;
      ctx_matrix_identity (&rasterizer->state->gstate.transform);
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float half_width_x = rasterizer->state->gstate.line_width/2;
      float half_width_y = rasterizer->state->gstate.line_width/2;
      if (rasterizer->state->gstate.line_width <= 0.0f)
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
                      ctx_rasterizer_finish_shape (rasterizer);
                      ctx_rasterizer_move_to (rasterizer, prev_x+dy, prev_y-dx);
                    }
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  // XXX possible miter line-to
                  ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
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
              float length = ctx_fast_hypotf (dx, dy);
              dx = dx/length * half_width_x;
              dy = dy/length * half_width_y;
              if (length>0.001f)
                {
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  // XXX possible miter line-to
                  ctx_rasterizer_line_to (rasterizer, x-dy,      y+dx);
                }
              prev_x = x;
              prev_y = y;
              if (entry->code == CTX_NEW_EDGE)
                {
                  x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;
                  dx = x - prev_x;
                  dy = y - prev_y;
                  length = ctx_fast_hypotf (dx, dy);
                  if (length>0.001f)
                    {
                      dx = dx / length * half_width_x;
                      dy = dy / length * half_width_y;
                      ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                      ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
                    }
                }
              if ( (prev_x != x) && (prev_y != y) )
                {
                  prev_x = x;
                  prev_y = y;
                }
            }
          start = end+1;
        }
      ctx_rasterizer_finish_shape (rasterizer);
      switch (rasterizer->state->gstate.line_cap)
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
                          ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;
                  has_prev = 1;
                }
              ctx_rasterizer_move_to (rasterizer, x, y);
              ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
              ctx_rasterizer_finish_shape (rasterizer);
              break;
            }
        }
      switch (rasterizer->state->gstate.line_join)
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
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                }
              break;
            }
        }
#if 0
      ctx_rasterizer_poly_to_edges    (rasterizer);
      ctx_rasterizer_rasterize_edges (rasterizer, 1, NULL);
#else
      CtxFillRule rule_backup = rasterizer->state->gstate.fill_rule;
      rasterizer->state->gstate.fill_rule = CTX_FILL_RULE_WINDING;
      rasterizer->preserve = 0; // so fill isn't tripped
      ctx_rasterizer_fill (rasterizer);
      rasterizer->state->gstate.fill_rule = rule_backup;
#endif
      //rasterizer->state->gstate.source = source_backup;
      rasterizer->state->gstate.transform = transform_backup;
    }
  if (preserved)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
}

static void
ctx_rasterizer_clip (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxEntry temp[count]; /* copy of already built up path's poly line  */
  if (rasterizer->preserve)
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) ); }
  int minx = 5000;
  int miny = 5000;
  int maxx = -5000;
  int maxy = -5000;
  int prev_x = 0;
  int prev_y = 0;
  for (int i = 0; i < count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      float x, y;
      if (entry->code == CTX_NEW_EDGE)
        {
          prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
          prev_y = entry->data.s16[1] * 1.0f / CTX_RASTERIZER_AA;
          if (prev_x < minx) { minx = prev_x; }
          if (prev_y < miny) { miny = prev_y; }
          if (prev_x > maxx) { maxx = prev_x; }
          if (prev_y > maxy) { maxy = prev_y; }
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / CTX_RASTERIZER_AA;
      if (x < minx) { minx = x; }
      if (y < miny) { miny = y; }
      if (x > maxx) { maxx = x; }
      if (y > maxy) { maxy = y; }
    }
  rasterizer->state->gstate.clip_min_x = ctx_maxi (minx,
                                         rasterizer->state->gstate.clip_min_x);
  rasterizer->state->gstate.clip_min_y = ctx_maxi (miny,
                                         rasterizer->state->gstate.clip_min_y);
  rasterizer->state->gstate.clip_max_x = ctx_mini (maxx,
                                         rasterizer->state->gstate.clip_max_x);
  rasterizer->state->gstate.clip_max_y = ctx_mini (maxy,
                                         rasterizer->state->gstate.clip_max_y);
  ctx_rasterizer_reset (rasterizer);
  if (rasterizer->preserve)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
}

static void
ctx_rasterizer_set_texture (CtxRasterizer *rasterizer,
                            int   no,
                            float x,
                            float y)
{
  if (no < 0 || no >= CTX_MAX_TEXTURES) { no = 0; }
  if (rasterizer->ctx->texture[no].data == NULL)
    {
      ctx_log ("failed setting texture %i\n", no);
      return;
    }
  rasterizer->state->gstate.source.type = CTX_SOURCE_IMAGE;
  rasterizer->state->gstate.source.image.buffer = &rasterizer->ctx->texture[no];
  //ctx_user_to_device (rasterizer->state, &x, &y);
  rasterizer->state->gstate.source.image.x0 = 0;
  rasterizer->state->gstate.source.image.y0 = 0;
  rasterizer->state->gstate.source.transform = rasterizer->state->gstate.transform;
  ctx_matrix_translate (&rasterizer->state->gstate.source.transform, x, y);
  ctx_matrix_invert (&rasterizer->state->gstate.source.transform);
}

#if 0
static void
ctx_rasterizer_load_image (CtxRasterizer *rasterizer,
                           const char  *path,
                           float x,
                           float y)
{
  // decode PNG, put it in image is slot 1,
  // magic width height stride format data
  ctx_buffer_load_png (&rasterizer->ctx->texture[0], path);
  ctx_rasterizer_set_texture (rasterizer, 0, x, y);
}
#endif

static void
ctx_rasterizer_set_pixel (CtxRasterizer *rasterizer,
                          uint16_t x,
                          uint16_t y,
                          uint8_t r,
                          uint8_t g,
                          uint8_t b,
                          uint8_t a)
{
  rasterizer->state->gstate.source.type = CTX_SOURCE_COLOR;
  ctx_color_set_RGBA8 (rasterizer->state, &rasterizer->state->gstate.source.color, r, g, b, a);
#if 1
  ctx_rasterizer_pset (rasterizer, x, y, 255);
#else
  ctx_rasterizer_move_to (rasterizer, x, y);
  ctx_rasterizer_rel_line_to (rasterizer, 1, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, 1);
  ctx_rasterizer_rel_line_to (rasterizer, -1, 0);
  ctx_rasterizer_fill (rasterizer);
#endif
}

static void
ctx_rasterizer_rectangle (CtxRasterizer *rasterizer,
                          float x,
                          float y,
                          float width,
                          float height)
{
  ctx_rasterizer_move_to (rasterizer, x, y);
  ctx_rasterizer_rel_line_to (rasterizer, width, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, height);
  ctx_rasterizer_rel_line_to (rasterizer, -width, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, -height);
  ctx_rasterizer_rel_line_to (rasterizer, 0.3, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

static void
ctx_rasterizer_round_rectangle (CtxRasterizer *rasterizer, float x, float y, float width, float height, float corner_radius)
{
  float aspect  = 1.0f;
  float radius  = corner_radius / aspect;
  float degrees = CTX_PI / 180.0f;

  ctx_rasterizer_finish_shape (rasterizer);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + radius, radius, 180 * degrees, 270 * degrees, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

static void
ctx_rasterizer_process (void *user_data, CtxCommand *command)
{
  CtxEntry *entry = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) user_data;
  CtxState *state = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  switch (c->code)
    {
      case CTX_LINE_TO:
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_LINE_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_MOVE_TO:
        ctx_rasterizer_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rasterizer_rel_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_CURVE_TO:
        ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
                                 c->c.x1, c->c.y1,
                                 c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
                                     c->c.x1, c->c.y1,
                                     c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_ARC:
        ctx_rasterizer_arc (rasterizer, c->arc.x, c->arc.y, c->arc.radius, c->arc.angle1, c->arc.angle2, c->arc.direction);
        break;
      case CTX_RECTANGLE:
        ctx_rasterizer_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                  c->rectangle.width, c->rectangle.height);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_rasterizer_round_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                        c->rectangle.width, c->rectangle.height,
                                        c->rectangle.radius);
        break;
      case CTX_SET_PIXEL:
        ctx_rasterizer_set_pixel (rasterizer, c->set_pixel.x, c->set_pixel.y,
                                  c->set_pixel.rgba[0],
                                  c->set_pixel.rgba[1],
                                  c->set_pixel.rgba[2],
                                  c->set_pixel.rgba[3]);
        break;
      case CTX_TEXTURE:
        ctx_rasterizer_set_texture (rasterizer, ctx_arg_u32 (0),
                                    ctx_arg_float (2), ctx_arg_float (3) );
        break;
#if 0
      case CTX_LOAD_IMAGE:
        ctx_rasterizer_load_image (rasterizer, ctx_arg_string(),
                                   ctx_arg_float (0), ctx_arg_float (1) );
        break;
#endif
      case CTX_GRADIENT_STOP:
        {
          float rgba[4]= {ctx_u8_to_float (ctx_arg_u8 (4) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+1) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+2) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+3) )
                         };
          ctx_rasterizer_gradient_add_stop (rasterizer,
                                            ctx_arg_float (0), rgba);
        }
        break;
      case CTX_LINEAR_GRADIENT:
#if CTX_GRADIENT_CACHE
        ctx_gradient_cache_reset();
#endif
        ctx_state_gradient_clear_stops (rasterizer->state);
        break;
      case CTX_RADIAL_GRADIENT:
#if CTX_GRADIENT_CACHE
        ctx_gradient_cache_reset();
#endif
        ctx_state_gradient_clear_stops (rasterizer->state);
        break;
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_SAVE:
      case CTX_RESTORE:
        rasterizer->uses_transforms = 1;
        ctx_interpret_transforms (rasterizer->state, entry, NULL);
        break;
      case CTX_STROKE:
        ctx_rasterizer_stroke (rasterizer);
        break;
      case CTX_SET_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_TEXT:
        ctx_rasterizer_text (rasterizer, ctx_arg_string(), 0);
        break;
      case CTX_TEXT_STROKE:
        ctx_rasterizer_text (rasterizer, ctx_arg_string(), 1);
        break;
      case CTX_GLYPH:
        ctx_rasterizer_glyph (rasterizer, entry[0].data.u32[0], entry[0].data.u8[4]);
        break;
      case CTX_FILL:
        ctx_rasterizer_fill (rasterizer);
        break;
      case CTX_NEW_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLIP:
        ctx_rasterizer_clip (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_finish_shape (rasterizer);
        break;
    }
  ctx_interpret_pos_bare (rasterizer->state, entry, NULL);
  ctx_interpret_style (rasterizer->state, entry, NULL);
  if (command->code == CTX_SET_LINE_WIDTH)
    {
      float x = state->gstate.line_width;
      /* normalize line width according to scaling factor
       */
      x = x * ctx_maxf (ctx_maxf (ctx_fabsf (state->gstate.transform.m[0][0]),
                                  ctx_fabsf (state->gstate.transform.m[0][1]) ),
                        ctx_maxf (ctx_fabsf (state->gstate.transform.m[1][0]),
                                  ctx_fabsf (state->gstate.transform.m[1][1]) ) );
      state->gstate.line_width = x;
    }
}

static int
ctx_rect_intersect (const CtxRectangle *a, const CtxRectangle *b)
{
  if (a->x >= b->x + b->width ||
      b->x >= a->x + a->width ||
      a->y >= b->y + b->height ||
      b->y >= a->y + a->height) return 0;

  return 1;
}

static void
_ctx_add_hash (CtxHasher *hasher, CtxRectangle *shape_rect, uint32_t hash)
{
  CtxRectangle rect = {0,0, hasher->rasterizer.blit_width/hasher->cols,
                            hasher->rasterizer.blit_height/hasher->rows};
  int hno = 0;
  for (int row = 0; row < hasher->rows; row++)
    for (int col = 0; col < hasher->cols; col++, hno++)
    {
      rect.x = col * rect.width;
      rect.y = row * rect.height;
      if (ctx_rect_intersect (shape_rect, &rect))
      {
        hasher->hashes[row * hasher->cols + col] *= 5;
        hasher->hashes[row * hasher->cols + col] ^= hash;
      }
    }
}

static void
ctx_hasher_process (void *user_data, CtxCommand *command)
{
  CtxEntry *entry = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) user_data;
  CtxHasher *hasher = (CtxHasher*) user_data;
  CtxState *state = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  switch (c->code)
    {
      case CTX_TEXT:
        {
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());
          float height = ctx_get_font_size (rasterizer->ctx);

          int hash = ctx_strhash (ctx_arg_string(), 0); // clips strings XXX

           CtxRectangle shape_rect = {
              rasterizer->x, rasterizer->y - height,
              width, height * 2
           };

           hash *= 21129;
           hash += shape_rect.x;
           hash *= 124229;
           hash += shape_rect.y;

          _ctx_add_hash (hasher, &shape_rect, hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_TEXT_STROKE:
        {
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());
          float height = ctx_get_font_size (rasterizer->ctx);

          int hash = 99+ctx_strhash (ctx_arg_string(), 0); // clips strings XXX

           CtxRectangle shape_rect = {
              rasterizer->x, rasterizer->y - height,
              width, height * 2
           };
           hash *= 21129;
           hash += shape_rect.x;
           hash *= 124229;
           hash += shape_rect.y;

          _ctx_add_hash (hasher, &shape_rect, hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_GLYPH:
        // XXX check bounds
          ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_FILL:
        {
        int hash = ctx_rasterizer_poly_to_hash (rasterizer);
        CtxRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV,
          rasterizer->scan_min / CTX_RASTERIZER_AA,

          (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV -
          rasterizer->col_min / CTX_SUBDIV,

          (rasterizer->scan_max + CTX_RASTERIZER_AA-1)/ CTX_RASTERIZER_AA -
          rasterizer->scan_min / CTX_RASTERIZER_AA
        };
        //fprintf (stderr, "%i,%i %ix%i \n", shape_rect.x, shape_rect.y, shape_rect.width, shape_rect.height);

        // XXX not doing a good job with state
        hash ^= (rasterizer->state->gstate.fill_rule * 23);
        hash ^= (rasterizer->state->gstate.source.type * 117);

        // XXX color does not work - we need to do a get-color, this rgba
        // is possibly even unused
        hash ^= (rasterizer->state->gstate.source.color.rgba[0] * 111);
        hash ^= (rasterizer->state->gstate.source.color.rgba[1] * 129);
        hash ^= (rasterizer->state->gstate.source.color.rgba[2] * 147);
        hash ^= (rasterizer->state->gstate.source.color.rgba[3] * 477);

        _ctx_add_hash (hasher, &shape_rect, hash);

        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;
        }
        break;
      case CTX_STROKE:
        // XXX check bounds
        //   update hashes
        //ctx_rasterizer_stroke (rasterizer);
        {
        int hash = ctx_rasterizer_poly_to_hash (rasterizer);
        CtxRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV,
          rasterizer->scan_min / CTX_RASTERIZER_AA,

          (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV -
          rasterizer->col_min / CTX_SUBDIV,

          (rasterizer->scan_max + CTX_RASTERIZER_AA-1)/ CTX_RASTERIZER_AA -
          rasterizer->scan_min / CTX_RASTERIZER_AA
        };

        shape_rect.width += rasterizer->state->gstate.line_width * 2;
        shape_rect.height += rasterizer->state->gstate.line_width * 2;
        shape_rect.x -= rasterizer->state->gstate.line_width;
        shape_rect.y -= rasterizer->state->gstate.line_width;

        // XXX not doing a good job with state
        hash ^= (int)(rasterizer->state->gstate.line_width * 110);
        hash ^= (rasterizer->state->gstate.line_cap * 23);
        hash ^= (rasterizer->state->gstate.source.type * 117);
        hash ^= (rasterizer->state->gstate.source.color.rgba[0] * 111);
        hash ^= (rasterizer->state->gstate.source.color.rgba[1] * 129);
        hash ^= (rasterizer->state->gstate.source.color.rgba[2] * 147);
        hash ^= (rasterizer->state->gstate.source.color.rgba[3] * 477);

        _ctx_add_hash (hasher, &shape_rect, hash);
        }
        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;
        break;
        /* the above cases are the actual painting cases and 
         * the only ones differing from the rasterizer
         */

      case CTX_LINE_TO:
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_LINE_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_MOVE_TO:
        ctx_rasterizer_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rasterizer_rel_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_CURVE_TO:
        ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
                                 c->c.x1, c->c.y1,
                                 c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
                                     c->c.x1, c->c.y1,
                                     c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_ARC:
        ctx_rasterizer_arc (rasterizer, c->arc.x, c->arc.y, c->arc.radius, c->arc.angle1, c->arc.angle2, c->arc.direction);
        break;
      case CTX_RECTANGLE:
        ctx_rasterizer_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                  c->rectangle.width, c->rectangle.height);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_rasterizer_round_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                        c->rectangle.width, c->rectangle.height,
                                        c->rectangle.radius);
        break;
      case CTX_SET_PIXEL:
        ctx_rasterizer_set_pixel (rasterizer, c->set_pixel.x, c->set_pixel.y,
                                  c->set_pixel.rgba[0],
                                  c->set_pixel.rgba[1],
                                  c->set_pixel.rgba[2],
                                  c->set_pixel.rgba[3]);
        break;
      case CTX_TEXTURE:
        ctx_rasterizer_set_texture (rasterizer, ctx_arg_u32 (0),
                                    ctx_arg_float (2), ctx_arg_float (3) );
        break;
#if 0
      case CTX_LOAD_IMAGE:
        ctx_rasterizer_load_image (rasterizer, ctx_arg_string(),
                                   ctx_arg_float (0), ctx_arg_float (1) );
        break;
#endif
      case CTX_GRADIENT_STOP:
        {
          float rgba[4]= {ctx_u8_to_float (ctx_arg_u8 (4) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+1) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+2) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+3) )
                         };
          ctx_rasterizer_gradient_add_stop (rasterizer,
                                            ctx_arg_float (0), rgba);
        }
        break;
      case CTX_LINEAR_GRADIENT:
#if CTX_GRADIENT_CACHE
        ctx_gradient_cache_reset();
#endif
        ctx_state_gradient_clear_stops (rasterizer->state);
        break;
      case CTX_RADIAL_GRADIENT:
#if CTX_GRADIENT_CACHE
        ctx_gradient_cache_reset();
#endif
        ctx_state_gradient_clear_stops (rasterizer->state);
        break;
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_SAVE:
      case CTX_RESTORE:
        rasterizer->uses_transforms = 1;
        ctx_interpret_transforms (rasterizer->state, entry, NULL);
        break;
      case CTX_SET_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_NEW_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLIP:
        ctx_rasterizer_clip (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_finish_shape (rasterizer);
        break;
    }
  ctx_interpret_pos_bare (rasterizer->state, entry, NULL);
  ctx_interpret_style (rasterizer->state, entry, NULL);
  if (command->code == CTX_SET_LINE_WIDTH)
    {
      float x = state->gstate.line_width;
      /* normalize line width according to scaling factor
       */
      x = x * ctx_maxf (ctx_maxf (ctx_fabsf (state->gstate.transform.m[0][0]),
                                  ctx_fabsf (state->gstate.transform.m[0][1]) ),
                        ctx_maxf (ctx_fabsf (state->gstate.transform.m[1][0]),
                                  ctx_fabsf (state->gstate.transform.m[1][1]) ) );
      state->gstate.line_width = x;
    }
}



#if CTX_ENABLE_RGB8

static inline void
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

static inline void
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
static inline void
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

static inline void
ctx_RGBA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int gray = (rgba[0]+rgba[1]+rgba[2]) /3 ;
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/CTX_RASTERIZER_AA, 0, 127);
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

static int
ctx_composite_GRAY1 (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_GRAY1_to_RGBA8 (rasterizer, x, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x, &pixels[0], coverage, count);
  ctx_RGBA8_to_GRAY1 (rasterizer, x, &pixels[0], dst, count);
  return ret;
}
#endif

#if CTX_ENABLE_GRAY2
static inline void
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

static inline void
ctx_RGBA8_to_GRAY2 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (rgba[0]+rgba[1]+rgba[2]) /3 ;
      val >>= 6;
      *pixel = *pixel & (~ (3 << ( (x&3) <<1) ) );
      *pixel = *pixel | ( (val << ( (x&3) <<1) ) );
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

static int
ctx_composite_GRAY2 (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_GRAY2_to_RGBA8 (rasterizer, x, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x, &pixels[0], coverage, count);
  ctx_RGBA8_to_GRAY2 (rasterizer, x, &pixels[0], dst, count);
  return ret;
}
#endif

#if CTX_ENABLE_GRAY4
static inline void
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

static inline void
ctx_RGBA8_to_GRAY4 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (rgba[0]+rgba[1]+rgba[2]) /3 ;
      val >>= 4;
      *pixel = *pixel & (~ (15 << ( (x&1) <<2) ) );
      *pixel = *pixel | ( (val << ( (x&1) <<2) ) );
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

static int
ctx_composite_GRAY4 (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_GRAY4_to_RGBA8 (rasterizer, x, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x, &pixels[0], coverage, count);
  ctx_RGBA8_to_GRAY4 (rasterizer, x, &pixels[0], dst, count);
  return ret;
}
#endif

#if CTX_ENABLE_GRAY8
static inline void
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

static inline void
ctx_RGBA8_to_GRAY8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = (rgba[0]+rgba[1]+rgba[2]) /3;
      // for internal uses... using only green would work
      pixel+=1;
      rgba +=4;
    }
}
#endif

#if CTX_ENABLE_GRAYA8
static inline void
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

static inline void
ctx_RGBA8_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = (rgba[0]+rgba[1]+rgba[2]) /3;
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

static int
ctx_composite_RGB332 (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_RGB332_to_RGBA8 (rasterizer, x, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x, &pixels[0], coverage, count);
  ctx_RGBA8_to_RGB332 (rasterizer, x, &pixels[0], dst, count);
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

static int
ctx_composite_RGB565 (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_RGB565_to_RGBA8 (rasterizer, x, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x, &pixels[0], coverage, count);
  ctx_RGBA8_to_RGB565 (rasterizer, x, &pixels[0], dst, count);
  return ret;
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

static int
ctx_composite_RGB565_BS (CtxRasterizer *rasterizer, int x, uint8_t *dst, uint8_t *coverage, int count)
{
  int ret;
  uint8_t pixels[count * 4];
  ctx_RGB565_BS_to_RGBA8 (rasterizer, x, dst, &pixels[0], count);
  ret = ctx_composite_RGBA8 (rasterizer, x, &pixels[0], coverage, count);
  ctx_RGBA8_to_RGB565_BS (rasterizer, x, &pixels[0], dst, count);
  return ret;
}

#endif

static void
ctx_rasterizer_deinit (CtxRasterizer *rasterizer)
{
  ctx_renderstream_deinit (&rasterizer->edge_list);
  free (rasterizer);
}

static CtxPixelFormatInfo ctx_pixel_formats[]=
{
  {
    CTX_FORMAT_RGBA8, 4, 32, 4, 0, 0,
    NULL, NULL, ctx_composite_RGBA8
  },

#if CTX_ENABLE_BGRA8
  {
    CTX_FORMAT_BGRA8, 4, 32, 4, 0, 0,
    ctx_BGRA8_to_RGBA8, ctx_RGBA8_to_BGRA8, ctx_composite_BGRA8
  },
#endif
#if CTX_ENABLE_GRAYF
  {
    CTX_FORMAT_GRAYF, 1, 32, 4, 0, 0,
    NULL, NULL, ctx_composite_GRAYF
  },
#endif
#if CTX_ENABLE_RGBAF
  {
    CTX_FORMAT_RGBAF, 4, 128, 4 * 4, 0, 0,
    NULL, NULL, ctx_composite_RGBAF
  },
#endif
#if CTX_ENABLE_RGB8
  {
    CTX_FORMAT_RGB8, 3, 24, 4, 0, 0,
    ctx_RGB8_to_RGBA8, ctx_RGBA8_to_RGB8, ctx_composite_RGBA8_convert
  },
#endif
#if CTX_ENABLE_GRAY1
  {
    CTX_FORMAT_GRAY1, 1, 1, 4, 1, 1,
    ctx_GRAY1_to_RGBA8, ctx_RGBA8_to_GRAY1, ctx_composite_GRAY1
  },
#endif
#if CTX_ENABLE_GRAY2
  {
    CTX_FORMAT_GRAY2, 1, 2, 4, 4, 4,
    ctx_GRAY2_to_RGBA8, ctx_RGBA8_to_GRAY2, ctx_composite_GRAY2
  },
#endif
#if CTX_ENABLE_GRAY4
  {
    CTX_FORMAT_GRAY4, 1, 4, 4, 16, 16,
    ctx_GRAY4_to_RGBA8, ctx_RGBA8_to_GRAY4, ctx_composite_GRAY4
  },
#endif
#if CTX_ENABLE_GRAY8
  {
    CTX_FORMAT_GRAY8, 1, 8, 4, 0, 0,
    ctx_GRAY8_to_RGBA8, ctx_RGBA8_to_GRAY8, ctx_composite_RGBA8_convert
  },
#endif
#if CTX_ENABLE_GRAYA8
  {
    CTX_FORMAT_GRAYA8, 2, 16, 4, 0, 0,
    ctx_GRAYA8_to_RGBA8, ctx_RGBA8_to_GRAYA8, ctx_composite_RGBA8_convert
  },
#endif
#if CTX_ENABLE_RGB332
  {
    CTX_FORMAT_RGB332, 3, 8, 4, 10, 12,
    ctx_RGB332_to_RGBA8, ctx_RGBA8_to_RGB332,
    ctx_composite_RGB332
  },
#endif
#if CTX_ENABLE_RGB565
  {
    CTX_FORMAT_RGB565, 3, 16, 4, 32, 64,
    ctx_RGB565_to_RGBA8, ctx_RGBA8_to_RGB565,
    ctx_composite_RGB565
  },
#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED
  {
    CTX_FORMAT_RGB565_BYTESWAPPED, 3, 16, 4, 32, 64,
    ctx_RGB565_BS_to_RGBA8,
    ctx_RGBA8_to_RGB565_BS,
    ctx_composite_RGB565_BS
  },
#endif
#if CTX_ENABLE_CMYKAF
  {
    CTX_FORMAT_CMYKAF, 5, 160, 4 * 5, 0, 0,
    NULL, NULL, ctx_composite_CMYKAF
  },
#endif
#if CTX_ENABLE_CMYKA8
  {
    CTX_FORMAT_CMYKA8, 5, 40, 4 * 5, 0, 0,
    NULL, NULL, ctx_composite_CMYKA8
  },
#endif
#if CTX_ENABLE_CMYK8
  {
    CTX_FORMAT_CMYK8, 5, 32, 4 * 5, 0, 0,
    NULL, NULL, ctx_composite_CMYK8
  },
#endif
};

static CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format)
{
  for (unsigned int i = 0; i < sizeof (ctx_pixel_formats) /sizeof (ctx_pixel_formats[0]); i++)
    {
      if (ctx_pixel_formats[i].pixel_format == format)
        {
          return &ctx_pixel_formats[i];
        }
    }
  return NULL;
}

static CtxRasterizer *
ctx_rasterizer_init (CtxRasterizer *rasterizer, Ctx *ctx, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format)
{
  ctx_memset (rasterizer, 0, sizeof (CtxRasterizer) );
  rasterizer->vfuncs.process = ctx_rasterizer_process;
  rasterizer->vfuncs.free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  rasterizer->edge_list.flags |= CTX_RENDERSTREAM_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->ctx         = ctx;
  ctx_state_init (rasterizer->state);
  rasterizer->buf         = data;
  rasterizer->blit_x      = x;
  rasterizer->blit_y      = y;
  rasterizer->blit_width  = width;
  rasterizer->blit_height = height;
  rasterizer->state->gstate.clip_min_x  = x;
  rasterizer->state->gstate.clip_min_y  = y;
  rasterizer->state->gstate.clip_max_x  = x + width - 1;
  rasterizer->state->gstate.clip_max_y  = y + height - 1;
  rasterizer->blit_stride = stride;
  rasterizer->scan_min    = 5000;
  rasterizer->scan_max    = -5000;
  rasterizer->format = ctx_pixel_format_info (pixel_format);
  return rasterizer;
}

static CtxRasterizer *
ctx_hasher_init (CtxRasterizer *rasterizer, Ctx *ctx, CtxState *state, int width, int height, int rows, int cols)
{
  CtxHasher *hasher = (CtxHasher*)rasterizer;
  ctx_memset (rasterizer, 0, sizeof (CtxHasher) );
  rasterizer->vfuncs.process = ctx_hasher_process;
  rasterizer->vfuncs.free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  rasterizer->edge_list.flags |= CTX_RENDERSTREAM_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->ctx         = ctx;
  ctx_state_init (rasterizer->state);
  rasterizer->blit_x      = 0;
  rasterizer->blit_y      = 0;
  rasterizer->blit_width  = width;
  rasterizer->blit_height = height;
  rasterizer->state->gstate.clip_min_x  = 0;
  rasterizer->state->gstate.clip_min_y  = 0;
  rasterizer->state->gstate.clip_max_x  = width - 1;
  rasterizer->state->gstate.clip_max_y  = height - 1;
  rasterizer->scan_min    = 5000;
  rasterizer->scan_max    = -5000;

  hasher->rows = rows;
  hasher->cols = cols;

  hasher->hashes   = calloc (sizeof(uint64_t), rows * cols);

  return rasterizer;
}


Ctx *
ctx_new_for_buffer (CtxBuffer *buffer)
{
  Ctx *ctx = ctx_new ();
  ctx_set_renderer (ctx,
                    ctx_rasterizer_init ( (CtxRasterizer *) malloc (sizeof (CtxRasterizer) ),
                                          ctx, &ctx->state,
                                          buffer->data, 0, 0, buffer->width, buffer->height,
                                          buffer->stride, buffer->format->pixel_format) );
  return ctx;
}

Ctx *
ctx_new_for_framebuffer (void *data, int width, int height,
                         int stride,
                         CtxPixelFormat pixel_format)
{
  Ctx *ctx = ctx_new ();
  ctx_set_renderer (ctx,
                    ctx_rasterizer_init ( (CtxRasterizer *) malloc (sizeof (CtxRasterizer) ),
                                          ctx, &ctx->state, data, 0, 0, width, height,
                                          stride, pixel_format) );
  return ctx;
}

// ctx_new_for_stream (FILE *stream);

#if 0
CtxRasterizer *ctx_rasterizer_new (void *data, int x, int y, int width, int height,
                                   int stride, CtxPixelFormat pixel_format)
{
  CtxState    *state    = (CtxState *) malloc (sizeof (CtxState) );
  CtxRasterizer *rasterizer = (CtxRasterizer *) malloc (sizeof (CtxRenderer) );
  ctx_rasterizer_init (rasterizer, state, data, x, y, width, height,
                       stride, pixel_format);
}
#endif

Ctx *ctx_hasher_new (int width, int height, int cols, int rows)
{
  Ctx *ctx = ctx_new ();
  CtxState    *state    = &ctx->state;
  CtxRasterizer *rasterizer = (CtxRasterizer *) malloc (sizeof (CtxHasher) );
  ctx_hasher_init (rasterizer, ctx, state, width, height, rows, cols);
  ctx_set_renderer (ctx, (void*)rasterizer);
  return ctx;
}
uint64_t ctx_hash_get_hash (Ctx *ctx, int col, int row)
{
  CtxHasher *hasher = (CtxHasher*)ctx->renderer;
  if (row < 0) row =0;
  if (col < 0) col =0;
  if (row >= hasher->rows) row = hasher->rows-1;
  if (col >= hasher->cols) col = hasher->cols-1;

  return hasher->hashes[row*hasher->cols+col];
}

/* add an or-able value to pixelformat to indicate vflip+hflip
 */
void
ctx_blit (Ctx *ctx, void *data, int x, int y, int width, int height,
          int stride, CtxPixelFormat pixel_format)
{
  CtxRasterizer *rasterizer = (CtxRasterizer *) malloc (sizeof (CtxRasterizer) );
  CtxState *state = malloc (sizeof (CtxState));
  /* we borrow the state of ctx, this makes us non-thradable,
   * but saves memory, this should be made configurable at compile-time
   */
  ctx_rasterizer_init (rasterizer, ctx, state, data, x, y, width, height,
                       stride, pixel_format);
  {
    CtxIterator iterator;
    ctx_iterator_init (&iterator, &ctx->renderstream, 0, CTX_ITERATOR_EXPAND_BITPACK);
    for (CtxCommand *command = ctx_iterator_next (&iterator);
         command; command = ctx_iterator_next (&iterator) )
      { ctx_rasterizer_process (rasterizer, command); }
  }
  ctx_rasterizer_deinit (rasterizer);
  free (state);
}
#endif

void
ctx_current_point (Ctx *ctx, float *x, float *y)
{
  if (!ctx)
    { 
      if (x) { *x = 0.0f; }
      if (y) { *y = 0.0f; }
    }
#if CTX_RASTERIZER
  if (ctx->renderer)
    {
      if (x) { *x = ( (CtxRasterizer *) (ctx->renderer) )->x; }
      if (y) { *y = ( (CtxRasterizer *) (ctx->renderer) )->y; }
      return;
    }
#endif
  if (x) { *x = ctx->state.x; }
  if (y) { *y = ctx->state.y; }
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
#if CTX_CURRENT_PATH
  switch (entry->code)
    {
      case CTX_TEXT:
      case CTX_CLIP:
      case CTX_NEW_PATH:
      case CTX_FILL:
      case CTX_STROKE:
              // XXX unless preserve
        ctx->current_path.count = 0;
        break;
      case CTX_CLOSE_PATH:
      case CTX_LINE_TO:
      case CTX_MOVE_TO:
      case CTX_QUAD_TO:
      case CTX_SMOOTH_TO:
      case CTX_SMOOTHQ_TO:
      case CTX_REL_QUAD_TO:
      case CTX_REL_SMOOTH_TO:
      case CTX_REL_SMOOTHQ_TO:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_RECTANGLE:
        ctx_renderstream_add_entry (&ctx->current_path, entry);
        break;
      default:
        break;
    }
#endif
  if (ctx->renderer && ctx->renderer->process)
    {
      ctx->renderer->process (ctx->renderer, (CtxCommand *) entry);
    }
  else
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
          entry->code == CTX_SET ||
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
    { return -1; }
  fseek (file, 0, SEEK_END);
  size = remaining = ftell (file);
  if (length)
    { *length =size; }
  rewind (file);
  buffer = (char*)malloc (size + 8);
  if (!buffer)
    {
      fclose (file);
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
  *contents = (unsigned char*) buffer;
  buffer[size] = 0;
  return 0;
}
#endif


static inline int ctx_utf8_len (const unsigned char first_byte)
{
  if      ( (first_byte & 0x80) == 0)
    { return 1; } /* ASCII */
  else if ( (first_byte & 0xE0) == 0xC0)
    { return 2; }
  else if ( (first_byte & 0xF0) == 0xE0)
    { return 3; }
  else if ( (first_byte & 0xF8) == 0xF0)
    { return 4; }
  return 1;
}

static const char *ctx_utf8_skip (const char *s, int utf8_length)
{
  int count;
  if (!s)
    { return NULL; }
  for (count = 0; *s; s++)
    {
      if ( (*s & 0xC0) != 0x80)
        { count++; }
      if (count == utf8_length + 1)
        { return s; }
    }
  return s;
}

//  XXX  :  unused
static inline int ctx_utf8_strlen (const char *s)
{
  int count;
  if (!s)
    { return 0; }
  for (count = 0; *s; s++)
    if ( (*s & 0xC0) != 0x80)
      { count++; }
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
  if (ch < 0x80)
    {
      dest[0] = (char) ch;
      return 1;
    }
  if (ch < 0x800)
    {
      dest[0] = (ch>>6) | 0xC0;
      dest[1] = (ch & 0x3F) | 0x80;
      return 2;
    }
  if (ch < 0x10000)
    {
      dest[0] = (ch>>12) | 0xE0;
      dest[1] = ( (ch>>6) & 0x3F) | 0x80;
      dest[2] = (ch & 0x3F) | 0x80;
      return 3;
    }
  if (ch < 0x110000)
    {
      dest[0] = (ch>>18) | 0xF0;
      dest[1] = ( (ch>>12) & 0x3F) | 0x80;
      dest[2] = ( (ch>>6) & 0x3F) | 0x80;
      dest[3] = (ch & 0x3F) | 0x80;
      return 4;
    }
  return 0;
}

uint32_t
ctx_utf8_to_unichar (const char *input)
{
  const uint8_t *utf8 = (const uint8_t *) input;
  uint8_t c = utf8[0];
  if ( (c & 0x80) == 0)
    { return c; }
  else if ( (c & 0xE0) == 0xC0)
    return ( (utf8[0] & 0x1F) << 6) |
           (utf8[1] & 0x3F);
  else if ( (c & 0xF0) == 0xE0)
    return ( (utf8[0] & 0xF)  << 12) |
           ( (utf8[1] & 0x3F) << 6) |
           (utf8[2] & 0x3F);
  else if ( (c & 0xF8) == 0xF0)
    return ( (utf8[0] & 0x7)  << 18) |
           ( (utf8[1] & 0x3F) << 12) |
           ( (utf8[2] & 0x3F) << 6) |
           (utf8[3] & 0x3F);
  else if ( (c & 0xFC) == 0xF8)
    return ( (utf8[0] & 0x3)  << 24) |
           ( (utf8[1] & 0x3F) << 18) |
           ( (utf8[2] & 0x3F) << 12) |
           ( (utf8[3] & 0x3F) << 6) |
           (utf8[4] & 0x3F);
  else if ( (c & 0xFE) == 0xFC)
    return ( (utf8[0] & 0x1)  << 30) |
           ( (utf8[1] & 0x3F) << 24) |
           ( (utf8[2] & 0x3F) << 18) |
           ( (utf8[3] & 0x3F) << 12) |
           ( (utf8[4] & 0x3F) << 6) |
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

CtxFontEngine ctx_font_engine_stb =
{
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
    { return -1; }
  ctx_fonts[ctx_font_count].type = 1;
  ctx_fonts[ctx_font_count].name = (char *) malloc (strlen (name) + 1);
  ctx_strcpy ( (char *) ctx_fonts[ctx_font_count].name, name);
  if (!stbtt_InitFont (&ctx_fonts[ctx_font_count].stb.ttf_info, ttf_contents, 0) )
    {
      ctx_log ( "Font init failed\n");
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
      ctx_log ( "File load failed\n");
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
    { return 0.0f; }
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
    { return -1; }
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
    { ctx_fill (ctx); }
  return 0;
}
#endif

#if CTX_FONT_ENGINE_CTX

static float
ctx_glyph_kern_ctx (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  float font_size = ctx->state.gstate.font_size;
  if (font->ctx.first_kern == -1)
    { return 0.0; }
  for (int i = font->ctx.first_kern; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_KERNING_PAIR)
        {
          if (font->ctx.first_kern == 0) { font->ctx.first_kern = i; }
          if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
            { return entry->data.s32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE; }
        }
    }
  if (font->ctx.first_kern == 0)
    { font->ctx.first_kern = -1; }
  return 0;
}
#if 0
static int ctx_glyph_find (Ctx *ctx, CtxFont *font, uint32_t unichar)
{
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
        { return i; }
    }
  return 0;
}
#endif

static int ctx_font_find_glyph (CtxFont *font, uint32_t glyph)
{
  for (int i = 0; i < font->ctx.glyphs; i++)
    {
      if (font->ctx.index[i * 2] == glyph)
        { return font->ctx.index[i * 2 + 1]; }
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
    { return 0.0; }  // XXX : fallback
  for (int i = start; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        if (entry->data.u32[0] == (unsigned) unichar)
          { return (entry->data.u32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE); }
    }
  return 0.0;
}

static int
ctx_glyph_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxState *state = &ctx->state;
  CtxIterator iterator;
  CtxRenderstream  renderstream = { (CtxEntry *) font->ctx.data,
                                    font->ctx.length,
                                    font->ctx.length, 0, 0
                                  };
  float origin_x = state->x;
  float origin_y = state->y;
  ctx_current_point (ctx, &origin_x, &origin_y);
  int in_glyph = 0;
  float font_size = state->gstate.font_size;
  int start = 0;
  start = ctx_font_find_glyph (font, unichar);
  if (start < 0)
    { return -1; }  // XXX : fallback
  ctx_iterator_init (&iterator, &renderstream, start, CTX_ITERATOR_EXPAND_BITPACK);
  CtxCommand *command;
  while ( (command= ctx_iterator_next (&iterator) ) )
    {
      CtxEntry *entry = &command->entry;
      if (in_glyph)
        {
          if (entry->code == CTX_DEFINE_GLYPH)
            {
              if (stroke)
                { ctx_stroke (ctx); }
              else
                { ctx_fill (ctx); }
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
  // for the last glyph in a font
  if (stroke)
    { ctx_stroke (ctx); }
  else
    { ctx_fill (ctx); }
  ctx_restore (ctx);
  return -1;
}

uint32_t ctx_glyph_no (Ctx *ctx, int no)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  if (no < 0 || no >= font->ctx.glyphs)
    { return 0; }
  return font->ctx.index[no*2];
}

static void ctx_font_init_ctx (CtxFont *font)
{
  int glyph_count = 0;
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        { glyph_count ++; }
    }
  font->ctx.glyphs = glyph_count;
#if CTX_RENDERSTREAM_STATIC
  static uint32_t idx[512]; // one might have to adjust this for
  // larger fonts XXX
  // should probably be made a #define
  font->ctx.index = &idx[0];
#else
  font->ctx.index = (uint32_t *) malloc (sizeof (uint32_t) * 2 * glyph_count);
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

CtxFontEngine ctx_font_engine_ctx =
{
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
  if (length % sizeof (CtxEntry) )
    { return -1; }
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }
  ctx_fonts[ctx_font_count].type = 0;
  ctx_fonts[ctx_font_count].name = name;
  ctx_fonts[ctx_font_count].ctx.data = (CtxEntry *) data;
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
      ctx_log ( "File load failed\n");
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
  ctx_memset (commands, 0, sizeof (commands) );
  commands[0] = ctx_u32 (CTX_GLYPH, unichar, 0);
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
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1) )
    {
      sum += ctx_glyph_width (ctx, ctx_utf8_to_unichar (utf8) );
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
  switch ( (int) ctx_state_get (state, CTX_text_align) )
    //switch (state->gstate.text_align)
    {
      case CTX_TEXT_ALIGN_START:
      case CTX_TEXT_ALIGN_LEFT:
        break;
      case CTX_TEXT_ALIGN_CENTER:
        x -= ctx_text_width (ctx, string) /2;
        break;
      case CTX_TEXT_ALIGN_END:
      case CTX_TEXT_ALIGN_RIGHT:
        x -= ctx_text_width (ctx, string);
        break;
    }
  float y = ctx->state.y;
  float baseline_offset = 0.0f;
  switch ( (int) ctx_state_get (state, CTX_text_baseline) )
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
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1) )
    {
      if (*utf8 == '\n')
        {
          y += ctx->state.gstate.font_size * ctx_state_get (state, CTX_line_spacing);
          x = x0;
          if (visible)
            { ctx_move_to (ctx, x, y); }
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
              x += ctx_glyph_kern (ctx, unichar, ctx_utf8_to_unichar (next_utf8) );
            }
          if (visible)
            { ctx_move_to (ctx, x, y); }
        }
    }
  if (!visible)
    { ctx_move_to (ctx, x, y); }
}


CtxGlyph *
ctx_glyph_allocate (int n_glyphs)
{
  return (CtxGlyph *) malloc (sizeof (CtxGlyph) * n_glyphs);
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
  ctx_process_cmd_str (ctx, CTX_TEXT, string, 0, 0);
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
  ctx_process_cmd_str (ctx, CTX_TEXT_STROKE, string, 0, 0);
  _ctx_text (ctx, string, 1, 0);
#else
  _ctx_text (ctx, string, 1, 1);
#endif
}

static void ctx_setup ()
{
  static int initialized = 0;
  if (initialized) { return; }
  initialized = 1;
#if CTX_FONT_ENGINE_CTX
  ctx_font_count = 0; // oddly - this is needed in arduino
#if CTX_FONT_regular
  ctx_load_font_ctx ("sans-ctx", ctx_font_regular, sizeof (ctx_font_regular) );
#endif
#if CTX_FONT_mono
  ctx_load_font_ctx ("mono-ctx", ctx_font_mono, sizeof (ctx_font_mono) );
#endif
#if CTX_FONT_bold
  ctx_load_font_ctx ("bold-ctx", ctx_font_bold, sizeof (ctx_font_bold) );
#endif
#if CTX_FONT_italic
  ctx_load_font_ctx ("italic-ctx", ctx_font_italic, sizeof (ctx_font_italic) );
#endif
#if CTX_FONT_sans
  ctx_load_font_ctx ("sans-ctx", ctx_font_sans, sizeof (ctx_font_sans) );
#endif
#if CTX_FONT_serif
  ctx_load_font_ctx ("serif-ctx", ctx_font_serif, sizeof (ctx_font_serif) );
#endif
#if CTX_FONT_symbol
  ctx_load_font_ctx ("symbol-ctx", ctx_font_symbol, sizeof (ctx_font_symbol) );
#endif
#if CTX_FONT_emoji
  ctx_load_font_ctx ("emoji-ctx", ctx_font_emoji, sizeof (ctx_font_emoji) );
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

typedef struct _CtxCairo CtxCairo;
struct
  _CtxCairo
{
  CtxImplementation vfuncs;
  Ctx *ctx;
  cairo_t *cr;
  cairo_pattern_t *pat;
  cairo_surface_t *image;
  int preserve;

};

static void
ctx_cairo_process (CtxCairo *ctx_cairo, CtxCommand *c)
{
  CtxEntry *entry = (CtxEntry *) &c->entry;
  cairo_t *cr = ctx_cairo->cr;
  switch (entry->code)
    {
      case CTX_LINE_TO:
        cairo_line_to (cr, c->line_to.x, c->line_to.y);
        break;
      case CTX_REL_LINE_TO:
        cairo_rel_line_to (cr, c->rel_line_to.x, c->rel_line_to.y);
        break;
      case CTX_MOVE_TO:
        cairo_move_to (cr, c->move_to.x, c->move_to.y);
        break;
      case CTX_REL_MOVE_TO:
        cairo_rel_move_to (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_CURVE_TO:
        cairo_curve_to (cr, ctx_arg_float (0), ctx_arg_float (1),
                        ctx_arg_float (2), ctx_arg_float (3),
                        ctx_arg_float (4), ctx_arg_float (5) );
        break;
      case CTX_REL_CURVE_TO:
        cairo_rel_curve_to (cr,ctx_arg_float (0), ctx_arg_float (1),
                            ctx_arg_float (2), ctx_arg_float (3),
                            ctx_arg_float (4), ctx_arg_float (5) );
        break;
      case CTX_PRESERVE:
        ctx_cairo->preserve = 1;
        break;
      case CTX_QUAD_TO:
        {
          double x0, y0;
          cairo_get_current_point (cr, &x0, &y0);
          float cx = ctx_arg_float (0);
          float cy = ctx_arg_float (1);
          float  x = ctx_arg_float (2);
          float  y = ctx_arg_float (3);
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
          float cx = ctx_arg_float (0) + x0;
          float cy = ctx_arg_float (1) + y0;
          float  x = ctx_arg_float (2) + x0;
          float  y = ctx_arg_float (3) + y0;
          cairo_curve_to (cr,
                          (cx * 2 + x0) / 3.0f, (cy * 2 + y0) / 3.0f,
                          (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                          x,                              y);
        }
        break;
      /* rotate/scale/translate does not occur in fully minified data stream */
      case CTX_ROTATE:
        cairo_rotate (cr, ctx_arg_float (0) );
        break;
      case CTX_SCALE:
        cairo_scale (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_TRANSLATE:
        cairo_translate (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_SET_LINE_WIDTH:
        cairo_set_line_width (cr, ctx_arg_float (0) );
        break;
      case CTX_ARC:
        if (ctx_arg_float (5) == 0)
          cairo_arc (cr, ctx_arg_float (0), ctx_arg_float (1),
                     ctx_arg_float (2), ctx_arg_float (3),
                     ctx_arg_float (4) );
        else
          cairo_arc_negative (cr, ctx_arg_float (0), ctx_arg_float (1),
                              ctx_arg_float (2), ctx_arg_float (3),
                              ctx_arg_float (4) );
        break;
      case CTX_SET_RGBA_U8:
        cairo_set_source_rgba (cr, ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ),
                               ctx_u8_to_float (ctx_arg_u8 (3) ) );
        break;
#if 0
      case CTX_SET_RGBA_STROKE: // XXX : we need to maintain
        //       state for the two kinds
        cairo_set_source_rgba (cr, ctx_arg_u8 (0) /255.0,
                               ctx_arg_u8 (1) /255.0,
                               ctx_arg_u8 (2) /255.0,
                               ctx_arg_u8 (3) /255.0);
        break;
#endif
      case CTX_RECTANGLE:
        cairo_rectangle (cr, c->rectangle.x, c->rectangle.y,
                         c->rectangle.width, c->rectangle.height);
        break;
      case CTX_SET_PIXEL:
        cairo_set_source_rgba (cr, ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ),
                               ctx_u8_to_float (ctx_arg_u8 (3) ) );
        cairo_rectangle (cr, ctx_arg_u16 (2), ctx_arg_u16 (3), 1, 1);
        cairo_fill (cr);
        break;
      case CTX_FILL:
        if (ctx_cairo->preserve)
        {
          cairo_fill_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_fill (cr);
        }
        break;
      case CTX_STROKE:
        if (ctx_cairo->preserve)
        {
          cairo_stroke_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_stroke (cr);
        }
        break;
      case CTX_IDENTITY:
        cairo_identity_matrix (cr);
        break;
      case CTX_CLIP:
        if (ctx_cairo->preserve)
        {
          cairo_clip_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_clip (cr);
        }
        break;
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
        cairo_set_font_size (cr, ctx_arg_float (0) );
        break;
      case CTX_SET_MITER_LIMIT:
        cairo_set_miter_limit (cr, ctx_arg_float (0) );
        break;
      case CTX_SET_LINE_CAP:
        {
          int cairo_val = CAIRO_LINE_CAP_SQUARE;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_CAP_ROUND:
                cairo_val = CAIRO_LINE_CAP_ROUND;
                break;
              case CTX_CAP_SQUARE:
                cairo_val = CAIRO_LINE_CAP_SQUARE;
                break;
              case CTX_CAP_NONE:
                cairo_val = CAIRO_LINE_CAP_BUTT;
                break;
            }
          cairo_set_line_cap (cr, cairo_val);
        }
        break;
      case CTX_SET_BLEND_MODE:
        {
          // XXX does not map to cairo
        }
        break;
      case CTX_SET_COMPOSITING_MODE:
        {
          int cairo_val = CAIRO_OPERATOR_OVER;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_COMPOSITE_SOURCE_OVER:
                cairo_val = CAIRO_OPERATOR_OVER;
                break;
              case CTX_COMPOSITE_COPY:
                cairo_val = CAIRO_OPERATOR_SOURCE;
                break;
            }
          cairo_set_operator (cr, cairo_val);
        }
      case CTX_SET_LINE_JOIN:
        {
          int cairo_val = CAIRO_LINE_JOIN_ROUND;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_JOIN_ROUND:
                cairo_val = CAIRO_LINE_JOIN_ROUND;
                break;
              case CTX_JOIN_BEVEL:
                cairo_val = CAIRO_LINE_JOIN_BEVEL;
                break;
              case CTX_JOIN_MITER:
                cairo_val = CAIRO_LINE_JOIN_MITER;
                break;
            }
          cairo_set_line_join (cr, cairo_val);
        }
        break;
      case CTX_LINEAR_GRADIENT:
        {
          if (ctx_cairo->pat)
            {
              cairo_pattern_destroy (ctx_cairo->pat);
              ctx_cairo->pat = NULL;
            }
          ctx_cairo->pat = cairo_pattern_create_linear (ctx_arg_float (0), ctx_arg_float (1),
                           ctx_arg_float (2), ctx_arg_float (3) );
          cairo_pattern_add_color_stop_rgba (ctx_cairo->pat, 0, 0, 0, 0, 1);
          cairo_pattern_add_color_stop_rgba (ctx_cairo->pat, 1, 1, 1, 1, 1);
          cairo_set_source (cr, ctx_cairo->pat);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          if (ctx_cairo->pat)
            {
              cairo_pattern_destroy (ctx_cairo->pat);
              ctx_cairo->pat = NULL;
            }
          ctx_cairo->pat = cairo_pattern_create_radial (ctx_arg_float (0), ctx_arg_float (1),
                           ctx_arg_float (2), ctx_arg_float (3),
                           ctx_arg_float (4), ctx_arg_float (5) );
          cairo_set_source (cr, ctx_cairo->pat);
        }
        break;
      case CTX_GRADIENT_STOP:
        cairo_pattern_add_color_stop_rgba (ctx_cairo->pat,
                                           ctx_arg_float (0),
                                           ctx_u8_to_float (ctx_arg_u8 (4) ),
                                           ctx_u8_to_float (ctx_arg_u8 (5) ),
                                           ctx_u8_to_float (ctx_arg_u8 (6) ),
                                           ctx_u8_to_float (ctx_arg_u8 (7) ) );
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
          image = cairo_image_surface_create_from_png (ctx_arg_string() );
          cairo_set_source_surface (cr, image, ctx_arg_float (0), ctx_arg_float (1) );
        }
        break;
#endif
      case CTX_TEXT:
        /* XXX: implement some linebreaking/wrap behavior here */
        cairo_show_text (cr, ctx_arg_string () );
        break;
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
      case CTX_REPEAT_HISTORY:
        break;
    }
  ctx_process (ctx_cairo->ctx, entry);
}

void ctx_cairo_free (CtxCairo *ctx_cairo)
{
  if (ctx_cairo->pat)
    { cairo_pattern_destroy (ctx_cairo->pat); }
  if (ctx_cairo->image)
    { cairo_surface_destroy (ctx_cairo->image); }
  free (ctx_cairo);
}

void
ctx_render_cairo (Ctx *ctx, cairo_t *cr)
{
  CtxIterator iterator;
  CtxCommand *command;
  CtxCairo    ctx_cairo = {{(void*)ctx_cairo_process, NULL, NULL}, ctx, cr, NULL, NULL};
  ctx_iterator_init (&iterator, &ctx->renderstream, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_cairo_process (&ctx_cairo, command); }
}

Ctx *
ctx_new_for_cairo (cairo_t *cr)
{
  Ctx *ctx = ctx_new ();
  CtxCairo *ctx_cairo = calloc(sizeof(CtxCairo),1);
  ctx_cairo->vfuncs.free = (void*)ctx_cairo_free;
  ctx_cairo->vfuncs.process = (void*)ctx_cairo_process;
  ctx_cairo->ctx = ctx;
  ctx_cairo->cr = cr;

  ctx_set_renderer (ctx, (void*)ctx_cairo);
  return ctx;
}

#endif

void
ctx_render_ctx (Ctx *ctx, Ctx *d_ctx)
{
  CtxIterator iterator;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->renderstream, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_process (d_ctx, &command->entry); }
}

#if CTX_FORMATTER

typedef enum CtxFormatter
{
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
    { fwrite ("  ", 1, 2, stream); }
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
          case CTX_SET_KEY:              name="set_param"; break;
          case CTX_SET_COLOR:            name="set_color"; break;
          case CTX_DEFINE_GLYPH:         name="define_glyph"; break;
          case CTX_SET_PIXEL:            name="set_pixel"; break;
          case CTX_SET_GLOBAL_ALPHA:     name="set_global_alpha"; break;
          case CTX_TEXT:                 name="text"; break;
          case CTX_TEXT_STROKE:          name="text_stroke"; break;
          case CTX_SAVE:                 name="save"; break;
          case CTX_RESTORE:              name="restore"; break;
          case CTX_RECTANGLE:            name="rectangle"; break;
          case CTX_LINEAR_GRADIENT:      name="linear_gradient"; break;
          case CTX_RADIAL_GRADIENT:      name="radial_gradient"; break;
          case CTX_GRADIENT_STOP:        name="gradient_add_stop"; break;
          case CTX_MEDIA_BOX:            name="media_box"; break;
          case CTX_MOVE_TO:              name="move_to"; break;
          case CTX_LINE_TO:              name="line_to"; break;
          case CTX_NEW_PATH:             name="new_path"; break;
          case CTX_REL_MOVE_TO:          name="rel_move_to"; break;
          case CTX_REL_LINE_TO:          name="rel_line_to"; break;
          case CTX_FILL:                 name="fill"; break;
          case CTX_EXIT:                 name="exit"; break;
          case CTX_APPLY_TRANSFORM:      name="set_transform"; break;
          case CTX_REL_ARC_TO:           name="rel_arc_to"; break;
          case CTX_GLYPH:                name="glyph"; break;
          case CTX_TEXTURE:              name="texture"; break;
          case CTX_IDENTITY:             name="identity"; break;
          case CTX_CLOSE_PATH:           name="close_path"; break;
          case CTX_PRESERVE:             name="preserve"; break;
          case CTX_FLUSH:                name="flush"; break;
          case CTX_RESET:                name="reset"; break;
          case CTX_SET_FONT:             name="set_font"; break;
          case CTX_STROKE:               name="stroke"; break;
          case CTX_CLIP:                 name="clip"; break;
          case CTX_ARC:                  name="arc"; break;
          case CTX_SCALE:                name="scale"; break;
          case CTX_TRANSLATE:            name="translate"; break;
          case CTX_ROTATE:               name="rotate"; break;
          case CTX_ARC_TO:               name="arc_to"; break;
          case CTX_CURVE_TO:             name="curve_to"; break;
          case CTX_REL_CURVE_TO:         name="rel_curve_to"; break;
          case CTX_REL_QUAD_TO:          name="rel_quad_to"; break;
          case CTX_QUAD_TO:              name="quad_to"; break;
          case CTX_SMOOTH_TO:            name="smooth_to"; break;
          case CTX_REL_SMOOTH_TO:        name="rel_smooth_to"; break;
          case CTX_SMOOTHQ_TO:           name="smoothq_to"; break;
          case CTX_REL_SMOOTHQ_TO:       name="rel_smoothq_to"; break;
          case CTX_NEW_PAGE:             name="new_page"; break;
          case CTX_HOR_LINE_TO:          name="hor_line_to"; break;
          case CTX_VER_LINE_TO:          name="ver_line_to"; break;
          case CTX_REL_HOR_LINE_TO:      name="rel_hor_line_to"; break;
          case CTX_REL_VER_LINE_TO:      name="rel_ver_line_to"; break;
          case CTX_SET_COMPOSITING_MODE: name="set_compositing_mode"; break;
          case CTX_SET_BLEND_MODE:       name="set_blend_mode"; break;
          case CTX_SET_TEXT_ALIGN:       name="set_text_align"; break;
          case CTX_SET_TEXT_BASELINE:    name="set_text_baseline"; break;
          case CTX_SET_TEXT_DIRECTION:   name="set_text_direction"; break;
          case CTX_SET_FONT_SIZE:        name="set_font_size"; break;
          case CTX_SET_MITER_LIMIT:      name="set_miter_limit"; break;
          case CTX_SET_LINE_JOIN:        name="set_line_join"; break;
          case CTX_SET_LINE_CAP:         name="set_line_cap"; break;
          case CTX_SET_LINE_WIDTH:       name="set_line_width"; break;
          case CTX_SET_FILL_RULE:        name="set_fill_rule"; break;
          case CTX_SET:                  name="setprop"; break;
        }
      if (name)
        {
          fwrite (name, 1, strlen ( (char *) name), stream);
          fwrite (" (", 1, 2, stream);
          if (code == CTX_SAVE)
            { (*indent)++; }
          else if (code == CTX_RESTORE)
            { (*indent)--; }
          return;
        }
    }
#endif
  {
    uint8_t name[3];
    name[0]=CTX_SET_KEY;
    name[2]='\0';
    switch (code)
      {
        case CTX_SET_GLOBAL_ALPHA:
          name[1]='a';
          break;
        case CTX_SET_COMPOSITING_MODE:
          name[1]='m';
          break;
        case CTX_SET_BLEND_MODE:
          name[1]='b';
          break;
        case CTX_SET_TEXT_ALIGN:
          name[1]='t';
          break;
        case CTX_SET_TEXT_BASELINE:
          name[1]='b';
          break;
        case CTX_SET_TEXT_DIRECTION:
          name[1]='d';
          break;
        case CTX_SET_FONT_SIZE:
          name[1]='f';
          break;
        case CTX_SET_MITER_LIMIT:
          name[1]='l';
          break;
        case CTX_SET_LINE_JOIN:
          name[1]='j';
          break;
        case CTX_SET_LINE_CAP:
          name[1]='c';
          break;
        case CTX_SET_LINE_WIDTH:
          name[1]='w';
          break;
        case CTX_SET_FILL_RULE:
          name[1]='r';
          break;
        default:
          name[0] = code;
          name[1] = 0;
          break;
      }
    fwrite (name, 1, strlen ( (char *) name), stream);
    fwrite (" ", 1, 1, stream);
    if (formatter)
      { fwrite ("(", 1, 1, stream); }
  }
}

static void
ctx_print_entry_enum (FILE *stream, int formatter, int *indent, CtxEntry *entry, int args)
{
  _ctx_print_name (stream, entry->code, formatter, indent);
  for (int i = 0; i <  args; i ++)
    {
      int val = ctx_arg_u8 (i);
      if (i>0)
        { fwrite (" ", 1, 1, stream); }
#if CTX_VERBOSE_NAMES
      if (formatter)
        {
          const char *str = NULL;
          switch (entry->code)
            {
              case CTX_SET_TEXT_BASELINE:
                switch (val)
                  {
                    case CTX_TEXT_BASELINE_ALPHABETIC:
                      str = "alphabetic";
                      break;
                    case CTX_TEXT_BASELINE_TOP:
                      str = "top";
                      break;
                    case CTX_TEXT_BASELINE_BOTTOM:
                      str = "bottom";
                      break;
                    case CTX_TEXT_BASELINE_HANGING:
                      str = "hanging";
                      break;
                    case CTX_TEXT_BASELINE_MIDDLE:
                      str = "middle";
                      break;
                    case CTX_TEXT_BASELINE_IDEOGRAPHIC:
                      str = "ideographic";
                      break;
                  }
                break;
              case CTX_SET_TEXT_ALIGN:
                switch (val)
                  {
                    case CTX_TEXT_ALIGN_LEFT:
                      str = "left";
                      break;
                    case CTX_TEXT_ALIGN_RIGHT:
                      str = "right";
                      break;
                    case CTX_TEXT_ALIGN_START:
                      str = "start";
                      break;
                    case CTX_TEXT_ALIGN_END:
                      str = "end";
                      break;
                    case CTX_TEXT_ALIGN_CENTER:
                      str = "center";
                      break;
                  }
                break;
              case CTX_SET_LINE_CAP:
                switch (val)
                  {
                    case CTX_CAP_NONE:
                      str = "none";
                      break;
                    case CTX_CAP_ROUND:
                      str = "round";
                      break;
                    case CTX_CAP_SQUARE:
                      str = "square";
                      break;
                  }
                break;
              case CTX_SET_LINE_JOIN:
                switch (val)
                  {
                    case CTX_JOIN_MITER:
                      str = "miter";
                      break;
                    case CTX_JOIN_ROUND:
                      str = "round";
                      break;
                    case CTX_JOIN_BEVEL:
                      str = "bevel";
                      break;
                  }
                break;
              case CTX_SET_FILL_RULE:
                switch (val)
                  {
                    case CTX_FILL_RULE_WINDING:
                      str = "winding";
                      break;
                    case CTX_FILL_RULE_EVEN_ODD:
                      str = "evenodd";
                      break;
                  }
                break;
              case CTX_SET_BLEND_MODE:
                switch (val)
                  {
            case CTX_BLEND_NORMAL: str = "normal"; break;
            case CTX_BLEND_MULTIPLY: str= "multiply"; break;
            case CTX_BLEND_SCREEN: str = "screen"; break;
            case CTX_BLEND_OVERLAY: str = "overlay"; break;
            case CTX_BLEND_DARKEN: str = "darken"; break;
            case CTX_BLEND_LIGHTEN: str = "lighten"; break;
            case CTX_BLEND_COLOR_DODGE: str = "colorDodge"; break;
            case CTX_BLEND_COLOR_BURN: str = "colorBurn"; break;
            case CTX_BLEND_HARD_LIGHT: str = "hardLight"; break;
            case CTX_BLEND_SOFT_LIGHT: str = "softLight"; break;
            case CTX_BLEND_DIFFERENCE: str = "difference"; break;
            case CTX_BLEND_EXCLUSION: str = "exclusion"; break;
            case CTX_BLEND_HUE: str = "hue"; break;
            case CTX_BLEND_SATURATION:  str = "saturation"; break;
            case CTX_BLEND_COLOR: str = "color"; break; 
            case CTX_BLEND_LUMINOSITY: str = "luminosity"; break;
                  }
                break;
              case CTX_SET_COMPOSITING_MODE:
                switch (val)
                  {
              case CTX_COMPOSITE_SOURCE_OVER: str = "sourceOver"; break;
              case CTX_COMPOSITE_COPY: str = "copy"; break;
              case CTX_COMPOSITE_CLEAR: str = "clear"; break;
              case CTX_COMPOSITE_SOURCE_IN: str = "sourceIn"; break;
              case CTX_COMPOSITE_SOURCE_OUT: str = "sourceOut"; break;
              case CTX_COMPOSITE_SOURCE_ATOP: str = "sourceAtop"; break;
              case CTX_COMPOSITE_DESTINATION: str = "destination"; break;
              case CTX_COMPOSITE_DESTINATION_OVER: str = "destinationOver"; break;
              case CTX_COMPOSITE_DESTINATION_IN: str = "destinationIn"; break;
              case CTX_COMPOSITE_DESTINATION_OUT: str = "destinationOut"; break;
              case CTX_COMPOSITE_DESTINATION_ATOP: str = "destinationAtop"; break;
              case CTX_COMPOSITE_XOR: str = "xor"; break;
                  }

               break;
            }
          if (str)
            { fprintf (stream, "%s", str); }
          else
            { fprintf (stream, "%i", val); }
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
  if (!string | !stream) { return; }
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
  sprintf (temp, "%0.3f", val);
  int j;
  for (j = 0; temp[j]; j++)
    if (j == ',') { temp[j] = '.'; }
  j--;
  if (j>0)
    while (temp[j] == '0')
      {
        temp[j]=0;
        j--;
      }
  if (temp[j]=='.')
    { temp[j]='\0'; }
  fwrite (temp, 1, strlen (temp), stream);
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
                  { fwrite (" ", 1, 1, stream); }
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

static void
ctx_stream_process (void *user_data, CtxCommand *c)
{
  CtxEntry *entry = &c->entry;
  void **user_data_array = (void **) user_data;
  FILE *stream    = (FILE *) user_data_array[0];
  int   formatter = (size_t) (user_data_array[1]);
  int  *indent    = (int *) &user_data_array[2];
    switch (entry->code)
  //switch ((CtxCode)(entry->code))
    {
      case CTX_LINE_TO:
      case CTX_REL_LINE_TO:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_MOVE_TO:
      case CTX_REL_MOVE_TO:
        ctx_print_entry (stream, formatter, indent, entry, 2);
        break;
      case CTX_ARC_TO:
        ctx_print_entry (stream, formatter, indent, entry, 5);
        break;
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_RADIAL_GRADIENT:
      case CTX_APPLY_TRANSFORM:
        ctx_print_entry (stream, formatter, indent, entry, 6);
        break;
      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_REL_QUAD_TO:
      case CTX_LINEAR_GRADIENT:
      case CTX_MEDIA_BOX:
        ctx_print_entry (stream, formatter, indent, entry, 4);
        break;
      case CTX_SET_FONT_SIZE:
      case CTX_SET_MITER_LIMIT:
      case CTX_ROTATE:
      case CTX_SET_LINE_WIDTH:
      case CTX_VER_LINE_TO:
      case CTX_HOR_LINE_TO:
        ctx_print_entry (stream, formatter, indent, entry, 1);
        break;
      case CTX_SET:
        _ctx_print_name (stream, entry->code, formatter, indent);
        switch (c->set.key_hash)
        {
           case CTX_x: fprintf (stream, " 'x' "); break;
           case CTX_y: fprintf (stream, " 'y' "); break;
           case CTX_width: fprintf (stream, " width "); break;
           case CTX_height: fprintf (stream, " height "); break;
           default:
             fprintf (stream, " %d ", c->set.key_hash);
        }
        fprintf (stream, "\"");
        ctx_print_escaped_string (stream, (char*)c->set.utf8);
        fprintf (stream, "\"");
        _ctx_print_endcmd (stream, formatter);
        break;
      case CTX_SET_COLOR:
        if (formatter ||  1)
          {
            _ctx_indent (stream, *indent);
            switch ( (int) c->set_color.model)
              {
                case CTX_GRAY:
                  fprintf (stream, "gray ");
                  ctx_print_float (stream, c->graya.g);
                  break;
                case CTX_GRAYA:
                  fprintf (stream, "graya ");
                  ctx_print_float (stream, c->graya.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->graya.a);
                  break;
                case CTX_RGBA:
                  if (c->rgba.a != 1.0)
                  {
                    fprintf (stream, "rgba ");
                    ctx_print_float (stream, c->rgba.r);
                    fprintf (stream, " ");
                    ctx_print_float (stream, c->rgba.g);
                    fprintf (stream, " ");
                    ctx_print_float (stream, c->rgba.b);
                    fprintf (stream, " ");
                    ctx_print_float (stream, c->rgba.a);
                    break;
                  }
                case CTX_RGB:
                  if (c->rgba.r == c->rgba.g && c->rgba.g == c->rgba.b)
                  {
                    fprintf (stream, "gray ");
                    ctx_print_float (stream, c->rgba.r);
                    fprintf (stream, " ");
                    break;
                  }
                  fprintf (stream, "rgb ");
                  ctx_print_float (stream, c->rgba.r);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.b);
                  break;
                case CTX_DRGB:
                  fprintf (stream, "drgb ");
                  ctx_print_float (stream, c->rgba.r);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.b);
                  break;
                case CTX_DRGBA:
                  fprintf (stream, "drgba ");
                  ctx_print_float (stream, c->rgba.r);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.b);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.a);
                  break;
                case CTX_CMYK:
                  fprintf (stream, "cmyk ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  break;
                case CTX_CMYKA:
                  fprintf (stream, "cmyka ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.a);
                  break;
                case CTX_DCMYK:
                  fprintf (stream, "dcmyk ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  break;
                case CTX_DCMYKA:
                  fprintf (stream, "dcmyka ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.a);
                  break;
              }
          }
        else
          {
            ctx_print_entry (stream, formatter, indent, entry, 1);
          }
        break;
      case CTX_SET_RGBA_U8:
        if (formatter)
          {
            _ctx_indent (stream, *indent);
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
                  { fwrite (", ", 2, 1, stream); }
                else
                  { fwrite (" ", 1, 1, stream); }
              }
            ctx_print_float (stream, ctx_u8_to_float (ctx_arg_u8 (c) ) );
          }
        _ctx_print_endcmd (stream, formatter);
        break;
      case CTX_SET_PIXEL:
#if 0
        ctx_set_pixel_u8 (d_ctx,
                          ctx_arg_u16 (2), ctx_arg_u16 (3),
                          ctx_arg_u8 (0),
                          ctx_arg_u8 (1),
                          ctx_arg_u8 (2),
                          ctx_arg_u8 (3) );
#endif
        break;
      case CTX_FILL:
      case CTX_RESET:
      case CTX_STROKE:
      case CTX_IDENTITY:
      case CTX_CLIP:
      case CTX_NEW_PATH:
      case CTX_CLOSE_PATH:
      case CTX_SAVE:
      case CTX_PRESERVE:
      case CTX_RESTORE:
        ctx_print_entry (stream, formatter, indent, entry, 0);
        break;
      case CTX_SET_TEXT_ALIGN:
      case CTX_SET_TEXT_BASELINE:
      case CTX_SET_TEXT_DIRECTION:
      case CTX_SET_FILL_RULE:
      case CTX_SET_LINE_CAP:
      case CTX_SET_LINE_JOIN:
      case CTX_SET_COMPOSITING_MODE:
        ctx_print_entry_enum (stream, formatter, indent, entry, 1);
        break;
      case CTX_GRADIENT_STOP:
        _ctx_print_name (stream, entry->code, formatter, indent);
        for (int c = 0; c < 4; c++)
          {
            if (c)
              { fwrite ("  ", 1, 1, stream); }
            ctx_print_float (stream, ctx_u8_to_float (ctx_arg_u8 (4+c) ) );
          }
        _ctx_print_endcmd (stream, formatter);
        break;
      case CTX_TEXT:
      case CTX_TEXT_STROKE:
      case CTX_SET_FONT:
        _ctx_print_name (stream, entry->code, formatter, indent);
        fprintf (stream, "\"");
        ctx_print_escaped_string (stream, ctx_arg_string() );
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

void
ctx_render_stream (Ctx *ctx, FILE *stream, int formatter)
{
  CtxIterator iterator;
  void *user_data[3]= {stream, (void *) (size_t) (formatter), NULL};
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->renderstream, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_stream_process (user_data, command); }
  fprintf (stream, "\n");
}

#endif

/* the parser comes in the end, nothing in ctx knows about the parser  */

#if CTX_PARSER

/* ctx parser, */

struct
  _CtxParser
{
  int        t_args; // total number of arguments seen for current command
  Ctx       *ctx;
  int        state;
  uint8_t    holding[CTX_PARSER_MAXLEN]; /*  */
  int        line; /*  for error reporting */
  int        col;  /*  for error reporting */
  int        pos;
  double     numbers[12];
  int        n_numbers;
  int        decimal;
  CtxCode    command;
  int        n_args;
  uint32_t   set_key_hash;
  float      pcx;
  float      pcy;
  int        color_components;
  int        color_model; // 1 gray 3 rgb 4 cmyk
  float      left_margin; // set by last user provided move_to
  int        width;       // <- maybe should be float
  int        height;
  float      cell_width;
  float      cell_height;
  int        cursor_x;    // <- leaking in from terminal
  int        cursor_y;

  void (*exit) (void *exit_data);
  void *exit_data;
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len);
  int   (*get_prop)(void *prop_data, const char *key, char **data, int *len);
  void *prop_data;
};

void
ctx_parser_set_size (CtxParser *parser,
                 int        width,
                 int        height,
                 float      cell_width,
                 float      cell_height)
{
  if (cell_width > 0)
    parser->cell_width       = cell_width;
  if (cell_height > 0)
    parser->cell_height      = cell_height;
  if (width > 0)
    parser->width            = width;
  if (height > 0)
    parser->height           = height;
}

static CtxParser *
ctx_parser_init (CtxParser *parser,
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
                 void *exit_data
                )
{
  ctx_memset (parser, 0, sizeof (CtxParser) );
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
  parser->command          = CTX_MOVE_TO;
  parser->set_prop         = set_prop;
  parser->get_prop         = get_prop;
  parser->prop_data        = prop_data;
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
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
  void  *prop_data,
  void (*exit) (void *exit_data),
  void *exit_data)
{
  return ctx_parser_init ( (CtxParser *) malloc (sizeof (CtxParser) ),
                           ctx,
                           width, height,
                           cell_width, cell_height,
                           cursor_x, cursor_y, set_prop, get_prop, prop_data,
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
      case CTX_RESET:
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
      case CTX_SET_RGB_SPACE:
      case CTX_SET_CMYK_SPACE:
      case CTX_SET_DCMYK_SPACE:
      case CTX_SET_DRGB_SPACE:
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
      case CTX_ROUND_RECTANGLE:
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
      case CTX_APPLY_TRANSFORM:
      case CTX_RADIAL_GRADIENT:
        return 6;
      case CTX_TEXT_STROKE:
      case CTX_TEXT: // special case
      case CTX_SET:
      case CTX_GET:
        return 100; /* 100 is a special value,
                   which means string|number accepted
                 */
      //case CTX_SET_KEY:
      case CTX_SET_COLOR:
        return 200;  /* 200 means number of components */
      case CTX_GRADIENT_STOP:
        return 201;  /* 201 means number of components+1 */
      case CTX_FUNCTION: /* special interpretation   */
        return 300;
        default:
#if 1
      case CTX_TEXTURE:
        case CTX_SET_RGBA_U8:
        case CTX_BITPIX:
        case CTX_BITPIX_DATA:
        case CTX_NOP:
        case CTX_NEW_EDGE:
        case CTX_EDGE:
        case CTX_EDGE_FLIPPED:
        case CTX_REPEAT_HISTORY:
        case CTX_CONT:
        case CTX_DATA:
        case CTX_DATA_REV:
        case CTX_DEFINE_GLYPH:
        case CTX_KERNING_PAIR:
        case CTX_SET_PIXEL:
        case CTX_REL_LINE_TO_X4:
        case CTX_REL_LINE_TO_REL_CURVE_TO:
        case CTX_REL_CURVE_TO_REL_LINE_TO:
        case CTX_REL_CURVE_TO_REL_MOVE_TO:
        case CTX_REL_LINE_TO_X2:
        case CTX_MOVE_TO_REL_LINE_TO:
        case CTX_REL_LINE_TO_REL_MOVE_TO:
        case CTX_FILL_MOVE_TO:
        case CTX_REL_QUAD_TO_REL_QUAD_TO:
        case CTX_REL_QUAD_TO_S16:
#endif
        return 0;
    }
}

static int ctx_parser_set_command (CtxParser *parser, CtxCode code)
{
        //fprintf (stderr, "%i %s\n", code, parser->holding);
  if (code < 127 && code > 16)
  {
  parser->n_args = ctx_arguments_for_code (code);
  if (parser->n_args >= 200)
    {
      parser->n_args = (parser->n_args % 100) + parser->color_components;
    }
  //parser->t_args = 0;
#if 0
  else if (parser->n_args >= 100)
    {
      parser->n_args = (parser->n_args % 100);
    }
#endif
  }
  return code;
}

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model);

static int ctx_parser_resolve_command (CtxParser *parser, const uint8_t *str)
{
  uint32_t ret = str[0]; /* if it is single char it already is the CtxCode */
  if (str[0] && str[1])
    {
      uint32_t str_hash;
      /* trim ctx_ and CTX_ prefix */
      if ( (str[0] == 'c' && str[1] == 't' && str[2] == 'x' && str[3] == '_') ||
           (str[0] == 'C' && str[1] == 'T' && str[2] == 'X' && str[3] == '_') )
        {
          str += 4;
        }
      if ( (str[0] == 's' && str[1] == 'e' && str[2] == 't' && str[3] == '_') )
        { str += 4; }
      str_hash = ctx_strhash ( (char *) str, 0);
      switch (str_hash)
        {
#define CTX_ENABLE_DEFUN 1
#if CTX_ENABLE_DEFUN
#define CTX_function CTX_STRH('f','u','n','c','t','i','o','n',0,0,0,0,0,0)
#define CTX_endfun CTX_STRH('e','n','d','f','u','n',0,0,0,0,0,0,0,0)

          case CTX_function:  ret = CTX_FUNCTION; break;
          //case CTX_endfun:    ret = CTX_ENDFUN; break;
#endif
          /* first a list of mappings to one_char hashes, handled in a
           * separate fast path switch without hashing
           */
          case CTX_arcTo:
          case CTX_arc_to:         ret = CTX_ARC_TO; break;
          case CTX_arc:            ret = CTX_ARC; break;
          case CTX_curveTo:
          case CTX_curve_to:       ret = CTX_CURVE_TO; break;
          case CTX_setkey:         ret = CTX_SET; parser->t_args=0;break;
          case CTX_getkey:         ret = CTX_GET; break;
          case CTX_restore:        ret = CTX_RESTORE; break;
          case CTX_stroke:         ret = CTX_STROKE; break;
          case CTX_fill:           ret = CTX_FILL; break;
          case CTX_flush:          ret = CTX_FLUSH; break;
          case CTX_hor_line_to:
          case CTX_horLineTo:      ret = CTX_HOR_LINE_TO; break;
          case CTX_rotate:         ret = CTX_ROTATE; break;
          case CTX_color:          ret = CTX_SET_COLOR; break;
          case CTX_line_to:
          case CTX_lineTo:         ret = CTX_LINE_TO; break;
          case CTX_move_to:
          case CTX_moveTo:         ret = CTX_MOVE_TO; break;
          case CTX_scale:          ret = CTX_SCALE; break;
          case CTX_new_page:
          case CTX_newPage:        ret = CTX_NEW_PAGE; break;
          case CTX_quad_to:
          case CTX_quadTo:         ret = CTX_QUAD_TO; break;
          case CTX_media_box:
          case CTX_mediaBox:       ret = CTX_MEDIA_BOX; break;
          case CTX_smoothTo:
          case CTX_smooth_to:      ret = CTX_SMOOTH_TO; break;
          case CTX_smoothQuadTo:
          case CTX_smooth_quad_to: ret = CTX_SMOOTHQ_TO; break;

          case CTX_clear:          ret = CTX_COMPOSITE_CLEAR; break;
          case CTX_copy:           ret = CTX_COMPOSITE_COPY; break;
          case CTX_destinationOver:
          case CTX_destination_over: ret = CTX_COMPOSITE_DESTINATION_OVER; break;
          case CTX_destinationOut:
          case CTX_destination_out:  ret = CTX_COMPOSITE_DESTINATION_OUT; break;
          case CTX_sourceOver:
          case CTX_source_over:      ret = CTX_COMPOSITE_SOURCE_OVER; break;
          case CTX_sourceAtop:
          case CTX_source_atop:      ret = CTX_COMPOSITE_SOURCE_ATOP; break;
          case CTX_destinationAtop:
          case CTX_destination_atop: ret = CTX_COMPOSITE_DESTINATION_ATOP; break;
          case CTX_sourceOut:
          case CTX_source_out:     ret = CTX_COMPOSITE_SOURCE_OUT; break;
          case CTX_sourceIn:
          case CTX_source_in:      ret = CTX_COMPOSITE_SOURCE_IN; break;
          case CTX_xor:            ret = CTX_COMPOSITE_XOR; break;
          case CTX_darken:         ret = CTX_BLEND_DARKEN; break;
          case CTX_lighten:        ret = CTX_BLEND_LIGHTEN; break;
          //case CTX_color:          ret = CTX_BLEND_COLOR; break;
          //
          //  XXX check that he special casing for color works
          //      it is the first collision and it is due to our own
          //      color, not w3c for now unique use of it
          //
          case CTX_hue:            ret = CTX_BLEND_HUE; break;
          case CTX_multiply:       ret = CTX_BLEND_MULTIPLY; break;
          case CTX_screen:         ret = CTX_BLEND_SCREEN; break;
          case CTX_difference:     ret = CTX_BLEND_DIFFERENCE; break;
          case CTX_reset:          ret = CTX_RESET; break;
          case CTX_verLineTo:
          case CTX_ver_line_to:    ret = CTX_VER_LINE_TO; break;
          case CTX_exit:
          case CTX_done:           ret = CTX_EXIT; break;
          case CTX_close_path:
          case CTX_closePath:      ret =  CTX_CLOSE_PATH; break;
          case CTX_new_path:
          case CTX_begin_path:
          case CTX_beginPath:
          case CTX_newPath:        ret =  CTX_NEW_PATH; break;
          case CTX_rel_arc_to:
          case CTX_relArcTo:       ret = CTX_REL_ARC_TO; break;
          case CTX_clip:           ret = CTX_CLIP; break;
          case CTX_rel_curve_to:
          case CTX_relCurveTo:     ret = CTX_REL_CURVE_TO; break;
          case CTX_save:           ret = CTX_SAVE; break;
          case CTX_translate:      ret = CTX_TRANSLATE; break;
          case CTX_linear_gradient:
          case CTX_linearGradient: ret = CTX_LINEAR_GRADIENT; break;
          case CTX_rel_hor_line_to:
          case CTX_relHorLineTo:   ret = CTX_REL_HOR_LINE_TO; break;
          case CTX_rel_line_to:
          case CTX_relLineTo:      ret = CTX_REL_LINE_TO; break;
          case CTX_relMoveTo:
          case CTX_rel_move_to:    ret = CTX_REL_MOVE_TO; break;
          case CTX_font:           ret = CTX_SET_FONT; break;
          case CTX_radial_gradient:ret = CTX_RADIAL_GRADIENT; break;
          case CTX_gradient_add_stop:
          case CTX_gradientAddStop:
          case CTX_addStop:
          case CTX_add_stop:       ret = CTX_GRADIENT_STOP; break;
          case CTX_rel_quad_to:
          case CTX_relQuadTo:      ret = CTX_REL_QUAD_TO; break;
          case CTX_rectangle:
          case CTX_rect:           ret = CTX_RECTANGLE; break;
          case CTX_round_rectangle:
          case CTX_roundRectangle: ret = CTX_ROUND_RECTANGLE; break;
          case CTX_rel_smooth_to:
          case CTX_relSmoothTo:    ret = CTX_REL_SMOOTH_TO; break;
          case CTX_relSmoothqTo:
          case CTX_rel_smoothq_to: ret = CTX_REL_SMOOTHQ_TO; break;
          case CTX_text_stroke:
          case CTX_textStroke:     ret = CTX_TEXT_STROKE; break;
          case CTX_rel_ver_line_to:
          case CTX_relVerLineTo:   ret = CTX_REL_VER_LINE_TO; break;
          case CTX_text:           ret = CTX_TEXT; break;
          case CTX_identity:       ret = CTX_IDENTITY; break;
          case CTX_transform:      ret = CTX_APPLY_TRANSFORM; break;

          case STR (CTX_SET_KEY,'m',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_composite:
          case CTX_compositing_mode:
          case CTX_compositingMode:
            return ctx_parser_set_command (parser, CTX_SET_COMPOSITING_MODE);
          case CTX_blend:
          case CTX_blending:
          case CTX_blending_mode:
          case CTX_blendMode:
            return ctx_parser_set_command (parser, CTX_SET_BLEND_MODE);
          case CTX_rgb_space:
          case CTX_rgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_RGB_SPACE);
          case CTX_cmykSpace:
          case CTX_cmyk_space:
            return ctx_parser_set_command (parser, CTX_SET_CMYK_SPACE);
          case CTX_drgb_space:
          case CTX_drgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_DRGB_SPACE);
          case STR (CTX_SET_KEY,'r',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_fill_rule:
          case CTX_fillRule:
            return ctx_parser_set_command (parser, CTX_SET_FILL_RULE);
          case STR (CTX_SET_KEY,'f',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_font_size:
          case CTX_fontSize:
            return ctx_parser_set_command (parser, CTX_SET_FONT_SIZE);
          case STR (CTX_SET_KEY,'l',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_miter_limit:
          case CTX_miterLimit:
            return ctx_parser_set_command (parser, CTX_SET_MITER_LIMIT);
          case STR (CTX_SET_KEY,'t',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_text_align:
          case CTX_textAlign:
            return ctx_parser_set_command (parser, CTX_SET_TEXT_ALIGN);
          case STR (CTX_SET_KEY,'b',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_textBaseline:
          case CTX_text_baseline:
            return ctx_parser_set_command (parser, CTX_SET_TEXT_BASELINE);
          case STR (CTX_SET_KEY,'d',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_textDirection:
          case CTX_text_direction:
            return ctx_parser_set_command (parser, CTX_SET_TEXT_DIRECTION);
          case STR (CTX_SET_KEY,'j',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_join:
          case CTX_line_join:
          case CTX_lineJoin:
            return ctx_parser_set_command (parser, CTX_SET_LINE_JOIN);
          case STR (CTX_SET_KEY,'c',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_cap:
          case CTX_line_cap:
          case CTX_lineCap:
            return ctx_parser_set_command (parser, CTX_SET_LINE_CAP);
          case STR (CTX_SET_KEY,'w',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_line_width:
          case CTX_lineWidth:
            return ctx_parser_set_command (parser, CTX_SET_LINE_WIDTH);
          case STR (CTX_SET_KEY,'a',0,0,0,0,0,0,0,0,0,0,0,0) :
          case CTX_global_alpha:
          case CTX_globalAlpha:
            return ctx_parser_set_command (parser, CTX_SET_GLOBAL_ALPHA);
          /* strings are handled directly here,
           * instead of in the one-char handler, using return instead of break
           */
          case CTX_gray:
            ctx_parser_set_color_model (parser, CTX_GRAY);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_graya:
            ctx_parser_set_color_model (parser, CTX_GRAYA);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_rgb:
            ctx_parser_set_color_model (parser, CTX_RGB);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_drgb:
            ctx_parser_set_color_model (parser, CTX_DRGB);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_rgba:
            ctx_parser_set_color_model (parser, CTX_RGBA);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_drgba:
            ctx_parser_set_color_model (parser, CTX_DRGBA);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_cmyk:
            ctx_parser_set_color_model (parser, CTX_CMYK);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_cmyka:
            ctx_parser_set_color_model (parser, CTX_CMYKA);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_lab:
            ctx_parser_set_color_model (parser, CTX_LAB);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_laba:
            ctx_parser_set_color_model (parser, CTX_LABA);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_lch:
            ctx_parser_set_color_model (parser, CTX_LCH);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          case CTX_lcha:
            ctx_parser_set_color_model (parser, CTX_LCHA);
            return ctx_parser_set_command (parser, CTX_SET_COLOR);
          /* words that correspond to low integer constants
          */
          case CTX_winding:     return CTX_FILL_RULE_WINDING;
          case CTX_evenOdd:
          case CTX_even_odd:    return CTX_FILL_RULE_EVEN_ODD;
          case CTX_bevel:       return CTX_JOIN_BEVEL;
          case CTX_round:       return CTX_JOIN_ROUND;
          case CTX_miter:       return CTX_JOIN_MITER;
          case CTX_none:        return CTX_CAP_NONE;
          case CTX_square:      return CTX_CAP_SQUARE;
          case CTX_start:       return CTX_TEXT_ALIGN_START;
          case CTX_end:         return CTX_TEXT_ALIGN_END;
          case CTX_left:        return CTX_TEXT_ALIGN_LEFT;
          case CTX_right:       return CTX_TEXT_ALIGN_RIGHT;
          case CTX_center:      return CTX_TEXT_ALIGN_CENTER;
          case CTX_top:         return CTX_TEXT_BASELINE_TOP;
          case CTX_bottom :     return CTX_TEXT_BASELINE_BOTTOM;
          case CTX_middle:      return CTX_TEXT_BASELINE_MIDDLE;
          case CTX_alphabetic:  return CTX_TEXT_BASELINE_ALPHABETIC;
          case CTX_hanging:     return CTX_TEXT_BASELINE_HANGING;
          case CTX_ideographic: return CTX_TEXT_BASELINE_IDEOGRAPHIC;
#undef STR
#undef LOWER
          default:
            ret = str_hash;
        }
    }
  if (ret == CTX_CLOSE_PATH2)
    { ret = CTX_CLOSE_PATH; }
  /* handling single char, and ret = foo; break;  in cases above*/
  return ctx_parser_set_command (parser, (CtxCode) ret);
}

enum
{
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

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model)
{
  parser->color_model      = color_model;
  parser->color_components = ctx_color_model_get_components (color_model);
}

static void ctx_parser_get_color_rgba (CtxParser *parser, int offset, float *red, float *green, float *blue, float *alpha)
{
  /* XXX - this function is to be deprecated */
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

static void ctx_parser_set (CtxParser *parser, uint32_t key_hash,
                            const char *value, int val_len)
{
  if (parser->set_prop)
  {
     parser->set_prop (parser->prop_data, key_hash, value, val_len);
  }
}

char *ctx_parser_get (CtxParser *parser, const char *key)
{
  char *ret = NULL;
  int len = 0;
  if (parser->get_prop)
  {
     parser->get_prop (parser->prop_data, key, &ret, &len);
  }
  return ret;
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
      default:
        break; // to silence warnings about missing ones
      case CTX_PRESERVE:
        ctx_preserve (ctx);
        break;
      case CTX_FILL:
        ctx_fill (ctx);
        break;
      case CTX_SAVE:
        ctx_save (ctx);
        break;
      case CTX_STROKE:
        ctx_stroke (ctx);
        break;
      case CTX_RESTORE:
        ctx_restore (ctx);
        break;
#if CTX_ENABLE_CM
      case CTX_SET_DRGB_SPACE:
        ctx_set_drgb_space (ctx, arg (0) );
        break;
      case CTX_SET_DCMYK_SPACE:
        ctx_set_dcmyk_space (ctx, arg (0) );
        break;
      case CTX_SET_RGB_SPACE:
        ctx_set_rgb_space (ctx, arg (0) );
        break;
      case CTX_SET_CMYK_SPACE:
        ctx_set_cmyk_space (ctx, arg (0) );
        break;
#endif
      case CTX_SET_COLOR:
        {
          switch (parser->color_model)
            {
              case CTX_GRAY:
                ctx_set_gray (ctx, arg (0) );
                break;
              case CTX_GRAYA:
                ctx_set_rgba (ctx, arg (0), arg (0), arg (0), arg (1) );
                break;
              case CTX_RGB:
                ctx_set_rgb (ctx, arg (0), arg (1), arg (2) );
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYK:
                ctx_set_cmyk (ctx, arg (0), arg (1), arg (2), arg (3) );
                break;
              case CTX_CMYKA:
                ctx_set_cmyka (ctx, arg (0), arg (1), arg (2), arg (3), arg (4) );
                break;
#else
              /* when there is no cmyk support at all in rasterizer
               * do a naive mapping to RGB on input.
               */
              case CTX_CMYK:
              case CTX_CMYKA:
                {
                  float r,g,b,a = 1.0f;
                  ctx_cmyk_to_rgb (arg (0), arg (1), arg (2), arg (3), &r, &g, &b);
                  if (parser->color_model == CTX_CMYKA)
                    { a = arg (4); }
                  ctx_set_rgba (ctx, r, g, b, a);
                }
                break;
#endif
              case CTX_RGBA:
                ctx_set_rgba (ctx, arg (0), arg (1), arg (2), arg (3) );
                break;
              case CTX_DRGB:
                ctx_set_drgba (ctx, arg (0), arg (1), arg (2), 1.0);
                break;
              case CTX_DRGBA:
                ctx_set_drgba (ctx, arg (0), arg (1), arg (2), arg (3) );
                break;
            }
        }
        break;
      case CTX_ARC_TO:
        ctx_arc_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4) );
        break;
      case CTX_REL_ARC_TO:
        ctx_rel_arc_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4) );
        break;
      case CTX_REL_SMOOTH_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          float ax = 2 * ctx_x (ctx) - cx;
          float ay = 2 * ctx_y (ctx) - cy;
          ctx_curve_to (ctx, ax, ay, arg (0) +  cx, arg (1) + cy,
                        arg (2) + cx, arg (3) + cy);
          parser->pcx = arg (0) + cx;
          parser->pcy = arg (1) + cy;
        }
        break;
      case CTX_SMOOTH_TO:
        {
          float ax = 2 * ctx_x (ctx) - parser->pcx;
          float ay = 2 * ctx_y (ctx) - parser->pcy;
          ctx_curve_to (ctx, ax, ay, arg (0), arg (1),
                        arg (2), arg (3) );
          parser->pcx = arg (0);
          parser->pcx = arg (1);
        }
        break;
      case CTX_SMOOTHQ_TO:
        ctx_quad_to (ctx, parser->pcx, parser->pcy, arg (0), arg (1) );
        break;
      case CTX_REL_SMOOTHQ_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          parser->pcx = 2 * ctx_x (ctx) - parser->pcx;
          parser->pcy = 2 * ctx_y (ctx) - parser->pcy;
          ctx_quad_to (ctx, parser->pcx, parser->pcy, arg (0) +  cx, arg (1) + cy);
        }
        break;
      case CTX_VER_LINE_TO:
        ctx_line_to (ctx, ctx_x (ctx), arg (0) );
        parser->command = CTX_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_HOR_LINE_TO:
        ctx_line_to (ctx, arg (0), ctx_y (ctx) );
        parser->command = CTX_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_HOR_LINE_TO:
        ctx_rel_line_to (ctx, arg (0), 0.0f);
        parser->command = CTX_REL_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_VER_LINE_TO:
        ctx_rel_line_to (ctx, 0.0f, arg (0) );
        parser->command = CTX_REL_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_ARC:
        ctx_arc (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        break;
      case CTX_APPLY_TRANSFORM:
        ctx_apply_transform (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        break;
      case CTX_CURVE_TO:
        ctx_curve_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        parser->pcx = arg (2);
        parser->pcy = arg (3);
        parser->command = CTX_CURVE_TO;
        break;
      case CTX_REL_CURVE_TO:
        parser->pcx = arg (2) + ctx_x (ctx);
        parser->pcy = arg (3) + ctx_y (ctx);
        ctx_rel_curve_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        parser->command = CTX_REL_CURVE_TO;
        break;
      case CTX_LINE_TO:
        ctx_line_to (ctx, arg (0), arg (1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg (0);
        parser->pcy = arg (1);
        break;
      case CTX_MOVE_TO:
        ctx_move_to (ctx, arg (0), arg (1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg (0);
        parser->pcy = arg (1);
        parser->left_margin = parser->pcx;
        break;
      case CTX_SET_FONT_SIZE:
        ctx_set_font_size (ctx, arg (0) );
        break;
      case CTX_SET_MITER_LIMIT:
        ctx_set_miter_limit (ctx, arg (0) );
        break;
      case CTX_SCALE:
        ctx_scale (ctx, arg (0), arg (1) );
        break;
      case CTX_QUAD_TO:
        parser->pcx = arg (0);
        parser->pcy = arg (1);
        ctx_quad_to (ctx, arg (0), arg (1), arg (2), arg (3) );
        parser->command = CTX_QUAD_TO;
        break;
      case CTX_REL_QUAD_TO:
        parser->pcx = arg (0) + ctx_x (ctx);
        parser->pcy = arg (1) + ctx_y (ctx);
        ctx_rel_quad_to (ctx, arg (0), arg (1), arg (2), arg (3) );
        parser->command = CTX_REL_QUAD_TO;
        break;
      case CTX_CLIP:
        ctx_clip (ctx);
        break;
      case CTX_TRANSLATE:
        ctx_translate (ctx, arg (0), arg (1) );
        break;
      case CTX_ROTATE:
        ctx_rotate (ctx, arg (0) );
        break;
      case CTX_SET_FONT:
        ctx_set_font (ctx, (char *) parser->holding);
        break;


      case CTX_SET:
        parser->t_args++;
        if (parser->t_args % 2 == 1)
        {
           if (parser->n_numbers == 1)
           {
             parser->set_key_hash = parser->numbers[0];
           }
           else
           {
             parser->set_key_hash = ctx_strhash ((char*)parser->holding, 0);
           }
        }
        else
        {
           if (parser->set_prop)
             ctx_parser_set (parser, parser->set_key_hash, (char*)parser->holding, parser->pos);
           else
             ctx_set (ctx, parser->set_key_hash, (char*)parser->holding, parser->pos);
        }
        parser->command = CTX_SET;
        break;
      case CTX_GET:
        {
        char *val = ctx_parser_get (parser, (char*)parser->holding);
        if (val)
          free (val);
        parser->command = CTX_GET;
        }
        break;

      case CTX_TEXT_STROKE:
      case CTX_TEXT:
        if (parser->n_numbers == 1)
          { ctx_rel_move_to (ctx, -parser->numbers[0], 0.0); }  //  XXX : scale by font(size)
        else
          {
            for (char *c = (char *) parser->holding; c; )
              {
                char *next_nl = ctx_strchr (c, '\n');
                if (next_nl)
                  { *next_nl = 0; }
                /* do our own layouting on a per-word basis?, to get justified
                 * margins? then we'd want explict margins rather than the
                 * implicit ones from move_to's .. making move_to work within
                 * margins.
                 */
                if (cmd == CTX_TEXT_STROKE)
                  { ctx_text_stroke (ctx, c); }
                else
                  { ctx_text (ctx, c); }
                if (next_nl)
                  {
                    *next_nl = '\n'; // swap it newline back in
                    ctx_move_to (ctx, parser->left_margin, ctx_y (ctx) +
                                 ctx_get_font_size (ctx) );
                    c = next_nl + 1;
                    if (c[0] == 0)
                      { c = NULL; }
                  }
                else
                  {
                    c = NULL;
                  }
              }
          }
        if (cmd == CTX_TEXT_STROKE)
          { parser->command = CTX_TEXT_STROKE; }
        else
          { parser->command = CTX_TEXT; }
        break;
      case CTX_REL_LINE_TO:
        ctx_rel_line_to (ctx, arg (0), arg (1) );
        parser->pcx += arg (0);
        parser->pcy += arg (1);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rel_move_to (ctx, arg (0), arg (1) );
        parser->pcx += arg (0);
        parser->pcy += arg (1);
        parser->left_margin = ctx_x (ctx);
        break;
      case CTX_SET_LINE_WIDTH:
        ctx_set_line_width (ctx, arg (0) );
        break;
      case CTX_SET_LINE_JOIN:
        ctx_set_line_join (ctx, (CtxLineJoin) arg (0) );
        break;
      case CTX_SET_LINE_CAP:
        ctx_set_line_cap (ctx, (CtxLineCap) arg (0) );
        break;
      case CTX_SET_COMPOSITING_MODE:
        ctx_set_compositing_mode (ctx, (CtxCompositingMode) arg (0) );
        break;
      case CTX_SET_BLEND_MODE:
        {
          int blend_mode = arg(0);
          if (blend_mode == CTX_SET_COLOR) blend_mode = CTX_BLEND_COLOR;
          ctx_set_blend_mode (ctx, blend_mode);
        }
        break;
      case CTX_SET_FILL_RULE:
        ctx_set_fill_rule (ctx, (CtxFillRule) arg (0) );
        break;
      case CTX_SET_TEXT_ALIGN:
        ctx_set_text_align (ctx, (CtxTextAlign) arg (0) );
        break;
      case CTX_SET_TEXT_BASELINE:
        ctx_set_text_baseline (ctx, (CtxTextBaseline) arg (0) );
        break;
      case CTX_SET_TEXT_DIRECTION:
        ctx_set_text_direction (ctx, (CtxTextDirection) arg (0) );
        break;
      case CTX_IDENTITY:
        ctx_identity (ctx);
        break;
      case CTX_RECTANGLE:
        ctx_rectangle (ctx, arg (0), arg (1), arg (2), arg (3) );
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_linear_gradient (ctx, arg (0), arg (1), arg (2), arg (3) );
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_radial_gradient (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        break;
      case CTX_GRADIENT_STOP:
        {
          float red, green, blue, alpha;
          ctx_parser_get_color_rgba (parser, 1, &red, &green, &blue, &alpha);
          ctx_gradient_add_stop (ctx, arg (0), red, green, blue, alpha);
        }
        break;
      case CTX_SET_GLOBAL_ALPHA:
        ctx_set_global_alpha (ctx, arg (0) );
        break;
      case CTX_NEW_PATH:
        ctx_new_path (ctx);
        break;
      case CTX_CLOSE_PATH:
        ctx_close_path (ctx);
        break;
      case CTX_EXIT:
        if (parser->exit)
          { parser->exit (parser->exit_data);
            return;
          }
        break;
      case CTX_FLUSH:
        //ctx_flush (ctx);
        break;
      case CTX_RESET:
        ctx_reset (ctx);
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
  if (parser->pos > (int) sizeof (parser->holding)-2)
    { parser->pos = sizeof (parser->holding)-2; }
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
              { *value  *= ( (parser->width) /100.0); }
            else
              { *value *= ( (parser->height) /100.0); }
          }
        break;
      default: // even means x coord
        if (arg_no % 2 == 0)
          { *value  *= ( (parser->width) /100.0); }
        else
          { *value *= ( (parser->height) /100.0); }
        break;
    }
}

static void ctx_parser_transform_cell (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  float small = parser->cell_width;
  if (small > parser->cell_height)
    { small = parser->cell_height; }
  switch (code)
    {
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
        switch (arg_no)
          {
            case 0:
            case 3:
              *value *= parser->cell_width;
              break;
            case 1:
            case 4:
              *value *= parser->cell_height;
              break;
            case 2:
            case 5:
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
            *value *= (arg_no%2==0) ?parser->cell_width:parser->cell_height;
          }
        break;
      case CTX_RECTANGLE:
        if (arg_no % 2 == 0)
          { *value *= parser->cell_width; }
        else
          {
            if (! (arg_no > 1) )
              { (*value) -= 1.0f; }
            *value *= parser->cell_height;
          }
        break;
      default: // even means x coord odd means y coord
        *value *= (arg_no%2==0) ?parser->cell_width:parser->cell_height;
        break;
    }
}

// %h %v %m %M

static void ctx_parser_word_done (CtxParser *parser)
{
  parser->holding[parser->pos]=0;
  int command = ctx_parser_resolve_command (parser, parser->holding);
  if ((command >= 0 && command < 16) 
      || (command > 127) || (command < 0)
      )  // special case low enum values
    {                   // and enum values too high to be
                        // commands - permitting passing words
                        // for strings in some cases
      parser->numbers[parser->n_numbers] = command;

      // trigger transition from number
      parser->state = CTX_PARSER_NUMBER;
      ctx_parser_feed_byte (parser, ',');
    }
  else if (command > 0)
    {
      parser->command = (CtxCode) command;
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
          parser->command = (CtxCode) ctx_parser_resolve_command (parser, buf);
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
      case '\n':
        parser->col=0;
        parser->line++;
        break;
      default:
        parser->col++;
    }
  switch (parser->state)
    {
      case CTX_PARSER_NEUTRAL:
        switch (byte)
          {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 11: case 12: case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31:
              break;
            case ' ': case '\t': case '\r': case '\n':
            case ';': case ',':
            case '(': case ')':
            case '{': case '}':
            case '=':
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
              case 0:
              case 1:
              case 2:
              case 3:
              case 4:
              case 5:
              case 6:
              case 7:
              case 8:
              case 11:
              case 12:
              case 14:
              case 15:
              case 16:
              case 17:
              case 18:
              case 19:
              case 20:
              case 21:
              case 22:
              case 23:
              case 24:
              case 25:
              case 26:
              case 27:
              case 28:
              case 29:
              case 30:
              case 31:
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case ' ':
              case '\t':
              case '\r':
              case '\n':
              case ';':
              case ',':
              case '(':
              case ')':
              case '{':
              case '}':
              case '=':
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
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
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
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
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_cell (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '%': // percent of width/height
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              default:
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                parser->state = CTX_PARSER_WORD;
                parser->pos = 0;
                ctx_parser_holding_append (parser, byte);
                break;
            }
          if ( (parser->state != CTX_PARSER_NUMBER &&
                parser->state != CTX_PARSER_NEGATIVE_NUMBER) || new_neg)
            {
              parser->n_numbers ++;
              //parser->t_args ++;
              if (parser->n_numbers == parser->n_args || parser->n_args == 100)
                {
                  ctx_parser_dispatch_command (parser);
                }
              if (parser->n_numbers > 10)
                { parser->n_numbers = 10; }
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
            case ' ': case '\t': case '\r': case '\n':
            case ';': case ',':
            case '(': case ')': case '=': case '{': case '}':
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
            case '\\': parser->state = CTX_PARSER_STRING_APOS_ESCAPED; break;
            case '\'': parser->state = CTX_PARSER_NEUTRAL;
              ctx_parser_string_done (parser); break;
            default:
              ctx_parser_holding_append (parser, byte); break;
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


#if CTX_EVENTS

void
ctx_init (int *argc, char ***argv)
{
  if (!getenv ("CTX_VERSION"))
  {
    int i;
    char *new_argv[*argc+3];
    new_argv[0] = "ctx";
    for (i = 0; i < *argc; i++)
    {
      new_argv[i+1] = *argv[i];
    }
    new_argv[i+1] = NULL;
    execvp (new_argv[0], new_argv);
    // if this fails .. we continue normal startup
    // and end up in self-hosted braille
  }
}

void _ctx_resized (Ctx *ctx, int width, int height, long time);

void ctx_set_size (Ctx *ctx, int width, int height)
{
  if (ctx->events.width != width || ctx->events.height != height)
  {
    ctx->events.width = width;
    ctx->events.height = height;
    _ctx_resized (ctx, width, height, 0);
  }
}

typedef struct CtxIdleCb {
  int (*cb) (Ctx *ctx, void *idle_data);
  void *idle_data;

  void (*destroy_notify)(void *destroy_data);
  void *destroy_data;

  int   ticks_full;
  int   ticks_remaining;
  int   is_idle;
  int   id;
} CtxIdleCb;

#include <sys/time.h>

static struct timeval start_time;

#define usecs(time)    ((time.tv_sec - start_time.tv_sec) * 1000000 + time.     tv_usec)

static void
_ctx_init_ticks (void)
{
  static int done = 0;
  if (done)
    return;
  done = 1;
  gettimeofday (&start_time, NULL);
}

static inline long
_ctx_ticks (void)
{
  struct timeval measure_time;
  gettimeofday (&measure_time, NULL);
  return usecs (measure_time) - usecs (start_time);
}

long
ctx_ticks (void)
{
  return _ctx_ticks ();
}

uint32_t ctx_ms (Ctx *ctx)
{
  return _ctx_ticks () / 1000;
}


void _ctx_events_init (Ctx *ctx)
{
  CtxEvents *events = &ctx->events;
  _ctx_init_ticks ();
  events->tap_delay_min  = 40;
  events->tap_delay_max  = 800;
  events->tap_delay_max  = 8000000; /* quick reflexes needed making it hard for some is an argument against very short values  */

  events->tap_delay_hold = 1000;
  events->tap_hysteresis = 32;  /* XXX: should be ppi dependent */
}


void ctx_add_key_binding_full (Ctx *ctx,
                           const char *key,
                           const char *action,
                           const char *label,
                           CtxCb       cb,
                           void       *cb_data,
                           CtxDestroyNotify destroy_notify,
                           void       *destroy_data)
{
  CtxEvents *events = &ctx->events;
  if (events->n_bindings +1 >= CTX_MAX_KEYBINDINGS)
  {
    fprintf (stderr, "warning: binding overflow\n");
    return;
  }
  events->bindings[events->n_bindings].nick = strdup (key);
  strcpy (events->bindings[events->n_bindings].nick, key);

  if (action)
    events->bindings[events->n_bindings].command = action ? strdup (action) : NULL;
  if (label)
    events->bindings[events->n_bindings].label = label ? strdup (label) : NULL;
  events->bindings[events->n_bindings].cb = cb;
  events->bindings[events->n_bindings].cb_data = cb_data;
  events->bindings[events->n_bindings].destroy_notify = destroy_notify;
  events->bindings[events->n_bindings].destroy_data = destroy_data;
  events->n_bindings++;
}

void ctx_add_key_binding (Ctx *ctx,
                          const char *key,
                          const char *action,
                          const char *label,
                          CtxCb       cb,
                          void       *cb_data)
{
  ctx_add_key_binding_full (ctx, key, action, label, cb, cb_data, NULL, NULL);
}


void ctx_clear_bindings (Ctx *ctx)
{
  CtxEvents *events = &ctx->events;
  int i;
  for (i = 0; events->bindings[i].nick; i ++)
  {
    if (events->bindings[i].destroy_notify)
      events->bindings[i].destroy_notify (events->bindings[i].destroy_data);
    free (events->bindings[i].nick);
    if (events->bindings[i].command)
      free (events->bindings[i].command);
    if (events->bindings[i].label)
      free (events->bindings[i].label);
  }
  memset (&events->bindings, 0, sizeof (events->bindings));
  events->n_bindings = 0;
}

static void _ctx_bindings_key_down (CtxEvent *event, void *data1, void *data2)
{
  Ctx *ctx = event->ctx;
  CtxEvents *events = &ctx->events;
  int i;
  int handled = 0;

  for (i = events->n_bindings-1; i>=0; i--)
    if (!strcmp (events->bindings[i].nick, event->string))
    {
      if (events->bindings[i].cb)
      {
        events->bindings[i].cb (event, events->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
        handled = 1;
      }
    }
  if (!handled)
  for (i = events->n_bindings-1; i>=0; i--)
    if (!strcmp (events->bindings[i].nick, "unhandled"))
    {
      if (events->bindings[i].cb)
      {
        events->bindings[i].cb (event, events->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
      }
    }
}

CtxBinding *ctx_get_bindings (Ctx *ctx)
{
  return &ctx->events.bindings[0];
}


void ctx_remove_idle (Ctx *ctx, int handle)
{
  CtxList *l;
  CtxList *to_remove = NULL;

  if (!ctx->events.idles)
  {
    return;
  }
  for (l = ctx->events.idles; l; l = l->next)
  {
    CtxIdleCb *item = l->data;
    if (item->id == handle)
      ctx_list_prepend (&to_remove, item);
  }
  for (l = to_remove; l; l = l->next)
  {
    CtxIdleCb *item = l->data;
    if (item->destroy_notify)
      item->destroy_notify (item->destroy_data);
    ctx_list_remove (&ctx->events.idles, l->data);
  }
}

int ctx_add_timeout_full (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                          void (*destroy_notify)(void *destroy_data), void *destroy_data)
{
  CtxIdleCb *item = calloc (sizeof (CtxIdleCb), 1);
  item->cb              = idle_cb;
  item->idle_data       = idle_data;
  item->id              = ++ctx->events.idle_id;
  item->ticks_full      = 
  item->ticks_remaining = ms * 1000;
  item->destroy_notify  = destroy_notify;
  item->destroy_data    = destroy_data;
  ctx_list_append (&ctx->events.idles, item);
  return item->id;
}

int ctx_add_timeout (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data)
{
  return ctx_add_timeout_full (ctx, ms, idle_cb, idle_data, NULL, NULL);
}

int ctx_add_idle_full (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                                 void (*destroy_notify)(void *destroy_data), void *destroy_data)
{
  CtxIdleCb *item = calloc (sizeof (CtxIdleCb), 1);
  item->cb = idle_cb;
  item->idle_data = idle_data;
  item->id = ++ctx->events.idle_id;
  item->ticks_full =
  item->ticks_remaining = -1;
  item->is_idle = 1;
  item->destroy_notify = destroy_notify;
  item->destroy_data = destroy_data;
  ctx_list_append (&ctx->events.idles, item);
  return item->id;
}

int ctx_add_idle (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data)
{
  return ctx_add_idle_full (ctx, idle_cb, idle_data, NULL, NULL);
}

/* using bigger primes would be a good idea, this falls apart due to rounding
 * when zoomed in close
 */
static double path_hash (void *path)
{
  double ret = 0;
#if 0
  int i;
  cairo_path_data_t *data;
  if (!path)
    return 0.99999;
  for (i = 0; i <path->num_data; i += path->data[i].header.length)
  {
    data = &path->data[i];
    switch (data->header.type) {
      case CAIRO_PATH_MOVE_TO:
        ret *= 17;
        ret += data[1].point.x;
        ret *= 113;
        ret += data[1].point.y;
        break;
      case CAIRO_PATH_LINE_TO:
        ret *= 121;
        ret += data[1].point.x;
        ret *= 1021;
        ret += data[1].point.y;
        break;
      case CAIRO_PATH_CURVE_TO:
        ret *= 3111;
        ret += data[1].point.x;
        ret *= 23;
        ret += data[1].point.y;
        ret *= 107;
        ret += data[2].point.x;
        ret *= 739;
        ret += data[2].point.y;
        ret *= 3;
        ret += data[3].point.x;
        ret *= 51;
        ret += data[3].point.y;
        break;
      case CAIRO_PATH_CLOSE_PATH:
        ret *= 51;
        break;
    }
  }
#endif
  return ret;
}

void _ctx_item_ref (CtxItem *item)
{
  if (item->ref_count < 0)
  {
    fprintf (stderr, "EEEEK!\n");
  }
  item->ref_count++;
}


void _ctx_item_unref (CtxItem *item)
{
  if (item->ref_count <= 0)
  {
    fprintf (stderr, "EEEEK!\n");
    return;
  }
  item->ref_count--;
  if (item->ref_count <=0)
  {
    {
      int i;
      for (i = 0; i < item->cb_count; i++)
      {
        if (item->cb[i].finalize)
          item->cb[i].finalize (item->cb[i].data1, item->cb[i].data2,
                                   item->cb[i].finalize_data);
      }
    }
    if (item->path)
    {
      //cairo_path_destroy (item->path);
    }
    free (item);
  }
}


static int
path_equal (void *path,
            void *path2)
{
  //  XXX
  return 0;
}

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
                      void    *finalize_data)
{
  if (!ctx->events.frozen)
  {
    CtxItem *item;


    /* early bail for listeners outside screen  */
    {
      float tx = x;
      float ty = y;
      float tw = width;
      float th = height;
      ctx_user_to_device (&ctx->state, &tx, &ty);
      ctx_user_to_device_distance (&ctx->state, &tw, &th);
      if (ty > ctx->events.height * 2 ||
          tx > ctx->events.width * 2 ||
          tx + tw < 0 ||
          ty + th < 0)
      {
        if (finalize)
          finalize (data1, data2, finalize_data);
        return;
      }
    }

    item = calloc (sizeof (CtxItem), 1);
    item->x0 = x;
    item->y0 = y;
    item->x1 = x + width;
    item->y1 = y + height;
    item->cb[0].types = types;
    item->cb[0].cb = cb;
    item->cb[0].data1 = data1;
    item->cb[0].data2 = data2;
    item->cb[0].finalize = finalize;
    item->cb[0].finalize_data = finalize_data;
    item->cb_count = 1;
    item->types = types;
    //item->path = cairo_copy_path (cr); // XXX
    item->path_hash = path_hash (item->path);
    ctx_get_matrix (ctx, &item->inv_matrix);
    ctx_matrix_invert (&item->inv_matrix);

    if (ctx->events.items)
    {
      CtxList *l;
      for (l = ctx->events.items; l; l = l->next)
      {
        CtxItem *item2 = l->data;

        /* store multiple callbacks for one entry when the paths
         * are exact matches, reducing per event traversal checks at the
         * cost of a little paint-hit (XXX: is this the right tradeoff,
         * perhaps it is better to spend more time during event processing
         * than during paint?)
         */
        if (item->path_hash == item2->path_hash &&
            path_equal (item->path, item2->path))
        {
          /* found an item, copy over cb data  */
          item2->cb[item2->cb_count] = item->cb[0];
          free (item);
          item2->cb_count++;
          item2->types |= types;
          /* increment ref_count? */
         return;
        }
      }
    }
    item->ref_count = 1;
    ctx_list_prepend_full (&ctx->events.items, item, (void*)_ctx_item_unref, NULL);
  }
}

void ctx_event_stop_propagate (CtxEvent *event)
{
  if (event)
    event->stop_propagate = 1;
}

void ctx_listen (Ctx          *ctx,
                 CtxEventType  types,
                 CtxCb         cb,
                 void*         data1,
                 void*         data2)
{
  float x, y, width, height;
  /* generate bounding box of what to listen for - from current cairo path */
  if (types & CTX_KEY)
  {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
  }
  else
  {
     float ex1,ey1,ex2,ey2;
     ctx_path_extents (ctx, &ex1, &ey1, &ex2, &ey2);
     x = ex1;
     y = ey1;
     width = ex2 - ex1;
     height = ey2 - ey1;
  }

  if (types == CTX_DRAG_MOTION)
    types = CTX_DRAG_MOTION | CTX_DRAG_PRESS;
  return ctx_listen_full (ctx, x, y, width, height, types, cb, data1, data2, NULL, NULL);
}

typedef struct _CtxGrab CtxGrab;

struct _CtxGrab
{
  CtxItem *item;
  int      device_no;
  int      timeout_id;
  int      start_time;
  float    x; // for tap and hold
  float    y;
  CtxEventType  type;
};

static void grab_free (Ctx *ctx, CtxGrab *grab)
{
  if (grab->timeout_id)
  {
    ctx_remove_idle (ctx, grab->timeout_id);
    grab->timeout_id = 0;
  }
  _ctx_item_unref (grab->item);
  free (grab);
}

static void device_remove_grab (Ctx *ctx, CtxGrab *grab)
{
  ctx_list_remove (&ctx->events.grabs, grab);
  grab_free (ctx, grab);
}

static CtxGrab *device_add_grab (Ctx *ctx, int device_no, CtxItem *item, CtxEventType type)
{
  CtxGrab *grab = calloc (1, sizeof (CtxGrab));
  grab->item = item;
  grab->type = type;
  _ctx_item_ref (item);
  grab->device_no = device_no;
  ctx_list_append (&ctx->events.grabs, grab);
  return grab;
}

CtxList *device_get_grabs (Ctx *ctx, int device_no)
{
  CtxList *ret = NULL;
  CtxList *l;
  for (l = ctx->events.grabs; l; l = l->next)
  {
    CtxGrab *grab = l->data;
    if (grab->device_no == device_no)
      ctx_list_append (&ret, grab);
  }
  return ret;
}

static void _mrg_restore_path (Ctx *ctx, void *path)  //XXX
{
  //int i;
  //cairo_path_data_t *data;
  //cairo_new_path (cr);
  //cairo_append_path (cr, path);
}

CtxList *_ctx_detect_list (Ctx *ctx, float x, float y, CtxEventType type)
{
  CtxList *a;
  CtxList *ret = NULL;

  if (type == CTX_KEY_DOWN ||
      type == CTX_KEY_UP ||
      type == CTX_MESSAGE ||
      type == (CTX_KEY_DOWN|CTX_MESSAGE) ||
      type == (CTX_KEY_DOWN|CTX_KEY_UP) ||
      type == (CTX_KEY_DOWN|CTX_KEY_UP|CTX_MESSAGE))
  {
    for (a = ctx->events.items; a; a = a->next)
    {
      CtxItem *item = a->data;
      if (item->types & type)
      {
        ctx_list_prepend (&ret, item);
        return ret;
      }
    }
    return NULL;
  }

  for (a = ctx->events.items; a; a = a->next)
  {
    CtxItem *item= a->data;
  
    float u, v;
    u = x;
    v = y;
    ctx_matrix_apply_transform (&item->inv_matrix, &u, &v);

    if (u >= item->x0 && v >= item->y0 &&
        u <  item->x1 && v <  item->y1 && 
        item->types & type)
    {
      if (item->path)
      {
        _mrg_restore_path (ctx, item->path);
        if (ctx_in_fill (ctx, u, v))
        {
          ctx_new_path (ctx);
          ctx_list_prepend (&ret, item);
        }
        ctx_new_path (ctx);
      }
      else
      {
        ctx_list_prepend (&ret, item);
      }
    }
  }
  return ret;
}

CtxItem *_ctx_detect (Ctx *ctx, float x, float y, CtxEventType type)
{
  CtxList *l = _ctx_detect_list (ctx, x, y, type);
  if (l)
  {
    ctx_list_reverse (&l);
    CtxItem *ret = l->data;
    ctx_list_free (&l);
    return ret;
  }
  return NULL;
}

static int
_ctx_emit_cb_item (Ctx *ctx, CtxItem *item, CtxEvent *event, CtxEventType type, float x, float y)
{
  static CtxEvent s_event;
  CtxEvent transformed_event;
  int i;

  if (!event)
  {
    event = &s_event;
    event->type = type;
    event->x = x;
    event->y = y;
  }
  event->ctx = ctx;
  transformed_event = *event;
  transformed_event.device_x = event->x;
  transformed_event.device_y = event->y;

  {
    float tx, ty;
    tx = transformed_event.x;
    ty = transformed_event.y;
    ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
    transformed_event.x = tx;
    transformed_event.y = ty;

    if ((type & CTX_DRAG_PRESS) ||
        (type & CTX_DRAG_MOTION) ||
        (type & CTX_MOTION))   /* probably a worthwhile check for the performance 
                                  benefit
                                */
    {
      tx = transformed_event.start_x;
      ty = transformed_event.start_y;
      ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
      transformed_event.start_x = tx;
      transformed_event.start_y = ty;
    }


    tx = transformed_event.delta_x;
    ty = transformed_event.delta_y;
    ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
    transformed_event.delta_x = tx;
    transformed_event.delta_y = ty;
  }

  transformed_event.state = ctx->events.modifier_state;
  transformed_event.type = type;

  for (i = item->cb_count-1; i >= 0; i--)
  {
    if (item->cb[i].types & type)
    {
      item->cb[i].cb (&transformed_event, item->cb[i].data1, item->cb[i].data2);
      event->stop_propagate = transformed_event.stop_propagate; /* copy back the response */
      if (event->stop_propagate)
        return event->stop_propagate;
    }
  }
  return 0;
}
#if CTX_EVENTS
static int ctx_native_events = 0;
int mrg_nct_consume_events (Ctx *ctx);
int mrg_ctx_consume_events (Ctx *ctx);
CtxEvent *ctx_get_event (Ctx *ctx)
{
  static CtxEvent copy;
  if (!ctx->events.ctx_get_event_enabled)
    ctx->events.ctx_get_event_enabled = 1;

  if (ctx_native_events)
    mrg_ctx_consume_events (ctx);
  else
    mrg_nct_consume_events (ctx);

  if (ctx->events.events)
    {
      copy = *((CtxEvent*)(ctx->events.events->data));
      ctx_list_remove (&ctx->events.events, ctx->events.events->data);
      return &copy;
    }

  return NULL;
}
#endif

static int
_ctx_emit_cb (Ctx *ctx, CtxList *items, CtxEvent *event, CtxEventType type, float x, float y)
{
  CtxList *l;
  event->stop_propagate = 0;
  for (l = items; l; l = l->next)
  {
    _ctx_emit_cb_item (ctx, l->data, event, type, x, y);
    if (event->stop_propagate)
      return event->stop_propagate;
  }
  return 0;
}

/*
 * update what is the currently hovered item and returns it.. and the list of hits
 * a well.
 *
 */
static CtxItem *_ctx_update_item (Ctx *ctx, int device_no, float x, float y, CtxEventType type, CtxList **hitlist)
{
  CtxItem *current = NULL;

  CtxList *l = _ctx_detect_list (ctx, x, y, type);
  if (l)
  {
    ctx_list_reverse (&l);
    current = l->data;
  }
  if (hitlist)
    *hitlist = l;
  else
    ctx_list_free (&l);

  if (ctx->events.prev[device_no] == NULL || current == NULL || (current->path_hash != ctx->events.prev[device_no]->path_hash))
  {
    //int focus_radius = 2;
    if (current)
      _ctx_item_ref (current);

    if (ctx->events.prev[device_no])
    {
      {
#if 0
        CtxRectangle rect = {floor(ctx->events.prev[device_no]->x0-focus_radius),
                             floor(ctx->events.prev[device_no]->y0-focus_radius),
                             ceil(ctx->events.prev[device_no]->x1)-floor(ctx->events.prev[device_no]->x0) + focus_radius * 2,
                             ceil(ctx->events.prev[device_no]->y1)-floor(ctx->events.prev[device_no]->y0) + focus_radius * 2};
        mrg_queue_draw (mrg, &rect);
#endif 
      }

      _ctx_emit_cb_item (ctx, ctx->events.prev[device_no], NULL, CTX_LEAVE, x, y);
      _ctx_item_unref (ctx->events.prev[device_no]);
      ctx->events.prev[device_no] = NULL;
    }
    if (current)
    {
#if 0
      {
        CtxRectangle rect = {floor(current->x0-focus_radius),
                             floor(current->y0-focus_radius),
                             ceil(current->x1)-floor(current->x0) + focus_radius * 2,
                             ceil(current->y1)-floor(current->y0) + focus_radius * 2};
        mrg_queue_draw (mrg, &rect);
      }
#endif
      _ctx_emit_cb_item (ctx, current, NULL, CTX_ENTER, x, y);
      ctx->events.prev[device_no] = current;
    }
  }
  current = _ctx_detect (ctx, x, y, type);
  return current;
}

static int tap_and_hold_fire (Ctx *ctx, void *data)
{
  CtxGrab *grab = data;
  CtxList *list = NULL;
  ctx_list_prepend (&list, grab->item);
  CtxEvent event = {0, };

  event.ctx = ctx;
  event.time = ctx_ms (ctx);

  event.device_x = 
  event.x = ctx->events.pointer_x[grab->device_no];
  event.device_y = 
  event.y = ctx->events.pointer_y[grab->device_no];

  // XXX: x and y coordinates
  int ret = _ctx_emit_cb (ctx, list, &event, CTX_TAP_AND_HOLD,
      ctx->events.pointer_x[grab->device_no], ctx->events.pointer_y[grab->device_no]);

  ctx_list_free (&list);

  grab->timeout_id = 0;

  return 0;

  return ret;
}

int ctx_pointer_drop (Ctx *ctx, float x, float y, int device_no, uint32_t time,
                      char *string)
{
  CtxList *l;
  CtxList *hitlist = NULL;

  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    ctx->events.pointer_x[0] = x;
    ctx->events.pointer_y[0] = y;
  }

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &ctx->events.drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->ctx = ctx;
  event->x = x;
  event->y = y;

  event->delta_x = event->delta_y = 0;

  event->device_no = device_no;
  event->string    = string;
  event->time      = time;
  event->stop_propagate = 0;

  _ctx_update_item (ctx, device_no, x, y, CTX_DROP, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;
    _ctx_emit_cb_item (ctx, item, event, CTX_DROP, x, y);

    if (event->stop_propagate)
      l = NULL;
  }

  //mrg_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);

  return 0;
}

int ctx_pointer_press (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxEvents *events = &ctx->events;
  CtxList *hitlist = NULL;
  events->pointer_x[device_no] = x;
  events->pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    events->pointer_x[0] = x;
    events->pointer_y[0] = y;
  }

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &events->drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->x = event->start_x = event->prev_x = x;
  event->y = event->start_y = event->prev_y = y;

  event->delta_x = event->delta_y = 0;

  event->device_no = device_no;
  event->time      = time;
  event->stop_propagate = 0;

  if (events->pointer_down[device_no] == 1)
  {
    fprintf (stderr, "events thought device %i was already down\n", device_no);
  }
  /* doing just one of these two should be enough? */
  events->pointer_down[device_no] = 1;
  switch (device_no)
  {
    case 1:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON1;
      break;
    case 2:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON2;
      break;
    case 3:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON3;
      break;
    default:
      break;
  }

  CtxGrab *grab = NULL;
  CtxList *l;

  _ctx_update_item (ctx, device_no, x, y, 
      CTX_PRESS | CTX_DRAG_PRESS | CTX_TAP | CTX_TAP_AND_HOLD, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;
    if (item &&
        ((item->types & CTX_DRAG)||
         (item->types & CTX_TAP) ||
         (item->types & CTX_TAP_AND_HOLD)))
    {
      grab = device_add_grab (ctx, device_no, item, item->types);
      grab->start_time = time;

      if (item->types & CTX_TAP_AND_HOLD)
      {
         grab->timeout_id = ctx_add_timeout (ctx, events->tap_delay_hold, tap_and_hold_fire, grab);
      }
    }
    _ctx_emit_cb_item (ctx, item, event, CTX_PRESS, x, y);
    if (!event->stop_propagate)
      _ctx_emit_cb_item (ctx, item, event, CTX_DRAG_PRESS, x, y);

    if (event->stop_propagate)
      l = NULL;
  }

  //events_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);
  return 0;
}


void _ctx_resized (Ctx *ctx, int width, int height, long time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_DOWN);
  CtxEvent event = {0, };

  if (!time)
    time = ctx_ms (ctx);
  
  event.ctx = ctx;
  event.time = time;
  event.string = "resize-event"; /* gets delivered to clients as a key_down event, maybe message shouldbe used instead?
   */

  if (item)
  {
    event.stop_propagate = 0;
    _ctx_emit_cb_item (ctx, item, &event, CTX_KEY_DOWN, 0, 0);
  }
}


int ctx_pointer_release (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxEvents *events = &ctx->events;
  if (time == 0)
    time = ctx_ms (ctx);

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &events->drag_event[device_no];

  event->time = time;
  event->x = x;
  event->ctx = ctx;
  event->y = y;
  event->device_no = device_no;
  event->stop_propagate = 0;

  switch (device_no)
  {
    case 1:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON1)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON1;
      break;
    case 2:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON2)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON2;
      break;
    case 3:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON3)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON3;
      break;
    default:
      break;
  }

  //events_queue_draw (mrg, NULL); /* in case of style change */

  if (events->pointer_down[device_no] == 0)
  {
    fprintf (stderr, "device %i already up\n", device_no);
  }
  events->pointer_down[device_no] = 0;

  events->pointer_x[device_no] = x;
  events->pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    events->pointer_x[0] = x;
    events->pointer_y[0] = y;
  }
  CtxList *hitlist = NULL;
  CtxList *grablist = NULL , *g= NULL;
  CtxGrab *grab;

  _ctx_update_item (ctx, device_no, x, y, CTX_RELEASE | CTX_DRAG_RELEASE, &hitlist);
  grablist = device_get_grabs (ctx, device_no);

  for (g = grablist; g; g = g->next)
  {
    grab = g->data;

    if (!event->stop_propagate)
    {
      if (grab->item->types & CTX_TAP)
      {
        long delay = time - grab->start_time;

        if (delay > events->tap_delay_min &&
            delay < events->tap_delay_max &&
            (
              (event->start_x - x) * (event->start_x - x) +
              (event->start_y - y) * (event->start_y - y)) < ctx_pow2(events->tap_hysteresis)
            )
        {
          _ctx_emit_cb_item (ctx, grab->item, event, CTX_TAP, x, y);
        }
      }

      if (!event->stop_propagate && grab->item->types & CTX_DRAG_RELEASE)
      {
        _ctx_emit_cb_item (ctx, grab->item, event, CTX_DRAG_RELEASE, x, y);
      }
    }

    device_remove_grab (ctx, grab);
  }

  if (hitlist)
  {
    if (!event->stop_propagate)
      _ctx_emit_cb (ctx, hitlist, event, CTX_RELEASE, x, y);
    ctx_list_free (&hitlist);
  }
  ctx_list_free (&grablist);
  return 0;
}

/*  for multi-touch, need a list of active grabs - thus a grab corresponds to
 *  a device id. even during drag-grabs events propagate; to stop that stop
 *e
 propagation like for other events.
 *
 *
 */
int ctx_pointer_motion (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxList *hitlist = NULL;
  CtxList *grablist = NULL, *g;
  CtxGrab *grab;

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &ctx->events.drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->ctx  = ctx;
  event->x    = x;
  event->y    = y;
  event->time = time;
  event->device_no = device_no;
  event->stop_propagate = 0;
  
  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;

  if (device_no <= 3)
  {
    ctx->events.pointer_x[0] = x;
    ctx->events.pointer_y[0] = y;
  }

  /* XXX: too brutal; should use enter/leave events */
  //if (getenv ("CTX_FAST") == NULL)
 // mrg_queue_draw (mrg, NULL); /* XXX: not really needed for all backends,
 //                                     needs more tinkering */
  grablist = device_get_grabs (ctx, device_no);
  _ctx_update_item (ctx, device_no, x, y, CTX_MOTION, &hitlist);

  event->delta_x = x - event->prev_x;
  event->delta_y = y - event->prev_y;
  event->prev_x  = x;
  event->prev_y  = y;

  CtxList *remove_grabs = NULL;

  for (g = grablist; g; g = g->next)
  {
    grab = g->data;

    if ((grab->type & CTX_TAP) ||
        (grab->type & CTX_TAP_AND_HOLD))
    {
      if (
          (
            (event->start_x - x) * (event->start_x - x) +
            (event->start_y - y) * (event->start_y - y)) >
              ctx_pow2(ctx->events.tap_hysteresis)
         )
      {
        //fprintf (stderr, "-");
        ctx_list_prepend (&remove_grabs, grab);
      }
      else
      {
        //fprintf (stderr, ":");
      }
    }

    if (grab->type & CTX_DRAG_MOTION)
    {
      _ctx_emit_cb_item (ctx, grab->item, event, CTX_DRAG_MOTION, x, y);
      if (event->stop_propagate)
        break;
    }
  }
  if (remove_grabs)
  {
    for (g = remove_grabs; g; g = g->next)
      device_remove_grab (ctx, g->data);
    ctx_list_free (&remove_grabs);
  }
  if (hitlist)
  {
    if (!event->stop_propagate)
      _ctx_emit_cb (ctx, hitlist, event, CTX_MOTION, x, y);
    ctx_list_free (&hitlist);
  }
  ctx_list_free (&grablist);
  return 0;
}

void ctx_incoming_message (Ctx *ctx, const char *message, long time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_MESSAGE);
  CtxEvent event = {0, };

  if (!time)
    time = ctx_ms (ctx);

  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_MESSAGE;
    event.time = time;
    event.string = message;

    fprintf (stderr, "{%s|\n", message);

      for (i = 0; i < item->cb_count; i++)
      {
        if (item->cb[i].types & (CTX_MESSAGE))
        {
          event.state = ctx->events.modifier_state;
          item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
          if (event.stop_propagate)
            return;// event.stop_propagate;
        }
      }
  }
}

int ctx_scrolled (Ctx *ctx, float x, float y, CtxScrollDirection scroll_direction, uint32_t time)
{
  CtxList *hitlist = NULL;
  CtxList *l;

  int device_no = 0;
  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;

  CtxEvent *event = &ctx->events.drag_event[device_no];  /* XXX: might
                                       conflict with other code
                                       create a sibling member
                                       of drag_event?*/
  if (time == 0)
    time = ctx_ms (ctx);

  event->x         = event->start_x = event->prev_x = x;
  event->y         = event->start_y = event->prev_y = y;
  event->delta_x   = event->delta_y = 0;
  event->device_no = device_no;
  event->time      = time;
  event->stop_propagate = 0;
  event->scroll_direction = scroll_direction;

  _ctx_update_item (ctx, device_no, x, y, CTX_SCROLL, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;

    _ctx_emit_cb_item (ctx, item, event, CTX_SCROLL, x, y);

    if (event->stop_propagate)
      l = NULL;
  }

  //mrg_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);
  return 0;
}

int ctx_key_press (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_DOWN);
  CtxEvent event = {0,};

  if (time == 0)
    time = ctx_ms (ctx);

  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_KEY_DOWN;
    event.unicode = keyval; 
    event.string = string;
    event.stop_propagate = 0;
    event.time = time;

    for (i = 0; i < item->cb_count; i++)
    {
      if (item->cb[i].types & (CTX_KEY_DOWN))
      {
        event.state = ctx->events.modifier_state;
        item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
#if 0
        char buf[256];
        ctx_set (ctx, ctx_strhash ("title", 0), buf, strlen(buf));
        ctx_flush (ctx);
#endif
        if (event.stop_propagate)
          return event.stop_propagate;
      }
    }
  }
  return 0;
}

void ctx_freeze           (Ctx *ctx)
{
  ctx->events.frozen ++;
}

void ctx_thaw             (Ctx *ctx)
{
  ctx->events.frozen --;
}

float ctx_pointer_x (Ctx *ctx)
{
  return ctx->events.pointer_x[0];
}

float ctx_pointer_y (Ctx *ctx)
{
  return ctx->events.pointer_y[0];
}

int ctx_pointer_is_down (Ctx *ctx, int no)
{
  if (no < 0 || no > CTX_MAX_DEVICES) return 0;
  return ctx->events.pointer_down[no];
}

void _ctx_debug_overlays (Ctx *ctx)
{
  CtxList *a;
  ctx_save (ctx);

  ctx_set_line_width (ctx, 2);
  ctx_set_rgba (ctx, 0,0,0.8,0.5);
  for (a = ctx->events.items; a; a = a->next)
  {
    float current_x = ctx_pointer_x (ctx);
    float current_y = ctx_pointer_y (ctx);
    CtxItem *item = a->data;
    CtxMatrix matrix = item->inv_matrix;

    ctx_matrix_apply_transform (&matrix, &current_x, &current_y);

    if (current_x >= item->x0 && current_x < item->x1 &&
        current_y >= item->y0 && current_y < item->y1)
    {
      ctx_matrix_invert (&matrix);
      ctx_set_matrix (ctx, &matrix);
      _mrg_restore_path (ctx, item->path);
      ctx_stroke (ctx);
    }
  }
  ctx_restore (ctx);
}

int ctx_count (Ctx *ctx)
{
  return ctx->renderstream.count;
}

#include <fcntl.h>
#include <sys/ioctl.h>

int ctx_terminal_width (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 80;
  return ws.ws_xpixel;
} 

int ctx_terminal_height (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 80;
  return ws.ws_ypixel;
} 

int ctx_terminal_cols (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 80;
  return ws.ws_col;
} 

int ctx_terminal_rows (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 25;
  return ws.ws_row;
}

static inline int _ctx_rgba8_manhattan_diff (const uint8_t *a, const uint8_t *b)
{
  int c;
  int diff = 0;
  for (c = 0; c<3;c++)
    diff += ctx_pow2(a[c]-b[c]);
  return diff;
}


//CtxOutputmode _outputmode = CTX_OUTPUT_MODE_BRAILLE;

static inline void _ctx_utf8_output_buf (uint8_t *pixels,
                          int format,
                          int width,
                          int height,
                          int stride,
                          int reverse)
{
  char *utf8_gray_scale[]= {" ","░","▒","▓","█","█", NULL};
  int no = 0;
  switch (format)
    {
      case CTX_FORMAT_GRAY2:
        {
          for (int y= 0; y < height; y++)
            {
              no = y * stride;
              for (int x = 0; x < width; x++)
                {
                  int val4= (pixels[no] & (3 << ( (x % 4) *2) ) ) >> ( (x%4) *2);
                  int val = (int) CTX_CLAMP (5.0 * val4 / 3.0, 0, 5);
                  if (!reverse)
                  { val = 5-val; }
                  printf ("%s", utf8_gray_scale[val]);
                  if ( (x % 4) == 3)
                    { no++; }
                }
              printf ("\n");
            }
        }
        break;
      case CTX_FORMAT_GRAY1:
        for (int row = 0; row < height/4; row++)
          {
            for (int col = 0; col < width /2; col++)
              {
                int unicode = 0;
                int bitno = 0;
                for (int x = 0; x < 2; x++)
                  for (int y = 0; y < 3; y++)
                    {
                      int no = (row * 4 + y) * stride + (col*2+x) /8;
                      int set = pixels[no] & (1<< ( (col * 2 + x) % 8) );
                      if (reverse) { set = !set; }
                      if (set)
                        { unicode |=  (1<< (bitno) ); }
                      bitno++;
                    }
                {
                  int x = 0;
                  int y = 3;
                  int no = (row * 4 + y) * stride + (col*2+x) /8;
                  int setA = pixels[no] & (1<< ( (col * 2 + x) % 8) );
                  no = (row * 4 + y) * stride + (col*2+x+1) /8;
                  int setB = pixels[no] & (1<< (   (col * 2 + x + 1) % 8) );
                  if (reverse) { setA = !setA; }
                  if (reverse) { setB = !setB; }
                  if (setA != 0 && setB==0)
                    { unicode += 0x2840; }
                  else if (setA == 0 && setB)
                    { unicode += 0x2880; }
                  else if ( (setA != 0) && (setB != 0) )
                    { unicode += 0x28C0; }
                  else
                    { unicode += 0x2800; }
                  uint8_t utf8[5];
                  utf8[ctx_unichar_to_utf8 (unicode, utf8)]=0;
                  printf ("%s", utf8);
                }
              }
            printf ("\n");
          }
        break;
      case CTX_FORMAT_RGBA8:
        {
        for (int row = 0; row < height/4; row++)
          {
            for (int col = 0; col < width /2; col++)
              {
                int unicode = 0;
                int bitno = 0;

                uint8_t rgba2[4] = {0,0,0,255};
                uint8_t rgba1[4] = {0,0,0,255};
                int     rgbasum[4] = {0,};
                int     col_count = 0;

                for (int xi = 0; xi < 2; xi++)
                  for (int yi = 0; yi < 4; yi++)
                      {
                        int noi = (row * 4 + yi) * stride + (col*2+xi) * 4;
                        int diff = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba2);
                        if (diff > 32*32)
                        {
                          for (int c = 0; c < 3; c++)
                          {
                            rgbasum[c] += pixels[noi+c];
                          }
                          col_count++;
                        }
                      }
                if (col_count)
                for (int c = 0; c < 3; c++)
                {
                  rgba1[c] = rgbasum[c] / col_count;
                }



                // to determine color .. find two most different
                // colors in set.. and threshold between them..
                // even better dither between them.
                //
  printf ("\e[38;2;%i;%i;%im", rgba1[0], rgba1[1], rgba1[2]);
  //printf ("\e[48;2;%i;%i;%im", rgba2[0], rgba2[1], rgba2[2]);

                for (int x = 0; x < 2; x++)
                  for (int y = 0; y < 3; y++)
                    {
                      int no = (row * 4 + y) * stride + (col*2+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba1)< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba2))

                      int set = CHECK_IS_SET;
                      if (reverse) { set = !set; }
                      if (set)
                        { unicode |=  (1<< (bitno) ); }
                      bitno++;
                    }
                {
                  int x = 0;
                  int y = 3;
                  int no = (row * 4 + y) * stride + (col*2+x) * 4;
                  int setA = CHECK_IS_SET;
                  no = (row * 4 + y) * stride + (col*2+x+1) * 4;
                  int setB = CHECK_IS_SET;
#undef CHECK_IS_SET
                  if (reverse) { setA = !setA; }
                  if (reverse) { setB = !setB; }
                  if (setA != 0 && setB==0)
                    { unicode += 0x2840; }
                  else if (setA == 0 && setB)
                    { unicode += 0x2880; }
                  else if ( (setA != 0) && (setB != 0) )
                    { unicode += 0x28C0; }
                  else
                    { unicode += 0x2800; }
                  uint8_t utf8[5];
                  utf8[ctx_unichar_to_utf8 (unicode, utf8)]=0;
                  printf ("%s", utf8);
                }
              }
            printf ("\n\r");
          }
          printf ("\e[38;2;%i;%i;%im", 255,255,255);
        }
        break;

      case CTX_FORMAT_GRAY4:
        {
          int no = 0;
          for (int y= 0; y < height; y++)
            {
              no = y * stride;
              for (int x = 0; x < width; x++)
                {
                  int val = (pixels[no] & (15 << ( (x % 2) *4) ) ) >> ( (x%2) *4);
                  val = val * 6 / 16;
                  if (reverse) { val = 5-val; }
                  val = CTX_CLAMP (val, 0, 4);
                  printf ("%s", utf8_gray_scale[val]);
                  if (x % 2 == 1)
                    { no++; }
                }
              printf ("\n");
            }
        }
        break;
      case CTX_FORMAT_CMYK8:
        {
          for (int c = 0; c < 4; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=4)
                    {
                      int val = (int) CTX_CLAMP (pixels[no+c]/255.0*6.0, 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
        break;
      case CTX_FORMAT_RGB8:
        {
          for (int c = 0; c < 3; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=3)
                    {
                      int val = (int) CTX_CLAMP (pixels[no+c]/255.0*6.0, 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
        break;
      case CTX_FORMAT_CMYKAF:
        {
          for (int c = 0; c < 5; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=5)
                    {
                      int val = (int) CTX_CLAMP ( (pixels[no+c]*6.0), 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
    }
}

enum {
  NC_MOUSE_NONE  = 0,
  NC_MOUSE_PRESS = 1,  /* "mouse-pressed", "mouse-released" */
  NC_MOUSE_DRAG  = 2,  /* + "mouse-drag"   (motion with pressed button) */
  NC_MOUSE_ALL   = 3   /* + "mouse-motion" (also delivered for release) */
};

#define DECTCEM_CURSOR_SHOW      "\033[?25h"
#define DECTCEM_CURSOR_HIDE      "\033[?25l"
#define TERMINAL_MOUSE_OFF       "\033[?1000l\033[?1003l"
#define TERMINAL_MOUSE_ON_BASIC  "\033[?1000h"
#define TERMINAL_MOUSE_ON_DRAG   "\033[?1000h\033[?1003h" /* +ON_BASIC for wider */
#define TERMINAL_MOUSE_ON_FULL   "\033[?1000h\033[?1004h" /* compatibility */
#define XTERM_ALTSCREEN_ON       "\033[?47h"
#define XTERM_ALTSCREEN_OFF      "\033[?47l"

/*************************** input handling *************************/

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#define DELAY_MS  100  

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

static int  size_changed = 0;       /* XXX: global state */
static int  signal_installed = 0;   /* XXX: global state */

static const char *mouse_modes[]=
{TERMINAL_MOUSE_OFF,
 TERMINAL_MOUSE_ON_BASIC,
 TERMINAL_MOUSE_ON_DRAG,
 TERMINAL_MOUSE_ON_FULL,
 NULL};

/* note that a nick can have multiple occurences, the labels
 * should be kept the same for all occurences of a combination. */
typedef struct NcKeyCode {
  char *nick;          /* programmers name for key (combo) */
  char *label;         /* utf8 label for key */
  char  sequence[10];  /* terminal sequence */
} NcKeyCode;
static const NcKeyCode keycodes[]={  
  {"up",                  "↑",     "\033[A"},
  {"down",                "↓",     "\033[B"},
  {"right",               "→",     "\033[C"},
  {"left",                "←",     "\033[D"},

  {"shift-up",            "⇧↑",    "\033[1;2A"},
  {"shift-down",          "⇧↓",    "\033[1;2B"},
  {"shift-right",         "⇧→",    "\033[1;2C"},
  {"shift-left",          "⇧←",    "\033[1;2D"},

  {"alt-up",              "^↑",    "\033[1;3A"},
  {"alt-down",            "^↓",    "\033[1;3B"},
  {"alt-right",           "^→",    "\033[1;3C"},
  {"alt-left",            "^←",    "\033[1;3D"},

  {"alt-shift-up",        "alt-s↑", "\033[1;4A"},
  {"alt-shift-down",      "alt-s↓", "\033[1;4B"},
  {"alt-shift-right",     "alt-s→", "\033[1;4C"},
  {"alt-shift-left",      "alt-s←", "\033[1;4D"},

  {"control-up",          "^↑",    "\033[1;5A"},
  {"control-down",        "^↓",    "\033[1;5B"},
  {"control-right",       "^→",    "\033[1;5C"},
  {"control-left",        "^←",    "\033[1;5D"},

  /* putty */
  {"control-up",          "^↑",    "\033OA"},
  {"control-down",        "^↓",    "\033OB"},
  {"control-right",       "^→",    "\033OC"},
  {"control-left",        "^←",    "\033OD"},

  {"control-shift-up",    "^⇧↑",   "\033[1;6A"},
  {"control-shift-down",  "^⇧↓",   "\033[1;6B"},
  {"control-shift-right", "^⇧→",   "\033[1;6C"},
  {"control-shift-left",  "^⇧←",   "\033[1;6D"},

  {"control-up",          "^↑",    "\033Oa"},
  {"control-down",        "^↓",    "\033Ob"},
  {"control-right",       "^→",    "\033Oc"},
  {"control-left",        "^←",    "\033Od"},

  {"shift-up",            "⇧↑",    "\033[a"},
  {"shift-down",          "⇧↓",    "\033[b"},
  {"shift-right",         "⇧→",    "\033[c"},
  {"shift-left",          "⇧←",    "\033[d"},

  {"insert",              "ins",   "\033[2~"},
  {"delete",              "del",   "\033[3~"},
  {"page-up",             "PgUp",  "\033[5~"},
  {"page-down",           "PdDn",  "\033[6~"},
  {"home",                "Home",  "\033OH"},
  {"end",                 "End",   "\033OF"},
  {"home",                "Home",  "\033[H"},
  {"end",                 "End",   "\033[F"},
  {"control-delete",      "^del",  "\033[3;5~"},
  {"shift-delete",        "⇧del",  "\033[3;2~"},
  {"control-shift-delete","^⇧del", "\033[3;6~"},

  {"F1",        "F1",  "\033[11~"},
  {"F2",        "F2",  "\033[12~"},
  {"F3",        "F3",  "\033[13~"},
  {"F4",        "F4",  "\033[14~"},
  {"F1",        "F1",  "\033OP"},
  {"F2",        "F2",  "\033OQ"},
  {"F3",        "F3",  "\033OR"},
  {"F4",        "F4",  "\033OS"},
  {"F5",        "F5",  "\033[15~"},
  {"F6",        "F6",  "\033[16~"},
  {"F7",        "F7",  "\033[17~"},
  {"F8",        "F8",  "\033[18~"},
  {"F9",        "F9",  "\033[19~"},
  {"F9",        "F9",  "\033[20~"},
  {"F10",       "F10", "\033[21~"},
  {"F11",       "F11", "\033[22~"},
  {"F12",       "F12", "\033[23~"},
  {"tab",       "↹",  {9, '\0'}},
  {"shift-tab", "shift+↹",  "\033[Z"},
  {"backspace", "⌫",  {127, '\0'}},
  {"space",     "␣",   " "},
  {"esc",        "␛",  "\033"},
  {"return",    "⏎",  {10,0}},
  {"return",    "⏎",  {13,0}},
  /* this section could be autogenerated by code */
  {"control-a", "^A",  {1,0}},
  {"control-b", "^B",  {2,0}},
  {"control-c", "^C",  {3,0}},
  {"control-d", "^D",  {4,0}},
  {"control-e", "^E",  {5,0}},
  {"control-f", "^F",  {6,0}},
  {"control-g", "^G",  {7,0}},
  {"control-h", "^H",  {8,0}}, /* backspace? */
  {"control-i", "^I",  {9,0}},
  {"control-j", "^J",  {10,0}},
  {"control-k", "^K",  {11,0}},
  {"control-l", "^L",  {12,0}},
  {"control-n", "^N",  {14,0}},
  {"control-o", "^O",  {15,0}},
  {"control-p", "^P",  {16,0}},
  {"control-q", "^Q",  {17,0}},
  {"control-r", "^R",  {18,0}},
  {"control-s", "^S",  {19,0}},
  {"control-t", "^T",  {20,0}},
  {"control-u", "^U",  {21,0}},
  {"control-v", "^V",  {22,0}},
  {"control-w", "^W",  {23,0}},
  {"control-x", "^X",  {24,0}},
  {"control-y", "^Y",  {25,0}},
  {"control-z", "^Z",  {26,0}},
  {"alt-0",     "%0",  "\0330"},
  {"alt-1",     "%1",  "\0331"},
  {"alt-2",     "%2",  "\0332"},
  {"alt-3",     "%3",  "\0333"},
  {"alt-4",     "%4",  "\0334"},
  {"alt-5",     "%5",  "\0335"},
  {"alt-6",     "%6",  "\0336"},
  {"alt-7",     "%7",  "\0337"}, /* backspace? */
  {"alt-8",     "%8",  "\0338"},
  {"alt-9",     "%9",  "\0339"},
  {"alt-+",     "%+",  "\033+"},
  {"alt--",     "%-",  "\033-"},
  {"alt-/",     "%/",  "\033/"},
  {"alt-a",     "%A",  "\033a"},
  {"alt-b",     "%B",  "\033b"},
  {"alt-c",     "%C",  "\033c"},
  {"alt-d",     "%D",  "\033d"},
  {"alt-e",     "%E",  "\033e"},
  {"alt-f",     "%F",  "\033f"},
  {"alt-g",     "%G",  "\033g"},
  {"alt-h",     "%H",  "\033h"}, /* backspace? */
  {"alt-i",     "%I",  "\033i"},
  {"alt-j",     "%J",  "\033j"},
  {"alt-k",     "%K",  "\033k"},
  {"alt-l",     "%L",  "\033l"},
  {"alt-n",     "%N",  "\033m"},
  {"alt-n",     "%N",  "\033n"},
  {"alt-o",     "%O",  "\033o"},
  {"alt-p",     "%P",  "\033p"},
  {"alt-q",     "%Q",  "\033q"},
  {"alt-r",     "%R",  "\033r"},
  {"alt-s",     "%S",  "\033s"},
  {"alt-t",     "%T",  "\033t"},
  {"alt-u",     "%U",  "\033u"},
  {"alt-v",     "%V",  "\033v"},
  {"alt-w",     "%W",  "\033w"},
  {"alt-x",     "%X",  "\033x"},
  {"alt-y",     "%Y",  "\033y"},
  {"alt-z",     "%Z",  "\033z"},
  /* Linux Console  */
  {"home",      "Home", "\033[1~"},
  {"end",       "End",  "\033[4~"},
  {"F1",        "F1",   "\033[[A"},
  {"F2",        "F2",   "\033[[B"},
  {"F3",        "F3",   "\033[[C"},
  {"F4",        "F4",   "\033[[D"},
  {"F5",        "F5",   "\033[[E"},
  {"F6",        "F6",   "\033[[F"},
  {"F7",        "F7",   "\033[[G"},
  {"F8",        "F8",   "\033[[H"},
  {"F9",        "F9",   "\033[[I"},
  {"F10",       "F10",  "\033[[J"},
  {"F11",       "F11",  "\033[[K"},
  {"F12",       "F12",  "\033[[L"}, 
  {NULL, }
};

static struct termios orig_attr;    /* in order to restore at exit */
static int    nc_is_raw = 0;
static int    atexit_registered = 0;
static int    mouse_mode = NC_MOUSE_NONE;

static void _nc_noraw (void)
{
  if (nc_is_raw && tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr) != -1)
    nc_is_raw = 0;
}

static void
nc_at_exit (void)
{
  printf (TERMINAL_MOUSE_OFF);
  printf (XTERM_ALTSCREEN_OFF);
  _nc_noraw();
  fprintf (stdout, "\e[2J\e[H\e[?25h");
  //if (ctx_native_events)
  fprintf (stdout, "\e[?6150l");
}

static int _nc_raw (void)
{
  struct termios raw;
  if (!isatty (STDIN_FILENO))
    return -1;
  if (!atexit_registered)
    {
      atexit (nc_at_exit);
      atexit_registered = 1;
    }
  if (tcgetattr (STDIN_FILENO, &orig_attr) == -1)
    return -1;
  raw = orig_attr;  /* modify the original mode */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return -1;
  nc_is_raw = 1;
  tcdrain(STDIN_FILENO);
  tcflush(STDIN_FILENO, 1);
  return 0;
}

static int match_keycode (const char *buf, int length, const NcKeyCode **ret)
{
  int i;
  int matches = 0;

  if (!strncmp (buf, "\033[M", MIN(length,3)))
    {
      if (length >= 6)
        return 9001;
      return 2342;
    }
  for (i = 0; keycodes[i].nick; i++)
    if (!strncmp (buf, keycodes[i].sequence, length))
      {
        matches ++;
        if ((int)strlen (keycodes[i].sequence) == length && ret)
          {
            *ret = &keycodes[i];
            return 1;
          }
      }
  if (matches != 1 && ret)
    *ret = NULL;
  return matches==1?2:matches;
}

static void nc_resize_term (int  dummy)
{
  size_changed = 1;
}

int ctx_has_event (Ctx  *n, int delay_ms)
{
  struct timeval tv;
  int retval;
  fd_set rfds;

  if (size_changed)
    return 1;
  FD_ZERO (&rfds);
  FD_SET (STDIN_FILENO, &rfds);
  tv.tv_sec = 0; tv.tv_usec = delay_ms * 1000; 
  retval = select (1, &rfds, NULL, NULL, &tv);
  if (size_changed)
    return 1;
  return retval == 1 && retval != -1;
}

static const char *mouse_get_event_int (Ctx *n, int *x, int *y)
{
  static int prev_state = 0;
  const char *ret = "mouse-motion";
  float relx, rely;
  signed char buf[3];
  read (n->mouse_fd, buf, 3);
  relx = buf[1];
  rely = -buf[2];

  n->mouse_x += relx * 0.1;
  n->mouse_y += rely * 0.1;

  if (n->mouse_x < 1) n->mouse_x = 1;
  if (n->mouse_y < 1) n->mouse_y = 1;
  if (n->mouse_x >= n->events.width)  n->mouse_x = n->events.width;
  if (n->mouse_y >= n->events.height) n->mouse_y = n->events.height;

  if (x) *x = n->mouse_x;
  if (y) *y = n->mouse_y;

  if ((prev_state & 1) != (buf[0] & 1))
    {
      if (buf[0] & 1) ret = "mouse-press";
    }
  else if (buf[0] & 1)
    ret = "mouse-drag";

  if ((prev_state & 2) != (buf[0] & 2))
    {
      if (buf[0] & 2) ret = "mouse2-press";
    }
  else if (buf[0] & 2)
    ret = "mouse2-drag";

  if ((prev_state & 4) != (buf[0] & 4))
    {
      if (buf[0] & 4) ret = "mouse1-press";
    }
  else if (buf[0] & 4)
    ret = "mouse1-drag";

  prev_state = buf[0];
  return ret;
}

static const char *mev_type = NULL;
static int         mev_x = 0;
static int         mev_y = 0;
static int         mev_q = 0;

static const char *mouse_get_event (Ctx  *n, int *x, int *y)
{
  if (!mev_q)
    return NULL;
  *x = mev_x;
  *y = mev_y;
  mev_q = 0;
  return mev_type;
}

static int mouse_has_event (Ctx *n)
{
  struct timeval tv;
  int retval;

  if (mouse_mode == NC_MOUSE_NONE)
    return 0;

  if (mev_q)
    return 1;

  if (n->mouse_fd == 0)
    return 0;
  return 0;

  {
    fd_set rfds;
    FD_ZERO (&rfds);
    FD_SET(n->mouse_fd, &rfds);
    tv.tv_sec = 0; tv.tv_usec = 0;
    retval = select (n->mouse_fd+1, &rfds, NULL, NULL, &tv);
  }

  if (retval != 0)
    {
      int nx = 0, ny = 0;
      const char *type = mouse_get_event_int (n, &nx, &ny);

      if ((mouse_mode < NC_MOUSE_DRAG && mev_type && !strcmp (mev_type, "drag")) ||
          (mouse_mode < NC_MOUSE_ALL && mev_type && !strcmp (mev_type, "motion")))
        {
          mev_q = 0;
          return mouse_has_event (n);
        }

      if ((mev_type && !strcmp (type, mev_type) && !strcmp (type, "mouse-motion")) ||
         (mev_type && !strcmp (type, mev_type) && !strcmp (type, "mouse1-drag")) ||
         (mev_type && !strcmp (type, mev_type) && !strcmp (type, "mouse2-drag")))
        {
          if (nx == mev_x && ny == mev_y)
          {
            mev_q = 0;
            return mouse_has_event (n);
          }
        }
      mev_x = nx;
      mev_y = ny;
      mev_type = type;
      mev_q = 1;
    }
  return retval != 0;
}

const char *ctx_nct_get_event (Ctx *n, int timeoutms, int *x, int *y)
{
  unsigned char buf[20];
  int length;


  if (x) *x = -1;
  if (y) *y = -1;

  if (!signal_installed)
    {
      _nc_raw ();
      signal_installed = 1;
      signal (SIGWINCH, nc_resize_term);
    }
  if (mouse_mode) // XXX too often to do it all the time!
    printf(mouse_modes[mouse_mode]);

  {
    int elapsed = 0;
    int got_event = 0;

    do {
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      got_event = mouse_has_event (n);
      if (!got_event)
        got_event = ctx_has_event (n, MIN(DELAY_MS, timeoutms-elapsed));
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      /* only do this if the client has asked for idle events,
       * and perhaps programmed the ms timer?
       */
      elapsed += MIN(DELAY_MS, timeoutms-elapsed);
      if (!got_event && timeoutms && elapsed >= timeoutms)
        return "idle";
    } while (!got_event);
  }

  if (mouse_has_event (n))
    return mouse_get_event (n, x, y);

  for (length = 0; length < 10; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
        const NcKeyCode *match = NULL;

        /* special case ESC, so that we can use it alone in keybindings */
        if (length == 0 && buf[0] == 27)
          {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO (&rfds);
            FD_SET (STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * DELAY_MS;
            if (select (1, &rfds, NULL, NULL, &tv) == 0)
              return "esc";
          }

        switch (match_keycode ((void*)buf, length + 1, &match))
          {
            case 1: /* unique match */
              if (!match)
                return NULL;
              return match->nick;
              break;
            case 9001: /* mouse event */
              if (x) *x = ((unsigned char)buf[4]-32)*1.0;
              if (y) *y = ((unsigned char)buf[5]-32)*1.0;
              switch (buf[3])
                {
                  case 32: return "mouse-press";
                  case 33: return "mouse1-press";
                  case 34: return "mouse2-press";
                  case 40: return "alt-mouse-press";
                  case 41: return "alt-mouse1-press";
                  case 42: return "alt-mouse2-press";
                  case 48: return "control-mouse-press";
                  case 49: return "control-mouse1-press";
                  case 50: return "control-mouse2-press";
                  case 56: return "alt-control-mouse-press";
                  case 57: return "alt-control-mouse1-press";
                  case 58: return "alt-control-mouse2-press";
                  case 64: return "mouse-drag";
                  case 65: return "mouse1-drag";
                  case 66: return "mouse2-drag";
                  case 71: return "mouse-motion"; /* shift+motion */
                  case 72: return "alt-mouse-drag";
                  case 73: return "alt-mouse1-drag";
                  case 74: return "alt-mouse2-drag";
                  case 75: return "mouse-motion"; /* alt+motion */
                  case 80: return "control-mouse-drag";
                  case 81: return "control-mouse1-drag";
                  case 82: return "control-mouse2-drag";
                  case 83: return "mouse-motion"; /* ctrl+motion */
                  case 91: return "mouse-motion"; /* ctrl+alt+motion */
                  case 95: return "mouse-motion"; /* ctrl+alt+shift+motion */
                  case 96: return "scroll-up";
                  case 97: return "scroll-down";
                  case 100: return "shift-scroll-up";
                  case 101: return "shift-scroll-down";
                  case 104: return "alt-scroll-up";
                  case 105: return "alt-scroll-down";
                  case 112: return "control-scroll-up";
                  case 113: return "control-scroll-down";
                  case 116: return "control-shift-scroll-up";
                  case 117: return "control-shift-scroll-down";
                  case 35: /* (or release) */
                  case 51: /* (or ctrl-release) */
                  case 43: /* (or alt-release) */
                  case 67: return "mouse-motion";
                           /* have a separate mouse-drag ? */
                  default: {
                             static char rbuf[100];
                             sprintf (rbuf, "mouse (unhandled state: %i)", buf[3]);
                             return rbuf;
                           }
                }
            case 0: /* no matches, bail*/
              { 
                static char ret[256];
                if (length == 0 && ctx_utf8_len (buf[0])>1) /* single unicode
                                                               char */
                  {
                    read (STDIN_FILENO, &buf[length+1], ctx_utf8_len(buf[0])-1);
                    buf[ctx_utf8_len(buf[0])]=0;
                    strcpy (ret, (void*)buf);
                    return ret;
                  }
                if (length == 0) /* ascii */
                  {
                    buf[1]=0;
                    strcpy (ret, (void*)buf);
                    return ret;
                  }
                sprintf (ret, "unhandled %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c'",
                  length>=0? buf[0]: 0, length>=0? buf[0]>31?buf[0]:'?': ' ', 
                  length>=1? buf[1]: 0, length>=1? buf[1]>31?buf[1]:'?': ' ', 
                  length>=2? buf[2]: 0, length>=2? buf[2]>31?buf[2]:'?': ' ', 
                  length>=3? buf[3]: 0, length>=3? buf[3]>31?buf[3]:'?': ' ',
                  length>=4? buf[4]: 0, length>=4? buf[4]>31?buf[4]:'?': ' ',
                  length>=5? buf[5]: 0, length>=5? buf[5]>31?buf[5]:'?': ' ',
                  length>=6? buf[6]: 0, length>=6? buf[6]>31?buf[6]:'?': ' ');
                return ret;
              }
              return NULL;
            default: /* continue */
              break;
          }
      }
    else
      return "key read eek";
  return "fail";
}

const char *ctx_native_get_event (Ctx *n, int timeoutms)
{
  static unsigned char buf[256];
  int length;

  if (!signal_installed)
    {
      _nc_raw ();
      signal_installed = 1;
      signal (SIGWINCH, nc_resize_term);
    }
//if (mouse_mode) // XXX too often to do it all the time!
//  printf(mouse_modes[mouse_mode]);

  {
    int elapsed = 0;
    int got_event = 0;

    do {
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      got_event = ctx_has_event (n, MIN(DELAY_MS, timeoutms-elapsed));
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      /* only do this if the client has asked for idle events,
       * and perhaps programmed the ms timer?
       */
      elapsed += MIN(DELAY_MS, timeoutms-elapsed);
      if (!got_event && timeoutms && elapsed >= timeoutms)
      {
        return "idle";
      }
    } while (!got_event);
  }

  for (length = 0; length < 200; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
         if (buf[length]=='\n')
         {
           buf[length]=0;
           return (void*)buf;
         }
      }
  return NULL;
}

typedef struct _CtxBraille CtxBraille;

struct _CtxBraille
{
   void (*render) (void *braille, CtxCommand *command);
   void (*flush)  (void *braille);
   void (*free)   (void *braille);
   Ctx     *ctx;
   int      width;
   int      height;
   int      cols;
   int      rows;
   uint8_t *pixels;
   Ctx     *host;
   int      was_down;
};

int mrg_nct_consume_events (Ctx *ctx)
{
  int ix, iy;
  CtxBraille *braille = (void*)ctx->renderer;
  const char *event = NULL;

  {
    float x, y;
    event = ctx_nct_get_event (ctx, 50, &ix, &iy);

    x = (ix - 1.0 + 0.5) / braille->cols * ctx->events.width;
    y = (iy - 1.0)       / braille->rows * ctx->events.height;

    if (!strcmp (event, "mouse-press"))
    {
      ctx_pointer_press (ctx, x, y, 0, 0);
      braille->was_down = 1;
    } else if (!strcmp (event, "mouse-release"))
    {
      ctx_pointer_release (ctx, x, y, 0, 0);
    } else if (!strcmp (event, "mouse-motion"))
    {
      //nct_set_cursor_pos (backend->term, ix, iy);
      //nct_flush (backend->term);
      if (braille->was_down)
      {
        ctx_pointer_release (ctx, x, y, 0, 0);
        braille->was_down = 0;
      }
      ctx_pointer_motion (ctx, x, y, 0, 0);
    } else if (!strcmp (event, "mouse-drag"))
    {
      ctx_pointer_motion (ctx, x, y, 0, 0);
    } else if (!strcmp (event, "size-changed"))
    {
#if 0
      int width = nct_sys_terminal_width ();
      int height = nct_sys_terminal_height ();
      nct_set_size (backend->term, width, height);
      width *= CPX;
      height *= CPX;
      free (mrg->glyphs);
      free (mrg->styles);
      free (backend->nct_pixels);
      backend->nct_pixels = calloc (width * height * 4, 1);
      mrg->glyphs = calloc ((width/CPX) * (height/CPX) * 4, 1);
      mrg->styles = calloc ((width/CPX) * (height/CPX) * 1, 1);
      mrg_set_size (mrg, width, height);
      mrg_queue_draw (mrg, NULL);
#endif
    }
    else
    {
      if (!strcmp (event, "esc"))
        ctx_key_press (ctx, 0, "escape", 0);
      else if (!strcmp (event, "space"))
        ctx_key_press (ctx, 0, "space", 0);
      else if (!strcmp (event, "enter"))
        ctx_key_press (ctx, 0, "\n", 0);
      else if (!strcmp (event, "return"))
        ctx_key_press (ctx, 0, "\n", 0);
      else
      ctx_key_press (ctx, 0, event, 0);
    }
  }

  return 1;
}

int mrg_ctx_consume_events (Ctx *ctx)
{
  int ix, iy;
  CtxBraille *braille = (void*)ctx->renderer;
  const char *event = NULL;
  if (ctx_native_events)
    {
      float x = 0, y = 0;
      char event_type[128]="";
      event = ctx_native_get_event (ctx, 50);
      {
      FILE *file = fopen ("/tmp/log", "a");
      fprintf (file, "[%s]\n", event);
      fclose (file);
      }
      if (event)
      {
      sscanf (event, "%s %f %f", event_type, &x, &y);
      if (!strcmp (event_type, "idle"))
      {
      }
      else if (!strcmp (event_type, "mouse-press"))
      {
        ctx_pointer_press (ctx, x, y, 1, 0);
      }
      else if (!strcmp (event_type, "mouse-drag")||
               !strcmp (event_type, "mouse-motion"))
      {
        ctx_pointer_motion (ctx, x, y, 1, 0);
      }
      else if (!strcmp (event_type, "mouse-release"))
      {
        ctx_pointer_release (ctx, x, y, 1, 0);
      }
      else if (!strcmp (event_type, "message"))
      {
        ctx_incoming_message (ctx, event + strlen ("message"), 0);
      }
      else
      {
        ctx_key_press (ctx, 0, event, 0);
      }
      }
    }
  else
    {
      float x, y;
      event = ctx_nct_get_event (ctx, 50, &ix, &iy);

      x = (ix - 1.0 + 0.5) / braille->cols * ctx->events.width;
      y = (iy - 1.0)       / braille->rows * ctx->events.height;

      if (!strcmp (event, "mouse-press"))
      {
        ctx_pointer_press (ctx, x, y, 0, 0);
        braille->was_down = 1;
      } else if (!strcmp (event, "mouse-release"))
      {
        ctx_pointer_release (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "mouse-motion"))
      {
        //nct_set_cursor_pos (backend->term, ix, iy);
        //nct_flush (backend->term);
        if (braille->was_down)
        {
          ctx_pointer_release (ctx, x, y, 0, 0);
          braille->was_down = 0;
        }
        ctx_pointer_motion (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "mouse-drag"))
      {
        ctx_pointer_motion (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "size-changed"))
      {
#if 0
        int width = nct_sys_terminal_width ();
        int height = nct_sys_terminal_height ();
        nct_set_size (backend->term, width, height);
        width *= CPX;
        height *= CPX;
        free (mrg->glyphs);
        free (mrg->styles);
        free (backend->nct_pixels);
        backend->nct_pixels = calloc (width * height * 4, 1);
        mrg->glyphs = calloc ((width/CPX) * (height/CPX) * 4, 1);
        mrg->styles = calloc ((width/CPX) * (height/CPX) * 1, 1);
        mrg_set_size (mrg, width, height);
        mrg_queue_draw (mrg, NULL);
#endif
      }
      else
      {
        if (!strcmp (event, "esc"))
          ctx_key_press (ctx, 0, "escape", 0);
        else if (!strcmp (event, "space"))
          ctx_key_press (ctx, 0, "space", 0);
        else if (!strcmp (event, "enter"))
          ctx_key_press (ctx, 0, "\n", 0);
        else if (!strcmp (event, "return"))
          ctx_key_press (ctx, 0, "\n", 0);
        else
        ctx_key_press (ctx, 0, event, 0);
      }
    }

  return 1;
}


const char *ctx_key_get_label (Ctx  *n, const char *nick)
{
  int j;
  int found = -1;
  for (j = 0; keycodes[j].nick; j++)
    if (found == -1 && !strcmp (keycodes[j].nick, nick))
      return keycodes[j].label;
  return NULL;
}

static void _ctx_mouse (Ctx *term, int mode)
{
  //if (term->is_st && mode > 1)
  //  mode = 1;
  if (mode != mouse_mode)
  {
    printf (mouse_modes[mode]);
    fflush (stdout);
  }
  mouse_mode = mode;
}


static void ctx_braille_flush (CtxBraille *braille)
{
  int width =  braille->width;
  int height = braille->height;
  ctx_render_ctx (braille->ctx, braille->host);
  printf ("\e[H");
  _ctx_utf8_output_buf (braille->pixels,
                        CTX_FORMAT_RGBA8,
                        width, height, width * 4, 0);
}

void ctx_braille_free (CtxBraille *braille)
{
  nc_at_exit ();
  free (braille->pixels);
  ctx_free (braille->host);
  free (braille);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

Ctx *ctx_new_braille (int width, int height)
{
  Ctx *ctx = ctx_new ();
  CtxBraille *braille = calloc (sizeof (CtxBraille), 1);
  if (width <= 0 || height <= 0)
  {
    width  = ctx_terminal_cols  () * 2;
    height = (ctx_terminal_rows ()-1) * 4;
  }
  braille->ctx = ctx;
  braille->width  = width;
  braille->height = height;
  braille->cols = (width + 1) / 2;
  braille->rows = (height + 3) / 4;
  braille->pixels = (uint8_t*)malloc (width * height * 4);
  braille->host = ctx_new_for_framebuffer (braille->pixels,
                  width, height,
                  width * 4, CTX_FORMAT_RGBA8);
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, braille);
  ctx_set_size (ctx, width, height);
 // ctx_set_size (braille->host, width, height);
  braille->flush = (void*)ctx_braille_flush;
  braille->free  = (void*)ctx_braille_free;
  return ctx;
}

typedef struct _CtxCtx CtxCtx;

struct _CtxCtx
{
   void (*render) (void *ctxctx, CtxCommand *command);
   void (*flush)  (void *ctxctx);
   void (*free)   (void *ctxctx);
   Ctx *ctx;
   int  width;
   int  height;
   int  cols;
   int  rows;
};

static void ctx_ctx_flush (CtxCtx *ctxctx)
{
  if (ctx_native_events)
    fprintf (stdout, "\e[?6150h");
  fprintf (stdout, "\e[2J\e[H\e[?25l\e[?7020h reset\n");
  ctx_render_stream (ctxctx->ctx, stdout, 0);
  fprintf (stdout, "\ndone\n\e");
  fflush (stdout);
}

void ctx_ctx_free (CtxCtx *ctxctx)
{
  nc_at_exit ();
  free (ctxctx);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}


Ctx *ctx_new_ctx (int width, int height)
{
  Ctx *ctx = ctx_new ();
  CtxCtx *ctxctx = calloc (sizeof (CtxCtx), 1);
  ctx_native_events = 1;
  if (width <= 0 || height <= 0)
  {
    ctxctx->cols = ctx_terminal_cols ();
    ctxctx->rows = ctx_terminal_rows ();
    width = ctxctx->width = ctx_terminal_width ();
    height = ctxctx->height = ctx_terminal_height ();
  }
  else
  {
    ctxctx->width  = width;
    ctxctx->height = height;
    ctxctx->cols = width / 80;
    ctxctx->rows = height / 24;
  }
  ctxctx->ctx = ctx;
  if (!ctx_native_events)
    _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, ctxctx);
  ctx_set_size (ctx, width, height);
  ctxctx->flush = (void*)ctx_ctx_flush;
  ctxctx->free  = (void*)ctx_ctx_free;
  return ctx;
}

Ctx *ctx_new_ui (int width, int height)
{
  if (getenv ("CTX_VERSION"))
          // full blown ctx - in terminal or standalone
    return ctx_new_ctx (width, height);
  else
          // braille in terminal
    return ctx_new_braille (width, height);
 
  //
  // look for ctx in terminal <
  // look for linux console
  // look for kitty image protocol in terminal
  // look for iterm2 image protocol in terminal
  // look for sixels in terminal
  // use braille
}

#endif


#endif


