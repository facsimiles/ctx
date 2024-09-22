#include "ctx-split.h"

#if CTX_SDL

/**/

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
   int           lshift;
   int           rshift;

   SDL_Window   *window;
   SDL_Renderer *backend;
   SDL_Texture  *texture;

   int           fullscreen;

};

int ctx_show_fps = 1;
void ctx_sdl_set_title (void *self, const char *new_title)
{
   Ctx *ctx = (Ctx*)self;
   CtxSDL *sdl = (CtxSDL*)ctx->backend;
   if (!ctx_show_fps)
   SDL_SetWindowTitle (sdl->window, new_title);
}

static long ctx_sdl_start_time = 0;

static void ctx_sdl_show_frame (CtxSDL *sdl, int block)
{
  CtxTiled *tiled = &sdl->tiled;
  CtxBackend *backend = (CtxBackend*)tiled;
  if (tiled->shown_cursor != backend->ctx->cursor)
  {
    tiled->shown_cursor = backend->ctx->cursor;
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
      usleep (500);
      count ++;
      if (count > 900)
      {
        tiled->shown_frame = tiled->render_frame;
        fprintf (stderr, "[drop]");
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
    int x = tiled->min_col * tiled->width/CTX_HASH_COLS;
    int y = tiled->min_row * tiled->height/CTX_HASH_ROWS;
    int x1 = (tiled->max_col+1) * tiled->width/CTX_HASH_COLS;
    int y1 = (tiled->max_row+1) * tiled->height/CTX_HASH_ROWS;

    if (_ctx_damage_control)
    {
      x = 0;
      y = 0;
      x1 = tiled->width;
      y1 = tiled->height;
    }

    int width = x1 - x;
    int height = y1 - y;
    tiled->min_row = 100;
    tiled->max_row = 0;
    tiled->min_col = 100;
    tiled->max_col = 0;

    SDL_Rect r = {x, y, width, height};
    SDL_UpdateTexture (sdl->texture, &r,
                      (void*)(tiled->pixels + y * tiled->width * 4 + x * 4),
                      tiled->width * 4);
    SDL_RenderClear (sdl->backend);
    SDL_RenderCopy (sdl->backend, sdl->texture, NULL, NULL);
    SDL_RenderPresent (sdl->backend);


  if (ctx_show_fps)
  {
    static char tmp_title[1024];
    static uint64_t prev_time = 0;
    uint64_t time = ctx_ticks ();
    float fps = 1000000.0f/  (time - ctx_sdl_start_time);
    float fps2 = 1000000.0f/  (time - prev_time);
    prev_time = time;
    static float fps_avg = 0.0f;

    if (time - prev_time < 1000 * 1000 * 0.05f)
    fps_avg = (fps_avg * 0.9f + fps2 *  0.1f);

    sprintf (tmp_title, "FPS: %.1f %.1f %.1f", (double)(fps2*0.75f+fps_avg*0.25f), (double)fps2, (double)fps);

    SDL_SetWindowTitle (sdl->window, tmp_title);
  }
  }
  tiled->shown_frame = tiled->render_frame;
}


void ctx_sdl_consume_events (Ctx *ctx)
{
  static float x = 0.0f;
  static float y = 0.0f;
  CtxBackend *backend = (void*)ctx->backend;
  CtxTiled    *tiled = (void*)backend;
  CtxSDL      *sdl = (void*)backend;
  SDL_Event event;
  //int got_events = 0;

  ctx_sdl_show_frame (sdl, 0);

  while (SDL_PollEvent (&event))
  {
    //got_events ++;
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
        x = event.motion.x;
        y = event.motion.y;
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
#if 1
      case SDL_TEXTINPUT:
    //  if (!active)
    //    break;
        if (!sdl->lctrl && !sdl->rctrl && !sdl->lalt 
           //&& ( (vt && vt_keyrepeat (vt) ) || (key_repeat==0) )
           )
          {
            const char *name = event.text.text;
            int keycode = 0;
            if (!strcmp (name, " ") ) { name = "space"; }
            if (name[0] && name[1] == 0)
            {
              keycode = name[0];
              keycode = toupper (keycode);
              switch (keycode)
              {
                case '.':  keycode = 190; break;
                case ';':  keycode = 59; break;
                case ',':  keycode = 188; break;
                case '/':  keycode = 191; break;
                case '\'': keycode = 222; break;
                case '`':  keycode = 192; break;
                case '[':  keycode = 219; break;
                case ']':  keycode = 221; break;
                case '\\': keycode = 220; break;
              }
            }
            ctx_key_press (ctx, keycode, name, 0);
          }
        break;
#endif
      case SDL_KEYDOWN:
        {
          char buf[32] = "";
          const char *name = buf;
          if (!event.key.repeat)
          {
            sdl->key_balance ++;
            sdl->key_repeat = 0;
          }
          else
          {
            sdl->key_repeat ++;
          }
          int keycode;
          name = ctx_sdl_keysym_to_name (event.key.keysym.sym, &keycode);

          ctx_key_down (ctx, keycode, name, 0);

          if (ctx_utf8_strlen (name) > 1 ||
              (ctx->events.modifier_state &
                                           (CTX_MODIFIER_STATE_CONTROL|
                                            CTX_MODIFIER_STATE_ALT))
              )
          if (strcmp(name, "space"))
            ctx_key_press (ctx, keycode, name, 0);
        }
        break;
      case SDL_KEYUP:
        {
           sdl->key_balance --;
           int keycode;
           const char *name = ctx_sdl_keysym_to_name (event.key.keysym.sym, &keycode);
           ctx_key_up (ctx, keycode, name, 0);
        }
        break;
      case SDL_QUIT:
        ctx_exit (ctx);
        break;
      case SDL_DROPFILE:
        ctx_pointer_drop (ctx, x, y, 0, 0, event.drop.file);
        break;
      case SDL_DROPTEXT:
        if (!strncmp ("file://", event.drop.file, 7))
          ctx_pointer_drop (ctx, x, y, 0, 0, event.drop.file + 7);
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
          ctx_sdl_show_frame (sdl, 1);
          int width = event.window.data1;
          int height = event.window.data2;
          SDL_DestroyTexture (sdl->texture);
          sdl->texture = SDL_CreateTexture (sdl->backend, SDL_PIXELFORMAT_ABGR8888,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
          ctx_free (tiled->pixels);
          tiled->pixels = ctx_calloc (width * height, 4);

          tiled->width  = width;
          tiled->height = height;
          ctx_set_size (backend->ctx, width, height);
          ctx_set_size (tiled->ctx_copy, width, height);
        }
        break;
    }
  }
}

static void ctx_sdl_set_clipboard (Ctx *ctx, const char *text)
{
  if (text)
    SDL_SetClipboardText (text);
}

static char *ctx_sdl_get_clipboard (Ctx *ctx)
{
  return SDL_GetClipboardText ();
}

inline static void ctx_sdl_start_frame (Ctx *ctx)
{
  CtxSDL  *sdl = (CtxSDL*)ctx->backend;
  ctx_sdl_show_frame (sdl, 1);
  ctx_sdl_start_time = ctx_ticks ();
}

void ctx_sdl_destroy (CtxSDL *sdl)
{
  if (sdl->texture)
    SDL_DestroyTexture (sdl->texture);
  if (sdl->backend)
    SDL_DestroyRenderer (sdl->backend);
  if (sdl->window)
  {
    SDL_DestroyWindow (sdl->window);
  }
  sdl->texture = NULL;
  sdl->backend = NULL;
  sdl->window = NULL;

  ctx_tiled_destroy ((CtxTiled*)sdl);
}

void ctx_sdl_set_fullscreen (Ctx *ctx, int val)
{
  CtxSDL *sdl = (void*)ctx->backend;

  if (val)
  {
    SDL_SetWindowFullscreen (sdl->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }
  else
  {
    SDL_SetWindowFullscreen (sdl->window, 0);
  }
  // XXX we're presuming success
  sdl->fullscreen = val;
}
int ctx_sdl_get_fullscreen (Ctx *ctx)
{
  CtxSDL *sdl = (void*)ctx->backend;
  return sdl->fullscreen;
}

Ctx *ctx_new_sdl (int width, int height)
{


#if CTX_RASTERIZER

  CtxSDL *sdl = (CtxSDL*)ctx_calloc (1, sizeof (CtxSDL));
  CtxTiled *tiled = (void*)sdl;
  CtxBackend *backend = (CtxBackend*)sdl;
#if CTX_BABL
  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);
#endif
  if (width <= 0 || height <= 0)
  {
    width  = 1920;
    height = 1080;
  }
  sdl->window = SDL_CreateWindow("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  //sdl->backend = SDL_CreateRenderer (sdl->window, -1, SDL_RENDERER_SOFTWARE);
  sdl->backend = SDL_CreateRenderer (sdl->window, -1, 0);
  if (!sdl->backend)
  {
     ctx_destroy (backend->ctx);
     ctx_free (sdl);
     return NULL;
  }
  sdl->fullscreen = 0;


  ctx_show_fps = getenv ("CTX_SHOW_FPS")!=NULL;

  sdl->texture = SDL_CreateTexture (sdl->backend,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);

  SDL_StartTextInput ();
  SDL_EnableScreenSaver ();
  SDL_GL_SetSwapInterval (1);

  backend->type = CTX_BACKEND_SDL;
  backend->ctx      = _ctx_new_drawlist (width, height);
  tiled->ctx_copy = _ctx_new_drawlist (width, height);
  tiled->width    = width;
  tiled->height   = height;
  tiled->cols     = 80;
  tiled->rows     = 20;
  ctx_set_backend (backend->ctx, sdl);
  ctx_set_backend (tiled->ctx_copy, sdl);

  tiled->pixels = (uint8_t*)ctx_malloc (width * height * 4);
  tiled->show_frame = (void*)ctx_sdl_show_frame;


  backend->set_windowtitle = (void*)ctx_sdl_set_title;
  backend->end_frame = ctx_tiled_end_frame;
  backend->process = (void*)ctx_drawlist_process;
  backend->start_frame = ctx_sdl_start_frame;
  backend->destroy = (void*)ctx_sdl_destroy;
  backend->consume_events = ctx_sdl_consume_events;

  backend->set_clipboard = ctx_sdl_set_clipboard;
  backend->get_clipboard = ctx_sdl_get_clipboard;

  for (int i = 0; i < _ctx_max_threads; i++)
  {
    tiled->host[i] = ctx_new_for_framebuffer (tiled->pixels,
                     tiled->width/CTX_HASH_COLS, tiled->height/CTX_HASH_ROWS,
                     tiled->width * 4, CTX_FORMAT_RGBA8);
    ctx_set_texture_source (tiled->host[i], backend->ctx);
  }

  ctx_set_texture_cache (tiled->ctx_copy, tiled->host[0]);
  ctx_set_texture_cache (backend->ctx, tiled->host[0]);
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

  return backend->ctx;
#else
  return NULL;
#endif
}
#endif
