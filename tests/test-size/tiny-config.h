#define CTX_MAX_STATES     6  // lowered from 16, saving quite a few kb, 2 is also possible
                              // but then there is no save/restore for the user, with 4 there is two

#define CTX_MAX_KEYDB       10
#define CTX_RASTERIZER      1
#define CTX_MATH            1
#define CTX_LIMIT_FORMATS   1
#define CTX_RESOLVED_FONTS  2
#define CTX_MAX_SCANLINE_LENGTH 1024
#define CTX_STATIC_OPAQUE   0
#define CTX_FRAGMENT_SPECIALIZE 0
#define CTX_32BIT_SEGMENTS  0
#define CTX_ENABLE_RGBA8    1
#define CTX_FAST_FILL_RECT  0
#define CTX_FAST_STROKE_RECT  0
#define CTX_ENABLE_RGB565_BS  0
#define CTX_ENABLE_CMYK     0
#define CTX_STROKE_1PX      0
#define CTX_GSTATE_PROTECT  0
#define CTX_ENABLE_CM       0
#define CTX_NATIVE_GRAYA8   0
#define CTX_FONT_SHAPE_CACHE 0
#define CTX_RASTERIZER_BEZIER_FIXED_POINT 0
#define CTX_PARSER          0
#define CTX_RASTERIZER_SWITCH_DISPATCH 0
#define CTX_MAX_TEXTURES    1
#define CTX_MAX_GRADIENT_STOPS 8  // 16 is default
#define CTX_U8_TO_FLOAT_LUT 0
#define CTX_EVENTS          0
#define CTX_INLINED_GRADIENTS 0
#define CTX_GLYPH_CACHE     0
#define CTX_DITHER          0 
#define CTX_INLINE_FILL_RULE 0
#define CTX_FORMATTER       0
#define CTX_CURRENT_PATH    0
#define CTX_SHAPE_CACHE     0
#define CTX_PARSER_FIXED_TEMP 1
#define CTX_PARSER_MAXLEN     1024

#define CTX_GRADIENT_CACHE  0
#define CTX_ENABLE_CLIP     1  // < 60 bytes extra - but stringpool size
                               //   should be increased to store poly-lines
                               //   of active clips
#define CTX_ENABLE_SHADOW_BLUR       0 // < 4kb extra code size
#define CTX_BLOATY_FAST_PATHS        0
#define CTX_BITPACK_PACKER           0
#define CTX_FORCE_INLINES            0
#define CTX_RASTERIZER_AA           15   // slightly smaller size code path
#define CTX_COMPOSITING_GROUPS       0   // ~3kb code size
#define CTX_BLENDING_AND_COMPOSITING 0   // 4392 bytes of code difference
#define CTX_INLINED_NORMAL           0   // up to 10kb code size difference
                                         // big performance impact
#define CTX_STRINGPOOL_SIZE     8   // XXX : should be factored out
#define CTX_MIN_EDGE_LIST_SIZE  512 // is also MIN_RENDERSTREAM_SIZE with RENDERSTREAM_STATIC
#define CTX_RENDERSTREAM_STATIC 1
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
#define CTX_AUDIO               0
#define CTX_VT                  0
#define CTX_CURRENT_PATH        0
#define CTX_ENABLE_SHADOW_BLUR  0
#define CTX_PTY                 0
#define CTX_MAX_KEYBINDINGS     16
#define CTX_MAX_DEVICES          1
#define CTX_MAX_CBS                6
#define CTX_MAX_LISTEN_FDS         1
#define CTX_ONE_FONT_ENGINE        1
