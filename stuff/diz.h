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
 CTX_ATOM_SYMLINK,
} CtxAtom;

typedef struct Diz {
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

  int     dirty;
} Diz;

Diz *diz_new      (void);
void diz_destroy  (Diz *diz);

void diz_save     (Diz *diz, int text_file);
Diz *diz_new      (void);
void diz_destroy  (Diz *diz);
void diz_set_path (Diz *diz, const char *path, const char *title);


void
diz_set_path_text_editor  (Diz *diz,
                     const char *path,
                     const char *title);

/* metadata setting API */
void diz_swap         (Diz *diz, int item_no_a, int item_no_b);
void diz_remove       (Diz *diz, int item_no);

// XXX : currently does not work properly with multi-keys
void diz_unset        (Diz *diz, int item_no, const char *key);
void diz_unset_value  (Diz *diz, int no, const char *key, const char *value);
int  diz_insert       (Diz *diz, int pos, const char *item_name);

void diz_set_name     (Diz *diz, int item_no, const char *new_name);

void diz_set_string   (Diz *diz, int item_no, const char *key, const char *value);
void diz_add_string   (Diz *diz, int item_no, const char *key, const char *value);
void diz_set_float    (Diz *diz, int item_no, const char *key, float value);

void diz_add_string_unique (Diz *diz, int item_no, const char *key, const char *value);

/* metadata query API */

int     diz_count         (Diz *diz);
int     diz_name_to_no    (Diz *diz, const char *name);

char   *diz_get_name      (Diz *diz, int item_no);

int     diz_key_count     (Diz *diz, int item_no);
char   *diz_key_name      (Diz *diz, int item_no, int no);
char   *diz_get_string    (Diz *diz, int item_no, const char *key);
float   diz_get_float     (Diz *diz, int item_no, const char *key, float def_val);
int     diz_get_int       (Diz *diz, int item_no, const char *key, int def_val);


int     diz_value_count   (Diz *diz, int item_no, const char *key);
char   *diz_get_string_no (Diz *diz, int item_no, const char *key, int value_no);

////////////////////////////

CtxAtom diz_type_atom          (Diz *diz, int item_no);
int     diz_measure_chunk      (Diz *diz, int item_no);
int     diz_item_get_level     (Diz *diz, int item_no);
int     diz_get_parent         (Diz *diz, int item_no);

int     diz_ancestor_is_folded (Diz *diz, int item_no);
int     diz_has_children       (Diz *diz, int item_no);

int     diz_prev               (Diz *diz, int item_no);
int     diz_next               (Diz *diz, int item_no);
int     diz_prev_sibling       (Diz *diz, int item_no);
int     diz_next_sibling       (Diz *diz, int item_no);


void    diz_dump               (Diz *diz);

// should be internal only?
void diz_dirt (Diz *diz);
#endif


