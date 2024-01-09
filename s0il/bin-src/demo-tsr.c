#include "s0il.h"

static void tsr_view_foo(Ui *ui) {
  char buf[23];
  const char *str = "fnord";
  ui_start_frame(ui);
  ui_text(ui, str);
  static int i = 0;
  sprintf(buf, "f%i", i++);
  ui_text(ui, buf);
  ui_end_frame(ui);
}

MAIN(demo_tsr) {
  Ui *ui = ui_host(NULL);

  // XXX: this is ugly
  s0il_pop_fun(ui);
  s0il_push_fun(ui, tsr_view_foo, NULL, NULL, NULL);
  s0il_push_fun(ui, tsr_view_foo, NULL, NULL, NULL);

  return 42;
}
