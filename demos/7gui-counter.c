#include "itk.h"

static int counter_ui (ITK  *itk, void *data)
{
  static int value = 0;
  itk_panel_start (itk, "7gui - Counter",
                   0, 0, ctx_width (itk->ctx), ctx_height (itk->ctx));

  itk_labelf (itk, "%i", value);
  itk_sameline (itk);
  if (itk_button (itk, "count"))
    value++;

  itk_panel_end (itk);
  return 0;
}

int main (int argc, char **argv)
{
  itk_main (counter_ui, NULL);
  return 0;
}