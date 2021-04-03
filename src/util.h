#ifndef __CTX_UTIL_H
#define __CTX_UTIL_H

inline static float ctx_fast_hypotf (float x, float y)
{
  if (x < 0) { x = -x; }
  if (y < 0) { y = -y; }
  if (x < y)
    { return 0.96f * y + 0.4f * x; }
  else
    { return 0.96f * x + 0.4f * y; }
}

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

#if CTX_FONTS_FROM_FILE

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
    CtxFileContent *c = l->data;
    if (!strcmp (c->path, path))
    {
       if (c->free_data)
       {
         free (c->contents);
       }
       c->free_data = free_data;
       c->contents = (unsigned char*)contents;
       c->length = length;
       return;
    }
  }
  CtxFileContent *c = calloc (sizeof (CtxFileContent), 1);
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
  if (length < 0) length = strlen ((const char*)contents);
  fwrite (contents, 1, length, file);
  fclose (file);
}

static int
__ctx_file_get_contents (const char     *path,
                        unsigned char **contents,
                        long           *length)
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
  if (length)
    { *length =size; }
  rewind (file);
  buffer = (char*)malloc (size + 8);
  if (!buffer)
    {
      fclose (file);
      return -1;
    }
  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      free (buffer);
      return -1;
    }
  fclose (file);
  *contents = (unsigned char*) buffer;
  buffer[size] = 0;
  return 0;
}

#include <limits.h>

int
ctx_get_contents (const char     *uri,
                  unsigned char **contents,
                  long           *length)
{
  char temp_uri[PATH_MAX];
  if (uri[0] == '/')
  {
    snprintf (temp_uri, sizeof (temp_uri)-1, "file://%s", uri);
    uri = temp_uri;
  }
  for (CtxList *l = registered_contents; l; l = l->next)
  {
    CtxFileContent *c = l->data;
    if (!strcmp (c->path, uri))
    {
      contents = malloc (c->length+1);
      contents[c->length]=0;
      if (length) *length = c->length;
      return 0;
    }
  }

  if (!strncmp (uri, "file://", 7))
    return __ctx_file_get_contents (uri + 7, contents, length);
  else
    return __ctx_file_get_contents (uri, contents, length);
}



#endif


#endif

