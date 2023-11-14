#include "ui.h"

void view_spirals (Ui *ui)
{
  Ctx *ctx = ui->ctx;
  static int dot_count = 0;
  static int shape = 0;
  float dot_scale = 42;
  static float twist = 2.9645f;

  if (ui->frame_no == 0) dot_count = 27;
  dot_count += 2;
  if (dot_count >= 42)
  {
    shape = !shape;
    dot_count = 27;
  }

  ui_draw_bg(ui);
  for (int i = 0; i < dot_count; i ++)
  {
    float x = ctx_width (ctx)/ 2;
    float y = ctx_height (ctx) / 2;
    float radius = ctx_height (ctx) / dot_scale;
    float dist = (i) * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
    float twisted = (i * twist);
    x += cosf (twisted) * dist;
    y += sinf (twisted) * dist;
    if (shape == 0)
    {
      ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
    }
    else
    {
      ctx_save (ctx);
      ctx_translate (ctx, x, y);
      ctx_rotate (ctx, twisted);
      ctx_rectangle (ctx, -radius, -radius, radius*2, radius*2);
      ctx_restore (ctx);
    }
    ctx_fill (ctx);
  }

}


