#ifndef COLLECTION_H
#define COLLECTION_H

// doesnt belong here - it is global for stuff...
//
typedef enum CtxAtom {
 CTX_ATOM_TEXT = 0,
 CTX_ATOM_LAYOUTBOX,
 CTX_ATOM_STARTPAGE, // for layout purposes, alse causes a newpage
 CTX_ATOM_NEWPAGE,
 CTX_ATOM_STARTGROUP,
 CTX_ATOM_ENDGROUP,
 CTX_ATOM_RECTANGLE,
 CTX_ATOM_CTX,
 CTX_ATOM_FILE,
} CtxAtom;

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

  char  **cache;
  int     cache_size;
} Collection;

void metadata_load        (Collection *collection,
                           const char *path,
                           int text_file);
void metadata_save        (Collection *collection,
                           int text_file);


void dir_mkdir_ancestors   (const char *path,
                            unsigned int mode);
int metadata_count         (Collection  *collection);
char *metadata_get_name_escaped (Collection *collection,
                                 int         no);
char *metadata_get_name    (Collection *collection,
                            int         no);
char *metadata_escape_item (const char *item);
int metadata_item_to_no    (Collection *collection,
                            const char *item);
int metadata_item_key_count(Collection *collection,
                            int         no);
char *metadata_key_name    (Collection *collection,
                            int ino,
                            int no);
char *metadata_get_string  (Collection *collection,
                            int no,
                            const char *key);
char *metadata_get         (Collection *collection,
                            int no,
                            const char *key);
float metadata_get_float   (Collection *collection,
                            int no,
                            const char *key, float def_val);
int metadata_get_int       (Collection *collection,
                            int no,
                            const char *key, int def_val);
void metadata_swap         (Collection *collection,
                            int no_a,
                            int no_b);
void metadata_remove       (Collection *collection,
                            int no);
void metadata_unset        (Collection *collection,
                            int no,
                            const char *key);
int metadata_insert        (Collection *collection,
                            int pos,
                            const char *item);
void metadata_set_name     (Collection *collection, int no, const char *new_name);
void metadata_add          (Collection *collection, int no, const char *key, const char *value);
void metadata_set          (Collection *collection, int no, const char *key, const char *value);
void metadata_set_float    (Collection *collection, int no, const char *key, float value);

void collection_set_path (Collection *collection,
                          const char *path,
                          const char *title);
int collection_item_get_level (Collection *collection, int no);
int collection_get_parent     (Collection *collection, int no);

CtxAtom collection_item_get_type_atom (Collection *collection, int i);
int     collection_measure_chunk      (Collection *collection, int no);

int collection_ancestor_folded (Collection *collection, int no);
int collection_has_children    (Collection *collection, int no);
void collection_update_files   (Collection *collection);

#endif


