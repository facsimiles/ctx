#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "ctx-font-regular.h"
#define CTX_RASTERIZER_AA   5
#define CTX_RASTERIZER_AA2  2
#define CTX_ENABLE_GRAYF                1
#define CTX_ENABLE_GRAY1                1 //needed for font-cache

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

  ctx_linear_gradient (ctx, 0, 0, 80,0);
  ctx_gradient_clear_stops (ctx);
  ctx_gradient_add_stop_u8 (ctx, 0, 255,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1,   0,255,255,255);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);

  char *utf8 = "rgba8\n";
  ctx_move_to (ctx, 10, 9);
  ctx_set_font_size (ctx, 12);
  ctx_set_line_width (ctx, 2);
  ctx_set_rgba_u8 (ctx, 0, 0, 0, 255);
  ctx_text_stroke (ctx, utf8);
  ctx_set_rgba_u8 (ctx, 255, 0, 255, 255);
  ctx_move_to (ctx, 10, 9);
  ctx_text (ctx, utf8);

  static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};
  int no = 0;
  int reverse = 1;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      if (reverse)
        printf ("%s", utf8_gray_scale[5-(int)CTX_CLAMP(pixels[no]/255.0*6.0, 0, 5)]);
      else
        printf ("%s", utf8_gray_scale[(int)CTX_CLAMP(pixels[no]/255.0*6, 0, 5)]);
      no+=4;
    }
    printf ("\n");
  }
  no=0;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      if (reverse)
        printf ("%s", utf8_gray_scale[5-(int)CTX_CLAMP(pixels[no+1]/255.0*6.0, 0, 5)]);
      else
        printf ("%s", utf8_gray_scale[(int)CTX_CLAMP(pixels[no+1]/255.0*6, 0, 5)]);
      no+=4;
    }
    printf ("\n");
  }
  no=0;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      if (reverse)
        printf ("%s", utf8_gray_scale[5-(int)CTX_CLAMP(pixels[no+2]/255.0*6.0, 0, 5)]);
      else
        printf ("%s", utf8_gray_scale[(int)CTX_CLAMP(pixels[no+1]/255.0*6, 0, 5)]);
      no+=4;
    }
    printf ("\n");
  }
  stbi_write_png ("rgba8.png", WIDTH, HEIGHT, 4, pixels, WIDTH * 4);
  ctx_free (ctx);

  return 0;
}
