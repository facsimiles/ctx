#include <stdint.h>
#include "ctx-font-regular.h"

#define CTX_IMPLEMENTATION
#include "ctx.h"

#define WIDTH    72
#define HEIGHT   24
#define STRIDE   WIDTH/4

static uint8_t pixels[STRIDE*HEIGHT]={0,};
static char *utf8_gray_scale[]={" ","░","▓","█", NULL};

int main (int argc, char **argv)
{
  Ctx *ctx;

  ctx = ctx_new_for_framebuffer (
    &pixels[0], WIDTH, HEIGHT, STRIDE, CTX_FORMAT_GRAY2);

  for (int i = 0; i < sizeof(pixels); i++)
    pixels[i] = 0;

  ctx_linear_gradient (ctx, 0, 0, 80,0);
  ctx_gradient_clear_stops (ctx);
  ctx_gradient_add_stop_u8 (ctx, 0, 0,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1, 255,255,255,255);
  ctx_rectangle (ctx, 0, 0, WIDTH, HEIGHT);
  ctx_fill (ctx);


  char *utf8 = "2 bits\n4 grays";
  ctx_move_to (ctx, 4, 9);
  ctx_set_font_size (ctx, 12);
  ctx_set_line_width (ctx, 2);
  ctx_set_rgba_u8 (ctx, 0, 0, 0, 255);
  ctx_text_stroke (ctx, utf8);
  ctx_set_rgba_u8 (ctx, 255, 255, 255, 255);
  ctx_move_to (ctx, 4, 9);
  ctx_text (ctx, utf8);

  int no = 0;
  int reverse = 0;
  for (int y= 0; y < HEIGHT; y++)
  {
    no = y * STRIDE;
    for (int x = 0; x < WIDTH; x++)
    {
      int val = (pixels[no] & (3 << ((x % 4)*2)) ) >> ((x%4)*2);
      printf ("%s", utf8_gray_scale[3-val]);
      if ((x % 4) == 3)
        no++;
    }
    printf ("\n");
  }

  ctx_free (ctx);

  return 0;
}
