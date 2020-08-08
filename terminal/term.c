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

#include <SDL.h>

#include "ctx-font-regular.h"
#include "ctx-font-mono.h"
#define CTX_EVENTS            1
#define CTX_SHAPE_CACHE       1
#include <immintrin.h> // is detected by ctx, and enables AVX2

#define CTX_MAX_JOURNAL_SIZE     1024*64

#define CTX_IMPLEMENTATION
#include "ctx.h"
#include "vt-line.h"
#include "terminal.h"
#include "vt.h"

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


typedef struct _CtxClient CtxClient;

CtxList *vts = NULL;
//SDL_mutex *clients_mutex = NULL;

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
float font_size    = 32.0;
float line_spacing = 2.0;

static char *execute_self = NULL;

static CtxList *clients;//CtxClient clients[CTX_MAX_CLIENTS]={{NULL,},};
static CtxClient *active = NULL;

static CtxClient *client_by_id (int id);

int client_resize (int id, int width, int height);

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
  client->vt = vt_new (commandline, width/font_size*2, height/font_size, font_size, line_spacing, client->id);
  return client;
}

CtxClient *add_client_argv (const char **argv, int x, int y, int width, int height, int ctx)
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
  CtxClient *ret = add_client (string->str, x, y, width, height, ctx);
  vt_string_free (string, 1);
  return ret;
}

extern float ctx_shape_cache_rate;

static int last_x = 0;
static int last_y = 0;
static CtxClient *find_active (int x, int y)
{
  CtxClient *ret = 0;
  last_x = x;
  last_y = y;
  for (CtxList *l = clients; l; l = l->next)
  {
     CtxClient *c = l->data;
     if (x > c->x && x < c->x+c->width &&
         y > c->y && y < c->y+c->height)
       ret = c;
  }
  active = ret;
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
  //SDL_LockMutex (clients_mutex);
  if (client->vt)
    vt_destroy (client->vt);

  if (client->title)
    free (client->title);

  ctx_list_remove (&clients, client);
  if (clients == NULL)
    do_quit = 1;

  if (client == active)
  {
    active = find_active (last_x, last_y);
  }
  free (client);
  //SDL_UnlockMutex (clients_mutex);
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
          execlp (execute_self, execute_self, NULL, NULL);
          exit (0);
        }
      fprintf (stderr, "!\n");
    }
  else if (!strcmp (event, "shift-control-q") )
    {
      do_quit = 1; // global?
    }
  else if (!strcmp (event, "shift-control-w") )
    {
      active->do_quit = 1;
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
     //SDL_LockMutex (clients_mutex);
     client->width = width;
     client->height = height;
     vt_set_term_size (client->vt, width / vt_cw (client->vt), height / vt_ch (client->vt) );

     //SDL_UnlockMutex (clients_mutex);
     return 1;
   }
   return 0;
}

int update_vt (Ctx *ctx, CtxClient *client)
{
      VT *vt = client->vt;
      //int width = client->width;
      //int height = client->height;
      //int vt_x = client->x;
      //int vt_y = client->y;
      int in_scroll = (vt_has_blink (client->vt) >= 10);

      if ( (client->drawn_rev != vt_rev (vt) ) ||
           vt_has_blink (vt) ||
           in_scroll)
        {
          client->drawn_rev = vt_rev (vt);
          vt_draw (vt, ctx, 0, 0);
          return 1;
        }
      return 0;
}


int ctx_count (Ctx *ctx);

static int dirt = 0;

static int update_vts (Ctx *ctx)
{
  int changes = 0;
  for (CtxList *l = clients; l; l = l->next)
  {
    CtxClient *client = l->data;
      if (client->vt && update_vt (ctx, client))
          changes++;
  }
  dirt += changes;
  return changes;
}

#if 0
void render_fun (void *data)
{
  while(!do_quit)
  {
    SDL_LockMutex (clients_mutex);
    int changes = update_cts ();
    SDL_UnlockMutex (clients_mutex);
    if (!changes) usleep (20000);
  }
}
#endif

int main (int argc, char **argv)
{
  ctx_init (&argc, &argv);
  int width = 1280;
  int height = 768;
  Ctx *ctx = ctx_new_ui (width, height);
  width = ctx_width (ctx);
  height = ctx_height (ctx);
  if (argv[1] == NULL)
  {
    active = add_client (vt_find_shell_command(), 0, 0, width, height, 0);
  }
  else
  {
    active = add_client_argv ((void*)&argv[1], 0, 0, width, height, 0);
  }
  signal (SIGCHLD,signal_child);

  int sleep_time = 200;
  while (clients)
    {
      CtxList *to_remove = NULL;
      int changes = 0;

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

      changes += update_vts (ctx);
      if (changes)
      {
        ctx_reset (ctx);
        ctx_flush (ctx);
      }
      else
      {
        usleep (1000 *  15);
      }

      CtxEvent *event;
      static int mouse_down = 0;
      while ((event = ctx_get_event (ctx)))
      {
        char buf[64];
        switch (event->type)
        {
          case CTX_RELEASE:
            if (event->device_no == 0)
                    mouse_down = 0;
            sprintf (buf, "mouse-release %.0f %.0f", (float) event->x,
                                                     (float) event->y);
            handle_event (buf);
            break;
          case CTX_PRESS:
            if (event->device_no == 0)
                    mouse_down = 1;
            sprintf (buf, "mouse-press %.0f %.0f", (float) event->x,
                                                  (float) event->y);
            handle_event (buf);
            break;
          case CTX_MOTION:
#if 1
            if (mouse_down)
            {
              sprintf (buf, "mouse-drag %.0f %.0f", (float) event->x,
                                                    (float) event->y);
            }
            else
#endif
            {
              sprintf (buf, "mouse-motion %.0f %.0f", (float) event->x,
                                                      (float) event->y);
            }
            handle_event (buf);
            break;

          case CTX_KEY_DOWN:
            if (!strcmp (event->string, "resize-event"))
            {
               if (active)
                 client_resize (active->id, ctx_width (ctx), ctx_height (ctx));
            }
            else
            {
              handle_event (event->string);
            }
            break;

          default:
            break;
        }
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
  return 0;
}
