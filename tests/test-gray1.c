
#include <stdint.h>
#include "ctx-font-regular.h"

#define CTX_RASTERIZER_AA   5
#define CTX_RASTERIZER_AA2  2

#define CTX_IMPLEMENTATION
#include "ctx.h"

#define WIDTH    72
#define HEIGHT   24

uint8_t pixels[(WIDTH/8+(WIDTH%8?1:0))*HEIGHT];

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new_for_framebuffer (
    &pixels[0], WIDTH, HEIGHT, WIDTH/8+(WIDTH%8?1:0), CTX_FORMAT_GRAY1);

  for (int i = 0; i < sizeof(pixels); i++)
    pixels[i] = 0;

  ctx_linear_gradient (ctx, 0, 0, 80,0);
  ctx_gradient_clear_stops (ctx);
  ctx_gradient_add_stop_u8 (ctx, 0, 0,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1, 255,255,255,255);
  ctx_rectangle (ctx, 0, 0, WIDTH, HEIGHT);
  ctx_fill (ctx);

  char *utf8 = "1bit\n";
  ctx_move_to (ctx, 22, 18);
  ctx_set_font_size (ctx, 20);
  ctx_set_line_width (ctx, 3);
  ctx_set_rgba_u8 (ctx, 0, 0, 0, 255);
  ctx_text_stroke (ctx, utf8);
  ctx_set_rgba_u8 (ctx, 255, 255, 255, 255);
  ctx_move_to (ctx, 22, 18);
  ctx_text (ctx, utf8);

  int no = 0;
  int reverse = 0;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      int set = pixels[no] & (1<< (x % 8));
      if (reverse) set = !set;
      printf ("%s", set?" ":"█");

      if (x % 8 == 7)
        no++;
    }
    printf ("\n");
  }

  ctx_free (ctx);

  return 0;
}
