

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

#define ENABLE_CLICK 0

#include <SDL.h>
#include "ctx.h"
#include "vt-line.h"
#include "terminal.h"

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


typedef struct _CT CT;
typedef struct _CtxClient CtxClient;

typedef struct CTRenderer {
   CtxImplementation vfuncs;
   CT *ct;
} CTRenderer;

struct _CT
{
  VtPty      vtpty;
  int       id;
  unsigned char buf[BUFSIZ]; // need one per vt
  int keyrepeat;
  int       lastx;
  int       lasty;
  int        done;
  int        result;
  long       rev;

  CtxClient *client;
#if 0
  ssize_t (*write) (void *serial_obj, const void *buf, size_t count);
  ssize_t (*read) (void *serial_obj, void *buf, size_t count);
  int    (*waitdata) (void *serial_obj, int timeout);
  void   (*resize) (void *serial_obj, int cols, int rows, int px_width, int px_height);
#endif
  CTRenderer renderer;
  Ctx       *ctx;
  CtxParser *parser;
};

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
              CT *ct = l->data;
              if (ct->vtpty.pid == pid)
                {
                  ct->done = 1;
                  ct->result = status;
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

int ct_poll (CT *ct, int timeout)
{
  int read_size = sizeof (ct->buf);
  int got_data = 0;
  int remaining_chars = 1024 * 1024;
  int len = 0;
  // audio_task (vt, 0);
  read_size = ctx_mini (read_size, remaining_chars);
  while (timeout > 100 &&
         remaining_chars > 0 &&
         vtpty_waitdata ((void*)ct, timeout))
    {
      len = vtpty_read ((void*)ct, ct->buf, read_size);
      for (int i = 0; i < len; i++)
        { ctx_parser_feed_byte (ct->parser, ct->buf[i]); }
      got_data+=len;
      remaining_chars -= len;
      timeout -= 10;
   // audio_task (vt, 0);
    }
  if (got_data < 0)
    {
      if (kill (ct->vtpty.pid, 0) != 0)
        {
          ct->done = 1;
        }
    }
  else
  {
    //ct->rev++;
  }
  return got_data;
}

static void ct_run_command (CT *vt, const char *command, int width, int height)
{
  struct winsize ws;
  ws.ws_row = 40;
  ws.ws_col = 20;
  ws.ws_xpixel = width;
  ws.ws_ypixel = height;
  vt->vtpty.pid = forkpty (&vt->vtpty.pty, NULL, NULL, &ws);
  if (vt->vtpty.pid == 0)
    {
      int i;
      for (i = 3; i<768; i++) { close (i); } /*hack, trying to close xcb */
      unsetenv ("TERM");
      unsetenv ("COLUMNS");
      unsetenv ("LINES");
      unsetenv ("TERMCAP");
      unsetenv ("COLOR_TERM");
      unsetenv ("COLORTERM");
      unsetenv ("VTE_VERSION");
      setenv ("TERM", "ctx", 1);
      setenv ("CTX_VERSION", "0", 1);
      system (command);
      exit (0);
    }
  else if (vt->vtpty.pid < 0)
    {
      //VT_error ("forkpty failed (%s)", command);
    }
  //fcntl(vt->vtpty.pty, F_SETFL, O_NONBLOCK);
}

static void ct_rev_inc (void *data)
{
   CT *ct = data;
   ct->rev++;
}

static int ct_set_prop (CT *ct, uint32_t key, const char *val, int len);
static int ct_get_prop (CT *ct, const char *key, const char **val, int *len);

CT *ct_new (const char *command, int width, int height, int id, void *pixels)
{
  CT *ct = calloc (sizeof (CT), 1);
  ct->id = id;
  ct->lastx = -1;
  ct->lasty = -1;
#if 0
  ct->waitdata      = vtpty_waitdata;
  ct->read          = vtpty_read;
  ct->write         = vtpty_write;
  ct->resize        = vtpty_resize;
#endif
  ct->done               = 0;
  ct->ctx = ctx_new ();
  ct->renderer.ct = ct;
  _ctx_set_transformation (ct->ctx, 0);
          
  ct->parser = ctx_parser_new (ct->ctx, width, height, width/40.0, height/20.0, 1, 1, (void*)ct_set_prop, (void*)ct_get_prop, ct, ct_rev_inc, ct);
  ctx_clear (ct->ctx);

  if (command)
    {
      ct_run_command ((void*)ct, command, width, height);
    }
  //if (cols <= 0) { cols = DEFAULT_COLS; }
  //if (rows <= 0) { cols = DEFAULT_ROWS; }
  //vt_set_term_size (ct, cols, rows);
  //ct->width = width;
  //ct->height = width;
  ctx_list_prepend (&vts, ct);
  return ct;
}
pid_t       ct_get_pid (CT *ct)
{
  return ct->vtpty.pid;
}
void ct_feed_keystring (CT *ct, const char *str)
{
  vtpty_write ((void*)ct, str, strlen (str));
  vtpty_write ((void*)ct, "\n", 1);
}

void ct_destroy (CT *ct)
{
  ctx_list_remove (&vts, ct);
  kill (ct->vtpty.pid, 9);
  close (ct->vtpty.pty);
  free (ct);
}

#include "vt.h"

struct
_CtxClient {
  VT *vt;
  CT *ct;
  SDL_Texture  *texture;
  uint8_t      *pixels;
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
float font_size    = 32.0;
float line_spacing = 2.0;

static char *execute_self = NULL;

static int pointer_down[3] = {0,};
static int lctrl = 0;
static int lalt = 0;
static int rctrl = 0;

#define CTX_MAX_CLIENTS 16
static CtxClient clients[CTX_MAX_CLIENTS]={{NULL,},};
static int client_count = 0;
static int active = 0; // active client

static SDL_Window   *window;
static SDL_Renderer *renderer;

static CtxClient *client_by_id (int id);

int client_resize (int id, int width, int height);

static void client_unmaximize (CtxClient *client)
{
  if (!client->maximized)
    return;
  client->x = client->unmaximized_x;
  client->y = client->unmaximized_y;
  client_resize (client->id, client->unmaximized_width, client->unmaximized_height);
}
static void client_maximize (CtxClient *client)
{
  int w, h;
  if (client->maximized)
    return;

  client->unmaximized_x = client->x;
  client->unmaximized_y = client->y;
  client->unmaximized_width = client->width;
  client->unmaximized_height = client->height;
  client->maximized = 1;
  SDL_GetWindowSize (window, &w, &h);
  client_resize (client->id, w, h);
  client->x = 0;
  client->y = 0;
}
int id_to_no (int id);

static void client_raise (CtxClient *client)
{
  CtxClient temp;
  int no = id_to_no (client->id);
  if (no >= client_count-2)
    return;
  temp = clients[no];
  clients[no] = clients[no+1];
  clients[no+1] = temp;
}

static void client_lower (CtxClient *client)
{
  CtxClient temp;
  int no = id_to_no (client->id);
  if (no < 1)
    return;
  temp = clients[no];
  clients[no] = clients[no-1];
  clients[no-1] = temp;
}
static void client_raise_top (CtxClient *client)
{
  int no = id_to_no (client->id);
  for (int i = no; i < client_count; i++)
    client_raise (client);
}
static void client_lower_bottom (CtxClient *client)
{
  int no = id_to_no (client->id);
  for (int i = 0; i < no; i ++)
    client_lower (client);
}
void client_set_title (int id, const char *new_title);

static int ct_get_prop (CT *ct, const char *key, const char **val, int *len)
{
  uint32_t key_hash = ctx_strhash (key, 0);
  char str[4096]="";
  fprintf (stderr, "%s: %s %i\n", __FUNCTION__, key, key_hash);
  CtxClient *client = client_by_id (ct->id);
  if (!client)
    return 0;
  switch (key_hash)
  {
    case CTX_title:
      sprintf (str, "setkey %s %s\n", key, client->title);
      break;
    case CTX_x:      
      sprintf (str, "setkey %s %i\n", key, client->x);
      break;
    case CTX_y:    
      sprintf (str, "setkey %s %i\n", key, client->y);
      break;
    case CTX_width:
      sprintf (str, "setkey %s %i\n", key, client->width);
      break;
    case CTX_height:
      sprintf (str, "setkey %s %i\n", key, client->width);
      break;
    default:
      sprintf (str, "setkey %s undefined\n", key);
      break;
  }
  if (str[0])
  {
    vtpty_write ((void*)ct, str, strlen (str));
    fprintf (stderr, "%s", str);
  }
  return 0;
}

static int ct_set_prop (CT *ct, uint32_t key_hash, const char *val, int len)
{
  float fval = strtod (val, NULL);
  CtxClient *client = client_by_id (ct->id);
  uint32_t val_hash = ctx_strhash (val, 0);
  if (!client)
    return 0;
  fprintf (stderr, "%i %s\n", key_hash, val);

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
  return 1;
}

void sdl_setup (int width, int height)
{
  window = SDL_CreateWindow ("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  //renderer = SDL_CreateRenderer (window, -1, 0);
  renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_SOFTWARE);
  SDL_StartTextInput ();

  SDL_EnableScreenSaver ();
}

void add_client (const char *commandline, int x, int y, int width, int height, int ctx)
{
  static int global_id = 0;
  CtxClient *client = &clients[client_count++];
  client->id = global_id++;
  client->x = x;
  client->y = y;

  client->width = width;
  client->height = height;
  client->texture = SDL_CreateTexture (renderer,
                                   SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   width, height);
  client->pixels = calloc (width * height, 4);
  if (ctx)
  {
    client->ct = ct_new (commandline, width, height, client->id, client->pixels);
  }
  else
  {
    client->vt = vt_new (commandline, width/font_size*2, height/font_size, font_size, line_spacing, client->id);
  }
}

void add_client_argv (const char **argv, int x, int y, int width, int height, int ctx)
{
  VtString *string = vt_string_new ("");
  for (int i = 0; argv[i]; i++)
  {
    for (int c = 0; argv[i][c]; c++)
    {
       switch (argv[i][c])
       {
         case '"':vt_string_append_str (string, "\\\"");break;
         case '\'':vt_string_append_str (string, "\\\'");break;
         default:vt_string_append_byte (string, argv[i][c]);break;
       }
    }
    vt_string_append_byte (string, ' ');
  }
  add_client (string->str, x, y, width, height, ctx);
  vt_string_free (string, 1);
}

extern float ctx_shape_cache_rate;

static int last_x = 0;
static int last_y = 0;
static int find_active (int x, int y)
{
  int ret = 0;
  last_x = x;
  last_y = y;
  for (int i = 0; i < client_count; i++)
  {
     if (x > clients[i].x && x < clients[i].x+clients[i].width &&
         y > clients[i].y && y < clients[i].y+clients[i].height)
             ret = i;
  }
  return ret;
}

int id_to_no (int id)
{
  for (int i = 0 ;i < client_count; i++)
          if (clients[i].id == id)
                  return i;
  return -1;
}

static CtxClient *client_by_id (int id)
{
  int no = id_to_no (id);
  if (no < 0) return NULL;
  return &clients[no];
}

void client_remove (int no)
{
  if (clients[no].vt)
    vt_destroy (clients[no].vt);
  if (clients[no].ct)
    ct_destroy (clients[no].ct);

  SDL_DestroyTexture (clients[no].texture);
  free(clients[no].pixels);

  clients[no]=clients[client_count-1];
  client_count--;
  if (client_count == 0)
    do_quit = 1;
  find_active (last_x, last_y);
}

void client_remove_by_id (int id)
{
  int no = id_to_no (id);
  if (no>=0)
    client_remove (no);
}

void client_set_title (int id, const char *new_title)
{
  int no = id_to_no (id);
  if (no < 0) return;
  if (clients[no].title) free (clients[no].title);
  clients[no].title = strdup (new_title);
  if (client_count == 1)
    SDL_SetWindowTitle (window, new_title);
}

static void handle_event (const char *event)
{
  VT *vt = clients[active].vt;
  CT *ct = clients[active].ct;
  if (!strcmp (event, "shift-return"))
   event = "return";
  else
  if (!strcmp (event, "shift-control-v") )
    {
      char *text = SDL_GetClipboardText ();
      if (text)
        {
          if (vt)
            vt_paste (vt, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-c") && vt)
    {
      char *text = vt_get_selection (vt);
      if (text)
        {
          SDL_SetClipboardText (text);
          free (text);
        }
    }

  else if (!strcmp (event, "shift-control-n") )
    {
      pid_t pid;
      if ( (pid=fork() ) ==0)
        {
          for (int i = 0; i<700; i++)
            { close (i); }
          execlp (execute_self, execute_self, NULL);
          exit (0);
        }
    }
  else if (!strcmp (event, "shift-control-q") )
    {
      do_quit = 1; // global?
    }
  else if (!strcmp (event, "shift-control-w") )
    {
      clients[active].do_quit = 1;
    }
  else
    {
      if (vt)
        vt_feed_keystring (vt, event);
      if (ct)
        ct_feed_keystring (ct, event);
    }
}

static int key_balance = 0;
static int key_repeat = 0;

void client_move (int id, int x, int y)
{
   int no = id_to_no (id);
   if (no < 0) return;
   clients[no].x = x;
   clients[no].y = y;
}

int client_resize (int id, int width, int height)
{
   int no = id_to_no (id);
   if (no < 0) return 0;

   if ( (height != clients[no].height) || (width != clients[no].width) )
   {
     SDL_DestroyTexture (clients[no].texture);
     clients[no].texture = SDL_CreateTexture (renderer,
                                  SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  width, height);
     free (clients[no].pixels);
     clients[no].pixels = calloc (width * height, 4);
     clients[no].width = width;
     clients[no].height = height;
     if (clients[no].vt)
     {
       vt_set_term_size (clients[no].vt, width / vt_cw (clients[no].vt), height / vt_ch (clients[no].vt) );
     }
     else
     {
       char str[128];
       ctx_parser_set_size (clients[no].ct->parser, width, height, -1, -1);
       sprintf (str, "resized %i %i\n", width, height);
       vtpty_write ((void*)clients[no].ct, str, strlen (str));
     }

     return 1;
   }
   return 0;
}

static int sdl_check_events ()
{
  int got_event = 0;
  SDL_Event      event;
  CtxClient *client = &clients[active];
  VT *vt = clients[active].vt;
  CT *ct = clients[active].ct;

  int last_motion_x = -1;
  int last_motion_y = -1;
  static long last_event_tick = 0;

  event.type = 0;
    while (SDL_PollEvent (&event) )
      {
        char buf[64];
        switch (event.type)
          {
            case SDL_MOUSEWHEEL:
              if (vt)
                vt_set_scroll (vt, vt_get_scroll (vt) + event.wheel.y);
              else if (ct) {};
              break;
            case SDL_WINDOWEVENT:
              if (client_count == 1)
              {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                  {
                    int width  = event.window.data1;
                    int height = event.window.data2;
                    got_event = 1;
                    if (client_resize (clients[0].id, width, height))
                        return 1;
                  }
              }
              break;
            case SDL_MOUSEMOTION:
              {
                last_motion_x = event.motion.x;
                last_motion_y = event.motion.y;
                if (pointer_down[0] == 0)
                  usleep (20000);
                else
                  usleep (8000);
                //usleep (15000);
                //usleep (5000);
                got_event = 1;
              }
              break;
            case SDL_MOUSEBUTTONDOWN:
              {
                last_motion_x = -1;
                active = find_active (event.motion.x, event.motion.y);
                sprintf (buf, "mouse-press %.0f %.0f",
                         (float) event.button.x - client->x,
                         (float) event.button.y - client->y);
                handle_event (buf);
                got_event = 1;
                pointer_down[0] = 1;
              }
              break;
            case SDL_MOUSEBUTTONUP:
              {
                last_motion_x = -1;
                active = find_active (event.motion.x, event.motion.y);
                sprintf (buf, "mouse-release %.0f %.0f",
                         (float) event.button.x - client->x,
                         (float) event.button.y - client->y);
                handle_event (buf);
                got_event = 1;
                pointer_down[0] = 0;
              }
              break;
            case SDL_TEXTINPUT:
              if (!lctrl && !rctrl && !lalt &&
                  ( (vt && vt_keyrepeat (vt) ) || (key_repeat==0) )
                 )
                {
                  const char *name = event.text.text;
                  if (!strcmp (name, " ") ) { name = "space"; }
                  handle_event (name);
                  got_event = 1;
                }
              break;
            case SDL_KEYUP:
              {
                key_balance--;
                switch (event.key.keysym.sym)
                  {
                    case SDLK_LCTRL:
                      lctrl=0;
                      break;
                    case SDLK_LALT:
                      lalt =0;
                      break;
                    case SDLK_RCTRL:
                      rctrl=0;
                      break;
                  }
              }
              break;
            case SDL_KEYDOWN:
              {
                char buf[32] = "";
                char *name = buf;
                if (!event.key.repeat)
                  {
                    key_balance++;
                    key_repeat = 0;
                  }
                else
                  {
                    key_repeat++;
                  }
                buf[ctx_unichar_to_utf8 (event.key.keysym.sym, (void *) buf)]=0;
                switch (event.key.keysym.sym)
                  {
                    case SDLK_LCTRL: lctrl=1; break;
                    case SDLK_LALT: lalt=1; break;
                    case SDLK_RCTRL: rctrl=1; break;
                    case SDLK_F1: name = "F1"; break;
                    case SDLK_F2: name = "F2"; break;
                    case SDLK_F3: name = "F3"; break;
                    case SDLK_F4: name = "F4"; break;
                    case SDLK_F5: name = "F5"; break;
                    case SDLK_F6: name = "F6"; break;
                    case SDLK_F7: name = "F7"; break;
                    case SDLK_F8: name = "F8"; break;
                    case SDLK_F9: name = "F9"; break;
                    case SDLK_F10: name = "F10"; break;
                    case SDLK_F11: name = "F11"; break;
                    case SDLK_F12: name = "F12"; break;
                    case SDLK_ESCAPE: name = "escape"; break;
                    case SDLK_DOWN: name = "down"; break;
                    case SDLK_LEFT: name = "left"; break;
                    case SDLK_UP: name = "up"; break;
                    case SDLK_RIGHT: name = "right"; break;
                    case SDLK_BACKSPACE: name = "backspace"; break;
                    case SDLK_SPACE: name = "space"; break;
                    case SDLK_TAB: name = "tab"; break;
                    case SDLK_DELETE: name = "delete"; break;
                    case SDLK_INSERT: name = "insert"; break;
                    case SDLK_RETURN:
                      //if (key_repeat == 0) // return never should repeat
                      name = "return";   // on a DEC like terminal
                      break;
                    case SDLK_HOME: name = "home"; break;
                    case SDLK_END: name = "end"; break;
                    case SDLK_PAGEDOWN: name = "page-down"; break;
                    case SDLK_PAGEUP: name = "page-up"; break;
                    default:
                      ;
                  }
                if (strlen (name) )
                  {
                    if (event.key.keysym.mod & (KMOD_CTRL) ||
                        event.key.keysym.mod & (KMOD_ALT) ||
                        strlen (name) >= 2)
                      {
                        if (event.key.keysym.mod & (KMOD_CTRL) )
                          {
                            static char buf[64] = "";
                            sprintf (buf, "control-%s", name);
                            name = buf;
                          }
                        if (event.key.keysym.mod & (KMOD_ALT) )
                          {
                            static char buf[128] = "";
                            sprintf (buf, "alt-%s", name);
                            name = buf;
                          }
                        if (event.key.keysym.mod & (KMOD_SHIFT) )
                          {
                            static char buf[196] = "";
                            sprintf (buf, "shift-%s", name);
                            name = buf;
                          }
                        if (strcmp (name, "space") &&
                            ( (vt && vt_keyrepeat (vt) ) || (key_repeat==0) )
                           )
                          {
                            handle_event (name);
                            got_event = 1;
                          }
                      }
                  }
              }
              break;
          }
      }

  if (last_motion_x >=0)
  {static char buf[127];
    last_motion_x = event.motion.x;
    last_motion_y = event.motion.y;
    active = find_active (last_motion_x, last_motion_y);
    if (pointer_down[0])
      sprintf (buf, "mouse-drag %.0f %.0f",
               (float) last_motion_x - client->x,
               (float) last_motion_y - client->y);
    else
      sprintf (buf, "mouse-motion %.0f %.0f",
               (float) last_motion_x - client->x,
               (float) last_motion_y - client->y);
    handle_event (buf);
    last_motion_x = -1;
  }
  else
  {
  }


  if (ctx_ticks () - last_event_tick >  1000000)
  { static char *idle = "idle";
    handle_event (idle);
    last_motion_x = -1;
    got_event = 1;
  }

  if (got_event)
    last_event_tick = ctx_ticks ();

  return got_event;
}

int update_vt (CtxClient *client)
{
        VT *vt = client->vt;
        int width = client->width;
        int height = client->height;
        //int vt_x = client->x;
        //int vt_y = client->y;
        int in_scroll = (vt_has_blink (client->vt) >= 10);

       
        if (vt_is_done (vt) )
        {
          client_remove_by_id (client->id);
          return -1;
        }

      if ( (client->drawn_rev != vt_rev (vt) ) ||
           vt_has_blink (vt) ||
           in_scroll)
        {
          client->drawn_rev = vt_rev (vt);
          SDL_Rect dirty;
#if 0
          // XXX this works for initial line in started shell
          // but falls apart when bottom is reached,
          // needs investigation, this is the code path that
          // can be turned into threaded rendering.
          Ctx *ctx = ctx_new ();
          vt_draw (vt, ctx, 0, 0);
          ctx_blit (ctx, client->pixels, 0,0, width, height, width * 4, CTX_FORMAT_BGRA8);
#else
          // render directlty to framebuffer in immediate mode - skips
          // creation of renderstream.
          //
          // terminal is also keeping track of state of already drawn
          // pixels and often only repaints what is needed XXX  need API
          //                                              to force full draw
          Ctx *ctx = ctx_new_for_framebuffer (client->pixels, width, height, width * 4, CTX_FORMAT_BGRA8);
          //fprintf (stderr, "%i\r", no);
          vt_draw (vt, ctx, 0, 0);
#endif
          ctx_dirty_rect (ctx, &dirty.x, &dirty.y, &dirty.w, &dirty.h);
          ctx_free (ctx);

#if 1 // < flipping this turns on subtexture updates, needs bounds tuning
          dirty.w ++;
          dirty.h ++;
          if (dirty.x + dirty.w > width)
            { dirty.w = width - dirty.x; }
          if (dirty.y + dirty.h > height)
            { dirty.h = height - dirty.y; }
          SDL_UpdateTexture (client->texture,
                             &dirty,
                             (uint8_t *) client->pixels + sizeof (Uint32) * (width * dirty.y + dirty.x), width * sizeof (Uint32) );
#else
          SDL_UpdateTexture (client->texture, NULL,
                             (void *) client->pixels, width * sizeof (Uint32) );
#endif
          return 1;
        }
      return 0;
}


int ctx_count (Ctx *ctx);

int update_ct (CtxClient *client)
{
    CT *ct = client->ct;
    int width = client->width;
    int height = client->height;
    //int in_scroll;
   
    if (vt_is_done ((void*)ct) )
    {
      client_remove_by_id (client->id);
      return -1;
    }

  if ( (client->drawn_rev != ct->rev))
    {
      client->drawn_rev = ct->rev;
      Ctx *dctx = ctx_new_for_framebuffer (client->pixels, width, height, width * 4, CTX_FORMAT_BGRA8);

      //fprintf (stderr, "%i\n", ctx_count (ct->ctx));
      ctx_render_ctx (ct->ctx, dctx);
      ctx_clear (ct->ctx);

#if 1 // < flipping this turns on subtexture updates, needs bounds tuning
          SDL_Rect dirty;
          ctx_dirty_rect (dctx, &dirty.x, &dirty.y, &dirty.w, &dirty.h);
          dirty.w ++;
          dirty.h ++;
          if (dirty.x + dirty.w > width)
            { dirty.w = width - dirty.x; }
          if (dirty.y + dirty.h > height)
            { dirty.h = height - dirty.y; }
          SDL_UpdateTexture (client->texture,
                             &dirty,
                             (uint8_t *) client->pixels + sizeof (Uint32) * (width * dirty.y + dirty.x), width * sizeof (Uint32) );
#else
          SDL_UpdateTexture (client->texture, NULL,
                             (void *) client->pixels, width * sizeof (Uint32) );
#endif
          ctx_free (dctx);
          return 1;
        }
      return 0;
}

int vt_main (int argc, char **argv)
{
  int width = 80 * font_size/2;
  int height = 24 * font_size;
  execute_self = malloc (strlen (argv[0]) + 16);
  sprintf (execute_self, "%s", argv[0]);
  sdl_setup (width, height);

  if (argv[1] == NULL)
  {
    add_client (vt_find_shell_command(), 0, 0, width, height, 0);
  }
  else
  {
    add_client_argv ((void*)&argv[1], 0, 0, width, height, 1);
  }

  //add_client ("/home/pippin/src/ctx/examples/bash.sh", 0, height/2, width/2, height/2, 1);
  //add_client ("/home/pippin/src/ctx/examples/ui", width/2, height/2, width/2, height/2, 1);
  signal (SIGCHLD,signal_child);

  int sleep_time = 10;
  while (!do_quit)
    {
      int changes = 0;
      for (int no = 0; no < client_count; no++)
      {
          if (clients[no].vt)
          {
            if (update_vt (&clients[no]))
              changes++;
          }
          else
          {
            if (update_ct (&clients[no]))
              changes++;
          }
      }

      if (changes)
      {
      SDL_RenderClear (renderer);
      for (int no = 0; no < client_count; no++)
      {
          SDL_Rect SrcR;
          SDL_Rect DestR;
          SrcR.x = 0;
          SrcR.y = 0;
          SrcR.w = clients[no].width;
          SrcR.h = clients[no].height;

          DestR.x = clients[no].x;
          DestR.y = clients[no].y;
          DestR.w = clients[no].width;
          DestR.h = clients[no].height;

          SDL_RenderCopy (renderer, clients[no].texture, &SrcR, &DestR);
        }

        SDL_RenderPresent (renderer);
      }
      else
      {
         usleep (5000);
      }

      int got_event = sdl_check_events ();

      if (!got_event)
        {
          sleep_time = 200;
        }
#if 0
      if (in_scroll)
        {
          sleep_time = 200;
        }
#endif

      for (int a = 0; a < client_count; a++)
      {
        if (clients[a].vt)
        {
          if (vt_poll (clients[a].vt, sleep_time/client_count) )
          {
            if (sleep_time > 2500)
              { sleep_time = 2500; }
          }
          else
          {
            sleep_time *= 1.5;
          }
        }
        else
        {
          if (ct_poll (clients[a].ct, sleep_time/client_count) )
          {
            if (sleep_time > 2500)
              { sleep_time = 2500; }
          }
          else
          {
            sleep_time *= 1.5;
          }
        }
      }

      if (sleep_time > 60000)
        { sleep_time = 60000; }
    }
  while (client_count)
    client_remove (client_count);
  return 0;
}
