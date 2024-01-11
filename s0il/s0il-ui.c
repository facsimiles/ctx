#include "port_config.h"
#include "s0il.h"

typedef struct _UiWidget UiWidget;

void s0il_program_runner_init(void);
int _init_main(int argc, char **argv) {
  // Ui *ui = ui_host(NULL);
  s0il_program_runner_init();

  system("rm -f /tmp/_s0il_*");
  const char wasm_magic[] = {0, 'a', 's', 'm'};
  s0il_add_magic("application/wasm", NULL, wasm_magic, sizeof wasm_magic, 0);
  const char elf_magic_32bit[] = {0x7f, 'E', 'L', 'F', 1, 1, 1, 0, 0, 0};
  const char elf_magic_64bit[] = {0x7f, 'E', 'L', 'F', 2, 1, 1, 0, 0, 0};
  // uint8_t elf_magic[]={0x7f, 'E','L','F', 0,0, 0, 0, 0, 0};
  s0il_add_magic("application/x-sharedlib", NULL, elf_magic_32bit, 8, 0);
  s0il_add_magic("application/x-sharedlib", NULL, elf_magic_64bit, 8, 0);

  s0il_add_magic("application/flow3r", "inode/directory", "flow3r.toml", -1, 0);

  // register text and image handlers
  s0il_system("s0il-text --register");
  s0il_system("s0il-image --register");
  s0il_system("s0il-dir --register");

  const char mpg1_magic[] = {0x00, 0x00, 0x01, 0xba};

  const char gif_magic1[] = {0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0};
  const char gif_magic2[] = {0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0};

  s0il_add_magic("video/mpeg", ".mpg", mpg1_magic, 4, 0);
  s0il_add_magic("image/gif", ".gif", gif_magic1, -1, 0);
  s0il_add_magic("image/gif", ".gif", gif_magic2, -1, 0);

  const char z_magic[] = {0x1f, 0x9d, 0};
  s0il_add_magic("application/gzip", ".z", z_magic, -1, 0);
  const char gz_magic[] = {0x1f, 0x8b, 0};

  s0il_add_magic("application/gz", ".gz", gz_magic, -1, 0);
  const char bz2_magic[] = {0x42, 0x5a, 0x68, 0};
  s0il_add_magic("application/bzip2", ".gz", bz2_magic, -1, 0);
  const char zip_magic[] = {0x50, 0x4b, 0x03, 0x04, 0};
  const char zip_magic2[] = {0x50, 0x4b, 0x05, 0x06, 0};

  s0il_add_magic("application/zip", ".zip", zip_magic, -1, 0);
  s0il_add_magic("application/zip", ".zip", zip_magic2, -1, 0);

#if 0
  const char wasm_magic[] = {0x00, 0x61, 0x73, 0x6d};
  s0il_add_magic(ui, "application/wasm", ".wasm", wasm_magic, 4, 0);
#endif

  const char flac_magic[] = {0x66, 0x4c, 0x61, 0x43, 0};

  s0il_add_magic("audio/flac", ".flac", flac_magic, -1, 0);
  const char midi_magic[] = {0x4d, 0x54, 0x68, 0x64, 0};
  s0il_add_magic("audio/sp-midi", ".mid", midi_magic, -1, 0);
  const char wav_magic[] = {0x52, 0x49, 0x46, 0x46, 0};
  s0il_add_magic("audio/x-wav", ".wav", wav_magic, -1, 0);
  s0il_add_magic("audio/mp3", ".mp3", NULL, 0, 0);

  const char s0il_magic[] = {0, 's', '0', 'i', 'l'};
  s0il_add_magic("s0il/built-in", NULL, s0il_magic, 5, 0);

#if EMSCRIPTEN
  mkdir("/sd", 0777);
#endif

  return 0;
}

#define UI_MAX_WIDGETS 32
#define UI_MAX_HISTORY 16

typedef enum {
  ui_type_none = 0,
  ui_type_button,
  ui_type_slider,
  ui_type_entry
} ui_type;

typedef enum {
  ui_state_default = 0,
  ui_state_hot,
  ui_state_lost_focus,
  ui_state_commit,
} ui_state;

struct _UiWidget {
  void *id; // unique identifier
  ui_type type;

  float x; // bounding box of widget
  float y;
  float width;
  float height;

  uint8_t state;           // < maybe refactor to be named bools instead?
  bool visible : 1;        // set on first registration/creation/re-registration
  bool fresh : 1;          // set on first registration/creation/re-registration
                           // of widget - to initialize stable values
  float min_val;           //  used by slider
  float max_val;           //
  float step;              //
  float float_data;        //
  float float_data_target; //
};

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

typedef struct UiView {
  char *name;
  ui_fun fun;
  char *binary_path;
} UiView;

#include <unistd.h>

typedef struct ui_history_item {
  char *location;
  float scroll_offset;
  void *focused;
  ui_fun fun;
  void *data;
  ui_data_finalize data_finalize;
} ui_history_item;

struct _Ui {
  Ctx *ctx;
  char *location;
  char *interpreter;

  ui_fun fun;
  void *data;
  ui_data_finalize data_finalize;
  int delta_ms;
  void *focused_id;
  float scroll_offset;
  float font_size_vh;
  float font_size_px; // computed from vh
  float view_elapsed;
  int frame_no;
  bool interactive_debug;
  bool show_fps;
  bool gradient_bg;
  bool draw_tips;

  float osk_focus_target;
  float overlay_fade;
  bool owns_ctx;
  void *active_id;
  int activate;
  float width;
  float height;
  float line_height;
  float scroll_offset_target;
  float scroll_speed; // fraction of height per second
  float y;
  int cursor_pos;
  int selection_length;

  int widget_count;
  UiWidget widgets[UI_MAX_WIDGETS];
  char temp_text[128];
  UiView views[64];
  int n_views;

  ui_style_t style;

  bool fake_circle;
  bool focus_first;
  int queued_next;

  int history_count;
  ui_history_item history[UI_MAX_HISTORY];
};

void _ctx_toggle_in_idle_dispatch(Ctx *ctx);
static int launch_program_handler = 0;
static int program_return_value = 0;

void ui_fake_circle(Ui *ui, bool on) { ui->fake_circle = on; }
void view_program(Ui *ui);

static void drop_state(Ui *ui) {
  if (ui->history_count > 0) {
#if 0
    int no = ui->history_count - 1;
    if (ui->location)
      free(ui->location);
    ui->location = ui->history[no].location;
    ui->history[no].location = NULL;
    ui->fun = ui->history[no].fun;
    ui->data = ui->history[no].data;
    ui->data_finalize = ui->history[no].data_finalize;
    ui->queued_next = 0;
    ui_set_scroll_offset(ui, ui->history[no].scroll_offset);
    ui->focused_id = ui->history[no].focused;
#endif
    ui->history_count -= 1;
  }
}

int launch_program_interpreter(Ctx *ctx, void *data) {
  Ui *ui = data;
  if (launch_program_handler) {
    ctx_remove_idle(ctx, launch_program_handler);
    s0il_output_state_reset();
    _ctx_toggle_in_idle_dispatch(ctx);
    int retval;

    if (ui->interpreter) {
      char *argv[5] = {ui->interpreter, ui->location, NULL, NULL, NULL};
      retval = s0il_runvp(ui->interpreter, argv);
    } else {
      char *argv[2] = {ui->location, NULL};
      retval = s0il_runvp(ui->location, argv);
    }
    s0il_do(ui, "kb-hide");
    if (s0il_output_state() == 1) {
      // TODO : draw a visual count-down
      // sleep (2);
    }

    launch_program_handler = 0;

    _ctx_toggle_in_idle_dispatch(ctx);
    {
      ctx_start_frame(ctx);
      ui_start_frame(ui);
      ui_end_frame(ui);
      ctx_end_frame(ctx);
      ctx_queue_draw(ctx);
    }
    s0il_output_state_reset();

    if (ui->fun == view_program) {
      s0il_pop_fun(ui);
    } else {
        printf("dropping state - something is amiss, we chould just leave the program view\n");
      drop_state(ui);
    }
    program_return_value = retval;
  }
  return 0;
}

static CtxClient *term_client = NULL;
static void handle_event(Ctx *ctx, CtxEvent *ctx_event, const char *event) {
  // printf ("FE! %s\n", event);
  ctx_client_feed_keystring(term_client, ctx_event, event);
}

static void terminal_key_any(CtxEvent *event, void *userdata, void *userdata2) {
  Ui *ui = userdata;
  {
    switch (event->type) {
    case CTX_KEY_PRESS: {
      const char *string = event->string;
      if (ui_keyboard_visible(ui)) {
        if (!strcmp(string, "shift-space")) {
          s0il_do(ui, "kb-hide");
          return;
        }
        if (!strcmp(string, "escape")) {
          s0il_do(ui, "kb-hide");
          return;
        }
      } else {
        if (!strcmp(string, "shift-space")) {
          s0il_do(ui, "kb-show");
          return;
        }
      }
      if (!strcmp(string, "control-q")) {
        s0il_do(ui, "exit");
        return;
      }

      handle_event(event->ctx, event, event->string);

    } break;
    case CTX_KEY_UP: {
      char buf[1024];
      snprintf(buf, sizeof(buf) - 1, "keyup %i %i", event->unicode,
               event->state);
      handle_event(event->ctx, event, buf);
    } break;
    case CTX_KEY_DOWN: {
      char buf[1024];
      snprintf(buf, sizeof(buf) - 1, "keydown %i %i", event->unicode,
               event->state);
      handle_event(event->ctx, event, buf);
    } break;
    default:
      break;
    }
  }
}
int ctx_osk_mode = 0;

static void draw_term(Ui *ui) {
  Ctx *ctx = ui->ctx;
  float font_size = ui->height / 16.0f;
  if (!term_client) {
    int flags = 0;
    term_client = ctx_client_new_argv(ctx, NULL, 0, 0, ui->width, ui->height,
                                      font_size, flags, NULL, NULL);
    ctx_client_maximize(ctx, ctx_client_id(term_client));
    ctx_client_resize(ctx, ctx_client_id(term_client), ui->width * 180 / 240,
                      ui->height * 180 / 240);
  }
  ctx_save(ctx);
  ctx_rectangle(ctx, 0, 0, ctx_width(ctx), ctx_height(ctx));
  ctx_gray(ctx, 0.0f);
  ctx_fill(ctx);
  if (ctx_osk_mode) {
    int y = vt_get_cursor_y(ctx_client_vt(term_client));
    if (y < 3)
      y = 3;
    if (y > 10)
      y = 10;
    ctx_translate(ctx, ctx_width(ctx) * 35 / 240,
                  ctx_height(ctx) * 35 / 240 - (y - 3) * font_size);
  } else {
    ctx_translate(ctx, ctx_width(ctx) * 35 / 240, ctx_height(ctx) * 35 / 240);
  }
  ctx_clients_draw(ctx, 0);
  ctx_restore(ctx);
  ctx_listen(ctx, CTX_KEY_PRESS, terminal_key_any, ui, NULL);
  ctx_listen(ctx, CTX_KEY_DOWN, terminal_key_any, ui, NULL);
  ctx_listen(ctx, CTX_KEY_UP, terminal_key_any, ui, NULL);
}

void view_program(Ui *ui) {

  if (ui->data == NULL) {
    if (!launch_program_handler)
      launch_program_handler =
          ctx_add_timeout(ui->ctx, 0, launch_program_interpreter, ui);
    s0il_set_data(ui, (void *)1, NULL);
  } else {
    if (s0il_output_state() == 1) {
      ui_start_frame(ui);
      draw_term(ui);
      ui_end_frame(ui);
      ui_add_key_binding(ui, "control-q", "exit", "quit");
      ui_add_key_binding(ui, "escape", "exit",
                         "quit"); // XXX having
        // tri-state send ESC.. and having vim/quit in
        // term conflicts.. - alt+f4 instead?
    }
  }
}

///////////////////////////////////////////////////////////////////////

static void ui_set_color_a(Ctx *ctx, float *rgba, float alpha);
static void ui_set_color(Ctx *ctx, float *rgba);
#define UI_ID_STR(label) ((void *)(size_t)(ctx_strhash(label) | 1))
#define UI_ID ((void *)(__LINE__ * 2))
#define em (ui->font_size_px)
int demo_mode = 1;
void ui_keyboard(Ui *ui);
#ifdef CTX_FLOW3R
static bool is_touch = false;
#else
static bool is_touch = true;
#endif
static bool osk_captouch = false;

////////////////////////////////////////////////////////

static float prev_backlight = 100.0f;
void s0il_backlight(float backlight) {
  if (prev_backlight != backlight) {
#if CTX_ESP
    esp_backlight(backlight);
#endif
    prev_backlight = backlight;
  }
}

void view_settings_ui(Ui *ui) {
  ui_start_frame(ui);
  float line_height = ui->line_height;

  ui->y += ui->height * 0.05;
  ui_title(ui, "settings/ui");

  ui->font_size_vh = ui_slider(ui, "font size", 3, 20, 0.1, ui->font_size_vh);
  ui->show_fps = ui_toggle(ui, "show fps", ui->show_fps);

  if (ui->gradient_bg)
    ui_text(ui, "Background top");
  else
    ui_text(ui, "Background RGB");

  ui->style.bg[0] =
      ui_slider_coords(ui, UI_ID, ui->width * 0.1f, ui->y, ui->width * 0.8f,
                       line_height, 0, 1, 8 / 255.0, ui->style.bg[0]);
  ui->y += line_height;
  ui->style.bg[1] =
      ui_slider_coords(ui, UI_ID, ui->width * 0.1f, ui->y, ui->width * 0.8f,
                       line_height, 0, 1, 8 / 255.0, ui->style.bg[1]);
  ui->y += line_height;
  ui->style.bg[2] =
      ui_slider_coords(ui, UI_ID, ui->width * 0.1f, ui->y, ui->width * 0.8f,
                       line_height, 0, 1, 8 / 255.0, ui->style.bg[2]);
  ui->y += line_height;

  ui->gradient_bg = ui_toggle(ui, "gradient bg", ui->gradient_bg);

  if (ui->gradient_bg) {
    ctx_move_to(ui->ctx, ui->width * 0.5f, ui->y + em);
    ctx_text(ui->ctx, "Background bottom");
    ui->y += em;
    ui->style.bg2[0] =
        ui_slider_coords(ui, UI_ID, ui->width * 0.1f, ui->y, ui->width * 0.8f,
                         line_height, 0, 1, 8 / 255.0, ui->style.bg2[0]);
    ui->y += line_height;
    ui->style.bg2[1] =
        ui_slider_coords(ui, UI_ID, ui->width * 0.1f, ui->y, ui->width * 0.8f,
                         line_height, 0, 1, 8 / 255.0, ui->style.bg2[1]);
    ui->y += line_height;
    ui->style.bg2[2] =
        ui_slider_coords(ui, UI_ID, ui->width * 0.1f, ui->y, ui->width * 0.8f,
                         line_height, 0, 1, 8 / 255.0, ui->style.bg2[2]);
    ui->y += line_height;
  }

  ui->interactive_debug =
      ui_toggle(ui, "show interaction zones", ui->interactive_debug);
  ui->style.interactive_bg[3] = ui->interactive_debug ? 0.3f : 0.0f;

  ui_end_frame(ui);
}

// XXX : why is this not a linked list?
void s0il_add_view(Ui *ui,
                 const char *name, // or mime-type
                 ui_fun fun,       // either fun - or binary_path ..
                 const char *binary_path) {
  if (ui->n_views + 1 >= 64)
    return;
  ui->views[ui->n_views].name = strdup(name);
  // ui->views[ui->n_views].category = strdup (category);
  ui->views[ui->n_views].fun = fun;
  if (binary_path)
    ui->views[ui->n_views].binary_path = strdup(binary_path);
  ui->n_views++;
}

void ui_load_view(Ui *ui, const char *target);

static void save_state(Ui *ui) {
  if (ui->history_count + 1 < UI_MAX_HISTORY) {
    if (ui->history[ui->history_count].location)
      free(ui->history[ui->history_count].location);
    if (ui->location == NULL) {
      ui->history[ui->history_count].location = strdup(ui->views[0].name);
    } else
      ui->history[ui->history_count].location = strdup(ui->location);

    ui->history[ui->history_count].focused = ui->focused_id;
    ui->history[ui->history_count].fun = ui->fun;
    ui->history[ui->history_count].data = ui->data;
    ui->history[ui->history_count].data_finalize = ui->data_finalize;
    ui->history[ui->history_count].scroll_offset = ui->scroll_offset;
    ui->history_count++;
  } else {
    printf("history overflow\n");
  }
}


static void restore_state(Ui *ui) {
  if (ui->history_count > 0) {
    int no = ui->history_count - 1;
    if (ui->location)
      free(ui->location);
    ui->location = ui->history[no].location;
    ui->history[no].location = NULL;
    ui->fun = ui->history[no].fun;
    ui->data = ui->history[no].data;
    ui->data_finalize = ui->history[no].data_finalize;
    ui->history_count -= 1;
    ui->queued_next = 0;
    ui_set_scroll_offset(ui, ui->history[no].scroll_offset);
    ui->focused_id = ui->history[no].focused;
  } else
    s0il_do(ui, "exit");
}

void s0il_pop_fun(Ui *ui) {
  s0il_set_data(ui, NULL, NULL);
  restore_state(ui);
}

static void ui_unhandled(Ui *ui) {
  ui_start_frame(ui);
  ui_title(ui, "unknown view");
  ui_text(ui, ui->location);
  ui_end_frame(ui);
}

char *s0il_load_file(Ui *ui, const char *path, int *ret_length) {
  FILE *file = s0il_fopen(path, "rb");
  char *data = NULL;
  if (file) {
    s0il_fseek(file, 0, SEEK_END);
    long length = s0il_ftell(file);
    s0il_fseek(file, 0, SEEK_SET);
    if (ret_length) *ret_length = length;
    data = malloc(length + 1);
    if (data)
    {
      s0il_fread(data, length, 1, file);
      s0il_fclose(file);
      ((char *)data)[length] = 0;
    }
  } else
  {
    printf("load failing for %s\n", path);
  }
  return data;
}

static void ui_view_file(Ui *ui) {
  const char *mime_type = s0il_detect_media_path(ui->location);
  ui->interpreter = NULL;
  for (int i = ui->n_views - 1; i >= 0; i--) {
    const char *name = ui->views[i].name;

    if ((name && !strcmp(name, mime_type))) {
      ui->fun = ui->views[i].fun;
      if (!ui->fun && ui->views[i].binary_path) {
        ui->fun = view_program;
        ui->interpreter = ui->views[i].binary_path;
      }
      return;
    }
  }

  ui_start_frame(ui);
  ui_text(ui, mime_type);
  ui_text(ui, ui->location);
  ui_end_frame(ui);
}

static void ui_view_404(Ui *ui) {
  ui_start_frame(ui);
  ui_title(ui, "404");
  ui_text(ui, ui->location);
  ui_end_frame(ui);
}

void s0il_set_data(Ui *ui, void *data, ui_data_finalize data_finalize) {
  if (ui->data && ui->data_finalize)
    ui->data_finalize(ui->data);
  ui->data = data;
  ui->data_finalize = data_finalize;
}

void *s0il_get_data(Ui *ui) { return ui->data; }

const char *ui_get_location(Ui *ui) { return ui->location; }

int ui_get_frame_no(Ui *ui) { return ui->frame_no; }

void s0il_push_fun(Ui *ui, ui_fun fun, const char *location, void *data,
                 ui_data_finalize data_finalize) {
  if (ui->fun)
    save_state(ui);
  if (ui->location)
    free(ui->location);
  ui->location = NULL;
  if (location)
    ui->location = strdup(location);

  ui->focused_id = NULL;
  ui_set_scroll_offset(ui, 0);

  ctx_osk_mode = 0;
  ui->frame_no = 0;
  ui->view_elapsed = 0;

  if (ui->focus_first)
  {
    ui->queued_next = 2;
  }

  ui->fun = fun;

  ui->data = NULL;
  ui->data_finalize = NULL;
  if (data)
    s0il_set_data(ui, data, data_finalize);
  ui->overlay_fade = 0.7;
}

void ui_load_view(Ui *ui, const char *target) {
  if (target[0] == '/') {
    struct stat st;
    if (s0il_stat(target, &st) == 0) {
      s0il_push_fun(ui, ui_view_file, target, NULL, NULL);
    } else {
      s0il_push_fun(ui, ui_view_404, target, NULL, NULL);
    }
  } else {
    for (int i = ui->n_views - 1; i >= 0; i--)
      if (!strcmp(ui->views[i].name, target)) {
        s0il_push_fun(ui, ui->views[i].fun, target, NULL, NULL);
        return;
      }
    char *epath;
    if ((epath = s0il_path_lookup(ui, target))) {
       s0il_set_data(ui, NULL, NULL);
       s0il_push_fun(ui, view_program, epath, NULL, NULL);
       free(epath);
       return;
    }

    s0il_push_fun(ui, ui_unhandled, target, NULL, NULL);
  }
}

static UiWidget *ui_widget_by_id(Ui *ui, void *id);

void s0il_do(Ui *ui, const char *action) {
  ctx_queue_draw(ui->ctx);
  // printf ("s0il_do: %s\n", action);
  if (!strcmp(action, "exit")) {
    ctx_exit(ui->ctx);
    // we proxy some keys, since this makes binding code simpler
  } else if (!strcmp(action, "backspace") || !strcmp(action, "return") ||
             !strcmp(action, "left") || !strcmp(action, "escape") ||
             !strcmp(action, "right") || !strcmp(action, "space")) {
    ctx_key_press(ui->ctx, 0, action, 0);
  } else if (!strcmp(action, "back")) {
    ui->overlay_fade = 0.7;
    if (ctx_osk_mode > 1) {
      s0il_do(ui, "kb-hide");
    } else if (ui->fun == view_program) {
      ctx_exit(ui->ctx);
    } else {
      s0il_pop_fun(ui);
    }
  } else if (!strcmp(action, "activate")) {
    UiWidget *widget = ui_widget_by_id(ui, ui->focused_id);
    if (ui->focused_id)
      ui->activate = 1;
    if (widget && (widget->type == ui_type_entry))
      s0il_do(ui, "kb-show");
  } else if (!strcmp(action, "focus-next")) {
    ui_focus_next(ui);
  } else if (!strcmp(action, "focus-previous")) {
    ui_focus_prev(ui);
  } else if (!strcmp(action, "kb-collapse")) {
    ctx_osk_mode = 1;
#if CTX_FLOW3R
    bsp_captouch_key_events(2);
#endif
  } else if (!strcmp(action, "kb-show")) {
    ctx_osk_mode = 2;
#if CTX_FLOW3R
    bsp_captouch_key_events(1);
#endif
  } else if (!strcmp(action, "kb-hide")) {
    ctx_osk_mode = 0;
#if CTX_FLOW3R
    bsp_captouch_key_events(2);
#endif
  } else
  {
    ui_load_view(ui, action);
  }
}

int s0il_do_main(int argc, char **argv) {
  Ui *ui = ui_host(NULL);
  if (argv[1])
    s0il_do(ui, argv[1]);
  else {
    printf("Usage: s0il_do <action | view | path>\n");
    printf("  actions: back\n"
           "           activate\n"
           "           kb-show\n"
           "           kb-hide\n"
           "           focus-next\n"
           "           focus-previous\n"
           "           kb-collapse\n"
           "  news views can be registered by programs, either TSR\n"
           "  or with new invokation each time.\n");
  }
  return 0;
}

void ui_cb_do(CtxEvent *event, void *data1, void *data2) {
  Ui *ui = data1;
  const char *target = data2;
  if (data2) {
  }
  event->stop_propagate = 1;
  // printf ("%s %s %s\n", data1, data2, event->string);
  s0il_do(ui, target);
}

void ui_overlay_button(Ui *ui, float x, float y, float w, float h,
                       const char *label, const char *action) {
  Ctx *ctx = ui->ctx;
  float m = w;
  if (m > h)
    m = h;
  ctx_save(ctx);
  ctx_rectangle(ctx, x, y, w, h);
  ctx_listen(ctx, CTX_PRESS, ui_cb_do, ui, (void *)action);
  if (ui->overlay_fade <= 0.0f) {
    ctx_begin_path(ctx);
  } else {
    ctx_queue_draw(ctx);
    ctx_rgba(ctx, 0, 0, 0, ui->overlay_fade);
    ctx_fill(ctx);
    if (ui->overlay_fade > 0.02) {
      ctx_rgba(ctx, 1, 1, 1, ui->overlay_fade);
      ctx_move_to(ctx, x + 0.5 * w, y + 0.8 * h);
      ctx_font_size(ctx, 0.8 * m);
      ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
      ctx_text(ctx, label);
    }
  }
  ctx_restore(ctx);
}

static Ui *def_ui = NULL;

Ui *ui_host(Ctx *ctx) {
  if (!def_ui) {
    bool owns_ctx = false;
    if (!ctx) {
      ctx = ctx_new(-1, -1, NULL);
      owns_ctx = true;
    }
    def_ui = ui_new(ctx);
    def_ui->owns_ctx = owns_ctx;
  }

  if (ctx && def_ui->ctx != ctx)
    printf("ctx mismatch\n");
  return def_ui;
}

#ifdef S0IL_NATIVE
static Ctx *_ctx_host = NULL;
Ctx *ctx_host(void) { return _ctx_host; }
#endif

int s0il_do_main(int argc, char **argv);
Ui *ui_new(Ctx *ctx) {
  Ui *ui = calloc(1, sizeof(Ui));
  s0il_bundle_main("_init", _init_main);
  s0il_bundle_main("s0il_do", s0il_do_main);
  if (!def_ui) {
    def_ui = ui;
#ifdef S0IL_NATIVE
    _ctx_host = ctx;
#endif
    // s0il_add_magic(ui, "application/octet-stream", ".bin", NULL, 0, 0);
  }
  ui->ctx = ctx;
  ui->style.bg[0] = 0.1;
  ui->style.bg[1] = 0.2;
  ui->style.bg[2] = 0.3;
  ui->style.bg[3] = 1.0;
  ui->style.fg[0] = 1.0;
  ui->style.fg[1] = 1.0;
  ui->style.fg[2] = 1.0;
  ui->style.fg[3] = 1.0;

  ui->style.focused_fg[0] = 1.0;
  ui->style.focused_fg[1] = 1.0;
  ui->style.focused_fg[2] = 0.0;
  ui->style.focused_fg[3] = 1.0;

  ui->style.focused_bg[0] = 1.0;
  ui->style.focused_bg[1] = 1.0;
  ui->style.focused_bg[2] = 1.0;
  ui->style.focused_bg[3] = 0.1;

  ui->style.interactive_fg[0] = 1.0;
  ui->style.interactive_fg[1] = 0.0;
  ui->style.interactive_fg[2] = 0.0;
  ui->style.interactive_fg[3] = 0.8;
#if 0
static float color_focused_fg[4]  = {1,0,0,0.8};
//static float color_focused_bg[4]  = {1,1,0,0.8};
static float color_interactive[4] = {1,0,0,0.0}; 
static float color_bg[4]          = {0.1, 0.2, 0.3, 1.0};
static float color_bg2[4]         = {0.8, 0.9, 1.0, 1.0};
static float color_fg[4]; // black or white automatically based on bg
#endif
  ui->focus_first = true;

#if CTX_FLOW3R
  ui->osk_focus_target = 0.33f;
  osk_captouch = true;
#else
  ui->osk_focus_target = 0.18f;
  osk_captouch = true;
#endif
  osk_captouch = true;
  ui->scroll_speed = 1.5f;

  float width = ctx_width(ctx);
  float height = ctx_height(ctx);

  if (height <= width)
    ui->font_size_vh = 9.0f;
  else
    ui->font_size_vh = width / height * 9;

  s0il_add_view(ui, "settings-ui", view_settings_ui, NULL);

  return ui;
}

int s0il_output_state(void);

#if CTX_ESP
#include "esp_task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
#include <unistd.h>

void s0il_iteration(Ui *ui) {
  Ctx *ctx = ui->ctx;
  float width = ctx_width(ctx);
  float height = ctx_height(ctx);

  {
#if CTX_ESP
    int os = s0il_output_state(); // 1 text 2 graphics 3 both
    if (os > 1) {
      vTaskDelay(1);
      return;
    }
#endif

#if 0
#ifdef EMSCRIPTEN
    if (1)//ctx_need_redraw (ctx))
#else
    if (ctx_need_redraw (ctx))
#endif
#else
    if(1)
#endif
    {
      width = ctx_width(ctx);
      height = ctx_height(ctx);
      ctx_start_frame(ctx);
      ui_add_key_binding(ui, "escape", "back", "leave view");
      ui_add_key_binding(ui, "backspace", "back", "leave view");

      ctx_save(ctx);

      if (ui->fun)
        ui->fun(ui);
      if (ui->queued_next) {
        ui->queued_next--;
        if (ui->queued_next <= 0 && ui->widget_count)
          s0il_do(ui, "focus-next");
      }
      ctx_restore(ctx);

      if (is_touch) {
        ui_overlay_button(ui, 0, 0, width, height * 0.12, "back", "back");
      }

      if (ui->draw_tips) {
        if (!is_touch) {
          ctx_save(ctx);

          ctx_translate(ctx, width / 2, height / 2);
          ctx_rotate(ctx, -M_PI * 2 / 5 / 2);
          ctx_translate(ctx, -width / 2, -height / 2);

          const char *labels[] = {"ok", "next", "space", "prev", "back"};
          const char *actions[] = {"return", "right", "space", "left",
                                   "backspace"};

          for (int i = 0; i < 5; i++) {
            ctx_translate(ctx, width / 2, height / 2);
            ctx_rotate(ctx, M_PI * 2 / 5);
            ctx_translate(ctx, -width / 2, -height / 2);
            ui_overlay_button(ui, 0, 0, width, height * 0.12, labels[i],
                              actions[i]);
          }

          ctx_restore(ctx);
        }
      }

      ui_keyboard(ui);

      if (ui->show_fps) {
        char buf[32];
        ctx_save(ctx);
        ctx_font_size(ctx, ui->height / 20);
        ctx_rgba(ctx, ui->style.bg[0], ui->style.bg[1], ui->style.bg[2], 0.8f);
        ctx_rectangle(ctx, 0, 0, width, ui->height / 20);
        ctx_fill(ctx);
        ui_set_color(ctx, ui->style.fg);
        ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
        ctx_move_to(ctx, ctx_width(ctx) / 2, ui->height / 20);
        static float fps = 0.0f;

        fps = fps * 0.6f + 0.4f * (1000.0f / ui->delta_ms);
        sprintf(buf, "%.1f", (double)fps);
        ctx_text(ctx, buf);
        ctx_restore(ctx);
      }

      ctx_end_frame(ctx);

      ui->frame_no++;
      ui->view_elapsed += ui->delta_ms / 1000.0f;
    } else {
      ctx_handle_events(ctx);
    }
  }
}

void s0il_main(Ui *ui) {
  const char *start_location = NULL;
  Ctx *ctx = ui->ctx;

  if (start_location) {
    ui_load_view(ui, start_location);
  } else if (ui->fun) {
  } else {
    printf("nothing to do\n");
    return;
  }

  while (!ctx_has_exited(ctx))
    s0il_iteration(ui);
}
static void ui_set_color(Ctx *ctx, float *rgba) {
  ctx_rgba(ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
}

static void ui_set_color_a(Ctx *ctx, float *rgba, float alpha) {
  ctx_rgba(ctx, rgba[0], rgba[1], rgba[2], rgba[3] * alpha);
}

static UiWidget *ui_widget_by_id(Ui *ui, void *id) {
  for (int i = 0; i < ui->widget_count; i++) {
    if (ui->widgets[i].id == id)
      return &ui->widgets[i];
  }
  return NULL;
}
static void ui_set_focus(Ui *ui, UiWidget *widget) {
  ctx_queue_draw(ui->ctx);
  for (int i = 0; i < ui->widget_count; i++)
    if (ui->focused_id) {
      UiWidget *old_widget = ui_widget_by_id(ui, ui->focused_id);
      if (old_widget == widget)
      {
        ctx_queue_draw(ui->ctx);
        return;
      }
      if (old_widget) {
        old_widget->state = ui_state_lost_focus;
      }
      if (ui->active_id) {
        printf("text commit?\n");
        old_widget->state = ui_state_commit;
        ui->active_id = NULL;
        s0il_do(ui, "kb-hide");
      }
      ui->active_id = NULL;
      ui->focused_id = NULL;
    }
  if (widget) {
    ui->focused_id = widget->id;
    widget->state = ui_state_hot;
  }
  ctx_queue_draw(ui->ctx);
}

void ui_focus_next(Ui *ui) {
  bool found = false;
  ctx_queue_draw(ui->ctx);
  if (ui->widget_count == 0) {
    ui_scroll_to(ui, ui->scroll_offset - ui->height * 0.2f);
    return;
  }

  for (int i = 0; i < ui->widget_count; i++) {
    if (found || !ui->focused_id) {
      ui->focused_id = ui->widgets[i].id;
      ctx_queue_draw(ui->ctx);
      return;
    }
    if (ui->widgets[i].id == ui->focused_id) {
      found = true;
    }
  }
  ui->focused_id = NULL;
}

void ui_focus_prev(Ui *ui) {
  bool found = false;
  ctx_queue_draw(ui->ctx);
  if (ui->widget_count == 0) {
    ui_scroll_to(ui, ui->scroll_offset + ui->height * 0.2f);
    return;
  }
  for (int i = ui->widget_count - 1; i >= 0; i--) {
    if (found || !ui->focused_id) {
      ui->focused_id = ui->widgets[i].id;
      ctx_queue_draw(ui->ctx);
      return;
    }
    if (ui->widgets[i].id == ui->focused_id) {
      found = true;
    }
  }
  ui->focused_id = NULL;
}

static void ui_slider_key_press(CtxEvent *event, void *userdata,
                                void *userdata2) {
  Ui *ui = userdata;
  UiWidget *widget = userdata2;

  const char *string = event->string;

  ctx_queue_draw(ui->ctx);
  if (!strcmp(string, "right") || !strcmp(string, "up")) {
#if 0
    widget->float_data += widget->step;
    if (widget->float_data >= widget->max_val)
      widget->float_data = widget->max_val;
#else
    widget->float_data_target = widget->float_data + widget->step;
    if (widget->float_data_target >= widget->max_val)
      widget->float_data_target = widget->max_val;
#endif
  } else if (!strcmp(string, "down") || !strcmp(string, "left")) {
#if 0
    widget->float_data -= widget->step;
    if (widget->float_data <= widget->min_val)
      widget->float_data = widget->min_val;
#else
    widget->float_data_target = widget->float_data - widget->step;
    if (widget->float_data_target <= widget->min_val)
      widget->float_data_target = widget->min_val;

#endif
  } else if (!strcmp(string, "escape") || !strcmp(string, "space") ||
             !strcmp(string, "backspace") || !strcmp(string, "return")) {
    ui->active_id = NULL;
    printf("deactivated slider\n");
  }
}

static void ui_entry_key_press(CtxEvent *event, void *userdata,
                               void *userdata2) {
  Ui *ui = userdata;
  UiWidget *widget = userdata2;
  const char *string = event->string;
  ctx_queue_draw(ui->ctx);

  if (!strcmp(string, "space"))
    string = " ";
  if (!strcmp(string, "backspace")) {
    if (ui->cursor_pos) {
      int old_cursor_pos = ui->cursor_pos;
      ui->cursor_pos--;
      while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
        ui->cursor_pos--;
      if (ui->cursor_pos < 0)
        ui->cursor_pos = 0;
      memmove(&ui->temp_text[ui->cursor_pos], &ui->temp_text[old_cursor_pos],
              strlen(&ui->temp_text[old_cursor_pos]) + 1);
    }
  } else if (!strcmp(string, "delete")) {
    if (ui->cursor_pos < (int)strlen(ui->temp_text)) {
      memmove(&ui->temp_text[ui->cursor_pos],
              &ui->temp_text[ui->cursor_pos + 1],
              strlen(&ui->temp_text[ui->cursor_pos + 1]) + 1);
      while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
        memmove(&ui->temp_text[ui->cursor_pos],
                &ui->temp_text[ui->cursor_pos + 1],
                strlen(&ui->temp_text[ui->cursor_pos + 1]) + 1);
    }
  } else if (!strcmp(string, "return")) {
    widget->state = ui_state_commit;
    ui->active_id = NULL;
  } else if (!strcmp(string, "escape")) {
    ui->active_id = NULL;
    printf("deactivated\n");
    s0il_do(ui, "kb-hide");
  } else if (!strcmp(string, "left")) {
    ui->cursor_pos--;
    while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
      ui->cursor_pos--;
    if (ui->cursor_pos < 0)
      ui->cursor_pos = 0;
  } else if (!strcmp(string, "home")) {
    ui->cursor_pos = 0;
  } else if (!strcmp(string, "end")) {
    ui->cursor_pos = strlen(ui->temp_text);
  } else if (!strcmp(string, "right")) {
    ui->cursor_pos++;
    while ((ui->temp_text[ui->cursor_pos] & 192) == 128)
      ui->cursor_pos++;
    if ((int)strlen(ui->temp_text) < ui->cursor_pos)
      ui->cursor_pos = strlen(ui->temp_text);
  } else if (ctx_utf8_strlen(string) == 1) {
    int insert_len = strlen(string);
    if (strlen(ui->temp_text) + insert_len + 1 < sizeof(ui->temp_text)) {
      memmove(&ui->temp_text[ui->cursor_pos + insert_len],
              &ui->temp_text[ui->cursor_pos],
              strlen(&ui->temp_text[ui->cursor_pos]) + 1);
      memcpy(&ui->temp_text[ui->cursor_pos], string, insert_len);
      ui->cursor_pos += insert_len;
    }
  }
}

static void ui_pan(CtxEvent *event, void *data1, void *data2) {
  float *fptr = data1;
  demo_mode = 0;
  if (fabsf(event->start_y - event->y) > 8.0f)
    ui_set_focus(data2, NULL);
  *fptr += event->delta_y;
  if (*fptr > 0)
    *fptr = 0;
  ctx_queue_draw(event->ctx);
}

float ui_get_font_size(Ui *ui) { return ui->font_size_px; }

static void ui_slider_drag_float(CtxEvent *event, void *data1, void *data2) {
  Ui *ui = data1;
  UiWidget *widget = ui_widget_by_id(ui, data2);
  ui->active_id = NULL;
  if (!widget)
    return;
  float new_val = ((event->x - widget->x) / widget->width) *
                      (widget->max_val - widget->min_val) +
                  widget->min_val;
  if (new_val < widget->min_val)
    new_val = widget->min_val;
  if (new_val > widget->max_val)
    new_val = widget->max_val;
  widget->float_data = new_val;
  widget->float_data_target = new_val;
  event->stop_propagate = 1;
  ctx_queue_draw(ui->ctx);
}

static UiWidget *ui_widget_register(Ui *ui, ui_type type, float x, float y,
                                    float width, float height, void *id) {
  Ctx *ctx = ui->ctx;
  if (ui->widget_count + 1 >= UI_MAX_WIDGETS) {
    printf("too many widgets\n");
    return &ui->widgets[ui->widget_count];
  }

  UiWidget *widget = &ui->widgets[ui->widget_count++];

  if (widget->id != id) {
    widget->id = id;
    widget->state = ui_state_default;
    widget->fresh = 1;
    widget->type = type;
  } else {
    widget->fresh = 0;
  }
  widget->x = x;
  widget->y = y;
  widget->width = width;
  widget->height = height;

  widget->visible =
      (x >= -em && x < ui->width + em && y >= -em && y < ui->height + em);

  // if (widget->focusable)
  {
    bool focused = (id == ui->focused_id);
    if (focused) {
      if (ui->active_id != widget->id) {

        if (ui->style.focused_bg[3] > 0.0f) {
          ctx_save(ctx);
          ui_set_color(ctx, ui->style.focused_bg);
          ctx_rectangle(ctx, x - em / 2, y, width + em, height);
          ctx_fill(ctx);
          ctx_restore(ctx);
        }
#if 0
          if (ui->style.focused_fg[3]>0.0f)
          {
          ctx_save(ctx);
          ui_set_color(ctx, ui->style.focused_fg);
          ctx_rectangle (ctx, x - em/2, y - em/2, width + em, height + em);
          ctx_stroke (ctx);
          ctx_restore(ctx);
          }
#endif
      }
    }
  }
  return widget;
}

float ui_slider_coords(Ui *ui, void *id, float x, float y, float width,
                       float height, float min_val, float max_val, float step,
                       float value) {
  Ctx *ctx = ui->ctx;
  UiWidget *widget =
      ui_widget_register(ui, ui_type_slider, x, y, width, height, id);
  if (widget->fresh) {
    widget->float_data = value;
    widget->float_data_target = value;
  }
  widget->min_val = min_val;
  widget->max_val = max_val;
  widget->step = step;

  bool focused = (widget->id == ui->focused_id);
  if (focused && ui->activate) {
    widget->float_data = value;
    widget->float_data_target = value;
    printf("!activating slider\n");
    ui->active_id = widget->id;
    ui->activate = 0;
  }

  float rel_value =
      ((value)-widget->min_val) / (widget->max_val - widget->min_val);
  if (ui->active_id == widget->id) {
    value = widget->float_data =
        (value * 0.8 + 0.2 * widget->float_data_target);
  }

  if (widget->visible) {

    ctx_save(ctx);

    ctx_line_width(ctx, 2.0);
    ui_set_color(ctx, ui->style.fg);
    ctx_move_to(ctx, x, y + height / 2);
    ctx_line_to(ctx, x + width, y + height / 2);
    ctx_stroke(ctx);

    if (ui->active_id == widget->id)
      ui_set_color(ctx, ui->style.focused_fg);
    else
      ui_set_color(ctx, ui->style.fg);

    ctx_arc(ctx, x + rel_value * width, y + height / 2, height * 0.34, 0,
            2 * 3.1415, 0);
    ctx_fill(ctx);
    ui_set_color(ctx, ui->style.bg);
    ctx_arc(ctx, x + rel_value * width, y + height / 2, height * 0.3, 0.0,
            3.1415 * 2, 0);
    ctx_fill(ctx);

    ctx_rectangle(ctx, x + rel_value * width - height * 0.75, y, height * 1.5,
                  height);
    ctx_listen(ctx, CTX_DRAG, ui_slider_drag_float, ui, widget->id);
    if (ui->interactive_debug) {
      ui_set_color(ctx, ui->style.interactive_bg);
      ctx_fill(ctx);
    } else {
      ctx_begin_path(ctx);
    }
    ctx_restore(ctx);
  }

  return widget->float_data;
}

static void ui_button_drag(CtxEvent *event, void *data1, void *data2) {
  Ui *ui = data1;
  UiWidget *widget = ui_widget_by_id(ui, data2);
  if (!widget)
    return;

  if (event->type == CTX_DRAG_PRESS) {
    ui_set_focus(ui, widget);
    widget->state = ui_state_hot;
    ctx_queue_draw(ui->ctx);
  } else if (event->type == CTX_DRAG_RELEASE) {
    if (widget->id == ui->focused_id) {
      if (widget->state == ui_state_hot)
        ui->activate = 1;
    }
    ctx_queue_draw(ui->ctx);
  } else {
    if ((event->y < widget->y) || (event->x < widget->x) ||
        (event->y > widget->y + widget->height) ||
        (event->x > widget->x + widget->width)) {
      widget->state = ui_state_lost_focus;
      ctx_queue_draw(ui->ctx);
    } else {
      widget->state = ui_state_hot;
      ctx_queue_draw(ui->ctx);
    }
  }
}

int ui_button_coords(Ui *ui, float x, float y, float width, float height,
                     const char *label, int active, void *id) {
  Ctx *ctx = ui->ctx;
  if (width <= 0)
    width = ctx_text_width(ctx, label);
  if (height <= 0)
    height = em * 1.4;

  UiWidget *widget =
      ui_widget_register(ui, ui_type_button, x, y, width, height, id);
  if (widget->visible) {
    ctx_save(ctx);

    ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);

    ui_set_color(ctx, ui->style.fg);

    if (active) {
      ctx_rectangle(ctx, x, y, width, height);
      ctx_stroke(ctx);
    }

    ctx_move_to(ctx, x + width / 2, y + em);
    ctx_text(ctx, label);

    ctx_begin_path(ctx);
    ctx_rectangle(ctx, x, y, width, height);
    ctx_listen(ctx, CTX_DRAG, ui_button_drag, ui, widget->id);
    if (ui->interactive_debug) {
      ui_set_color(ctx, ui->style.interactive_bg);
      ctx_fill(ctx);
    } else {
      ctx_begin_path(ctx);
    }
    ctx_restore(ctx);
  }

  bool focused = (id == ui->focused_id);
  if ((focused && ui->activate)) {
    ui->activate = 0;
    widget->state = ui_state_default;
    ctx_queue_draw(ctx);
    return 1;
  }
  return 0;
}

void ui_end_frame(Ui *ui) {
  Ctx *ctx = ui->ctx;
  float delta_y = ui->scroll_speed * ui->height * ui->delta_ms / 1000.0;
  if (ui->focused_id) {
    UiWidget *focused = ui_widget_by_id(ui, ui->focused_id);

    if (focused) {
      if (ctx_osk_mode == 2) // when keyboard, snap to pos
      {
        if (fabs(ui->height * ui->osk_focus_target - focused->y) > 2)
          ui->scroll_offset += ui->height * ui->osk_focus_target - focused->y;
      } else {
        if (focused->y - delta_y > ui->height * 0.6) {
          ui->scroll_offset -= delta_y;
          ctx_queue_draw(ctx);
        } else if (focused->y + delta_y < ui->height * 0.15) {
          ui->scroll_offset += delta_y;
          ctx_queue_draw(ctx);
        }
      }
    }
  } else {
    if (ui->scroll_offset_target < ui->scroll_offset - delta_y) {
      ui->scroll_offset -= delta_y;
      ctx_queue_draw(ctx);
    } else if (ui->scroll_offset_target > ui->scroll_offset + delta_y) {
      ui->scroll_offset += delta_y;
      ctx_queue_draw(ctx);
    }
  }

  if (ui->active_id) {
    UiWidget *widget = ui_widget_by_id(ui, ui->active_id);
    if (widget)
      switch (widget->type) {
      case ui_type_button:
      case ui_type_none:
        break;
      case ui_type_entry:
        ctx_listen(ctx, CTX_KEY_PRESS, ui_entry_key_press, ui, widget);
        break;
      case ui_type_slider:
        ctx_listen(ctx, CTX_KEY_PRESS, ui_slider_key_press, ui, widget);
        break;
      }
  } else {
    ui_add_key_binding(ui, "up", "focus-previous", "previous focusable item");
    ui_add_key_binding(ui, "left", "focus-previous", "previous focusable item");
    ui_add_key_binding(ui, "shift-tab", "focus-previous", "previous focusable item");
    ui_add_key_binding(ui, "down", "focus-next", "next focusable item");
    ui_add_key_binding(ui, "right", "focus-next", "next focusable item");
    ui_add_key_binding(ui, "tab", "focus-next", "next focusable item");
    ui_add_key_binding(ui, "space", "activate", "activate");
    ui_add_key_binding(ui, "return", "activate", "activate");
  }
  ui_add_key_binding(ui, "control-q", "exit", "quit");

#if 1
  if (is_touch) {
    float width = ctx_width(ctx);
    float height = ctx_height(ctx);
    ui_overlay_button(ui, 0, 0, width, height * 0.12, "back", "back");
    if (ctx_osk_mode <= 1)
      ui_overlay_button(ui, 0, height - height * 0.14, width, height * 0.14,
                        "kb", "kb-show");
  }
#endif

#if defined(S0IL_NATIVE) || defined(EMSCRIPTEN)
  if (ui->fake_circle) {
    float min_dim = ctx_width(ctx);
    if (ctx_height(ctx) < min_dim)
      min_dim = ctx_height(ctx);
    ctx_save(ctx);
    ctx_rectangle(ctx, 0, 0, ctx_width(ctx), ctx_height(ctx));
    ctx_arc(ctx, ctx_width(ctx) / 2, ctx_height(ctx) / 2, min_dim / 2, 0,
            3.1415 * 2, 1);
    ctx_rgba(ctx, 0, 0, 0, 0.9);
    ctx_fill_rule(ctx, CTX_FILL_RULE_EVEN_ODD);
    ctx_fill(ctx);
    if (prev_backlight <= 99.0f) {
      ctx_rectangle(ctx, 0, 0, ctx_width(ctx), ctx_height(ctx));
      float alpha = 1.0f - (prev_backlight / 100.0f * 0.8 + 0.2);
      ctx_rgba(ctx, 0.0f, 0.0f, 0.0f, alpha);
      ctx_fill(ctx);
    }
    ctx_restore(ctx);
  }
#endif

  ui->overlay_fade -= 0.3 * ui->delta_ms / 1000.0f;
}

void ui_draw_bg(Ui *ui) {
  Ctx *ctx = ui->ctx;
  float width = ctx_width(ctx);
  float height = ctx_height(ctx);

  static float prev_red = 0;
  static float prev_green = 0;
  static float prev_blue = 0;
  if (ui->style.bg[0] != prev_red || ui->style.bg[1] != prev_green ||
      ui->style.bg[2] != prev_blue) {
    if (ui->style.bg[0] + ui->style.bg[1] + ui->style.bg[2] > 1.8) {
      ui->style.fg[0] = ui->style.fg[1] = ui->style.fg[2] = 0.0f;
      ui->style.fg[3] = 1.0f;

      ui->style.focused_bg[0] = 0.0;
      ui->style.focused_bg[1] = 0.0;
      ui->style.focused_bg[2] = 0.0;
      ui->style.focused_bg[3] = 0.1;

    } else {
      ui->style.fg[0] = ui->style.fg[1] = ui->style.fg[2] = 1.0f;
      ui->style.fg[3] = 1.0f;

      ui->style.focused_bg[0] = 1.0;
      ui->style.focused_bg[1] = 1.0;
      ui->style.focused_bg[2] = 1.0;
      ui->style.focused_bg[3] = 0.1;
    }

    prev_red = ui->style.bg[0];
    prev_green = ui->style.bg[1];
    prev_blue = ui->style.bg[2];
  }

  ctx_rectangle(ctx, 0, 0, width, height);

  ui_set_color(ctx, ui->style.bg);

  if (ui->gradient_bg) {
    ctx_linear_gradient(ctx, 0, 0, 0, height);
    ctx_gradient_add_stop(ctx, 0.0, ui->style.bg[0], ui->style.bg[1],
                          ui->style.bg[2], 1.0f);
    ctx_gradient_add_stop(ctx, 1.0, ui->style.bg2[0], ui->style.bg2[1],
                          ui->style.bg2[2], 1.0f);
  }
  ctx_fill(ctx);
  ui_set_color(ctx, ui->style.fg);
}

void ui_start_frame(Ui *ui) {
  Ctx *ctx = ui->ctx;

  static long int prev_ticks = -1;
  long int ticks = ctx_ticks();
  long int ticks_delta = ticks - prev_ticks;
  if (ticks_delta > 1000000 || prev_ticks == -1)
    ticks_delta = 10;
  prev_ticks = ticks;
  ui->delta_ms = ticks_delta / 1000;

  ui_draw_bg(ui);
  ui->width = ctx_width(ctx);
  ui->height = ctx_height(ctx);
  ui_fake_circle(ui, ui->width == ui->height);
  ui->font_size_px = ui->font_size_vh * ui->height / 100.0f;
  ui->line_height = ui->font_size_px * 1.7;
  ctx_rectangle(ctx, 0, 0, ui->width, ui->height);
  ctx_listen(ctx, CTX_DRAG, ui_pan, &ui->scroll_offset, ui);
  ctx_listen(ctx, CTX_DRAG, ui_pan, &ui->scroll_offset_target, ui);
  ctx_begin_path(ctx);
  ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_font_size(ctx, ui_get_font_size(ui));
  ctx_font(ctx, "Regular");

  ui->y = (int)(ui->scroll_offset + ui->height * 0.15);
  ui->widget_count = 0;
}

float ui_slider(Ui *ui, const char *label, float min, float max, float step,
                float value) {
  Ctx *ctx = ui->ctx;
  ctx_save(ctx);
  ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_move_to(ctx, ui->width * 0.5, ui->y + em);
  ctx_text(ctx, label);

  ui->y += em * 0.8f;
  float ret =
      ui_slider_coords(ui, UI_ID_STR(label), ui->width * 0.1, ui->y,
                       ui->width * 0.8, ui->line_height, min, max, step, value);
  ui->y += ui->line_height;
  ctx_restore(ctx);
  return ret;
}

bool ui_toggle(Ui *ui, const char *label, bool value) {
  Ctx *ctx = ui->ctx;
  if (ui->y > -2 * em && ui->y < ui->height - em) {
    ctx_move_to(ctx, ui->width * 0.5, ui->y + em);
    ctx_text(ctx, label);
  }
  ui->y += ui->line_height;
  if (ui_button_coords(ui, ui->width * 0.15, ui->y, em * 2, 0, "off",
                       value == 0, UI_ID_STR(label)))
    value = false;
  if (ui_button_coords(
          ui, ui->width * 0.50, ui->y, em * 2, 0, "on", value == 1,
          UI_ID_STR(label + 1))) // XXX hack - skipping first label char
    value = true;
  ui->y += ui->line_height;
  return value;
}

void ui_title(Ui *ui, const char *string) {
  Ctx *ctx = ui->ctx;
  float scale = 1.4f;

  if (ui->y > -em && ui->y < ui->height + em) {
    ctx_save(ctx);
    ctx_font_size(ctx, ctx_get_font_size(ctx) * scale);
    ctx_move_to(ctx, ui->width * 0.5, ui->y + em * scale);
    ctx_text(ctx, string);
    ctx_restore(ctx);
  };
  ui->y += ui->line_height * scale;
}

void ui_seperator(Ui *ui) {
  Ctx *ctx = ui->ctx;
  ctx_move_to(ctx, ui->width * 0.4, ui->y + ui->line_height * 0.3);
  ctx_rel_quad_to(ctx, ui->width * 0.2, -ui->line_height * 0.1, ui->width * 0.2,
                  -ui->line_height * 0.3);
  ctx_rel_quad_to(ctx, -ui->width * 0.1, 0.0, -ui->width * 0.2,
                  ui->line_height * 0.7);
  ctx_rel_quad_to(ctx, 0, -ui->line_height * 0.3, ui->width * 0.2,
                  -ui->line_height * 0.3);
  ctx_stroke(ctx);
  ui->y += ui->line_height;
}

void ui_text(Ui *ui, const char *string) {
  Ctx *ctx = ui->ctx;
  if (ui->y > -em && ui->y < ui->height + em) {
    ctx_move_to(ctx, ui->width * 0.5, ui->y + em);
    ctx_text(ctx, string);
  };
  ui->y += ui->line_height;
}

int ui_button(Ui *ui, const char *label) {
  return ui->y += ui->line_height,
         ui_button_coords(ui, ui->width * 0.0, ui->y - ui->line_height,
                          ui->width, ui->line_height, label, 0,
                          UI_ID_STR(label));
}

static void ui_entry_drag(CtxEvent *event, void *data1, void *data2) {
  Ui *ui = data1;
  UiWidget *widget = ui_widget_by_id(ui, data2);
  if (!widget)
    return;

  if (event->type == CTX_DRAG_PRESS) {
    event->stop_propagate = 0;
    ui_set_focus(ui, widget);
    widget->state = ui_state_default;
  } else if (event->type == CTX_DRAG_RELEASE) {
    event->stop_propagate = 0;
    if (widget->state != ui_state_lost_focus) {
      if (data2 != ui->active_id) {
        s0il_do(ui, "kb-show");
        s0il_do(ui, "activate");
      }
    }
  } else {
    if ((event->y < widget->y) || (event->x < widget->x) ||
        (event->y > widget->y + widget->height) ||
        (event->x > widget->x + widget->width)) {
      widget->state = ui_state_lost_focus;
    }
  }
}

char *ui_entry_coords(Ui *ui, void *id, float x, float y, float w, float h,
                      const char *fallback, const char *value) {
  Ctx *ctx = ui->ctx;
  UiWidget *widget = ui_widget_register(ui, ui_type_entry, x, y, w, h, id);

  bool focused = (id == ui->focused_id);
  if (focused && ui->activate) {
    if (value)
      strcpy(ui->temp_text, value);
    else {
      ui->temp_text[0] = 0;
    }
    ui->cursor_pos = strlen(ui->temp_text);
    printf("!activating\n");
    ui->active_id = widget->id;
    ui->activate = 0;
  }

  const char *to_show = value;
  if (ui->active_id == widget->id)
    to_show = &ui->temp_text[0];

  if (!(to_show && to_show[0]))
    to_show = fallback;

  if (widget->visible) {
    ctx_save(ctx);
    ctx_text_align(ctx, CTX_TEXT_ALIGN_START);
    if (ui->active_id != widget->id) {
      ctx_move_to(ctx, x + w / 5, y + em);
      ctx_font_size(ctx, em);

      if (to_show && to_show[0]) {
        if (to_show == fallback)
          ui_set_color_a(ctx, ui->style.fg, 0.5);
        ctx_text(ctx, to_show);
      }
    } else {
      char temp = ui->temp_text[ui->cursor_pos];
      float tw_pre = 0;
      float tw_selection = 0;
      int sel_bytes = 0;

      ctx_font_size(ctx, em);
      ui->temp_text[ui->cursor_pos] = 0;
      tw_pre = ctx_text_width(ctx, ui->temp_text);
      ui->temp_text[ui->cursor_pos] = temp;
      if (ui->selection_length) {
        for (int i = 0; i < ui->selection_length; i++) {
          sel_bytes += ctx_utf8_len(ui->temp_text[ui->cursor_pos + sel_bytes]);
        }
        temp = ui->temp_text[ui->cursor_pos + sel_bytes];
        ui->temp_text[ui->cursor_pos + sel_bytes] = 0;
        tw_selection = ctx_text_width(ctx, &ui->temp_text[ui->cursor_pos]);
        ui->temp_text[ui->cursor_pos + sel_bytes] = temp;
      }

      ctx_move_to(ctx, x + w / 5, y + em);

      if (to_show && to_show[0]) {
        if (to_show == fallback)
          ui_set_color_a(ctx, ui->style.fg, 0.5);
        ctx_text(ctx, to_show);
      }

      ctx_rectangle(ctx, x + w / 5 + tw_pre - 1, y + 0.1 * em, 2 + tw_selection,
                    em);
      ctx_save(ctx);
      ui_set_color(ctx, ui->style.focused_fg);
      ctx_fill(ctx);
      ctx_restore(ctx);
    }

    ctx_begin_path(ctx);
    ctx_rectangle(ctx, x, y, w, h);
    ctx_listen(ctx, CTX_DRAG, ui_entry_drag, ui, widget->id);
    if (ui->interactive_debug) {
      ui_set_color(ctx, ui->style.interactive_bg);
      ctx_fill(ctx);
    } else {
      ctx_begin_path(ctx);
    }

    ctx_restore(ctx);
  }

  if (widget->state == ui_state_commit) {
    widget->state = ui_state_default;
    s0il_do(ui, "kb-hide");
    return strdup(ui->temp_text);
  }
  return NULL;
}

int ui_entry_realloc(Ui *ui, const char *label, const char *fallback, char **strptr)
{
  const char *str = "";
  if (*strptr) str = *strptr;
  char *ret = ui_entry(ui,label,fallback,str);
  if (ret)
  {
    if (*strptr)free(*strptr);
    *strptr = ret;
    return 1;
  }
  return 0;
}

char *ui_entry(Ui *ui, const char *label, const char *fallback, const char *value) {
  char *ret = NULL;
  ctx_save(ui->ctx);
  ctx_font_size(ui->ctx, 0.75 * em);
  ctx_move_to(ui->ctx, ui->width * 0.5, ui->y + 0.1 * em);
  ctx_text(ui->ctx, label);
  if ((ret = ui_entry_coords(ui, UI_ID_STR(label), ui->width * 0.15, ui->y,
                             ui->width * 0.7, ui->line_height, fallback,
                             value))) {
  }
  ui->y += ui->line_height;
  ctx_restore(ui->ctx);

  return ret;
}

void ui_set_scroll_offset(Ui *ui, float offset) {
  ui->scroll_offset = offset;
  ui->scroll_offset_target = offset;
}

void ui_scroll_to(Ui *ui, float offset) { ui->scroll_offset_target = offset; }

void ui_destroy(Ui *ui) {
  if (!ui)
    return;
  bool owns_ctx = ui->owns_ctx;
  Ctx *ctx = ui->ctx;
  if (ui->location)
    free(ui->location);
  ui->location = NULL;
  // XXX : destroy more
  free(ui);
  if (owns_ctx)
    ctx_destroy(ctx);
}

Ctx *ui_ctx(Ui *ui) { return ui->ctx; }

void *ui_data(Ui *ui) { return ui->data; }

float ui_x(Ui *ui) {
  return 0.0f; // return ui->x;
}

float ui_y(Ui *ui) { return ui->y; }

void ui_move_to(Ui *ui, float x, float y) {
  // ui->x = x;
  ui->y = y;
}

#define DEF_SLIDER(type)                                                       \
  void ui_slider_##type(Ui *ui, const char *label, type *val, type min,        \
                        type max, type step) {                                 \
    *val = ui_slider(ui, label, min, max, step, *val);                         \
  }

DEF_SLIDER(float)
DEF_SLIDER(int)
DEF_SLIDER(uint8_t)
DEF_SLIDER(uint16_t)
DEF_SLIDER(uint32_t)
DEF_SLIDER(int8_t)
DEF_SLIDER(int16_t)
DEF_SLIDER(int32_t)
#undef DEF_SLIDER

void ui_add_key_binding(Ui *ui, const char *key, const char *action,
                        const char *label) {
  ctx_add_key_binding(ui->ctx, key, action, label, ui_cb_do, ui);
}

void s0il_view_views(Ui *ui) {
  ui_start_frame(ui);
  for (int i = ui->n_views - 1; i >= 0; i--) {
    const char *name = ui->views[i].name;
    ui_text(ui, name);
  }
  ui_end_frame(ui);
}
const char *s0il_location(Ui *ui)
{
  return ui->location;
}

#if S0IL_NATIVE || EMSCRIPTEN // simulated ctx_set_pixels - that uses a texture
uint8_t scratch[1024 * 1024 * 4];
Ctx *ctx_host(void);
void ctx_RGB565_BS_to_RGBA8(void *rasterizer, int x, const void *buf,
                            uint8_t *rgba, int count);

void ctx_set_pixels(Ctx *ctx, void *user_data, int x0, int y0, int w, int h,
                    void *buf) {
  if (ctx == NULL)
    ctx = ctx_host();
  uint8_t *src = (uint8_t *)buf;
  int in_w = w;
  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x0 + w > ctx_width(ctx)) {
    fprintf(stderr, "adjusting xbounds from %i %i\n", x0, w);
    w = ctx_width(ctx) - x0;
  }
  if (y0 + h > ctx_height(ctx)) {
    h = ctx_height(ctx) - y0;
    fprintf(stderr, "adjusting ybounds\n");
  }
  for (int i = 0; i < h; i++) {
    ctx_RGB565_BS_to_RGBA8(NULL, x0, src + i * in_w * 2, scratch + i * w * 4,
                           w);
  }
  char eid[64];
  ctx_start_frame(ctx);
  ctx_define_texture(ctx, NULL, in_w, h, in_w * 4, CTX_FORMAT_RGBA8, scratch,
                     eid);
  ctx_draw_texture(ctx, eid, x0, y0, w, h);
  // printf ("should set pixels %i %i %i %i %p\n", x, y, w, h, buf);
  ctx_end_frame(ctx);
}
#endif
