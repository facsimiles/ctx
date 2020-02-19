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

#if USE_SDL
#include <SDL.h>
#endif

#if USE_MMM
#include "mmm.h"
#include "mmm-pset.h"
#endif

#include "ctx-font-regular.h"
#include "ctx-font-mono.h"

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
static MrgVT *vt = NULL;

#if USE_MMM
static Mmm *mmm = NULL;
#elif USE_SDL
static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;
static uint8_t      *pixels;
static int pointer_down[3] = {0,};
static int lctrl = 0;
static int rctrl = 0;
#endif

static char *execute_self = NULL;

int vt_width;
int vt_height;

#if USE_SDL
void sdl_setup (int width, int height)
{
  window = SDL_CreateWindow("gvt", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  renderer = SDL_CreateRenderer (window, -1, 0);
  //renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_SOFTWARE);
  SDL_StartTextInput ();
  texture = SDL_CreateTexture (renderer,
	SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
  pixels = calloc (width * height, 4);
}
#elif USE_MMM
void mmm_setup (int width, int height)
{
  mmm = mmm_new (vt_width, vt_height, 0, NULL);
  unsetenv ("MMM_PATH");
}
#endif

void
signal_child (int signum)
{
  pid_t pid;
  int   status;
  while ((pid = waitpid(-1, &status, WNOHANG)) != -1)
    {
      if (pid)
      {
      if (pid == vt_child)
      {
	exit(0);
        do_quit = 1;
        return;
      }
      else
      {
        fprintf (stderr, "child signal ? %i %i\n", pid, vt_child);
      }
    }
    }
}

static int16_t pcm_queue[1<<18];
static int     pcm_write_pos = 0;
static int     pcm_read_pos  = 0;

void terminal_queue_pcm_sample (int16_t sample)
{
  if (pcm_write_pos >= (1<<18)-1)
  {
    /*  TODO  :  fix cyclic buffer */
    pcm_write_pos = 0;
    pcm_read_pos  = 0;
  }
  pcm_queue[pcm_write_pos++]=sample;
  pcm_queue[pcm_write_pos++]=sample;
}

extern float ctx_shape_cache_rate;
float click_volume = 0.05;

void audio_task (int click)
{
#if USE_MMM
  int free_frames = mmm_pcm_get_frame_chunk (mmm)+24;
  //int free_frames = mmm_pcm_get_free_frames (mmm);
  int queued = (pcm_write_pos - pcm_read_pos)/2;
  if (free_frames > 6) free_frames -= 4;
  int frames = queued;

  if (frames > free_frames) frames = free_frames;
  if (frames > 0)
  {
    if (click)
    {
      int16_t pcm_data[]={-32000 * click_volume,32000 * click_volume,0,0};
      mmm_pcm_queue (mmm, (int8_t*) pcm_data, 1);
    }

    mmm_pcm_queue (mmm, (int8_t*)&pcm_queue[pcm_read_pos], frames);
    pcm_read_pos += frames*2;
  }
#if ENABLE_CLICK
  else
  {
    int16_t pcm_silence[4096]={0,};

    if (click)
    {
      int16_t pcm_data[]={-32000 * click_volume,32000 * click_volume,0,0};
      mmm_pcm_queue (mmm, (int8_t*) pcm_data, 1);
    }

    if (free_frames > 500)
    mmm_pcm_queue (mmm, (int8_t*) pcm_silence, 500);
  }
#endif
#endif
}

static void handle_event (const char *event)
{
	//if (!strcmp (event, "shift-return"))
	 // event = "return";
	//else
	if (!strcmp (event, "shift-page-up"))
	{
	  int new_scroll = ctx_vt_get_scroll (vt) + ctx_vt_get_rows (vt)/2;
	  ctx_vt_set_scroll (vt, new_scroll);
	  ctx_vt_rev_inc (vt);
	} else if (!strcmp (event, "shift-page-down"))
	{
	  int new_scroll = ctx_vt_get_scroll (vt) - ctx_vt_get_rows (vt)/2;
	  if (new_scroll < 0) new_scroll = 0;
	  ctx_vt_set_scroll (vt, new_scroll);
	  ctx_vt_rev_inc (vt);
	} else if (!strcmp (event, "shift-control-v")) {
	  char *text = SDL_GetClipboardText ();
	  if (text)
	  {
            ctx_vt_paste (vt, text);
	    free (text);
	  }
	} else if (!strcmp (event, "shift-control--") ||
	           !strcmp (event, "control--")) {
	  font_size /= 1.15;
	  font_size = (int) (font_size);
	  if (font_size < 5) font_size = 5;

	  ctx_vt_set_font_size (vt, font_size);
          ctx_vt_set_term_size (vt, vt_width / ctx_vt_cw (vt), vt_height / ctx_vt_ch (vt));
	} else if (!strcmp (event, "shift-control-=") ||
	           !strcmp (event, "control-=")) {
	  float old = font_size;
	  font_size *= 1.15;
	  font_size = (int)(font_size);
	  if (old == font_size) font_size = old+1;
	  if (font_size > 200) font_size = 200;

	  ctx_vt_set_font_size (vt, font_size);
          ctx_vt_set_term_size (vt, vt_width / ctx_vt_cw (vt), vt_height / ctx_vt_ch (vt));
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
	  ctx_vt_open_log (vt, "/tmp/ctx-vt");
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
	  int cw = ctx_vt_cw (vt);
	  int ch = ctx_vt_ch (vt);
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
	        ctx_vt_mouse (vt, VT_MOUSE_MOTION, x/cw + 1, y/ch + 1, x, y);
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
	        ctx_vt_mouse (vt, VT_MOUSE_PRESS, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
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
	        ctx_vt_mouse (vt, VT_MOUSE_DRAG, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
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
	        ctx_vt_mouse (vt, VT_MOUSE_RELEASE, x/cw + 1, y/ch + 1, x, y);
	      }
	    }
	  }
	}
        else
        {
          ctx_vt_feed_keystring (vt, event);
	  // make optional?
	  ctx_vt_set_scroll (vt, 0);
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
          ctx_vt_set_term_size (vt, width / ctx_vt_cw (vt), height / ctx_vt_ch (vt));
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
        if (!lctrl && !rctrl && 
		((ctx_vt_keyrepeat (vt)) || (key_repeat==0))
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
		if (key_repeat == 0) // return never should repeat
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
		((ctx_vt_keyrepeat (vt)) || (key_repeat==0))
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
  vt = ctx_vt_new (argv[1]?argv[1]:ctx_vt_find_shell_command(), DEFAULT_COLS, DEFAULT_ROWS, font_size, line_spacing);

#if USE_MMM
  ctx_vt_set_mmm (vt, mmm);
  mmm_pcm_set_sample_rate (mmm, 8000);
#endif

  int sleep_time = 10;
  long drawn_rev = 0;

  vt_child = ctx_vt_get_pid (vt);
  signal (SIGCHLD, signal_child);
  while(!do_quit)
  {

      int in_scroll = (ctx_vt_has_blink (vt) >= 10);


      if ((drawn_rev != ctx_vt_rev (vt)) ||
          ctx_vt_has_blink (vt) ||
	  in_scroll)
      {
        drawn_rev = ctx_vt_rev (vt);

#if USE_MMM
      int width; int height;
      int stride;
        mmm_client_check_size (mmm, &width, &height);

        if (vt_width != width ||  vt_height!=height)
        {
          ctx_vt_set_term_size (vt, width / ctx_vt_cw (vt), height / ctx_vt_ch (vt));
	  vt_width = width;
	  vt_height = height;
	  drawn_rev = 0;
        }

        pixels = mmm_get_buffer_write (mmm, &width, &height, &stride, NULL);
#endif
        Ctx *ctx = ctx_new_for_framebuffer (pixels, vt_width, vt_height, vt_width * 4, CTX_FORMAT_BGRA8);

        ctx_vt_draw (vt, ctx, 0, 0);

        ctx_free (ctx);

#if USE_MMM
        mmm_write_done (mmm, 0, 0, -1, -1);
#elif USE_SDL
    SDL_UpdateTexture(texture, NULL, (void*)pixels, vt_width * sizeof (Uint32));

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
#endif
        audio_task (0);
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
          sleep_time = 800;
	  drawn_rev = 0;
      }
      if (in_scroll)
      {
	sleep_time = 400;
      }
      if (!got_event)
      {
        audio_task (0);
        if (ctx_vt_poll (vt, sleep_time))
	{
	  if (sleep_time > 25000)
	    sleep_time = 25000;
	}
        sleep_time *= 1.5;
        if (sleep_time > 100000)
          sleep_time = 100000;
      }
      audio_task (got_event);
  }
  ctx_vt_destroy (vt);
#if USE_MMM
  if (mmm)
    mmm_destroy (mmm);
#endif
  return 0;
}
