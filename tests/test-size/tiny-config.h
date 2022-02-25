#define CTX_MAX_STATES      4 // lowered from 10, saving quite a few kb, 2 is also possible
                              // but then there is no save/restore for the user, with 4 there is two

#define CTX_RASTERIZER          1
#define CTX_MATH            1
#define CTX_LIMIT_FORMATS   1
#define CTX_FRAGMENT_SPECIALIZE 0
#define CTX_32BIT_SEGMENTS  0
#define CTX_ENABLE_RGBA8    1
#define CTX_FAST_FILL_RECT  0
#define CTX_ENABLE_RGB565_BS  0
#define CTX_ENABLE_CMYK     0
#define CTX_ENABLE_CM       0
#define CTX_NATIVE_GRAYA8   0
#define CTX_PARSER          0
#define CTX_RASTERIZER_SWITCH_DISPATCH 0
#define CTX_MAX_TEXTURES    1
#define CTX_U8_TO_FLOAT_LUT 0
#define CTX_EVENTS          0
#define CTX_RENDER_CTX      0
#define CTX_DITHER          0 
#define CTX_FORMATTER       0
#define CTX_CURRENT_PATH    0
#define CTX_SHAPE_CACHE     0
#define CTX_PARSER_FIXED_TEMP 1
#define CTX_PARSER_MAXLEN     1024

#define CTX_GRADIENT_CACHE  1
#define CTX_ENABLE_CLIP     1  // < 60 bytes extra - but stringpool size
                               //   should be increased to store poly-lines
                               //   of active clips
#define CTX_ENABLE_SHADOW_BLUR       0 // < 4kb extra code size
#define CTX_BLOATY_FAST_PATHS        0
#define CTX_BITPACK_PACKER           0
#define CTX_FORCE_INLINES            0
#define CTX_RASTERIZER_AA           15   // slightly smaller size code path
#define CTX_RASTERIZER_FORCE_AA      0   // slightly smaller size code path
#define CTX_COMPOSITING_GROUPS       1   // ~3kb code size
#define CTX_BLENDING_AND_COMPOSITING 1   // 4392 bytes of code difference
#define CTX_INLINED_NORMAL           0   // up to 10kb code size difference
                                         // big performance impact
#define CTX_STRINGPOOL_SIZE     8   // XXX : should be factored out
#define CTX_MIN_EDGE_LIST_SIZE  196 // is also MIN_RENDERSTREAM_SIZE with RENDERSTREAM_STATIC
#define CTX_RENDERSTREAM_STATIC 1
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
#define CTX_AUDIO               0
#define CTX_CLIENTS             0
