#ifndef METADATA_NOTEST

#define NO_LIBCURL 1
#define CTX_GET_CONTENTS 1
#define CTX_IMPLEMENTATION
#include "ctx.h"

#endif
#include <stdio.h>

static char *metadata      = NULL;
static long  metadata_len  = 0;
static long  metadata_size = 0;
static char *metadata_path = NULL;

static void metadata_load (const char *path)
{
  if (metadata_path) free (metadata_path);
  metadata_path = malloc (strlen (path) + 10);
  if (metadata)
    free (metadata);
  metadata = NULL;
  metadata_len = 0;
  snprintf (metadata_path, strlen(path)+10, "%s/ctx.idx", path);
  ctx_get_contents (metadata_path, &metadata, &metadata_size);
  metadata_len = metadata_size;
}

#if 1
static void metadata_save (void)
{
  if (!metadata_path) return;
  FILE *file = fopen (metadata_path, "w");
  fwrite (metadata, metadata_len, 1, file);
  fclose (file);
}
#endif

static int metadata_count (void)
{
  const char *m = metadata;
  if (!m) return -1;
  int count = 0;
  if (m[0])
  do {
     if (m[0] != ' ' && m[0] != '\n') count ++;
     while (m && *m && *m != '\n') m++;
     if (*m == '\n') m++;
  } while (m[0]);
  return count;
}

static char *metadata_item_name (int no)
{
  const char *m = metadata;
  if (!m) return NULL;
  int count = 0;
  do {
     if (m && m[0] && m[0] != ' ' && m[0] != '\n'){
        if (count == no)
        {
          CtxString *str = ctx_string_new ("");
          for (int i = 0; m[i] && (m[i]!='\n'); i++)
          {
            ctx_string_append_byte (str, m[i]);
          }
          return ctx_string_dissolve (str);
        }
        count ++;
     }
     while (m && *m && *m != '\n') m++;
     if (*m == '\n') m++;
  } while (m && m[0]);
  return NULL;
}


static const char *metadata_find_no (int no)
{
  const char *m = metadata;
  int count = 0;
  while (m && *m)
  {
    if (m[0] != ' ')
    {
      while (m && *m && *m != '\n') m++;
      if (m && *m == '\n') m++;
      if (count == no)
        return m;
      count++;
    }
    else
    {
      while (m && *m && *m != '\n') m++;
      if (m && *m == '\n') m++;
    }
  }
  return NULL;
}

static int _metadata_metalen (const char *m)
{
  int len = 0;
  do {
    if (*m && m[0] == ' ') 
    while (*m && m[0] != '\n') { m++; len++ ;}
    if (*m == '\n') { m++; len++ ;}
  } while (*m && m[0] == ' ');
  return len;
}

static const char *metadata_find_item (const char *item)
{
  int item_len = strlen (item);
  const char *m = metadata;

  while (m && *m)
  {
    if (m && !strncmp (m, item, item_len) && m[item_len]=='\n')
      return m + item_len + 1;
    while (m && *m && *m != '\n') m++;
    if (m && *m == '\n') m++;
  }
  return NULL;
}

int metadata_item_to_no (const char *item)
{
  int item_len = strlen (item);
  const char *m = metadata;
  int no = 0;

  while (m && *m)
  {
    if (m && !strncmp (m, item, item_len) && m[item_len]=='\n')
      return no;
    while (m && *m && *m != '\n') m++;
    if (m && *m == '\n') m++;
    if (m[0] != ' ') no ++;
  }
  return -1;
}

static int metadata_item_key_count (const char *item)
{
  const char *m = metadata_find_item (item);
  if (!m) return -1;
  int count = 0;
  if (m[0] == ' ')
  do {
     while (m && *m && *m != '\n') m++;
     if (*m == '\n') m++;
     count ++;
  } while (m[0] == ' ');
  return count;
}

static char *metadata_key_name (const char *item, int no)
{
  const char *m = metadata_find_item (item);
  CtxString *str = ctx_string_new ("");
  if (!m) return NULL;
  int count = 0;
  if (m[0] == ' ')
  do {
     if (count == no)
     {
     while (*m && m[0] == ' ') m++;
     while (*m && m[0] != '=') {
       ctx_string_append_byte (str, m[0]);
       m++;
     }
       return ctx_string_dissolve (str);
     }
     while (m && *m && *m != '\n') m++;
     if (*m == '\n') m++;
     count ++;
  } while (m[0] == ' ');
  return NULL;
}

static char *metadata_key_string (const char *item, const char *key)
{
  const char *m = metadata_find_item (item);
  CtxString *str = ctx_string_new ("");
  if (!m) return "";
  if (m[0] == ' ')
  do {
     ctx_string_set (str, "");
     while (*m && m[0] == ' ') m++;
     while (*m && m[0] != '=') {
       ctx_string_append_byte (str, m[0]);
       m++;
     }
     if (!strcmp (key, str->str))
     {
       ctx_string_set (str, "");
       m++;
       while (*m && m[0] != '\n') {
         ctx_string_append_byte (str, m[0]);
         m++;
       }
       return ctx_string_dissolve (str);
     }
     while (m && *m && *m != '\n') m++;
     if (*m == '\n') m++;
  } while (m[0] == ' ');
  ctx_string_free (str, 1);
  return NULL;
}

static float metadata_key_float (const char *item, const char *key)
{
   char *value = metadata_key_string (item, key);
   if (value)
   {
     float ret = atof (value);
     free (value);
     return ret;
   }
   return 0.0f;
}

static int metadata_key_int (const char *item, const char *key)
{
  return metadata_key_float (item, key);
}

#ifndef METADATA_NOTEST

void metadata_swap (int no_a, int no_b)
{
   if (no_b < no_a)
   {
     int tmp = no_a;
     no_a = no_b; no_b = tmp;
   }

   char *a_name = metadata_item_name (no_a);
   char *b_name = metadata_item_name (no_b);
   int a_name_len = strlen (a_name);
   int b_name_len = strlen (b_name);
   const char *a_meta = metadata_find_no (no_a);
   const char *b_meta = metadata_find_no (no_b);

   int a_start = (a_meta - metadata) - a_name_len - 1;
   int a_len = a_name_len + 1 + _metadata_metalen (a_meta);


   int b_start = (b_meta - metadata) - b_name_len - 1;
   int b_len = b_name_len + 1 + _metadata_metalen (b_meta);

   char *a_temp = malloc (a_len);
   char *b_temp = malloc (b_len);

   memcpy (a_temp, metadata + a_start, a_len);
   memcpy (b_temp, metadata + b_start, b_len);

   memmove (metadata + b_start, metadata + b_start + b_len, metadata_len - b_start - b_len);
   metadata_len -= b_len;
   memmove (metadata + a_start, metadata + a_start + a_len, metadata_len - a_start - a_len);
   metadata_len -= a_len;

   b_start -= a_len;
   memmove (metadata + b_start + a_len, metadata + b_start, metadata_len - b_start);
   memcpy (metadata + b_start, a_temp, a_len);
   metadata_len += a_len;
   memmove (metadata + a_start + b_len, metadata + a_start, metadata_len - a_start);
   memcpy (metadata + a_start, b_temp, b_len);
   metadata_len += b_len;

   free (a_temp);
   free (b_temp);
}

void metadata_remove (int no)
{
   char *a_name = metadata_item_name (no);
   int a_name_len = strlen (a_name);
   const char *a_meta = metadata_find_no (no);

   int a_start = (a_meta - metadata) - a_name_len - 1;
   int a_len = a_name_len + 1 + _metadata_metalen (a_meta);

   memmove (metadata + a_start, metadata + a_start + a_len, metadata_len - a_start - a_len);
   metadata_len -= a_len;
   metadata[metadata_len]=0;
}

void metadata_unset (const char *item, const char *key)
{
   const char *a_meta = metadata_find_item (item);
   CtxString *str = ctx_string_new ("");
   const char *m = a_meta;

   const char *prop_start = m;
   int a_len = 0;

  if (!m) return;
  if (m[0] == ' ')
  do {
     ctx_string_set (str, "");
     a_len = 0;
     prop_start = m;
     while (*m && m[0] == ' '){ m++;
       a_len++;
     }
     while (*m && m[0] != '=') {
       ctx_string_append_byte (str, m[0]);
       m++;
       a_len++;
     }
     while (m && *m && *m != '\n'){
       m++;
       a_len++;
     }
     if (*m == '\n'){
       m++;
       a_len ++;
     }
     if (!strcmp (key, str->str))
     {
       break;
     }
     prop_start = NULL;
  } while (m[0] == ' ');

  if (!prop_start)
    return;
   
   int a_start = prop_start - metadata;
   //int a_len = a_name_len + 1 + _metadata_metalen (a_meta);

   memmove (metadata + a_start, metadata + a_start + a_len, metadata_len - a_start - a_len);
   metadata_len -= a_len;
   metadata[metadata_len]=0;

}

static void _metadata_insert (int pos, const char *data, int len)
{
  if (metadata_len + len >= metadata_size - 1)
  {
     metadata_size = metadata_len + len + 1024;
     metadata = realloc (metadata, metadata_size);
  }
  memmove (metadata + pos + len, metadata + pos, metadata_len - pos);
  memcpy (metadata + pos, data, len);
  metadata_len += len;
}

void metadata_insert (int pos, const char *item)
{
  const char *m = metadata_find_no (pos);
  if (m)
  {
    char *name = metadata_item_name (pos);
    m -= strlen (name) + 1;
    free (name);
  }
  else
  {
    m = metadata + metadata_len;
  }
  char *tmp[strlen (item) + 3];
  snprintf (tmp, sizeof(tmp), "%s\n", item);

  _metadata_insert (m-metadata, tmp, strlen (tmp));
}

void metadata_rename (int pos, const char *new_name)
{
  const char *m = metadata_find_no (pos);
  if (m)
  {
    char *name = metadata_item_name (pos);
    int name_len = strlen (name);
    free (name);

    m -= name_len + 1;
    memmove (m, m + name_len + 1, metadata_len - (m-metadata) - name_len - 1);
    metadata_len -= name_len + 1;
  }
  else
  {
    return;
  }
  char *tmp[strlen (new_name) + 3];
  snprintf (tmp, sizeof(tmp), "%s\n", new_name);

  _metadata_insert (m-metadata, tmp, strlen (tmp));
}

void metadata_add (const char *item, const char *key, const char *value)
{
  CtxString *str = ctx_string_new ( " ");
  metadata_unset (item, key);

  const char *m = metadata_find_item (item);
  int offset = metadata_len;
  if (m)
  {
    while (m[0] == ' ')
    {
       while (m[0] != '\n') m++;
       if (m[0] == '\n') m++;
    }
    offset = m - metadata;
  }
  else
  {
    ctx_string_append_str (str, item);
    ctx_string_append_byte (str, '\n');
  }
  ctx_string_append_str (str, key);
  ctx_string_append_byte (str, '=');
  ctx_string_append_str (str, value);
  ctx_string_append_byte (str, '\n');

  _metadata_insert (offset, str->str, str->length);
}

void metadata_set (const char *item, const char *key, const char *value)
{
  metadata_unset (item, key);
  metadata_add (item, key, value);
}

void metadata_set_float (const char *item, const char *key, float value)
{
  char str[64];
  sprintf (str, "%f", value);
  while (str[strlen(str)-1] == '0')
    str[strlen(str)-1] = 0;
  metadata_set (item, key, str);
}

void metadata_dump (void)
{
  int item_count = metadata_count ();
  fprintf (stderr, "item count: %i\n", item_count);
  for (int i = 0; i < item_count; i ++)
  {
    char *item_name = metadata_item_name (i);
    fprintf (stderr, "%i : %s : %i\n", i, item_name, metadata_item_to_no (item_name));
    int keys = metadata_item_key_count (item_name);
    for (int k = 0; k < keys; k ++)
    {
      char *key_name = metadata_key_name (item_name, k);
      fprintf (stderr, " %s=%s\n", key_name,
                      metadata_key_string (item_name, key_name));
    }
  }
}

int main (int argc, char **argv)
{
  if (argv[1])
    metadata_load (argv[1]);
  metadata_dump ();
  metadata_unset ("ctx.c", "label");
  metadata_set ("README", "hello", "there");
  metadata_set ("README", "hello", "void");
  metadata_swap (0, 3);
  metadata_remove(3);
//metadata_rename(0, "we need speedier access");
  metadata_insert(0, "abc");
  metadata_insert(1, "def");
  //metadata_save ();

  fprintf (stderr, "{%s}\n", metadata);
  metadata_dump ();
  return 0;
}

#endif