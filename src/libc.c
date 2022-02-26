#ifndef __CTX_LIBC_H
#define __CTX_LIBC_H

#if !__COSMOPOLITAN__
#include <stddef.h>
#endif

#if 0
static inline void
ctx_memset (void *ptr, uint8_t val, int length)
{
  uint8_t *p = (uint8_t *) ptr;
  for (int i = 0; i < length; i ++)
    { p[i] = val; }
}
#else
#define ctx_memset memset
#endif


static inline void ctx_strcpy (char *dst, const char *src)
{
  int i = 0;
  for (i = 0; src[i]; i++)
    { dst[i] = src[i]; }
  dst[i] = 0;
}

static inline char *_ctx_strchr (const char *haystack, char needle)
{
  const char *p = haystack;
  while (*p && *p != needle)
    {
      p++;
    }
  if (*p == needle)
    { return (char *) p; }
  return NULL;
}
static inline char *ctx_strchr (const char *haystack, char needle)
{
  return _ctx_strchr (haystack, needle);
}

static inline int ctx_strcmp (const char *a, const char *b)
{
  int i;
  for (i = 0; a[i] && b[i]; a++, b++)
    if (a[0] != b[0])
      { return 1; }
  if (a[0] == 0 && b[0] == 0) { return 0; }
  return 1;
}

static inline int ctx_strncmp (const char *a, const char *b, size_t n)
{
  size_t i;
  for (i = 0; a[i] && b[i] && i < n; a++, b++)
    if (a[0] != b[0])
      { return 1; }
  return 0;
}

static inline int ctx_strlen (const char *s)
{
  int len = 0;
  for (; *s; s++) { len++; }
  return len;
}

static inline char *ctx_strstr (const char *h, const char *n)
{
  int needle_len = ctx_strlen (n);
  if (n[0]==0)
    { return (char *) h; }
  while (h)
    {
      h = ctx_strchr (h, n[0]);
      if (!h)
        { return NULL; }
      if (!ctx_strncmp (h, n, needle_len) )
        { return (char *) h; }
      h++;
    }
  return NULL;
}

static inline char *ctx_strdup (const char *str)
{
  int len = ctx_strlen (str);
  char *ret = (char*)malloc (len + 1);
  memcpy (ret, str, len);
  ret[len]=0;
  return ret;
}

#endif
