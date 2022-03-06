#include "ctx-split.h"

#if CTX_EVENTS

#define CTX_START_STRING "U\n"  // or " start_frame "
#define CTX_END_STRING   "\nX"  // or "\ndone"
#define CTX_END_STRING2  "\n"

static void ctx_card10_start_frame (Ctx *ctx)
{
}

static void ctx_card10_flush (Ctx *ctx)
{
  CtxCard10 *card10 = (CtxCard10*)ctx->backend;
  
  // hook into client, which we somehow got


  //ctx_render_stream (card10->backend.ctx, stdout, 0);
}

void ctx_card10_free (CtxCtx *ctxcard10)
{
  free (ctxcard10);
}

void ctx_card10_pcm (Ctx *ctx)
{
   /* can we just call ctx_pcm ? */
}

void ctx_card10_consume_events (Ctx *ctx)
{
  //int ix, iy;
  CtxCard10 *card10 = (CtxCard10*)ctx->backend;
  const char *event = NULL;
#if CTX_AUDIO
  ctx_card10_pcm (ctx);
#endif

#if 0
    { /* XXX : this is a work-around for signals not working properly, we are polling the
         size with an ioctl per consume-events
         */
      struct winsize ws;
      ioctl(0,TIOCGWINSZ,&ws);
      ctxctx->cols = ws.ws_col;
      ctxctx->rows = ws.ws_row;
      ctx_set_size (ctx, ws.ws_xpixel, ws.ws_ypixel);
    }
#endif

    //char *cmd = ctx_strdup_printf ("touch /tmp/ctx-%ix%i", ctxctx->width, ctxctx->height);
    //system (cmd);
    //free (cmd);

    do {
      float x = 0, y = 0;
      int b = 0;
      char event_type[128]="";

      if (card10->events)
      {
        event = card10->events->data;
        ctx_list_remove (&card10->events, &card10);
        //ctx_native_get_event (ctx, 1000/120);
      }
      else
      {
        event = NULL;
      }

      if (event)
      {
      sscanf (event, "%s %f %f %i", event_type, &x, &y, &b);
      if (!strcmp (event_type, "idle"))
      {
         event = NULL;
      }
      else if (!strcmp (event_type, "pp"))
      {
        ctx_pointer_press (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "pd")||
               !strcmp (event_type, "pm"))
      {
        ctx_pointer_motion (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "pr"))
      {
        ctx_pointer_release (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "message"))
      {
        ctx_incoming_message (ctx, event + strlen ("message"), 0);
      } else if (!strcmp (event, "size-changed"))
      {
        //fprintf (stdout, "\e[H\e[2J\e[?25l");
        //ctxctx->cols = ctx_terminal_cols ();
        //ctxctx->rows = ctx_terminal_rows ();

        //system ("touch /tmp/ctx-abc");

      // ctx_set_size (ctx, ctx_terminal_width(), ctx_terminal_height());

        ctx_queue_draw (ctx); // XXX add coordinates

      //   ctx_key_press(ctx,0,"size-changed",0);
      }
      else if (!strcmp (event_type, "keyup"))
      {
        char buf[4]={ x, 0 };
        ctx_key_up (ctx, (int)x, buf, 0);
      }
      else if (!strcmp (event_type, "keydown"))
      {
        char buf[4]={ x, 0 };
        ctx_key_down (ctx, (int)x, buf, 0);
      }
      else
      {
        ctx_key_press (ctx, 0, event, 0);
      }
      }
    } while (event);
}

Ctx *ctx_new_card10 (int width, int height)
{
  float font_size = 12.0;
  Ctx    *ctx    = ctx_new ();
  CtxCard10  *card10 = (CtxCard10*)calloc (sizeof (CtxCard10), 1);
  CtxBackend *backend = (CtxBackend*)card10;
  if (width <= 0 || height <= 0)
  {
    width  = ctx->events.width  = 512;
    height = ctx->events.height = 384;
    font_size = height / 20;
    ctx_font_size (ctx, font_size);
  }
  else
  {
    ctx->events.width  = width;
    ctx->events.height = height;
  }
  backend->process = (void*)ctx_drawlist_process;
  backend->ctx = ctx;
  backend->start_frame = ctx_card10_start_frame;
  backend->flush = ctx_card10_flush;
  backend->free  = (void(*)(void *))ctx_card10_free;
  backend->consume_events = ctx_card10_consume_events;
  ctx_set_backend (ctx, card10);
  ctx_set_size (ctx, width, height);
  return ctx;
}

#endif
