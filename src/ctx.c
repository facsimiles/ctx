#include "ctx-split.h"

#if CTX_EVENTS
int ctx_width (Ctx *ctx)
{
  return ctx->events.width;
}
int ctx_height (Ctx *ctx)
{
  return ctx->events.height;
}
#else
int ctx_width (Ctx *ctx)
{
  return 512;
}
int ctx_height (Ctx *ctx)
{
  return 384;
}
#endif

int ctx_rev (Ctx *ctx)
{
  return ctx->rev;
}

CtxState *ctx_get_state (Ctx *ctx)
{
  return &ctx->state;
}

void ctx_dirty_rect (Ctx *ctx, int *x, int *y, int *width, int *height)
{
  if ( (ctx->state.min_x > ctx->state.max_x) ||
       (ctx->state.min_y > ctx->state.max_y) )
    {
      if (x) { *x = 0; }
      if (y) { *y = 0; }
      if (width) { *width = 0; }
      if (height) { *height = 0; }
      return;
    }
  if (ctx->state.min_x < 0)
    { ctx->state.min_x = 0; }
  if (ctx->state.min_y < 0)
    { ctx->state.min_y = 0; }
  if (x) { *x = ctx->state.min_x; }
  if (y) { *y = ctx->state.min_y; }
  if (width) { *width = ctx->state.max_x - ctx->state.min_x; }
  if (height) { *height = ctx->state.max_y - ctx->state.min_y; }
  //fprintf (stderr, "%i %i %ix%i\n", *x, *y, *width, *height);
}

#if CTX_CURRENT_PATH
CtxIterator *
ctx_current_path (Ctx *ctx)
{
  CtxIterator *iterator = &ctx->current_path_iterator;
  ctx_iterator_init (iterator, &ctx->current_path, 0, CTX_ITERATOR_EXPAND_BITPACK);
  return iterator;
}

void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2)
{
  float minx = 50000.0;
  float miny = 50000.0;
  float maxx = -50000.0;
  float maxy = -50000.0;
  float x = 0;
  float y = 0;

  CtxIterator *iterator = ctx_current_path (ctx);
  CtxCommand *command;

  while ((command = ctx_iterator_next (iterator)))
  {
     int got_coord = 0;
     switch (command->code)
     {
        // XXX missing many curve types
        case CTX_LINE_TO:
        case CTX_MOVE_TO:
          x = command->move_to.x;
          y = command->move_to.y;
          got_coord++;
          break;
        case CTX_REL_LINE_TO:
        case CTX_REL_MOVE_TO:
          x += command->move_to.x;
          y += command->move_to.y;
          got_coord++;
          break;
        case CTX_CURVE_TO:
          x = command->curve_to.x;
          y = command->curve_to.y;
          got_coord++;
          break;
        case CTX_REL_CURVE_TO:
          x += command->curve_to.x;
          y += command->curve_to.y;
          got_coord++;
          break;
        case CTX_RECTANGLE:
        case CTX_ROUND_RECTANGLE:
          x = command->rectangle.x;
          y = command->rectangle.y;
          minx = ctx_minf (minx, x);
          miny = ctx_minf (miny, y);
          maxx = ctx_maxf (maxx, x);
          maxy = ctx_maxf (maxy, y);

          x += command->rectangle.width;
          y += command->rectangle.height;
          got_coord++;
          break;
     }
    if (got_coord)
    {
      minx = ctx_minf (minx, x);
      miny = ctx_minf (miny, y);
      maxx = ctx_maxf (maxx, x);
      maxy = ctx_maxf (maxy, y);
    }
  }
  if (ex1) *ex1 = minx;
  if (ey1) *ey1 = miny;
  if (ex2) *ex2 = maxx;
  if (ey2) *ey2 = maxy;
}

#else
void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2)
{
}
#endif


static void
ctx_gstate_push (CtxState *state)
{
  if (state->gstate_no + 1 >= CTX_MAX_STATES)
    { return; }
  state->gstate_stack[state->gstate_no] = state->gstate;
  state->gstate_no++;
  ctx_state_set (state, CTX_new_state, 0.0);
  state->has_clipped=0;
}

static void
ctx_gstate_pop (CtxState *state)
{
  if (state->gstate_no <= 0)
    { return; }
  state->gstate = state->gstate_stack[state->gstate_no-1];
  state->gstate_no--;
}




void
ctx_close_path (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_CLOSE_PATH);
}


uint8_t *
ctx_get_image_data (Ctx *ctx, int sx, int sy, int sw, int sh, int format, int stride)
{
   // NYI
   return NULL;
}

void
ctx_put_image_data (Ctx *ctx, uint8_t *data, int w, int h, int format, int stride,
                    int dx, int dy, int dirtyX, int dirtyY,
                    int dirtyWidth, int dirtyHeight)
{
   // NYI
}
                    

void ctx_texture (Ctx *ctx, int id, float x, float y)
{
  CtxEntry commands[2];
  if (id < 0) { return; }
  commands[0] = ctx_u32 (CTX_TEXTURE, id, 0);
  commands[1] = ctx_f  (CTX_CONT, x, y);
  ctx_process (ctx, commands);
}

void
ctx_image_path (Ctx *ctx, const char *path, float x, float y)
{
  int id = ctx_texture_load (ctx, -1, path);
  ctx_texture (ctx, id, x, y);

  // query if image exists .. 
  //   if it doesnt load, decode, encode, upload to path/
}


void
ctx_set_pixel_u8 (Ctx *ctx, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_PIXEL, r, g, b, a, 0, 0, 0, 0);
  command.data.u16[2]=x;
  command.data.u16[3]=y;
  ctx_process (ctx, &command);
}


void
ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1)
{
  CtxEntry command[2]=
  {
    ctx_f (CTX_LINEAR_GRADIENT, x0, y0),
    ctx_f (CTX_CONT,            x1, y1)
  };
  ctx_process (ctx, command);
}

void
ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0, float x1, float y1, float r1)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_RADIAL_GRADIENT, x0, y0),
    ctx_f (CTX_CONT,            r0, x1),
    ctx_f (CTX_CONT,            y1, r1)
  };
  ctx_process (ctx, command);
}

void ctx_gradient_add_stop_u8
(Ctx *ctx, float pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry entry = ctx_f (CTX_GRADIENT_STOP, pos, 0);
  entry.data.u8[4+0] = r;
  entry.data.u8[4+1] = g;
  entry.data.u8[4+2] = b;
  entry.data.u8[4+3] = a;
  ctx_process (ctx, &entry);
}

void ctx_gradient_add_stop
(Ctx *ctx, float pos, float r, float g, float b, float a)
{
  int ir = r * 255;
  int ig = g * 255;
  int ib = b * 255;
  int ia = a * 255;
  ir = CTX_CLAMP (ir, 0,255);
  ig = CTX_CLAMP (ig, 0,255);
  ib = CTX_CLAMP (ib, 0,255);
  ia = CTX_CLAMP (ia, 0,255);
  ctx_gradient_add_stop_u8 (ctx, pos, ir, ig, ib, ia);
}

void ctx_preserve (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_PRESERVE);
}

void ctx_fill (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_FILL);
}

void ctx_stroke (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_STROKE);
}


static void ctx_empty (Ctx *ctx)
{
#if CTX_RASTERIZER
  if (ctx->renderer == NULL)
#endif
    {
      ctx->renderstream.count = 0;
      ctx->renderstream.bitpack_pos = 0;
    }
}

void _ctx_set_store_clear (Ctx *ctx)
{
  ctx->transformation |= CTX_TRANSFORMATION_STORE_CLEAR;
}

#if CTX_EVENTS
static void
ctx_event_free (CtxEvent *event)
{
  free (event);
}

static void
ctx_collect_events (CtxEvent *event, void *data, void *data2)
{
  Ctx *ctx = data;
  CtxEvent *copy;
  if (event->type == CTX_KEY_DOWN && !strcmp (event->string, "idle"))
    return;
  copy = malloc (sizeof (CtxEvent));
  *copy = *event;
  ctx_list_append_full (&ctx->events.events, copy, (void*)ctx_event_free, NULL);
}
#endif

#if CTX_EVENTS
static void _ctx_bindings_key_down (CtxEvent *event, void *data1, void *data2);
#endif

void ctx_reset (Ctx *ctx)
{
  //CTX_PROCESS_VOID (CTX_RESET);
  //if (ctx->transformation & CTX_TRANSFORMATION_STORE_CLEAR)
  //  { return; }
  ctx_empty (ctx);
  ctx_state_init (&ctx->state);
#if CTX_EVENTS
  ctx_list_free (&ctx->events.items);

  if (ctx->events.ctx_get_event_enabled)
  {
    ctx_clear_bindings (ctx);

    ctx_listen_full (ctx, 0,0,0,0,
                     CTX_KEY_DOWN, _ctx_bindings_key_down, ctx, ctx,
                     NULL, NULL);
#if 0
    // should be enabled if a mask of CTX_KEY_DOWN|UP is passed to get_event?
    ctx_listen_full (ctx, 0, 0, 0,0,
                     CTX_KEY_DOWN, ctx_collect_events, ctx, ctx,
                     NULL, NULL);
    ctx_listen_full (ctx, 0, 0, 0,0,
                     CTX_KEY_UP, ctx_collect_events, ctx, ctx,
                     NULL, NULL);
#endif

    ctx_listen_full (ctx, 0, 0, ctx->events.width, ctx->events.height,
                     CTX_PRESS|CTX_RELEASE|CTX_MOTION, ctx_collect_events, ctx, ctx,
                     NULL, NULL);
  }
#endif
}

void ctx_begin_path (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_BEGIN_PATH);
}

void ctx_clip (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_CLIP);
}

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len);

void ctx_start_move (Ctx *ctx)
{
  ctx_set (ctx, CTX_start_move, "", 0);
}

void ctx_save (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_SAVE);
}
void ctx_restore (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_RESTORE);
}

void ctx_start_group (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_START_GROUP);
}

void ctx_end_group (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_END_GROUP);
}

void ctx_line_width (Ctx *ctx, float x)
{
  if (ctx->state.gstate.line_width != x)
    CTX_PROCESS_F1 (CTX_LINE_WIDTH, x);
}

void ctx_shadow_blur (Ctx *ctx, float x)
{
#if CTX_ENABLE_SHADOW_BLUR
  if (ctx->state.gstate.shadow_blur != x)
#endif
    CTX_PROCESS_F1 (CTX_SHADOW_BLUR, x);
}

void ctx_shadow_rgba (Ctx *ctx, float r, float g, float b, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SHADOW_COLOR, CTX_RGBA, r),
    ctx_f (CTX_CONT, g, b),
    ctx_f (CTX_CONT, a, 0)
  };
  ctx_process (ctx, command);
}

void ctx_shadow_offset_x (Ctx *ctx, float x)
{
#if CTX_ENABLE_SHADOW_BLUR
  if (ctx->state.gstate.shadow_offset_x != x)
#endif
    CTX_PROCESS_F1 (CTX_SHADOW_OFFSET_X, x);
}

void ctx_shadow_offset_y (Ctx *ctx, float x)
{
#if CTX_ENABLE_SHADOW_BLUR
  if (ctx->state.gstate.shadow_offset_y != x)
#endif
    CTX_PROCESS_F1 (CTX_SHADOW_OFFSET_Y, x);
}

void
ctx_global_alpha (Ctx *ctx, float global_alpha)
{
  if (ctx->state.gstate.global_alpha_f != global_alpha)
    CTX_PROCESS_F1 (CTX_GLOBAL_ALPHA, global_alpha);
}

float
ctx_get_global_alpha (Ctx *ctx)
{
  return ctx->state.gstate.global_alpha_f;
}

void
ctx_font_size (Ctx *ctx, float x)
{
  CTX_PROCESS_F1 (CTX_FONT_SIZE, x);
}

float ctx_get_font_size  (Ctx *ctx)
{
  return ctx->state.gstate.font_size;
}

void
ctx_miter_limit (Ctx *ctx, float limit)
{
  CTX_PROCESS_F1 (CTX_MITER_LIMIT, limit);
}

float ctx_get_line_width (Ctx *ctx)
{
  return ctx->state.gstate.line_width;
}

void
_ctx_font (Ctx *ctx, const char *name)
{
  ctx->state.gstate.font = ctx_resolve_font (name);
}

void
ctx_set (Ctx *ctx, uint32_t key_hash, const char *string, int len)
{
  if (len <= 0) len = strlen (string);
  ctx_process_cmd_str (ctx, CTX_SET, string, key_hash, len);
}

const char *
ctx_get (Ctx *ctx, const char *key)
{
  static char retbuf[32];
  int len = 0;
  CTX_PROCESS_U32(CTX_GET, ctx_strhash (key, 0), 0);
  while (read (STDIN_FILENO, &retbuf[len], 1) != -1)
    {
      if(retbuf[len]=='\n')
        break;
      retbuf[++len]=0;
    }
  return retbuf;
}

void
ctx_font (Ctx *ctx, const char *name)
{
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_FONT, name, 0, 0);
#else
  _ctx_font (ctx, name);
#endif
}

void ctx_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    { CTX_PROCESS_F (CTX_MOVE_TO, x, y); }
  else
    { CTX_PROCESS_F (CTX_LINE_TO, x, y); }
}

void ctx_move_to (Ctx *ctx, float x, float y)
{
  CTX_PROCESS_F (CTX_MOVE_TO,x,y);
}

void ctx_curve_to (Ctx *ctx, float x0, float y0,
                   float x1, float y1,
                   float x2, float y2)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_CURVE_TO, x0, y0),
    ctx_f (CTX_CONT,     x1, y1),
    ctx_f (CTX_CONT,     x2, y2)
  };
  ctx_process (ctx, command);
}

void ctx_round_rectangle (Ctx *ctx,
                          float x0, float y0,
                          float w, float h,
                          float radius)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_ROUND_RECTANGLE, x0, y0),
    ctx_f (CTX_CONT,      w, h),
    ctx_f (CTX_CONT,      radius, 0)
  };
  ctx_process (ctx, command);
}

void ctx_view_box (Ctx *ctx,
                   float x0, float y0,
                   float w, float h)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_VIEW_BOX, x0, y0),
    ctx_f (CTX_CONT,     w, h)
  };
  ctx_process (ctx, command);
}

void ctx_rectangle (Ctx *ctx,
                    float x0, float y0,
                    float w, float h)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_RECTANGLE, x0, y0),
    ctx_f (CTX_CONT,      w, h)
  };
  ctx_process (ctx, command);
}

void ctx_rel_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CTX_PROCESS_F (CTX_REL_LINE_TO,x,y);
}

void ctx_rel_move_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    {
      CTX_PROCESS_F (CTX_MOVE_TO,x,y);
      return;
    }
  CTX_PROCESS_F (CTX_REL_MOVE_TO,x,y);
}

void ctx_line_cap (Ctx *ctx, CtxLineCap cap)
{
  if (ctx->state.gstate.line_cap != cap)
    CTX_PROCESS_U8 (CTX_LINE_CAP, cap);
}
void ctx_fill_rule (Ctx *ctx, CtxFillRule fill_rule)
{
  if (ctx->state.gstate.fill_rule != fill_rule)
    CTX_PROCESS_U8 (CTX_FILL_RULE, fill_rule);
}
void ctx_line_join (Ctx *ctx, CtxLineJoin join)
{
  if (ctx->state.gstate.line_join != join)
    CTX_PROCESS_U8 (CTX_LINE_JOIN, join);
}
void ctx_blend_mode (Ctx *ctx, CtxBlend mode)
{
  CTX_PROCESS_U8 (CTX_BLEND_MODE, mode);
}
void ctx_compositing_mode (Ctx *ctx, CtxCompositingMode mode)
{
  CTX_PROCESS_U8 (CTX_COMPOSITING_MODE, mode);
}
void ctx_text_align (Ctx *ctx, CtxTextAlign text_align)
{
  CTX_PROCESS_U8 (CTX_TEXT_ALIGN, text_align);
}
void ctx_text_baseline (Ctx *ctx, CtxTextBaseline text_baseline)
{
  CTX_PROCESS_U8 (CTX_TEXT_BASELINE, text_baseline);
}
void ctx_text_direction (Ctx *ctx, CtxTextDirection text_direction)
{
  CTX_PROCESS_U8 (CTX_TEXT_DIRECTION, text_direction);
}

void
ctx_rel_curve_to (Ctx *ctx,
                  float x0, float y0,
                  float x1, float y1,
                  float x2, float y2)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[3]=
  {
    ctx_f (CTX_REL_CURVE_TO, x0, y0),
    ctx_f (CTX_CONT, x1, y1),
    ctx_f (CTX_CONT, x2, y2)
  };
  ctx_process (ctx, command);
}

void
ctx_rel_quad_to (Ctx *ctx,
                 float cx, float cy,
                 float x,  float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[2]=
  {
    ctx_f (CTX_REL_QUAD_TO, cx, cy),
    ctx_f (CTX_CONT, x, y)
  };
  ctx_process (ctx, command);
}

void
ctx_quad_to (Ctx *ctx,
             float cx, float cy,
             float x,  float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[2]=
  {
    ctx_f (CTX_QUAD_TO, cx, cy),
    ctx_f (CTX_CONT, x, y)
  };
  ctx_process (ctx, command);
}

void ctx_arc (Ctx  *ctx,
              float x0, float y0,
              float radius,
              float angle1, float angle2,
              int   direction)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_ARC, x0, y0),
    ctx_f (CTX_CONT, radius, angle1),
    ctx_f (CTX_CONT, angle2, direction)
  };
  ctx_process (ctx, command);
}

static int ctx_coords_equal (float x1, float y1, float x2, float y2, float tol)
{
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
}

static float
ctx_point_seg_dist_sq (float x, float y,
                       float vx, float vy, float wx, float wy)
{
  float l2 = ctx_pow2 (vx-wx) + ctx_pow2 (vy-wy);
  if (l2 < 0.0001)
    { return ctx_pow2 (x-vx) + ctx_pow2 (y-vy); }
  float t = ( (x - vx) * (wx - vx) + (y - vy) * (wy - vy) ) / l2;
  t = ctx_maxf (0, ctx_minf (1, t) );
  float ix = vx + t * (wx - vx);
  float iy = vy + t * (wy - vy);
  return ctx_pow2 (x-ix) + ctx_pow2 (y-iy);
}

static void
ctx_normalize (float *x, float *y)
{
  float length = ctx_hypotf ( (*x), (*y) );
  if (length > 1e-6f)
    {
      float r = 1.0f / length;
      *x *= r;
      *y *= r;
    }
}

void
ctx_arc_to (Ctx *ctx, float x1, float y1, float x2, float y2, float radius)
{
  // XXX : should partially move into rasterizer to preserve comand
  //       even if an arc preserves all geometry, just to ensure roundtripping
  //       of data
  /* from nanovg - but not quite working ; uncertain if arc or wrong
   * transfusion is the cause.
   */
  float x0 = ctx->state.x;
  float y0 = ctx->state.y;
  float dx0,dy0, dx1,dy1, a, d, cx,cy, a0,a1;
  int dir;
  if (!ctx->state.has_moved)
    { return; }
  if (1)
    {
      // Handle degenerate cases.
      if (ctx_coords_equal (x0,y0, x1,y1, 0.5f) ||
          ctx_coords_equal (x1,y1, x2,y2, 0.5f) ||
          ctx_point_seg_dist_sq (x1,y1, x0,y0, x2,y2) < 0.5 ||
          radius < 0.5)
        {
          ctx_line_to (ctx, x1,y1);
          return;
        }
    }
  // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
  dx0 = x0-x1;
  dy0 = y0-y1;
  dx1 = x2-x1;
  dy1 = y2-y1;
  ctx_normalize (&dx0,&dy0);
  ctx_normalize (&dx1,&dy1);
  a = ctx_acosf (dx0*dx1 + dy0*dy1);
  d = radius / ctx_tanf (a/2.0f);
#if 0
  if (d > 10000.0f)
    {
      ctx_line_to (ctx, x1, y1);
      return;
    }
#endif
  if ( (dx1*dy0 - dx0*dy1) > 0.0f)
    {
      cx = x1 + dx0*d + dy0*radius;
      cy = y1 + dy0*d + -dx0*radius;
      a0 = ctx_atan2f (dx0, -dy0);
      a1 = ctx_atan2f (-dx1, dy1);
      dir = 0;
    }
  else
    {
      cx = x1 + dx0*d + -dy0*radius;
      cy = y1 + dy0*d + dx0*radius;
      a0 = ctx_atan2f (-dx0, dy0);
      a1 = ctx_atan2f (dx1, -dy1);
      dir = 1;
    }
  ctx_arc (ctx, cx, cy, radius, a0, a1, dir);
}

void
ctx_rel_arc_to (Ctx *ctx, float x1, float y1, float x2, float y2, float radius)
{
  x1 += ctx->state.x;
  y1 += ctx->state.y;
  x2 += ctx->state.x;
  y2 += ctx->state.y;
  ctx_arc_to (ctx, x1, y1, x2, y2, radius);
}

void
ctx_exit (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_EXIT);
}

#include <stdio.h>
#include <unistd.h>

void
ctx_flush (Ctx *ctx)
{
  /* XXX: should be fully moved into the renderers
   *      to permit different behavior and get rid
   *      of the extranous flush() vfunc.
   */
  ctx->rev++;
//  CTX_PROCESS_VOID (CTX_FLUSH);
#if 0
  //printf (" \e[?2222h");
  ctx_renderstream_compact (&ctx->renderstream);
  for (int i = 0; i < ctx->renderstream.count - 1; i++)
    {
      CtxEntry *entry = &ctx->renderstream.entries[i];
      fwrite (entry, 9, 1, stdout);
#if 0
      uint8_t  *buf = (void *) entry;
      for (int j = 0; j < 9; j++)
        { printf ("%c", buf[j]); }
#endif
    }
  printf ("Xx.Xx.Xx.");
  fflush (NULL);
#endif
  if (ctx->renderer && ctx->renderer->flush)
    ctx->renderer->flush (ctx->renderer);
  ctx->renderstream.count = 0;
  ctx_state_init (&ctx->state);
}

////////////////////////////////////////

void
ctx_interpret_style (CtxState *state, CtxEntry *entry, void *data)
{
  CtxCommand *c = (CtxCommand *) entry;
  switch (entry->code)
    {
      case CTX_LINE_WIDTH:
        state->gstate.line_width = ctx_arg_float (0);
        break;
#if CTX_ENABLE_SHADOW_BLUR
      case CTX_SHADOW_BLUR:
        state->gstate.shadow_blur = ctx_arg_float (0);
        break;
      case CTX_SHADOW_OFFSET_X:
        state->gstate.shadow_offset_x = ctx_arg_float (0);
        break;
      case CTX_SHADOW_OFFSET_Y:
        state->gstate.shadow_offset_y = ctx_arg_float (0);
        break;
#endif
      case CTX_LINE_CAP:
        state->gstate.line_cap = (CtxLineCap) ctx_arg_u8 (0);
        break;
      case CTX_FILL_RULE:
        state->gstate.fill_rule = (CtxFillRule) ctx_arg_u8 (0);
        break;
      case CTX_LINE_JOIN:
        state->gstate.line_join = (CtxLineJoin) ctx_arg_u8 (0);
        break;
      case CTX_COMPOSITING_MODE:
        state->gstate.compositing_mode = (CtxCompositingMode) ctx_arg_u8 (0);
        break;
      case CTX_BLEND_MODE:
        state->gstate.blend_mode = (CtxBlend) ctx_arg_u8 (0);
        break;
      case CTX_TEXT_ALIGN:
        ctx_state_set (state, CTX_text_align, ctx_arg_u8 (0) );
        break;
      case CTX_TEXT_BASELINE:
        ctx_state_set (state, CTX_text_baseline, ctx_arg_u8 (0) );
        break;
      case CTX_TEXT_DIRECTION:
        ctx_state_set (state, CTX_text_direction, ctx_arg_u8 (0) );
        break;
      case CTX_GLOBAL_ALPHA:
        state->gstate.global_alpha_u8 = ctx_float_to_u8 (ctx_arg_float (0) );
        state->gstate.global_alpha_f = ctx_arg_float (0);
        break;
      case CTX_FONT_SIZE:
        state->gstate.font_size = ctx_arg_float (0);
        break;
      case CTX_MITER_LIMIT:
        state->gstate.miter_limit = ctx_arg_float (0);
        break;

      case CTX_COLOR:
        {
          CtxColor *color = &state->gstate.source.color;
          state->gstate.source.type = CTX_SOURCE_COLOR;
          switch ( (int) ctx_arg_float (0) )
            {
              case CTX_RGB:
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, 1.0f);
                break;
              case CTX_RGBA:
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
              case CTX_DRGBA:
                ctx_color_set_drgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYKA:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_CMYK:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
              case CTX_DCMYKA:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_DCMYK:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
#endif
              case CTX_GRAYA:
                ctx_color_set_graya (state, color, c->graya.g, c->graya.a);
                break;
              case CTX_GRAY:
                ctx_color_set_graya (state, color, c->graya.g, 1.0f);
                break;
            }
        }
        break;
      case CTX_SET_RGBA_U8:
        //ctx_source_deinit (&state->gstate.source);
        state->gstate.source.type = CTX_SOURCE_COLOR;
        ctx_color_set_RGBA8 (state, &state->gstate.source.color,
                             ctx_arg_u8 (0),
                             ctx_arg_u8 (1),
                             ctx_arg_u8 (2),
                             ctx_arg_u8 (3) );
        //for (int i = 0; i < 4; i ++)
        //  state->gstate.source.color.rgba[i] = ctx_arg_u8(i);
        break;
#if 0
      case CTX_SET_RGBA_STROKE:
        //ctx_source_deinit (&state->gstate.source);
        state->gstate.source_stroke = state->gstate.source;
        state->gstate.source_stroke.type = CTX_SOURCE_COLOR;
        for (int i = 0; i < 4; i ++)
          { state->gstate.source_stroke.color.rgba[i] = ctx_arg_u8 (i); }
        break;
#endif
      //case CTX_TEXTURE:
      //  state->gstate.source.type = CTX_SOURCE_
      //  break;
      case CTX_LINEAR_GRADIENT:
        {
          float x0 = ctx_arg_float (0);
          float y0 = ctx_arg_float (1);
          float x1 = ctx_arg_float (2);
          float y1 = ctx_arg_float (3);
          float dx, dy, length, start, end;
          length = ctx_hypotf (x1-x0,y1-y0);
          dx = (x1-x0) / length;
          dy = (y1-y0) / length;
          start = (x0 * dx + y0 * dy) / length;
          end =   (x1 * dx + y1 * dy) / length;
          state->gstate.source.linear_gradient.length = length;
          state->gstate.source.linear_gradient.dx = dx;
          state->gstate.source.linear_gradient.dy = dy;
          state->gstate.source.linear_gradient.start = start;
          state->gstate.source.linear_gradient.end = end;
          state->gstate.source.linear_gradient.rdelta = (end-start)!=0.0?1.0f/(end - start):1.0;
          state->gstate.source.type = CTX_SOURCE_LINEAR_GRADIENT;
          state->gstate.source.transform = state->gstate.transform;
          ctx_matrix_invert (&state->gstate.source.transform);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          float x0 = ctx_arg_float (0);
          float y0 = ctx_arg_float (1);
          float r0 = ctx_arg_float (2);
          float x1 = ctx_arg_float (3);
          float y1 = ctx_arg_float (4);
          float r1 = ctx_arg_float (5);
          state->gstate.source.radial_gradient.x0 = x0;
          state->gstate.source.radial_gradient.y0 = y0;
          state->gstate.source.radial_gradient.r0 = r0;
          state->gstate.source.radial_gradient.x1 = x1;
          state->gstate.source.radial_gradient.y1 = y1;
          state->gstate.source.radial_gradient.r1 = r1;
          state->gstate.source.radial_gradient.rdelta = (r1 - r0) != 0.0 ? 1.0f/(r1-r0):0.0;
          state->gstate.source.type      = CTX_SOURCE_RADIAL_GRADIENT;
          state->gstate.source.transform = state->gstate.transform;
          ctx_matrix_invert (&state->gstate.source.transform);
        }
        break;
    }
}

void
ctx_interpret_transforms (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
    {
      case CTX_SAVE:
        ctx_gstate_push (state);
        break;
      case CTX_RESTORE:
        ctx_gstate_pop (state);
        break;
      case CTX_IDENTITY:
        ctx_matrix_identity (&state->gstate.transform);
        break;
      case CTX_TRANSLATE:
        ctx_matrix_translate (&state->gstate.transform,
                              ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_SCALE:
        ctx_matrix_scale (&state->gstate.transform,
                          ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_ROTATE:
        ctx_matrix_rotate (&state->gstate.transform, ctx_arg_float (0) );
        break;
      case CTX_APPLY_TRANSFORM:
        {
          CtxMatrix m;
          ctx_matrix_set (&m,
                          ctx_arg_float (0), ctx_arg_float (1),
                          ctx_arg_float (2), ctx_arg_float (3),
                          ctx_arg_float (4), ctx_arg_float (5) );
          ctx_matrix_multiply (&state->gstate.transform,
                               &state->gstate.transform, &m); // XXX verify order
        }
#if 0
        ctx_matrix_set (&state->gstate.transform,
                        ctx_arg_float (0), ctx_arg_float (1),
                        ctx_arg_float (2), ctx_arg_float (3),
                        ctx_arg_float (4), ctx_arg_float (5) );
#endif
        break;
    }
}

/*
 * this transforms the contents of entry according to ctx->transformation
 */
void
ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data)
{
  CtxCommand *c = (CtxCommand *) entry;
  float start_x = state->x;
  float start_y = state->y;
  int had_moved = state->has_moved;
  switch (entry->code)
    {
      case CTX_MOVE_TO:
      case CTX_LINE_TO:
        {
          float x = c->c.x0;
          float y = c->c.y0;
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              _ctx_user_to_device (state, &x, &y);
              ctx_arg_float (0) = x;
              ctx_arg_float (1) = y;
            }
        }
        break;
      case CTX_ARC:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            float temp;
            _ctx_user_to_device (state, &c->arc.x, &c->arc.y);
            temp = 0;
            _ctx_user_to_device_distance (state, &c->arc.radius, &temp);
          }
        break;
      case CTX_LINEAR_GRADIENT:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
        {
        _ctx_user_to_device (state, &c->linear_gradient.x1, &c->linear_gradient.y1);
        _ctx_user_to_device (state, &c->linear_gradient.x2, &c->linear_gradient.y2);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
        {
          float temp;
          _ctx_user_to_device (state, &c->radial_gradient.x1, &c->radial_gradient.y1);
          temp = 0;
          _ctx_user_to_device_distance (state, &c->radial_gradient.r1, &temp);
          _ctx_user_to_device (state, &c->radial_gradient.x2, &c->radial_gradient.y2);
          temp = 0;
          _ctx_user_to_device_distance (state, &c->radial_gradient.r2, &temp);
        }
        break;
      case CTX_CURVE_TO:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            for (int c = 0; c < 3; c ++)
              {
                float x = entry[c].data.f[0];
                float y = entry[c].data.f[1];
                _ctx_user_to_device (state, &x, &y);
                entry[c].data.f[0] = x;
                entry[c].data.f[1] = y;
              }
          }
        break;
      case CTX_QUAD_TO:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            for (int c = 0; c < 2; c ++)
              {
                float x = entry[c].data.f[0];
                float y = entry[c].data.f[1];
                _ctx_user_to_device (state, &x, &y);
                entry[c].data.f[0] = x;
                entry[c].data.f[1] = y;
              }
          }
        break;
      case CTX_REL_MOVE_TO:
      case CTX_REL_LINE_TO:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            for (int c = 0; c < 1; c ++)
              {
                float x = state->x;
                float y = state->y;
                _ctx_user_to_device (state, &x, &y);
                entry[c].data.f[0] = x;
                entry[c].data.f[1] = y;
              }
            if (entry->code == CTX_REL_MOVE_TO)
              { entry->code = CTX_MOVE_TO; }
            else
              { entry->code = CTX_LINE_TO; }
          }
        break;
      case CTX_REL_CURVE_TO:
        {
          float nx = state->x + ctx_arg_float (4);
          float ny = state->y + ctx_arg_float (5);
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              for (int c = 0; c < 3; c ++)
                {
                  float x = nx + entry[c].data.f[0];
                  float y = ny + entry[c].data.f[1];
                  _ctx_user_to_device (state, &x, &y);
                  entry[c].data.f[0] = x;
                  entry[c].data.f[1] = y;
                }
              entry->code = CTX_CURVE_TO;
            }
        }
        break;
      case CTX_REL_QUAD_TO:
        {
          float nx = state->x + ctx_arg_float (2);
          float ny = state->y + ctx_arg_float (3);
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              for (int c = 0; c < 2; c ++)
                {
                  float x = nx + entry[c].data.f[0];
                  float y = ny + entry[c].data.f[1];
                  _ctx_user_to_device (state, &x, &y);
                  entry[c].data.f[0] = x;
                  entry[c].data.f[1] = y;
                }
              entry->code = CTX_QUAD_TO;
            }
        }
        break;
    }
  if ((((Ctx *) (data) )->transformation & CTX_TRANSFORMATION_RELATIVE))
    {
      int components = 0;
      _ctx_user_to_device (state, &start_x, &start_y);
      switch (entry->code)
        {
          case CTX_MOVE_TO:
            if (had_moved) { components = 1; }
            break;
          case CTX_LINE_TO:
            components = 1;
            break;
          case CTX_CURVE_TO:
            components = 3;
            break;
          case CTX_QUAD_TO:
            components = 2;
            break;
        }
      if (components)
        {
          for (int c = 0; c < components; c++)
            {
              entry[c].data.f[0] -= start_x;
              entry[c].data.f[1] -= start_y;
            }
          switch (entry->code)
            {
              case CTX_MOVE_TO:
                entry[0].code = CTX_REL_MOVE_TO;
                break;
              case CTX_LINE_TO:
                entry[0].code = CTX_REL_LINE_TO;
                break;
                break;
              case CTX_CURVE_TO:
                entry[0].code = CTX_REL_CURVE_TO;
                break;
              case CTX_QUAD_TO:
                entry[0].code = CTX_REL_QUAD_TO;
                break;
            }
        }
    }
}

void
ctx_interpret_pos_bare (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
    {
      case CTX_RESET:
        ctx_state_init (state);
        break;
      case CTX_CLIP:
      case CTX_BEGIN_PATH:
      case CTX_FILL:
      case CTX_STROKE:
        state->has_moved = 0;
        break;
      case CTX_MOVE_TO:
      case CTX_LINE_TO:
        {
          float x = ctx_arg_float (0);
          float y = ctx_arg_float (1);
          state->x = x;
          state->y = y;
          if (!state->has_moved)
            {
              state->has_moved = 1;
            }
        }
        break;
      case CTX_CURVE_TO:
        state->x = ctx_arg_float (4);
        state->y = ctx_arg_float (5);
        if (!state->has_moved)
          {
            state->has_moved = 1;
          }
        break;
      case CTX_QUAD_TO:
        state->x = ctx_arg_float (2);
        state->y = ctx_arg_float (3);
        if (!state->has_moved)
          {
            state->has_moved = 1;
          }
        break;
      case CTX_ARC:
        state->x = ctx_arg_float (0) + ctx_cosf (ctx_arg_float (4) ) * ctx_arg_float (2);
        state->y = ctx_arg_float (1) + ctx_sinf (ctx_arg_float (4) ) * ctx_arg_float (2);
        break;
      case CTX_REL_MOVE_TO:
      case CTX_REL_LINE_TO:
        state->x += ctx_arg_float (0);
        state->y += ctx_arg_float (1);
        break;
      case CTX_REL_CURVE_TO:
        state->x += ctx_arg_float (4);
        state->y += ctx_arg_float (5);
        break;
      case CTX_REL_QUAD_TO:
        state->x += ctx_arg_float (2);
        state->y += ctx_arg_float (3);
        break;
        // XXX missing some smooths
    }
}

void
ctx_interpret_pos (CtxState *state, CtxEntry *entry, void *data)
{
  if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) ||
       ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_RELATIVE) )
    {
      ctx_interpret_pos_transform (state, entry, data);
    }
  ctx_interpret_pos_bare (state, entry, data);
}

CTX_STATIC void
ctx_state_init (CtxState *state)
{
  ctx_memset (state, 0, sizeof (CtxState) );
  state->gstate.global_alpha_u8 = 255;
  state->gstate.global_alpha_f  = 1.0;
  state->gstate.font_size       = 12;
  state->gstate.line_width      = 2.0;
  ctx_state_set (state, CTX_line_spacing, 1.0f);
  state->min_x                  = 8192;
  state->min_y                  = 8192;
  state->max_x                  = -8192;
  state->max_y                  = -8192;
  ctx_matrix_identity (&state->gstate.transform);
}

void _ctx_set_transformation (Ctx *ctx, int transformation)
{
  ctx->transformation = transformation;
}

static void
_ctx_init (Ctx *ctx)
{
  for (int i = 0; i <256;i++)
    ctx_u8_float[i] = i/255.0f;

  ctx_state_init (&ctx->state);
  ctx->renderer = NULL;
#if CTX_CURRENT_PATH
  ctx->current_path.flags |= CTX_RENDERSTREAM_CURRENT_PATH;
#endif
  //ctx->transformation |= (CtxTransformation) CTX_TRANSFORMATION_SCREEN_SPACE;
  //ctx->transformation |= (CtxTransformation) CTX_TRANSFORMATION_RELATIVE;
#if CTX_BITPACK
  ctx->renderstream.flags |= CTX_TRANSFORMATION_BITPACK;
#endif
}

static void ctx_setup ();

#if CTX_RENDERSTREAM_STATIC
static Ctx ctx_state;
#endif

void ctx_set_renderer (Ctx  *ctx,
                       void *renderer)
{
  if (ctx->renderer && ctx->renderer->free)
    ctx->renderer->free (ctx->renderer);
  ctx->renderer = (CtxImplementation*)renderer;
}

void *ctx_get_renderer (Ctx *ctx)
{
  return ctx->renderer;
}

Ctx *
ctx_new (void)
{
  ctx_setup ();
#if CTX_RENDERSTREAM_STATIC
  Ctx *ctx = &ctx_state;
#else
  Ctx *ctx = (Ctx *) malloc (sizeof (Ctx) );
#endif
  ctx_memset (ctx, 0, sizeof (Ctx) );
  _ctx_init (ctx);
  return ctx;
}

void
ctx_renderstream_deinit (CtxRenderstream *renderstream)
{
#if !CTX_RENDERSTREAM_STATIC
  if (renderstream->entries && ! (renderstream->flags & CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES) )
    {
      free (renderstream->entries); 
    }
#endif
  renderstream->entries = NULL;
  renderstream->size = 0;
}

static void ctx_deinit (Ctx *ctx)
{
  if (ctx->renderer)
    {
      if (ctx->renderer->free)
        ctx->renderer->free (ctx->renderer);
      ctx->renderer    = NULL;
    }
  ctx_renderstream_deinit (&ctx->renderstream);
#if CTX_CURRENT_PATH
  ctx_renderstream_deinit (&ctx->current_path);
#endif
}

void ctx_free (Ctx *ctx)
{
  if (!ctx)
    { return; }
#if CTX_EVENTS
  ctx_clear_bindings (ctx);
#endif
  ctx_deinit (ctx);
#if !CTX_RENDERSTREAM_STATIC
  free (ctx);
#endif
}

Ctx *ctx_new_for_renderstream (void *data, size_t length)
{
  Ctx *ctx = ctx_new ();
  ctx->renderstream.flags   |= CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES;
  ctx->renderstream.entries  = (CtxEntry *) data;
  ctx->renderstream.count    = length / sizeof (CtxEntry);
  return ctx;
}

////////////////////////////////////


#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
void
ctx_gradient_cache_reset (void);
#endif

#else
void
ctx_gradient_cache_reset (void)
{
}
#endif

static void ctx_setup ()
{
  ctx_font_setup ();
}

void
ctx_render_ctx (Ctx *ctx, Ctx *d_ctx)
{
  CtxIterator iterator;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->renderstream, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_process (d_ctx, &command->entry); }
}
