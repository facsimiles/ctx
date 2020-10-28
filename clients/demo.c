#include <unistd.h>
#include <math.h>
#include "ctx.h"
#include "itk.h"

typedef void (*TestFun)(ITK *itk, int frame_no);
typedef struct Test{
  const char *title;
  TestFun     fun;
}Test;

extern Test tests[];
extern int n_tests;
int test_no = 0;
int frame_no = 0;

int do_quit = 0;
void itk_key_quit (CtxEvent *event, void *userdata, void *userdata2)
{
  do_quit = 1;
}

extern int _ctx_threads;
extern int _ctx_enable_hash_cache;

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  Ctx *ctx = ctx_new_ui (-1, -1);

  ITK *itk = itk_new (ctx);

  const CtxEvent *event;
  uint8_t abc = 11;
  int   baz = 1;
  int chosen = 1;
  int enable_keybindings = 1;
  char input[256]="fnord";
  itk->dirty = 1;
  while (!do_quit)
  {
    int width = ctx_width (ctx);
    int height = ctx_height (ctx);
      itk_reset (itk);
#if 0
    ctx_save (ctx);
    ctx_rectangle (ctx, 0, 0, width, height);
    ctx_gray (ctx, 0);
    ctx_fill (ctx);
#endif
    tests[test_no].fun (itk, frame_no++);

//  itk->dirty=1;
//  if (itk->dirty)
    {
      itk_panel_start (itk, "ctx and itk demo", 0, 0, width*0.2, height);
      itk_seperator (itk);
#if 0
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
#endif

      itk_slider_int   (itk, "demo no", &test_no, 0, n_tests-1, 1);

      static int itk_widgets = 0;
      if (itk_expander (itk, "ITK widgets", &itk_widgets))
      {

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
      //itk_slider_cb    (itk, "slide cb", &slide_float, -10.0, 10.0, 0.5, NULL, get_float, set_float, NULL);
      //0.0, 100.0, 0.1);
      static int   slide_int = 10;
      itk_slider_int   (itk, "slide int", &slide_int, 0, 100, 1);
      //
      itk_slider_uint8  (itk, "slide byte", &abc, 0, 100, 1);
      
      itk_entry (itk, "Foo", "text entry", (char*)&input, sizeof(input)-1, NULL, NULL);

      itk_choice (itk, "power", &chosen, NULL, NULL);
      itk_choice_add (itk, 0, "on");
      itk_choice_add (itk, 1, "off");
      itk_choice_add (itk, 2, "good");
      itk_choice_add (itk, 2025, "green");
      itk_choice_add (itk, 2030, "electric");
      itk_choice_add (itk, 2040, "novel");

      itk_toggle (itk, "baz ", &baz);
      }

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


      itk_ctx_settings (itk);

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
    while ((event = ctx_get_event (ctx)))
    {
   //   if (event->type == CTX_MOTION){
   //           itk->dirty++;
   //   };//
    }
  }
  ctx_free (ctx);
  return 0;
}

static void card_gradients (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  int width  = ctx_width (ctx);
  int height = ctx_height(ctx);
  frame_no %= 256;

  ctx_linear_gradient (ctx, 0.0, 0.0,  0.0, height);
  ctx_gradient_add_stop (ctx, 0, 1, 1, 1, 1);
  ctx_gradient_add_stop (ctx, 1, 0, 0, 0, 1);
  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_fill (ctx);

  ctx_radial_gradient (ctx, width * 0.4 + frame_no, height * 0.4, height * 0.1,
                            width * 0.4 + frame_no, height * 0.4, height * 0.4);
  ctx_gradient_add_stop (ctx, 0, 1, 1, 1, 1);
  ctx_gradient_add_stop (ctx, 1, 0, 0, 0, 1);
  ctx_arc (ctx, width/2 + frame_no, height/2, height * 0.3, 0, 1.9 * CTX_PI, 0);
  ctx_fill (ctx);
}

static void card_dots (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  static int   dot_count = 100;
  static float twist = -0.1619;
  static float dot_scale = 160.0;

      /* clear */
      ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
      ctx_rgba8 (ctx, 0,0,0,255);
      ctx_fill (ctx);

      ctx_rgba(ctx, 1, 1, 1, 0.5);
      for (int i = 0; i < dot_count; i ++)
      {
        float x = ctx_width (ctx)/ 2;
        float y = ctx_height (ctx) / 2;
        float radius = ctx_height (ctx) / dot_scale;
        float dist = i * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
        float twisted = (i * twist);
        x += cos (twisted) * dist;
        y += sin (twisted) * dist;
        ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
        ctx_fill (ctx);
      }

      itk_panel_start (itk, "spiraling dots", ctx_width(ctx)*3/4,0,ctx_width(ctx)/4, ctx_height(ctx)/3);
      itk_slider_int (itk, "count",          &dot_count, 1,   4000, 10);
      itk_slider_float (itk, "radius",    &dot_scale, 2.0, 200.0, 4.5);
      itk_slider_float (itk, "twist amount", &twist, -3.14152, 3.14152, 0.0005);
      if (itk_button (itk, "+0.0001"))
      {
        twist += 0.0001;
      }
      if (itk_button (itk, "-0.0001"))
      {
        twist -= 0.0001;
      }

      itk_ctx_settings (itk);
      itk_panel_end (itk);
}

static void slider (Ctx *ctx, float x0, float y0, float width, float pos)
{
  int height = ctx_height (ctx);
  ctx_gray (ctx, 1.0);
  ctx_line_width (ctx, height * 0.025);
  ctx_move_to (ctx, x0, y0);
  ctx_line_to (ctx, x0 + width, y0);
  ctx_stroke (ctx);
  ctx_arc (ctx, x0 + width * pos, y0, height * 0.05, 0.0, CTX_PI*1.95, 0);
  ctx_fill (ctx);
}

static void card_sliders (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  int height = ctx_height (ctx);
  int width = ctx_width (ctx);
  frame_no = frame_no % 400;
  /* a custom shape that could be wrapped in a function */

  ctx_round_rectangle (ctx, height * 0.1, height * 0.1,
                            width - height * 0.2,
                            height - height * 0.2,
                            height * 0.1);
  ctx_rgba (ctx, 0.5, 0.5, 1, 1);
  ctx_fill (ctx);

  slider (ctx, height * 0.2, height * 0.4, width - height * 0.4, (frame_no  % 400) / 400.0);
  slider (ctx, height * 0.2, height * 0.5, width - height * 0.4, (frame_no  % 330) / 330.0);
  slider (ctx, height * 0.2, height * 0.6, width - height * 0.4, (frame_no  % 100) / 100.0);
}

static void _analog_clock (Ctx     *ctx,
                           uint64_t ms,
                           float    x,
                           float    y,
                           float    radius,
                           int smoothstep)
{
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;

  ms = ((uint32_t)(ms)) % 1000;
  s %= 60;
  m %= 60;
  h %= 12;

  float r;
  ctx_save (ctx);

  ctx_rgba8 (ctx, 255, 255, 255, 196);

#if 0
  ctx_set_rgba_u8 (ctx, 127, 127, 127, 255);
  ctx_move_to (ctx, x, y);
  ctx_arc (ctx, x, y, radius * 0.9, 0.0, CTX_PI * 2, 0);
  ctx_set_line_width (ctx, radius * 0.2);
  ctx_set_line_cap (ctx, CTX_CAP_NONE);
  ctx_stroke (ctx);
#else
  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

  for (int markh = 0; markh < 12; markh++)
  {
    r = markh * CTX_PI * 2 / 12.0 - CTX_PI / 2;

    ctx_move_to (ctx, x + cosf(r) * radius * 0.7f, y + sinf (r) * radius * 0.7f);
    ctx_line_to (ctx, x + cosf(r) * radius * 0.8f, y + sinf (r) * radius * 0.8f);
    ctx_stroke (ctx);
  }
  ctx_line_width (ctx, radius * 0.01f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

#if 0
  for (int markm = 0; markm < 60; markm++)
  {
    r = markm * CTX_PI * 2 / 60.0 - CTX_PI / 2;

    ctx_move_to (ctx, x + cosf(r) * radius * 0.75f, y + sinf (r) * radius * 0.75f);
    ctx_line_to (ctx, x + cosf(r) * radius * 0.8f, y + sinf (r) * radius * 0.8f);
    ctx_stroke (ctx);
  }
#endif


#endif

  ctx_line_width (ctx, radius * 0.075f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

  r = m * CTX_PI * 2 / 60.0 - CTX_PI / 2;
  ;
#if 1
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.7f, y + sinf (r) * radius * 0.7f);
  ctx_stroke (ctx);
#endif

  r = h * CTX_PI * 2 / 12.0 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.4f, y + sinf (r) * radius * 0.4f);
  ctx_stroke (ctx);


  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_NONE);

  if (smoothstep)
    r = (s + ms / 1000.0f) * CTX_PI * 2 / 60 - CTX_PI / 2;
  else
    r = (s ) * CTX_PI * 2 / 60 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.78f, y + sinf (r) * radius * 0.78f);
  ctx_stroke (ctx);

  ctx_restore (ctx);
}

static void card_clock1 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  uint64_t ms64 = ctx_ticks() / 1000;
  int width = ctx_width (ctx);
  int height = ctx_height (ctx);
  _analog_clock (ctx, ms64, width/2, height/2, height/2, 1);
}

static void card_clock2 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  uint64_t ms64 = ctx_ticks()/ 1000;
  int width = ctx_width (ctx);
  int height = ctx_height (ctx);
  _analog_clock (ctx, ms64, width/2, height/2, height/2, 0);
}

static void card_fill_rule (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;

  ctx_save (ctx);
  ctx_translate (ctx, 40.0, 0);
  ctx_scale (ctx, 2.4, 2.4);
  ctx_line_width (ctx, 6);

  ctx_arc (ctx, 64, 64, 40, 0, 1.9*CTX_PI, 0);
  ctx_close_path (ctx);
  ctx_arc (ctx, 192, 64, 40, 0, -1.9*CTX_PI, 1);
  ctx_close_path (ctx);
  ctx_rectangle (ctx, 12, 12, 232, 70);

  ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
  ctx_rgba (ctx, 0, 0.7, 0, 1);
  ctx_preserve (ctx);
  ctx_fill (ctx);
  ctx_rgba (ctx, 1, 1, 1, 1); ctx_stroke (ctx);

  ctx_translate (ctx, 0, 128);
  ctx_arc (ctx, 64, 64, 40, 0, 1.9*CTX_PI, 0);
  ctx_close_path (ctx);
  ctx_arc (ctx, 192, 64, 40, 0, -1.9*CTX_PI, 1);
  ctx_close_path (ctx);
  ctx_rectangle (ctx, 12, 12, 232, 70);

  ctx_fill_rule (ctx, CTX_FILL_RULE_WINDING);
  ctx_rgba (ctx, 0, 0, 0.9, 1);
  ctx_preserve (ctx);
  ctx_fill (ctx);
  ctx_rgba (ctx, 1, 1, 1, 1); ctx_stroke (ctx);
  ctx_restore (ctx);
}

static void card_curve_to (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  ctx_save (ctx);
  frame_no %= 400;
  ctx_translate (ctx, 40.0, 0);
  ctx_scale (ctx, 0.33, 0.33);
  float x=25.6,  y=128.0;
  float x1=102.4 + frame_no - 102, y1=230.4,
        x2=153.6, y2=25.6 + frame_no,
        x3=230.4, y3=128.0;

  ctx_move_to (ctx, x, y);
  ctx_curve_to (ctx, x1, y1, x2, y2, x3, y3);

  ctx_line_width (ctx, 10.0);
  ctx_stroke (ctx);

  ctx_rgba (ctx, 1, 0.2, 0.2, 0.6);
  ctx_line_width (ctx, 6.0);
  ctx_move_to (ctx,x,y);   ctx_line_to (ctx,x1,y1);
  ctx_move_to (ctx,x2,y2); ctx_line_to (ctx,x3,y3);
  ctx_stroke (ctx);
  ctx_restore (ctx);
}

float rect1_x = 0.1;
float rect1_y = 0.1;
float rect2_x = 0.4;
float rect2_y = 0.4;

static void rect_drag (CtxEvent *event, void *data1, void *data2)
{
  float *x = data1;
  float *y = data2;
  *x += event->delta_x;
  *y += event->delta_y;
  event->stop_propagate=1;
}

static void card_drag (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  ctx_save (ctx);
  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_gray (ctx, 0);
  ctx_fill (ctx);
  frame_no %= 400;

  ctx_scale (ctx, width, height);

  ctx_rectangle (ctx, rect1_x, rect1_y, 0.2, 0.2);
  ctx_rgb (ctx, 1,0,0);
  ctx_listen (ctx, CTX_DRAG, rect_drag, &rect1_x, &rect1_y);
  ctx_fill (ctx);

  ctx_rectangle (ctx, rect2_x, rect2_y, 0.2, 0.2);
  ctx_rgb (ctx, 1,1,0);
  ctx_listen (ctx, CTX_DRAG, rect_drag, &rect2_x, &rect2_y);
  ctx_fill (ctx);

  ctx_restore (ctx);
}

Test tests[]=
{
  {"gradients", card_gradients},
  {"dots",       card_dots},
  {"drag",      card_drag},
  {"sliders",   card_sliders},
  {"clock1",     card_clock1},
  {"clock2",     card_clock2},
  {"fill rule",  card_fill_rule},
  {"curve to",  card_curve_to},
};
int n_tests = sizeof(tests)/sizeof(tests[0]);

