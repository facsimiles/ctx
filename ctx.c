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

#include "ctx-font-regular.h"
#include "ctx-font-mono.h"

//#include "Roboto-Regular.h"
//#include "DejaVuSansMono.h"
//#include "DejaVuSans.h"
//#include "0xA000-Mono.h"
//#include "unscii-16.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define CTX_BACKEND_TEXT         1
#define CTX_PARSER               1
#define CTX_FORMATTER            1
#define CTX_EVENTS               1
#define CTX_BITPACK_PACKER       1
#define CTX_GRADIENT_CACHE       1
#define CTX_ENABLE_CMYK          1
#define CTX_ENABLE_CM            1

#define CTX_SHAPE_CACHE          0 // disabled - it is buggy with threads
#define CTX_SHAPE_CACHE_MAX_DIM  96
#define CTX_SHAPE_CACHE_DIM      (64*64)
#define CTX_SHAPE_CACHE_ENTRIES  (512)
#define CTX_MATH                 1
#define CTX_MAX_JOURNAL_SIZE     1024*64
#ifndef CTX_AVX2
#define CTX_AVX2 0 // forcing AVX2 even if immintrin is detected -
#else
#include <immintrin.h> // is detected by ctx, and enables AVX2
#endif

#define CTX_IMPLEMENTATION 1

#include "ctx.h"
