
#if CTX_IMPLEMENTATION|CTX_COMPOSITOR

#ifndef __CTX_INTERNAL_H
#define __CTX_INTERNAL_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/select.h> 

typedef struct _CtxRasterizer CtxRasterizer;
typedef struct _CtxGState     CtxGState;
typedef struct _CtxState      CtxState;

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
#if CTX_BABL
  const Babl *space;
#else
  void   *space; // gets copied from state when color is declared
#endif
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


enum _CtxSourceType
{
  CTX_SOURCE_COLOR = 0,
  CTX_SOURCE_IMAGE,
  CTX_SOURCE_LINEAR_GRADIENT,
  CTX_SOURCE_RADIAL_GRADIENT,
};

typedef enum _CtxSourceType CtxSourceType;

typedef struct _CtxPixelFormatInfo CtxPixelFormatInfo;


struct _CtxBuffer
{
  void               *data;
  int                 width;
  int                 height;
  int                 stride;
  int                 revision; // XXX NYI, number to update when contents change
                                //
  CtxPixelFormatInfo *format;
  void (*free_func) (void *pixels, void *user_data);
  void               *user_data;
};

//void _ctx_user_to_device          (CtxState *state, float *x, float *y);
//void _ctx_user_to_device_distance (CtxState *state, float *x, float *y);

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
      float rdelta;
    } linear_gradient;
    struct
    {
      float x0;
      float y0;
      float r0;
      float x1;
      float y1;
      float r1;
      float rdelta;
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
  uint8_t       global_alpha_u8;

  float         line_width;
  float         miter_limit;
  float         font_size;
#if CTX_ENABLE_SHADOW_BLUR
  float         shadow_blur;
  float         shadow_offset_x;
  float         shadow_offset_y;
#endif
  int           clipped:1;

  int16_t       clip_min_x;
  int16_t       clip_min_y;
  int16_t       clip_max_x;
  int16_t       clip_max_y;

#if CTX_ENABLE_CM
#if CTX_BABL
  const Babl   *device_space;
  const Babl   *rgb_space;       
  const Babl   *cmyk_space;
#else
  void         *device_space;
  void         *rgb_space;       
  void         *cmyk_space;
#endif
#endif
  CtxCompositingMode  compositing_mode; // bitfield refs lead to
  CtxBlend                  blend_mode; // non-vectorization

  CtxColorModel color_model;
  /* bitfield-pack small state-parts */
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



#define CTX_MAX_KEYDB 64 // number of entries in keydb
                         // entries are "copy-on-change" between states

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
  int           has_clipped:1;
  float         x;
  float         y;
  int           min_x;
  int           min_y;
  int           max_x;
  int           max_y;
  CtxKeyDbEntry keydb[CTX_MAX_KEYDB];
  char          stringpool[CTX_STRINGPOOL_SIZE];
#if CTX_GRADIENTS
  CtxGradient   gradient; /* we keep only one gradient,
                             this goes icky with multiple
                             restores - it should really be part of
                             graphics state..
                             XXX, with the stringpool gradients
                             can be stored there.
                           */
#endif
  int16_t       gstate_no;
  CtxGState     gstate;
  CtxGState     gstate_stack[CTX_MAX_STATES];//at end, so can be made dynamic
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


enum _CtxIteratorFlag
{
  CTX_ITERATOR_FLAT           = 0,
  CTX_ITERATOR_EXPAND_BITPACK = 2,
  CTX_ITERATOR_DEFAULTS       = CTX_ITERATOR_EXPAND_BITPACK
};
typedef enum _CtxIteratorFlag CtxIteratorFlag;


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

// include list implementation - since it already is a header+inline online
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
  void              *backend;
#if CTX_EVENTS 
  int                quit;
  int                dirty;
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


void ctx_process (Ctx *ctx, CtxEntry *entry);
CtxBuffer *ctx_buffer_new (int width, int height,
                           CtxPixelFormat pixel_format);
void ctx_buffer_free (CtxBuffer *buffer);

void
ctx_state_gradient_clear_stops (CtxState *state);


void ctx_interpret_style         (CtxState *state, CtxEntry *entry, void *data);
void ctx_interpret_transforms    (CtxState *state, CtxEntry *entry, void *data);
void ctx_interpret_pos           (CtxState *state, CtxEntry *entry, void *data);
void ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data);


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
  CtxPixelFormat composite_format;

  void         (*to_comp) (CtxRasterizer *r,
                           int x, const void * __restrict__ src, uint8_t * __restrict__ comp, int count);
  void         (*from_comp) (CtxRasterizer *r,
                             int x, const uint8_t * __restrict__ comp, void *__restrict__ dst, int count);
  void         (*apply_coverage) (CtxRasterizer *r, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, int x, uint8_t *coverage,
                          int count);
  void         (*setup) (CtxRasterizer *r);
};

CTX_STATIC void
_ctx_user_to_device (CtxState *state, float *x, float *y);
CTX_STATIC void
_ctx_user_to_device_distance (CtxState *state, float *x, float *y);
CTX_STATIC void ctx_state_init (CtxState *state);
void
ctx_interpret_pos_bare (CtxState *state, CtxEntry *entry, void *data);
void
ctx_renderstream_deinit (CtxRenderstream *renderstream);

CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format);


int ctx_utf8_len (const unsigned char first_byte);
const char *ctx_utf8_skip (const char *s, int utf8_length);
int ctx_utf8_strlen (const char *s);
int
ctx_unichar_to_utf8 (uint32_t  ch,
                     uint8_t  *dest);

uint32_t
ctx_utf8_to_unichar (const char *input);
typedef struct _CtxHasher CtxHasher;

typedef struct CtxEdge
{
#if CTX_BLOATY_FAST_PATHS
  uint32_t index;  // provide for more aligned memory accesses.
  uint32_t pad;    //
#else
  uint16_t index;
#endif
  int32_t  x;     /* the center-line intersection      */
  int32_t  dx;
} CtxEdge;

typedef void (*CtxFragment) (CtxRasterizer *rasterizer, float x, float y, void *out);

#define CTX_MAX_GAUSSIAN_KERNEL_DIM    512

struct _CtxRasterizer
{
  CtxImplementation vfuncs;
  /* these should be initialized and used as the bounds for rendering into the
     buffer as well XXX: not yet in use, and when in use will only be
     correct for axis aligned clips - proper rasterization of a clipping path
     would be yet another refinement on top.
   */

#if CTX_ENABLE_SHADOW_BLUR
  float      kernel[CTX_MAX_GAUSSIAN_KERNEL_DIM];
#endif

  int        aa;          // level of vertical aa
  int        force_aa;    // force full AA
  int        active_edges;
#if CTX_RASTERIZER_FORCE_AA==0
  int        pending_edges;   // this-scanline
  int        ending_edges;    // count of edges ending this scanline
#endif
  int        edge_pos;         // where we're at in iterating all edges
  CtxEdge    edges[CTX_MAX_EDGES];

  int        scanline;
  int        scan_min;
  int        scan_max;
  int        col_min;
  int        col_max;

#if CTX_BRAILLE_TEXT
  CtxList   *glyphs;
#endif

  CtxRenderstream edge_list;

  CtxState  *state;
  Ctx       *ctx;
  Ctx       *texture_source; /* normally same as ctx */

  void      *buf;

#if CTX_COMPOSITING_GROUPS
  void      *saved_buf; // when group redirected
  CtxBuffer *group[CTX_GROUP_MAX];
#endif

  float      x;  // < redundant? use state instead?
  float      y;

  float      first_x;
  float      first_y;
  int8_t     needs_aa; // count of how many edges implies antialiasing
  int        has_shape:2;
  int        has_prev:2;
  int        preserve:1;
  int        uses_transforms:1;
#if CTX_BRAILLE_TEXT
  int        term_glyphs:1; // store appropriate glyphs for redisplay
#endif

  int16_t    blit_x;
  int16_t    blit_y;
  int16_t    blit_width;
  int16_t    blit_height;
  int16_t    blit_stride;

  CtxPixelFormatInfo *format;

#if CTX_ENABLE_SHADOW_BLUR
  int in_shadow;
#endif
  int in_text;
  int shadow_x;
  int shadow_y;

  CtxFragment         fragment;
  uint8_t             color[4*5];

#define CTX_COMPOSITE_ARGUMENTS CtxRasterizer *rasterizer, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, int x0, uint8_t * __restrict__ coverage, int count

  void (*comp_op)(CTX_COMPOSITE_ARGUMENTS);
#if CTX_ENABLE_CLIP
  CtxBuffer *clip_buffer;
#endif

  uint8_t *clip_mask;
};

#if CTX_RASTERIZER
void ctx_rasterizer_deinit (CtxRasterizer *rasterizer);
#endif

#if CTX_EVENTS
extern int ctx_native_events;

#if CTX_SDL
extern int ctx_sdl_events;
int ctx_sdl_consume_events (Ctx *ctx);
#endif

#if CTX_FB
extern int ctx_fb_events;
int ctx_fb_consume_events (Ctx *ctx);
#endif


int ctx_nct_consume_events (Ctx *ctx);
int ctx_ctx_consume_events (Ctx *ctx);

#endif

enum {
  NC_MOUSE_NONE  = 0,
  NC_MOUSE_PRESS = 1,  /* "mouse-pressed", "mouse-released" */
  NC_MOUSE_DRAG  = 2,  /* + "mouse-drag"   (motion with pressed button) */
  NC_MOUSE_ALL   = 3   /* + "mouse-motion" (also delivered for release) */
};
void _ctx_mouse (Ctx *term, int mode);
void nc_at_exit (void);

int ctx_terminal_width  (void);
int ctx_terminal_height (void);
int ctx_terminal_cols   (void);
int ctx_terminal_rows   (void);
extern int ctx_frame_ack;

int ctx_nct_consume_events (Ctx *ctx);

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
   int  was_down;
};

// XXX the common members of sdl and fbdev
typedef struct _CtxThreaded CtxThreaded;
struct _CtxThreaded
{
   void (*render) (void *fb, CtxCommand *command);
   void (*flush)  (void *fb);
   void (*free)   (void *fb);
   Ctx          *ctx;
   Ctx          *ctx_copy;
   int           width;
   int           height;
   int           cols;
   int           rows;
   int           was_down;
   Ctx          *host[CTX_MAX_THREADS];
   CtxAntialias  antialias;
};


extern int _ctx_threads;
extern int _ctx_enable_hash_cache;
void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);
const char *
ctx_get (Ctx *ctx, const char *key);

int ctx_renderer_is_braille (Ctx *ctx);
Ctx *ctx_new_ctx (int width, int height);
Ctx *ctx_new_fb (int width, int height);
Ctx *ctx_new_sdl (int width, int height);
Ctx *ctx_new_braille (int width, int height);

int ctx_resolve_font (const char *name);
extern float ctx_u8_float[256];
#define ctx_u8_to_float(val_u8) ctx_u8_float[((uint8_t)(val_u8))]
//#define ctx_u8_to_float(val_u8) (val_u8/255.0f)
//
//


static uint8_t ctx_float_to_u8 (float val_f)
{
   return (val_f * 255.99f);
#if 0
  int val_i = val_f * 255.999f;
  if (val_i < 0) { return 0; }
  else if (val_i > 255) { return 255; }
  return val_i;
#endif
}


#define CTX_CSS_LUMINANCE_RED   0.3f
#define CTX_CSS_LUMINANCE_GREEN 0.59f
#define CTX_CSS_LUMINANCE_BLUE  0.11f

/* works on both float and uint8_t */
#define CTX_CSS_RGB_TO_LUMINANCE(rgb)  (\
  (rgb[0]) * CTX_CSS_LUMINANCE_RED + \
  (rgb[1]) * CTX_CSS_LUMINANCE_GREEN +\
  (rgb[2]) * CTX_CSS_LUMINANCE_BLUE)

const char *ctx_nct_get_event (Ctx *n, int timeoutms, int *x, int *y);
const char *ctx_native_get_event (Ctx *n, int timeoutms);
void
ctx_color_get_rgba8 (CtxState *state, CtxColor *color, uint8_t *out);
void ctx_color_get_graya_u8 (CtxState *state, CtxColor *color, uint8_t *out);
float ctx_float_color_rgb_to_gray (CtxState *state, const float *rgb);
void ctx_color_get_graya (CtxState *state, CtxColor *color, float *out);
void ctx_rgb_to_cmyk (float r, float g, float b,
              float *c_out, float *m_out, float *y_out, float *k_out);
uint8_t ctx_u8_color_rgb_to_gray (CtxState *state, const uint8_t *rgb);
#if CTX_ENABLE_CMYK
void ctx_color_get_cmyka (CtxState *state, CtxColor *color, float *out);
#endif
CTX_STATIC void ctx_color_set_RGBA8 (CtxState *state, CtxColor *color, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void ctx_color_set_rgba (CtxState *state, CtxColor *color, float r, float g, float b, float a);
CTX_STATIC void ctx_color_set_drgba (CtxState *state, CtxColor *color, float r, float g, float b, float a);
void ctx_color_get_cmyka (CtxState *state, CtxColor *color, float *out);
CTX_STATIC void ctx_color_set_cmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a);
CTX_STATIC void ctx_color_set_dcmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a);
CTX_STATIC void ctx_color_set_graya (CtxState *state, CtxColor *color, float gray, float alpha);

CTX_STATIC int ctx_color_model_get_components (CtxColorModel model);

void ctx_state_set (CtxState *state, uint32_t key, float value);
CTX_STATIC void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f);
CTX_STATIC void ctx_font_setup ();
float ctx_state_get (CtxState *state, uint32_t hash);
CTX_STATIC void
ctx_rasterizer_rel_move_to (CtxRasterizer *rasterizer, float x, float y);
CTX_STATIC void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y);

CTX_STATIC void
ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y);
CTX_STATIC void
ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y);
CTX_STATIC void
ctx_rasterizer_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2);
CTX_STATIC void
ctx_rasterizer_rel_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2);

CTX_STATIC void
ctx_rasterizer_reset (CtxRasterizer *rasterizer);
CTX_STATIC uint32_t ctx_rasterizer_poly_to_hash (CtxRasterizer *rasterizer);
CTX_STATIC void
ctx_rasterizer_arc (CtxRasterizer *rasterizer,
                    float        x,
                    float        y,
                    float        radius,
                    float        start_angle,
                    float        end_angle,
                    int          anticlockwise);

CTX_STATIC void
ctx_rasterizer_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y);

CTX_STATIC void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y);

CTX_STATIC void
ctx_rasterizer_rectangle (CtxRasterizer *rasterizer,
                          float x,
                          float y,
                          float width,
                          float height);

CTX_STATIC void ctx_rasterizer_finish_shape (CtxRasterizer *rasterizer);
CTX_STATIC void ctx_rasterizer_clip (CtxRasterizer *rasterizer);
CTX_STATIC void
ctx_rasterizer_set_font (CtxRasterizer *rasterizer, const char *font_name);

CTX_STATIC void
ctx_rasterizer_gradient_add_stop (CtxRasterizer *rasterizer, float pos, float *rgba);
CTX_STATIC void
ctx_rasterizer_set_pixel (CtxRasterizer *rasterizer,
                          uint16_t x,
                          uint16_t y,
                          uint8_t r,
                          uint8_t g,
                          uint8_t b,
                          uint8_t a);
CTX_STATIC void
ctx_rasterizer_rectangle (CtxRasterizer *rasterizer,
                          float x,
                          float y,
                          float width,
                          float height);
CTX_STATIC void
ctx_rasterizer_round_rectangle (CtxRasterizer *rasterizer, float x, float y, float width, float height, float corner_radius);


#if CTX_ENABLE_CM // XXX to be moved to ctx.h
void
ctx_set_drgb_space (Ctx *ctx, int device_space);
void
ctx_set_dcmyk_space (Ctx *ctx, int device_space);
void
ctx_rgb_space (Ctx *ctx, int device_space);
void
ctx_set_cmyk_space (Ctx *ctx, int device_space);
#endif

#endif

CtxRasterizer *
ctx_rasterizer_init (CtxRasterizer *rasterizer, Ctx *ctx, Ctx *texture_source, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format, CtxAntialias antialias);


CTX_INLINE static uint8_t ctx_lerp_u8 (uint8_t v0, uint8_t v1, uint8_t dx)
{
#if 0
  return v0 + ((v1-v0) * dx)/255;
#else
  return ( ( ( ( (v0) <<8) + (dx) * ( (v1) - (v0) ) ) ) >>8);
#endif
}

CTX_INLINE static float
ctx_lerpf (float v0, float v1, float dx)
{
  return v0 + (v1-v0) * dx;
}

#endif

