#include <unistd.h>
#include <math.h>
#include <ctype.h>
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
  //ctx_set_dirty (ctx, 1);
  while (!do_quit)
  {
    int width = ctx_width (ctx);
    int height = ctx_height (ctx);
#if 0
    ctx_save (ctx);
    ctx_rectangle (ctx, 0, 0, width, height);
    ctx_gray (ctx, 0);
    ctx_fill (ctx);
#endif

//  itk->dirty=1;
    if (ctx_is_dirty (ctx))
    {
 //   ctx_set_dirty (ctx, 0);
      ctx_reset (ctx);
      itk_reset (itk); // does set_dirty 0

      if (enable_keybindings)
        itk_key_bindings (itk);

      tests[test_no].fun (itk, frame_no++);
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


      ctx_flush (ctx);
    }
    else
    {
       usleep (1000 * 20);
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
  ctx_set_dirty (ctx, 1);
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
  ctx_set_dirty (ctx, 1);
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

typedef struct DragObject
{
  float x;
  float y;
  float width;
  float height;
  float red;
  float green;
  float blue;
  float alpha;
} DragObject;


static void rect_drag (CtxEvent *event, void *data1, void *data2)
{
  float *x = data1;
  float *y = data2;
  *x += event->delta_x;
  *y += event->delta_y;
  event->stop_propagate=1;
}

static void object_drag (CtxEvent *event, void *data1, void *data2)
{
  DragObject *obj = data1;
  obj->x += event->delta_x;
  obj->y += event->delta_y;
  event->stop_propagate=1;
  ctx_set_dirty (event->ctx, 1);
}

static void card_drag (ITK *itk, int frame_no)
{
  static DragObject objects[4];
  static int first_run = 1;
  if (first_run)
  for (int i = 0; i <  4; i++)
  {
    objects[i].x = (i+1) * 0.1;
    objects[i].y = (i+1) * 0.1;
    objects[i].width = 0.2;
    objects[i].height = 0.2;
    objects[i].red = 0.5;
    objects[i].red = 0.1;
    objects[i].red = 0.9;
    objects[i].alpha = 0.6;
    first_run = 0;
  }

  Ctx *ctx = itk->ctx;
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  ctx_save (ctx);
  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_gray (ctx, 0);
  ctx_fill (ctx);
  frame_no %= 400;

  ctx_scale (ctx, width, height);

  for (int i = 0; i <  4; i++)
  {
    ctx_rectangle (ctx, objects[i].x, objects[i].y, objects[i].width, objects[i].height);
    ctx_rgba (ctx, objects[i].red, objects[i].green, objects[i].blue, objects[i].alpha);
    ctx_listen (ctx, CTX_DRAG, object_drag, &objects[i], itk);
    ctx_fill (ctx);
  }
#if 0
  ctx_rectangle (ctx, rect1_x, rect1_y, 0.2, 0.2);
  ctx_rgb (ctx, 1,0,0);
  ctx_listen (ctx, CTX_DRAG, rect_drag, &rect1_x, &rect1_y);
  ctx_fill (ctx);

  ctx_rectangle (ctx, rect2_x, rect2_y, 0.2, 0.2);
  ctx_rgb (ctx, 1,1,0);
  ctx_listen (ctx, CTX_DRAG, rect_drag, &rect2_x, &rect2_y);
  ctx_fill (ctx);
#endif

  ctx_restore (ctx);
}


static void card_7GUI1 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  itk_panel_start (itk, "7gui1 - counter", 300, 0, ctx_width (ctx) - 300, ctx_height (ctx));
  static int value = 0;
  itk_labelf (itk, "%i\n", value);
  itk_sameline (itk);
  if (itk_button (itk, "count")){
    value++;
  }
  itk_panel_end (itk);
}

static char celcius_val[20]="";
static char fahrenheit_val[20]="";

static void commit_celcius (ITK *itk, void *data)
{
  float celcius;
  CtxControl *control = itk_focused_control (itk);
  strcpy (control->val, itk->entry_copy);
  for (int i = 0; celcius_val[i]; i++)
    if (!((celcius_val[i] >= '0' && celcius_val[i] <= '9') || celcius_val[i]=='.'))
      return;
  celcius = atof (celcius_val);
  sprintf (fahrenheit_val, "%.2f", celcius * (9/5.0) + 32);
}

static void commit_fahrenheit (ITK *itk, void *data)
{
  float fahrenheit;
  CtxControl *control = itk_focused_control (itk);
  strcpy (control->val, itk->entry_copy);
  for (int i = 0; fahrenheit_val[i]; i++)
    if (!((fahrenheit_val[i] >= '0' && fahrenheit_val[i] <= '9') || fahrenheit_val[i]=='.'))
      return;
  fahrenheit = atof (fahrenheit_val);
  sprintf (celcius_val, "%.2f", (fahrenheit - 32) * (5/9.0));
}


static void card_7GUI2 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  itk_panel_start (itk, "7gui1 - tempconv", 300, 0, ctx_width (ctx) - 300, ctx_height (ctx));

  itk_entry (itk, "celcius", "C", celcius_val, 20-1, commit_celcius, NULL);
  itk_entry (itk, "fahrenheit", "F", fahrenheit_val, 20-1, commit_fahrenheit, NULL);
  itk_panel_end (itk);
}

static void card_7GUI3 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  static char depart_date[20]="22.09.1957";
  static char return_date[20]="22.09.1957";
  itk_panel_start (itk, "7gui3 - book flight", 300, 0, ctx_width (ctx) - 300, ctx_height (ctx));

  static int return_flight = 0;
  itk_choice (itk, "", &return_flight, NULL, NULL);
  itk_choice_add (itk, 0, "one-way flight");
  itk_choice_add (itk, 1, "return flight");
  itk_entry (itk, "depart", "dd.mm.yyyy", depart_date, 20-1, NULL, NULL);
  itk_entry (itk, "return", "dd.mm.yyyy", return_date, 20-1, NULL, NULL);

  if (itk_button (itk, "Book"))
  {
  }

  itk_panel_end (itk);
}

static void card_7GUI4 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  static float e = 0.0;
  static float d = 10.0;

  itk_panel_start (itk, "7gui4 - timer", 300, 0, ctx_width (ctx) - 300, ctx_height (ctx));

  itk_slider_float (itk, "elapsed", &e, 0.0, d, 0.1);
  itk_labelf (itk, "%.1f of %.1f", e, d);
  itk_slider_float (itk, "duration", &d, 0.0, 300.0, 0.5);

  static unsigned long prev_ticks = 0;
  unsigned long ticks = ctx_ticks ();

  if (e<d)
  {
    if (prev_ticks)
      e += (ticks-prev_ticks)/1000.0/1000.0;
  }

  prev_ticks = ticks;

  if (itk_button (itk, "Reset"))
    e = 0.0;

  itk_panel_end (itk);
  ctx_set_dirty (itk->ctx, 1);
}

static char name[20]="";
static char surname[20]="";

#define MAX_NAMES 23

typedef struct _name
{ char name[80];
  char surname[80];
  int  id;
} Name;

static Name name_list[MAX_NAMES]={{"Unknown", "Slaritbartfast"},
                                  {"Øyvind", "Kolås"}};
static int name_count = 2;


static void card_7GUI5 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  static char filter_prefix[20];

  itk_panel_start (itk, "7gui5 - CRUD", 300, 0, ctx_width (ctx) - 300, ctx_height (ctx));

  itk->width/=2;
  itk_entry (itk, "Filter prefix:", "", filter_prefix, 20-1, NULL, NULL);

  int saved_y = itk->y;
  itk->x += itk->width;
  itk->x0 += itk->width;
  itk_entry (itk, "Name:", "",    name, 20-1, NULL, NULL);
  itk_entry (itk, "Surname:", "", surname, 20-1, NULL, NULL);
  itk->x -= itk->width;
  itk->x0 -= itk->width;

  itk->y=saved_y;

  for (int i = 0; i < name_count; i++)
  {
    int show = 1;
    if (filter_prefix[0])
    {
      for (int j = 0; filter_prefix[j] && j<20; j++)
      {
        if (tolower(name_list[i].surname[j]) != tolower(filter_prefix[j]))
          show = 0;
      }
    }
    if (show)
      itk_labelf (itk, "%s, %s", name_list[i].surname, name_list[i].name);
  }

  if (itk_button (itk, "Create"))
  {
    strcpy (name_list[name_count].name, name);
    strcpy (name_list[name_count].surname, surname);
    name_count++;
  }
  itk_sameline (itk);
  if (itk_button (itk, "Update"))
  {
  }
  itk_sameline (itk);
  if (itk_button (itk, "Delete"))
  {
  }
  itk_sameline (itk);

  itk->width*=2;
  itk_panel_end (itk);
}

typedef struct _Circle
{
  float x;
  float y;
  float radius;
} Circle;

#define MAX_CIRCLES 20
static Circle circle_list[MAX_CIRCLES];
static int circle_count = 0;

static void circle_editor_release_cb (CtxEvent *event, void *data1, void *data2)
{
  circle_list[circle_count].x = event->x;
  circle_list[circle_count].y = event->y;
  circle_list[circle_count].radius = 100;
  circle_count++;
  event->stop_propagate = 1;
  ctx_set_dirty (event->ctx, 1);
}

static void circle_editor_circle_release_cb (CtxEvent *event, void *data1, void *data2)
{
  fprintf (stderr, "!!!!!!\n");
  event->stop_propagate = 1;
  ctx_set_dirty (event->ctx, 1);
}

static int nearest_circle = -1;

static void circle_editor_motion_cb (CtxEvent *event, void *data1, void *data2)
{
  float nearest_sq_dist = 4096*4096;
  int nearest = -1;
  for (int i = 0; i < circle_count; i ++)
  {
     float sq_dist = (circle_list[i].x-event->x)*
                     (circle_list[i].x-event->x)+
                     (circle_list[i].y-event->y)*
                     (circle_list[i].y-event->y);
     if (sq_dist < circle_list[i].radius * circle_list[i].radius)
     if (sq_dist < nearest_sq_dist)
     {
       nearest_sq_dist = sq_dist;
       nearest = i;
     }
  }
  if (nearest != nearest_circle)
  {
    // it's changed
    nearest_circle = nearest;
    ctx_set_dirty (event->ctx, 1);
  }

  event->stop_propagate = 1;
}

static void card_7GUI6 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  itk_panel_start (itk, "7gui6 - circle editor", 300, 0, ctx_width (ctx) - 300, ctx_height (ctx));

  if (itk_button (itk, "Undo"))
  {
  }
  itk_sameline (itk);
  if (itk_button (itk, "Redo"))
  {
  }

  ctx_rectangle (ctx, itk->x, itk->y,
                  itk->width, itk->panel->height - (itk->y-itk->panel->y));
  ctx_listen (ctx, CTX_RELEASE, circle_editor_release_cb, NULL, NULL);
  ctx_listen (ctx, CTX_MOTION, circle_editor_motion_cb, NULL, NULL);

  ctx_rgb (ctx, 1,0,0);
  ctx_fill (ctx);

  for (int i = 0; i < circle_count; i ++)
  {
     if (i == nearest_circle)
       ctx_gray (ctx, 0.5);
     else
       ctx_gray (ctx, 1.0);
     ctx_arc (ctx, circle_list[i].x, circle_list[i].y, circle_list[i].radius, 0.0, 3*2, 0);
     ctx_fill (ctx);
     ctx_rectangle (ctx, circle_list[i].x - circle_list[i].radius,
                     circle_list[i].y - circle_list[i].radius,
                     circle_list[i].radius * 2, circle_list[i].radius * 2);
     ctx_listen (ctx, CTX_RELEASE, circle_editor_circle_release_cb, NULL, NULL);
     ctx_begin_path (ctx);
  }


  itk_panel_end (itk);
}

typedef enum {
  CELL_TYPE_NIL,
  CELL_TYPE_NUMBER,
  CELL_TYPE_FORMULA,
  CELL_TYPE_LABEL
} CellType;

typedef struct _Cell Cell;
struct _Cell {
  char  display[40];
  char  value[80];
  int   dirty;
  CellType type;
  double number;
  Cell *dependencies[30];
  int   dependencies_count;
};

static void cell_formula_compute(Cell *cell);
static void update_cell (Cell *cell)
{
  if (cell->dirty)
  {
    cell->type = CELL_TYPE_NIL;
    if (cell->value[0]==0)
    {
      cell->display[0] = 0;
      cell->dirty = 0;
      cell->number = 0.0;
      cell->type = CELL_TYPE_NIL;
      return;
    }
    int is_number = 1;
    for (int i = 0; cell->value[i]; i++)
    {
      int val = cell->value[i];
      if ( ! ((val >= '0' && val <= '9')  || val == '.'))
        is_number = 0;
    }

    if (is_number)
    {
      cell->number = atof (cell->value); // XXX - locale dependent
      //sprintf (cell->display, "%.2f", cell->number);
      strcpy (cell->display, cell->value);
      cell->type = CELL_TYPE_NUMBER;
    }
    else
    {
      if (cell->value[0]=='=')
      {
        cell_formula_compute (cell);
        cell->type = CELL_TYPE_FORMULA;
      }
      else
      {
        sprintf (cell->display, "%s", cell->value);
        cell->type = CELL_TYPE_LABEL;
      }
    }
    cell->dirty = 0;
  }
}

static int str_is_number (const char *str, double *number)
{
  //int is_number = 1;
  int len = 0;
  if (str[0] == 0) return 0;
  for (int i = 0; str[i]; i++)
  {
    if (!((str[i]>='0' && str[i]<='9') || str[i] == '.'))
    {
      break; 
    }
    len = i + 1;
  }
  if (((str[0]>='0' && str[0]<='9') || str[0] == '.'))
  {
    if (number) *number = atof (str); // XXX locale dependent
    return len;
  }
  return 0;
}

static int str_is_coord (const char *str, int *colp, int *rowp)
{
  int len = 0;
  if (str[0] >= 'A' && str[1] <= 'Z')
  {
    int col = str[0]-'A';
    if (str[1] && str[1] >= '0' && str[1] <= '9')
    {
      int row = 0;
      if (str[2] && str[2] >= '0' && str[2] <= '9')
      {
        row = (str[1] - '0') * 10 + (str[2]-'0');
        len = 3;
      }
      else
      {
        row = (str[1] - '0');
        len = 2;
      }
      if (colp) *colp = col;
      if (rowp) *rowp = row;
      return len;
    }
  }
  return 0;
}

#define SPREADSHEET_COLS 27
#define SPREADSHEET_ROWS 100

static Cell spreadsheet[SPREADSHEET_ROWS][SPREADSHEET_COLS]={0,};
static float col_width[SPREADSHEET_COLS];

static int spreadsheet_first_row = 0;
static int spreadsheet_first_col = 0;

static void cell_formula_compute(Cell *cell)
{
  double arg1 = 0.0;
  int operator = 0;
  double arg2 = 0.0;
  int arg1_col=0;
  int arg1_row=0;
  int arg2_col=0;
  int arg2_row=0;
  int len=0;
  int len2=0;

  int rest = 0;
  cell->number = -14;

  if ((len=str_is_coord (cell->value+1, &arg1_col, &arg1_row)))
  {
    if (cell == &spreadsheet[arg1_row][arg1_col])
    {
      sprintf (cell->display, "¡CIRCREF!");
      return;
    }
    update_cell (&spreadsheet[arg1_row][arg1_col]);
    arg1 = spreadsheet[arg1_row][arg1_col].number;
    rest = len + 1;
  }
  else if ((len=str_is_number (cell->value+1, &arg1)))
  {
    rest = len + 1;
  }

  if (rest)(operator = cell->value[rest]);
  if (operator && cell->value[rest+1])
  {
    if ((len2 = str_is_coord (cell->value+rest+1, &arg2_col, &arg2_row)))
    {
      if (cell == &spreadsheet[arg2_row][arg2_col])
      {
        sprintf (cell->display, "¡CIRCREF!");
        return;
      }
      update_cell (&spreadsheet[arg2_row][arg2_col]);
      arg2 = spreadsheet[arg2_row][arg2_col].number;
    }
    else if ((len2 = str_is_number (cell->value+rest+1, &arg2)))
    {
    }
  }

  switch (operator)
  {
    case 0:
            if (cell->value[1]=='s'&&
                cell->value[2]=='u'&&
                cell->value[3]=='m')
            {
              len  = str_is_coord (cell->value+4+1, &arg1_col, &arg1_row);
              len2 = str_is_coord (cell->value+4+1 + len + 1, &arg2_col, &arg2_row);
              if (len && len2)
              {
                double sum = 0.0f;
                for (int v = arg1_row; v <= arg2_row; v++)
                  for (int u = arg1_col; u <= arg2_col; u++)
                  {
                     if (&spreadsheet[v][u] != cell)
                     {
                       update_cell (&spreadsheet[v][u]);
                       sum += spreadsheet[v][u].number;
                     }
                     else
                     {
                       sprintf (cell->display, "¡CIRCREF!");
                       return;
                     }
                  }
                cell->number = sum;
              }
            }
            else
            {
              cell->number = arg1;
            }
            break;
    case '+': cell->number = arg1 + arg2; break;
    case '-': cell->number = arg1 - arg2; break;
    case '*': cell->number = arg1 * arg2; break;
    case '/': cell->number = arg1 / arg2; break;
    default: sprintf(cell->display, "!ERROR"); return;
  }
  sprintf (cell->display, "%.2f", cell->number);
}

static int spreadsheet_col = 0;
static int spreadsheet_row = 0;

static void spreadsheet_keynav (CtxEvent *event, void *data, void *data2)
{
  if (!strcmp (event->string, "up"))
  {
    spreadsheet_row --;
    if (spreadsheet_row < 0) spreadsheet_row = 0;
  }
  else if (!strcmp (event->string, "down"))
  {
    spreadsheet_row ++;
  }
  else if (!strcmp (event->string, "left"))
  {
    spreadsheet_col --;
    if (spreadsheet_col < 0) spreadsheet_col = 0;
  }
  else if (!strcmp (event->string, "right"))
  {
    spreadsheet_col ++;
  }

  event->stop_propagate=1;
  ctx_set_dirty (event->ctx, 1);
}

static void dirty_cell (Cell *cell)
{
  cell->dirty = 1;
  for (int i = 0; i < cell->dependencies_count; i++)
  {
    dirty_cell (cell->dependencies[i]);
  }
}

static void cell_mark_dep (Cell *cell, Cell *dependency)
{
  if (cell != dependency)
  cell->dependencies[cell->dependencies_count++]=dependency;
}

static void cell_unmark_dep (Cell *cell, Cell *dependency)
{
  if (cell != dependency)
  for (int i = 0; i < cell->dependencies_count; i++)
  {
    if (cell->dependencies[i] == dependency)
    {
       cell->dependencies[i] = 
         cell->dependencies[cell->dependencies_count-1];
       cell->dependencies_count--;
       return;
    }
  }
  fprintf (stderr, "tried unmarking nonexisting dep\n");
}

static void formula_update_deps (Cell *cell, const char *formula, int unmark)
{
  for (int i = 0; formula[i]; i++)
  {
    if (formula[i] >= 'A' && formula[i] <= 'Z')
    {
      int col = formula[i] - 'A';
      int row = 0 ;
      if (formula[i+1] && formula[i+1]>='0' && formula[i+1]<='9')
      {
        int n = 0;
        if (formula[i+2] && formula[i+2]>='0' && formula[i+2]<='9')
        {
          row = (formula[i+1] - '0') * 10 +
                (formula[i+2] - '0')
                ;
          n = i + 3;
        }
        else
        {
          row = formula[i+1] - '0';
          n = i + 2;
        }

        if (formula[n]==':')
        {
           int target_row = row;
           int target_col = col;
           n++;

    if (formula[n] >= 'A' && formula[n] <= 'Z')
    {
      target_col = formula[n] - 'A';
      if (formula[n+1] && formula[n+1]>='0' && formula[n+1]<='9')
      {
        if (formula[n+2] && formula[n+2]>='0' && formula[n+2]<='9')
        {
          target_row = (formula[n+1] - '0') * 10 +
                (formula[n+2] - '0')
                ;
        }
        else
        {
          target_row = formula[n+1] - '0';
        }
      }
    }
           for (int v = row; v <= target_row; v++)
           for (int u = col; u <= target_col; u++)
           {

        if (u >= 0 && u <= 26 && v >= 0 && v <= 99)
        {
          if (unmark)
            cell_unmark_dep (&spreadsheet[v][u], cell);
          else
            cell_mark_dep (&spreadsheet[v][u], cell);
        }

           }
        }
        else
        {

        if (col >= 0 && col <= 26 && row >= 0 && row <= 99)
        {
          if (unmark)
            cell_unmark_dep (&spreadsheet[row][col], cell);
          else
            cell_mark_dep (&spreadsheet[row][col], cell);
        }

        }

      }
    }
  }
}

static void cell_set_value (Cell *cell, const char *value)
{
  formula_update_deps (cell, cell->value, 1);
  formula_update_deps (cell, value, 0);
  strcpy (cell->value, value);
  dirty_cell (cell);
}

static void commit_cell (ITK *itk, void *data)
{
  //CtxControl *control = itk_focused_control (itk);
  Cell *cell = data;
  cell_set_value (cell, itk->entry_copy);
}

static void card_7GUI7 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  float row_height = em * 1.2;
  static int inited = 0;
  if (!inited)
  {
    int row = 0;
    for (int i = 0; i < SPREADSHEET_COLS; i++)
      col_width[i] = em * 4;
    inited = 1;

    cell_set_value (&spreadsheet[row][0], "5");
    cell_set_value (&spreadsheet[row][1], "3");
    cell_set_value (&spreadsheet[row][2], "4");
    cell_set_value (&spreadsheet[row][3], "6");
    cell_set_value (&spreadsheet[row][4], "7");
    cell_set_value (&spreadsheet[row][5], "8");
    cell_set_value (&spreadsheet[row][6], "9");
    cell_set_value (&spreadsheet[row][7], "1");
    cell_set_value (&spreadsheet[row][8], "2");
    cell_set_value (&spreadsheet[row][9], "=sum(A0:I0)");

    row++;
    cell_set_value (&spreadsheet[row][0], "6");
    cell_set_value (&spreadsheet[row][1], "7");
    cell_set_value (&spreadsheet[row][2], "2");
    cell_set_value (&spreadsheet[row][3], "1");
    cell_set_value (&spreadsheet[row][4], "9");
    cell_set_value (&spreadsheet[row][5], "5");
    cell_set_value (&spreadsheet[row][6], "3");
    cell_set_value (&spreadsheet[row][7], "4");
    cell_set_value (&spreadsheet[row][8], "8");
    cell_set_value (&spreadsheet[row][9], "=sum(A1:I1)");

    row++;
    cell_set_value (&spreadsheet[row][0], "1");
    cell_set_value (&spreadsheet[row][1], "9");
    cell_set_value (&spreadsheet[row][2], "8");
    cell_set_value (&spreadsheet[row][3], "3");
    cell_set_value (&spreadsheet[row][4], "4");
    cell_set_value (&spreadsheet[row][5], "2");
    cell_set_value (&spreadsheet[row][6], "5");
    cell_set_value (&spreadsheet[row][7], "6");
    cell_set_value (&spreadsheet[row][8], "7");
    cell_set_value (&spreadsheet[row][9], "=sum(A2:I2)");

    row++;
    cell_set_value (&spreadsheet[row][0], "8");
    cell_set_value (&spreadsheet[row][1], "5");
    cell_set_value (&spreadsheet[row][2], "9");
    cell_set_value (&spreadsheet[row][3], "7");
    cell_set_value (&spreadsheet[row][4], "6");
    cell_set_value (&spreadsheet[row][5], "1");
    cell_set_value (&spreadsheet[row][6], "4");
    cell_set_value (&spreadsheet[row][7], "2");
    cell_set_value (&spreadsheet[row][8], "3");
    cell_set_value (&spreadsheet[row][9], "=sum(A3:I3)");


    row++;
    cell_set_value (&spreadsheet[row][0], "4");
    cell_set_value (&spreadsheet[row][1], "2");
    cell_set_value (&spreadsheet[row][2], "6");
    cell_set_value (&spreadsheet[row][3], "8");
    cell_set_value (&spreadsheet[row][4], "5");
    cell_set_value (&spreadsheet[row][5], "3");
    cell_set_value (&spreadsheet[row][6], "7");
    cell_set_value (&spreadsheet[row][7], "9");
    cell_set_value (&spreadsheet[row][8], "1");
    cell_set_value (&spreadsheet[row][9], "=sum(A4:I4)");


    row++;
    cell_set_value (&spreadsheet[row][0], "7");
    cell_set_value (&spreadsheet[row][1], "1");
    cell_set_value (&spreadsheet[row][2], "3");
    cell_set_value (&spreadsheet[row][3], "9");
    cell_set_value (&spreadsheet[row][4], "2");
    cell_set_value (&spreadsheet[row][5], "4");
    cell_set_value (&spreadsheet[row][6], "8");
    cell_set_value (&spreadsheet[row][7], "5");
    cell_set_value (&spreadsheet[row][8], "6");
    cell_set_value (&spreadsheet[row][9], "=sum(A5:I5)");

    row++;
    cell_set_value (&spreadsheet[row][0], "9");
    cell_set_value (&spreadsheet[row][1], "6");
    cell_set_value (&spreadsheet[row][2], "1");
    cell_set_value (&spreadsheet[row][3], "5");
    cell_set_value (&spreadsheet[row][4], "3");
    cell_set_value (&spreadsheet[row][5], "7");
    cell_set_value (&spreadsheet[row][6], "2");
    cell_set_value (&spreadsheet[row][7], "8");
    cell_set_value (&spreadsheet[row][8], "4");
    cell_set_value (&spreadsheet[row][9], "=sum(A6:I6)");


    row++;
    cell_set_value (&spreadsheet[row][0], "2");
    cell_set_value (&spreadsheet[row][1], "8");
    cell_set_value (&spreadsheet[row][2], "7");
    cell_set_value (&spreadsheet[row][3], "4");
    cell_set_value (&spreadsheet[row][4], "1");
    cell_set_value (&spreadsheet[row][5], "9");
    cell_set_value (&spreadsheet[row][6], "6");
    cell_set_value (&spreadsheet[row][7], "3");
    cell_set_value (&spreadsheet[row][8], "5");
    cell_set_value (&spreadsheet[row][9], "=sum(A7:I7)");

    row++;
    cell_set_value (&spreadsheet[row][0], "3");
    cell_set_value (&spreadsheet[row][1], "4");
    cell_set_value (&spreadsheet[row][2], "5");
    cell_set_value (&spreadsheet[row][3], "2");
    cell_set_value (&spreadsheet[row][4], "8");
    cell_set_value (&spreadsheet[row][5], "6");
    cell_set_value (&spreadsheet[row][6], "1");
    cell_set_value (&spreadsheet[row][7], "7");
    cell_set_value (&spreadsheet[row][8], "9");
    cell_set_value (&spreadsheet[row][9], "=sum(A8:I8)");

    row=9;
    cell_set_value (&spreadsheet[row][0], "=sum(A0:A8)");
    cell_set_value (&spreadsheet[row][1], "=sum(B0:B8)");
    cell_set_value (&spreadsheet[row][2], "=sum(C0:C8)");
    cell_set_value (&spreadsheet[row][3], "=sum(D0:D8)");
    cell_set_value (&spreadsheet[row][4], "=sum(E0:E8)");
    cell_set_value (&spreadsheet[row][5], "=sum(F0:F8)");
    cell_set_value (&spreadsheet[row][6], "=sum(G0:G8)");
    cell_set_value (&spreadsheet[row][7], "=sum(H0:H8)");
    cell_set_value (&spreadsheet[row][8], "=sum(I0:I8)");

    row=11;
    cell_set_value (&spreadsheet[row][0], "=sum(A0:C2)");
    cell_set_value (&spreadsheet[row][1], "=sum(D0:F2)");
    cell_set_value (&spreadsheet[row][2], "=sum(G0:I2)");
    row++;
    cell_set_value (&spreadsheet[row][0], "=sum(A3:C5)");
    cell_set_value (&spreadsheet[row][1], "=sum(D3:F5)");
    cell_set_value (&spreadsheet[row][2], "=sum(G3:I5)");
    row++;
    cell_set_value (&spreadsheet[row][0], "=sum(A6:C8)");
    cell_set_value (&spreadsheet[row][1], "=sum(D6:F8)");
    cell_set_value (&spreadsheet[row][2], "=sum(G6:I8)");


  }
  itk_panel_start (itk, "7gui7 - spreadsheet", 300, 0, ctx_width (ctx) - 300, ctx_height (ctx));
  float saved_x = itk->x;
  float saved_x0 = itk->x0;
  float saved_y = itk->y;

  if (!itk->entry_copy)
  {
    ctx_add_key_binding (ctx, "left", NULL, "foo", spreadsheet_keynav, NULL);
    ctx_add_key_binding (ctx, "right", NULL, "foo", spreadsheet_keynav, NULL);
    ctx_add_key_binding (ctx, "up", NULL, "foo", spreadsheet_keynav, NULL);
    ctx_add_key_binding (ctx, "down", NULL, "foo", spreadsheet_keynav, NULL);
  }

  /* draw white background */
  ctx_gray (ctx, 1.0);
  ctx_rectangle (ctx, saved_x, saved_y, itk->width, ctx_height (ctx));
  ctx_fill (ctx);

  float row_header_width = em * 1.5;

  /* draw gray gutters for col/row headers */
  ctx_rectangle (ctx, saved_x, saved_y, itk->width, row_height);
  ctx_gray (ctx, 0.7);
  ctx_fill (ctx);
  ctx_rectangle (ctx, saved_x, saved_y, row_header_width, ctx_height (ctx));
  ctx_fill (ctx);

  ctx_font_size (ctx, em);
  ctx_gray (ctx, 0.0);

  // ensure current cell is within viewport 
  if (spreadsheet_col < spreadsheet_first_col)
    spreadsheet_first_col = spreadsheet_col;
  else
  {
  float x = saved_x + row_header_width;
  int found = 0;
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    if (col == spreadsheet_col)
    {
       if (x + col_width[col] > itk->panel->x + itk->panel->width)
         spreadsheet_first_col++;
       found = 1;
    }
    x += col_width[col];
  }
  if (!found)
    spreadsheet_first_col++;
  }

  if (spreadsheet_row < spreadsheet_first_row)
    spreadsheet_first_row = spreadsheet_row;
  else
  {
  float y = saved_y + em;
  int found = 0;
  for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
  {
    if (row == spreadsheet_row)
    {
       if (y + row_height > itk->panel->y + itk->panel->height)
         spreadsheet_first_row++;
       found = 1;
    }
    y += row_height;
  }
  if (!found)
    spreadsheet_first_row++;
  }

  /* draw col labels */
  float x = saved_x + row_header_width;
  ctx_save (ctx);
  ctx_gray (ctx, 0.1);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    float y = saved_y + em;
    char label[4]="E";

    ctx_move_to (ctx, x + col_width[col]/2, y);
    label[0]=col+'A';
    ctx_text (ctx, label);
    x += col_width[col];
  }
  ctx_restore (ctx);

  /* draw vertical lines */
  x = saved_x + row_header_width;
  ctx_gray (ctx, 0.5);
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    float y = saved_y + em;
    ctx_move_to (ctx, floor(x)+0.5, y - row_height);
    ctx_rel_line_to (ctx, 0, itk->panel->height);
    ctx_line_width (ctx, 1.0);
    ctx_stroke (ctx);
    x += col_width[col];
  }

  /* row header labels */
  ctx_save (ctx);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_RIGHT);
  x = saved_x + row_header_width - em * 0.1;
  ctx_gray (ctx, 0.1);
  {
    float y = saved_y + row_height + em;
    for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
    {
      char label[10];
      sprintf (label, "%i", row);
      ctx_move_to (ctx, x, y);
      ctx_text (ctx, label);
      y += row_height;
    }
  }
  ctx_restore (ctx);
  ctx_gray (ctx, 0.5);
  /* draw horizontal lines */
  {
    float y = saved_y + row_height;
    for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
    {
      ctx_move_to (ctx, x, floor(y)+0.5);
      ctx_rel_line_to (ctx, itk->width, 0);
      ctx_line_width (ctx, 1.0);
      ctx_stroke (ctx);
      y += row_height;
    }
  }

  ctx_gray (ctx, 0.0);
  x = saved_x + row_header_width;
  for (int col = spreadsheet_first_col; x < itk->panel->x + itk->panel->width; col++)
  {
    float y = saved_y + row_height + em;
    for (int row = spreadsheet_first_row; y < itk->panel->y + itk->panel->height; row++)
    {
      Cell *cell = &spreadsheet[row][col];
      int drawn = 0;

      if (spreadsheet_col == col && spreadsheet_row == row)
      {
        drawn = 1;
        itk->x = x;
        itk->y = y - em;
        itk->width = col_width[col];
        itk_entry (itk, "", "", cell->value, sizeof(cell->value)-1, commit_cell, cell);
        itk->focus_no = itk->control_no-1;
        if (itk->focus_label){
          free (itk->focus_label);
          itk->focus_label = NULL;
        }

        /* draw cursor around selected cell */
        ctx_gray (ctx, 0);
        ctx_rectangle (ctx, x-em*0.1, y - em-em*0.1, col_width[col]+em*0.2, row_height+em*0.2);
        ctx_line_width (ctx, em*0.2);
        ctx_stroke (ctx);
        ctx_gray (ctx, 1);
        ctx_rectangle (ctx, x-em*0.1, y - em-em*0.1, col_width[col]+em*0.2, row_height+em*0.2);
        ctx_line_width (ctx, em*0.1);
        ctx_stroke (ctx);
        ctx_gray (ctx, 0);
      }
      else
      {

      }
      if (!drawn)
      {
        update_cell (cell);

        if (cell->display[0])
        {
          switch (cell->type)
          {
            case CELL_TYPE_NUMBER:
            case CELL_TYPE_FORMULA:
              ctx_save (ctx);
              ctx_text_align (ctx, CTX_TEXT_ALIGN_RIGHT);
              ctx_move_to (ctx, x + col_width[col] - em * 0.1, y);
              if (cell->display[0]=='!')
              {
                ctx_save (ctx);
                ctx_rgb (ctx, 1,0,0);
                ctx_text (ctx, cell->display);
                ctx_restore (ctx);
              }
              else
              {
                ctx_text (ctx, cell->display);
              }
              ctx_restore (ctx);
              break;
            default:
              ctx_move_to (ctx, x + em * 0.1, y);
              ctx_text (ctx, cell->display);
              break;
          }
        }
      }

      y += row_height;
    }
    x += col_width[col];
  }

  ctx_rectangle (ctx, saved_x + row_header_width,
                 saved_y + itk->panel->height - row_height*2,
                 (itk->panel->x+itk->panel->width - saved_x) - row_header_width - em,
                 row_height);
  ctx_rgb (ctx, 0,1,0);
  ctx_fill (ctx);

  ctx_rectangle (ctx, itk->panel->x + itk->panel->width - 
                 em,
                 saved_y + row_height,
                 em, itk->panel->height - row_height * 3);
  ctx_rgb (ctx, 0,1,0);
  ctx_fill (ctx);

  itk->x  = saved_x;
  itk->x0 = saved_x0;
  itk->y  = saved_y;

  itk_panel_end (itk);
}


Test tests[]=
{
  {"7gui 7",     card_7GUI7},
  {"gradients",  card_gradients},
  {"dots",       card_dots},
  {"drag",       card_drag},
  {"sliders",    card_sliders},
  {"7gui 1",     card_7GUI1},
  {"7gui 2",     card_7GUI2},
  {"7gui 3",     card_7GUI3},
  {"7gui 4",     card_7GUI4},
  {"7gui 5",     card_7GUI5},
  {"7gui 6",     card_7GUI6},
  {"clock1",     card_clock1},
  {"clock2",     card_clock2},
  {"fill rule",  card_fill_rule},
  {"curve to",   card_curve_to},
};
int n_tests = sizeof(tests)/sizeof(tests[0]);
