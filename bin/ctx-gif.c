/*
 * Copyright 2019, 2021 Øyvind Kolås <pippin@gimp.org>
 *
 *  plays a gif animation on loop using stb_image, originally written as
 *  a l0dable for card10, decoding GIF with stb_image this way with
 *  callbacks is efficient enough for use on a microcontroller.
 *
 */

#if CTX_STB_IMAGE

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#if CTX_CAIRO
#include <cairo.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ctx.h"

#define STBI_NO_STDIO
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

static int ctx_gif_stb_w = -1;
static int ctx_gif_stb_h = -1;

static stbi__context s;

static FILE *gf = NULL;

static char    *gif_active_path = NULL;
static uint8_t *gifbuf = NULL;
static stbi__gif g;

static int frameno = 0;

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
  ctx_gif_stb_w = -1;
  ctx_gif_stb_h = -1;

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
  if (ctx_gif_stb_w < 0) ctx_gif_stb_w = g.w;
  if (ctx_gif_stb_h < 0) ctx_gif_stb_h = g.h;

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
                                 int       h)
{
  if (!gifbuf)
    return;
  ctx_start_frame (ctx);
  char eid[65];

  ctx_rectangle (ctx, 0,0, ctx_width(ctx), ctx_height(ctx));
  ctx_save (ctx);
  float scale = ctx_width (ctx) * 1.0 / ctx_gif_stb_w;
  float scaleh = ctx_height (ctx) * 1.0 / ctx_gif_stb_h;
  if (scaleh < scale) scale = scaleh;
  ctx_translate (ctx, (ctx_width(ctx)-ctx_gif_stb_w*scale)/2.0, 
                      (ctx_height(ctx)-ctx_gif_stb_h*scale)/2.0);
  ctx_scale (ctx, scale, scale);
  //ctx_image_smoothing (ctx, 0);
  ctx_define_texture (ctx, NULL, ctx_gif_stb_w, ctx_gif_stb_h, ctx_gif_stb_w * 4, CTX_FORMAT_RGBA8,
                      gifbuf, eid);
  ctx_compositing_mode (ctx, CTX_COMPOSITE_COPY);
  ctx_fill (ctx);
  ctx_restore (ctx);
  ctx_end_frame (ctx);
}

static void liberate_resources (void)
{
  epicfb_stb_gif_stop ();
}

/****************************/

#if CTX_BIN_BUNDLE
int ctx_gif_main (int argc, char *argv[])
#else
int main (int argc, char *argv[])
#endif
{
  Ctx *ctx;
  char *path = argv[1];

  if (! (strstr (path, ".gif") || strstr (path, ".GIF")))
  {
    fprintf (stderr, "%s doesn't end in .gif or .GIF", path);
    return 1;
  }

  if (strchr (path, ':'))
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  ctx = ctx_new (-1, -1, NULL);

  int frame_start = ctx_ticks ();
  while (!quit)
  {

    int delay = 0;
    int x0 = 0;
    int y0 = 0;
    int  w = 160;
    int  h = 0;

    if (path)
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
              epicfb_stb_gif_blit (ctx, x0, y0, w, h);
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
              epicfb_stb_gif_blit (ctx, x0, y0, w, h);
            }
            else
            {
              goto repeat; /* we loop GIFs infinitely */
            }
          }
        }
    }

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
      int now = ctx_ticks ();
      if (delay == 0) delay = 100;
      if (1000 * (delay) - (now-frame_start) > 0)
        usleep (1000 * (delay) - (now-frame_start) );
    }
    frame_start = ctx_ticks ();
  }
  liberate_resources (); /* to please valgrind */
  ctx_destroy (ctx);
  return 0;
}

#endif
