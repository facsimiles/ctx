#include <stdint.h>
#include <SDL.h>
#include "ctx-font-ascii.h"
#define CTX_LIMIT_FORMATS       1
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



static void _analog_clock (Ctx     *ctx,
		           uint32_t ms,
			   float    x,
			   float    y,
			   float    radius,
			   int smoothstep)
{
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;

  ms = ((uint32_t)(ms))%1000;
  s %= 60;
  m %= 60;
  h %= 12;

  float r;
  ctx_save (ctx);

  ctx_rgba8 (ctx, 127,127,127,255);
  ctx_arc (ctx, x, y, radius * 0.9, 0.0, CTX_PI * 2 + 0.001, 0); // XXX
  ctx_line_width (ctx, radius * 0.1);
  ctx_line_cap (ctx, CTX_CAP_NONE);
  ctx_stroke (ctx);

  ctx_line_width (ctx, 7);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  ctx_rgba8 (ctx, 188,188,188,255);

  r = m * CTX_PI * 2/ 60.0 - CTX_PI/2;
	  ;
#if 1
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + ctx_cosf(r) * radius * 0.7f, y + ctx_sinf (r) * radius * 0.7f);
  ctx_stroke (ctx);
#endif

  r = h * CTX_PI * 2/ 12.0 - CTX_PI/2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + ctx_cosf(r) * radius * 0.4f, y + ctx_sinf (r) * radius * 0.4f);
  ctx_stroke (ctx);


  ctx_line_width (ctx, 2);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  ctx_rgba8 (ctx, 255,0,0,127);

  if (smoothstep)
    r = (s + ms/1000.0f) * CTX_PI * 2/ 60 - CTX_PI/2;
  else
    r = (s ) * CTX_PI * 2/ 60 - CTX_PI/2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + ctx_cosf(r) * radius * 0.75f, y + ctx_sinf (r) * radius * 0.75f);
  ctx_stroke (ctx);

  ctx_restore (ctx);
}

/* change ttf support to import to ctx then free up the ttf data?  configured
 * with/without ranges..  or even as multiple sub-fonts for unicode ranges.
 */

float x = 0;
float y = 100;
void red_rect (CtxEvent *event, void *userdata, void *userdata2)
{
   x += event->delta_x;// - 50;
   y += event->delta_y;// - 50;
}

void green_rect (CtxEvent *event, void *userdata, void *userdata2)
{
   ctx_event_stop_propagate (event);
   ctx_start_move (event->ctx);
}

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  Ctx *ctx = ctx_new_ui (-1, -1);

#ifndef REALLY_TINY
  char *utf8 = "ctx\n";
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
    ctx_reset          (ctx);
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    ctx_save (ctx);
    ctx_rectangle      (ctx, 0, 0, width, height);
    ctx_compositing_mode (ctx, CTX_COMPOSITE_CLEAR);
    ctx_fill           (ctx);
    ctx_restore (ctx);
    ctx_move_to        (ctx, 10+x, height * 0.2);
    ctx_font_size  (ctx, height * 0.1 + x/4.0);
    ctx_line_width (ctx, 2);
    ctx_rgba       (ctx, 0, 0, 0, 1);
    ctx_text_stroke    (ctx, utf8);
    ctx_rgba8      (ctx, 255, 255, 255, 255);
    ctx_move_to        (ctx, height * 0.05 +x, height * 0.2);
    ctx_text           (ctx, utf8);
    ctx_font_size  (ctx, height * 0.2);

    ctx_move_to        (ctx, height * 0.05, height * 0.4);
    ctx_rgb        (ctx, 1, 0,0);
    ctx_text           (ctx, message);

    ctx_rectangle (ctx, x,y,height * 0.2,height * 0.2);
    ctx_listen    (ctx, CTX_DRAG, red_rect, NULL, NULL);
    ctx_fill (ctx);

    ctx_rgb        (ctx, 0, 1,0);
    ctx_rectangle (ctx, 0,height * 0.8,height * 0.2,height * 0.2);
    ctx_listen    (ctx, CTX_PRESS, green_rect, NULL, NULL);
    ctx_fill (ctx);
    
    if ( mx >  ctx_width(ctx)/2)
    _analog_clock (ctx,
		   ctx_ticks()/1000,
                   ctx_width(ctx)/2,
                   ctx_height(ctx)/2,
                   ctx_height(ctx)/3,
                   1);


    if (ctx_pointer_is_down (ctx, 0))
    {
      ctx_arc      (ctx, mx, my, 5.0, 0.0, CTX_PI*2, 0);
      ctx_rgba (ctx, 1, 1, 1, 0.5);
      ctx_fill     (ctx);
    }

    ctx_flush          (ctx);
    //usleep (50000);
    //if (strcmp (event, "idle"))
    //fprintf (stderr, "[%s :%i %i]", event, mx, my);

    //x+=0.25;
    if (x > ctx_width (ctx)) x= 0;
   
    if ((event = ctx_get_event (ctx)))
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
       default:
         break;
    }
    }
  }
  ctx_free (ctx);
  return 0;
}
