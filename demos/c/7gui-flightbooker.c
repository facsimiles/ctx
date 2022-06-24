#include "itk.h"

static int booker_ui (ITK *itk, void *data)
{
  Ctx *ctx = itk->ctx;
  static char depart_date[20]="22.09.1957";
  static char return_date[20]="22.09.1957";
  itk_panel_start (itk, "7gui - Flight Booker", ctx_width(ctx)*0.2, 0, ctx_width (ctx) * 0.8, ctx_height (ctx));

  static int return_flight = 0;
  itk_set_flag (itk, ITK_FLAG_SHOW_LABEL, 0);
  return_flight = itk_choice (itk, "", return_flight);
  itk_choice_add (itk, 0, "one-way flight");
  itk_choice_add (itk, 1, "return flight");

  itk_entry_str_len (itk, "depart", "dd.mm.yyyy", depart_date, 20-1);

  if (!return_flight)
    itk_set_flag (itk, ITK_FLAG_ACTIVE, 0);
  itk_entry_str_len (itk, "return", "dd.mm.yyyy", return_date, 20-1);

  if (itk_button (itk, "Book"))
  {
  }

  itk_panel_end (itk);
  return 1;
}

int main (int argc, char **argv)
{
  itk_main (booker_ui, NULL);
  return 0;
}
