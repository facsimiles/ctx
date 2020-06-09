#include <stdint.h>

#include "ctx-font-ascii.h"
#define CTX_LIMIT_FORMATS
#define CTX_ENABLE_RGBA8        1
#define CTX_ENABLE_CMYK         0
#define CTX_ENABLE_CM           0
#define CTX_PARSER              0
#define CTX_DITHER              0 // implied by limit formats and only rgba being anbled, but we're explicit
#define CTX_FORMATTER           1
#define CTX_EVENTS              1
#define CTX_BITPACK_PACKER      0
#define CTX_GRADIENT_CACHE      0
#define CTX_RENDERSTREAM_STATIC 0
#define CTX_FONTS_FROM_FILE     0 /* leaves out code */
#define CTX_IMPLEMENTATION

#include "ctx.h"


/* change ttf support to import to ctx then free up the ttf data?  configured
 * with/without ranges..  or even as multiple sub-fonts for unicode ranges.
 */

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new_ui (-1, -1);
  float x = 0;

  //fprintf (stderr, "%ix%i\n", ctx_width (ctx), ctx_height (ctx));
#ifndef REALLY_TINY
  char *utf8 = "tinytest\necho foobaz\n";
#else
  char *utf8 = "";
#endif

  char message[256] = "hello there";

  const CtxEvent *event;
  int mx, my;
  int do_quit = 0;
  //event = ctx_get_event (ctx, 1000, &mx, &my);
  //fprintf (stderr, "[%s :%i %i]", event, mx, my);
  //
  while (!do_quit)
  {
    ctx_clear          (ctx);
    //ctx_set_rgba       (ctx, 0.5, 0.5, 0.5, 1);
    ctx_set_rgba       (ctx, 0.0, 0.0, 0.0, 1);
    ctx_rectangle      (ctx, 0, 0, ctx_width(ctx), ctx_height(ctx));
    ctx_fill           (ctx);
    ctx_move_to        (ctx, 10+x, 9);
    ctx_set_font_size  (ctx, 12 + x/4);
    ctx_set_line_width (ctx, 2);
    ctx_set_rgba       (ctx, 0, 0, 0, 1);
    ctx_text_stroke    (ctx, utf8);
    ctx_set_rgba8      (ctx, 255, 255, 255, 255);
    ctx_move_to        (ctx, 10+x, 9);
    ctx_text           (ctx, utf8);
    ctx_move_to        (ctx, 10, 130);
    ctx_set_font_size  (ctx, 40);

    ctx_set_rgb       (ctx, 1, 0,0);
    ctx_text           (ctx, message);

    if (ctx_pointer_is_down (ctx, 0))
    {
      ctx_arc            (ctx, mx, my, 5.0, 0.0, CTX_PI*2, 0);
      ctx_set_rgba      (ctx, 1, 1, 1, 0.5);
      ctx_fill           (ctx);
    }


    ctx_flush          (ctx);
    //usleep (1000);
    //if (strcmp (event, "idle"))
    //fprintf (stderr, "[%s :%i %i]", event, mx, my);

    x+=0.25;
    if (x > ctx_width (ctx)) x= 0;
   
    if(event = ctx_get_event (ctx))
    {
    switch (event->type)
    {
       case CTX_MOTION:
         mx = event->x;
         my = event->y;
         sprintf (message, "%s %.1f %.1f", 
                          ctx_pointer_is_down (ctx, 0)?
                         "drag":"motion", event->x, event->y);

         break;
       case CTX_PRESS:
         mx = event->x;
         my = event->y;
         sprintf (message, "press %.1f %.1f", event->x, event->y);
         break;
       case CTX_RELEASE:
         mx = event->x;
         my = event->y;
         sprintf (message, "release %.1f %.1f", event->x, event->y);
         break;
       case CTX_DRAG:
         mx = event->x;
         my = event->y;
         sprintf (message, "drag %.1f %.1f", event->x, event->y);
         break;
       case CTX_KEY_DOWN:
         if (!strcmp (event->string, "q"))
         {
            do_quit = 1;
            fprintf (stderr, "quit!\n");
         }
         if (strcmp (event->string, "idle"))
           sprintf (message, "key %s", event->string);
         break;
    }
    }
  }
  ctx_free (ctx);
  return 0;
}
