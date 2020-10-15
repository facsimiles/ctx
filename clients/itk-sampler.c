#include <unistd.h>
#include "ctx.h"
#include "itk.h"

void pressed (void *userdata)
{
  fprintf (stderr, "pressed\n");
}

int do_quit = 0;
void itk_key_quit (CtxEvent *event, void *userdata, void *userdata2)
{
  do_quit = 1;
}

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  Ctx *ctx = ctx_new_ui (-1, -1);

  ITK *itk = itk_new (ctx);

  const CtxEvent *event;
  int mx, my;
  int   baz = 1;
  int   bax = 0;
  int chosen = 1;
  int enable_keybindings = 1;
  char input[256]="fnord";
  ctx_get_event (ctx);

  event = (void*)0x1;
  //fprintf (stderr, "[%s :%i %i]", event, mx, my);
  itk->dirty = 1;
  while (!do_quit)
  {
    float width = ctx_width (ctx);
    if (itk->width != width)
    {
      itk->width = width;
      itk->dirty++;
    }
    if (itk->dirty)
    {
      int width = ctx_width (ctx);
      int height = ctx_height (ctx);
      int x = 0;
      int y = 0;
      itk->dirty=0;
      itk_reset (itk);

      ctx_save (ctx);
      ctx_rectangle (ctx, x, y, width, height);
      ctx_gray (ctx, 0);
      ctx_fill (ctx);

      itk_panel_start (itk, "Immediate Toolkit", x+30, y+30, width-60, height-100);
      itk_seperator (itk);

      enum Mode
      {
        Mode_Copy,
        Mode_Move,
        Mode_Swap
      };
#if 1
      static int mode = Mode_Move;
      if (itk_radio(itk, "copy", mode==Mode_Copy)){mode = Mode_Copy;};
      itk_sameline (itk);
      if (itk_radio(itk, "move", mode==Mode_Move)){mode = Mode_Move;};
      itk_sameline (itk);
      if (itk_radio(itk, "swap", mode==Mode_Swap)){mode = Mode_Swap;};
#endif
      static int presses = 0;

      if (itk_button (itk, "press me"))
      {
        presses ++;
        fprintf (stderr, "%i %i\n", presses, presses % 1);
      }

      if (presses % 2)
      {
        itk_sameline (itk);
        itk_label (itk, "thanks for pressing me");
      }
      itk_entry (itk, "Foo", "text entry", (char*)&input, sizeof(input)-1, NULL, NULL);
      itk_choice (itk, "power", &chosen, NULL, NULL);
      itk_choice_add (itk, 0, "on");
      itk_choice_add (itk, 1, "off");
      itk_choice_add (itk, 2, "good");
      itk_choice_add (itk, 2025, "green");
      itk_choice_add (itk, 2030, "electric");
      itk_choice_add (itk, 2040, "novel");

      static int itk_items = 0;
      if (itk_expander (itk, "items", &itk_items))
      {
        for (int i = 0; i < 15; i++)
        {
          char buf[20];
          sprintf (buf, "%i", i);
          itk_button (itk, buf);
        }
      }

      static int itk_settings = 0;
      if (itk_expander (itk, "Ui settings", &itk_settings))
      {
        itk_toggle (itk, "keybindings", &enable_keybindings);
        itk_slider (itk, "font size ", &itk->font_size, 5.0, 100.0, 0.1);
        itk_slider (itk, "hgap", &itk->rel_hgap, 0.0, 3.0, 0.1);
        itk_slider (itk, "vgap", &itk->rel_vgap, 0.0, 3.0, 0.1);
        itk_slider (itk, "scroll speed", &itk->scroll_speed, 0.0, 16.0, 0.1);
  //    itk_slider (itk, "width", &itk->width, 5.0, 600.0, 1.0);
        itk_slider (itk, "ver advance", &itk->rel_ver_advance, 0.1, 4.0, 0.01);
        itk_slider (itk, "baseline", &itk->rel_baseline, 0.1, 4.0, 0.01);
        itk_slider (itk, "value pos", &itk->value_pos, 0.0, 40.0, 0.1);
        itk_slider (itk, "value width", &itk->value_width, 0.0, 40.0, 0.02);
      }

      itk_toggle (itk, "baz ", &baz);
      if (itk_button (itk, " press me "))
      {
        fprintf (stderr, "imgui style press\n");
      }
      itk_sameline (itk);
      if (itk_button (itk, "or me"))
      {
        fprintf (stderr, "imgui style press2\n");
      }
      itk_toggle (itk, "barx: ", &bax);
      itk_sameline (itk);
      if (itk_button (itk, "or me 3"))
      {
        fprintf (stderr, "imgui style press3\n");
      }

      itk_panel_end (itk);

      itk_done (itk);

      ctx_add_key_binding (ctx, "control-q", NULL, "foo", itk_key_quit, NULL);

      if (enable_keybindings)
        itk_key_bindings (itk);

      ctx_flush (ctx);
    }
    else
    {
      usleep (10000);
    }
    while (event = ctx_get_event (ctx))
    {
   //   if (event->type == CTX_MOTION){
   //           itk->dirty++;
   //   };//
    }
  }
  ctx_free (ctx);
  return 0;
}
