#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "ctx.h"

static int quit = 0;

static float ox0 = 0;
static float oy0 = 0;
static float scale = 1.0;
static int dirty = 1;

/****************************/
static char *path = NULL;

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

static unsigned char *contents = NULL;

int ctx_text_main(int argc, char *argv[])
{
  Ctx *ctx;
  path = argv[1];

  if (strchr (path, ':') && (strchr(path,':')-path) < 6)
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  if (path) ctx_get_contents (path, &contents, NULL);

  if (!contents) return 1;

  ctx = ctx_new_ui (-1, -1);

  char eid[65]="";

  float cursor_translate = ctx_height (ctx) * 0.25;
  float shift_cursor_translate = 1;//cursor_translate / 8;

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
      ctx_gray (ctx, 1.0f);
      ctx_fill (ctx);
      //ctx_translate (ctx, ox0, oy0);

      ctx_font_size (ctx, 60);
      ctx_gray (ctx, 0.0f);
      ctx_move_to (ctx, ox0, oy0);
      ctx_text (ctx, (char*)contents);

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
        default:
          break;
      }
    }
  }
  ctx_free (ctx);
  return 0;
}
