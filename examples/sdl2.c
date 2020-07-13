#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <SDL.h>

#include "ctx-font-regular.h"
//#include "ctx-font-mono.h"
//#include "DejaVuSansMono.h"
//#include "DejaVuSans.h"
//#include "0xA000-Mono.h"
//#include "Vera.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

// this setup comes from the terminal, none of them are needed

#define CTX_BACKEND_TEXT         1
#define CTXP                     1
#define CTX_GRADIENT_CACHE       1
#define CTX_SHAPE_CACHE          1
#define CTX_SHAPE_CACHE_MAX_DIM  48
#define CTX_SHAPE_CACHE_DIM      (48*48)
#define CTX_SHAPE_CACHE_ENTRIES  (512)
#define CTX_RASTERIZER_AA        5
#define CTX_RASTERIZER_FORCE_AA  1
#define CTX_IMPLEMENTATION
#include "ctx.h"

int   do_quit      = 0;

static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;
static uint8_t      *pixels;

int width;
int height;

static int sdl_check_events ()
{
  int got_event = 0;
  static SDL_Event      event;
  if (SDL_WaitEventTimeout (&event, 0))
  do
  {
    switch (event.type)
    {
      case SDL_WINDOWEVENT: 
        { 
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) 
          { 
              int new_width  = event.window.data1; 
              int new_height = event.window.data2; 
	      got_event = 1;

	      if ((new_height != height) || (new_width != width))
	      {
                SDL_DestroyTexture (texture);
                texture = SDL_CreateTexture (renderer,SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING, new_width, new_height);
                pixels = realloc (pixels, new_width * new_height * 4);
                width = new_width;
                height = new_height;
	        return 1;
	      }
          }
	}
	break;
   case SDL_MOUSEBUTTONDOWN:
        fprintf (stderr, "mouse-press %.0f %.0f\n",
               (float)event.button.x,
               (float)event.button.y);
        break;
     case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
          case SDLK_LEFT:          break;
          case SDLK_ESCAPE: do_quit= 1; break;
        }
        got_event = 1;
      break;
    }
  }
  while (SDL_PollEvent (&event));
  return got_event;
}

int main(int argc, char **argv)
{
  width = 640;
  height = 480;
  window = SDL_CreateWindow("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);

#if 0 // hardware renderer
  renderer = SDL_CreateRenderer (window, -1, 0);
#else // software renderer
  renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_SOFTWARE);
#endif

  texture = SDL_CreateTexture (renderer,
	SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
  pixels = calloc (width * height, 4);
  SDL_EnableScreenSaver ();

  int x = 0;
  while(!do_quit)
  {
    SDL_Rect dirty;

    Ctx *ctx = ctx_new_for_framebuffer (pixels, width, height, width * 4, CTX_FORMAT_BGRA8);

    ctx_rectangle (ctx, 0,0,width,height);
    ctx_set_rgba (ctx, 0, 0, 0, 1);
    ctx_fill (ctx);
    ctx_rectangle (ctx, 20+x, 30, 100, 100);
    x++;
    ctx_set_rgba (ctx, 1, 1, 1, 1);
    ctx_fill (ctx);
    ctx_font_size (ctx, height * 0.1);
    ctx_move_to (ctx, width * 0.3, height * 0.4);
    ctx_text (ctx, "hello SDL2\n");
    ctx_set_rgba (ctx, 1, 1, 1, 1);

    ctx_dirty_rect (ctx, &dirty.x, &dirty.y, &dirty.w, &dirty.h);
    ctx_free (ctx);

#if 0 // < flipping this turns on subtexture updates, needs bounds tuning
    dirty.w ++;
    dirty.h ++;
    if (dirty.x + dirty.w > width)
            dirty.w = width - dirty.x;
    if (dirty.y + dirty.h > height)
            dirty.h = height - dirty.y;
    SDL_UpdateTexture(texture,
                    &dirty,
                    (void*)pixels + sizeof(Uint32) * (width * dirty.y + dirty.x) , width * sizeof (Uint32));
#else
    SDL_UpdateTexture(texture, NULL,
                      (void*)pixels, width * sizeof (Uint32));
#endif

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    if (!sdl_check_events ())
    {
      //SDL_Delay (100);
    }

  }
  return 0;
}
