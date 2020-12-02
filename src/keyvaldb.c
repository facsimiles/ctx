#include "ctx-split.h"

float ctx_state_get (CtxState *state, uint32_t hash)
{
  for (int i = state->gstate.keydb_pos-1; i>=0; i--)
    {
      if (state->keydb[i].key == hash)
        { return state->keydb[i].value; }
    }
  return -0.0;
}

void ctx_state_set (CtxState *state, uint32_t key, float value)
{
  if (key != CTX_new_state)
    {
      if (ctx_state_get (state, key) == value)
        { return; }
      for (int i = state->gstate.keydb_pos-1;
           state->keydb[i].key != CTX_new_state && i >=0;
           i--)
        {
          if (state->keydb[i].key == key)
            {
              state->keydb[i].value = value;
              return;
            }
        }
    }
  if (state->gstate.keydb_pos >= CTX_MAX_KEYDB)
    { return; }
  state->keydb[state->gstate.keydb_pos].key = key;
  state->keydb[state->gstate.keydb_pos].value = value;
  state->gstate.keydb_pos++;
}


#define CTX_KEYDB_STRING_START (-90000.0)
#define CTX_KEYDB_STRING_END   (CTX_KEYDB_STRING_START + CTX_STRINGPOOL_SIZE)

static int ctx_float_is_string (float val)
{
  return val >= CTX_KEYDB_STRING_START && val <= CTX_KEYDB_STRING_END;
}

CTX_STATIC int ctx_float_to_string_index (float val)
{
  int idx = -1;
  if (ctx_float_is_string (val))
  {
    idx = val - CTX_KEYDB_STRING_START;
  }
  return idx;
}

CTX_STATIC float ctx_string_index_to_float (int index)
{
  return CTX_KEYDB_STRING_START + index;
}

void *ctx_state_get_blob (CtxState *state, uint32_t key)
{
  float stored = ctx_state_get (state, key);
  int idx = ctx_float_to_string_index (stored);
  if (idx >= 0)
  {
     return &state->stringpool[idx];
  }
  // format number as string?
  return NULL;
}

const char *ctx_state_get_string (CtxState *state, uint32_t key)
{
  const char *ret = (char*)ctx_state_get_blob (state, key);
  if (ret && ret[0] == 127)
    return NULL;
  return ret;
}


static void ctx_state_set_blob (CtxState *state, uint32_t key, uint8_t *data, int len)
{
  int idx = state->gstate.stringpool_pos;

  if (idx + len > CTX_STRINGPOOL_SIZE)
  {
    ctx_log ("blowing varpool size [%c..]\n", data[0]);
    fprintf (stderr, "blowing varpool size [%c%c%c..]\n", data[0],data[1], data[1]?data[2]:0);
#if 0
    for (int i = 0; i< CTX_STRINGPOOL_SIZE; i++)
    {
       if (i==0) fprintf (stderr, "\n%i ", i);
       else      fprintf (stderr, "%c", state->stringpool[i]);
    }
#endif
    return;
  }

  memcpy (&state->stringpool[idx], data, len);
  state->gstate.stringpool_pos+=len;
  state->stringpool[state->gstate.stringpool_pos++]=0;
  ctx_state_set (state, key, ctx_string_index_to_float (idx));
}

CTX_STATIC void ctx_state_set_string (CtxState *state, uint32_t key, const char *string)
{
  float old_val = ctx_state_get (state, key);
  int   old_idx = ctx_float_to_string_index (old_val);

  if (old_idx >= 0)
  {
    const char *old_string = ctx_state_get_string (state, key);
    if (old_string && !strcmp (old_string, string))
      return;
  }

  if (ctx_str_is_number (string))
  {
    ctx_state_set (state, key, strtod (string, NULL));
    return;
  }
  // should do same with color
 
  // XXX should special case when the string modified is at the
  //     end of the stringpool.
  ctx_state_set_blob (state, key, (uint8_t*)string, strlen(string));
}

static int ctx_state_get_color (CtxState *state, uint32_t key, CtxColor *color)
{
  CtxColor *stored = (CtxColor*)ctx_state_get_blob (state, key);
  if (stored)
  {
    if (stored->magic == 127)
    {
      *color = *stored;
      return 0;
    }
  }
  return -1;
}

CTX_STATIC void ctx_state_set_color (CtxState *state, uint32_t key, CtxColor *color)
{
  CtxColor mod_color;
  CtxColor old_color;
  mod_color = *color;
  mod_color.magic = 127;
  if (ctx_state_get_color (state, key, &old_color)==0)
  {
    if (!memcmp (&mod_color, &old_color, sizeof (mod_color)))
      return;
  }
  ctx_state_set_blob (state, key, (uint8_t*)&mod_color, sizeof (CtxColor));
}

const char *ctx_get_string (Ctx *ctx, uint32_t hash)
{
  return ctx_state_get_string (&ctx->state, hash);
}
float ctx_get_float (Ctx *ctx, uint32_t hash)
{
  return ctx_state_get (&ctx->state, hash);
}
int ctx_get_int (Ctx *ctx, uint32_t hash)
{
  return ctx_state_get (&ctx->state, hash);
}
void ctx_set_float (Ctx *ctx, uint32_t hash, float value)
{
  ctx_state_set (&ctx->state, hash, value);
}
void ctx_set_string (Ctx *ctx, uint32_t hash, const char *value)
{
  ctx_state_set_string (&ctx->state, hash, value);
}
void ctx_set_color (Ctx *ctx, uint32_t hash, CtxColor *color)
{
  ctx_state_set_color (&ctx->state, hash, color);
}
int  ctx_get_color (Ctx *ctx, uint32_t hash, CtxColor *color)
{
  return ctx_state_get_color (&ctx->state, hash, color);
}
int ctx_is_set (Ctx *ctx, uint32_t hash)
{
  return ctx_get_float (ctx, hash) != -0.0f;
}
int ctx_is_set_now (Ctx *ctx, uint32_t hash)
{
  return ctx_is_set (ctx, hash);
}
