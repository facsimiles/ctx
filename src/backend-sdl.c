#include "ctx-split.h"

#if CTX_SDL

typedef struct _CtxSDL CtxSDL;
struct _CtxSDL
{
   CtxTiled  tiled;
   /* where we diverge from fb*/
   int           key_balance;
   int           key_repeat;
   int           lctrl;
   int           lalt;
   int           rctrl;

   SDL_Window   *window;
   SDL_Renderer *renderer;
   SDL_Texture  *texture;

// cnd_t  cond;
// mtx_t  mtx;
};

#include "stb_image_write.h"

void ctx_screenshot (Ctx *ctx, const char *output_path)
{
#if CTX_SCREENSHOT
  int valid = 0;
  CtxSDL *sdl = (void*)ctx->renderer;

  if (ctx_renderer_is_sdl (ctx)) valid = 1;
#if CTX_FB
  if (ctx_renderer_is_fb  (ctx)) valid = 1;
#endif

  if (!valid)
    return;

#if CTX_FB
  // we rely on the same layout
  for (int i = 0; i < sdl->width * sdl->height; i++)
  {
    int tmp = sdl->pixels[i*4];
    sdl->pixels[i*4] = sdl->pixels[i*4 + 2];
    sdl->pixels[i*4 + 2] = tmp;
  }
#endif

  stbi_write_png (output_path, sdl->width, sdl->height, 4, sdl->pixels, sdl->width*4);

#if CTX_FB
  for (int i = 0; i < sdl->width * sdl->height; i++)
  {
    int tmp = sdl->pixels[i*4];
    sdl->pixels[i*4] = sdl->pixels[i*4 + 2];
    sdl->pixels[i*4 + 2] = tmp;
  }
#endif
#endif
}

void ctx_sdl_set_title (void *self, const char *new_title)
{
   CtxSDL *sdl = self;
   SDL_SetWindowTitle (sdl->window, new_title);
}

static inline int
ctx_tiled_threads_done (CtxTiled *tiled)
{
  int sum = 0;
  for (int i = 0; i < _ctx_max_threads; i++)
  {
     if (tiled->rendered_frame[i] == tiled->render_frame)
       sum ++;
  }
  return sum;
}

static void ctx_sdl_show_frame (CtxSDL *sdl, int block)
{
  CtxTiled *tiled = &sdl->tiled;
  if (tiled->shown_cursor != tiled->ctx->cursor)
  {
    tiled->shown_cursor = tiled->ctx->cursor;
    SDL_Cursor *new_cursor =  NULL;
    switch (tiled->shown_cursor)
    {
      case CTX_CURSOR_UNSET: // XXX: document how this differs from none
                             //      perhaps falling back to arrow?
        break;
      case CTX_CURSOR_NONE:
        new_cursor = NULL;
        break;
      case CTX_CURSOR_ARROW:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        break;
      case CTX_CURSOR_CROSSHAIR:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        break;
      case CTX_CURSOR_WAIT:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
        break;
      case CTX_CURSOR_HAND:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
        break;
      case CTX_CURSOR_IBEAM:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
        break;
      case CTX_CURSOR_MOVE:
      case CTX_CURSOR_RESIZE_ALL:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        break;
      case CTX_CURSOR_RESIZE_N:
      case CTX_CURSOR_RESIZE_S:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
        break;
      case CTX_CURSOR_RESIZE_E:
      case CTX_CURSOR_RESIZE_W:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        break;
      case CTX_CURSOR_RESIZE_NE:
      case CTX_CURSOR_RESIZE_SW:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
        break;
      case CTX_CURSOR_RESIZE_NW:
      case CTX_CURSOR_RESIZE_SE:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
        break;
    }
    if (new_cursor)
    {
      SDL_Cursor *old_cursor = SDL_GetCursor();
      SDL_SetCursor (new_cursor);
      SDL_ShowCursor (1);
      if (old_cursor)
        SDL_FreeCursor (old_cursor);
    }
    else
    {
      SDL_ShowCursor (0);
    }
  }

  if (tiled->shown_frame == tiled->render_frame)
  {
    return;
  }

  if (block)
  {
    int count = 0;
    while (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
    {
      usleep (50);
      count ++;
      if (count > 2000)
      {
        tiled->shown_frame = tiled->render_frame;
        return;
      }
    }
  }
  else
  {
    if (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
      return;
  }

  if (tiled->min_row == 100)
  {
  }
  else
  {
#if 1
    int x = tiled->min_col * tiled->width/CTX_HASH_COLS;
    int y = tiled->min_row * tiled->height/CTX_HASH_ROWS;
    int x1 = (tiled->max_col+1) * tiled->width/CTX_HASH_COLS;
    int y1 = (tiled->max_row+1) * tiled->height/CTX_HASH_ROWS;
    int width = x1 - x;
    int height = y1 - y;
#endif
    tiled->min_row = 100;
    tiled->max_row = 0;
    tiled->min_col = 100;
    tiled->max_col = 0;

    SDL_Rect r = {x, y, width, height};
    SDL_UpdateTexture (sdl->texture, &r,
                      //(void*)sdl->pixels,
                      (void*)(tiled->pixels + y * tiled->width * 4 + x * 4),
                      
                      tiled->width * 4);
    SDL_RenderClear (sdl->renderer);
    SDL_RenderCopy (sdl->renderer, sdl->texture, NULL, NULL);
    SDL_RenderPresent (sdl->renderer);
  }

  tiled->shown_frame = tiled->render_frame;
}

int ctx_sdl_consume_events (Ctx *ctx)
{
  CtxTiled *tiled = (void*)ctx->renderer;
  CtxSDL *sdl = (void*)ctx->renderer;
  SDL_Event event;
  int got_events = 0;

  ctx_sdl_show_frame (sdl, 0);

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
        ctx_pointer_motion (ctx, event.tfinger.x * tiled->width, event.tfinger.y * tiled->height,
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
          ctx_pointer_press (ctx, event.tfinger.x * tiled->width, event.tfinger.y * tiled->height, 
          (event.tfinger.fingerId%10) + 4, 0);
        }
        }
        break;
      case SDL_FINGERUP:
        ctx_pointer_release (ctx, event.tfinger.x * tiled->width, event.tfinger.y * tiled->height,
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
          ctx_sdl_show_frame (sdl, 1);
          int width = event.window.data1;
          int height = event.window.data2;
          SDL_DestroyTexture (sdl->texture);
          sdl->texture = SDL_CreateTexture (sdl->renderer, SDL_PIXELFORMAT_ABGR8888,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
          free (tiled->pixels);
          tiled->pixels = calloc (4, width * height);

          tiled->width  = width;
          tiled->height = height;
          ctx_set_size (tiled->ctx, width, height);
          ctx_set_size (tiled->ctx_copy, width, height);
        }
        break;
    }
  }
  return 1;
}
#else
void ctx_screenshot (Ctx *ctx, const char *path)
{
}
#endif

#if CTX_SDL

static void ctx_sdl_set_clipboard (CtxSDL *sdl, const char *text)
{
  if (text)
    SDL_SetClipboardText (text);
}

static char *ctx_sdl_get_clipboard (CtxSDL *sdl)
{
  return SDL_GetClipboardText ();
}

inline static void ctx_sdl_reset (CtxSDL *sdl)
{
  ctx_sdl_show_frame (sdl, 1);
}

void ctx_tiled_free (CtxTiled *tiled)
{
  CtxTiled *tiled = (void*)sdl;
  tiled->quit = 1;
  mtx_lock (&tiled->mtx);
  cnd_broadcast (&tiled->cond);
  mtx_unlock (&tiled->mtx);

  while (tiled->thread_quit < _ctx_max_threads)
    usleep (1000);

  if (tiled->pixels)
  {
    free (tiled->pixels);
  tiled->pixels = NULL;
  for (int i = 0 ; i < _ctx_max_threads; i++)
  {
    ctx_free (tiled->host[i]);
    tiled->host[i]=NULL;
  }

  ctx_free (tiled->ctx_copy);
  }
  // leak?
}

inline static void ctx_tiled_flush (CtxTiled *tiled)
{
  if (tiled->shown_frame == tiled->render_frame)
  {
    int dirty_tiles = 0;
    ctx_set_drawlist (tiled->ctx_copy, &tiled->ctx->drawlist.entries[0],
                                           tiled->ctx->drawlist.count * 9);
    if (_ctx_enable_hash_cache)
    {
      Ctx *hasher = ctx_hasher_new (tiled->width, tiled->height,
                        CTX_HASH_COLS, CTX_HASH_ROWS);
      ctx_render_ctx (tiled->ctx_copy, hasher);

      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          uint8_t *new_hash = ctx_hasher_get_hash (hasher, col, row);
          if (new_hash && memcmp (new_hash, &tiled->hashes[(row * CTX_HASH_COLS + col) *  20], 20))
          {
            memcpy (&tiled->hashes[(row * CTX_HASH_COLS +  col)*20], new_hash, 20);
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
            dirty_tiles++;
          }
          else
          {
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = -1;
          }
        }
      free (((CtxHasher*)(hasher->renderer))->hashes);
      ctx_free (hasher);
    }
    else
    {
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
          dirty_tiles++;
        }
    }
    int dirty_no = 0;
    if (dirty_tiles)
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        if (tiled->tile_affinity[row * CTX_HASH_COLS + col] != -1)
        {
          tiled->tile_affinity[row * CTX_HASH_COLS + col] = dirty_no * (_ctx_max_threads) / dirty_tiles;
          dirty_no++;
          if (col > tiled->max_col) tiled->max_col = col;
          if (col < tiled->min_col) tiled->min_col = col;
          if (row > tiled->max_row) tiled->max_row = row;
          if (row < tiled->min_row) tiled->min_row = row;
        }
      }

#if CTX_DAMAGE_CONTROL
    for (int i = 0; i < tiled->width * tiled->height; i++)
    {
      int new_ = (tiled->pixels[i*4+0]+ tiled->pixels[i*4+1]+ tiled->pixels[i*4+2])/3;
      //if (new_>1) new_--;
      tiled->pixels[i*4]  = (tiled->pixels[i*4] + 255)/2;
      tiled->pixels[i*4+1]= (tiled->pixels[i*4+1] + new_)/2;
      tiled->pixels[i*4+2]= (tiled->pixels[i*4+1] + new_)/2;
    }
#endif

    tiled->render_frame = ++tiled->frame;

    mtx_lock (&tiled->mtx);
    cnd_broadcast (&tiled->cond);
    mtx_unlock (&tiled->mtx);
  }
}

inline static void ctx_sdl_flush (CtxSDL *sdl)
{
  ctx_tiled_flush ((void*)sdl);
  //CtxTiled *tiled = (void*)sdl;
}


void ctx_sdl_free (CtxSDL *sdl)
{

  if (sdl->texture)
  SDL_DestroyTexture (sdl->texture);
  if (sdl->renderer)
  SDL_DestroyRenderer (sdl->renderer);
  if (sdl->window)
  SDL_DestroyWindow (sdl->window);

  ctx_tiled_free ((CtxTiled*)sdl);
}

static unsigned char *sdl_icc = NULL;
static long sdl_icc_length = 0;

static
void ctx_tiled_render_fun (void **data)
{
  int      no = (size_t)data[0];
  CtxTiled *tiled = data[1];

  while (!tiled->quit)
  {
    Ctx *host = tiled->host[no];

    mtx_lock (&tiled->mtx);
    cnd_wait(&tiled->cond, &tiled->mtx);
    mtx_unlock (&tiled->mtx);

    if (tiled->render_frame != tiled->rendered_frame[no])
    {
      int hno = 0;
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++, hno++)
        {
          if (tiled->tile_affinity[hno]==no)
          {
            int x0 = ((tiled->width)/CTX_HASH_COLS) * col;
            int y0 = ((tiled->height)/CTX_HASH_ROWS) * row;
            int width = tiled->width / CTX_HASH_COLS;
            int height = tiled->height / CTX_HASH_ROWS;

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->renderer;
#if 1 // merge horizontally adjecant tiles of same affinity into one job
            while (col + 1 < CTX_HASH_COLS &&
                   tiled->tile_affinity[hno+1] == no)
            {
              width += tiled->width / CTX_HASH_COLS;
              col++;
              hno++;
            }
#endif
            ctx_rasterizer_init (rasterizer,
                                 host, tiled->ctx, &host->state,
                                 &tiled->pixels[tiled->width * 4 * y0 + x0 * 4],
                                 0, 0, width, height,
                                 tiled->width*4, CTX_FORMAT_RGBA8,
                                 tiled->antialias);
            if (sdl_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, sdl_icc, sdl_icc_length);

            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (tiled->ctx_copy, host);
          }
        }
      tiled->rendered_frame[no] = tiled->render_frame;
    }
  }

  tiled->thread_quit++; // need atomic?
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
  CtxTiled *tiled = (void*)sdl;

  _ctx_file_get_contents ("/tmp/ctx.icc", &sdl_icc, &sdl_icc_length);
  if (width <= 0 || height <= 0)
  {
    width  = 1920;
    height = 1080;
  }
  sdl->window = SDL_CreateWindow("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  //sdl->renderer = SDL_CreateRenderer (sdl->window, -1, SDL_RENDERER_SOFTWARE);
  sdl->renderer = SDL_CreateRenderer (sdl->window, -1, 0);
  if (!sdl->renderer)
  {
     ctx_free (tiled->ctx);
     free (sdl);
     return NULL;
  }
#if CTX_BABL
  babl_init ();
#endif

  ctx_sdl_events = 1;
  sdl->texture = SDL_CreateTexture (sdl->renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);

  SDL_StartTextInput ();
  SDL_EnableScreenSaver ();

  tiled->ctx = ctx_new ();
  tiled->ctx_copy = ctx_new ();
  tiled->width  = width;
  tiled->height = height;
  tiled->cols = 80;
  tiled->rows = 20;
  ctx_set_renderer (tiled->ctx, sdl);
  ctx_set_renderer (tiled->ctx_copy, sdl);

  tiled->pixels = (uint8_t*)malloc (width * height * 4);

  ctx_set_size (tiled->ctx,      width, height);
  ctx_set_size (tiled->ctx_copy, width, height);

  tiled->flush = (void*)ctx_sdl_flush;
  tiled->reset = (void*)ctx_sdl_reset;
  tiled->free  = (void*)ctx_sdl_free;
  tiled->set_clipboard = (void*)ctx_sdl_set_clipboard;
  tiled->get_clipboard = (void*)ctx_sdl_get_clipboard;

  for (int i = 0; i < _ctx_max_threads; i++)
  {
    tiled->host[i] = ctx_new_for_framebuffer (tiled->pixels,
                     tiled->width/CTX_HASH_COLS, tiled->height/CTX_HASH_ROWS,
                     tiled->width * 4, CTX_FORMAT_RGBA8);
    ctx_set_texture_source (tiled->host[i], tiled->ctx);
  }

  mtx_init (&tiled->mtx, mtx_plain);
  cnd_init (&tiled->cond);

#define start_thread(no)\
  if(_ctx_max_threads>no){ \
    static void *args[2]={(void*)no, };\
    thrd_t tid;\
    args[1]=sdl;\
    thrd_create (&tid, (void*)ctx_tiled_render_fun, args);\
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

  ctx_flush (tiled->ctx);
  return tiled->ctx;
#else
  return NULL;
#endif
}
#else

int ctx_renderer_is_sdl (Ctx *ctx)
{
  return 0;
}
#endif
