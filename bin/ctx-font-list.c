#include "ctx.h"

#if CTX_BIN_BUNDLE
int ctx_font_list_main (int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{
  Ctx *ctx = ctx_new_drawlist (0,0);

  for (int i = 0; i < CTX_MAX_FONTS; i++)
  {
    const char *name = ctx_get_font_name (ctx, i);
    if (name)
      printf ("%s\n", name);
  }
  ctx_destroy (ctx);
  return 0;
}
