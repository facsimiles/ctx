#include "s0il.h"

static void tsr_view_foo (Ui *ui)
{
  char buf[23];
  const char *str = "fnord";
  ui_start_frame(ui);
  ui_text(ui, str);
  static int i = 0;
  sprintf (buf, "f%i", i++);
  ui_text(ui, buf);
  ui_end_frame(ui);
}

MAIN(tsr_ui)
{
  Ui *ui = ui_host(NULL);

  // XXX: this is ugly
  ui_pop_fun (ui);
  ui_push_fun (ui, tsr_view_foo, NULL, NULL, NULL);
  ui_push_fun (ui, tsr_view_foo, NULL, NULL, NULL);

  return 42;
}
