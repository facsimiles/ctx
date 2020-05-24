#include <stdint.h>

#include "ctx-font-regular2.h"
#define CTX_LIMIT_FORMATS
#define CTX_ENABLE_RGBA8                1

#define CTX_IMPLEMENTATION

#include "ctx.h"

#define WIDTH    72
#define HEIGHT   24

uint8_t pixels[WIDTH*HEIGHT*4];

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_RGBA8);

  for (int i = 0; i < WIDTH*HEIGHT*4; i++)
    pixels[i] = 0;

  ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 1);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);

  char *utf8 = "tinytest\necho foobaz\n";
  ctx_move_to (ctx, 10, 9);
  ctx_set_font_size (ctx, 12);
  ctx_set_line_width (ctx, 2);
  ctx_set_rgba (ctx, 0, 0, 0, 1);
  ctx_text_stroke (ctx, utf8);
  ctx_set_rgba_u8 (ctx, 255, 255, 255, 255);
  ctx_move_to (ctx, 10, 9);
  ctx_text (ctx, utf8);

  static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};
  int no = 0;
  no=0;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      printf ("%s", utf8_gray_scale[5-(int)CTX_CLAMP(pixels[no+1]/255.0*6.0, 0, 5)]);
      no+=4;
    }
    printf ("\n");
  }
  ctx_free (ctx);

  return 0;
}
