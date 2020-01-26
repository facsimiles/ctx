
#include <stdint.h>
#include "ctx-font-regular.h"

#define CTX_SHAPE_CACHE  0
#define CTX_IMPLEMENTATION
#include "ctx.h"

#define WIDTH    160
#define HEIGHT   64

#define STRIDE (WIDTH/8+(WIDTH%8?1:0))
uint8_t pixels[STRIDE*HEIGHT];

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new_for_framebuffer (
    &pixels[0], WIDTH, HEIGHT, STRIDE, CTX_FORMAT_GRAY1);

  for (int i = 0; i < sizeof(pixels); i++)
    pixels[i] = 0;

  ctx_linear_gradient (ctx, 0, 0, 80,0);
  ctx_gradient_clear_stops (ctx);
  ctx_gradient_add_stop_u8 (ctx, 0, 0,0,0,255);
  ctx_gradient_add_stop_u8 (ctx, 1, 255,255,255,255);
  //ctx_set_rgba_u8 (ctx, 0,0,0, 255);
  ctx_rectangle (ctx, 0, 0, WIDTH, HEIGHT);
  ctx_fill (ctx);

  char *utf8 = "1bit\n";
  ctx_move_to (ctx, 2, 60);
  ctx_set_font_size (ctx, 40);
  ctx_set_line_width (ctx, 2);
  ctx_set_rgba_u8 (ctx, 255, 255, 255, 255);
  ctx_text_stroke (ctx, utf8);
  ctx_set_rgba_u8 (ctx, 0, 0, 0, 255);
  ctx_move_to (ctx, 2, 60);
  ctx_text (ctx, utf8);
  ctx_set_rgba_u8 (ctx, 0, 0, 0, 255);
  ctx_move_to (ctx, 76.3, 10);
  ctx_set_font_size (ctx, 13);
  //ctx_set_rgba_u8 (ctx, 255, 255, 255, 255);
  ctx_text (ctx, "ctx works well\nmaybe solid\ngrays also\nshould be\ndithered");

  int reverse = 1;
#if 0
  int no = 0;
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
#else


  for (int row = 0; row < HEIGHT/4; row++)
  {
  for (int col = 0; col < WIDTH /2; col++)
  {
    int unicode = 0;
    int bitno = 0;
    for (int x = 0; x < 2; x++)
    for (int y = 0; y < 3; y++)
    {
       int no = (row * 4 + y) * STRIDE + (col*2+x)/8;
       int set = pixels[no] & (1<< ((col * 2 + x) % 8));
      if (reverse) set = !set;
       if (set)
         unicode |=  (1<<(bitno));
       bitno++;
    }
    {
       int x = 0; int y = 3;

       int no = (row * 4 + y) * STRIDE + (col*2+x)/8;
       int setA = pixels[no] & (1<< ((col * 2 + x) % 8));
       no = (row * 4 + y) * STRIDE + (col*2+x+1)/8;
       int setB = pixels[no] & (1<< (   (col * 2 + x + 1) % 8));
      if (reverse) setA = !setA;
      if (reverse) setB = !setB;

       if (setA != 0 && setB==0)
       {
	 unicode += 0x2840;
       }
       else if (setA == 0 && setB)
       {
	 unicode += 0x2880;
       }
       else if ((setA != 0) && (setB != 0))
       {
	 unicode += 0x28C0;
       }
       else
       {
         unicode += 0x2800;
       }
    }

    uint8_t utf8[5];
    utf8[ctx_unichar_to_utf8 (unicode, utf8)]=0;
    printf ("%s", utf8);
  }
    printf ("\n");
  }
#endif

  ctx_free (ctx);

  return 0;
}
