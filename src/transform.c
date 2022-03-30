#ifndef __CTX_TRANSFORM
#define __CTX_TRANSFORM
#include "ctx-split.h"


static inline void
_ctx_matrix_apply_transform_only_x (const CtxMatrix *m, float *x, float y_in)
{
  //float x_in = *x;
  //*x = ( (x_in * m->m[0][0]) + (y_in * m->m[1][0]) + m->m[2][0]);
  float y_res;
  _ctx_matrix_apply_transform (m, x, &y_res);
}

void
ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y)
{
  _ctx_matrix_apply_transform (m, x, y);
}

static inline int
determine_transform_type (const CtxMatrix *m)
{
  if (m->m[2][0] != 0.0f ||
      m->m[2][1] != 0.0f ||
      m->m[2][2] != 1.0f)
    return 3;
  if (m->m[0][1] != 0.0f ||
      m->m[1][0] != 0.0f)
    return 3;
  if (m->m[0][2] != 0.0f ||
      m->m[1][2] != 0.0f ||
      m->m[0][0] != 1.0f ||
      m->m[1][1] != 1.0f)
    return 2;
  return 1;
}

#define TRANSFORM_SHIFT (10)
#define TRANSFORM_SCALE (1<<TRANSFORM_SHIFT)

static inline void
_ctx_transform_prime (CtxState *state)
{
   state->gstate.transform_type = 
     determine_transform_type (&state->gstate.transform);

   for (int c = 0; c < 3; c++)
   {
     state->gstate.prepped_transform.m[0][c] =
             (int)(state->gstate.transform.m[0][c] * TRANSFORM_SCALE);
     state->gstate.prepped_transform.m[1][c] =
             (int)(state->gstate.transform.m[1][c] * TRANSFORM_SCALE);
     state->gstate.prepped_transform.m[2][c] =
             (int)(state->gstate.transform.m[2][c] * TRANSFORM_SCALE);
   }
}

static inline void
_ctx_matrix_apply_transform_perspective_fixed (const Ctx16f16Matrix *m, int x_in, int y_in,
                int *x_out, int *y_out)
{
  int w  = (((x_in * m->m[2][0] +
               y_in * m->m[2][1])>>TRANSFORM_SHIFT) +
                     (m->m[2][2]));
  int w_recip = w?TRANSFORM_SCALE / w:0;


  *x_out = ((((((x_in * m->m[0][0] +
               y_in * m->m[0][1])>>TRANSFORM_SHIFT) +
                     (m->m[0][2])) * w_recip)>>TRANSFORM_SHIFT) * CTX_SUBDIV) >> TRANSFORM_SHIFT;
  *y_out = ((((((x_in * m->m[1][0] +
               y_in * m->m[1][1])>>TRANSFORM_SHIFT) +
                     (m->m[1][2])) * w_recip)>>TRANSFORM_SHIFT) * CTX_FULL_AA) >> TRANSFORM_SHIFT;

}

static inline void
_ctx_matrix_apply_transform_affine_fixed (const Ctx16f16Matrix *m, int x_in, int y_in,
                int *x_out, int *y_out)
{
  *x_out = ((((x_in * m->m[0][0] +
               y_in * m->m[0][1])>>TRANSFORM_SHIFT) +
                     (m->m[0][2])) * CTX_SUBDIV) >>TRANSFORM_SHIFT;
  *y_out = ((((x_in * m->m[1][0] +
               y_in * m->m[1][1])>>TRANSFORM_SHIFT) +
                     (m->m[1][2])) * CTX_FULL_AA) >>TRANSFORM_SHIFT;
}

static inline void
_ctx_matrix_apply_transform_scale_translate_fixed (const Ctx16f16Matrix *m, int x_in, int y_in, int *x_out, int *y_out)
{
  *x_out = ((((x_in * m->m[0][0])>>TRANSFORM_SHIFT) +
                     (m->m[0][2])) * CTX_SUBDIV) >>TRANSFORM_SHIFT;
  *y_out = ((((y_in * m->m[1][1])>>TRANSFORM_SHIFT) +
                     (m->m[1][2])) * CTX_FULL_AA) >>TRANSFORM_SHIFT;
}

static inline void
_ctx_user_to_device_prepped_fixed (CtxState *state, int x, int y, int *x_out, int *y_out)
{
  switch (state->gstate.transform_type)
  {
    case 0:
      _ctx_transform_prime (state);
      _ctx_user_to_device_prepped_fixed (state, x, y, x_out, y_out);
      break;
    case 1:  // identity
      *x_out = (x * CTX_SUBDIV) / TRANSFORM_SCALE;
      *y_out = (y * CTX_FULL_AA) / TRANSFORM_SCALE;
      break;
    case 2:  // scale/translate
      _ctx_matrix_apply_transform_scale_translate_fixed (&state->gstate.prepped_transform, x, y, x_out, y_out);
      break;
    case 3:  // affine
      _ctx_matrix_apply_transform_affine_fixed (&state->gstate.prepped_transform, x, y, x_out, y_out);
      break;
    case 4:  // perspective
      _ctx_matrix_apply_transform_perspective_fixed (&state->gstate.prepped_transform, x, y, x_out, y_out);
      break;
  }
}

static inline void
_ctx_user_to_device_prepped (CtxState *state, float x, float y, int *x_out, int *y_out)
{
  int x_in = (int)(x * TRANSFORM_SCALE);
  int y_in = (int)(y * TRANSFORM_SCALE);
  _ctx_user_to_device_prepped_fixed (state, x_in, y_in, x_out, y_out);
}

static inline void
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

CTX_STATIC inline void
_ctx_device_to_user (CtxState *state, float *x, float *y)
{
  CtxMatrix m = state->gstate.transform;
  ctx_matrix_invert (&m);
  _ctx_matrix_apply_transform (&m, x, y);
}

CTX_STATIC void
_ctx_device_to_user_distance (CtxState *state, float *x, float *y)
{
  CtxMatrix m = state->gstate.transform;
  ctx_matrix_invert (&m);
  _ctx_matrix_apply_transform (&m, x, y);
  *x -= m.m[2][0];
  *y -= m.m[2][1];
}

void ctx_device_to_user          (Ctx *ctx, float *x, float *y)
{
  _ctx_device_to_user (&ctx->state, x, y);
}

void ctx_device_to_user_distance (Ctx *ctx, float *x, float *y)
{
  _ctx_device_to_user_distance (&ctx->state, x, y);
}


CTX_STATIC void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f, float g, float h, float i)
{
  matrix->m[0][0] = a;
  matrix->m[0][1] = b;
  matrix->m[0][2] = c;
  matrix->m[1][0] = d;
  matrix->m[1][1] = e;
  matrix->m[1][2] = f;
  matrix->m[2][0] = g;
  matrix->m[2][1] = h;
  matrix->m[2][2] = i;
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
  transform.m[0][2] = x;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = 1.0f;
  transform.m[1][2] = y;
  transform.m[2][0] = 0.0f;
  transform.m[2][1] = 0.0f;
  transform.m[2][2] = 1.0f;
  _ctx_matrix_multiply (matrix, matrix, &transform);
}

void
ctx_matrix_scale (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = x;
  transform.m[0][1] = 0.0f;
  transform.m[0][2] = 0.0f;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = y;
  transform.m[1][2] = 0.0f;
  transform.m[2][0] = 0.0f;
  transform.m[2][1] = 0.0f;
  transform.m[2][2] = 1.0;
  _ctx_matrix_multiply (matrix, matrix, &transform);
}


void
ctx_matrix_rotate (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_sin = ctx_sinf (-angle);
  float val_cos = ctx_cosf (-angle);
  transform.m[0][0] = val_cos;
  transform.m[0][1] = val_sin;
  transform.m[0][2] = 0;
  transform.m[1][0] = -val_sin;
  transform.m[1][1] = val_cos;
  transform.m[1][2] = 0;
  transform.m[2][0] = 0.0f;
  transform.m[2][1] = 0.0f;
  transform.m[2][2] = 1.0f;
  _ctx_matrix_multiply (matrix, matrix, &transform);
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
ctx_apply_transform (Ctx *ctx, float a, float b,
                     float c, float d, 
                     float e, float f, float g, float h, float i)
{
  CtxEntry command[5]=
  {
    ctx_f (CTX_APPLY_TRANSFORM, a, b),
    ctx_f (CTX_CONT,            c, d),
    ctx_f (CTX_CONT,            e, f),
    ctx_f (CTX_CONT,            g, h),
    ctx_f (CTX_CONT,            i, 0)
  };
  ctx_process (ctx, command);
}

void
ctx_get_transform  (Ctx *ctx, float *a, float *b,
                    float *c, float *d,
                    float *e, float *f,
                    float *g, float *h,
                    float *i)
{
  if (a) { *a = ctx->state.gstate.transform.m[0][0]; }
  if (b) { *b = ctx->state.gstate.transform.m[0][1]; }
  if (c) { *c = ctx->state.gstate.transform.m[0][2]; }
  if (d) { *d = ctx->state.gstate.transform.m[1][0]; }
  if (e) { *e = ctx->state.gstate.transform.m[1][1]; }
  if (f) { *f = ctx->state.gstate.transform.m[1][2]; }
  if (g) { *g = ctx->state.gstate.transform.m[2][0]; }
  if (h) { *h = ctx->state.gstate.transform.m[2][1]; }
  if (i) { *i = ctx->state.gstate.transform.m[2][2]; }
}

void
ctx_source_transform (Ctx *ctx, float a, float b,  // hscale, hskew
                      float c, float d,  // vskew,  vscale
                      float e, float f,
                      float g, float h,
                      float i)  // htran,  vtran
{
  CtxEntry command[5]=
  {
    ctx_f (CTX_SOURCE_TRANSFORM, a, b),
    ctx_f (CTX_CONT,             c, d),
    ctx_f (CTX_CONT,             e, f),
    ctx_f (CTX_CONT,             g, h),
    ctx_f (CTX_CONT,             i, 0)
  };
  ctx_process (ctx, command);
}

void
ctx_source_transform_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_source_transform (ctx,
    matrix->m[0][0], matrix->m[0][1], matrix->m[0][2],
    matrix->m[1][0], matrix->m[1][1], matrix->m[1][2],
    matrix->m[2][0], matrix->m[2][1], matrix->m[2][2]
    
    );
}

void ctx_apply_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_apply_transform (ctx,
    matrix->m[0][0], matrix->m[0][1], matrix->m[0][2],
    matrix->m[1][0], matrix->m[1][1], matrix->m[1][2],
    matrix->m[2][0], matrix->m[2][1], matrix->m[2][2]);
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

static inline float
ctx_matrix_determinant (const CtxMatrix *m)
{
  float det = m->m[0][0] * (m->m[1][1] * m->m[2][2] -
                            m->m[1][2] * m->m[2][1])
              - m->m[0][1] * (m->m[1][0] * m->m[2][2] -
                              m->m [1][2] * m->m [2][0])
              + m->m[0][2] * (m->m[1][0] * m->m[2][1] -
                              m->m[1][1] * m->m[2][0]);
  return det;
}

void
ctx_matrix_invert (CtxMatrix *m)
{
  CtxMatrix t = *m;
  float c = 1.0f / ctx_matrix_determinant (m);

  m->m [0][0] = (t.m [1][1] * t.m [2][2] -
                   t.m [1][2] * t.m [2][1]) * c;
  m->m [1][0] = (t.m [1][2] * t.m [2][0] -
                   t.m [1][0] * t.m [2][2]) * c;
  m->m [2][0] = (t.m [1][0] * t.m [2][1] -
                   t.m [1][1] * t.m [2][0]) * c;

  m->m [0][1] = (t.m [0][2] * t.m [2][1] -
                   t.m [0][1] * t.m [2][2]) * c;
  m->m [1][1] = (t.m [0][0] * t.m [2][2] -
                   t.m [0][2] * t.m [2][0]) * c;
  m->m [2][1] = (t.m [0][1] * t.m [2][0] -
                   t.m [0][0] * t.m [2][1]) * c;

  m->m [0][2] = (t.m [0][1] * t.m [1][2] -
                   t.m [0][2] * t.m [1][1]) * c;
  m->m [1][2] = (t.m [0][2] * t.m [1][0] -
                   t.m [0][0] * t.m [1][2]) * c;
  m->m [2][2] = (t.m [0][0] * t.m [1][1] -
                   t.m [0][1] * t.m [1][0]) * c;
}

#endif
