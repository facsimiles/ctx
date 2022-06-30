#include "itk.h"

typedef struct _Circle
{
  float x;
  float y;
  float radius;
} Circle;

#define MAX_CIRCLES 20
static Circle circle_list[MAX_CIRCLES];
static int circle_count = 0;

static int nearest_circle = -1;
static void circle_editor_release_cb (CtxEvent *event, void *data1, void *data2)
{
  circle_list[circle_count].x = event->x;
  circle_list[circle_count].y = event->y;
  circle_list[circle_count].radius = 100;
  nearest_circle = circle_count;
  circle_count++;
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

static void circle_editor_circle_release_cb (CtxEvent *event, void *data1, void *data2)
{
  fprintf (stderr, "!!!!!!\n");
  event->stop_propagate = 1;
  ctx_queue_draw (event->ctx);
}

static void circle_editor_motion_cb (CtxEvent *event, void *data1, void *data2)
{
  float nearest_sq_dist = 4096*4096;
  int nearest = -1;
  for (int i = 0; i < circle_count; i ++)
  {
     float sq_dist = (circle_list[i].x-event->x)*
                     (circle_list[i].x-event->x)+
                     (circle_list[i].y-event->y)*
                     (circle_list[i].y-event->y);
     if (sq_dist < circle_list[i].radius * circle_list[i].radius)
     if (sq_dist < nearest_sq_dist)
     {
       nearest_sq_dist = sq_dist;
       nearest = i;
     }
  }
  if (nearest != nearest_circle)
  {
    // it's changed
    nearest_circle = nearest;
    ctx_queue_draw (event->ctx);
  }

  event->stop_propagate = 1;
}

static int circle_editor_ui (ITK *itk, void *data)
{
  Ctx *ctx = itk_ctx (itk);
  itk_panel_start (itk, "7gui - Circle Editor",
                  ctx_width(ctx)*0.2, 0,
                  ctx_width (ctx) * 0.8, ctx_height (ctx));

  if (itk_button (itk, "Undo"))
  {
  }
  itk_sameline (itk);
  if (itk_button (itk, "Redo"))
  {
  }

  ctx_rectangle (ctx, itk_x (itk), itk_y (itk),
                  itk_wrap_width (itk), itk_height (itk) - (itk_y (itk)-
                         itk_edge_top (itk)));
  ctx_listen (ctx, CTX_RELEASE, circle_editor_release_cb, NULL, NULL);
  ctx_listen (ctx, CTX_MOTION, circle_editor_motion_cb, NULL, NULL);

  ctx_rgb (ctx, 1,0,0);
  ctx_fill (ctx);

  for (int i = 0; i < circle_count; i ++)
  {
     if (i == nearest_circle)
       ctx_gray (ctx, 0.5);
     else
       ctx_gray (ctx, 1.0);
     ctx_arc (ctx, circle_list[i].x, circle_list[i].y, circle_list[i].radius, 0.0, 3*2, 0);
     ctx_fill (ctx);
     ctx_rectangle (ctx, circle_list[i].x - circle_list[i].radius,
                     circle_list[i].y - circle_list[i].radius,
                     circle_list[i].radius * 2, circle_list[i].radius * 2);
     ctx_listen (ctx, CTX_RELEASE, circle_editor_circle_release_cb, NULL, NULL);
     ctx_begin_path (ctx);
  }


  itk_panel_end (itk);
  return 1;
}

int main (int argc, char **argv)
{
  itk_main (circle_editor_ui, NULL);
  return 0;
}
