#define _DEFAULT_SOURCE

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ENABLE_CLICK 0

#include <SDL.h>
#include "ctx.h"
#include "vt-line.h"
#include "vt.h"

typedef struct
CtxClient {
  VT *vt;
  SDL_Texture  *texture;
  uint8_t      *pixels;
  char         *title;
  int           x;
  int           y;
  int           vt_width;
  int           vt_height;
  int           do_quit;
  long          drawn_rev;
  int           id;
}CtxClient;

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
static int client_pos = 0;
static int active = 0; // active client

static SDL_Window   *window;
static SDL_Renderer *renderer;

void sdl_setup (int width, int height)
{
  window = SDL_CreateWindow ("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  //renderer = SDL_CreateRenderer (window, -1, 0);
  renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_SOFTWARE);
  SDL_StartTextInput ();

  SDL_EnableScreenSaver ();
}

void add_client (const char *commandline, int x, int y, int width, int height)
{
  static int global_id = 0;
  clients[client_pos].id = global_id++;
  clients[client_pos].x = x;
  clients[client_pos].y = y;

  clients[client_pos].vt_width = width;//( (int) (font_size/line_spacing + 0.999) ) * DEFAULT_COLS;
  clients[client_pos].vt_height = height;//font_size * DEFAULT_ROWS;
  clients[client_pos].vt = vt_new (commandline, DEFAULT_COLS, DEFAULT_ROWS, font_size, line_spacing, clients[client_pos].id);
  clients[client_pos].texture = SDL_CreateTexture (renderer,
                                   SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   width, height);
  clients[client_pos].pixels = calloc (width * height, 4);
  client_pos++;
}


extern float ctx_shape_cache_rate;

static int last_x = 0;
static int last_y = 0;
static int find_active (int x, int y)
{
  int ret = 0;
  last_x = x;
  last_y = y;
  for (int i = 0; i < client_pos; i++)
  {
     if (x > clients[i].x && x < clients[i].x+clients[i].vt_width &&
         y > clients[i].y && y < clients[i].y+clients[i].vt_height)
             ret = i;
  }
  return ret;
}

int id_to_no (int id)
{
  for (int i = 0 ;i < client_pos; i++)
          if (clients[i].id == id)
                  return i;
  return -1;
}

void remove_client (int no)
{
  vt_destroy (clients[no].vt);
  SDL_DestroyTexture (clients[no].texture);
  free(clients[no].pixels);

  clients[no]=clients[client_pos-1];
  client_pos--;
  if (client_pos == 0)
    do_quit = 1;
  find_active (last_x, last_y);
}

void remove_client_by_id (int id)
{
  int no = id_to_no (id);
  if (no>=0)
    remove_client (no);
}

void terminal_set_title (int id, const char *new_title)
{
  int no = id_to_no (id);
  if (no < 0) return;
  if (clients[no].title) free (clients[no].title);
  clients[no].title = strdup (new_title);
  if (client_pos == 1)
    SDL_SetWindowTitle (window, new_title);
}

static void handle_event (const char *event)
{
  VT *vt = clients[active].vt;
  if (!strcmp (event, "shift-return"))
   event = "return";
  else
  if (!strcmp (event, "shift-control-v") )
    {
      char *text = SDL_GetClipboardText ();
      if (text)
        {
          vt_paste (vt, text);
          free (text);
        }
    }
  else if (!strcmp (event, "shift-control-c") )
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
      vt_feed_keystring (vt, event);
      // make optional? - reset of scroll on key input
      vt_set_scroll (vt, 0);
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
   if ( (height != clients[no].vt_height) || (width != clients[no].vt_width) )
   {
     SDL_DestroyTexture (clients[no].texture);
     clients[no].texture = SDL_CreateTexture (renderer,
                                  SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  width, height);
     free (clients[no].pixels);
     clients[no].pixels = calloc (width * height, 4);
     clients[no].vt_width = width;
     clients[no].vt_height = height;
     vt_set_term_size (clients[no].vt, width / vt_cw (clients[no].vt), height / vt_ch (clients[no].vt) );
     return 1;
   }
   return 0;
}

static int sdl_check_events ()
{
  int got_event = 0;
  static SDL_Event      event;
  VT *vt = clients[active].vt;
  if (SDL_WaitEventTimeout (&event, 15) )
    do
      {
        char buf[64];
        switch (event.type)
          {
            case SDL_MOUSEWHEEL:
              vt_set_scroll (vt, vt_get_scroll (vt) + event.wheel.y);
              break;
            case SDL_WINDOWEVENT:
              if (client_pos == 1)
              {
#if 1
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                  {
                    int width  = event.window.data1;
                    int height = event.window.data2;
                    //        host->stride = host->width * host->bpp;
                    got_event = 1;
                    if (client_resize (clients[0].id, width, height))
                        return 1;
                  }
#endif
              }
              break;
            case SDL_MOUSEMOTION:
              {
                active = find_active (event.motion.x, event.motion.y);
                if (pointer_down[0])
                  sprintf (buf, "mouse-drag %.0f %.0f",
                           (float) event.motion.x,
                           (float) event.motion.y);
                else
                  sprintf (buf, "mouse-motion %.0f %.0f",
                           (float) event.motion.x,
                           (float) event.motion.y);
                handle_event (buf);
                got_event = 1;
              }
              break;
            case SDL_MOUSEBUTTONDOWN:
              {
                active = find_active (event.motion.x, event.motion.y);
                sprintf (buf, "mouse-press %.0f %.0f",
                         (float) event.button.x,
                         (float) event.button.y);
                handle_event (buf);
                got_event = 1;
                pointer_down[0] = 1;
              }
              break;
            case SDL_MOUSEBUTTONUP:
              {
                active = find_active (event.motion.x, event.motion.y);
                sprintf (buf, "mouse-release %.0f %.0f",
                         (float) event.button.x,
                         (float) event.button.y);
                handle_event (buf);
                got_event = 1;
                pointer_down[0] = 0;
              }
              break;
            case SDL_TEXTINPUT:
              if (!lctrl && !rctrl && !lalt &&
                  ( (vt_keyrepeat (vt) ) || (key_repeat==0) )
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
                            ( (vt_keyrepeat (vt) ) || (key_repeat==0) )
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
    while (SDL_PollEvent (&event) );
  return got_event;
}

int vt_main (int argc, char **argv)
{
  static int width = 800;
  static int height = 600;
  execute_self = malloc (strlen (argv[0]) + 16);
  sprintf (execute_self, "%s", argv[0]);
  sdl_setup (width, height);
  add_client (argv[1]?argv[1]:vt_find_shell_command(), 0, 0, width, height);
  add_client ("/usr/bin/top", width/3, height/3, width/2, height/2);

  int sleep_time = 10;
  while (!do_quit)
    {
again:
      SDL_RenderClear (renderer);
      for (int no = 0; no < client_pos; no++)
      {
        VT *vt = clients[no].vt;
        int vt_width = clients[no].vt_width;
        int vt_height = clients[no].vt_height;
        //int vt_x = clients[no].x;
        //int vt_y = clients[no].y;
        int in_scroll = (vt_has_blink (clients[no].vt) >= 10);

        if (vt_is_done (vt) )
        {
          remove_client (no);
          goto again;
        }

      if ( (clients[no].drawn_rev != vt_rev (vt) ) ||
           vt_has_blink (vt) ||
           in_scroll)
        {
          clients[no].drawn_rev = vt_rev (vt);
          SDL_Rect dirty;
#if 0
          // XXX this works for initial line in started shell
          // but falls apart when bottom is reached,
          // needs investigation, this is the code path that
          // can be turned into threaded rendering.
          Ctx *ctx = ctx_new ();
          vt_draw (vt, ctx, 0, 0);
          ctx_blit (ctx, pixels, 0,0, vt_width, vt_height, vt_width * 4, CTX_FORMAT_BGRA8);
#else
          // render directlty to framebuffer in immediate mode - skips
          // creation of renderstream.
          //
          // terminal is also keeping track of state of already drawn
          // pixels and often only repaints what is needed XXX  need API
          //                                              to force full draw
          Ctx *ctx = ctx_new_for_framebuffer (clients[no].pixels, vt_width, vt_height, vt_width * 4, CTX_FORMAT_BGRA8);
          fprintf (stderr, "%i\r", no);
          vt_draw (vt, ctx, 0, 0);
#endif
          ctx_dirty_rect (ctx, &dirty.x, &dirty.y, &dirty.w, &dirty.h);
          ctx_free (ctx);

#if 0 // < flipping this turns on subtexture updates, needs bounds tuning
          dirty.w ++;
          dirty.h ++;
          if (dirty.x + dirty.w > vt_width)
            { dirty.w = vt_width - dirty.x; }
          if (dirty.y + dirty.h > vt_height)
            { dirty.h = vt_height - dirty.y; }
          SDL_UpdateTexture (texture,
                             &dirty,
                             (uint8_t *) pixels + sizeof (Uint32) * (vt_width * dirty.y + dirty.x), vt_width * sizeof (Uint32) );
#else
          SDL_UpdateTexture (clients[no].texture, NULL,
                             (void *) clients[no].pixels, vt_width * sizeof (Uint32) );
#endif
        }
          SDL_Rect SrcR;
          SDL_Rect DestR;
          SrcR.x = 0;
          SrcR.y = 0;
          SrcR.w = clients[no].vt_width;
          SrcR.h = clients[no].vt_height;

          DestR.x = clients[no].x;
          DestR.y = clients[no].y;
          DestR.w = clients[no].vt_width;
          DestR.h = clients[no].vt_height;

          SDL_RenderCopy (renderer, clients[no].texture, &SrcR, &DestR);
        }
          SDL_RenderPresent (renderer);
      int got_event = 0;
      //if (!in_scroll)
        got_event = sdl_check_events ();


      if (got_event)
        {
          sleep_time = 200;
        }
#if 0
      if (in_scroll)
        {
          sleep_time = 200;
        }
#endif

      for (int a = 0; a < client_pos; a++)
      {
        if (vt_poll (clients[a].vt, sleep_time/client_pos) )
        {
          if (sleep_time > 2500)
            { sleep_time = 2500; }
        }
        else
        {
          sleep_time *= 1.5;
        }
      }

      if (sleep_time > 60000)
        { sleep_time = 60000; }
    }
  while (client_pos)
    remove_client (client_pos);
  return 0;
}
