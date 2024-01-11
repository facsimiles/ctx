#include "s0il.h"

void draw_clock(Ctx *ctx, float x, float y, float radius,
                uint64_t ms_since_midnight) {
  uint64_t ms = ms_since_midnight;
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;
  // int smoothstep = 1;

  ctx_save(ctx);

  ms = ((uint64_t)(ms)) % 1000;
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

void view_clock (Ui *ui)
{
  Ctx *ctx = ui_ctx (ui);
  float w = ctx_width(ctx);
  float h = ctx_height(ctx);
  float min = w < h ? w : h;
  ui_start_frame(ui);

  struct timeval t;
  gettimeofday(&t, NULL);

  int tz = +1;

  t.tv_sec += tz * (60 * 60);

  draw_clock(ctx, w / 2, h / 2, min / 2, t.tv_sec * 1000 + t.tv_usec / 1000);

  ui_add_key_binding(ui, "escape", "exit", "leave view");
  ui_add_key_binding(ui, "backspace", "exit", "leave view");

  ui_end_frame(ui);
}

MAIN(clock) {
#if 0
  Ctx *ctx = ctx_new(-1, -1, NULL);
  Ui *ui = ui_new(ctx);
  do {
    ctx_start_frame(ctx);

    float w = ctx_width(ctx);
    float h = ctx_height(ctx);
    float min = w < h ? w : h;
    ui_start_frame(ui);

    struct timeval t;
    gettimeofday(&t, NULL);

    int tz = +1;

    t.tv_sec += tz * (60 * 60);

    draw_clock(ctx, w / 2, h / 2, min / 2, t.tv_sec * 1000 + t.tv_usec / 1000);

    ui_add_key_binding(ui, "escape", "exit", "leave view");
    ui_add_key_binding(ui, "backspace", "exit", "leave view");

    ui_end_frame(ui);
    ctx_end_frame(ctx);
  } while (!ctx_has_exited(ctx));
  ui_destroy(ui);
  ctx_destroy(ctx);
  return 0;
#else
  if (argv[1] && !strcmp(argv[1], "--register"))
    s0il_add_view(ui_host(ctx_host()), "clock", view_clock, NULL);
  else
  {
    s0il_push_fun(ui_host(ctx_host()), view_clock, "clock", NULL, NULL);
  }
  return 42;
#endif
}
