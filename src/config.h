
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
 */
#ifndef CTX_BACKEND_TEXT
#define CTX_BACKEND_TEXT 1
#endif


#define CTX_RASTERIZER_AA_SLOPE_LIMIT3    (13424/CTX_SUBDIV/15)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT5    (20425/CTX_SUBDIV/15)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT15   (50425/CTX_SUBDIV/15)

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

/* whether we have a shape-cache where we keep pre-rasterized bitmaps of
 * commonly occuring small shapes, disabled by default since it has some
 * glitches (and potential hangs with multi threading).
 */
#ifndef CTX_SHAPE_CACHE
#define CTX_SHAPE_CACHE        0
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
#define CTX_ENABLE_SHADOW_BLUR    1
#endif

#ifndef CTX_GRADIENTS
#define CTX_GRADIENTS             1
#endif

/* some optinal micro-optimizations that are known to increase code size
 */
#ifndef CTX_SORTING_NETWORKS
#define CTX_SORTING_NETWORKS         1
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

#ifndef CTX_FORMATTER
#define CTX_FORMATTER       1
#endif

#ifndef CTX_PARSER
#define CTX_PARSER          1
#endif

#ifndef CTX_CURRENT_PATH
#define CTX_CURRENT_PATH    1
#endif

#ifndef CTX_XML
#define CTX_XML             1
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
#define CTX_MAX_JOURNAL_SIZE 1024*1024*16
#endif

#ifndef CTX_DRAWLIST_STATIC
#define CTX_DRAWLIST_STATIC  0
#endif

#ifndef CTX_MIN_EDGE_LIST_SIZE
#define CTX_MIN_EDGE_LIST_SIZE   1024
#endif

#ifndef CTX_RASTERIZER_AA
#define CTX_RASTERIZER_AA 15   // vertical-AA of CTX_ANTIALIAS_DEFAULT
#endif

/* The maximum complexity of a single path
 */
#ifndef CTX_MAX_EDGE_LIST_SIZE
#define CTX_MAX_EDGE_LIST_SIZE  CTX_MIN_EDGE_LIST_SIZE
#endif

#ifndef CTX_STRINGPOOL_SIZE
  // XXX should be possible to make zero and disappear when codepaths not in use
  //     to save size, for card10 this is defined as a low number (some text
  //     properties still make use of it)
  //     
  //     for desktop-use this should be fully dynamic, possibly
  //     with chained pools, gradients are stored here.
#define CTX_STRINGPOOL_SIZE     1000 //
#endif

/* whether we dither or not for gradients
 */
#ifndef CTX_DITHER
#define CTX_DITHER 1
#endif

/*  only source-over clear and copy will work, the API still
 *  through - but the renderer is limited, for use to measure
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

/* create one-off inlined inner loop for normal blend mode
 */
#ifndef CTX_INLINED_NORMAL     
#define CTX_INLINED_NORMAL      1
#endif

#ifndef CTX_INLINED_GRADIENTS
#define CTX_INLINED_GRADIENTS   1
#endif

#ifndef CTX_BRAILLE_TEXT
#define CTX_BRAILLE_TEXT        0
#endif

/* including immintrin.h triggers building of AVX2 code paths, if - like
 * sometimes when including SDL one does want it at all do a
 * #define CTX_AVX2 0  before including ctx.h for implementation.
 */
#ifndef CTX_AVX2
#ifdef _IMMINTRIN_H_INCLUDED
#define CTX_AVX2         1
#else
#define CTX_AVX2         0
#endif
#endif

/* Build code paths for grayscale rasterization, normally this is handled
 * by the RGBA8 codepaths; on microcontrollers with eink this might be
 * a better option.
 */
#ifndef CTX_NATIVE_GRAYA8
#define CTX_NATIVE_GRAYA8       0
#endif

/* enable CMYK rasterization targets
 */
#ifndef CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK         1
#endif

/* enable color management, slightly increases CtxColor struct size, can
 * be disabled for microcontrollers.
 */
#ifndef CTX_ENABLE_CM
#define CTX_ENABLE_CM           1
#endif

#ifndef CTX_EVENTS
#define CTX_EVENTS              1
#endif

#ifndef CTX_LIMIT_FORMATS
#define CTX_LIMIT_FORMATS       0
#endif

#ifndef CTX_ENABLE_FLOAT
#define CTX_ENABLE_FLOAT 0
#endif

/* by default ctx includes all pixel formats, on microcontrollers
 * it can be useful to slim down code and runtime size by only
 * defining the used formats, set CTX_LIMIT_FORMATS to 1, and
 * manually add CTX_ENABLE_ flags for each of them.
 */
#if CTX_LIMIT_FORMATS
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

/* by including ctx-font-regular.h, or ctx-font-mono.h the
 * built-in fonts using ctx drawlist encoding is enabled
 */
#if CTX_FONT_regular || CTX_FONT_mono || CTX_FONT_bold \
  || CTX_FONT_italic || CTX_FONT_sans || CTX_FONT_serif \
  || CTX_FONT_ascii
#ifndef CTX_FONT_ENGINE_CTX
#define CTX_FONT_ENGINE_CTX        1
#endif
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

#ifdef _BABL_H
#define CTX_BABL 1
#else
#define CTX_BABL 0
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
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS  120
#endif

#ifndef CTX_MAX_FONTS
#define CTX_MAX_FONTS            3
#endif

#ifndef CTX_MAX_STATES
#define CTX_MAX_STATES           10
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
#define CTX_MAX_TEXTURES         16
#endif

#ifndef CTX_HASH_ROWS
#define CTX_HASH_ROWS            8
#endif
#ifndef CTX_HASH_COLS
#define CTX_HASH_COLS            8
#endif

#ifndef CTX_MAX_THREADS
#define CTX_MAX_THREADS          8 // runtime is max of cores/2 and this
#endif



#define CTX_RASTERIZER_EDGE_MULTIPLIER  1024

#ifndef CTX_COMPOSITE_SUFFIX
#define CTX_COMPOSITE_SUFFIX(symbol)     symbol##_default
#endif

#ifndef CTX_IMPLEMENTATION
#define CTX_IMPLEMENTATION 0
#else
#undef CTX_IMPLEMENTATION
#define CTX_IMPLEMENTATION 1
#endif


#ifdef CTX_RASTERIZER
#if CTX_RASTERIZER==0
#if CTX_SDL || CTX_FB
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
#endif
#else
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
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

#ifndef CTX_GRADIENT_CACHE_ELEMENTS
#define CTX_GRADIENT_CACHE_ELEMENTS 256
#endif

#ifndef CTX_PARSER_MAX_ARGS
#define CTX_PARSER_MAX_ARGS 20
#endif


#ifndef CTX_SCREENSHOT
#define CTX_SCREENSHOT 0
#endif

#ifndef CTX_ALSA_AUDIO
#define CTX_ALSA_AUDIO 0
#endif

#if NO_ALSA
#undef CTX_ALSA_AUDIO
#define CTX_ALSA_AUDIO 0
#endif

#ifndef CTX_AUDIO
#define CTX_AUDIO 0
#endif

#ifndef CTX_TILED
#if CTX_SDL || CTX_FB
#define CTX_TILED 1
#else
#define CTX_TILED 0
#endif
#endif

#ifndef CTX_THREADS
#if CTX_FB
#define CTX_THREADS 1
#else
#define CTX_THREADS 0
#endif
#endif


#if CTX_THREADS
#include <threads.h>
#endif
