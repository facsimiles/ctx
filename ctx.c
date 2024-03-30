/* this file is the configuration and implementation of the ctx 
 * library, with adaptations to permit multipass SIMD builds.
 */

#include <stdint.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#if CTX_BABL
#include <babl/babl.h>
#endif

#include "local.conf"
//#define CTX_STB_TTF            1
#define CTX_DAMAGE_CONTROL       1// visualize damage in fb/sdl/wasm backends
#define CTX_MATH                 1
#define CTX_32BIT_SEGMENTS       1
#define CTX_EXTRAS               0
#define CTX_GSTATE_PROTECT       0
#define CTX_FONT_SHAPE_CACHE     1
#define CTX_FONTS_FROM_FILE      1
//#define CTX_MAX_EDGES            255
//#define CTX_SCANBIN              0
//#define CTX_RASTERIZER_AA        15
#define CTX_MAX_TEXTURES         1024  // default: 32
				       //
/* for misc storage with compressed/
   variable size for each save|restore*/
#define CTX_STRINGPOOL_SIZE      10000 // default: 2000
				       //
#define CTX_MIN_JOURNAL_SIZE     1024 // default: 512 ~4kb
#define CTX_MAX_JOURNAL_SIZE     1024*1024*8 // default: 1024*1024*8 // 72mb
#define CTX_MIN_EDGE_LIST_SIZE   1024*16     // default: 1024*4
					     // when max is not set it is set to min
//#define CTX_PARSER_MAXLEN      1024*1024*64 // default: 1024*1024*16

#define CTX_HASH_COLS            6 // default: 6
#define CTX_HASH_ROWS            5 // default: 5
#define CTX_MAX_KEYDB            128  // default: 64
#define CTX_GET_CONTENTS         1
#define CTX_PROTOCOL_U8_COLOR    1
#include "stb_image.h"

   typedef unsigned char   stbtt_uint8;
   typedef signed   char   stbtt_int8;
   typedef unsigned short  stbtt_uint16;
   typedef signed   short  stbtt_int16;                     
   typedef unsigned int    stbtt_uint32;
   typedef signed   int    stbtt_int32;


//#include "stb_truetype.h"

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

#define ITK_HAVE_FS 0

#define CTX_STATIC_FONT(font) \
  ctx_load_font_ctx(ctx_font_##font##_name, \
                    ctx_font_##font,       \
                    sizeof (ctx_font_##font))

//#define CTX_FONT0 CTX_STATIC_FONT("sans-ctx", ascii)
#if CTX_STATIC_FONTS
#include "Cousine-Regular.h"
#include "Arimo-Regular.h"
//#include "Roboto-Regular.h"
#if 0  // build more static fonts and uncomment these..
#include "Carlito-Regular.h"
#include "Carlito-Bold.h"
#include "Carlito-Italic.h"
#include "Carlito-BoldItalic.h"
#include "Cousine-Bold.h"
#include "Cousine-Italic.h"
#include "Cousine-BoldItalic.h"
#include "Arimo-Bold.h"
#include "Arimo-Italic.h"
#include "Arimo-BoldItalic.h"
#include "Tinos-Regular.h"
#include "Tinos-Bold.h"
#include "Tinos-Italic.h"
#include "Tinos-BoldItalic.h"
#endif

#define CTX_FONT_0   CTX_STATIC_FONT(Arimo_Regular)
//#define CTX_FONT_0   CTX_STATIC_FONT(Roboto_Regular)
#if 0
#define CTX_FONT_1   CTX_STATIC_FONT(Arimo_Bold)
#define CTX_FONT_2   CTX_STATIC_FONT(Arimo_Italic)
#define CTX_FONT_3   CTX_STATIC_FONT(Arimo_BoldItalic)
#define CTX_FONT_4   CTX_STATIC_FONT(Tinos_Regular)
#define CTX_FONT_5   CTX_STATIC_FONT(Tinos_Bold)
#define CTX_FONT_6   CTX_STATIC_FONT(Tinos_Italic)
#define CTX_FONT_7   CTX_STATIC_FONT(Tinos_BoldItalic)
#endif

#define CTX_FONT_8   CTX_STATIC_FONT(Cousine_Regular)
#if 0
#define CTX_FONT_9   CTX_STATIC_FONT(Cousine_Italic)
#define CTX_FONT_10  CTX_STATIC_FONT(Cousine_Bold)
#define CTX_FONT_11  CTX_STATIC_FONT(Cousine_BoldItalic)
#define CTX_FONT_12  CTX_STATIC_FONT(Carlito_Regular)
#define CTX_FONT_13  CTX_STATIC_FONT(Carlito_Bold)
#define CTX_FONT_14  CTX_STATIC_FONT(Carlito_Italic)
#define CTX_FONT_15  CTX_STATIC_FONT(Carlito_BoldItalic)
#endif
#endif

#define CTX_IMPLEMENTATION 1

#include "ctx.h"

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
#else
#undef CTX_ENABLE_CM
#define CTX_ENABLE_CM  0
#endif


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
