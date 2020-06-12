/* this is to serve as a minimal - no dependencies application
 * integrating with the oc display server.
 */
#include "mmm.h"
#include "mmm-pset.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "ctx-font-regular.h"
#define CTX_IMPLEMENTATION
#include "ctx.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

static int W = 400;
static int H = 300;

int tx = 0;
int ty = 0;


void fill_render (Mmm *fb)
{
  Ctx *ctx;
  //int x, y;
  unsigned char *buffer;
  int width, height, stride;
  mmm_client_check_size (fb, &width, &height);

  buffer = mmm_get_buffer_write (fb, &width, &height, &stride, NULL);
  //bpp = mmm_get_bytes_per_pixel (fb);
  ctx = ctx_new_for_framebuffer (buffer, width, height, stride, CTX_FORMAT_RGBA8);

  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_set_rgba (ctx, 1, 1, 1, 1);
  ctx_fill (ctx);
  ctx_set_rgba (ctx, 0, 0, 0, 1);
  ctx_rectangle (ctx, width * 0.2, height * 0.2, width * 0.6, height * 0.6);
  ctx_fill (ctx);
  ctx_set_rgba (ctx, 1, 1, 1, 1);
  ctx_set_font_size (ctx, height * 0.1);
  ctx_set_font (ctx, "regular");
  ctx_move_to (ctx, width * 0.3, height * 0.6);
  ctx_text (ctx, "mmm+ctx");

  ctx_free (ctx);
  mmm_write_done (fb, 0, 0, -1, -1);
}

int quit = 0;

void event_handling (Mmm *fb)
{
  while (mmm_has_event (fb))
    {
      const char *event = mmm_get_event (fb);
      float x = 0, y = 0;
      const char *p;
      if (!strcmp (event, "q")) quit = 1;
      p = strchr (event, ' ');
      if (p)
        {
          x = atof (p+1);
          p = strchr (p+1, ' ');
          if (p)
            y = atof (p+1);
        }
      tx = x;
      ty = y;
    }
}

int main ()
{
  Mmm *fb = mmm_new (W, H, (MmmFlag)0, NULL);
  //int j;
  if (!fb)
    {
      fprintf (stderr, "failed to open buffer\n");
      return -1;
    }

  W = mmm_get_width (fb);
  H = mmm_get_height (fb);

  tx = W/2;
  ty = H/2;

  while (!quit)
  {
    event_handling (fb);
    fill_render (fb);
  }

  mmm_destroy (fb);
  fprintf (stderr, "ctx-test done!\n");
  return 0;
}
