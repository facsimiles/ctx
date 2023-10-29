/*
 * SPDX-FileCopyrightText: 2023 Øyvind Kolås
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "port_config.h"
#include "ctx.h"

#if CTX_ESP
void esp_backlight(int percent);
void esp_restart(void);
#endif

int  ctx_osk_mode = 1;
void ctx_osk_draw (Ctx *ctx);

int        demo_mode = 1;
static int demo_rounds = 0; // used for recording different fps measurement
static int demo_timeout_ms = 10 * 1000 ; // 5 seconds per test
static int demo_screen_remaining_ms = 0;

int        screen_no = 0;
int        frame_no = 0;
float      screen_elapsed = 0;

int        show_fps = 0;   // < boolean ui setting toggles
int        gradient_bg = 0;
int        interactive_debug = 0;

static float font_size = 16.0; // dynamically set on start
#define em font_size

static float color_interactive[4] = {1,0,0,0.0}; 
static float color_bg[4]          = {0.1, 0.2, 0.3, 1.0};
static float color_bg2[4]         = {0.8, 0.9, 1.0, 1.0};
static float color_fg[4]; // black or white automatically based on bg

static float overlay_fade = 1.0;

static void
set_color (Ctx *ctx, float *rgba)
{
  ctx_rgba(ctx, rgba[0],rgba[1],rgba[2],rgba[3]);
}

static void
set_color_a (Ctx *ctx, float *rgba, float alpha)
{
  ctx_rgba(ctx, rgba[0],rgba[1],rgba[2],rgba[3]*alpha);
}

/////////////////////////////////////////////

float x = DISPLAY_WIDTH/2;
float y = DISPLAY_HEIGHT/2;

typedef struct _Widget Widget;
struct _Widget {
  void *id;
  float x;
  float y;
  float width;
  float height;
  float min_val;
  float max_val;
  void *data;
  int   state;
};

typedef enum
{
  ui_state_default = 0,
  ui_state_hot,
  ui_state_lost_focus
} ui_state;

#define MAX_WIDGETS 32
static Widget widgets[MAX_WIDGETS];
static int widget_no = 0;

static void screen_pan (CtxEvent *event, void *data1, void *data2)
{
  float *fptr = data2;

//if ((event->type == CTX_DRAG_MOTION))
  {
    *fptr += event->delta_y;
    demo_mode = 0;
    if (*fptr > 0)
      *fptr = 0;
  }
}

void draw_bg (Ctx *ctx)
{
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);

  static float prev_red = 0;
  static float prev_green = 0;
  static float prev_blue = 0;
  if (color_bg[0] != prev_red ||
      color_bg[1] != prev_green ||
      color_bg[2] != prev_blue)
  {
    if (color_bg[0] +
        color_bg[1] +
        color_bg[2] > 1.8)
    {
      color_fg[0] = 
      color_fg[1] = 
      color_fg[2] = 0.0f;
      color_fg[3] = 1.0f;
    }
    else
    {
      color_fg[0] = 
      color_fg[1] = 
      color_fg[2] = 1.0f;
      color_fg[3] = 1.0f;
    }

    prev_red = color_bg[0];
    prev_green = color_bg[1];
    prev_blue = color_bg[2];
  }

  ctx_rectangle(ctx,0,0,width,height);

  set_color (ctx, color_bg);
  if (gradient_bg)
  {
    ctx_linear_gradient (ctx,0,0,0,height);
    ctx_gradient_add_stop (ctx, 0.0, color_bg[0], color_bg[1], color_bg[2], 1.0f);
    ctx_gradient_add_stop (ctx, 1.0, color_bg2[0], color_bg2[1], color_bg2[2], 1.0f);
  }
  ctx_fill(ctx);
  ctx_font_size(ctx,font_size);
  set_color(ctx,color_fg);
  widget_no = 0;
}


////////////////////////////////////////////////////////////////////
int is_down = 0;
float vx = 2.0;
float vy = 2.33;
static void bg_motion (CtxEvent *event, void *data1, void *data2)
{
  x = event->x;
  y = event->y;
  vx = 0;
  vy = 0;
  if (event->type != CTX_DRAG_MOTION) is_down = 0;
  else is_down = 1;
    
  if (event->type == CTX_DRAG_RELEASE)
  {
    vx = event->delta_x;
    vy = event->delta_y;
  }
}


Widget *widget_by_id(void *id)
{
  for (int i = 0; i < widget_no; i++)
  {
    if (widgets[i].id == id)
      return &widgets[i];
  }
  return NULL;
}

static void slider_drag_float (CtxEvent *event, void *data1, void *data2)
{
   Widget *widget = widget_by_id (data2);
   if (!widget) return;
   float *value_ptr = widget->data;
   float new_val = ((event->x - widget->x) / widget->width) * (widget->max_val-widget->min_val) + widget->min_val;
   if (new_val < widget->min_val) new_val = widget->min_val;
   if (new_val > widget->max_val) new_val = widget->max_val;
   *value_ptr = new_val;
   event->stop_propagate = 1;
}

static Widget *
widget_register (Ctx *ctx, float x, float y, float width, float height, void *id)
{
   if (widget_no+1 >= MAX_WIDGETS) { printf("too many widgets\n");return &widgets[widget_no]; }

   Widget *widget  = &widgets[widget_no++];
   if (widget->id != id)
   {
     widget->id = id;
     widget->data = NULL; // data is by kept the same from run to run when id is stable
     widget->state = ui_state_default;
   }
   widget->x = x;
   widget->y = y;
   widget->width = width;
   widget->height = height;
   return widget;
}


static void
slider_float (Ctx *ctx, float x, float y, float width, float height, float *val, float min_val, float max_val)
{
   Widget *widget  = widget_register(ctx,x,y,width,height,val);
   widget->data = val;
   widget->min_val = min_val;
   widget->max_val = max_val;

   float rel_value = ((*val) - widget->min_val) / (widget->max_val-widget->min_val);
   ctx_save(ctx); 


   ctx_line_width(ctx,2.0);
   set_color(ctx, color_fg);
   ctx_move_to (ctx, x, y + height/2);
   ctx_line_to (ctx, x + width, y + height/2);
   ctx_stroke (ctx);

   set_color(ctx, color_fg);
   ctx_arc (ctx, x + rel_value * width, y + height/2, height*0.34, 0, 2*3.1415, 0);
   ctx_fill (ctx);
   set_color(ctx, color_bg);
   ctx_arc (ctx, x + rel_value * width, y + height/2, height*0.3, 0.0, 3.1415*2, 0);
   ctx_fill (ctx);


   ctx_rectangle (ctx, x + rel_value * width - height * 0.75, y, height * 1.5, height);
   ctx_listen (ctx, CTX_DRAG, slider_drag_float, NULL, widget->id);
   if (color_interactive[3]>0.0)
   {
     set_color(ctx, color_interactive);
     ctx_fill(ctx);
   }
   else
   {
     ctx_begin_path (ctx);
   }
   ctx_restore (ctx);
}

static void button_drag (CtxEvent *event, void *data1, void *data2)
{
   Widget *widget = widget_by_id (data2);
   if (!widget) return;

  if (event->type == CTX_DRAG_PRESS)
  {
    widget->state = ui_state_hot;
    event->stop_propagate = 0;
  }
  else if (event->type == CTX_DRAG_RELEASE)
  {
    if (widget->state == ui_state_hot)
      widget->data = (void*)1;
    event->stop_propagate = 0;
  }
  else
  {
   if ((event->y < widget->y) ||
       (event->x < widget->x) ||
       (event->y > widget->y + widget->height) ||
       (event->x > widget->x + widget->width))
   {
     widget->state = ui_state_lost_focus;
   }
  }
}

static int
button (Ctx *ctx, float x, float y, float width, float height, const char *label, int active, void *id)
{
   if (width <= 0) width = ctx_text_width (ctx, label);
   if (height <= 0) height = em * 1.4;
   width += 2 * em;

   Widget *widget  = widget_register(ctx,x,y,width,height,id);
   ctx_save(ctx); 

   ctx_text_align(ctx, CTX_TEXT_ALIGN_START);

   if (widget->state == ui_state_hot)
   {
     set_color_a(ctx, color_fg, 0.33);
     ctx_rectangle (ctx, x - em, y - em/2, width + 2*em, height + em);
     ctx_fill (ctx);
   }

   set_color(ctx, color_fg);

   if (active)
   {
     ctx_rectangle (ctx, x, y, width, height);
     ctx_stroke (ctx);
   }
#if 0
   else
   {
     ctx_gray(ctx,0.5);
     ctx_rectangle (ctx, x, y, width, height);
     ctx_fill (ctx);
   }
#endif

   set_color(ctx, color_fg);

   ctx_move_to (ctx, x + 1 * em, y+em);
   ctx_text (ctx, label);

   ctx_begin_path (ctx);
   ctx_rectangle (ctx, x, y, width, height);
   ctx_listen (ctx, CTX_DRAG, button_drag, NULL, widget->id);
   if (color_interactive[3]>0.0)
   {
     set_color(ctx, color_interactive);
     ctx_fill(ctx);
   }
   else
   {
     ctx_begin_path (ctx);
   }
   ctx_restore (ctx);
   widget_no ++;

   if (widget->data)
   {
     widget->data = NULL;
     widget->state = ui_state_default;
     return 1;
   }
   return 0;
}


#define SLIDER_FLOAT(label,min,max,ptr) \
   ctx_save(ctx);ctx_text_align(ctx,CTX_TEXT_ALIGN_START);\
   ctx_move_to (ctx, width * 0.1, y+font_size);\
   ctx_text (ctx, label);\
   ctx_text_align(ctx,CTX_TEXT_ALIGN_END);\
   {char buf[256];sprintf(buf, "%.1f", *ptr);\
    ctx_move_to (ctx, width * 0.9, y+font_size);\
   ctx_text (ctx, buf);}\
   y += font_size;\
   slider_float (ctx, width * 0.1, y, width * 0.8, line_height, ptr, min, max);\
   y += line_height;ctx_restore(ctx);

#define TOGGLE(label, ptr) \
   ctx_move_to (ctx, width * 0.5, y+font_size);\
   ctx_text (ctx, label);\
   y += line_height;\
   if (button(ctx, width * 0.15, y, 0, 0, "off", *ptr==0, (void*)(__LINE__ * 4)))\
      {*ptr=0;};\
   if (button(ctx, width * 0.50, y, 0, 0, "on", *ptr==1, (void*)(__LINE__ * 4 + 1)))\
      {*ptr=1;};\
   y += line_height;

#define TEXT(string) \
   ctx_move_to (ctx, width * 0.5, y+font_size);\
   ctx_text (ctx, string);\
   y += line_height;

#define UI_START() \
   float width = ctx_width (ctx);\
   float height = ctx_height (ctx);\
   draw_bg (ctx);\
   float line_height = font_size * 1.7;\
   static float scroll_offset = 0;\
   if (frame_no == 0 && demo_mode) \
   {\
     scroll_offset = 0;\
   }\
   ctx_rectangle(ctx,0,0,width, height);\
   ctx_listen (ctx, CTX_DRAG, screen_pan, NULL, &scroll_offset);\
   ctx_begin_path (ctx); \
   ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);\
   float y = (int)(scroll_offset + height * 0.15); \

#define BUTTON(label) \
   y += line_height, button(ctx, width * 0.15, y-line_height, 0, 0, label, 0, (void*)(__LINE__ * 4))

static void go(const char *name);


void screen_menu (Ctx *ctx, uint32_t delta_ms)
{
   UI_START();
   if (demo_mode)
   {
     if (frame_no == 0)
     {
       scroll_offset -= 0.0;
     }
     else {
       scroll_offset -= 0.020 * delta_ms;
     }
   }

   if (BUTTON("title"))    go("title");
   if (BUTTON("clock"))    go("clock");
   if (BUTTON("settings")) go("settings");
   if (BUTTON("spirals"))  go("spirals");
   if (BUTTON("bouncy"))   go("bouncy");
   if (BUTTON("todo"))     go("todo");
#if CTX_ESP
   if (BUTTON("reboot"))   esp_restart();
#endif

   y+= line_height;
}

static void screen_todo (Ctx *ctx, uint32_t delta_ms)
{
   UI_START();
   TEXT("animated fades");
   TEXT("file system browser");
   TEXT("text editor");
}

void screen_settings (Ctx *ctx, uint32_t delta_ms)
{
   UI_START();
   static float backlight  = 30.0;

   if (demo_mode && frame_no > 1)
   {
     if (screen_elapsed < 3.0f)
     {
       backlight = backlight + 0.01 * delta_ms;
       if (backlight>=90.0)
         backlight = 90.0;
     }
     else {
       backlight = 30;
       scroll_offset -= 0.05 * delta_ms;
     }
   }

   SLIDER_FLOAT("font size", 11.0f, 37.0f, &font_size);

   static float prev_backlight = 0.0f;
   if (prev_backlight != backlight)
   {
#if CTX_ESP
     esp_backlight (backlight);
#endif
     prev_backlight = backlight;
   }

   SLIDER_FLOAT("backlight", 0.0f, 100.0f, &backlight);
   TOGGLE("show fps", &show_fps);


   ctx_move_to (ctx, width * 0.5f, y+font_size);
   if (gradient_bg)
   ctx_text (ctx, "bg start gradient RGB");
   else
   ctx_text (ctx, "Background RGB");
   y += font_size;
   slider_float (ctx, width * 0.1f, y, width * 0.8f, line_height, &color_bg[0], 0,1);
   y += line_height;
   slider_float (ctx, width * 0.1f, y, width * 0.8f, line_height, &color_bg[1], 0,1);
   y += line_height;
   slider_float (ctx, width * 0.1f, y, width * 0.8f, line_height, &color_bg[2], 0,1);
   y += line_height;

   TOGGLE("gradient bg", &gradient_bg);

   if (gradient_bg)
   {
     ctx_move_to (ctx, width * 0.5f, y+font_size);
     ctx_text (ctx, "bg end gradient RGB");
     y += font_size;
     slider_float (ctx, width * 0.1f, y, width * 0.8f, line_height, &color_bg2[0], 0,1);
     y += line_height;
     slider_float (ctx, width * 0.1f, y, width * 0.8f, line_height, &color_bg2[1], 0,1);
     y += line_height;
     slider_float (ctx, width * 0.1f, y, width * 0.8f, line_height, &color_bg2[2], 0,1);
     y += line_height;
   }
 
   static int interactive_debug = 0;
   TOGGLE("show interaction zones", &interactive_debug);
   color_interactive[3] = interactive_debug ? 0.3f : 0.0f;
}

////////

void screen_bouncy (Ctx *ctx, uint32_t delta_ms)
{
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    static float dim = 100;
    if (frame_no == 0)
    {
      x = width /2;
      y = height/2;
      vx = 2.0;
      vy = 2.33;
    }

    ctx_rectangle(ctx,0,0,width, height);
    ctx_listen (ctx, CTX_DRAG, bg_motion, NULL, NULL);
    ctx_begin_path (ctx); // clear the path, listen doesnt
    draw_bg (ctx);

    ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(ctx, CTX_TEXT_BASELINE_MIDDLE);
    ctx_move_to(ctx, width/2, height/2);ctx_text(ctx, TITLE);


    if (!is_down)
    {
      ctx_logo (ctx, x, y, dim);
    }
    else
    {
      ctx_rgb (ctx, 0.5, 0.5, 1);
      ctx_rectangle(ctx, (int)x, 0, 1, height);ctx_fill(ctx);
      ctx_rectangle(ctx, 0, (int)y, width, 1);ctx_fill(ctx);
    }
    x += vx;
    y += vy;

    if (x <= dim/2 || x >= width - dim/2)
      vx *= -1;
    if (y <= dim/2 || y >= height - dim/2)
      vy *= -1;
}
//////////////////////////////////////////////

void screen_clock (Ctx *ctx, uint32_t delta_ms)
{
  uint32_t ms = ctx_ticks ()/1000;
  uint32_t s = ms / 1000;
  uint32_t m = s / 60;
  uint32_t h = m / 60;
  float radius = ctx_width(ctx)/2;
  int smoothstep = 1;
  float x = ctx_width (ctx)/ 2;
  float y = ctx_height (ctx)/ 2;
  if (radius > ctx_height (ctx)/2) radius = ctx_height (ctx)/2;

  ms = ((uint32_t)(ms)) % 1000;
  s %= 60;
  m %= 60;
  h %= 12;
  
  draw_bg (ctx);
  float r; 
  
  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  
  for (int markh = 0; markh < 12; markh++)
  { 
    r = markh * CTX_PI * 2 / 12.0 - CTX_PI / 2;
    
    if (markh == 0)
    {
    ctx_move_to (ctx, x + cosf(r) * radius * 0.7f, y + sinf (r) * radius * 0.65f); 
    ctx_line_to (ctx, x + cosf(r) * radius * 0.8f, y + sinf (r) * radius * 0.85f);   
    }
    else
    {
    ctx_move_to (ctx, x + cosf(r) * radius * 0.7f, y + sinf (r) * radius * 0.7f);   
    ctx_line_to (ctx, x + cosf(r) * radius * 0.8f, y + sinf (r) * radius * 0.8f);   
    }
    ctx_stroke (ctx);
  }
  ctx_line_width (ctx, radius * 0.01f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);

  ctx_line_width (ctx, radius * 0.075f);
  ctx_line_cap (ctx, CTX_CAP_ROUND);
  
  r = m * CTX_PI * 2 / 60.0 - CTX_PI / 2;
  ;

  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.7f, y + sinf (r) * radius * 0.7f);
  ctx_stroke (ctx);

  
  r = (h + m/60.0) * CTX_PI * 2 / 12.0 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.4f, y + sinf (r) * radius * 0.4f);
  ctx_stroke (ctx);
  
  
  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_NONE);
  
  if (smoothstep && frame_no > 2) 
    r = (s + ms / 1000.0f) * CTX_PI * 2 / 60 - CTX_PI / 2;
  else
    r = (s ) * CTX_PI * 2 / 60 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.78f, y + sinf (r) * radius * 0.78f);
  ctx_stroke (ctx);
}


static void screen_spirals (Ctx *ctx, uint32_t delta_ms)
{
  static int dot_count = 0;
  static int shape = 0;
  float dot_scale = 42;
  static float twist = 2.9645f;

  if (frame_no == 0) dot_count = 27;
  dot_count += 2;
  if (dot_count >= 42)
  {
    shape = !shape;
    dot_count = 27;
  }

  draw_bg(ctx);
  for (int i = 0; i < dot_count; i ++)
  {
    float x = ctx_width (ctx)/ 2;
    float y = ctx_height (ctx) / 2;
    float radius = ctx_height (ctx) / dot_scale;
    float dist = (i) * (ctx_height (ctx)/ 2) / (dot_count * 1.0f);
    float twisted = (i * twist);
    x += cosf (twisted) * dist;
    y += sinf (twisted) * dist;
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

}

typedef void (*screen_fun)(Ctx *ctx, uint32_t delta_ms);

typedef struct Screen
{
  const char *name;
  screen_fun  fun;
  float       fps[4];
} Screen;

void screen_title (Ctx *ctx, uint32_t delta_ms);
static Screen screens[]={
  {"title",    screen_title,   {0,}},
  {"settings", screen_settings,{0,}},
  {"menu",     screen_menu,    {0,}},
  {"clock",    screen_clock,   {0,}},
  {"bouncy",   screen_bouncy,  {0,}},
  {"spirals",  screen_spirals, {0,}},
  {"todo",     screen_todo,    {0,}},
};

void screen_title (Ctx *ctx, uint32_t delta_ms)
{
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);

  draw_bg (ctx);
  float ty = height/2;
  char buf[256];

  if (demo_rounds == 0)
  {
    ctx_move_to(ctx, width * 0.2f,ty);ctx_text(ctx,TITLE);
    ty+=font_size;
    ctx_move_to(ctx, width * 0.2f,ty);ctx_text(ctx,SUBTITLE);
    ty+=font_size;
    sprintf(buf, "%.0fx%.0f", width, height);
    ctx_move_to(ctx, width * 0.2,ty);
    ctx_text(ctx,buf);
  }
  else
  {
    int n_screens = sizeof(screens)/sizeof(screens[0]);
      if (frame_no == 0) printf("\n");
    for (int i = 1; i < n_screens; i++)
    {
      char buf[64];
      ctx_move_to(ctx, width * 0.1f,ty);ctx_text(ctx,screens[i].name);
      sprintf (buf, "%.1f", screens[i].fps[0]);
      ctx_move_to(ctx, width * 0.5f,ty);ctx_text(ctx,buf);
      sprintf (buf, "%.1f", screens[i].fps[1]);
      ctx_move_to(ctx, width * 0.7f,ty);ctx_text(ctx,buf);
      ty+=font_size;

      if (frame_no == 0)
      {
        printf ("%s  %.1f %.1f\n", screens[i].name, screens[i].fps[0], screens[i].fps[1]);
      }
    }
  }
 
  ctx_logo (ctx, width/2,height/5,height/3);
}

static void screen_load_no(int no)
{
  int n_screens = sizeof(screens)/sizeof(screens[0]);
  if (no < 0) no = n_screens - 1;
  else if (no >= n_screens) no = 0;

  screen_no = no;
  frame_no = 0;
  screen_elapsed = 0; 
}


static void screen_next(void)
{
  screen_load_no (screen_no + 1);
  if (screen_no == 0 && demo_mode) {
    gradient_bg = !gradient_bg;
    demo_rounds++;
  }
}

static void screen_prev(void)
{
  if (screen_no)
    screen_load_no (screen_no - 1);
}

static void
go(const char *target)
{
  printf ("screen-load: %s\n", target);
  overlay_fade = 0.7;
  if (!strcmp (target, "kb-collapse"))
  {
    ctx_osk_mode = 1;
  }
  else if (!strcmp (target, "kb-show"))
  {
    ctx_osk_mode = 2;
  }
  else if (!strcmp (target, "kb-hide"))
  {
    ctx_osk_mode = 0;
  }
  else if (!strcmp (target, "next"))
  {
    screen_next ();
  }
  else if (!strcmp (target, "prev"))
  {
    screen_prev ();
  }
  else
  {
    int n_screens = sizeof(screens)/sizeof(screens[0]);
    for (int i = 0; i < n_screens; i++)
    if (!strcmp (screens[i].name, target))
      {
        screen_load_no(i);
        return;
      }
  }
}

static void go_cb (CtxEvent *event, void *data1, void *data2)
{
  const char *target = data1;
  if (!strcmp (target, "quit"))
    ctx_quit (event->ctx);
  else
    go (target);
  demo_mode = 0;
}

void overlay_button (Ctx *ctx, float x, float y, float w, float h, const char *label, char *action)
{
      ctx_save(ctx);
       ctx_rectangle (ctx, x,y,w,h);
       ctx_listen (ctx, CTX_PRESS, go_cb, action, NULL);
      if (overlay_fade <= 0.0f)
      {
        ctx_begin_path(ctx);
      }
      else
      {
        ctx_rgba(ctx,0,0,0,overlay_fade);
        ctx_fill(ctx);
        if (overlay_fade > 0.5)
        {
          ctx_rgba(ctx,1,1,1,overlay_fade);
          ctx_move_to (ctx, x+0.5 * w, y + 0.98 * h);
          ctx_font_size (ctx, 0.8 * h);
          ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
          ctx_text (ctx, label);
        }
          
      }
      ctx_restore (ctx);
}


void app_main(void)
{
    Ctx *ctx = ctx_new(DISPLAY_WIDTH,DISPLAY_HEIGHT,NULL);

    long int prev_ticks = ctx_ticks();

    float width = ctx_width (ctx);
    float height = ctx_height (ctx);

    if (height <= width)
      font_size = height * 0.09f;
    else
      font_size = width * 0.09f;
    demo_screen_remaining_ms = demo_timeout_ms;

    //ctx_get_event(ctx);
    while (!ctx_has_quit (ctx))
    {
      ctx_start_frame (ctx);
      long int ticks = ctx_ticks ();
      long int ticks_delta = ticks - prev_ticks;
      if (ticks_delta > 1000000) ticks_delta = 10; 
      prev_ticks = ticks;

      ctx_save (ctx);

      if (screen_no >= 0 && screen_no < sizeof(screens)/sizeof(screens[0]))
        screens[screen_no].fun(ctx, ticks_delta/1000);

      frame_no++;
      demo_screen_remaining_ms-= ticks_delta/1000;
      screen_elapsed += ticks_delta* 1/(1000*1000.0f);

      if (demo_mode && demo_screen_remaining_ms < 0)
      {
        demo_screen_remaining_ms = demo_timeout_ms;
        if (demo_rounds < 2)
          screens[screen_no].fps[demo_rounds] = frame_no * 1000.0f / demo_timeout_ms;

        screen_next ();
      }
 
      ctx_restore (ctx);


      if (screen_no != 2)
      {
        overlay_button (ctx, 0,0,width,height*0.12, "menu", "menu");
      }
      ctx_osk_draw (ctx);

      if (show_fps)
      {
         char buf[32];
         ctx_save (ctx);
         ctx_font_size (ctx,16);
         ctx_rgba(ctx,color_bg[0], color_bg[1], color_bg[2], 0.66f);
         ctx_rectangle(ctx,0,0,width, 17);
         ctx_fill(ctx);
         set_color(ctx,color_fg);
         ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
         ctx_move_to (ctx, ctx_width(ctx)/2, 13);
         static float fps = 0.0f;

         fps = fps * 0.6f + 0.4f * (1000 * 1000.0f/ticks_delta);
         sprintf(buf, "%.1f", fps);
         ctx_text (ctx, buf);
         ctx_restore (ctx);
      }

      float min_dim = ctx_width(ctx);
      if (ctx_height (ctx) < min_dim) min_dim = ctx_height (ctx);

#if CTX_ESP==0
      ctx_save (ctx);
      ctx_rectangle (ctx, 0,0,ctx_width(ctx),ctx_height(ctx));
      ctx_arc (ctx, ctx_width(ctx)/2, ctx_height(ctx)/2, min_dim/2, 0, 3.1415*2, 1);
      ctx_rgba (ctx, 0,0,0,0.9);
      ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);
      ctx_fill (ctx);
      ctx_restore (ctx);
#endif
      ctx_add_key_binding (ctx, "escape", NULL, "foo",    go_cb, "menu");
      ctx_add_key_binding (ctx, "left", NULL, "foo",      go_cb, "prev");
      ctx_add_key_binding (ctx, "right", NULL, "foo",     go_cb, "next");
      ctx_add_key_binding (ctx, "control-q", NULL, "foo", go_cb, "quit");
 
      ctx_end_frame (ctx);
      //ctx_handle_events (ctx); // could this be dealt with in tiled end_Frame?
    }
}

/////////////////////////// keyboard

typedef struct KeyCap {
  char *label;
  char *label_shifted;
  char *label_fn;
  char *label_fn_shifted;
  float wfactor; // 1.0 is regular, tab is 1.5
  char *sequence;
  char *sequence_shifted;
  char *sequence_fn;
  char *sequence_fn_shifted;
  int   sticky;
  int   down;
  int   hovered;
} KeyCap;

typedef struct KeyBoard {
  KeyCap keys[9][30];
  int shifted;
  int control;
  int alt;
  int fn;
  int down;
} KeyBoard;

static float osk_pos = 1.0f;

static float osk_rows = 11.5f;

static void ctx_on_screen_key_event (CtxEvent *event, void *data1, void *data2)
{
  KeyCap *key = data1;
  KeyBoard *kb = data2;
  float h = ctx_height (event->ctx);
  float w = ctx_width (event->ctx);
  int rows = 0;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / osk_rows; // keycell
  float y0 = h * osk_pos - c * rows;

  //if (event->y < y0)
  //  return;

  key = NULL;

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
      float y = row * c + y0;
      if (event->x >= x &&
          event->x < x + c * cap->wfactor-0.1 &&
          event->y >= y &&
          event->y < y + c * 0.9)
       {
         key = cap;
         if (cap->hovered != 1)
         {
           ctx_queue_draw (event->ctx);
         }
         cap->hovered = 1;
       }
      else
       {
         cap->hovered = 0;
       }

      x += cap->wfactor * c;
    }
  }

  event->stop_propagate = 1;
  switch (event->type)
  {
     default:
       break;
     case CTX_MOTION:
         ctx_queue_draw (event->ctx);
       break;
     case CTX_DRAG_MOTION:
       if (!key)
         ctx_queue_draw (event->ctx);
       break;
     case CTX_DRAG_PRESS:
       kb->down = 1;
       ctx_queue_draw (event->ctx);
       break;
     case CTX_DRAG_RELEASE:
       kb->down = 0;
        ctx_queue_draw (event->ctx);
       if (!key)
         return;

      if (key->sticky)
      {
        if (key->down)
          key->down = 0;
        else
          key->down = 1;

        if (!strcmp (key->label, "Shift"))
        {
          kb->shifted = key->down;
        }
        else if (!strcmp (key->label, "Ctrl"))
        {
          kb->control = key->down;
        }
        else if (!strcmp (key->label, "Alt"))
        {
          kb->alt = key->down;
        }
        else if (!strcmp (key->label, "Fn"))
        {
          kb->fn = key->down;
        }
      }
      else
      {
        if (!strcmp (key->sequence, "kb-collapse"))
        {
          go ("kb-collapse");
        }
        else if (kb->control || kb->alt)
        {
          char combined[200]="";
          if (kb->shifted)
          {
            sprintf (&combined[strlen(combined)], "shift-");
          }
          if (kb->control)
          {
            sprintf (&combined[strlen(combined)], "control-");
          }
          if (kb->alt)
          {
            sprintf (&combined[strlen(combined)], "alt-");
          }
          if (kb->fn)
            sprintf (&combined[strlen(combined)], "%s", key->sequence_fn);
          else
            sprintf (&combined[strlen(combined)], "%s", key->sequence);
          ctx_key_press (event->ctx, 0, combined, 0);
        }
        else
        {
          const char *sequence = key->sequence;

          if (kb->fn && kb->shifted && key->sequence_fn_shifted)
          {
            sequence = key->sequence_fn_shifted;
          }
          else if (kb->fn && key->sequence_fn)
          {
            sequence = key->sequence_fn;
          }
          else if (kb->shifted && key->sequence_shifted)
          {
            sequence = key->sequence_shifted;
          }
          ctx_key_press (event->ctx, 0, sequence, 0);
        }
      }
      break;
  }
}

KeyBoard kb_round = {
   {  
#if 1
     { 
       {" "," ",NULL,NULL,0.0f,"","",NULL,NULL,0,},
       {"Shift","Shift",NULL,NULL,1.4f,"","",NULL,NULL,1,},
       {"Fn","Fn",NULL,NULL,1.0f," "," ",NULL,NULL,1,},
       {"Ctrl","Ctrl",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {"Alt","Alt",NULL,NULL,1.3f,"","",NULL,NULL,1,},
       {"Esc","Esc",NULL,NULL,1.3f,"escape","escape",NULL,NULL,0},
       {"\\/","\\/",NULL,NULL,1.0f,"kb-collapse","kb-collapse",NULL,NULL,0,},
   //  {"↑","↑","PgUp","PgUp",1.0f,"up","up","page-up","page-up",0,},
   //  {"↓","↓","PgDn","PgDn",1.0f,"down","down","page-down","page-down",0,},
   //  {"←","←","Home","Home",1.0f,"left","left","home","home",0,},
   //  {"→","→","End","End",1.0f,"right","right","end","end",0,},
       //{"⏎","⏎",NULL,NULL,1.5f,"return","return",NULL,NULL,0},
       //{"⌫","⌫",NULL,NULL,1.2f,"backspace","backspace",NULL,NULL,0},
       {"bs","bs",NULL,NULL,1.2f,"backspace","backspace",NULL,NULL,0,},
       {"ret","ret",NULL,NULL,1.5f,"return","return",NULL,NULL,0},
       {"1","!","F1","F1",1.0f,"1","!","F1","F1",0},
       {"2","@","F2","F2",1.0f,"2","@","F2","F2",0},
       {"3","#","F3","F3",1.0f,"3","#","F3","F3",0},
       {"4","$","F4","F4",1.0f,"4","$","F4","F4",0},
       {"5","%","F5","F5",1.0f,"5","%","F5","F5",0},
       {"6","^","F6","F6",1.0f,"6","^","F6","F6",0},
       {"7","&","F7","F7",1.0f,"7","&","F7","F7",0},
       {"8","*","F8","F8",1.0f,"8","*","F8","F8",0},
       {"9","(","F9","F9",1.0f,"9","(","F9","F9",0},
       {"0",")","F10","F10",1.0f,"0",")","F10","F10",0},
       {"-","_","F11","F11",1.0f,"-","_","F11","F11",0},
       {"=","+","F12","F12",1.0f,"=","+","F12","F12",0},
       {NULL}},
#endif
     //⌨
     {
       {" "," ",NULL,NULL,0.8f,"","",NULL,NULL,0,},
       //{"Fn","Fn",NULL,NULL,1.0f," "," ",NULL,NULL,1,},
       {"q","Q","1","Esc",  1.0f,"q","Q","1","escape",0,},
       {"w","W","2","Tab",  1.0f,"w","W","2","tab",0,},
       {"e","E","3","ret",  1.0f,"e","E","3","return",0,},
       {"r","R","4","",  1.0f,"r","R","4","",0,},
       {"t","T","5","",  1.0f,"t","T","5","",0,},
       {"y","Y","6","",  1.0f,"y","Y","6","",0,},
       {"u","U","7","",  1.0f,"u","U","7","",0,},
       {"i","I","8","",  1.0f,"i","I","8","",0,},
       {"o","O","9","",  1.0f,"o","O","9","",0,},
       {"p","P","0","",1.0f,"p","P","0","",0,},


       {NULL} },
     { 
       {" "," ",NULL,NULL,1.2f,"","",NULL,NULL,0,},
       {"a","A","!","`",1.0f,"a","A","!","`",0,},
       {"s","S","@","~",1.0f,"s","S","@","~",0,},
       {"d","D","#","",1.0f,"d","D","#","",0,},
       {"f","F","$","",1.0f,"f","F","$","",0,},
       {"g","G","%","",1.0f,"g","G","%","",0,},
       {"h","H","^","",1.0f,"h","H","^","",0,},
       {"j","J","&","",1.0f,"j","J","&","",0,},
       {"k","K","*","",1.0f,"k","K","*","",0,},
       {"l","L","(","",1.0f,"l","L","(","",0,},
       {"!","!",")","",1.0f,"!","!",")","",0,},
//     {"ret","ret",NULL,NULL,1.5f,"return","return",NULL,NULL,0},

//     {";",":",")","",1.0f,";",":",")","",0,},

       {NULL} },

     {
       {" "," ",NULL,NULL,1.6f,"","",NULL,NULL,0,},
       {"z","Z","/","[",1.0f,"z","Z","/","[",0,},
       {"x","X","?","]",1.0f,"x","X","?","]",0,},
       {"c","C","'","{",1.0f,"c","C","'","{",0,},
       {"v","V","\"","}",1.0f,"v","V","\"","}",0,},
       {"b","B","+","\\",1.0f,"b","B","+","\\",0,},
       {"n","N","-","Ø",1.0f,"n","N","-","Ø",0,},
       {"m","M","=","å",1.0f,"m","M","=","å",0,},
//     {",","<","_",NULL,1.0f,",","<","_",NULL,0,},
//     {".",">","|",NULL,1.0f,".",">","|",NULL,0,},

       {NULL} },
     { {"","",NULL,NULL,3.0f,"","",NULL,NULL,0,},
       {"","",NULL,NULL,5.1f,"space","space",NULL,NULL,0,},


/*
*/
       {NULL} },

     { {NULL}},
   }
};


void ctx_osk_draw (Ctx *ctx)
{
  static float fade = 0.0;
  KeyBoard *kb = &kb_round;
  static long prev_ticks = -1;
  long ticks = ctx_ticks ();
  float elapsed_ms = 0;
  if (prev_ticks > 0)
  {
    elapsed_ms = (ticks - prev_ticks) / 1000.0f;
  }
  prev_ticks = ticks; 

  //printf ("%f\n", elapsed_ms);
  overlay_fade -= 0.3 * elapsed_ms/1000.0f;
  

  float h = ctx_height (ctx);
  float w = ctx_width (ctx);
  float m = h;
  if (w < h)
    m = w;

  switch (ctx_osk_mode)
  {
    case 2:

  fade = 0.6;
  if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
     fade = 0.8;

  int rows = 0;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / osk_rows; // keycell
  float y0 = h * osk_pos - c * rows;
      
  ctx_save (ctx);
  ctx_rectangle (ctx, 0,
                      y0,
                      w,
                      c * rows);
  ctx_listen (ctx, CTX_DRAG, ctx_on_screen_key_event, NULL, kb);
  ctx_rgba (ctx, 0,0,0, 0.8 * fade);
  //if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
  ctx_fill (ctx);
  //else
//ctx_line_width (ctx, m * 0.01);
  //ctx_begin_path (ctx);
#if 0
  ctx_rgba (ctx, 1,1,1, 0.5);
  ctx_stroke (ctx);
#endif

  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_line_width (ctx, m * 0.01);

  float font_size = c * 0.9;
  ctx_font_size (ctx, font_size);

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
      float y = row * c + y0;
  
      const char *label = cap->label;

      if ((kb->fn && kb->shifted && cap->label_fn_shifted))
      {
        label = cap->label_fn_shifted;
      }
      else if (kb->fn && cap->label_fn)
      {
        label = cap->label_fn;
      }
      else if (kb->shifted && cap->label_shifted)
      {
        label = cap->label_shifted;
      }

      if (ctx_utf8_strlen (label) > 1)
      {
        if (font_size != c * 0.66)
        {
          font_size = c * 0.66;
          ctx_font_size (ctx, font_size);
        }
      }
      else
      {
        if (font_size != c * 0.95)
        {
          font_size = c * 0.95;
          ctx_font_size (ctx, font_size);
        }
      }

      ctx_begin_path (ctx);
      ctx_rectangle (ctx, x, y,
                                c * (cap->wfactor-0.1),
                                c * 0.9);
                                //,c * 0.1);
      
      if (cap->down || (cap->hovered && kb->down))
      {
        ctx_rgba (ctx, 1,1,1, fade);
#if 1
      ctx_fill (ctx);
#else
      ctx_preserve (ctx);
      ctx_fill (ctx);

      ctx_rgba (ctx, 0,0,0, fade);
#endif
      }
      else ctx_begin_path (ctx);

      if (cap->down || (cap->hovered && kb->down))
        ctx_rgba (ctx, 1,1,1, fade);
      else
        ctx_rgba (ctx, 0,0,0, fade);

      ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
      ctx_text_baseline (ctx, CTX_TEXT_BASELINE_MIDDLE);

#if 0
      ctx_move_to (ctx, x + cap->wfactor * c*0.5, y + c * 0.5);
      ctx_text_stroke (ctx, label);
#endif

      ctx_move_to (ctx, x + cap->wfactor * c*0.5, y + c * 0.5);



      if (cap->down || (cap->hovered && kb->down))
        ctx_rgba (ctx, 0,0,0, fade);
      else
        ctx_rgba (ctx, 1,1,0.8, fade);

      ctx_text (ctx, label);

      if (cap->hovered && kb->down)
      {
         ctx_save (ctx);
         ctx_rgba (ctx, 0,0,0.0, 0.7*fade);
         ctx_rectangle (ctx, x - c * 0.5 * cap->wfactor, y - c * 4, c * 2 * cap->wfactor, c * 3);
         ctx_fill (ctx);
         ctx_rgba (ctx, 1,1,0.8, fade);
         ctx_move_to (ctx, x+ c * 0.5 * cap->wfactor, y - c * 3);
         ctx_font_size (ctx, c * 2);
         ctx_text (ctx, label);
         ctx_restore (ctx);
      }

      x += cap->wfactor * c;
    }
  }
  ctx_restore (ctx);
     break;
     case 1:
       overlay_button (ctx, 0, h - h * 0.14, w, h * 0.14, "kb", "kb-show");
       break;
  }
}
