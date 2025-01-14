#ifndef __CTX_UTIL_H
#define __CTX_UTIL_H


static int ctx_str_is_number (const char *str)
{
  int got_digit = 0;
  for (int i = 0; str[i]; i++)
  {
    if (str[i] >= '0' && str[i] <= '9')
    {
       got_digit ++;
    }
    else if (str[i] == '.')
    {
    }
    else
      return 0;
  }
  if (got_digit)
    return 1;
  return 0;
}

#if CTX_GET_CONTENTS

typedef struct CtxFileContent
{
  char *path;
  unsigned char *contents;
  long  length;
  int   free_data;
} CtxFileContent;

CtxList *registered_contents = NULL;

void
ctx_register_contents (const char *path,
                       const unsigned char *contents,
                       long length,
                       int  free_data)
{
  // if (path[0] != '/') && strchr(path, ':')) 
  //   with this check regular use is faster, but we lose
  //   generic filesystem overrides..
  for (CtxList *l = registered_contents; l; l = l->next)
  {
    CtxFileContent *c = (CtxFileContent*)l->data;
    if (!ctx_strcmp (c->path, path))
    {
       if (c->free_data)
       {
         ctx_free (c->contents);
       }
       c->free_data = free_data;
       c->contents = (unsigned char*)contents;
       c->length = length;
       return;
    }
  }
  CtxFileContent *c = (CtxFileContent*)ctx_calloc (sizeof (CtxFileContent), 1);
  c->free_data = free_data;
  c->contents = (unsigned char*)contents;
  c->length    = length;
  ctx_list_append (&registered_contents, c);
}

void
_ctx_file_set_contents (const char     *path,
                        const unsigned char  *contents,
                        long            length)
{
  FILE *file;
  file = fopen (path, "wb");
  if (!file)
    { return; }
  if (length < 0) length = ctx_strlen ((const char*)contents);
  fwrite (contents, 1, length, file);
  fclose (file);
}

static int
___ctx_file_get_contents (const char     *path,
                          unsigned char **contents,
                          long           *length,
                          long            max_len)
{
  FILE *file;
  long  size;
  long  remaining;
  char *buffer;
  file = fopen (path, "rb");
  if (!file)
    { return -1; }
  fseek (file, 0, SEEK_END);
  size = remaining = ftell (file);

  if (size > max_len)
  {
     size = remaining = max_len;
  }

  if (length)
    { *length =size; }
  rewind (file);
  buffer = (char*)ctx_malloc (size + 8);
  if (!buffer)
    {
      fclose (file);
      return -1;
    }
  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      ctx_free (buffer);
      return -1;
    }
  fclose (file);
  *contents = (unsigned char*) buffer;
  buffer[size] = 0;
  return 0;
}

static int
__ctx_file_get_contents (const char     *path,
                        unsigned char **contents,
                        long           *length)
{
  return ___ctx_file_get_contents (path, contents, length, 1024*1024*1024);
}

#if !__COSMOPOLITAN__
#include <limits.h>
#endif




#endif


#endif

