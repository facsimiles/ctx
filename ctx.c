/* this file contains a default configuration, it produces ctx.o
 * which is reused by the other targets in the build system
 */

#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#ifndef NO_SDL
#include <SDL.h>
#endif
#include <math.h>

#define _CTX_INTERNAL_FONT_ // drops ascii in favor of regular
#include "ctx-font-regular.h"
#include "ctx-font-mono.h"

//#include "Roboto-Regular.h"
//#include "DejaVuSansMono.h"
//#include "DejaVuSans.h"
//#include "0xA000-Mono.h"
//#include "unscii-16.h"

#include "stb_truetype.h"
#include "stb_image.h"

#define CTX_DAMAGE_CONTROL       0// visualize damage in fb/sdl backends
#define CTX_PARSER               1
#define CTX_FORMATTER            1
#define CTX_EVENTS               1
#define CTX_BITPACK_PACKER       0 // turned of due to asan report
#define CTX_GRADIENT_CACHE       1
#define CTX_ENABLE_CMYK          1
#define CTX_ENABLE_CM            1
#define CTX_RASTERIZER_AA        5
#define CTX_FORCE_AA             1

#define CTX_STRINGPOOL_SIZE      10000 // for misc storage with compressed/
                                       // variable size for each save|restore
#define CTX_MATH                 1
#define CTX_MIN_JOURNAL_SIZE     1024*128
#define CTX_MIN_EDGE_LIST_SIZE   2048

#define CTX_SHAPE_CACHE          0 // disabled - it is buggy with threads
#define CTX_SHAPE_CACHE_MAX_DIM  96
#define CTX_SHAPE_CACHE_DIM      (64*64)
#define CTX_SHAPE_CACHE_ENTRIES  (512)

#ifndef CTX_AVX2
#define CTX_AVX2 0 
#else
#include <immintrin.h> // is detected by ctx, and enables AVX2
#endif

#define CTX_HAVE_SIMD  1 // makes ctx call our SIMD-setup dispatcher

#define CTX_IMPLEMENTATION 1
#define CTX_RASTERIZER 1


#include "ctx.h"

extern CtxPixelFormatInfo *ctx_pixel_formats;
extern CtxPixelFormatInfo  ctx_pixel_formats_avx2[];
extern CtxPixelFormatInfo  ctx_pixel_formats_sse2[];
extern CtxPixelFormatInfo  ctx_pixel_formats_mmx[];

void ctx_simd_setup ()
{
  static int done = 0;
  if (done) return;
  done = 1;
  if(__builtin_cpu_supports("mmx"))
  {
    ctx_pixel_formats = ctx_pixel_formats_mmx;
  }
  if(__builtin_cpu_supports("sse3"))
  {
    ctx_pixel_formats = ctx_pixel_formats_sse2;
  }
  if(__builtin_cpu_supports("avx2"))
  {
    ctx_pixel_formats = ctx_pixel_formats_avx2;
  }
}
