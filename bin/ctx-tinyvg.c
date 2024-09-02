#include "local.conf"
#if CTX_TINYVG

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ctx.h"

static int tvg_w = -1;
static int tvg_h = -1;

static float tvg_ox0 = 0;
static float tvg_oy0 = 0;
static float tvg_scale = 1.0;
static int tvg_dirty = 1;

static int tvg_auto_size = 1;

/****************************/

static void tvg_drag (CtxEvent *event, void *data0, void *data1)
{
   tvg_ox0 += event->delta_x / tvg_scale;
   tvg_oy0 += event->delta_y / tvg_scale;
   tvg_dirty++;
   tvg_auto_size = 0;
}

static void tvg_scroll (CtxEvent *event, void *data0, void *data1)
{
#if 0
   tvg_ox0 += event->delta_x / tvg_scale;
   tvg_oy0 += event->delta_y / tvg_scale;
   tvg_scale *= 1.1;
   tvg_dirty++;
#endif
   //auto_sisze = 0;
}

void ctx_handle_tvg (Ctx *ctx, const char *path)
{
  static int quit = 0;
  if (!path) return;
  path = realpath (path, NULL);

  uint8_t *data = NULL;
  long length = 0;
  ctx_get_contents (path, &data, &length);
  if (!data)
    return;

  ctx_tinyvg_get_size (data, length, &tvg_w, &tvg_h);

  tvg_auto_size = 1;
  tvg_scale  = ctx_width (ctx) * 1.0 / tvg_w;
  float tvg_scaleh = ctx_height (ctx) * 1.0 / tvg_h;

  if (tvg_scaleh < tvg_scale)
     tvg_scale = tvg_scaleh;

  // center
  tvg_ox0 = (ctx_width(ctx)/tvg_scale-(tvg_w)) / 2;
  tvg_oy0 = (ctx_height(ctx)/tvg_scale-(tvg_h)) / 2;


  float cursor_translate = ctx_height (ctx) * 0.25;
  float shift_cursor_translate = 1;//cursor_translate / 8;

  quit = 0;
  while (!quit)
  {
    CtxEvent *event;
    if (tvg_dirty)
    {
      if (tvg_auto_size)
      {
         tvg_scale = ctx_width (ctx) * 1.0 / tvg_w;
         tvg_scaleh = ctx_height (ctx) * 1.0 / tvg_h;
         if (tvg_scaleh < tvg_scale)
           tvg_scale = tvg_scaleh;
         tvg_ox0 = (ctx_width(ctx)/tvg_scale-(tvg_w)) / 2;
         tvg_oy0 = (ctx_height(ctx)/tvg_scale-(tvg_h)) / 2;
      }

      ctx_start_frame (ctx);
      ctx_save (ctx);
      ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_listen (ctx, CTX_DRAG_MOTION, tvg_drag, NULL, NULL);
      ctx_listen (ctx, CTX_SCROLL, tvg_scroll, NULL, NULL);
      ctx_gray (ctx, 0.0f);
      ctx_fill (ctx);
      //ctx_begin_path (ctx);
      //ctx_compositing_mode (ctx, CTX_COMPOSITE_COPY);
      //ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_save (ctx);

      ctx_scale (ctx, tvg_scale, tvg_scale);
      ctx_translate (ctx, tvg_ox0, tvg_oy0);

#if 0
      ctx_tinyvg_draw (ctx, data, length, 0);
#else
      static Ctx *dl = NULL;
      if (!dl)
      {
	dl = ctx_new_drawlist (tvg_w, tvg_h);
	ctx_tinyvg_draw (dl, data, length, 0);
      }
      ctx_render_ctx (dl, ctx);
#endif
#if 0
      ctx_rectangle (ctx, 0,0, tvg_w, tvg_h);
      ctx_rgba (ctx, 1,0,0,0.25);
      ctx_stroke (ctx);
      ctx_logo (ctx, tvg_w * 0.5, tvg_h * 0.5, tvg_h * 0.4);
#endif
      ctx_restore (ctx);

      ctx_restore (ctx);
      ctx_end_frame (ctx);
      tvg_dirty = 0;
    }
   
    while ((event = ctx_get_event (ctx)))
    {
      switch (event->type)
      {
        case CTX_KEY_PRESS:
          if (!strcmp (event->string, "q"))
            quit = 1;
          else if (!strcmp (event->string, "down"))
            tvg_oy0 -= cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "up"))
            tvg_oy0 += cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "left"))
            tvg_ox0 += cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "right"))
            tvg_ox0 -= cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "shift-down"))
            tvg_oy0 -= shift_cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "shift-up"))
            tvg_oy0 += shift_cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "shift-left"))
            tvg_ox0 += shift_cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "shift-right"))
            tvg_ox0 -= shift_cursor_translate / tvg_scale;
          else if (!strcmp (event->string, "f"))
          {
            tvg_scale = ctx_width (ctx) * 1.0 / tvg_w;
            tvg_scaleh = ctx_height (ctx) * 1.0 / tvg_h;
            if (tvg_scaleh < tvg_scale)
              tvg_scale = tvg_scaleh;
            tvg_ox0 = (ctx_width(ctx)/tvg_scale-(tvg_w)) / 2;
            tvg_oy0 = (ctx_height(ctx)/tvg_scale-(tvg_h)) / 2;
            tvg_auto_size = 1;
          }
          else if (!strcmp (event->string, "1"))
          {
            tvg_ox0 -=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 -=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_scale = 1.0;
            tvg_ox0 +=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 +=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_auto_size = 0;
          }
          else if (!strcmp (event->string, "2"))
          {
            tvg_ox0 -=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 -=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_scale = 2.0;
            tvg_ox0 +=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 +=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_auto_size = 0;
          }
          else if (!strcmp (event->string, "5"))
          {
            tvg_ox0 -=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 -=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_scale = 0.5;
            tvg_ox0 +=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 +=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_auto_size = 0;
          }
          else if (!strcmp (event->string, "+")||
                   !strcmp (event->string, "="))
          {
            tvg_ox0 -=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 -=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_scale *= 1.1;
            tvg_ox0 +=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 +=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_auto_size = 0;
          }
          else if (!strcmp (event->string, "-"))
          {
            tvg_ox0 -=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 -=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_scale /= 1.1;
            tvg_ox0 +=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 +=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_auto_size = 0;
          }
          else if (!strcmp (event->string, "."))
          {
            tvg_ox0 -=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 -=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_scale *= 1.01;
            tvg_ox0 +=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 +=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_auto_size = 0;
          }
          else if (!strcmp (event->string, ","))
          {
            tvg_ox0 -=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 -=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_scale /= 1.01;
            tvg_ox0 +=  (ctx_width (ctx) / 2) / tvg_scale;
            tvg_oy0 +=  (ctx_height (ctx) / 2) / tvg_scale;
            tvg_auto_size = 0;
          }
          tvg_dirty ++;
          break;
        default:
          break;
      }
    }
  }
}

#if CTX_BIN_BUNDLE
int ctx_tinyvg_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  Ctx *ctx;
  char *path = argv[1];
  if (strchr (path, ':') && (strchr(path,':')-path) < 6)
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  ctx = ctx_new (-1, -1, NULL);
  ctx_handle_tvg (ctx, path);
  ctx_destroy (ctx);
  return 0;
}

#else
#if CTX_BIN_BUNDLE
int ctx_tinyvg_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{return -1;}
#endif
