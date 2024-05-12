
#if CTX_IMPLEMENTATION|CTX_COMPOSITE

#ifndef __CTX_INTERNAL_H
#define __CTX_INTERNAL_H

#if !__COSMOPOLITAN__
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#endif


#if CTX_BRANCH_HINTS
#define CTX_LIKELY(x)      __builtin_expect(!!(x), 1)
#define CTX_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#else
#define CTX_LIKELY(x)      (x)
#define CTX_UNLIKELY(x)    (x)
#endif

typedef struct _CtxRasterizer CtxRasterizer;
typedef struct _CtxGState     CtxGState;
//typedef struct _CtxState      CtxState;

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
  float   red;
  float   green;
  float   blue;
#if CTX_BABL
  const Babl *space; // gets copied from state when color is declared
#else
  void   *space; // gets copied from state when color is declared, 
#endif
#endif
};

typedef struct _CtxGradientStop CtxGradientStop;

struct _CtxGradientStop
{
  CtxColor color;
  float   pos;
};


enum _CtxSourceType
{
  CTX_SOURCE_COLOR = 0,
  CTX_SOURCE_TEXTURE,
  CTX_SOURCE_LINEAR_GRADIENT,
  CTX_SOURCE_RADIAL_GRADIENT,
  CTX_SOURCE_CONIC_GRADIENT,
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
  int                 frame;      // last frame used in, everything > 3 can be removed,
                                  // as clients wont rely on it.
  char               *eid;        // might be NULL, when not - should be unique for pixel contents
  const CtxPixelFormatInfo *format;
  void (*free_func) (void *pixels, void *user_data);
  void               *user_data;

#if CTX_ENABLE_CM
#if CTX_BABL
  const Babl *space;
#else
  void       *space; 
#endif
#endif
#if CTX_ENABLE_CM
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
  CtxGradientStop stops[CTX_MAX_GRADIENT_STOPS];
  int n_stops;
};

struct _CtxSource
{
  int type;
  CtxMatrix  set_transform;
  CtxMatrix  transform;
  uint32_t   pad;
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
      float length;
      float dx_scaled;
      float dy_scaled;
      float start_scaled;
    } linear_gradient;
    struct
    {
      float x;
      float y;
      float start_angle;
      float cycles;
    } conic_gradient;
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


typedef struct _Ctx16f16Matrix     Ctx16f16Matrix;
struct
  _Ctx16f16Matrix
{
#if CTX_32BIT_SEGMENTS
  int64_t m[3][3];  // forcing higher precision easily, the extra
                    // memory cost is minuscle
#else
  int32_t m[3][3];
#endif
};


struct _CtxGState
{
#if CTX_32BIT_SEGMENTS
  uint32_t      keydb_pos;
  uint32_t      stringpool_pos;
#else
  uint16_t      keydb_pos;      // this limits these
  uint16_t      stringpool_pos; // 
#endif

  CtxMatrix     transform;
  Ctx16f16Matrix  prepped_transform;
  CtxSource     source_stroke;
  CtxSource     source_fill;
  float         global_alpha_f;

  float         line_width;
  float         line_dash_offset;
  float         stroke_pos;
  float         feather;
  float         miter_limit;
  float         font_size;
#if CTX_ENABLE_SHADOW_BLUR
  float         shadow_blur;
  float         shadow_offset_x;
  float         shadow_offset_y;
#endif
  unsigned int  transform_type:3;
  unsigned int        clipped:1;
  CtxColorModel    color_model:8;
  /* bitfield-pack small state-parts */
  CtxLineCap          line_cap:2;
  CtxLineJoin        line_join:2;
  CtxFillRule        fill_rule:1;
  unsigned int image_smoothing:1;
  unsigned int            font:6;
  unsigned int            bold:1;
  unsigned int          italic:1;

  uint8_t       global_alpha_u8;
  int16_t       clip_min_x;
  int16_t       clip_min_y;
  int16_t       clip_max_x;
  int16_t       clip_max_y;
  int           n_dashes;

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
  CtxExtend                 extend;
  long  tolerance_fixed;
  float tolerance;
  float dashes[CTX_MAX_DASHES]; // XXX moving dashes 
                                //  to state storage,. will
                                //  allow it to be larger,
                                //  free up memory, and
                                //  make save/restore faster
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
  unsigned int count;
  int size;
  uint32_t  flags;
  int       bitpack_pos;  // stream is bitpacked up to this offset
};

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
  int  has_moved;
  unsigned int  has_clipped:1;
  int8_t        source; // used for the single-shifting to stroking
                // 0  = fill
                // 1  = start_stroke
                // 2  = in_stroke
                //
                //   if we're at in_stroke at start of a source definition
                //   we do filling
  int16_t       gstate_no;

  float         x;
  float         y;
  float         first_x;
  float         first_y;
  int           ink_min_x;
  int           ink_min_y;
  int           ink_max_x;
  int           ink_max_y;
#if CTX_GSTATE_PROTECT
  int           gstate_waterlevel;
#endif
  CtxGState     gstate;
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
  int   (*glyph)       (CtxFont *font, Ctx *ctx, int glyphid, int stroke);
  float (*glyph_width) (CtxFont *font, Ctx *ctx, int glyphid);
  float (*glyph_kern)  (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB);

  // return -1 for not found or 0 or positive number for found glyph
  int   (*glyph_lookup)  (CtxFont *font, Ctx *ctx, uint32_t unichar);
};

#if CTX_FONT_ENGINE_HARFBUZZ
#include <hb.h>
#endif

#pragma pack(push,1)
struct _CtxFont
{
#if CTX_ONE_FONT_ENGINE==0
  CtxFontEngine *engine;
#endif
  union
  {
    struct
    {
      CtxEntry *data;
    //uint16_t length;
      /* we've got ~110 bytes to fill to cover as
         much data as stbtt_fontinfo */
      //int16_t glyph_pos[26]; // for a..z
    } ctx;
#if CTX_FONT_ENGINE_CTX_FS
    struct
    {
      const char *name;
      char *path;
    } ctx_fs;
#endif
#if CTX_FONT_ENGINE_STB
    struct
    {
      const char *name;
      stbtt_fontinfo ttf_info;
    } stb;
#endif
#if CTX_FONT_ENGINE_HARFBUZZ
    struct
    {
      const char *name;
      char *path;
      hb_blob_t *blob;
      hb_face_t *face;
      hb_font_t *font;
      hb_draw_funcs_t *draw_funcs;
#if HB_VERSION_MAJOR >= 7
      hb_paint_funcs_t *paint_funcs;
#endif
      //int x_scale;
      //int y_scale;
      float scale;
    } hb;
#endif



#if 0
    struct { int start; int end; int gw; int gh; const uint8_t *data;} monobitmap;
#endif
  };
#if CTX_ONE_FONT_ENGINE==0
  uint8_t type:3; // 0 ctx    1 stb    2 monobitmap
  uint8_t monospaced:1;
#endif
};
#pragma pack(pop)

enum _CtxIteratorFlag
{
  CTX_ITERATOR_FLAT           = 0,
  CTX_ITERATOR_EXPAND_BITPACK = 2,
  CTX_ITERATOR_DEFAULTS       = CTX_ITERATOR_EXPAND_BITPACK
};
typedef enum _CtxIteratorFlag CtxIteratorFlag;


struct _CtxIterator
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
  CtxEvent         drag_event[CTX_MAX_DEVICES];
  CtxList         *idles;
  CtxList         *idles_to_remove;
  CtxList         *idles_to_add;
  CtxList         *events; // for ctx_get_event
  CtxBinding       bindings[CTX_MAX_KEYBINDINGS]; /*< better as list, uses no mem if unused */
  int              n_bindings;
  CtxItem         *prev[CTX_MAX_DEVICES];
  float            pointer_x[CTX_MAX_DEVICES];
  float            pointer_y[CTX_MAX_DEVICES];
  unsigned char    pointer_down[CTX_MAX_DEVICES];
  unsigned int     in_idle_dispatch:1;
  unsigned int     ctx_get_event_enabled:1;
  CtxModifierState modifier_state;
  int              idle_id;
  CtxList         *items;
  CtxItem         *last_item;
  float            tap_hysteresis;
#if CTX_VT
  CtxList         *clients;
  CtxClient *active;
  CtxClient *active_tab;
#endif
  int              tap_delay_min;
  int              tap_delay_max;
  int              tap_delay_hold;
};
#endif

typedef struct _CtxEidInfo
{
  char *eid;
  int   frame;
  int   width;
  int   height;
} CtxEidInfo;


struct _CtxGlyphEntry
{
  uint32_t  unichar;
  uint16_t  offset;
  CtxFont  *font;
};
typedef struct _CtxGlyphEntry CtxGlyphEntry;

struct _Ctx
{
  CtxBackend       *backend;
  void  (*process)  (Ctx *ctx, const CtxCommand *entry);
  CtxState          state;        /**/
  CtxDrawlist       drawlist;
  int               transformation;
  int               width;
  int               height;
  int               dirty;
  Ctx              *texture_cache;
  CtxList          *deferred;
  CtxList          *eid_db;
  int               frame; /* used for texture lifetime */
  uint32_t          bail;
  CtxBackend       *backend_pushed;
  CtxBuffer         texture[CTX_MAX_TEXTURES];
  int               exit;
#if CTX_EVENTS 
  CtxCursor         cursor;
  CtxEvents         events;
  int               mouse_fd;
  int               mouse_x;
  int               mouse_y;
#endif
#if CTX_CURRENT_PATH
  CtxDrawlist       current_path; // possibly transformed coordinates !
  CtxIterator       current_path_iterator;
#endif
#if CTX_GLYPH_CACHE
  CtxGlyphEntry     glyph_index_cache[CTX_GLYPH_CACHE_SIZE];
#endif
  CtxFont *fonts; // a copy to keep it alive with mp's
                  // garbage collector, the fonts themselves
                  // are static and shared beyond ctx contexts
 

};

#if 0
#define ctx_process(ctx,entry)  ctx->process (ctx, (CtxCommand *)(entry));
#else
static inline void
ctx_process (Ctx *ctx, const CtxEntry *entry)
{
  ctx->process (ctx, (CtxCommand *) entry);
}
#endif

CtxBuffer *ctx_buffer_new (int width, int height,
                           CtxPixelFormat pixel_format);
void ctx_buffer_destroy (CtxBuffer *buffer);

static void
ctx_state_gradient_clear_stops (CtxState *state);

static inline void ctx_interpret_style         (CtxState *state, const CtxEntry *entry, void *data);
static inline void ctx_interpret_transforms    (CtxState *state, const CtxEntry *entry, void *data);
static inline void ctx_interpret_pos           (CtxState *state, CtxEntry *entry, void *data);
static inline void ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data);

struct _CtxInternalFsEntry
{
  char *path;
  int   length;
  char *data;
};


typedef void (*ctx_apply_coverage_fun) (unsigned int count, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, uint8_t *coverage, CtxRasterizer *r, int x);

struct _CtxPixelFormatInfo
{
  CtxPixelFormat pixel_format:8;
  uint8_t        components; /* number of components */
  uint8_t        bpp; /* bits  per pixel - for doing offset computations
                         along with rowstride found elsewhere, if 0 it indicates
                         1/8  */
  uint8_t        ebpp; /*effective bytes per pixel - for doing offset
                         computations, for formats that get converted, the
                         ebpp of the working space applied */
  uint8_t        dither_red_blue;
  uint8_t        dither_green;
  CtxPixelFormat composite_format:8;

  void         (*to_comp) (CtxRasterizer *r,
                           int x, const void * __restrict__ src, uint8_t * __restrict__ comp, int count);
  void         (*from_comp) (CtxRasterizer *r,
                             int x, const uint8_t * __restrict__ comp, void *__restrict__ dst, int count);
  ctx_apply_coverage_fun apply_coverage;
  void         (*setup) (CtxRasterizer *r);
};


CTX_STATIC inline void
_ctx_user_to_device (CtxState *state, float *x, float *y);
CTX_STATIC void
_ctx_user_to_device_distance (CtxState *state, float *x, float *y);
CTX_STATIC void ctx_state_init (CtxState *state);
static inline void
ctx_interpret_pos_bare (CtxState *state, const CtxEntry *entry, void *data);
static inline void
ctx_drawlist_deinit (CtxDrawlist *drawlist);

//extern CtxPixelFormatInfo *(*ctx_pixel_format_info) (CtxPixelFormat format);
const CtxPixelFormatInfo *ctx_pixel_format_info (CtxPixelFormat format);



extern void (*ctx_composite_stroke_rect) (CtxRasterizer *rasterizer,
                           float          x0,
                           float          y0,
                           float          x1,
                           float          y1,
                           float          line_width);

extern void (*ctx_composite_setup) (CtxRasterizer *rasterizer);


extern void (*ctx_rasterizer_rasterize_edges) (CtxRasterizer *rasterizer, const int fill_rule);

extern void (*ctx_composite_fill_rect) (CtxRasterizer *rasterizer,
                           float        x0,
                           float        y0,
                           float        x1,
                           float        y1,
                           uint8_t      cov);


const char *ctx_utf8_skip (const char *s, int utf8_length);
int ctx_utf8_strlen (const char *s);
int
ctx_unichar_to_utf8 (uint32_t  ch,
                     uint8_t  *dest);

uint32_t
ctx_utf8_to_unichar (const char *input);


typedef struct _CtxHasher CtxHasher;

typedef void (*CtxFragment) (CtxRasterizer *rasterizer, float x, float y, float z, void *out, int count, float dx, float dy, float dz);

#define CTX_MAX_GAUSSIAN_KERNEL_DIM    512

typedef enum {
   CTX_COV_PATH_FALLBACK =0,
   CTX_COV_PATH_RGBA8_OVER,
   CTX_COV_PATH_RGBA8_COPY,
   CTX_COV_PATH_RGBA8_COPY_FRAGMENT,
   CTX_COV_PATH_RGBA8_OVER_FRAGMENT,
   CTX_COV_PATH_GRAYA8_COPY,
   CTX_COV_PATH_GRAY1_COPY,
   CTX_COV_PATH_GRAY2_COPY,
   CTX_COV_PATH_GRAY4_COPY,
   CTX_COV_PATH_RGB565_COPY,
   CTX_COV_PATH_RGB332_COPY,
   CTX_COV_PATH_GRAY8_COPY,
   CTX_COV_PATH_RGBAF_COPY,
   CTX_COV_PATH_RGB8_COPY,
   CTX_COV_PATH_CMYK8_COPY,
   CTX_COV_PATH_CMYKA8_COPY,
   CTX_COV_PATH_CMYKAF_COPY,
   CTX_COV_PATH_GRAYAF_COPY
} CtxCovPath;

struct _CtxRasterizer
{
  CtxBackend backend;
  /* these should be initialized and used as the bounds for rendering into the
     buffer as well XXX: not yet in use, and when in use will only be
     correct for axis aligned clips - proper rasterization of a clipping path
     would be yet another refinement on top.
   */


#define CTX_COMPOSITE_ARGUMENTS unsigned int count, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, uint8_t * __restrict__ coverage, CtxRasterizer *rasterizer, int x0
  void (*comp_op)(CTX_COMPOSITE_ARGUMENTS);
  CtxFragment fragment;
  //Ctx       *ctx;
  CtxState  *state;
  CtxCovPath  comp;
  unsigned int  swap_red_green;
  ctx_apply_coverage_fun apply_coverage;

  unsigned int active_edges;
  unsigned int edge_pos;         // where we're at in iterating all edges
  unsigned int pending_edges;
  unsigned int horizontal_edges;
  unsigned int ending_edges;

  unsigned int aa;          // level of vertical aa
  int  convex;
  unsigned int  scan_aa[4]; // 0=none, 1 = 3, 2 = 5, 3 = 15

  int        scanline;
  int        scan_min;
  int        scan_max;
  int        col_min;
  int        col_max;

  int        inner_x;
  int        inner_y;

  float      x;
  float      y;

  int        first_edge;

  uint16_t    blit_x;
  uint16_t    blit_y;
  int32_t    blit_width;
  int32_t    blit_height;
  uint32_t    blit_stride;


  unsigned int  unused; // kept for layout
  unsigned int  clip_rectangle;
  int           has_prev;
  void      *buf;
#if CTX_ENABLE_SHADOW_BLUR
  unsigned int  in_shadow:1;
  float         feather_x;
  float         feather_y;
  float         feather;
#endif

  const CtxPixelFormatInfo *format;
  Ctx       *texture_source; /* normally same as ctx */

  uint16_t   color_native;  //
  uint16_t   color_nativeB[8];
  uint8_t    color[8*5];   // in compositing format
  int edges[CTX_MAX_EDGES]; // integer position in edge array
  CtxDrawlist edge_list;
			   
  unsigned int  preserve;
  unsigned int  in_text;


#if CTX_STATIC_OPAQUE
  uint8_t opaque[CTX_MAX_SCANLINE_LENGTH];
#endif
#if CTX_ENABLE_CLIP
  CtxBuffer *clip_buffer;
#endif

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
  int gradient_cache_valid;
  uint8_t gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];
  int gradient_cache_elements;
#endif
#endif


#if CTX_BRAILLE_TEXT
  unsigned int  term_glyphs:1; // store appropriate glyphs for redisplay
#endif
#if CTX_BRAILLE_TEXT
  CtxList   *glyphs;
#endif

#if CTX_COMPOSITING_GROUPS
  void      *saved_buf; // when group redirected
  CtxBuffer *group[CTX_GROUP_MAX];
#endif
#if CTX_ENABLE_SHADOW_BLUR
  float      kernel[CTX_MAX_GAUSSIAN_KERNEL_DIM];
#endif
  unsigned int shadow_active_edges;
  unsigned int shadow_edge_pos;
  int shadow_edges[CTX_MAX_EDGES*2];

#if CTX_SCANBIN
  uint32_t scan_bins[CTX_MAX_SCANLINES][CTX_MAX_EDGES];
#if CTX_MAX_EDGES>255
  uint32_t scan_bin_count[CTX_MAX_SCANLINES];
#else
  uint8_t scan_bin_count[CTX_MAX_SCANLINES];
#endif
#endif


};

struct _CtxSHA1 {
    uint64_t length;
    uint32_t state[5], curlen;
    unsigned char buf[64];
};
typedef struct _CtxMurmur CtxMurmur;
struct _CtxMurmur {
    uint32_t state[2];
};


#pragma pack(push,1)
typedef struct CtxCommandState
{
  uint16_t pos;
  uint32_t active;
} CtxCommandState;
#pragma pack(pop)

struct _CtxHasher
{
  CtxRasterizer rasterizer;
  int           cols;
  int           rows;
  uint32_t      hashes[CTX_HASH_COLS*CTX_HASH_ROWS];
  CtxMurmur     murmur_fill[CTX_MAX_STATES]; 
  CtxMurmur     murmur_stroke[CTX_MAX_STATES];
  int           source_level;
  int           pos; 

  int           prev_command;

  CtxDrawlist  *drawlist;

};

#if CTX_RASTERIZER
void ctx_rasterizer_deinit (CtxRasterizer *rasterizer);
void ctx_rasterizer_destroy (CtxRasterizer *rasterizer);
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


typedef struct _CtxCtx CtxCtx;
struct _CtxCtx
{
   CtxBackend backend;
   int  width;
   int  height;
   int  cols;
   int  rows;
   int  was_down;
};

extern int _ctx_max_threads;
extern int _ctx_enable_hash_cache;
void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);
const char *
ctx_get (Ctx *ctx, const char *key);

Ctx *ctx_new_ctx (int width, int height);
Ctx *ctx_new_fb (int width, int height);
Ctx *ctx_new_headless (int width, int height);
Ctx *ctx_new_kms (int width, int height);
Ctx *ctx_new_sdl (int width, int height);
Ctx *ctx_new_term (int width, int height);
Ctx *ctx_new_termimg (int width, int height);

int ctx_resolve_font (const char *name);

#if CTX_U8_TO_FLOAT_LUT
extern float ctx_u8_float[256];
#define ctx_u8_to_float(val_u8) ctx_u8_float[((uint8_t)(val_u8))]
#else
#define ctx_u8_to_float(val_u8) (val_u8/255.0f)
#endif

static inline uint8_t ctx_float_to_u8 (float val_f)
{
#if 1 
  union { float f; uint32_t i; } u;
  u.f = 32768.0f + val_f * (255.0f / 256.0f);
  return (uint8_t)u.i;
#else
  return val_f < 0.0f ? 0 : val_f > 1.0f ? 0xff : 0xff * val_f +  0.5f;
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

static void ctx_state_set (CtxState *state, uint32_t key, float value);

CTX_STATIC void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f, float g, float h, float i);


CTX_STATIC void ctx_font_setup (Ctx *ctx);
static float ctx_state_get (CtxState *state, uint32_t hash);

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

CTX_STATIC void ctx_rasterizer_close_path (CtxRasterizer *rasterizer);
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
  const uint32_t si_ga = (v1 & 0xff00ff00);
  const uint32_t si_rb = v1 & 0x00ff00ff;
  const uint32_t di_rb = v0 & 0x00ff00ff;
  const uint32_t d_rb = si_rb - di_rb;
  const uint32_t di_ga = v0 & 0xff00ff00;
  const uint32_t d_ga = (si_ga >>8) - (di_ga>>8);
  return
     (((di_rb + ((0xff00ff + d_rb * cov)>>8)) & 0x00ff00ff)) |
     (((di_ga + (0xff00ff + d_ga * cov))      & 0xff00ff00));

#endif
}

CTX_INLINE static void ctx_lerp_RGBA8_split (const uint32_t v0, const uint32_t v1, const uint8_t dx,
                                             uint32_t *dest_ga, uint32_t *dest_rb)
{
  const uint32_t cov = dx;
  const uint32_t si_ga = v1 & 0xff00ff00;
  const uint32_t si_rb = v1 & 0x00ff00ff;
  const uint32_t di_ga = v0 & 0xff00ff00;
  const uint32_t di_rb = v0 & 0x00ff00ff;
  const uint32_t d_rb = si_rb - di_rb;
  const uint32_t d_ga = (si_ga >>8) - (di_ga >> 8);
  *dest_rb = (((di_rb + ((0xff00ff + d_rb * cov)>>8)) & 0x00ff00ff));
  *dest_ga = (((di_ga + (0xff00ff + d_ga * cov))      & 0xff00ff00));
}

CTX_INLINE static uint32_t ctx_lerp_RGBA8_merge (uint32_t di_ga, uint32_t di_rb, uint32_t si_ga, uint32_t si_rb, const uint8_t dx)
{
  const uint32_t cov = dx;
  const uint32_t d_rb = si_rb - di_rb;
  const uint32_t d_ga = (si_ga >> 8) - (di_ga >> 8);
  return
     (((di_rb + ((0xff00ff + d_rb * cov)>>8)) & 0x00ff00ff))  |
      ((di_ga + ((0xff00ff + d_ga * cov)      & 0xff00ff00)));
}

CTX_INLINE static uint32_t ctx_lerp_RGBA8_2 (const uint32_t v0, uint32_t si_ga, uint32_t si_rb, const uint8_t dx)
{
  const uint32_t cov = dx;
  const uint32_t di_ga = ( v0 & 0xff00ff00);
  const uint32_t di_rb = v0 & 0x00ff00ff;
  const uint32_t d_rb = si_rb - di_rb;
  const uint32_t d_ga = si_ga - (di_ga>>8);
  return
     (((di_rb + ((0xff00ff + d_rb * cov)>>8)) & 0x00ff00ff)) |
     (((di_ga + (0xff00ff + d_ga * cov))      & 0xff00ff00));
}

CTX_INLINE static float
ctx_lerpf (float v0, float v1, float dx)
{
  return v0 + (v1-v0) * dx;
}

CTX_INLINE static float
ctx_catmull_rom (float v0, float v1, float v2, float v3, float t)
{
   float ya = v0, yb = v1, yc = v2, yd = v3;
   float a3 = 0.5f * (-ya + 3 * yb - 3 * yc + yd);
   float a2 = 0.5f * (2 * ya - 5 * yb + 4 * yc - yd);
   float a1 = 0.5f * (-ya + yc);
   float a0 = yb;
   return a3 * t * t * t +
          a2 * t * t +
          a1 * t +
          a0;
}

CTX_INLINE static float
ctx_catmull_rom_left (float v0, float v1, float v2, float t)
{
   float ya = v0, yb = v1, yc = v2;
   float a2 = 0.5f * (ya - 2 * yb + yc);
   float a1 = 0.5f * (-3 * ya + 4 * yb - yc);
   float a0 = ya;
   return a2 * t * t +
          a1 * t +
          a0;
}

CTX_INLINE static float
ctx_catmull_rom_right (float v0, float v1, float v2, float t)
{
   float ya = v0, yb = v1, yc = v2;
   float a2 = 0.5f * (ya - 2 * yb + yc);
   float a1 = 0.5f * (-ya + yc);
   float a0 = yb;
   return a2 * t * t +
          a1 * t +
          a0;
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
uint8_t *ctx_define_texture_pixel_data (const CtxEntry *entry);
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
//#include <threads.h>
#endif
#endif
typedef struct _CtxTiled CtxTiled;


typedef struct _EvSource EvSource;
struct _EvSource
{
  void   *priv; /* private storage  */

  /* returns non 0 if there is events waiting */
  int   (*has_event) (EvSource *ev_source);

  /* get an event, the returned event should be freed by the caller  */
  char *(*get_event) (EvSource *ev_source);

  /* destroy/unref this instance */
  void  (*destroy)   (EvSource *ev_source);

  /* get the underlying fd, useful for using select on  */
  int   (*get_fd)    (EvSource *ev_source);


  void  (*set_coord) (EvSource *ev_source, double x, double y);
  /* set_coord is needed to warp relative cursors into normalized range,
   * like normal mice/trackpads/nipples - to obey edges and more.
   */

  /* if this returns non-0 select can be used for non-blocking.. */
};

struct _CtxTiled
{
   CtxBackend backend;
   void (*show_frame) (void *backend, int block);
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
   //_Atomic 
           int   thread_quit;
#endif
   int           shown_frame;
   int           render_frame;
   int           rendered_frame[CTX_MAX_THREADS];
   int           frame;
   int       min_col; // hasher cols and rows
   int       min_row;
   int       max_col;
   int       max_row;
   uint32_t  hashes[CTX_HASH_ROWS * CTX_HASH_COLS];
   int8_t    tile_affinity[CTX_HASH_ROWS * CTX_HASH_COLS]; // which render thread no is
                                                           // responsible for a tile
                                                           //

   int           pointer_down[3];

   CtxCursor     shown_cursor;
   int          vt_active;
   EvSource    *evsource[4];
   int          evsource_count;
   uint8_t      *fb;
#if CTX_THREADS
#if CTX_TILED
   cnd_t  cond;
   mtx_t  mtx;
#endif
#endif
};

static inline Ctx *ctx_backend_get_ctx (void *backend)
{
  CtxBackend *r = (CtxBackend*)backend;
  if (r) return r->ctx;
  return NULL;
}

void
_ctx_texture_prepare_color_management (CtxState  *state,
                                       CtxBuffer *buffer);

int ctx_is_set (Ctx *ctx, uint32_t hash);

static Ctx *_ctx_new_drawlist (int width, int height);


static inline void
_ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y)
{
  float x_in = *x;
  float y_in = *y;
  float w =   (x_in * m->m[2][0]) + (y_in * m->m[2][1]) + m->m[2][2];
  float w_recip = 1.0f/w;
  *x = ( (x_in * m->m[0][0]) + (y_in * m->m[0][1]) + m->m[0][2]) * w_recip;
  *y = ( (x_in * m->m[1][0]) + (y_in * m->m[1][1]) + m->m[1][2]) * w_recip;
}



static inline void
_ctx_matrix_multiply (CtxMatrix       *result,
                      const CtxMatrix *t,
                      const CtxMatrix *s)
{
  CtxMatrix r;

  for (unsigned int i = 0; i < 3; i++)
  {
    r.m[i][0] = t->m[i][0] * s->m[0][0]
              + t->m[i][1] * s->m[1][0]
              + t->m[i][2] * s->m[2][0];
    r.m[i][1] = t->m[i][0] * s->m[0][1]
              + t->m[i][1] * s->m[1][1]
              + t->m[i][2] * s->m[2][1];
    r.m[i][2] = t->m[i][0] * s->m[0][2]
              + t->m[i][1] * s->m[1][2]
              + t->m[i][2] * s->m[2][2];
  }
  *result = r;
}

static inline void
_ctx_matrix_identity (CtxMatrix *matrix)
{
  matrix->m[0][0] = 1.0f;
  matrix->m[0][1] = 0.0f;
  matrix->m[0][2] = 0.0f;
  matrix->m[1][0] = 0.0f;
  matrix->m[1][1] = 1.0f;
  matrix->m[1][2] = 0.0f;
  matrix->m[2][0] = 0.0f;
  matrix->m[2][1] = 0.0f;
  matrix->m[2][2] = 1.0f;
}

static inline void
_ctx_user_to_device_prepped (CtxState *state, float x, float y, int *out_x, int *out_y);
static inline void
_ctx_user_to_device_prepped_fixed (CtxState *state, int x, int y, int *x_out, int *y_out);

CTX_STATIC int ctx_float_to_string_index (float val);

void
ctx_render_ctx_masked (Ctx *ctx, Ctx *d_ctx, uint32_t mask);

static void ctx_state_set_blob (CtxState *state, uint32_t key, uint8_t *data, int len);


static inline void
_ctx_transform_prime (CtxState *state);

void ctx_push_backend (Ctx *ctx,
                       void *backend);
void ctx_pop_backend (Ctx *ctx);


static CTX_INLINE float ctx_fmod1f (float val)
{
  val = ctx_fabsf(val);
  return val - (int)(val);
}

static CTX_INLINE float ctx_fmodf (float val, float modulus)
{
  return ctx_fmod1f(val/modulus) * modulus;
}

static CTX_INLINE int ctx_nearly_zero(float val)
{
  return (val > 0.001f) & (val > -0.001f);
}

int ctx_glyph_find (Ctx *ctx, CtxFont *font, uint32_t unichar);

#if EMSCRIPTEN
#define CTX_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define CTX_EXPORT
#endif

float ctx_get_feather (Ctx *ctx);
void ctx_feather (Ctx *ctx, float x);


#endif

#if CTX_EVENTS
#include <sys/select.h>
#endif
