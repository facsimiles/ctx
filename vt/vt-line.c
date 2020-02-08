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

#include "vt-line.h"

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
#define mrg_unichar_to_utf8 ctx_unichar_to_utf8

static void vt_string_init (VtString *string, int initial_size)
{
  string->allocated_length = initial_size;
  string->length = 0;
  string->utf8_length = 0;
  string->str = malloc (string->allocated_length + 1);
  string->str[0]='\0';
}

static void vt_string_destroy (VtString *string)
{
  if (string->str)
  {
    free (string->str);
    string->str = NULL;
  }
}

void vt_string_clear (VtString *string)
{
  string->length = 0;
  string->utf8_length = 0;
  string->str[string->length]=0;
}

static inline void _vt_string_append_byte (VtString *string, char  val)
{
  if ((val & 0xC0) != 0x80)
    string->utf8_length++;
  if (string->length + 1 >= string->allocated_length)
    {
      char *old = string->str;
      string->allocated_length *= 2;
      string->str = malloc (string->allocated_length);
      memcpy (string->str, old, string->allocated_length/2);
      free (old);
    }
  string->str[string->length++] = val;
  string->str[string->length] = '\0';
}

void vt_string_append_byte (VtString *string, char  val)
{
  _vt_string_append_byte (string, val);
}

void vt_string_append_unichar (VtString *string, unsigned int unichar)
{
  char *str;
  char utf8[5];
  utf8[mrg_unichar_to_utf8 (unichar, (unsigned char*)utf8)]=0;
  str = utf8;

  while (str && *str)
    {
      _vt_string_append_byte (string, *str);
      str++;
    }
}

static inline void _vt_string_append_str (VtString *string, const char *str)
{
  if (!str) return;
  while (*str)
    {
      _vt_string_append_byte (string, *str);
      str++;
    }
}
void vt_string_append_str (VtString *string, const char *str)
{
  _vt_string_append_str (string, str);
}

VtString *vt_string_new_with_size (const char *initial, int initial_size)
{
  VtString *string = calloc (sizeof (VtString), 1);
  vt_string_init (string, initial_size);
  if (initial)
    _vt_string_append_str (string, initial);

  string->style = calloc (sizeof(uint64_t), initial_size * 2);
  string->style_size = initial_size * 2;

  return string;
}

VtString *vt_string_new (const char *initial)
{
  return vt_string_new_with_size (initial, 8);
}

void vt_string_append_data (VtString *string, const char *str, int len)
{
  int i;
  for (i = 0; i<len; i++)
    _vt_string_append_byte (string, str[i]);
}

void vt_string_append_string (VtString *string, VtString *string2)
{
  const char *str = vt_string_get (string2);
  while (str && *str)
    {
      _vt_string_append_byte (string, *str);
      str++;
    }
}

const char *vt_string_get (VtString *string)
{
  return string->str;
}

int vt_string_get_length (VtString *string)
{
  return string->length;
}

void
vt_string_free (VtString *string, int freealloc)
{
  if (freealloc)
    {
      vt_string_destroy (string);
    }
  if (string->style)
    free (string->style);
  free (string);
}

void
vt_string_set (VtString *string, const char *new_string)
{
  vt_string_clear (string);
  _vt_string_append_str (string, new_string);
}

void vt_string_replace_utf8 (VtString *string, int pos, const char *new_glyph)
{
  int new_len = mrg_utf8_len (*new_glyph);
  int old_len = string->utf8_length;
  char tmpg[3]=" ";

  if (new_len <= 1 && new_glyph[0] < 32)
  {
    new_len = 1;
    tmpg[0]=new_glyph[0]+64;
    new_glyph = tmpg;
  }


  {
    for (int i = old_len; i <= pos; i++)
    {
      _vt_string_append_byte (string, ' ');
      old_len++;
    }
  }

#if 1
  if (pos == old_len)
  {
    _vt_string_append_str (string, new_glyph);
    return;
  }
#endif


  if (string->length + new_len >= string->allocated_length)
  {
    char *tmp;
    char *defer;
    string->allocated_length = string->length + new_len;
    tmp = calloc (string->allocated_length + 1, 1);
    strcpy (tmp, string->str);
    defer = string->str;
    string->str = tmp;
    free (defer);
  }

  char *p = (void*)mrg_utf8_skip (string->str, pos);
  int prev_len = mrg_utf8_len (*p);
  char *rest;
  if (*p == 0 || *(p+prev_len) == 0)
  {
    rest = strdup("");
  }
  else
  {
    if (p + prev_len >= string->length  + string->str)
      rest = strdup ("");
    else
      rest = strdup (p + prev_len);
  }

  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, strlen (rest) + 1);
  string->length += new_len;
  string->length -= prev_len;
  free (rest);

  string->utf8_length = mrg_utf8_strlen (string->str);
}

void vt_string_insert_utf8 (VtString *string, int pos, const char *new_glyph)
{
  int new_len = mrg_utf8_len (*new_glyph);
  int old_len = string->utf8_length;
  char tmpg[3]=" ";
  if (new_len <= 1 && new_glyph[0] < 32)
  {
    tmpg[0]=new_glyph[0]+64;
    new_glyph = tmpg;
  }

  if (old_len == pos)
  {
    vt_string_append_str (string, new_glyph);
    return;
  }

  {
    for (int i = old_len; i <= pos; i++)
    {
      _vt_string_append_byte (string, ' ');
      old_len++;
    }
  }

  if (string->length + new_len + 1  > string->allocated_length)
  {
    char *tmp;
    char *defer;
    string->allocated_length = string->length + new_len + 1;
    tmp = calloc (string->allocated_length + 1, 1);
    strcpy (tmp, string->str);
    defer = string->str;
    string->str = tmp;
    free (defer);
  }

  char *p = (void*)mrg_utf8_skip (string->str, pos);
  int prev_len = mrg_utf8_len (*p);
  char *rest;
  if (*p == 0 || *(p+prev_len) == 0)
  {
    rest = strdup("");
  }
  else
  {
    rest = strdup (p);
  }

  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, strlen (rest) + 1);
  string->length += new_len;
  free (rest);

  string->utf8_length = mrg_utf8_strlen (string->str);
}

void vt_string_remove_utf8 (VtString *string, int pos)
{
  int old_len = string->utf8_length;

  {
    for (int i = old_len; i <= pos; i++)
    {
      _vt_string_append_byte (string, ' ');
      old_len++;
    }
  }

  char *p = (void*)mrg_utf8_skip (string->str, pos);
  int prev_len = mrg_utf8_len (*p);
  char *rest;
  if (*p == 0 || *(p+prev_len) == 0)
  {
    rest = strdup("");
    prev_len = 0;
  }
  else
  {
    rest = strdup (p + prev_len);
  }

  strcpy (p, rest);
  string->str[string->length - prev_len] = 0;
  free (rest);

  string->length = strlen (string->str);
  string->utf8_length = mrg_utf8_strlen (string->str);
}

