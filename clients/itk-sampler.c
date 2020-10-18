#include <unistd.h>
#include "ctx.h"
#include "itk.h"

int do_quit = 0;
void itk_key_quit (CtxEvent *event, void *userdata, void *userdata2)
{
  do_quit = 1;
}

extern int _ctx_threads;

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
  itk->dirty = 1;
  while (!do_quit)
  {
    float width = ctx_width (ctx);
    if (itk->dirty)
    {
      int width = ctx_width (ctx);
      int height = ctx_height (ctx);
      int x = 0;
      int y = 0;
      itk_reset (itk);
      itk->dirty=0;

      ctx_save (ctx);
      ctx_rectangle (ctx, x, y, width, height);
      ctx_gray (ctx, 0);
      ctx_fill (ctx);

      if(1)itk_panel_start (itk, "Immediate Toolkit", x+30, y+30, width-60, height-100);
      else itk_panel_start (itk, "Immediate Toolkit", x+0, y+0, width, height);
      itk_seperator (itk);
      itk_begin_menu_bar (itk, "main");
       itk_begin_menu (itk, "foo");
        itk_menu_item (itk, "foo 1");
        itk_menu_item (itk, "foo 2");
       itk_end_menu (itk);
       itk_begin_menu (itk, "bar");
        itk_menu_item (itk, "bar 1");
        itk_menu_item (itk, "bar 2");
 
        itk_begin_menu (itk, "baz");
          itk_menu_item (itk, "baz 1");
          itk_menu_item (itk, "baz 2");
          itk_menu_item (itk, "baz 3");
        itk_end_menu (itk);

       itk_end_menu (itk);
      itk_end_menu_bar (itk);
      itk_seperator (itk);

      static int presses = 0;
      if (itk_button (itk, "button"))
        presses ++;

      if (presses % 2)
      {
        itk_sameline (itk);
        itk_label (itk, "label");
      }

      enum Mode
      {
        Mode_Rew,
        Mode_Fwd,
        Mode_Play,
      };

      static int mode = Mode_Fwd;
      if (itk_radio(itk, "rew", mode==Mode_Rew)){mode = Mode_Rew;};
      itk_sameline (itk);
      if (itk_radio(itk, "fwd", mode==Mode_Fwd)){mode = Mode_Fwd;};
      itk_sameline (itk);
      if (itk_radio(itk, "play", mode==Mode_Play)){mode = Mode_Play;};


      static float slide_float = 10.0;
      itk_slider_float (itk, "slide float", &slide_float, 0.0, 100.0, 0.1);

      itk_entry (itk, "Foo", "text entry", (char*)&input, sizeof(input)-1, NULL, NULL);

      itk_choice (itk, "power", &chosen, NULL, NULL);
      itk_choice_add (itk, 0, "on");
      itk_choice_add (itk, 1, "off");
      itk_choice_add (itk, 2, "good");
      itk_choice_add (itk, 2025, "green");
      itk_choice_add (itk, 2030, "electric");
      itk_choice_add (itk, 2040, "novel");

      itk_toggle (itk, "baz ", &baz);

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

      static int ctx_settings = 0;
      if (itk_expander (itk, "CTX settings", &ctx_settings))
      {
        //itk_toggle (itk, "focus wraparound", &itk->focus_wraparound);
        static float val;
        val = _ctx_threads;
        itk_slider_float (itk, "threads", &val, 1.0, 8.0, 1.0);
      }

      static int itk_settings = 0;
      if (itk_expander (itk, "ITK settings", &itk_settings))
      {
        itk_toggle (itk, "focus wraparound", &itk->focus_wraparound);
        itk_toggle (itk, "enable keybindings", &enable_keybindings);
        itk_toggle (itk, "light mode", &itk->light_mode);
        itk_slider_float (itk, "global scale", &itk->scale, 0.1, 8.0, 0.1);
        itk_slider_float (itk, "font size ", &itk->font_size, 4.0, 60.0, 0.25);
        itk_slider_float (itk, "hgap", &itk->rel_hgap, 0.0, 3.0, 0.02);
        itk_slider_float (itk, "vgap", &itk->rel_vgap, 0.0, 3.0, 0.02);
        itk_slider_float (itk, "scroll speed", &itk->scroll_speed, 0.0, 16.0, 0.1);
        itk_slider_float (itk, "ver advance", &itk->rel_ver_advance, 0.1, 4.0, 0.01);
        itk_slider_float (itk, "baseline", &itk->rel_baseline, 0.1, 4.0, 0.01);
        itk_slider_float (itk, "hmargin", &itk->rel_hmargin, 0.0, 40.0, 0.1);
        itk_slider_float (itk, "vmargin", &itk->rel_vmargin, 0.0, 40.0, 0.1);
        itk_slider_float (itk, "value width", &itk->value_width, 0.0, 40.0, 0.02);
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
