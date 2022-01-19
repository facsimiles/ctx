#if CTX_BABL
#include <babl/babl.h>
#endif
#if 0
#define CTX_FRAGMENT_SPECIALIZE  1
#define CTX_GRADIENT_CACHE 1
#define CTX_NATIVE_GRAYA8  1
#define CTX_ENABLE_CMYK    1
#define CTX_ENABLE_CM      1
#define CTX_MATH           1
#define CTX_COMPOSITE      1
#else

#define CTX_EVENTS               1

#define CTX_DITHER               0 //
#define CTX_BRAILLE_TEXT         1
#define CTX_ENABLE_CMYK          1
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

#endif

#define CTX_SIMD_SUFFIX(symbol)  symbol##_x86_64_v3

#include "ctx.h"
