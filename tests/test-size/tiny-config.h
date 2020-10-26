
#define CTX_MAX_STATES      3 // lowered from 10, saving quite a few kb, 2 is also possible
                              // but then there is no save/restore for the user, with 3 there is one

#define CTX_MATH            1
#define CTX_LIMIT_FORMATS   1
#define CTX_NATIVE_GRAYA8   1
#define CTX_ENABLE_RGBA8    1 // this format is mandatory
#define CTX_ENABLE_CMYK     0
#define CTX_ENABLE_CM       0
#define CTX_PARSER          0
#define CTX_MAX_TEXTURES    1
#define CTX_EVENTS          0
#define CTX_RENDER_CTX      0
#define CTX_DITHER          0 // implied by limit formats and only rgba being anbled, but we're explicit
#define CTX_FORMATTER       0
#define CTX_CURRENT_PATH    0
#define CTX_SHAPE_CACHE     0
#define CTX_GRADIENT_CACHE  0
#define CTX_ENABLE_CLIP     0
#define CTX_ENABLE_SHADOW_BLUR     0
#define CTX_BLOATY_FAST_PATHS 0
#define CTX_BITPACK_PACKER  0
#define CTX_FORCE_INLINES   0
#define CTX_RASTERIZER_FORCE_AA      1   // slightly smaller size code path
#define CTX_COMPOSITING_GROUPS       0
#define CTX_BLENDING_AND_COMPOSITING 0   // 4392 bytes of code difference
#define CTX_INLINED_NORMAL           0   // 328 bytes of difference, big speed impact
#define CTX_STRINGPOOL_SIZE     1   // XXX : should be factored our
#define CTX_MIN_EDGE_LIST_SIZE  512 // is also MIN_RENDERSTREAM_SIZE with RENDERSTREAM_STATIC
#define CTX_RENDERSTREAM_STATIC 1
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
