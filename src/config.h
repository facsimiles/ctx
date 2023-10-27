/* definitions that determine which features are included and their settings,
 * for particular platforms - in particular microcontrollers ctx might need
 * tuning for different quality/performance/resource constraints.
 *
 * the way to configure ctx is to set these defines, before both including it
 * as a header and in the file where CTX_IMPLEMENTATION is set to include the
 * implementation for different featureset and runtime settings.
 *
 */

/* whether the font rendering happens in backend or front-end of API, the
 * option is used set to 0 by the tool that converts ttf fonts to ctx internal
 * representation - both should be possible so that this tool can be made
 * into a TTF/OTF font import at runtime (perhaps even with live subsetting).
 *
 * improving this feature and making it runtime selectable could also
 * be part of encoding all text as beziers upon pdf export
 */
#ifndef CTX_BACKEND_TEXT
#define CTX_BACKEND_TEXT 1
#endif


#define CTX_RASTERIZER_AA_SLOPE_LIMIT3           (65536/CTX_SUBDIV/15)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA   (65536/CTX_SUBDIV/15)
//#define CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA (120536/CTX_SUBDIV/15)
//#define CTX_RASTERIZER_AA_SLOPE_LIMIT3_FAST_AA (105000/CTX_SUBDIV/15)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT5           (140425/CTX_SUBDIV/15)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT15          (260425/CTX_SUBDIV/15)

/* subpixel-aa coordinates used in BITPACKing of drawlist
 *
 * powers of 2 is faster
 */
#ifndef CTX_SUBDIV
#define CTX_SUBDIV   8  //  max framebufer width 4095
//#define CTX_SUBDIV  10  //  max framebufer width 3250
//#define CTX_SUBDIV  16  //  max framebufer width 2047
//#define CTX_SUBDIV  24  //  max framebufer width 1350
//#define CTX_SUBDIV  32  //  max framebufer width 1023
#endif


// 8    12 68 40 24
// 16   12 68 40 24
/* scale-factor for font outlines prior to bit quantization by CTX_SUBDIV
 *
 * changing this also changes font file format - the value should be baked
 * into the ctxf files making them less dependent on the ctx used to
 * generate them
 */
#define CTX_BAKE_FONT_SIZE    160

/* pack some linetos/curvetos/movetos into denser drawlist instructions,
 * permitting more vectors to be stored in the same space, experimental
 * feature with added overhead.
 */
#ifndef CTX_BITPACK
#define CTX_BITPACK           1
#endif

#ifndef CTX_PARSER_FIXED_TEMP
#define CTX_PARSER_FIXED_TEMP 0
         // when 1  CTX_PARSER_MAXLEN is the fixed max stringlen
#endif   // and no allocations happens beyond creating the parser,
         // when 0 the scratchbuf for parsing is a separate dynamically
         // growing buffer, that maxes out at CTX_PARSER_MAXLEN
         //
#ifndef CTX_PARSER_MAXLEN
#if CTX_PARSER_FIXED_TEMP
#define CTX_PARSER_MAXLEN  1024*128        // This is the maximum texture/string size supported
#else
#define CTX_PARSER_MAXLEN  1024*1024*16    // 16mb
#endif
#endif


#ifndef CTX_FAST_FILL_RECT
#define CTX_FAST_FILL_RECT 1    /*  matters most for tiny rectangles where it shaves overhead, for larger rectangles
                                    a ~15-20% performance win can be seen. */
#endif

#ifndef CTX_FAST_STROKE_RECT
#define CTX_FAST_STROKE_RECT 0
#endif


#ifndef CTX_COMPOSITING_GROUPS
#define CTX_COMPOSITING_GROUPS   1
#endif

/* maximum nesting level of compositing groups
 */
#ifndef CTX_GROUP_MAX
#define CTX_GROUP_MAX             8
#endif

#ifndef CTX_ENABLE_CLIP
#define CTX_ENABLE_CLIP           1
#endif

/* use a 1bit clip buffer, saving RAM on microcontrollers, other rendering
 * will still be antialiased.
 */
#ifndef CTX_1BIT_CLIP
#define CTX_1BIT_CLIP             0
#endif


#ifndef CTX_ENABLE_SHADOW_BLUR
#define CTX_ENABLE_SHADOW_BLUR    0
#endif

#ifndef CTX_GRADIENTS
#define CTX_GRADIENTS             1
#endif

#ifndef CTX_ALIGNED_STRUCTS
#define CTX_ALIGNED_STRUCTS       1
#endif

#ifndef CTX_GRADIENT_CACHE
#define CTX_GRADIENT_CACHE        1
#endif


#ifndef CTX_FONTS_FROM_FILE
#define CTX_FONTS_FROM_FILE  0
#endif

#ifndef CTX_GET_CONTENTS
#if CTX_FONTS_FROM_FILE
#define CTX_GET_CONTENTS    1
#else
#define CTX_GET_CONTENTS    0
#endif
#endif

#ifndef CTX_FORMATTER
#define CTX_FORMATTER       1
#endif

#ifndef CTX_PARSER
#define CTX_PARSER          1
#endif

#ifndef CTX_CURRENT_PATH
#define CTX_CURRENT_PATH    1
#endif

#ifndef CTX_VT
#define CTX_VT              0
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
 * edgelist and drawlist.
 */
#ifndef CTX_MIN_JOURNAL_SIZE
#define CTX_MIN_JOURNAL_SIZE      512
#endif

/* The maximum size we permit the drawlist to grow to,
 * the memory used is this number * 9, where 9 is sizeof(CtxEntry)
 */
#ifndef CTX_MAX_JOURNAL_SIZE
//#define CTX_MAX_JOURNAL_SIZE   CTX_MIN_JOURNAL_SIZE
#define CTX_MAX_JOURNAL_SIZE 1024*1024*8
#endif

#ifndef CTX_DRAWLIST_STATIC
#define CTX_DRAWLIST_STATIC  0
#endif

#ifndef CTX_MIN_EDGE_LIST_SIZE
#define CTX_MIN_EDGE_LIST_SIZE   1024*4
#endif

#ifndef CTX_RASTERIZER_AA
#define CTX_RASTERIZER_AA 15   // vertical-AA of CTX_ANTIALIAS_DEFAULT
#endif

/* The maximum complexity of a single path
 */
#ifndef CTX_MAX_EDGE_LIST_SIZE
#define CTX_MAX_EDGE_LIST_SIZE  CTX_MIN_EDGE_LIST_SIZE
#endif

#ifndef CTX_MAX_KEYDB
#define CTX_MAX_KEYDB 64 // number of entries in keydb
                         // entries are "copy-on-change" between states
#endif

#ifndef CTX_STRINGPOOL_SIZE
  // XXX should be possible to make zero and disappear when codepaths not in use
  //     to save size, for card10 this is defined as a low number (some text
  //     properties still make use of it)
  //     
  //     for desktop-use this should be fully dynamic, possibly
  //     with chained pools, gradients are stored here.
#define CTX_STRINGPOOL_SIZE     2000 //
#endif

#ifndef CTX_32BIT_SEGMENTS
#define CTX_32BIT_SEGMENTS 1  // without this clipping problems might
                              // occur when drawing far outside the viewport
                              // or with large translate amounts
                              // on micro controllers you most often will
                              // want this set to 0
#endif

/* whether we dither or not for gradients
 */
#ifndef CTX_DITHER
#define CTX_DITHER 0
#endif

/*  with 0 only source-over clear and copy will work, the API still
 *  through - but the backend is limited, for use to measure
 *  size and possibly in severely constrained ROMs.
 */
#ifndef CTX_BLENDING_AND_COMPOSITING
#define CTX_BLENDING_AND_COMPOSITING 1
#endif

/*  this forces the inlining of some performance
 *  critical paths.
 */
#ifndef CTX_FORCE_INLINES
#define CTX_FORCE_INLINES               1
#endif

/* create one-off inlined inner loop for normal blend mode (for floating point,
 * and grayscale for RGBA8 manual loops overrrides. Disabling this should speed
 * up compiles at penalty for the given formats.
 */
#ifndef CTX_INLINED_NORMAL     
#define CTX_INLINED_NORMAL      0
#endif

/*
 *  do not use manual RGBA8 code but rely on ctx inline templating
 */
#ifndef CTX_INLINED_NORMAL     
#define CTX_INLINED_NORMAL_RGBA8  0
#endif

#ifndef CTX_RASTERIZER_SWITCH_DISPATCH
#define CTX_RASTERIZER_SWITCH_DISPATCH  1 // marginal improvement for some
                                          // modes, maybe get rid of this?
#endif

#ifndef CTX_U8_TO_FLOAT_LUT
#define CTX_U8_TO_FLOAT_LUT  0
#endif

#ifndef CTX_INLINED_GRADIENTS
#define CTX_INLINED_GRADIENTS   1
#endif

#ifndef CTX_BRAILLE_TEXT
#define CTX_BRAILLE_TEXT        0
#endif

/* Build code paths for grayscale rasterization, this makes clipping
 * faster.
 */
#ifndef CTX_NATIVE_GRAYA8
#define CTX_NATIVE_GRAYA8       1
#endif

/* enable CMYK rasterization targets
 */
#ifndef CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK         1
#endif

/* enable color management, slightly increases CtxColor struct size, should
 * be disabled for microcontrollers.
 */


#ifndef CTX_EVENTS
#define CTX_EVENTS              1
#endif

#ifndef CTX_MAX_DEVICES
#define CTX_MAX_DEVICES 16
#endif

#ifndef CTX_MAX_KEYBINDINGS
#define CTX_MAX_KEYBINDINGS 256
#endif


#ifndef CTX_PROTOCOL_U8_COLOR
#define CTX_PROTOCOL_U8_COLOR 0
#endif

#ifndef CTX_TERMINAL_EVENTS
#if CTX_EVENTS
#define CTX_TERMINAL_EVENTS 1
#else
#define CTX_TERMINAL_EVENTS 0
#endif
#endif

#ifndef CTX_LIMIT_FORMATS
#define CTX_LIMIT_FORMATS       0
#endif

#ifndef CTX_ENABLE_FLOAT
#define CTX_ENABLE_FLOAT        0
#endif

/* by default ctx includes all pixel formats, on microcontrollers
 * it can be useful to slim down code and runtime size by only
 * defining the used formats, set CTX_LIMIT_FORMATS to 1, and
 * manually add CTX_ENABLE_ flags for each of them.
 */
#if CTX_LIMIT_FORMATS
#if CTX_NATIVE_GRAYA8
#define CTX_ENABLE_GRAYA8               1
#define CTX_ENABLE_GRAY8                1
#endif
#else

#define CTX_ENABLE_GRAY1                1
#define CTX_ENABLE_GRAY2                1
#define CTX_ENABLE_GRAY4                1
#define CTX_ENABLE_GRAY8                1
#define CTX_ENABLE_GRAYA8               1
#define CTX_ENABLE_GRAYF                1
#define CTX_ENABLE_GRAYAF               1

#define CTX_ENABLE_RGB8                 1
#define CTX_ENABLE_RGBA8                1
#define CTX_ENABLE_BGRA8                1
#define CTX_ENABLE_RGB332               1
#define CTX_ENABLE_RGB565               1
#define CTX_ENABLE_RGB565_BYTESWAPPED   1
#define CTX_ENABLE_RGBAF                1
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT                1
#define CTX_ENABLE_YUV420               1

#if CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK8                1
#define CTX_ENABLE_CMYKA8               1
#define CTX_ENABLE_CMYKAF               1
#endif
#endif

#ifndef CTX_RGB565_ALPHA
#define CTX_RGB565_ALPHA                0   // when enabled pure purple is transparent,
                                            // for a ~15% overall performance hit
#endif

#ifndef CTX_RGB332_ALPHA
#define CTX_RGB332_ALPHA                0   // when enabled pure purple is transparent,
                                            // for a ~15% overall performance hit
#endif

#ifndef CTX_RESOLVED_FONTS
#define CTX_RESOLVED_FONTS 8   // how many font-strings to cache the resolution for in a static
			       // hash-table
#endif

/* by including ctx-font-regular.h, or ctx-font-mono.h the
 * built-in fonts using ctx drawlist encoding is enabled
 */
#ifndef CTX_NO_FONTS
#ifndef CTX_FONT_ENGINE_CTX
#define CTX_FONT_ENGINE_CTX        1
#endif
#endif

#ifndef CTX_ONE_FONT_ENGINE
#define CTX_ONE_FONT_ENGINE 0
#endif


#ifndef CTX_FONT_ENGINE_CTX_FS
#define CTX_FONT_ENGINE_CTX_FS 0
#endif

/* If stb_strutype.h is included before ctx.h add integration code for runtime loading
 * of opentype fonts.
 */
#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
#ifndef CTX_FONT_ENGINE_STB
#define CTX_FONT_ENGINE_STB        1
#endif
#else
#define CTX_FONT_ENGINE_STB        0
#endif

#ifndef CTX_BABL
#ifdef _BABL_H
#define CTX_BABL 1
#else
#define CTX_BABL 0
#endif
#endif

#ifndef _BABL_H
#undef CTX_BABL
#define CTX_BABL 0
#endif

#ifndef CTX_ALWAYS_USE_NEAREST_FOR_SCALE1
#define CTX_ALWAYS_USE_NEAREST_FOR_SCALE1 0
#endif

/* include the bitpack packer, can be opted out of to decrease code size
 */
#ifndef CTX_BITPACK_PACKER
#define CTX_BITPACK_PACKER 0
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

#ifdef CTX_ENABLE_CMYKF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_GRAYF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_GRAYAF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_RGBAF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_CMYKAF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_CMYKF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
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
#define CTX_ENABLE_CMYK   1
#endif

#define CTX_PI                              3.141592653589793f
#ifndef CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS  (128)
#endif

#ifndef CTX_MAX_FRAMEBUFFER_WIDTH
#define CTX_MAX_FRAMEBUFFER_WIDTH 2560
#endif

#ifndef CTX_MAX_FONTS
#define CTX_MAX_FONTS            32
#endif

#ifndef CTX_GLYPH_CACHE
#define CTX_GLYPH_CACHE 1
#endif

#ifndef CTX_GLYPH_CACHE_SIZE
#define CTX_GLYPH_CACHE_SIZE     128
#endif

#ifndef CTX_MAX_STATES
#define CTX_MAX_STATES           16
#endif

#ifndef CTX_MAX_EDGES
#define CTX_MAX_EDGES            257
#endif

#ifndef CTX_MAX_LINGERING_EDGES
#define CTX_MAX_LINGERING_EDGES  64
#endif


#ifndef CTX_MAX_PENDING
#define CTX_MAX_PENDING          128
#endif

#ifndef CTX_MAX_TEXTURES
#define CTX_MAX_TEXTURES         32
#endif

#ifndef CTX_HASH_ROWS
#define CTX_HASH_ROWS            6
#endif
#ifndef CTX_HASH_COLS
#define CTX_HASH_COLS            5
#endif

#ifndef CTX_INLINE_FILL_RULE
#define CTX_INLINE_FILL_RULE 1
#endif

#ifndef CTX_MAX_THREADS
#define CTX_MAX_THREADS          8 // runtime is max of cores/2 and this
#endif

#ifndef CTX_FRAGMENT_SPECIALIZE
#define CTX_FRAGMENT_SPECIALIZE 1
#endif

#define CTX_RASTERIZER_EDGE_MULTIPLIER  1024
                                        // increasing this to 2048
                                        // removes artifacts in top half of res-diagram -
                                        // but reduces maximum available buffer width
#ifndef CTX_IMPLEMENTATION
#define CTX_IMPLEMENTATION 0
#else
#undef CTX_IMPLEMENTATION
#define CTX_IMPLEMENTATION 1
#endif

#ifndef CTX_MAX_SCANLINE_LENGTH
#define CTX_MAX_SCANLINE_LENGTH 4096
#endif


#ifndef CTX_MAX_CBS
#define CTX_MAX_CBS              1 //128
#endif

#ifndef CTX_STATIC_OPAQUE // causes a CTX_MAX_SCANLINE_LENGTH
                          // buffer of 255 bytes to be part of
                          // rasterizer
#define CTX_STATIC_OPAQUE 1
#endif

#ifndef CTX_SYNC_FRAMES
#define CTX_SYNC_FRAMES  1
#endif

#ifdef CTX_RASTERIZER
#if CTX_RASTERIZER==0
#if CTX_SDL || CTX_FB || CTX_HEADLESS
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
#endif
#else
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
#endif
#endif

#if CTX_SDL || CTX_FB || CTX_HEADLESS
#if CTX_EVENTS
#undef CTX_EVENTS
#endif
#define CTX_EVENTS 1
#endif




#ifndef CTX_HEADLESS

#if CTX_FB || CTX_SDL || CTX_KMS
#define CTX_HEADLESS 1
#endif
#endif


#ifndef CTX_GRADIENT_CACHE_ELEMENTS
#define CTX_GRADIENT_CACHE_ELEMENTS 256
#endif

#ifndef CTX_PARSER_MAX_ARGS
#define CTX_PARSER_MAX_ARGS 20
#endif

#ifndef CTX_MAX_DASHES
#define CTX_MAX_DASHES CTX_PARSER_MAX_ARGS
#endif

#ifndef CTX_SCREENSHOT
#define CTX_SCREENSHOT 0
#endif

#ifndef CTX_ALSA
#define CTX_ALSA 0
#endif

#ifndef CTX_AUDIO
#define CTX_AUDIO 0
#endif

#if CTX_AUDIO==0
#if CTX_ALSA
#undef CTX_ALSA
#define CTX_ALSA 0
#endif
#endif

#ifndef CTX_CURL
#define CTX_CURL 0
#endif

#ifndef CTX_TILED
#if CTX_SDL || CTX_FB || CTX_KMS || CTX_HEADLESS
#define CTX_TILED 1
#else
#define CTX_TILED 0
#endif
#if !CTX_RASTERIZER
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
#endif
#endif


#ifndef CTX_TILED_MERGE_HORIZONTAL_NEIGHBORS
#define CTX_TILED_MERGE_HORIZONTAL_NEIGHBORS 1
#endif


#ifndef CTX_THREADS
#if CTX_TILED
#define CTX_THREADS 1
#else
#define CTX_THREADS 0
#endif
#endif

#if CTX_THREADS
#include <pthread.h>
#define mtx_lock pthread_mutex_lock
#define mtx_unlock pthread_mutex_unlock
#define mtx_t pthread_mutex_t
#define cnd_t pthread_cond_t
#define mtx_plain NULL
#define mtx_init pthread_mutex_init
#define cnd_init(a) pthread_cond_init(a,NULL)
#define cnd_wait pthread_cond_wait
#define cnd_broadcast pthread_cond_broadcast
#define thrd_create(tid, tiled_render_fun, args) pthread_create(tid, NULL, tiled_render_fun, args)
#define thrd_t pthread_t
#else

#define mtx_lock(a)
#define mtx_unlock(a)
#define mtx_t size_t
#define cnd_t size_t
#define mtx_plain 0
#define mtx_init(a,b)
#define cnd_init(a)
#define cnd_wait(a,b)
#define cnd_broadcast(c)
#define thrd_create(tid, tiled_render_fun, args) 0
#define thrd_t size_t

#endif

#ifndef CTX_SIMD_SUFFIX
#define CTX_SIMD_SUFFIX(symbol) symbol##_generic
#define CTX_SIMD_BUILD 0
#else


#define CTX_SIMD_BUILD 1
#ifdef CTX_COMPOSITE
#undef CTX_COMPOSITE
#define CTX_COMPOSITE 1
#endif

#endif


#if CTX_RASTERIZER
#ifndef CTX_COMPOSITE
#define CTX_COMPOSITE 1
#endif
#else
#ifndef CTX_COMPOSITE
#define CTX_COMPOSITE 0
#endif
#endif

#ifndef CTX_COMPOSITE
#define CTX_COMPOSITE 0
#endif

#ifndef CTX_MAX_GRADIENT_STOPS
#define CTX_MAX_GRADIENT_STOPS 16
#endif

#ifndef CTX_BRANCH_HINTS
#define CTX_BRANCH_HINTS  1
#endif

#ifdef EMSCRIPTEN
#define CTX_WASM 1
#else
#define CTX_WASM 0
#endif

#ifndef CTX_MAX_LISTEN_FDS
#define CTX_MAX_LISTEN_FDS 128 // becomes max clients..
#endif

#if CTX_WASM
#undef CTX_THREADS
#define CTX_THREADS 0
#undef CTX_HEADLESS
#define CTX_HEADLESS 0
#undef CTX_TILED
#define CTX_TILED 0
#undef CTX_EVENTS
#define CTX_EVENTS 1
#undef CTX_PARSER
#define CTX_PARSER 1
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
#endif

#ifndef CTX_TINYVG
#define CTX_TINYVG 0
#endif

#ifndef CTX_PDF
#define CTX_PDF 0
#endif

#if CTX_IMAGE_WRITE

#if CTX_AUDIO==0
#define MINIZ_NO_INFLATE_APIS
#endif

#else

#if CTX_AUDIO==0
#define MINIZ_NO_DEFLATE_APIS
#define MINIZ_NO_INFLATE_APIS
#endif

#endif

#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_STDIO


//#define uncompress tinf_uncompress
//#define Z_OK TINF_OK
//#define Z_BUF_ERROR TINF_BUF_ERROR
//#define Z_DATA_ERROR TINF_DATA_ERROR

#ifndef CTX_RAW_KB_EVENTS
#define CTX_RAW_KB_EVENTS 0
#endif


#ifndef CTX_BAREMETAL
#define CTX_BAREMETAL 0
#endif

#ifndef CTX_ENABLE_CM
#if CTX_BAREMETAL
#define CTX_ENABLE_CM           0
#else
#define CTX_ENABLE_CM           1
#endif
#endif

#if CTX_IMPLEMENTATION
#ifndef SQUOZE_IMPLEMENTATION
#define SQUOZE_IMPLEMENTATION         1
#define SQUOZE_LIMIT_IMPLEMENTATIONS  1
#define SQUOZE_IMPLEMENTATION_32_UTF8 1
#define SQUOZE_USE_INTERN             0
#endif
#endif

#ifndef CTX_PTY
#define CTX_PTY 1
#endif

#ifndef CTX_STROKE_1PX   
#define CTX_STROKE_1PX    0
#endif

#ifndef CTX_PICO
#define CTX_PICO 0
#endif


#ifndef CTX_GSTATE_PROTECT
#define CTX_GSTATE_PROTECT  0
#endif

#ifndef CTX_COMPOSITE_O3
#define CTX_COMPOSITE_O3 0
#endif

#ifndef CTX_COMPOSITE_O2
#define CTX_COMPOSITE_O2 0
#endif

#ifndef CTX_RASTERIZER_O3
#define CTX_RASTERIZER_O3 0
#endif

#ifndef CTX_RASTERIZER_O2
#define CTX_RASTERIZER_O2 0
#endif

#if CTX_KMS || CTX_FB
#undef CTX_RAW_KB_EVENTS
#define CTX_RAW_KB_EVENTS 1
#endif

#ifndef CTX_YUV_LUTS
#define  CTX_YUV_LUTS 0
#endif
