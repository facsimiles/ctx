#define CTX_DITHER 1
#define CTX_PROTOCOL_U8_COLOR 1
#define CTX_LIMIT_FORMATS 0
#define CTX_32BIT_SEGMENTS 1
#define CTX_RASTERIZER 1
#define CTX_RASTERIZER_AA 3
#define CTX_ENABLE_GRAY1 1
#define CTX_ENABLE_GRAY2 1
#define CTX_ENABLE_GRAY4 1
#define CTX_ENABLE_GRAY8 1
#define CTX_ENABLE_GRAYA8 1
#define CTX_ENABLE_RGB8 1
#define CTX_ENABLE_RGBA8 1
#define CTX_ENABLE_BGRA8 1
// #define CTX_GET_CONTENTS                   1
#define CTX_ENABLE_RGB332 1
#define CTX_ENABLE_RGB565 1
#define CTX_ENABLE_RGB565_BYTESWAPPED 1
#define CTX_COMPOSITING_GROUPS 0
#define CTX_ENABLE_CM 0
#define CTX_ALWAYS_USE_NEAREST_FOR_SCALE1 1
#define CTX_EVENTS 1
#define CTX_FORCE_INLINES 0
#define CTX_RAW_KB_EVENTS 0
#define CTX_THREADS 0
#define CTX_TILED 0
#define CTX_VT 1
#define CTX_PTY 0
#define CTX_PARSER 1
#define CTX_BAREMETAL 1
#define CTX_ONE_FONT_ENGINE 1
// #define CTX_ESP                            1
#define CTX_MAX_SCANLINE_LENGTH 640
#define CTX_MAX_FRAMEBUFFER_WIDTH CTX_MAX_SCANLINE_LENGTH
#define CTX_MAX_JOURNAL_SIZE (1024 * 512)
// is also max and limits complexity
// of paths that can be filled
#define CTX_MIN_EDGE_LIST_SIZE 512

#define CTX_HASH_COLS 5
#define CTX_HASH_ROWS 5

#define CTX_MAX_DASHES 32
#define CTX_MAX_GRADIENT_STOPS 10
#define CTX_MAX_STATES 10
#define CTX_MAX_EDGES 127
#define CTX_MAX_PENDING 64
#define CTX_FORMATTER 0
#define CTX_GRADIENT_CACHE_ELEMENTS 128
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 64
#define CTX_MAX_KEYDB 16
#define CTX_MAX_TEXTURES 16
#define CTX_PARSER_MAXLEN 512
#define CTX_PARSER_FIXED_TEMP 1
#define CTX_STRINGPOOL_SIZE 256
#define CTX_MAX_DEVICES 1
#define CTX_MAX_KEYBINDINGS 32
#define CTX_MAX_CBS 8
#define CTX_MAX_LISTEN_FDS 1
#define CTX_TERMINAL_EVENTS 0
#define CTX_FRAGMENT_SPECIALIZE 1
#define CTX_GSTATE_PROTECT 1
#define CTX_COMPOSITE_O2 1
#define CTX_RASTERIZER_SWITCH_DISPATCH 0
#define CTX_NATIVE_GRAYA8 0
#define CTX_AVOID_CLIPPED_SUBDIVISION 0
#define CTX_AUDIO 1
#define CTX_AUDIO_HOST 1
#define CTX_STB_IMAGE 1
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdint.h>
#define CTX_STATIC_FONT(font)                                                  \
  ctx_load_font_ctx(ctx_font_##font##_name, ctx_font_##font,                   \
                    sizeof(ctx_font_##font))

#include "Arimo-Regular.h"
#include "Cousine-Regular.h"
#define CTX_FONT_1 CTX_STATIC_FONT(Arimo_Regular)
#define CTX_FONT_2 CTX_STATIC_FONT(Cousine_Regular)

#define CTX_IMPLEMENTATION
#include "ctx.h"
