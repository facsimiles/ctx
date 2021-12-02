
uint32_t    ctx_strhash        (const char *str);
CtxColor   *ctx_color_new      (void);
int         ctx_get_int        (Ctx *ctx, uint32_t hash);
int         ctx_get_is_set     (Ctx *ctx, uint32_t hash);
Ctx        *ctx_new_for_buffer (CtxBuffer *buffer);
