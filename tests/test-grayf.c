#include <stdint.h>

#include "ctx-font-regular.h"
#define CTX_RASTERIZER_AA   5
#define CTX_ENABLE_GRAYF                1
#define CTX_ENABLE_GRAY1                1 //needed for font-cache

#define CTX_IMPLEMENTATION
#include "ctx.h"

#define WIDTH    72
#define HEIGHT   24

float pixels[WIDTH*HEIGHT];

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_GRAYF);

  for (int i = 0; i < WIDTH*HEIGHT; i++)
    pixels[i] = 0;

  ctx_linear_gradient (ctx, 0, 0, 80,0);
  ctx_gradient_clear_stops (ctx);
  ctx_gradient_add_stop_u8 (ctx, 0, 0,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1, 255,255,255,255);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);

  char *utf8 = "gray float\nno dither";
  ctx_move_to (ctx, 10, 9);
  ctx_set_font_size (ctx, 10);
  ctx_set_line_width (ctx, 2);
  ctx_set_rgba_u8 (ctx, 0, 0, 0, 255);
  ctx_text_stroke (ctx, utf8);
  ctx_set_rgba_u8 (ctx, 255, 255, 255, 255);
  ctx_move_to (ctx, 10, 9);
  ctx_text (ctx, utf8);

  static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};
  int no = 0;
  int reverse = 1;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++, no++)
    {
      if (reverse)
        printf ("%s", utf8_gray_scale[5-(int)CTX_CLAMP(pixels[no]*6.0, 0, 5)]);
      else
        printf ("%s", utf8_gray_scale[(int)CTX_CLAMP(pixels[no]*6, 0, 5)]);
    }
    printf ("\n");
  }
  ctx_free (ctx);

  return 0;
}
