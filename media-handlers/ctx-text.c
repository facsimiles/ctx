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

static int target_line_no = -1;

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


   if (event->type == CTX_DRAG_RELEASE)
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
   target_line_no = -1;
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

static int _ctx_str_count_lines (const char *str)
{
  int count = 0;
  for (const char *p = str; *p; p++)
    if (*p == '\n') count ++;
  return count;
}

static unsigned char *contents = NULL;
static uint8_t **lines = NULL;


static uint64_t prev_ticks = 0;

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

  n_lines = _ctx_str_count_lines (contents);
  lines = calloc (sizeof (char *) * (n_lines+1), 1);
  if(1){
    int line_no = 0;
    lines[line_no]=&contents[0];
    for (int i = 0; contents[i];i++)
    {
      if (contents[i]=='\n')
      {
        contents[i]=0;
        line_no++;
        lines[line_no]=&contents[i+1];
      }
    }
  }
  lines[n_lines]=NULL;

  ctx = ctx_new_ui (-1, -1);

  char eid[65]="";


  float cursor_translate = ctx_height (ctx) * 0.25;
  float shift_cursor_translate = 1;//cursor_translate / 8;

  prev_ticks = ctx_ticks ();

  while (!quit)
  {
    CtxEvent *event;


    uint64_t ticks = ctx_ticks ();
    if (target_line_no >=0 && target_line_no < line_no)
    {
       velocity = 0.0003;
    }
    else if (target_line_no >=0 && target_line_no > line_no)
    {
       velocity = -0.0003;
    }


      uint64_t delta = 0;
    if (fabsf(velocity) > 0.000000000000001)
    {
      dirty = 1;

      if (prev_ticks)
      {
        delta =  (ticks - prev_ticks);
        oy0 += velocity * delta;
      }

      convert_scroll_offset ();
      prev_ticks = ticks;
    }


    if (target_line_no != -1 && fabs(target_line_no - oy0/font_size - line_no) < 0.2)//velocity * delta)
    {
       velocity = 0.0;
       line_no = target_line_no;
       target_line_no = -1;
    }

    ctx_font (ctx, "mono");
    if (dirty)
    {
      float height = ctx_height (ctx);
      ctx_reset (ctx);
      ctx_save (ctx);
      ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
      ctx_listen (ctx, CTX_DRAG_MOTION|CTX_DRAG_RELEASE|CTX_DRAG_PRESS, image_drag, NULL, NULL);
      ctx_listen (ctx, CTX_SCROLL, image_scroll, NULL, NULL);
      ctx_gray (ctx, 1.0f);
      ctx_fill (ctx);
      //ctx_translate (ctx, ox0, oy0);

      ctx_font_size (ctx, font_size);
      ctx_gray (ctx, 0.0f);
      float y = 0.0;

      for (int i = line_no-1; i < n_lines && y < height + font_size * line_height * 2 ; i++)
      {
        if (i >=0)
        {
          float margin_left = font_size * 0.5;
          ctx_move_to (ctx, margin_left, oy0 + y);
          int highlight_line = line_no;
          if (target_line_no >= 0) highlight_line = target_line_no;
          if (i == highlight_line)
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
            if (target_line_no != -1)
              target_line_no++;
            else
              target_line_no = line_no + 1;
          }
          else if (!strcmp (event->string, "up"))
          {
            if (target_line_no != -1)
              target_line_no --;
            else
              target_line_no = line_no - 1;
          }
          else if (!strcmp (event->string, "page-down"))
          {
            if (target_line_no != -1)
              target_line_no+= 10;
            else
              target_line_no = line_no + 10;
          }
          else if (!strcmp (event->string, "page-up"))
          {
            if (target_line_no != -1)
              target_line_no -= 10;
            else
              target_line_no = line_no - 10;
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
