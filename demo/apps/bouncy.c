#include "ui.h"

////////////////////////////////////////////////////////////////////
static float bx = 142;
static float by = 123;
static int is_down = 0;
static float vx = 2.0;
static float vy = 2.33;

static void bg_motion (CtxEvent *event, void *data1, void *data2)
{
  if (data1 || data2){}
  bx = event->x;
  by = event->y;
  vx = 0;
  vy = 0;
  if (event->type != CTX_DRAG_MOTION) is_down = 0;
  else is_down = 1;
    
  if (event->type == CTX_DRAG_RELEASE)
  {
    vx = event->delta_x;
    vy = event->delta_y;
  }
}

void view_bouncy (Ui *ui)
{
    Ctx *ctx = ui->ctx;
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    static float dim = 100;
    if (ui->frame_no == 0)
    {
      bx = width /2;
      by = height/2;
      vx = 2.0;
      vy = 2.33;
    }

    ctx_rectangle(ctx,0,0,width, height);
    ctx_listen (ctx, CTX_DRAG, bg_motion, NULL, NULL);
    ctx_begin_path (ctx); // clear the path, listen doesnt
    ui_draw_bg (ui);

    ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(ctx, CTX_TEXT_BASELINE_MIDDLE);
    ctx_move_to(ctx, width/2, height/2);ctx_text(ctx, "vector graphics");

    if (!is_down)
    {
      ctx_logo (ctx, bx, by, dim);
    }
    else
    {
      ctx_rgb (ctx, 0.5, 0.5, 1);
      ctx_rectangle(ctx, (int)bx, 0, 1, height);ctx_fill(ctx);
      ctx_rectangle(ctx, 0, (int)by, width, 1);ctx_fill(ctx);
    }
    bx += vx;
    by += vy;

    if (bx <= dim/2 || bx >= width - dim/2)
      vx *= -1;
    if (by <= dim/2 || by >= height - dim/2)
      vy *= -1;
}
//////////////////////////////////////////////
