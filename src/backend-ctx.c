#include "ctx-split.h"

#if CTX_EVENTS

static void ctx_ctx_flush (CtxCtx *ctxctx)
{
  if (ctx_native_events)
    fprintf (stdout, "\e[?201h");
  fprintf (stdout, "\e[H\e[?25l\e[?200h reset\n");
#if 1
  ctx_render_stream (ctxctx->ctx, stdout, 0);
#else
  CtxString *string = ctx_string_new("");
  

  ctx_string_free (string, 1);
#endif


  fprintf (stdout, "\ndone\n\e");
  fflush (stdout);
}

void ctx_ctx_free (CtxCtx *ctx)
{
  nc_at_exit ();
  free (ctx);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

Ctx *ctx_new_ctx (int width, int height)
{
  Ctx *ctx = ctx_new ();
  CtxCtx *ctxctx = (CtxCtx*)calloc (sizeof (CtxCtx), 1);
  ctx_native_events = 1;
  if (width <= 0 || height <= 0)
  {
    ctxctx->cols = ctx_terminal_cols ();
    ctxctx->rows = ctx_terminal_rows ();
    width  = ctxctx->width  = ctx_terminal_width ();
    height = ctxctx->height = ctx_terminal_height ();
  }
  else
  {
    ctxctx->width  = width;
    ctxctx->height = height;
    ctxctx->cols   = width / 80;
    ctxctx->rows   = height / 24;
  }
  ctxctx->ctx = ctx;
  if (!ctx_native_events)
    _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, ctxctx);
  ctx_set_size (ctx, width, height);
  ctxctx->flush = (void(*)(void *))ctx_ctx_flush;
  ctxctx->free  = (void(*)(void *))ctx_ctx_free;
  fprintf (stdout, "\e[2J");
  fprintf (stdout, "\e[?1049h");
  fflush (stdout);
  return ctx;
}


int ctx_ctx_consume_events (Ctx *ctx)
{
  int ix, iy;
  CtxCtx *ctxctx = (CtxCtx*)ctx->renderer;
  const char *event = NULL;
  if (ctx_native_events)
    {
      float x = 0, y = 0;
      int b = 0;
      char event_type[128]="";
      event = ctx_native_get_event (ctx, 1000/60);
#if 0
      if(event){
        FILE *file = fopen ("/tmp/log", "a");
        fprintf (file, "[%s]\n", event);
        fclose (file);
      }
#endif
      if (event)
      {
      sscanf (event, "%s %f %f %i", event_type, &x, &y, &b);
      if (!strcmp (event_type, "idle"))
      {
      }
      else if (!strcmp (event_type, "mouse-press"))
      {
        ctx_pointer_press (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "mouse-drag")||
               !strcmp (event_type, "mouse-motion"))
      {
        ctx_pointer_motion (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "mouse-release"))
      {
        ctx_pointer_release (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "message"))
      {
        ctx_incoming_message (ctx, event + strlen ("message"), 0);
      }
      else
      {
        ctx_key_press (ctx, 0, event, 0);
      }
      }
    }
  else
    {
      float x, y;
      event = ctx_nct_get_event (ctx, 20, &ix, &iy);

      x = (ix - 1.0 + 0.5) / ctxctx->cols * ctx->events.width;
      y = (iy - 1.0)       / ctxctx->rows * ctx->events.height;

      if (!strcmp (event, "mouse-press"))
      {
        ctx_pointer_press (ctx, x, y, 0, 0);
        ctxctx->was_down = 1;
      } else if (!strcmp (event, "mouse-release"))
      {
        ctx_pointer_release (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "mouse-motion"))
      {
        //nct_set_cursor_pos (backend->term, ix, iy);
        //nct_flush (backend->term);
        if (ctxctx->was_down)
        {
          ctx_pointer_release (ctx, x, y, 0, 0);
          ctxctx->was_down = 0;
        }
        ctx_pointer_motion (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "mouse-drag"))
      {
        ctx_pointer_motion (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "size-changed"))
      {
#if 0
        int width = nct_sys_terminal_width ();
        int height = nct_sys_terminal_height ();
        nct_set_size (backend->term, width, height);
        width *= CPX;
        height *= CPX;
        free (mrg->glyphs);
        free (mrg->styles);
        free (backend->nct_pixels);
        backend->nct_pixels = calloc (width * height * 4, 1);
        mrg->glyphs = calloc ((width/CPX) * (height/CPX) * 4, 1);
        mrg->styles = calloc ((width/CPX) * (height/CPX) * 1, 1);
        mrg_set_size (mrg, width, height);
        mrg_queue_draw (mrg, NULL);
#endif
      }
      else
      {
        if (!strcmp (event, "esc"))
          ctx_key_press (ctx, 0, "escape", 0);
        else if (!strcmp (event, "space"))
          ctx_key_press (ctx, 0, "space", 0);
        else if (!strcmp (event, "enter"))
          ctx_key_press (ctx, 0, "\n", 0);
        else if (!strcmp (event, "return"))
          ctx_key_press (ctx, 0, "\n", 0);
        else
        ctx_key_press (ctx, 0, event, 0);
      }
    }

  return 1;
}

#endif
