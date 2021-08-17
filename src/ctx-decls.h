
uint64_t    thash              (const char *utf8);
const char *thash_decode       (uint64_t hash);
uint64_t    ctx_strhash        (const char *str);
CtxColor   *ctx_color_new      (void);
int         ctx_get_int        (Ctx *ctx, uint64_t hash);
int         ctx_get_is_set     (Ctx *ctx, uint64_t hash);
Ctx        *ctx_new_for_buffer (CtxBuffer *buffer);
