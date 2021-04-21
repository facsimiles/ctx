/*
 * Copyright 2019, 2021 Øyvind Kolås <pippin@gimp.org>
 *
 *  plays a gif animation on loop using stb_image, originally written as
 *  a l0dable for card10 needs cleanup/dead-code removal
 *
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ctx.h"
#if 0
static const char *extensions[]={
   ".gif", ".GIF", NULL
};
#endif
static float slide_seconds = 5.0f;
static int   slideshow = 0;
static int   fit = 0;

#define STBI_NO_STDIO
//#define STBI_ONLY_PNG
//#define STBI_ONLY_JPEG
#define STBI_ONLY_GIF
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* we need more defs, copied from internals */
typedef uint32_t stbi__uint32;
typedef int16_t stbi__int16;


typedef struct
{
   stbi__uint32 img_x, img_y;
   int img_n, img_out_n;

   stbi_io_callbacks io;
   void *io_user_data;

   int read_from_callbacks;
   int buflen;
   stbi_uc buffer_start[128];
   int callback_already_read;

   stbi_uc *img_buffer, *img_buffer_end;
   stbi_uc *img_buffer_original, *img_buffer_original_end;
} stbi__context;

typedef struct
{
   stbi__int16 prefix;
   stbi_uc first;
   stbi_uc suffix;
} stbi__gif_lzw;


typedef struct
{
   int w,h;
   stbi_uc *out;                 // output buffer (always 4 components)
   stbi_uc *background;          // The current "background" as far as a gif is concerned
   stbi_uc *history;
   int flags, bgindex, ratio, transparent, eflags;
   stbi_uc  pal[256][4];
   stbi_uc lpal[256][4];
   stbi__gif_lzw codes[8192];
   stbi_uc *color_table;
   int parse, step;
   int lflags;
   int start_x, start_y;
   int max_x, max_y;
   int cur_x, cur_y;
   int line_size;
   int delay;
} stbi__gif;

stbi_uc *stbi__gif_load_next(stbi__context *s, stbi__gif *g, int *comp, int req_comp, stbi_uc *two_back);


/////////////////////////////////////////////////////



static int img_no = 0;
static int slide_start = 0;

static int quit = 0;
static int file_size = 0;

static int read_cb (void *user, char *data, int size)
{
  FILE *file = user;
  return fread (data, 1, size, file);
}
static void skip_cb (void *user, int n)
{
  FILE *file = user;
  long pos = ftell (file);
  fseek (file, pos + n, SEEK_SET);
}

static int eof_cb (void *user)
{
  FILE *file = user;
  long pos = ftell (file);
  if (pos >= file_size)
    return 1;
  return 0;
}

static stbi_io_callbacks clbk = {read_cb, skip_cb, eof_cb};



static char *stb_active_path = NULL;
static uint8_t *stb_buffer = NULL;
static int stb_w = -1;
static int stb_h = -1;
static int stb_c = 0;

static void stb_clear_cache (void)
{
  if (stb_active_path)
    free (stb_active_path);
  stb_active_path = NULL;
  if (stb_buffer)
    free (stb_buffer);
  stb_buffer = NULL;
}

void epicfb_stb (int         x0,
                 int         y0,
                 int         w,
                 int         h,
                 uint8_t     opacity,
                 int         filter,
                 int         cache,
                 const char *path)
{
  uint8_t *buffer = NULL;
  int imgw = 0, imgh = 0, c = 0;

  if (!cache)
  {
    stb_clear_cache ();
  }

  if (cache)
  {
    if (stb_active_path)
    {
     if (!strcmp (stb_active_path, path))
      {
        buffer = stb_buffer;
        stb_buffer = NULL;
        free (stb_active_path);
        stb_active_path = NULL;
        imgw = stb_w;
        imgh = stb_h;
        c  = stb_c;
      }
      else
      {
        stb_clear_cache ();
      }
    }
  }

  if (!buffer)
  {
    FILE *f = fopen (path, "rb");
    if (!f)
      return;
    fseek (f, 0, SEEK_END);
    file_size = ftell (f);
    fseek (f, 0, SEEK_SET);
    buffer  = stbi_load_from_callbacks (&clbk, (void*)f, &imgw, &imgh, &c, 0);

    fclose (f);
    if (!buffer)
    {
      return;
    }
  }

  // XXX blit!

  if (cache)
  {
    stb_active_path = strdup (path);
    stb_buffer = buffer;
    stb_w = imgw;
    stb_h = imgh;
    stb_c = c;
  }
  else
  {
    free (buffer);
  }
}

#ifdef STBI_ONLY_GIF

static stbi__context s;
unsigned char *result = 0;

static FILE *gf = NULL;

char    *gif_active_path = NULL;
uint8_t *gifbuf = NULL;
  stbi__gif g;

int frameno = 0;

static void epicfb_stb_gif_stop (void);

void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c, void *user);


static int epicfb_stb_gif_init (const char *path)
{
  if (gif_active_path)
  {
    epicfb_stb_gif_stop ();
  }
  frameno = 0;
  gf = fopen (path, "rb");
  if (!gf)
    return -1;
  stb_w = -1;
  stb_h = -1;

  fseek (gf, 0, SEEK_END);
  file_size = ftell (gf);
  fseek (gf, 0, SEEK_SET);
  memset (&s, 0, sizeof (s));
  memset (&g, 0, sizeof (g));
  stbi__start_callbacks (&s, &clbk, (void*)gf);

  gif_active_path = strdup (path);
  return 0;
}

static int epicfb_stb_gif_load_frame ()
{
  int c;
  if (!gif_active_path)
    return -1;

  gifbuf = stbi__gif_load_next (&s, &g, &c, 4, 0);
  if (gifbuf == (uint8_t*) &s)
    {
      gifbuf = NULL;
      epicfb_stb_gif_stop ();
      return -1;
    }
  if (stb_w < 0) stb_w = g.w;
  if (stb_h < 0) stb_h = g.h;

  frameno++;

  return g.delay;
}

static void epicfb_stb_gif_stop (void)
{
  if (!gif_active_path)
    return;
  free (gif_active_path);
  gif_active_path = NULL;
  fclose (gf);
  gf = NULL;
  if (g.out)
  {
    free (g.out);
    g.out = NULL;
  }
  if (g.history)
  {
    free (g.history);
    g.history = NULL;
  }
  if (g.background)
  {
    free (g.background);
    g.background = NULL;
  }
}

static void epicfb_stb_gif_blit (Ctx *ctx,
                                 int       x0,
                                 int       y0,
                                 int       w,
                                 int       h,
                                 uint8_t   opacity,
                                 int       filter)
{
  if (!gifbuf)
    return;
  ctx_reset (ctx);
  char eid[65];
  ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
  ctx_save (ctx);
  float scale = ctx_width (ctx) * 1.0 / stb_w;
  float scaleh = ctx_height (ctx) * 1.0 / stb_h;
  if (scaleh < scale) scale = scaleh;
  ctx_scale (ctx, scale, scale);
  //ctx_image_smoothing (ctx, 0);
  ctx_define_texture (ctx, NULL, stb_w, stb_h, stb_w * 4, CTX_FORMAT_RGBA8,
                      gifbuf, eid);
  ctx_fill (ctx);
  ctx_restore (ctx);
  ctx_flush (ctx);
}

#endif

static void liberate_resources (void)
{
  stb_clear_cache ();
#ifdef STBI_ONLY_GIF
  if (gif_active_path)
    epicfb_stb_gif_stop ();
#endif
}




/****************************/


char *path = NULL;

static void cb_next_file (void *data)
{
}

static void cb_prev_file (void *data)
{
}

static void cb_toggle_slideshow (void *data)
{
  slideshow = !slideshow;
  slide_start = ctx_ticks ();
}

int main(int argc, char *argv[])
{
  Ctx *ctx;
  path = argv[1];

  ctx = ctx_new_ui (-1, -1);

  while (!quit)
  {

    int delay = 0;
    int x0 = 0;
    int y0 = 0;
    int  w = 160;
    int  h = 0;
    int opacity = 255;
    int filter  = 1;
    int cache   = 1;

    switch (fit)
    {
      case 0: // width
        break;
      case 1: // height
        w = 0;
        h = 80;
        break;
      case 2: // 1:1
        w = 0;
        h = 0;
        break;
    }

    if (path)
    {
#ifdef STBI_ONLY_GIF
      if (strstr (path, ".gif") ||
          strstr (path, ".GIF"))
      {
repeat:
        if (!gif_active_path ||
            (gif_active_path && strcmp (path, gif_active_path)))
        {
          if (epicfb_stb_gif_init (path) == 0)
          {
            delay = epicfb_stb_gif_load_frame ();
            if (delay >= 0)
            {
              epicfb_stb_gif_blit (ctx, x0, y0, w, h, opacity, filter);
            }
          }
        }
        else
        {
          if (gif_active_path)
          {
            delay = epicfb_stb_gif_load_frame ();
            if (delay >= 0)
            {
              epicfb_stb_gif_blit (ctx, x0, y0, w, h, opacity, filter);
            }
            else
            {
              goto repeat; /* we loop GIFs infinitely */
            }
          }
        }
      }
      else
#endif
      {
        epicfb_stb (x0, y0, w, h, opacity, filter, cache, path);
      }
    }
    else
    {
      if (img_no == 0)
        return 0;
      img_no = 0;
      //path = epicfb_path_no (img_no, extensions);
    }
    //epicfb_update (pixels, EPICFB_CLEAR);
    ctx_rectangle (ctx, 0,0,ctx_width (ctx), ctx_height (ctx));
    ctx_rgb (ctx, 0,0,0);
    ctx_fill (ctx);



    CtxEvent *event;
   
    while ((event = ctx_get_event (ctx)))
    {
      switch (event->type)
      {
        case CTX_KEY_PRESS:
          if (!strcmp (event->string, "q"))
            quit = 1;
          break;
        default:
          break;
      }
    }

    if (delay>=0) /* only gifs set it non-0 */
    {
      int frame_start = ctx_ticks ();
      int now = ctx_ticks ();
      usleep (1000 * (delay - (now-frame_start)));
    }

    if (slideshow)
    {
      if (ctx_ticks () > slide_start + slide_seconds * 1000)
        cb_next_file (NULL);
    }

#if 0
    epicfb_events ();

    if (epicfb_event (BUTTON_RIGHT, LONG_PRESS))
      cb_toggle_slideshow (NULL);

    if (epicfb_event (BUTTON_RIGHT, PRESS))
      cb_next_file (NULL);

    if (epicfb_event (BUTTON_LEFT, PRESS))
      cb_prev_file (NULL);

    if (epicfb_event (BUTTON_SELECT, PRESS))
    {
      fit++;
      if (fit > 2) fit = 0;
    }

    if (epicfb_event (BUTTON_QUIT, PRESS))
    {
      quit = 1;
    }
#endif
  }
  liberate_resources (); /* to please valgrind */
  ctx_free (ctx);
  return 0;
}
