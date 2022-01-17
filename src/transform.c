#ifndef __CTX_TRANSFORM
#define __CTX_TRANSFORM
#include "ctx-split.h"


static inline void
_ctx_matrix_apply_transform_only_x (const CtxMatrix *m, float *x, float y_in)
{
  float x_in = *x;
  *x = ( (x_in * m->m[0][0]) + (y_in * m->m[1][0]) + m->m[2][0]);
}

void
ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y)
{
  _ctx_matrix_apply_transform (m, x, y);
}

CTX_STATIC inline void
_ctx_user_to_device (CtxState *state, float *x, float *y)
{
  _ctx_matrix_apply_transform (&state->gstate.transform, x, y);
}

CTX_STATIC void
_ctx_user_to_device_distance (CtxState *state, float *x, float *y)
{
  const CtxMatrix *m = &state->gstate.transform;
  _ctx_matrix_apply_transform (m, x, y);
  *x -= m->m[2][0];
  *y -= m->m[2][1];
}

void ctx_user_to_device          (Ctx *ctx, float *x, float *y)
{
  _ctx_user_to_device (&ctx->state, x, y);
}
void ctx_user_to_device_distance (Ctx *ctx, float *x, float *y)
{
  _ctx_user_to_device_distance (&ctx->state, x, y);
}

CTX_STATIC void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f)
{
  matrix->m[0][0] = a;
  matrix->m[0][1] = b;
  matrix->m[1][0] = c;
  matrix->m[1][1] = d;
  matrix->m[2][0] = e;
  matrix->m[2][1] = f;
}


void
ctx_matrix_identity (CtxMatrix *matrix)
{
  _ctx_matrix_identity (matrix);
}

void
ctx_matrix_multiply (CtxMatrix       *result,
                     const CtxMatrix *t,
                     const CtxMatrix *s)
{
  _ctx_matrix_multiply (result, t, s);
}

void
ctx_matrix_translate (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = 1.0f;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] = x;
  transform.m[2][1] = y;
  _ctx_matrix_multiply (matrix, &transform, matrix);
}

void
ctx_matrix_scale (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = x;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = y;
  transform.m[2][0] = 0.0f;
  transform.m[2][1] = 0.0f;
  _ctx_matrix_multiply (matrix, &transform, matrix);
}


/* for multiples of 90 degree rotations, we return no rotation */
int
ctx_matrix_no_skew_or_rotate (CtxMatrix *matrix)
{
  if (matrix->m[0][1] != 0.0f) return 0;
  if (matrix->m[1][0] != 0.0f) return 0;
  return 1;
}

void
ctx_matrix_rotate (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_sin = ctx_sinf (angle);
  float val_cos = ctx_cosf (angle);
  transform.m[0][0] =  val_cos;
  transform.m[0][1] = val_sin;
  transform.m[1][0] = -val_sin;
  transform.m[1][1] = val_cos;
  transform.m[2][0] =     0.0f;
  transform.m[2][1] = 0.0f;
  _ctx_matrix_multiply (matrix, &transform, matrix);
}

#if 0
static void
ctx_matrix_skew_x (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = ctx_tanf (angle);
  transform.m[0][0] =    1.0f;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = val_tan;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f;
  transform.m[2][1] = 0.0f;
  _ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
ctx_matrix_skew_y (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = ctx_tanf (angle);
  transform.m[0][0] =    1.0f;
  transform.m[0][1] = val_tan;
  transform.m[1][0] =    0.0f;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f;
  transform.m[2][1] = 0.0f;
  _ctx_matrix_multiply (matrix, &transform, matrix);
}
#endif


void
ctx_identity (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_IDENTITY);
}



void
ctx_apply_transform (Ctx *ctx, float a, float b,  // hscale, hskew
                     float c, float d,  // vskew,  vscale
                     float e, float f)  // htran,  vtran
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_APPLY_TRANSFORM, a, b),
    ctx_f (CTX_CONT,            c, d),
    ctx_f (CTX_CONT,            e, f)
  };
  ctx_process (ctx, command);
}

void
ctx_get_transform  (Ctx *ctx, float *a, float *b,
                    float *c, float *d,
                    float *e, float *f)
{
  if (a) { *a = ctx->state.gstate.transform.m[0][0]; }
  if (b) { *b = ctx->state.gstate.transform.m[0][1]; }
  if (c) { *c = ctx->state.gstate.transform.m[1][0]; }
  if (d) { *d = ctx->state.gstate.transform.m[1][1]; }
  if (e) { *e = ctx->state.gstate.transform.m[2][0]; }
  if (f) { *f = ctx->state.gstate.transform.m[2][1]; }
}

void
ctx_source_transform (Ctx *ctx, float a, float b,  // hscale, hskew
                      float c, float d,  // vskew,  vscale
                      float e, float f)  // htran,  vtran
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SOURCE_TRANSFORM, a, b),
    ctx_f (CTX_CONT,             c, d),
    ctx_f (CTX_CONT,             e, f)
  };
  ctx_process (ctx, command);
}

void
ctx_source_transform_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_source_transform (ctx,
    matrix->m[0][0], matrix->m[0][1],
    matrix->m[1][0], matrix->m[1][1],
    matrix->m[2][0], matrix->m[2][1]);
}

void ctx_apply_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_apply_transform (ctx,
                       matrix->m[0][0], matrix->m[0][1],
                       matrix->m[1][0], matrix->m[1][1],
                       matrix->m[2][0], matrix->m[2][1]);
}

void ctx_get_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  *matrix = ctx->state.gstate.transform;
}

void ctx_set_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_identity (ctx);
  ctx_apply_matrix (ctx, matrix);
}

void ctx_rotate (Ctx *ctx, float x)
{
  if (x == 0.0f)
    return;
  CTX_PROCESS_F1 (CTX_ROTATE, x);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->drawlist.count--; }
}

void ctx_scale (Ctx *ctx, float x, float y)
{
  if (x == 1.0f && y == 1.0f)
    return;
  CTX_PROCESS_F (CTX_SCALE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->drawlist.count--; }
}

void ctx_translate (Ctx *ctx, float x, float y)
{
  if (x == 0.0f && y == 0.0f)
    return;
  CTX_PROCESS_F (CTX_TRANSLATE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->drawlist.count--; }
}

void
ctx_matrix_invert (CtxMatrix *m)
{
  CtxMatrix t = *m;
  float invdet, det = m->m[0][0] * m->m[1][1] -
                      m->m[1][0] * m->m[0][1];
  if (det > -0.0000001f && det < 0.0000001f)
    {
      m->m[0][0] = m->m[0][1] =
                     m->m[1][0] = m->m[1][1] =
                                    m->m[2][0] = m->m[2][1] = 0.0;
      return;
    }
  invdet = 1.0f / det;
  m->m[0][0] = t.m[1][1] * invdet;
  m->m[1][0] = -t.m[1][0] * invdet;
  m->m[2][0] = (t.m[1][0] * t.m[2][1] - t.m[1][1] * t.m[2][0]) * invdet;
  m->m[0][1] = -t.m[0][1] * invdet;
  m->m[1][1] = t.m[0][0] * invdet;
  m->m[2][1] = (t.m[0][1] * t.m[2][0] - t.m[0][0] * t.m[2][1]) * invdet ;
}



#endif
