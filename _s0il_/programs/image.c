#include "s0il.h"

MAIN(image)
{
   if (argv[1]==NULL)
   {
     Ui *ui = ui_host(NULL);
     char png_magic[]={0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
     char jpg_magic1[]={0xff, 0xd8, 0xff, 0xdb, 0xff, 0xd8, 0xff, 0xe0};
     char jpg_magic2[]={0xff, 0xd8, 0xff, 0xe0};
     char jpg_magic3[]={0xff, 0xd8, 0xff, 0xee};
     char jpg_magic4[]={0xff, 0xd8, 0xff, 0xe1};
    
     magic_add("image/png",  NULL, png_magic,  sizeof(png_magic),  0);
     magic_add("image/jpeg", NULL, jpg_magic1, sizeof(jpg_magic1), 0);
     magic_add("image/jpeg", NULL, jpg_magic2, sizeof(jpg_magic2), 0);
     magic_add("image/jpeg", NULL, jpg_magic3, sizeof(jpg_magic3), 0);
     magic_add("image/jpeg", NULL, jpg_magic4, sizeof(jpg_magic4), 0);
// {0, "image/gif",  ".gif", 6, {0x47, 0x49, 0x46, 0x38, 0x37, 0x61}},
// {0, "image/gif",  ".gif", 6, {0x47, 0x49, 0x46, 0x38, 0x39, 0x61}},


     ui_register_view(ui, "image/png",  NULL, argv[0]);
     ui_register_view(ui, "image/jpeg", NULL, argv[0]);

     return 0;
   }

   int frame_no = 0;
   Ctx *ctx = ctx_new (-1, -1, NULL);

   int width = 0;
   int height = 0;
   char eid[64];
   do {
      ctx_start_frame (ctx);

  //  if (frame_no == 0)
        ctx_texture_load (ctx, argv[1], &width, &height, eid);
      int dwidth = ctx_width (ctx);// + frame_no;
      int dheight = ctx_height (ctx);//+ frame_no;
      if (width)
      {
        float factor  = dwidth / ((float)width);
        float factorv = dheight / ((float)height);
        
        if (factorv < factor)
          factor = factorv;
        
        ctx_draw_image (ctx, argv[1], 
           (dwidth - width * factor)/2,
           (dheight - height * factor)/2,
           width * factor, height * factor);
   
      ctx_add_key_binding (ctx, "escape", "exit", "foo",  ui_cb_do, ui_host(ctx));
      ctx_add_key_binding (ctx, "backspace", "exit", "foo",  ui_cb_do, ui_host(ctx));
   
      }
      ctx_end_frame (ctx);
      frame_no ++;
    } while (!ctx_has_exited (ctx));
   ctx_destroy(ctx);

  return 0;
}
