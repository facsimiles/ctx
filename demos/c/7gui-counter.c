#include "itk.h"

static int counter_ui (ITK  *itk, void *data)
{
  static int value = 0;
  Ctx *ctx = itk_ctx (itk);
  float em = itk_em (itk);
  itk_panel_start (itk, "7gui - Counter",
                   0, 0, ctx_width (ctx), ctx_height (ctx));

  itk_set_xy (itk, em *3, em *4);

  itk_labelf (itk, "%i", value);
  if (itk_button (itk, "count"))
    value++;

#if 0
  itk_labelf (itk, "%i", value);
  itk_sameline (itk);
  if (itk_button (itk, "count"))
    value++;
#endif

  itk_panel_end (itk);
  return 1;
}

int main (int argc, char **argv)
{
  itk_main (counter_ui, NULL);
  return 0;
}
