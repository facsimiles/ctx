#include "ctx-split.h"

int ctx_utf8_len (const unsigned char first_byte)
{
  if      ( (first_byte & 0x80) == 0)
    { return 1; } /* ASCII */
  else if ( (first_byte & 0xE0) == 0xC0)
    { return 2; }
  else if ( (first_byte & 0xF0) == 0xE0)
    { return 3; }
  else if ( (first_byte & 0xF8) == 0xF0)
    { return 4; }
  return 1;
}


const char *ctx_utf8_skip (const char *s, int utf8_length)
{
  int count;
  if (!s)
    { return NULL; }
  for (count = 0; *s; s++)
    {
      if ( (*s & 0xC0) != 0x80)
        { count++; }
      if (count == utf8_length + 1)
        { return s; }
    }
  return s;
}

//  XXX  :  unused
int ctx_utf8_strlen (const char *s)
{
  int count;
  if (!s)
    { return 0; }
  for (count = 0; *s; s++)
    if ( (*s & 0xC0) != 0x80)
      { count++; }
  return count;
}

int
ctx_unichar_to_utf8 (uint32_t  ch,
                     uint8_t  *dest)
{
  /* http://www.cprogramming.com/tutorial/utf8.c  */
  /*  Basic UTF-8 manipulation routines
    by Jeff Bezanson
    placed in the public domain Fall 2005 ... */
  if (ch < 0x80)
    {
      dest[0] = (char) ch;
      return 1;
    }
  if (ch < 0x800)
    {
      dest[0] = (ch>>6) | 0xC0;
      dest[1] = (ch & 0x3F) | 0x80;
      return 2;
    }
  if (ch < 0x10000)
    {
      dest[0] = (ch>>12) | 0xE0;
      dest[1] = ( (ch>>6) & 0x3F) | 0x80;
      dest[2] = (ch & 0x3F) | 0x80;
      return 3;
    }
  if (ch < 0x110000)
    {
      dest[0] = (ch>>18) | 0xF0;
      dest[1] = ( (ch>>12) & 0x3F) | 0x80;
      dest[2] = ( (ch>>6) & 0x3F) | 0x80;
      dest[3] = (ch & 0x3F) | 0x80;
      return 4;
    }
  return 0;
}

uint32_t
ctx_utf8_to_unichar (const char *input)
{
  const uint8_t *utf8 = (const uint8_t *) input;
  uint8_t c = utf8[0];
  if ( (c & 0x80) == 0)
    { return c; }
  else if ( (c & 0xE0) == 0xC0)
    return ( (utf8[0] & 0x1F) << 6) |
           (utf8[1] & 0x3F);
  else if ( (c & 0xF0) == 0xE0)
    return ( (utf8[0] & 0xF)  << 12) |
           ( (utf8[1] & 0x3F) << 6) |
           (utf8[2] & 0x3F);
  else if ( (c & 0xF8) == 0xF0)
    return ( (utf8[0] & 0x7)  << 18) |
           ( (utf8[1] & 0x3F) << 12) |
           ( (utf8[2] & 0x3F) << 6) |
           (utf8[3] & 0x3F);
  else if ( (c & 0xFC) == 0xF8)
    return ( (utf8[0] & 0x3)  << 24) |
           ( (utf8[1] & 0x3F) << 18) |
           ( (utf8[2] & 0x3F) << 12) |
           ( (utf8[3] & 0x3F) << 6) |
           (utf8[4] & 0x3F);
  else if ( (c & 0xFE) == 0xFC)
    return ( (utf8[0] & 0x1)  << 30) |
           ( (utf8[1] & 0x3F) << 24) |
           ( (utf8[2] & 0x3F) << 18) |
           ( (utf8[3] & 0x3F) << 12) |
           ( (utf8[4] & 0x3F) << 6) |
           (utf8[5] & 0x3F);
  return 0;
}
