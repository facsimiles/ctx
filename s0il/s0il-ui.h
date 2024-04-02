#ifndef _UI_H_
#define  _UI_H_
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <math.h>
#include <pthread.h>
#include "ctx.h"


/////////////////////////////////////////////
typedef struct _Ui       Ui;
typedef void (*ui_fun)(Ui *ui);
typedef void (*ui_data_finalize)(void *data);

void ui_add_key_binding   (Ui *ui, const char *key, const char *action, const char *label);

// create a Ui instance from ctx, should be paired
// with ui_destroy
Ui *ui_new                (Ctx *ctx);
void ui_destroy           (Ui *ui);

// like ui_new, can take NULL for ctx - should not
// be ui_destroy'ed - reuses the system pre-existing
// Ui instance.
Ui *ui_host               (Ctx *ctx);

char *s0il_load_file      (Ui *ui, const char *path, int *ret_length);

// launch an ui view at location, which is am absolute file-system path or
// a specially recognized view name
void s0il_launch          (Ui *ui);

// do an action, or go to a specified view - if activating a new
// view the current view is first pushed on the view stack,
//   built-in/reserved commands: back kb-hide kb-show kb-collapse exit
//
// names starting with / are taken to be local file system paths

// based on mime-type a registered view is used for rendering.
void s0il_do              (Ui *ui, const char *name);

//
void  s0il_set_data       (Ui *ui, void *data, ui_data_finalize data_finalize);

//
void *s0il_get_data       (Ui *ui);

void
s0il_push_fun (Ui *ui, ui_fun fun, const char *location, void *data, ui_data_finalize data_finalize);

// get the current location / path
// this is set by either s0il_push_fun or indirectly by s0il_do
const char *s0il_location  (Ui *ui);
void        s0il_add_view  (Ui         *ui,
                            const char *name, 
                            ui_fun      fun,
                            const char *binary_path);
void        s0il_pop_fun   (Ui *ui);
void        s0il_iteration (Ui *ui);

//////////////////////////////////////////////////////////////////////////

// draw background like ui would - without needing a matching ui_end
void ui_draw_bg(Ui *ui);

// for each frame should be call bafore other functions, includes a call to
// ui_draw_bg
void ui_start_frame       (Ui *ui);

// should be called after all ui functions in a frame are done with
void ui_end_frame         (Ui *ui);

void ui_scroll_to         (Ui *ui, float offset);
void ui_set_scroll_offset (Ui *ui, float offset);

void ui_focus_next        (Ui *ui);
void ui_focus_prev        (Ui *ui);

int  ui_button (Ui *ui, const char *label);
bool ui_toggle (Ui *ui, const char *label, bool value);
void ui_title  (Ui *ui, const char *string);
void ui_text   (Ui *ui, const char *string);
void ui_textf  (Ui *ui, const char *string, ...);

// todo ui_radio int(ui,label,int set)
// todo ui_expander
// todo ui_choice(ui,label,in_val)
// todo ui_choide_add(ui,value, char label)
// void ui_control_no(ui)

void ui_seperator(Ui *ui);
void ui_newline(Ui *ui);

char *ui_entry(Ui *ui, const char *label, const char *fallback, const char *value);
int ui_entry_realloc(Ui *ui, const char *label, const char *fallback, char **strptr);
float ui_slider(Ui *ui, const char *label, float min, float max, float step, float value);
void ui_slider_float (Ui *ui, const char *label, float *val, float min, float max, float step);
void ui_slider_int (Ui *ui, const char *label, int *val, int min, int max, int step);
void ui_slider_uint8 (Ui *ui, const char *label, uint8_t *val, uint8_t min, uint8_t max, uint8_t step);
void ui_slider_uint16 (Ui *ui, const char *label, uint16_t *val, uint16_t min, uint16_t max, uint16_t step);
void ui_slider_uint32 (Ui *ui, const char *label, uint32_t *val, uint32_t min, uint32_t max, uint32_t step);
void ui_slider_int8 (Ui *ui, const char *label, int8_t *val, int8_t min, int8_t max, int8_t step);
void ui_slider_int16 (Ui *ui, const char *label, int16_t *val, int16_t min, int16_t max, int16_t step);
void ui_slider_int32 (Ui *ui, const char *label, int32_t *val, int32_t min, int32_t max, int32_t step);
float ui_slider_coords(Ui *ui, void *id,
                       float x, float y, float width, float height,
                       float min_val, float max_val, float step, float value);
int ui_button_coords(Ui *ui, float x, float y, float width, float height,
                     const char *label, int active, void *id);
char * ui_entry_coords(Ui *ui, void *id, float x, float y, float w, float h,
                       const char *fallback, const char *value);

float ui_get_font_size    (Ui *ui);
void  ui_cb_do            (CtxEvent *event, void *data1, void *data2);
char *ui_basename         (const char *in);
Ctx  *ui_ctx              (Ui *ui);
void  ui_keyboard         (Ui *ui);
float ui_x                (Ui *ui);
float ui_y                (Ui *ui);
void  ui_move_to          (Ui *ui, float x, float y);
bool  ui_keyboard_visible (Ui *ui);
void  ui_fake_circle      (Ui *ui, bool on);

#endif

