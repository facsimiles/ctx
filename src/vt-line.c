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


#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#if !__COSMOPOLITAN__
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

int ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
#define mrg_unichar_to_utf8 ctx_unichar_to_utf8
void ctx_string_init (CtxString *string, int initial_size);

VtLine *vt_line_new_with_size (const char *initial, int initial_size)
{
  VtLine *line = ctx_calloc (sizeof (VtLine), 1);
  CtxString *string = (CtxString*)line;
  ctx_string_init (string, initial_size);
  if (initial)
    { ctx_string_append_str (string, initial); }
  line->style = ctx_calloc (sizeof (uint64_t), initial_size);
  line->style_size = initial_size;
  string->is_line = 1;
  return line;
}

VtLine *vt_line_new (const char *initial)
{
  return vt_line_new_with_size (initial, 8);
}
