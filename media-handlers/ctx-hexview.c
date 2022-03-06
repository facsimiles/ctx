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
static float font_size = 50.0;
float line_height = 1.2;
int n_lines = 0;

static int cursor_line_no = 0;

static float velocity = 0.0000;


/****************************/
static char *path = NULL;

int line_no = 0;

static void convert_scroll_offset (void)
{
   while (oy0 < -font_size * line_height)
   {
      oy0 += font_size * line_height;
      line_no ++;
      if (line_no >= n_lines-1) line_no = n_lines-1;
   }
   while (oy0 > font_size * line_height)
   {
      oy0 -= font_size * line_height;
      line_no --;
      if (line_no < 0) line_no = 0;
   }
}

static void image_drag (CtxEvent *event, void *data0, void *data1)
{
   static uint64_t prev_event_ticks = 0;
   uint64_t ticks = ctx_ticks ();


   if (event->type == CTX_DRAG_PRESS)
   {
   cursor_line_no = event->y / (font_size * line_height) + line_no;
   }
   else if (event->type == CTX_DRAG_RELEASE)
   {
     if (prev_event_ticks)
        velocity = event->delta_y * 1.0 / (ticks - prev_event_ticks);
   }
   else
   {
     oy0 += event->delta_y / scale;
     velocity = 0;
   }


   prev_event_ticks = ticks;

   dirty++;
   convert_scroll_offset ();
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

static uint64_t prev_ticks = 0;

int ctx_hexview_main(int argc, char *argv[])
{
  Ctx *ctx;
  path = argv[1];

  if (strchr (path, ':') && (strchr(path,':')-path) < 6)
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  long length;
  if (path) ctx_get_contents (path, &contents, &length);

  if (!contents) return 1;

  ctx = ctx_new (-1, -1, NULL);

  //char eid[65]="";

  float cursor_translate = ctx_height (ctx) * 0.25;
  float shift_cursor_translate = 1;//cursor_translate / 8;

  prev_ticks = ctx_ticks ();

  while (!quit)
  {
    CtxEvent *event;

    uint64_t ticks = ctx_ticks ();

    uint64_t delta = 0;
    if (fabsf(velocity) > 0.000000000000001)
    {
      dirty = 1;

      if (prev_ticks)
      {
        delta =  (ticks - prev_ticks);
        if (delta < 10000)
        {
          oy0 += velocity * delta;
          convert_scroll_offset ();
        }
      }

      prev_ticks = ticks;
    }

    ctx_font (ctx, "mono");
    if (dirty)
    {
      float height = ctx_height (ctx);
      ctx_start_frame (ctx);
      ctx_save (ctx);
      ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_listen (ctx, CTX_DRAG_MOTION|CTX_DRAG_RELEASE|CTX_DRAG_PRESS, image_drag, NULL, NULL);
      ctx_listen (ctx, CTX_SCROLL, image_scroll, NULL, NULL);
      ctx_gray (ctx, 1.0f);
      ctx_fill (ctx);
      //ctx_translate (ctx, ox0, oy0);

      ctx_font_size (ctx, font_size);
      ctx_gray (ctx, 0.0f);
#if 0
      float y = 0.0;
      for (int i = line_no-1; i < n_lines && y < height + font_size * line_height * 2 ; i++)
      {
        if (i >=0)
        {
          float margin_left = font_size * 0.5;
          ctx_move_to (ctx, margin_left, (oy0 + y));
          if (i == cursor_line_no)
            {
               ctx_save (ctx);
               ctx_rgb (ctx, 1,0,0);
               ctx_text (ctx, (char*)lines[i]);
               ctx_restore (ctx);
            }
          else
          {
            ctx_text (ctx, (char*)lines[i]);
          }
        }
        y += font_size * line_height;
      }
#endif
      float x = 0.0;
      float y = 0.0;
      char *hex_alphabet = "0123456789ABCDEF";
      for (int i = 0; i < length && y < ctx_height (ctx); i++)
      {
         uint8_t val = contents[i];
         char str[3] = {hex_alphabet[val/16],
                        hex_alphabet[val%16],
                        0};
         ctx_move_to (ctx, x, y + font_size);
         ctx_text (ctx, str);
         x += font_size;
         if (x + font_size > ctx_width (ctx))
         {
           x = 0;
           y += font_size;
         }
      }
      ctx_move_to (ctx, 0,20);
      char buf[23];
      static int frame = 0;
      sprintf (buf, "%i", frame++);
      ctx_text (ctx, buf);

      ctx_restore (ctx);
      ctx_end_frame (ctx);

      dirty = 0;
      if (1)
      {
      ctx_queue_draw (ctx);
      dirty = 1;
      }

      float target_y = 0.0 + (cursor_line_no + 1 - line_no) * line_height * font_size;

      if (target_y < font_size * line_height * 2)
      {
        velocity = +0.008;
      }
      else if (target_y > height - font_size * line_height * 2)
      {
        velocity = -0.008;
      }
      else
        velocity = 0.000;
      usleep (1000);
    }
    else
      usleep (1000);
   
    while ((event = ctx_get_event (ctx)))
    {
      switch (event->type)
      {
        case CTX_KEY_PRESS:
                dirty++;
          if (!strcmp (event->string, "q"))
            quit = 1;
          else if (!strcmp (event->string, "+"))
            font_size += 1;
          else if (!strcmp (event->string, "="))
            font_size += 1;
          else if (!strcmp (event->string, "-"))
            font_size -= 1;
          else if (!strcmp (event->string, "down"))
          {
            cursor_line_no ++;
            if (cursor_line_no > n_lines-1)
              cursor_line_no = n_lines-1;
          }
          else if (!strcmp (event->string, "up"))
          {
            cursor_line_no --;
            if (cursor_line_no < 0)
              cursor_line_no = 0;
          }
          else if (!strcmp (event->string, "page-down"))
          {
            cursor_line_no +=10;
            if (cursor_line_no > n_lines-1)
              cursor_line_no = n_lines-1;
          }
          else if (!strcmp (event->string, "page-up"))
          {
            cursor_line_no -=10;
            if (cursor_line_no < 0)
              cursor_line_no = 0;
          }
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
