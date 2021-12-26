#include "itk.h"

static int timer_ui (ITK *itk, void *data)
{
  Ctx *ctx = itk->ctx;
  static float e = 0.0;
  static float d = 10.0;

  /* timer part */
  static unsigned long prev_ticks = 0;
  unsigned long ticks = ctx_ticks ();
  if (e<d)
  {
    if (prev_ticks)
      e += (ticks-prev_ticks)/1000.0/1000.0;
    ctx_queue_draw (itk->ctx); // queue a redraw
                                 // causing our in-place timer to work
  }
  prev_ticks = ticks;

  itk_panel_start (itk, "7gui - Timer",
                  ctx_width(ctx)*0.2, 0,
                  ctx_width (ctx) * 0.8, ctx_height (ctx));

  itk_slider_float (itk, "elapsed", &e, 0.0, d, 0.1);
  itk_labelf (itk, "%.1f", e);
  itk_slider_float (itk, "duration", &d, 0.0, 300.0, 0.5);

  if (itk_button (itk, "Reset"))
    e = 0.0;

  itk_panel_end (itk);
  return 0;
}

int main (int argc, char **argv)
{
  itk_main (timer_ui, NULL);
  return 0;
}
