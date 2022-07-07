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

extern int _ctx_max_threads;
extern int _ctx_enable_hash_cache;

static void demo_next (CtxEvent *event, void *data1, void *data2)
{
        test_no ++;
        ctx_queue_draw (event->ctx);
}

static void demo_prev (CtxEvent *event, void *data1, void *data2)
{
        test_no --;
        ctx_queue_draw (event->ctx);
}

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new (-1, -1, NULL);

  ITK *itk = itk_new (ctx);

  if (argv[1]) test_no = atoi(argv[1]);

  uint8_t abc = 11;
  int   baz = 1;
  int chosen = 1;
  int enable_keybindings = 1;
  char input[256]="fnord";
  //ctx_queue_draw (ctx);
  while (!ctx_has_quit (ctx))
  {
    int width  = ctx_width (ctx);
    int height = ctx_height (ctx);
#if 0
    ctx_save (ctx);
    ctx_rectangle (ctx, 0, 0, width, height);
    ctx_gray (ctx, 0);
    ctx_fill (ctx);
#endif

//  itk->dirty=1;
    if (ctx_need_redraw (ctx))
    {
      itk_reset (itk);

      if (enable_keybindings)
        itk_key_bindings (itk);

      tests[test_no].fun (itk, frame_no++);
#if 1
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
        itk_label (itk, "label");
      }
      itk_newline (itk);

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
      
      if (
      itk_entry_str_len (itk, "Foo", "text entry", (char*)&input, sizeof(input)-1))
      {
         fprintf (stderr, "text entry commited: \"%s\"\n", input);
      }

      chosen = itk_choice (itk, "power", chosen);
      itk_choice_add (itk, 0,    "on");
      itk_choice_add (itk, 1,    "off");
      itk_choice_add (itk, 2,    "good");
      itk_choice_add (itk, 2025, "green");
      itk_choice_add (itk, 2030, "electric");
      itk_choice_add (itk, 2040, "novel");

      baz = itk_toggle (itk, "baz ", baz);
      }

#if 0
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
#endif

      itk_ctx_settings (itk);
      itk_itk_settings (itk);

      itk_panel_end (itk);
#endif

      itk_done (itk);

      ctx_add_key_binding (ctx, "page-up", NULL, "foo", demo_prev, NULL);
      ctx_add_key_binding (ctx, "page-down", NULL, "foo", demo_next, NULL);
      ctx_add_key_binding (ctx, "control-q", NULL, "foo", itk_key_quit, NULL);
      ctx_add_key_binding (ctx, "q", NULL, "foo", itk_key_quit, NULL);

      ctx_end_frame (ctx);
    }
    ctx_handle_events (ctx);
  }
  ctx_destroy (ctx);
  return 0;
}

static void card_gradients (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);
  int width  = ctx_width (ctx);
  int height = ctx_height(ctx);
  frame_no %= (int)(ctx_width (ctx) /2);

  ctx_linear_gradient (ctx, 0.0, 0.0,  0.0, height);
  ctx_gradient_add_stop (ctx, 0, 1, 1, 1, 1);
  ctx_gradient_add_stop (ctx, 1, 0, 0, 0, 1);
  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_fill (ctx);

  ctx_radial_gradient (ctx, width * 0.4 + frame_no, height * 0.4, height * 0.1,
                            width * 0.4 + frame_no, height * 0.4, height * 0.4);
  ctx_gradient_add_stop (ctx, 0, 1, 1, 1, 1);
  ctx_gradient_add_stop (ctx, 1, 0, 0, 0, 1);
  ctx_arc (ctx, width/2 + frame_no, height/2, height * 0.3, 0, 2.0 * CTX_PI, 0);
  ctx_fill (ctx);
  ctx_queue_draw (ctx);
}

static void card_dots (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);
  static int   dot_count = 500;
  static float twist = 2.9645;
  static float dot_scale = 42.0;

      /* clear */
      ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
      ctx_rgba8 (ctx, 0,0,0,255);
      ctx_fill (ctx);

      ctx_rgba (ctx, 1, 1, 1, 0.5);
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

      itk_panel_start (itk, "spiraling dots", ctx_width(ctx)*3/4,0,ctx_width(ctx)/4, ctx_height(ctx)/2);
      itk_slider_int (itk, "count",          &dot_count, 1,  10000, 10);
      itk_slider_float (itk, "radius",    &dot_scale, 2.0, 400.0, 4.5);
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
  ctx_gray_stroke (ctx, 1.0);
  ctx_gray (ctx, 1.0);
  ctx_line_width (ctx, height * 0.025);
  ctx_move_to (ctx, x0, y0);
  ctx_line_to (ctx, x0 + width, y0);
  ctx_stroke (ctx);
  ctx_arc (ctx, x0 + width * pos, y0, height * 0.05, 0.0, CTX_PI*2, 0);
  ctx_fill (ctx);
}

static void card_sliders (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);
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
  ctx_queue_draw (ctx);
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

  ctx_rgba_stroke (ctx, 1,1,1,0.8);
  ctx_rgba (ctx, 1,1,1,0.8);

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
  Ctx *ctx = itk_ctx (itk);
  uint64_t ms64 = ctx_ticks() / 1000;
  int width = ctx_width (ctx);
  int height = ctx_height (ctx);
  _analog_clock (ctx, ms64, width/2, height/2, height/2, 1);
  ctx_queue_draw (ctx);
}

static void card_clock2 (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);
  uint64_t ms64 = ctx_ticks()/ 1000;
  int width = ctx_width (ctx);
  int height = ctx_height (ctx);
  _analog_clock (ctx, ms64, width/2, height/2, height/2, 0);
  ctx_queue_draw (ctx);
}

static void card_fill_rule (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);

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
  ctx_rgba_stroke (ctx, 1, 1, 1, 1); ctx_stroke (ctx);

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
  ctx_rgba_stroke (ctx, 1, 1, 1, 1); ctx_stroke (ctx);
  ctx_restore (ctx);
}

static void card_curve_to (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);
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
  ctx_queue_draw (event->ctx);
}

static void card_drag (ITK *itk, int frame_no)
{
  static DragObject objects[8];
  static int first_run = 1;
  if (first_run)
  for (int i = 0; i <  8; i++)
  {
    objects[i].x = (i+1) * 10;
    objects[i].y = (i+1) * 10;
    objects[i].width = 20;
    objects[i].height = 20;
    objects[i].red = 0.5;
    objects[i].red = 0.1;
    objects[i].red = 0.9;
    objects[i].alpha = 0.6;
    first_run = 0;
  }

  Ctx *ctx = itk_ctx (itk);
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  ctx_save (ctx);
  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_gray (ctx, 0);
  ctx_fill (ctx);
  frame_no %= 400;

  ctx_rotate (ctx, 0.2);
  ctx_scale (ctx, width/100, height/100);
  ctx_scale (ctx, 0.9, 0.9);

  for (int i = 0; i <  8; i++)
  {
    switch (i)
    {
      case 0:
      case 1:
      case 2:
      case 3:
         ctx_rectangle (ctx, objects[i].x, objects[i].y, objects[i].width, objects[i].height);
         break;
      case 4:
        ctx_move_to (ctx, objects[i].x, objects[i].y);
        ctx_rel_line_to (ctx, 0, objects[i].height);
        ctx_rel_line_to (ctx, objects[i].width, 0);
        break;
      case 5:
        ctx_move_to (ctx, objects[i].x, objects[i].y);
        ctx_line_to (ctx, objects[i].x +  objects[i].width, objects[i].y);
        ctx_line_to (ctx, objects[i].x +  objects[i].width, objects[i].y+objects[i].height);
        break;
      case 6:
      case 7:
        ctx_arc (ctx, objects[i].x+objects[i].width/2, objects[i].y+objects[i].width/2,
                      objects[i].width/2,
                      0.0, CTX_PI *  2.0, 0);
        break;
    }
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

#include <dirent.h>
#include <sys/stat.h>

static void card_files (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);
  float em = itk_em (itk);
  //float row_height = em * 1.2;
  itk_panel_start (itk, "files", ctx_width(ctx)*0.2, 0, ctx_width (ctx) * 0.8, ctx_height (ctx));
  struct dirent **namelist;
  char *path = "./";///home/pippin/src/ctx";
#define PATH_SEP "/"
  int n = scandir (path, &namelist, NULL, NULL);//alphasort);
  if (!n)
  {
    itk_labelf (itk, "no files\n");
  }

  for (int i = 0; i < n; i++)
  {
  //if (strcmp (namelist[i]->d_name, "."))
    if (namelist[i]->d_name[0] != '.')
    {
      struct stat stat_buf;
      char *newpath = malloc (strlen(path)+strlen(namelist[i]->d_name) + 2);
      if (!strcmp (path, PATH_SEP))
        sprintf (newpath, "%s%s", PATH_SEP, namelist[i]->d_name);
      else
        sprintf (newpath, "%s%s%s", path, PATH_SEP, namelist[i]->d_name);
      lstat (newpath, &stat_buf);
      free (newpath);
      itk_add_control (itk, UI_LABEL, "foo", itk_x (itk), itk_y (itk), itk_wrap_width (itk), em * itk_rel_ver_advance (itk));
      itk_labelf (itk, "%s\n", namelist[i]->d_name);
      itk_sameline (itk);
      itk_set_x (itk, itk_edge_left (itk) + itk_em (itk) * 10);
      itk_labelf (itk, "%i", stat_buf.st_size);
    }
  }


  itk_panel_end (itk);
}

static int textures_inited = 0;
#define TEXTURE_W 128
#define TEXTURE_H 128
static uint8_t texture_rgba[TEXTURE_W *  TEXTURE_H * 4];
static uint8_t texture_rgb[TEXTURE_W *  TEXTURE_H * 3];
static uint8_t texture_gray[TEXTURE_W *  TEXTURE_H * 1];
static uint8_t texture_graya[TEXTURE_W *  TEXTURE_H * 2];

static void init_textures  (void)
{
  if (textures_inited)
    return;

  int i = 0;
  for (int v = 0; v <  TEXTURE_H; v++)
  for (int u = 0; u <  TEXTURE_W; u++, i++)
  {
     uint8_t r = u * 255 / (TEXTURE_W-1);
     uint8_t g = v * 255 / (TEXTURE_W-1);
     uint8_t b = (u+v) * 255 / (TEXTURE_W + TEXTURE_H - 1 -1);
     uint8_t a = 255;
     texture_gray[i+0] = g;

     texture_graya[i*2+0] = g;
     texture_graya[i*2+1] = a;

     texture_rgb[i*3+0] = r;
     texture_rgb[i*3+1] = g;
     texture_rgb[i*3+2] = b;

     texture_rgba[i*4+0] = r;
     texture_rgba[i*4+1] = g;
     texture_rgba[i*4+2] = b;
     texture_rgba[i*4+3] = a;
  }
  textures_inited = 1;
}

static void card_textures (ITK *itk, int frame_no)
{
  Ctx *ctx = itk_ctx (itk);
  init_textures  ();

  itk_panel_start (itk, "textures", ctx_width(ctx)*0.2, 0, ctx_width (ctx) * 0.8, ctx_height (ctx));
  ctx_save(ctx);
  ctx_translate (ctx, 400, 50);
    char eid[65]="";
    ctx_define_texture (ctx, NULL,
                      TEXTURE_W, TEXTURE_H, 0,
                      CTX_FORMAT_RGBA8,
                      &texture_rgba[0], eid);
    ctx_rectangle (ctx, 0, 0, TEXTURE_W, TEXTURE_H);
    ctx_fill (ctx);
    ctx_translate (ctx, 0, TEXTURE_H * 1.2);
    ctx_define_texture (ctx, NULL,
                      TEXTURE_W, TEXTURE_H, 0,
                      CTX_FORMAT_RGB8,
                      &texture_rgb[0], eid);
    ctx_rectangle (ctx, 0, 0, TEXTURE_W, TEXTURE_H);
    ctx_fill (ctx);
#if 1
    ctx_translate (ctx, 0, TEXTURE_H * 1.2);
    ctx_define_texture (ctx, NULL,
                      TEXTURE_W, TEXTURE_H, 0,
                      CTX_FORMAT_GRAY8,
                      &texture_gray[0], eid);
    ctx_rectangle (ctx, 0, 0, TEXTURE_W, TEXTURE_H);
    ctx_fill (ctx);
#endif
#if 1
    ctx_translate (ctx, 0, TEXTURE_H * 1.2);
    ctx_define_texture (ctx, NULL,
                      TEXTURE_W, TEXTURE_H, 0,
                      CTX_FORMAT_GRAYA8,
                      &texture_graya[0], eid);
    ctx_rectangle (ctx, 0, 0, TEXTURE_W, TEXTURE_H);
    ctx_fill (ctx);
#if 0

    ctx_translate (ctx, 0, TEXTURE_H * 1.2);
    //ctx_rgb (ctx, 1 ,0,0);
    ctx_define_texture (ctx, NULL,
                      TEXTURE_W, TEXTURE_H, 0,
                      12,
                      &texture_gray[0], eid);
    ctx_rectangle (ctx, 0, 0, TEXTURE_W, TEXTURE_H);
    ctx_fill (ctx);
#endif
    ctx_translate (ctx, 0, TEXTURE_H * 1.2);

    ctx_image_smoothing (ctx, 0);
    ctx_draw_image (ctx, "file:///home/pippin/src/ctx/tig.png",
                         0.0, 0.0, 6120.0, 6120.0);
#endif

    ctx_restore (ctx);
    itk_panel_end (itk);
}


static void ctx_spacer (Ctx *ctx, const char *name, float dx, float dy)
{
  ctx_deferred_rel_line_to (ctx, name, dx, dy);
}

static void distribute_space_hor_cb (Ctx *ctx, void *userdata, const char *name, int count, float *x, float *y, float *w, float *h)
{
  float *fptr = (float*)userdata;
  *x += fptr[0] / count;
}

static void ctx_spacer_fill_horizontal (Ctx *ctx, const char *name, float space)
{
  ctx_resolve (ctx, name, distribute_space_hor_cb, &space);
}

static void set_y_cb (Ctx *ctx, void *userdata, const char *name, int count, float *x, float *y, float *w, float *h)
{
  float *fptr = (float*)userdata;
  *y = fptr[0];
}

static void ctx_spacer_set_y (Ctx *ctx, const char *name, float space)
{
  ctx_resolve (ctx, name, set_y_cb, &space);
}

static void set_x_cb (Ctx *ctx, void *userdata, const char *name, int count, float *x, float *y, float *w, float *h)
{
  float *fptr = (float*)userdata;
  *x = fptr[0];
}

static void ctx_spacer_set_x (Ctx *ctx, const char *name, float space)
{
  ctx_resolve (ctx, name, set_x_cb, &space);
}

static void card_deferred (ITK *itk, int frame_no)
{

  float ascent = 0.8;
  float descent = 0.2;
  float line_gap = 0.1;

  Ctx *ctx = itk_ctx (itk);
  float em = itk_em (itk);
  int height = ctx_height (ctx);
  int width = ctx_width (ctx);
  frame_no = frame_no % 400;
  /* a custom shape that could be wrapped in a function */

  float x0 = height * 0.4;
  float y0 = height * 0.2;
  float computed_line_height = 0.0f;

  for (int i = 0; i < 3; i++)
  {
    computed_line_height = 0.0f;
    computed_line_height = ctx_maxf (computed_line_height, em);
    ctx_font_size (ctx, em);
  ctx_move_to (ctx, x0, y0);
  ctx_rel_line_to (ctx, width, 0);
  ctx_rgb (ctx, 1,0,0);
  ctx_rel_move_to (ctx, -width, 0);
  ctx_stroke (ctx);
  ctx_rgb (ctx, 1,1,1);

  
  ctx_save (ctx);
  ctx_move_to (ctx, x0, y0);
  ctx_spacer (ctx, "line", 0.0f, em);

  ctx_rel_move_to (ctx, 0, em * -0.4);

  ctx_text (ctx, "foo");
  ctx_spacer (ctx, "space", em, 0.0f);
  ctx_text (ctx, "bar");
  ctx_spacer (ctx, "space", em, 0.0f);
  ctx_text (ctx, "baz");
  ctx_spacer (ctx, "space", em, 0.0f);
  if (i != 1)
  {
  ctx_font_size (ctx, em * 2.4);
  computed_line_height = ctx_maxf (computed_line_height, em * 2.4);
  }

  ctx_text (ctx, "qux");
  ctx_spacer (ctx, "space", em, 0.0f);
  ctx_text (ctx, "qux");
  ctx_spacer (ctx, "space", em, 0.0f);
  ctx_text (ctx, "qux");
  ctx_spacer_fill_horizontal (ctx, "space", -em * 4);//0.0f);

  ctx_spacer_set_y (ctx, "line", computed_line_height * ascent);

  y0 += computed_line_height * (1.0f+line_gap);


  }













  ctx_move_to (ctx, x0, y0);
  ctx_rel_line_to (ctx, width, 0);
  ctx_stroke (ctx);

  //ctx_deferred_rectangle (ctx, "box", 500,400, 100, 100);
  //ctx_fill (ctx);

  ctx_restore (ctx);

  ctx_queue_draw (ctx);
}

Test tests[]=
{
  {"deferred",   card_deferred},
  {"sliders",    card_sliders},
  {"gradients",  card_gradients},
  {"textures",   card_textures},
  {"drag",       card_drag},
  {"clock1",     card_clock1},
  {"files",      card_files},
  {"dots",       card_dots},
//{"clock2",     card_clock2},
//{"fill rule",  card_fill_rule},
//{"curve to",   card_curve_to},
};
int n_tests = sizeof(tests)/sizeof(tests[0]);
