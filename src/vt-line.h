#if CTX_VT

/* mrg - MicroRaptor Gui
 * Copyright (c) 2014 Øyvind Kolås <pippin@hodefoting.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VT_LINE_H
#define VT_LINE_H

#include "ctx.h"

#ifndef CTX_UNLIKELY
#define CTX_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#define CTX_LIKELY(x)      __builtin_expect(!!(x), 1)
#endif
#ifndef CTX_MAX
#define CTX_MAX(a,b) (((a)>(b))?(a):(b))
#endif

typedef struct _VtLine   VtLine;

#if CTX_VT_STYLE_SIZE==32
typedef uint32_t vt_style_t;
#else
typedef uint64_t vt_style_t;
#endif

struct _VtLine
{
  CtxString string;
  /* line extends string, permitting string ops to operate on it  */

  vt_style_t *style;

  void     *ctx; // each line can have an attached ctx context;
  char     *prev;
  int       style_size;
  int       prev_length;
  CtxString *frame;


  void     *ctx_copy; // each line can have an attached ctx context;
  // clearing should be brutal enough to unset the context of the current
  // at least in alt-screen mode
  int       double_width;
  int       double_height_top;
  int       double_height_bottom;
  int       contains_proportional;
  float     xscale;
  float     yscale;
  float     y_offset;
  int       in_scrolling_region;
  int       wrapped;

  /*  XXX:  needs refactoring to a CtxList of links/images */
  void     *images[4];
  int       image_col[4];
  float     image_X[4]; // 0.0 - 1.0 offset in cell
  float     image_Y[4];
  int       image_rows[4];
  int       image_cols[4];
  int       image_subx[4];
  int       image_suby[4];
  int       image_subw[4];
  int       image_subh[4];
};


static inline uint64_t vt_line_get_style (VtLine *string, int pos)
{
  if (string->string.is_line==0)
    return 0;
  if (pos < 0 || pos >= string->style_size)
    return 0;
  return string->style[pos];
}

#if !__COSMOPOLITAN__
#include <stdlib.h>
#endif

static inline void vt_line_set_style (VtLine *string, int pos, uint64_t style)
{
  if (string->string.is_line==0)
    return;
  if (pos < 0 || pos >= 512)
    return;
  if (pos >= string->style_size)
    {
      int new_size = pos + 8;
      string->style = ctx_realloc (string->style, string->style_size, new_size * sizeof (uint64_t) );
      memset (&string->style[string->style_size], 0, (new_size - string->style_size) * sizeof (uint64_t) );
      string->style_size = new_size;
    }
  string->style[pos] = style;
}
static inline void vt_line_clear_style (VtLine *string)
{
  if (string->string.is_line==0)
    return;
  if (string->style)
  {
    memset (string->style, 0, string->style_size * sizeof (uint64_t) );
  }
}

VtLine *vt_line_new_with_size (const char *initial, int initial_size);
VtLine *vt_line_new (const char *initial);

static inline void        vt_line_free           (VtLine *line, int freealloc)
{
  CtxString *string = (CtxString*)line;

#if 1
  //if (string->is_line)
  {
    VtLine *line = (VtLine*)string;
    if (line->frame)
      ctx_string_free (line->frame, 1);
    if (line->style)
      { ctx_free (line->style); }
    if (line->ctx)
      { ctx_destroy (line->ctx); }
    if (line->ctx_copy)
      { ctx_destroy (line->ctx_copy); }
  }
#endif

  ctx_string_free (string, freealloc);
}
static inline const char *vt_line_get            (VtLine *line)
{
  CtxString *string = (CtxString*)line;
  return ctx_string_get (string);
}
static inline uint32_t    vt_line_get_unichar    (VtLine *line, int pos)
{
  CtxString *string = (CtxString*)line;
  return ctx_string_get_unichar (string, pos);
}
static inline int         vt_line_get_length     (VtLine *line)
{
  CtxString *string = (CtxString*)line;
  return ctx_string_get_length (string);
}
static inline int         vt_line_get_utf8length     (VtLine *line)
{
  CtxString *string = (CtxString*)line;
  return ctx_string_get_utf8length (string);
}
static inline void        vt_line_set            (VtLine *line, const char *new_string)
{
  CtxString *string = (CtxString*)line;
  ctx_string_set (string, new_string);
}
static inline void        vt_line_clear          (VtLine *line)
{
  CtxString *string = (CtxString*)line;
  ctx_string_clear (string);
  vt_line_clear_style ((VtLine*)string);
}
static inline void        vt_line_append_str     (VtLine *line, const char *str)
{
  CtxString *string = (CtxString*)line;
  ctx_string_append_str (string, str);
}

#if 0
static inline void _ctx_string_append_byte (CtxString *string, char  val)
{
  if (CTX_LIKELY((val & 0xC0) != 0x80))
    { string->utf8_length++; }
  if (CTX_UNLIKELY(string->length + 2 >= string->allocated_length))
    {
      char *old = string->str;
      string->allocated_length = CTX_MAX (string->allocated_length * 2, string->length + 2);
      string->str = (char*)ctx_realloc (old, string->allocated_length);
    }
  string->str[string->length++] = val;
  string->str[string->length] = '\0';
}
#endif

static inline void        vt_line_append_byte    (VtLine *line, char  val)
{
  CtxString *string = (CtxString*)line;
  _ctx_string_append_byte (string, val);
}
static inline void        vt_line_append_string  (VtLine *line, CtxString *string2)
{
  CtxString *string = (CtxString*)line;
  ctx_string_append_string (string, string2);
}
static inline void        vt_line_append_unichar (VtLine *line, unsigned int unichar)
{
  CtxString *string = (CtxString*)line;
  ctx_string_append_unichar (string, unichar);
}


static inline void vt_line_append_data    (VtLine *line, const char *data, int len)
{
  CtxString *string = (CtxString*)line;
  ctx_string_append_data (string, data, len);
}
static inline void vt_line_append_utf8char (VtLine *line, const char *str)
{
  CtxString *string = (CtxString*)line;
  ctx_string_append_utf8char (string, str);
}
static inline void vt_line_replace_utf8   (VtLine *line, int pos, const char *new_glyph)
{
  CtxString *string = (CtxString*)line;
  ctx_string_replace_utf8 (string, pos, new_glyph);
}


static inline void vt_line_insert_utf8    (VtLine *line, int pos, const char *new_glyph)
{
  CtxString *string = (CtxString*)line;
  ctx_string_insert_utf8 (string, pos, new_glyph);
  int len = vt_line_get_length (line);
  for (int i = pos; i < len; i++)
    vt_line_set_style (line, i, vt_line_get_style (line, i-1));
}

static inline void vt_line_insert_unichar (VtLine *line, int pos, uint32_t new_glyph)
{
  CtxString *string = (CtxString*)line;
  ctx_string_insert_unichar (string, pos, new_glyph);
  int len = vt_line_get_length (line);
  for (int i = 1; i < len; i++)
    vt_line_set_style (line, i, vt_line_get_style (line, i-1));
}
static inline void vt_line_replace_unichar (VtLine *line, int pos, uint32_t unichar)
{
  CtxString *string = (CtxString*)line;
  ctx_string_replace_unichar (string, pos, unichar);
}

static inline void vt_line_remove (VtLine *line, int pos)
{ 
  CtxString *string = (CtxString*)line;
  ctx_string_remove (string, pos);

  for (int i = pos; i < line->style_size-1; i++)
  {
    line->style[i] = line->style[i+1];
  }
}


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
