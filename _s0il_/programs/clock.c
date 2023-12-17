#include "s0il.h"

void draw_clock(Ctx *ctx, float x, float y, float radius,
                uint32_t ms_since_midnight) {
  uint32_t ms = ms_since_midnight;
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;
  // int smoothstep = 1;

  ctx_save(ctx);

  ms = ((uint32_t)(ms)) % 1000;
  s %= 60;
  m %= 60;
  h %= 12;

  float r;

  ctx_line_width(ctx, radius * 0.02f);
  ctx_line_cap(ctx, CTX_CAP_ROUND);

  for (int markh = 0; markh < 12; markh++) {
    r = markh * CTX_PI * 2 / 12.0 - CTX_PI / 2;

    if (markh == 0) {
      ctx_move_to(ctx, x + cosf(r) * radius * 0.7f,
                  y + sinf(r) * radius * 0.65f);
      ctx_line_to(ctx, x + cosf(r) * radius * 0.8f,
                  y + sinf(r) * radius * 0.85f);
    } else {
      ctx_move_to(ctx, x + cosf(r) * radius * 0.7f,
                  y + sinf(r) * radius * 0.7f);
      ctx_line_to(ctx, x + cosf(r) * radius * 0.8f,
                  y + sinf(r) * radius * 0.8f);
    }
    ctx_stroke(ctx);
  }
  ctx_line_width(ctx, radius * 0.01f);
  ctx_line_cap(ctx, CTX_CAP_ROUND);

  ctx_line_width(ctx, radius * 0.075f);
  ctx_line_cap(ctx, CTX_CAP_ROUND);

  r = m * CTX_PI * 2 / 60.0 - CTX_PI / 2;
  ;

  ctx_move_to(ctx, x, y);
  ctx_line_to(ctx, x + cosf(r) * radius * 0.7f, y + sinf(r) * radius * 0.7f);
  ctx_stroke(ctx);

  r = (h + m / 60.0) * CTX_PI * 2 / 12.0 - CTX_PI / 2;
  ctx_move_to(ctx, x, y);
  ctx_line_to(ctx, x + cosf(r) * radius * 0.4f, y + sinf(r) * radius * 0.4f);
  ctx_stroke(ctx);

  ctx_line_width(ctx, radius * 0.02f);
  ctx_line_cap(ctx, CTX_CAP_NONE);

  r = (s + ms / 1000.0f) * CTX_PI * 2 / 60 - CTX_PI / 2;

  ctx_move_to(ctx, x, y);
  ctx_line_to(ctx, x + cosf(r) * radius * 0.78f, y + sinf(r) * radius * 0.78f);
  ctx_stroke(ctx);

  ctx_restore(ctx);
}

MAIN(clock) {
  // Ctx *ctx = ctx_host();//new (-1,-1,NULL);
  Ctx *ctx = ctx_new(-1, -1, NULL);
  do {
    ctx_start_frame(ctx);

    float w = ctx_width(ctx);
    float h = ctx_height(ctx);
    float min = w < h ? w : h;
#if 0
      if (ui)
      {
        ui_draw_bg (ui); // this draws the configured bacground and leaves ctx
                         // with the configured foreground color set
      }
      else
#endif
    {
      // when running directly on host without a wrapping ctx we
      // do not get a ui - this makes that work
      ctx_gray(ctx, 0.0f);
      ctx_rectangle(ctx, 0, 0, ctx_width(ctx), ctx_height(ctx));
      ctx_fill(ctx);
      ctx_rgba(ctx, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    draw_clock(ctx, w / 2, h / 2, min / 2, ctx_ticks() / 1000);

    ctx_add_key_binding(ctx, "escape", "exit", "foo", ui_cb_do, ui_host(NULL));
    ctx_add_key_binding(ctx, "backspace", "exit", "foo", ui_cb_do,
                        ui_host(NULL));
    ctx_end_frame(ctx);
  } while (!ctx_has_exited(ctx));
  ctx_destroy(ctx);
  return 0;
}
