#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ctx.h"

static int stb_w = -1;
static int stb_h = -1;

static int image_smoothing = 1;

static float ox0 = 0;
static float oy0 = 0;
static float scale = 1.0;
static float angle = 0.0;
static int dirty = 1;

static int auto_size = 1;

static int rotating = 0;
static int repeat = 0;

/****************************/
static char *path = NULL;

static void image_drag (CtxEvent *event, void *data0, void *data1)
{
   ox0 += event->delta_x / scale;
   oy0 += event->delta_y / scale;
   dirty++;
   auto_size = 0;
}

static void image_scroll (CtxEvent *event, void *data0, void *data1)
{
#if 0
   ox0 += event->delta_x / scale;
   oy0 += event->delta_y / scale;
   scale *= 1.1;
   dirty++;
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

  ctx_tinyvg_get_size (data, length, &stb_w, &stb_h);

  auto_size = 1;
  scale  = ctx_width (ctx) * 1.0 / stb_w;
  float scaleh = ctx_height (ctx) * 1.0 / stb_h;

  if (scaleh < scale)
     scale = scaleh;

  // center
  ox0 = (ctx_width(ctx)/scale-(stb_w)) / 2;
  oy0 = (ctx_height(ctx)/scale-(stb_h)) / 2;


  float cursor_translate = ctx_height (ctx) * 0.25;
  float shift_cursor_translate = 1;//cursor_translate / 8;

  quit = 0;
  while (!quit)
  {
    CtxEvent *event;
    if (dirty)
    {
      if (auto_size)
      {
         scale = ctx_width (ctx) * 1.0 / stb_w;
         scaleh = ctx_height (ctx) * 1.0 / stb_h;
         if (scaleh < scale)
           scale = scaleh;
         ox0 = (ctx_width(ctx)/scale-(stb_w)) / 2;
         oy0 = (ctx_height(ctx)/scale-(stb_h)) / 2;
      }

      ctx_reset (ctx);
      ctx_save (ctx);
      ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_listen (ctx, CTX_DRAG_MOTION, image_drag, NULL, NULL);
      ctx_listen (ctx, CTX_SCROLL, image_scroll, NULL, NULL);
      ctx_gray (ctx, 0.0f);
      ctx_fill (ctx);
      //ctx_begin_path (ctx);
      if (image_smoothing == 0)
        ctx_image_smoothing (ctx, 0);
      if (repeat)
        ctx_extend (ctx, CTX_EXTEND_REPEAT);
      //ctx_compositing_mode (ctx, CTX_COMPOSITE_COPY);
      //ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_save (ctx);

      if (angle != 0.0f)
      {
          ctx_rotate (ctx, 0.08);
           ctx_apply_transform (ctx, 1, 0, 0.0,
                               0, 1, 0,
                               0.0, 0.001, 1.2);

      }

      ctx_scale (ctx, scale, scale);
      ctx_translate (ctx, ox0, oy0);


      ctx_tinyvg_draw (ctx, data, length, 0);
#if 0
      ctx_rectangle (ctx, 0,0, stb_w, stb_h);
      ctx_rgba (ctx, 1,0,0,0.25);
      ctx_stroke (ctx);
      ctx_logo (ctx, stb_w * 0.5, stb_h * 0.5, stb_h * 0.4);
#endif
      ctx_restore (ctx);

      ctx_restore (ctx);
      ctx_flush (ctx);
      dirty = 0;
      if (rotating)
      {
        angle += 0.001;
        dirty = 1;ctx_queue_draw (ctx);
        if (angle > 1.0) angle = 0.0;
      }
    }
   
    while ((event = ctx_get_event (ctx)))
    {
      switch (event->type)
      {
        case CTX_KEY_PRESS:
          if (!strcmp (event->string, "q"))
            quit = 1;
          else if (!strcmp (event->string, "i"))
            image_smoothing = !image_smoothing;
          else if (!strcmp (event->string, "down"))
            oy0 -= cursor_translate / scale;
          else if (!strcmp (event->string, "up"))
            oy0 += cursor_translate / scale;
          else if (!strcmp (event->string, "left"))
            ox0 += cursor_translate / scale;
          else if (!strcmp (event->string, "right"))
            ox0 -= cursor_translate / scale;
          else if (!strcmp (event->string, "shift-down"))
            oy0 -= shift_cursor_translate / scale;
          else if (!strcmp (event->string, "shift-up"))
            oy0 += shift_cursor_translate / scale;
          else if (!strcmp (event->string, "shift-left"))
            ox0 += shift_cursor_translate / scale;
          else if (!strcmp (event->string, "shift-right"))
            ox0 -= shift_cursor_translate / scale;
          else if (!strcmp (event->string, "f"))
          {
            scale = ctx_width (ctx) * 1.0 / stb_w;
            scaleh = ctx_height (ctx) * 1.0 / stb_h;
            if (scaleh < scale)
              scale = scaleh;
            ox0 = (ctx_width(ctx)/scale-(stb_w)) / 2;
            oy0 = (ctx_height(ctx)/scale-(stb_h)) / 2;
            auto_size = 1;
          }
          else if (!strcmp (event->string, "1"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale = 1.0;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "2"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale = 2.0;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "5"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale = 0.5;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "+")||
                   !strcmp (event->string, "="))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale *= 1.1;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "-"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale /= 1.1;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "."))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale *= 1.01;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, ","))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale /= 1.01;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "p"))
          {
            rotating = !rotating;
            ctx_queue_draw (event->ctx);
          }
          else if (!strcmp (event->string, "r"))
          {
            repeat = !repeat;
            ctx_queue_draw (event->ctx);
          }
          dirty ++;
          break;
        default:
          break;
      }
    }
  }
}

int ctx_tinyvg_main(int argc, char *argv[])
{
  Ctx *ctx;
  path = argv[1];
  if (strchr (path, ':') && (strchr(path,':')-path) < 6)
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  ctx = ctx_new (-1, -1, NULL);
  ctx_handle_tvg (ctx, path);
  ctx_free (ctx);
  return 0;
}
