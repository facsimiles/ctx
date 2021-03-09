/* this is a program containing the same a tiny, but without ctx
 * allowing us to measure what these bits from musl libc cost in bytes
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#define WIDTH    72
#define HEIGHT   24

int main (int argc, char **argv)
{
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4);

  for (int i = 0; i < WIDTH*HEIGHT*4; i++)
    pixels[i] = 0;

  char *utf8 = "tinytest\necho foobaz\n";
#if 0
  ctx_rgba (ctx, 0.5, 0.5, 0.5, 1);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);
  ctx_move_to (ctx, 10, 9);
  ctx_font_size (ctx, 12);
  ctx_line_width (ctx, 2);
  ctx_rgba (ctx, 0, 0, 0, 1);
  ctx_stroke_text (ctx, utf8);
  ctx_rgba_u8 (ctx, 255, 255, 255, 255);
  ctx_move_to (ctx, 10, 9);
  ctx_fill_text (ctx, utf8);
#endif
  printf ("%s\n", utf8);

  static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};
  int no = 0;
  no=0;
  for (int y= 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      printf ("%s", utf8_gray_scale[5-(int)pixels[no+1]]);
      no+=4;
    }
    printf ("\n");
  }
  free (pixels);

  return 0;
}
