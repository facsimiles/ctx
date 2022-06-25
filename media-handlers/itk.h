#if !__COSMOPOLITAN__
#include <stdarg.h>
#include <unistd.h>
#include <math.h>

#endif
#include "ctx.h"

/* An immediate mode toolkit for ctx, ctx expects to receive full frames of
 * data to draw and by keeping knowledge of the contents of the previous frame
 * avoid re-drawing unchanged areas of the display.
 *
 * 
 *
 * TODO/BUGS:
 *   - more than one scroll per panel
 *   - horizontal scroll
 */

typedef struct _ITK ITK;
typedef struct _ITKPanel ITKPanel;

extern int _itk_key_bindings_active;
ITK *itk_new        (Ctx *ctx);
void itk_free       (ITK *itk);
void itk_reset      (ITK *itk);

ITKPanel *itk_panel_start (ITK *itk, const char *title, int x, int y, int width, int height);
void      itk_panel_end   (ITK *itk);

void itk_newline    (ITK *itk);
void itk_sameline   (ITK *itk);
void itk_seperator  (ITK *itk);
void itk_titlebar   (ITK *itk, const char *label);

void itk_label      (ITK *itk, const char *label);
void itk_labelf     (ITK *itk, const char *format, ...);


int  itk_toggle     (ITK *itk, const char *label, int in_val);

/* returns 1 when pressed */
int  itk_button     (ITK *itk, const char *label);

/* this is already modernized - it is the same as itk_toggle but gets rendered
 * differently
 */
int  itk_radio      (ITK *itk, const char *label, int set);


/* needs tweaking / expander2 it should return the state of the expander */
int  itk_expander   (ITK *itk, const char *label, int *val);

/* return newly set value if ant change */
int  itk_choice     (ITK *itk, const char *label, int in_val);
void itk_choice_add (ITK *itk, int value, const char *label);



/*
 * returns NULL if value is unchanged or a newly allocated string
 * when entry has been changed.
 *
 */
char *itk_entry (ITK *itk,
                 const char *label,
                 const char *fallback,
                 const char *val);



/* returns the new value - if it has changed due to interaction
 */
float itk_slider    (ITK *itk, const char *label, float value, double min, double max, double step);

/* these are utilities to keep some code a little bit shorter
 */
void itk_slider_int   (ITK *itk, const char *label, int *val, int min, int max, int step);
void itk_slider_float (ITK *itk, const char *label, float *val, float min, float max, float step);
void itk_slider_uint8 (ITK *itk, const char *label, uint8_t *val, uint8_t min, uint8_t max, uint8_t step);

/*  returns 1 when the value has been changed
 *
 *  this expects a string to write to maxlen is length including
 *  room for terminating \0
 *
 *  return 1 when the value has been changed
 */
int itk_entry_str_len (ITK        *itk,
                       const char *label,
                       const char *fallback,
                       char       *val,
                       int         maxlen);

/* to be called on focus changes that might take focus away from
 * edited itk_entry
 */
void itk_entry_commit (ITK *itk);

/* return new value if changed */


//void itk_choice_add (ITK *itk, int value, const char *label);
void        itk_done         (ITK *itk);
void        itk_style_color  (Ctx *ctx, const char *name);
const char *itk_style_string (const char *name);
float       itk_style_float  (char *name);
float       itk_em           (ITK *itk);

/* runs until ctx_quit itk->ctx) is called */
void itk_run_ui       (ITK *itk, int (*ui_fun)(ITK *itk, void *data), void *ui_data);


/*
   A helper function that does itk_run_ui on a ctx context that is both created
   and destroyed, this helps keep small programs tiny.

   void itk_main (int (*ui_fun)(ITK *itk, void *data), void *ui_data)
   {
     Ctx *ctx = ctx_new (-1, -1, NULL);
     ITK *itk = itk_new (ctx);
     itk_run_ui (itk, ui_fun, ui_data);
     itk_free (itk);
     ctx_destroy (ctx);
    }
 */

void itk_main         (int (*ui_fun)(ITK *itk, void *data), void *ui_data);
void itk_key_bindings (ITK *itk);

typedef struct _CtxControl CtxControl;
CtxControl *itk_focused_control (ITK *itk);
CtxControl *itk_find_control    (ITK *itk, int no);
CtxControl *itk_add_control     (ITK *itk,
                                 int type,
                                 const char *label,
                                 float x, float y,
                                 float width, float height);
void itk_set_flag               (ITK *itk, int flag, int on);


void itk_panels_reset_scroll    (ITK *itk);

void itk_ctx_settings (ITK *itk);
void itk_itk_settings (ITK *itk);
void itk_key_quit (CtxEvent *event, void *userdata, void *userdata2);

enum {
  UI_SLIDER = 1,
  UI_EXPANDER,
  UI_TOGGLE,
  UI_LABEL,
  UI_TITLEBAR,
  UI_BUTTON,
  UI_CHOICE,
  UI_ENTRY,
  UI_MENU,
  UI_SEPARATOR,
  UI_RADIO
};

enum {
  ITK_FLAG_SHOW_LABEL = (1<<0),
  ITK_FLAG_ACTIVE     = (1<<1),
  ITK_FLAG_DEFAULT    = (ITK_FLAG_SHOW_LABEL|ITK_FLAG_ACTIVE)
};
 // XXX : commit or cancel entry on focus change
 //


struct _ITKPanel{
  int   x;
  int   y;
  int   width;
  int   height;
  int   expanded;
  int   max_y;
  float scroll_start_y;
  float scroll;

  int   do_scroll_jump;
  const char *title;
};

typedef enum
{
  ITK_BG,
  ITK_FG,
  ITK_FOCUSED_BG,

  ITK_INTERACTIVE,
  ITK_INTERACTIVE_BG,

  ITK_ENTRY_CURSOR,
  ITK_ENTRY_FALLBACK,

  ITK_SCROLL_FG,
  ITK_SCROLL_BG,

  ITK_BUTTON_FG,
  ITK_BUTTON_BG,
  ITK_BUTTON_FOCUSED_BG,
  ITK_BUTTON_PRESSED_FG,
  ITK_BUTTON_PRESSED_BG,
  ITK_BUTTON_SHADOW,

  ITK_SLIDER_TEXT,
  ITK_SLIDER_CURSOR,

  ITK_LAST_COLOR
} ItkControlType;

typedef struct ITKPal{
  char id;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} IKTPal;

struct _CtxControl{
  int no;
  int ref_count;
  uint64_t flags;
  int type; /* this should be a pointer to the vfuncs/class struct
               instead - along with one optional instance data per control */
  char *label;
  void *id; /* possibly unique identifier */

  float x;
  float y;
  float width;
  float height;
  void *val;

  float value;

  char *entry_value;

  char *fallback;
  float min;
  float max;
  float step;
};

typedef struct _UiChoice  UiChoice;
struct _UiChoice
{
  int   val;
  char *label;
};

struct _ITK{
  Ctx *ctx;
  int (*ui_fun)(ITK *itk, void *data);
  void *ui_data;
  float x0;
  float y0;
  float x;
  float y;


  /// ITK ancestors.. for css
  float stored_x; // for sameline()

  float font_size;
  float width;
  float height;
  float rel_hmargin;
  float rel_vmargin;
  float label_width;

  float scale;

  float rel_ver_advance;
  float rel_baseline;
  float rel_hgap;
  float rel_hpad;
  float rel_vgap;
  float scroll_speed;

  int   return_value; // when set to 1, we return the internally held from the
                      // defining app state when the widget was drawn/intercations
                      // started.

  float slider_value; // for reporting back slider value

  int   active;  // 0 not actively editing
                 // 1 currently in edit-mode of focused widget
                 // 2 means return edited value

  int   active_entry;
  int   focus_wraparound;

  int   focus_no;
  int   focus_x;
  int   focus_y;
  int   focus_width;
  char *focus_label;

  char *entry_copy;
  int   entry_pos;
  ITKPanel *panel;

  CtxList *old_controls;
  CtxList *controls;
  CtxList *choices;
  CtxList *panels;
  int hovered_no;
  int control_no;
  int choice_active;

  int choice_no;  // the currenlt active choice if the choice context is visible (or the current control is a choice)

  int popup_x;
  int popup_y;
  int popup_width;
  int popup_height;

  char *active_menu_path;
  char *menu_path;

  uint64_t next_flags;
  void *next_id; // to pre-empt a control and get it a more unique
                 // identifier than the numeric pos

  int line_no;
  int lines_drawn;
  int light_mode;
};


#ifdef ITK_IMPLEMENTATION

void itk_begin_menu_bar (ITK *itk, const char *title)
{
  if (itk->menu_path)
    free (itk->menu_path);
  itk->menu_path = title?strdup (title):NULL;
}

void itk_begin_menu (ITK *itk, const char *title)
{
  char *tmp = malloc (strlen (title) + (itk->menu_path?strlen (itk->menu_path):0) + 2);
  sprintf (tmp, "%s/%s", itk->menu_path?itk->menu_path:"", title);
  if (itk->menu_path)
          free (itk->menu_path);
  itk->menu_path = tmp;
  if (itk_button (itk, title))
  {
     if (itk->active_menu_path) free (itk->active_menu_path);
     itk->active_menu_path = strdup (itk->menu_path);
  }; itk_sameline (itk);
}

void itk_menu_item (ITK *itk, const char *title)
{
  char *tmp = malloc (strlen (title) + (itk->menu_path?strlen (itk->menu_path):0) + 2);
  sprintf (tmp, "%s/%s", itk->menu_path?itk->menu_path:"", title);
  //fprintf (stderr, "[%s]\n", tmp);
  free (tmp);
}

void itk_end_menu (ITK *itk)
{
  if (itk->menu_path)
  {
    char *split = strrchr (itk->menu_path, '/');
    if (split) *split = 0;
  }
}

void itk_end_menu_bar (ITK *itk)
{
  itk_newline (itk);
}

static char *itk_style=NULL;

const char *itk_style_string (const char *name)
{
  if (!itk_style)
    return NULL;
  char *p = itk_style;
  static char ret[64];
  int name_len = strlen (name);
  while (p && *p)
  {
    while (*p == ' ')p++;
    if (!strncmp (p, name, name_len))
    {
      if (p[name_len]==':')
      {
        for (int i = 2; p[name_len+i] && (p[name_len+i] != ';')
                        && (p[name_len+i] != '\n'); i++)
        {
          ret[i-2]=p[name_len+i];
          ret[i-1]=0;
        }
        return ret;
      }
    }
    else
    {
      p = strchr (p, '\n');
      if (p) p++;
    }
  }
  return NULL;
}

float itk_style_float (char *name)
{
   const char *str = itk_style_string (name);
   if (str)
   {
     return atof (str);
   }
   return 0.0f;
}

void itk_style_color (Ctx *ctx, const char *name)
{
   const char *str = itk_style_string (name);
   if (str)
   {
     while (*str == ' ')str++;
     ctx_color (ctx, str);
     //ctx_stroke_source (ctx);
     //ctx_color (ctx, str);
   }
   else
   {
     ctx_rgb (ctx, 0, 0, 0); // XXX : this shows up in thumbnails
     //ctx_rgb_stroke (ctx, 1, 0, 1);
   }
}

void itk_set_font_size (ITK *itk, float font_size)
{
  itk->font_size = font_size;
}

ITK *itk_new (Ctx *ctx)
{
  ITK *itk              = calloc (sizeof (ITK), 1);
  itk->ctx              = ctx;
  //itk->panels = NULL;
  itk->focus_wraparound = 1;
  itk->scale            = 1.0;
  itk->font_size        = getenv("ITK_FONT_SIZE")?atoi(getenv("ITK_FONT_SIZE")):ctx_get_font_size(ctx);
  itk->label_width      = 0.5;
  itk->rel_vmargin      = 0.5;
  itk->rel_hmargin      = 0.5;
  itk->rel_ver_advance  = 1.2;
  itk->rel_baseline     = 0.8;
  itk->rel_hgap         = 0.5;
  itk->menu_path = strdup ("main/foo");
  itk->rel_hpad         = 0.3;
  itk->rel_vgap         = 0.2;
  itk->scroll_speed     = 1.0/8.0;
  itk->light_mode       = 1;
  ctx_queue_draw (ctx);
  if (ctx_backend_type (ctx) == CTX_BACKEND_TERM)
  {
    itk->scale     = 1.0;
    itk->font_size = 3;
    itk->rel_vgap = 0.0;
    itk->rel_ver_advance = 1.0;
    itk->rel_hgap = 0.0;
    itk->rel_hpad = 0.0;
  }
  itk->width = itk->font_size * 15;

  return itk;
}

float itk_em (ITK *itk)
{
  return itk->font_size * itk->scale;
}

void itk_free (ITK *itk)
{
  if (itk->menu_path)
    free (itk->menu_path);
  free (itk);
}

static inline void control_ref (CtxControl *control)
{
  control->ref_count ++;
}

static inline void control_unref (CtxControl *control)
{
  if (control->ref_count <= 0)
  {
    CtxControl *w = control;

    if (w->label)
      free (w->label);
    if (w->fallback)
      free (w->fallback);
    if (w->entry_value)
      free (w->entry_value);

    free (w);
    return;
  }
  control->ref_count--;
}

void control_finalize (void *control, void *foo, void *bar)
{
  control_unref (control);
}

void itk_reset (ITK *itk)
{
  Ctx *ctx = itk->ctx;
  ctx_start_frame           (ctx);

  if (itk_style)
    free (itk_style);
  unsigned char *style = NULL;
#if CTX_FONTS_FROM_FILE
  ctx_get_contents ("/tmp/itk-style", &style, NULL);
#endif
  if (style)
  {
    itk_style = (void*)style;
  }
  else
  {
    itk_style = strdup (
"wallpaper: #111\n"
//"wallpaper: #677\n"
"\n"
"itk-font-size: 32.0;\n"

"titlebar-bg:          #0007;\n"
"titlebar-fg:          #999a;\n"
"titlebar-close:       #fff9;\n"
"titlebar-focused-close: #c44;\n"
"titlebar-focused-bg:  #333b;\n"
"titlebar-focused-fg:  #ffff;\n"
"\n"
"terminal-bg:         #000f;\n"
"terminal-bg-reverse: #ddde;\n"
"terminal-active-bg:         #000b;\n"
"terminal-active-bg-reverse: #dddb;\n"
"\n"
"itk-bg:             rgba(30,40,50, 1.0);\n"
"itk-focused-bg:     rgba(0,0,0,1.0);\n"
"itk-focused-fg:     rgb(255,255,255,1.0);\n"
"itk-fg:             rgb(225,225,225);\n"
"itk-interactive:    rgb(255,5,5);\n"
"itk-interactive-bg: rgba(50,40,50,0.45);\n"
"itk-entry-cursor:   rgb(225,245,140);\n"
"itk-entry-fallback: rgb(225,245,140);\n"
"itk-button-focused-bg: rgb(60,60,160);\n"
"itk-button-shadow:  rgba(0,0,0,0.4);\n"
"itk-button-bg:      rgb(0,0,0);\n"
"itk-button-fg:      rgb(255,255,255);\n"
"itk-scroll-bg:      rgba(0,0,0,0.3),\n"
"itk-scroll-fg:      rgba(255,255,255,0.3);\n"
"itk-slider-cursor:  rgb(255,0,0);\n"
"itk-slider-text:    rgba(255,255,255,0.5);\n"
"\n"
"# light mode follows\n"
"\n"
"itk-bg: rgb(230,230,230);\n"
"itk-focused-bg: rgb(255,255,255);\n"
"itk-interactive: rgb(255,5,5);\n"
"itk-interactive-bg: rgba(255,245,230,0.5);\n"
"itk-fg: rgb(30,40,50:\n"
"itk-entry-cursor: rgb(0,0,0);\n"
"itk-entry-fallback: rgb(225,245,140);\n"
"itk-button-bg: rgb(235,235,235);\n"
"itk-button-shadow: rgba(0,0,0,0.44);\n"
"itk-button-focused-bg: rgb(255,255,25);\n"
"itk-button-fg: rgb(0,0,0);\n"
"itk-scroll-bg: rgba(0,0,0,0.1);\n"
"itk-scroll-fg: rgba(0,0,0,0.44);\n"
"itk-slider-cursor: rgb(255,0,0);\n"
"itk-slider-text: rgba(0,0,0,0.5);\n"
    );

  }

  ctx_save (ctx);
  ctx_font (ctx, "Regular");
  ctx_font_size (ctx, itk_em (itk));

  itk->next_flags = ITK_FLAG_DEFAULT;
  itk->panel      = NULL;

  while (itk->old_controls)
  {
    CtxControl *control = itk->old_controls->data;
    control_unref (control);
    ctx_list_remove (&itk->old_controls, control);
  }
  itk->old_controls = itk->controls;
  itk->controls = NULL;
  while (itk->choices)
  {
    UiChoice *choice = itk->choices->data;
    free (choice->label);
    free (choice);
    ctx_list_remove (&itk->choices, choice);
  }
  itk->control_no = 0;
}

ITKPanel *add_panel (ITK *itk, const char *label, float x, float y, float width, float height)
{
  ITKPanel *panel;
  for (CtxList *l = itk->panels; l; l = l->next)
  {
    ITKPanel *panel = l->data;
    if (!strcmp (panel->title, label))
      return panel;
  }
  panel = calloc (sizeof (ITKPanel), 1);
  panel->title = strdup (label);
  panel->x = x;
  panel->y = y;
  panel->width = width;
  panel->height = height;
  ctx_list_prepend (&itk->panels, panel);

  itk->panel = panel;
  return panel;
}

void
itk_panels_reset_scroll (ITK *itk)
{
  if (!itk || !itk->panels)
          return;
  for (CtxList *l = itk->panels; l; l = l->next)
  {
    ITKPanel *panel = l->data;
    panel->scroll = 0.0;
    panel->do_scroll_jump = 1;
  }
}


/* adds a control - should be done before the drawing of the
 * control itself - as this call might draw a highlight in
 * the background.
 *
 * This also allocats a runtime control, used for focus handling
 * and ephemreal persistance of possible interaction states -
 * useful for accesibility.
 */
CtxControl *itk_add_control (ITK *itk,
                             int type,
                             const char *label,
                             float x, float y,
                             float width, float height)
{
  CtxControl *control = calloc (sizeof (CtxControl), 1);
  float em = itk_em (itk);
  control->flags = itk->next_flags;
  itk->next_flags = ITK_FLAG_DEFAULT;
  control->label = strdup (label);
  if (itk->next_id)
  {
    control->id = itk->next_id;
    itk->next_id = NULL;
  }

  control->type = type;
  control->ref_count=0;
  control->x = x;
  control->y = y;
  control->no = itk->control_no;
  itk->control_no++;
  control->width = width;
  control->height = height;
  ctx_list_prepend (&itk->controls, control);

  if (itk->focus_no == control->no && itk->focus_no != 0)
  {
     if (itk->y - itk->panel->scroll < em * 2)
     {
        if (itk->panel->scroll != 0.0f)
        {
          itk->panel->scroll -= itk->scroll_speed * itk->panel->height * (itk->panel->do_scroll_jump?5:1);
          if (itk->panel->scroll<0.0)
            itk->panel->scroll=0.0;
          ctx_queue_draw (itk->ctx);
        }
     }
     else if (itk->y - itk->panel->scroll +  control->height > itk->panel->y + itk->panel->height - em * 2 && control->height < itk->panel->height - em * 2)
     {
          itk->panel->scroll += itk->scroll_speed * itk->panel->height * (itk->panel->do_scroll_jump?5:1);

        ctx_queue_draw (itk->ctx);
     }
     else
     {
       itk->panel->do_scroll_jump = 0;
     }

  }

  ctx_rectangle (itk->ctx, x, y, width, height);
  if (itk->focus_no == control->no &&
      control->type != UI_BUTTON)  // own-bg
  {
#if 1
    itk_style_color (itk->ctx, "itk-focused-bg");
    ctx_fill (itk->ctx);
    ctx_rectangle (itk->ctx, x, y, width, height);
    itk_style_color (itk->ctx, "itk-fg");
    ctx_line_width (itk->ctx, 2.0f);
    ctx_stroke (itk->ctx);
#endif
  }
  else
  {
    if (control->flags & ITK_FLAG_ACTIVE)
    if (control->type != UI_LABEL && // no-bg
        control->type != UI_BUTTON)  // own-bg
    {
      itk_style_color (itk->ctx, "itk-interactive-bg");
      ctx_fill (itk->ctx);
    }
  }

  switch (control->type)
  {
    case UI_SLIDER:
      control->ref_count++;
      break;
    default:
      break;
  }

  return control;
}

static void itk_text (ITK *itk, const char *text)
{
  Ctx *ctx = itk->ctx;
  itk_style_color (itk->ctx, "itk-fg");
  ctx_move_to (ctx, itk->x,  itk->y + itk_em (itk) * itk->rel_baseline);
  ctx_text (ctx, text);
  itk->x += ctx_text_width (ctx, text);
}

#if 0
static void itk_base (ITK *itk, const char *label, float x, float y, float width, float height, int focused)
{
  Ctx *ctx = itk->ctx;
  if (focused)
  {
    itk_style_color (itk->ctx, "itk-focused-bg");

    ctx_rectangle (ctx, x, y, width, height);
    ctx_fill (ctx);
  }
#if 0
  else
    itk_style_color (itk->ctx, "itk-bg");

  if (itk->line_no >= itk->lines_drawn)
  {
    ctx_rectangle (ctx, x, y, width, height);
    ctx_fill (ctx);
    itk->lines_drawn = itk->line_no -1;
  }
#endif
}
#endif

void itk_newline (ITK *itk)
{
  itk->y += itk_em (itk) * (itk->rel_ver_advance + itk->rel_vgap);
  itk->stored_x = itk->x;
  itk->x = itk->x0;
  itk->line_no++;
}

void itk_sameline (ITK *itk)
{
  itk->y -= itk_em (itk) * (itk->rel_ver_advance + itk->rel_vgap);
  itk->x = itk->stored_x;
  itk->line_no--;
}

void itk_seperator (ITK *itk)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
//itk_base (itk, NULL, itk->x0, itk->y, itk->width, em * itk->rel_ver_advance / 4, 0);
  ctx_rectangle (ctx, itk->x0 - em * itk->rel_hmargin, itk->y, itk->width - em * itk->rel_hmargin*2, em * itk->rel_ver_advance/4);
  ctx_gray (ctx, 0.5);
  ctx_fill (ctx);
  itk_newline (itk);
  itk->y -= em * itk->rel_ver_advance * 0.75;
}

void itk_label (ITK *itk, const char *label)
{
//float em = itk_em (itk);
//itk_base (itk, NULL, itk->x, itk->y, itk->width, em * itk->rel_ver_advance, 0);
  itk_text (itk, label);

  itk->x += itk->rel_hgap * itk->font_size;
  itk_newline (itk);
}

void itk_labelf (ITK *itk, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  itk_label (itk, buffer);
  free (buffer);
}

static void titlebar_drag (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  ITKPanel *panel = userdata;
  
#if 1
  //fprintf (stderr, "%d %f %f\n", event->delta_x, event->delta_y);
  panel->x += event->delta_x;
  panel->y += event->delta_y;
  if (panel->y < 0) panel->y = 0;
#else
  panel->x = event->x - panel->width / 2;
  panel->y = event->y;
#endif

  event->stop_propagate = 1;
  ctx_queue_draw (itk->ctx);
}

void itk_titlebar (ITK *itk, const char *label)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);

  //CtxControl *control = itk_add_control (itk, UI_TITLEBAR, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_DRAG, titlebar_drag, itk->panel, itk, NULL, NULL);

  ctx_begin_path (ctx);
  itk->line_no = 0;
  itk->lines_drawn = 0;
  //itk_base (itk, label, control->x, control->y, control->width - em * itk->rel_hmargin, em * itk->rel_ver_advance, itk->focus_no == control->no);
  itk_text (itk, label);
  //itk->lines_drawn = 1;

  itk_newline (itk);
}

void itk_scroll_start (ITK *itk, float height)
{
  ITKPanel *panel = itk->panel;
  Ctx *ctx = itk->ctx;
  ctx_save (ctx);
  itk->panel->scroll_start_y = itk->y;
  ctx_rectangle (ctx, itk->x0 - itk->rel_hmargin*itk_em(itk), itk->y, panel->width, panel->height - (itk->y - panel->y));
  ctx_clip (ctx);
  ctx_begin_path (ctx);
  ctx_translate (ctx, 0.0, -panel->scroll);
}

// only applies to next created
void itk_id (ITK *itk, void *id)
{
  itk->next_id = id; 
}

// only applies to next created
void itk_set_flag (ITK *itk, int flag, int on)
{
  if (on)
  {
    itk->next_flags |= flag;
  }
  else
  {
    if (itk->next_flags & flag)
      itk->next_flags -= flag;
  }
}

void itk_scroll_drag (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  ITKPanel *panel = data2;
  float scrollbar_height = panel->height - (panel->scroll_start_y - panel->y);
  float th = scrollbar_height * (scrollbar_height /  (panel->max_y-panel->scroll_start_y));
  if (th > scrollbar_height)
  {
    panel->scroll = 0;
    event->stop_propagate = 1;
    ctx_queue_draw (itk->ctx);
    return;
  }
  panel->scroll = ((event->y - panel->scroll_start_y - th / 2) / (scrollbar_height-th)) *
               (panel->max_y - panel->scroll_start_y - scrollbar_height)
          ;
  itk->focus_no = -1;
  ctx_queue_draw (itk->ctx);

  if (panel->scroll < 0) panel->scroll = 0;

  event->stop_propagate = 1;
}

void itk_scroll_end (ITK *itk)
{
  ITKPanel *panel = itk->panel;
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  ctx_restore (ctx);
  itk->panel->max_y = itk->y;

  float scrollbar_height = panel->height - (panel->scroll_start_y - panel->y);
  float scrollbar_width = em;

#if 1
  ctx_begin_path (ctx);
  ctx_rectangle (ctx, panel->x + panel->width- scrollbar_width,
                      panel->scroll_start_y,
                      scrollbar_width,
                      scrollbar_height);
  ctx_listen (ctx, CTX_DRAG, itk_scroll_drag, itk, panel);
  itk_style_color (itk->ctx, "itk-scroll-bg");
  ctx_fill (ctx);
#endif

  ctx_begin_path (ctx);
  float th = scrollbar_height * (scrollbar_height /  (panel->max_y-panel->scroll_start_y));
  if (th > scrollbar_height) th = scrollbar_height;

  ctx_rectangle (ctx, panel->x + panel->width- scrollbar_width,
                      panel->scroll_start_y +
                      (panel->scroll / (panel->max_y-panel->scroll_start_y)) * ( scrollbar_height ),
                      scrollbar_width,
                      th
                      
                      );

  itk_style_color (itk->ctx, "itk-scroll-fg");
  ctx_fill (ctx);


}

ITKPanel *itk_panel_start (ITK *itk, const char *title,
                      int x, int y, int width, int height)
{
  Ctx *ctx = itk->ctx;
  ITKPanel *panel = add_panel (itk, title, x, y, width, height);
  float em = itk_em (itk);
  itk->x0 = itk->x = panel->x + em * itk->rel_hmargin;
  itk->y0 = itk->y = panel->y;
  if (panel->width != 0)
  {
    panel->width = width;
    panel->height = height;
  }
  itk->width  = panel->width;
  itk->height = panel->height;

  itk->panel = panel;

  itk_style_color (itk->ctx, "itk-fg");
  ctx_begin_path (ctx);
  ctx_rectangle (ctx, panel->x, panel->y, panel->width, panel->height);
  ctx_line_width (ctx, 2);
  ctx_stroke (ctx);

  itk_style_color (itk->ctx, "itk-bg");
  ctx_rectangle (ctx, panel->x, panel->y, panel->width, panel->height);
  ctx_fill (ctx);

  if (title[0])
    itk_titlebar (itk, title);

  itk_scroll_start (itk, panel->height - (itk->y - panel->y));
  return panel;
}

void itk_panel_resize_drag (CtxEvent *event, void *data, void *data2)
{
  ITKPanel *panel = data;
  panel->width += event->delta_x;
  panel->height += event->delta_y;
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

void itk_panel_end (ITK *itk)
{
  Ctx *ctx = itk->ctx;
  ITKPanel *panel = itk->panel;
  float em = itk_em (itk);
  itk_scroll_end (itk);

  ctx_rectangle (ctx, panel->x + panel->width - em,
                      panel->y + panel->height - em,
                      em,
                      em);
  ctx_listen (ctx, CTX_DRAG, itk_panel_resize_drag, panel, itk);
  itk_style_color (itk->ctx, "itk-button-fg");
  ctx_begin_path (ctx);
  ctx_move_to (ctx, panel->x + panel->width,
                    panel->y + panel->height);
#if 1
  ctx_rel_line_to (ctx, -em, 0);
  ctx_rel_line_to (ctx, em, -em);
  ctx_rel_line_to (ctx, 0, em);
#endif
  ctx_fill (ctx);

  itk->panel = NULL;
}

static void itk_float_constrain (CtxControl *control, float *val)
{
  float new_val = *val;
  if (new_val < control->min) new_val = control->min;
  if (new_val > control->max) new_val = control->max;
  if (new_val > 0)
  {
     if (control->step > 0.0)
     {
       new_val = (int)(new_val / control->step) * control->step;
     }
  }
  else
  {
     if (control->step > 0.0)
     {
       new_val = -new_val;
       new_val = (int)(new_val / control->step) * control->step;
       new_val = -new_val;
     }
  }
  *val = new_val;
}
void itk_set_focus (ITK *itk, int pos);

static void itk_slider_cb_drag (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl  *control = userdata;
  float new_val;

  itk_set_focus (itk, control->no);
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
  new_val = ((event->x - control->x) / (control->width)) * (control->max-control->min) + control->min;

  itk_float_constrain (control, &new_val);

  itk->return_value = 1;
  control->value = new_val;
  itk->slider_value = new_val;
  //if (control->set_val)
  //  control->set_val (control->val, new_val, control->data);
}

float itk_slider (ITK *itk, const char *label, float value, double min, double max, double step)
{
  Ctx *ctx = itk->ctx;
  char buf[100] = "";
  float em = itk_em (itk);

  float new_x = itk->x + (itk->label_width) * itk->width;
  itk_text (itk, label);
  itk->x = new_x;

  CtxControl *control = itk_add_control (itk, UI_SLIDER, label, itk->x, itk->y, itk->width * (1.0 - itk->label_width) - em * 1.5, em * itk->rel_ver_advance);
  //control->data = data;
  //
  control->value  = value;
  control->min  = min;
  control->max  = max;
  control->step = step;

  if (itk->focus_no == control->no)
    itk_style_color (itk->ctx, "itk-focused-bg");
  else
    itk_style_color (itk->ctx, "itk-interactive-bg");
  ctx_rectangle (ctx, itk->x, itk->y, control->width, em * itk->rel_ver_advance);
  ctx_fill (ctx);
  control_ref (control);
  ctx_rectangle (ctx, itk->x, itk->y, control->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_DRAG, itk_slider_cb_drag, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);

  double fval = value;

  if (step == 1.0)
  {
    sprintf (buf, "%.0f", fval);
  }
  else
  {
    sprintf (buf, "%.3f", fval);
  }
  ctx_move_to (ctx, itk->x, itk->y + em * itk->rel_baseline);
  itk_style_color (itk->ctx, "itk-slider-text");
  ctx_text (ctx, buf);

  float rel_val = ((fval) - min) / (max-min);
  itk_style_color (itk->ctx, "itk-slider-cursor");
  ctx_rectangle (ctx, itk->x + control->width * rel_val, itk->y, em/8, control->height);
  ctx_fill (ctx);
  ctx_rectangle (ctx, itk->x, itk->y + em*5/6, control->width, em/8);
  ctx_fill (ctx);

  itk->x += (1.0 - itk->label_width) * itk->width;
  itk_newline (itk);

  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return itk->slider_value;
  }
  return value;
}

void itk_slider_float (ITK *itk, const char *label, float *val, float min, float max, float step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_int (ITK *itk, const char *label, int *val, int min, int max, int step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_double (ITK *itk, const char *label, double *val, double min, double max, double step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_uint8 (ITK *itk, const char *label, uint8_t *val, uint8_t min, uint8_t max, uint8_t step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_uint16 (ITK *itk, const char *label, uint16_t *val, uint16_t min, uint16_t max, uint16_t step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_uint32 (ITK *itk, const char *label, uint32_t *val, uint32_t min, uint32_t max, uint32_t step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_int8 (ITK *itk, const char *label, int8_t *val, int8_t min, int8_t max, int8_t step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_int16 (ITK *itk, const char *label, int16_t *val, int16_t min, int16_t max, int16_t step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

void itk_slider_int32 (ITK *itk, const char *label, int32_t *val, int32_t min, int32_t max, int32_t step)
{
  *val = itk_slider (itk, label, *val, min, max, step);
}

CtxControl *itk_find_control (ITK *itk, int no)
{
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == no)
     return control;
  }
  return NULL;
}


void itk_entry_commit (ITK *itk)
{
  if (itk->active_entry<0)
     return;

  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == itk->active_entry)
    {
      switch (control->type)
      {
        case UI_ENTRY:
         if (itk->entry_copy)
         {
  //fprintf (stderr, "ec %i %s\n", itk->active_entry, itk->entry_copy);
  //         strcpy (control->val, itk->entry_copy);
 //          free (itk->entry_copy);
 //          itk->entry_copy = NULL;
           itk->active = 2;
           ctx_queue_draw (itk->ctx);
         }
      }
      return;
    }
  }
}

void entry_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  event->stop_propagate = 1;

  if (itk->active)
  {
    itk_entry_commit (itk);
  }
  else
  {
    itk->entry_copy = strdup (control->val);
    itk->entry_pos  = strlen (control->val);
    itk->active     = 1;
    itk->active_entry = control->no;
  }

  itk_set_focus (itk, control->no);
  ctx_queue_draw (event->ctx);
}


char *itk_entry (ITK        *itk,
                 const char *label,
                 const char *fallback,
                 const char *in_val)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  float new_x = itk->x + itk->label_width * itk->width;

  float ewidth = itk->width * (1.0 - itk->label_width);

  if (label[0]) {
    itk_text (itk, label);
    itk->x = new_x;
  }
  else
  {
    ewidth = itk->width;
  }
  CtxControl *control = itk_add_control (itk, UI_ENTRY, label, itk->x, itk->y, ewidth, em * itk->rel_ver_advance);
  control->entry_value = strdup (in_val);
  if (fallback)
    control->fallback = strdup (fallback);
  control->ref_count++;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);

  if (control->flags & ITK_FLAG_ACTIVE)
  {
    ctx_listen_with_finalize (ctx, CTX_CLICK, entry_clicked, control, itk, control_finalize, NULL);
    control_ref (control);
  }

  ctx_begin_path (ctx);
  ctx_move_to (ctx, itk->x, itk->y + em * itk->rel_baseline);
  if (itk->active &&
      itk->entry_copy && itk->focus_no == control->no)
  {
    int backup = itk->entry_copy[itk->entry_pos];
    char buf[4]="|";
    itk->entry_copy[itk->entry_pos]=0;
    itk_style_color (itk->ctx, "itk-interactive");
    ctx_text (ctx, itk->entry_copy);
    itk_style_color (itk->ctx, "itk-entry-cursor");
    ctx_text (ctx, buf);

    buf[0]=backup;
    if (backup)
    {
      itk_style_color (itk->ctx, "itk-interactive");
      ctx_text (ctx, buf);
      ctx_text (ctx, &itk->entry_copy[itk->entry_pos+1]);
      itk->entry_copy[itk->entry_pos] = backup;
    }
  }
  else
  {
    if (in_val[0])
    {
      if (control->flags & ITK_FLAG_ACTIVE)
        itk_style_color (itk->ctx, "itk-interactive");
      else
        itk_style_color (itk->ctx, "itk-fg");
      ctx_text (ctx, in_val);
    }
    else
    {
      if (control->fallback)
      {
        itk_style_color (itk->ctx, "itk-entry-fallback");
        ctx_text (ctx, control->fallback);
      }
    }
  }
  itk->x += ewidth;
  itk_newline (itk);

  if (itk->active == 2 && control->no == itk->active_entry)
  {
    itk->active = 0;
    itk->active_entry = -1;
    char *copy = itk->entry_copy;
    itk->entry_copy = NULL;
    return copy;
  }
  return NULL;
}

int itk_entry_str_len (ITK *itk, const char *label, const char *fallback, char *val, int maxlen)
{
   char *new_val;
   if ((new_val = itk_entry (itk, label, fallback, val)))
   {
      if ((int)strlen (new_val) > maxlen -1)
        new_val[maxlen-1]=0;
      strcpy (val, new_val);
      free (new_val);
      return 1;
   }
   return 0;
}

void toggle_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  itk->return_value = 1; // reusing despite name
  *val = (*val)?0:1;
  event->stop_propagate = 1;
  itk_set_focus (itk, control->no);
  ctx_queue_draw (event->ctx);
}

int itk_toggle_deprecated (ITK *itk, const char *label, int *val)
{
  int old_val = *val;
  int new_val = itk_toggle (itk, label, old_val);
  if (new_val !=old_val)
  {
    *val = new_val;
    return 1;
  }
  return 0;
}

static void button_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
  itk_set_focus (itk, control->no);
  itk->return_value = 1;
}

int itk_toggle (ITK *itk, const char *label, int input_val)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  float width = ctx_text_width (ctx, label) + em * 1 + em * itk->rel_hpad;
  CtxControl *control = itk_add_control (itk, UI_TOGGLE, label, itk->x, itk->y, width, em * itk->rel_ver_advance);
 // itk_base (itk, label, itk->x, itk->y, width, em * itk->rel_ver_advance, itk->focus_no == control->no);

  itk_style_color (itk->ctx, "itk-interactive");

  ctx_begin_path (ctx);
  ctx_rectangle (ctx, itk->x + em * 0.1, itk->y + em * 0.1, em, em);
  ctx_line_width (ctx, em * 0.07);
  ctx_stroke (ctx);

  if (input_val == 1)
  {
    ctx_move_to (ctx, itk->x + em * 0.3, itk->y + em * 0.6);
    ctx_line_to (ctx, itk->x + em * 0.6, itk->y + em * 0.9);
    ctx_line_to (ctx, itk->x + em * 0.9, itk->y + em * 0.3);
  //ctx_line_width (ctx, em * 0.1);
    ctx_stroke (ctx);
  }

  ctx_move_to (ctx, itk->x + em * 1 + em * itk->rel_hpad,  itk->y + em * itk->rel_baseline);
  itk_style_color (itk->ctx, "itk-fg");
  ctx_text (ctx, label);

  control->type = UI_TOGGLE;
  //control->val = val;
  control_ref (control);
  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);
  itk->x += width;

  itk->x += itk->rel_hgap * em;
  itk_newline (itk);

  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return !input_val;
  }
  return input_val;
}


int itk_radio (ITK *itk, const char *label, int set)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  float width = ctx_text_width (ctx, label) + em * 1 + em * itk->rel_hpad;
  CtxControl *control = itk_add_control (itk, UI_RADIO, label, itk->x, itk->y, width, em * itk->rel_ver_advance);
//  itk_base (itk, label, itk->x, itk->y, width, em * itk->rel_ver_advance, itk->focus_no == control->no);

  itk_style_color (itk->ctx, "itk-interactive");
  ctx_begin_path (ctx);
  ctx_arc (ctx, itk->x + em * 0.55, itk->y + em * 0.57, em * 0.4, 0.0, 6.3, 0);
  ctx_close_path (ctx);
  ctx_line_width (ctx, em * 0.07);
  ctx_stroke (ctx);

  if (set)
  {
    ctx_arc (ctx, itk->x + em * 0.55, itk->y + em * 0.57, em * 0.2, 0.0, 6.3, 0);
    ctx_fill (ctx);
  }

  ctx_move_to (ctx, itk->x + em * 1 + em * itk->rel_hpad,  itk->y + em * itk->rel_baseline);
  itk_style_color (itk->ctx, "itk-fg");
  ctx_text (ctx, label);

  control->type = UI_RADIO;
  control_ref (control);
  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);
  itk->x += width;

  itk->x += itk->rel_hgap * em;
  itk_newline (itk);
  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return 1;
  }
   return 0;
}

void expander_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  *val = (*val)?0:1;
  itk_set_focus (itk, control->no);
  ctx_queue_draw (event->ctx);
}

int itk_expander (ITK *itk, const char *label, int *val)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  CtxControl *control = itk_add_control (itk, UI_EXPANDER, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  control->val = val;

  //control_ref (control);
  control_ref (control);
  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, toggle_clicked, control, itk, control_finalize, NULL);
  //itk_style_color (itk->ctx, "itk-interactive-bg");
  //ctx_fill (ctx);

  ctx_begin_path (ctx);
  {
     //itk_base (itk, label, control->x, control->y, control->width, em * itk->rel_ver_advance, itk->focus_no == control->no);
     itk_style_color (itk->ctx, "itk-interactive");
     if (*val)
     {
       ctx_move_to (ctx, itk->x, itk->y);

       ctx_rel_move_to (ctx, em*0.1, em*0.1);
       ctx_rel_line_to (ctx, em*0.8, 0);
       ctx_rel_line_to (ctx, -0.4*em, em*0.8);
       ctx_fill (ctx);
     }
     else
     {
       ctx_move_to (ctx, itk->x, itk->y);
       ctx_rel_move_to (ctx, em*0.1, em*0.1);
       ctx_rel_line_to (ctx, 0, em*0.8);
       ctx_rel_line_to (ctx, em*0.8, -0.4*em);
       ctx_close_path (ctx);
       ctx_line_width (ctx, em * 0.07);
       ctx_stroke (ctx);
     }
     itk->x += em * (1 + itk->rel_hpad);
     itk_text (itk, label);
  }

  itk_newline (itk);
  return *val;
}

int itk_button (ITK *itk, const char *label)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  float width = ctx_text_width (ctx, label) + em * itk->rel_hpad * 2;
  CtxControl *control = itk_add_control (itk, UI_BUTTON, label, itk->x, itk->y, width, em * itk->rel_ver_advance);

  itk_style_color (itk->ctx, "itk-button-shadow");
  ctx_begin_path (ctx);
  ctx_round_rectangle (ctx, itk->x + em * 0.1, itk->y + em * 0.1, width, em * itk->rel_ver_advance, em*0.33);
  ctx_fill (ctx);

#if 0
  itk_style_color (itk->ctx, "itk-interactive");
  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_line_width (ctx, 1);
  ctx_stroke (ctx);
#endif

  {
    float px = ctx_pointer_x (itk->ctx);
    float py = ctx_pointer_y (itk->ctx);
    if (px >= control->x && px <= control->x + control->width &&
        py >= control->y && py <= control->y + control->height)
    {
      itk_style_color (itk->ctx, "itk-button-hover-bg");
    }
  else
    {
  if (itk->focus_no == control->no)
    itk_style_color (itk->ctx, "itk-button-focused-bg");
  else
    itk_style_color (itk->ctx, "itk-interactive-bg");
  }
  }

  ctx_round_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance, em * 0.33);
  ctx_fill (ctx);


  itk_style_color (itk->ctx, "itk-button-fg");
  ctx_move_to (ctx, itk->x + em * itk->rel_hpad,  itk->y + em * itk->rel_baseline);
  ctx_text (ctx, label);

  control_ref (control);
  control->type = UI_BUTTON;
  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);
  itk->x += width;
  itk->x += itk->rel_hgap * em;

  itk_newline (itk);
  if (control->no == itk->focus_no && itk->return_value)
  {
    itk->return_value = 0;
    ctx_queue_draw (ctx);
    return 1;
  }
  return 0;
}

static void itk_choice_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  itk->choice_active = 1;
  itk->choice_no = control->value;
  itk_set_focus (itk, control->no);
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

int itk_choice (ITK *itk, const char *label, int val)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);

  float new_x = itk->x + itk->width * (itk->label_width);
  itk_text (itk, label);
  itk->x = new_x;

  itk_style_color (itk->ctx, "itk-bg");
  CtxControl *control = itk_add_control (itk, UI_CHOICE, label, itk->x, itk->y, itk->width * (1.0-itk->label_width), em * itk->rel_ver_advance);
  control->value = val;

  control_ref (control);
  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, itk_choice_clicked, control, itk, control_finalize, NULL);

  ctx_begin_path (ctx);
  if (itk->focus_no == control->no)
    itk_style_color (itk->ctx, "itk-focused-bg");
  else
    itk_style_color (itk->ctx, "itk-interactive-bg");
  ctx_rectangle (ctx, itk->x, itk->y, control->width, em * itk->rel_ver_advance);
  ctx_fill (ctx);

  itk_newline (itk);
  if (control->no == itk->focus_no)
  {
    if (!itk->choice_active)
    {
      itk->choice_no = val;
    }
    else
    {
      control->value = val;
    }
    itk->popup_x = control->x;
    itk->popup_y = control->y + (itk->panel?-itk->panel->scroll:0);
    itk->popup_width = control->width;
    itk->popup_height = control->height;
    if (itk->return_value)
    {
      itk->return_value = 0;
      return itk->choice_no;
    }
  }

  return val;
}

void itk_choice_add (ITK *itk, int value, const char *label)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  CtxControl *control = itk->controls->data;
  {
    if (((int)control->value) == value)
    {
      ctx_move_to (ctx, itk->x + itk->label_width * itk->width
                      ,
                      itk->y + em * itk->rel_baseline  - em * (itk->rel_ver_advance + itk->rel_vgap));
      ctx_rgb (ctx, 1, 0, 0);
      ctx_text (ctx, label);
    }
  }
  if (control->no == itk->focus_no)
  {
     UiChoice *choice= calloc (sizeof (UiChoice), 1);
     choice->val = value;
     choice->label = strdup (label);
     ctx_list_prepend (&itk->choices, choice);
  }
}


void itk_set_focus (ITK *itk, int pos)
{
   if (itk->focus_no != pos)
   {
     itk->focus_no = pos;
     if (itk->focus_label){
       free (itk->focus_label);
       itk->focus_label = NULL;
     }

     int n_controls = ctx_list_length (itk->controls);
     CtxList *iter = ctx_list_nth (itk->controls, n_controls-itk->focus_no-1);
     if (iter)
     {
       CtxControl *control = iter->data;
       if (control->label)
         itk->focus_label = strdup (control->label);
     }
     itk_entry_commit (itk);
     ctx_queue_draw (itk->ctx);
   }
}

CtxControl *itk_hovered_control(ITK *itk)
{
  float px = ctx_pointer_x (itk->ctx);
  float py = ctx_pointer_y (itk->ctx);
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (px >= control->x && px <= control->x + control->width &&
        py >= control->y && py <= control->y + control->height)
    {
      return control;
    }
  }
  return NULL;
}

CtxControl *itk_focused_control(ITK *itk)
{
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == itk->focus_no)
    {
      if (itk->focus_label)
      {
        if (control->label && !strcmp (itk->focus_label, control->label))
          return control;
      }
      else
        return control;
    }
  }

  if (itk->focus_label)
  {
    for (CtxList *l = itk->controls; l; l=l->next)
    {
      CtxControl *control = l->data;
      if (control->label && !strcmp (itk->focus_label, control->label))
      {
         itk->focus_no = control->no;
         ctx_queue_draw (itk->ctx);
         free (itk->focus_label);
         itk->focus_label = NULL;
         return control;
      }
    }
    free (itk->focus_label);
    itk->focus_label = NULL;
  }

  return NULL;
}

#define ITK_DIRECTION_PREVIOUS  -1
#define ITK_DIRECTION_NEXT       1
#define ITK_DIRECTION_LEFT       2
#define ITK_DIRECTION_RIGHT      3
#define ITK_DIRECTION_UP         4
#define ITK_DIRECTION_DOWN       5

void itk_focus (ITK *itk, int dir)
{
   itk_entry_commit (itk);
   if (itk->focus_no < 0)
   {
     itk->focus_no = 0;
     return;
   }

   if (dir == ITK_DIRECTION_PREVIOUS ||
       dir == ITK_DIRECTION_NEXT)
   {
     itk->focus_no += dir;
     if (itk->focus_label){
       free (itk->focus_label);
       itk->focus_label = NULL;
     }

     int n_controls = ctx_list_length (itk->controls);
     CtxList *iter = ctx_list_nth (itk->controls, n_controls-itk->focus_no-1);
     if (iter)
     {
       CtxControl *control = iter->data;
       if (control->label)
       {
         itk->focus_label = strdup (control->label);
       }
     }
     else
     {
       if (itk->focus_wraparound)
       {
         if (itk->focus_no > 1)
           itk->focus_no = 0;
         else
           itk->focus_no = itk->control_no - 1;
       }
       else
       {
         if (itk->focus_no <= 1)
           itk->focus_no = 0;
         else
           itk->focus_no = itk->control_no - 1;
       }
     }
     // XXX no control means inifinie loop?
     CtxControl *control =
             itk_focused_control (itk);
#if 1
     if (!control || 
         !(control->flags & ITK_FLAG_ACTIVE)){
       itk_focus (itk, dir);
     }
#endif
   }
  else 
  {
    /* this implements the non-inner element portions of:
     *   https://drafts.csswg.org/css-nav-1/#find-the-shortest-distance
     *
     * validity is determined by the centers of the items.
     */

    CtxControl *control = itk_focused_control (itk);
    CtxControl *best = control;

    if (!best)
       best = itk->controls->data;

    float best_dist = 10000000000.0;
    {
      //float mid_ref_x = control->x + control->width/2;
      //float mid_ref_y = control->y + control->height/2;
      for (CtxList *iter = itk->controls; iter; iter=iter->next)
      {
        CtxControl *candidate = iter->data;
        //float mid_cand_x = candidate->x + candidate->width/2;
        //float mid_cand_y = candidate->y + candidate->height/2;

        int valid = 0;
        if (candidate != control)
        switch (dir)
        {
          case ITK_DIRECTION_DOWN:
            valid = candidate->y > control->y;
            break;
          case ITK_DIRECTION_UP:
            valid = candidate->y < control->y;
            break;
          case ITK_DIRECTION_LEFT:
            valid = candidate->x < control->x;
            break;
          case ITK_DIRECTION_RIGHT:
            valid = candidate->x > control->x;
            break;
        }

        if (valid)
        {
          float cand_coord[2]={0.f,0.f};
          float control_coord[2]={0.f,0.f};

          float overlap = 0.0f;
          float orthogonalSize = 1.0f;
          float orthogonalBias = 1.0f;
          float orthogonalWeight = 1.0f;
          float alignWeight = 5.0f;


        switch (dir)
        {
          case ITK_DIRECTION_DOWN:
            control_coord[1] = control->y + control->height;
            cand_coord[1]    = candidate->y;
            orthogonalSize   = control->width;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 2.0f;

            if (candidate->x + candidate->width < control->x)
            {
////    ---------- control
///  --            candidate
              cand_coord[0]    = candidate->x + candidate->width;
              control_coord[0] = control->x;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///  ------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap = (candidate->x+candidate->width) - (control->x);
            }
            else if (candidate->x > control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///       ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///   --------------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap          = control->width;
            }
            else if (candidate->x > control->x &&
                     candidate->x < control->x + control->width &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///         ----------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = control->x + control->width -
                                 candidate->x;
            }
            else if (candidate->x > control->x + control->width)
            {
///     ----------
///                  ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = control->x + control->width;
            }
            else
            {
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }

            break;
          case ITK_DIRECTION_UP:

            control_coord[1] = control->y;
            cand_coord[1]    = candidate->y + candidate->height;
            orthogonalSize   = control->width;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 2.0f;

            if (candidate->x + candidate->width < control->x)
            {
////    ---------- control
///  --            candidate
              cand_coord[0]    = candidate->x + candidate->width;
              control_coord[0] = control->x;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///  ------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap = (candidate->x+candidate->width) - (control->x);
            }
            else if (candidate->x > control->x &&
                     candidate->x + candidate->width < control->x + control->width)
            {
///     ----------
///       ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }
            else if (candidate->x < control->x &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///   --------------
              cand_coord[0]    = control->x;
              control_coord[0] = control->x;
              overlap          = control->width;
            }
            else if (candidate->x > control->x &&
                     candidate->x < control->x + control->width &&
                     candidate->x + candidate->width > control->x + control->width)
            {
///     ----------
///         ----------
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = control->x + control->width -
                                 candidate->x;
            }
            else if (candidate->x > control->x + control->width)
            {
///     ----------
///                  ------
              cand_coord[0]    = candidate->x;
              control_coord[0] = control->x + control->width;
            }
            else
            {
              cand_coord[0]    = candidate->x;
              control_coord[0] = candidate->x;
              overlap          = candidate->width;
            }

            break;
          case ITK_DIRECTION_LEFT:
            control_coord[0] = control->x;
            cand_coord[0]    = candidate->x + candidate->width;
            orthogonalSize   = control->height;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 30.0f;

            if (candidate->y + candidate->height < control->y)
            {
////    ---------- control
///  --            candidate
              cand_coord[1]    = candidate->y + candidate->height;
              control_coord[1] = control->y;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///  ------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = (candidate->y+candidate->height) - (control->y);
            }
            else if (candidate->y > control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///       ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = candidate->height;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///   --------------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = control->height;
            }
            else if (candidate->y > control->y &&
                     candidate->y < control->y + control->height &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///         ----------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = control->y + control->height - candidate->y;
            }
            else if (candidate->y > control->y + control->height)
            {
///     ----------
///                  ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = control->y + control->height;
            }
            else
            {
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap          = candidate->height;
            }




            break;
          case ITK_DIRECTION_RIGHT:
            control_coord[0] = control->x + control->width;
            cand_coord[0]    = candidate->x;
            orthogonalSize   = control->height;
            orthogonalBias   = orthogonalSize / 2.0f;
            orthogonalWeight = 30.0f;

            if (candidate->y + candidate->height < control->y)
            {
////    ---------- control
///  --            candidate
              cand_coord[1]    = candidate->y + candidate->height;
              control_coord[1] = control->y;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///  ------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = (candidate->y+candidate->height) - (control->y);
            }
            else if (candidate->y > control->y &&
                     candidate->y + candidate->height < control->y + control->height)
            {
///     ----------
///       ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = candidate->height;
            }
            else if (candidate->y < control->y &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///   --------------
              cand_coord[1]    = control->y;
              control_coord[1] = control->y;
              overlap = control->height;
            }
            else if (candidate->y > control->y &&
                     candidate->y < control->y + control->height &&
                     candidate->y + candidate->height > control->y + control->height)
            {
///     ----------
///         ----------
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap = control->y + control->height - candidate->y;
            }
            else if (candidate->y > control->y + control->height)
            {
///     ----------
///                  ------
              cand_coord[1]    = candidate->y;
              control_coord[1] = control->y + control->height;
            }
            else
            {
              cand_coord[1]    = candidate->y;
              control_coord[1] = candidate->y;
              overlap          = candidate->height;
            }

            break;
        }

        float displacement =  0.0f;

        switch (dir)
        {
          case ITK_DIRECTION_DOWN:
          case ITK_DIRECTION_UP:
            displacement = (fabsf(cand_coord[0]-control_coord[0]) +
                    orthogonalBias) * orthogonalWeight;
            break;
          case ITK_DIRECTION_LEFT:
          case ITK_DIRECTION_RIGHT:
            displacement = (fabsf(cand_coord[1]-control_coord[1]) +
                    orthogonalBias) * orthogonalWeight;
            break;
        }

        float alignBias = overlap / orthogonalSize;
        float alignment = alignWeight * alignBias;

        float euclidian = hypotf (cand_coord[0]-control_coord[0],
                                  cand_coord[1]-control_coord[1]);


        float dist = euclidian  + displacement - alignment - sqrtf(sqrtf(overlap));
        // here we deviate from the algorithm - giving a smaller bonus to overlap
        // to ensure more intermediate small ones miht be chosen
        //float dist = euclidian  + displacement - alignment - sqrtf(overlap);
          if (dist <= best_dist)
          {
             best_dist = dist;
             best = candidate;
          }
        }
      }
    }

    itk_entry_commit (itk);
    itk_set_focus (itk, best->no);
  }
}

void itk_key_tab (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  itk_focus (itk, ITK_DIRECTION_NEXT);

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void itk_key_shift_tab (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  itk_focus (itk, ITK_DIRECTION_PREVIOUS);

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void itk_key_return (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  if (control->no == itk->focus_no)
  {
    switch (control->type)
    {
      case UI_CHOICE:
       {
          itk->choice_active = !itk->choice_active;
       }
       break;
      case UI_ENTRY:
       {
         if (itk->active)
         {
           itk_entry_commit (itk);
         }
         else
         {
           itk->entry_copy = strdup (control->entry_value);
           itk->entry_pos = strlen (itk->entry_copy);
           itk->active = 1;
           itk->active_entry = control->no;
         }
       }
       break;
      case UI_SLIDER:
        if (itk->active)
        {
          itk->active = 0;
        }
        else
        {
          itk->active = 1;
        }
        break;
      case UI_TOGGLE:
          itk->return_value=1;
          break;
      case UI_EXPANDER:
        {
          int *val = control->val;
          *val = !(*val);
        }
        break;
      case UI_RADIO:
      case UI_BUTTON:
        {
          itk->return_value=1;
        }
        break;
    }
  }
  event->stop_propagate=1;
  ctx_queue_draw (event->ctx);
}

void itk_key_left (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;

  if (itk->active)
  {
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_CHOICE:
      itk_key_return (event, data, data2);
      break;
    case UI_ENTRY:
      itk->entry_pos --;
      if (itk->entry_pos < 0)
        itk->entry_pos = 0;
      break;
    case UI_SLIDER:
      {
        double val = control->value;
        val -= control->step;
        if (val < control->min)
          val = control->min;

        itk->slider_value = control->value = val;
        itk->return_value = 1;
      }
      break;
  }
  }
  else
  {
    itk_focus (itk, ITK_DIRECTION_LEFT);
  }

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void itk_key_right (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;

  if (itk->active)
  {
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_BUTTON:
    case UI_CHOICE:
    case UI_RADIO:
      // itk_key_return (event, data, data2);
      break;
    case UI_ENTRY:
     itk->entry_pos ++;
     if (itk->entry_pos > (int)strlen (itk->entry_copy))
       itk->entry_pos = strlen (itk->entry_copy);
      break;
    case UI_SLIDER:
      {
        double val = control->value;
        val += control->step;
        if (val > control->max) val = control->max;

        itk->slider_value = control->value = val;
        itk->return_value = 1;
      }
      break;
  }
  }
  else
  {
    itk_focus (itk, ITK_DIRECTION_RIGHT);
  }

  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void itk_key_up (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);

  if (control && control->type == UI_CHOICE && itk->choice_active)
  {
    int old_val = itk->choice_no;
    int prev_val = old_val;
    for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
    {
      UiChoice *choice = l->data;
      if (choice->val == old_val)
      {
        itk->choice_no = prev_val;
        itk->return_value = 1;
        l=NULL;
      }
      prev_val = choice->val;
    }
  }
  else if (control)
  {
    itk_focus (itk, ITK_DIRECTION_UP);
  }
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

void itk_key_down (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (control && control->type == UI_CHOICE && itk->choice_active)
  {
    {
    int old_val = itk->choice_no;
    for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
    {
      UiChoice *choice = l->data;
      if (choice->val == old_val)
      {
         if (l->next)
         {
           l = l->next;
           choice = l->data;
           itk->choice_no = choice->val;
           itk->return_value = 1;
         }
      }
    }
    }
  }
  else if (control)
  {
    itk_focus (itk, ITK_DIRECTION_DOWN);
  }
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}


void itk_key_backspace (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  if (!itk->entry_copy) return;
  if (!itk->active) return;

  switch (control->type)
  {
    case UI_ENTRY:
     {
       if (itk->active && itk->entry_pos > 0)
       {
         memmove (&itk->entry_copy[itk->entry_pos-1], &itk->entry_copy[itk->entry_pos],
                   strlen (&itk->entry_copy[itk->entry_pos] )+ 1);
         itk->entry_pos --;
       }
     }
     break;
  }
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void itk_key_delete (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  if (!itk->entry_copy) return;
  if (!itk->active) return;
  if ((int)strlen (itk->entry_copy) > itk->entry_pos)
  {
    itk_key_right (event, data, data2);
    itk_key_backspace (event, data, data2);
  }
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void itk_key_unhandled (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata;

  if (itk->active && itk->entry_copy)
    {
      const char *str = event->string;
      if (!strcmp (str, "space"))
        str = " ";

      if (ctx_utf8_strlen (str) == 1)
      {

      char *tmp = malloc (strlen (itk->entry_copy) + strlen (str) + 1);

      char *rest = strdup (&itk->entry_copy[itk->entry_pos]);
      itk->entry_copy[itk->entry_pos]=0;

      sprintf (tmp, "%s%s%s", itk->entry_copy, str, rest);
      free (rest);
      itk->entry_pos+=strlen(str);
      free (itk->entry_copy);
      itk->entry_copy = tmp;
      ctx_queue_draw (event->ctx);
      }
      else
      {
              fprintf (stderr, "unhandled %s\n", str);
      }
    }
  event->stop_propagate = 1;
}

void itk_key_bindings (ITK *itk)
{
  Ctx *ctx = itk->ctx;
  ctx_add_key_binding (ctx, "tab", NULL, "focus next",            itk_key_tab,       itk);
  ctx_add_key_binding (ctx, "shift-tab", NULL, "focus previous",      itk_key_shift_tab, itk);

  ctx_add_key_binding (ctx, "up", NULL, "spatial focus up",        itk_key_up,    itk);
  ctx_add_key_binding (ctx, "down", NULL, "spatical focus down",   itk_key_down,  itk);
  ctx_add_key_binding (ctx, "right", NULL, "spatial focus right",  itk_key_right, itk);
  ctx_add_key_binding (ctx, "left", NULL, "spatial focus left",    itk_key_left,  itk);

  ctx_add_key_binding (ctx, "return", NULL, "enter/edit", itk_key_return,    itk);
  ctx_add_key_binding (ctx, "backspace", NULL, NULL,    itk_key_backspace, itk);
  ctx_add_key_binding (ctx, "delete", NULL, NULL,       itk_key_delete,    itk);
  ctx_add_key_binding (ctx, "any", NULL, NULL,          itk_key_unhandled, itk);
}

static void itk_choice_set (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  itk->choice_no = (size_t)(data2);
  itk->return_value = 1;
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

void ctx_event_block (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  itk->choice_active = 0;
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

void itk_done (ITK *itk)
{
  Ctx *ctx = itk->ctx;

  CtxControl *control = itk_focused_control (itk);
#if 0
  CtxControl *hovered_control = itk_hovered_control (itk);
  int hovered_no = hovered_control ? hovered_control->no : -1;

  if (itk->hovered_no != hovered_no)
  {
    itk->hovered_no = hovered_no;
    ctx_queue_draw (ctx, 1);
  }
#endif

  float em = itk_em (itk);
  if (!control){
    ctx_restore (ctx);
    return;
  }

  if (control->type == UI_CHOICE && itk->choice_active)
  {
    float x = control->x;
    float y = itk->popup_y;

    if (y + (ctx_list_length (itk->choices) + 0.5) * em > ctx_height (ctx))
    {
      y = itk->popup_y - (ctx_list_length (itk->choices) - 0.5) * em;
    }

    itk_style_color (itk->ctx, "itk-focused-bg");
    ctx_rectangle (ctx, x,
                        y,
                        itk->popup_width,
                        em * (ctx_list_length (itk->choices) + 0.5));
    ctx_fill (ctx);
    itk_style_color (itk->ctx, "itk-fg");
    ctx_rectangle (ctx, x,
                        y,
                        itk->popup_width,
                        em * (ctx_list_length (itk->choices) + 0.5));
    ctx_line_width (ctx, 2);
    ctx_stroke (ctx);

    int no = 0;

    ctx_rectangle (ctx, 0,0,ctx_width(ctx), ctx_height(ctx));
    ctx_listen (ctx, CTX_CLICK, ctx_event_block, itk, NULL);
    ctx_begin_path (ctx);

    ctx_list_reverse (&itk->choices);
    for (CtxList *l = itk->choices; l; l = l->next, no++)
    {
      UiChoice *choice = l->data;
      ctx_rectangle (ctx, x,
                          y + em * (no),
                          em * 4,
                          em * 1.5);
      ctx_listen (ctx, CTX_CLICK, itk_choice_set, itk, (void*)((size_t)choice->val));
      ctx_begin_path (ctx);
      ctx_move_to (ctx, x + em * (0.5),
                        y + em * (no+1));

      if (choice->val == itk->choice_no)
        itk_style_color (itk->ctx, "itk-interactive");
      else
        itk_style_color (itk->ctx, "itk-fg");
      ctx_text (ctx, choice->label);
    }
  }
  ctx_restore (ctx);
}

int ctx_renderer_is_sdl (Ctx *ctx);
int ctx_renderer_is_fb  (Ctx *ctx);

extern int ctx_show_fps;

void
itk_ctx_settings (ITK *itk)
{
#ifdef CTX_MAX_THREADS
  static int ctx_settings = 0;
  static int inited = 0;
  static int threads;
  static int hash_cache_enabled;
  Ctx *ctx = itk->ctx;

  if (!inited){
    if (!ctx_backend_is_tiled (ctx))
       return;
    inited = 1;
    threads = ctx_get_render_threads (ctx);
    hash_cache_enabled = ctx_get_hash_cache (ctx);
  }
  if (itk_expander (itk, "CTX settings", &ctx_settings))
  {

    hash_cache_enabled = itk_toggle (itk, "hash cache", hash_cache_enabled);
#if CTX_SDL
    ctx_show_fps = itk_toggle (itk, "fps debug", ctx_show_fps);
#endif
    if (hash_cache_enabled != ctx_get_hash_cache (ctx)){
      ctx_set_hash_cache (ctx, hash_cache_enabled);
    }
    itk_slider_int (itk, "threads", &threads, 1, CTX_MAX_THREADS, 1);
    if (threads != ctx_get_render_threads (ctx))
    {
      ctx_set_render_threads (ctx, threads);
    }

    static int choice = -1;
    int set = ctx_get_antialias (ctx);
    if (choice < 0)
      choice = set;
    choice = itk_choice (itk, "Antialiasing", choice);
    itk_choice_add (itk, CTX_ANTIALIAS_NONE,    "none");
    itk_choice_add (itk, CTX_ANTIALIAS_FAST,    "fast = 3");
    itk_choice_add (itk, CTX_ANTIALIAS_GOOD,    "good = 5");
    itk_choice_add (itk, CTX_ANTIALIAS_DEFAULT, "default");
    if (set != choice)
      ctx_set_antialias (ctx, choice);
  }
#endif
}

void
itk_itk_settings (ITK *itk)
{
   static int itk_settings = 0;
   if (itk_expander (itk, "ITK settings", &itk_settings))
   {
     //itk->focus_wraparound = itk_toggle (itk, "focus wraparound", itk->focus_wraparound);
     //enable_keybindings = itk_toggle (itk, "enable keybindings", enable_keybindings);
     //itk->light_mode = itk_toggle (itk, "light mode", itk->light_mode);
     itk_slider_float (itk, "global scale", &itk->scale, 0.1, 8.0, 0.1);
     itk_slider_float (itk, "font size ", &itk->font_size, 3.0, 60.0, 0.25);
     itk_slider_float (itk, "hgap", &itk->rel_hgap, 0.0, 3.0, 0.02);
     itk_slider_float (itk, "vgap", &itk->rel_vgap, 0.0, 3.0, 0.02);
     itk_slider_float (itk, "scroll speed", &itk->scroll_speed, 0.0, 1.0, 0.01);
     itk_slider_float (itk, "ver advance", &itk->rel_ver_advance, 0.1, 4.0, 0.01);
     itk_slider_float (itk, "baseline", &itk->rel_baseline, 0.1, 4.0, 0.01);
     itk_slider_float (itk, "hmargin", &itk->rel_hmargin, 0.0, 40.0, 0.1);
     itk_slider_float (itk, "vmargin", &itk->rel_vmargin, 0.0, 40.0, 0.1);
     itk_slider_float (itk, "label width", &itk->label_width, 0.0, 40.0, 0.02);
   }
}

void itk_key_quit (CtxEvent *event, void *userdata, void *userdata2)
{
  ctx_quit (event->ctx);
}

int _itk_key_bindings_active = 1;

static int
itk_iteration (double time, void *data)
{
  ITK *itk = (ITK*)data;
  Ctx *ctx = itk->ctx;
  int   ret_val = 1;

    if (ctx_need_redraw (ctx))
    {
      itk_reset (itk);
      if (_itk_key_bindings_active)
        itk_key_bindings (itk);
      ctx_add_key_binding (itk->ctx, "control-q", NULL, "Quit", itk_key_quit, NULL);
      ret_val = itk->ui_fun (itk, itk->ui_data);

      itk_done (itk);
      ctx_end_frame (ctx);
    }

    ctx_handle_events (ctx);
    return ret_val;
}

#ifdef EMSCRIPTEN
#include <emscripten/html5.h>
#endif

void itk_run_ui (ITK *itk, int (*ui_fun)(ITK *itk, void *data), void *ui_data)
{
  itk->ui_fun = ui_fun;
  itk->ui_data = ui_data;
  int   ret_val = 1;

#ifdef EMSCRIPTEN
#ifdef ASYNCIFY
  while (!ctx_has_quit (itk->ctx) && (ret_val == 1))
  {
    ret_val = itk_iteration (0.0, itk);
  }
#else
  emscripten_request_animation_frame_loop (itk_iteration, itk);
  return;
#endif
#else
  while (!ctx_has_quit (itk->ctx) && (ret_val == 1))
  {
    ret_val = itk_iteration (0.0, itk);
  }
#endif
}

void itk_main (int (*ui_fun)(ITK *itk, void *data), void *ui_data)
{
  Ctx *ctx = ctx_new (-1, -1, NULL);
  ITK  *itk = itk_new (ctx);
  itk_run_ui (itk, ui_fun, ui_data);
  itk_free (itk);
  ctx_destroy (ctx);
}



#endif
