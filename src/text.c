#include "ctx-split.h"

static CtxFont ctx_fonts[CTX_MAX_FONTS];
static int     ctx_font_count = 0;

#if CTX_FONT_ENGINE_STB
static float
ctx_glyph_width_stb (CtxFont *font, Ctx *ctx, uint32_t unichar);
static float
ctx_glyph_kern_stb (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB);
static int
ctx_glyph_stb (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke);

CtxFontEngine ctx_font_engine_stb =
{
#if CTX_FONTS_FROM_FILE
  ctx_load_font_ttf_file,
#endif
  ctx_load_font_ttf,
  ctx_glyph_stb,
  ctx_glyph_width_stb,
  ctx_glyph_kern_stb,
};

int
ctx_load_font_ttf (const char *name, const void *ttf_contents, int length)
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }
  ctx_fonts[ctx_font_count].type = 1;
  ctx_fonts[ctx_font_count].name = (char *) malloc (strlen (name) + 1);
  ctx_strcpy ( (char *) ctx_fonts[ctx_font_count].name, name);
  if (!stbtt_InitFont (&ctx_fonts[ctx_font_count].stb.ttf_info, ttf_contents, 0) )
    {
      ctx_log ( "Font init failed\n");
      return -1;
    }
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_stb;
  ctx_font_count ++;
  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ttf_file (const char *name, const char *path)
{
  uint8_t *contents = NULL;
  long length = 0;
  _ctx_file_get_contents (path, &contents, &length);
  if (!contents)
    {
      ctx_log ( "File load failed\n");
      return -1;
    }
  return ctx_load_font_ttf (name, contents, length);
}
#endif

static int
ctx_glyph_stb_find (CtxFont *font, uint32_t unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  int index = font->stb.cache_index;
  if (font->stb.cache_unichar == unichar)
    {
      return index;
    }
  font->stb.cache_unichar = 0;
  index = font->stb.cache_index = stbtt_FindGlyphIndex (ttf_info, unichar);
  font->stb.cache_unichar = unichar;
  return index;
}

static float
ctx_glyph_width_stb (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  float font_size          = ctx->state.gstate.font_size;
  float scale              = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int advance, lsb;
  int glyph = ctx_glyph_stb_find (font, unichar);
  if (glyph==0)
    { return 0.0f; }
  stbtt_GetGlyphHMetrics (ttf_info, glyph, &advance, &lsb);
  return (advance * scale);
}

static float
ctx_glyph_kern_stb (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  float font_size = ctx->state.gstate.font_size;
  float scale = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int glyphA = ctx_glyph_stb_find (font, unicharA);
  int glyphB = ctx_glyph_stb_find (font, unicharB);
  return stbtt_GetGlyphKernAdvance (ttf_info, glyphA, glyphB) * scale;
}

static int
ctx_glyph_stb (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  int glyph = ctx_glyph_stb_find (font, unichar);
  if (glyph==0)
    { return -1; }
  float font_size = ctx->state.gstate.font_size;
  int   baseline = ctx->state.y;
  float origin_x = ctx->state.x;
  float origin_y = baseline;
  float scale    = stbtt_ScaleForPixelHeight (ttf_info, font_size);;
  stbtt_vertex *vertices = NULL;
  ctx_begin_path (ctx);
  int num_verts = stbtt_GetGlyphShape (ttf_info, glyph, &vertices);
  for (int i = 0; i < num_verts; i++)
    {
      stbtt_vertex *vertex = &vertices[i];
      switch (vertex->type)
        {
          case STBTT_vmove:
            ctx_move_to (ctx,
                         origin_x + vertex->x * scale, origin_y - vertex->y * scale);
            break;
          case STBTT_vline:
            ctx_line_to (ctx,
                         origin_x + vertex->x * scale, origin_y - vertex->y * scale);
            break;
          case STBTT_vcubic:
            ctx_curve_to (ctx,
                          origin_x + vertex->cx  * scale, origin_y - vertex->cy  * scale,
                          origin_x + vertex->cx1 * scale, origin_y - vertex->cy1 * scale,
                          origin_x + vertex->x   * scale, origin_y - vertex->y   * scale);
            break;
          case STBTT_vcurve:
            ctx_quad_to (ctx,
                         origin_x + vertex->cx  * scale, origin_y - vertex->cy  * scale,
                         origin_x + vertex->x   * scale, origin_y - vertex->y   * scale);
            break;
        }
    }
  stbtt_FreeShape (ttf_info, vertices);
  if (stroke)
    {
      ctx_stroke (ctx);
    }
  else
    { ctx_fill (ctx); }
  return 0;
}
#endif

#if CTX_FONT_ENGINE_CTX

/* XXX: todo remove this, and rely on a binary search instead
 */
static int ctx_font_find_glyph_cached (CtxFont *font, uint32_t glyph)
{
  for (int i = 0; i < font->ctx.glyphs; i++)
    {
      if (font->ctx.index[i * 2] == glyph)
        { return font->ctx.index[i * 2 + 1]; }
    }
  return -1;
}

static int ctx_glyph_find_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  int ret = ctx_font_find_glyph_cached (font, unichar);
  if (ret >= 0) return ret;

  for (int i = 0; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH &&
        entry->data.u32[0] == unichar)
    {
       return i;
       // XXX this could be prone to insertion of valid header
       // data in included bitmaps.. is that an issue?
       //   
    }
  }
  return -1;
}


static float
ctx_glyph_kern_ctx (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  float font_size = ctx->state.gstate.font_size;
  int first_kern = ctx_glyph_find_ctx (font, ctx, unicharA);
  if (first_kern < 0) return 0.0;
  for (int i = first_kern + 1; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_KERNING_PAIR)
        {
          if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
            { return entry->data.s32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE; }
        }
      if (entry->code == CTX_DEFINE_GLYPH)
        return 0.0;
    }
  return 0.0;
}
#if 0
static int ctx_glyph_find (Ctx *ctx, CtxFont *font, uint32_t unichar)
{
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
        { return i; }
    }
  return 0;
}
#endif


static float
ctx_glyph_width_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  CtxState *state = &ctx->state;
  float font_size = state->gstate.font_size;
  int   start     = ctx_glyph_find_ctx (font, ctx, unichar);
  if (start < 0)
    { return 0.0; }  // XXX : fallback
  for (int i = start; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        if (entry->data.u32[0] == (unsigned) unichar)
          { return (entry->data.u32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE); }
    }
  return 0.0;
}

static int
ctx_glyph_drawlist (CtxFont *font, Ctx *ctx, CtxDrawlist *drawlist, uint32_t unichar, int stroke)
{
  CtxState *state = &ctx->state;
  CtxIterator iterator;
  float origin_x = state->x;
  float origin_y = state->y;
  ctx_current_point (ctx, &origin_x, &origin_y);
  int in_glyph = 0;
  float font_size = state->gstate.font_size;
  int start = 0;
  if (font->type == 0)
  {
  start = ctx_glyph_find_ctx (font, ctx, unichar);
  if (start < 0)
    { return -1; }  // XXX : fallback glyph
  }
  ctx_iterator_init (&iterator, drawlist, start, CTX_ITERATOR_EXPAND_BITPACK);
  CtxCommand *command;

  /* XXX :  do a binary search instead of a linear search */
  while ( (command= ctx_iterator_next (&iterator) ) )
    {
      CtxEntry *entry = &command->entry;
      if (in_glyph)
        {
          if (entry->code == CTX_DEFINE_GLYPH)
            {
              if (stroke)
                { ctx_stroke (ctx); }
              else
                {
#if CTX_RASTERIZER
#if CTX_ENABLE_SHADOW_BLUR
      if (ctx->renderer && ((CtxRasterizer*)(ctx->renderer))->in_shadow)
      {
        ctx_rasterizer_shadow_fill ((CtxRasterizer*)ctx->renderer);
        ((CtxRasterizer*)(ctx->renderer))->in_shadow = 1;
      }
      else
#endif
#endif
         ctx_fill (ctx); 
               
                }
              ctx_restore (ctx);
              return 0;
            }
          ctx_process (ctx, entry);
        }
      else if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
        {
          in_glyph = 1;
          ctx_save (ctx);
          ctx_translate (ctx, origin_x, origin_y);
          ctx_move_to (ctx, 0, 0);
          ctx_begin_path (ctx);
          ctx_scale (ctx, font_size / CTX_BAKE_FONT_SIZE,
                     font_size / CTX_BAKE_FONT_SIZE);
        }
    }
  if (stroke)
    { ctx_stroke (ctx);
    }
  else
    { 
    
#if CTX_RASTERIZER
#if CTX_ENABLE_SHADOW_BLUR
      if (ctx->renderer && ((CtxRasterizer*)(ctx->renderer))->in_shadow)
      {
        ctx_rasterizer_shadow_fill ((CtxRasterizer*)ctx->renderer);
        ((CtxRasterizer*)(ctx->renderer))->in_shadow = 1;
      }
      else
#endif
#endif
      {
         ctx_fill (ctx); 
      }
    }
  ctx_restore (ctx);
  return -1;
}

static int
ctx_glyph_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxDrawlist drawlist = { (CtxEntry *) font->ctx.data,
                           font->ctx.length,
                           font->ctx.length, 0, 0
                         };
  return ctx_glyph_drawlist (font, ctx, &drawlist, unichar, stroke);
}

uint32_t ctx_glyph_no (Ctx *ctx, int no)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  if (no < 0 || no >= font->ctx.glyphs)
    { return 0; }
  return font->ctx.index[no*2];
}

static void ctx_font_init_ctx (CtxFont *font)
{
  int glyph_count = 0;
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        { glyph_count ++; }
    }
  font->ctx.glyphs = glyph_count;
#if CTX_DRAWLIST_STATIC
  static uint32_t idx[512]; // one might have to adjust this for
  // larger fonts XXX
  // should probably be made a #define
  font->ctx.index = &idx[0];
#else
  font->ctx.index = (uint32_t *) malloc (sizeof (uint32_t) * 2 * glyph_count);
#endif
  int no = 0;
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        {
          font->ctx.index[no*2]   = entry->data.u32[0];
          font->ctx.index[no*2+1] = i;
          no++;
        }
    }
}

int
ctx_load_font_ctx (const char *name, const void *data, int length);
#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path);
#endif

static CtxFontEngine ctx_font_engine_ctx =
{
#if CTX_FONTS_FROM_FILE
  ctx_load_font_ctx_file,
#endif
  ctx_load_font_ctx,
  ctx_glyph_ctx,
  ctx_glyph_width_ctx,
  ctx_glyph_kern_ctx,
};

int
ctx_load_font_ctx (const char *name, const void *data, int length)
{
  if (length % sizeof (CtxEntry) )
    { return -1; }
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }
  ctx_fonts[ctx_font_count].type = 0;
  ctx_fonts[ctx_font_count].name = name;
  ctx_fonts[ctx_font_count].ctx.data = (CtxEntry *) data;
  ctx_fonts[ctx_font_count].ctx.length = length / sizeof (CtxEntry);
  ctx_font_init_ctx (&ctx_fonts[ctx_font_count]);
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_ctx;
  ctx_font_count++;
  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path)
{
  uint8_t *contents = NULL;
  long length = 0;
  _ctx_file_get_contents (path, &contents, &length);
  if (!contents)
    {
      ctx_log ( "File load failed\n");
      return -1;
    }
  return ctx_load_font_ctx (name, contents, length);
}
#endif
#endif

#if CTX_FONT_ENGINE_CTX_FS

static float
ctx_glyph_kern_ctx_fs (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
#if 0
  float font_size = ctx->state.gstate.font_size;
  int first_kern = ctx_glyph_find_ctx (font, ctx, unicharA);
  if (first_kern < 0) return 0.0;
  for (int i = first_kern + 1; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_KERNING_PAIR)
        {
          if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
            { return entry->data.s32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE; }
        }
      if (entry->code == CTX_DEFINE_GLYPH)
        return 0.0;
    }
#endif
  return 0.0;
}

static float
ctx_glyph_width_ctx_fs (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  CtxState *state = &ctx->state;
  char path[1024];
  sprintf (path, "%s/%010p", font->ctx_fs.path, unichar);
  uint8_t *data = NULL;
  long int len_bytes = 0;
  _ctx_file_get_contents (path, &data, &len_bytes);
  float ret = 0.0;
  float font_size = state->gstate.font_size;
  if (data){
    Ctx *glyph_ctx = ctx_new ();
    ctx_parse (glyph_ctx, data);
    //fprintf (stderr, "\n");
    for (int i = 0; i < glyph_ctx->drawlist.count; i++)
    {
      CtxEntry *e = &glyph_ctx->drawlist.entries[i];
   // fprintf (stderr, "%c:", e->code);
      if (e->code == CTX_DEFINE_GLYPH)
        ret = e->data.u32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE;
    }
    free (data);
    ctx_free (glyph_ctx);
  }
  return ret;
}

static int
ctx_glyph_ctx_fs (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  char path[1024];
  sprintf (path, "%s/%010p", font->ctx_fs.path, unichar);
  uint8_t *data = NULL;
  long int len_bytes = 0;
  _ctx_file_get_contents (path, &data, &len_bytes);
//fprintf (stderr, "%s %li\n", path, len_bytes);

  if (data){
    Ctx *glyph_ctx = ctx_new ();
    ctx_parse (glyph_ctx, data);
    int ret = ctx_glyph_drawlist (font, ctx, &(glyph_ctx->drawlist),
                                  unichar, stroke);
    free (data);
    ctx_free (glyph_ctx);
    return ret;
  }
  return -1;
}

int
ctx_load_font_ctx_fs (const char *name, const void *data, int length);

static CtxFontEngine ctx_font_engine_ctx_fs =
{
#if CTX_FONTS_FROM_FILE
  NULL,
#endif
  ctx_load_font_ctx_fs,
  ctx_glyph_ctx_fs,
  ctx_glyph_width_ctx_fs,
  ctx_glyph_kern_ctx_fs,
};

int
ctx_load_font_ctx_fs (const char *name, const void *path, int length) // length is ignored
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }

  ctx_fonts[ctx_font_count].type = 42;
  ctx_fonts[ctx_font_count].name = name;
  ctx_fonts[ctx_font_count].ctx_fs.path = strdup (path);
  int path_len = strlen (path);
  if (ctx_fonts[ctx_font_count].ctx_fs.path[path_len-1] == '/')
   ctx_fonts[ctx_font_count].ctx_fs.path[path_len-1] = 0;
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_ctx_fs;
  ctx_font_count++;
  return ctx_font_count-1;
}

#endif

int
_ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  // a begin-path here did not remove stray spikes in terminal
  return font->engine->glyph (font, ctx, unichar, stroke);
}

int
ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
#if CTX_BACKEND_TEXT
  CtxEntry commands[3]; // 3 to silence incorrect warning from static analysis
  ctx_memset (commands, 0, sizeof (commands) );
  commands[0] = ctx_u32 (CTX_GLYPH, unichar, 0);
  commands[0].data.u8[4] = stroke;
  ctx_process (ctx, commands);
  return 0; // XXX is return value used?
#else
  return _ctx_glyph (ctx, unichar, stroke);
#endif
}

float
ctx_glyph_width (Ctx *ctx, int unichar)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  return font->engine->glyph_width (font, ctx, unichar);
}

static float
ctx_glyph_kern (Ctx *ctx, int unicharA, int unicharB)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  return font->engine->glyph_kern (font, ctx, unicharA, unicharB);
}

float
ctx_text_width (Ctx        *ctx,
                const char *string)
{
  float sum = 0.0;
  if (!string)
    return 0.0f;
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1) )
    {
      sum += ctx_glyph_width (ctx, ctx_utf8_to_unichar (utf8) );
    }
  return sum;
}

static void
_ctx_glyphs (Ctx     *ctx,
             CtxGlyph *glyphs,
             int       n_glyphs,
             int       stroke)
{
  for (int i = 0; i < n_glyphs; i++)
    {
      {
        uint32_t unichar = glyphs[i].index;
        ctx_move_to (ctx, glyphs[i].x, glyphs[i].y);
        ctx_glyph (ctx, unichar, stroke);
      }
    }
}

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible)
{
  CtxState *state = &ctx->state;
  float x = ctx->state.x;
  switch ( (int) ctx_state_get (state, CTX_text_align) )
    //switch (state->gstate.text_align)
    {
      case CTX_TEXT_ALIGN_START:
      case CTX_TEXT_ALIGN_LEFT:
        break;
      case CTX_TEXT_ALIGN_CENTER:
        x -= ctx_text_width (ctx, string) /2;
        break;
      case CTX_TEXT_ALIGN_END:
      case CTX_TEXT_ALIGN_RIGHT:
        x -= ctx_text_width (ctx, string);
        break;
    }
  float y = ctx->state.y;
  float baseline_offset = 0.0f;
  switch ( (int) ctx_state_get (state, CTX_text_baseline) )
    {
      case CTX_TEXT_BASELINE_HANGING:
        /* XXX : crude */
        baseline_offset = ctx->state.gstate.font_size  * 0.55;
        break;
      case CTX_TEXT_BASELINE_TOP:
        /* XXX : crude */
        baseline_offset = ctx->state.gstate.font_size  * 0.7;
        break;
      case CTX_TEXT_BASELINE_BOTTOM:
        baseline_offset = -ctx->state.gstate.font_size * 0.1;
        break;
      case CTX_TEXT_BASELINE_ALPHABETIC:
      case CTX_TEXT_BASELINE_IDEOGRAPHIC:
        baseline_offset = 0.0f;
        break;
      case CTX_TEXT_BASELINE_MIDDLE:
        baseline_offset = ctx->state.gstate.font_size * 0.25;
        break;
    }
  float x0 = x;
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1) )
    {
      if (*utf8 == '\n')
        {
          y += ctx->state.gstate.font_size * ctx_state_get (state, CTX_line_spacing);
          x = x0;
          if (visible)
            { ctx_move_to (ctx, x, y); }
        }
      else
        {
          uint32_t unichar = ctx_utf8_to_unichar (utf8);
          if (visible)
            {
              ctx_move_to (ctx, x, y + baseline_offset);
              _ctx_glyph (ctx, unichar, stroke);
            }
          const char *next_utf8 = ctx_utf8_skip (utf8, 1);
          if (next_utf8)
            {
              x += ctx_glyph_width (ctx, unichar);
              x += ctx_glyph_kern (ctx, unichar, ctx_utf8_to_unichar (next_utf8) );
            }
          if (visible)
            { ctx_move_to (ctx, x, y); }
        }
    }
  if (!visible)
    { ctx_move_to (ctx, x, y); }
}


CtxGlyph *
ctx_glyph_allocate (int n_glyphs)
{
  return (CtxGlyph *) malloc (sizeof (CtxGlyph) * n_glyphs);
}
void
gtx_glyph_free     (CtxGlyph *glyphs)
{
  free (glyphs);
}

void
ctx_glyphs (Ctx        *ctx,
            CtxGlyph   *glyphs,
            int         n_glyphs)
{
  _ctx_glyphs (ctx, glyphs, n_glyphs, 0);
}

void
ctx_glyphs_stroke (Ctx        *ctx,
                   CtxGlyph   *glyphs,
                   int         n_glyphs)
{
  _ctx_glyphs (ctx, glyphs, n_glyphs, 1);
}

void
ctx_fill_text (Ctx        *ctx,
               const char *string)
{
  if (!string)
    return;
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_FILL_TEXT, string, 0, 0);
  _ctx_text (ctx, string, 0, 0);
#else
  _ctx_text (ctx, string, 0, 1);
#endif
}


void
ctx_text (Ctx *ctx, const char *string)
{
  ctx_fill_text (ctx, string);
}

void
ctx_stroke_text (Ctx        *ctx,
                 const char *string)
{
  if (!string)
    return;
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_STROKE_TEXT, string, 0, 0);
  _ctx_text (ctx, string, 1, 0);
#else
  _ctx_text (ctx, string, 1, 1);
#endif
}

static int _ctx_resolve_font (const char *name)
{
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (!ctx_strcmp (ctx_fonts[i].name, name) )
        { return i; }
    }
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (ctx_strstr (ctx_fonts[i].name, name) )
        { return i; }
    }
  return -1;
}

int ctx_resolve_font (const char *name)
{
  int ret = _ctx_resolve_font (name);
  if (ret >= 0)
    { return ret; }
  if (!ctx_strcmp (name, "regular") )
    {
      int ret = _ctx_resolve_font ("sans");
      if (ret >= 0) { return ret; }
      ret = _ctx_resolve_font ("serif");
      if (ret >= 0) { return ret; }
    }
  return 0;
}

CTX_STATIC void ctx_font_setup ()
{
  static int initialized = 0;
  if (initialized) { return; }
  initialized = 1;
#if CTX_FONT_ENGINE_CTX
  ctx_font_count = 0; // oddly - this is needed in arduino

#if CTX_FONT_ENGINE_CTX_FS
  ctx_load_font_ctx_fs ("sans-ctx", "/tmp/ctx-regular", 0);
#else
#if CTX_FONT_ascii
  ctx_load_font_ctx ("sans-ctx", ctx_font_ascii, sizeof (ctx_font_ascii) );
#endif
#if CTX_FONT_regular
  ctx_load_font_ctx ("sans-ctx", ctx_font_regular, sizeof (ctx_font_regular) );
#endif
#endif

#if CTX_FONT_mono
  ctx_load_font_ctx ("mono-ctx", ctx_font_mono, sizeof (ctx_font_mono) );
#endif
#if CTX_FONT_bold
  ctx_load_font_ctx ("bold-ctx", ctx_font_bold, sizeof (ctx_font_bold) );
#endif
#if CTX_FONT_italic
  ctx_load_font_ctx ("italic-ctx", ctx_font_italic, sizeof (ctx_font_italic) );
#endif
#if CTX_FONT_sans
  ctx_load_font_ctx ("sans-ctx", ctx_font_sans, sizeof (ctx_font_sans) );
#endif
#if CTX_FONT_serif
  ctx_load_font_ctx ("serif-ctx", ctx_font_serif, sizeof (ctx_font_serif) );
#endif
#if CTX_FONT_symbol
  ctx_load_font_ctx ("symbol-ctx", ctx_font_symbol, sizeof (ctx_font_symbol) );
#endif
#if CTX_FONT_emoji
  ctx_load_font_ctx ("emoji-ctx", ctx_font_emoji, sizeof (ctx_font_emoji) );
#endif
#endif

#if NOTO_EMOJI_REGULAR
  ctx_load_font_ttf ("sans-NotoEmoji_Regular", ttf_NotoEmoji_Regular_ttf, ttf_NotoEmoji_Regular_ttf_len);
#endif
#if ROBOTO_LIGHT
  ctx_load_font_ttf ("sans-light-Roboto_Light", ttf_Roboto_Light_ttf, ttf_Roboto_Light_ttf_len);
#endif
#if ROBOTO_REGULAR
  ctx_load_font_ttf ("sans-Roboto_Regular", ttf_Roboto_Regular_ttf, ttf_Roboto_Regular_ttf_len);
#endif
#if ROBOTO_BOLD
  ctx_load_font_ttf ("sans-bold-Roboto_Bold", ttf_Roboto_Bold_ttf, ttf_Roboto_Bold_ttf_len);
#endif
#if DEJAVU_SANS
  ctx_load_font_ttf ("sans-DejaVuSans", ttf_DejaVuSans_ttf, ttf_DejaVuSans_ttf_len);
#endif
#if VERA
  ctx_load_font_ttf ("sans-Vera", ttf_Vera_ttf, ttf_Vera_ttf_len);
#endif
#if UNSCII_16
  ctx_load_font_ttf ("mono-unscii16", ttf_unscii_16_ttf, ttf_unscii_16_ttf_len);
#endif
#if XA000_MONO
  ctx_load_font_ttf ("mono-0xA000", ttf_0xA000_Mono_ttf, ttf_0xA000_Mono_ttf_len);
#endif
#if DEJAVU_SANS_MONO
  ctx_load_font_ttf ("mono-DejaVuSansMono", ttf_DejaVuSansMono_ttf, ttf_DejaVuSansMono_ttf_len);
#endif
#if NOTO_MONO_REGULAR
  ctx_load_font_ttf ("mono-NotoMono_Regular", ttf_NotoMono_Regular_ttf, ttf_NotoMono_Regular_ttf_len);
#endif
}


