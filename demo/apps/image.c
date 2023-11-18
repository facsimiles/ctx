#include "ui.h"

void view_image (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   ui_start (ui);
   int width = 0;
   int height = 0;
   char eid[64];
   ctx_texture_load (ctx, ui->location, &width, &height, eid);
   if (width)
   {
     float factor  = ui->width / ((float)width);
     float factorv = ui->height / ((float)height);
     
     if (factorv < factor)
       factor = factorv;
     
     ctx_draw_texture (ctx, eid, 
        (ui->width - width * factor)/2,
        (ui->height - height * factor)/2,
        width * factor, height * factor);
   }
   ui_end (ui);
}

