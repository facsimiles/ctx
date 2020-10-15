typedef struct _ITK ITK;

ITK *itk_new (Ctx *ctx);
void itk_free (ITK *itk);
void itk_reset (ITK *itk);

void itk_panel_start (ITK *itk, const char *title, int x, int y, int width, int height);
void itk_panel_end (ITK *itk);

void itk_newline (ITK *itk);
void itk_sameline (ITK *itk);
void itk_seperator (ITK *itk);
void itk_label (ITK *itk, const char *label);
void itk_titlebar (ITK *itk, const char *label);
void itk_slider  (ITK *itk, const char *label, float *val, float min, float max, float step);
void itk_entry   (ITK *itk, const char *label, const char *fallback, char *val, int maxlen,
                  void (*commit)(void *commit_data), void *commit_data);
void itk_toggle  (ITK *itk, const char *label, int *val);
int  itk_radio    (ITK *itk, const char *label, int set);
int  itk_expander (ITK *itk, const char *label, int *val);
int  itk_button   (ITK *itk, const char *label);
void itk_choice  (ITK *itk, const char *label, int *val, void (*action)(void *user_data), void *user_data);
void itk_choice_add (ITK *itk, int value, const char *label);

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


typedef struct _ITKPanel ITKPanel;
struct _ITKPanel{
  int x;
  int y;
  int width;
  int height;
  int expanded;
  int max_y;
  float scroll_start_y;
  float scroll;

  const char *title;
};

typedef enum
{
  ITK_BG,
  ITK_FG,
  ITK_FOCUSED_BG,

  ITK_ENTRY_BG,
  ITK_ENTRY_FG,
  ITK_ENTRY_CURSOR,
  ITK_ENTRY_FALLBACK,

  ITK_SCROLL_FG,
  ITK_SCROLL_BG,
  ITK_SCROLL_FOCUSED_BG,

  ITK_BUTTON_FG,
  ITK_BUTTON_BG,
  ITK_BUTTON_FOCUSED_BG,
  ITK_BUTTON_PRESSED_FG,
  ITK_BUTTON_PRESSED_BG,

  ITK_SLIDER_TEXT,
  ITK_SLIDER_CURSOR
} ItkControlType;

uint8_t theme[][5]={
  {ITK_BG,                 30,40,50,255},
  {ITK_FOCUSED_BG,         60,70,80,255},
  {ITK_FG,                 225,225,225,255},
  {ITK_ENTRY_BG,           10,20,60,255},
  {ITK_ENTRY_FG,           255,5,5,255},
  {ITK_ENTRY_CURSOR,       225,245,140,255},
  {ITK_ENTRY_FALLBACK,     225,245,140,255},
  {ITK_BUTTON_FOCUSED_BG,  60,60,160,255},
  {ITK_BUTTON_BG,          30,40,150,255},
  {ITK_BUTTON_FG,          255,255,255,255},
  {ITK_SCROLL_BG,          30,40,150,255},
  {ITK_SCROLL_FG,          60,80,190,255},
  {ITK_SLIDER_CURSOR,      255,0,0,127},
  {ITK_SLIDER_TEXT,        255,255,255,127},
};

typedef struct _CtxControl CtxControl;
struct _CtxControl{
  int no;
  int ref_count;
  int type; /* this should be a pointer to the vfuncs/class struct
               instead - along with one optional instance data per control */
  char *label;
  float x;
  float y;
  float width;
  float height;
  void *val;

  char *fallback;
  float min;
  float max;
  float step;
  void (*action)(void *user_data);
  void (*commit)(void *commit_data);
  void *commit_data;
};

typedef struct _UiChoice  UiChoice;
struct _UiChoice
{
  int   val;
  char *label;
};

struct _ITK{
  Ctx *ctx;
  float x0;
  float y0;
  float x;
  float y;

  float stored_x; // for sameline()

  float font_size;
  float width;
  float height;
  float value_pos;
  float value_width;

  float scale;

  float rel_ver_advance;
  float rel_baseline;
  float rel_hgap;
  float rel_vgap;
  float scroll_speed;

  int   dirty;
  int   button_pressed;

  int focus_wraparound;

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
  int control_no;
  int choice_active;

  int line_no;
  int lines_drawn;
};

void itk_set_color (ITK *itk, int color)
{
  for (int i = 0; i < sizeof (theme)/sizeof(theme[0]); i++)
  {
    if (theme[i][0] == color)
    {
      ctx_rgba (itk->ctx, theme[i][1]/255.0,
                          theme[i][2]/255.0,
                          theme[i][3]/255.0,
                          theme[i][4]/255.0);
      return;
    }
  }
}

ITK *itk_new (Ctx *ctx)
{
  ITK *itk     = calloc (sizeof (ITK), 1);
  itk->ctx             = ctx;
  itk->focus_wraparound = 1;
  itk->scale           = 1.5;
  itk->font_size       = 18;
  itk->width           = itk->font_size * 15;
  itk->value_width     = 0.4;
  itk->value_pos       = 0.5;
  itk->rel_ver_advance = 1.2;
  itk->rel_baseline    = 0.8;
  itk->rel_hgap        = 0.5;
  itk->rel_vgap        = 0.2;
  itk->scroll_speed    = 0.333;
  itk->dirty ++;

  return itk;
}

float itk_em (ITK *itk)
{
  return itk->font_size * itk->scale;
}

void itk_free (ITK *itk)
{
  free (itk);
}

void control_unref (CtxControl *control)
{
  if (control->ref_count < 0)
  {
    CtxControl *w = control;

    if (w->label)
      free (w->label);
    if (w->fallback)
      free (w->fallback);
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
  ctx_reset                 (ctx);
  ctx_save (ctx);
  ctx_font (ctx, "regular");
  ctx_font_size (ctx, itk_em (itk));

  itk->panel = NULL;

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
  Ctx *ctx = itk->ctx;
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

CtxControl *add_control (ITK *itk, const char *label, float x, float y, float width, float height)
{
  CtxControl *control = calloc (sizeof (CtxControl), 1);
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  control->label = strdup (label);

  // refind focus..
if(0)  if (itk->focus_label){
     if (!strcmp (itk->focus_label, label))
     {
       itk->focus_no = itk->control_no;
     }
  }

  control->x = x;
  control->y = y;
  control->no = itk->control_no;
  itk->control_no++;
  control->width = width;
  control->height = height;
  ctx_list_prepend (&itk->controls, control);

  if (itk->focus_no == control->no)
  {
     if (itk->y - itk->panel->scroll > itk->panel->y + itk->panel->height - em * 2)
     {
        itk->panel->scroll += itk->scroll_speed * em;
#if 0
        if (itk->panel->scroll > itk->panel->max_y - itk->panel->scroll_start_y - (itk->panel->height-itk->panel->scroll_start_y-itk->panel->y)) - em * itk->rel_ver_advance;
            itk->panel->scroll = itk->panel->max_y - itk->panel->scroll_start_y - (itk->panel->height-itk->panel->scroll_start_y-itk->panel->y) - em * itk->rel_ver_advance;
#endif

        itk->dirty++;
     }
     else if (itk->y - itk->panel->scroll < em * 2)
     {
        itk->panel->scroll -= itk->scroll_speed * em;
        if (itk->panel->scroll<0.0)
          itk->panel->scroll=0.0;
        itk->dirty++;
     }

  }

  return control;
}

static void itk_text (ITK *itk, const char *text)
{
  Ctx *ctx = itk->ctx;
  itk_set_color (itk, ITK_FG);
  ctx_move_to (ctx, itk->x,  itk->y + itk_em (itk) * itk->rel_baseline);
  ctx_text (ctx, text);
  itk->x += ctx_text_width (ctx, text);
}

static void itk_base (ITK *itk, const char *label, float x, float y, float width, float height, int focused)
{
  Ctx *ctx = itk->ctx;
  if (focused)
  {
    itk_set_color (itk, ITK_FOCUSED_BG);

    ctx_rectangle (ctx, x, y, width, height);
    ctx_fill (ctx);
  }
#if 0
  else
    itk_set_color (itk, ITK_BG);

  if (itk->line_no >= itk->lines_drawn)
  {
    ctx_rectangle (ctx, x, y, width, height);
    ctx_fill (ctx);
    itk->lines_drawn = itk->line_no -1;
  }
#endif
}

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
  char buf[100] = "";
  float em = itk_em (itk);
  itk_base (itk, NULL, itk->x, itk->y, itk->width, em * itk->rel_ver_advance / 4, 0);
  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance/4);
  ctx_gray (ctx, 0.5);
  ctx_fill (ctx);
  itk_newline (itk);
  itk->y -= em * itk->rel_ver_advance * 0.75;
}

void itk_label (ITK *itk, const char *label)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  char buf[100] = "";
  itk_base (itk, NULL, itk->x, itk->y, itk->width, em * itk->rel_ver_advance, 0);
  itk_text (itk, label);

  itk->x += itk->rel_hgap * itk->font_size;
  itk_newline (itk);
}

static void titlebar_drag (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  ITKPanel *panel = userdata;
  
#if 1
  //fprintf (stderr, "%d %f %f\n", event->delta_x, event->delta_y);
  panel->x += event->delta_x;
  panel->y += event->delta_y;
#else
  panel->x = event->x - panel->width / 2;
  panel->y = event->y;
#endif

  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_titlebar (ITK *itk, const char *label)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);

  CtxControl *control = add_control (itk, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  control->type = UI_TITLEBAR;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_DRAG, titlebar_drag, itk->panel, itk, NULL, NULL);

  ctx_begin_path (ctx);
  itk->line_no = 0;
  itk->lines_drawn = 0;
  itk_base (itk, label, control->x, control->y, control->width, em * itk->rel_ver_advance, itk->focus_no == control->no);
  itk_text (itk, label);
  //itk->lines_drawn = 1;

  itk_newline (itk);
}

void itk_panel_start (ITK *itk, const char *title,
                      int x, int y, int width, int height)
{
  Ctx *ctx = itk->ctx;
  ITKPanel *panel = add_panel (itk, title, x, y, width, height);
  itk->x0 = itk->x = panel->x;
  itk->y0 = itk->y = panel->y;
  itk->width  = panel->width;
  itk->height = panel->height;
  itk->panel = panel;
  {
    itk_set_color (itk, ITK_BG);
    ctx_begin_path (ctx);
    ctx_rectangle (ctx, panel->x, panel->y, panel->width, panel->height);
//  fprintf (stderr, "%i %i %i %i\n", panel->x, panel->y, panel->width, panel->height);
    ctx_fill (ctx);
    itk_titlebar (itk, title);

    ctx_save (ctx);
    itk->panel->scroll_start_y = itk->y;
    ctx_rectangle (ctx, itk->x, itk->y, panel->width, panel->height - (itk->y - panel->y));
    ctx_clip (ctx);
    ctx_begin_path (ctx);
    ctx_translate (ctx, 0.0, -panel->scroll);
  }
}

void itk_panel_scroll_drag (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  ITKPanel *panel = data2;
  float em = itk_em (itk);
  float scrollbar_height = panel->height - (panel->scroll_start_y - panel->y);
  float scrollbar_width = em;
  float th = scrollbar_height * (scrollbar_height /  (panel->max_y-panel->scroll_start_y));
  if (th > scrollbar_height)
  {
    panel->scroll = 0;
    event->stop_propagate = 1;
    itk->dirty++;
    return;
  }

  panel->scroll = ((event->y - panel->scroll_start_y - th / 2) / (scrollbar_height-th)) *
          (panel->max_y - panel->scroll_start_y - scrollbar_height)
          ;
  itk->focus_no = -1;
  itk->dirty++;

  if (panel->scroll < 0) panel->scroll = 0;

  event->stop_propagate = 1;
}

void itk_panel_resize_drag (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data2;
  ITKPanel *panel = data;
  panel->width += event->delta_x;
  panel->height += event->delta_y;
  itk->dirty++;
  event->stop_propagate = 1;
}

void itk_panel_end (ITK *itk)
{
  Ctx *ctx = itk->ctx;
  ITKPanel *panel = itk->panel;
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
  ctx_listen (ctx, CTX_DRAG, itk_panel_scroll_drag, itk, panel);
  itk_set_color (itk, ITK_SCROLL_BG);
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

  itk_set_color (itk, ITK_SCROLL_FG);
  ctx_fill (ctx);

  ctx_rectangle (ctx, panel->x + panel->width - em,
                      panel->y + panel->height - em,
                      em,
                      em);
  ctx_listen (ctx, CTX_DRAG, itk_panel_resize_drag, panel, itk);
  itk_set_color (itk, ITK_BUTTON_FG);
  ctx_begin_path (ctx);
  ctx_move_to (ctx, panel->x + panel->width,
                    panel->y + panel->height);
#if 1
  ctx_rel_line_to (ctx, -em, 0);
  ctx_rel_line_to (ctx, em, -em);
  ctx_rel_line_to (ctx, 0, em);
#endif
  ctx_fill (ctx);

  /* set global clip - workaround until we have better */
  ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
  ctx_clip (ctx);

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

void slider_drag (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl  *control = userdata;
  float *val = control->val;
  float new_val;
  
  new_val = ((event->x - control->x) / (control->width * itk->value_width)) * (control->max-control->min) + control->min;
  itk_float_constrain (control, &new_val);

  itk->focus_no = control->no;
  *val = new_val;
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_slider (ITK *itk, const char *label, float *val, float min, float max, float step)
{
  Ctx *ctx = itk->ctx;
  char buf[100] = "";
  itk_set_color (itk, ITK_BUTTON_BG);
  float em = itk_em (itk);
  CtxControl *control = add_control (itk, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  control->min = min;
  control->max = max;
  control->step = step;
  control->val = val;
  control->type = UI_SLIDER;
  control->ref_count++;
  control->ref_count++;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width*itk->value_width, em * itk->rel_ver_advance);

  ctx_listen_with_finalize (ctx, CTX_DRAG, slider_drag, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);


  itk_base (itk, label, control->x, control->y, control->width, em * itk->rel_ver_advance,
                        itk->focus_no == control->no);

  sprintf (buf, "%.3f", *val);
  ctx_move_to (ctx, itk->x + em * itk->value_pos, itk->y + em * itk->rel_baseline);
  itk_set_color (itk, ITK_SLIDER_TEXT);
  ctx_text (ctx, buf);

  float rel_val = ((*val) - min) / (max-min);
  itk_set_color (itk, ITK_SLIDER_CURSOR);
  ctx_rectangle (ctx, itk->x + (itk->width*itk->value_width-em/4) * rel_val, itk->y, em/4, control->height);
  ctx_fill (ctx);

  ctx_rectangle (ctx, itk->x, itk->y + em*5/6, itk->width * itk->value_width, em/8);

  ctx_fill (ctx);

  itk->x += itk->value_width * itk->width;
  itk_text (itk, label);
  itk_newline (itk);
}

void entry_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  event->stop_propagate = 1;
  itk->focus_no = control->no;
  itk->dirty++;
}

void itk_entry (ITK *itk, const char *label, const char *fallback, char *val, int maxlen,
                  void (*commit)(void *commit_data),
                                void *commit_data)
{
  Ctx *ctx = itk->ctx;
  char buf[100] = "";
  float em = itk_em (itk);
  CtxControl *control = add_control (itk, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  control->val = val;
  control->type = UI_ENTRY;
  if (fallback)
    control->fallback = strdup (fallback);
  control->ref_count++;
  control->ref_count++;
  control->commit = commit;
  control->commit_data = commit_data;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);

  ctx_listen_with_finalize (ctx, CTX_CLICK, entry_clicked, control, itk, control_finalize, NULL);

  ctx_begin_path (ctx);
  itk_base (itk, label, control->x, control->y, control->width, em * itk->rel_ver_advance,
                        itk->focus_no == control->no);

  ctx_move_to (ctx, itk->x + em * itk->value_pos, itk->y + em * itk->rel_baseline);
  if (itk->entry_copy)
  {
    int backup = itk->entry_copy[itk->entry_pos];
    char buf[4]="|";
    itk->entry_copy[itk->entry_pos]=0;
    itk_set_color (itk, ITK_ENTRY_FG);
    ctx_text (ctx, itk->entry_copy);
    itk_set_color (itk, ITK_ENTRY_CURSOR);
    ctx_text (ctx, buf);

    buf[0]=backup;
    if (backup)
    {
      itk_set_color (itk, ITK_ENTRY_FG);
      ctx_text (ctx, buf);
      ctx_text (ctx, &itk->entry_copy[itk->entry_pos+1]);
      itk->entry_copy[itk->entry_pos] = backup;
    }
  }
  else
  {
    if (val[0])
    {
      itk_set_color (itk, ITK_ENTRY_FG);
      ctx_text (ctx, val);
    }
    else
    {
      if (control->fallback)
      {
        itk_set_color (itk, ITK_ENTRY_FALLBACK);
        ctx_text (ctx, control->fallback);
      }
    }
  }
  itk->x += itk->value_width * itk->width;
  itk_text (itk, label);

  itk_newline (itk);
}

void toggle_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  *val = (*val)?0:1;
  event->stop_propagate = 1;
  itk->focus_no = control->no;
  itk->dirty++;
}

void itk_toggle (ITK *itk, const char *label, int *val)
{
  Ctx *ctx = itk->ctx;
  char buf[100] = "";
  float em = itk_em (itk);

  CtxControl *control = add_control (itk, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  control->val = val;
  control->type = UI_TOGGLE;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, toggle_clicked, control, itk, control_finalize, NULL);

  ctx_begin_path (ctx);
  itk_base (itk, label, control->x, control->y, control->width, em * itk->rel_ver_advance,
                        itk->focus_no == control->no);

  itk_set_color (itk, ITK_ENTRY_FG);
  ctx_move_to (ctx, itk->x + em * itk->value_pos, itk->y + em * itk->rel_baseline);
  if (*val)
  {
    ctx_text (ctx, "1 ");
  }
  else
  {
    ctx_text (ctx, "0 ");
  }

  ctx_rgba (ctx, 0, 0, 0, 1);
  itk->x += ctx_text_width (ctx, "XX") + em * itk->value_pos;
  if (label)
  {
    itk_text (itk, label);
  }

  itk->x += itk->rel_hgap * em;
  itk_newline (itk);
}

static void button_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  if (control->action)
    control->action (control->val);
  event->stop_propagate = 1;
  itk->dirty ++;
  itk->focus_no = control->no;
  itk->button_pressed = 1;
}

int itk_radio (ITK *itk, const char *label, int set)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  float width = ctx_text_width (ctx, label) + ctx_text_width (ctx, "X");
  CtxControl *control = add_control (itk, label, itk->x, itk->y, width, em * itk->rel_ver_advance);
  itk_base (itk, label, itk->x, itk->y, width, em * itk->rel_ver_advance, itk->focus_no == control->no);
  itk_set_color (itk, ITK_ENTRY_FG);
  ctx_move_to (ctx, itk->x,  itk->y + em * itk->rel_baseline);
  if (set)
  ctx_text (ctx, "X");
  else
  ctx_text (ctx, "O");
  itk_set_color (itk, ITK_FG);
  ctx_text (ctx, label);

  control->type = UI_RADIO;
  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);
  itk->x += width;

  itk->x += itk->rel_hgap * em;
  itk_newline (itk);
  if (control->no == itk->focus_no && itk->button_pressed)
  {
    itk->button_pressed = 0;
    itk->dirty++;
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
  itk->focus_no = control->no;
  itk->dirty++;
}

int itk_expander (ITK *itk, const char *label, int *val)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  char buf[100] = "";
  ctx_rgb (ctx, 1, 1, 1);
  CtxControl *control = add_control (itk, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  control->val = val;
  control->type = UI_EXPANDER;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, toggle_clicked, control, itk, control_finalize, NULL);

  ctx_begin_path (ctx);
  {
     char *tmp = malloc (strlen (label) + 10);
     sprintf (tmp, "%s %s", *val?"V":"-", label);
     itk_base (itk, label, control->x, control->y, control->width, em * itk->rel_ver_advance,
              itk->focus_no == control->no);
     itk_text (itk, tmp);
     free (tmp);
  }

  itk_newline (itk);
  return *val;
}

int itk_button (ITK *itk, const char *label)
{
  Ctx *ctx = itk->ctx;
  float width = ctx_text_width (ctx, label);
  float em = itk_em (itk);
  CtxControl *control = add_control (itk, label, itk->x, itk->y, width, em * itk->rel_ver_advance);

  if (itk->focus_no == control->no)
    itk_set_color (itk, ITK_BUTTON_FOCUSED_BG);
  else
    itk_set_color (itk, ITK_BUTTON_BG);

  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_fill (ctx);

  itk_set_color (itk, ITK_BUTTON_FG);
  ctx_move_to (ctx, itk->x,  itk->y + em * itk->rel_baseline);
  ctx_text (ctx, label);

  control->type = UI_BUTTON;
  ctx_rectangle (ctx, itk->x, itk->y, width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, itk, control_finalize, NULL);
  ctx_begin_path (ctx);
  itk->x += width;
  itk->x += itk->rel_hgap * em;

  itk_newline (itk);
  if (control->no == itk->focus_no && itk->button_pressed)
  {
    itk->button_pressed = 0;
    itk->dirty++;
    return 1;
  }
  return 0;
}

void choice_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata2;
  CtxControl *control = userdata;
  itk->choice_active = 1;
  itk->focus_no = control->no;
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_choice (ITK *itk, const char *label, int *val, void (*action)(void *user_data), void *user_data)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  char buf[100] = "";
  itk_set_color (itk, ITK_BG);
  CtxControl *control = add_control (itk, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  control->action = action;
  control->commit_data = user_data;
  control->val = val;
  control->type = UI_CHOICE;

  ctx_rectangle (ctx, itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, choice_clicked, control, itk, control_finalize, NULL);

  ctx_fill (ctx);
  ctx_begin_path (ctx);
  itk_base (itk, label, itk->x, itk->y, itk->width, em * itk->rel_ver_advance, itk->focus_no == control->no);
  itk->x = itk->x + itk->width * itk->value_width;
  itk_text (itk, label);

  itk_newline (itk);
}

void itk_choice_add (ITK *itk, int value, const char *label)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  CtxControl *control = itk->controls->data;
  {
    int *val = control->val;
    if (*val == value)
    {
      ctx_move_to (ctx, itk->x + em * itk->value_pos,
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

void entry_commit (ITK *itk)
{

  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == itk->focus_no)
    {
      switch (control->type)
      {
        case UI_ENTRY:
         if (itk->entry_copy)
         {
           if (control->commit)
           {
             control->commit (control->commit_data);
           }
           else
           {
             strcpy (control->val, itk->entry_copy);
           }
           free (itk->entry_copy);
           itk->entry_copy = NULL;
         }
      }
      return;
    }
  }
}

void itk_focus (ITK *itk, int dir)
{

   entry_commit (itk);
   if (itk->focus_no < 0)
   {
     itk->focus_no = 0;
     return;
   }
   itk->focus_no += dir;
#if 0
   if (itk->focus_label){
     free (itk->focus_label);
     itk->focus_label = NULL;
   }
#endif

   int n_controls = ctx_list_length (itk->controls);
   CtxList *iter = ctx_list_nth (itk->controls, n_controls-itk->focus_no-1);
   if (iter)
   {
     CtxControl *control = iter->data;
//   itk->focus_label = strdup (control->label);
     //fprintf (stderr, "%s\n", control_focus_label);
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
}

CtxControl *itk_focused_control(ITK *itk)
{
  for (CtxList *l = itk->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == itk->focus_no)
      return control;
  }
  return NULL;
}

void itk_key_up (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);

  if (control && control->type == UI_CHOICE && itk->choice_active)
  {
    int *val = control->val;
    int old_val = *val;
    int prev_val = old_val;
    for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
    {
      UiChoice *choice = l->data;
      if (choice->val == old_val)
      {
        *val = prev_val;
        l=NULL;
      }
      prev_val = choice->val;
    }
  }
  else
  {
    itk_focus (itk, -1);
  }
  itk->dirty++;
  event->stop_propagate = 1;
}

void itk_key_down (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (control && control->type == UI_CHOICE && itk->choice_active)
  {
    int *val = control->val;
    int old_val = *val;
    for (CtxList *l = itk->choices; l; l=l?l->next:NULL)
    {
      UiChoice *choice = l->data;
      if (choice->val == old_val)
      {
         if (l->next)
         {
           l = l->next;
           choice = l->data;
           (*val) = choice->val;
         }
      }
    }
  }
  else
  {
    itk_focus (itk, 1);
  }
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_key_tab (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  switch (control->type)
  {

  }
  itk_focus (itk, 1);

  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_key_shift_tab (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  switch (control->type)
  {

  }
  itk_focus (itk, -1);

  event->stop_propagate = 1;
  itk->dirty++;
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
         if (itk->entry_copy)
         {
           entry_commit (itk);
         }
         else
         {
           itk->entry_copy = strdup (control->val);
           itk->entry_pos = strlen (control->val);
         }
       }
       break;
      case UI_SLIDER:
        {
           // XXX edit value
        }
        break;
      case UI_TOGGLE:
      case UI_EXPANDER:
        {
          int *val = control->val;
          *val = !(*val);
        }
        break;
      case UI_RADIO:
      case UI_BUTTON:
        {
          if (control->action)
            control->action(control->val);
          itk->button_pressed=1;
        }
        break;
    }
  }
  event->stop_propagate=1;
  itk->dirty++;
}

void itk_key_left (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_CHOICE:
      itk_key_return (event, data, data2);
      break;
    case UI_ENTRY:
      {
        if (itk->entry_copy)
        {
          itk->entry_pos --;
          if (itk->entry_pos < 0) itk->entry_pos = 0;
        }
      }
      break;
    case UI_SLIDER:
      {
        float *val = control->val;
        *val -= control->step;
        if (*val < control->min) *val = control->min;
      }
      break;
  }
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_key_right (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_BUTTON:
    case UI_CHOICE:
    case UI_RADIO:
      itk_key_return (event, data, data2);
      break;
    case UI_ENTRY:
      {
        if (itk->entry_copy)
        {
          itk->entry_pos ++;
          if (itk->entry_pos > strlen (itk->entry_copy))
            itk->entry_pos = strlen (itk->entry_copy);
        }
        else
        {
          itk_key_return (event, data, data2);
        }
      }
      break;
    case UI_SLIDER:
      {
        float *val = control->val;
        *val += control->step;
        if (*val > control->max) *val = control->max;
      }
      break;
  }
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_key_backspace (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  switch (control->type)
  {
    case UI_ENTRY:
     {
       if (itk->entry_copy && itk->entry_pos > 0)
       {
         memmove (&itk->entry_copy[itk->entry_pos-1], &itk->entry_copy[itk->entry_pos],
                   strlen (&itk->entry_copy[itk->entry_pos] )+ 1);
         itk->entry_pos --;
       }
     }
     break;
  }
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_key_delete (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  if (!control) return;
  if (strlen (itk->entry_copy) > itk->entry_pos)
  {
    itk_key_right (event, data, data2);
    itk_key_backspace (event, data, data2);
  }
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_key_unhandled (CtxEvent *event, void *userdata, void *userdata2)
{
  ITK *itk = userdata;
  if (itk->entry_copy)
    {
      const char *str = event->string;
      if (!strcmp (str, "space"))
        str = " ";

      char *tmp = malloc (strlen (itk->entry_copy) + strlen (str) + 1);

      char *rest = strdup (&itk->entry_copy[itk->entry_pos]);
      itk->entry_copy[itk->entry_pos]=0;

      sprintf (tmp, "%s%s%s", itk->entry_copy, str, rest);
      free (rest);
      itk->entry_pos+=strlen(str);
      free (itk->entry_copy);
      itk->entry_copy = tmp;
      itk->dirty++;
    }
  event->stop_propagate = 1;
}

void itk_key_bindings (ITK *itk)
{
  Ctx *ctx = itk->ctx;
  ctx_add_key_binding (ctx, "up", NULL, "focus prev",   itk_key_up,        itk);
  ctx_add_key_binding (ctx, "down", NULL, "focus next", itk_key_down,      itk);
  ctx_add_key_binding (ctx, "right", NULL, "",          itk_key_right,     itk);
  ctx_add_key_binding (ctx, "left", NULL, "",           itk_key_left,      itk);
  ctx_add_key_binding (ctx, "return", NULL, "",         itk_key_return,    itk);
  ctx_add_key_binding (ctx, "backspace", NULL, "",      itk_key_backspace, itk);
  ctx_add_key_binding (ctx, "delete", NULL, "",         itk_key_delete,    itk);
  ctx_add_key_binding (ctx, "tab", NULL, "",            itk_key_tab,       itk);
  ctx_add_key_binding (ctx, "shift-tab", NULL, "",      itk_key_shift_tab, itk);
  ctx_add_key_binding (ctx, "unhandled", NULL, "",      itk_key_unhandled, itk);
}

void ctx_choice_set (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  CtxControl *control = itk_focused_control (itk);
  int *val = control->val;
  *val = (size_t)(data2);
  itk->dirty++;
  event->stop_propagate = 1;
}

void ctx_event_block (CtxEvent *event, void *data, void *data2)
{
  ITK *itk = data;
  itk->choice_active = 0;
  event->stop_propagate = 1;
  itk->dirty++;
}

void itk_done (ITK *itk)
{
  Ctx *ctx = itk->ctx;
  CtxControl *control = itk_focused_control (itk);
  float em = itk_em (itk);
  if (!control) return;

  if (control->type == UI_CHOICE && itk->choice_active)
  {
    ctx_rgb (ctx, 1,1,1);
    ctx_rectangle (ctx, control->x + em * itk->value_pos,
                        control->y,
                        em * 4,
                        em * (ctx_list_length (itk->choices) + 0.5));
    ctx_fill (ctx);
    ctx_rgb (ctx, 0,0,0);
    ctx_rectangle (ctx, control->x + em * itk->value_pos,
                        control->y,
                        em * 4,
                        em * (ctx_list_length (itk->choices) + 0.5));
    ctx_line_width (ctx, 1);
    ctx_stroke (ctx);

    int no = 0;

    ctx_rectangle (ctx, 0,0,ctx_width(ctx), ctx_height(ctx));
    ctx_listen (ctx, CTX_CLICK, ctx_event_block, itk, NULL);
    ctx_begin_path (ctx);

    ctx_list_reverse (&itk->choices);
    for (CtxList *l = itk->choices; l; l = l->next, no++)
    {
      UiChoice *choice = l->data;
      ctx_rectangle (ctx, control->x + em * (itk->value_pos),
                          control->y + em * (no),
                          em * 4,
                          em * 1.5);
      ctx_listen (ctx, CTX_CLICK, ctx_choice_set, itk, (void*)((size_t)choice->val));
      ctx_begin_path (ctx);
      ctx_move_to (ctx, control->x + em * (itk->value_pos + 0.5),
                        control->y + em * (no+1));
      int *val = control->val;
      if (choice->val == *val)
        ctx_rgb (ctx, 1,0,0);
      else
      ctx_rgb (ctx, 0,0,0);
      ctx_text (ctx, choice->label);
    }
  }
  ctx_restore (ctx);
}
