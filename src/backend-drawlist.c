
void ctx_drawlist_clear (Ctx *ctx)
{
  ctx->drawlist.count = 0;
}

static void ctx_drawlist_backend_destroy (void *backend)
{
  ctx_free (backend);
}

static void ctx_update_current_path (Ctx *ctx, const CtxEntry *entry)
{
#if CTX_CURRENT_PATH
  switch (entry->code)
    {
      case CTX_TEXT:
      case CTX_RESET_PATH:
        ctx->current_path.count = 0;
        break;
      case CTX_CLIP:
      case CTX_FILL:
      case CTX_STROKE:
              // XXX unless preserve
        ctx->current_path.count = 0;
        break;
      case CTX_CLOSE_PATH:
      case CTX_LINE_TO:
      case CTX_MOVE_TO:
      case CTX_CURVE_TO:
      case CTX_QUAD_TO:
      case CTX_SMOOTH_TO:
      case CTX_SMOOTHQ_TO:
      case CTX_REL_LINE_TO:
      case CTX_REL_MOVE_TO:
      case CTX_REL_QUAD_TO:
      case CTX_REL_SMOOTH_TO:
      case CTX_REL_SMOOTHQ_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_RECTANGLE:
      case CTX_ROUND_RECTANGLE:
        ctx_drawlist_add_entry (&ctx->current_path, entry);
        break;
      default:
        break;
    }
#endif
}

static void
ctx_drawlist_process (Ctx *ctx, const CtxCommand *command)
{
  CtxEntry *entry = (CtxEntry*)command;
#if CTX_CURRENT_PATH
  ctx_update_current_path (ctx, entry);
#endif
  /* these functions can alter the code and coordinates of
     command that in the end gets added to the drawlist
   */
  ctx_interpret_style (&ctx->state, entry, ctx);
  ctx_interpret_transforms (&ctx->state, entry, ctx);
  ctx_interpret_pos (&ctx->state, entry, ctx);
  ctx_drawlist_add_entry (&ctx->drawlist, entry);
}

static CtxBackend *ctx_drawlist_backend_new (void)
{
  CtxBackend *backend = (CtxBackend*)ctx_calloc (1, sizeof (CtxCtx));
                       // the sizeof(CtxCtx) should actually be sizeof(CtxBackend)
                       // but static analysis complains about event code
                       // initializing the extra members - which might most
                       // often be a false report - we add slack since it is
                       // "only" ~ 40 bytes per instance.
  backend->process = ctx_drawlist_process;
  backend->destroy = ctx_drawlist_backend_destroy;
  backend->type = CTX_BACKEND_DRAWLIST;
  return backend;
}
