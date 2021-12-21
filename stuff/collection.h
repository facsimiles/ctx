#ifndef COLLECTION_H
#define COLLECTION_H

typedef struct Collection {
  char   *path;
  char   *title;
  int     count;

  char   *metadata;
  int     metadata_len;
  long    metadata_size;
  char   *metadata_path;
  char   *metadata_cache;
  int     metadata_cache_no;
} Collection;

void metadata_load (Collection *collection, const char *path, int text_file);



void dir_mkdir_ancestors (const char *path, unsigned int mode);
void metadata_save (Collection *collection);
int metadata_count (Collection *collection);
char *metadata_get_name_escaped (Collection *collection, int no);
char *metadata_get_name (Collection *collection, int no);
char *metadata_escape_item (const char *item);
int metadata_item_to_no (Collection *collection, const char *item);
int metadata_item_key_count (Collection *collection, int no);
char *metadata_key_name (Collection *collection, int ino, int no);
char *metadata_get_string (Collection *collection, int no, const char *key);
char *metadata_get (Collection *collection, int no, const char *key);
float metadata_get_float (Collection *collection, int no, const char *key, float def_val);
int metadata_get_int (Collection *collection, int no, const char *key, int def_val);
void metadata_swap (Collection *collection, int no_a, int no_b);
void metadata_remove (Collection *collection, int no);
void metadata_unset (Collection *collection, int no, const char *key);
int metadata_insert (Collection *collection, int pos, const char *item);
void metadata_set_name (Collection *collection, int pos, const char *new_name);
void metadata_add (Collection *collection, int no, const char *key, const char *value);
void metadata_set (Collection *collection, int no, const char *key, const char *value);
void metadata_set_float (Collection *collection, int no, const char *key, float value);

#endif
