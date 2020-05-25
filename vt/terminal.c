#define _DEFAULT_SOURCE

#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define USE_SDL 1
#define USE_MMM 0
#define ENABLE_CLICK 0

#if USE_SDL
#include <SDL.h>
#endif

#if USE_MMM
#include "mmm.h"
#include "mmm-pset.h"
#endif

//#include "ctx-font-regular.h"
//#include "ctx-font-mono.h"
#include "DejaVuSansMono.h"
#include "DejaVuSans.h"
//#include "0xA000-Mono.h"
//#include "Vera.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define CTX_BACKEND_TEXT         1
#define CTX_PARSER               1
#define CTX_BITPACK_PACKER       0
#define CTX_GRADIENT_CACHE       1
#define CTX_SHAPE_CACHE          1
#define CTX_SHAPE_CACHE_MAX_DIM  48
#define CTX_SHAPE_CACHE_DIM      (48*48)
#define CTX_SHAPE_CACHE_ENTRIES  (512)
#define CTX_RASTERIZER_AA        5
#define CTX_RASTERIZER_FORCE_AA  1
#define CTX_IMPLEMENTATION
#include "ctx.h"
#include "vt-line.h"
#include "vt.h"

int   do_quit      = 0;
float font_size    = 32.0;
float line_spacing = 2.0;

static pid_t vt_child;
static VT *vt = NULL;

#if USE_MMM
static Mmm *mmm = NULL;
#elif USE_SDL
static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;
static uint8_t      *pixels;
static int pointer_down[3] = {0,};
static int lctrl = 0;
static int lalt = 0;
static int rctrl = 0;
#endif

static char *execute_self = NULL;

int vt_width;
int vt_height;

#if USE_SDL

void sdl_setup (int width, int height)
{
  window = SDL_CreateWindow("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  //renderer = SDL_CreateRenderer (window, -1, 0);
  renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_SOFTWARE);
  SDL_StartTextInput ();
  texture = SDL_CreateTexture (renderer,
	SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
  pixels = calloc (width * height, 4);
  SDL_EnableScreenSaver ();
}
#elif USE_MMM
void mmm_setup (int width, int height)
{
  mmm = mmm_new (vt_width, vt_height, 0, NULL);
  unsetenv ("MMM_PATH");
}
#endif

void terminal_set_title (const char *new_title)
{
#if USE_SDL
   SDL_SetWindowTitle (window, new_title);
#elif USE_MMM
   mmm_set_title (mmm, new_title);
#endif
}


long drawn_rev = 0;
extern float ctx_shape_cache_rate;

static void handle_event (const char *event)
{
	//if (!strcmp (event, "shift-return"))
	 // event = "return";
	//else
	if (!strcmp (event, "shift-page-up"))
	{
	  int new_scroll = vt_get_scroll (vt) + vt_get_rows (vt)/2;
	  vt_set_scroll (vt, new_scroll);
	  vt_rev_inc (vt);
	} else if (!strcmp (event, "shift-page-down"))
	{
	  int new_scroll = vt_get_scroll (vt) - vt_get_rows (vt)/2;
	  if (new_scroll < 0) new_scroll = 0;
	  vt_set_scroll (vt, new_scroll);
	  vt_rev_inc (vt);
	} else if (!strcmp (event, "shift-control-v")) {
#if USE_SDL
	  char *text = SDL_GetClipboardText ();
	  if (text)
	  {
            vt_paste (vt, text);
	    free (text);
	  }
#endif
	} else if (!strcmp (event, "shift-control-c")) {
#if USE_SDL
	  char *text = vt_get_selection (vt);
	  if (text)
	  {
	    SDL_SetClipboardText (text);
	    free (text);
	  }
#endif
	} else if (!strcmp (event, "shift-control-l")) {
	  vt_set_local (vt, !vt_get_local (vt));
	} else if (!strcmp (event, "shift-control--") ||
	           !strcmp (event, "control--")) {
	  font_size /= 1.15;
	  font_size = (int) (font_size);
	  if (font_size < 5) font_size = 5;

	  vt_set_font_size (vt, font_size);
          vt_set_term_size (vt, vt_width / vt_cw (vt), vt_height / vt_ch (vt));
	} else if (!strcmp (event, "shift-control-=") ||
	           !strcmp (event, "control-=")) {
	  float old = font_size;
	  font_size *= 1.15;
	  font_size = (int)(font_size);
	  if (old == font_size) font_size = old+1;
	  if (font_size > 200) font_size = 200;

	  vt_set_font_size (vt, font_size);
          vt_set_term_size (vt, vt_width / vt_cw (vt), vt_height / vt_ch (vt));
	} else if (!strcmp (event, "shift-control-n")) {
	  pid_t pid;
	  if ((pid=fork())==0)
	  {
	    for (int i = 0; i<700; i++)
              close(i);
	    execlp (execute_self, execute_self, NULL);
	    exit(0);
	  }
	} else if (!strcmp (event, "shift-control-r")) {
	  vt_open_log (vt, "/tmp/ctx-vt");
	}
        else if (!strcmp (event, "shift-control-q"))
        {
          do_quit = 1;
        }
        else if (!strcmp (event, "shift-control-w"))
        {
          do_quit = 1;
        }
        else if (!strncmp (event, "mouse-", 5))
	{
	  int cw = vt_cw (vt);
	  int ch = vt_ch (vt);
	  if (!strncmp (event + 6, "motion", 6))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        vt_mouse (vt, VT_MOUSE_MOTION, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
	  }
	  else if (!strncmp (event + 6, "press", 5))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        vt_mouse (vt, VT_MOUSE_PRESS, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
            drawn_rev = 0;
	  }
	  else if (!strncmp (event + 6, "drag", 4))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        vt_mouse (vt, VT_MOUSE_DRAG, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
            drawn_rev = 0;
	  }
	  else if (!strncmp (event + 6, "release", 7))
	  {
            int x = 0, y = 0;
	    char *s = strchr (event, ' ');
	    if (s)
	    {
	      x = atoi (s);
	      s = strchr (s + 1, ' ');
	      if (s)
	      {
	        y = atoi (s);
	        vt_mouse (vt, VT_MOUSE_RELEASE, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
            drawn_rev = 0;
	  }
	}
        else
        {
          vt_feed_keystring (vt, event);
	  // make optional?
	  vt_set_scroll (vt, 0);
        }
}


#if USE_MMM
int mmm_check_events (Mmm *mmm)
{
	int got_event = 0;
      while (mmm_has_event (mmm))
      {
        const char *event = mmm_get_event (mmm);
	handle_event (event);
	got_event = 1;
      }
      return got_event;
}
#elif USE_SDL
static int key_balance = 0;
static int key_repeat = 0;

static int sdl_check_events ()
{
  int got_event = 0;
  static SDL_Event      event;
  if (SDL_WaitEventTimeout (&event, 15))
  do
  {
    char buf[64];
    switch (event.type)
    {
      case SDL_MOUSEWHEEL:
	 vt_set_scroll (vt, vt_get_scroll(vt) + event.wheel.y);
	 break;
      case SDL_WINDOWEVENT: 
        { 
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) 
          { 
              int width  = event.window.data1; 
              int height = event.window.data2; 
    //        host->stride = host->width * host->bpp; 
	      got_event = 1;

	      if ((height != vt_height) || (width != vt_width))
	      {
  SDL_DestroyTexture (texture);
  texture = SDL_CreateTexture (renderer,
	SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
  free (pixels);
  pixels = calloc (width * height, 4);
  vt_width = width;
  vt_height = height;
          vt_set_term_size (vt, width / vt_cw (vt), height / vt_ch (vt));
	  return 1;
	      }
          }
	}
	break;
      case SDL_MOUSEMOTION:
        {
          if (pointer_down[0])
            sprintf (buf, "mouse-drag %.0f %.0f",
                 (float)event.motion.x,
                 (float)event.motion.y);
          else
            sprintf (buf, "mouse-motion %.0f %.0f",
                 (float)event.motion.x,
                 (float)event.motion.y);

          handle_event (buf);
	  got_event = 1;
        }
        break;
   case SDL_MOUSEBUTTONDOWN:
        {
          sprintf (buf, "mouse-press %.0f %.0f",
               (float)event.button.x,
               (float)event.button.y);
          handle_event (buf);
	  got_event = 1;
          pointer_down[0] = 1;
        }
        break;
     case SDL_MOUSEBUTTONUP:
        {
          sprintf (buf, "mouse-release %.0f %.0f",
               (float)event.button.x,
               (float)event.button.y);

          handle_event (buf);
	  got_event = 1;
          pointer_down[0] = 0;
        }
        break;
      case SDL_TEXTINPUT:
        if (!lctrl && !rctrl && !lalt &&
		((vt_keyrepeat (vt)) || (key_repeat==0))
              ) {
          const char *name = event.text.text;
          if (!strcmp (name, " ")) name = "space";

          handle_event (name);
	  got_event = 1;
        }
        break;
     case SDL_KEYUP:
        {
	  key_balance--;
          switch (event.key.keysym.sym)
          {
            case SDLK_LCTRL:     lctrl=0;      break;
            case SDLK_LALT:      lalt =0;      break;
            case SDLK_RCTRL:     rctrl=0;      break;
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

          buf[ctx_unichar_to_utf8 (event.key.keysym.sym, (void*)buf)]=0;

          switch (event.key.keysym.sym)
          {
            case SDLK_LCTRL:     lctrl=1;      break;
            case SDLK_LALT:      lalt=1;      break;
            case SDLK_RCTRL:     rctrl=1;      break;
            case SDLK_F1:        name = "F1";       break;
            case SDLK_F2:        name = "F2";       break;
            case SDLK_F3:        name = "F3";       break;
            case SDLK_F4:        name = "F4";       break;
            case SDLK_F5:        name = "F5";       break;
            case SDLK_F6:        name = "F6";       break;
            case SDLK_F7:        name = "F7";       break;
            case SDLK_F8:        name = "F8";       break;
            case SDLK_F9:        name = "F9";       break;
            case SDLK_F10:       name = "F10";      break;
            case SDLK_F11:       name = "F11";      break;
            case SDLK_F12:       name = "F12";      break;
            case SDLK_ESCAPE:    name = "escape";   break;
            case SDLK_DOWN:      name = "down";     break;
            case SDLK_LEFT:      name = "left";     break;
            case SDLK_UP:        name = "up";       break;
            case SDLK_RIGHT:     name = "right";    break;
            case SDLK_BACKSPACE: name = "backspace";break;
            case SDLK_SPACE:     name = "space";    break;
            case SDLK_TAB:       name = "tab";      break;
            case SDLK_DELETE:    name = "delete";   break;
            case SDLK_INSERT:    name = "insert";   break;
            case SDLK_RETURN:
		//if (key_repeat == 0) // return never should repeat
		  name = "return";   // on a DEC like terminal
		break;
            case SDLK_HOME:      name = "home";     break;
            case SDLK_END:       name = "end";      break;
            case SDLK_PAGEDOWN:  name = "page-down";break;
            case SDLK_PAGEUP:    name = "page-up";  break;

            default:;
          }
         if (strlen(name)){
          if (event.key.keysym.mod & (KMOD_CTRL) ||
              event.key.keysym.mod & (KMOD_ALT) ||
              strlen (name) >= 2)
          {

          if (event.key.keysym.mod & (KMOD_CTRL))
          {
            static char buf[64] = "";
            sprintf (buf, "control-%s", name);
            name = buf;
          }
          if (event.key.keysym.mod & (KMOD_ALT))
          {
            static char buf[128] = "";
            sprintf (buf, "alt-%s", name);
            name = buf;
          }
          if (event.key.keysym.mod & (KMOD_SHIFT))
          {
            static char buf[196] = "";
            sprintf (buf, "shift-%s", name);
            name = buf;
          }
            if (strcmp (name, "space") &&
		((vt_keyrepeat (vt)) || (key_repeat==0))
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
  while (SDL_PollEvent (&event));
  return got_event;
}
#endif


int vt_main(int argc, char **argv)
{
  execute_self = malloc (strlen (argv[0]) + 16);
  sprintf (execute_self, "%s", argv[0]);

  vt_width = ((int)(font_size/line_spacing + 0.999)) * DEFAULT_COLS;
  vt_height = font_size * DEFAULT_ROWS;
#if USE_MMM
  unsigned char *pixels;
  mmm_setup (vt_width, vt_height);
#elif USE_SDL
  sdl_setup (vt_width, vt_height);
#endif
  setsid();
  vt = vt_new (argv[1]?argv[1]:vt_find_shell_command(), DEFAULT_COLS, DEFAULT_ROWS, font_size, line_spacing);

#if USE_MMM
  vt_set_mmm (vt, mmm);
  mmm_pcm_set_sample_rate (mmm, 8000);
#endif

  int sleep_time = 10;

  vt_child = vt_get_pid (vt);
  while(!do_quit)
  {
     int in_scroll = (vt_has_blink (vt) >= 10);

     if (vt_is_done (vt))
       do_quit = 1;
          

      if ((drawn_rev != vt_rev (vt)) ||
          vt_has_blink (vt) ||
	  in_scroll)
      {
        drawn_rev = vt_rev (vt);

#if USE_SDL
        SDL_Rect dirty;
#endif

#if USE_MMM
      int width; int height;
      int stride;
        mmm_client_check_size (mmm, &width, &height);

        if (vt_width != width ||  vt_height!=height)
        {
          vt_set_term_size (vt, width / vt_cw (vt), height / vt_ch (vt));
	  vt_width = width;
	  vt_height = height;
	  drawn_rev = 0;
        }

        pixels = mmm_get_buffer_write (mmm, &width, &height, &stride, NULL);
#endif
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

        Ctx *ctx = ctx_new_for_framebuffer (pixels, vt_width, vt_height, vt_width * 4, CTX_FORMAT_BGRA8);

        vt_draw (vt, ctx, 0, 0);
#endif

#if USE_SDL
        ctx_dirty_rect (ctx, &dirty.x, &dirty.y, &dirty.w, &dirty.h);
#endif
        ctx_free (ctx);

#if USE_MMM
        mmm_write_done (mmm, 0, 0, -1, -1);
#elif USE_SDL
    

#if 1 // < flipping this turns on subtexture updates, needs bounds tuning
    dirty.w ++;
    dirty.h ++;
    if (dirty.x + dirty.w > vt_width)
            dirty.w = vt_width - dirty.x;
    if (dirty.y + dirty.h > vt_height)
            dirty.h = vt_height - dirty.y;
    SDL_UpdateTexture(texture,
                    &dirty,
                    (uint8_t*)pixels + sizeof(Uint32) * (vt_width * dirty.y + dirty.x) , vt_width * sizeof (Uint32));
#else
    SDL_UpdateTexture(texture, NULL,
                      (void*)pixels, vt_width * sizeof (Uint32));
#endif

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
#endif
      }

      int got_event = 0;
      if (!in_scroll)
      #if USE_MMM
	got_event = mmm_check_events (mmm);
      #elif USE_SDL
	got_event = sdl_check_events ();
      #endif

      if (got_event)
      {
          sleep_time = 200;
      }
      if (in_scroll)
      {
	sleep_time = 200;
      }

      if (vt_poll (vt, sleep_time))
      {
        if (sleep_time > 2500)
          sleep_time = 2500;
      }
      else
      {
        sleep_time *= 1.5;
      }
      if (sleep_time > 60000)
        sleep_time = 60000;
  }
  vt_destroy (vt);
#if USE_MMM
  if (mmm)
    mmm_destroy (mmm);
#endif
  return 0;
}
