#include "ctx-split.h"
#include <fcntl.h>
#include <sys/ioctl.h>

#if CTX_EVENTS

typedef struct _CtxBraille CtxBraille;
struct _CtxBraille
{
   void (*render) (void *braille, CtxCommand *command);
   void (*flush)  (void *braille);
   void (*free)   (void *braille);
   Ctx     *ctx;
   int      width;
   int      height;
   int      cols;
   int      rows;
   int      was_down;
   uint8_t *pixels;
   Ctx     *host;
};

inline static void ctx_braille_flush (CtxBraille *braille)
{
  int width =  braille->width;
  int height = braille->height;
  ctx_render_ctx (braille->ctx, braille->host);
  printf ("\e[H");
  _ctx_utf8_output_buf (braille->pixels,
                        CTX_FORMAT_RGBA8,
                        width, height, width * 4, 0);
#if CTX_BRAILLE_TEXT
  CtxRasterizer *rasterizer = (CtxRasterizer*)(braille->host->renderer);
  // XXX instead sort and inject along with braille
  for (CtxList *l = rasterizer->glyphs; l; l = l->next)
  {
      CtxTermGlyph *glyph = l->data;
      printf ("\e[0m\e[%i;%iH%c", glyph->row, glyph->col, glyph->unichar);
      free (glyph);
  }
  while (rasterizer->glyphs)
    ctx_list_remove (&rasterizer->glyphs, rasterizer->glyphs->data);
#endif
}

void ctx_braille_free (CtxBraille *braille)
{
  nc_at_exit ();
  free (braille->pixels);
  ctx_free (braille->host);
  free (braille);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

int ctx_renderer_is_braille (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_braille_free)
          return 1;
  return 0;
}

Ctx *ctx_new_braille (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  CtxBraille *braille = calloc (sizeof (CtxBraille), 1);
  int maxwidth = ctx_terminal_cols  () * 2;
  int maxheight = (ctx_terminal_rows ()-1) * 4;
  if (width <= 0 || height <= 0)
  {
    width = maxwidth;
    height = maxheight;
  }
  if (width > maxwidth) width = maxwidth;
  if (height > maxheight) height = maxheight;
  braille->ctx = ctx;
  braille->width  = width;
  braille->height = height;
  braille->cols = (width + 1) / 2;
  braille->rows = (height + 3) / 4;
  braille->pixels = (uint8_t*)malloc (width * height * 4);
  braille->host = ctx_new_for_framebuffer (braille->pixels,
                                           width, height,
                                           width * 4, CTX_FORMAT_RGBA8);
#if CTX_BRAILLE_TEXT
  ((CtxRasterizer*)braille->host->renderer)->term_glyphs=1;
#endif
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, braille);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, 4.0f);
  braille->flush = (void*)ctx_braille_flush;
  braille->free  = (void*)ctx_braille_free;
#endif
  return ctx;
}

#endif
