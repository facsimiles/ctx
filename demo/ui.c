#include "port_config.h"
#include "ui.h"
#include <dirent.h>

static void ui_set_color_a (Ctx *ctx, float *rgba, float alpha);
static void ui_set_color (Ctx *ctx, float *rgba);
#define UI_ID_STR(label) ((void*)(size_t)(ctx_strhash(label)|1))
#define UI_ID ((void*)(__LINE__*2))
#define em (ui->font_size)
int demo_mode = 1;
static void ui_osk_draw (Ui *ui);
#ifdef CTX_FLOW3R
static bool is_touch     = false;
#else
static bool is_touch     = true;
#endif
static bool osk_captouch = false;

int  ctx_osk_mode = 0;




////////////////////////////////////////////////////////

static float backlight  = 100.0;
static void ui_backlight (float backlight)
{
   static float prev_backlight = -10.0f;
   if (prev_backlight != backlight)
   {
#if CTX_ESP
     esp_backlight (backlight);
#endif
     prev_backlight = backlight;
   }

}

void view_settings (Ui *ui)
{
   ui_start (ui);

   ui->y += ui->height * 0.05;
   ui_text(ui,"settings");

   if (ui_button(ui, "ui"))   ui_do(ui, "settings-ui");
   if (ui_button(ui, "wifi")) ui_do(ui, "wifi");

   backlight = ui_slider(ui,"backlight", 0.0f, 100.0f, 5.0, backlight);

   ui_backlight (backlight);

#if CTX_FLOW3R
   flow3r_synthesize_key_events = ui_toggle(ui,"cap-touch keys", flow3r_synthesize_key_events);
#endif
   ui_end(ui);
}

void view_settings_ui (Ui *ui)
{
   ui_start (ui);
   //static float backlight  = 100.0;
   float line_height = ui->line_height;

   ui->y += ui->height * 0.05;
   ui_text(ui,"settings/ui");

   ui->font_size = ui_slider(ui,"font size", 18,45,0.5, ui->font_size);
   ui->show_fps  = ui_toggle(ui,"show fps", ui->show_fps);


   if (ui->gradient_bg)
     ui_text(ui,"Background top");
   else
     ui_text(ui,"Background RGB");

   ui->style.bg[0] = ui_slider_coords (ui, UI_ID, ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, ui->style.bg[0]);
   ui->y += line_height;
   ui->style.bg[1] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, ui->style.bg[1]);
   ui->y += line_height;
   ui->style.bg[2] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, ui->style.bg[2]);
   ui->y += line_height;

   ui->gradient_bg = ui_toggle(ui,"gradient bg", ui->gradient_bg);

   if (ui->gradient_bg)
   {
     ctx_move_to (ui->ctx, ui->width * 0.5f, ui->y+em);
     ctx_text (ui->ctx, "Background bottom");
     ui->y += em;
   ui->style.bg2[0] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, ui->style.bg2[0]);
     ui->y += line_height;
   ui->style.bg2[1] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, ui->style.bg2[1]);
     ui->y += line_height;
   ui->style.bg2[2] = ui_slider_coords (ui, UI_ID,ui->width * 0.1f, ui->y, ui->width * 0.8f, line_height, 0,1,8/255.0, ui->style.bg2[2]);
     ui->y += line_height;
   }

   ui->interactive_debug = ui_toggle(ui,"show interaction zones", ui->interactive_debug);
   ui->style.interactive_bg[3] = ui->interactive_debug ? 0.3f : 0.0f;

   ui_end(ui);
}


void ui_register_view (Ui         *ui,
                       const char *name,
                       const char *category,
                       ui_fun      fun)
{
  if (ui->n_views + 1 >= 64) return;
  ui->views[ui->n_views].name = strdup (name);
  ui->views[ui->n_views].category = strdup (category);
  ui->views[ui->n_views].fun = fun;
  ui->n_views++;
}

#define UI_MAX_HISTORY 16

typedef struct ui_history_item {
  char *view;
  float scroll_offset;
  void *focused;
  ui_fun fun;
  void *data;
  ui_data_finalize data_finalize;
} ui_history_item;

static int   history_count = 0;
ui_history_item history[UI_MAX_HISTORY];

void
ui_load_view(Ui *ui, const char *target);

static void save_state(Ui *ui)
{
  if (history_count + 1 < UI_MAX_HISTORY)
  {
    if (history[history_count].view)
      free(history[history_count].view);
    if (ui->location == NULL)
    {
      history[history_count].view = strdup (ui->views[0].name);
    }
    else
      history[history_count].view = strdup (ui->location);
      
    history[history_count].focused = ui->focused_id;
    history[history_count].fun = ui->fun;
    history[history_count].data = ui->data;
    history[history_count].data_finalize = ui->data_finalize;
    history[history_count].scroll_offset = ui->scroll_offset;
    history_count++;
  }
  else
  {
    printf ("history overflow\n");
  }
}

static void restore_state(Ui *ui)
{
    if (history_count>0)
    {
       int no = history_count-1;
       if (ui->location)
         free (ui->location);
       ui->location = history[no].view;
       history[no].view = NULL;
       ui->fun = history[no].fun;
       ui->data = history[no].data;
       ui->data_finalize = history[no].data_finalize;
       history_count  -= 1;
       ui->queued_next = 0;
       ui_set_scroll_offset (ui, history[no].scroll_offset);
       ui->focused_id = history[no].focused;
    }
    else printf ("tried to restore without history\n");
}

void
ui_pop_fun(Ui *ui)
{
  if (ui->data && ui->data_finalize)
    ui->data_finalize (ui->data);
  restore_state (ui);
}

static void ui_unhandled (Ui *ui)
{
   ui_start (ui);
   ui_text(ui,"unknown view");
   ui_text(ui,ui->location);
   ui_end (ui);
}

void ui_load_file (Ui *ui, const char *path)
{
  FILE *file = fopen(ui->location, "rb");
  if (file)
  {
     fseek(file, 0, SEEK_END);
     long length = ftell(file);
     fseek(file, 0, SEEK_SET);
     ui->data = malloc(length + 1);
     fread(ui->data, length, 1, file);
     fclose(file);
     ((char*)ui->data)[length] = 0;
     ui->data_finalize = free;
  }
}


static void ui_view_file (Ui *ui)
{
   ui_start (ui);
   ui_text(ui,"file");
   ui_text(ui,ui->location);

   for (int i = 0; i < ui->n_views; i++)
   {
     const char *cat = ui->views[i].category;
     if (cat && 
         strstr (ui->location, cat) &&
         (strstr (ui->location, cat)[strlen(cat)]==0))
     {
        ui->fun = ui->views[i].fun;
        break;
     }
   }

   ui_end (ui);
}

static void ui_view_dir (Ui *ui)
{
   ui_start (ui);
   ui_text(ui,ui->location);

   int n = 0;

   DIR *dir = opendir(ui->location);

   if (dir)
   {
   struct dirent *ent;

   while ((ent = readdir(dir)))
   {
     const char *basename = ent->d_name;

     if (basename[0] == '.')
       continue;

     if (ui->y > ui->height)
     {
     }
     else if (ui->y > -ui->font_size)
     {
       if (ui_button(ui, basename))
       {
         char *target = malloc (strlen (ui->location) + 3 + strlen (basename));
         if (target)
         {
           if (ui->location[strlen(ui->location)-1]=='/')
             sprintf(target, "%s%s", ui->location, basename);
           else
             sprintf(target, "%s/%s", ui->location, basename);
           ui_do(ui, target);
           free(target);
         }
       }
     }
     else
     {
       ui_text(ui,basename);

     }
     n++;
   }
   closedir(dir);
   }

   ui_end (ui);
}

static void ui_view_404 (Ui *ui)
{
   ui_start (ui);
   ui_text(ui,"404");
   ui_text(ui,ui->location);
   ui_end (ui);
}

void
ui_push_fun(Ui *ui, ui_fun fun, const char *location, void *data, ui_data_finalize data_finalize)
{
  if (ui->fun)
    save_state(ui);
  if (ui->location)
    free (ui->location);
  ui->location = NULL;
  if (location)
    ui->location = strdup (location);

  ui->focused_id = NULL;
  ui_set_scroll_offset (ui, 0);

  //ctx_osk_mode = 0;
  ui->frame_no = 0;
  ui->view_elapsed = 0; 

  if (ui->focus_first)
  {
    ui->queued_next = 2;
  }

  ui->fun = fun;
  ui->data = data;
  ui->data_finalize = data_finalize;
  ui->overlay_fade = 0.7;
}

void
ui_load_view(Ui *ui, const char *target)
{
  if (target[0]=='/')
  {
    struct stat st;
    if (stat(target, &st) == 0) {
      if (S_ISDIR(st.st_mode))
        ui_push_fun (ui, ui_view_dir, target, NULL, NULL);
      else
        ui_push_fun (ui, ui_view_file, target, NULL, NULL);
    }
    else
    {
      ui_push_fun (ui, ui_view_404, target, NULL, NULL);
    }
  }
  else
  {
#if 0
    int n_views = sizeof(views)/sizeof(views[0]);

    for (int i = 0; i < n_views; i++)
    if (!strcmp (views[i].name, target))
    {
      ui_push_fun (ui, views[i].fun, target, NULL, NULL);
      return;
    }
#endif

    for (int i = 0; i < ui->n_views; i++)
    if (!strcmp (ui->views[i].name, target))
    {
      ui_push_fun (ui, ui->views[i].fun, target, NULL, NULL);
      return;
    }
    ui_push_fun (ui, ui_unhandled, target, NULL, NULL);
  }
}

static UiWidget *ui_widget_by_id(Ui *ui, void *id);


void
ui_do(Ui *ui, const char *action)
{
  printf ("ui_do: %s\n", action);
  if (!strcmp (action, "backspace") ||
      !strcmp (action, "return") ||
      !strcmp (action, "left") ||
      !strcmp (action, "escape") ||
      !strcmp (action, "right") ||
      !strcmp (action, "space"))
  {
    ctx_key_press (ui->ctx, 0, action, 0);
  }
  else if (!strcmp (action, "back"))
  {
    ui_pop_fun (ui);
    ui->overlay_fade = 0.7;
  }
  else if (!strcmp (action, "activate"))
  {
    UiWidget *widget = ui_widget_by_id (ui, ui->focused_id);
    if (ui->focused_id)
      ui->activate = 1;
    if (widget && (widget->type == ui_type_entry))
       ui_do(ui, "kb-show");
  }
  else if (!strcmp (action, "focus-next"))
  {
    ui_focus_next(ui);
  }
  else if (!strcmp (action, "focus-prev"))
  {
    ui_focus_prev(ui);
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
  else ui_load_view(ui, action);
}

void ui_cb_do (CtxEvent *event, void *data1, void *data2)
{
  Ui *ui = data1;
  const char *target = data2;
  if (data2) {}
  event->stop_propagate=1;
  //printf ("%s %s %s\n", data1, data2, event->string);
  if (!strcmp (target, "quit"))
    ctx_quit (event->ctx);
  else
    ui_do (ui, target);
}

void overlay_button (Ui *ui, float x, float y, float w, float h, const char *label, char *action)
{
  Ctx *ctx = ui->ctx;
  float m = w;
  if (m > h) m = h;
      ctx_save(ctx);
       ctx_rectangle (ctx, x,y,w,h);
       ctx_listen (ctx, CTX_PRESS, ui_cb_do, ui, action);
      if (ui->overlay_fade <= 0.0f)
      {
        ctx_begin_path(ctx);
      }
      else
      {
        ctx_rgba(ctx,0,0,0,ui->overlay_fade);
        ctx_fill(ctx);
        if (ui->overlay_fade > 0.2)
        {
          ctx_rgba(ctx,1,1,1,ui->overlay_fade);
          ctx_move_to (ctx, x+0.5 * w, y + 0.8 * h);
          ctx_font_size (ctx, 0.8 * m);
          ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
          ctx_text (ctx, label);
        }
          
      }
      ctx_restore (ctx);
}

static Ui *def_ui = NULL;

Ui *_default_ui(void)
{
  return def_ui;
}

Ui *ui_new(Ctx *ctx)
{
  Ui *ui = calloc (1, sizeof (Ui));
  if (!def_ui)def_ui = ui;
  ui->ctx = ctx;
  ui->style.bg[0]=0.1;
  ui->style.bg[1]=0.2;
  ui->style.bg[2]=0.3;
  ui->style.bg[3]=1.0;
  ui->style.fg[0]=1.0;
  ui->style.fg[1]=1.0;
  ui->style.fg[2]=0.8;
  ui->style.fg[3]=1.0;

  ui->style.focused_fg[0]=1.0;
  ui->style.focused_fg[1]=1.0;
  ui->style.focused_fg[2]=0.0;
  ui->style.focused_fg[3]=1.0;

  ui->style.focused_bg[0]=1.0;
  ui->style.focused_bg[1]=1.0;
  ui->style.focused_bg[2]=1.0;
  ui->style.focused_bg[3]=0.1;


  ui->style.interactive_fg[0]=1.0;
  ui->style.interactive_fg[1]=0.0;
  ui->style.interactive_fg[2]=0.0;
  ui->style.interactive_fg[3]=0.8;
#if 0
static float color_focused_fg[4]  = {1,0,0,0.8};
//static float color_focused_bg[4]  = {1,1,0,0.8};
static float color_interactive[4] = {1,0,0,0.0}; 
static float color_bg[4]          = {0.1, 0.2, 0.3, 1.0};
static float color_bg2[4]         = {0.8, 0.9, 1.0, 1.0};
static float color_fg[4]; // black or white automatically based on bg
#endif
#if CTX_FLOW3R
  ui->focus_first = true;
#endif
  ui->focus_first = true;

#if CTX_FLOW3R
    ui->osk_focus_target = 0.33f;
    osk_captouch = true;
#else
    ui->osk_focus_target = 0.18f;
#endif
    ui->scroll_speed = 1.5f;

  ui_register_view (ui, "settings", "os", view_settings);
  ui_register_view (ui, "settings-ui", "os", view_settings_ui);

  return ui;
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

static void ctx_on_view_key_event (CtxEvent *event, void *data1, void *data2)
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


void captouch_keyboard (Ctx *ctx)
{
   float width = ctx_width (ctx);
   float height = ctx_height (ctx);
   ctx_save (ctx);
   ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
   float _em = height / 10;
   ctx_font_size (ctx, _em);
   
   float rad_pos;
#if CTX_FLOW3R
   float raw_angle = bsp_captouch_angle (&rad_pos, 20, 0);
#else
   float raw_angle = -1.0f;
   rad_pos = 0.5f;
#endif
   static float angle = -1.0f;


   if (raw_angle >= 0)
   {
     if (angle<0) angle = raw_angle;
     else
      // the values are already snapped, thus this acts as a debouncing
      // for position making the hovered position be the registered as
      // release position.
      angle = angle * 0.8f + raw_angle * 0.2f;
   }
   else
     angle = -1.0;


   static bool shift_down = false;
   static bool fn_down = false;
   static int last_col = 0;
   static int last_row = 0;
   static bool last_active = false;
   static bool last_down = false;
   bool down = false;
   int cursor_col = -1;

   int cursor_row = 0;
   static float smoothed_row = 0.0f;

   if (angle >=0)
   {
     cursor_col = (int)(angle * 20+ 0.5f);
     if (cursor_col >= 20) cursor_col = 0;

     if (rad_pos < 0.15)
       cursor_row = 0;
     else if (rad_pos < 0.25)
       cursor_row = 1;
     else
       cursor_row = 2;

     down = true;
   }

   smoothed_row = smoothed_row * 0.8f + (cursor_row+0.5f) * 0.2f;
   cursor_row = smoothed_row;

   const char *neutral_rows[][12]={{"q","w","e","r","t","y","u","i","o","p","0",0},
                     {"a","s","d","f","g","h","j","k","l",";",0},
                     {"z","x","c","v","b","n","m",",",".","/","A",0}};
   
   const char *shift_rows[][12]={{"Q","W","E","R","T","Y","U","I","O","P","~"},
                           {"A","S","D","F","G","H","J","K","L",":",0},
                           {"Z","X","C","V","B","N","M","<",">","?","a",0}};


   const char *fn_rows[][12]={{"1","2","3","4","5","6","7","8","9","0","a",0},
                        {"!","@","#","$","%","^","&","*","(",")",0},
                        {"-","_","+","=","[","]","|","'"," ","\"","~",0}};

   const char *fn_shift_rows[][12]={{"{","}","`","~","\\","€","≈","¡","¿","π","0",0},
                                    {"æ","Æ","Ø","ø","å","Å","è","é","É","•",0},
                                    {"ß","…","ü","Ü","ö","Ö","♥","ð","ñ","Ñ","0",0}};

   ctx_save(ctx);
   ctx_rgba(ctx, 0,0,0,0.6);
   ctx_arc(ctx, width/2, height/2, height*0.6, M_PI*1.05f, -M_PI*0.05f, 1);
   ctx_arc(ctx, width/2, height/2, height*0.20, -M_PI*0.05f, M_PI*1.05f, 0);
   ctx_fill (ctx);
   ctx_restore(ctx);

   if ((down != last_down) && (down==false))
   {
      if (last_active)
      {
         if (last_row == 0 && last_col == 10)
         {
            fn_down = !fn_down;
         }
         else if (last_row == 2 && last_col == 10)
         {
            shift_down = !shift_down;
         }
         else
         {
           if (fn_down)
           {
             if (shift_down)
               ctx_key_press (ctx, 0, fn_shift_rows[last_row][last_col], 0);
             else
               ctx_key_press (ctx, 0, fn_rows[last_row][last_col], 0);
           }
           else
           {
             if (shift_down)
               ctx_key_press (ctx, 0, shift_rows[last_row][last_col], 0);
             else
               ctx_key_press (ctx, 0, neutral_rows[last_row][last_col], 0);
           }
         }
      }
   }

   last_down = false;
   last_active = false;
   for (int row = 0; row < 3; row++)
   {

     for (int col = 0; neutral_rows[row][col]; col++)
     {
       int i = 15 - col;

       ctx_save (ctx);
       ctx_translate (ctx, width/2, height/2);
       ctx_rotate (ctx, (i) / (1.0f * 20) * M_PI * 2 + M_PI);
       if (i==cursor_col && row == cursor_row)
       {
         ctx_rgba (ctx, 1.0f, 1.0f, 1.0f, 1.0f);
         ctx_rectangle (ctx, -_em/2, height * 0.5f - (3-row)* _em, _em, _em * 1.2);
         ctx_fill (ctx);
         last_col = col;
         last_row = row;
         last_active = true;
         last_down = true;
         ctx_rgba (ctx, 0.0f, 0.0f, 0.0f, 1.0f);
       }
       ctx_move_to (ctx, 0, (height * 0.5 - (2.2-row)*_em));
       if (fn_down)
       {
         if (shift_down)
           ctx_text (ctx, fn_shift_rows[row][col]);
         else
           ctx_text (ctx, fn_rows[row][col]);
       }
       else
       {
         if (shift_down)
           ctx_text (ctx, shift_rows[row][col]);
         else
           ctx_text (ctx, neutral_rows[row][col]);
       }
       ctx_restore (ctx);
     }
   }

#if 0
   if (angle >= 0)
   {
     ctx_save (ctx);
     ctx_translate (ctx, width/2, height/2);
     ctx_rotate (ctx, angle * M_PI * 2 + M_PI);
     ctx_move_to (ctx, 0, 0.5 * height/2);
     ctx_line_to (ctx, 0, height/2);
     ctx_line_width (ctx, 2.0);
     ctx_gray (ctx, 1.0f);
     ctx_stroke (ctx);
     ctx_restore (ctx);
   }
#endif
}

static void ui_osk_draw (Ui *ui)
{
  Ctx *ctx = ui->ctx;
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
  ui->overlay_fade -= 0.3 * elapsed_ms/1000.0f;
  

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
      if (osk_captouch) {
        captouch_keyboard (ctx);      
        return;
      }

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
  ctx_listen (ctx, CTX_DRAG, ctx_on_view_key_event, ui, (void*)kb);
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
       overlay_button (ui, 0, h - h * 0.14, w, h * 0.14, "kb", "kb-show");
       break;
  }
}

int elf_output_state(void);

#if CTX_ESP
#include "esp_task.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

void ui_main(Ui *ui, const char *start_location)
{
    Ctx *ctx = ui->ctx;
    long int prev_ticks = ctx_ticks();
    float width  = ctx_width (ctx);
    float height = ctx_height (ctx);

    if (start_location)
    {
       ui_load_view (ui, start_location);
    }
    else
    {
      if (!ui->fun)
        ui_load_view (ui, "menu");
    }
    //ui_do(ui, "splash");
    if (height <= width)
      ui->font_size = height * 0.09f;
    else
      ui->font_size = width * 0.09f;
    while (!ctx_has_quit (ctx))
    {
#if CTX_ESP
      int os = elf_output_state (); // 1 text 2 graphics 3 both
      if (os > 1) {
         vTaskDelay(1);
         continue;
      }
#endif
 
      width = ctx_width (ctx);
      height = ctx_height (ctx);
      ctx_start_frame (ctx);
      ctx_add_key_binding (ctx, "escape",    "back", "foo", ui_cb_do, ui);
      ctx_add_key_binding (ctx, "backspace", "back", "foo", ui_cb_do, ui);
      long int ticks = ctx_ticks ();
      long int ticks_delta = ticks - prev_ticks;
      if (ticks_delta > 1000000) ticks_delta = 10; 
      prev_ticks = ticks;

      ctx_save (ctx);

      ui->delta_ms = ticks_delta/1000;

      if (ui->fun)
        ui->fun(ui);
      if (ui->queued_next)
      {
        ui->queued_next--;
        if (ui->queued_next <= 0 && ui->widget_count)
           ui_do(ui,"focus-next");
      }
      ctx_restore (ctx);


      if (is_touch){
        overlay_button (ui, 0,0,width,height*0.12, "back", "back");
      }

      if (ui->draw_tips)
      {
      if (!is_touch)
      {
        ctx_save (ctx);

        ctx_translate (ctx, width/2,height/2);
        ctx_rotate (ctx, -M_PI*2/5/2);
        ctx_translate (ctx, -width/2,-height/2);

        const char *labels[]  = {"ok", "next","space","prev","back"};
        char *actions[] = {"return", "right","space","left","backspace"};

        for (int i = 0; i < 5; i++)
        {
          ctx_translate (ctx, width/2,height/2);
          ctx_rotate (ctx, M_PI*2/5);
          ctx_translate (ctx, -width/2,-height/2);
          overlay_button (ui, 0,0,width,height*0.12, labels[i], actions[i]);
        }


        ctx_restore (ctx);
      }    
      }



      ui_osk_draw (ui);

      if (ui->show_fps)
      {
         char buf[32];
         ctx_save (ctx);
         ctx_font_size (ctx, ui->height / 20);
         ctx_rgba(ctx,ui->style.bg[0], ui->style.bg[1], ui->style.bg[2], 0.8f);
         ctx_rectangle(ctx,0,0,width, ui->height/20);
         ctx_fill(ctx);
         ui_set_color(ctx,ui->style.fg);
         ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
         ctx_move_to (ctx, ctx_width(ctx)/2, ui->height/20);
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
      if (backlight <= 99.0f)
      {
      ctx_rectangle (ctx, 0,0,ctx_width(ctx),ctx_height(ctx));
      float alpha = 1.0f-(backlight/100.0f * 0.8 + 0.2);
      ctx_rgba (ctx, 0.0f,0.0f,0.0f, alpha);
      ctx_fill (ctx);
      }
      ctx_restore (ctx);
#endif
      ctx_end_frame (ctx);

      ui->frame_no++;
      ui->view_elapsed += ticks_delta* 1/(1000*1000.0f);
    }
}
static void
ui_set_color (Ctx *ctx, float *rgba)
{
  ctx_rgba(ctx, rgba[0], rgba[1], rgba[2], rgba[3]);
}

static void
ui_set_color_a (Ctx *ctx, float *rgba, float alpha)
{
  ctx_rgba(ctx, rgba[0], rgba[1], rgba[2], rgba[3] * alpha);
}


static UiWidget *ui_widget_by_id(Ui *ui, void *id)
{
  for (int i = 0; i < ui->widget_count; i++)
  {
    if (ui->widgets[i].id == id)
      return &ui->widgets[i];
  }
  return NULL;
}
static void ui_set_focus (Ui *ui, UiWidget *widget)
{
  for (int i = 0; i < ui->widget_count; i++)
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

void ui_focus_next(Ui *ui)
{
  bool found = false;
  if (ui->widget_count == 0)
  {
    ui_scroll_to (ui, ui->scroll_offset - ui->height * 0.2f);
    return;
  }

  for (int i = 0; i < ui->widget_count; i++)
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

void ui_focus_prev(Ui *ui)
{
  bool found = false;
  if (ui->widget_count == 0)
  {
    ui_scroll_to (ui, ui->scroll_offset + ui->height * 0.2f);
    return;
  }
  for (int i=ui->widget_count-1;i>=0; i--)
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
#if 0
    widget->float_data += widget->step;
    if (widget->float_data >= widget->max_val)
      widget->float_data = widget->max_val;
#else
    widget->float_data_target = widget->float_data + widget->step;
    if (widget->float_data_target >= widget->max_val)
      widget->float_data_target = widget->max_val;
#endif
  }
  else if (!strcmp (string, "down")
       ||  !strcmp (string, "left"))
  {
#if 0
    widget->float_data -= widget->step;
    if (widget->float_data <= widget->min_val)
      widget->float_data = widget->min_val;
#else
    widget->float_data_target = widget->float_data - widget->step;
    if (widget->float_data_target <= widget->min_val)
      widget->float_data_target = widget->min_val;

#endif
  }
  else if (!strcmp (string, "escape")
        || !strcmp (string, "space")
        || !strcmp (string, "backspace")
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
    if (ui->cursor_pos < (int)strlen (ui->temp_text))
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
     ui_do(ui, "kb-hide");
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
    if ((int)strlen (ui->temp_text) < ui->cursor_pos)
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
  float *fptr = data1;
  demo_mode = 0;
  if (fabs (event->start_y - event->y) > 8.0f)
    ui_set_focus (data2, NULL);
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
   widget->float_data_target = new_val;
   event->stop_propagate = 1;
}

static UiWidget *
ui_widget_register (Ui *ui, ui_type type, float x, float y, float width, float height, void *id)
{
   Ctx *ctx = ui->ctx;
   if (ui->widget_count+1 >= UI_MAX_WIDGETS) { printf("too many widgets\n");return &ui->widgets[ui->widget_count]; }

   UiWidget *widget  = &ui->widgets[ui->widget_count++];
  
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

          if (ui->style.focused_bg[3]>0.0f)
          {
            ctx_save(ctx);
            ui_set_color(ctx, ui->style.focused_bg);
            ctx_rectangle (ctx, x - em/2, y, width + em, height);
            ctx_fill (ctx);
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

float
ui_slider_coords (Ui *ui, void *id, float x, float y, float width, float height, float min_val, float max_val, float step, float value)
{
   Ctx *ctx = ui->ctx;
   UiWidget *widget = ui_widget_register(ui,ui_type_slider,x,y,width,height,id);
   if (widget->fresh)
   {
     widget->float_data = value;
     widget->float_data_target = value;
   }
   widget->min_val = min_val;
   widget->max_val = max_val;
   widget->step = step;

   bool focused = (widget->id == ui->focused_id);
   if (focused && ui->activate)
   {
     widget->float_data = value;
     widget->float_data_target = value;
     printf ("!activating slider\n");
     ui->active_id = widget->id;
     ui->activate = 0;
   }

   float rel_value = ((value) - widget->min_val) / (widget->max_val-widget->min_val);
   if (ui->active_id == widget->id)
   {
     value = widget->float_data = (value * 0.8 + 0.2 * widget->float_data_target);
   }

   if (widget->visible)
   {

   ctx_save(ctx); 

   ctx_line_width(ctx,2.0);
   ui_set_color(ctx, ui->style.fg);
   ctx_move_to (ctx, x, y + height/2);
   ctx_line_to (ctx, x + width, y + height/2);
   ctx_stroke (ctx);

   if (ui->active_id == widget->id)
   ui_set_color(ctx, ui->style.focused_fg);
   else
   ui_set_color(ctx, ui->style.fg);
   ctx_arc (ctx, x + rel_value * width, y + height/2, height*0.34, 0, 2*3.1415, 0);
   ctx_fill (ctx);
   ui_set_color(ctx, ui->style.bg);
   ctx_arc (ctx, x + rel_value * width, y + height/2, height*0.3, 0.0, 3.1415*2, 0);
   ctx_fill (ctx);

   ctx_rectangle (ctx, x + rel_value * width - height * 0.75, y, height * 1.5, height);
   ctx_listen (ctx, CTX_DRAG, ui_slider_drag_float, ui, widget->id);
   if (ui->interactive_debug)
   {
     ui_set_color(ctx, ui->style.interactive_bg);
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

  if (event->type == CTX_DRAG_PRESS)
  {
    ui_set_focus (ui, widget);
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

   UiWidget *widget  = ui_widget_register(ui,ui_type_button,x,y,width,height,id);
   if (widget->visible)
   {
   ctx_save(ctx); 

   ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);

   ui_set_color(ctx, ui->style.fg);

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
   if (ui->interactive_debug)
   {
     ui_set_color(ctx, ui->style.interactive_bg);
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
          if (fabs(ui->height * ui->osk_focus_target - focused->y) > 2)
            ui->scroll_offset += ui->height * ui->osk_focus_target - focused->y;
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
        ctx_add_key_binding (ctx, "up",     "focus-prev", "foo",    ui_cb_do, ui);
        ctx_add_key_binding (ctx, "left",   "focus-prev", "foo",    ui_cb_do, ui);
        ctx_add_key_binding (ctx, "shift-tab", "focus-prev", "foo", ui_cb_do, ui);

        ctx_add_key_binding (ctx, "tab",    "focus-next", "foo", ui_cb_do, ui);
        ctx_add_key_binding (ctx, "right",  "focus-next", "foo", ui_cb_do, ui);
        ctx_add_key_binding (ctx, "right",  "focus-next", "foo", ui_cb_do, ui);
        ctx_add_key_binding (ctx, "down",   "focus-next", "foo", ui_cb_do, ui);
        ctx_add_key_binding (ctx, "return", "activate", "foo",  ui_cb_do, ui);
        ctx_add_key_binding (ctx, "space",  "activate", "foo",  ui_cb_do, ui);
      }
      ctx_add_key_binding (ctx, "control-q", "quit", "foo", ui_cb_do, ui);
}

void ui_draw_bg (Ui *ui)
{
  Ctx *ctx = ui->ctx;
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);

  static float prev_red = 0;
  static float prev_green = 0;
  static float prev_blue = 0;
  if (ui->style.bg[0] != prev_red ||
      ui->style.bg[1] != prev_green ||
      ui->style.bg[2] != prev_blue)
  {
    if (ui->style.bg[0] +
        ui->style.bg[1] +
        ui->style.bg[2] > 1.8)
    {
      ui->style.fg[0] = 
      ui->style.fg[1] = 
      ui->style.fg[2] = 0.0f;
      ui->style.fg[3] = 1.0f;

      ui->style.focused_bg[0]=0.0;
      ui->style.focused_bg[1]=0.0;
      ui->style.focused_bg[2]=0.0;
      ui->style.focused_bg[3]=0.1;

    }
    else
    {
      ui->style.fg[0] = 
      ui->style.fg[1] = 
      ui->style.fg[2] = 1.0f;
      ui->style.fg[3] = 1.0f;

      ui->style.focused_bg[0]=1.0;
      ui->style.focused_bg[1]=1.0;
      ui->style.focused_bg[2]=1.0;
      ui->style.focused_bg[3]=0.1;
    }

    prev_red = ui->style.bg[0];
    prev_green = ui->style.bg[1];
    prev_blue = ui->style.bg[2];
  }

  ctx_rectangle(ctx,0,0,width,height);

  ui_set_color (ctx, ui->style.bg);

  if (ui->gradient_bg)
  {
    ctx_linear_gradient (ctx,0,0,0,height);
    ctx_gradient_add_stop (ctx, 0.0, ui->style.bg[0], ui->style.bg[1], ui->style.bg[2], 1.0f);
    ctx_gradient_add_stop (ctx, 1.0, ui->style.bg2[0], ui->style.bg2[1], ui->style.bg2[2], 1.0f);
  }
  ctx_fill(ctx);
  ui_set_color(ctx,ui->style.fg);
}

void ui_start (Ui *ui)
{
   Ctx *ctx = ui->ctx;
   ui_draw_bg(ui);
   ui->width = ctx_width (ctx);
   ui->height = ctx_height (ctx);
   ui->line_height = ui->font_size * 1.7;
   ctx_rectangle(ctx,0,0,ui->width, ui->height);
   ctx_listen (ctx, CTX_DRAG, ui_pan, &ui->scroll_offset, ui);
   ctx_listen (ctx, CTX_DRAG, ui_pan, &ui->scroll_offset_target, ui);
   ctx_begin_path (ctx);
   ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
   ctx_font_size(ctx,ui_get_font_size (ui));

   ui->y = (int)(ui->scroll_offset + ui->height * 0.15);
   ui->widget_count = 0;
}

float
ui_slider (Ui *ui,
           const char *label, float min, float max, float step, float value)
{  Ctx *ctx = ui->ctx;
   ctx_save(ctx);ctx_text_align(ctx,CTX_TEXT_ALIGN_CENTER);
   ctx_move_to (ctx, ui->width * 0.5, ui->y+em);
   ctx_text (ctx, label);

   ui->y += em * 0.8f;
   float ret = ui_slider_coords (ui, UI_ID_STR(label), ui->width * 0.1, ui->y, ui->width * 0.8, ui->line_height, min, max, step, value);
   ui->y += ui->line_height;
   ctx_restore(ctx);
   return ret;
}

bool ui_toggle(Ui *ui, const char *label, bool value)
{ 
   Ctx *ctx = ui->ctx;
   if (ui->y>-2 * em && ui->y < ui->height - em)
   {
     ctx_move_to (ctx, ui->width * 0.5, ui->y+em);
     ctx_text (ctx, label);
   }
   ui->y += ui->line_height;
   if (ui_button_coords(ui, ui->width * 0.15, ui->y, em * 2, 0, "off", value==0, UI_ID_STR(label)))
      value=false;
   if (ui_button_coords(ui, ui->width * 0.50, ui->y, em * 2, 0, "on", value==1, UI_ID_STR(label+1))) // XXX hack - skipping first label char
      value=true;
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

int ui_button(Ui *ui, const char *label)
{
   return ui->y += ui->line_height, ui_button_coords(ui, ui->width * 0.0, ui->y-ui->line_height, ui->width, ui->line_height, label, 0, UI_ID_STR(label));
}

static void ui_entry_drag (CtxEvent *event, void *data1, void *data2)
{
  Ui *ui = data1;
  UiWidget *widget = ui_widget_by_id (ui, data2);
  if (!widget) return;

  if (event->type == CTX_DRAG_PRESS)
  {
    event->stop_propagate = 0;
    ui_set_focus (ui, widget);
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
  UiWidget *widget  = ui_widget_register(ui,ui_type_entry,x,y,w,h, id);

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
          ui_set_color_a (ctx, ui->style.fg, 0.5);
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
           ui_set_color_a (ctx, ui->style.fg, 0.5);
         ctx_text (ctx, to_show);
       }
  
       ctx_rectangle (ctx, x + w/5 + tw_pre -1, y + ui->font_size * 0.1,
                      2 + tw_selection, ui->font_size);
       ctx_save (ctx);
       ui_set_color(ctx, ui->style.focused_fg);
       ctx_fill (ctx);
       ctx_restore (ctx);
    }
  
    ctx_begin_path (ctx);
    ctx_rectangle (ctx, x, y, w, h);
    ctx_listen (ctx, CTX_DRAG, ui_entry_drag, ui, widget->id);
    if (ui->interactive_debug)
    {
      ui_set_color(ctx, ui->style.interactive_bg);
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
    ui_do (ui, "kb-hide");
    return strdup (ui->temp_text);
  }
  return NULL;
}

int 
ui_entry (Ui *ui,
          const char *label, const char *fallback, char **strptr)
{
   char *ret = NULL;
   ctx_save(ui->ctx);ctx_font_size (ui->ctx, ui->font_size * 0.75);
   ctx_move_to(ui->ctx, ui->width * 0.5, ui->y + ui->font_size * 0.1);ctx_text(ui->ctx, label);
   if ((ret = ui_entry_coords(ui, UI_ID_STR(label), ui->width * 0.15, ui->y, ui->width * 0.7, ui->line_height, fallback, *strptr)))
   {
     if (*strptr) free (*strptr);
     *strptr = ret;
   }
   ui->y += ui->line_height;
   ctx_restore(ui->ctx);

   return ret != NULL;
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

void ui_destroy (Ui *ui)
{
   if (ui->location)
     free (ui->location);
   ui->location = NULL;
  // XXX : destroy more
  free (ui);
}
