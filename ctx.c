/* this file is the configuration and implementation of the ctx 
 * library, with adaptations to permit multipass SIMD builds.
 */


#include <stdint.h>
#include <termios.h>
#include <unistd.h>

#if CTX_BABL
#include <babl/babl.h>
#endif

#define CTX_DAMAGE_CONTROL       1// visualize damage in fb/sdl backends
#define CTX_PARSER               1
#define CTX_FORMATTER            1
#define CTX_EVENTS               1
#define CTX_CLIENTS              1
#define CTX_FONTS_FROM_FILE      1

#define CTX_TINYVG               1

#define CTX_SHAPE_CACHE          1

#define CTX_DITHER               0 //
#define CTX_BRAILLE_TEXT         1
#define CTX_ENABLE_CMYK          1
#define CTX_AUDIO                1
#define CTX_MAX_TEXTURES         1024
#define CTX_STRINGPOOL_SIZE      10000 // for misc storage with compressed/
                                       // variable size for each save|restore
//#define CTX_MIN_JOURNAL_SIZE   512         // ~4kb
#define CTX_MAX_JOURNAL_SIZE     1024*1024*8 // 72mb
#define CTX_MIN_EDGE_LIST_SIZE   4096*2
//#define CTX_PARSER_MAXLEN      1024*1024*64

//#define CTX_FONT_ENGINE_CTX_FS   1
//#define CTX_HASH_COLS  1
//#define CTX_HASH_ROWS  1

#define CTX_SCREENSHOT           1 
#define CTX_RASTERIZER           1

#include "stb_image_write.h"

#ifdef CTX_SIMD_X86_64_V2
#define CTX_SIMD_SUFFIX(symbol)  symbol##_x86_64_v2
#endif
#ifdef CTX_SIMD_X86_64_V3
#define CTX_SIMD_SUFFIX(symbol)  symbol##_x86_64_v3
#endif
#ifdef CTX_SIMD_ARM_NEON
#define CTX_SIMD_SUFFIX(symbol)  symbol##_arm_neon
#endif

#ifndef CTX_SIMD_SUFFIX

#if CTX_SDL
#include <SDL.h>
#endif

#if CTX_CAIRO
#include <cairo.h>
#endif

#define _CTX_INTERNAL_FONT_ // drops ascii in favor of regular
#include "ctx-font-regular.h"
#include "ctx-font-mono.h"
//#include "DejaVuSansMono.h"
//#include "DejaVuSans.h"
//#include "Roboto-Regular.h"
//#include "NotoMono-Regular.h"
//#include "DejaVuSansMono.h"
//#include "0xA000-Mono.h"
//#include "unscii-16.h"


// optional dependencies
#if 0 
  #include "stb_truetype.h"
  #include "stb_image.h"
  #include "stb_image_write.h"
#endif


#define CTX_IMPLEMENTATION 1

#include "ctx.h"

#define ITK_IMPLEMENTATION 1
#include "itk.h"   // for completeness, itk wants to be built in the ctx
                   // compilation unit to be influenced by the ctx config

#if CTX_SIMD
#if CTX_X86_64
void ctx_simd_setup_x86_64_v2 (void);
void ctx_simd_setup_x86_64_v3 (void);
void ctx_simd_setup (void)
{
  switch (ctx_x86_64_level())
  {
    default:
    case 0:
    case 1: break;
    case 2: ctx_simd_setup_x86_64_v2 ();break;
    case 3: ctx_simd_setup_x86_64_v3 ();break;
  }
}
#else // must be arm if we have SIMD enabled and are not x86_64
void ctx_simd_setup_arm_neon (void);
int ctx_arm_has_neon (int *armv);
void ctx_simd_setup (void)
{
  if (ctx_arm_has_neon (NULL))
    ctx_simd_setup_arm_neon ();
}
#endif
#endif

#if CTX_BABL
#include <babl/babl.h>
#endif


#define CTX_BRAILLE_TEXT         1
#define CTX_ENABLE_CMYK          1

#ifdef CTX_SIMD_X86_64_V2
#define CTX_SIMD_SUFFIX(symbol)  symbol##_x86_64_v2
#endif
#ifdef CTX_SIMD_X86_64_V3
#define CTX_SIMD_SUFFIX(symbol)  symbol##_x86_64_v3
#endif
#ifdef CTX_SIMD_ARM_NEON
#define CTX_SIMD_SUFFIX(symbol)  symbol##_armv7l_neon
#endif

#endif

#include "ctx.h"
