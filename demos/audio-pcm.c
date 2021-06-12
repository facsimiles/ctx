#include "ctx.h"
#include <math.h>

#include <stdio.h>
#include <unistd.h>

float hz    = 440;
float phase = 0.0;

static int audio_cb (Ctx *ctx, void *data)
{
  int count = 8192 * 10;
  float buf[count * 2];
  int i;
  count = ctx_pcm_get_frame_chunk (ctx);

  if (count <= 0)
  {
    return 1;
  }

  for (i = 0; i < count; i++)
  {
    phase += hz/(48000.0);
    buf[i*2] = sin(phase * M_PI * 2) * 0.1;
    buf[i*2+1] = sin(phase * M_PI * 2) * 0.1;
  }
  ctx_pcm_queue (ctx, (void*)buf, count);
  return 1;
}

int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new_ui (400, 300);
//mrg_set_ui (mrg, ui, argv[1]?argv[1]:"world");
  ctx_pcm_set_sample_rate (ctx, 44100);
  ctx_pcm_set_format (ctx, CTX_f32S);

  ctx_add_idle (ctx, audio_cb, NULL); /* can also be put in a thread */

  int quit = 0;
  int dirty = 1;
  while (!quit)
  {
    CtxEvent *event;
    if (dirty)
    {
       ctx_reset (ctx);
       ctx_save (ctx);
       ctx_rectangle (ctx, 0,0,ctx_width(ctx), ctx_height(ctx));
       ctx_gray (ctx, 0.5);
       ctx_fill (ctx);
       ctx_flush (ctx);
       dirty = 0;
    }
    while ((event = ctx_get_event (ctx)))
    {
      switch (event->type)
      {
        case CTX_KEY_PRESS:
          if (!strcmp (event->string, "q"))
            quit = 1;
          if (!strcmp (event->string, "="))
            hz += 10;
          if (!strcmp (event->string, "+"))
            hz += 10;
          if (!strcmp (event->string, "-"))
            hz -= 10;
          break;
        default:
          break;
      }
    }
  }
  ctx_free (ctx);
  return 0;
}
