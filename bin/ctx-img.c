#if CTX_STB_IMAGE

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
static float img_scale = 1.0;
static float angle = 0.0;
static int img_dirty = 1;

static int auto_size = 1;

static int rotating = 0;
static int repeat = 0;

/****************************/
static char *path = NULL;

static uint8_t *stb_pixels = NULL;
static int stb_components = 0;

static void image_drag (CtxEvent *event, void *data0, void *data1)
{
   ox0 += event->delta_x / img_scale;
   oy0 += event->delta_y / img_scale;
   img_dirty++;
   auto_size = 0;
}

static void image_scroll (CtxEvent *event, void *data0, void *data1)
{
   Ctx *ctx = event->ctx;
   ox0 -=  (ctx_width (ctx) / 2) / img_scale;
   oy0 -=  (ctx_height (ctx) / 2) / img_scale;

   if (event->scroll_direction == -1)
      img_scale *= 1.1;
   else
      img_scale /= 1.1;
   ox0 +=  (ctx_width (ctx) / 2) / img_scale;
   oy0 +=  (ctx_height (ctx) / 2) / img_scale;
   auto_size = 0;
   img_dirty=1;ctx_queue_draw (ctx);
#if 0
   ox0 += event->delta_x / img_scale;
   oy0 += event->delta_y / img_scale;
   img_scale *= 1.1;
   img_dirty++;
#endif
   //auto_sisze = 0;
}

void ctx_handle_img (Ctx *ctx, const char *path)
{
  static int quit = 0;
  if (!path) return;

  if (path) stb_pixels = stbi_load (path, &stb_w, &stb_h, &stb_components, 4);

  if (!stb_pixels) return;

  auto_size = 1;
  img_scale  = ctx_width (ctx) * 1.0 / stb_w;
  float scaleh = ctx_height (ctx) * 1.0 / stb_h;

  if (scaleh < img_scale)
     img_scale = scaleh;

  char eid[65]="";

  // center
  ox0 = (ctx_width(ctx)/img_scale-(stb_w)) / 2;
  oy0 = (ctx_height(ctx)/img_scale-(stb_h)) / 2;


  float cursor_translate = ctx_height (ctx) * 0.25;
  float shift_cursor_translate = 1;//cursor_translate / 8;

  quit = 0;
  while (!quit)
  {
    CtxEvent *event;
    if (img_dirty)
    {
      if (auto_size)
      {
         img_scale = ctx_width (ctx) * 1.0 / stb_w;
         scaleh = ctx_height (ctx) * 1.0 / stb_h;
         if (scaleh < img_scale)
           img_scale = scaleh;
         ox0 = (ctx_width(ctx)/img_scale-(stb_w)) / 2;
         oy0 = (ctx_height(ctx)/img_scale-(stb_h)) / 2;
      }

      ctx_start_frame (ctx);
      ctx_save (ctx);
      ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_listen (ctx, CTX_DRAG_MOTION, image_drag, NULL, NULL);
      ctx_listen (ctx, CTX_SCROLL, image_scroll, NULL, NULL);
      ctx_gray (ctx, 0.0f);
      ctx_fill (ctx);
      //ctx_reset_path (ctx);
      if (image_smoothing == 0)
        ctx_image_smoothing (ctx, 0);
      if (repeat)
        ctx_extend (ctx, CTX_EXTEND_REPEAT);
      ctx_compositing_mode (ctx, CTX_COMPOSITE_COPY);

#if 1
      ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_save (ctx);

      if (angle != 0.0f)
      {
          ctx_rotate (ctx, 0.08);
           ctx_apply_transform (ctx, 1, 0, 0.0,
                               0, 1, 0,
                               0.0, 0.001, 1.2);

      }

      ctx_scale (ctx, img_scale, img_scale);
      ctx_translate (ctx, ox0, oy0);



      if (eid[0])
        ctx_texture (ctx, eid, 0.0f, 0.0f);
      else
        ctx_define_texture (ctx, NULL, stb_w, stb_h, stb_w * 4, CTX_FORMAT_RGBA8,
                            stb_pixels, eid);
      ctx_fill (ctx);
#if 0
      ctx_rectangle (ctx, 0,0, stb_w, stb_h);
      ctx_rgba (ctx, 1,0,0,0.25);
      ctx_stroke (ctx);
      ctx_logo (ctx, stb_w * 0.5, stb_h * 0.5, stb_h * 0.4);
#endif
      ctx_restore (ctx);

#else
      if (!eid[0])
      {
        ctx_define_texture (ctx, NULL, stb_w, stb_h, stb_w * 4, CTX_FORMAT_RGBA8,
                            stb_pixels, eid);
      }
      if (eid[0])
      {
        ctx_draw_texture (ctx, eid, ox0 * img_scale, oy0 * img_scale, stb_w * img_scale, stb_h *  img_scale);
        //ctx_rectangle (ctx, ox0 * img_scale, oy0 * img_scale, stb_w * img_scale, stb_h *  img_scale);
        ctx_rgba (ctx, 0.5,1.0,0.0,0.5);
        ctx_fill (ctx);

      }
#endif
      ctx_restore (ctx);
      ctx_end_frame (ctx);
      img_dirty = 0;
      if (rotating)
      {
        angle += 0.001;
        img_dirty = 1;ctx_queue_draw (ctx);
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
            oy0 -= cursor_translate / img_scale;
          else if (!strcmp (event->string, "up"))
            oy0 += cursor_translate / img_scale;
          else if (!strcmp (event->string, "left"))
            ox0 += cursor_translate / img_scale;
          else if (!strcmp (event->string, "right"))
            ox0 -= cursor_translate / img_scale;
          else if (!strcmp (event->string, "shift-down"))
            oy0 -= shift_cursor_translate / img_scale;
          else if (!strcmp (event->string, "shift-up"))
            oy0 += shift_cursor_translate / img_scale;
          else if (!strcmp (event->string, "shift-left"))
            ox0 += shift_cursor_translate / img_scale;
          else if (!strcmp (event->string, "shift-right"))
            ox0 -= shift_cursor_translate / img_scale;
          else if (!strcmp (event->string, "f"))
          {
            img_scale = ctx_width (ctx) * 1.0 / stb_w;
            scaleh = ctx_height (ctx) * 1.0 / stb_h;
            if (scaleh < img_scale)
              img_scale = scaleh;
            ox0 = (ctx_width(ctx)/img_scale-(stb_w)) / 2;
            oy0 = (ctx_height(ctx)/img_scale-(stb_h)) / 2;
            auto_size = 1;
          }
          else if (!strcmp (event->string, "1"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / img_scale;
            oy0 -=  (ctx_height (ctx) / 2) / img_scale;
            img_scale = 1.0;
            ox0 +=  (ctx_width (ctx) / 2) / img_scale;
            oy0 +=  (ctx_height (ctx) / 2) / img_scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "2"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / img_scale;
            oy0 -=  (ctx_height (ctx) / 2) / img_scale;
            img_scale = 2.0;
            ox0 +=  (ctx_width (ctx) / 2) / img_scale;
            oy0 +=  (ctx_height (ctx) / 2) / img_scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "5"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / img_scale;
            oy0 -=  (ctx_height (ctx) / 2) / img_scale;
            img_scale = 0.5;
            ox0 +=  (ctx_width (ctx) / 2) / img_scale;
            oy0 +=  (ctx_height (ctx) / 2) / img_scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "+")||
                   !strcmp (event->string, "="))
          {
            ox0 -=  (ctx_width (ctx) / 2) / img_scale;
            oy0 -=  (ctx_height (ctx) / 2) / img_scale;
            img_scale *= 1.1;
            ox0 +=  (ctx_width (ctx) / 2) / img_scale;
            oy0 +=  (ctx_height (ctx) / 2) / img_scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "-"))
          {
            ox0 -=  (ctx_width (ctx) / 2) / img_scale;
            oy0 -=  (ctx_height (ctx) / 2) / img_scale;
            img_scale /= 1.1;
            ox0 +=  (ctx_width (ctx) / 2) / img_scale;
            oy0 +=  (ctx_height (ctx) / 2) / img_scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, "."))
          {
            ox0 -=  (ctx_width (ctx) / 2) / img_scale;
            oy0 -=  (ctx_height (ctx) / 2) / img_scale;
            img_scale *= 1.01;
            ox0 +=  (ctx_width (ctx) / 2) / img_scale;
            oy0 +=  (ctx_height (ctx) / 2) / img_scale;
            auto_size = 0;
          }
          else if (!strcmp (event->string, ","))
          {
            ox0 -=  (ctx_width (ctx) / 2) / img_scale;
            oy0 -=  (ctx_height (ctx) / 2) / img_scale;
            img_scale /= 1.01;
            ox0 +=  (ctx_width (ctx) / 2) / img_scale;
            oy0 +=  (ctx_height (ctx) / 2) / img_scale;
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
          img_dirty ++;
          break;
        default:
          break;
      }
    }
  }
}

#if CTX_BIN_BUNDLE
int ctx_img_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
  Ctx *ctx;
  path = argv[1];
  if (!argv[1]) return -1;

  if (strchr (path, ':') && (strchr(path,':')-path) < 6)
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  ctx = ctx_new (-1, -1, NULL);
  ctx_handle_img (ctx, path);
  ctx_destroy (ctx);
  return 0;
}


#else
#if CTX_BIN_BUNDLE
int ctx_img_main (int argc, char *argv[])
#else
int main (int argc, char *argv[])
#endif
{return -1;}

#endif
