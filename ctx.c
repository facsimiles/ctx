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

//#include "stb_image_write.h"
//#include "stb_image.h"

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

#define CTX_STATIC_FONT(font_string, font_data) \
  ctx_load_font_ctx(font_string,                \
                    ctx_font_##font_data,       \
                    sizeof (ctx_font_##font_data))

#include "Carlito-Regular.h"
#include "Carlito-Bold.h"
#include "Carlito-Italic.h"
#include "Carlito-BoldItalic.h"
#include "Cousine-Regular.h"
#include "Cousine-Bold.h"
#include "Cousine-Italic.h"
#include "Cousine-BoldItalic.h"
#include "Arimo-Regular.h"
#include "Arimo-Bold.h"
#include "Arimo-Italic.h"
#include "Arimo-BoldItalic.h"
#include "Tinos-Regular.h"
#include "Tinos-Bold.h"
#include "Tinos-Italic.h"
#include "Tinos-BoldItalic.h"

#define CTX_FONT_0 CTX_STATIC_FONT("carlito-regular", Carlito_Regular)
#define CTX_FONT_1 CTX_STATIC_FONT("carlito-bold", Carlito_Bold)
#define CTX_FONT_2 CTX_STATIC_FONT("carlito-italic", Carlito_Italic)
#define CTX_FONT_3 CTX_STATIC_FONT("carlito-bold-italic", Carlito_BoldItalic)
#define CTX_FONT_4 CTX_STATIC_FONT("cousine-regular mono-regular", Cousine_Regular)
#define CTX_FONT_5 CTX_STATIC_FONT("cousine-italic mono-italic",  Cousine_Italic)
#define CTX_FONT_6 CTX_STATIC_FONT("cousine-bold mono-bold",  Cousine_Bold)
#define CTX_FONT_7 CTX_STATIC_FONT("cousine-bold-italic mono-bold-italic",  Cousine_BoldItalic)
#define CTX_FONT_8 CTX_STATIC_FONT("tinos-regular", Tinos_Regular)
#define CTX_FONT_9 CTX_STATIC_FONT("tinos-bold", Tinos_Bold)
#define CTX_FONT_10 CTX_STATIC_FONT("tinos-italic", Tinos_Italic)
#define CTX_FONT_11 CTX_STATIC_FONT("tinos-bold-italic", Tinos_BoldItalic)
#define CTX_FONT_12 CTX_STATIC_FONT("arimo-regular sans-regular", Arimo_Regular)
#define CTX_FONT_13 CTX_STATIC_FONT("arimo-bold sans-bold", Arimo_Bold)
#define CTX_FONT_14 CTX_STATIC_FONT("arimo-italic sans-italic", Arimo_Italic)
#define CTX_FONT_15 CTX_STATIC_FONT("arimo-bold-italic sans-bold-italic", Arimo_BoldItalic)

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
