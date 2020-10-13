

typedef struct _CtxControls CtxControls;

CtxControls *cctx_new (Ctx *ctx);
void cctx_free (CtxControls *cctx);
void ui_reset (CtxControls *cctx);
void ui_newline (CtxControls *cctx);
void ui_sameline (CtxControls *cctx);
void ui_seperator (CtxControls *cctx);
void ui_label (CtxControls *cctx, const char *label);
void ui_titlebar (CtxControls *cctx, const char *label);
void ui_slider  (CtxControls *cctx, const char *label, float *val, float min, float max, float step);
void ui_entry   (CtxControls *cctx, const char *label, const char *fallback, char *val, int maxlen,
                  void (*commit)(void *commit_data), void *commit_data);
void ui_toggle  (CtxControls *cctx, const char *label, int *val);
int  ui_radio    (CtxControls *cctx, const char *label, int set);
int  ui_expander (CtxControls *cctx, const char *label, int *val);
int  ui_button2  (CtxControls *cctx, const char *label);
void ui_choice  (CtxControls *cctx, const char *label, int *val, void (*action)(void *user_data), void *user_data);
void ui_choice_add (CtxControls *cctx, int value, const char *label);


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

typedef struct _CtxControl CtxControl;
struct _CtxControl{
  int no;
  int ref_count;
  int type;
  char *label;
  float x;
  float y;
  float width;
  float height;
  char *fallback;
  void *val;
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

struct _CtxControls{
  Ctx *ctx;
  float x0;
  float x;
  float y;

  float stored_x; // for sameline()

  float font_size;
  float width;
  float value_pos;
  float value_width;

  float rel_ver_advance;
  float rel_baseline;
  float winx;
  float winy;

  int   dirty;
  int   button_pressed;

  int   focus_no;
  int   focus_x;
  int   focus_y;
  int   focus_width;
  char *focus_label;

  char *entry_copy;
  int   entry_pos;

  CtxList *old_controls;
  CtxList *controls;
  CtxList *choices;
  int control_no;
  int choice_active;

  int line_no;
  int lines_drawn;
};

CtxControls *cctx_new (Ctx *ctx)
{
  CtxControls *cctx     = calloc (sizeof (CtxControls), 1);
  cctx->ctx             = ctx;
  cctx->font_size       = 28;
  cctx->width           = cctx->font_size * 15;
  cctx->value_width     = 0.4;
  cctx->value_pos       = 0.5;
  cctx->rel_ver_advance = 1.0;
  cctx->rel_baseline    = 0.7;
  cctx->dirty ++;

  return cctx;
}

void cctx_free (CtxControls *cctx)
{
  free (cctx);
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

void ui_reset (CtxControls *cctx)
{
  Ctx *ctx = cctx->ctx;
  ctx_reset                 (ctx);


  while (cctx->old_controls)
  {
    CtxControl *control = cctx->old_controls->data;
    control_unref (control);
    ctx_list_remove (&cctx->old_controls, control);
  }
  cctx->old_controls = cctx->controls;
  cctx->controls = NULL;
  while (cctx->choices)
  {
    UiChoice *choice = cctx->choices->data;
    free (choice->label);
    free (choice);
    ctx_list_remove (&cctx->choices, choice);
  }
  cctx->control_no = 0;
  cctx->x0 =
  cctx->x = cctx->winx;
  cctx->y = cctx->winy;
}


CtxControl *add_control (CtxControls *cctx, const char *label, float x, float y, float width, float height)
{
  CtxControl *control = calloc (sizeof (CtxControl), 1);
  Ctx *ctx = cctx->ctx;
  control->label = strdup (label);

if(0)  if (cctx->focus_label){
     if (!strcmp (cctx->focus_label, label))
     {
       cctx->focus_no = cctx->control_no;
     }
  }


  control->x = x;
  control->y = y;
  control->no = cctx->control_no;
  cctx->control_no++;
  control->width = width;
  control->height = height;
  ctx_list_prepend (&cctx->controls, control);

#if 0
  float px = ctx_pointer_x (ctx);
  float py = ctx_pointer_y (ctx);

  if (px >= control->x &&  px < control->x +  control->width &&
      py >= control->y &&  py < control->y +  control->height)
  {
    if (cctx->focus_no != control->no)
    {
      cctx->focus_no = control->no;
      cctx->dirty++;
    }
  }
#endif

  return control;
}

static void ui_text (CtxControls *cctx, const char *text)
{
  Ctx *ctx = cctx->ctx;
  ctx_rgb (ctx, 0, 0, 0);
  ctx_move_to (ctx, cctx->x,  cctx->y + cctx->font_size * cctx->rel_baseline);
  ctx_text (ctx, text);
  cctx->x += ctx_text_width (ctx, text);
}

static void ui_base (CtxControls *cctx, const char *label, float x, float y, float width, float height, int focused)
{
  Ctx *ctx = cctx->ctx;
  ctx_font_size (ctx, cctx->font_size);
  if (focused)
    ctx_rgb (ctx, 1, 1, 1);
  else
    ctx_rgb (ctx, 0.9,0.9,0.9);

  if (cctx->line_no >= cctx->lines_drawn)
  {
    ctx_rectangle (ctx, x, y, width, height);
    ctx_fill (ctx);
    cctx->lines_drawn = cctx->line_no -1;
  }
}


void ui_newline (CtxControls *cctx)
{
  cctx->y += cctx->font_size * cctx->rel_ver_advance;
  cctx->stored_x = cctx->x;
  cctx->x = cctx->x0;
  cctx->line_no++;
}

void ui_sameline (CtxControls *cctx)
{
  cctx->y -= cctx->font_size * cctx->rel_ver_advance;
  cctx->x = cctx->stored_x;
  cctx->line_no--;
}


void ui_seperator (CtxControls *cctx)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ui_base (cctx, NULL, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance / 4, 0);
  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance/4);
  ctx_gray (ctx, 0.5);
  ctx_fill (ctx);
  ctx_restore (ctx);
  ui_newline (cctx);
  cctx->y -= cctx->font_size * cctx->rel_ver_advance * 0.75;
}


void ui_label (CtxControls *cctx, const char *label)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ui_base (cctx, NULL, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance, 0);
  ui_text (cctx, label);

  ctx_restore (ctx);
  ui_newline (cctx);
}

void titlebar_drag (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl  *control = userdata;
  
  cctx->winx += event->delta_x;
  cctx->winy += event->delta_y;

  event->stop_propagate = 1;
  cctx->dirty++;
}


void ui_titlebar (CtxControls *cctx, const char *label)
{
  Ctx *ctx = cctx->ctx;
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->type = UI_TITLEBAR;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_DRAG, titlebar_drag, control, cctx, control_finalize, NULL);

  ctx_begin_path (ctx);
  cctx->line_no = 0;
  cctx->lines_drawn = 0;
  ui_base (cctx, label, control->x, control->y, control->width, cctx->font_size * cctx->rel_ver_advance, cctx->focus_no == control->no);
  ui_text (cctx, label);
  //cctx->lines_drawn = 1;

  ctx_restore (ctx);
  ui_newline (cctx);
}

static void ui_float_constrain (CtxControl *control, float *val)
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
  CtxControls *cctx = userdata2;
  CtxControl  *control = userdata;
  float *val = control->val;
  float new_val;
  
  new_val = ((event->x - control->x) / (control->width * cctx->value_width)) * (control->max-control->min) + control->min;
  ui_float_constrain (control, &new_val);

  *val = new_val;
  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_slider (CtxControls *cctx, const char *label, float *val, float min, float max, float step)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ctx_font_size (ctx, cctx->font_size);
  ctx_rgb (ctx, 1, 1, 1);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->min = min;
  control->max = max;
  control->step = step;
  control->val = val;
  control->type = UI_SLIDER;
  control->ref_count++;
  control->ref_count++;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width*cctx->value_width, cctx->font_size * cctx->rel_ver_advance);

  ctx_listen_with_finalize (ctx, CTX_DRAG, slider_drag, control, cctx, control_finalize, NULL);
  ctx_begin_path (ctx);

  ui_base (cctx, label, control->x, control->y, control->width, cctx->font_size * 2,
                        cctx->focus_no == control->no);

  sprintf (buf, "%f", *val);
  ctx_move_to (ctx, cctx->x + cctx->font_size * cctx->value_pos, cctx->y + cctx->font_size * cctx->rel_baseline);
  ctx_rgb (ctx, 1, 0, 0);
  ctx_text (ctx, buf);

  float rel_val = ((*val) - min) / (max-min);
  ctx_rectangle (ctx, cctx->x + (cctx->width*cctx->value_width-cctx->font_size/4) * rel_val, cctx->y, cctx->font_size/4, control->height * 0.8);
  ctx_fill (ctx);

  cctx->x += cctx->value_width * cctx->width;
  ui_text (cctx, label);
  ui_newline (cctx);

  ctx_restore (ctx);
}

void entry_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  event->stop_propagate = 1;
  cctx->focus_no = control->no;
  cctx->dirty++;
}

void ui_entry (CtxControls *cctx, const char *label, const char *fallback, char *val, int maxlen,
                  void (*commit)(void *commit_data),
                                void *commit_data)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->val = val;
  control->type = UI_ENTRY;
  if (fallback)
    control->fallback = strdup (fallback);
  control->ref_count++;
  control->ref_count++;
  control->commit = commit;
  control->commit_data = commit_data;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);

  ctx_listen_with_finalize (ctx, CTX_CLICK, entry_clicked, control, cctx, control_finalize, NULL);

  ctx_begin_path (ctx);
  ui_base (cctx, label, control->x, control->y, control->width, cctx->font_size * 2,
                        cctx->focus_no == control->no);

  ctx_move_to (ctx, cctx->x + cctx->font_size * cctx->value_pos, cctx->y + cctx->font_size * cctx->rel_baseline);
  ctx_rgb (ctx, 1, 0, 0);
  if (cctx->entry_copy)
  {
    int backup = cctx->entry_copy[cctx->entry_pos];
    char buf[4]="|";
    cctx->entry_copy[cctx->entry_pos]=0;
    ctx_rgb (ctx, 1, 0, 0);
    ctx_text (ctx, cctx->entry_copy);
    ctx_rgb (ctx, 0, 0, 0);
    ctx_text (ctx, buf);

    buf[0]=backup;
    if (backup)
    {
      ctx_rgb (ctx, 1, 0, 0);
      ctx_text (ctx, buf);
      ctx_text (ctx, &cctx->entry_copy[cctx->entry_pos+1]);
      cctx->entry_copy[cctx->entry_pos] = backup;
    }
  }
  else
  {
    if (val[0])
    {
      ctx_text (ctx, val);
    }
    else
    {
      if (control->fallback)
      {
        ctx_gray (ctx, 0.4);
        ctx_text (ctx, control->fallback);
      }
    }
  }
  cctx->x += cctx->value_width * cctx->width;
  ui_text (cctx, label);

  ctx_restore (ctx);
  ui_newline (cctx);
}

void toggle_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  *val = (*val)?0:1;
  event->stop_propagate = 1;
  cctx->focus_no = control->no;
  cctx->dirty++;
}

void ui_toggle (CtxControls *cctx, const char *label, int *val)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->val = val;
  control->type = UI_TOGGLE;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, toggle_clicked, control, cctx, control_finalize, NULL);

  ctx_begin_path (ctx);
  ui_base (cctx, label, control->x, control->y, control->width, cctx->font_size * 2,
                        cctx->focus_no == control->no);

  ctx_rgba (ctx, 1, 0, 0, 1);
  ctx_move_to (ctx, cctx->x + cctx->font_size * cctx->value_pos, cctx->y + cctx->font_size * cctx->rel_baseline);
  if (*val)
  {
    ctx_text (ctx, "X");
  }
  else
  {
    ctx_text (ctx, "V");
  }

  ctx_rgba (ctx, 0, 0, 0, 1);
  cctx->x += ctx_text_width (ctx, "X") + cctx->font_size * cctx->value_pos;
  if (label)
  {
    ui_text (cctx, label);
  }

  ui_newline (cctx);
  ctx_restore (ctx);
}

static void button_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  if (control->action)
    control->action (control->val);
  event->stop_propagate = 1;
  cctx->dirty ++;
  cctx->focus_no = control->no;
  cctx->button_pressed = 1;
}

int ui_radio (CtxControls *cctx, const char *label, int set)
{
  Ctx *ctx = cctx->ctx;
  ctx_font_size (ctx, cctx->font_size);
  float width = ctx_text_width (ctx, label);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, width, cctx->font_size * cctx->rel_ver_advance);
  ui_base (cctx, label, cctx->x, cctx->y, width, cctx->font_size * cctx->rel_ver_advance, cctx->focus_no == control->no);
  ctx_rgb (ctx, 0, 0, 1);
  ctx_move_to (ctx, cctx->x,  cctx->y + cctx->font_size * cctx->rel_baseline);
  if (set)
  ctx_text (ctx, "X");
  else
  ctx_text (ctx, "O");
  ctx_text (ctx, label);

  control->type = UI_RADIO;
  ctx_rectangle (ctx, cctx->x, cctx->y, width, cctx->font_size * cctx->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, cctx, control_finalize, NULL);
  ctx_begin_path (ctx);
  cctx->x += width;

#if 1
  float px = ctx_pointer_x (ctx);
  float py = ctx_pointer_y (ctx);

  if (px >= control->x &&  px < control->x +  control->width &&
      py >= control->y &&  py < control->y +  control->height)
  {
    cctx->focus_no = control->no;
  }
#endif

  ui_newline (cctx);
  ctx_restore (ctx);
  if (control->no == cctx->focus_no && cctx->button_pressed)
  {
    cctx->button_pressed = 0;
    cctx->dirty++;
    return 1;
  }
   return 0;
}



void expander_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  *val = (*val)?0:1;
  cctx->focus_no = control->no;
  cctx->dirty++;
}



int ui_expander (CtxControls *cctx, const char *label, int *val)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->val = val;
  control->type = UI_EXPANDER;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, toggle_clicked, control, cctx, control_finalize, NULL);

  ctx_begin_path (ctx);
  {
     char *tmp = malloc (strlen (label) + 10);
     sprintf (tmp, "%s %s", *val?"V":"-", label);
     ui_base (cctx, label, control->x, control->y, control->width, cctx->font_size * 2,
              cctx->focus_no == control->no);
     ui_text (cctx, tmp);
     free (tmp);
  }

  ctx_restore (ctx);
  ui_newline (cctx);
  return *val;
}


int ui_button2 (CtxControls *cctx, const char *label)
{
  Ctx *ctx = cctx->ctx;
  ctx_font_size (ctx, cctx->font_size);
  float width = ctx_text_width (ctx, label);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, width, cctx->font_size * cctx->rel_ver_advance);
  ui_base (cctx, label, cctx->x, cctx->y, width, cctx->font_size * cctx->rel_ver_advance, cctx->focus_no == control->no);
  ctx_rgb (ctx, 0, 0, 1);
  ctx_move_to (ctx, cctx->x,  cctx->y + cctx->font_size * cctx->rel_baseline);
  ctx_text (ctx, label);

  control->type = UI_BUTTON;
  ctx_rectangle (ctx, cctx->x, cctx->y, width, cctx->font_size * cctx->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, cctx, control_finalize, NULL);
  ctx_begin_path (ctx);
  cctx->x += width;

#if 0
  float px = ctx_pointer_x (ctx);
  float py = ctx_pointer_y (ctx);

  if (px >= control->x &&  px < control->x +  control->width &&
      py >= control->y &&  py < control->y +  control->height)
  {
    cctx->focus_no = control->no;
  }
#endif


  ui_newline (cctx);
  if (control->no == cctx->focus_no && cctx->button_pressed)
  {
    cctx->button_pressed = 0;
    cctx->dirty++;
    return 1;
  }
  return 0;
}

int ui_button (CtxControls *cctx, const char *label, void (*action)(void *user_data), void *user_data)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 0, 1);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->action = action;
  control->val = user_data;
  control->type = UI_BUTTON;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, cctx, control_finalize, NULL);

  ctx_fill (ctx);
  ctx_begin_path (ctx);
  if (0)
  ui_base (cctx, label, control->x, control->y, control->width, cctx->font_size * 2,
           cctx->focus_no == control->no);
  ctx_font_size (ctx, cctx->font_size);
  ctx_rgb (ctx, 0,0,0);
  ui_text (cctx, label);

  ui_newline (cctx);
  ctx_restore (ctx);

  if (control->no == cctx->focus_no && cctx->button_pressed)
  {
    cctx->button_pressed = 0;
    cctx->dirty++;
    return 1;
  }
  return 0;
}

void choice_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  cctx->choice_active = 1;
  cctx->focus_no = control->no;
  event->stop_propagate = 1;
  cctx->dirty++;
  fprintf (stderr, "active!\n");
}


void ui_choice (CtxControls *cctx, const char *label, int *val, void (*action)(void *user_data), void *user_data)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->action = action;
  control->commit_data = user_data;
  control->val = val;
  control->type = UI_CHOICE;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  ctx_listen_with_finalize (ctx, CTX_CLICK, choice_clicked, control, cctx, control_finalize, NULL);

  ctx_fill (ctx);
  ctx_begin_path (ctx);
  ui_base (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, cctx->focus_no == control->no);
  cctx->x = cctx->x + cctx->width * cctx->value_width;
  ui_text (cctx, label);

  ctx_restore (ctx);
  ui_newline (cctx);
}

void ui_choice_add (CtxControls *cctx, int value, const char *label)
{
  Ctx *ctx = cctx->ctx;
  CtxControl *control = cctx->controls->data;
  {
    int *val = control->val;
    if (*val == value)
    {
      ctx_save (ctx);
      ctx_font_size (ctx, cctx->font_size);
      ctx_move_to (ctx, cctx->x + cctx->font_size * cctx->value_pos,
                      cctx->y + cctx->font_size * cctx->rel_baseline  - cctx->font_size * cctx->rel_ver_advance);
      ctx_rgb (ctx, 1, 0, 0);
      ctx_text (ctx, label);
      ctx_restore (ctx);
    }
  }
  if (control->no == cctx->focus_no)
  {
     UiChoice *choice= calloc (sizeof (UiChoice), 1);
     choice->val = value;
     choice->label = strdup (label);
     ctx_list_prepend (&cctx->choices, choice);
  }
}

void entry_commit (CtxControls *cctx)
{

  for (CtxList *l = cctx->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == cctx->focus_no)
    {
      switch (control->type)
      {
        case UI_ENTRY:
         if (cctx->entry_copy)
         {
           if (control->commit)
           {
             control->commit (control->commit_data);
           }
           else
           {
             strcpy (control->val, cctx->entry_copy);
           }
           free (cctx->entry_copy);
           cctx->entry_copy = NULL;
         }
      }
      return;
    }
  }
}

void ui_focus (CtxControls *cctx, int dir)
{
   entry_commit (cctx);
   cctx->focus_no += dir;

   if (cctx->focus_label){
     free (cctx->focus_label);
     cctx->focus_label = NULL;
   }

   int n_controls = ctx_list_length (cctx->controls);
   CtxList *iter = ctx_list_nth (cctx->controls, n_controls-cctx->focus_no-1);
   if (iter)
   {
     CtxControl *control = iter->data;
     cctx->focus_label = strdup (control->label);
     //fprintf (stderr, "%s\n", control_focus_label);
   }
}

CtxControl *ui_focused_control(CtxControls *cctx)
{
  for (CtxList *l = cctx->controls; l; l=l->next)
  {
    CtxControl *control = l->data;
    if (control->no == cctx->focus_no)
      return control;
  }
  return NULL;
}

void ui_key_up (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);

  if (control && control->type == UI_CHOICE && cctx->choice_active)
  {
    int *val = control->val;
    int old_val = *val;
    int prev_val = old_val;
    for (CtxList *l = cctx->choices; l; l=l?l->next:NULL)
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
    ui_focus (cctx, -1);
  }
  cctx->dirty++;
  event->stop_propagate = 1;
}

void ui_key_down (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (control && control->type == UI_CHOICE && cctx->choice_active)
  {
    int *val = control->val;
    int old_val = *val;
    for (CtxList *l = cctx->choices; l; l=l?l->next:NULL)
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
    ui_focus (cctx, 1);
  }
  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_key_tab (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  switch (control->type)
  {

  }
  ui_focus (cctx, 1);

  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_key_shift_tab (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  switch (control->type)
  {

  }
  ui_focus (cctx, -1);

  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_key_return (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  if (control->no == cctx->focus_no)
  {
    switch (control->type)
    {
      case UI_CHOICE:
       {
          cctx->choice_active = !cctx->choice_active;
       }
       break;
      case UI_ENTRY:
       {
         if (cctx->entry_copy)
         {
           entry_commit (cctx);
         }
         else
         {
           cctx->entry_copy = strdup (control->val);
           cctx->entry_pos = strlen (control->val);
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
          cctx->button_pressed=1;
        }
        break;
    }
  }
  event->stop_propagate=1;
  cctx->dirty++;
}

void ui_key_left (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_CHOICE:
      ui_key_return (event, data, data2);
      break;
    case UI_ENTRY:
      {
        if (cctx->entry_copy)
        {
          cctx->entry_pos --;
          if (cctx->entry_pos < 0) cctx->entry_pos = 0;
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
  cctx->dirty++;
}

void ui_key_right (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_BUTTON:
    case UI_CHOICE:
    case UI_RADIO:
      ui_key_return (event, data, data2);
      break;
    case UI_ENTRY:
      {
        if (cctx->entry_copy)
        {
          cctx->entry_pos ++;
          if (cctx->entry_pos > strlen (cctx->entry_copy))
            cctx->entry_pos = strlen (cctx->entry_copy);
        }
        else
        {
          ui_key_return (event, data, data2);
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
  cctx->dirty++;
}

void ui_key_backspace (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  switch (control->type)
  {
    case UI_ENTRY:
     {
       if (cctx->entry_copy && cctx->entry_pos > 0)
       {
         memmove (&cctx->entry_copy[cctx->entry_pos-1], &cctx->entry_copy[cctx->entry_pos],
                   strlen (&cctx->entry_copy[cctx->entry_pos] )+ 1);
         cctx->entry_pos --;
       }
     }
     break;
  }
  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_key_delete (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  if (strlen (cctx->entry_copy) > cctx->entry_pos)
  {
    ui_key_right (event, data, data2);
    ui_key_backspace (event, data, data2);
  }
  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_key_unhandled (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata;
  if (cctx->entry_copy)
    {
      const char *str = event->string;
      if (!strcmp (str, "space"))
        str = " ";

      char *tmp = malloc (strlen (cctx->entry_copy) + strlen (str) + 1);

      char *rest = strdup (&cctx->entry_copy[cctx->entry_pos]);
      cctx->entry_copy[cctx->entry_pos]=0;

      sprintf (tmp, "%s%s%s", cctx->entry_copy, str, rest);
      free (rest);
      cctx->entry_pos+=strlen(str);
      free (cctx->entry_copy);
      cctx->entry_copy = tmp;
      cctx->dirty++;
    }
  event->stop_propagate = 1;
}

void ui_key_bindings (CtxControls *cctx)
{
  Ctx *ctx = cctx->ctx;
  ctx_add_key_binding (ctx, "up", NULL, "focus prev",   ui_key_up,        cctx);
  ctx_add_key_binding (ctx, "down", NULL, "focus next", ui_key_down,      cctx);
  ctx_add_key_binding (ctx, "right", NULL, "",          ui_key_right,     cctx);
  ctx_add_key_binding (ctx, "left", NULL, "",           ui_key_left,      cctx);
  ctx_add_key_binding (ctx, "return", NULL, "",         ui_key_return,    cctx);
  ctx_add_key_binding (ctx, "backspace", NULL, "",      ui_key_backspace, cctx);
  ctx_add_key_binding (ctx, "delete", NULL, "",         ui_key_delete,    cctx);
  ctx_add_key_binding (ctx, "tab", NULL, "",            ui_key_tab,       cctx);
  ctx_add_key_binding (ctx, "shift-tab", NULL, "",      ui_key_shift_tab, cctx);
  ctx_add_key_binding (ctx, "unhandled", NULL, "",      ui_key_unhandled, cctx);
}

void ctx_choice_set (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  CtxControl *control = ui_focused_control (cctx);
  int *val = control->val;
  *val = (size_t)(data2);
  cctx->dirty++;
  event->stop_propagate = 1;
}

void ctx_event_block (CtxEvent *event, void *data, void *data2)
{
  CtxControls *cctx = data;
  cctx->choice_active = 0;
  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_done (CtxControls *cctx)
{
  Ctx *ctx = cctx->ctx;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;

  if (control->type == UI_CHOICE && cctx->choice_active)
  {
    ctx_save (ctx);
    ctx_rgb (ctx, 1,1,1);
    ctx_rectangle (ctx, control->x + cctx->font_size * cctx->value_pos,
                        control->y,
                        cctx->font_size * 4,
                        cctx->font_size * (ctx_list_length (cctx->choices) + 0.5));
    ctx_fill (ctx);
    ctx_rgb (ctx, 0,0,0);
    ctx_rectangle (ctx, control->x + cctx->font_size * cctx->value_pos,
                        control->y,
                        cctx->font_size * 4,
                        cctx->font_size * (ctx_list_length (cctx->choices) + 0.5));
    ctx_line_width (ctx, 1);
    ctx_stroke (ctx);

    ctx_font_size (ctx, cctx->font_size);
    int no = 0;

    ctx_rectangle (ctx, 0,0,ctx_width(ctx), ctx_height(ctx));
    ctx_listen (ctx, CTX_CLICK, ctx_event_block, cctx, NULL);
    ctx_begin_path (ctx);

    ctx_list_reverse (&cctx->choices);
    for (CtxList *l = cctx->choices; l; l = l->next, no++)
    {
      UiChoice *choice = l->data;
      ctx_rectangle (ctx, control->x + cctx->font_size * (cctx->value_pos),
                          control->y + cctx->font_size * (no),
                          cctx->font_size * 4,
                          cctx->font_size * 1.5);
      ctx_listen (ctx, CTX_CLICK, ctx_choice_set, cctx, (void*)((size_t)choice->val));
      ctx_begin_path (ctx);
      ctx_move_to (ctx, control->x + cctx->font_size * (cctx->value_pos + 0.5),
                        control->y + cctx->font_size * (no+1));
      int *val = control->val;
      if (choice->val == *val)
        ctx_rgb (ctx, 1,0,0);
      else
      ctx_rgb (ctx, 0,0,0);
      ctx_text (ctx, choice->label);
    }
    ctx_restore (ctx);
  }
}
