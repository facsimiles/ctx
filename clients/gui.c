#include "ctx.h"


enum {
  UI_SLIDER = 1,
  UI_EXPANDER,
  UI_TOGGLE,
  UI_LABEL,
  UI_BUTTON,
  UI_CHOICE,
  UI_ENTRY,
  UI_MENU,
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
  void (*commit)(CtxControl *control, void *commit_data);
  void *commit_data;
};

typedef struct _UiChoice  UiChoice;
struct _UiChoice
{
  int   val;
  char *label;
};

typedef struct _CtxControls CtxControls;
struct _CtxControls{
  Ctx *ctx;
  float x;
  float y;
  float font_size;
  float width;
  float value_pos;
  float rel_ver_advance;
  float rel_baseline;

  int   dirty;

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
};

CtxControls *cctx_new (Ctx *ctx)
{
  CtxControls *cctx     = calloc (sizeof (CtxControls), 1);
  cctx->ctx             = ctx;
  cctx->font_size       = 28;
  cctx->width           = cctx->font_size * 20;
  cctx->value_pos       = 6;
  cctx->rel_ver_advance = 1.2;
  cctx->rel_baseline    = 0.77;
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
  cctx->x = 10;
  cctx->y = 10;
}

CtxControl *add_control (CtxControls *cctx, const char *label, float x, float y, float width, float height)
{
  CtxControl *control = calloc (sizeof (CtxControl), 1);
  control->label = strdup (label);

  if (cctx->focus_label){
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
  return control;
}

static void ui_base (CtxControls *cctx, const char *label, float x, float y, float width, float height, int focused)
{
  Ctx *ctx = cctx->ctx;
  ctx_font_size (ctx, cctx->font_size);
  ctx_rgb (ctx, 1, 1, 1);

  ctx_rectangle (ctx, x, y, width, height+1);
  ctx_fill (ctx);
  ctx_move_to (ctx, x + cctx->font_size * 0.5, y + cctx->font_size * cctx->rel_baseline);

  ctx_rgb (ctx, 0, 0, 0);
  ctx_text (ctx, label);

  if (focused)
  {
    ctx_rgb (ctx, 1, 0, 0);
    ctx_rectangle (ctx, x+2, y+2, width-4, height-4);
    ctx_line_width (ctx, 2);
    ctx_stroke (ctx);
  }
}

static void ui_control_post (CtxControls *cctx)
{
  cctx->y += cctx->font_size * cctx->rel_ver_advance;
}

void ui_label (CtxControls *cctx, const char *label)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ui_base (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, 0);

  ctx_restore (ctx);
  ui_control_post (cctx);
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
  
  new_val = ((event->x - control->x) / control->width) * (control->max-control->min) + control->min;
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

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);

  ctx_listen_with_finalize (ctx, CTX_DRAG, slider_drag, control, cctx, control_finalize, NULL);
  ctx_begin_path (ctx);

  ui_base (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, cctx->focus_no == control->no);

  sprintf (buf, "%f", *val);
  ctx_move_to (ctx, cctx->x + cctx->font_size * cctx->value_pos, cctx->y + cctx->font_size * cctx->rel_baseline);
  ctx_rgb (ctx, 1, 0, 0);
  ctx_text (ctx, buf);

  float rel_val = ((*val) - min) / (max-min);
  ctx_rectangle (ctx, cctx->x + (cctx->width-cctx->font_size/4) * rel_val, cctx->y, cctx->font_size/4, control->height * 0.8);
  ctx_fill (ctx);

  ctx_restore (ctx);
  ui_control_post (cctx);
}


void entry_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  event->stop_propagate = 1;
  cctx->dirty++;
}

void ui_entry (CtxControls *cctx, const char *label, const char *fallback, char *val, int maxlen,
                  void (*commit)(CtxControl *control, void *commit_data),
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

  ctx_fill (ctx);

  ctx_begin_path (ctx);
  ui_base (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, cctx->focus_no == control->no);

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

  ctx_restore (ctx);
  ui_control_post (cctx);
}

void toggle_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  *val = (*val)?0:1;
  event->stop_propagate = 1;
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
  ui_base (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, cctx->focus_no == control->no);

  if (*val)
  {
    ctx_rgba (ctx, 1, 1, 0, 1);
  }
  else
  {
    ctx_rgba (ctx, 0.2, 0.6, 0, 1);
  }

  ctx_rectangle (ctx, cctx->x + cctx->width-cctx->font_size * 4,  cctx->y, cctx->font_size*3, cctx->font_size * 2);
  ctx_fill (ctx);

  ctx_restore (ctx);
  ui_control_post (cctx);
}

void expander_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  int *val = control->val;
  *val = (*val)?0:1;
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
     sprintf (tmp, "%s %s", *val?"V":">", label);
     ui_base (cctx, tmp, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, cctx->focus_no == control->no);
     free (tmp);
  }

  if (*val)
  {
    ctx_rgba (ctx, 1, 1, 0, 1);
  }
  else
  {
    ctx_rgba (ctx, 0.2, 0.6, 0, 1);
  }

  ctx_restore (ctx);
  ui_control_post (cctx);
  return *val;
}

void button_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControls *cctx = userdata2;
  CtxControl *control = userdata;
  control->action (control->val);
  event->stop_propagate = 1;
  cctx->dirty ++;
}

void ui_button (CtxControls *cctx, const char *label, void (*action)(void *user_data), void *user_data)
{
  Ctx *ctx = cctx->ctx;
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  CtxControl *control = add_control (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * cctx->rel_ver_advance);
  control->action = action;
  control->val = user_data;
  control->type = UI_BUTTON;

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * 2);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, cctx, control_finalize, NULL);

  ctx_fill (ctx);
  ctx_begin_path (ctx);
  ui_base (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, cctx->focus_no == control->no);

  ctx_rectangle (ctx, cctx->x + cctx->width-cctx->font_size * 4,  cctx->y, cctx->font_size*3, cctx->font_size * 2);
  ctx_fill (ctx);

  ctx_restore (ctx);
  ui_control_post (cctx);
}

void choice_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  CtxControl *control = userdata;
  //control->action (control->val);
  event->stop_propagate = 1;
}

static int ui_choice_active = 0;


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

  ctx_rectangle (ctx, cctx->x, cctx->y, cctx->width, cctx->font_size * 2);
  ctx_listen_with_finalize (ctx, CTX_CLICK, choice_clicked, control, cctx, control_finalize, NULL);

  ctx_fill (ctx);
  ctx_begin_path (ctx);
  ui_base (cctx, label, cctx->x, cctx->y, cctx->width, cctx->font_size * 2, cctx->focus_no == control->no);

  ctx_restore (ctx);
  ui_control_post (cctx);
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
             control->commit (control, control->commit_data);
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

   if (cctx->focus_label) free (cctx->focus_label);

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

void ui_key_up (CtxControls *cctx)
{
  CtxControl *control = ui_focused_control (cctx);

  if (control && control->type == UI_CHOICE && ui_choice_active)
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
}

void ui_key_down (CtxControls *cctx)
{
  CtxControl *control = ui_focused_control (cctx);
  if (control && control->type == UI_CHOICE && ui_choice_active)
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
}

void ui_key_left (CtxControls *cctx)
{
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  switch (control->type)
  {
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
}

void ui_key_return (CtxControls *cctx)
{
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  if (control->no == cctx->focus_no)
  {
    switch (control->type)
    {
      case UI_CHOICE:
       {
          ui_choice_active = !ui_choice_active;
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
      case UI_BUTTON:
        {
          control->action(control->val);
        }
        break;
    }
  }
}

void ui_key_right (CtxControls *cctx)
{
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_EXPANDER:
    case UI_BUTTON:
    case UI_CHOICE:
      ui_key_return (cctx);
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
          ui_key_return (cctx);
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
}

void ui_key_backspace (CtxControls *cctx)
{
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
  
}

void ui_key_delete (CtxControls *cctx)
{
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;
  if (strlen (cctx->entry_copy) > cctx->entry_pos)
  {
    ui_key_right (cctx);
    ui_key_backspace (cctx);
  }
}


void ui_done (CtxControls *cctx)
{
  Ctx *ctx = cctx->ctx;
  CtxControl *control = ui_focused_control (cctx);
  if (!control) return;

  if (control->type == UI_CHOICE && ui_choice_active)
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
    ctx_list_reverse (&cctx->choices);
    for (CtxList *l = cctx->choices; l; l = l->next, no++)
    {
      UiChoice *choice = l->data;
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

void pressed (void *userdata)
{
  fprintf (stderr, "pressed\n");
}

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  Ctx *ctx = ctx_new_ui (-1, -1);

#ifndef REALLY_TINY
  char *utf8 = "ctx\n";
#else
  char *utf8 = "";
#endif

  CtxControls *cctx = cctx_new (ctx);

  char message[256] = "hello there";

  const CtxEvent *event;
  int mx, my;
  int do_quit = 0;
  int   baz = 1;
  int   bax = 0;
  int chosen = 1;
  char input[10]="fnord";
    static int expanded = 0;
  ctx_get_event (ctx);

  event = (void*)0x1;
  //fprintf (stderr, "[%s :%i %i]", event, mx, my);
  cctx->dirty = 1;
  while (!do_quit)
  {
    if (cctx->dirty)
    {
    cctx->dirty=0;
    ctx_reset                 (ctx);
    float width  = ctx_width  (ctx);
    float height = ctx_height (ctx);
    ui_reset                  (cctx);

    ctx_save                  (ctx);
    ctx_rectangle             (ctx, 0, 0, width, height);
    //ctx_compositing_mode      (ctx, CTX_COMPOSITE_CLEAR);
    ctx_gray (ctx, 0);
    ctx_fill                  (ctx);
    ctx_restore               (ctx);
    ctx_move_to               (ctx, 10, height * 0.2);
    //ctx_font_size  (ctx, height * 0.1 + x/4.0);
    ctx_line_width            (ctx, 2);
    ctx_rgba                  (ctx, 0, 0, 0, 1);
    ctx_text_stroke           (ctx, utf8);
    ctx_rgba8                 (ctx, 255, 255, 255, 255);
    ctx_move_to               (ctx, height * 0.05, height * 0.2);
    ctx_text                  (ctx, utf8);
    //ctx_font_size  (ctx, height * 0.2);

    ctx_move_to               (ctx, height * 0.05, height * 0.4);
    ctx_rgb                   (ctx, 1, 0,0);
    ctx_text                  (ctx, message);

#if 1
    ui_label  (cctx, "Test UI");
    ui_slider (cctx, "font-size: ", &cctx->font_size, 5.0, 100.0, 0.1);
    ui_slider (cctx, "width: ", &cctx->width, 5.0, 600.0, 1.0);
    ui_slider (cctx, "ver_advance: ", &cctx->rel_ver_advance, 0.1, 4.0, 0.01);
    ui_slider (cctx, "baseline: ", &cctx->rel_baseline, 0.1, 4.0, 0.01);
    ui_slider (cctx, "value_pos: ", &cctx->value_pos, 0.0, 40.0, 0.1);

    if (ui_expander (cctx, "More..", &expanded))
    {
    ui_entry  (cctx, "input: ", "ba", input, sizeof(input)-1, NULL, NULL);
    ui_choice (cctx, "food: ", &chosen, NULL, NULL);
    ui_choice_add (cctx, 0, "fruit");
    ui_choice_add (cctx, 1, "chicken");
    ui_choice_add (cctx, 2, "potato");
    ui_choice_add (cctx, 3, "rice");
    ui_choice (cctx, "power: ", &chosen, NULL, NULL);
    ui_choice_add (cctx, 0, "on");
    ui_choice_add (cctx, 1, "off");
    ui_choice_add (cctx, 2, "good");
    ui_choice_add (cctx, 2025, "green");
    ui_choice_add (cctx, 2030, "electric");
    ui_choice_add (cctx, 2040, "novel");

    ui_toggle (cctx, "baz: ", &baz);
    ui_toggle (cctx, "bax: ", &bax);
    ui_button (cctx, "press me", pressed, NULL);
    ui_done (cctx);
    }



    //ctx_clear_bindings (ctx);
#endif

#if 0
    ctx_rectangle (ctx, x,y,height * 0.2,height * 0.2);
    ctx_listen    (ctx, CTX_DRAG, red_rect, NULL, NULL);
    ctx_fill (ctx);
    ctx_rgb   (ctx, 1, 1, 1);
    ctx_move_to (ctx, x, y + height *  0.1);
    ctx_text (ctx, "drag me");

    ctx_rgb        (ctx, 0, 1,0);
    ctx_rectangle (ctx, 0,height * 0.8,height * 0.2,height * 0.2);
    ctx_listen    (ctx, CTX_PRESS, green_rect, NULL, NULL);
    ctx_fill (ctx);
#endif
    
    if (ctx_pointer_is_down (ctx, 0))
    {
      ctx_arc      (ctx, mx, my, 5.0, 0.0, CTX_PI*2, 0);
      ctx_rgba (ctx, 1, 1, 1, 0.5);
      ctx_fill     (ctx);
    }

    ctx_flush          (ctx);
    }
    else
    {
      usleep (10000);
    }

    int max_events_in_a_row = 8; /* without this - and without rate limiting
                                     elsewhere, motion events can keep drawing
                                     from happening */
#if 1
    while ((event = ctx_get_event (ctx)) &&  (max_events_in_a_row--))
    {
    switch (event->type)
    {
       case CTX_MOTION:
         mx = event->x;
         my = event->y;
         sprintf (message, "%s %.1f %.1f", 
                          ctx_pointer_is_down (ctx, 0)?
                         "drag":"motion", event->x, event->y);
         break;
       case CTX_PRESS:
         mx = event->x;
         my = event->y;
         sprintf (message, "press %.1f %.1f", event->x, event->y);
         break;
       case CTX_RELEASE:
         mx = event->x;
         my = event->y;
         sprintf (message, "release %.1f %.1f", event->x, event->y);
         break;
       case CTX_DRAG:
         mx = event->x;
         my = event->y;
         sprintf (message, "drag %.1f %.1f", event->x, event->y);
         break;
       case CTX_KEY_DOWN:
         cctx->dirty++;
         if (!strcmp (event->string, "up"))
         {
           ui_key_up (cctx);
         }
         else
         if (!strcmp (event->string, "down"))
         {
           ui_key_down (cctx);
         }
         else if (!strcmp (event->string, "right"))
         {
           ui_key_right (cctx);
         }
         else if (!strcmp (event->string, "left"))
         {
           ui_key_left (cctx);
         }
         else if (!strcmp (event->string, "return"))
         {
           ui_key_return (cctx);
         }
         else if (!strcmp (event->string, "backspace"))
         {
           ui_key_backspace (cctx);
         }
         else if (!strcmp (event->string, "delete"))
         {
           ui_key_delete (cctx);
         }
         else if (!strcmp (event->string, "control-q"))
         {
            do_quit = 1;
            fprintf (stderr, "quit!\n");
         }
         else
         if (strcmp (event->string, "idle"))
         {
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
           }
           else
           {
             sprintf (message, "key %s", event->string);
           }
         }
         break;
       default:
         break;
    }
    }
#endif
  }
  ctx_free (ctx);
  return 0;
}
