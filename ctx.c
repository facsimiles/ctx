#if !__COSMOPOLITAN__

#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>

#endif

#ifndef NO_BABL
#include <babl/babl.h>
#endif

#ifndef NO_SDL
#include <SDL.h>
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

#if !__COSMOPOLITAN__
#include "stb_truetype.h"
#include "stb_image.h"
#endif

#define CTX_DITHER               0 //
#define CTX_DAMAGE_CONTROL       1// visualize damage in fb/sdl backends
#define CTX_PARSER               1
#define CTX_FORMATTER            1
#define CTX_EVENTS               1
#define CTX_BRAILLE_TEXT         1
#define CTX_NATIVE_GRAYA8        1

#define CTX_GRADIENT_CACHE       1 // does not work well with threads
#define CTX_BITPACK_PACKER       0 // turned of due to asan report
#define CTX_ENABLE_CMYK          1
#define CTX_ENABLE_CM            1
#define CTX_RASTERIZER_AA        15
#define CTX_AUDIO                1
#define CTX_FONTS_FROM_FILE      1
#define CTX_ALSA_AUDIO           1
#define CTX_VT                   1
#define CTX_MAX_TEXTURES         256
#define CTX_STRINGPOOL_SIZE      10000 // for misc storage with compressed/
                                       // variable size for each save|restore
#define CTX_MATH                 1

//#define CTX_MIN_JOURNAL_SIZE   512         // ~4kb
#define CTX_MAX_JOURNAL_SIZE     1024*1024*8 // 72mb
#define CTX_MIN_EDGE_LIST_SIZE   4096*2
//#define CTX_PARSER_MAXLEN      1024*1024*64

//#define CTX_FONT_ENGINE_CTX_FS   1

//#define CTX_HASH_COLS  1
//#define CTX_HASH_ROWS  1

#define CTX_SHAPE_CACHE          1   // causes some glitching
#define CTX_SHAPE_CACHE_MAX_DIM  128
#define CTX_SHAPE_CACHE_DIM      (64*64)
#define CTX_SHAPE_CACHE_ENTRIES  (2048)   // max-total of 8mb, in normal use a lot less

#define CTX_SCREENSHOT           0  // it brings in stb_save_image dep so is not default,
                                    // rewrite as ppm?

#define CTX_IMPLEMENTATION 1
#define CTX_RASTERIZER     1

#include "ctx.h"


