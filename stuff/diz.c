#include "diz.h"
#include "ctx.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

static void  mkdir_ancestors      (const char  *path,
                                   unsigned int mode);

static void  diz_update_files     (Diz *diz);
static void _diz_remove           (Diz *diz, int no);

static char *diz_get_name_escaped (Diz *diz,
                                   int  no);

static char *diz_escape_item      (const char *item);


void diz_wipe_cache (Diz *diz, int full)
{
  if (full)
    diz->metadata_cache_no = -3;
  if (diz->cache)
  {
    for (int i = 0; i < diz->cache_size; i++)
    {
       if (diz->cache[i])
       {
        free (diz->cache[i]);
        diz->cache[i] = NULL;
       }
    }
    free (diz->cache);
    diz->cache = NULL;
    diz->cache_size = 0;
  }
}

void diz_load (Diz *diz, const char *path, int text_editor_mode)
{
  diz->metadata_cache_no = -3;
  diz->metadata_cache = NULL;
  if (diz->metadata_path) free (diz->metadata_path);
  diz->metadata_path = malloc (strlen (path) + 20);
  if (diz->metadata)
    free (diz->metadata);
  diz->metadata = NULL;
  diz->metadata_len = 0;
  diz->metadata_size = 0;
  if (text_editor_mode)
  {
    uint8_t *contents = NULL;
    long length = 0;

    strcpy (diz->metadata_path, path);
    ctx_get_contents (diz->metadata_path, &contents, &length);
    if (contents)
    {
       CtxString *line = ctx_string_new ("");
       int i;
       for (i = 0; contents[i]; i++)
       {
         char p = contents[i];
         if (p == '\n')
         {
           diz_insert (diz, -1, line->str);
           ctx_string_set (line, "");
         }
         else
         {
           ctx_string_append_byte (line, p);
         }
       }
       if (line->str[0])
         diz_insert (diz, -1, line->str);
       free (contents);
    }

  }
  else
  {
    snprintf (diz->metadata_path, strlen(path)+20, "%s/.ctx/index", path);
  ctx_get_contents (diz->metadata_path, (uint8_t**)&diz->metadata, &diz->metadata_size);
  diz->metadata_len = (int)diz->metadata_size;
  }
  diz_wipe_cache (diz, 1);
}

static void mkdir_ancestors (const char *path, unsigned int mode)
{
  char *tmppaths=strdup (path);
  char *sl = strchr (tmppaths, '/');
  while (sl && *sl)
  {
    sl ++;
    sl = strchr (sl, '/');
    if (sl)
    {
      *sl = '\0';
      mkdir (tmppaths, mode);
      *sl = '/';
    }
  }
  free (tmppaths);
}

void diz_save (Diz *diz, int text_editor_mode)
{
  if (!diz->metadata_path) return;
  mkdir_ancestors (diz->metadata_path, 0777);

  FILE *file = fopen (diz->metadata_path, "w");
  if (text_editor_mode)
  {
    int count = diz_count (diz);
    for (int i = 0; i < count; i++)
    {
      char *line = diz_get_name (diz, i);
      fwrite (line, strlen (line), 1, file);
      fwrite ("\n", 1, 1, file);
      free (line);
    }
  }
  else
  {
    fwrite (diz->metadata, diz->metadata_len, 1, file);
  }
  fclose (file);
}

int diz_count (Diz *diz)
{
  const char *m = diz->metadata;
  if (!m) return -1;
  int count = 0;
  if (m[0])
  do {
     if (m[0] != ' ') count ++;
     while (m && *m && *m != '\n') m++;
     if (*m == '\n') m++;
  } while (m[0]);
  return count;
}

static char *diz_get_name_escaped (Diz *diz, int no)
{
  const char *m = diz->metadata;
  if (!m) return NULL;
  int count = 0;
  do {
     if (m && m[0] != ' '){
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

static char *diz_find_no (Diz *diz, int no)
{
  char *m = diz->metadata;
  int count = 0;

  if (no == -1)
  {
    return m;
  }

  int update_cache = 1;
#if 1
  if (diz->metadata_cache_no == no) return diz->metadata_cache;
  if (diz->metadata_cache_no < no && diz->metadata_cache_no > 0)
  {
    if (no - diz->metadata_cache_no < 4) /* keep a small look-ahead, much of the
                                       time */
      update_cache = 0;
    m = diz->metadata_cache;
    count = diz->metadata_cache_no;
    m-= 1;
  }
#endif

  while (*m)
  {
    if (m[0] != ' ')
    {
      while (*m && *m != '\n') m++;
      if (*m == '\n') m++;
      if (count == no)
      {
        if (update_cache)
        {
          diz->metadata_cache = m;
          diz->metadata_cache_no = no;
        }
        return m;
      }
      count++;
    }
    else
    {
      // it is a key
      while (*m && *m != '\n') m++;
      if (*m && *m == '\n') m++;
    }
  }
  return NULL;
}

char *diz_get_name (Diz *diz, int no)
{
  if (diz->cache)
  { if (diz->cache[no])
      return strdup (diz->cache[no]);
  }
  else
  {
    int new_count = diz_count (diz);
    diz->cache_size = new_count;
    diz->cache = calloc (sizeof(void*), new_count);
  }

  if (no == -1)
  {
     return diz_get_string (diz, no, "title");
  }


  /* this makes use reuse the cache */
  const char *m = diz_find_no (diz, no);
  if (!m) return NULL;
  m--;
  m--;
  while (m[0]!='\n' && m != diz->metadata)m--;
  if (m !=diz->metadata) m++;

  CtxString *str = ctx_string_new ("");
  for (int i = 0; m[i] && (m[i]!='\n'); i++)
  {
    switch (m[i])
    {
      case '\\':
        if (m[i+1])
        {
          switch (m[i+1])
          {
            case 'n':
              ctx_string_append_byte (str, '\n');
              break;
            case '0':
              ctx_string_append_byte (str, '\0');
              break;
            default:
              ctx_string_append_byte (str, m[i+1]);
          }
          i++;
        }
        else
        {
          ctx_string_append_byte (str, m[i]);
        }
        break;
      default:
        ctx_string_append_byte (str, m[i]);
    }
  }

  diz->cache[no]=strdup(str->str);
  return ctx_string_dissolve (str);
}


static int _diz_metalen (Diz *diz, const char *m)
{
  int len = 0;
  do {
    if (*m && m[0] == ' ') 
    {
      while (*m && m[0] != '\n') { m++; len++ ;}
      if (*m == '\n') { m++; len++ ;}
    }
  } while (*m && m[0] == ' ');
  return len;
}

static char *diz_escape_item (const char *item) // XXX expand with length to
                                              // handle \0 and
                                              // thus arbitrary blobs?
{
  // this is kind of a hack - but it avoids
  // bare newlines - which some parts of the code
  // still has trouble with
  // XXX XXX XXX
  if (item[0]==0)return strdup("\\0");
  
  CtxString *str = ctx_string_new ("");
  for (int i = 0; item[i]; i++)
  {
    switch (item[i])
    {
      case ' ':
        if (i != 0)
        {
          ctx_string_append_byte (str, item[i]);
        }
        else
        {
          ctx_string_append_byte (str, '\\');
          ctx_string_append_byte (str, item[i]);
        }
        break;
      case '\0':
        ctx_string_append_byte (str, '\\');
        ctx_string_append_byte (str, '0');
        break;
      case '\n':
        ctx_string_append_byte (str, '\\');
        ctx_string_append_byte (str, 'n');
        break;
      case '\\':
        ctx_string_append_byte (str, '\\');
        ctx_string_append_byte (str, '\\');
        break;
      default:
        ctx_string_append_byte (str, item[i]);
    }
  }
  return ctx_string_dissolve (str);
}

static const char *diz_find_item (Diz *diz, const char *item)
{
  char *escaped_item = diz_escape_item (item);
  int item_len = strlen (escaped_item);
  const char *m = diz->metadata;

  while (m && *m)
  {
    if (m && !strncmp (m, escaped_item, item_len) && m[item_len]=='\n')
    {
      free (escaped_item);
      return m + item_len + 1;
    }
    while (m && *m && *m != '\n') m++;
    if (m && *m == '\n') m++;
  }
  free (escaped_item);
  return NULL;
}

int diz_name_to_no (Diz *diz, const char *item)
{
  char *escaped_item = diz_escape_item (item);
  int item_len = strlen (escaped_item);
  const char *m = diz->metadata;
  int no = 0;

  while (m && *m)
  {
    if (m && !strncmp (m, escaped_item, item_len) && m[item_len]=='\n')
    {
      free (escaped_item);
      return no;
    }
    while (m && *m && *m != '\n') m++;
    if (m && *m == '\n') m++;
    if (m[0] != ' ') no ++;
  }
  free (escaped_item);
  return -1;
}

int diz_value_count   (Diz *diz, int item_no, const char *key)
{
  const char *m = diz_find_no (diz, item_no);
  if (!m) return -1;
  int count = 0;
  int keylen = strlen (key);
  if (m[0] == ' ')
  do {
    if (!strncmp (m+1, key, keylen) && m[1+keylen] == '=')
      count ++;
    while (m && *m && *m != '\n') m++;
    if (*m == '\n') m++;
  } while (m[0] == ' ');
  return count;
}

char *diz_get_string_no (Diz *diz, int item_no, const char *key, int value_no){
  const char *m = diz_find_no (diz, item_no);
  if (!m) return strdup("xXx");
  int count = 0;
  int keylen = strlen (key);
  if (m[0] == ' ')
  do {

    if (!strncmp (m+1, key, keylen) && m[1+keylen]== '=')
    {
      if (value_no == count)
      {
        char ret[1034];
        int i;
        for (i = keylen; m[i] != '\n' && i-keylen < 1024; i++)
          ret[i-keylen] = m[i];
        ret[i-keylen] = 0;
        return strdup (ret);
      }
      count ++;
    }
    while (m && *m && *m != '\n') m++;
    if (*m == '\n') m++;
  } while (m[0] == ' ');
  return strdup("xxxx");
}

int diz_key_count (Diz *diz, int no)
{
  const char *m = diz_find_no (diz, no);
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

char *diz_key_name (Diz *diz, int ino, int no)
{
  const char *m = diz_find_no (diz, ino);
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

char *diz_get_string (Diz *diz, int no, const char *key)
{
  const char *m = diz_find_no (diz, no);
  CtxString *str = ctx_string_new ("");
  if (!m) return NULL;
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
       int gotesc = 0;
       while (*m && m[0] != '\n') {
         char val = m[0];
         if (gotesc)
         {
           switch (val)
           {
             case '0':
               ctx_string_append_byte (str, '\0');
               break;
             case 'n':
               ctx_string_append_byte (str, '\n');
               break;
             default:
               ctx_string_append_byte (str, m[0]);
               break;
           }
           gotesc = 0;
         }
         else
         switch (val)
         {
           case '\\':
             gotesc = 1;
             break;
           default:
             ctx_string_append_byte (str, m[0]);
             gotesc = 0;
             break;
         }
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

char *diz_get (Diz *diz, int no, const char *key)
{
  return diz_get_string (diz, no, key);
}

void diz_swap (Diz *diz, int no_a, int no_b)
{
   if (no_a == no_b) return;
   if (no_b < no_a)
   {
     int tmp = no_a;
     no_a = no_b; no_b = tmp;
   }

   char *a_name = diz_get_name_escaped (diz, no_a);
   char *b_name = diz_get_name_escaped (diz, no_b);
   int a_name_len = strlen (a_name);
   int b_name_len = strlen (b_name);
   const char *a_meta = diz_find_no (diz, no_a);
   const char *b_meta = diz_find_no (diz, no_b);

   int a_start = (a_meta - diz->metadata) - a_name_len - 1;
   int a_len = a_name_len + 1 + _diz_metalen (diz, a_meta);


   int b_start = (b_meta - diz->metadata) - b_name_len - 1;
   int b_len = b_name_len + 1 + _diz_metalen (diz, b_meta);

   char *a_temp = malloc (a_len);
   char *b_temp = malloc (b_len);

   memcpy (a_temp, diz->metadata + a_start, a_len);
   memcpy (b_temp, diz->metadata + b_start, b_len);

   memmove (diz->metadata + b_start,
            diz->metadata + b_start + b_len, diz->metadata_len - b_start - b_len);
   diz->metadata_len -= b_len;
   memmove (diz->metadata + a_start,
            diz->metadata + a_start + a_len, diz->metadata_len - a_start - a_len);
   diz->metadata_len -= a_len;

   b_start -= a_len;
   memmove (diz->metadata + b_start + a_len,
            diz->metadata + b_start, diz->metadata_len - b_start);
   memcpy (diz->metadata + b_start, a_temp, a_len);
   diz->metadata_len += a_len;
   memmove (diz->metadata + a_start + b_len,
            diz->metadata + a_start,
            diz->metadata_len - a_start);
   memcpy (diz->metadata + a_start, b_temp, b_len);
   diz->metadata_len += b_len;

   free (a_temp);
   free (b_temp);

   diz->metadata_cache_no = -3;
   diz_wipe_cache (diz, 1);
}

static void _diz_remove (Diz *diz, int no)
{
   char *a_name = diz_get_name_escaped (diz, no);
   int a_name_len = strlen (a_name);
   const char *a_meta = diz_find_no (diz, no);

   int a_start = (a_meta - diz->metadata) - a_name_len - 1;
   int a_len = a_name_len + 1 + _diz_metalen (diz, a_meta);

   memmove (diz->metadata + a_start,
            diz->metadata + a_start + a_len,
            diz->metadata_len - a_start - a_len);
   diz->metadata_len -= a_len;
   diz->metadata[diz->metadata_len]=0;
   diz_wipe_cache (diz, 1);
   //diz_update_files (diz);
   //
   // on a timeout? XXX
}

void diz_remove (Diz *diz, int no)
{
   _diz_remove (diz, no);
   diz_update_files (diz);
}

void diz_unset (Diz *diz, int no, const char *key)
{
  const char *a_meta = diz_find_no (diz, no);
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
   
   int a_start = prop_start - diz->metadata;

   memmove (diz->metadata + a_start,
            diz->metadata + a_start + a_len, diz->metadata_len - a_start - a_len);
   diz->metadata_len -= a_len;
   diz->metadata[diz->metadata_len]=0;
   diz_wipe_cache (diz, 1);
}

static void _diz_insert (Diz *diz, int pos, const char *data, int len)
{
  int wipe_full = (pos != diz->metadata_len);
  if (diz->metadata_len + len >= diz->metadata_size - 1)
  {
     diz->metadata_size = diz->metadata_len + len + 1024;
     diz->metadata = realloc (diz->metadata, diz->metadata_size);
  }
  memmove (diz->metadata + pos + len, diz->metadata + pos, diz->metadata_len - pos);
  memcpy (diz->metadata + pos, data, len);
  diz->metadata_len += len;
  diz->metadata[diz->metadata_len] = 0;
  diz_wipe_cache (diz, wipe_full);
}

int diz_insert (Diz *diz, int pos, const char *item)
{
  const char *m = NULL;
  if (pos == -1)
    pos = diz->metadata_len;
  else
    m = diz_find_no (diz, pos);

  if (m)
  {
    char *name = diz_get_name_escaped (diz, pos);
    m -= strlen (name) + 1;
    free (name);
  }
  else
  {
    m = diz->metadata + diz->metadata_len;
  }
  char *escaped_item = diz_escape_item (item);
  char tmp[strlen (escaped_item) + 3];
  snprintf (tmp, sizeof(tmp), "%s\n", escaped_item);
  free (escaped_item);

  _diz_insert (diz, m-diz->metadata, tmp, strlen (tmp));
  return pos;
}

void diz_set_name (Diz *diz, int pos, const char *new_name)
{
  // TODO : if it is a file, do a corresponding rename!
  //
  if (pos == -1)
  {
     diz_set_string (diz, pos, "title", new_name);
     return;
  }

  char *m = diz_find_no (diz, pos);
  if (m)
  {
    char *name = diz_get_name_escaped (diz, pos);
    int name_len = strlen (name);
    free (name);

    m -= name_len + 1;
    memmove (m, m + name_len + 1, diz->metadata_len - (m-diz->metadata) - name_len - 1);
    diz->metadata_len -= name_len + 1;
  }
  else
  {
    return;
  }

  char *new_escaped = diz_escape_item (new_name);
  char tmp[strlen (new_escaped) + 3];
  snprintf (tmp, sizeof(tmp), "%s\n", new_escaped);
  free (new_escaped);

  _diz_insert (diz, m-diz->metadata, tmp, strlen (tmp));
}

void diz_add_string (Diz *diz, int no, const char *key, const char *value)
{
  CtxString *str = ctx_string_new ( " ");
  diz_unset (diz, no, key);

  const char *m = diz_find_no (diz, no);
  int offset = diz->metadata_len;
  if (m)
  {
    while (m[0] == ' ')
    {
       while (m[0] != '\n') m++;
       if (m[0] == '\n') m++;
    }
    offset = m - diz->metadata;
  }
  else
  {
    // XXX assert?
    //ctx_string_append_str (str, item);
    //ctx_string_append_byte (str, '\n');
  }
  ctx_string_append_str (str, key);
  ctx_string_append_byte (str, '=');
  ctx_string_append_str (str, value);
  ctx_string_append_byte (str, '\n');

  _diz_insert (diz, offset, str->str, str->length);
}

void diz_add_string_unique (Diz *diz, int item_no, const char *key, const char *new_value)
{
  int value_count = diz_value_count (diz, item_no, key);
  for (int i = 0; i < value_count; i++)
  {
    char *value = diz_get_string_no (diz, item_no, key, i);
    if (value)
    {
      if (!strcmp (value, new_value))
      {
        free (value);
        return;
      }
      free (value);
    }
  }
  diz_add_string (diz, item_no, key, new_value);
}

void diz_set_string (Diz *diz, int item_no, const char *key, const char *value)
{
  diz_unset (diz, item_no, key);
  diz_add_string (diz, item_no, key, value);
}


void diz_dump (Diz *diz)
{
  int item_count = diz_count (diz);
  fprintf (stdout, "item count: %i\n", item_count);
  int indent = 0;

  for (int i = -1; i < item_count; i ++)
  {
    char *item_name = i>=0? diz_get_name (diz, i):NULL;
    for (int i = 0; i < indent; i++) fprintf (stdout, " ");
    if (item_name)
      fprintf (stdout, "%s     :%i,%i\n", item_name, i, diz_name_to_no (diz, item_name));
    int keys = diz_key_count (diz, i);
    for (int k = 0; k < keys; k ++)
    {
      char *key_name = diz_key_name (diz, i, k);
      for (int i = 0; i < indent; i++) fprintf (stdout, " ");
      fprintf (stdout, "   %s=%s\n", key_name,
                      diz_get_string (diz, i, key_name));
      if (!strcmp (key_name, "type") &&
          !strcmp(diz_get_string (diz, i, key_name), "startgroup"))
              indent +=4;
      else if (!strcmp (key_name, "type") &&
          !strcmp(diz_get_string (diz, i, key_name), "endgroup"))
              indent -=4;
    }
    fflush (stdout);
  }
}

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <ctype.h>
#include <signal.h>

static int custom_sort (const struct dirent **a,
                        const struct dirent **b)
{
  if ((*a)->d_type != (*b)->d_type)
  {
    return ((*a)->d_type - (*b)->d_type);
  }
  return strcmp ((*a)->d_name , (*b)->d_name);
}

extern int text_editor;

static void
diz_update_files (Diz *diz)
{
  struct  dirent **namelist = NULL;
  int     n;

  n = scandir (diz->path, &namelist, NULL, custom_sort);

  CtxList *to_add = NULL; // XXX a temporary list here might be redundant
  for (int i = 0; i < n; i++)
  {
    int found = 0;
    const char *name = namelist[i]->d_name;
    if (diz_name_to_no (diz, name)>=0)
       found = 1;
    if (!found && (name[0] != '.'))
    {
      ctx_list_prepend (&to_add, strdup (name));
    }
  }
  while (to_add)
  {
    char *name = to_add->data;
    int n = diz_insert(diz, -1, name);
    diz_set_string (diz, n, "type", "file");
    ctx_list_remove (&to_add, name);
    free (name);
  }

  diz->count = diz_count (diz);
  CtxList *to_remove = NULL;
  for (int i = 0; i < diz->count; i++)
  {
    int atom = diz_type_atom (diz, i);
    if (atom == CTX_ATOM_FILE)
    {
      char *name = diz_get_name (diz, i);
      char *path = ctx_strdup_printf ("%s/%s", diz->path, name);
      struct stat stat_buf;
      if (lstat (path, &stat_buf) != 0)
      {
        //DIR_INFO ("removing file item %s", name);
        ctx_list_prepend (&to_remove, strdup (name));
      }
      free (name);
      free (path);
    }
  }
  while (to_remove)
  {
    char *name = to_remove->data;
    int no = 0;
    if ((no = diz_name_to_no (diz, name))>=0)
    {
      _diz_remove (diz, no);
    }
    ctx_list_remove (&to_remove, name);
    free (name);
  }

  diz->count = diz_count (diz);

  while (n--)
    free (namelist[n]);
  free (namelist);

  // TODO fully remove non-existent diz when empty?
  //if (added)
  //  diz_dirt();
}


void
diz_set_path (Diz *diz,
                     const char *path,
                     const char *title)
{
  char *resolved_path = realpath (path, NULL);
  char *title2 = NULL;

  if (title) title2 = strdup (title);
  else title2 = strdup (path);


  if (diz->path)
    free (diz->path);
  diz->path = resolved_path;
  if (diz->title)
    free (diz->title);
  diz->title = title2;

  diz_load (diz, resolved_path, text_editor);
  diz->count = diz_count (diz);

  if (text_editor)
    return;

  diz_update_files (diz);
}

int diz_item_get_level (Diz *diz, int no)
{
  int level = 0;
  for (int i = 0; i <= no; i++)
  {
    int atom = diz_type_atom (diz, i);
    switch (atom)
    {
      case CTX_ATOM_STARTGROUP:
        level++;
        break;
      case CTX_ATOM_ENDGROUP:
        level--;
        break;
      default:
        break;
    }
  }
  return level;
}

int diz_measure_chunk (Diz *diz, int no)
{
  int count = 1;
  int self_level = diz_item_get_level (diz, no);
  int level;

  do {
    no ++;
    level = diz_item_get_level (diz, no);
    if (level > self_level) count++;
  } while (level > self_level);
  if (count > 1) count ++;
  
  return count;
}

//////////// below are code making interacting with it nicer

int diz_get_parent (Diz *diz, int no)
{
  int level = 1;
  CtxAtom atom;
  while (level > 0 && no >= 0)
  {
    no--;
    atom = diz_type_atom (diz, no);
    if (atom == CTX_ATOM_STARTGROUP)
      {
        level--;
      }
    else if (atom == CTX_ATOM_ENDGROUP)
      {
        level++;
      }
  }
  no--;
  if (no < 0)
    no = 0;
  return no;
}

int diz_ancestor_is_folded (Diz *diz, int no)
{
  int iter = diz_get_parent (diz, no);
  while (iter != 0)
  {
    if (diz_get_int (diz, iter+1, "folded", 0))
    {
      return 1;
    }
    iter = diz_get_parent (diz, iter);
  }
  return 0;
}

int diz_has_children (Diz *diz, int no)
{
  CtxAtom  atom = diz_type_atom (diz, no+1);
  if (atom == CTX_ATOM_STARTGROUP)
    return 1;
  return 0;
}

int
diz_prev (Diz *diz, int i)
{
 int pos = i;
 int again = 0;

 /* XXX : retest with collapsed predecessors
  *
  */
 do {
   pos -= 1;
   int atom = diz_type_atom (diz, pos);
   switch (atom)
   {
     case CTX_ATOM_STARTGROUP:
     case CTX_ATOM_ENDGROUP:
     case CTX_ATOM_LAYOUTBOX:
       again = 1;
       break;
     default:
       if (diz_ancestor_is_folded (diz, pos))
         again = 1;
       else
         again = 0;
   }
 } while (pos >= 0 && again);

 //int prev_sibling = dir_prev_sibling (diz, i);
 // move forward towards pos, as far as we can, skipping folded
 // our last position is what we should return


 return pos;
}

int
diz_next (Diz *diz, int i)
{
 int pos = i;
 int again = 0;

 if (i+1>=diz->count)
   return -1;

 do {
   pos += 1;
   int atom = diz_type_atom (diz, pos);
   switch (atom)
   {
     case CTX_ATOM_STARTGROUP:
     case CTX_ATOM_ENDGROUP:
     case CTX_ATOM_LAYOUTBOX:
       again = 1;
       break;
     default:
       again = 0;
   }
 } while (pos >= 0 && again);
 return pos;
}

int
diz_prev_sibling (Diz *diz, int i)
{
  int pos = i;
  int start_level = 0;
  int level = 0; // not absolute level, but relative level balance
  pos --;
  int atom = diz_type_atom (diz, pos);
  if (atom == CTX_ATOM_ENDGROUP)
          level ++;
  else if (atom == CTX_ATOM_STARTGROUP)
          level --;
  while (level > start_level)
  {
    pos--;
    atom = diz_type_atom (diz, pos);
    if (atom == CTX_ATOM_STARTGROUP)
      {
        level--;
      }
    else if (atom == CTX_ATOM_ENDGROUP)
      {
        level++;
      }
    else
    {
    }
  }
  while (atom == CTX_ATOM_STARTGROUP ||
         atom == CTX_ATOM_LAYOUTBOX ||
         atom == CTX_ATOM_NEWPAGE)
  {
    pos--;
    atom = diz_type_atom (diz, pos);
  }
  if (level < start_level || pos < 0)
  {
     return -1;
  }
  return pos;
}

int
diz_next_sibling (Diz *diz, int i)
{
  int start_level = 0;
  int level = 0;

  int atom;
 
  i++;
  atom = diz_type_atom (diz, i);
  if (atom == CTX_ATOM_ENDGROUP)
  {
    return -1;
  }
  if (atom == CTX_ATOM_STARTGROUP)
  {
    level++;

    while (level > start_level && i < diz->count)
    {
      i++;
      atom = diz_type_atom (diz, i);
      if (atom == CTX_ATOM_STARTGROUP)
      {
        level++;
      }
      else if (atom == CTX_ATOM_ENDGROUP)
      {
        level--;
      }
    }
    level++;
    while (atom == CTX_ATOM_ENDGROUP)
    {
       level--;
       i++;
       atom = diz_type_atom (diz, i);
    }
  }
  while (atom == CTX_ATOM_NEWPAGE)
  {
    i++;
    atom = diz_type_atom (diz, i);
  }

  if (level != start_level || i >= diz->count)
  {
     return -1;
  }
  return i;
}

void diz_set_float (Diz *diz, int item_no, const char *key, float value)
{
  char str[64];
  sprintf (str, "%f", value);
  while (str[strlen(str)-1] == '0')
    str[strlen(str)-1] = 0;
  if (str[strlen(str)-1] == '.')
    str[strlen(str)-1] = 0;
  diz_set_string (diz, item_no, key, str);
}

float diz_get_float (Diz *diz, int item_no, const char *key, float def_val)
{
   char *value = diz_get_string (diz, item_no, key);
   float ret = def_val;
   if (value)
   {
     ret = atof (value);
     free (value);
   }
   return ret;
}

int diz_get_int (Diz *diz, int no, const char *key, int def_val)
{
   char *value = diz_get_string (diz, no, key);
   int ret = def_val;
   if (value)
   {
     ret = atoi (value);
     free (value);
   }
   return ret;
}

CtxAtom diz_type_atom (Diz *diz, int i)
{
  if (text_editor)
    return CTX_ATOM_TEXT;

  char *type = diz_get_string (diz, i, "type");
  if (type)
  {
#if 0
    if (!strcmp (type, "layoutbox") && layout_config.use_layout_boxes)
    {
      free (type); return CTX_ATOM_LAYOUTBOX;
    }
    else
#endif
    if (!strcmp (type, "newpage"))
    {free (type); return CTX_ATOM_NEWPAGE;}

    else if (!strcmp (type, "startpage"))
    {free (type); return CTX_ATOM_STARTPAGE;}

    else if (!strcmp (type, "startgroup"))
    {free (type); return CTX_ATOM_STARTGROUP;}

    else if (!strcmp (type, "endgroup"))
    {free (type); return CTX_ATOM_ENDGROUP;}

    else if (!strcmp (type, "rectangle"))
    {free (type); return CTX_ATOM_RECTANGLE;}

    else if (!strcmp (type, "text"))
    {free (type); return CTX_ATOM_TEXT;}

    else if (!strcmp (type, "ctx"))
    {free (type); return CTX_ATOM_CTX;}

    else if (!strcmp (type, "file"))
    {free (type); return CTX_ATOM_FILE;}
    free (type);
  }
  return CTX_ATOM_TEXT;
}

