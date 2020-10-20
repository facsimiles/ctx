#define _DEFAULT_SOURCE

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
#include <pty.h>

#ifndef NO_SDL
#include <SDL.h> // for clipboard texts
#endif

#include "ctx.h"
#include "vt-line.h"
#include "terminal.h"
#include "vt.h"

Ctx *ctx = NULL; // initialized in main

#define CTX_x            CTX_STRH('x',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_y            CTX_STRH('y',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_lower_bottom CTX_STRH('l','o','w','e','r','-','b','o','t','t','o','m',0,0)
#define CTX_lower        CTX_STRH('l','o','w','e','r',0,0,0,0,0,0,0,0,0)
#define CTX_raise        CTX_STRH('r','a','i','s','e',0,0,0,0,0,0,0,0,0)
#define CTX_raise_top    CTX_STRH('r','a','i','s','e','-','t','o','p',0,0,0,0,0)
#define CTX_terminate    CTX_STRH('t','e','r','m','i','n','a','t','e',0,0,0,0,0)
#define CTX_maximize     CTX_STRH('m','a','x','i','m','i','z','e',0,0,0,0,0,0)
#define CTX_unmaximize   CTX_STRH('u','n','m','a','x','i','m','i','z','e',0,0,0,0)
#define CTX_width        CTX_STRH('w','i','d','t','h',0,0,0,0,0,0,0,0,0)
#define CTX_title        CTX_STRH('t','i','t','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_action       CTX_STRH('a','c','t','i','o','n',0,0,0,0,0,0,0,0)
#define CTX_height       CTX_STRH('h','e','i','g','h','t',0,0,0,0,0,0,0,0)

void ctx_sdl_set_title (void *self, const char *new_title);
int ctx_renderer_is_sdl (Ctx *ctx);
int ctx_renderer_is_braille (Ctx *ctx);

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);

int vt_set_prop (VT *vt, uint32_t key_hash, const char *val)
{
#if 1
//fprintf (stderr, "%i: %s\n", key_hash, val);
  switch (key_hash)
  {
    case CTX_title:  
     ctx_set (ctx, CTX_title, val, strlen (val));
#ifndef NO_SDL
     if (ctx_renderer_is_sdl (ctx))
       ctx_sdl_set_title (ctx_get_renderer (ctx), val);
#endif

     break;
  }
#else
  float fval = strtod (val, NULL);
  CtxClient *client = client_by_id (ct->id);
  uint32_t val_hash = ctx_strhash (val, 0);
  if (!client)
    return 0;

  if (key_hash == ctx_strhash("start_move", 0))
  {
    start_moving (client);
    moving_client = 1;
    return 0;
  }

  //fprintf (stderr, "%i %s\n", key_hash, val);

// set "pcm-hz"       "8000"
// set "pcm-bits"     "8"
// set "pcm-encoding" "ulaw"
// set "play-pcm"     "d41ata312313"
// set "play-pcm-ref" "foo.wav"

// get "free"
// storage of blobs for referencing when drawing or for playback
// set "foo.wav"      "\3\1\1\4\"
// set "fnord.png"    "PNG12.4a312"

  switch (key_hash)
  {
    case CTX_title:  client_set_title (ct->id, val); break;
    case CTX_x:      client->x = fval; break;
    case CTX_y:      client->y = fval; break;
    case CTX_width:  client_resize (ct->id, fval, client->height); break;
    case CTX_height: client_resize (ct->id, client->width, fval); break;
    case CTX_action:
      switch (val_hash)
      {
        case CTX_maximize:     client_maximize (client); break;
        case CTX_unmaximize:   client_unmaximize (client); break;
        case CTX_lower:        client_lower (client); break;
        case CTX_lower_bottom: client_lower_bottom (client);  break;
        case CTX_raise:        client_raise (client); break;
        case CTX_raise_top:    client_raise_top (client); break;
      }
      break;
  }
  ct->rev++;
  fprintf (stderr, "%s: %i %s %i\n", __FUNCTION__, key_hash, val, len);
#endif
  return 0;
}


int vtpty_waitdata (void  *data, int timeout)
{
  VtPty *vtpty = data;
  struct timeval tv;
  fd_set fdset;
  FD_ZERO (&fdset);
  FD_SET (vtpty->pty, &fdset);
  tv.tv_sec = 0;
  tv.tv_usec = timeout;
  tv.tv_sec  = timeout / 1000000;
  tv.tv_usec = timeout % 1000000;
  if (select (vtpty->pty+1, &fdset, NULL, NULL, &tv) == -1)
    {
      perror ("select");
      return 0;
    }
  if (FD_ISSET (vtpty->pty, &fdset) )
    {
      return 1;
    }
  return 0;
}

typedef struct _CtxClient CtxClient;

CtxList *vts = NULL;

static void signal_child (int signum)
{
  pid_t pid;
  int   status;
  if ( (pid = waitpid (-1, &status, WNOHANG) ) != -1)
    {
      if (pid)
        {
          for (CtxList *l = vts; l; l=l->next)
            {
              VtPty *vt = l->data;
              if (vt->pid == pid)
                {
                  vt->done = 1;
                  //vt->result = status;
                }
            }
        }
    }
}

void vtpty_resize (void *data, int cols, int rows, int px_width, int px_height)
{
  VtPty *vtpty = data;
  struct winsize ws;
  ws.ws_row = rows;
  ws.ws_col = cols;
  ws.ws_xpixel = px_width;
  ws.ws_ypixel = px_height;
  ioctl (vtpty->pty, TIOCSWINSZ, &ws);
}

ssize_t vtpty_write (void *data, const void *buf, size_t count)
{
  VtPty *vtpty = data;
  return write (vtpty->pty, buf, count);
}

ssize_t vtpty_read (void  *data, void *buf, size_t count)
{
  VtPty *vtpty = data;
  return read (vtpty->pty, buf, count);
}

struct
_CtxClient {
  VT           *vt;
  char         *title;
  int           x;
  int           y;
  int           width;
  int           height;
  int           maximized;
  int           unmaximized_x;
  int           unmaximized_y;
  int           unmaximized_width;
  int           unmaximized_height;
  int           do_quit;
  long          drawn_rev;
  int           id;
};

int   do_quit      = 0;
float font_size    = -1;
float line_spacing = 2.0;

int   on_screen_keyboard = 0;

static char *execute_self = NULL;

static CtxList *clients;
static CtxClient *active = NULL;

static CtxClient *client_by_id (int id);

int client_resize (int id, int width, int height);

void terminal_long_tap (Ctx *ctx, VT *vt)
{
  on_screen_keyboard = !on_screen_keyboard;
  vt_rev_inc (vt); // forcing redraw
}

CtxClient *add_client (const char *commandline, int x, int y, int width, int height, int ctx)
{
  static int global_id = 0;
  CtxClient *client = calloc (sizeof (CtxClient), 1);
  ctx_list_append (&clients, client);
  client->id = global_id++;
  client->x = x;
  client->y = y;

  client->width = width;
  client->height = height;
  //client->vt = vt_new (commandline, width/font_size*2, height/font_size, font_size, line_spacing, client->id);
  client->vt = vt_new (commandline, width, height, font_size, line_spacing, client->id);
  return client;
}

CtxClient *add_client_argv (const char **argv, int x, int y, int width, int height, int ctx)
{
  VtString *string = vt_string_new ("");
  for (int i = 0; argv[i]; i++)
  {
    if (i > 0)
      vt_string_append_byte (string, ' ');
    for (int c = 0; argv[i][c]; c++)
    {
       switch (argv[i][c])
       {
         case '"':vt_string_append_str (string, "\\\"");break;
         case '\'':vt_string_append_str (string, "\\\'");break;
         default:vt_string_append_byte (string, argv[i][c]);break;
       }
    }
  }
  CtxClient *ret = add_client (string->str, x, y, width, height, ctx);
  vt_string_free (string, 1);
  return ret;
}

extern float ctx_shape_cache_rate;

static CtxClient *find_active (int x, int y)
{
  CtxClient *ret = NULL;
  for (CtxList *l = clients; l; l = l->next)
  {
     CtxClient *c = l->data;
     if (x > c->x && x < c->x+c->width &&
         y > c->y && y < c->y+c->height)
       ret = c;
  }
  //active = ret;
  return ret;
}

int id_to_no (int id)
{
  CtxList *l;
  int no = 0;

  for (l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->id == id)
      return no;
    no++;
  }
  return -1;
}

static CtxClient *client_by_id (int id)
{
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
    if (client->id == id)
      return client;
  }
  return NULL;
}

void client_remove (CtxClient *client)
{
  if (client->vt)
    vt_destroy (client->vt);

  if (client->title)
    free (client->title);

  ctx_list_remove (&clients, client);
  if (clients == NULL)
    do_quit = 1;

  if (client == active)
  {
    active = NULL;//find_active (last_x, last_y);
  }
  free (client);
}

#if 0
void client_remove_by_id (int id)
{
  int no = id_to_no (id);
  if (no>=0)
    client_remove (no);
}
#endif

static void handle_event (const char *event)
{
  if (!active)
    return;
  VT *vt = active->vt;
  if (!strcmp (event, "shift-return"))
   event = "return";
  else
  if (!strcmp (event, "shift-control-v") )
    {
#ifndef NO_SDL
      char *text = SDL_GetClipboardText ();
      if (text)
        {
          if (vt)
            vt_paste (vt, text);
          free (text);
        }
#endif
    }
  else if (!strcmp (event, "shift-control-c") && vt)
    {
#ifndef NO_SDL
      char *text = vt_get_selection (vt);
      if (text)
        {
          SDL_SetClipboardText (text);
          free (text);
        }
#endif
    }

  else if (!strcmp (event, "shift-control-n") )
    {
      pid_t pid;
      if ( (pid=fork() ) ==0)
        {
          unsetenv ("CTX_VERSION");
          execlp (execute_self, execute_self, NULL);
          exit (0);
        }
      fprintf (stderr, "!%s!\n", execute_self);
    }
  else if (!strcmp (event, "shift-control-q") )
    {
      do_quit = 1; // global?
    }
  else if (!strcmp (event, "shift-control-w") )
    {
      active->do_quit = 1;
    }
  else if (!strcmp (event, "shift-control-s") )
    {
      if (vt)
      {
        char *sel = vt_get_selection (vt);
        if (sel)
        {
          vt_feed_keystring (vt, sel);
          free (sel);
        }
      }
    }
  else
    {
      if (vt)
        vt_feed_keystring (vt, event);
    }
}

void client_move (int id, int x, int y)
{
   CtxClient *client = client_by_id (id);
   if (!client) return;
   client->x = x;
   client->y = y;
}

int client_resize (int id, int width, int height)
{
   CtxClient *client = client_by_id (id);

   if (client && ((height != client->height) || (width != client->width) ))
   {
     client->width = width;
     client->height = height;
     vt_set_px_size (client->vt, width, height);
     return 1;
   }
   return 0;
}

int is_reset = 0;
void prepare_for_draw (Ctx *ctx)
{
  if (!is_reset)
  {
    ctx_reset (ctx);
    is_reset = 1;
  }
}

static int dirty = 0;

int update_vt (Ctx *ctx, CtxClient *client, int is_reset)
{
  VT *vt = client->vt;
  int in_scroll = (vt_has_blink (client->vt) >= 10);

  if ( (client->drawn_rev != vt_rev (vt) ) ||
       vt_has_blink (vt) ||
       in_scroll ||
       dirty)
    {
      prepare_for_draw (ctx);

      client->drawn_rev = vt_rev (vt);
      vt_draw (vt, ctx, client->x, client->y);
      return 1;
    }
  return 0;
}

int ctx_count (Ctx *ctx);

static int dirt = 0;

static int update_vts (Ctx *ctx, int changes_in)
{
  int changes = 0;
  is_reset = 0;
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
      if (client->vt && update_vt (ctx, client, dirt + changes + changes_in))
          changes++;
  }
  dirt += changes;
  return changes;
}

void vt_set_ctx (VT *vt, Ctx *ctx);

#if 0
static int consume_toggle (char **argv, int *i)
{
 int j;
 for (j = *i; argv[j]; j++)
 {
   argv[j-1] = argv[j];
 }
 argv[j-1]=0;
 *i-=1;
 return 1;
}
#endif

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
//

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
  int   down;
  int   hovered;
} KeyCap;

typedef struct KeyBoard {
  KeyCap keys[9][30];
  int shifted;
  int control;
  int alt;
  int fn;
  int down;
} KeyBoard;


static void ctx_on_screen_key_event (CtxEvent *event, void *data1, void *data2)
{
  KeyCap *key = data1;
  KeyBoard *kb = data2;
  float h = ctx_height (ctx);
  float w = ctx_width (ctx);
  int rows;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / 14.5; // keycell
  float y0 = h - c * rows;

  key = NULL;

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
      float y = row * c + y0;
      ctx_round_rectangle (ctx, x, y,
                                c * (cap->wfactor-0.1),
                                c * 0.9,
                                c * 0.1);
      if (event->x >= x &&
          event->x < x + c * cap->wfactor-0.1 &&
          event->y >= y &&
          event->y < y + c * 0.9)
       {
         key = cap;
         if (cap->hovered != 1)
         {
           dirty ++;
         }
         cap->hovered = 1;
       }
      else
       {
         cap->hovered = 0;
       }

      x += cap->wfactor * c;
    }
  }

  event->stop_propagate = 1;
  switch (event->type)
  {
     default:
       break;
     case CTX_MOTION:
         dirty ++;
       break;
     case CTX_DRAG_MOTION:
       if (!key)
         dirty ++;
       break;
     case CTX_DRAG_PRESS:
       kb->down = 1;
       dirty ++;
       break;
     case CTX_DRAG_RELEASE:
       kb->down = 0;
        dirty ++;
       if (!key)
         return;

      if (key->sticky)
      {
        if (key->down)
          key->down = 0;
        else
          key->down = 1;

        if (!strcmp (key->label, "Shift"))
        {
          kb->shifted = key->down;
        }
        else if (!strcmp (key->label, "Ctrl"))
        {
          kb->control = key->down;
        }
        else if (!strcmp (key->label, "Alt"))
        {
          kb->alt = key->down;
        }
        else if (!strcmp (key->label, "Fn"))
        {
          kb->fn = key->down;
        }
      }
      else
      {
        if (kb->control || kb->alt)
        {
          char combined[200]="";
          if (kb->shifted)
          {
            sprintf (&combined[strlen(combined)], "shift-");
          }
          if (kb->control)
          {
            sprintf (&combined[strlen(combined)], "control-");
          }
          if (kb->alt)
          {
            sprintf (&combined[strlen(combined)], "alt-");
          }
          if (kb->fn)
            sprintf (&combined[strlen(combined)], key->sequence_fn);
          else
            sprintf (&combined[strlen(combined)], key->sequence);
          ctx_key_press (ctx, 0, combined, 0);
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
          ctx_key_press (ctx, 0, sequence, 0);
        }
      }
      break;
  }
}

KeyBoard en_intl = {
   {  
     { {"Esc","Esc","`","~",1.0f,"escape","escape","`","~",0},
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
       {"-","_","F11","F11",1.0f,"-","_","F11","F11",0},
       {"=","+","F12","F12",1.0f,"=","+","F12","F12",0},
       {"⌫","⌫",NULL,NULL,1.2f,"backspace","backspace",NULL,NULL,0},
       {NULL}},
     //⌨
     { {"Tab","Tab",NULL,NULL,1.3f,"tab","tab",NULL,NULL,0},
       {"q","Q",NULL,NULL,1.0f,"q","Q",NULL,NULL,0},
       {"w","W",NULL,NULL,1.0f,"w","W",NULL,NULL,0},
       {"e","E",NULL,NULL,1.0f,"e","E",NULL,NULL,0},
       {"r","R",NULL,NULL,1.0f,"r","R",NULL,NULL,0},
       {"t","T",NULL,NULL,1.0f,"t","T",NULL,NULL,0},
       {"y","Y",NULL,NULL,1.0f,"y","Y",NULL,NULL,0},
       {"u","U",NULL,NULL,1.0f,"u","U",NULL,NULL,0},
       {"i","I",NULL,NULL,1.0f,"i","I",NULL,NULL,0},
       {"o","O",NULL,NULL,1.0f,"o","O",NULL,NULL,0},
       {"p","P",NULL,NULL,1.0f,"p","P",NULL,NULL,0},
       {"[","{",NULL,NULL,1.0f,"[","{",NULL,NULL,0},
       {"]","}",NULL,NULL,1.0f,"]","}",NULL,NULL,0},
       {"\\","|",NULL,NULL,1.0f,"\\","|",NULL,NULL,0},
       {NULL} },
     { {"Fn","Fn",NULL,NULL,1.5f," "," ",NULL,NULL,1},
       {"a","A",NULL,NULL,1.0f,"a","A",NULL,NULL,0},
       {"s","S",NULL,NULL,1.0f,"s","S",NULL,NULL,0},
       {"d","D",NULL,NULL,1.0f,"d","D",NULL,NULL,0},
       {"f","F",NULL,NULL,1.0f,"f","F",NULL,NULL,0},
       {"g","G",NULL,NULL,1.0f,"g","G",NULL,NULL,0},
       {"h","H",NULL,NULL,1.0f,"h","H",NULL,NULL,0},
       {"j","J",NULL,NULL,1.0f,"j","J",NULL,NULL,0},
       {"k","K",NULL,NULL,1.0f,"k","K",NULL,NULL,0},
       {"l","L",NULL,NULL,1.0f,"l","L",NULL,NULL,0},
       {";",":",NULL,NULL,1.0f,";",":",NULL,NULL,0},
       {"'","\"",NULL,NULL,1.0f,"'","\"",NULL,NULL,0},
       {"⏎","⏎",NULL,NULL,1.5f,"return","return",NULL,NULL,0},
       {NULL} },
     { {"Ctrl","Ctrl",NULL,NULL,1.7f,"","",NULL,NULL,1},
       {"z","Z",NULL,NULL,1.0f,"z","Z",NULL,NULL,0},
       {"x","X",NULL,NULL,1.0f,"x","X",NULL,NULL,0},
       {"c","C",NULL,NULL,1.0f,"c","C",NULL,NULL,0},
       {"v","V",NULL,NULL,1.0f,"v","V",NULL,NULL,0},
       {"b","B",NULL,NULL,1.0f,"b","B",NULL,NULL,0},
       {"n","N",NULL,NULL,1.0f,"n","N",NULL,NULL,0},
       {"m","M",NULL,NULL,1.0f,"m","M",NULL,NULL,0},
       {",","<",NULL,NULL,1.0f,",","<",NULL,NULL,0},
       {".",">",NULL,NULL,1.0f,".",">",NULL,NULL,0},
       {"/","?",NULL,NULL,1.0f,"/","?",NULL,NULL,0},
       {"↑","↑","PgUp","PgUp",1.0f,"up","up","page-up","page-up",0},
       {NULL} },
     { {"Shift","Shift",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {"Alt","Alt",NULL,NULL,1.3f,"","",NULL,NULL,1},
       {" "," ",NULL,NULL,8.1f,"space","space",NULL,NULL,0},

       {"←","←","Home","Home",1.0f,"left","left","home","home",0},
       {"↓","↓","PgDn","PgDn",1.0f,"down","down","page-down","page-down",0},
       {"→","→","End","End",1.0f,"right","right","end","end",0},
       {NULL} },
     { {NULL}},
   }
};



void ctx_osk_draw (Ctx *ctx)
{
  if (!on_screen_keyboard)
    return;
  static float fade = 0.0;
  KeyBoard *kb = &en_intl;

  fade = 0.2;
  if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
     fade = 0.9;

  float h = ctx_height (ctx);
  float w = ctx_width (ctx);
  float m = h;
  int rows = 0;
  for (int row = 0; kb->keys[row][0].label; row++)
    rows = row+1;

  float c = w / 14.5; // keycell
  float y0 = h - c * rows;
  if (w < h)
    m = w;
      
  prepare_for_draw (ctx);

  ctx_save (ctx);
#if 1
  ctx_round_rectangle (ctx, 0,
                            y0,
                            w,
                            c * rows,
                            c * 0.0);
  ctx_listen (ctx, CTX_DRAG, ctx_on_screen_key_event, NULL, &en_intl);
  ctx_rgba (ctx, 0,0,0, 0.5 * fade);
  ctx_preserve (ctx);
  if (kb->down || kb->alt || kb->control || kb->fn || kb->shifted)
    ctx_fill (ctx);
  //ctx_line_width (ctx, m * 0.01);
  ctx_begin_path (ctx);
#if 0
  ctx_rgba (ctx, 1,1,1, 0.5);
  ctx_stroke (ctx);
#endif
#endif

  ctx_text_align (ctx, CTX_TEXT_ALIGN_CENTER);
  ctx_line_width (ctx, m * 0.01);

  float font_size = c * 0.7;
  ctx_font_size (ctx, font_size);

  for (int row = 0; kb->keys[row][0].label; row++)
  {
    float x = c * 0.0;
    for (int col = 0; kb->keys[row][col].label; col++)
    {
      KeyCap *cap = &(kb->keys[row][col]);
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
        if (font_size != c * 0.33)
        {
          font_size = c * 0.33;
          ctx_font_size (ctx, font_size);
        }
      }
      else
      {
        if (font_size != c * 0.7)
        {
          font_size = c * 0.7;
          ctx_font_size (ctx, font_size);
        }
      }

      ctx_begin_path (ctx);
      ctx_round_rectangle (ctx, x, y,
                                c * (cap->wfactor-0.1),
                                c * 0.9,
                                c * 0.1);
      //ctx_listen (ctx, CTX_MOTION|CTX_PRESS|CTX_RELEASE|CTX_ENTER|CTX_LEAVE, ctx_on_screen_key_event, cap, &en_intl);
      
      if (cap->down || (cap->hovered && kb->down))
      {
        ctx_rgba (ctx, 1,1,1, fade);
#if 1
      ctx_fill (ctx);
#else
      ctx_preserve (ctx);
      ctx_fill (ctx);

      ctx_rgba (ctx, 0,0,0, fade);

      ctx_stroke (ctx);
#endif
      }
      if (cap->down || (cap->hovered && kb->down))
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

      if (cap->down || (cap->hovered && kb->down))
        ctx_rgba (ctx, 0,0,0, fade);
      else
        ctx_rgba (ctx, 1,1,1, fade);

      ctx_text (ctx, label);

      x += cap->wfactor * c;
    }
  }
  ctx_restore (ctx);
}

static void terminal_key_any (CtxEvent *event, void *userdata, void *userdata2)
{
  if (!strcmp (event->string, "resize-event"))
  {
     if (active)
       client_resize (active->id, ctx_width (ctx), ctx_height (ctx));
  }
  else
  {
    handle_event (event->string);
  }
}

int terminal_main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  execute_self = argv[0];
  float global_scale = 1.0;
  int width = -1;
  int height = -1;
  int cols = -1;

  for (int i = 1; argv[i]; i++)
  {
    if (!strcmp (argv[i], "--help"))
    {
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

  ctx  = ctx_new_ui (width, height);
  width = ctx_width (ctx);
  height = ctx_height (ctx);

  if (ctx_renderer_is_braille (ctx) && font_size <= 0)
  {
    font_size = 10.0;
  }

  if (cols < 0)
    cols = 80;

  if (font_size < 0)
    font_size = 2 * width / cols;

  if (argv[1] == NULL)
  {
    active = add_client (vt_find_shell_command(), 0, 0, width, height, 0);
  }
  else
  {
    active = add_client_argv ((void*)&argv[1], 0, 0, width, height, 0);
  }
  if (!active)
    return 1;
  vt_set_ctx (active->vt, ctx);

#if 0
  active=add_client (vt_find_shell_command(), width/2, height/2, width/2, height/2, 0);

  vt_set_ctx (active->vt, ctx);
#endif
  signal (SIGCHLD,signal_child);


  int sleep_time = 200;
  while (clients)
    {
      CtxList *to_remove = NULL;
      int changes = 0;
      CtxClient *client = find_active (ctx_pointer_x (ctx), ctx_pointer_y (ctx));
      if (client)
        active = client;

      for (CtxList *l = clients; l; l = l->next)
      {
        CtxClient *client = l->data;
        if (client->vt)
          {
            if (vt_is_done (client->vt))
              ctx_list_prepend (&to_remove, client);
          }
      }
      for (CtxList *l = to_remove; l; l = l->next)
      {
        client_remove (l->data);
        changes++;
      }
      while (to_remove)
      {
        ctx_list_remove (&to_remove, to_remove->data);
      }

      if (dirty)
        changes ++;
      changes += update_vts (ctx, changes);


      if (changes)
      {
        dirty = 0;
        ctx_osk_draw (ctx);
        ctx_add_key_binding (ctx, "unhandled", NULL, "", terminal_key_any, NULL);
        ctx_flush (ctx);
      }
      else
      {
        usleep (1000 * 5);
      }

      CtxEvent *event;
      while ((event = ctx_get_event (ctx)))
      {
      }
      for (CtxList *l = clients; l; l = l->next)
      {
        CtxClient *client = l->data;
        if (vt_poll (client->vt, sleep_time) )
        {
        }
      }
    }

  while (clients)
    client_remove (clients->data);

  ctx_free (ctx);
  return 0;
}
