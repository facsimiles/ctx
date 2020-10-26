#include <stdint.h>

#include "tiny-config.h"
#undef CTX_PARSER
#define CTX_PARSER          1

#define CTX_IMPLEMENTATION

#include "ctx.h"

#define WIDTH    72
#define HEIGHT   24

int main (int argc, char **argv)
{
  uint8_t pixels[WIDTH*HEIGHT*4];
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_RGBA8);

  ctx_rgba (ctx, 0.5, 0.5, 0.5, 1);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);

#ifndef REALLY_TINY
  char *utf8 = "tinytest\necho foobaz\n";
#else
  char *utf8 = "";
#endif
  ctx_move_to (ctx, 10, 9);
  ctx_font_size (ctx, 12);
  ctx_line_width (ctx, 2);
  ctx_rgba (ctx, 0, 0, 0, 1);
  ctx_text_stroke (ctx, utf8);
  ctx_rgba8 (ctx, 255, 255, 255, 255);
  ctx_move_to (ctx, 10, 9);
  ctx_text (ctx, utf8);

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
  ctx_parse (ctx, "M 100 100");
  ctx_free (ctx);
  return 0;
}
