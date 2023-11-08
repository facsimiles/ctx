#include "ui.h"

void view_captouch (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   ui_start(ui);

   for (int i = 0; i < 10; i++)
   {
      ctx_save (ctx);
      ctx_translate (ctx, ui->width/2, ui->height/2);
      ctx_rotate (ctx, (i+bsp_captouch_angular(i)/32767.0) / 10.0f * M_PI * 2 + M_PI);
      ctx_rectangle (ctx, -5, (bsp_captouch_radial(i)/65535.0f) * ui->height/2-5, 10, 10);
      if (i % 2 == 0)
        ctx_rgba (ctx, 1, 0, 1, bsp_captouch_down(i)?1.0f:0.5f);
      else
        ctx_rgba (ctx, 0, 1, 1, bsp_captouch_down(i)?1.0f:0.5f);
      ctx_fill(ctx);
      ctx_restore (ctx);
   }
   float rad_pos = 0.0f;
   float angle = bsp_captouch_angle(&rad_pos);
   if (angle >= 0.0f)
   {
     ctx_save (ctx);
     ctx_translate (ctx, ui->width/2, ui->height/2);
     ctx_rotate (ctx, angle * M_PI * 2 + M_PI);
     ctx_move_to (ctx, 0, 0.8 * ui->height/2);
     ctx_line_to (ctx, 0, ui->height/2);
     ctx_gray (ctx, 1.0f);
     ctx_stroke (ctx);
     ctx_restore (ctx);
     
   }

   ui_end(ui);
}
