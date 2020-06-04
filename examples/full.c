#include <stdint.h>

#include "ctx-font-ascii.h"
#define CTX_PARSER          1
#define CTX_FORMATTER       1
#define CTX_BITPACK_PACKER  1
#define CTX_GRADIENT_CACHE   1
#define CTX_FONTS_FROM_FILE  1 /* leaves out code */
#define CTX_IMPLEMENTATION

#include "ctx.h"
#include "svg.h"

#define WIDTH    72
#define HEIGHT   24

int main (int argc, char **argv)
{
  uint8_t *pixels = (uint8_t*)malloc (WIDTH*HEIGHT*4);
  Ctx *ctx = ctx_new_for_framebuffer (
    pixels, WIDTH, HEIGHT, WIDTH*4,
    CTX_FORMAT_RGBA8);

  ctx_set_rgba (ctx, 0.5, 0.5, 0.5, 1);
  ctx_rectangle (ctx, 0, 0, 80, 24);
  ctx_fill (ctx);

#ifndef REALLY_TINY
  char *utf8 = "tinytest\necho foobaz\n";
#else
  char *utf8 = "";
#endif
  ctx_move_to (ctx, 10, 9);
  ctx_set_font_size (ctx, 12);
  ctx_set_line_width (ctx, 2);
  ctx_set_rgba (ctx, 0, 0, 0, 1);
  ctx_text_stroke (ctx, utf8);
  ctx_set_rgba8 (ctx, 255, 255, 255, 255);
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
  {
       CtxParser *ctxp = ctx_parser_new (ctx, WIDTH, HEIGHT, WIDTH/20, HEIGHT/20, 1, 1, NULL, NULL);

       for (int i = 0; i<3; i++)
       {
         ctx_parser_feed_byte (ctxp, ' ');
       }

       ctx_parser_free (ctxp);
       ctx_render_stream (ctx, stdout, 1);
  }
  {
      Mrg *mrg = mrg_new (ctx, 100, 100);
      mrg_print_xml (mrg, "abc");
  }

  ctx_free (ctx);
  free (pixels);
  return 0;
}
