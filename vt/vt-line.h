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

#ifndef MRG_STRING_H
#define MRG_STRING_H

#include "vt-utf8.h"

typedef struct _VtString VtString;

struct _VtString
{
  char *str;
  int   length;
  int   utf8_length;
  int   allocated_length;

  uint64_t *style;
  int       style_size;

  void     *ctx; // each line can have an attached ctx context;
                 // clearing could be brutal enough to unset the context of the current
  int       double_width;
  int       double_height_top;
  int       double_height_bottom;
}  __attribute((packed));


static inline uint64_t vt_string_get_style (VtString *string, int pos)
{
  if (pos < 0 || pos >= string->style_size)
    return 0;
  return string->style[pos];
}

#include <stdlib.h>

static inline void vt_string_set_style (VtString *string, int pos, uint64_t style)
{
  if (pos < 0 || pos >= 512)
    return;

  if (pos >= string->style_size)
  {
    int new_size = pos + 16;
    string->style = realloc (string->style, new_size * sizeof (uint64_t));
    memset (&string->style[string->style_size], 0, (new_size - string->style_size) * sizeof (uint64_t));
    string->style_size = new_size;
  }
  string->style[pos] = style;
}


VtString   *vt_string_new_with_size  (const char *initial, int initial_size);
VtString   *vt_string_new            (const char *initial);
void        vt_string_free           (VtString *string, int freealloc);
const char *vt_string_get            (VtString *string);
int         vt_string_get_length     (VtString *string);
void        vt_string_set            (VtString *string, const char *new_string);
void        vt_string_clear          (VtString *string);
void        vt_string_append_str     (VtString *string, const char *str);
void        vt_string_append_byte    (VtString *string, char  val);
void        vt_string_append_string  (VtString *string, VtString *string2);
void        vt_string_append_unichar (VtString *string, unsigned int unichar);
void        vt_string_append_data    (VtString *string, const char *data, int len);
void        vt_string_append_printf  (VtString *string, const char *format, ...);
void        vt_string_replace_utf8   (VtString *string, int pos, const char *new_glyph);
void        vt_string_insert_utf8    (VtString *string, int pos, const char *new_glyph);
void        vt_string_remove_utf8    (VtString *string, int pos);


#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
