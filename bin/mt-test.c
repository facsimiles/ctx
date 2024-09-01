// a simple multi touch test


#include "ctx.h"

typedef struct MtInfo {
  int id;
  float x;
  float y;
  int down;
} MtInfo;

MtInfo fingers[10]={};

static void action_quit(CtxEvent *event, void *data1, void *data2) {
  ctx_exit(event->ctx);
  ctx_queue_draw(event->ctx);
} 

static int device_id_to_finger(int device_id)
{
  for (int i =1; i<10;i++)
  {
    if (fingers[i].id == device_id) return i;
  }
  for (int i =1; i<10;i++)
  {
    if (fingers[i].id == 0){
      fingers[i].id = device_id;
      return i;
    }
  }
  return 0;
}

static void drag_event (CtxEvent *event, void *a, void *b)
{
  int finger_no = device_id_to_finger (event->device_no);
  switch (event->type)
  {
    case CTX_DRAG_PRESS:
     fingers[finger_no].x = event->x;
     fingers[finger_no].y = event->y;
     fingers[finger_no].down = 1;
     break;
    case CTX_DRAG_RELEASE:
     fingers[finger_no].x = event->x;
     fingers[finger_no].y = event->y;
     fingers[finger_no].down = 0;
     break;
    case CTX_DRAG_MOTION:
     fingers[finger_no].x = event->x;
     fingers[finger_no].y = event->y;
     break;
    default: break;
  }
}


static int mt_test(Ctx *ctx) {
  float width = ctx_width (ctx);
  float height = ctx_height (ctx);
  ctx_start_frame (ctx);
  ctx_rgb (ctx, 0,0,0);
  ctx_paint (ctx);


  for (int i = 0; i < 10; i++)
  {
    float x = fingers[i].x;
    float y = fingers[i].y;
    ctx_rectangle (ctx, x-25, y-25, 50, 50);
    if (fingers[i].down)
    {
      ctx_rgba (ctx,1,1,0,1);
    }
    else
    {
      ctx_rgba (ctx,1,1,1,0.5);
    }
    ctx_fill (ctx);
  }

  ctx_rectangle (ctx, 0, 0, width, height);
  ctx_listen (ctx, CTX_DRAG, drag_event, NULL, NULL);

  ctx_add_key_binding(ctx, "escape", NULL, "foo", action_quit, NULL);

  ctx_end_frame (ctx);
  return 1;
}

int main(int argc, char **argv) {
  Ctx *ctx = ctx_new(-1, -1, NULL);
  ctx_get_event(ctx);
  for (int i = 0; i < 10; i++)
  {
    fingers[i].x = -100;
    fingers[i].y = -100;
  }
  while (!ctx_has_exited(ctx))
    mt_test (ctx);
  ctx_destroy(ctx);
  return 0;
}
