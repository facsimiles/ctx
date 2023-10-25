/*
 * SPDX-FileCopyrightText: 2023 Øyvind Kolås
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ctx.h"
Ctx *esp_ctx(void);
void esp_backlight(int percent);

int        demo_mode = 1;
static int demo_rounds = 0; // used for recording different fps measurement
static int demo_timeout_ms = 5 * 1000 ; // 5 seconds per test
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

float x = 120.0;
float y = 180.0;

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
  *fptr += event->delta_y;
  demo_mode = 0;
  if (*fptr > 0)
    *fptr = 0;
}

void draw_bg (Ctx *ctx)
{
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);

  static float prev_red = 0;
  static float prev_green = 0;
  static float prev_blue = 0;
  static int rect_fuzz = 1; // XXX partial redraw hack
                            // geometry changing is picked up, but not the color XXX
  if (color_bg[0] != prev_red ||
      color_bg[1] != prev_green ||
      color_bg[2] != prev_blue)
  {
    rect_fuzz = !rect_fuzz;

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


  ctx_rectangle(ctx,rect_fuzz,0,width,height);

  if (gradient_bg)
  {
    set_color (ctx, color_bg); // XXX bug - without this bright color not working?
    ctx_linear_gradient (ctx,0,0,0,height);
    ctx_gradient_add_stop (ctx, 0.0, color_bg[0], color_bg[1], color_bg[2], 1.0f);
    ctx_gradient_add_stop (ctx, 1.0, color_bg2[0], color_bg2[1], color_bg2[2], 1.0f);
  }
  else
  {
    set_color (ctx, color_bg);
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

void screen_bouncy (Ctx *ctx, uint32_t delta_ms)
{
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    static float dim = 100;
    if (frame_no == 0)
    {
      x = 120.0;
      y = 120.0;
      vx = 2.0;
      vy = 2.33;
    }

    ctx_rectangle(ctx,0,0,width, height);
    ctx_listen (ctx, CTX_DRAG, bg_motion, NULL, NULL);
    ctx_begin_path (ctx); // clear the path, listen doesnt
    draw_bg (ctx);

    ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(ctx, CTX_TEXT_BASELINE_MIDDLE);
    ctx_move_to(ctx, width/2, height/2);ctx_text(ctx,"esp32c3");

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


   ctx_line_width(ctx,3.0);
   set_color_a(ctx, color_fg, 0.2f);
   ctx_move_to (ctx, x, y + height/2);
   ctx_line_to (ctx, x + width, y + height/2);
   ctx_stroke (ctx);

   set_color(ctx, color_fg);
   ctx_arc (ctx, x + rel_value * width, y + height/2, height*0.3, 0, 2*3.1415, 0);
   ctx_stroke (ctx);
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
    event->stop_propagate = 1;
  }
  else if (event->type == CTX_DRAG_RELEASE)
  {
    if (widget->state == ui_state_hot)
      widget->data = (void*)1;
    event->stop_propagate = 1;
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
   ctx_listen (ctx, CTX_MOTION, screen_pan, NULL, &scroll_offset);\
   ctx_begin_path (ctx); \
   ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);\
   float y = (int)(scroll_offset + height * 0.15); \


static void screen_load(const char *name);

#define BUTTON(label) \
   y += line_height, button(ctx, width * 0.15, y-line_height, 0, 0, label, 0, (void*)(__LINE__ * 4))

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
       scroll_offset -= 0.03 * delta_ms;
     }
   }

   if (BUTTON("title"))    screen_load ("title");
   if (BUTTON("clock"))    screen_load ("clock");
   if (BUTTON("settings")) screen_load ("settings");
   if (BUTTON("spirals"))  screen_load ("spirals");
   if (BUTTON("bouncy"))   screen_load ("bouncy");
   if (BUTTON("reboot"))   esp_restart();

   y+= line_height;
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
     esp_backlight (backlight);
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
Screen screens[]={
  {"title",    screen_title,{0,}},
  {"settings", screen_settings,{0,}},
  {"menu",     screen_menu,{0,}},
  {"clock",    screen_clock,{0,}},
  {"bouncy",   screen_bouncy,{0,}},
  {"spirals",  screen_spirals,{0,}},
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
  ctx_move_to(ctx, width * 0.2f,ty);ctx_text(ctx,"esp32c3");
  ty+=font_size;
  ctx_move_to(ctx, width * 0.2f,ty);ctx_text(ctx,"RISC-V 160mhz");
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
 
  ctx_logo (ctx, 120,50,80);
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

static void screen_load(const char *name)
{
  int n_screens = sizeof(screens)/sizeof(screens[0]);
  for (int i = 0; i < n_screens; i++)
    if (!strcmp (screens[i].name, name))
      {
        screen_load_no(i);
        return;
      }
}

static void screen_next(void)
{
  screen_load_no (screen_no + 1);
  if (screen_no == 0 && demo_mode) {
       if(0)switch (demo_rounds)
       {
         case 0: gradient_bg = !gradient_bg; break;
         //case 1: font_size = height/6; break;
         //case 2: font_size = height/10; break;
         default: 
       }
    demo_rounds++;}
}

void menu_press (CtxEvent *event, void *data1, void *data2)
{
  screen_load ("menu");
  demo_mode = 0;
}

void app_main(void)
{
    Ctx *ctx = esp_ctx();

    long int prev_ticks = ctx_ticks();

    float width = ctx_width (ctx);
    float height = ctx_height (ctx);

    if (height >= width)
      font_size = height * 0.09f;
    else
      font_size = width * 0.09f;
    demo_screen_remaining_ms = demo_timeout_ms;

    for (;;) {
      ctx_start_frame (ctx);
      long int ticks = ctx_ticks ();
      long int ticks_delta = ticks - prev_ticks;
      if (ticks_delta > 1000000) ticks_delta = 10; 
      prev_ticks = ticks;

      ctx_save (ctx);

      ctx_rectangle(ctx, 0.0 * width, 0.0 * height, width, 0.12 * height);
      ctx_listen (ctx, CTX_PRESS, menu_press, NULL, NULL);
      ctx_begin_path(ctx);

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

      ctx_end_frame (ctx);
      vTaskDelay(1);
    }
}
