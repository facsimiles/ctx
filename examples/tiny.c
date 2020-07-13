#include <stdint.h>

#include "ctx-font-ascii.h"
#define CTX_LIMIT_FORMATS   1
#define CTX_ENABLE_RGBA8    1 // this format is mandatory
#define CTX_ENABLE_CMYK     0
#define CTX_ENABLE_CM       0
#define CTX_PARSER          0
#define CTX_RENDER_CTX      0
#define CTX_DITHER          0 // implied by limit formats and only rgba being anbled, but we're explicit
#define CTX_FORMATTER       0
#define CTX_CURRENT_PATH    0
#define CTX_NATIVE_GRAYA8   0
#define CTX_SHAPE_CACHE     0
#define CTX_GRADIENT_CACHE  0
#define CTX_ENABLE_CLIP     0
#define CTX_SHADOW_BLUR     0
#define CTX_BLOATY_FAST_PATHS 0
#define CTX_BITPACK_PACKER  0
#define CTX_FORCE_INLINES   0
#define CTX_RASTERIZER_FORCE_AA      1   // slightly smaller size code path
#define CTX_COMPOSITING_GROUPS       0
#define CTX_BLENDING_AND_COMPOSITING 0   // 4392 bytes of code difference
#define CTX_INLINED_NORMAL           0   // 328 bytes of difference, big speed impact
#define CTX_RENDERSTREAM_STATIC 1
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
#define CTX_IMPLEMENTATION

#include "ctx.h"

#define WIDTH    72
#define HEIGHT   24

int main (int argc, char **argv)
{
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_RGBA8);

#ifndef REALLY_TINY
  char *utf8 = "tinytest\necho foobaz\n";
#else
  char *utf8 = "";
#endif
  ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 1);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);
  ctx_move_to (ctx, 10, 9);
  ctx_set_font_size (ctx, 12);
  ctx_set_line_width (ctx, 2);
  ctx_set_rgba (ctx, 0, 0, 0, 1);
  ctx_text_stroke (ctx, utf8);
  ctx_rgba8 (ctx, 255, 255, 255, 255);

  ctx_fill (ctx);
  ctx_move_to (ctx, 10, 9);
  ctx_text (ctx, utf8);
  ctx_begin_path (ctx);

#ifndef REALLY_TINY
  static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};
  int no = 0;
  no=0;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++, no+=4)
      printf ("%s", utf8_gray_scale[5-(int)CTX_CLAMP(pixels[no+1]/255.0*6.0, 0, 5)]);
    printf ("\n");
  }
#endif
  ctx_free (ctx);
  free (pixels);
  return 0;
}
