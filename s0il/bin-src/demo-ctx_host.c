#include "s0il.h"

MAIN(demo_ctx_host) {
  Ctx *ctx = ctx_host(); // ctx_new(240,240,NULL);
  if (!ctx)
    return -1;

  float width = ctx_width(ctx);
  float height = ctx_height(ctx);
  for (int i = 0; i <= 5; i++) {
    ctx_start_frame(ctx);

    ctx_rectangle(ctx, 0, 0, width, height);
    ctx_rgba(ctx, i * (1.0f / 5.0f), 1, 0, 1.0f);
    ctx_fill(ctx);
    ctx_rgba(ctx, 1, 1, 1, 1.0f);
    ctx_move_to(ctx, width / 2, height / 2);
    char buf[1024];
    sprintf(buf, "%i", i);
    ctx_text(ctx, buf);

    ctx_end_frame(ctx);
    usleep(1000*10);
  }
  // ctx_destroy (ctx);

  return 0;
}
