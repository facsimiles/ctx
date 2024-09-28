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
   int           prev_fullscreen;

   Ctx          *ctx;

   int           width;
   int           height;

   char         *title;
   const char   *prev_title;


   int   clipboard_requested;
   char *clipboard;

   char *clipboard_pasted;

   CtxCursor shown_cursor;
};

static const char *ctx_sdl_keysym_to_name (unsigned int sym, int *r_keycode)
{
  static char buf[16]="";
  buf[ctx_unichar_to_utf8 (sym, (void*)buf)]=0;
  int scan_code = sym;
  const char *name = &buf[0];
   switch (sym)
   {
     case SDLK_RSHIFT: name="shift";scan_code = 16 ; break;
     case SDLK_LSHIFT: name="shift";scan_code = 16 ; break;
     case SDLK_LCTRL: name="control";scan_code = 17 ; break;
     case SDLK_RCTRL: name="control";scan_code = 17 ; break;
     case SDLK_LALT:  name="alt";scan_code = 18 ; break;
     case SDLK_RALT:  name="alt";scan_code = 18 ; break;
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

static void sdl_cb_consume_events (Ctx *ctx, void *user_data)
{
  static float x = 0.0f;
  static float y = 0.0f;
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
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
          sdl->width  = width;
          sdl->height = height;
        }
        break;
    }
  }
}

static void sdl_cb_set_pixels (Ctx *ctx, void *user_data, int x, int y, int w, int h, void *buf)
{
  SDL_Rect r = {x, y, w, h};
  SDL_UpdateTexture (((CtxSDLCb*)user_data)->texture, &r, buf, w * 4);
}

static void sdl_cb_renderer_idle (Ctx *ctx, void *user_data)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;

  if (sdl->clipboard_requested)
  {
    char *tmp = SDL_GetClipboardText ();
    sdl->clipboard = ctx_strdup (tmp);
    SDL_free (tmp);
    sdl->clipboard_requested = 0;
  }
  if (sdl->clipboard_pasted)
  {
    SDL_SetClipboardText (sdl->clipboard_pasted);
    ctx_free (sdl->clipboard_pasted);
    sdl->clipboard_pasted = NULL;
  }

  if (ctx_width(ctx) != sdl->width ||
      ctx_height(ctx) != sdl->height)
  {
    SDL_DestroyTexture (sdl->texture);
    sdl->texture = SDL_CreateTexture (sdl->backend, SDL_PIXELFORMAT_ABGR8888,
                          SDL_TEXTUREACCESS_STREAMING, sdl->width, sdl->height);
    ctx_set_size (ctx, sdl->width, sdl->height);
  }

  if (sdl->fullscreen != sdl->prev_fullscreen)
  {
    if (sdl->fullscreen)
    {
      SDL_SetWindowFullscreen (sdl->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    else
    {
      SDL_SetWindowFullscreen (sdl->window, 0);
    }
    sdl->prev_fullscreen = sdl->fullscreen;
  }

  if (sdl->prev_title != sdl->title)
  {
    SDL_SetWindowTitle (sdl->window, sdl->title);
    sdl->prev_title = sdl->title;
  }

}

static int sdl_cb_frame_done (Ctx *ctx, void *user_data)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  //SDL_RenderClear (sdl->backend);
  SDL_RenderCopy (sdl->backend, sdl->texture, NULL, NULL);
  SDL_RenderPresent (sdl->backend);

  sdl_cb_renderer_idle (ctx, user_data);

  if (sdl->shown_cursor != ctx->cursor)
  {
    sdl->shown_cursor = ctx->cursor;

    SDL_Cursor *new_cursor =  NULL;
    switch (sdl->shown_cursor)
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

  return 0;
}

static int sdl_cb_renderer_init (Ctx *ctx, void *user_data)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;

  sdl->window = SDL_CreateWindow("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		                 sdl->width, sdl->height, SDL_WINDOW_SHOWN |SDL_WINDOW_RESIZABLE);
  //sdl->backend = SDL_CreateRenderer (sdl->window, -1, SDL_RENDERER_SOFTWARE);
  sdl->backend = SDL_CreateRenderer (sdl->window, -1, 0);
  if (!sdl->backend)
  {
     ctx_free (sdl);
     return -1;
  }
  sdl->fullscreen = 0;

  sdl->texture = SDL_CreateTexture (sdl->backend,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        sdl->width, sdl->height);
  if (!sdl->texture)
  {
     ctx_free (sdl);
     return -1;
  }

  SDL_StartTextInput ();
  SDL_EnableScreenSaver ();
  SDL_GL_SetSwapInterval (1);

  return 0;
}

static void sdl_cb_renderer_stop (Ctx *ctx, void *user_data)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  if (sdl->texture)
    SDL_DestroyTexture (sdl->texture);
  if (sdl->backend)
    SDL_DestroyRenderer (sdl->backend);
  if (sdl->window)
    SDL_DestroyWindow (sdl->window);
  sdl->texture = NULL;
  sdl->backend = NULL;
  sdl->window  = NULL;
}

static void sdl_cb_set_fullscreen (Ctx *ctx, void *user_data, int fullscreen)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  sdl->fullscreen = fullscreen;
}

static int sdl_cb_get_fullscreen (Ctx *ctx, void *user_data)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  return sdl->fullscreen;
}

static char *sdl_cb_get_clipboard (Ctx *ctx, void *user_data)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
#if 0
  if (sdl->clipboard)
     ctx_free (sdl->clipboard);
  sdl->clipboard = NULL;
#endif
  sdl->clipboard_requested = 1;
  while (sdl->clipboard_requested)
	  usleep (1000);
  return sdl->clipboard?sdl->clipboard:"";
}

static void sdl_cb_set_clipboard (Ctx *ctx, void *user_data, const char *utf8)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  if (sdl->clipboard_pasted)
    {
       fprintf (stderr, "still contents in clipboard - leaking\n");
    }
  sdl->clipboard_pasted = ctx_strdup (utf8);
}

static void sdl_cb_windowtitle (Ctx *ctx, void *user_data, const char *utf8)
{
  CtxSDLCb *sdl = (CtxSDLCb*)user_data;
  if (!sdl->title || strcmp(sdl->title, utf8))
  {
    if (sdl->title)
       ctx_free (sdl->title);
    sdl->title = ctx_strdup (utf8);
  }
}

Ctx *ctx_new_sdl_cb (int width, int height)
{
  CtxSDLCb *sdl = (CtxSDLCb*)ctx_calloc (1, sizeof (CtxSDLCb));
  if (width <= 0 || height <= 0)
  {
    width  = 1920;
    height = 1080;
  }

  sdl->width = width;
  sdl->height = height;

  CtxCbConfig config = {
    .format         = CTX_FORMAT_RGBA8,
    .flags          = 0
	            | CTX_FLAG_HASH_CACHE 
	         // | CTX_FLAG_LOWFI
	            | CTX_FLAG_DOUBLE_BUFFER 
		 // | CTX_FLAG_POINTER
	           ,
    .memory_budget  = 1920 * 1080 * 4 / 3,
    .renderer_init  = sdl_cb_renderer_init,
    .renderer_idle  = sdl_cb_renderer_idle,
    .set_pixels     = sdl_cb_set_pixels,
    .update_fb      = sdl_cb_frame_done,
    .renderer_stop  = sdl_cb_renderer_stop,
    .consume_events = sdl_cb_consume_events,

    .get_fullscreen = sdl_cb_get_fullscreen,
    .set_fullscreen = sdl_cb_set_fullscreen,
    .get_clipboard  = sdl_cb_get_clipboard,
    .set_clipboard  = sdl_cb_set_clipboard,
    .windowtitle    = sdl_cb_windowtitle,
    .user_data      = sdl,
  };


  Ctx *ctx = ctx_new_cb (width, height, &config);
  if (!ctx)
    return NULL;
  sdl->ctx = ctx;
  return sdl->ctx;
}

#endif
