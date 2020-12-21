#include "ctx-split.h"

CTX_STATIC int
ctx_conts_for_entry (CtxEntry *entry)
{
    switch (entry->code)
    {
      case CTX_DATA:
        return entry->data.u32[1];
      case CTX_LINEAR_GRADIENT:
        return 1;
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_APPLY_TRANSFORM:
      case CTX_COLOR:
      case CTX_ROUND_RECTANGLE:
      case CTX_SHADOW_COLOR:
        return 2;
      case CTX_RECTANGLE:
      case CTX_VIEW_BOX:
      case CTX_REL_QUAD_TO:
      case CTX_QUAD_TO:
      case CTX_TEXTURE:
        return 1;
      default:
        return 0;
    }
}

// expanding arc_to to arc can be the job
// of a layer in front of renderer?
//   doing:
//     rectangle
//     arc
//     ... etc reduction to beziers
//     or even do the reduction to
//     polylines directly here...
//     making the rasterizer able to
//     only do poly-lines? will that be faster?

/* the iterator - should decode bitpacked data as well -
 * making the rasterizers simpler, possibly do unpacking
 * all the way to absolute coordinates.. unless mixed
 * relative/not are wanted.
 */


CTX_STATIC void
ctx_iterator_init (CtxIterator      *iterator,
                   CtxRenderstream  *renderstream,
                   int               start_pos,
                   int               flags)
{
  iterator->renderstream   = renderstream;
  iterator->flags          = flags;
  iterator->bitpack_pos    = 0;
  iterator->bitpack_length = 0;
  iterator->pos            = start_pos;
  iterator->end_pos        = renderstream->count;
  iterator->in_history     = -1; // -1 is a marker used for first run
  ctx_memset (iterator->bitpack_command, 0, sizeof (iterator->bitpack_command) );
}

CTX_STATIC CtxEntry *_ctx_iterator_next (CtxIterator *iterator)
{
  int ret = iterator->pos;
  CtxEntry *entry = &iterator->renderstream->entries[ret];
  if (ret >= iterator->end_pos)
    { return NULL; }
  if (iterator->in_history == 0)
    { iterator->pos += (ctx_conts_for_entry (entry) + 1); }
  iterator->in_history = 0;
  if (iterator->pos >= iterator->end_pos)
    { return NULL; }
  return &iterator->renderstream->entries[iterator->pos];
}

// 6024x4008
#if CTX_BITPACK
CTX_STATIC void
ctx_iterator_expand_s8_args (CtxIterator *iterator, CtxEntry *entry)
{
  int no = 0;
  for (int cno = 0; cno < 4; cno++)
    for (int d = 0; d < 2; d++, no++)
      iterator->bitpack_command[cno].data.f[d] =
        entry->data.s8[no] * 1.0f / CTX_SUBDIV;
  iterator->bitpack_command[0].code =
    iterator->bitpack_command[1].code =
      iterator->bitpack_command[2].code =
        iterator->bitpack_command[3].code = CTX_CONT;
  iterator->bitpack_length = 4;
  iterator->bitpack_pos = 0;
}

CTX_STATIC void
ctx_iterator_expand_s16_args (CtxIterator *iterator, CtxEntry *entry)
{
  int no = 0;
  for (int cno = 0; cno < 2; cno++)
    for (int d = 0; d < 2; d++, no++)
      iterator->bitpack_command[cno].data.f[d] = entry->data.s16[no] * 1.0f /
          CTX_SUBDIV;
  iterator->bitpack_command[0].code =
    iterator->bitpack_command[1].code = CTX_CONT;
  iterator->bitpack_length = 2;
  iterator->bitpack_pos    = 0;
}
#endif

CtxCommand *
ctx_iterator_next (CtxIterator *iterator)
{
  CtxEntry *ret;
#if CTX_BITPACK
  int expand_bitpack = iterator->flags & CTX_ITERATOR_EXPAND_BITPACK;
again:
  if (iterator->bitpack_length)
    {
      ret = &iterator->bitpack_command[iterator->bitpack_pos];
      iterator->bitpack_pos += (ctx_conts_for_entry (ret) + 1);
      if (iterator->bitpack_pos >= iterator->bitpack_length)
        {
          iterator->bitpack_length = 0;
        }
      return (CtxCommand *) ret;
    }
#endif
  ret = _ctx_iterator_next (iterator);
#if CTX_BITPACK
  if (ret && expand_bitpack)
    switch ((CtxCode)(ret->code))
      {
        case CTX_REL_CURVE_TO_REL_LINE_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_CURVE_TO;
          iterator->bitpack_command[1].code =
          iterator->bitpack_command[2].code = CTX_CONT;
          iterator->bitpack_command[3].code = CTX_REL_LINE_TO;
          // 0.0 here is a common optimization - so check for it
          if (ret->data.s8[6]== 0 && ret->data.s8[7] == 0)
            { iterator->bitpack_length = 3; }
          else
            iterator->bitpack_length          = 4;
          goto again;
        case CTX_REL_LINE_TO_REL_CURVE_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
          iterator->bitpack_command[1].code = CTX_REL_CURVE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_REL_CURVE_TO_REL_MOVE_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_CURVE_TO;
          iterator->bitpack_command[3].code = CTX_REL_MOVE_TO;
          iterator->bitpack_length          = 4;
          goto again;
        case CTX_REL_LINE_TO_X4:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code =
          iterator->bitpack_command[1].code =
          iterator->bitpack_command[2].code =
          iterator->bitpack_command[3].code = CTX_REL_LINE_TO;
          iterator->bitpack_length          = 4;
          goto again;
        case CTX_REL_QUAD_TO_S16:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_QUAD_TO;
          iterator->bitpack_length          = 1;
          goto again;
        case CTX_REL_QUAD_TO_REL_QUAD_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code =
          iterator->bitpack_command[2].code = CTX_REL_QUAD_TO;
          iterator->bitpack_length          = 3;
          goto again;
        case CTX_REL_LINE_TO_X2:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code =
          iterator->bitpack_command[1].code = CTX_REL_LINE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_REL_LINE_TO_REL_MOVE_TO:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
          iterator->bitpack_command[1].code = CTX_REL_MOVE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_MOVE_TO_REL_LINE_TO:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_MOVE_TO;
          iterator->bitpack_command[1].code = CTX_REL_MOVE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_FILL_MOVE_TO:
          iterator->bitpack_command[1]      = *ret;
          iterator->bitpack_command[0].code = CTX_FILL;
          iterator->bitpack_command[1].code = CTX_MOVE_TO;
          iterator->bitpack_pos             = 0;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_LINEAR_GRADIENT:
        case CTX_QUAD_TO:
        case CTX_REL_QUAD_TO:
        case CTX_TEXTURE:
        case CTX_RECTANGLE:
        case CTX_VIEW_BOX:
        case CTX_ARC:
        case CTX_ARC_TO:
        case CTX_REL_ARC_TO:
        case CTX_COLOR:
        case CTX_SHADOW_COLOR:
        case CTX_RADIAL_GRADIENT:
        case CTX_CURVE_TO:
        case CTX_REL_CURVE_TO:
        case CTX_APPLY_TRANSFORM:
        case CTX_ROUND_RECTANGLE:
        case CTX_TEXT:
        case CTX_TEXT_STROKE:
        case CTX_FONT:
        case CTX_LINE_DASH:
        case CTX_FILL:
        case CTX_NOP:
        case CTX_MOVE_TO:
        case CTX_LINE_TO:
        case CTX_REL_MOVE_TO:
        case CTX_REL_LINE_TO:
        case CTX_VER_LINE_TO:
        case CTX_REL_VER_LINE_TO:
        case CTX_HOR_LINE_TO:
        case CTX_REL_HOR_LINE_TO:
        case CTX_ROTATE:
        case CTX_FLUSH:
        case CTX_TEXT_ALIGN:
        case CTX_TEXT_BASELINE:
        case CTX_TEXT_DIRECTION:
        case CTX_MITER_LIMIT:
        case CTX_GLOBAL_ALPHA:
        case CTX_COMPOSITING_MODE:
        case CTX_BLEND_MODE:
        case CTX_SHADOW_BLUR:
        case CTX_SHADOW_OFFSET_X:
        case CTX_SHADOW_OFFSET_Y:
        case CTX_RESET:
        case CTX_EXIT:
        case CTX_BEGIN_PATH:
        case CTX_CLOSE_PATH:
        case CTX_SAVE:
        case CTX_CLIP:
        case CTX_PRESERVE:
        case CTX_DEFINE_GLYPH:
        case CTX_IDENTITY:
        case CTX_FONT_SIZE:
        case CTX_START_GROUP:
        case CTX_END_GROUP:
        case CTX_RESTORE:
        case CTX_LINE_WIDTH:
        case CTX_STROKE:
        case CTX_KERNING_PAIR:
        case CTX_SCALE:
        case CTX_GLYPH:
        case CTX_SET_PIXEL:
        case CTX_FILL_RULE:
        case CTX_LINE_CAP:
        case CTX_LINE_JOIN:
        case CTX_NEW_PAGE:
        case CTX_SET_KEY:
        case CTX_TRANSLATE:
        case CTX_GRADIENT_STOP:
        case CTX_CONT: // shouldnt happen
          iterator->bitpack_length = 0;
          return (CtxCommand *) ret;
#if 1
        default: // XXX remove - and get better warnings
          iterator->bitpack_command[0] = ret[0];
          iterator->bitpack_command[1] = ret[1];
          iterator->bitpack_command[2] = ret[2];
          iterator->bitpack_command[3] = ret[3];
          iterator->bitpack_command[4] = ret[4];
          iterator->bitpack_pos = 0;
          iterator->bitpack_length = 1;
          goto again;
#endif
      }
#endif
  return (CtxCommand *) ret;
}

CTX_STATIC void ctx_renderstream_compact (CtxRenderstream *renderstream);
CTX_STATIC void
ctx_renderstream_resize (CtxRenderstream *renderstream, int desired_size)
{
#if CTX_RENDERSTREAM_STATIC
  if (renderstream->flags & CTX_RENDERSTREAM_EDGE_LIST)
    {
      static CtxEntry sbuf[CTX_MAX_EDGE_LIST_SIZE];
      renderstream->entries = &sbuf[0];
      renderstream->size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (renderstream->flags & CTX_RENDERSTREAM_CURRENT_PATH)
    {
      static CtxEntry sbuf[CTX_MAX_EDGE_LIST_SIZE];
      renderstream->entries = &sbuf[0];
      renderstream->size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else
    {
      static CtxEntry sbuf[CTX_MAX_JOURNAL_SIZE];
      renderstream->entries = &sbuf[0];
      renderstream->size = CTX_MAX_JOURNAL_SIZE;
      ctx_renderstream_compact (renderstream);
    }
#else
  int new_size = desired_size;
  int min_size = CTX_MIN_JOURNAL_SIZE;
  int max_size = CTX_MAX_JOURNAL_SIZE;
  if ((renderstream->flags & CTX_RENDERSTREAM_EDGE_LIST))
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (renderstream->flags & CTX_RENDERSTREAM_CURRENT_PATH)
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else
    {
      ctx_renderstream_compact (renderstream);
    }

  if (new_size < renderstream->size)
    { return; }
  if (renderstream->size == max_size)
    { return; }
  if (new_size < min_size)
    { new_size = min_size; }
  if (new_size < renderstream->count)
    { new_size = renderstream->count + 4; }
  if (new_size >= max_size)
    { new_size = max_size; }
  if (new_size != renderstream->size)
    {
      //fprintf (stderr, "growing renderstream %p %i to %d from %d\n", renderstream, renderstream->flags, new_size, renderstream->size);
  if (renderstream->entries)
    {
      //printf ("grow %p to %d from %d\n", renderstream, new_size, renderstream->size);
      CtxEntry *ne =  (CtxEntry *) malloc (sizeof (CtxEntry) * new_size);
      memcpy (ne, renderstream->entries, renderstream->size * sizeof (CtxEntry) );
      free (renderstream->entries);
      renderstream->entries = ne;
      //renderstream->entries = (CtxEntry*)malloc (renderstream->entries, sizeof (CtxEntry) * new_size);
    }
  else
    {
      //printf ("allocating for %p %d\n", renderstream, new_size);
      renderstream->entries = (CtxEntry *) malloc (sizeof (CtxEntry) * new_size);
    }
  renderstream->size = new_size;
    }
  //fprintf (stderr, "renderstream %p is %d\n", renderstream, renderstream->size);
#endif
}

CTX_STATIC int
ctx_renderstream_add_single (CtxRenderstream *renderstream, CtxEntry *entry)
{
  int max_size = CTX_MAX_JOURNAL_SIZE;
  int ret = renderstream->count;
  if (renderstream->flags & CTX_RENDERSTREAM_EDGE_LIST)
    {
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (renderstream->flags & CTX_RENDERSTREAM_CURRENT_PATH)
    {
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  if (renderstream->flags & CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES)
    {
      return ret;
    }
  if (ret + 8 >= renderstream->size - 20)
    {
      int new = CTX_MAX (renderstream->size * 2, ret + 8);
      ctx_renderstream_resize (renderstream, new);
    }

  if (renderstream->count >= max_size - 20)
    {
      return 0;
    }
  renderstream->entries[renderstream->count] = *entry;
  ret = renderstream->count;
  renderstream->count++;
  return ret;
}




int
ctx_add_single (Ctx *ctx, void *entry)
{
  return ctx_renderstream_add_single (&ctx->renderstream, (CtxEntry *) entry);
}

int
ctx_renderstream_add_entry (CtxRenderstream *renderstream, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int ret = 0;
  for (int i = 0; i < length; i ++)
    {
      ret = ctx_renderstream_add_single (renderstream, &entry[i]);
    }
  return ret;
}

int ctx_append_renderstream (Ctx *ctx, void *data, int length)
{
  CtxEntry *entries = (CtxEntry *) data;
  if (length % sizeof (CtxEntry) )
    {
      ctx_log("drawlist not multiple of 9\n");
      return -1;
    }
  for (unsigned int i = 0; i < length / sizeof (CtxEntry); i++)
    {
      ctx_renderstream_add_single (&ctx->renderstream, &entries[i]);
    }
  return 0;
}

int ctx_set_renderstream (Ctx *ctx, void *data, int length)
{
  CtxRenderstream *renderstream = &ctx->renderstream;
  ctx->renderstream.count = 0;
  if (renderstream->flags & CTX_RENDERSTREAM_DOESNT_OWN_ENTRIES)
    {
      return -1;
    }
  if (length % 9) return -1;
  ctx_renderstream_resize (renderstream, length/9);
  memcpy (renderstream->entries, data, length);
  renderstream->count = length / 9;
  return length;
}


int ctx_get_renderstream_count (Ctx *ctx)
{
  return ctx->renderstream.count;
}

const CtxEntry *ctx_get_renderstream (Ctx *ctx)
{
  return ctx->renderstream.entries;
}

int
ctx_add_data (Ctx *ctx, void *data, int length)
{
  if (length % sizeof (CtxEntry) )
    {
      //ctx_log("err\n");
      return -1;
    }
  /* some more input verification might be in order.. like
   * verify that it is well-formed up to length?
   *
   * also - it would be very useful to stop processing
   * upon flush - and do renderstream resizing.
   */
  return ctx_renderstream_add_entry (&ctx->renderstream, (CtxEntry *) data);
}

int ctx_renderstream_add_u32 (CtxRenderstream *renderstream, CtxCode code, uint32_t u32[2])
{
  CtxEntry entry = {code, {{0},}};
  entry.data.u32[0] = u32[0];
  entry.data.u32[1] = u32[1];
  return ctx_renderstream_add_single (renderstream, &entry);
}

int ctx_renderstream_add_data (CtxRenderstream *renderstream, const void *data, int length)
{
  CtxEntry entry = {CTX_DATA, {{0},}};
  entry.data.u32[0] = 0;
  entry.data.u32[1] = 0;
  int ret = ctx_renderstream_add_single (renderstream, &entry);
  if (!data) { return -1; }
  int length_in_blocks;
  if (length <= 0) { length = strlen ( (char *) data) + 1; }
  length_in_blocks = length / sizeof (CtxEntry);
  length_in_blocks += (length % sizeof (CtxEntry) ) ?1:0;
  if (renderstream->count + length_in_blocks + 4 > renderstream->size)
    { ctx_renderstream_resize (renderstream, renderstream->count * 1.2 + length_in_blocks + 32); }
  if (renderstream->count >= renderstream->size)
    { return -1; }
  renderstream->count += length_in_blocks;
  renderstream->entries[ret].data.u32[0] = length;
  renderstream->entries[ret].data.u32[1] = length_in_blocks;
  memcpy (&renderstream->entries[ret+1], data, length);
  {
    //int reverse = ctx_renderstream_add (renderstream, CTX_DATA_REV);
    CtxEntry entry = {CTX_DATA_REV, {{0},}};
    entry.data.u32[0] = length;
    entry.data.u32[1] = length_in_blocks;
    ctx_renderstream_add_single (renderstream, &entry);
    /* this reverse marker exist to enable more efficient
       front to back traversal, can be ignored in other
       direction, is this needed after string setters as well?
     */
  }
  return ret;
}

CTX_STATIC CtxEntry
ctx_void (CtxCode code)
{
  CtxEntry command;
  command.code = code;
  command.data.u32[0] = 0;
  command.data.u32[1] = 0;
  return command;
}

CTX_STATIC CtxEntry
ctx_f (CtxCode code, float x, float y)
{
  CtxEntry command = ctx_void (code);
  command.data.f[0] = x;
  command.data.f[1] = y;
  return command;
}

CTX_STATIC CtxEntry
ctx_u32 (CtxCode code, uint32_t x, uint32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.u32[0] = x;
  command.data.u32[1] = y;
  return command;
}

#if 0
CTX_STATIC CtxEntry
ctx_s32 (CtxCode code, int32_t x, int32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.s32[0] = x;
  command.data.s32[1] = y;
  return command;
}
#endif

CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1)
{
  CtxEntry command = ctx_void (code);
  command.data.s16[0] = x0;
  command.data.s16[1] = y0;
  command.data.s16[2] = x1;
  command.data.s16[3] = y1;
  return command;
}

CTX_STATIC CtxEntry
ctx_u8 (CtxCode code,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d,
        uint8_t e, uint8_t f, uint8_t g, uint8_t h)
{
  CtxEntry command = ctx_void (code);
  command.data.u8[0] = a;
  command.data.u8[1] = b;
  command.data.u8[2] = c;
  command.data.u8[3] = d;
  command.data.u8[4] = e;
  command.data.u8[5] = f;
  command.data.u8[6] = g;
  command.data.u8[7] = h;
  return command;
}

#define CTX_PROCESS_VOID(cmd) do {\
  CtxEntry command = ctx_void (cmd); \
  ctx_process (ctx, &command);}while(0) \

#define CTX_PROCESS_F(cmd, x, y) do {\
  CtxEntry command = ctx_f(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_F1(cmd, x) do {\
  CtxEntry command = ctx_f(cmd, x, 0);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U32(cmd, x, y) do {\
  CtxEntry command = ctx_u32(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U8(cmd, x) do {\
  CtxEntry command = ctx_u8(cmd, x,0,0,0,0,0,0,0);\
  ctx_process (ctx, &command);}while(0)


CTX_STATIC void
ctx_process_cmd_str_with_len (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1, int len)
{
  CtxEntry commands[1 + 2 + len/8];
  ctx_memset (commands, 0, sizeof (commands) );
  commands[0] = ctx_u32 (code, arg0, arg1);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = len;
  commands[1].data.u32[1] = len/9+1;
  memcpy( (char *) &commands[2].data.u8[0], string, len);
  ( (char *) (&commands[2].data.u8[0]) ) [len]=0;
  ctx_process (ctx, commands);
}

CTX_STATIC void
ctx_process_cmd_str (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1)
{
  ctx_process_cmd_str_with_len (ctx, code, string, arg0, arg1, strlen (string));
}


#if CTX_BITPACK_PACKER
CTX_STATIC int
ctx_last_history (CtxRenderstream *renderstream)
{
  int last_history = 0;
  int i = 0;
  while (i < renderstream->count)
    {
      CtxEntry *entry = &renderstream->entries[i];
      i += (ctx_conts_for_entry (entry) + 1);
    }
  return last_history;
}
#endif

#if CTX_BITPACK_PACKER

CTX_STATIC float
find_max_dev (CtxEntry *entry, int nentrys)
{
  float max_dev = 0.0;
  for (int c = 0; c < nentrys; c++)
    {
      for (int d = 0; d < 2; d++)
        {
          if (entry[c].data.f[d] > max_dev)
            { max_dev = entry[c].data.f[d]; }
          if (entry[c].data.f[d] < -max_dev)
            { max_dev = -entry[c].data.f[d]; }
        }
    }
  return max_dev;
}

CTX_STATIC void
pack_s8_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      { entry[0].data.s8[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV; }
}

CTX_STATIC void
pack_s16_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      { entry[0].data.s16[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV; }
}
#endif

#if CTX_BITPACK_PACKER
CTX_STATIC void
ctx_renderstream_remove_tiny_curves (CtxRenderstream *renderstream, int start_pos)
{
  CtxIterator iterator;
  if ( (renderstream->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_iterator_init (&iterator, renderstream, start_pos, CTX_ITERATOR_FLAT);
  iterator.end_pos = renderstream->count - 5;
  CtxCommand *command = NULL;
  while ( (command = ctx_iterator_next (&iterator) ) )
    {
      CtxEntry *entry = &command->entry;
      /* things smaller than this have probably been scaled down
         beyond recognition, bailing for both better packing and less rasterization work
       */
      if (command[0].code == CTX_REL_CURVE_TO)
        {
          float max_dev = find_max_dev (entry, 3);
          if (max_dev < 1.0)
            {
              entry[0].code = CTX_REL_LINE_TO;
              entry[0].data.f[0] = entry[2].data.f[0];
              entry[0].data.f[1] = entry[2].data.f[1];
              entry[1].code = CTX_NOP;
              entry[2].code = CTX_NOP;
            }
        }
    }
}
#endif

#if CTX_BITPACK_PACKER
CTX_STATIC void
ctx_renderstream_bitpack (CtxRenderstream *renderstream, int start_pos)
{
#if CTX_BITPACK
  int i = 0;
  if ( (renderstream->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_renderstream_remove_tiny_curves (renderstream, renderstream->bitpack_pos);
  i = renderstream->bitpack_pos;
  if (start_pos > i)
    { i = start_pos; }
  while (i < renderstream->count - 4) /* the -4 is to avoid looking past
                                    initialized data we're not ready
                                    to bitpack yet*/
    {
      CtxEntry *entry = &renderstream->entries[i];
      if (entry[0].code == CTX_SET_RGBA_U8 &&
          entry[1].code == CTX_MOVE_TO &&
          entry[2].code == CTX_REL_LINE_TO &&
          entry[3].code == CTX_REL_LINE_TO &&
          entry[4].code == CTX_REL_LINE_TO &&
          entry[5].code == CTX_REL_LINE_TO &&
          entry[6].code == CTX_FILL &&
          ctx_fabsf (entry[2].data.f[0] - 1.0f) < 0.02f &&
          ctx_fabsf (entry[3].data.f[1] - 1.0f) < 0.02f)
        {
          entry[0].code = CTX_SET_PIXEL;
          entry[0].data.u16[2] = entry[1].data.f[0];
          entry[0].data.u16[3] = entry[1].data.f[1];
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
          entry[4].code = CTX_NOP;
          entry[5].code = CTX_NOP;
          entry[6].code = CTX_NOP;
        }
#if 1
      else if (entry[0].code == CTX_REL_LINE_TO)
        {
          if (entry[1].code == CTX_REL_LINE_TO &&
              entry[2].code == CTX_REL_LINE_TO &&
              entry[3].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_LINE_TO_X4;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_CURVE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_LINE_TO_REL_CURVE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_LINE_TO &&
                   entry[2].code == CTX_REL_LINE_TO &&
                   entry[3].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_LINE_TO_X4;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_MOVE_TO)
            {
              float max_dev = find_max_dev (entry, 2);
              if (max_dev < 31000 / CTX_SUBDIV)
                {
                  pack_s16_args (entry, 2);
                  entry[0].code = CTX_REL_LINE_TO_REL_MOVE_TO;
                  entry[1].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 2);
              if (max_dev < 31000 / CTX_SUBDIV)
                {
                  pack_s16_args (entry, 2);
                  entry[0].code = CTX_REL_LINE_TO_X2;
                  entry[1].code = CTX_NOP;
                }
            }
        }
#endif
#if 1
      else if (entry[0].code == CTX_REL_CURVE_TO)
        {
          if (entry[3].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[3].code == CTX_REL_MOVE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_CURVE_TO_REL_MOVE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else
            {
              float max_dev = find_max_dev (entry, 3);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 3);
                  ctx_arg_s8 (6) =
                    ctx_arg_s8 (7) = 0;
                  entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                }
            }
        }
#endif
#if 1
      else if (entry[0].code == CTX_REL_QUAD_TO)
        {
          if (entry[2].code == CTX_REL_QUAD_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_QUAD_TO_REL_QUAD_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else
            {
              float max_dev = find_max_dev (entry, 2);
              if (max_dev < 3100 / CTX_SUBDIV)
                {
                  pack_s16_args (entry, 2);
                  entry[0].code = CTX_REL_QUAD_TO_S16;
                  entry[1].code = CTX_NOP;
                }
            }
        }
#endif
#if 1
      else if (entry[0].code == CTX_FILL &&
               entry[1].code == CTX_MOVE_TO)
        {
          entry[0] = entry[1];
          entry[0].code = CTX_FILL_MOVE_TO;
          entry[1].code = CTX_NOP;
        }
#endif
#if 1
      else if (entry[0].code == CTX_MOVE_TO &&
               entry[1].code == CTX_MOVE_TO &&
               entry[2].code == CTX_MOVE_TO)
        {
          entry[0]      = entry[2];
          entry[0].code = CTX_MOVE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
        }
#endif
#if 1
      else if ( (entry[0].code == CTX_MOVE_TO &&
                 entry[1].code == CTX_MOVE_TO) ||
                (entry[0].code == CTX_REL_MOVE_TO &&
                 entry[1].code == CTX_MOVE_TO) )
        {
          entry[0]      = entry[1];
          entry[0].code = CTX_MOVE_TO;
          entry[1].code = CTX_NOP;
        }
#endif
      i += (ctx_conts_for_entry (entry) + 1);
    }
  int source = renderstream->bitpack_pos;
  int target = renderstream->bitpack_pos;
  int removed = 0;
  /* remove nops that have been inserted as part of shortenings
   */
  while (source < renderstream->count)
    {
      CtxEntry *sentry = &renderstream->entries[source];
      CtxEntry *tentry = &renderstream->entries[target];
      while (sentry->code == CTX_NOP && source < renderstream->count)
        {
          source++;
          sentry = &renderstream->entries[source];
          removed++;
        }
      if (sentry != tentry)
        { *tentry = *sentry; }
      source ++;
      target ++;
    }
  renderstream->count -= removed;
  renderstream->bitpack_pos = renderstream->count;
#endif
}

#endif

CTX_STATIC void
ctx_renderstream_compact (CtxRenderstream *renderstream)
{
#if CTX_BITPACK_PACKER
  int last_history;
  last_history = ctx_last_history (renderstream);
#else
  if (renderstream) {};
#endif
#if CTX_BITPACK_PACKER
  ctx_renderstream_bitpack (renderstream, last_history);
#endif
}


