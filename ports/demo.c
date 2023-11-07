#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "port_config.h"
#include "ctx.h"

#if CTX_ESP
void esp_backlight(int percent);
void esp_restart(void);
int wifi_init_sta(const char *ssid, const char *password);
#endif

int demo_mode = 1;

int  ctx_osk_mode = 0;
void ctx_osk_draw (Ctx *ctx);

static float color_focused_fg[4]  = {1,0,0,0.8};
//static float color_focused_bg[4]  = {1,1,0,0.8};
static float color_interactive[4] = {1,0,0,0.0}; 
static float color_bg[4]          = {0.1, 0.2, 0.3, 1.0};
static float color_bg2[4]         = {0.8, 0.9, 1.0, 1.0};
static float color_fg[4]; // black or white automatically based on bg

int        screen_no      = 0;
int        frame_no       = 0;
float      screen_elapsed = 0.0f;

bool       show_fps          = false; // < boolean ui setting toggles
bool       gradient_bg       = false;
bool       interactive_debug = false;



static float overlay_fade = 1.0;


/////////////////////////////////////////////

typedef struct _UiWidget UiWidget;
typedef struct _Ui       Ui;
static Ui *ui = NULL;

#define em (ui->font_size)
void ui_do(Ui *ui, const char *name);
void ui_start (Ui *ui);
void ui_end (Ui *ui);

void ui_focus_next(void);
void ui_focus_prev(void);

int  ui_button(Ui *ui, void *id, const char *label);
bool ui_toggle(Ui *ui, void *id, const char *label, bool value);
void ui_text(Ui *ui, const char *string);

float ui_slider (Ui *ui, void *id,
                 const char *label, float min, float max, float step, float value);

float
ui_slider_coords (Ui *ui, void *id,
                  float x, float y, float width, float height,
                  float min_val, float max_val, float step, float value);

char * ui_entry_coords(Ui *ui, void *id, float x, float y, float w, float h,
         const char *fallback, const char *value);

void ui_set_color_a (Ctx *ctx, float *rgba, float alpha);
void ui_set_color (Ctx *ctx, float *rgba);
float ui_get_font_size (Ui *ui);

typedef enum
{
  ui_type_none = 0,
  ui_type_button,
  ui_type_slider,
  ui_type_entry
} ui_type;


typedef enum
{
  ui_state_default = 0,
  ui_state_hot,
  ui_state_lost_focus,
  ui_state_commit,
} ui_state;

struct _UiWidget {
  void *id; // unique identifier - in functions can be based on __LINE__ +fileid
            // UI_ID is a macro that uses __LINE__ * 4
  ui_type type;

  float x;  // bounding box of widget
  float y;
  float width;
  float height;

  uint8_t state;    // < maybe refactor to be named bools instead?
  bool visible:1;     // set on first registration/creation/re-registration
  bool fresh:1;     // set on first registration/creation/re-registration
                    // of widget - to initialize stable values
  float min_val;    //  used by slider
  float max_val;    //
  float step;       //
  float float_data; //

};

#define MAX_WIDGETS 32

struct _Ui {
  Ctx *ctx;
  float font_size;
  int widget_no;
  void *focused_id;
  void *active_id;
  int activate;
  int   delta_ms;
  float width;
  float height;
  float line_height;
  float scroll_offset;
  float scroll_offset_target;
  float scroll_speed;  // fraction of height per second
  float y;
  int cursor_pos;
  int selection_length;

  UiWidget widgets[MAX_WIDGETS];
  char temp_text[128];
};



#define UI_DO(command) ui_do(ui, command)

void ui_scroll_to (Ui *ui, float offset);
void ui_set_scroll_offset (Ui *ui, float offset);

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

  ui_set_color (ctx, color_bg);

  if (gradient_bg)
  {
    ctx_linear_gradient (ctx,0,0,0,height);
    ctx_gradient_add_stop (ctx, 0.0, color_bg[0], color_bg[1], color_bg[2], 1.0f);
    ctx_gradient_add_stop (ctx, 1.0, color_bg2[0], color_bg2[1], color_bg2[2], 1.0f);
  }
  ctx_fill(ctx);
  ui_set_color(ctx,color_fg);
}


////////////////////////////////////////////////////////////////////
float bx = DISPLAY_WIDTH/2;
float by = DISPLAY_HEIGHT/2;
int is_down = 0;
float vx = 2.0;
float vy = 2.33;
static void bg_motion (CtxEvent *event, void *data1, void *data2)
{
  bx = event->x;
  by = event->y;
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


#define UI_ID                    ((void*)(__LINE__*4))
#define UI_TEXT(string)          ui_text(ui, string)
#define UI_TOGGLE(label, value)  ui_toggle(ui, UI_ID, label, value)
#define UI_BUTTON(label)         ui_button(ui, UI_ID, label)
#define UI_SLIDER(label,min,max,step,value) \
   ui_slider(ui,UI_ID,label,min,max,step,value)
#define UI_ENTRY(label, fallback, strptr) do{\
   char *ret = NULL;\
   ctx_save(ui->ctx);ctx_font_size (ui->ctx, ui->font_size * 0.75);\
   ctx_move_to(ui->ctx, ui->width * 0.5, ui->y + ui->font_size * 0.1);ctx_text(ui->ctx, label);\
   if ((ret = ui_entry_coords(ui, UI_ID, ui->width * 0.15, ui->y, ui->width * 0.7, ui->line_height, fallback, *strptr)))\
   {\
     if (*strptr) free (*strptr);\
     *strptr = ret;\
   }ctx_restore(ui->ctx);\
   ui->y += ui->line_height;}while(0)


static char *wifi_ssid = NULL;
static char *wifi_password = NULL;

void screen_menu (Ui *ui)
{
   ui_start (ui);

   if (UI_BUTTON("about"))    UI_DO("about");
#ifdef APP_SSH
   if (UI_BUTTON("ssh"))      UI_DO("ssh");
#endif
   if (UI_BUTTON("clock"))    UI_DO("clock");

   if (UI_BUTTON("wifi"))    UI_DO("wifi");
   if (UI_BUTTON("settings")) UI_DO("settings");

   if (UI_BUTTON("spirals"))  UI_DO("spirals");
   if (UI_BUTTON("bouncy"))   UI_DO("bouncy");
   if (UI_BUTTON("todo"))     UI_DO("todo");
#if CTX_ESP
   if (UI_BUTTON("reboot"))   esp_restart();
#endif

   ui->y += ui->line_height;
   ui_end(ui);
}

////// term

static CtxClient *term_client = NULL;

static void term_handle_event (Ctx        *ctx,
                               CtxEvent   *ctx_event,
                               const char *event)
{
  ctx_client_feed_keystring (term_client, ctx_event, event);
}

static void terminal_key_any (CtxEvent *event, void *userdata, void *userdata2)  
{
  Ui *ui = userdata;
  switch (event->type)
  {
    case CTX_KEY_PRESS:
      term_handle_event (event->ctx, event, event->string);
      break;
    case CTX_KEY_UP:
      { char buf[1024];
        snprintf (buf, sizeof(buf)-1, "keyup %i %i", event->unicode, event->state);
        term_handle_event (event->ctx, event, buf);
      }
      break;
    case CTX_KEY_DOWN:
      { char buf[1024];
      if (!strcmp (event->string, "escape")){ ui_do(ui, "back"); return;}
        snprintf (buf, sizeof(buf)-1, "keydown %i %i", event->unicode, event->state);
        term_handle_event (event->ctx, event, buf);
      }
      break;
    default:
      break;
  }
}

#ifdef APP_SSH
int ssh_connect(Ctx *ctx, const char *host, int port, const char *user, const char *password);
static char *ssh_host = NULL;
static char *ssh_user = NULL;
static char *ssh_port = NULL;
static char *ssh_password = NULL;
static int ssh_connected = 0;

static void screen_term (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   ui_start (ui);
   float font_size = ctx_height(ctx)/17;
   if (!ssh_host)
   {
     ssh_host = strdup("192.168.92.98");
     ssh_user = strdup("user");
     ssh_port = strdup("22");
     ssh_password = strdup("password123");
   }
   if (!term_client)
   {
      int flags = 0;
      term_client = ctx_client_new_argv (ctx, NULL, 0,0,ctx_width(ctx),
                                  ctx_height(ctx), font_size,flags, NULL, NULL);
    
      ctx_client_maximize(ctx, ctx_client_id(term_client));
      ctx_client_resize (ctx, ctx_client_id(term_client), ctx_width(ctx)*216/240, ctx_height(ctx)*180/240);
#
      ctx_osk_mode = 2;
   }
   else if (!ctx_osk_mode) ctx_osk_mode = 1;
   
   switch (ssh_connected)
   {
     case 2:
{
     ctx_save(ctx);
     ctx_gray(ctx,0);
     ctx_listen (ctx, CTX_KEY_PRESS, terminal_key_any, ui, NULL);
     ctx_listen (ctx, CTX_KEY_DOWN,  terminal_key_any, ui, NULL);
     ctx_listen (ctx, CTX_KEY_UP,    terminal_key_any, ui, NULL);
     if (ctx_osk_mode > 1)
     {
       int y = ctx_vt_cursor_y (term_client) * font_size;
       ctx_translate (ctx, ctx_width(ctx) * 12/240, ctx_height(ctx)*80/240-y);
     }
     else
       ctx_translate (ctx, ctx_width(ctx) * 12/240, ctx_height(ctx)*35/240);
     ctx_clients_draw (ctx, 0);

     ctx_restore(ctx);
     }break;
     case 0:
       if (UI_BUTTON("connect"))
       {
         if (ssh_connect(ctx, ssh_host, atoi(ssh_port), ssh_user, ssh_password))
         {
           ssh_connected = 2;
           ui_set_scroll_offset (ui, 0);
           ui->focused_id = NULL;
         }
       }
       UI_ENTRY("host", "ip hostname", &ssh_host);
       UI_ENTRY("port", "22", &ssh_port);
       UI_ENTRY("user", "joe", &ssh_user);
       UI_ENTRY("password", "password", &ssh_password);
       break;
     case 1:
        UI_TEXT("unused screen");
        UI_TEXT("for conn error?");
       break;
 

   }

   ui_end (ui);
   ctx_clients_handle_events (ctx);
}
#endif

static void screen_todo (Ui *ui)
{
   ui_start (ui);
   UI_TEXT("file system browser");
   UI_TEXT("espnow-chat");
   UI_TEXT("scrolling/clipping/different positioning of text");
   UI_TEXT("selection in text entry");
   UI_TEXT("micropython");
   UI_TEXT("text editor");
   UI_TEXT("flow3r port");
   ui_end(ui);
}

static float prev_backlight = 100.0f;

#if CTX_ESP
static bool wifi_connected = false;
void screen_wifi (Ui *ui)
{
   ui_start (ui);
   ui->y += ui->height * 0.05;
   UI_TEXT("wifi");

   UI_ENTRY("essid", "wifi name", &wifi_ssid);
   UI_ENTRY("password", "joshua", &wifi_password);

   if (wifi_connected)
     UI_TEXT("connected");
   else
   if (UI_BUTTON("connect"))
   {
     wifi_connected = !wifi_init_sta(wifi_ssid, wifi_password); 
   }

   ui_end (ui);
}
#endif

void screen_settings (Ui *ui)
{
   ui_start (ui);
   static float backlight  = 100.0;
   float line_height = ui->line_height;

   ui->y += ui->height * 0.05;
   //if (UI_BUTTON("back")) UI_DO("back");
   UI_TEXT("settings");

   ui->font_size=UI_SLIDER("font size", 18,45,0.5,ui->font_size);

   if (prev_backlight != backlight)
   {
#if CTX_ESP
     esp_backlight (backlight);
#endif
     prev_backlight = backlight;
   }

#if 0
   static uint8_t byte = 11;
   static int8_t sbyte = 11;
   byte      = UI_SLIDER("uint8_t", 0, 255, 1.0, byte);
   sbyte     = UI_SLIDER("int8_t", -128, 127, 1.0, sbyte);
#endif
   backlight = UI_SLIDER("backlight", 0.0f, 100.0f, 5.0, backlight);
   show_fps  = UI_TOGGLE("show fps", show_fps);

   //ctx_move_to (ctx, ui->width * 0.5f, ui->y+em);
   if (gradient_bg)
     UI_TEXT("bg start gradient RGB");
   else
     UI_TEXT("Background RGB");
   color_bg[0] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, color_bg[0]);
   ui->y += line_height;
   color_bg[1] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, color_bg[1]);
   ui->y += line_height;
   color_bg[2] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, color_bg[2]);
   ui->y += line_height;

#if 0
   gradient_bg = UI_TOGGLE("gradient bg", gradient_bg);

   if (gradient_bg)
   {
     ctx_move_to (ctx, ui->width * 0.5f, ui->y+em);
     ctx_text (ctx, "bg end gradient RGB");
     ui->y += em;
   color_bg2[0] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, color_bg2[0]);
     ui->y += line_height;
   color_bg2[1] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, color_bg2[1]);
     ui->y += line_height;
   color_bg2[2] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, color_bg2[2]);
     ui->y += line_height;
   }
#endif 
   interactive_debug = UI_TOGGLE("show interaction zones", interactive_debug);
   color_interactive[3] = interactive_debug ? 0.3f : 0.0f;
   ui_end(ui);
}

////////

void screen_bouncy (Ui *ui)
{
    Ctx *ctx = ui->ctx;
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    static float dim = 100;
    if (frame_no == 0)
    {
      bx = width /2;
      by = height/2;
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
      ctx_logo (ctx, bx, by, dim);
    }
    else
    {
      ctx_rgb (ctx, 0.5, 0.5, 1);
      ctx_rectangle(ctx, (int)bx, 0, 1, height);ctx_fill(ctx);
      ctx_rectangle(ctx, 0, (int)by, width, 1);ctx_fill(ctx);
    }
    bx += vx;
    by += vy;

    if (bx <= dim/2 || bx >= width - dim/2)
      vx *= -1;
    if (by <= dim/2 || by >= height - dim/2)
      vy *= -1;
}
//////////////////////////////////////////////

void screen_clock (Ui *ui)
{
  Ctx *ctx = ui->ctx;
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
      ctx_move_to (ctx, x + cosf(r) * radius * 0.7f,
                        y + sinf (r) * radius * 0.65f); 
      ctx_line_to (ctx, x + cosf(r) * radius * 0.8f,
                        y + sinf (r) * radius * 0.85f);   
    }
    else
    {
      ctx_move_to (ctx, x + cosf(r) * radius * 0.7f,
                        y + sinf (r) * radius * 0.7f);   
      ctx_line_to (ctx, x + cosf(r) * radius * 0.8f,
                        y + sinf (r) * radius * 0.8f);   
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
  ctx_line_to (ctx, x + cosf(r) * radius * 0.7f,
                    y + sinf(r) * radius * 0.7f);
  ctx_stroke (ctx);

  
  r = (h + m/60.0) * CTX_PI * 2 / 12.0 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.4f,
                    y + sinf(r) * radius * 0.4f);
  ctx_stroke (ctx);
  
  
  ctx_line_width (ctx, radius * 0.02f);
  ctx_line_cap (ctx, CTX_CAP_NONE);
  
  if (smoothstep && frame_no > 2) 
    r = (s + ms / 1000.0f) * CTX_PI * 2 / 60 - CTX_PI / 2;
  else
    r = (s ) * CTX_PI * 2 / 60 - CTX_PI / 2;
  ctx_move_to (ctx, x, y);
  ctx_line_to (ctx, x + cosf(r) * radius * 0.78f,
                    y + sinf(r) * radius * 0.78f);
  ctx_stroke (ctx);
}

static void screen_spirals (Ui *ui)
{
  Ctx *ctx = ui->ctx;
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

typedef void (*screen_fun)(Ui *ui);

typedef struct Screen
{
  const char *name;
  screen_fun  fun;
  float       fps[4];
} Screen;

void screen_about (Ui *ui);
static Screen screens[]={
  {"menu",     screen_menu,    {0,}},
  {"about",    screen_about,   {0,}},
  {"settings", screen_settings,{0,}},
#if CTX_ESP
  {"wifi",     screen_wifi,    {0,}},
#endif
  {"clock",    screen_clock,   {0,}},
  {"bouncy",   screen_bouncy,  {0,}},
  {"spirals",  screen_spirals, {0,}},
#ifdef APP_SSH
  {"ssh",      screen_term,    {0,}},
#endif
  {"todo",     screen_todo,    {0,}},
};

void screen_about (Ui *ui)
{
  Ctx *ctx = ui->ctx;
  ui_start (ui);
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  draw_bg (ctx);
  float ty = height/2;
  char buf[256];

  {
    ctx_move_to(ctx, width * 0.5f,ty);ctx_text(ctx,TITLE);
    ty+=em;
    ctx_move_to(ctx, width * 0.5f,ty);ctx_text(ctx,SUBTITLE);
    ty+=em;
    sprintf(buf, "%.0fx%.0f", width, height);
    ctx_move_to(ctx, width * 0.5,ty);
    ctx_text(ctx,buf);
  }
#if 0
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
      ty+=em;

      if (frame_no == 0)
      {
        printf ("%s  %.1f %.1f\n", screens[i].name, screens[i].fps[0], screens[i].fps[1]);
      }
    }
  }
#endif
 
  ctx_logo (ctx, width/2,height/5,height/3);
  ui_end (ui);
}

#define MAX_HISTORY 8
static int history_count = 0;
static char *history_screen[MAX_HISTORY];
static float history_scroll_offset[MAX_HISTORY];
static void *history_focused[MAX_HISTORY];


static void ui_load_screen_no(Ui *ui, int no)
{
  int n_screens = sizeof(screens)/sizeof(screens[0]);
  if (no < 0) no = n_screens - 1;
  else if (no >= n_screens) no = 0;

  if (history_count + 1 < MAX_HISTORY)
  {
    if (history_screen[history_count])
      free(history_screen[history_count]);
    history_screen[history_count] = strdup (screens[screen_no].name);
    history_focused[history_count] = ui->focused_id;
    history_scroll_offset[history_count] = ui->scroll_offset;
    //printf ("save scroll %f focus %p to %i\n", ui->scroll_offset, ui->focused_id, history_count);
    history_count++;
  }
  ui->focused_id = NULL;
  ui_set_scroll_offset (ui, 0);

  ctx_osk_mode = 0;
  screen_no = no;
  frame_no = 0;
  screen_elapsed = 0; 
}

void
ui_load_screen(Ui *ui, const char *target)
{
  int n_screens = sizeof(screens)/sizeof(screens[0]);
  overlay_fade = 0.7;
  for (int i = 0; i < n_screens; i++)
  if (!strcmp (screens[i].name, target))
    {
      ui_load_screen_no (ui, i);
      return;
    }
  printf ("screen: %s unhandled!\n", target);
}


void
ui_do(Ui *ui, const char *action)
{
  printf ("ui_do: %s\n", action);
  if (!strcmp (action, "back"))
  {
    if (history_count>0)
    {
       int no = history_count-1;
       ui_load_screen (ui, history_screen[no]);
       history_count-=2;
       ui_set_scroll_offset (ui, history_scroll_offset[no]);
       ui->focused_id = history_focused[no];
    }
  }
  else if (!strcmp (action, "activate"))
  {
    if (ui->focused_id)
      ui->activate = 1;
  }
  else if (!strcmp (action, "focus-next"))
  {
    ui_focus_next();
  }
  else if (!strcmp (action, "focus-prev"))
  {
    ui_focus_prev();
  }
  else if (!strcmp (action, "kb-collapse"))
  {
    ctx_osk_mode = 1;
  }
  else if (!strcmp (action, "kb-show"))
  {
    ctx_osk_mode = 2;
  }
  else if (!strcmp (action, "kb-hide"))
  {
    ctx_osk_mode = 0;
  }
  else ui_load_screen(ui, action);
}

static void ui_cb (CtxEvent *event, void *data1, void *data2)
{
  const char *target = data1;
  event->stop_propagate=1;
  if (!strcmp (target, "quit"))
    ctx_quit (event->ctx);
  else
    ui_do (ui, target);
}

void overlay_button (Ctx *ctx, float x, float y, float w, float h, const char *label, char *action)
{
  float m = w;
  if (m > h) m = h;
      ctx_save(ctx);
       ctx_rectangle (ctx, x,y,w,h);
       ctx_listen (ctx, CTX_PRESS, ui_cb, action, NULL);
      if (overlay_fade <= 0.0f)
      {
        ctx_begin_path(ctx);
      }
      else
      {
        ctx_rgba(ctx,0,0,0,overlay_fade);
        ctx_fill(ctx);
        if (overlay_fade > 0.2)
        {
          ctx_rgba(ctx,1,1,1,overlay_fade);
          ctx_move_to (ctx, x+0.5 * w, y + 0.8 * h);
          ctx_font_size (ctx, 0.8 * m);
          ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
          ctx_text (ctx, label);
        }
          
      }
      ctx_restore (ctx);
}

void ui_main(Ui *ui, Ctx *ctx);

#if CTX_ESP
void app_main(void)
#else
int main (int argc, char **argv)
#endif
{
#if CTX_ESP
     wifi_ssid = strdup ("internet");
     wifi_password = strdup ("tryagain");
     wifi_connected = !wifi_init_sta(wifi_ssid, wifi_password); 
#endif
    Ctx *ctx = ctx_new(DISPLAY_WIDTH,
                       DISPLAY_HEIGHT, NULL);
    
    ui = calloc (1, sizeof (Ui));
    ui->scroll_speed = 1.5;


    ui_do(ui, "menu");
    ui_main(ui, ctx);
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
} KeyCap;

typedef struct KeyCapState {
  uint8_t down;
  uint8_t hovered;
} KeyCapState;


static KeyCapState kb_cap_state[9][30];
typedef struct KeyBoardLayout {
  KeyCap keys[9][30];
} KeyBoardLayout;

typedef struct KeyBoard {
  const KeyBoardLayout *layout;
  int shifted;
  int control;
  int alt;
  int fn;
  int down;
} KeyBoard;

static float osk_pos = 1.0f;

static float osk_rows = 10.0f;

static void ctx_on_screen_key_event (CtxEvent *event, void *data1, void *data2)
{
  const KeyCap *key = data1;
  Ui *ui = data1;
  KeyCapState *key_state = data1;
  KeyBoard *kb = data2;
  float h = ctx_height (event->ctx);
  float w = ctx_width (event->ctx);
  int rows = 0;
  for (int row = 0; kb->layout->keys[row][0].label; row++)
    rows = row+1;

  float c = w / osk_rows; // keycell
  float y0 = h * osk_pos - c * rows;

  event->stop_propagate = 1;
  if (//(event->y - event->start_y > c * rows / 2) || 
     (event->y - event->start_y > c * 2 && event->y > h * 0.9))
  {
    ui_do (ui, "kb-collapse");
    for (int row = 0; kb->layout->keys[row][0].label; row++)
    for (int col = 0; kb->layout->keys[row][col].label; col++)
    { 
      kb_cap_state[row][col].hovered = 0;
    }
    return;
  }

  key = NULL;
  key_state = NULL;
  for (int row = 0; kb->layout->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->layout->keys[row][col].label; col++)
    {
      const KeyCap *cap = &(kb->layout->keys[row][col]);
      KeyCapState *cap_state = &kb_cap_state[row][col];
      float y = row * c + y0;
      if (event->x >= x &&
          event->x < x + c * cap->wfactor-0.1 &&
          event->y >= y &&
          event->y < y + c * 0.9)
       {
         key = cap;
         key_state = cap_state;
         if (cap_state->hovered != 1)
         {
           ctx_queue_draw (event->ctx);
         }
         cap_state->hovered = 1;
       }
      else
       {
         cap_state->hovered = 0;
       }

      x += cap->wfactor * c;
    }
  }

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
        key_state->down = !key_state->down;

        if (!strcmp (key->label, "Shift"))
          kb->shifted = key_state->down;
        else if (!strcmp (key->label, "Ctrl"))
          kb->control = key_state->down;
        else if (!strcmp (key->label, "Alt"))
          kb->alt = key_state->down;
        else if (!strcmp (key->label, "Fn"))
          kb->fn = key_state->down;
      }
      else
      {
        if (!strcmp (key->sequence, "kb-collapse"))
        {
          ui_do (ui, "kb-collapse");
        }
        else if (kb->control || kb->alt)
        {
          char combined[200]="";
          if (kb->shifted)
            sprintf (&combined[strlen(combined)], "shift-");
          if (kb->control)
            sprintf (&combined[strlen(combined)], "control-");
          if (kb->alt)
            sprintf (&combined[strlen(combined)], "alt-");
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

static const KeyBoardLayout kb_round = {
   {  
     { 
       {"Esc","Esc",NULL,NULL,1.3f,"escape","escape",NULL,NULL,0},
       {"←","←","Home","Home",1.0f,"left","left","home","home",0,},
       {"↑","↑","PgUp","PgUp",1.0f,"up","up","page-up","page-up",0,},
       {"↓","↓","PgDn","PgDn",1.0f,"down","down","page-down","page-down",0,},
       {"→","→","End","End",1.0f,"right","right","end","end",0,},
       {",","<","_",NULL,1.0f,",","<","_",NULL,0,},
       {".",">","|",NULL,1.0f,".",">","|",NULL,0,},
//     {" "," ",NULL,NULL,0.0f,"","",NULL,NULL,0,},
       {NULL}},
#if 1
     { 
       {" "," ",NULL,NULL,0.0f,"","",NULL,NULL,0,},
       {"Shift","Shift",NULL,NULL,1.4f,"","",NULL,NULL,1,},
       {"Fn","Fn",NULL,NULL,1.0f," "," ",NULL,NULL,1,},
       {"Ctrl","Ctrl",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {"Alt","Alt",NULL,NULL,1.3f,"","",NULL,NULL,1,},
   //  {"\\/","\\/",NULL,NULL,1.0f,"kb-collapse","kb-collapse",NULL,NULL,0,},
   //  {"←","←","Home","Home",1.0f,"left","left","home","home",0,},
   //  {"→","→","End","End",1.0f,"right","right","end","end",0,},
       {"ret","ret",NULL,NULL,2.0f,"return","return",NULL,NULL,0},
       {"bs","bs",NULL,NULL,2.0f,"backspace","backspace",NULL,NULL,0},
       //{"ret","ret",NULL,NULL,1.5f,"return","return",NULL,NULL,0},
       {NULL}},
#endif

#if 0
     { 
       {" "," ",NULL,NULL,0.0f,"","",NULL,NULL,0,},
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
       //{"bs","bs",NULL,NULL,1.2f,"backspace","backspace",NULL,NULL,0,},
       //{"-","_","F11","F11",1.0f,"-","_","F11","F11",0},
       //{"=","+","F12","F12",1.0f,"=","+","F12","F12",0},
       {NULL}},
#endif
     //⌨
     {
       {" "," ",NULL,NULL,0.0f,"","",NULL,NULL,0,},
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
       {" "," ",NULL,NULL,0.5f,"","",NULL,NULL,0,},
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
       {" "," ",NULL,NULL,1.2f,"","",NULL,NULL,0,},
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
     { {"","",NULL,NULL,2.5f,"","",NULL,NULL,0,},
       {"","",NULL,NULL,5.1f,"space","space",NULL,NULL,0,},


/*
*/
       {NULL} },

     { {NULL}},
   }
};

static KeyBoard keyboard = {&kb_round, 0, 0, 0, 0, 0};

void ctx_osk_draw (Ctx *ctx)
{
  static float fade = 0.0;
  const KeyBoard *kb = &keyboard;
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

  fade = 1.0f;
  //if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
  //   fade = 0.9;

  int rows = 0;
  for (int row = 0; kb->layout->keys[row][0].label; row++)
    rows = row+1;

  float c = w / osk_rows; // keycell
  float y0 = h * osk_pos - c * rows;
      
  ctx_save (ctx);
  ctx_rectangle (ctx, 0,
                      y0,
                      w,
                      c * rows);
  ctx_listen (ctx, CTX_DRAG, ctx_on_screen_key_event, ui, (void*)kb);
  ctx_rgba (ctx, 0,0,0, 0.8 * fade);
  ctx_fill (ctx);

  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_line_width (ctx, m * 0.01);

  float font_size = c * 0.9;
  ctx_font_size (ctx, font_size);

  for (int row = 0; kb->layout->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->layout->keys[row][col].label; col++)
    {
      const KeyCap *cap = &(kb->layout->keys[row][col]);
      KeyCapState *cap_state = &(kb_cap_state[row][col]);
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
      
      if (cap_state->down || (cap_state->hovered && kb->down))
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

      if (cap_state->down || (cap_state->hovered && kb->down))
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



      if (cap_state->down || (cap_state->hovered && kb->down))
        ctx_rgba (ctx, 0,0,0, fade);
      else
        ctx_rgba (ctx, 1,1,0.8, fade);

      ctx_text (ctx, label);

      if (cap_state->hovered && kb->down)
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

///  UI 

void ui_main(Ui *ui, Ctx *ctx)
{
    long int prev_ticks = ctx_ticks();
    float width = ctx_width (ctx);
    float height = ctx_height (ctx);
    if (height <= width)
      ui->font_size = height * 0.09f;
    else
      ui->font_size = width * 0.09f;
    while (!ctx_has_quit (ctx))
    {
      width = ctx_width (ctx);
      height = ctx_height (ctx);
      ctx_start_frame (ctx);
      ctx_add_key_binding (ctx, "escape", NULL, "foo",  ui_cb, "back");
      long int ticks = ctx_ticks ();
      long int ticks_delta = ticks - prev_ticks;
      if (ticks_delta > 1000000) ticks_delta = 10; 
      prev_ticks = ticks;

      ctx_save (ctx);

      if (screen_no >= 0 && screen_no < sizeof(screens)/sizeof(screens[0]))
      {
        ui->delta_ms = ticks_delta/1000;
        ui->ctx = ctx;
        screens[screen_no].fun(ui);
      }

      frame_no++;
      screen_elapsed += ticks_delta* 1/(1000*1000.0f);

      ctx_restore (ctx);

      if (screen_no != 0)
      {
        overlay_button (ctx, 0,0,width,height*0.12, "back", "back");
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
         ui_set_color(ctx,color_fg);
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
      if (prev_backlight <= 99.0f)
      {
      ctx_rectangle (ctx, 0,0,ctx_width(ctx),ctx_height(ctx));
      float alpha = 1.0f-(prev_backlight/100.0f * 0.8 + 0.2);
      ctx_rgba (ctx, 0.0f,0.0f,0.0f, alpha);
      ctx_fill (ctx);
      }
      ctx_restore (ctx);
#endif

 
      ctx_end_frame (ctx);
    }
}
void
ui_set_color (Ctx *ctx, float *rgba)
{
  ctx_rgba(ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
}

void
ui_set_color_a (Ctx *ctx, float *rgba, float alpha)
{
  ctx_rgba(ctx, rgba[0], rgba[1], rgba[2], rgba[3] * alpha);
}


UiWidget *ui_widget_by_id(Ui *ui, void *id)
{
  for (int i = 0; i < ui->widget_no; i++)
  {
    if (ui->widgets[i].id == id)
      return &ui->widgets[i];
  }
  return NULL;
}
static void ui_set_focus (UiWidget *widget)
{
  for (int i = 0; i < ui->widget_no; i++)
  if (ui->focused_id)
  {
    UiWidget *old_widget = ui_widget_by_id (ui, ui->focused_id);
    if (old_widget == widget) return;
    if (old_widget)
    {
      old_widget->state = ui_state_lost_focus;
    }
    if (ui->active_id)
    {
      printf("text commit?\n");
      old_widget->state = ui_state_commit;
      ui->active_id = NULL;
      ui_do(ui, "kb-hide");
    }
    ui->active_id = NULL;
    ui->focused_id = NULL;
  }
  if (widget)
  {
    ui->focused_id = widget->id;
    widget->state = ui_state_hot;
  }
}

void ui_focus_next(void)
{
  bool found = false;
  if (ui->widget_no == 0)
  {
    ui_scroll_to (ui, ui->scroll_offset - ui->height * 0.2f);
    return;
  }

  for (int i = 0; i < ui->widget_no; i++)
  {
    if (found || !ui->focused_id)
    {
      ui->focused_id = ui->widgets[i].id;
      return;
    }
    if (ui->widgets[i].id == ui->focused_id)
    {
      found = true;
    }
  }
  ui->focused_id = NULL;
}

void ui_focus_prev(void)
{
  bool found = false;
  if (ui->widget_no == 0)
  {
    ui_scroll_to (ui, ui->scroll_offset + ui->height * 0.2f);
    return;
  }
  for (int i=ui->widget_no-1;i>=0; i--)
  {
    if (found || !ui->focused_id)
    {
      ui->focused_id = ui->widgets[i].id;
      return;
    }
    if (ui->widgets[i].id == ui->focused_id)
    {
      found = true;
    }
  }
  ui->focused_id = NULL;
}

static void ui_slider_key_press (CtxEvent *event,
                                 void *userdata, void *userdata2)
{
  Ui *ui = userdata;
  UiWidget *widget = userdata2;

  const char *string = event->string;

  if (!strcmp (string, "right")
   || !strcmp (string, "up"))
  {
    widget->float_data += widget->step;
    if (widget->float_data >= widget->max_val)
      widget->float_data = widget->max_val;
  }
  else if (!strcmp (string, "down")
       ||  !strcmp (string, "left"))
  {
    widget->float_data -= widget->step;
    if (widget->float_data <= widget->min_val)
      widget->float_data = widget->min_val;
  }
  else if (!strcmp (string, "escape")
        || !strcmp (string, "return"))
  {
     ui->active_id = NULL;
     printf ("deactivated slider\n");
  }
}

static void ui_entry_key_press (CtxEvent *event,
                                void *userdata, void *userdata2)
{
  Ui *ui = userdata;
  UiWidget *widget = userdata2;
  const char *string = event->string;

  if (!strcmp (string, "space"))
    string = " ";
  if (!strcmp (string, "backspace"))
  {
    if (ui->cursor_pos)
    {
      int old_cursor_pos = ui->cursor_pos;
      ui->cursor_pos--;
      while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
        ui->cursor_pos--;
      if (ui->cursor_pos < 0) ui->cursor_pos = 0;
        memmove (&ui->temp_text[ui->cursor_pos],
                 &ui->temp_text[old_cursor_pos],
                 strlen (&ui->temp_text[old_cursor_pos])+1);
    }
  }
  else if (!strcmp (string, "delete"))
  {
    if (ui->cursor_pos < strlen (ui->temp_text))
    {
      memmove (&ui->temp_text[ui->cursor_pos],
               &ui->temp_text[ui->cursor_pos+1],
               strlen (&ui->temp_text[ui->cursor_pos+1])+1);
      while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
        memmove (&ui->temp_text[ui->cursor_pos],
                 &ui->temp_text[ui->cursor_pos+1],
                 strlen (&ui->temp_text[ui->cursor_pos+1])+1);
    }
  }
  else if (!strcmp (string, "return"))
  {
     widget->state = ui_state_commit;
     ui->active_id = NULL;
  }
  else if (!strcmp (string, "escape"))
  {
     ui->active_id = NULL;
     printf ("deactivated\n");
  }
  else if (!strcmp (string, "left"))
  {
    ui->cursor_pos--;
    while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
      ui->cursor_pos--;
    if (ui->cursor_pos < 0) ui->cursor_pos = 0;
  }
  else if (!strcmp (string, "home"))
  {
    ui->cursor_pos = 0;
  }
  else if (!strcmp (string, "end"))
  {
    ui->cursor_pos = strlen (ui->temp_text);
  }
  else if (!strcmp (string, "right"))
  {
    ui->cursor_pos++;
    while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
      ui->cursor_pos++;
    if (strlen (ui->temp_text) < ui->cursor_pos)
      ui->cursor_pos = strlen (ui->temp_text);
  }
  else if (ctx_utf8_strlen (string) == 1)
  {
    int insert_len = strlen (string);
    if (strlen (ui->temp_text) + insert_len + 1 < sizeof (ui->temp_text))
    {
      memmove (&ui->temp_text[ui->cursor_pos+insert_len],
               &ui->temp_text[ui->cursor_pos], strlen (&ui->temp_text[ui->cursor_pos]) + 1);
      memcpy (&ui->temp_text[ui->cursor_pos], string, insert_len);
      ui->cursor_pos += insert_len;
    }
  }
}

static void ui_pan (CtxEvent *event, void *data1, void *data2)
{
  float *fptr = data2;
  demo_mode = 0;
  if (fabs (event->start_y - event->y) > 8.0f)
    ui_set_focus (NULL);
  *fptr += event->delta_y;
  if (*fptr > 0)
    *fptr = 0;
}

float ui_get_font_size (Ui *ui)
{
  return ui->font_size;
}

static void ui_slider_drag_float (CtxEvent *event, void *data1, void *data2)
{
   Ui *ui = data1;
   UiWidget *widget = ui_widget_by_id (ui, data2);
   ui->active_id = NULL;
   if (!widget) return;
   float new_val = ((event->x - widget->x) / widget->width) * (widget->max_val-widget->min_val) + widget->min_val;
   if (new_val < widget->min_val) new_val = widget->min_val;
   if (new_val > widget->max_val) new_val = widget->max_val;
   widget->float_data = new_val;
   event->stop_propagate = 1;
}

static UiWidget *
widget_register (Ui *ui, ui_type type, float x, float y, float width, float height, void *id)
{
   Ctx *ctx = ui->ctx;
   if (ui->widget_no+1 >= MAX_WIDGETS) { printf("too many widgets\n");return &ui->widgets[ui->widget_no]; }

   UiWidget *widget  = &ui->widgets[ui->widget_no++];
  
   if (widget->id != id)
   {
     widget->id = id;
     widget->state = ui_state_default;
     widget->fresh = 1;
     widget->type = type;
   }
   else
   {
     widget->fresh = 0;
   }
   widget->x      = x;
   widget->y      = y;
   widget->width  = width;
   widget->height = height;

   widget->visible = (x >= -em && x < ui->width + em &&
                      y >= -em && y < ui->height + em);

   //if (widget->focusable)
   {
      bool focused = (id == ui->focused_id);
      if (focused)
      {
        if (ui->active_id != widget->id)
        {
        ctx_save(ctx);
        ui_set_color(ctx, color_focused_fg);
        ctx_rectangle (ctx, x - em/2, y - em/2, width + em, height + em);
        ctx_stroke (ctx);
        ctx_restore(ctx);
        }
      }
   }
   return widget;
}

float
ui_slider_coords (Ui *ui, void *id, float x, float y, float width, float height, float min_val, float max_val, float step, float value)
{
   Ctx *ctx = ui->ctx;
   UiWidget *widget = widget_register(ui,ui_type_slider,x,y,width,height,id);
   if (widget->fresh)
     widget->float_data = value;
   widget->min_val = min_val;
   widget->max_val = max_val;
   widget->step = step;

   bool focused = (widget->id == ui->focused_id);
   if (focused && ui->activate)
   {
     widget->float_data = value;
     printf ("!activating slider\n");
     ui->active_id = widget->id;
     ui->activate = 0;
   }

   float rel_value = ((value) - widget->min_val) / (widget->max_val-widget->min_val);

   if (widget->visible)
   {

   ctx_save(ctx); 

   ctx_line_width(ctx,2.0);
   ui_set_color(ctx, color_fg);
   ctx_move_to (ctx, x, y + height/2);
   ctx_line_to (ctx, x + width, y + height/2);
   ctx_stroke (ctx);

   ui_set_color(ctx, color_fg);
   ctx_arc (ctx, x + rel_value * width, y + height/2, height*0.34, 0, 2*3.1415, 0);
   ctx_fill (ctx);
   ui_set_color(ctx, color_bg);
   ctx_arc (ctx, x + rel_value * width, y + height/2, height*0.3, 0.0, 3.1415*2, 0);
   ctx_fill (ctx);

   ctx_rectangle (ctx, x + rel_value * width - height * 0.75, y, height * 1.5, height);
   ctx_listen (ctx, CTX_DRAG, ui_slider_drag_float, ui, widget->id);
   if (color_interactive[3]>0.0)
   {
     ui_set_color(ctx, color_interactive);
     ctx_fill(ctx);
   }
   else
   {
     ctx_begin_path (ctx);
   }
   ctx_restore (ctx);
   }

   return widget->float_data;
}


static void ui_button_drag (CtxEvent *event, void *data1, void *data2)
{
  Ui *ui = data1;
  UiWidget *widget = ui_widget_by_id (ui, data2);
  if (!widget) return;

  //int widget_no = (widget - &widgets[0]);
  if (event->type == CTX_DRAG_PRESS)
  {
    ui_set_focus (widget);
    widget->state = ui_state_hot;
  }
  else if (event->type == CTX_DRAG_RELEASE)
  {
    if (widget->id == ui->focused_id)
    {
    if (widget->state == ui_state_hot)
      ui->activate = 1;
    }
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
   else
   {
     widget->state = ui_state_hot;
   }
  }
}

int
ui_button_coords (Ui *ui, float x, float y, float width, float height,
                  const char *label, int active, void *id)
{
   Ctx *ctx = ui->ctx;
   if (width <= 0) width = ctx_text_width (ctx, label);
   if (height <= 0) height = em * 1.4;

   UiWidget *widget  = widget_register(ui,ui_type_button,x,y,width,height,id);
   if (widget->visible)
   {
   ctx_save(ctx); 

   ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);

   ui_set_color(ctx, color_fg);

   if (active) 
   {
     ctx_rectangle (ctx, x, y, width, height);
     ctx_stroke (ctx);
   }

   ctx_move_to (ctx, x + width/2, y+em);
   ctx_text (ctx, label);

   ctx_begin_path (ctx);
   ctx_rectangle (ctx, x, y, width, height);
   ctx_listen (ctx, CTX_DRAG, ui_button_drag, ui, widget->id);
   if (color_interactive[3]>0.0)
   {
     ui_set_color(ctx, color_interactive);
     ctx_fill(ctx);
   }
   else
   {
     ctx_begin_path (ctx);
   }
   ctx_restore (ctx);
   }

   bool focused = (id == ui->focused_id);
   if ((focused && ui->activate))
   {
     ui->activate = 0;
     widget->state = ui_state_default;
     return 1;
   }
   return 0;
}

void ui_end (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   float delta_y = ui->scroll_speed * ui->height * ui->delta_ms/1000.0;
   if (ui->focused_id)
   {
    UiWidget *focused = ui_widget_by_id (ui, ui->focused_id);

    if (focused) {
      if (ctx_osk_mode == 2) // when keyboard, snap to pos
      {
          if (fabs(ui->height * 0.18 - focused->y) > 2)
            ui->scroll_offset += ui->height * 0.18 - focused->y;
      }
      else
      {
      if (focused->y - delta_y > ui->height * 0.6)
        ui->scroll_offset -= delta_y;
       else if (focused->y + delta_y < ui->height * 0.15) 
      {
        ui->scroll_offset += delta_y;
       }
      }
    } 
   }
   else
    {
      if (ui->scroll_offset_target < ui->scroll_offset - delta_y)
        ui->scroll_offset -= delta_y;
       else if (ui->scroll_offset_target > ui->scroll_offset  + delta_y)
        ui->scroll_offset += delta_y;
     }



      if (ui->active_id)
      {
        UiWidget *widget = ui_widget_by_id (ui, ui->active_id);
        if (widget) switch (widget->type)
        {
          case ui_type_button:
          case ui_type_none:
            break;
          case ui_type_entry:
            ctx_listen (ctx, CTX_KEY_PRESS, ui_entry_key_press, ui, widget);
          break;
          case ui_type_slider:
            ctx_listen (ctx, CTX_KEY_PRESS, ui_slider_key_press, ui, widget);
          break;
        }
      }
      else
      {
        ctx_add_key_binding (ctx, "up", NULL, "foo",      ui_cb, "focus-prev");
        ctx_add_key_binding (ctx, "left", NULL, "foo",    ui_cb, "focus-prev");
        ctx_add_key_binding (ctx, "shift-tab", NULL, "foo", ui_cb, "focus-prev");

        ctx_add_key_binding (ctx, "tab", NULL, "foo",     ui_cb, "focus-next");
        ctx_add_key_binding (ctx, "right", NULL, "foo",   ui_cb, "focus-next");
        ctx_add_key_binding (ctx, "right", NULL, "foo",   ui_cb, "focus-next");
        ctx_add_key_binding (ctx, "down", NULL, "foo",     ui_cb, "focus-next");
        //ctx_add_key_binding (ctx, "return", NULL, "foo",  ui_cb, "activate");
      }
      ctx_add_key_binding (ctx, "control-q", NULL, "foo", ui_cb, "quit");
}

void ui_start (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   draw_bg(ctx);
   ui->width = ctx_width (ctx);
   ui->height = ctx_height (ctx);
   ui->widget_no = 0;
   ui->line_height = ui->font_size * 1.7;
   ctx_rectangle(ctx,0,0,ui->width, ui->height);
   ctx_listen (ctx, CTX_DRAG, ui_pan, NULL, &ui->scroll_offset);
   ctx_listen (ctx, CTX_DRAG, ui_pan, NULL, &ui->scroll_offset_target);
   ctx_begin_path (ctx);
   ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
   ctx_font_size(ctx,ui_get_font_size (ui));
   ui->y = (int)(ui->scroll_offset + ui->height * 0.15);
}

float
ui_slider (Ui *ui, void *id,
           const char *label, float min, float max, float step, float value)
{  Ctx *ctx = ui->ctx;
   ctx_save(ctx);ctx_text_align(ctx,CTX_TEXT_ALIGN_CENTER);\
   ctx_move_to (ctx, ui->width * 0.5, ui->y+em);\
   ctx_text (ctx, label);

   ui->y += em * 0.8f;
   float ret = ui_slider_coords (ui, id, ui->width * 0.1, ui->y, ui->width * 0.8, ui->line_height, min, max, step, value);
   ui->y += ui->line_height;
   ctx_restore(ctx);
   return ret;
}

bool ui_toggle(Ui *ui, void *id, const char *label, bool value)
{ 
   Ctx *ctx = ui->ctx;
   if (ui->y>-2 * em && ui->y < ui->height - em)
   {
     ctx_move_to (ctx, ui->width * 0.5, ui->y+em);
     ctx_text (ctx, label);
   }
   ui->y += ui->line_height;
   if (ui_button_coords(ui, ui->width * 0.15, ui->y, em * 2, 0, "off", value==0, id))
      {value=false;};
   if (ui_button_coords(ui, ui->width * 0.50, ui->y, em * 2, 0, "on", value==1, id+1))
      {value=true;};
   ui->y += ui->line_height;
   return value;
}

void ui_text(Ui *ui, const char *string)
{
   Ctx *ctx = ui->ctx;
   if (ui->y>-em && ui->y < ui->height + em) { ctx_move_to (ctx, ui->width * 0.5, ui->y+em);\
   ctx_text (ctx, string); }; \
   ui->y += ui->line_height;
}

int ui_button(Ui *ui, void *id, const char *label)
{
   if (id == NULL) id = (void*)label;
   return ui->y += ui->line_height, ui_button_coords(ui, ui->width * 0.0, ui->y-ui->line_height, ui->width, ui->line_height, label, 0, id);
}

static void ui_entry_drag (CtxEvent *event, void *data1, void *data2)
{
  Ui *ui = data1;
  UiWidget *widget = ui_widget_by_id (ui, data2);
  if (!widget) return;

  if (event->type == CTX_DRAG_PRESS)
  {
    event->stop_propagate = 0;
    ui_set_focus (widget);
    widget->state = ui_state_default;
  }
  else if (event->type == CTX_DRAG_RELEASE)
  {
    event->stop_propagate = 0;
    if (widget->state != ui_state_lost_focus)
    {
      if (data2 != ui->active_id)
      {
      ui_do(ui, "kb-show");
      ui_do(ui, "activate");
      }
    }
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

char *
ui_entry_coords(Ui *ui,
                void *id,
                float x, float y, float w, float h,
                const char *fallback, const char *value)
{
  Ctx *ctx = ui->ctx;
  UiWidget *widget  = widget_register(ui,ui_type_entry,x,y,w,h, id);

  bool focused = (id == ui->focused_id);
  if (focused && ui->activate)
  {
    if (value)
      strcpy (ui->temp_text, value);
    else {
      ui->temp_text[0] = 0;
    }
    ui->cursor_pos = strlen (ui->temp_text);
    printf ("!activating\n");
    ui->active_id = widget->id;
    ui->activate = 0;
  }

  const char *to_show = value;
  if (ui->active_id == widget->id)
     to_show = &ui->temp_text[0];

  if (!(to_show && to_show[0]))
    to_show = fallback;

  if (widget->visible)
  {
    ctx_save (ctx);
    ctx_text_align (ctx, CTX_TEXT_ALIGN_START);
    if (ui->active_id != widget->id)
    {
       ctx_move_to (ctx, x + w/5, y + ui->font_size);
       ctx_font_size (ctx, ui->font_size);
    
      if (to_show && to_show[0])
      { 
        if (to_show == fallback)
          ui_set_color_a (ctx, color_fg, 0.5);
        ctx_text (ctx, to_show);
      }
    }
    else
    {
       char temp = ui->temp_text[ui->cursor_pos];
       float tw_pre = 0;
       float tw_selection = 0;
       int sel_bytes = 0;
  
       ctx_font_size (ctx, ui->font_size);
       ui->temp_text[ui->cursor_pos]=0;
       tw_pre = ctx_text_width (ctx, ui->temp_text);
       ui->temp_text[ui->cursor_pos]=temp;
       if (ui->selection_length)
       {
         for (int i = 0; i< ui->selection_length; i++)
         {
           sel_bytes += ctx_utf8_len (ui->temp_text[ui->cursor_pos + sel_bytes]);
         }
         temp = ui->temp_text[ui->cursor_pos + sel_bytes];
         ui->temp_text[ui->cursor_pos + sel_bytes] = 0;
         tw_selection = ctx_text_width (ctx, &ui->temp_text[ui->cursor_pos]);
         ui->temp_text[ui->cursor_pos + sel_bytes] = temp;
       }
  
       ctx_move_to (ctx, x + w/5, y + ui->font_size);
  
       if (to_show && to_show[0])
       { 
         if (to_show == fallback)
           ui_set_color_a (ctx, color_fg, 0.5);
         ctx_text (ctx, to_show);
       }
  
       ctx_rectangle (ctx, x + w/5 + tw_pre -1, y + ui->font_size * 0.1,
                      2 + tw_selection, ui->font_size);
       ctx_save (ctx);
       ui_set_color(ctx, color_focused_fg);
       ctx_fill (ctx);
       ctx_restore (ctx);
    }
  
    ctx_begin_path (ctx);
    ctx_rectangle (ctx, x, y, w, h);
    ctx_listen (ctx, CTX_DRAG, ui_entry_drag, ui, widget->id);
    if (color_interactive[3]>0.0)
    {
      ui_set_color(ctx, color_interactive);
      ctx_fill(ctx);
    }
    else
    {
      ctx_begin_path (ctx);
    }
  
    ctx_restore (ctx);
  }

  if (widget->state == ui_state_commit)
  {
    widget->state = ui_state_default;
    return strdup (ui->temp_text);
  }
  return NULL;
}


void ui_set_scroll_offset (Ui *ui, float offset)
{
  ui->scroll_offset = offset;
  ui->scroll_offset_target = offset;
}

void ui_scroll_to (Ui *ui, float offset)
{
  ui->scroll_offset_target = offset;
}

