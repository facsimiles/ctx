
#define CTX_PTY                 0
#define CTX_1BIT_CLIP           1
#define CTX_RASTERIZER_AA       15
#define CTX_RASTERIZER_FORCE_AA 0
#define CTX_SHAPE_CACHE         0
#define CTX_SHAPE_CACHE_DIM     16*18
#define CTX_SHAPE_CACHE_ENTRIES 128
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 36
#define CTX_MIN_EDGE_LIST_SIZE 256
#define CTX_MAX_EDGE_LIST_SIZE 512
#define CTX_MIN_JOURNAL_SIZE   8192
#define CTX_MAX_JOURNAL_SIZE   8192

#define CTX_LIMIT_FORMATS       1
#define CTX_DITHER              1
#define CTX_32BIT_SEGMENTS      0
#define CTX_ENABLE_RGB565       1
#define CTX_ENABLE_RGB565_BYTESWAPPED 1
#define CTX_BITPACK_PACKER      0
#define CTX_COMPOSITING_GROUPS  0
#define CTX_RENDERSTREAM_STATIC 0
#define CTX_GRADIENT_CACHE      1
#define CTX_ENABLE_CLIP         1
#define CTX_BLOATY_FAST_PATHS   0

#define CTX_VT         1
#define CTX_PARSER     1
#define CTX_RASTERIZER 1
#define CTX_EVENTS     1
#define CTX_RAW_KB_EVENTS 0
#define CTX_STRINGPOOL_SIZE 512
#define CTX_FORMATTER 0
#define CTX_TERMINAL_EVENTS 1
#define CTX_FONTS_FROM_FILE 0
#define CTX_IMPLEMENTATION 1

#define CTX_STATIC_FONT(font) \
  ctx_load_font_ctx(ctx_font_##font##_name, \
                    ctx_font_##font,       \
                    sizeof (ctx_font_##font))
#include <stdint.h>
#include "Arimo-Regular.h"
#include "Cousine-Regular.h"
#include "Cousine-Bold.h"
#include "Cousine-Italic.h"

#define CTX_FONT_0   CTX_STATIC_FONT(Arimo_Regular)
#define CTX_FONT_8   CTX_STATIC_FONT(Cousine_Regular)
#define CTX_FONT_9   CTX_STATIC_FONT(Cousine_Italic)
#define CTX_FONT_10  CTX_STATIC_FONT(Cousine_Bold)



#include "ctx.h"