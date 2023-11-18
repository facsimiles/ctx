#include "ui.h"

void view_text (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   if (ui->data == NULL)
   {
     ui_load_file (ui, ui->location);
   }

   ui_start (ui);
   ui_text(ui,ui->location);
   if (ui->data)
   {
     ctx_save(ctx);
     ctx_font(ctx, "Mono");
     ctx_text_align (ctx, CTX_TEXT_ALIGN_START);
     ctx_font_size(ctx, ui->font_size);
     float cw = ctx_text_width (ctx, " ");

     char word[128];
     int wordlen = 0;
     char *p = ui->data;

     float y = ui->y;
     float x0 = cw * 2;
     float x = x0;
     float x1 = ui->width - x0;
     float line_height = ui->font_size;
      
     while (*p)
     {
        switch (*p)
        {
          case ' ':
            {
              float word_width = (ctx_utf8_strlen (word)) * cw;
              if (x + word_width >= x1)
              {
                y += line_height;
                x = x0;
              }
              ctx_move_to (ctx, x, y);
              ctx_text (ctx, word);
              wordlen = 0;
              x += word_width + cw;
            }
            break;
          case '\n':
            {
              float word_width = (ctx_utf8_strlen (word)) * cw;
              if (x + word_width >= x1)
              {
                y += line_height;
                x = x0;
              }
              ctx_move_to (ctx, x, y);
              ctx_text (ctx, word);
              wordlen = 0;
              y += line_height;
              x = x0;
            }
            break;
          default:
            if (wordlen + 1 < 128)
            { word[wordlen++] = *p;
              word[wordlen] = 0;
            } break;
        }
        p++;
     }
     if (wordlen)
     {
       float word_width = (ctx_utf8_strlen (word)) * cw;
       if (x + word_width >= x1)
       {
         y += line_height;
         x = x0;
       }
       ctx_move_to (ctx, x, y);
       ctx_text (ctx, word);
       wordlen = 0;
       x += word_width + cw;
     }
     ctx_restore(ctx);
   }
   ui_end (ui);
}
