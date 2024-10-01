
#if !__COSMOPOLITAN__
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

//#include "ctx.h"
/* instead of including ctx.h we declare the few utf8
 * functions we use
 */
uint32_t ctx_utf8_to_unichar (const char *input);
int ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
int ctx_utf8_strlen (const char *s);

static void ctx_string_init (CtxString *string, int initial_size)
{
  string->allocated_length = initial_size;
  string->length = 0;
  string->utf8_length = 0;
  string->str = (char*)ctx_malloc (string->allocated_length + 1);
  string->str[0]='\0';
}

static void ctx_string_destroy (CtxString *string)
{
  if (string->str)
    {
      ctx_free (string->str);
      string->str = NULL;
    }
}

void ctx_string_clear (CtxString *string)
{
  string->length = 0;
  string->utf8_length = 0;
  string->str[string->length]=0;
}


void ctx_string_pre_alloc (CtxString *string, int size)
{
  char *old = string->str;
  int old_len = string->allocated_length;
  string->allocated_length = CTX_MAX (size + 2, string->length + 2);
  string->str = (char*)ctx_realloc (old, old_len, string->allocated_length);
}


static inline void _ctx_string_append_byte (CtxString *string, char  val)
{
  if (CTX_LIKELY((val & 0xC0) != 0x80))
    { string->utf8_length++; }
  if (CTX_UNLIKELY(string->length + 2 >= string->allocated_length))
    {
      char *old = string->str;
      int old_len = string->allocated_length;
      string->allocated_length = CTX_MAX ((int)(string->allocated_length * 1.5f), string->length + 2);
      string->str = (char*)ctx_realloc (old, old_len, string->allocated_length);
    }
  string->str[string->length++] = val;
  string->str[string->length] = '\0';
}

void ctx_string_append_byte (CtxString *string, char  val)
{
  _ctx_string_append_byte (string, val);
}

void ctx_string_append_unichar (CtxString *string, unsigned int unichar)
{
  char *str;
  char utf8[5];
  utf8[ctx_unichar_to_utf8 (unichar, (unsigned char *) utf8)]=0;
  str = utf8;
  while (str && *str)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

static inline void _ctx_string_append_str (CtxString *string, const char *str)
{
  if (!str) { return; }
  while (*str)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

void ctx_string_append_utf8char (CtxString *string, const char *str)
{
  if (!str) { return; }
  int len = ctx_utf8_len (*str);
  for (int i = 0; i < len && *str; i++)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

void ctx_string_append_str (CtxString *string, const char *str)
{
  _ctx_string_append_str (string, str);
}

CtxString *ctx_string_new_with_size (const char *initial, int initial_size)
{
  CtxString *string = (CtxString*)ctx_calloc (1, sizeof (CtxString));
  ctx_string_init (string, initial_size);
  if (initial)
    { _ctx_string_append_str (string, initial); }
  return string;
}

CtxString *ctx_string_new (const char *initial)
{
  return ctx_string_new_with_size (initial, 8);
}

void ctx_string_append_data (CtxString *string, const char *str, int len)
{
  int i;
  for (i = 0; i<len; i++)
    { _ctx_string_append_byte (string, str[i]); }
}

void ctx_string_append_string (CtxString *string, CtxString *string2)
{
  const char *str = ctx_string_get (string2);
  while (str && *str)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

const char *ctx_string_get (CtxString *string)
{
  return string->str;
}

int ctx_string_get_utf8length (CtxString *string)
{
  return string->utf8_length;
}

int ctx_string_get_length (CtxString *string)
{
  return string->length;
}

void
ctx_string_free (CtxString *string, int freealloc)
{
  if (freealloc)
    {
      ctx_string_destroy (string);
    }
#if 0
  if (string->is_line)
  {
    VtLine *line = (VtLine*)string;
    if (line->style)
      { ctx_free (line->style); }
    if (line->ctx)
      { ctx_destroy (line->ctx); }
    if (line->ctx_copy)
      { ctx_destroy (line->ctx_copy); }
  }
#endif
  ctx_free (string);
}

char       *ctx_string_dissolve       (CtxString *string)
{
  char *ret = string->str;
  ctx_string_free (string, 0);
  return ret;
}

void
ctx_string_set (CtxString *string, const char *new_string)
{
  ctx_string_clear (string);
  _ctx_string_append_str (string, new_string);
}


void ctx_string_replace_utf8 (CtxString *string, int pos, const char *new_glyph)
{
#if 1
  int old_len = string->utf8_length;
#else
  int old_len = ctx_utf8_strlen (string->str);// string->utf8_length;
#endif
  if (pos < 0) return;

  if (CTX_LIKELY(pos == old_len))
    {
      _ctx_string_append_str (string, new_glyph);
      return;
    }

  char tmpg[3]=" ";
  int new_len = ctx_utf8_len (*new_glyph);
  if (new_len <= 1 && new_glyph[0] < 32)
    {
      new_len = 1;
      tmpg[0]=new_glyph[0]+64;
      new_glyph = tmpg;
    }
  {
    for (int i = old_len; i <= pos + 2; i++)
      {
        _ctx_string_append_byte (string, ' ');
        old_len++;
      }
  }
  if (string->length + new_len  >= string->allocated_length - 2)
    {
      char *tmp;
      char *defer;
      string->allocated_length = string->length + new_len + 10;
      tmp = (char*) ctx_calloc (1, string->allocated_length + 1 + 8);
      strcpy (tmp, string->str);
      defer = string->str;
      string->str = tmp;
      ctx_free (defer);
    }
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if (*p == 0 || * (p+prev_len) == 0)
    {
      rest = ctx_strdup ("");
    }
  else
    {
      if (p + prev_len >= string->length  + string->str)
        { rest = ctx_strdup (""); }
      else
        { rest = ctx_strdup (p + prev_len); }
    }
  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, ctx_strlen (rest) + 1);

#if 1
  string->length += new_len;
  string->length -= prev_len;
#else
  if (string->length + new_len - prev_len != strlen (string->str))
  {
      printf ("owww: pos:%i olen:%i new_len:%i prev_len:%i actual:%i computed:%i\n",
			  pos, string->length, new_len, prev_len, strlen(string->str),
      string->length + new_len - prev_len != strlen (string->str)
			  );
  }
  //  XXX : fixing these to be update correctly - would speed up terminals
  string->length = strlen (string->str);
  //string->utf8_length = ctx_utf8_strlen (string->str);
#endif
  ctx_free (rest);
}

void ctx_string_replace_unichar (CtxString *string, int pos, uint32_t unichar)
{
  uint8_t utf8[8];
  ctx_unichar_to_utf8 (unichar, utf8);
  ctx_string_replace_utf8 (string, pos, (char *) utf8);
}

uint32_t ctx_string_get_unichar (CtxString *string, int pos)
{
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  if (!p)
    { return 0; }
  return ctx_utf8_to_unichar (p);
}

void ctx_string_insert_utf8 (CtxString *string, int pos, const char *new_glyph)
{
  int new_len = ctx_utf8_len (*new_glyph);
  int old_len = string->utf8_length;
  char tmpg[3]=" ";
  if (pos < 0)
    return;
  if (old_len == pos && 0)
    {
      ctx_string_append_str (string, new_glyph);
      return;
    }
  if (new_len <= 1 && new_glyph[0] < 32)
    {
      tmpg[0]=new_glyph[0]+64;
      new_glyph = tmpg;
    }
  {
    for (int i = old_len; i <= pos; i++)
      {
        _ctx_string_append_byte (string, ' ');
        old_len++;
      }
  }
  if (string->length + new_len + 1  > string->allocated_length)
    {
      char *tmp;
      char *defer;
      string->allocated_length = string->length + new_len + 1;
      tmp = (char*) ctx_calloc (1, string->allocated_length + 1);
      strcpy (tmp, string->str);
      defer = string->str;
      string->str = tmp;
      ctx_free (defer);
    }
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if ( (*p == 0 || * (p+prev_len) == 0) && pos != 0)
    {
      rest = ctx_strdup ("");
    }
  else
    {
      rest = ctx_strdup (p);
    }
  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, ctx_strlen (rest) + 1);
  ctx_free (rest);
  string->length = ctx_strlen (string->str);
  string->utf8_length = ctx_utf8_strlen (string->str);
}

void ctx_string_insert_unichar (CtxString *string, int pos, uint32_t unichar)
{
  uint8_t utf8[5]="";
  utf8[ctx_unichar_to_utf8(unichar, utf8)]=0;
  ctx_string_insert_utf8 (string, pos, (char*)utf8);
}

void ctx_string_remove (CtxString *string, int pos)
{
  int old_len = string->utf8_length;
  if (pos < 0)
    return;
  {
    for (int i = old_len; i <= pos; i++)
      {
        _ctx_string_append_byte (string, ' ');
        old_len++;
      }
  }
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if (!p || *p == 0)
    {
      return;
      rest = ctx_strdup ("");
      prev_len = 0;
    }
  else if (* (p+prev_len) == 0)
  {
      rest = ctx_strdup ("");
  }
  else
    {
      rest = ctx_strdup (p + prev_len);
    }
  strcpy (p, rest);
  string->str[string->length - prev_len] = 0;
  ctx_free (rest);
  string->length = ctx_strlen (string->str);
  string->utf8_length = ctx_utf8_strlen (string->str);
}

char *ctx_strdup_printf (const char *format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = (char*)ctx_malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  return buffer;
}

void ctx_string_append_printf (CtxString *string, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = (char*)ctx_malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  ctx_string_append_str (string, buffer);
  ctx_free (buffer);
}

CtxString *ctx_string_new_printf (const char *format, ...)
{
  CtxString *string = ctx_string_new ("");
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = (char*)ctx_malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  ctx_string_append_str (string, buffer);
  ctx_free (buffer);
  return string;
}


void
ctx_string_append_int (CtxString *string, int val)
{
  char buf[64];
  char *bp = &buf[0];
  int remainder;
  if (val < 0)
  {
    buf[0]='-';
    bp++;
    remainder = -val;
  }
  else
  remainder = val;

  int len = 0;
  do {
    int digit = remainder % 10;
    bp[len++] = digit + '0';
    remainder /= 10;
  } while (remainder);

  bp[len]=0;
  for (int i = 0; i < len/2; i++)
  {
    int tmp = bp[i];
    bp[i] = bp[len-1-i];
    bp[len-1-i] = tmp;
  }
  len += (val < 0);
  ctx_string_append_str (string, buf);
}

void
ctx_string_append_float (CtxString *string, float val)
{
  if (val < 0.0f)
  {
    ctx_string_append_byte (string, '-');
    val = -val;
  }
  int remainder = ((int)(val*10000))%10000;
  if (remainder % 10 > 5)
    remainder = remainder/10+1;
  else
    remainder /= 10;
  ctx_string_append_int (string, (int)val);
  if (remainder)
  {
    if (remainder<0)
      remainder=-remainder;
    ctx_string_append_byte (string, '.');
    if (remainder < 10)
      ctx_string_append_byte (string, '0');
    if (remainder < 100)
      ctx_string_append_byte (string, '0');
    ctx_string_append_int (string, remainder);
  }
}
