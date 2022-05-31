#include "collection.h"
#include "ctx.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>

void metadata_wipe_cache (Collection *collection, int full)
{
  if (full)
    collection->metadata_cache_no = -3;
  if (collection->cache)
  {
    for (int i = 0; i < collection->cache_size; i++)
    {
       if (collection->cache[i])
       {
        free (collection->cache[i]);
        collection->cache[i] = NULL;
       }
    }
    free (collection->cache);
    collection->cache = NULL;
    collection->cache_size = 0;
  }
}

void metadata_load (Collection *collection, const char *path, int text_editor_mode)
{
  collection->metadata_cache_no = -3;
  collection->metadata_cache = NULL;
  if (collection->metadata_path) free (collection->metadata_path);
  collection->metadata_path = malloc (strlen (path) + 20);
  if (collection->metadata)
    free (collection->metadata);
  collection->metadata = NULL;
  collection->metadata_len = 0;
  collection->metadata_size = 0;
  if (text_editor_mode)
  {
    uint8_t *contents = NULL;
    long length = 0;

    strcpy (collection->metadata_path, path);
    ctx_get_contents (collection->metadata_path, &contents, &length);
    if (contents)
    {
       CtxString *line = ctx_string_new ("");
       int i;
       for (i = 0; contents[i]; i++)
       {
         char p = contents[i];
         if (p == '\n')
         {
           metadata_insert (collection, -1, line->str);
           ctx_string_set (line, "");
         }
         else
         {
           ctx_string_append_byte (line, p);
         }
       }
       if (line->str[0])
         metadata_insert (collection, -1, line->str);
       free (contents);
    }

  }
  else
  {
    snprintf (collection->metadata_path, strlen(path)+20, "%s/.ctx/index", path);
  ctx_get_contents (collection->metadata_path, (uint8_t**)&collection->metadata, &collection->metadata_size);
  collection->metadata_len = (int)collection->metadata_size;
  }
  metadata_wipe_cache (collection, 1);
}

void dir_mkdir_ancestors (const char *path, unsigned int mode)
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

#if 1
void metadata_save (Collection *collection, int text_editor_mode)
{
  if (!collection->metadata_path) return;
  dir_mkdir_ancestors (collection->metadata_path, 0777);

  FILE *file = fopen (collection->metadata_path, "w");
  if (text_editor_mode)
  {
    int count = metadata_count (collection);
    for (int i = 0; i < count; i++)
    {
      char *line = metadata_get_name (collection, i);
      fwrite (line, strlen (line), 1, file);
      fwrite ("\n", 1, 1, file);
      free (line);
    }
  }
  else
  {
    fwrite (collection->metadata, collection->metadata_len, 1, file);
  }
  fclose (file);
}
#endif

int metadata_count (Collection *collection)
{
  const char *m = collection->metadata;
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

char *metadata_get_name_escaped (Collection *collection, int no)
{
  const char *m = collection->metadata;
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

static char *metadata_find_no (Collection *collection, int no)
{
  char *m = collection->metadata;
  int count = 0;

  if (no == -2)
  {
    return m;
  }

  int update_cache = 1;
#if 1
  if (collection->metadata_cache_no == no) return collection->metadata_cache;
  if (collection->metadata_cache_no < no && collection->metadata_cache_no > 0)
  {
    if (no - collection->metadata_cache_no < 4) /* keep a small look-ahead, much of the
                                       time */
      update_cache = 0;
    m = collection->metadata_cache;
    count = collection->metadata_cache_no;
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
          collection->metadata_cache = m;
          collection->metadata_cache_no = no;
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

char *metadata_get_name (Collection *collection, int no)
{
  if (collection->cache)
  { if (collection->cache[no])
      return strdup (collection->cache[no]);
  }
  else
  {
    int new_count = metadata_count (collection);
    collection->cache_size = new_count;
    collection->cache = calloc (sizeof(void*), new_count);
  }

  if (no == -2)
  {
     return metadata_get_string (collection, no, "title");
  }


  /* this makes use reuse the cache */
  const char *m = metadata_find_no (collection, no);
  if (!m) return NULL;
  m--;
  m--;
  while (m[0]!='\n' && m != collection->metadata)m--;
  if (m !=collection->metadata) m++;

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
            //case '-': // soft-hyphen
            //  ctx_string_append_byte (str, '.');
            //  break;
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

  collection->cache[no]=strdup(str->str);
  return ctx_string_dissolve (str);
}


static int _metadata_metalen (Collection *collection, const char *m)
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

char *metadata_escape_item (const char *item) // XXX expand with length to
                                              // handle \0 and
                                              // thus arbitrary blobs?
{
  if (item[0]==0)return strdup("\\0");
  // this is kind of a hack - but it avoids
  // bare newlines - which some parts of the code
  // still has trouble with
  // XXX XXX XXX
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

static const char *metadata_find_item (Collection *collection, const char *item)
{
  char *escaped_item = metadata_escape_item (item);
  int item_len = strlen (escaped_item);
  const char *m = collection->metadata;

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

int metadata_item_to_no (Collection *collection, const char *item)
{
  char *escaped_item = metadata_escape_item (item);
  int item_len = strlen (escaped_item);
  const char *m = collection->metadata;
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

int metadata_item_key_count (Collection *collection, int no)
{
  const char *m = metadata_find_no (collection, no);
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

char *metadata_key_name (Collection *collection, int ino, int no)
{
  const char *m = metadata_find_no (collection, ino);
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

char *metadata_get_string (Collection *collection, int no, const char *key)
{
  const char *m = metadata_find_no (collection, no);
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

char *metadata_get (Collection *collection, int no, const char *key)
{
  return metadata_get_string (collection, no, key);
}

float metadata_get_float (Collection *collection, int no, const char *key, float def_val)
{
   char *value = metadata_get_string (collection, no, key);
   float ret = def_val;//-1234.0f;
   if (value)
   {
     ret = atof (value);
     free (value);
   }
   return ret;
}

int metadata_get_int (Collection *collection, int no, const char *key, int def_val)
{
   char *value = metadata_get_string (collection, no, key);
   int ret = def_val;
   if (value)
   {
     ret = atoi (value);
     free (value);
   }
   return ret;
}

void metadata_swap (Collection *collection, int no_a, int no_b)
{
   if (no_a == no_b) return;
   if (no_b < no_a)
   {
     int tmp = no_a;
     no_a = no_b; no_b = tmp;
   }

   char *a_name = metadata_get_name_escaped (collection, no_a);
   char *b_name = metadata_get_name_escaped (collection, no_b);
   int a_name_len = strlen (a_name);
   int b_name_len = strlen (b_name);
   const char *a_meta = metadata_find_no (collection, no_a);
   const char *b_meta = metadata_find_no (collection, no_b);

   int a_start = (a_meta - collection->metadata) - a_name_len - 1;
   int a_len = a_name_len + 1 + _metadata_metalen (collection, a_meta);


   int b_start = (b_meta - collection->metadata) - b_name_len - 1;
   int b_len = b_name_len + 1 + _metadata_metalen (collection, b_meta);

   char *a_temp = malloc (a_len);
   char *b_temp = malloc (b_len);

   memcpy (a_temp, collection->metadata + a_start, a_len);
   memcpy (b_temp, collection->metadata + b_start, b_len);

   memmove (collection->metadata + b_start,
            collection->metadata + b_start + b_len, collection->metadata_len - b_start - b_len);
   collection->metadata_len -= b_len;
   memmove (collection->metadata + a_start,
            collection->metadata + a_start + a_len, collection->metadata_len - a_start - a_len);
   collection->metadata_len -= a_len;

   b_start -= a_len;
   memmove (collection->metadata + b_start + a_len,
            collection->metadata + b_start, collection->metadata_len - b_start);
   memcpy (collection->metadata + b_start, a_temp, a_len);
   collection->metadata_len += a_len;
   memmove (collection->metadata + a_start + b_len,
            collection->metadata + a_start,
            collection->metadata_len - a_start);
   memcpy (collection->metadata + a_start, b_temp, b_len);
   collection->metadata_len += b_len;

   free (a_temp);
   free (b_temp);

   collection->metadata_cache_no = -3;
   metadata_wipe_cache (collection, 1);
}


void metadata_remove (Collection *collection, int no)
{
   char *a_name = metadata_get_name_escaped (collection, no);
   int a_name_len = strlen (a_name);
   const char *a_meta = metadata_find_no (collection, no);

   int a_start = (a_meta - collection->metadata) - a_name_len - 1;
   int a_len = a_name_len + 1 + _metadata_metalen (collection, a_meta);

   memmove (collection->metadata + a_start,
            collection->metadata + a_start + a_len,
            collection->metadata_len - a_start - a_len);
   collection->metadata_len -= a_len;
   collection->metadata[collection->metadata_len]=0;
   metadata_wipe_cache (collection, 1);
   collection_update_files (collection);
}

void metadata_unset (Collection *collection, int no, const char *key)
{
  const char *a_meta = metadata_find_no (collection, no);
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
   
   int a_start = prop_start - collection->metadata;
   //int a_len = a_name_len + 1 + _metadata_metalen (a_meta);

   memmove (collection->metadata + a_start,
            collection->metadata + a_start + a_len, collection->metadata_len - a_start - a_len);
   collection->metadata_len -= a_len;
   collection->metadata[collection->metadata_len]=0;
   metadata_wipe_cache (collection, 1);
}

static void _metadata_insert (Collection *collection, int pos, const char *data, int len)
{
  int wipe_full = (pos != collection->metadata_len);
  if (collection->metadata_len + len >= collection->metadata_size - 1)
  {
     collection->metadata_size = collection->metadata_len + len + 1024;
     collection->metadata = realloc (collection->metadata, collection->metadata_size);
  }
  memmove (collection->metadata + pos + len, collection->metadata + pos, collection->metadata_len - pos);
  memcpy (collection->metadata + pos, data, len);
  collection->metadata_len += len;
  collection->metadata[collection->metadata_len] = 0;
  metadata_wipe_cache (collection, wipe_full);
}

int metadata_insert (Collection *collection, int pos, const char *item)
{
  const char *m = NULL;
  if (pos == -1)
    pos = collection->metadata_len;
  else
    m = metadata_find_no (collection, pos);

  if (m)
  {
    char *name = metadata_get_name_escaped (collection, pos);
    m -= strlen (name) + 1;
    free (name);
  }
  else
  {
    m = collection->metadata + collection->metadata_len;
  }
  char *escaped_item = metadata_escape_item (item);
  char tmp[strlen (escaped_item) + 3];
  snprintf (tmp, sizeof(tmp), "%s\n", escaped_item);
  free (escaped_item);

  _metadata_insert (collection, m-collection->metadata, tmp, strlen (tmp));
  return pos;
}

void metadata_set_name (Collection *collection, int pos, const char *new_name)
{
  // TODO : if it is a file, do a corresponding rename!
  //
  if (pos == -2)
  {
     metadata_set (collection, pos, "title", new_name);
     return;
  }

  char *m = metadata_find_no (collection, pos);
  if (m)
  {
    char *name = metadata_get_name_escaped (collection, pos);
    int name_len = strlen (name);
    free (name);

    m -= name_len + 1;
    memmove (m, m + name_len + 1, collection->metadata_len - (m-collection->metadata) - name_len - 1);
    collection->metadata_len -= name_len + 1;
  }
  else
  {
    return;
  }

  char *new_escaped = metadata_escape_item (new_name);
  char tmp[strlen (new_escaped) + 3];
  snprintf (tmp, sizeof(tmp), "%s\n", new_escaped);
  free (new_escaped);

  _metadata_insert (collection, m-collection->metadata, tmp, strlen (tmp));
}

void metadata_add (Collection *collection, int no, const char *key, const char *value)
{
  CtxString *str = ctx_string_new ( " ");
  metadata_unset (collection, no, key);

  const char *m = metadata_find_no (collection, no);
  int offset = collection->metadata_len;
  if (m)
  {
    while (m[0] == ' ')
    {
       while (m[0] != '\n') m++;
       if (m[0] == '\n') m++;
    }
    offset = m - collection->metadata;
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

  _metadata_insert (collection, offset, str->str, str->length);
}

void metadata_set (Collection *collection, int no, const char *key, const char *value)
{
  metadata_unset (collection, no, key);
  metadata_add (collection, no, key, value);
}

void metadata_set_float (Collection *collection, int no, const char *key, float value)
{
  char str[64];
  sprintf (str, "%f", value);
  while (str[strlen(str)-1] == '0')
    str[strlen(str)-1] = 0;
  if (str[strlen(str)-1] == '.')
    str[strlen(str)-1] = 0;
  metadata_set (collection, no, key, str);
}

void metadata_dump (Collection *collection)
{
  int item_count = metadata_count (collection);
  fprintf (stdout, "item count: %i\n", item_count);
  int indent = 0;

  for (int i = 0; i < item_count; i ++)
  {
    char *item_name = metadata_get_name (collection, i);
    for (int i = 0; i < indent; i++) fprintf (stdout, " ");
    fprintf (stdout, "%s     :%i,%i\n", item_name, i, metadata_item_to_no (collection, item_name));
    int keys = metadata_item_key_count (collection, i);
    for (int k = 0; k < keys; k ++)
    {
      char *key_name = metadata_key_name (collection, i, k);
      for (int i = 0; i < indent; i++) fprintf (stdout, " ");
      fprintf (stdout, "   %s=%s\n", key_name,
                      metadata_get_string (collection, i, key_name));
      if (!strcmp (key_name, "type") &&
          !strcmp(metadata_get_string (collection, i, key_name), "startgroup"))
              indent +=4;
      else if (!strcmp (key_name, "type") &&
          !strcmp(metadata_get_string (collection, i, key_name), "endgroup"))
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

void
collection_update_files (Collection *collection)
{
  struct  dirent **namelist = NULL;
  int     n;

  n = scandir (collection->path, &namelist, NULL, custom_sort);

  CtxList *to_add = NULL; // XXX a temporary list here might be redundant
  for (int i = 0; i < n; i++)
  {
    int found = 0;
    const char *name = namelist[i]->d_name;
    if (metadata_item_to_no (collection, name)>=0)
       found = 1;
    if (!found && (name[0] != '.'))
    {
      ctx_list_prepend (&to_add, strdup (name));
    }
  }
  while (to_add)
  {
    char *name = to_add->data;
    int n = metadata_insert(collection, -1, name);
    metadata_set (collection, n, "type", "file");
    ctx_list_remove (&to_add, name);
    free (name);
  }

  collection->count = metadata_count (collection);
  CtxList *to_remove = NULL;
  for (int i = 0; i < collection->count; i++)
  {
    int atom = collection_item_get_type_atom (collection, i);
    if (atom == CTX_ATOM_FILE)
    {
      char *name = metadata_get_name (collection, i);
      char *path = ctx_strdup_printf ("%s/%s", collection->path, name);
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
    if ((no = metadata_item_to_no (collection, name))>=0)
    {
      metadata_remove (collection, no);
    }
    ctx_list_remove (&to_remove, name);
    free (name);
  }

  collection->count = metadata_count (collection);

  while (n--)
    free (namelist[n]);
  free (namelist);

  // TODO fully remove non-existent collection when empty?
  //if (added)
  //  metadata_dirt();
}


void
collection_set_path (Collection *collection,
                     const char *path,
                     const char *title)
{
  char *resolved_path = realpath (path, NULL);
  char *title2 = NULL;

  if (title) title2 = strdup (title);
  else title2 = strdup (path);


  if (collection->path)
    free (collection->path);
  collection->path = resolved_path;
  if (collection->title)
    free (collection->title);
  collection->title = title2;

  metadata_load (collection, resolved_path, text_editor);
  collection->count = metadata_count (collection);

  if (text_editor)
    return;

  collection_update_files (collection);
}

int collection_item_get_level (Collection *collection, int no)
{
  int level = 0;
  for (int i = 0; i <= no; i++)
  {
    int atom = collection_item_get_type_atom (collection, i);
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


int collection_get_parent (Collection *collection, int no)
{
  int level = 1;
  CtxAtom atom;
  while (level > 0 && no >= 0)
  {
    no--;
    atom = collection_item_get_type_atom (collection, no);
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

CtxAtom collection_item_get_type_atom (Collection *collection, int i)
{
  if (text_editor)
    return CTX_ATOM_TEXT;

  char *type = metadata_get_string (collection, i, "type");
  if (type)
  {
#if 0
    if (!strcmp (type, "layoutbox") && layout_config.use_layout_boxes)
    {
      free (type); return CTX_ATOM_LAYOUTBOX;
    }
    else
#endif
    if (!strcmp (type, "newpage"))         {free (type); return CTX_ATOM_NEWPAGE;}
    else if (!strcmp (type, "startpage"))  {free (type); return CTX_ATOM_STARTPAGE;}
    else if (!strcmp (type, "startgroup")) {free (type); return CTX_ATOM_STARTGROUP;}
    else if (!strcmp (type, "endgroup"))   {free (type); return CTX_ATOM_ENDGROUP;}
    else if (!strcmp (type, "rectangle"))  {free (type); return CTX_ATOM_RECTANGLE;}
    else if (!strcmp (type, "text"))       {free (type); return CTX_ATOM_TEXT;}
    else if (!strcmp (type, "ctx"))        {free (type); return CTX_ATOM_CTX;}
    else if (!strcmp (type, "file"))       {free (type); return CTX_ATOM_FILE;}
    free (type);
  }
  return CTX_ATOM_TEXT;
}

int collection_measure_chunk (Collection *collection, int no)
{
  int count = 1;
  int self_level = collection_item_get_level (collection, no);
  int level;

  do {
    no ++;
    level = collection_item_get_level (collection, no);
    if (level > self_level) count++;
  } while (level > self_level);
  if (count > 1) count ++;
  
  return count;
}

int collection_ancestor_folded (Collection *collection, int no)
{
  int iter = collection_get_parent (collection, no);
  while (iter != 0)
  {
    if (metadata_get_int (collection, iter+1, "folded", 0))
    {
      return 1;
    }
    iter = collection_get_parent (collection, iter);
  }
  return 0;
}

int collection_has_children (Collection *collection, int no)
{
  CtxAtom  atom = collection_item_get_type_atom (collection, no+1);
  if (atom == CTX_ATOM_STARTGROUP)
    return 1;
  return 0;
}

