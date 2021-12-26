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

#include "stb_image_write.h"


void ctx_screenshot (Ctx *ctx, const char *output_path)
{
#if CTX_SCREENSHOT
  CtxTiled *tiled = (CtxTiled*)ctx->backend;
  if (!ctx_backend_is_tiled (ctx))
    return;

  // we rely on the same struxt layout XXX !
  for (int i = 0; i < tiled->width * tiled->height; i++)
  {
    int tmp = tiled->pixels[i*4];
    tiled->pixels[i*4] = tiled->pixels[i*4 + 2];
    tiled->pixels[i*4 + 2] = tmp;
  }

#if 1
  if (ctx_backend_type (ctx) != CTX_BACKEND_HEADLESS)
  for (int i = 0; i < tiled->width * tiled->height; i++)
  {
    int tmp = tiled->pixels[i*4];
    tiled->pixels[i*4] = tiled->pixels[i*4 + 2];
    tiled->pixels[i*4 + 2] = tmp;
  }
#endif
#ifdef INCLUDE_STB_IMAGE_WRITE_H
  stbi_write_png (output_path, tiled->width, tiled->height, 4, tiled->pixels, tiled->width*4);
#endif
#endif
}

int ctx_show_fps = 1;
void ctx_sdl_set_title (void *self, const char *new_title)
{
   CtxSDL *sdl = self;
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
      if (count > 300)
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
    SDL_RenderClear (sdl->backend);
    SDL_RenderCopy (sdl->backend, sdl->texture, NULL, NULL);
    SDL_RenderPresent (sdl->backend);


  if (ctx_show_fps)
  {
    static char tmp_title[1024];
    uint64_t time = ctx_ticks ();
    float fps = 1000000.0/  (time - ctx_sdl_start_time);
    static float fps_avg = 0.0f;
    fps_avg = (fps_avg * 0.9f + fps *  0.1f);
    sprintf (tmp_title, "FPS: %.1f ", (fps*0.75+fps_avg*0.25));

    SDL_SetWindowTitle (sdl->window, tmp_title);
  }
  }
  tiled->shown_frame = tiled->render_frame;
}

static const char *ctx_sdl_keysym_to_name (unsigned int sym, int *r_keycode)
{
  static char buf[16]="";
  buf[ctx_unichar_to_utf8 (sym, (void*)buf)]=0;
  int scan_code = sym;
  const char *name = &buf[0];
   switch (sym)
   {
     case SDLK_RSHIFT: scan_code = 16 ; break;
     case SDLK_LSHIFT: scan_code = 16 ; break;
     case SDLK_LCTRL: scan_code = 17 ; break;
     case SDLK_RCTRL: scan_code = 17 ; break;
     case SDLK_LALT:  scan_code = 18 ; break;
     case SDLK_RALT:  scan_code = 18 ; break;
     case SDLK_CAPSLOCK: name = "capslock"; scan_code = 20 ; break;
     //case SDLK_NUMLOCK: name = "numlock"; scan_code = 144 ; break;
     //case SDLK_SCROLLLOCK: name = "scrollock"; scan_code = 145 ; break;

     case SDLK_F1:     name = "F1"; scan_code = 112; break;
     case SDLK_F2:     name = "F2"; scan_code = 113; break;
     case SDLK_F3:     name = "F3"; scan_code = 114; break;
     case SDLK_F4:     name = "F4"; scan_code = 115; break;
     case SDLK_F5:     name = "F5"; scan_code = 116; break;
     case SDLK_F6:     name = "F6"; scan_code = 117; break;
     case SDLK_F7:     name = "F7"; scan_code = 118; break;
     case SDLK_F8:     name = "F8"; scan_code = 119; break;
     case SDLK_F9:     name = "F9"; scan_code = 120; break;
     case SDLK_F10:    name = "F10"; scan_code = 121; break;
     case SDLK_F11:    name = "F11"; scan_code = 122; break;
     case SDLK_F12:    name = "F12"; scan_code = 123; break;
     case SDLK_ESCAPE: name = "escape"; break;
     case SDLK_DOWN:   name = "down"; scan_code = 40; break;
     case SDLK_LEFT:   name = "left"; scan_code = 37; break;
     case SDLK_UP:     name = "up"; scan_code = 38;  break;
     case SDLK_RIGHT:  name = "right"; scan_code = 39; break;
     case SDLK_BACKSPACE: name = "backspace"; break;
     case SDLK_SPACE:  name = "space"; break;
     case SDLK_TAB:    name = "tab"; break;
     case SDLK_DELETE: name = "delete"; scan_code = 46; break;
     case SDLK_INSERT: name = "insert"; scan_code = 45; break;
     case SDLK_RETURN:
       //if (key_repeat == 0) // return never should repeat
       name = "return";   // on a DEC like terminal
       break;
     case SDLK_HOME:     name = "home"; scan_code = 36; break;
     case SDLK_END:      name = "end"; scan_code = 35; break;
     case SDLK_PAGEDOWN: name = "page-down"; scan_code = 34; break;
     case SDLK_PAGEUP:   name = "page-up"; scan_code = 33; break;
     case ',': scan_code = 188; break;
     case '.': scan_code = 190; break;
     case '/': scan_code = 191; break;
     case '`': scan_code = 192; break;
     case '[': scan_code = 219; break;
     case '\\': scan_code = 220; break;
     case ']':  scan_code = 221; break;
     case '\'': scan_code = 222; break;
     default:
       ;
   }
   if (sym >= 'a' && sym <='z') scan_code -= 32;
   if (r_keycode)
   {
     *r_keycode = scan_code;
   }
   return name;
}

void ctx_sdl_consume_events (Ctx *ctx)
{
  CtxBackend *backend = (void*)ctx->backend;
  CtxTiled    *tiled = (void*)backend;
  CtxSDL      *sdl = (void*)backend;
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
            //got_event = 1;
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
          switch (event.key.keysym.sym)
          {
            case SDLK_LSHIFT: sdl->lshift = 1; break;
            case SDLK_RSHIFT: sdl->rshift = 1; break;
            case SDLK_LCTRL:  sdl->lctrl = 1; break;
            case SDLK_LALT:   sdl->lalt = 1; break;
            case SDLK_RCTRL:  sdl->rctrl = 1; break;
          }
          if (sdl->lshift | sdl->rshift | sdl->lctrl | sdl->lalt | sdl->rctrl)
          {
            ctx->events.modifier_state ^= ~(CTX_MODIFIER_STATE_CONTROL|
                                            CTX_MODIFIER_STATE_ALT|
                                            CTX_MODIFIER_STATE_SHIFT);
            if (sdl->lshift | sdl->rshift)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_SHIFT;
            if (sdl->lctrl | sdl->rctrl)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_CONTROL;
            if (sdl->lalt)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_ALT;
          }
          int keycode;
          name = ctx_sdl_keysym_to_name (event.key.keysym.sym, &keycode);
          ctx_key_down (ctx, keycode, name, 0);

          if (strlen (name)
              &&(event.key.keysym.mod & (KMOD_CTRL) ||
                 event.key.keysym.mod & (KMOD_ALT) ||
                 ctx_utf8_strlen (name) >= 2))
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
               ctx_key_press (ctx, keycode, name, 0);
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
      case SDL_KEYUP:
        {
           sdl->key_balance --;
           switch (event.key.keysym.sym)
           {
             case SDLK_LSHIFT: sdl->lshift = 0; break;
             case SDLK_RSHIFT: sdl->rshift = 0; break;
             case SDLK_LCTRL: sdl->lctrl = 0; break;
             case SDLK_RCTRL: sdl->rctrl = 0; break;
             case SDLK_LALT:  sdl->lalt  = 0; break;
           }

          {
            ctx->events.modifier_state ^= ~(CTX_MODIFIER_STATE_CONTROL|
                                            CTX_MODIFIER_STATE_ALT|
                                            CTX_MODIFIER_STATE_SHIFT);
            if (sdl->lshift | sdl->rshift)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_SHIFT;
            if (sdl->lctrl | sdl->rctrl)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_CONTROL;
            if (sdl->lalt)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_ALT;
          }

           int keycode;
           const char *name = ctx_sdl_keysym_to_name (event.key.keysym.sym, &keycode);
           ctx_key_up (ctx, keycode, name, 0);
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
          sdl->texture = SDL_CreateTexture (sdl->backend, SDL_PIXELFORMAT_ABGR8888,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
          free (tiled->pixels);
          tiled->pixels = calloc (4, width * height);

          tiled->width  = width;
          tiled->height = height;
          ctx_set_size (backend->ctx, width, height);
          ctx_set_size (tiled->ctx_copy, width, height);
        }
        break;
    }
  }
}
#else
void ctx_screenshot (Ctx *ctx, const char *path)
{
}
#endif

#if CTX_SDL

static void ctx_sdl_set_clipboard (Ctx *ctx, const char *text)
{
  if (text)
    SDL_SetClipboardText (text);
}

static char *ctx_sdl_get_clipboard (Ctx *ctx)
{
  return SDL_GetClipboardText ();
}


inline static void ctx_sdl_reset (Ctx *ctx)
{
  CtxSDL  *sdl = (CtxSDL*)ctx->backend;
  ctx_sdl_show_frame (sdl, 1);
  ctx_sdl_start_time = ctx_ticks ();
}

void ctx_sdl_free (CtxSDL *sdl)
{

  if (sdl->texture)
  SDL_DestroyTexture (sdl->texture);
  if (sdl->backend)
  SDL_DestroyRenderer (sdl->backend);
  if (sdl->window)
  SDL_DestroyWindow (sdl->window);

  ctx_tiled_free ((CtxTiled*)sdl);
#if CTX_BABL
  babl_exit ();
#endif
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

  CtxSDL *sdl = (CtxSDL*)calloc (sizeof (CtxSDL), 1);
  CtxTiled *tiled = (void*)sdl;
  CtxBackend *backend = (CtxBackend*)sdl;

  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);
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
     ctx_free (backend->ctx);
     free (sdl);
     return NULL;
  }
#if CTX_BABL
  babl_init ();
#endif
  sdl->fullscreen = 0;


  ctx_show_fps = getenv ("CTX_SHOW_FPS")!=NULL;

  sdl->texture = SDL_CreateTexture (sdl->backend,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);

  SDL_StartTextInput ();
  SDL_EnableScreenSaver ();
  SDL_GL_SetSwapInterval (1);

  backend->ctx      = ctx_new ();
  backend->backend = "sdl";
  tiled->ctx_copy = ctx_new ();
  tiled->width    = width;
  tiled->height   = height;
  tiled->cols     = 80;
  tiled->rows     = 20;
  ctx_set_backend (backend->ctx, sdl);
  ctx_set_backend (tiled->ctx_copy, sdl);
  ctx_set_texture_cache (tiled->ctx_copy, backend->ctx);

  tiled->pixels = (uint8_t*)malloc (width * height * 4);
  tiled->show_frame = (void*)ctx_sdl_show_frame;

  ctx_set_size (backend->ctx,    width, height);
  ctx_set_size (tiled->ctx_copy, width, height);

  backend->set_windowtitle = (void*)ctx_sdl_set_title;
  backend->flush = ctx_tiled_flush;
  backend->reset = ctx_sdl_reset;
  backend->free  = (void*)ctx_sdl_free;
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
