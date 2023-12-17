#include "s0il.h"

static void app_ui_view_foo2(Ui *ui) {
  ui_start_frame(ui);
  ui_text(ui, "fnordfoo2");
  ui_end_frame(ui);
}

MAIN(app_ui2) {
  Ui *ui = ui_host(NULL);

  ui_push_fun(ui, app_ui_view_foo2, NULL, NULL, NULL);

  return 42;
}
