#include "ctx.h"
#include "ui.h"
#include <unistd.h>


static void app_ui_view_foo (Ui *ui)
{
  ui_start_frame(ui);
  ui_text(ui, "fnordfoo");
  ui_end_frame(ui);
}


MAIN(app_ui)
{
  Ctx *ctx = ctx_new(240,240,NULL);
  if (!ctx) return -1;

  Ui *ui = ui_new (ctx); 
  // XXX : this does not work - but app-ui2 does
  //       which is directly reusing the ui context without
  //       creating a new one - this due to the registered
  //       views... 

  ui_push_fun (ui, app_ui_view_foo, NULL, NULL, NULL);
  ui_main (ui, NULL);

  ui_destroy (ui);
  ctx_destroy (ctx);

  return 0;
}
