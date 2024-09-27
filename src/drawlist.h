#ifndef CTX_DRAWLIST_H
#define CTX_DRAWLIST_H

CTX_STATIC int
ctx_conts_for_entry (const CtxEntry *entry);
void
ctx_iterator_init (CtxIterator      *iterator,
                   CtxDrawlist  *drawlist,
                   int               start_pos,
                   int               flags);

int ctx_iterator_pos (CtxIterator *iterator);

CTX_STATIC void
ctx_drawlist_resize (CtxDrawlist *drawlist, int desired_size);
CTX_STATIC int
ctx_drawlist_add_single (CtxDrawlist *drawlist, const CtxEntry *entry);
CTX_STATIC int ctx_drawlist_add_entry (CtxDrawlist *drawlist, const CtxEntry *entry);
int
ctx_drawlist_insert_entry (CtxDrawlist *drawlist, int pos, CtxEntry *entry);
int
ctx_add_data (Ctx *ctx, void *data, int length);

int ctx_drawlist_add_u32 (CtxDrawlist *drawlist, CtxCode code, uint32_t u32[2]);
int ctx_drawlist_add_data (CtxDrawlist *drawlist, const void *data, int length);

CTX_STATIC CtxEntry
ctx_void (CtxCode code);
CTX_STATIC inline CtxEntry
ctx_f (CtxCode code, float x, float y);
CTX_STATIC CtxEntry
ctx_u32 (CtxCode code, uint32_t x, uint32_t y);
#if 0
CTX_STATIC CtxEntry
ctx_s32 (CtxCode code, int32_t x, int32_t y);
#endif

static inline CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1);
CTX_STATIC CtxEntry
ctx_u8 (CtxCode code,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d,
        uint8_t e, uint8_t f, uint8_t g, uint8_t h);

#define CTX_PROCESS_VOID(cmd) do {\
  CtxEntry commands[1] = {{cmd,{{0}}}};\
  ctx_process (ctx, &commands[0]);}while(0) \

#define CTX_PROCESS_F(cmd,x,y) do {\
  CtxEntry commands[1] = {ctx_f(cmd,x,y),};\
  ctx_process (ctx, &commands[0]);}while(0) \

#define CTX_PROCESS_F1(cmd,x) do {\
  CtxEntry commands[1] = {ctx_f(cmd,x,0),};\
  ctx_process (ctx, &commands[0]);}while(0) \

#define CTX_PROCESS_U32(cmd, x, y) do {\
  CtxEntry commands[1] = {ctx_u32(cmd, x, y)};\
  ctx_process (ctx, &commands[0]);}while(0)

#define CTX_PROCESS_U8(cmd, x) do {\
  CtxEntry commands[4] = {ctx_u8(cmd, x,0,0,0,0,0,0,0)};\
  ctx_process (ctx, &commands[0]);}while(0)


#if CTX_BITPACK_PACKER
CTX_STATIC unsigned int
ctx_last_history (CtxDrawlist *drawlist);
#endif

#if CTX_BITPACK_PACKER
CTX_STATIC void
ctx_drawlist_remove_tiny_curves (CtxDrawlist *drawlist, int start_pos);

CTX_STATIC void
ctx_drawlist_bitpack (CtxDrawlist *drawlist, unsigned int start_pos);
#endif

CTX_STATIC void
ctx_process_cmd_str (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1);
CTX_STATIC void
ctx_process_cmd_str_float (Ctx *ctx, CtxCode code, const char *string, float arg0, float arg1);
CTX_STATIC void
ctx_process_cmd_str_with_len (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1, int len);

#pragma pack(push,1)
typedef union 
CtxSegment {
#if CTX_32BIT_SEGMENTS
   struct {
     int16_t code;
     int16_t aa;
     int32_t x0;
     int32_t y0;
     int32_t y1;
     int32_t x1;
     int32_t val;
     int32_t delta;
   };
   struct {
     int16_t code__;
     int16_t aa__;
     int32_t y0_;
     int32_t y1_;
   };
#else
   struct {
     int8_t code;
     int8_t aa;
     int32_t x0;
     int16_t y0;
     int16_t y1;
     int32_t x1;
   };
   struct {
     int8_t code_;
     int8_t aa_;
     int32_t val;
     int16_t y0_;
     int16_t y1_;
     int32_t delta;
   };
#endif
   uint32_t u32[2];
  } CtxSegment;
#pragma pack(pop)

static inline CtxSegment
ctx_segment_s16 (CtxRasterizerCode code, int x0, int y0, int x1, int y1)
{
  CtxSegment segment;
  segment.x0 = x0;
  segment.y0 = y0;
  segment.x1 = x1;
  segment.y1 = y1;
  segment.code = code;
  return segment;
}

static inline void
ctx_edgelist_resize (CtxDrawlist *drawlist, int desired_size)
{
#if CTX_DRAWLIST_STATIC
    {
      static CtxSegment sbuf[CTX_MAX_EDGE_LIST_SIZE];
      drawlist->entries = (CtxEntry*)&sbuf[0];
      drawlist->size = CTX_MAX_EDGE_LIST_SIZE;
      drawlist->flags = CTX_DRAWLIST_DOESNT_OWN_ENTRIES;
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
      CtxEntry *ne =  (CtxEntry *) ctx_malloc (item_size * new_size);
      memcpy (ne, drawlist->entries, drawlist->size * item_size );
      ctx_free (drawlist->entries);
      drawlist->entries = ne;
      //drawlist->entries = (CtxEntry*)ctx_malloc (drawlist->entries, item_size * new_size);
    }
  else
    {
      //fprintf (stderr, "allocating for %p %d\n", drawlist, new_size);
      drawlist->entries = (CtxEntry *) ctx_malloc (item_size * new_size);
    }
  drawlist->size = new_size;
    }
  //fprintf (stderr, "drawlist %p is %d\n", drawlist, drawlist->size);
#endif
}

static CTX_INLINE int
ctx_edgelist_add_single (CtxDrawlist *drawlist, CtxEntry *entry)
{
  int ret = drawlist->count;

  if (CTX_UNLIKELY(ret + 2 >= drawlist->size))
    {
      if (CTX_UNLIKELY(ret+2 >= CTX_MAX_EDGE_LIST_SIZE- 20))
        return 0;
      int new_ = ctx_maxi (drawlist->size * 2, ret + 1024);
      new_ = ctx_mini (CTX_MAX_EDGE_LIST_SIZE, new_);
      ctx_edgelist_resize (drawlist, new_);
    }

  ((CtxSegment*)(drawlist->entries))[ret] = *(CtxSegment*)entry;
  drawlist->count++;
  return ret;
}

// special return values - controlling argument behavior for some codes
#define CTX_ARG_COLLECT_NUMBERS             50
#define CTX_ARG_STRING_OR_NUMBER            100
#define CTX_ARG_NUMBER_OF_COMPONENTS        200
#define CTX_ARG_NUMBER_OF_COMPONENTS_PLUS_1 201

static inline int ctx_arguments_for_code (CtxCode code)
{
  switch (code)
    {
      case CTX_SAVE:
      case CTX_START_GROUP:
      case CTX_END_GROUP:
      case CTX_IDENTITY:
      case CTX_CLOSE_PATH:
      case CTX_RESET_PATH:
      case CTX_START_FRAME:
      case CTX_END_FRAME:
      case CTX_RESTORE:
      case CTX_STROKE:
      case CTX_FILL:
      case CTX_PAINT:
      case CTX_DEFINE_FONT:
      case CTX_NEW_PAGE:
      case CTX_CLIP:
      case CTX_EXIT:
        return 0;
      case CTX_GLOBAL_ALPHA:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
      case CTX_EXTEND:
      case CTX_FONT_SIZE:
      case CTX_LINE_JOIN:
      case CTX_LINE_CAP:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
      case CTX_STROKE_POS:
      case CTX_FEATHER:
      case CTX_LINE_HEIGHT:
      case CTX_WRAP_LEFT:
      case CTX_WRAP_RIGHT:
      case CTX_IMAGE_SMOOTHING:
      case CTX_SHADOW_BLUR:
      case CTX_SHADOW_OFFSET_X:
      case CTX_SHADOW_OFFSET_Y:
      case CTX_FILL_RULE:
      case CTX_TEXT_ALIGN:
      case CTX_TEXT_BASELINE:
      case CTX_TEXT_DIRECTION:
      case CTX_MITER_LIMIT:
      case CTX_REL_VER_LINE_TO:
      case CTX_REL_HOR_LINE_TO:
      case CTX_HOR_LINE_TO:
      case CTX_VER_LINE_TO:
      case CTX_ROTATE:
      case CTX_GLYPH:
        return 1;
      case CTX_TRANSLATE:
      case CTX_REL_SMOOTHQ_TO:
      case CTX_LINE_TO:
      case CTX_MOVE_TO:
      case CTX_SCALE:
      case CTX_REL_LINE_TO:
      case CTX_REL_MOVE_TO:
      case CTX_SMOOTHQ_TO:
        return 2;
      case CTX_CONIC_GRADIENT:
      case CTX_LINEAR_GRADIENT:
      case CTX_REL_QUAD_TO:
      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_FILL_RECT:
      case CTX_STROKE_RECT:
      case CTX_REL_SMOOTH_TO:
      case CTX_VIEW_BOX:
      case CTX_SMOOTH_TO:
        return 4;
      case CTX_ROUND_RECTANGLE:
        return 5;
      case CTX_ARC:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_RADIAL_GRADIENT:
        return 6;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
        return 7;
      case CTX_APPLY_TRANSFORM:
      case CTX_SOURCE_TRANSFORM:
        return 9;
      case CTX_TEXT:
      case CTX_FONT:
      case CTX_COLOR_SPACE:
      case CTX_DEFINE_GLYPH:
      case CTX_KERNING_PAIR:
      case CTX_TEXTURE:
      case CTX_DEFINE_TEXTURE:
        return CTX_ARG_STRING_OR_NUMBER;
      case CTX_LINE_DASH: /* append to current dashes for each argument encountered */
        return CTX_ARG_COLLECT_NUMBERS;
      //case CTX_SET_KEY:
      case CTX_COLOR:
      case CTX_SHADOW_COLOR:
        return CTX_ARG_NUMBER_OF_COMPONENTS;
      case CTX_GRADIENT_STOP:
        return CTX_ARG_NUMBER_OF_COMPONENTS_PLUS_1;

        default:
#if 1
        case CTX_SET_RGBA_U8:
        case CTX_NOP:
        case CTX_CONT:
        case CTX_DATA:
        case CTX_DATA_REV:
        case CTX_SET_PIXEL:
        case CTX_REL_LINE_TO_X4:
        case CTX_REL_LINE_TO_REL_CURVE_TO:
        case CTX_REL_CURVE_TO_REL_LINE_TO:
        case CTX_REL_CURVE_TO_REL_MOVE_TO:
        case CTX_REL_LINE_TO_X2:
        case CTX_MOVE_TO_REL_LINE_TO:
        case CTX_REL_LINE_TO_REL_MOVE_TO:
        case CTX_FILL_MOVE_TO:
        case CTX_REL_QUAD_TO_REL_QUAD_TO:
        case CTX_REL_QUAD_TO_S16:
        case CTX_STROKE_SOURCE:
#endif
        return 0;
    }
}


#endif

