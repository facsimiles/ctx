#ifdef EMSCRIPTEN
#include "emscripten.h"

//#define CTX_DITHER 1
#define CTX_HASH_CACHE 1

#endif
#include <stdint.h>
#define _CTX_INTERNAL_FONT_
#include "ctx-font-regular.h"

//#define CTX_IMPLEMENTATION 1
//#define ITK_IMPLEMENTATION 1
#include "itk.h"

float render_time = 0.0;
float render_fps  = 0.0;

int        scene_frames = 0;
float      scene_elapsed_time = 0.0;
static uint64_t prev_ticks = 0;
  static int   dot_count = 200;
  static float twist = 2.9645;
  static float dot_scale = 42.0;
  static int shape = 0;
static int scene_no = 0;

typedef void (*SceneFun)(ITK *itk, int frame_no, float time_delta);
typedef struct Scene{
  const char *title;
  SceneFun     fun;
  float        duration; // in seconds
}Scene;

static void clear (Ctx *ctx)
{
  ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
  ctx_rgba8 (ctx, 0,0,0,255);
  ctx_fill (ctx);
}


static void scene_spirals (ITK *itk, int frame_no, float time_delta)
{
  Ctx *ctx = itk->ctx;
  clear (ctx);

  float alpha = 0.0;
  if (scene_elapsed_time > 2.0)
  {
     alpha = 1.0-(4.0 - scene_elapsed_time)/2.0;
     if (alpha > 1.0) alpha = 1.0;

     float width = ctx_width (ctx);
     float height = ctx_width (ctx);

     ctx_translate (ctx, width * 0.5, height * alpha * 0.2);
     ctx_scale (ctx, 1.0 + alpha, 1.0 + alpha);
     ctx_apply_transform (ctx, 1, 0, 0,
                               0, 1, 0,
                               0, 4.5/height*alpha, 1+alpha*0.4);
     ctx_translate (ctx, -width * 0.5, 0);
  }

  ctx_rgba (ctx, 1, 1, 1, 0.5 + alpha/2);
  for (int i = 0; i < dot_count; i ++)
  {
    float x = ctx_width (ctx)/ 2;
    float y = ctx_height (ctx) / 2;
    float radius = ctx_height (ctx) / dot_scale;
    float dist = i * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
    float twisted = (i * twist);
    x += cos (twisted) * dist;
    y += sin (twisted) * dist;
    if (shape == 0)
    {
      ctx_arc (ctx, x, y, radius, 0, CTX_PI * 2.1, 0);
    }
    else
    {
      ctx_save (ctx);
      ctx_translate (ctx, x, y);
      ctx_rotate (ctx, twisted);
      ctx_rectangle (ctx, -radius, -radius, radius*2, radius*2);
      ctx_restore (ctx);
    }
    ctx_fill (ctx);
  }
    twist += 0.00001;
    ctx_queue_draw (itk->ctx);

#if 0
  itk_panel_start (itk, "spiraling things", ctx_width(ctx)*3/4,0,ctx_width(ctx)/4, ctx_height(ctx)/3);
  //itk_slider_float (itk, "rendertime", &render_time, 0.0, 1000.0, 0.1);
  itk_slider_float (itk, "fps", &render_fps, 0.0, 1000.0, 0.1);
  itk_slider_int (itk, "count",          &dot_count, 1,  50000, 10);
  itk_slider_float (itk, "radius",    &dot_scale, 2.0, 400.0, 4.5);
  itk_slider_float (itk, "twist amount", &twist, -3.14152, 3.14152, 0.0005);
  itk_choice (itk, "shape", &shape, NULL, NULL);
  itk_choice_add (itk, 0, "circle");
  itk_choice_add (itk, 1, "square");

#if 1
      if (itk_button (itk, "+0.00001"))
      {
        twist += 0.00001;
      }
      if (itk_button (itk, "-0.00001"))
      {
        twist -= 0.00001;
      }
#endif

  itk_ctx_settings (itk);
  itk_panel_end (itk);
#endif

  twist += time_delta * 0.001;
}


static void scene_logo (ITK *itk, int frame_no, float time_delta)
{
  Ctx *ctx = itk->ctx;

  static int dir = 1;
  static float dim = 0.0;

  clear (ctx);

  if (dir > 0)
  {
     dim += time_delta * ctx_width (ctx) * 0.1;
     if (dim > ctx_width (ctx) * 0.5) dir *= -1;
  }
  else
  {
     dim -= time_delta * ctx_width (ctx) * 0.1;
     if (dim < ctx_width (ctx) * 0.1) dir *= -1;
  }
  

  ctx_logo (ctx, ctx_width(ctx)/2, ctx_height(ctx)*0.4, 
                  
            ctx_height (ctx) * 0.6 +
            ctx_height (ctx) * 0.3 * sin (scene_elapsed_time)
                  );

  if (scene_elapsed_time > 1.0)
  {
     float alpha = 1.0-(3.0 - scene_elapsed_time)/2.0;
     if (alpha > 1.0) alpha = 1.0;
     ctx_font_size (ctx, ctx_height (ctx) * 0.1);
     ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
     ctx_move_to (ctx, ctx_width (ctx) *0.5, ctx_height (ctx) *0.9);
     ctx_rgba (ctx, 1,1,1, alpha);
     ctx_text (ctx, "vector graphics");
  }
}

static void scene_circles (ITK *itk, int frame_no, float time_delta)
{
  Ctx *ctx = itk->ctx;
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  float alpha = 0.0;
  float beta = 0.0;
  if (scene_elapsed_time > 2.0)
  {
     alpha = (scene_elapsed_time-2.0)/2.0;
     alpha = ctx_minf (alpha, 1.0);
     alpha = ctx_maxf (alpha, 0.0);
  }
  beta = scene_elapsed_time/6.0;
  beta = ctx_minf (beta, 1.0);
  int count = beta * 8 + 2;

  clear (ctx);

  ctx_rgb (ctx, 1,1,1);

  alpha = alpha * 2;
  if (alpha > 1.0) alpha = (2 - alpha);
  else alpha = alpha / 2;

  for (int i = 0; i < count; i ++)
  {
    ctx_translate (ctx, sin (scene_elapsed_time) * height * 0.2, 0);
    ctx_arc (ctx, width * 0.5, height * 0.5, height / 20 * i,  0.0, M_PI*2, 0);
    ctx_translate (ctx, -2*sin (scene_elapsed_time) * height * 0.2, 0);
    ctx_arc (ctx, width * 0.5, height * 0.5, height / 20 * i,  0.0, M_PI*2, 0);
    ctx_translate (ctx, sin (scene_elapsed_time) * height * 0.2, 0);
  }
  ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
  ctx_fill (ctx);
}

static void scene_text (ITK *itk, int frame_no, float time_delta, float font_scale)
{
  Ctx *ctx = itk->ctx;
  char *words[]={"ipsum", "dolor", "sic", "amet", "foo", "fnord", "umbaba", "takete", "metameric", "unix"};
  int n_words = sizeof(words)/sizeof(words[0]); 
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  float alpha = 0.0;
  float beta = 0.0;
  static int last_lines = 0;

  static float scroll = 0.0;

  if (scene_elapsed_time > 2.0)
  {
     alpha = (scene_elapsed_time-2.0)/2.0;
     alpha = ctx_minf (alpha, 1.0);
     alpha = ctx_maxf (alpha, 0.0);
  }
  beta = scene_elapsed_time/6.0;
  beta = ctx_minf (beta, 1.0);

  clear (ctx);

  ctx_rgb (ctx, 1,1,1);

  alpha = alpha * 2;
  if (alpha > 1.0) alpha = (2 - alpha);
  else alpha = alpha / 2;

  float font_size = height * font_scale;

  if (font_size <= 4.0)
          font_size = 3.0;


  float line_height = 1.2;



  if (font_scale <  0.03)
          frame_no += 300;


  int count = frame_no;//beta * 200;
  ctx_font_size (ctx, font_size);

  float space_width = ctx_text_width (ctx, " ");
  int lines = 0;

  if (font_size <= 3){
          font_size = 3.0;
          line_height = 1.0;
          space_width = 2;
  }

  float y0 = font_size * line_height;
  float x0 = space_width;
  float y = y0;
  float x = x0;

  y = (int) y;

  ctx_save (ctx);
  //ctx_translate (ctx, 0.0, -scroll);

  srandom (0);

  int iscroll = scroll;

  for (int i = 0; i < count; i ++)
  {
    const char *word = words[random()%n_words];
    float word_width = ctx_text_width (ctx, word);
    if (x + word_width  >= width - font_size * 0.5)
    {
      x = x0;
      y += font_size * line_height;
      y = (int) y;
      lines++;
    }
    if (y - iscroll > 0)
    {
    ctx_move_to (ctx, x, (int)(y - iscroll));
    ctx_text (ctx, word);
    }
    if (random()%33==0)
    {
     ctx_move_to (ctx, x, (int)(y - iscroll));
     ctx_text (ctx, ".");
     y += font_size * line_height;
      y = (int) y;
     x = x0; word_width = space_width;
    }
    x+= word_width + space_width;
    x = (int)x;
  }
  ctx_restore (ctx);
  last_lines ++;
  if (y > height- font_size) scroll = scroll * 0.95 + 0.05 * (y -height + font_size)  ; else scroll = 0;
}

static void scene_text_50 (ITK *itk, int frame_no, float time_delta)
{
   scene_text (itk, frame_no, time_delta, 0.02);
}

static void scene_text_40 (ITK *itk, int frame_no, float time_delta)
{
   scene_text (itk, frame_no, time_delta, 0.025);
}

static void scene_text_30 (ITK *itk, int frame_no, float time_delta)
{
   scene_text (itk, frame_no, time_delta, 0.033);
}

static void scene_text_20 (ITK *itk, int frame_no, float time_delta)
{
   scene_text (itk, frame_no, time_delta, 0.05);
}

static void scene_text_10 (ITK *itk, int frame_no, float time_delta)
{
   scene_text (itk, frame_no, time_delta, 0.1);
}

static void scene_text_5 (ITK *itk, int frame_no, float time_delta)
{
   scene_text (itk, frame_no, time_delta, 0.2);
}

Scene scenes[]=
{
  {"logo",    scene_logo, 5.0},
  {"circles",    scene_circles, 10.0},
  {"spiral dots", scene_spirals, 10.0},
  {"text",    scene_text_5, 5.0},
  {"text",    scene_text_10, 5.0},
  {"text",    scene_text_20, 5.0},
  {"text",    scene_text_30, 5.0},
  {"text",    scene_text_40, 20.0},
  {"text",    scene_text_50, 20.0},

};

#ifdef EMSCRIPTEN
EMSCRIPTEN_KEEPALIVE
#endif
char ctx_input[8192] = "demo";


#ifdef EMSCRIPTEN
EMSCRIPTEN_KEEPALIVE
#endif
void reset_time(void)
{
  scene_no = 0;
  scene_frames = 0;
  scene_elapsed_time = 0;
}


#ifdef EMSCRIPTEN
EMSCRIPTEN_KEEPALIVE
#endif
void set_scene(int no)
{
  if (no>=0)scene_no = no;
}

#ifdef EMSCRIPTEN
EMSCRIPTEN_KEEPALIVE
#endif
float _ctx_pause = 0.0;

void ctx_parse2 (Ctx *ctx, const char *str, float *scene_time, int *scene_no);

int n_scenes = sizeof (scenes)/sizeof(scenes[0]);
static int ui_scenes (ITK *itk, void *data)
{
  Ctx *ctx = itk->ctx;

  uint64_t ticks = ctx_ticks ();

  render_time = (ticks - prev_ticks) / 1000.0 / 1000.0;
  render_fps = 1.0 / render_time;
  prev_ticks = ticks;

  ctx_save (ctx);
#ifdef EMSCRIPTEN
  EM_ASM(
   var input = document.getElementById('input');
   if (input)
     stringToUTF8(input.value, _ctx_input, 8191);
   if (document.getElementById('scene'))
     _set_scene (parseInt(document.getElementById('scene').value));
  );
#endif

  scene_elapsed_time += render_time;
  scene_frames ++;
  if (!strncmp (ctx_input, "demo", 4))
  {
    if ((unsigned)scene_no >= sizeof (scenes)/sizeof(scenes[0]))
      scene_no = sizeof (scenes)/sizeof(scenes[0])-1;

  scenes[scene_no].fun (itk, scene_frames, render_time);
  ctx_restore (ctx);

  if (scenes[scene_no].duration < scene_elapsed_time)
  {
    scene_no++;
    if (scene_no >= n_scenes)
      scene_no = 0;
    scene_frames = 0;
    scene_elapsed_time = 0;
  }
  }
  else
  {
    ctx_parse2 (itk->ctx, ctx_input, &scene_elapsed_time, &scene_no);
    if (scene_elapsed_time > 30) scene_elapsed_time = 0;
  }
    ctx_queue_draw (itk->ctx);

  return 1;
}

ITK *itk = NULL;

int main (int argc, char **argv)
{
  if (argv[1]) strncpy (ctx_input, argv[1], sizeof(ctx_input)-1);
  ctx_init (&argc, &argv);
  /* we use itk only for its main loop */
  itk_main (ui_scenes, NULL);
 
  return 0;
}
