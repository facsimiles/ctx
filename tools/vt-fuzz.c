
#define CTX_VT  1
#define CTX_PTY 1
#define CTX_IMPLEMENTATION

#include "ctx.h"


void escape_cb (CtxEvent *event, void *a, void *b)
{
  ctx_exit (event->ctx);
}

typedef struct _VT VT;

void        vt_draw                 (VT *vt, Ctx *ctx, double x, double y);
VT *vt_new (const char *command, int width, int height, float font_size, float line_spacing, int id, int can_launch);

int vt_poll (VT *vt, int timeout);
void vt_set_ctx (VT *vt, Ctx *ctx);
int main (int argc, char **argv)
{
  Ctx *ctx = ctx_new_drawlist (320,240);// (320, 240, NULL);
  VT *vt = vt_new (NULL, 320, 240, 14.0f, 1.2f, 0, 0);

  vt_set_ctx (vt, ctx);
  FILE *file = stdin;

  if (argv[1])
	  file = fopen (argv[1], "r");

  while (!ctx_has_exited (ctx))
  {
    char buf[32];
    int len = fread(buf, 1, sizeof(buf), file);
    if (len > 0)
    {
      for (int i = 0; i < len; i++) {
        ctx_vt_write (ctx, buf[i]);
      }
      vt_poll (vt, 10);
      ctx_queue_draw (ctx);
    }
    else if (len <= 0)
    {
      ctx_exit (ctx);
    }

    if (ctx_need_redraw(ctx))
    {
    ctx_start_frame (ctx);
    ctx_rgb(ctx, 0,0.1,0.2);
    ctx_paint(ctx);
    vt_draw (vt, ctx, 0, 0);

    ctx_rgb(ctx,0,1,0);
    ctx_move_to(ctx,30,30);
    ctx_font_size(ctx,24);
    char fbuf[44];
    static int frame_no=0;
    sprintf(fbuf, "%i", frame_no++);
    ctx_text(ctx, fbuf);

    //ctx_add_key_binding (ctx, "escape", NULL, NULL, escape_cb, NULL);
    ctx_end_frame (ctx);

    while (ctx_vt_has_data (ctx))
      ctx_vt_read (ctx);
    }
  }

  ctx_destroy (ctx);
  return 0;
}
