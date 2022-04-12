#include "ctx-split.h"

static CtxFont ctx_fonts[CTX_MAX_FONTS];// = NULL;
static int     ctx_font_count = 0;

static void ctx_font_setup (Ctx *ctx);

static inline int ctx_font_is_monospaced (CtxFont *font)
{
#if CTX_ONE_FONT_ENGINE
  return 0; // XXX
#else
  return font->monospaced;
#endif
}

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
  char buf[256];
  ctx_font_setup (NULL);
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }

  if (!stbtt_InitFont (&ctx_fonts[ctx_font_count].stb.ttf_info, ttf_contents, 0) )
    {
      ctx_log ( "Font init failed\n");
      return -1;
    }

  if (name == NULL || !strcmp (name, "import")){
  int length = 0;
  const char *val = stbtt_GetFontNameDefault (&ctx_fonts[ctx_font_count].stb.ttf_info,
                          &length);
  if (val)
  {
    memset(buf,0,sizeof(buf));
    memcpy(buf,val, length);
    name = buf;
  }
  else
    name = "import";
  }

  ctx_fonts[ctx_font_count].type = 1;
  ctx_fonts[ctx_font_count].stb.name = (char *) ctx_malloc (ctx_strlen (name) + 1);
  ctx_strcpy ( (char *) ctx_fonts[ctx_font_count].stb.name, name);

  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_stb;

  CtxFont *font = &ctx_fonts[ctx_font_count];
  if (font->engine->glyph_width (font, NULL, 'O') ==
      font->engine->glyph_width (font, NULL, 'I'))
  {
    font->monospaced = 1;
  }
  else
    font->monospaced = 0;

  ctx_font_count ++;
  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ttf_file (const char *name, const char *path)
{
  ctx_font_setup (NULL);
  uint8_t *contents = NULL;
  long length = 0;
  ctx_get_contents (path, &contents, &length);
  if (!contents)
    {
      ctx_log ( "File load failed\n");
      return -1;
    }
  return ctx_load_font_ttf (name, contents, length);
}
#endif

static int
ctx_glyph_stb_find (Ctx *ctx, CtxFont *font, uint32_t unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;

#if CTX_GLYPH_CACHE
  uint32_t hash = ((((size_t)(font) * 23) ^ unichar) * 17) %
            (CTX_GLYPH_CACHE_SIZE);
  if (ctx)
  {
    if (ctx->glyph_index_cache[hash].font == font &&
        ctx->glyph_index_cache[hash].unichar == unichar)
          return ctx->glyph_index_cache[hash].offset;
  }
#endif

  int index = stbtt_FindGlyphIndex (ttf_info, unichar);

#if CTX_GLYPH_CACHE
  if (ctx)
  {
    ctx->glyph_index_cache[hash].font    = font;
    ctx->glyph_index_cache[hash].unichar = unichar;
    ctx->glyph_index_cache[hash].offset  = index;
  }
#endif

  return index;
}

static float
ctx_glyph_width_stb (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  float font_size          = 1.0f;
  if (ctx)
      font_size = ctx->state.gstate.font_size;
  float scale              = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int advance, lsb;
  int glyph = ctx_glyph_stb_find (ctx, font, unichar);

#if CTX_EVENTS
  if (ctx && ctx_backend_type (ctx) == CTX_BACKEND_TERM && ctx_fabsf(3.0f - font_size) < 0.03f)
    return 2;
#endif

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
  int glyphA = ctx_glyph_stb_find (ctx, font, unicharA);
  int glyphB = ctx_glyph_stb_find (ctx, font, unicharB);
  return stbtt_GetGlyphKernAdvance (ttf_info, glyphA, glyphB) * scale;
}

static int
ctx_glyph_stb (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  int glyph = ctx_glyph_stb_find (ctx, font, unichar);
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

static inline int ctx_font_get_length (CtxFont *font)
{
   return font->ctx.data->data.u32[1];
}

#if CTX_FONT_ENGINE_CTX

static uint32_t
ctx_glyph_find_next (CtxFont *font, Ctx *ctx, int offset)
{
  int length = ctx_font_get_length (font);
  for (int i = offset; i < length; i++)
  {
    CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH)
    {
      return entry->data.u32[0];
    }
  }
  return 0;
}

static int ctx_glyph_find_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
#if CTX_GLYPH_CACHE
  uint32_t hash = ((((size_t)(font) * 23) ^ unichar) * 17) %
            (CTX_GLYPH_CACHE_SIZE);
  if (ctx)
  {
    if (ctx->glyph_index_cache[hash].font == font &&
        ctx->glyph_index_cache[hash].unichar == unichar)
          return ctx->glyph_index_cache[hash].offset;
  }
#endif
#if 0

  for (int i = 0; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH &&
        entry->data.u32[0] == unichar)
    {
#if CTX_GLYPH_CACHE
       if (ctx)
       {
         ctx->glyph_index_cache[hash].font    = font;
         ctx->glyph_index_cache[hash].unichar = unichar;
         ctx->glyph_index_cache[hash].offset  = i;
       }
#endif
       return i;
       // XXX this could be prone to insertion of valid header
       // data in included bitmaps.. is that an issue?
       //   
    }
  }
#else
  int start = 0;
  int end = ctx_font_get_length (font);
  int max_iter = 10;

  do {
    int middle = (start + end) / 2;
    uint32_t middle_glyph = ctx_glyph_find_next (font, ctx, middle);

    if (unichar  == middle_glyph)
    {
#if CTX_GLYPH_CACHE
       if (ctx)
       {
         ctx->glyph_index_cache[hash].font    = font;
         ctx->glyph_index_cache[hash].unichar = unichar;
         ctx->glyph_index_cache[hash].offset  = middle;
       }
#endif
       return middle;
    }
    else if (unichar < middle_glyph)
    {
       end = middle;
    } else
    {
       start = middle;
    }

    if (start == end)
      return -1;
  } while (max_iter -- > 0);
#endif
  return -1;
}


static float
ctx_glyph_kern_ctx (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  float font_size = ctx->state.gstate.font_size;
  int first_kern = ctx_glyph_find_ctx (font, ctx, unicharA);
  if (first_kern < 0) return 0.0;

#if CTX_EVENTS
  if (ctx_backend_type (ctx) == CTX_BACKEND_TERM && ctx_fabsf(3.0f - font_size) < 0.03f)
    return 0.0f;
#endif

  int length = ctx_font_get_length (font);
  for (int i = first_kern + 1; i < length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_KERNING_PAIR)
        {
          if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
            { return entry->data.s32[1] / 255.0f * font_size / CTX_BAKE_FONT_SIZE; }
        }
      if (entry->code == CTX_DEFINE_GLYPH)
        return 0.0;
    }
  return 0.0;
}

static float
ctx_glyph_width_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  float font_size = 1.0f;
  if (ctx)
  {
    CtxState *state = &ctx->state;
    font_size = state->gstate.font_size;
  }
  int   start     = ctx_glyph_find_ctx (font, ctx, unichar);
  if (start < 0)
    { return 0.0; }  // XXX : fallback

#if CTX_EVENTS
  if (ctx && ctx_backend_type (ctx) == CTX_BACKEND_TERM && 
                  ctx_fabsf(3.0f - font_size) < 0.03f 
                  )
    return 2.0f;
#endif

  int length = ctx_font_get_length (font);
  for (int i = start; i < length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        if (entry->data.u32[0] == (unsigned) unichar)
          { return (entry->data.u32[1] / 255.0f * font_size / CTX_BAKE_FONT_SIZE); }
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
#if CTX_ONE_FONT_ENGINE==0
  if (font->type == 0)
#endif
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
      if (ctx->backend && ((CtxRasterizer*)(ctx->backend))->in_shadow)
      {
        ctx_rasterizer_shadow_fill ((CtxRasterizer*)ctx->backend);
        ((CtxRasterizer*)(ctx->backend))->in_shadow = 1;
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
      if (ctx->backend && ((CtxRasterizer*)(ctx->backend))->in_shadow)
      {
        ctx_rasterizer_shadow_fill ((CtxRasterizer*)ctx->backend);
        ((CtxRasterizer*)(ctx->backend))->in_shadow = 1;
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
  CtxDrawlist drawlist;
  drawlist.entries = font->ctx.data;
  int length = ctx_font_get_length (font);
  drawlist.count = length;
  drawlist.size  = length;
  drawlist.flags = CTX_DRAWLIST_DOESNT_OWN_ENTRIES;
  return ctx_glyph_drawlist (font, ctx, &drawlist, unichar, stroke);
}

#if 0
uint32_t ctx_glyph_no (Ctx *ctx, int no)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  if (no < 0 || no >= font->ctx.glyphs)
    { return 0; }
  return font->ctx.index[no*2]; // needs index
}
#endif

static void ctx_font_init_ctx (CtxFont *font)
{
}

int
ctx_load_font_ctx (const char *name, const void *data, int length);
#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path);
#endif

#if CTX_ONE_FONT_ENGINE==0
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
#endif

int
ctx_load_font_ctx (const char *name, const void *data, int length)
{
  ctx_font_setup (NULL);
  if (length % sizeof (CtxEntry) )
    { return -1; }
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }

#if CTX_ONE_FONT_ENGINE==0
  ctx_fonts[ctx_font_count].type = 0;
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_ctx;
#endif
  //ctx_fonts[ctx_font_count].name = name;
  ctx_fonts[ctx_font_count].ctx.data = (CtxEntry *) data;
  //ctx_fonts[ctx_font_count].ctx.length = length / sizeof (CtxEntry);
  ctx_font_init_ctx (&ctx_fonts[ctx_font_count]);

  ctx_font_count++;

#if CTX_ONE_FONT_ENGINE==0
  CtxFont *font = &ctx_fonts[ctx_font_count-1];
  if (font->engine->glyph_width (font, NULL, 'O') ==
      font->engine->glyph_width (font, NULL, 'I'))
  {
    font->monospaced = 1;
  }
  else
    font->monospaced = 0;
#endif

  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path)
{
  ctx_font_setup (NULL);
  uint8_t *contents = NULL;
  long length = 0;
  ctx_get_contents (path, &contents, &length);
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
            { return entry->data.s32[1] / 255.0f * font_size / CTX_BAKE_FONT_SIZE; }
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
  ctx_get_contents (path, &data, &len_bytes);
  float ret = 0.0;
  float font_size = state->gstate.font_size;
  if (data){
    Ctx *glyph_ctx = ctx_new ();
    ctx_parse (glyph_ctx, data);
    for (int i = 0; i < glyph_ctx->drawlist.count; i++)
    {
      CtxEntry *e = &glyph_ctx->drawlist.entries[i];
      if (e->code == CTX_DEFINE_GLYPH)
        ret = e->data.u32[1] / 255.0f * font_size / CTX_BAKE_FONT_SIZE;
    }
    ctx_free (data);
    ctx_destroy (glyph_ctx);
  }
  return ret;
}

static int
ctx_glyph_ctx_fs (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  char path[1024];
  sprintf (path, "file://%s/%010p", font->ctx_fs.path, unichar);
  uint8_t *data = NULL;
  long int len_bytes = 0;
  ctx_get_contents (path, &data, &len_bytes);

  if (data){
    Ctx *glyph_ctx = ctx_new ();
    ctx_parse (glyph_ctx, data);
    int ret = ctx_glyph_drawlist (font, ctx, &(glyph_ctx->drawlist),
                                  unichar, stroke);
    ctx_free (data);
    ctx_destroy (glyph_ctx);
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
  ctx_font_setup (NULL);
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }

  ctx_fonts[ctx_font_count].type = 42;
  ctx_fonts[ctx_font_count].ctx_fs.name = name;
  ctx_fonts[ctx_font_count].ctx_fs.path = ctx_strdup (path);
  int path_len = ctx_strlen (path);
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
#if CTX_ONE_FONT_ENGINE
  return ctx_glyph_ctx (font, ctx, unichar, stroke);
#else
  return font->engine->glyph (font, ctx, unichar, stroke);
#endif

}

int
ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
#if CTX_BACKEND_TEXT
  CtxEntry commands[3]; // 3 to silence incorrect warning from static analysis
  ctx_memset (commands, 0, sizeof (commands) );
  if (stroke)
    unichar = unichar | (1<<31);
  commands[0] = ctx_u32 (CTX_GLYPH, unichar, 0);
  //commands[1].data.u8[4] = stroke;
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
#if CTX_ONE_FONT_ENGINE
  return ctx_glyph_width_ctx (font, ctx, unichar);
#else
  return font->engine->glyph_width (font, ctx, unichar);
#endif
}

static float
ctx_glyph_kern (Ctx *ctx, int unicharA, int unicharB)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
#if CTX_ONE_FONT_ENGINE
  return ctx_glyph_kern_ctx (font, ctx, unicharA, unicharB);
#else
  return font->engine->glyph_kern (font, ctx, unicharA, unicharB);
#endif
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


#define CTX_MAX_WORD_LEN 128

#if 1
static int ctx_glyph_find (Ctx *ctx, CtxFont *font, uint32_t unichar)
{
  int length = ctx_font_get_length (font);
  for (int i = 0; i < length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
        { return i; }
    }
  return 0;
}
#endif

static inline int
_ctx_text_substitute_ligatures (Ctx *ctx, CtxFont *font,
                                uint32_t *unichar, uint32_t next_unichar)
{
  if (ctx_font_is_monospaced (font))
    return 0;
  if (*unichar == 'f')
    switch (next_unichar)
    {
      case 'f': if (ctx_glyph_find (ctx, font, 0xfb00))
        {
          *unichar = 0xfb00;
          return 1;
        }
        break;
      case 'i':
        if (ctx_glyph_find (ctx, font, 0xfb01))
        {
          *unichar = 0xfb01;
          return 1;
        }
        break;
      case 'l': 
        if (ctx_glyph_find (ctx, font, 0xfb02))
        {
          *unichar = 0xfb02;
          return 1;
        }
        break;
      case 't': 
        if (ctx_glyph_find (ctx, font, 0xfb05))
        {
          *unichar = 0xfb05;
          return 1;
        }
        break;
    }
  return 0;
}

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible)
{
  char word[CTX_MAX_WORD_LEN];
  int word_len = 0;
  CtxState *state = &ctx->state;
  CtxFont *font = &ctx_fonts[state->gstate.font];
  float x = ctx->state.x;
  word[word_len]=0;
  switch ( (int) ctx_state_get (state, CTX_textAlign) )
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
  switch ( (int) ctx_state_get (state, CTX_textBaseline) )
    {
      case CTX_TEXT_BASELINE_HANGING:
        /* XXX : crude */
        baseline_offset = ctx->state.gstate.font_size  * 0.55f;
        break;
      case CTX_TEXT_BASELINE_TOP:
        /* XXX : crude */
        baseline_offset = ctx->state.gstate.font_size  * 0.7f;
        break;
      case CTX_TEXT_BASELINE_BOTTOM:
        baseline_offset = -ctx->state.gstate.font_size * 0.1f;
        break;
      case CTX_TEXT_BASELINE_ALPHABETIC:
      case CTX_TEXT_BASELINE_IDEOGRAPHIC:
        baseline_offset = 0.0f;
        break;
      case CTX_TEXT_BASELINE_MIDDLE:
        baseline_offset = ctx->state.gstate.font_size * 0.25f;
        break;
    }
  float x0 = x;
  float x1 = x + 10000.0f;
  
  float wrap_left = ctx_get_wrap_left (ctx);
  float wrap_right = ctx_get_wrap_right (ctx);
  if (wrap_left != wrap_right)
  {
    x0 = wrap_left;
  }

  if (*string)
  for (const char *utf8 = string; (utf8==string ) || utf8[-1]; utf8 = *utf8?ctx_utf8_skip (utf8, 1):(utf8+1))
    {
      if (*utf8 == '\n' ||
          *utf8 == ' ' ||
          *utf8 == '\0')
        {
          float word_width = 0.0;
          word[word_len]=0;

          for (const char *bp = &word[0]; *bp; bp = ctx_utf8_skip (bp, 1))
          {
            uint32_t unichar      = ctx_utf8_to_unichar (bp);
            const char *next_utf8 = ctx_utf8_skip (bp, 1);
            uint32_t next_unichar = *next_utf8?ctx_utf8_to_unichar (next_utf8):0;

            if (_ctx_text_substitute_ligatures (ctx, font, &unichar, next_unichar))
              bp++;

            float glyph_width     = ctx_glyph_width (ctx, unichar);
            word_width += glyph_width;
            if (next_unichar)
              word_width += ctx_glyph_kern (ctx, unichar, next_unichar);
          }

          if (wrap_left != wrap_right &&
              x + word_width >= wrap_right)
          {
            y += ctx->state.gstate.font_size * ctx_get_line_height (ctx);
            x = x0;
          }

          for (const char *bp = &word[0]; *bp; bp = ctx_utf8_skip (bp, 1))
          {
            uint32_t unichar      = ctx_utf8_to_unichar (bp);
            const char *next_utf8 = ctx_utf8_skip (bp, 1);
            uint32_t next_unichar = *next_utf8?ctx_utf8_to_unichar (next_utf8):0;

            if (_ctx_text_substitute_ligatures (ctx, font, &unichar, next_unichar))
              bp++;

            float glyph_width     = ctx_glyph_width (ctx, unichar);
            if (x + glyph_width >= x1)
            {
              y += ctx->state.gstate.font_size * ctx_get_line_height (ctx);
              x = x0;
            }
            if (visible)
            {
              ctx_move_to (ctx, x, y + baseline_offset);
              _ctx_glyph (ctx, unichar, stroke);
            }
            x += glyph_width;
            if (next_unichar)
              x += ctx_glyph_kern (ctx, unichar, next_unichar );
          }

          if (*utf8 == '\n')
          {
            y += ctx->state.gstate.font_size * ctx_get_line_height (ctx);
            x = x0;
          }
          else if (*utf8 == ' ')
          {
            x += ctx_glyph_width (ctx, ' ');
          }
          word_len=0;
          word[word_len]=0;
        }
      else
      {
        if (word_len + 1 < CTX_MAX_WORD_LEN-1)
          word[word_len++]=*utf8;
      }

    }
  if (!visible)
    { ctx_move_to (ctx, x, y); }
}


CtxGlyph *
ctx_glyph_allocate (int n_glyphs)
{
  return (CtxGlyph *) ctx_malloc (sizeof (CtxGlyph) * n_glyphs);
}
void
gtx_glyph_free     (CtxGlyph *glyphs)
{
  ctx_free (glyphs);
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
ctx_text (Ctx        *ctx,
          const char *string)
{
  if (!string)
    return;
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_TEXT, string, 0, 0);
  _ctx_text (ctx, string, 0, 0);
#else
  _ctx_text (ctx, string, 0, 1);
#endif
  ctx_begin_path (ctx); // XXX : this should be handled internally
}


void
ctx_fill_text (Ctx *ctx, const char *string,
               float x, float y)
{
  ctx_move_to (ctx, x, y);
  ctx_text (ctx, string);
}

void
ctx_text_stroke (Ctx        *ctx,
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

void
ctx_stroke_text (Ctx *ctx, const char *string,
               float x, float y)
{
  ctx_move_to (ctx, x, y);
  ctx_text_stroke (ctx, string);
}

static const char *ctx_font_get_name (CtxFont *font)
{
#if CTX_ONE_FONT_ENGINE
    return ((char*)(font->ctx.data+2))+1;
#else
  switch (font->type)
  {
    case 0:  return ((char*)(font->ctx.data+2))+1;
#if CTX_FONT_ENGINE_STB
    case 1:  return font->stb.name;
    case 2:  return font->stb.name;
#endif
  }
  return "-";
#endif
}


static int _ctx_resolve_font (const char *name)
{
  char temp[ctx_strlen (name)+1];
  /* first we look for exact */
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (!ctx_strcmp (ctx_font_get_name (&ctx_fonts[i]), name) )
        { return i; }
    }
  /* ... and substring matches for passed in string */
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (ctx_strstr (ctx_font_get_name (&ctx_fonts[i]), name) )
        { return i; }
    }

  /* then we normalize some names */
  if (!strncmp (name, "Helvetica", 9))
  {
     memset(temp,0,sizeof(temp));
     strncpy (temp, name + 4, sizeof(temp)-1);
     memcpy (temp, "Arrrr", 5); 
     name = temp;
  }
  else if (!strncmp (name, "Monospace", 9))
  {
     memset(temp,0,sizeof(temp));
     strncpy (temp, name + 2, sizeof(temp)-1);
     memcpy (temp, "Courier", 7); 
     name = temp;
  }
  else if (!strncmp (name, "Mono ", 5))
  {
    memset(temp,0,sizeof(temp));
    strncpy (temp + 3, name, sizeof(temp)-1-3);
    memcpy (temp, "Courier ", 8); 
    name = temp;
  }
  else if (!strcmp (name, "Mono"))
  {
    name = "Courier";
  }

  /* and attempt substring matching with mangled named
   * permitting matches with length and two first chars
   * to be valid
   */
  {
    char *subname = (char*)name;
    int namelen = 0; 
    if (strchr (subname, ' '))
    {
      subname = strchr (subname, ' ');
      namelen = subname - name;
      subname++;
    }
    for (int i = 0; i < ctx_font_count; i ++)
    {
      const char *font_name = ctx_font_get_name (&ctx_fonts[i]);
      if (font_name[0]==name[0] &&
          font_name[1]==name[1] &&
          font_name[namelen] == name[namelen] &&
          (namelen == 0 || ctx_strstr (font_name, subname) ))
        return i;
    }
  }

  /* then we look for a match of the substring after the first
   * space
   */
  if (strchr (name, ' '))
  {
     char *subname = strchr (name, ' ');
     for (int i = 0; i < ctx_font_count; i ++)
     {
       const char *font_name = ctx_font_get_name (&ctx_fonts[i]);
       if (ctx_strstr (font_name, subname) )
         { return i; }
     }
  }

  return -1;
}

const char *ctx_get_font_name (Ctx *ctx, int no)
{
  if (no >= 0 && no < ctx_font_count)
    return ctx_font_get_name (&ctx_fonts[no]);
  return NULL;
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



#if !( defined(CTX_FONT_0) ||\
       defined(CTX_FONT_1) ||\
       defined(CTX_FONT_2) ||\
       defined(CTX_FONT_3) ||\
       defined(CTX_FONT_4) ||\
       defined(CTX_FONT_5) ||\
       defined(CTX_FONT_6) ||\
       defined(CTX_FONT_7) ||\
       defined(CTX_FONT_8) ||\
       defined(CTX_FONT_9) ||\
       defined(CTX_FONT_10) ||\
       defined(CTX_FONT_11) ||\
       defined(CTX_FONT_12) ||\
       defined(CTX_FONT_13) ||\
       defined(CTX_FONT_14) ||\
       defined(CTX_FONT_15) ||\
       defined(CTX_FONT_16) ||\
       defined(CTX_FONT_17) ||\
       defined(CTX_FONT_18) ||\
       defined(CTX_FONT_19) ||\
       defined(CTX_FONT_20) ||\
       defined(CTX_FONT_21))
#define CTX_STATIC_FONT(font_string, font_data) \
  ctx_load_font_ctx(font_string, ctx_font_##font_data, sizeof (ctx_font_##font_data))
#define CTX_FONT_0 CTX_STATIC_FONT("sans-ctx", ascii)
#endif

static void ctx_font_setup (Ctx *ctx)
{
  static int initialized = 0;
  if (initialized) { 
    if (ctx)
      ctx->fonts = ctx_fonts;
    return;
  }
  initialized = 1;

  //if (!ctx_fonts)
#ifdef EMSCRIPTEN
    //ctx_fonts = calloc (CTX_MAX_FONTS, sizeof (CtxFont));
#else
    //ctx_fonts = ctx_calloc (CTX_MAX_FONTS, sizeof (CtxFont));
#endif
  if (ctx)
    ctx->fonts = &ctx_fonts[0];

  ctx_font_count = 0; // oddly - this is needed in arduino

#if CTX_FONT_ENGINE_CTX
#ifdef CTX_FONT_0
  CTX_FONT_0;
#endif
#ifdef CTX_FONT_1
  CTX_FONT_1;
#endif
#ifdef CTX_FONT_2
  CTX_FONT_2;
#endif
#ifdef CTX_FONT_3
  CTX_FONT_3;
#endif
#ifdef CTX_FONT_4
  CTX_FONT_4;
#endif
#ifdef CTX_FONT_5
  CTX_FONT_5;
#endif
#ifdef CTX_FONT_6
  CTX_FONT_6;
#endif
#ifdef CTX_FONT_7
  CTX_FONT_7;
#endif
#ifdef CTX_FONT_8
  CTX_FONT_8;
#endif
#ifdef CTX_FONT_9
  CTX_FONT_9;
#endif
#ifdef CTX_FONT_10
  CTX_FONT_10;
#endif
#ifdef CTX_FONT_11
  CTX_FONT_11;
#endif
#ifdef CTX_FONT_12
  CTX_FONT_12;
#endif
#ifdef CTX_FONT_13
  CTX_FONT_13;
#endif
#ifdef CTX_FONT_14
  CTX_FONT_14;
#endif
#ifdef CTX_FONT_15
  CTX_FONT_15;
#endif
#ifdef CTX_FONT_16
  CTX_FONT_16;
#endif
#ifdef CTX_FONT_17
  CTX_FONT_17;
#endif
#ifdef CTX_FONT_18
  CTX_FONT_18;
#endif
#ifdef CTX_FONT_19
  CTX_FONT_19;
#endif
#ifdef CTX_FONT_20
  CTX_FONT_20;
#endif
#ifdef CTX_FONT_21
  CTX_FONT_21;
#endif
#ifdef CTX_FONT_22
  CTX_FONT_22;
#endif
#ifdef CTX_FONT_23
  CTX_FONT_23;
#endif
#ifdef CTX_FONT_24
  CTX_FONT_24;
#endif
#ifdef CTX_FONT_25
  CTX_FONT_25;
#endif
#ifdef CTX_FONT_26
  CTX_FONT_26;
#endif
#ifdef ctx_font_27
  ctx_font_27;
#endif
#ifdef ctx_font_28
  ctx_font_28;
#endif
#ifdef CTX_FONT_29
  CTX_FONT_29;
#endif
#ifdef CTX_FONT_30
  CTX_FONT_30;
#endif
#ifdef CTX_FONT_31
  CTX_FONT_31;
#endif
#endif
}


