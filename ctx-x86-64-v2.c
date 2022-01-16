#ifndef NO_BABL
#include <babl/babl.h>
#endif

#define CTX_GRADIENT_CACHE 1
#define CTX_NATIVE_GRAYA8  1
#define CTX_ENABLE_CMYK    1
#define CTX_ENABLE_CM      1
#define CTX_MATH           1
#define CTX_COMPOSITE      1

#define CTX_SIMD_SUFFIX(symbol)  symbol##_x86_64_v2

#include "ctx.h"
