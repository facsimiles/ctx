#ifndef DIZ_H
#define DIZ_H

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

  int     is_text_editor;
} Diz;

Diz *diz_dir_new      (void);
void diz_dir_destroy  (Diz *diz);

void diz_dir_save     (Diz *diz);
Diz *diz_dir_new      (void);
void diz_dir_destroy  (Diz *diz);
void diz_dir_set_path (Diz *diz, const char *path);

void
diz_dir_set_path_text_editor  (Diz *diz, const char *path);

/* metadata setting API */
void diz_dir_swap         (Diz *diz, int item_no_a, int item_no_b);
void diz_dir_remove       (Diz *diz, int item_no);

// XXX : currently does not work properly with multi-keys
void diz_dir_unset        (Diz *diz, int item_no, const char *key);
void diz_dir_unset_value  (Diz *diz, int no, const char *key, const char *value);
int  diz_dir_insert       (Diz *diz, int pos, const char *item_name);

void diz_dir_set_name     (Diz *diz, int item_no, const char *new_name);

void diz_dir_set_string   (Diz *diz, int item_no, const char *key, const char *value);
void diz_dir_add_string   (Diz *diz, int item_no, const char *key, const char *value);
void diz_dir_set_float    (Diz *diz, int item_no, const char *key, float value);

void diz_dir_add_string_unique (Diz *diz, int item_no, const char *key, const char *value);

/* metadata query API */

int     diz_dir_count         (Diz *diz);
int     diz_dir_name_to_no    (Diz *diz, const char *name);

char   *diz_dir_get_name      (Diz *diz, int item_no);

int     diz_dir_key_count     (Diz *diz, int item_no);
char   *diz_dir_key_name      (Diz *diz, int item_no, int no);
char   *diz_dir_get_string    (Diz *diz, int item_no, const char *key);
float   diz_dir_get_float     (Diz *diz, int item_no, const char *key, float def_val);
int     diz_dir_get_int       (Diz *diz, int item_no, const char *key, int def_val);


int     diz_dir_value_count   (Diz *diz, int item_no, const char *key);
char   *diz_dir_get_string_no (Diz *diz, int item_no, const char *key, int value_no);

////////////////////////////

CtxAtom diz_dir_type_atom          (Diz *diz, int item_no);
int     diz_dir_measure_chunk      (Diz *diz, int item_no);
int     diz_dir_item_get_level     (Diz *diz, int item_no);
int     diz_dir_get_parent         (Diz *diz, int item_no);

int     diz_dir_ancestor_is_folded (Diz *diz, int item_no);
int     diz_dir_has_children       (Diz *diz, int item_no);

int     diz_dir_prev               (Diz *diz, int item_no);
int     diz_dir_next               (Diz *diz, int item_no);
int     diz_dir_prev_sibling       (Diz *diz, int item_no);
int     diz_dir_next_sibling       (Diz *diz, int item_no);


void    diz_dir_dump               (Diz *diz);

// should be internal only?
void diz_dir_dirt (Diz *diz);



#endif


