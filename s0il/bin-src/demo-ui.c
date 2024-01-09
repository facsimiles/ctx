#include "s0il.h"

static void app_ui_view_foo(Ui *ui) {
  ui_start_frame(ui);
  ui_text(ui, "fnordfoo");
  ui_end_frame(ui);
}

MAIN(demo_ui) {
  Ctx *ctx = ctx_new(240, 240, NULL);
  if (!ctx)
    return -1;

  Ui *ui = ui_new(ctx);
  s0il_push_fun(ui, app_ui_view_foo, NULL, NULL, NULL);
  s0il_main(ui);

  ui_destroy(ui);
  ctx_destroy(ctx);

  return 0;
}
