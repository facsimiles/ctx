//#define CTX_IMPLEMENTATION 1
//#define CSS_IMPLEMENTATION 1
#include <math.h>
#include "ctx.h"

static int spirals_ui (Css *itk, void *data)
{
  Ctx *ctx = css_ctx (itk);
  static int   dot_count = 500;
  static float twist = 2.9645;
  static float dot_scale = 42.0;

  static int shape = 1;

      /* clear */
      ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
      ctx_rgba8 (ctx, 0,0,0,255);
      ctx_fill (ctx);

      ctx_rgba (ctx, 1, 1, 1, 0.5);
      for (int i = 0; i < dot_count; i ++)
      {
        float x = ctx_width (ctx)/ 2;
        float y = ctx_height (ctx) / 2;
        float radius = ctx_height (ctx) / dot_scale;
        float dist = i * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
        float twisted = (i * twist);
        x += cos (twisted) * dist;
        y += sin (twisted) * dist;
        if (shape == 0)
          ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
        else
        {
          ctx_save (ctx);
          ctx_translate (ctx, x, y);
          ctx_rotate (ctx, twisted);
          ctx_rectangle (ctx, -radius, -radius, radius*2, radius*2);
          ctx_restore (ctx);
        }
        ctx_fill (ctx);
      }

      css_panel_start (itk, "spiraling things", ctx_width(ctx)*3/4,0,ctx_width(ctx)/4, ctx_height(ctx)/2);

      static int animate = 0;
      animate = css_toggle (itk, "animate", animate);
      if (animate)
      {
        twist += 0.00001;
        ctx_queue_draw (ctx);
      }
#if 1
      if (css_button (itk, "+0.00001"))
      {
        twist += 0.00001;
      }
      if (css_button (itk, "-0.00001"))
      {
        twist -= 0.00001;
      }
#endif

      css_slider_int (itk, "count",          &dot_count, 1,  50000, 10);
      css_slider_float (itk, "radius",    &dot_scale, 2.0, 400.0, 4.5);
      css_slider_float (itk, "twist amount", &twist, -3.14152, 3.14152, 0.0005);


      css_choice_add (itk, 0, "circle");
      css_choice_add (itk, 1, "square");
      shape = css_choice (itk, "shape", shape);


      css_ctx_settings (itk);
      css_panel_end (itk);
      return 1;
}

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  css_main (spirals_ui, NULL);
  return 0;
}
