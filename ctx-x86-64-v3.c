#if CTX_BABL
#include <babl/babl.h>
#endif

#define CTX_BRAILLE_TEXT         1
#define CTX_ENABLE_CMYK          1

#define CTX_SIMD_SUFFIX(symbol)  symbol##_x86_64_v3

#include "ctx.h"
