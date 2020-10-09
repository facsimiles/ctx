#include "ctx.h"

float ui_x = 10;
float ui_y = 10;
#define ui_font_size 25
float ui_width = ui_font_size * 10;

static int   control_focus_no = 0;
static int   control_focus_x = 0;
static int   control_focus_y = 0;
static int   control_focus_width = 0;
static char *control_focus_label = NULL;


char *ui_entry_copy = NULL;
int   ui_entry_pos = 0;
float ui_rel_val    = 4.0;
float ui_rel_height = 1.3;

enum {
  UI_SLIDER = 1,
  UI_TOGGLE,
  UI_LABEL,
  UI_BUTTON,
  UI_CHOICE,
  UI_ENTRY,
  UI_MENU,
};

typedef struct _UiControl UiControl;
struct _UiControl{
  int no;
  int ref_count;
  int type;
  float x;
  float y;
  float width;
  float height;
  char *label;
  char *fallback;
  void *val;
  float min;
  float max;
  float step;
  void (*action)(void *user_data);
  void (*commit)(UiControl *control, void *commit_data);
  void *commit_data;
};

typedef struct _CtxControls CtxControls;
struct _CtxControls{
  Ctx *ctx;

};


void control_unref (UiControl *control)
{
  if (control->ref_count < 0)
  {
    UiControl *w = control;

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

CtxList *old_controls = NULL;
CtxList *controls = NULL;
int control_no = 0;
typedef struct _UiChoice  UiChoice;
struct _UiChoice
{
  int   val;
  char *label;
};
static CtxList *choices = NULL;

void ui_reset (Ctx *ctx)
{
  while (old_controls)
  {
    UiControl *control = old_controls->data;
    control_unref (control);
    ctx_list_remove (&old_controls, control);
  }
  old_controls = controls;
  controls = NULL;
  while (choices)
  {
    UiChoice *choice = choices->data;
    free (choice->label);
    free (choice);
    ctx_list_remove (&choices, choice);
  }
  control_no = 0;
  ui_x = 10;
  ui_y = 10;
}

UiControl *add_control (const char *label, float x, float y, float width, float height)
{
  UiControl *control = calloc (sizeof (UiControl), 1);
  control->label = strdup (label);

  if (control_focus_label){
     if (!strcmp (control_focus_label, label))
     {
       control_focus_no = control_no;
     }
  }

  control->x = x;
  control->y = y;
  control->no = control_no;
  control_no++;
  control->width = width;
  control->height = height;
  ctx_list_prepend (&controls, control);
  return control;
}

static void ui_base (Ctx *ctx, const char *label, float x, float y, float width, float height, int focused)
{
  ctx_font_size (ctx, ui_font_size);
  ctx_rgb (ctx, 1, 1, 1);

  ctx_rectangle (ctx, x, y, width, height+1);
  ctx_fill (ctx);
  ctx_move_to (ctx, x + ui_font_size * 0.5, y + ui_font_size * 1.2);

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

static void ui_control_post (Ctx *ctx)
{
  ui_y += ui_font_size * ui_rel_height;
}

void ui_label (Ctx *ctx, const char *label)
{
  char buf[100] = "";
  ctx_save (ctx);
  ui_base (ctx, label, ui_x, ui_y, ui_width, ui_font_size * 2, 0);

  ctx_restore (ctx);
  ui_control_post (ctx);
}

static void ui_float_constrain (UiControl *control, float *val)
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
  UiControl *control = userdata;
  float *val = control->val;
  float new_val;
  
  new_val = ((event->x - control->x) / control->width) * (control->max-control->min) + control->min;
  ui_float_constrain (control, &new_val);

  *val = new_val;
  event->stop_propagate = 1;
}



void ui_slider (Ctx *ctx, const char *label, float *val, float min, float max, float step)
{
  char buf[100] = "";
  ctx_save (ctx);
  ctx_font_size (ctx, ui_font_size);
  ctx_rgb (ctx, 1, 1, 1);
  UiControl *control = add_control (label, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);
  control->min = min;
  control->max = max;
  control->step = step;
  control->val = val;
  control->type = UI_SLIDER;
  control->ref_count++;
  control->ref_count++;

  ctx_rectangle (ctx, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);

  ctx_listen_with_finalize (ctx, CTX_DRAG, slider_drag, control, NULL, control_finalize, NULL);
  ctx_begin_path (ctx);

  ui_base (ctx, label, ui_x, ui_y, ui_width, ui_font_size * 2, control_focus_no == control->no);

  sprintf (buf, "%f", *val);
  ctx_move_to (ctx, ui_x + ui_font_size * ui_rel_val, ui_y + ui_font_size * 1.2);
  ctx_rgb (ctx, 1, 0, 0);
  ctx_text (ctx, buf);

  float rel_val = ((*val) - min) / (max-min);
  ctx_rectangle (ctx, ui_x + (ui_width-ui_font_size/4) * rel_val, ui_y, ui_font_size/4, control->height * 0.8);
  ctx_fill (ctx);

  ctx_restore (ctx);
  ui_control_post (ctx);
}


void entry_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  event->stop_propagate = 1;
}

void ui_entry (Ctx *ctx, const char *label, const char *fallback, char *val, int maxlen,
                  void (*commit)(UiControl *control, void *commit_data),
                                void *commit_data)
{
  char buf[100] = "";
  ctx_save (ctx);
  UiControl *control = add_control (label, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);
  control->val = val;
  control->type = UI_ENTRY;
  if (fallback)
    control->fallback = strdup (fallback);
  control->ref_count++;
  control->ref_count++;
  control->commit = commit;
  control->commit_data = commit_data;

  ctx_rectangle (ctx, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);

  ctx_listen_with_finalize (ctx, CTX_CLICK, entry_clicked, control, NULL, control_finalize, NULL);

  ctx_fill (ctx);

  ctx_begin_path (ctx);
  ui_base (ctx, label, ui_x, ui_y, ui_width, ui_font_size * 2, control_focus_no == control->no);

  ctx_move_to (ctx, ui_x + ui_font_size * ui_rel_val, ui_y + ui_font_size * 1.2);
  ctx_rgb (ctx, 1, 0, 0);
  if (ui_entry_copy)
  {
    int backup = ui_entry_copy[ui_entry_pos];
    char buf[4]="|";
    ui_entry_copy[ui_entry_pos]=0;
    ctx_rgb (ctx, 1, 0, 0);
    ctx_text (ctx, ui_entry_copy);
    ctx_rgb (ctx, 0, 0, 0);
    ctx_text (ctx, buf);

    buf[0]=backup;
    if (backup)
    {
      ctx_rgb (ctx, 1, 0, 0);
      ctx_text (ctx, buf);
      ctx_text (ctx, &ui_entry_copy[ui_entry_pos+1]);
      ui_entry_copy[ui_entry_pos] = backup;
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
  ui_control_post (ctx);
}

void toggle_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  UiControl *control = userdata;
  int *val = control->val;
  if (*val)
  {
    *val = 0;
  }
  else
  {
    *val = 1;
  }
  event->stop_propagate = 1;
}

void ui_toggle (Ctx *ctx, const char *label, int *val)
{
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  UiControl *control = add_control (label, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);
  control->val = val;
  control->type = UI_TOGGLE;

  ctx_rectangle (ctx, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);
  ctx_listen_with_finalize (ctx, CTX_CLICK, toggle_clicked, control, NULL, control_finalize, NULL);

  ctx_begin_path (ctx);
  ui_base (ctx, label, ui_x, ui_y, ui_width, ui_font_size * 2, control_focus_no == control->no);

  if (*val)
  {
    ctx_rgba (ctx, 1, 1, 0, 1);
  }
  else
  {
    ctx_rgba (ctx, 0.2, 0.6, 0, 1);
  }

  ctx_rectangle (ctx, ui_x + ui_width-ui_font_size * 4,  ui_y, ui_font_size*3, ui_font_size * 2);
  ctx_fill (ctx);

  ctx_restore (ctx);
  ui_control_post (ctx);
}

void button_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  UiControl *control = userdata;
  control->action (control->val);
  event->stop_propagate = 1;
}

void ui_button (Ctx *ctx, const char *label, void (*action)(void *user_data), void *user_data)
{
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  UiControl *control = add_control (label, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);
  control->action = action;
  control->val = user_data;
  control->type = UI_BUTTON;

  ctx_rectangle (ctx, ui_x, ui_y, ui_width, ui_font_size * 2);
  ctx_listen_with_finalize (ctx, CTX_CLICK, button_clicked, control, NULL, control_finalize, NULL);

  ctx_fill (ctx);
  ctx_begin_path (ctx);
  ui_base (ctx, label, ui_x, ui_y, ui_width, ui_font_size * 2, control_focus_no == control->no);

  ctx_rectangle (ctx, ui_x + ui_width-ui_font_size * 4,  ui_y, ui_font_size*3, ui_font_size * 2);
  ctx_fill (ctx);

  ctx_restore (ctx);
  ui_control_post (ctx);
}

void choice_clicked (CtxEvent *event, void *userdata, void *userdata2)
{
  UiControl *control = userdata;
  //control->action (control->val);
  event->stop_propagate = 1;
}

static int ui_choice_active = 0;


void ui_choice (Ctx *ctx, const char *label, int *val, void (*action)(void *user_data), void *user_data)
{
  char buf[100] = "";
  ctx_save (ctx);
  ctx_rgb (ctx, 1, 1, 1);
  UiControl *control = add_control (label, ui_x, ui_y, ui_width, ui_font_size * ui_rel_height);
  control->action = action;
  control->commit_data = user_data;
  control->val = val;
  control->type = UI_CHOICE;

  ctx_rectangle (ctx, ui_x, ui_y, ui_width, ui_font_size * 2);
  ctx_listen_with_finalize (ctx, CTX_CLICK, choice_clicked, control, NULL, control_finalize, NULL);

  ctx_fill (ctx);
  ctx_begin_path (ctx);
  ui_base (ctx, label, ui_x, ui_y, ui_width, ui_font_size * 2, control_focus_no == control->no);

  ctx_restore (ctx);
  ui_control_post (ctx);
}

void ui_choice_add (Ctx *ctx, int value, const char *label)
{
  UiControl *control = controls->data;
  {
    int *val = control->val;
    if (*val == value)
    {
      ctx_save (ctx);
      ctx_font_size (ctx, ui_font_size);
      ctx_move_to (ctx, ui_x + ui_font_size * ui_rel_val,
                      ui_y + ui_font_size * 1.2 - ui_font_size * ui_rel_height);
      ctx_rgb (ctx, 1, 0, 0);
      ctx_text (ctx, label);
      ctx_restore (ctx);
    }
  }
  if (control->no == control_focus_no)
  {
     UiChoice *choice= calloc (sizeof (UiChoice), 1);
     choice->val = value;
     choice->label = strdup (label);
     ctx_list_prepend (&choices, choice);
  }
}

void ui_entry_commit (void)
{

  for (CtxList *l = controls; l; l=l->next)
  {
    UiControl *control = l->data;
    if (control->no == control_focus_no)
    {
      switch (control->type)
      {
        case UI_ENTRY:
         if (ui_entry_copy)
         {
           if (control->commit)
           {
             control->commit (control, control->commit_data);
           }
           else
           {
             strcpy (control->val, ui_entry_copy);
           }
           free (ui_entry_copy);
           ui_entry_copy = NULL;
         }
      }
      return;
    }
  }
}

void ui_focus (int dir)
{
   ui_entry_commit ();
   control_focus_no += dir;

   if (control_focus_label) free (control_focus_label);

   int n_controls = ctx_list_length (controls);
   CtxList *iter = ctx_list_nth (controls, n_controls-control_focus_no-1);
   if (iter)
   {
     UiControl *control = iter->data;
     control_focus_label = strdup (control->label);
     fprintf (stderr, "%s\n", control_focus_label);
   }
}

UiControl *ui_focused_control(void)
{
  for (CtxList *l = controls; l; l=l->next)
  {
    UiControl *control = l->data;
    if (control->no == control_focus_no)
      return control;
  }
  return NULL;
}

void ui_key_up ()
{
  UiControl *control = ui_focused_control ();

  if (control && control->type == UI_CHOICE && ui_choice_active)
  {
    int *val = control->val;
    int old_val = *val;
    int prev_val = old_val;
    for (CtxList *l = choices; l; l=l?l->next:NULL)
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
    ui_focus (-1);
  }
}

void ui_key_down ()
{
  UiControl *control = ui_focused_control ();
  if (control && control->type == UI_CHOICE && ui_choice_active)
  {
    int *val = control->val;
    int old_val = *val;
    for (CtxList *l = choices; l; l=l?l->next:NULL)
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
    ui_focus (1);
  }
}

void ui_key_left (void)
{
  UiControl *control = ui_focused_control ();
  if (!control) return;
  switch (control->type)
  {
    case UI_ENTRY:
      {
        if (ui_entry_copy)
        {
          ui_entry_pos --;
          if (ui_entry_pos < 0) ui_entry_pos = 0;
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



void ui_key_return (void)
{
  UiControl *control = ui_focused_control ();
  if (!control) return;
  if (control->no == control_focus_no)
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
         if (ui_entry_copy)
         {
           ui_entry_commit ();
         }
         else
         {
           ui_entry_copy = strdup (control->val);
           ui_entry_pos = strlen (control->val);
         }
       }
       break;
      case UI_SLIDER:
        {
           // XXX edit value
        }
        break;
      case UI_TOGGLE:
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

void ui_key_right (void)
{
  UiControl *control = ui_focused_control ();
  if (!control) return;
  switch (control->type)
  {
    case UI_TOGGLE:
    case UI_BUTTON:
    case UI_CHOICE:
      ui_key_return ();
      break;
    case UI_ENTRY:
      {
        if (ui_entry_copy)
        {
          ui_entry_pos ++;
          if (ui_entry_pos > strlen (ui_entry_copy))
            ui_entry_pos = strlen (ui_entry_copy);
        }
        else
        {
          ui_key_return ();
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

void ui_key_backspace (void)
{
  UiControl *control = ui_focused_control ();
  if (!control) return;
  switch (control->type)
  {
    case UI_ENTRY:
     {
       if (ui_entry_copy && ui_entry_pos > 0)
       {
         memmove (&ui_entry_copy[ui_entry_pos-1], &ui_entry_copy[ui_entry_pos],
                   strlen (&ui_entry_copy[ui_entry_pos] )+ 1);
         ui_entry_pos --;
       }
     }
     break;
  }
  
}

void ui_key_delete (void)
{
  UiControl *control = ui_focused_control ();
  if (!control) return;
  if (strlen (ui_entry_copy) > ui_entry_pos)
  {
    ui_key_right ();
    ui_key_backspace ();
  }
}


void ui_done (Ctx *ctx)
{
  UiControl *control = ui_focused_control ();
  if (!control) return;

  if (control->type == UI_CHOICE && ui_choice_active)
  {
    ctx_save (ctx);
    ctx_rgb (ctx, 1,1,1);
    ctx_rectangle (ctx, control->x + ui_font_size * ui_rel_val,
                        control->y,
                        ui_font_size * 4,
                        ui_font_size * (ctx_list_length (choices) + 0.5));
    ctx_fill (ctx);
    ctx_rgb (ctx, 0,0,0);
    ctx_rectangle (ctx, control->x + ui_font_size * ui_rel_val,
                        control->y,
                        ui_font_size * 4,
                        ui_font_size * (ctx_list_length (choices) + 0.5));
    ctx_line_width (ctx, 1);
    ctx_stroke (ctx);

    ctx_font_size (ctx, ui_font_size);
    int no = 0;
    ctx_list_reverse (&choices);
    for (CtxList *l = choices; l; l = l->next, no++)
    {
      UiChoice *choice = l->data;
      ctx_move_to (ctx, control->x + ui_font_size * (ui_rel_val + 0.5),
                        control->y + ui_font_size * (no+1));
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

  char message[256] = "hello there";

  const CtxEvent *event;
  int mx, my;
  int do_quit = 0;
  float foo = 0.42;
  float bar = 0.24;
  int   baz = 1;
  int   bax = 0;
  int chosen = 1;
  char input[10]="fnord";

  event = (void*)0x1;
  //fprintf (stderr, "[%s :%i %i]", event, mx, my);
  //
  while (!do_quit)
  {
    ctx_reset                 (ctx);
    float width  = ctx_width  (ctx);
    float height = ctx_height (ctx);
    ui_reset                  (ctx);
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
    ui_label  (ctx, "Test UI");
    ui_entry  (ctx, "input: ", "ba", input, sizeof(input)-1, NULL, NULL);
    ui_slider (ctx, "foo:2", &foo, 0.0, 1.0, 0.001);
    ui_slider (ctx, "foo: ", &foo, 0.0, 1.0, 0.25);

    if (bax)
    {
    ui_choice (ctx, "food: ", &chosen, NULL, NULL);
    ui_choice_add (ctx, 0, "fruit");
    ui_choice_add (ctx, 1, "chicken");
    ui_choice_add (ctx, 2, "potato");
    ui_choice_add (ctx, 3, "rice");
    }
    ui_choice (ctx, "power: ", &chosen, NULL, NULL);
    ui_choice_add (ctx, 0, "on");
    ui_choice_add (ctx, 1, "off");
    ui_choice_add (ctx, 2, "good");
    ui_choice_add (ctx, 2025, "green");
    ui_choice_add (ctx, 2030, "electric");
    ui_choice_add (ctx, 2040, "novel");

    ui_slider (ctx, "bar: ", &bar, 0.0, 100.0, 0.3333333333);
    ui_toggle (ctx, "baz: ", &baz);
    ui_toggle (ctx, "bax: ", &bax);
    ui_button (ctx, "press me", pressed, NULL);
    ui_done (ctx);



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
         if (!strcmp (event->string, "up"))
         {
           ui_key_up ();
         }
         else
         if (!strcmp (event->string, "down"))
         {
           ui_key_down ();
         }
         else if (!strcmp (event->string, "right"))
         {
           ui_key_right ();
         }
         else if (!strcmp (event->string, "left"))
         {
           ui_key_left ();
         }
         else if (!strcmp (event->string, "return"))
         {
           ui_key_return ();
         }
         else if (!strcmp (event->string, "backspace"))
         {
           ui_key_backspace ();
         }
         else if (!strcmp (event->string, "delete"))
         {
           ui_key_delete ();
         }
         else if (!strcmp (event->string, "control-q"))
         {
            do_quit = 1;
            fprintf (stderr, "quit!\n");
         }
         else
         if (strcmp (event->string, "idle"))
         {
           if (ui_entry_copy)
           {
             const char *str = event->string;
             if (!strcmp (str, "space"))
               str = " ";

             char *tmp = malloc (strlen (ui_entry_copy) + strlen (str) + 1);

             char *rest = strdup (&ui_entry_copy[ui_entry_pos]);
             ui_entry_copy[ui_entry_pos]=0;

             sprintf (tmp, "%s%s%s", ui_entry_copy, str, rest);
             free (rest);
             ui_entry_pos+=strlen(str);
             free (ui_entry_copy);
             ui_entry_copy = tmp;
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
