
#include "local.conf"
#if CTX_VT

#if !__COSMOPOLITAN__
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
//#include <pty.h>
#include <math.h>
//#include <malloc.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#endif

#include "ctx.h"
#include "terminal-keyboard.h"

#define OVERVIEW_TRANSITION_LENGTH  0.25f
static Ctx *ctx = NULL; // initialized in main

typedef struct _CtxClient CtxClient;
CtxList *ctx_clients(Ctx *ctx);

int ctx_client_get_width (Ctx *ctx, int id);
int ctx_client_get_height (Ctx *ctx, int id);
int ctx_client_get_x (Ctx *ctx, int id);
int ctx_client_get_y (Ctx *ctx, int id);

void ctx_screenshot (Ctx *ctx, const char *path);
void
vt_screenshot (const char *output_path)
{
  ctx_screenshot (ctx, output_path);
}

void ctx_client_lock (CtxClient *client);
void ctx_client_unlock (CtxClient *client);

#define VT_RECORD 0
static int terminal_no_new_tab = 0;

static char *execute_self = NULL;

float font_size    = -1;
float line_spacing = 2.0;

float leave_overview = 0.0f;
float overview_t = 0.0f;
/********************/
static float start_font_size = 22.0;

float add_x = 10;
float add_y = 100;

static const char *ctx_find_shell_command (void)
{
  if (access ("/.flatpak-info", F_OK) != -1)
  {
    static char ret[512];
    char buf[256];
    FILE *fp = popen("flatpak-spawn --host getent passwd $USER|cut -f 7 -d :", "r");
    if (fp)
    {
      while (fgets (buf, sizeof(buf), fp) != NULL)
      {
        if (buf[strlen(buf)-1]=='\n')
          buf[strlen(buf)-1]=0;
        sprintf (ret, "flatpak-spawn --env=TERM=xterm --host %s", buf);
      }
      pclose (fp);
      return ret;
    }
  }

  if (getenv ("SHELL"))
  {
    return getenv ("SHELL");
  }
  int i;
  const char *command = NULL;
  struct stat stat_buf;
  static const char *alts[][2] =
  {
    {"/bin/bash",     "/bin/bash"},
    {"/usr/bin/bash", "/usr/bin/bash"},
    {"/bin/sh",       "/bin/sh"},
    {"/usr/bin/sh",   "/usr/bin/sh"},
    {NULL, NULL}
  };
  for (i = 0; alts[i][0] && !command; i++)
    {
      lstat (alts[i][0], &stat_buf);
      if (S_ISREG (stat_buf.st_mode) || S_ISLNK (stat_buf.st_mode) )
        { command = alts[i][1]; }
    }
  return command;
}
static int in_overview = 0;

/*****************/

#define flag_is_set(a, f) (((a) & (f))!=0)
#define flag_set(a, f)    ((a) |= (f));
#define flag_unset(a, f)  ((a) &= ~(f));

int add_tab (Ctx  *ctx, const char *commandline, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = ctx_client_is_maximized (ctx, ctx_clients_active (ctx));
  int flags = CSS_CLIENT_UI_RESIZABLE |  CSS_CLIENT_TITLEBAR;
  if (can_launch) flags |= CSS_CLIENT_CAN_LAUNCH;

  //ctx_font_size (ctx, start_font_size); // we pass it as arg instead
  CtxClient *active = ctx_client_new (ctx, commandline, add_x, add_y,
    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2,
    start_font_size,
    flags, NULL, NULL);
  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    ctx_client_maximize (ctx, ctx_client_id (active));
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
//  ensure_layout (ctx);
  ctx_queue_draw (ctx);
  return ctx_client_id (active);
}

int add_tab_argv (Ctx  *ctx, char **argv, int can_launch)
{
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = ctx_client_is_maximized (ctx, ctx_clients_active (ctx));
  int flags = CSS_CLIENT_UI_RESIZABLE |  CSS_CLIENT_TITLEBAR;
  if (can_launch) flags |= CSS_CLIENT_CAN_LAUNCH;

  CtxClient *active = ctx_client_new_argv (ctx, argv, add_x, add_y,
                    ctx_width(ctx)/2, (ctx_height (ctx) - titlebar_h)/2,
                    start_font_size,
                    flags, NULL, NULL);
  add_y += ctx_height (ctx) / 20;
  add_x += ctx_height (ctx) / 20;

  if (was_maximized)
  {
    ctx_client_maximize (ctx, ctx_client_id (active));
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
  ctx_queue_draw (ctx);
  return ctx_client_id (active);
}


static void settings_thread (Ctx *ctx, void *user_data)
{
   int frame = 0;
   while (!ctx_has_exited (ctx))
   {
     fprintf (stderr, "[%i]", frame++);
     if (ctx_need_redraw (ctx))
     {
        char buf[1024];
        ctx_start_frame (ctx);
        ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
        ctx_rgb (ctx, 0.1, 0.2, 0.3);
        ctx_fill (ctx);
        ctx_rgba (ctx, 1, 1, 1, 0.5);
        ctx_line_width (ctx, 5.0);
        ctx_move_to (ctx, 0,0);
        ctx_line_to (ctx, ctx_width (ctx), ctx_height (ctx));
        ctx_move_to (ctx, ctx_width (ctx), 0);
        ctx_line_to (ctx, 0, ctx_height (ctx));
        ctx_stroke (ctx);

        sprintf (buf, "frame: %i", frame);
        ctx_rgba (ctx, 1, 1, 0, 1);
        ctx_move_to (ctx, ctx_width (ctx) * 0.1, ctx_height (ctx) * 0.5);
        ctx_font_size (ctx, ctx_height (ctx) * 0.1);
        ctx_text (ctx, buf);

        ctx_end_frame (ctx);
     }
     if (frame > 100) ctx_exit (ctx);
     
     ctx_handle_events (ctx);
   }
}


int add_settings_tab (const char *commandline, int can_launch)
{
#if 0
  float titlebar_h = ctx_height (ctx)/40;
  int was_maximized = ctx_client_is_maximized (ctx, ctx_clients_active (ctx));
  int flags = CSS_CLIENT_UI_RESIZABLE |  CSS_CLIENT_TITLEBAR;
  {
    ctx_client_maximize (ctx, ctx_client_id (active));
  }

  if (add_y + ctx_height(ctx)/2 > ctx_client_max_y_pos (ctx))
  {
    add_y = ctx_client_min_y_pos (ctx);
    add_x -= ctx_height (ctx) / 40 * 4;
  }
  ctx_queue_draw (ctx);
  return ctx_client_id (active);
#endif
  return 0;
}

static void add_tab_cb (CtxEvent *event, void *data, void *data2)
{
  event->stop_propagate = 1;
  if (!terminal_no_new_tab)
    add_tab (event->ctx, ctx_find_shell_command(), 1);
}

static void add_settings_tab_cb (CtxEvent *event, void *data, void *data2)
{
  event->stop_propagate = 1;
  if (!terminal_no_new_tab)
    add_settings_tab (ctx_find_shell_command(), 1);
}

int ctx_clients_tab_to_id (Ctx *ctx, int tab_no);

void switch_to_tab (Ctx *ctx, int tab_no)
{
  int id = ctx_clients_tab_to_id (ctx, tab_no);
  if (id >= 0)
  {
    ctx_client_maximize (ctx, id);
  }
}

CtxClient *ctx_client_by_id (Ctx *ctx, int id);

CtxEvent *ctx_event_copy (CtxEvent *event);


static void overview_init (Ctx *ctx);

static void overview_event (CtxEvent *event, void *a, void *b)
{
     in_overview = !in_overview;
     overview_init (event->ctx);
     if (in_overview)
     {
       overview_t = 0.0f;
     }
     else
     {
       leave_overview = OVERVIEW_TRANSITION_LENGTH;
     }
     ctx_queue_draw (event->ctx);
     ctx_event_stop_propagate (event);
}


int         vt_get_scroll           (VT *vt);
void        vt_set_scroll           (VT *vt, int scroll);

static void handle_event (Ctx        *ctx,
                          CtxEvent   *ctx_event,
                          const char *event)
{
  CtxBackendType backend_type = ctx_backend_type (ctx);
  int active_id = ctx_clients_active (ctx);
  CtxClient *active = active_id>=0?ctx_client_by_id (ctx, active_id):NULL;
  if (!active)
  {
     printf("no active client\n");
    return;
  }
  //if (active->internal)
  //  return;
  VT *vt = ctx_client_vt (active);

  CtxClient *client = active; //vt_get_client (vt);

  if (!vt)
  {
     ctx_client_add_event (active, ctx_event);
     return;
  }

  ctx_client_lock (client);

  if (!strcmp (event, "F11"))
  {
    ctx_set_fullscreen (ctx, !ctx_get_fullscreen (ctx));
  }
  else if (!strcmp (event, "shift-return"))
  {
    ctx_client_feed_keystring (active, ctx_event, "return");
  }
  else if (!strcmp (event, "shift-control-v") )
    {
      char *text = ctx_get_clipboard (ctx);
      if (text)
        {
          ctx_client_paste (ctx, active_id, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-c") && vt)
    {
      char *text = ctx_client_get_selection (ctx, active_id);
      if (text)
        {
          ctx_set_clipboard (ctx, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-t") ||
           ((backend_type == CTX_BACKEND_FB ||  // workaround for not having
            backend_type == CTX_BACKEND_TERM ||  // raw keyboard acces
            backend_type == CTX_BACKEND_KMS)
           &&   !strcmp (event, "control-t") ))
  {
    if (!terminal_no_new_tab)
      add_tab (ctx, ctx_find_shell_command(), 1);
  }
  else if (!strcmp (event, "shift-control-o") ||
           ((backend_type == CTX_BACKEND_FB  ||  // workaround for not having
            backend_type == CTX_BACKEND_TERM ||  // raw keyboard acces
            backend_type == CTX_BACKEND_KMS)
           &&   !strcmp (event, "control-o") ))
  {
     CtxEvent dummy;dummy.ctx=ctx;
     overview_event (&dummy, NULL, NULL);
  }
  else if (!strcmp (event, "shift-control-n") )
    {
      if (!terminal_no_new_tab)
      {
	       // XXX : we should also avoid this when running with kms/fb !
      pid_t pid;
      if ( (pid=fork() ) ==0)
        { 
          unsetenv ("CTX_VERSION");
          unsetenv ("CTX_BACKEND");
          execlp (execute_self, execute_self, NULL);
          exit (0);
        }
      }
    }


  else if (!strcmp (event, "alt-1"))   switch_to_tab(ctx, 0);
  else if (!strcmp (event, "alt-2"))   switch_to_tab(ctx, 1);
  else if (!strcmp (event, "alt-3"))   switch_to_tab(ctx, 2);
  else if (!strcmp (event, "alt-4"))   switch_to_tab(ctx, 3);
  else if (!strcmp (event, "alt-5"))   switch_to_tab(ctx, 4);
  else if (!strcmp (event, "alt-6"))   switch_to_tab(ctx, 5);
  else if (!strcmp (event, "alt-7"))   switch_to_tab(ctx, 6);
  else if (!strcmp (event, "alt-8"))   switch_to_tab(ctx, 7);
  else if (!strcmp (event, "alt-9"))   switch_to_tab(ctx, 8);
  else if (!strcmp (event, "alt-0"))   switch_to_tab(ctx, 9);
  else if (!strcmp (event, "shift-control-q") )
    {
      ctx_exit (ctx);
    }
  else if (!strcmp (event, "control-tab") )
    {
       CtxList *l = ctx_clients (ctx);
       int found = 0;
       CtxClient *next = NULL;
       for (; l; l = l->next)
       {
	 CtxClient *client = l->data;
	 if (found)
	 {
           if (!next) next = client;
	 }
	 if (client == active) found = 1;
       }
       if (!next)
	 next = ctx_clients (ctx)->data;
       ctx_client_focus (ctx, ctx_client_id (next));
       ctx_queue_draw (ctx);
    }
  else if (!strcmp (event, "shift-control-tab") )
    {
       CtxList *l = ctx_clients (ctx);
       CtxClient *prev = NULL;
       if (l->data == active)
       {
         for (; l; l = l->next)
	   prev = l->data;
       }
       else
       {
         for (; l; l = l->next)
         {
	   CtxClient *client = l->data;
	   if (client == active)
	     break;
	   prev = client;
         }
       }
       ctx_client_focus (ctx, ctx_client_id (prev));
       ctx_queue_draw (ctx);
    }
  else if (!strcmp (event, "shift-control-up") )
  {
    if (vt)
    {
      vt_set_scroll (vt, vt_get_scroll (vt)+1);
      ctx_queue_draw (ctx);
    }
  }
  else if (!strcmp (event, "shift-control-down") )
  {
    if (vt)
    {
      vt_set_scroll (vt, vt_get_scroll (vt)-1);
      ctx_queue_draw (ctx);
    }

  }
  else if (!strcmp (event, "shift-control-w") )
    {
      ctx_client_unlock (client);
      ctx_client_remove (ctx, client);
      ctx_queue_draw (ctx);
      return;
    }
  else if (!strcmp (event, "shift-control-s") )
    {
      if (vt)
      {
        char *sel = ctx_client_get_selection (ctx, ctx_client_id (active));
        if (sel)
        {
          ctx_client_feed_keystring (active, ctx_event, sel);
          free (sel);
        }
      }
    }
  else if (!strncmp (event, "shift-control-", 14))
    {
      ctx_client_feed_keystring (active, ctx_event, event + 6);
    }
  else
    {
      ctx_client_feed_keystring (active, ctx_event, event);
    }
  ctx_client_unlock (client);
}

static void ctx_client_close (CtxEvent *event, void *data, void *data2)
{
  Ctx *ctx = event->ctx;
  CtxList *clients = ctx_clients (ctx);
  CtxClient *client = data;

 // client->do_quit = 1;
  
  ctx_client_remove (ctx, client);
  if (clients == NULL)
    ctx_exit (ctx);//

  ctx_queue_draw (ctx);
  event->stop_propagate = 1;
}

static float consume_float (char **argv, int *i)
{
 float ret = 0.0;
 if (!argv[*i+1])
 {
   fprintf (stderr, "error parsing arguments\n");
   exit (2);
 }
 ret = atof (argv[*i+1]);
 (*i)++;
  int j;
  for (j = *i; argv[j]; j++)
  {
    argv[j-2] = argv[j];
  }
  argv[j-2]=0;
  (*i)-=2;
  return ret;
}
// good font size for 80col:
//
// 4 6 8 10 12 14 16 18 20 2
extern int on_screen_keyboard;
void ctx_osk_draw (Ctx *ctx);

static int malloc_trim_cb (Ctx *ctx, void *data)
{
#if GNU_C
  malloc_trim (64*1024);
#endif
  return 1;
}

static void terminal_key_any (CtxEvent *event, void *userdata, void *userdata2)
{
  if (event->type == CTX_KEY_PRESS &&
      event->string &&
      !strcmp (event->string, "resize-event"))
  {
    ctx_queue_draw (ctx);
  }
  else
  {
    switch (event->type)
    {
      case CTX_KEY_PRESS:
        handle_event (ctx, event, event->string);
        break;
      case CTX_KEY_UP:
        { char buf[1024];
          snprintf (buf, sizeof(buf)-1, "keyup %i %i", event->unicode, event->state);
          handle_event (ctx, event, buf);
        }
        break;
      case CTX_KEY_DOWN:
        { char buf[1024];
          snprintf (buf, sizeof(buf)-1, "keydown %i %i", event->unicode, event->state);
          handle_event (ctx, event, buf);
        }
        break;
      default:
        break;
    }
  }
}

extern int _ctx_enable_hash_cache;

void ctx_client_titlebar_draw (Ctx *ctx, CtxClient *client,
                               float x, float y, float width, float titlebar_height);

void draw_mini_panel (Ctx *ctx)
{
  float titlebar_height = font_size;

  ctx_save (ctx);
  ctx_font_size (ctx, titlebar_height * 0.9);
  float w = ctx_width (ctx);

#if 0
  ctx_rectangle (ctx, w - titlebar_height * 5, 0, titlebar_height * 4, titlebar_height);
  ctx_listen (ctx, CTX_PRESS, add_tab_cb, NULL, NULL);
  ctx_move_to (ctx, w - titlebar_height * 5/2, titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_rgba (ctx, 0.9, 0.9, 0.9, 0.5);
  ctx_text (ctx, "[add tab]");
#else

  ctx_rectangle (ctx, w - titlebar_height * 5, 0, titlebar_height * 5, titlebar_height * 2);
  ctx_listen (ctx, CTX_PRESS, overview_event, NULL, NULL);
  ctx_rgba (ctx, 1,0,0,0.4);
  ctx_fill (ctx);

  ctx_move_to (ctx, w - titlebar_height * 5/2, titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_rgba (ctx, 0.9, 0.9, 0.9, 0.5);
  ctx_text (ctx, "[overview]");
#endif

  ctx_restore (ctx);
}

void draw_panel (Css *itk, Ctx *ctx)
{
  struct tm local_time_res;
  struct timeval tv;
  gettimeofday (&tv, NULL);
  localtime_r (&tv.tv_sec, &local_time_res);

  float titlebar_height = font_size;
  float tab_width = ctx_width (ctx) - titlebar_height * 4 - titlebar_height * 2;

  ctx_save (ctx);
  ctx_rectangle (ctx, 0, 0, ctx_width (ctx), titlebar_height);
  ctx_gray (ctx, 0.0);
  ctx_fill (ctx);
  ctx_font_size (ctx, titlebar_height * 1.0);

#if 0
  ctx_move_to (ctx, ctx_width (ctx), titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_END);
  ctx_gray (ctx, 0.9);
  char buf[128];
  sprintf (buf, "%02i:%02i:%02i", local_time_res.tm_hour, local_time_res.tm_min, local_time_res.tm_sec);
  ctx_text (ctx, buf);
#endif

  ctx_begin_path (ctx);
#if 0
  ctx_rectangle (ctx, ctx_width(ctx)-titlebar_height * 10, 0, titlebar_height * 10, titlebar_height);
  ctx_listen (ctx, CTX_PRESS, add_settings_tab_cb, NULL, NULL);
  ctx_rgba(ctx, 1,0,1,1);
  //ctx_fill(ctx);
#endif

  int tabs = 0;
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (ctx_client_flags (client) & CSS_CLIENT_MAXIMIZED)
    tabs ++;
  }

  if (tabs)
  tab_width /= tabs;

  ctx_begin_path (ctx);
#if 0
  ctx_rectangle (ctx, 0, 0, titlebar_height * 1.5, titlebar_height);
  ctx_listen (ctx, CTX_PRESS, add_tab_cb, NULL, NULL);
  ctx_move_to (ctx, titlebar_height * 1.5/2, titlebar_height * 0.8);
  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_gray (ctx, 0.9);
  ctx_text (ctx, "+");
  float x = titlebar_height * 1.5;
#else
  float x = titlebar_height * 0.0;
#endif
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (ctx_client_flags (client) & CSS_CLIENT_MAXIMIZED)
    {
      ctx_begin_path (ctx);
      ctx_client_titlebar_draw (ctx, client, x, titlebar_height,
                     tab_width, titlebar_height);
    }
    x += tab_width;
  }
  ctx_restore (ctx);
  draw_mini_panel (ctx);
}

static char *set_title = NULL;
void vt_audio_task (VT *vt, int click);

void
terminal_update_title (const char *title)
{
  if (!title)title="NULL";
  if (set_title)
  {
    if (!strcmp (set_title, title))
    {
      return;
    }
    free (set_title);
  }
  set_title = strdup (title);
  ctx_windowtitle (ctx, set_title);
}

int ctx_input_pending (Ctx *ctx, int timeout);

void terminal_long_tap (CtxEvent *event, void *a, void *b)
{
  Ctx *ctx = event->ctx;
  on_screen_keyboard = !on_screen_keyboard;
  ctx_queue_draw (ctx);
}

int commandline_argv_start = 0;



void ctx_client_draw (Ctx *ctx, CtxClient *client, float x, float y);

typedef struct OverviewPos {
  int id;
  CtxClient *client;
  float x0;
  float y0;
  float w0;
  float h0;
  float x1;
  float y1;
  float scale0;  /// is always 1.0f
  float scale1;

} OverviewPos;

static OverviewPos  *opos           = NULL;
static int           opos_count = 0;


static void overview_cleanup (Ctx *ctx)
{
  if (opos)
  {
    ctx_free (opos);
    opos = NULL;
  }
  opos_count = 0;
}

int ctx_client_is_active_tab (Ctx *ctx, CtxClient *client);

static void overview_init (Ctx *ctx)
{
  if (opos)
  {
    //return;
    overview_cleanup (ctx);
  }

  CtxList *clients = ctx_clients (ctx);
  float em = ctx_get_font_size (ctx);
  int n_clients         = ctx_list_length (clients);

  float screen_width = ctx_width (ctx) - 3 * em;
  float screen_height = ctx_height (ctx);

  opos_count = n_clients;
  opos = (OverviewPos*)ctx_calloc(opos_count, sizeof(OverviewPos));


  int active_id = ctx_clients_active (ctx);
  int active_no = 0;
  CtxClient *active = NULL;

  int top_of_class = 0;
  {
    int rows = 1;
    int cols = 1;

    if (n_clients <= 4) { rows = 2; cols = 2;}

    while ( n_clients > cols * rows)
    {
      if (cols > rows * 1.2)
      {
	 rows++;
      } else cols++;
    }

    float col_width = screen_width / cols;
    float row_height = screen_height / rows;

    float icon_width = col_width - em;
    //float icon_height = row_height - em;
    
    int i = 0;

    for (CtxList *l = clients; l; l = l->next)
    {
      CtxClient *client = l->data;
      int client_id = ctx_client_id (client);

      int col = i % cols;
      int row = i / cols;

      opos[i].client = client;
      opos[i].id = client_id;
      opos[i].x0 = ctx_client_get_x (ctx, client_id);
      opos[i].y0 = ctx_client_get_y (ctx, client_id) - em * (n_clients > 1);
      opos[i].w0 = ctx_client_get_width(ctx, client_id);
      opos[i].h0 = ctx_client_get_height(ctx, client_id);
      opos[i].x1 = col * col_width + em;
      opos[i].y1 = row * row_height + em;

      opos[i].scale0 = 1.0f;
      opos[i].scale1 = icon_width/opos[i].w0;
    
      if ((ctx_client_flags (client) & CSS_CLIENT_MAXIMIZED))
      {
	 if (!ctx_client_is_active_tab (ctx, client))
	 {
           opos[i].x0 = opos[i].x1;
           opos[i].y0 = opos[i].y1;
           opos[i].scale0 = opos[i].scale1;
	 }
	 top_of_class = i;
      }
      if (client_id == active_id){
	      active_no = i;
	      active = client;
      }

      i++;
    }
  }


  if (!(ctx_client_flags (active) & CSS_CLIENT_MAXIMIZED))
    top_of_class = n_clients - 1;


  if (active_no != top_of_class)
  {
    OverviewPos temp = opos[top_of_class];
    opos[top_of_class] = opos[active_no];
    opos[active_no] = temp;
  }
}

static void overview_select_client (CtxEvent *event, void *client, void *data2)
{
  in_overview = 0;
  leave_overview = OVERVIEW_TRANSITION_LENGTH;

  int i;
  int old_active = 0;
  int new_active = 0;

  for (i = 0; i < opos_count; i++)
  {
    if (ctx_client_is_active_tab (event->ctx, opos[i].client))
      old_active = i;
    if (opos[i].client == client)
      new_active = i;
  }
  if (new_active != old_active)
  {
    opos[new_active].x0 = opos[old_active].x0;
    opos[new_active].y0 = opos[old_active].y0;
    opos[new_active].scale0 = 1.0f;
    opos[new_active].w0 = opos[old_active].w0;
    opos[new_active].h0 = opos[old_active].h0;

    opos[old_active].x0 = opos[old_active].x1;
    opos[old_active].y0 = opos[old_active].y1;
    opos[old_active].scale0 = opos[old_active].scale1;


    OverviewPos temp = opos[new_active];
    opos[new_active] = opos[old_active];
    opos[old_active] = temp;

  }


  event->stop_propagate =1;
  ctx_client_focus (event->ctx, ctx_client_id (client));
  overview_init (ctx);
  ctx_queue_draw (event->ctx);
}


static void overview (Ctx *ctx, float anim_t)
{
  CtxList *clients = ctx_clients (ctx);
  float em = ctx_get_font_size (ctx);
  int n_clients         = ctx_list_length (clients);

  float screen_width = ctx_width (ctx) - 3 * em;
  //float screen_height = ctx_height (ctx);

  static int dirty_count = 0;
  if (n_clients != opos_count)
  {
    dirty_count = 2;
  }

  if (!opos || dirty_count > 0)
  {
    if (dirty_count > 0)
      dirty_count--;
    overview_init (ctx);
  }

  int rows = 1;
  int cols = 1;

  if (n_clients <= 4) { rows = 2; cols = 2;}

  while ( n_clients > cols * rows)
  {
    if (cols > rows * 1.2)
    {
       rows++;
    } else cols++;
  }

  float col_width  = screen_width / cols;
  //float row_height = screen_height / rows;
  float icon_width  = col_width - em;
  //float icon_height = row_height - em;
  
  int i = 0;

  ctx_rgb(ctx,0.1,0.1,0.1);
  ctx_paint(ctx);

  for (i = 0; i < opos_count; i++)
  {
    CtxClient *client = opos[i].client;
    float x0 = opos[i].x0;
    float y0 = opos[i].y0;
    float x1 = opos[i].x1;
    float y1 = opos[i].y1;
    float w0 = opos[i].w0;
    float h0 = opos[i].h0;
    float scale0 = opos[i].scale0;
    float scale1 = opos[i].scale1;

    float x = x0 * (1.0-anim_t) + anim_t * x1;
    float y = y0 * (1.0-anim_t) + anim_t * y1;
    float scale = scale0 * (1.0-anim_t) + anim_t * scale1;
    float w = w0 * (1.0-anim_t) + anim_t * icon_width;
    float h = h0 * (w / w0);

    ctx_save(ctx);
    ctx_scale (ctx, scale, scale);
    ctx_client_draw (ctx, client, x / scale, y / scale);
    ctx_restore(ctx);
    ctx_rectangle (ctx, x-2, y-2, w+4, h+4);
    ctx_listen (ctx, CTX_DRAG_PRESS, overview_select_client, client, NULL);

    if (opos[i].id == ctx_clients_active (ctx))
	{ ctx_rgba(ctx,1,1,1,1);
          ctx_stroke(ctx);
	}
    else
    ctx_begin_path (ctx);
  }
}


#if CTX_BIN_BUNDLE
int ctx_terminal_main (int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{
  execute_self = argv[0];
  const char *commandline = NULL;
  float global_scale = 1.0;
  int width = -1;
  int height = -1;
  int cols = -1;

  if (getpid () == 1)
  {
    int ign;
    ign = system ("pkill plymouth"); // needed to enable keyboard input.. with the initrd that
                               // gets used with systemd
    ign = system ("mount -o remount,rw /");
    ign = system ("mount -a");
    if (ign) {};
  }

  for (int i = 1; argv[i]; i++)
  {
    if (!strcmp (argv[i], "--help"))
    {
    }
    else if (!strcmp (argv[i], "-e"))
    {
      commandline = argv[i+1];
      i++;
      if (!strcmp (commandline, "--"))
      {
        i++;
        commandline_argv_start = i;
      }
      terminal_no_new_tab = 1;
    }
    else if (!strcmp (argv[i], "--width"))
        width = consume_float (argv, &i);
    else if (!strcmp (argv[i], "--height"))
        height = consume_float (argv, &i);
    else if (!strcmp (argv[i], "--cols"))
        cols = consume_float (argv, &i);
    else if (!strcmp (argv[i], "--font-size"))
        font_size = consume_float (argv, &i) * global_scale;
    else if (argv[i][0]=='-')
    {
      switch (argv[i][1])
      {
        case 's': font_size = consume_float (argv, &i) * global_scale; break;
        case 'c': cols = consume_float (argv, &i); break;
        case 'w': width = consume_float (argv, &i); break;
        case 'h': height = consume_float (argv, &i); break;
      }
    }
  }

  if (font_size > 0 && width <0 && cols < 0)
  {
    cols = 80;
    width = font_size * cols / 2;
    height = font_size * 25;
  }

  ctx  = ctx_new (width, height, NULL);
  width = ctx_width (ctx);
  height = ctx_height (ctx);
  

#if CTX_TERM
  if (ctx_backend_type (ctx) == CTX_BACKEND_TERM && font_size <= 0)
  {
    font_size = ctx_term_get_cell_height (ctx);
  }
#endif

  if (cols <= 0)
  {
    //if (((double)(width))/height < (16.0/9.0))
    //  cols = 80;
    //else
      cols = 80;
  }

  if (font_size < 0)
    font_size = floorf (width / cols  * 2 / 1.5);

#if CTX_CSS
  Css *itk = css_new (ctx);
  css_set_scale (itk, 1.0);
  css_set_font_size (itk, font_size);
#else
  void *itk = NULL;
#endif

  start_font_size = font_size;
  ctx_font_size (ctx, font_size);

  if (commandline_argv_start)
  {
    ctx_client_maximize (ctx, add_tab_argv (ctx, &argv[commandline_argv_start], 1));
  }
  else
  {
    if (!commandline)
      commandline = ctx_find_shell_command();
    ctx_client_maximize (ctx, add_tab (ctx, commandline, 1));
  }

  int mt = ctx_add_timeout (ctx, 1000 * 20, malloc_trim_cb, NULL);

  //int sleep_time = 1000000/10;


  float prev_ms = ctx_ms (ctx);

  while (ctx_clients (ctx) && !ctx_has_exited (ctx))
    {
      //int changes = 0;
      int n_clients = ctx_list_length (ctx_clients (ctx));
      //ensure_layout (ctx);
      float ms = ctx_ms (ctx);
      float delta_s = (ms - prev_ms)/1000.0f;
      prev_ms = ms;

      if (ctx_need_redraw(ctx))
      {
#if CTX_CSS
        css_reset (itk);
        css_style_bg (itk, "wallpaper");
        ctx_font_size (ctx, css_em (itk));
#else
	ctx_start_frame (ctx);
	ctx_rgb (ctx, 0,0,0);
        ctx_font_size (ctx, font_size);
#endif
        ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
        ctx_fill (ctx);

	if (in_overview || leave_overview > 0.0f)
	{
	  if (leave_overview > 0.0f)
	  {
	    leave_overview -= delta_s;
            overview (ctx, leave_overview / OVERVIEW_TRANSITION_LENGTH);
	      ctx_queue_draw (ctx);
	  }
	  else
	  {
	    leave_overview = 0.0f;
	    overview_t += delta_s;
	    if (overview_t >= OVERVIEW_TRANSITION_LENGTH)
	      overview_t = OVERVIEW_TRANSITION_LENGTH;
	    else
	      ctx_queue_draw (ctx);
            overview (ctx, overview_t / OVERVIEW_TRANSITION_LENGTH);
	  }
	  
	}
	else
	{


        ctx_clients_draw (ctx, 0);

        if ((n_clients != 1) || (ctx_clients (ctx) &&
                                 !flag_is_set(
                                         ctx_client_flags (((CtxClient*)ctx_clients(ctx)->data)), CSS_CLIENT_MAXIMIZED)))
          draw_panel (itk, ctx);
        else
          draw_mini_panel (ctx);

	}
        ctx_osk_draw (ctx);
        //ctx_add_key_binding (ctx, "unhandled", NULL, "", terminal_key_any, NULL);
        ctx_listen (ctx, CTX_KEY_PRESS, terminal_key_any, NULL, NULL);
        ctx_listen (ctx, CTX_KEY_DOWN,  terminal_key_any, NULL, NULL);
        ctx_listen (ctx, CTX_KEY_UP,    terminal_key_any, NULL, NULL);

        ctx_rectangle (ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
        ctx_listen (ctx, CTX_TAP_AND_HOLD, terminal_long_tap, NULL, NULL);
        ctx_begin_path (ctx);

        ctx_end_frame (ctx);
      }

     {
       int active_id = ctx_clients_active (ctx);
       CtxClient *active = active_id>=0?ctx_client_by_id (ctx, active_id):NULL;
       if (active)
         terminal_update_title (ctx_client_title (active));
     }

     ctx_handle_events (ctx);
    }


  ctx_remove_idle (ctx, mt);

#if CTX_CSS
  css_destroy (itk);
#endif
  ctx_destroy (ctx);

  if (getpid () == 1)
  {
    if (system ("reboot --force")){};
  }

  return 0;
}

#else

#if CTX_BIN_BUNDLE
int ctx_terminal_main (int argc, char **argv)
#else
int main (int argc, char **argv)
#endif
{ return -1;
}

#endif
