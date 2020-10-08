#include <stdint.h>
#include "ctx-font-ascii.h"
#define CTX_PARSER              1
#define CTX_DITHER              0 // implied by limit formats and only rgba being anbled, but we're explicit
#define CTX_EVENTS              1
#define CTX_BITPACK_PACKER      0
#define CTX_GRADIENT_CACHE      0
#define CTX_RENDERSTREAM_STATIC 0
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
#define CTX_RASTERIZER 0

#define CTX_IMPLEMENTATION
#include "clock.c"
