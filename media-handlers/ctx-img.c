#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "ctx.h"

//#define STBI_NO_STDIO
#include "stb_image.h"


static int stb_w = -1;
static int stb_h = -1;

static int image_smoothing = 1;

static float ox0 = 0;
static float oy0 = 0;
static float scale = 1.0;
static int dirty = 1;

/****************************/
static char *path = NULL;

static uint8_t *stb_pixels = NULL;
static int stb_components = 0;

static void image_drag (CtxEvent *event, void *data0, void *data1)
{
   ox0 += event->delta_x / scale;
   oy0 += event->delta_y / scale;
   dirty++;
}

static void image_scroll (CtxEvent *event, void *data0, void *data1)
{
#if 0
   ox0 += event->delta_x / scale;
   oy0 += event->delta_y / scale;
   scale *= 1.1;
   dirty++;
#endif
}

int ctx_handle_img (Ctx *ctx, const char *path)
{
  static int quit = 0;
  if (!path) return 1;

  if (path) stb_pixels = stbi_load (path, &stb_w, &stb_h, &stb_components, 4);

  if (!stb_pixels) return 1;


  scale  = ctx_width (ctx) * 1.0 / stb_w;
  float scaleh = ctx_height (ctx) * 1.0 / stb_h;

  if (scaleh < scale)
     scale = scaleh;

  char eid[65]="";

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
      ctx_compositing_mode (ctx, CTX_COMPOSITE_COPY);

#if 1
      ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_save (ctx);
      ctx_scale (ctx, scale, scale);
      ctx_translate (ctx, ox0, oy0);
      if (eid[0])
        ctx_texture (ctx, eid, 0.0f, 0.0f);
      else
        ctx_define_texture (ctx, NULL, stb_w, stb_h, stb_w * 4, CTX_FORMAT_RGBA8,
                            stb_pixels, eid);
      ctx_fill (ctx);
      ctx_restore (ctx);
#else
      if (!eid[0])
      {
        ctx_define_texture (ctx, NULL, stb_w, stb_h, stb_w * 4, CTX_FORMAT_RGBA8,
                            stb_pixels, eid);
      }
      if (eid[0])
      {
        ctx_draw_texture (ctx, eid, ox0 * scale, oy0 * scale, stb_w * scale, stb_h *  scale);
        //ctx_rectangle (ctx, ox0 * scale, oy0 * scale, stb_w * scale, stb_h *  scale);
        ctx_rgba (ctx, 0.5,1.0,0.0,0.5);
        ctx_fill (ctx);

      }
#endif
      ctx_restore (ctx);
      ctx_flush (ctx);
      dirty = 0;
    }
    else
      usleep (1000);
   
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
          }
          else if (!strcmp (event->string, "1"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale = 1.0;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
          }
          else if (!strcmp (event->string, "2"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale = 2.0;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
          }
          else if (!strcmp (event->string, "5"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale = 0.5;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
          }
          else if (!strcmp (event->string, "+")||
                   !strcmp (event->string, "="))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale *= 1.1;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
          }
          else if (!strcmp (event->string, "-"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale /= 1.1;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
          }
          else if (!strcmp (event->string, "."))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale *= 1.01;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
          }
          else if (!strcmp (event->string, ","))
          {
            ox0 -=  (ctx_width (ctx) / 2) / scale;
            oy0 -=  (ctx_height (ctx) / 2) / scale;
            scale /= 1.01;
            ox0 +=  (ctx_width (ctx) / 2) / scale;
            oy0 +=  (ctx_height (ctx) / 2) / scale;
          }
          dirty ++;
          break;
        default:
          break;
      }
    }
  }
}

int ctx_img_main(int argc, char *argv[])
{
  Ctx *ctx;
  path = argv[1];

  if (strchr (path, ':') && (strchr(path,':')-path) < 6)
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  ctx = ctx_new_ui (-1, -1);
  ctx_handle_img (ctx, path);
  ctx_free (ctx);
  return 0;
}

