#include <math.h>
#include "ctx.h"
#include "itk.h"

static int do_quit = 0;

void itk_key_quit (CtxEvent *event, void *userdata, void *userdata2)
{
  do_quit = 1;
}


int main (int argc, char **argv)
{
  ITK *itk;
  int   dot_count = 100;
  float twist = -0.1619;
  float dot_scale = 160.0;

  ctx_init (&argc, &argv);
  Ctx *ctx = ctx_new_ui (-1, -1);
  itk = itk_new (ctx);

  while (!do_quit)
  {
    if (itk->dirty)
    {
      ctx_reset (ctx);

      /* clear */
      ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
      ctx_rgba8 (ctx, 0,0,0,255);
      ctx_fill (ctx);

      ctx_rgba(ctx, 1, 1, 1, 0.5);
      for (int i = 0; i < dot_count; i ++)
      {
        float x = ctx_width (ctx)/ 2;
        float y = ctx_height (ctx) / 2;
        float radius = ctx_height (ctx) / dot_scale;
        float dist = i * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
        float twisted = (i * twist);
        x += cos (twisted) * dist;
        y += sin (twisted) * dist;
        ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
        ctx_fill (ctx);
      }

      itk_reset (itk);
      itk_panel_start (itk, "spiraling dots", ctx_width(ctx)*3/4,0,ctx_width(ctx)/4, ctx_height(ctx)/3);
      itk_slider_int (itk, "count",          &dot_count, 1,   4000, 1);
      itk_slider_float (itk, "dot scale",    &dot_scale, 2.0, 100.0, 0.5);
      itk_slider_float (itk, "twist amount", &twist, -3.14152, 3.14152, 0.0005);
      if (itk_button (itk, "+0.0001"))
      {
        twist += 0.0001;
      }
      if (itk_button (itk, "-0.0001"))
      {
        twist -= 0.0001;
      }
      itk_panel_end (itk);
      itk_done (itk);

      itk_key_bindings (itk);

      ctx_flush (ctx);
      ctx_add_key_binding (ctx, "control-q", NULL, "foo", itk_key_quit, NULL);
    }
    while(ctx_get_event (ctx))
    {
    }
  }
  ctx_free (ctx);
  return 0;
}
