#ifndef _UI_H_
#define  _UI_H_
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "ctx.h"

#include <sys/stat.h>

#ifndef CTX_DEMO_WIFI_SSID
#define CTX_DEMO_WIFI_SSID     ""
#endif
#ifndef CTX_DEMO_WIFI_PASSWORD
#define CTX_DEMO_WIFI_PASSWORD ""
#endif

#ifndef CTX_DEMO_SSH_USER
#define CTX_DEMO_SSH_USER "pi"
#endif
#ifndef CTX_DEMO_SSH_PASSWORD
#define CTX_DEMO_SSH_PASSWORD "raspberry"
#endif
#ifndef CTX_DEMO_SSH_HOST
#define CTX_DEMO_SSH_HOST "raspberrypi"
#endif

#if CTX_ESP
void esp_backlight(int percent);
void esp_restart(void);
int wifi_init_sta(const char *ssid, const char *password);
#endif
#if CTX_FLOW3R
void     board_init           (void);
int16_t  bsp_captouch_angular (int petal);
uint16_t bsp_captouch_radial  (int petal);
bool     bsp_captouch_down    (int petal);
float    bsp_captouch_angle (float *radial_pos, int quantize, uint16_t petal_mask);
bool     bsp_captouch_screen_touched (void);
extern bool flow3r_synthesize_key_events;
#endif




/////////////////////////////////////////////

typedef struct _UiWidget UiWidget;
typedef struct _Ui       Ui;

Ui *ui_new(Ctx *ctx);
void ui_destroy (Ui *ui);
void ui_main(Ui *ui, const char *start_location);
void ui_do(Ui *ui, const char *name);
void ui_start(Ui *ui);
void ui_pop_fun(Ui *ui);

// draw background like ui would - without needing a matching ui_end
void ui_draw_bg(Ui *ui);

void ui_end(Ui *ui);
void ui_scroll_to (Ui *ui, float offset);
void ui_set_scroll_offset (Ui *ui, float offset);

void ui_focus_next(Ui *ui);
void ui_focus_prev(Ui *ui);

int  ui_button(Ui *ui, const char *label);
bool ui_toggle(Ui *ui, const char *label,
               bool value);
void ui_text(Ui *ui, const char *string);
void ui_spacer(Ui *ui);
int  ui_entry(Ui *ui, const char *label,
              const char *fallback, char **strptr);
float ui_slider(Ui *ui, const char *label,
                float min, float max, float step, float value);


float ui_slider_coords (Ui *ui, void *id,
                        float x, float y, float width, float height,
                        float min_val, float max_val, float step, float value);
int ui_button_coords (Ui *ui, float x, float y, float width, float height,
                      const char *label, int active, void *id);
char * ui_entry_coords(Ui *ui, void *id, float x, float y, float w, float h,
                       const char *fallback, const char *value);


float ui_get_font_size (Ui *ui);

typedef void (*ui_fun)(Ui *ui);
typedef void (*ui_data_finalize)(void *data);

void ui_register_view (Ui         *ui,
                       const char *name,
                       const char *category,
                       ui_fun      fun);

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
  void *id; // unique identifier
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
  float float_data_target; //

};



#define UI_MAX_WIDGETS 32

typedef struct ui_style_t {
  float bg[4];
  float bg2[4];
  float fg[4];
  float focused_fg[4];
  float focused_bg[4];
  float cursor_fg[4];
  float cursor_bg[4];
  float interactive_fg[4];
  float interactive_bg[4];
} ui_style_t;

typedef struct UiView
{
  const char *name;
  ui_fun      fun;
  const char *category;
} UiView;


struct _Ui {
  Ctx   *ctx;
  char  *location;
  ui_fun fun;
  void  *data;
  ui_data_finalize data_finalize;
  int    delta_ms;
  void  *focused_id;
  float  scroll_offset;
  float  font_size;
  float  view_elapsed;
  int    frame_no;
  bool   interactive_debug;
  bool   show_fps;
  bool   gradient_bg;
  bool   draw_tips;

  float  osk_focus_target;
  float  overlay_fade;

  void  *active_id;
  int    activate;
  float  width;
  float  height;
  float  line_height;
  float  scroll_offset_target;
  float  scroll_speed;  // fraction of height per second
  float  y;
  int    cursor_pos;
  int    selection_length;

  int      widget_count;
  UiWidget widgets[UI_MAX_WIDGETS];
  char     temp_text[128];
  UiView   views[64];
  int      n_views;

  ui_style_t style;

  bool   focus_first;
  int    queued_next;
};


void ui_load_file (Ui *ui, const char *path);

Ui *_default_ui(void);

#endif

