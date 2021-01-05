#ifndef CTX_DRAWLIST_H
#define CTX_DRAWLIST_H

CTX_STATIC int
ctx_conts_for_entry (CtxEntry *entry);
CTX_STATIC void
ctx_iterator_init (CtxIterator      *iterator,
                   CtxRenderstream  *drawlist,
                   int               start_pos,
                   int               flags);

CtxCommand *
ctx_iterator_next (CtxIterator *iterator);

CTX_STATIC void ctx_drawlist_compact (CtxRenderstream *drawlist);
CTX_STATIC void
ctx_drawlist_resize (CtxRenderstream *drawlist, int desired_size);
CTX_STATIC int
ctx_drawlist_add_single (CtxRenderstream *drawlist, CtxEntry *entry);
int
ctx_add_single (Ctx *ctx, void *entry);
int
ctx_drawlist_add_entry (CtxRenderstream *drawlist, CtxEntry *entry);
int ctx_append_drawlist (Ctx *ctx, void *data, int length);
int ctx_set_drawlist (Ctx *ctx, void *data, int length);
int ctx_get_drawlist_count (Ctx *ctx);
const CtxEntry *ctx_get_drawlist (Ctx *ctx);
int
ctx_add_data (Ctx *ctx, void *data, int length);

int ctx_drawlist_add_u32 (CtxRenderstream *drawlist, CtxCode code, uint32_t u32[2]);
int ctx_drawlist_add_data (CtxRenderstream *drawlist, const void *data, int length);

CTX_STATIC CtxEntry
ctx_void (CtxCode code);
CTX_STATIC CtxEntry
ctx_f (CtxCode code, float x, float y);
CTX_STATIC CtxEntry
ctx_u32 (CtxCode code, uint32_t x, uint32_t y);
#if 0
CTX_STATIC CtxEntry
ctx_s32 (CtxCode code, int32_t x, int32_t y);
#endif

CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1);
CTX_STATIC CtxEntry
ctx_u8 (CtxCode code,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d,
        uint8_t e, uint8_t f, uint8_t g, uint8_t h);

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


#if CTX_BITPACK_PACKER
CTX_STATIC int
ctx_last_history (CtxRenderstream *drawlist);
#endif

#if CTX_BITPACK_PACKER
CTX_STATIC void
ctx_drawlist_remove_tiny_curves (CtxRenderstream *drawlist, int start_pos);

CTX_STATIC void
ctx_drawlist_bitpack (CtxRenderstream *drawlist, int start_pos);
#endif

CTX_STATIC void
ctx_drawlist_compact (CtxRenderstream *drawlist);
CTX_STATIC void
ctx_process_cmd_str (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1);
CTX_STATIC void
ctx_process_cmd_str_with_len (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1, int len);

#endif

