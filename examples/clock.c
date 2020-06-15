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

  ctx_set_rgba8 (ctx, 127,127,127,255);
  ctx_arc (ctx, x, y, radius * 0.9, 0.0, CTX_PI * 2 + 0.001, 0); // XXX
  ctx_set_line_width (ctx, radius * 0.1);
  ctx_set_line_cap (ctx, CTX_CAP_NONE);
  ctx_stroke (ctx);

  ctx_set_line_width (ctx, 7);
  ctx_set_line_cap (ctx, CTX_CAP_ROUND);
  ctx_set_rgba8 (ctx, 188,188,188,255);

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


  ctx_set_line_width (ctx, 2);
  ctx_set_line_cap (ctx, CTX_CAP_ROUND);
  ctx_set_rgba8 (ctx, 255,0,0,127);

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
void red_rect (CtxEvent *event, void *userdata, void *userdata2)
{
   x += event->delta_x;// - 50;
}

void green_rect (CtxEvent *event, void *userdata, void *userdata2)
{
   char buf[32];
   static float start_x;
   static float start_y;
   static float start_ex;
   static float start_ey;

   static float set_x;
   static float set_y;
   switch (event->type)
   {
     case CTX_DRAG_PRESS:
      {
        const char *orig_xs = ctx_get (event->ctx, "x");
        set_x = start_x = strtod (orig_xs, NULL);
        const char *orig_ys = ctx_get (event->ctx, "y");
        set_y = start_y = strtod (orig_ys, NULL);

        start_ex = event->device_x;
        start_ey = event->device_y;
      }
      break;
     case CTX_DRAG_MOTION:
      {
        set_x  = ((event->device_x+set_x) - start_ex);
        set_y  = ((event->device_y+set_y) - start_ey);
        sprintf (buf, "%f", set_x);
        ctx_set (event->ctx, ctx_strhash("x",0), buf, strlen(buf));
        sprintf (buf, "%f", set_y);
        ctx_set (event->ctx, ctx_strhash("y",0), buf, strlen(buf));
      }
      break;
     case CTX_DRAG_RELEASE:
      break;
   }
   ctx_event_stop_propagate (event);
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
    ctx_clear          (ctx);
    //ctx_set_rgba       (ctx, 0.5, 0.5, 0.5, 1);
    ctx_set_rgba       (ctx, 0.0, 0.5, 0.0, 0.0);
    ctx_save (ctx);
    ctx_rectangle      (ctx, 0, 0, ctx_width(ctx), ctx_height(ctx));
    ctx_set_compositing_mode (ctx, CTX_COMPOSITE_SOURCE_COPY);
    ctx_fill           (ctx);
    ctx_restore (ctx);
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

    ctx_set_rgb        (ctx, 1, 0,0);
    ctx_text           (ctx, message);

    ctx_rectangle (ctx, x,200,100,100);
    ctx_listen    (ctx, CTX_DRAG, red_rect, NULL, NULL);
    ctx_fill (ctx);

    ctx_set_rgb        (ctx, 0, 1,0);
    ctx_rectangle (ctx, 100,250,100,100);
    ctx_listen    (ctx, CTX_DRAG, green_rect, NULL, NULL);
    ctx_fill (ctx);
    
    _analog_clock (ctx,
		   ctx_ticks()/1000,
                   ctx_width(ctx)/2,
                   ctx_height(ctx)/2,
                   ctx_height(ctx)/3,
                   1);


    if (ctx_pointer_is_down (ctx, 0))
    {
      ctx_arc      (ctx, mx, my, 5.0, 0.0, CTX_PI*2, 0);
      ctx_set_rgba (ctx, 1, 1, 1, 0.5);
      ctx_fill     (ctx);
    }


    ctx_flush          (ctx);
    usleep (50000);
    //if (strcmp (event, "idle"))
    //fprintf (stderr, "[%s :%i %i]", event, mx, my);

    //x+=0.25;
    if (x > ctx_width (ctx)) x= 0;
   
    if (event = ctx_get_event (ctx))
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
