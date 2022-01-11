#include "ctx-split.h"


static inline int
ctx_conts_for_entry (CtxEntry *entry)
{
    switch (entry->code)
    {
      case CTX_DATA:
        return entry->data.u32[1];
      case CTX_LINEAR_GRADIENT:
      //case CTX_DEFINE_TEXTURE:
        return 1;
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_APPLY_TRANSFORM:
      case CTX_SOURCE_TRANSFORM:
      case CTX_COLOR:
      case CTX_ROUND_RECTANGLE:
      case CTX_SHADOW_COLOR:
        return 2;
      case CTX_FILL_RECT:
      case CTX_STROKE_RECT:
      case CTX_RECTANGLE:
      case CTX_VIEW_BOX:
      case CTX_REL_QUAD_TO:
      case CTX_QUAD_TO:
        return 1;

      case CTX_TEXT:
      case CTX_LINE_DASH:
      case CTX_COLOR_SPACE:
      case CTX_STROKE_TEXT:
      case CTX_FONT:
      case CTX_TEXTURE:
        {
          int eid_len = entry[1].data.u32[1];
          return eid_len + 1;
        }
      case CTX_DEFINE_TEXTURE:
        {
          int eid_len = entry[2].data.u32[1];
          int pix_len = entry[2 + eid_len + 1].data.u32[1];
          return eid_len + pix_len + 2 + 1;
        }
      default:
        return 0;
    }
}

// expanding arc_to to arc can be the job
// of a layer in front of backend?
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
                   CtxDrawlist  *drawlist,
                   int               start_pos,
                   int               flags)
{
  iterator->drawlist   = drawlist;
  iterator->flags          = flags;
  iterator->bitpack_pos    = 0;
  iterator->bitpack_length = 0;
  iterator->pos            = start_pos;
  iterator->end_pos        = drawlist->count;
  iterator->first_run      = 1; // -1 is a marker used for first run
  ctx_memset (iterator->bitpack_command, 0, sizeof (iterator->bitpack_command) );
}

int ctx_iterator_pos (CtxIterator *iterator)
{
  return iterator->pos;
}

static inline CtxEntry *_ctx_iterator_next (CtxIterator *iterator)
{
  int ret = iterator->pos;
  CtxEntry *entry = &iterator->drawlist->entries[ret];
  if (CTX_UNLIKELY(ret >= iterator->end_pos))
    { return NULL; }

  if (CTX_UNLIKELY(iterator->first_run))
      iterator->first_run = 0;
  else
     iterator->pos += (ctx_conts_for_entry (entry) + 1);

  if (CTX_UNLIKELY(iterator->pos >= iterator->end_pos))
    { return NULL; }
  return &iterator->drawlist->entries[iterator->pos];
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
  if (CTX_UNLIKELY(iterator->bitpack_length))
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
  if (CTX_UNLIKELY(ret && expand_bitpack))
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
        case CTX_SOURCE_TRANSFORM:
        case CTX_ROUND_RECTANGLE:
        case CTX_TEXT:
        case CTX_STROKE_TEXT:
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
        case CTX_LINE_DASH_OFFSET:
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
        case CTX_DEFINE_TEXTURE:
        case CTX_GRADIENT_STOP:
        case CTX_DATA: // XXX : would be better if we hide the DATAs
        case CTX_CONT: // shouldnt happen
        default:
          iterator->bitpack_length = 0;
          return (CtxCommand *) ret;
#if 0
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

CTX_STATIC void ctx_drawlist_compact (CtxDrawlist *drawlist);
CTX_STATIC void
ctx_drawlist_resize (CtxDrawlist *drawlist, int desired_size)
{
  int flags=drawlist->flags;
#if CTX_DRAWLIST_STATIC
  if (flags & CTX_DRAWLIST_EDGE_LIST)
    {
      static CtxSegment sbuf[CTX_MAX_EDGE_LIST_SIZE];
      drawlist->entries = (CtxEntry*)&sbuf[0];
      drawlist->size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (flags & CTX_DRAWLIST_CURRENT_PATH)
    {
      static CtxEntry sbuf[CTX_MAX_EDGE_LIST_SIZE];
      drawlist->entries = &sbuf[0];
      drawlist->size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else
    {
      static CtxEntry sbuf[CTX_MAX_JOURNAL_SIZE];
      drawlist->entries = &sbuf[0];
      drawlist->size = CTX_MAX_JOURNAL_SIZE;
      if(0)ctx_drawlist_compact (drawlist);
    }
#else
  int new_size = desired_size;
  int min_size = CTX_MIN_JOURNAL_SIZE;
  int max_size = CTX_MAX_JOURNAL_SIZE;
  if ((flags & CTX_DRAWLIST_EDGE_LIST))
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (flags & CTX_DRAWLIST_CURRENT_PATH)
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else
    {
#if 0
      ctx_drawlist_compact (drawlist);
#endif
    }

  if (CTX_UNLIKELY(new_size < drawlist->size))
    { return; }
  if (CTX_UNLIKELY(drawlist->size == max_size))
    { return; }
  new_size = ctx_maxi (new_size, min_size);
  //if (new_size < drawlist->count)
  //  { new_size = drawlist->count + 4; }
  new_size = ctx_mini (new_size, max_size);
  if (new_size != drawlist->size)
    {
      int item_size = sizeof (CtxEntry);
      if (flags & CTX_DRAWLIST_EDGE_LIST) item_size = sizeof (CtxSegment);
      //fprintf (stderr, "growing drawlist %p %i to %d from %d\n", drawlist, flags, new_size, drawlist->size);
  if (drawlist->entries)
    {
      //printf ("grow %p to %d from %d\n", drawlist, new_size, drawlist->size);
      CtxEntry *ne =  (CtxEntry *) malloc (item_size * new_size);
      memcpy (ne, drawlist->entries, drawlist->size * item_size );
      free (drawlist->entries);
      drawlist->entries = ne;
      //drawlist->entries = (CtxEntry*)malloc (drawlist->entries, item_size * new_size);
    }
  else
    {
      //fprintf (stderr, "allocating for %p %d\n", drawlist, new_size);
      drawlist->entries = (CtxEntry *) malloc (item_size * new_size);
    }
  drawlist->size = new_size;
    }
  //fprintf (stderr, "drawlist %p is %d\n", drawlist, drawlist->size);
#endif
}

CTX_STATIC void
ctx_edgelist_resize (CtxDrawlist *drawlist, int desired_size)
{
#if CTX_DRAWLIST_STATIC
    {
      static CtxSegment sbuf[CTX_MAX_EDGE_LIST_SIZE];
      drawlist->entries = (CtxEntry*)&sbuf[0];
      drawlist->size = CTX_MAX_EDGE_LIST_SIZE;
    }
#else
  int new_size = desired_size;
  int min_size = CTX_MIN_JOURNAL_SIZE;
  int max_size = CTX_MAX_JOURNAL_SIZE;
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }

  if (CTX_UNLIKELY(drawlist->size == max_size))
    { return; }
  new_size = ctx_maxi (new_size, min_size);
  //if (new_size < drawlist->count)
  //  { new_size = drawlist->count + 4; }
  new_size = ctx_mini (new_size, max_size);
  if (new_size != drawlist->size)
    {
      int item_size = item_size = sizeof (CtxSegment);
      //fprintf (stderr, "growing drawlist %p %i to %d from %d\n", drawlist, flags, new_size, drawlist->size);
  if (drawlist->entries)
    {
      //printf ("grow %p to %d from %d\n", drawlist, new_size, drawlist->size);
      CtxEntry *ne =  (CtxEntry *) malloc (item_size * new_size);
      memcpy (ne, drawlist->entries, drawlist->size * item_size );
      free (drawlist->entries);
      drawlist->entries = ne;
      //drawlist->entries = (CtxEntry*)malloc (drawlist->entries, item_size * new_size);
    }
  else
    {
      //fprintf (stderr, "allocating for %p %d\n", drawlist, new_size);
      drawlist->entries = (CtxEntry *) malloc (item_size * new_size);
    }
  drawlist->size = new_size;
    }
  //fprintf (stderr, "drawlist %p is %d\n", drawlist, drawlist->size);
#endif
}


CTX_STATIC inline int
ctx_drawlist_add_single (CtxDrawlist *drawlist, CtxEntry *entry)
{
  unsigned int max_size = CTX_MAX_JOURNAL_SIZE;
  int ret = drawlist->count;
  int flags = drawlist->flags;
  if (CTX_LIKELY((flags & CTX_DRAWLIST_EDGE_LIST ||
       flags & CTX_DRAWLIST_CURRENT_PATH)))
    {
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  if (CTX_UNLIKELY(flags & CTX_DRAWLIST_DOESNT_OWN_ENTRIES))
    {
      return ret;
    }
  if (CTX_UNLIKELY(ret + 64 >= drawlist->size - 40))
    {
      int new_ = CTX_MAX (drawlist->size * 2, ret + 1024);
      ctx_drawlist_resize (drawlist, new_);
    }

  if (CTX_UNLIKELY(drawlist->count >= max_size - 20))
    {
      return 0;
    }
  if ((flags & CTX_DRAWLIST_EDGE_LIST))
    ((CtxSegment*)(drawlist->entries))[drawlist->count] = *(CtxSegment*)entry;
  else
    drawlist->entries[drawlist->count] = *entry;
  ret = drawlist->count;
  drawlist->count++;
  return ret;
}

static inline int
ctx_edgelist_add_single (CtxDrawlist *drawlist, CtxEntry *entry)
{
  int ret = drawlist->count;

  if (CTX_UNLIKELY(ret >= CTX_MAX_EDGE_LIST_SIZE- 20))
    {
      return 0;
    }
  if (CTX_UNLIKELY(ret + 2 >= drawlist->size))
    {
      int new_ = ctx_maxi (drawlist->size * 2, ret + 1024);
      new_ = ctx_mini (CTX_MAX_EDGE_LIST_SIZE, new_);
      ctx_edgelist_resize (drawlist, new_);
    }

  ((CtxSegment*)(drawlist->entries))[ret] = *(CtxSegment*)entry;
  drawlist->count++;
  return ret;
}

int
ctx_add_single (Ctx *ctx, void *entry)
{
  return ctx_drawlist_add_single (&ctx->drawlist, (CtxEntry *) entry);
}

CTX_STATIC inline int
ctx_drawlist_add_entry (CtxDrawlist *drawlist, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int ret = 0;
  for (int i = 0; i < length; i ++)
    {
      ret = ctx_drawlist_add_single (drawlist, &entry[i]);
    }
  return ret;
}

#if 0
int
ctx_drawlist_insert_entry (CtxDrawlist *drawlist, int pos, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int tmp_pos = ctx_drawlist_add_entry (drawlist, entry);
  for (int i = 0; i < length; i++)
  {
    for (int j = pos + i + 1; j < tmp_pos; j++)
      drawlist->entries[j] = entry[j-1];
    drawlist->entries[pos + i] = entry[i];
  }
  return pos;
}
#endif
int
ctx_drawlist_insert_entry (CtxDrawlist *drawlist, int pos, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int tmp_pos = ctx_drawlist_add_entry (drawlist, entry);
#if 1
  for (int i = 0; i < length; i++)
  {
    for (int j = tmp_pos; j > pos + i; j--)
      drawlist->entries[j] = drawlist->entries[j-1];
    drawlist->entries[pos + i] = entry[i];
  }
  return pos;
#endif
  return tmp_pos;
}

int ctx_append_drawlist (Ctx *ctx, void *data, int length)
{
  CtxEntry *entries = (CtxEntry *) data;
  if (length % sizeof (CtxEntry) )
    {
      ctx_log("drawlist not multiple of 9\n");
      return -1;
    }
  for (unsigned int i = 0; i < length / sizeof (CtxEntry); i++)
    {
      ctx_drawlist_add_single (&ctx->drawlist, &entries[i]);
    }
  return 0;
}

int ctx_set_drawlist (Ctx *ctx, void *data, int length)
{
  CtxDrawlist *drawlist = &ctx->drawlist;
  if (drawlist->flags & CTX_DRAWLIST_DOESNT_OWN_ENTRIES)
    {
      return -1;
    }
  ctx->drawlist.count = 0;
  if (!data || length == 0)
    return 0;
  if (CTX_UNLIKELY(length % 9)) return -1;
  ctx_drawlist_resize (drawlist, length/9);
  memcpy (drawlist->entries, data, length);
  drawlist->count = length / 9;
  return length;
}

int ctx_get_drawlist_count (Ctx *ctx)
{
  return ctx->drawlist.count;
}

const CtxEntry *ctx_get_drawlist (Ctx *ctx)
{
  return ctx->drawlist.entries;
}

int
ctx_add_data (Ctx *ctx, void *data, int length)
{
  if (CTX_UNLIKELY(length % sizeof (CtxEntry) ))
    {
      //ctx_log("err\n");
      return -1;
    }
  /* some more input verification might be in order.. like
   * verify that it is well-formed up to length?
   *
   * also - it would be very useful to stop processing
   * upon flush - and do drawlist resizing.
   */
  return ctx_drawlist_add_entry (&ctx->drawlist, (CtxEntry *) data);
}

int ctx_drawlist_add_u32 (CtxDrawlist *drawlist, CtxCode code, uint32_t u32[2])
{
  CtxEntry entry[3] = {{code, {{0},}},};
  entry[0].data.u32[0] = u32[0];
  entry[0].data.u32[1] = u32[1];
  return ctx_drawlist_add_single (drawlist, &entry[0]);
}

int ctx_drawlist_add_data (CtxDrawlist *drawlist, const void *data, int length)
{
  CtxEntry entry[3] = {{CTX_DATA, {{0},}}};
  entry[0].data.u32[0] = 0;
  entry[0].data.u32[1] = 0;
  int ret = ctx_drawlist_add_single (drawlist, &entry[0]);
  if (CTX_UNLIKELY(!data)) { return -1; }
  int length_in_blocks;
  if (length <= 0) { length = strlen ( (char *) data) + 1; }
  length_in_blocks = length / sizeof (CtxEntry);
  length_in_blocks += (length % sizeof (CtxEntry) ) ?1:0;
  if ((signed)drawlist->count + length_in_blocks + 4 > drawlist->size)
    { ctx_drawlist_resize (drawlist, drawlist->count * 1.2 + length_in_blocks + 32); }
  if (CTX_UNLIKELY((signed)drawlist->count >= drawlist->size))
    { return -1; }
  drawlist->count += length_in_blocks;
  drawlist->entries[ret].data.u32[0] = length;
  drawlist->entries[ret].data.u32[1] = length_in_blocks;
  memcpy (&drawlist->entries[ret+1], data, length);
  {
    //int reverse = ctx_drawlist_add (drawlist, CTX_DATA_REV);
    CtxEntry entry[3] = {{CTX_DATA_REV, {{0},}}};
    entry[0].data.u32[0] = length;
    entry[0].data.u32[1] = length_in_blocks;
    ctx_drawlist_add_single (drawlist, &entry[0]);

    /* this reverse marker exist to enable more efficient
       front to back traversal, can be ignored in other
       direction, is this needed after string setters as well?
     */
  }
  return ret;
}

CTX_STATIC inline CtxEntry
ctx_void (CtxCode code)
{
  CtxEntry command;
  command.code = code;
  return command;
}

CTX_STATIC inline CtxEntry
ctx_f (CtxCode code, float x, float y)
{
  CtxEntry command;
  command.code = code;
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

static inline CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1)
{
  CtxEntry command;
  command.code = code;
  command.data.s16[0] = x0;
  command.data.s16[1] = y0;
  command.data.s16[2] = x1;
  command.data.s16[3] = y1;
  return command;
}

static inline CtxSegment
ctx_segment_s16 (CtxCode code, int x0, int y0, int x1, int y1)
{
  CtxSegment command;
  command.code = code;
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
  CtxEntry command;
  command.code = code;
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

CTX_STATIC void
ctx_process_cmd_str_with_len (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1, int len)
{
  CtxEntry commands[1 + 2 + (len+1+1)/9];
  ctx_memset (commands, 0, sizeof (commands) );
  commands[0] = ctx_u32 (code, arg0, arg1);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = len;
  commands[1].data.u32[1] = (len+1+1)/9 + 1;
  memcpy( (char *) &commands[2].data.u8[0], string, len);
  ( (char *) (&commands[2].data.u8[0]) ) [len]=0;
  ctx_process (ctx, commands);
}

CTX_STATIC void
ctx_process_cmd_str (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1)
{
  ctx_process_cmd_str_with_len (ctx, code, string, arg0, arg1, strlen (string));
}

CTX_STATIC void
ctx_process_cmd_str_float (Ctx *ctx, CtxCode code, const char *string, float arg0, float arg1)
{
  uint32_t iarg0;
  uint32_t iarg1;
  memcpy (&iarg0, &arg0, sizeof (iarg0));
  memcpy (&iarg1, &arg1, sizeof (iarg1));
  ctx_process_cmd_str_with_len (ctx, code, string, iarg0, iarg1, strlen (string));
}

#if CTX_BITPACK_PACKER
CTX_STATIC unsigned int
ctx_last_history (CtxDrawlist *drawlist)
{
  unsigned int last_history = 0;
  unsigned int i = 0;
  while (i < drawlist->count)
    {
      CtxEntry *entry = &drawlist->entries[i];
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
ctx_drawlist_remove_tiny_curves (CtxDrawlist *drawlist, int start_pos)
{
  CtxIterator iterator;
  if ( (drawlist->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_iterator_init (&iterator, drawlist, start_pos, CTX_ITERATOR_FLAT);
  iterator.end_pos = drawlist->count - 5;
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
ctx_drawlist_bitpack (CtxDrawlist *drawlist, unsigned int start_pos)
{
#if CTX_BITPACK
  unsigned int i = 0;
  if ( (drawlist->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_drawlist_remove_tiny_curves (drawlist, drawlist->bitpack_pos);
  i = drawlist->bitpack_pos;
  if (start_pos > i)
    { i = start_pos; }
  while (i < drawlist->count - 4) /* the -4 is to avoid looking past
                                    initialized data we're not ready
                                    to bitpack yet*/
    {
      CtxEntry *entry = &drawlist->entries[i];
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

  unsigned int source = drawlist->bitpack_pos;
  unsigned int target = drawlist->bitpack_pos;
  int removed = 0;
  /* remove nops that have been inserted as part of shortenings
   */
  while (source < drawlist->count)
    {
      CtxEntry *sentry = &drawlist->entries[source];
      CtxEntry *tentry = &drawlist->entries[target];
      while (sentry->code == CTX_NOP && source < drawlist->count)
        {
          source++;
          sentry = &drawlist->entries[source];
          removed++;
        }
      if (sentry != tentry)
        { *tentry = *sentry; }
      source ++;
      target ++;
    }
  drawlist->count -= removed;
  drawlist->bitpack_pos = drawlist->count;
#endif
}

#endif

CTX_STATIC inline void
ctx_drawlist_compact (CtxDrawlist *drawlist)
{
#if CTX_BITPACK_PACKER
  unsigned int last_history;
  last_history = ctx_last_history (drawlist);
#else
  if (drawlist) {};
#endif
#if CTX_BITPACK_PACKER
  ctx_drawlist_bitpack (drawlist, last_history);
#endif
}

uint8_t *ctx_define_texture_pixel_data (CtxEntry *entry)
{
  return &entry[2 + 1 + 1 + ctx_conts_for_entry (&entry[2])].data.u8[0];
}

