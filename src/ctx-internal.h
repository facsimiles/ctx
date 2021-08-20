
#if CTX_IMPLEMENTATION|CTX_COMPOSITE

#ifndef __CTX_INTERNAL_H
#define __CTX_INTERNAL_H

#if !__COSMOPOLITAN__
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#endif

#define CTX_BRANCH_HINTS  1

#if CTX_BRANCH_HINTS
#define CTX_LIKELY(x)      __builtin_expect(!!(x), 1)
#define CTX_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#else
#define CTX_LIKELY(x)      (x)
#define CTX_UNLIKELY(x)    (x)
#endif

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
  const Babl *space; // gets copied from state when color is declared
#else
  void   *space; // gets copied from state when color is declared, 
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
  CTX_SOURCE_TEXTURE,
  CTX_SOURCE_LINEAR_GRADIENT,
  CTX_SOURCE_RADIAL_GRADIENT,
  CTX_SOURCE_INHERIT_FILL
};

typedef enum _CtxSourceType CtxSourceType;

typedef struct _CtxPixelFormatInfo CtxPixelFormatInfo;


struct _CtxBuffer
{
  void               *data;
  int                 width;
  int                 height;
  int                 stride;
  char               *eid;        // might be NULL, when not - should be unique for pixel contents
  int                 frame;      // last frame used in, everything > 3 can be removed,
                                  // as clients wont rely on it.
  CtxPixelFormatInfo *format;
  void (*free_func) (void *pixels, void *user_data);
  void               *user_data;

#if CTX_ENABLE_CM
#if CTX_BABL
  const Babl *space;
#else
  void       *space; 
#endif
#endif
#if 1
  CtxBuffer          *color_managed; /* only valid for one render target, cache
                                        for a specific space
                                        */
#endif
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
  CtxMatrix  set_transform;
  CtxMatrix  transform;
  union
  {
    CtxColor color;
    struct
    {
      uint8_t rgba[4]; // shares data with set color
      uint8_t pad;
      CtxBuffer *buffer;
    } texture;
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
  CtxSource     source_stroke;
  CtxSource     source_fill;
  float         global_alpha_f;
  uint8_t       global_alpha_u8;

  float         line_width;
  float         line_dash_offset;
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
  const Babl   *texture_space;
  const Babl   *rgb_space;       
  const Babl   *cmyk_space;

  const Babl   *fish_rgbaf_user_to_device;
  const Babl   *fish_rgbaf_texture_to_device;
  const Babl   *fish_rgbaf_device_to_user;

#else
  void         *device_space;
  void         *texture_space;
  void         *rgb_space;       
  void         *cmyk_space;
  void         *fish_rgbaf_user_to_device; // dummy padding
  void         *fish_rgbaf_texture_to_device; // dummy padding
  void         *fish_rgbaf_device_to_user; // dummy padding
#endif
#endif
  CtxCompositingMode  compositing_mode; // bitfield refs lead to
  CtxBlend                  blend_mode; // non-vectorization

  float dashes[CTX_PARSER_MAX_ARGS];
  int n_dashes;

  CtxColorModel    color_model;
  /* bitfield-pack small state-parts */
  CtxLineCap          line_cap:2;
  CtxLineJoin        line_join:2;
  CtxFillRule        fill_rule:1;
  unsigned int image_smoothing:1;
  unsigned int            font:6;
  unsigned int            bold:1;
  unsigned int          italic:1;
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

#define CTX_DRAWLIST_DOESNT_OWN_ENTRIES   64
#define CTX_DRAWLIST_EDGE_LIST            128
#define CTX_DRAWLIST_CURRENT_PATH         512
// BITPACK

struct _CtxDrawlist
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
  uint64_t key;
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
  int16_t       gstate_no;
  CtxGState     gstate;
  CtxGState     gstate_stack[CTX_MAX_STATES];//at end, so can be made dynamic
#if CTX_GRADIENTS
  CtxGradient   gradient; /* we keep only one gradient,
                             this goes icky with multiple
                             restores - it should really be part of
                             graphics state..
                             XXX, with the stringpool gradients
                             can be stored there.
                           */
#endif
  CtxKeyDbEntry keydb[CTX_MAX_KEYDB];
  char          stringpool[CTX_STRINGPOOL_SIZE];
  int8_t        source; // used for the single-shifting to stroking
                // 0  = fill
                // 1  = start_stroke
                // 2  = in_stroke
                //
                //   if we're at in_stroke at start of a source definition
                //   we do filling
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
      /* we've got ~110 bytes to fill to cover as
         much data as stbtt_fontinfo */
      //int16_t glyph_pos[26]; // for a..z
      int       glyphs; // number of glyphs
      uint32_t *index;
    } ctx;
    struct
    {
      char *path;
    } ctx_fs;
#if CTX_FONT_ENGINE_STB
    struct
    {
      stbtt_fontinfo ttf_info;
      int cache_index;
      uint32_t cache_unichar;
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
  int              first_run;
  CtxDrawlist *drawlist;
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

  CtxCursor       cursor; /* if 0 then UNSET and no cursor change is requested
                           */

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
  CtxItem         *last_item;
  CtxModifierState modifier_state;
  int              tap_delay_min;
  int              tap_delay_max;
  int              tap_delay_hold;
  double           tap_hysteresis;
};


#endif

typedef struct _CtxEidInfo
{
  char *eid;
  int   frame;
  int   width;
  int   height;
} CtxEidInfo;

struct _Ctx
{
  CtxImplementation *renderer;
  CtxDrawlist        drawlist;
  int                transformation;
  CtxBuffer          texture[CTX_MAX_TEXTURES];
  Ctx               *texture_cache;
  CtxList           *eid_db;
  int                rev;
  void              *backend;
  CtxState           state;        /**/
  int                frame; /* used for texture lifetime */
#if CTX_EVENTS 
  CtxCursor          cursor;
  int                quit;
  int                dirty;
  CtxEvents          events;
  int                mouse_fd;
  int                mouse_x;
  int                mouse_y;
#endif
#if CTX_CURRENT_PATH
  CtxDrawlist    current_path; // possibly transformed coordinates !
  CtxIterator        current_path_iterator;
#endif
};


static void ctx_process (Ctx *ctx, CtxEntry *entry);
CtxBuffer *ctx_buffer_new (int width, int height,
                           CtxPixelFormat pixel_format);
void ctx_buffer_free (CtxBuffer *buffer);

void
ctx_state_gradient_clear_stops (CtxState *state);

static inline void ctx_interpret_style         (CtxState *state, CtxEntry *entry, void *data);
static inline void ctx_interpret_transforms    (CtxState *state, CtxEntry *entry, void *data);
static inline void ctx_interpret_pos           (CtxState *state, CtxEntry *entry, void *data);
static inline void ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data);

struct _CtxInternalFsEntry
{
  char *path;
  int   length;
  char *data;
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
  CtxPixelFormat composite_format;

  void         (*to_comp) (CtxRasterizer *r,
                           int x, const void * __restrict__ src, uint8_t * __restrict__ comp, int count);
  void         (*from_comp) (CtxRasterizer *r,
                             int x, const uint8_t * __restrict__ comp, void *__restrict__ dst, int count);
  void         (*apply_coverage) (CtxRasterizer *r, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, int x, uint8_t *coverage,
                          int count);
  void         (*setup) (CtxRasterizer *r);
};


CTX_STATIC inline void
_ctx_user_to_device (CtxState *state, float *x, float *y);
CTX_STATIC void
_ctx_user_to_device_distance (CtxState *state, float *x, float *y);
CTX_STATIC void ctx_state_init (CtxState *state);
static inline void
ctx_interpret_pos_bare (CtxState *state, CtxEntry *entry, void *data);
static inline void
ctx_drawlist_deinit (CtxDrawlist *drawlist);

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

typedef void (*CtxFragment) (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy);

#define CTX_MAX_GAUSSIAN_KERNEL_DIM    512

struct _CtxShapeEntry
{
  uint32_t hash;
  uint16_t width;
  uint16_t height;
  int      last_frame; // xxx
  uint32_t uses;  // instrumented for longer keep-alive
  uint8_t  data[];
};

typedef struct _CtxShapeEntry CtxShapeEntry;


struct _CtxShapeCache
{
  CtxShapeEntry *entries[CTX_SHAPE_CACHE_ENTRIES];
  long size;
};

typedef struct _CtxShapeCache CtxShapeCache;


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

  unsigned int aa;          // level of vertical aa
  int fast_aa;
  int prev_active_edges;
  int active_edges;
  int pending_edges;
  int ending_edges;
  int edge_pos;         // where we're at in iterating all edges
  int edges[CTX_MAX_EDGES]; // integer position in edge array

  int scanline;
  int        scan_min;
  int        scan_max;
  int        col_min;
  int        col_max;

  CtxDrawlist edge_list;

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
  unsigned int needs_aa3; // count of how many edges implies antialiasing
  unsigned int needs_aa5; // count of how many edges implies antialiasing
  unsigned int needs_aa15; // count of how many edges implies antialiasing
  int        horizontal_edges;
  int        uses_transforms;
  int        has_shape:2;
  int        has_prev:2;
  int        preserve:1;

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
  int swap_red_green;
  uint8_t             color[4*5];

#define CTX_COMPOSITE_ARGUMENTS CtxRasterizer *rasterizer, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, int x0, uint8_t * __restrict__ coverage, int count

  void (*comp_op)(CTX_COMPOSITE_ARGUMENTS);
#if CTX_ENABLE_CLIP
  CtxBuffer *clip_buffer;
#endif

  int clip_rectangle;

#if CTX_SHAPE_CACHE
  CtxShapeCache shape_cache;
#endif
#if CTX_BRAILLE_TEXT
  int        term_glyphs:1; // store appropriate glyphs for redisplay
  CtxList   *glyphs;
#endif
};

struct _CtxSHA1 {
    uint64_t length;
    uint32_t state[5], curlen;
    unsigned char buf[64];
};


struct _CtxHasher
{
  CtxRasterizer rasterizer;
  int           cols;
  int           rows;
  uint8_t      *hashes;
  CtxSHA1       sha1_fill; 
  CtxSHA1       sha1_stroke;
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
   void (*reset)  (void *ctxvtx);
   void (*flush)  (void *ctxctx);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)   (void *ctxctx);
   Ctx *ctx;
   int  width;
   int  height;
   int  cols;
   int  rows;
   int  was_down;
};


extern int _ctx_max_threads;
extern int _ctx_enable_hash_cache;
void
ctx_set (Ctx *ctx, uint64_t key_hash, const char *string, int len);
const char *
ctx_get (Ctx *ctx, const char *key);

int ctx_renderer_is_term (Ctx *ctx);
Ctx *ctx_new_ctx (int width, int height);
Ctx *ctx_new_fb (int width, int height, int drm);
Ctx *ctx_new_sdl (int width, int height);
Ctx *ctx_new_term (int width, int height);
Ctx *ctx_new_termimg (int width, int height);

int ctx_resolve_font (const char *name);
extern float ctx_u8_float[256];
#define ctx_u8_to_float(val_u8) ctx_u8_float[((uint8_t)(val_u8))]
//#define ctx_u8_to_float(val_u8) (val_u8/255.0f)
//
//


static uint8_t ctx_float_to_u8 (float val_f)
{
  return val_f < 0.0f ? 0 : val_f > 1.0f ? 0xff : 0xff * val_f +  0.5f;
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

int ctx_color_model_get_components (CtxColorModel model);

static void ctx_state_set (CtxState *state, uint64_t key, float value);
CTX_STATIC void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f);
CTX_STATIC void ctx_font_setup ();
static float ctx_state_get (CtxState *state, uint64_t hash);

#if CTX_RASTERIZER

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
ctx_rasterizer_round_rectangle (CtxRasterizer *rasterizer, float x, float y, float width, float height, float corner_radius);

#endif

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

CTX_INLINE static uint32_t ctx_lerp_RGBA8 (const uint32_t v0, const uint32_t v1, const uint8_t dx)
{
#if 0
  char bv0[4];
  char bv1[4];
  char res[4];
  memcpy (&bv0[0], &v0, 4);
  memcpy (&bv1[0], &v1, 4);
  for (int c = 0; c < 4; c++)
    res [c] = ctx_lerp_u8 (bv0[c], bv1[c], dx);
  return ((uint32_t*)(&res[0]))[0];
#else
  const uint32_t cov = dx;
  const uint32_t si_ga = (v1 & 0xff00ff00) >> 8;
  const uint32_t si_rb = v1 & 0x00ff00ff;
  const uint32_t di_rb = v0 & 0x00ff00ff;
  const uint32_t d_rb = si_rb - di_rb;
  const uint32_t di_ga = v0 & 0xff00ff00;
  const uint32_t d_ga = si_ga - (di_ga>>8);
  return
     (((di_rb + ((0xff00ff + d_rb * cov)>>8)) & 0x00ff00ff))  |
      ((di_ga + ((0xff00ff + d_ga * cov)      & 0xff00ff00)));

#endif
}

CTX_INLINE static uint32_t ctx_lerp_RGBA8_2 (const uint32_t v0, uint32_t si_ga, uint32_t si_rb, const uint8_t dx)
{
  const uint32_t cov = dx;
  const uint32_t di_ga = ( v0 & 0xff00ff00);
  const uint32_t di_rb = v0 & 0x00ff00ff;
  uint32_t d_rb = si_rb - di_rb;
  const uint32_t d_ga = si_ga - (di_ga>>8);
  return
     (((di_rb + ((0xff00ff + d_rb * cov)>>8)) & 0x00ff00ff))  |
      ((di_ga + ((0xff00ff + d_ga * cov)      & 0xff00ff00)));
}



CTX_INLINE static float
ctx_lerpf (float v0, float v1, float dx)
{
  return v0 + (v1-v0) * dx;
}


#ifndef CTX_MIN
#define CTX_MIN(a,b)  (((a)<(b))?(a):(b))
#endif
#ifndef CTX_MAX
#define CTX_MAX(a,b)  (((a)>(b))?(a):(b))
#endif

static inline void *ctx_calloc (size_t size, size_t count);

void ctx_screenshot (Ctx *ctx, const char *output_path);


CtxSHA1 *ctx_sha1_new (void);
void ctx_sha1_free (CtxSHA1 *sha1);
int ctx_sha1_process(CtxSHA1 *sha1, const unsigned char * msg, unsigned long len);
int ctx_sha1_done(CtxSHA1 * sha1, unsigned char *out);

void _ctx_texture_lock (void);
void _ctx_texture_unlock (void);
uint8_t *ctx_define_texture_pixel_data (CtxEntry *entry);
void ctx_buffer_pixels_free (void *pixels, void *userdata);

/*ctx_texture_init:
 * return value: eid, as passed in or if NULL generated by hashing pixels and width/height
 * XXX  this is low-level and not to be used directly use define_texture instead.  XXX
 */
const char *ctx_texture_init (
                      Ctx        *ctx,
                      const char *eid,
                      int         width,
                      int         height,
                      int         stride,
                      CtxPixelFormat format,
                      void       *space,
                      uint8_t    *pixels,
                      void (*freefunc) (void *pixels, void *user_data),
                      void *user_data);

#if CTX_TILED
#if !__COSMOPOLITAN__
#include <threads.h>
#endif
#endif
typedef struct _CtxTiled CtxTiled;

struct _CtxTiled
{
   void (*render)    (void *term, CtxCommand *command);
   void (*reset)     (void *term);
   void (*flush)     (void *term);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)      (void *term);
   Ctx          *ctx;
   int           width;
   int           height;
   int           cols;
   int           rows;
   int           was_down;
   uint8_t      *pixels;
   Ctx          *ctx_copy;
   Ctx          *host[CTX_MAX_THREADS];
   CtxAntialias  antialias;
   int           quit;
#if CTX_TILED
   _Atomic int   thread_quit;
#endif
   int           shown_frame;
   int           render_frame;
   int           rendered_frame[CTX_MAX_THREADS];
   int           frame;
   int       min_col; // hasher cols and rows
   int       min_row;
   int       max_col;
   int       max_row;
   uint8_t  hashes[CTX_HASH_ROWS * CTX_HASH_COLS *  20];
   int8_t    tile_affinity[CTX_HASH_ROWS * CTX_HASH_COLS]; // which render thread no is
                                                           // responsible for a tile
                                                           //

   int           pointer_down[3];

   CtxCursor     shown_cursor;
#if CTX_TILED
   cnd_t  cond;
   mtx_t  mtx;
#endif
};

static void
_ctx_texture_prepare_color_management (CtxRasterizer *rasterizer,
                                      CtxBuffer     *buffer);

#endif

