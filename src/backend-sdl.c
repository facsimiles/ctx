#include "ctx-split.h"

#if CTX_SDL

 // 1 threads 13fps
 // 2 threads 20fps
 // 3 threads 27fps
 // 4 threads 29fps


typedef struct _CtxSDL CtxSDL;
struct _CtxSDL
{
   void (*render)    (void *braille, CtxCommand *command);
   void (*flush)     (void *braille);
   void (*free)      (void *braille);
   Ctx          *ctx;
   Ctx          *ctx_copy;
   int           width;
   int           height;
   int           cols;
   int           rows;
   int           was_down;
   Ctx          *host[CTX_MAX_THREADS];
   CtxAntialias  antialias;
   uint8_t      *pixels;
   int           quit;
   _Atomic int   thread_quit;

   int           shown_frame;
   int           render_frame;
   int           rendered_frame[CTX_MAX_THREADS];
   int           frame;
   int       min_col; // hasher cols and rows
   int       min_row;
   int       max_col;
   int       max_row;
   uint8_t  hashes[CTX_HASH_ROWS * CTX_HASH_COLS *  20];
   int8_t    tile_affinity[CTX_HASH_ROWS * CTX_HASH_COLS]; // which render thread no is
                                                           // responsible for a tile
                                                           //

   int           pointer_down[3];
   int           key_balance;
   int           key_repeat;
   int           lctrl;
   int           lalt;
   int           rctrl;

   SDL_Window   *window;
   SDL_Renderer *renderer;
   SDL_Texture  *texture;
};

void ctx_sdl_set_title (void *self, const char *new_title)
{
   CtxSDL *sdl = self;
   SDL_SetWindowTitle (sdl->window, new_title);
}

static inline int
sdl_render_threads_done (CtxSDL *sdl)
{
  int sum = 0;
  for (int i = 0; i < _ctx_threads; i++)
  {
     if (sdl->rendered_frame[i] == sdl->render_frame)
       sum ++;
  }
  return sum;
}

static void ctx_sdl_show_frame (CtxSDL *sdl)
{
  if (sdl->shown_frame != sdl->render_frame &&
      sdl_render_threads_done (sdl) == _ctx_threads)
  {
    SDL_UpdateTexture (sdl->texture, NULL,
                      (void*)sdl->pixels, sdl->width * sizeof (Uint32));
    SDL_RenderClear (sdl->renderer);
    SDL_RenderCopy (sdl->renderer, sdl->texture, NULL, NULL);
    SDL_RenderPresent (sdl->renderer);
    sdl->shown_frame = sdl->render_frame;
  }
}


int ctx_sdl_consume_events (Ctx *ctx)
{
  CtxSDL *sdl = (void*)ctx->renderer;
  SDL_Event event;
  int got_events = 0;
  if(0){
    const char *title = ctx_get (sdl->ctx, "title");
    static char *set_title = NULL;
    if (title)
    {
    if (set_title == NULL || strcmp (title, set_title))
    {
      if (set_title) free (set_title);
      set_title = strdup (title);
    }
    }
  }

  ctx_sdl_show_frame (sdl);

  while (SDL_PollEvent (&event))
  {
    got_events ++;
    switch (event.type)
    {
      case SDL_MOUSEBUTTONDOWN:
        SDL_CaptureMouse (1);
        ctx_pointer_press (ctx, event.button.x, event.button.y, event.button.button, 0);
        break;
      case SDL_MOUSEBUTTONUP:
        SDL_CaptureMouse (0);
        ctx_pointer_release (ctx, event.button.x, event.button.y, event.button.button, 0);
        break;
      case SDL_MOUSEMOTION:
        //  XXX : look at mask and generate motion for each pressed
        //        button
        ctx_pointer_motion (ctx, event.motion.x, event.motion.y, 1, 0);
        break;
      case SDL_FINGERMOTION:
        ctx_pointer_motion (ctx, event.tfinger.x * sdl->width, event.tfinger.y * sdl->height,
            (event.tfinger.fingerId%10) + 4, 0);
        break;
      case SDL_FINGERDOWN:
        {
        static int fdowns = 0;
        fdowns ++;
        if (fdowns > 1) // the very first finger down from SDL seems to be
                        // mirrored as mouse events, later ones not - at
                        // least under wayland
        {
          ctx_pointer_press (ctx, event.tfinger.x *sdl->width, event.tfinger.y * sdl->height, 
          (event.tfinger.fingerId%10) + 4, 0);
        }
        }
        break;
      case SDL_FINGERUP:
        ctx_pointer_release (ctx, event.tfinger.x * sdl->width, event.tfinger.y * sdl->height,
          (event.tfinger.fingerId%10) + 4, 0);
        break;
      case SDL_KEYUP:
        {
           sdl->key_balance --;
           switch (event.key.keysym.sym)
           {
             case SDLK_LCTRL: sdl->lctrl = 0; break;
             case SDLK_RCTRL: sdl->rctrl = 0; break;
             case SDLK_LALT:  sdl->lalt  = 0; break;
           }
        }
        break;
#if 1
      case SDL_TEXTINPUT:
    //  if (!active)
    //    break;
        if (!sdl->lctrl && !sdl->rctrl && !sdl->lalt 
           //&& ( (vt && vt_keyrepeat (vt) ) || (key_repeat==0) )
           )
          {
            const char *name = event.text.text;
            if (!strcmp (name, " ") ) { name = "space"; }
            ctx_key_press (ctx, 0, name, 0);
            //got_event = 1;
          }
        break;
#endif
      case SDL_KEYDOWN:
        {
          char buf[32] = "";
          char *name = buf;
          if (!event.key.repeat)
          {
            sdl->key_balance ++;
            sdl->key_repeat = 0;
          }
          else
          {
            sdl->key_repeat ++;
          }
          buf[ctx_unichar_to_utf8 (event.key.keysym.sym, (void*)buf)]=0;
          switch (event.key.keysym.sym)
          {
            case SDLK_LCTRL: sdl->lctrl = 1; break;
            case SDLK_LALT:  sdl->lalt = 1; break;
            case SDLK_RCTRL: sdl->rctrl = 1; break;
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
          if (strlen (name)
              &&(event.key.keysym.mod & (KMOD_CTRL) ||
                 event.key.keysym.mod & (KMOD_ALT) ||
                 strlen (name) >= 2))
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
            if (strcmp (name, "space"))
              {
               ctx_key_press (ctx, 0, name, 0);
              }
          }
          else
          {
#if 0
             ctx_key_press (ctx, 0, buf, 0);
#endif
          }
        }
        break;
      case SDL_QUIT:
        ctx_quit (ctx);
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
          while (sdl->shown_frame != sdl->render_frame) {
                  usleep (100);
                  ctx_sdl_show_frame (sdl);
          }
          int width = event.window.data1;
          int height = event.window.data2;
          SDL_DestroyTexture (sdl->texture);
          sdl->texture = SDL_CreateTexture (sdl->renderer, SDL_PIXELFORMAT_ABGR8888,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
          free (sdl->pixels);
          sdl->pixels = calloc (4, width * height);

          sdl->width  = width;
          sdl->height = height;
          ctx_set_size (sdl->ctx, width, height);
          ctx_set_size (sdl->ctx_copy, width, height);
        }
        break;
    }
  }
  return 1;
}
#endif

#if CTX_SDL

inline static void ctx_sdl_flush (CtxSDL *sdl)
{
  //int width =  sdl->width;
  int count = 0;
  while (sdl->shown_frame != sdl->render_frame && count < 1000)
  {
    usleep (100);
    ctx_sdl_show_frame (sdl);
    count++;
  }
  if (count >= 10000)
  {
    sdl->shown_frame = sdl->render_frame;
  }
  if (sdl->shown_frame == sdl->render_frame)
  {
    int dirty_tiles = 0;
    ctx_set_renderstream (sdl->ctx_copy, &sdl->ctx->renderstream.entries[0],
                                         sdl->ctx->renderstream.count * 9);
    if (_ctx_enable_hash_cache)
    {
    Ctx *hasher = ctx_hasher_new (sdl->width, sdl->height,
                      CTX_HASH_COLS, CTX_HASH_ROWS);
    ctx_render_ctx (sdl->ctx_copy, hasher);

    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        uint8_t *new_hash = ctx_hasher_get_hash (hasher, col, row);
        if (new_hash && memcmp (new_hash, &sdl->hashes[(row * CTX_HASH_COLS + col) *  20], 20))
        {
          memcpy (&sdl->hashes[(row * CTX_HASH_COLS +  col)*20], new_hash, 20);
          sdl->tile_affinity[row * CTX_HASH_COLS + col] = 1;
          dirty_tiles++;
        }
        else
        {
          sdl->tile_affinity[row * CTX_HASH_COLS + col] = -1;
        }
      }
    ctx_free (hasher);
    }
    else
    {
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          sdl->tile_affinity[row * CTX_HASH_COLS + col] = 1;
          dirty_tiles++;
        }
    }
    int dirty_no = 0;
    if (dirty_tiles)
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        if (sdl->tile_affinity[row * CTX_HASH_COLS + col] != -1)
        {
          sdl->tile_affinity[row * CTX_HASH_COLS + col] = dirty_no * (_ctx_threads) / dirty_tiles;
          dirty_no++;
        }
      }

    sdl->render_frame = ++sdl->frame;
  }
}

void ctx_sdl_free (CtxSDL *sdl)
{
  sdl->quit = 1;
  while (sdl->thread_quit < _ctx_threads)
    usleep (1000); // XXX : properly wait for threads instead
  if (sdl->pixels)
  {
    free (sdl->pixels);
  sdl->pixels = NULL;
  for (int i = 0 ; i < _ctx_threads; i++)
  {
    ctx_free (sdl->host[i]);
    sdl->host[i]=NULL;
  }
  SDL_DestroyTexture (sdl->texture);
  SDL_DestroyRenderer (sdl->renderer);
  SDL_DestroyWindow (sdl->window);
  ctx_free (sdl->ctx_copy);
  }
  //free (sdl); // kept alive for threads quit check..
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

static
void sdl_render_fun (void **data)
{
  int      no = (size_t)data[0];
  CtxSDL *sdl = data[1];

  int sleep_time = 2000;
  while (!sdl->quit)
  {
    Ctx *host = sdl->host[no];
    if (sdl->render_frame != sdl->rendered_frame[no])
    {
      int hno = 0;
      sleep_time = 2000;
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++, hno++)
        {
          if (sdl->tile_affinity[hno]==no)
          {
            int x0 = ((sdl->width)/CTX_HASH_COLS) * col;
            int y0 = ((sdl->height)/CTX_HASH_ROWS) * row;
            int width = sdl->width / CTX_HASH_COLS;
            int height = sdl->height / CTX_HASH_ROWS;

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->renderer;
#if 1 // merge horizontally adjecant tiles of same affinity into one job
            while (col + 1 < CTX_HASH_COLS &&
                   sdl->tile_affinity[hno+1] == no)
            {
              width += sdl->width / CTX_HASH_COLS;
              col++;
              hno++;
            }
#endif
            ctx_rasterizer_init (rasterizer,
                                 host, NULL, &host->state,
                                 &sdl->pixels[sdl->width * 4 * y0 + x0 * 4],
                                 0, 0, width, height,
                                 sdl->width*4, CTX_FORMAT_RGBA8,
                                 sdl->antialias);
            ((CtxRasterizer*)host->renderer)->texture_source = sdl->ctx;
            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (sdl->ctx_copy, host);
          }
        }
      sdl->rendered_frame[no] = sdl->render_frame;

      if (sdl_render_threads_done (sdl) == _ctx_threads)
      {
   //   ctx_render_stream (sdl->ctx_copy, stdout, 1);
   //   ctx_reset (sdl->ctx_copy);
      }
    }
    else
    {
      usleep (sleep_time);
      sleep_time *= 2;
      if (sleep_time > 1000000/8)
          sleep_time = 1000000/8;
    }
  }

  sdl->thread_quit++; // need atomic?
}

int ctx_renderer_is_sdl (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_sdl_free)
          return 1;
  return 0;
}

Ctx *ctx_new_sdl (int width, int height)
{
#if CTX_RASTERIZER
  CtxSDL *sdl = (CtxSDL*)calloc (sizeof (CtxSDL), 1);
  if (width <= 0 || height <= 0)
  {
    width  = 1280;
    height = 768;
  }
  sdl->window = SDL_CreateWindow("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  //sdl->renderer = SDL_CreateRenderer (sdl->window, -1, SDL_RENDERER_SOFTWARE);
  sdl->renderer = SDL_CreateRenderer (sdl->window, -1, 0);
  if (!sdl->renderer)
  {
     ctx_free (sdl->ctx);
     free (sdl);
     return NULL;
  }
  ctx_sdl_events = 1;
  sdl->texture = SDL_CreateTexture (sdl->renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);

  SDL_StartTextInput ();
  SDL_EnableScreenSaver ();

  sdl->ctx = ctx_new ();
  sdl->ctx_copy = ctx_new ();
  sdl->width  = width;
  sdl->height = height;
  sdl->cols = 80;
  sdl->rows = 20;
  sdl->pixels = (uint8_t*)malloc (width * height * 4);
  ctx_set_renderer (sdl->ctx, sdl);
  ctx_set_renderer (sdl->ctx_copy, sdl);

  ctx_set_size (sdl->ctx, width, height);
  ctx_set_size (sdl->ctx_copy, width, height);
  sdl->flush = (void*)ctx_sdl_flush;
  sdl->free  = (void*)ctx_sdl_free;

  for (int i = 0; i < _ctx_threads; i++)
  {
    sdl->host[i] = ctx_new_for_framebuffer (sdl->pixels,
                   sdl->width/CTX_HASH_COLS, sdl->height/CTX_HASH_ROWS,
                   sdl->width * 4, CTX_FORMAT_RGBA8);
    ((CtxRasterizer*)sdl->host[i]->renderer)->texture_source = sdl->ctx;
  }

#define start_thread(no)\
  if(_ctx_threads>no){ \
    static void *args[2]={(void*)no, };\
    args[1]=sdl;\
    SDL_CreateThread ((void*)sdl_render_fun, "render", args);\
  }
  start_thread(0);
  start_thread(1);
  start_thread(2);
  start_thread(3);
  start_thread(4);
  start_thread(5);
  start_thread(6);
  start_thread(7);
  start_thread(8);
  start_thread(9);
  start_thread(10);
  start_thread(11);
  start_thread(12);
  start_thread(13);
  start_thread(14);
  start_thread(15);
#undef start_thread

#endif
  ctx_flush (sdl->ctx);
  return sdl->ctx;
}
#else

int ctx_renderer_is_sdl (Ctx *ctx)
{
  return 0;
}
#endif
