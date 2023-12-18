#include "s0il.h"

#if CTX_FLOW3R

void view_captouch(Ui *ui) {
  Ctx *ctx = ui_ctx(ui);
  float width = ctx_width(ctx);
  float height = ctx_height(ctx);
  ui_start_frame(ui);

  for (int i = 0; i < 10; i++) {
    ctx_save(ctx);
    ctx_translate(ctx, width / 2, height / 2);
    ctx_rotate(ctx, (i + bsp_captouch_angular(i) / 32767.0) / 10.0f * M_PI * 2 +
                        M_PI);
    ctx_rectangle(ctx, -5, (bsp_captouch_radial(i) / 65535.0f) * height / 2 - 5,
                  10, 10);
    if (i % 2 == 0)
      ctx_rgba(ctx, 1, 0, 1, bsp_captouch_down(i) ? 1.0f : 0.5f);
    else
      ctx_rgba(ctx, 0, 1, 1, bsp_captouch_down(i) ? 1.0f : 0.5f);
    ctx_fill(ctx);
    ctx_restore(ctx);
  }
  float rad_pos = 0.0f;
  int petal_mask = 0; // (1<<4) | (1<<6);
  float angle = bsp_captouch_angle(&rad_pos, 0, petal_mask);
  float angle_quant = bsp_captouch_angle(&rad_pos, 20, petal_mask);
  if (angle >= 0.0f) {

    ctx_save(ctx);
    ctx_translate(ctx, width / 2, height / 2);
    ctx_rotate(ctx, angle_quant * M_PI * 2 + M_PI);
    ctx_move_to(ctx, 0, 0.6 * height / 2);
    ctx_line_to(ctx, 0, 0.9 * height / 2);
    ctx_rgba(ctx, 1.0f, 0.5f, 0, 1);
    ctx_line_width(ctx, 6.0);
    ctx_stroke(ctx);
    ctx_restore(ctx);

    ctx_save(ctx);
    ctx_translate(ctx, width / 2, height / 2);
    ctx_rotate(ctx, angle * M_PI * 2 + M_PI);
    ctx_move_to(ctx, 0, 0.8 * height / 2);
    ctx_line_to(ctx, 0, height / 2);
    ctx_line_width(ctx, 8.0);
    ctx_gray(ctx, 1.0f);
    ctx_stroke(ctx);
    ctx_restore(ctx);
  }

  ui_end_frame(ui);
}

#endif
MAIN() {
#if CTX_FLOW3R
  Ui *ui = ui_host(NULL);
  ui_pop_fun(ui);
  ui_push_fun(ui, view_captouch, NULL, NULL, NULL);
#endif
  return 42;
}
