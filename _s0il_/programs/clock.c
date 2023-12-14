#include "ui.h"

void draw_clock (Ctx *ctx)
{
  uint32_t ms = ctx_ticks ()/1000;
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;
  float radius = ctx_width(ctx)/2;
  //int smoothstep = 1;
  float x = ctx_width (ctx)/ 2;
  float y = ctx_height (ctx)/ 2;
  if (radius > ctx_height (ctx)/2) radius = ctx_height (ctx)/2;

  ms = ((uint32_t)(ms)) % 1000;
  s %= 60;
  m %= 60;
  h %= 12;
  
#if 1
  Ui *ui = ui_host(ctx);
  if (ui)
  {
    ui_draw_bg (ui);
  }
  else
#endif
  {
    // when running directly on host without a wrapping ctx we
    // do not get a ui - this makes that work
    ctx_gray(ctx, 0.0f);
    ctx_rectangle (ctx, 0,0, ctx_width(ctx),ctx_height(ctx));
    ctx_fill (ctx);
    ctx_gray(ctx, 1.0f);
  }
  float r; 
  
  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  
  for (int markh = 0; markh < 12; markh++)
  { 
    r = markh * CTX_PI * 2 / 12.0 - CTX_PI / 2;
    
    if (markh == 0)
    {
      ctx_move_to (ctx, x + cosf(r) * radius * 0.7f,
                        y + sinf (r) * radius * 0.65f); 
      ctx_line_to (ctx, x + cosf(r) * radius * 0.8f,
                        y + sinf (r) * radius * 0.85f);   
    }
    else
    {
      ctx_move_to (ctx, x + cosf(r) * radius * 0.7f,
                        y + sinf (r) * radius * 0.7f);   
      ctx_line_to (ctx, x + cosf(r) * radius * 0.8f,
                        y + sinf (r) * radius * 0.8f);   
    }
    ctx_stroke (ctx);
  }
  ctx_line_width (ctx, radius * 0.01f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

  ctx_line_width (ctx, radius * 0.075f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  
  r = m * CTX_PI * 2 / 60.0 - CTX_PI / 2;
  ;

  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.7f,
                    y + sinf(r) * radius * 0.7f);
  ctx_stroke (ctx);

  
  r = (h + m/60.0) * CTX_PI * 2 / 12.0 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.4f,
                    y + sinf(r) * radius * 0.4f);
  ctx_stroke (ctx);
  
  
  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_NONE);
 
  r = (s + ms / 1000.0f) * CTX_PI * 2 / 60 - CTX_PI / 2;

  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.78f,
                    y + sinf(r) * radius * 0.78f);
  ctx_stroke (ctx);

  ctx_queue_draw (ctx);
}

MAIN(clock)
{
  Ctx *ctx = ctx_new (-1,-1,NULL);
  ctx_queue_draw (ctx);
  do
  {
    if (ctx_need_redraw (ctx))
    {
      ctx_start_frame (ctx);

      draw_clock (ctx);
  
      ctx_add_key_binding (ctx, "escape", "exit", "foo",  ui_cb_do, ui_host(ctx));
      ctx_add_key_binding (ctx, "backspace", "exit", "foo",  ui_cb_do, ui_host(ctx));
      ctx_end_frame (ctx);
    }
    else
    {
      ctx_handle_events(ctx);
    }
  } while (!ctx_has_exited (ctx));
  ctx_reset_has_exited(ctx);
  ctx_destroy(ctx);
  
  return 0;
}
