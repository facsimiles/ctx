#include "ctx-split.h"

#if CTX_SDL

/**/

typedef struct _CtxSDLCb CtxSDLCb;
struct _CtxSDLCb
{
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

   Ctx          *ctx;

   int           width;
   int           height;
};

static CtxSDLCb *_ctx_sdl_cb = NULL;

static void ctx_sdl_cb_consume_events (Ctx *ctx)
{
  static float x = 0.0f;
  static float y = 0.0f;
  CtxSDLCb *sdl = _ctx_sdl_cb;
  SDL_Event event;

  while (SDL_PollEvent (&event))
  {
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
          ctx_pointer_press (ctx, event.tfinger.x * sdl->width, event.tfinger.y * sdl->height, 
          (event.tfinger.fingerId%10) + 4, 0);
        }
        }
        break;
      case SDL_FINGERUP:
        ctx_pointer_release (ctx, event.tfinger.x * sdl->width, event.tfinger.y * sdl->height,
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
          int width = event.window.data1;
          int height = event.window.data2;
          SDL_DestroyTexture (sdl->texture);
          sdl->texture = SDL_CreateTexture (sdl->backend, SDL_PIXELFORMAT_ABGR8888,
                          SDL_TEXTUREACCESS_STREAMING, width, height);

          sdl->width  = width;
          sdl->height = height;
          ctx_set_size (ctx, width, height);
//        ctx_set_size (tiled->ctx_copy, width, height);
        }
        break;
    }
  }
}

static void sdl_set_pixels (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  if (ctx != sdl->ctx)
    printf ("!@#!@#\n");

  SDL_Rect r = {x, y, w, h};
  SDL_UpdateTexture (sdl->texture, &r, buf, w * 4);
}

static int sdl_frame_done (Ctx *ctx, void *user_data)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  //SDL_RenderClear (sdl->backend);
  SDL_RenderCopy (sdl->backend, sdl->texture, NULL, NULL);
  SDL_RenderPresent (sdl->backend);
  return 0;
}

Ctx *ctx_new_sdl_cb (int width, int height)
{
  CtxSDLCb *sdl = (CtxSDLCb*)ctx_calloc (1, sizeof (CtxSDLCb));
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
     ctx_free (sdl);
     return NULL;
  }
  sdl->fullscreen = 0;

  ctx_show_fps = getenv ("CTX_SHOW_FPS")!=NULL;

  sdl->texture = SDL_CreateTexture (sdl->backend,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);
  if (!sdl->texture)
  {
     ctx_free (sdl);
     return NULL;
  }

  SDL_StartTextInput ();
  SDL_EnableScreenSaver ();
  SDL_GL_SetSwapInterval (1);

  sdl->width = width;
  sdl->height = height;
  _ctx_sdl_cb = sdl;
  sdl->ctx = ctx_new_cb (width, height, CTX_FORMAT_RGBA8, sdl_set_pixels, sdl, sdl_frame_done, sdl, width * height * 4, NULL, 
		  CTX_FLAG_HASH_CACHE);

  CtxBackend *backend = ctx_get_backend (sdl->ctx);
  backend->consume_events = ctx_sdl_cb_consume_events;

  return sdl->ctx;
}

#endif
