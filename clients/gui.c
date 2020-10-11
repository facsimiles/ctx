#include <unistd.h>
#include "ctx.h"
#include "gui.h"


void pressed (void *userdata)
{
  fprintf (stderr, "pressed\n");
}

int do_quit = 0;
void ui_key_quit (CtxEvent *event, void *userdata, void *userdata2)
{
  do_quit = 1;
}

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  Ctx *ctx = ctx_new_ui (-1, -1);

  CtxControls *cctx = cctx_new (ctx);

  char message[256] = "hello there";

  const CtxEvent *event;
  int mx, my;
  int   baz = 1;
  int   bax = 0;
  int chosen = 1;
  char input[256]="fnord";
  ctx_get_event (ctx);

  event = (void*)0x1;
  //fprintf (stderr, "[%s :%i %i]", event, mx, my);
  cctx->dirty = 1;
  while (!do_quit)
  {
    float width = ctx_width (ctx);
    if (cctx->width != width)
    {
      cctx->width = width;
      cctx->dirty++;
    }
    if (cctx->dirty)
    {
      cctx->dirty=0;
      ui_reset                  (cctx);

      ctx_save                  (ctx);
      ctx_rectangle             (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
      ctx_gray (ctx, 0);
      ctx_fill                  (ctx);

      ui_titlebar (cctx, "Test UI");
      ui_seperator (cctx);
      ui_entry (cctx, "Foo", "text entry", (char*)&input, sizeof(input)-1, NULL, NULL);
      ui_choice (cctx, "power", &chosen, NULL, NULL);
      ui_choice_add (cctx, 0, "on");
      ui_choice_add (cctx, 1, "off");
      ui_choice_add (cctx, 2, "good");
      ui_choice_add (cctx, 2025, "green");
      ui_choice_add (cctx, 2030, "electric");
      ui_choice_add (cctx, 2040, "novel");

      static int ui_settings = 1;
      if (ui_expander (cctx, "Ui settings", &ui_settings))
      {
        ui_slider (cctx, "font-size ", &cctx->font_size, 5.0, 100.0, 0.1);
  //    ui_slider (cctx, "width", &cctx->width, 5.0, 600.0, 1.0);
        ui_slider (cctx, "ver_advance", &cctx->rel_ver_advance, 0.1, 4.0, 0.01);
        ui_slider (cctx, "baseline", &cctx->rel_baseline, 0.1, 4.0, 0.01);
        ui_slider (cctx, "value_pos", &cctx->value_pos, 0.0, 40.0, 0.1);
        ui_slider (cctx, "value_width", &cctx->value_width, 0.0, 40.0, 0.02);
      }

      ui_toggle (cctx, "baz ", &baz);
      if (ui_button2 (cctx, "press me "))
      {
        fprintf (stderr, "imgui style press\n");
      }
      ui_sameline (cctx);
      if (ui_button2 (cctx, "or me"))
      {
        fprintf (stderr, "imgui style press2\n");
      }
      ui_toggle (cctx, "barx: ", &bax);
      ui_sameline (cctx);
      if (ui_button2 (cctx, "or me 3"))
      {
        fprintf (stderr, "imgui style press3\n");
      }

      ui_done (cctx);
      ctx_add_key_binding (ctx, "control-q", NULL, "foo", ui_key_quit, NULL);


      ui_key_bindings (cctx);

      ctx_flush           (ctx);
    }
    else
    {
      usleep (10000);
    }
    while (event = ctx_get_event (ctx))
    {
      ;//
    }
  }
  ctx_free (ctx);
  return 0;
}
