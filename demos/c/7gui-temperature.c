#include "itk.h"

static char celcius_val[20]="";
static char fahrenheit_val[20]="";

static void commit_celcius (ITK *itk, void *data)
{
}

static void commit_fahrenheit (ITK *itk, void *data)
{
}

static int temperature_ui (ITK *itk, void *data)
{
  Ctx *ctx = itk->ctx;
  itk_panel_start (itk, "7gui - Temperature Converter", ctx_width(ctx)*0.2, 0, ctx_width (ctx) * 0.8, ctx_height (ctx));

  if (itk_entry (itk, "celcius", "C", celcius_val, 20-1))
  {
    float celcius;
    CtxControl *control = itk_focused_control (itk);
    strcpy (control->val, itk->entry_copy);
    int invalid = 0;
    for (int i = 0; celcius_val[i]; i++)
      if (!((celcius_val[i] >= '0' && celcius_val[i] <= '9') || celcius_val[i]=='.'))
        invalid = 1;
    if (!invalid)
    {
      celcius = atof (celcius_val);
      sprintf (fahrenheit_val, "%.2f", celcius * (9/5.0) + 32);
    }
  }
  if (itk_entry (itk, "fahrenheit", "F", fahrenheit_val, 20-1))
  {
    float fahrenheit;
    CtxControl *control = itk_focused_control (itk);
    strcpy (control->val, itk->entry_copy);
    int invalid = 0;
    for (int i = 0; fahrenheit_val[i]; i++)
      if (!((fahrenheit_val[i] >= '0' && fahrenheit_val[i] <= '9') || fahrenheit_val[i]=='.'))
        invalid = 1;
    if (!invalid)
    {
      fahrenheit = atof (fahrenheit_val);
      sprintf (celcius_val, "%.2f", (fahrenheit - 32) * (5/9.0));
    }
  }
  itk_panel_end (itk);
  return 1;
}

int main (int argc, char **argv)
{
  itk_main (temperature_ui, NULL);
  return 0;
}
