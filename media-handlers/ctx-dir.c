//
// todo
//   custom order , size / time orders
//   layout view
//      paginated mode
//      
//      flow templates
//
// a file called ctx-template , searched in cur dir,
// ancestor dirs, and ctx folder.
// inserting a template, should be followed by newpage
// permitting chapter start to be template,newpage,template full-chapter,
// causing only the first bit of chapter to be styled this way.
//
// for runs of templates - instantiate or embed the following style of pages
//
// newpage can be issued or done manually
//

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <math.h>
#include <ctype.h>
#include "ctx.h"
#include "itk.h"
#include <signal.h>

typedef enum ImageZoom
{
  IMAGE_ZOOM_NONE = 0,
  IMAGE_ZOOM_STRETCH,
  IMAGE_ZOOM_FIT,
  IMAGE_ZOOM_FILL,
} ImageZoom;

typedef struct LayoutConfig
{
  int stack_horizontal;
  int stack_vertical;
  int fill_width;  // of view
  int fill_height; // of view
  int label;
  int zoom;

  // sort order

  float width; // in em
  float height; // in em
  float border; // in .. em ?
  float margin; // .. folded for inner

  int   fixed_size;
  int   fixed_pos;
  int   list_data;

  float padding_left;
  float padding_right;
  float padding_top;
  float padding_bottom;
  float level_indent;
  int use_layout_boxes;
  int outliner;
  int codes;

  int hide_non_file;
} LayoutConfig;

LayoutConfig layout_config = {
  1,1,0,0, 1,0,
  4, 4, 0, 0.5,
  0, 0, 0, 3,
  1
};

int show_keybindings = 0;
int focused_no = -1;
int layout_page_no = 0;
int layout_show_page = 0;
int layout_last_page = 0;
float dir_scale = 1.0f;
int tool_no = 0;


// TODO global variables that should be enclosed in a struct

typedef struct _CtxSHA1 CtxSHA1;
CtxSHA1 *ctx_sha1_new (void);
void ctx_sha1_free    (CtxSHA1 *sha1);
int  ctx_sha1_process (CtxSHA1 *sha1, const unsigned char * msg, unsigned long len);
int  ctx_sha1_done    (CtxSHA1 *sha1, unsigned char *out);

extern Ctx *ctx;

typedef struct Item {
  char *label;
  char *val;
} Item;

typedef struct Files {
  char   *path;
  char   *title;
  struct  dirent **namelist;
  int     n;

  char  **items;
  int     count;


} Files;
#define PATH_SEP "/"

#define TEXT_EDIT_OFF                   -10
#define TEXT_EDIT_FIND_CURSOR_FIRST_ROW -9
#define TEXT_EDIT_FIND_CURSOR_LAST_ROW  -8

CtxList *thumb_queue = NULL;
typedef struct ThumbQueueItem
{
  char *path;
  char *thumbpath;
} ThumbQueueItem;

static void queue_thumb (const char *path, const char *thumbpath)
{
  ThumbQueueItem *item;
  for (CtxList *l = thumb_queue; l; l=l->next)
  {
    item = l->data;
    if (!strcmp (item->path, path))
    {
      return;
    }
    if (!strcmp (item->thumbpath, thumbpath))
    {
      return;
    }
  }
  item = calloc (sizeof (ThumbQueueItem), 1);
  item->path = strdup (path);
  item->thumbpath = strdup (thumbpath);
  ctx_list_append (&thumb_queue, item);
}

char *ctx_get_thumb_path (const char *path) // XXX add dim as arg?
{
  char *hex="0123456789abcdefghijkl";
  unsigned char path_hash[40];
  char path_hash_hex[41];
  CtxSHA1 *sha1 = ctx_sha1_new ();
  char *rp = realpath (path, NULL);
  ctx_sha1_process (sha1, (uint8_t*)"file://", strlen ("file://"));
  ctx_sha1_process (sha1, (uint8_t*)path, strlen (rp));
  ctx_sha1_done(sha1, path_hash);
  ctx_sha1_free (sha1);
  free (rp);
  for (int j = 0; j < 20; j++)
  {
    path_hash_hex[j*2+0]=hex[path_hash[j]/16];
    path_hash_hex[j*2+1]=hex[path_hash[j]%16];
  }
  path_hash_hex[40]=0;
  return ctx_strdup_printf ("%s/.ctx-thumbnails/%s", getenv ("HOME"), path_hash_hex);
}

void ui_queue_thumb (const char *path)
{
  char *rp = realpath (path, NULL);
  char *thumb_path = ctx_get_thumb_path (rp);
  queue_thumb (rp, thumb_path);
  free (rp);
  free (thumb_path);
}

void viewer_load_path (const char *path, const char *name);

static int custom_sort (const struct dirent **a,
                        const struct dirent **b)
{
  if ((*a)->d_type != (*b)->d_type)
  {
    return ((*a)->d_type - (*b)->d_type);
  }
  return strcmp ((*a)->d_name , (*b)->d_name);
}

#define METADATA_NOTEST 1
#include "metadata/metadata.c"

typedef enum {
  CTX_DIR_LAYOUT,
  CTX_DIR_IMAGE
}
CtxDirView;

static int layout_find_item = -1;
ITK *itk = NULL;

static void set_layout (CtxEvent *e, void *d1, void *d2)
{
  if (e)
  ctx_set_dirty (e->ctx, 1);
  layout_config.stack_horizontal = 1;
  layout_config.stack_vertical = 1;
  layout_config.fill_width = 0;
  layout_config.fill_height = 0;
  layout_config.zoom = 0;
  layout_config.width = 5;
  layout_config.height = 5;
  layout_config.border = 0;
  layout_config.margin = 0.0;
  layout_config.fixed_size = 0;
  layout_config.fixed_pos = 0;
  layout_config.label = 1;
  layout_config.list_data = 0;
  layout_config.padding_left = 0.5f;
  layout_config.padding_right = 0.5f;
  layout_config.padding_top = 0.5f;
  layout_config.padding_bottom = 0.5f;

  layout_config.use_layout_boxes = 1;
  layout_config.level_indent = 2.0;
  layout_config.outliner = 0;
  layout_config.codes = 0;

  layout_config.hide_non_file = 0;
  if (focused_no>=0)
  {
    layout_find_item = focused_no;
    if (itk)
      itk->focus_no = -1;
  }
};

static void set_outline (CtxEvent *e, void *d1, void *d2)
{
  set_layout (e, d1, d2);
  layout_config.outliner = 1;
}

static void set_list (CtxEvent *e, void *d1, void *d2)
{
  ctx_set_dirty (e->ctx, 1);
  set_layout (e, d1, d2);
  layout_config.width = 2;
  layout_config.height = 2;
  layout_config.fixed_size = 1;
  layout_config.fixed_pos = 1;
  layout_config.stack_horizontal = 0;
  layout_config.label = 0;
  layout_config.list_data = 1;
  layout_config.padding_left = 0.1f;
  layout_config.padding_right = 0.1f;
  layout_config.padding_top = 0.1f;
  layout_config.padding_bottom = 0.1f;
  layout_config.use_layout_boxes = 0;
  layout_config.hide_non_file = 1;
}

static void set_grid (CtxEvent *e, void *d1, void *d2)
{
  set_layout (e, d1, d2);
  layout_config.fixed_size = 1;
  layout_config.fixed_pos = 1;
  layout_config.stack_horizontal = 1;
  layout_config.stack_vertical = 1;
  layout_config.label = 1;
  layout_config.use_layout_boxes = 0;
  layout_config.hide_non_file = 1;
}

typedef enum CtxBullet {
  CTX_BULLET_NONE = 0,
  CTX_BULLET_BULLET,
  CTX_BULLET_NUMBERS,
  CTX_BULLET_TODO,
  CTX_BULLET_DONE,
} CtxBullet;

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

CtxAtom item_get_type_atom (int i)
{
char *type = metadata_key_string2 (i, "type");
if (type)
{
   if (!strcmp (type, "ctx/layoutbox") && layout_config.use_layout_boxes)
     return CTX_ATOM_LAYOUTBOX;
   else if (!strcmp (type, "ctx/newpage"))    return CTX_ATOM_NEWPAGE;
   else if (!strcmp (type, "ctx/startpage"))  return CTX_ATOM_STARTPAGE;
   else if (!strcmp (type, "ctx/startgroup")) return CTX_ATOM_STARTGROUP;
   else if (!strcmp (type, "ctx/endgroup"))   return CTX_ATOM_ENDGROUP;
   else if (!strcmp (type, "ctx/rectangle"))  return CTX_ATOM_RECTANGLE;
   else if (!strcmp (type, "ctx/text"))       return CTX_ATOM_TEXT;
   else if (!strcmp (type, "ctx/ctx"))        return CTX_ATOM_CTX;
   else if (!strcmp (type, "ctx/file"))       return CTX_ATOM_FILE;
   free (type);
}
  return CTX_ATOM_TEXT;
}

Files file_state;
Files *files = &file_state;
static int metadata_dirty = 0;


void dm_set_path (Files *files, const char *path, const char *title)
{
  char *resolved_path = realpath (path, NULL);
  char *title2 = NULL;

  if (title) title2 = strdup (title);

  if (files->path)
    free (files->path);
  files->path = resolved_path;
  if (files->title)
    free (files->title);
  files->title = title2;

  if (files->namelist)
    free (files->namelist);
  files->namelist = NULL;
  files->n = scandir (files->path, &files->namelist, NULL, custom_sort);
  metadata_load (resolved_path);

  if (files->items)
  {
     for (int i = 0; i < files->count; i++)
       free (files->items[i]);
     free (files->items);
  }
  int meta_count = metadata_count ();
  files->count = 0;
  files->items = calloc (sizeof (char*), files->n + meta_count + 1);
  int pos;
  for (pos = 0; pos < meta_count; pos++)
  {
    files->items[files->count++] = metadata_item_name (pos);
  }

#if 0
  {
    if (0)
    {
    //set_grid (NULL, NULL, NULL);
    char *v_type = metadata_key_string (".", "view");
    if (v_type)
    {
      if (!strcmp (v_type, "layout"))
      {
         set_layout (NULL, NULL, NULL);
      }
      else if (!strcmp (v_type, "list"))
      {
         set_list (NULL, NULL, NULL);
      }
      else if (!strcmp (v_type, "grid"))
      {
         set_grid (NULL, NULL, NULL);
      }
      free (v_type);
    }
    }
  }
#endif

  CtxList *to_add = NULL;
  for (int i = 0; i < files->n; i++)
  {
    int found = 0;
    const char *name = files->namelist[i]->d_name;
    if (metadata_item_to_no (name)>=0)
       found = 1;
    if (!found && (name[0] != '.'))
    {
      ctx_list_prepend (&to_add, strdup (name));
    }
  }
  int added = 0;
  while (to_add)
  {
    char *name = to_add->data;
    int n = metadata_insert(-1, name);
    metadata_set2 (n, "type", "ctx/file");
    files->items[files->count++] = name;
    ctx_list_remove (&to_add, name);
    added++;
  }
  //if (added)
  //  metadata_dirt();
}

static void save_metadata(void)
{  if (metadata_dirty)
   {
     metadata_save ();
     dm_set_path (files, files->path, files->title);
     metadata_dirty = 0;
   }

}

static void metadata_dirt(void)
{
  metadata_dirty++;
  metadata_cache_no=-3;
  metadata_save ();
//  save_metadata ();
}



#include <libgen.h>
#include <limits.h>
#include <stdlib.h>

char *get_dirname (const char *path)
{
  char *ret;
  char *tmp = strdup (path);
  ret = strdup (dirname (tmp));
  free (tmp);
  return ret;
}

char *get_basename (const char *path)
{
  char *ret;
  char *tmp = strdup (path);
  ret = strdup (basename (tmp));
  free (tmp);
  return ret;
}

const char *get_suffix (const char *path)
{
  const char *last_dot = strrchr (path, '.');
  const char *last_slash = strrchr (path, '/');

  if (last_slash > last_dot) return "";
  return last_dot + 1;
}

int viewer_no = 0;

int ctx_handle_img (Ctx *ctx, const char *path);

static int text_edit = TEXT_EDIT_OFF;
static float text_edit_desired_x = -123;

CtxString *commandline = NULL;
int        commandline_cursor_start = 0;
int        commandline_cursor_end = 0;

static inline int is_text_editing (void)
{
  return (text_edit != TEXT_EDIT_OFF) ||
         (commandline->str[0]!=0);
}



static void dir_insert (CtxEvent *e, void *d1, void *d2)
{
  metadata_insert (focused_no, "");
  text_edit = 0;
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
  e->stop_propagate = 1;
}



static int path_is_dir (const char *path)
{
  struct stat stat_buf;
  if (!path || path[0]==0) return 0;
  lstat (path, &stat_buf);
  return S_ISDIR (stat_buf.st_mode);
}

static char *dir_metadata_path (const char *path);

static CtxList *history = NULL;
static CtxList *future = NULL;

static void _set_location (const char *location)
{
  if (location[0] == '/' || location[0] == '.')
  {
    if (path_is_dir (location))
    {
      dm_set_path (files, location, NULL);
      focused_no = -1;
      layout_find_item = 0;
    }
  } else
  {
    char *path = dir_metadata_path (location);
    if (path_is_dir (path))
    {
    }
    else
    {
      mkdir (path, 0777);
    }
    dm_set_path (files, path, location);
    free (path);
    focused_no = 0;
    layout_find_item = 0;
  }
}


static void history_forward (CtxEvent *event, void *d1, void *d2)
{
  if (future == NULL)
     return;

  char *location = future->data;
  ctx_list_remove (&future, location);

  if (files->title)
    ctx_list_prepend (&history, strdup (files->title));
  else
    ctx_list_prepend (&history, strdup (files->path));

  _set_location (location);
  free (location);

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void history_back (CtxEvent *event, void *d1, void *d2)
{
  if (history == NULL)
     return;
  char *location = history->data;
  ctx_list_remove (&history, location);

  if (files->title)
    ctx_list_prepend (&future, strdup (files->title));
  else
    ctx_list_prepend (&future, strdup (files->path));

  _set_location (location);
  ctx_set_dirty (event->ctx, 1);

  free (location);
  event->stop_propagate = 1;
}


static void set_location (const char *location)
{
  while (future)
  {
    free (future->data);
    ctx_list_remove (&future, future->data);
  }
  if (files->path)
  {
  if (files->title)
    ctx_list_prepend (&history, strdup (files->title));
  else
    ctx_list_prepend (&history, strdup (files->path));
  }
  _set_location (location);
}


static void dir_go_parent (CtxEvent *e, void *d1, void *d2)
{
  char *old_path = strdup (files->path);
  char *new_path = get_dirname (files->path);
  set_location (new_path);

  layout_find_item = metadata_item_to_no (strrchr (old_path, '/')+1);
  //fprintf (stderr, "%i, %s\n", layout_find_item, strrchr (old_path, '/')+1);
  free (old_path);

  itk->focus_no = -1;
  free (new_path);
  if (e)
  {
    ctx_set_dirty (e->ctx, 1);
    e->stop_propagate = 1;
  }
}

static void toggle_keybindings_display (CtxEvent *e, void *d1, void *d2)
{
  show_keybindings = !show_keybindings;
  ctx_set_dirty (e->ctx, 1);
}

static void outline_expand (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  e->stop_propagate = 1;
  if (item_get_type_atom (no+1) != CTX_ATOM_STARTGROUP)
    return;

  metadata_unset2 (no+1, "folded");
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void outline_collapse (CtxEvent *e, void *d1, void *d2)
{
  e->stop_propagate = 1;
  int no = focused_no;
  e->stop_propagate = 1;
  if (item_get_type_atom (no+1) != CTX_ATOM_STARTGROUP)
    return;

  metadata_set_float2 (no+1, "folded", 1);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}


static void toggle_reveal_codes (CtxEvent *e, void *d1, void *d2)
{
  layout_config.codes = !layout_config.codes;
  ctx_set_dirty (e->ctx, 1);
}
static int layout_focused_link = -1;

static int dir_item_count_links (int i)
{
  char *p = files->items[i];
  int in_link = 0;
  int count = 0;
  while (*p)
  {
    switch (*p)
    {
      case '[':
        in_link = 1;
        break;
      case ']': 
        if (in_link) count ++;
        in_link = 0;
        break;
    }
    p++;
  }
  return count;
}


static char *string_link_no (const char *string, int link_no)
{
  CtxString *str = ctx_string_new ("");
  CtxString *str_secondary = ctx_string_new ("");

  const char *p = string;
  int in_link = 0;
  int count = 0;
  while (p == string || p[-1])
  {
    switch (*p)
    {
      case '[':
        in_link = 1;
        ctx_string_set (str, "");
        ctx_string_set (str_secondary, "");
        break;
      case ']': 
        if (in_link) count ++;
        in_link = 0;
        break;
      case '(':
        ctx_string_set (str_secondary, "");
        if (p[-1]==']'){
                in_link = 2;
                count--;
        }
        break;
      case ')':
        if (in_link == 2){
          in_link = 0;
          count++;
        }
        break;
      case '\0':
      default:

        if (*p && in_link == 2)
        ctx_string_append_byte (str_secondary, *p);
        else if (*p && in_link == 1)
        ctx_string_append_byte (str, *p);
        else if (link_no == count-1)
        {
          char *ret = NULL;
          if (str_secondary->str[0])
            ret = ctx_string_dissolve (str_secondary);
          else
            ctx_string_free (str_secondary, 1);
          if (!ret)
            ret = ctx_string_dissolve (str);
          else
            ctx_string_free (str, 1);
          return ret;
        }
        break;
    }
    p++;
  }
  ctx_string_free (str_secondary, 1);
  ctx_string_free (str, 1);
  return NULL;
}


static char *dir_item_link_no (int item, int link_no)
{
  return string_link_no (files->items[item], link_no);
}

static const char *ctx_basedir (void)
{
  static char *val = NULL;
  if (!val)
  {
    char *home = getenv ("HOME");
    if (home)
    {
      val = ctx_strdup_printf ("%s/.ctx", home);
    }
    else
      val = strdup ("/tmp/ctx");
  }
  return val;
}


static char *dir_metadata_path (const char *path)
{
  char *ret;
  char *hex="0123456789abcdef";
  unsigned char hash[40];
  unsigned char hash_hex[51];
  CtxSHA1 *sha1 = ctx_sha1_new ();
  ctx_sha1_process (sha1, (uint8_t*)path, strlen (path));
  ctx_sha1_done (sha1, hash);
  ctx_sha1_free (sha1);
  for (int j = 0; j < 20; j++)
  {
    hash_hex[j*2+0]=hex[hash[j]/16];
    hash_hex[j*2+1]=hex[hash[j]%16];
  }
  hash_hex[40]=0;
  ret = ctx_strdup_printf ("%s/%s", ctx_basedir(), hash_hex);
  return ret;
}



static void dir_follow_link (CtxEvent *e, void *d1, void *d2)
{
  char *target = dir_item_link_no (focused_no, layout_focused_link);
  set_location (target);

  layout_focused_link = 0;
  ctx_set_dirty (e->ctx, 1);
}


static void item_activate (CtxEvent *e, void *d1, void *d2)
{
  //CtxEvent *e = &event; // we make a copy to permit recursion
  int no = focused_no;
  viewer_no = no;
  int virtual = (item_get_type_atom (no) == CTX_ATOM_TEXT);

  if (virtual)
  {
    text_edit = 0;
    ctx_set_dirty (e->ctx, 1);
    //_itk_key_bindings_active = 0;
    if (e)
      e->stop_propagate = 1;
    return;
  }

  char *new_path;

  if (!strcmp (files->items[no], ".."))
  {
    dir_go_parent (e, d1, d2);
    return;
  }
  else
  {
    new_path = ctx_strdup_printf ("%s/%s", files->path, 
                                  files->items[no]);
  }
  const char *media_type = ctx_path_get_media_type (new_path); 
  if (!strcmp(media_type, "inode/directory"))
  {
    set_location (new_path);
    itk->focus_no = 0;
  }
  else
  {
    viewer_load_path (new_path, files->items[no]);
  }
  free (new_path);

  if (e)
  {
    ctx_set_dirty (e->ctx, 1);
    e->stop_propagate = 1;
  }
}

static void item_drag (CtxEvent *e, void *d1, void *d2)
{
  //fprintf (stderr, "drag %i\n", e->type);
  switch (e->type)
  {
    case CTX_DRAG_PRESS:
      e->stop_propagate = 1;
      break;
    case CTX_DRAG_MOTION:
      float x = metadata_key_float2 (focused_no, "x", -10000);
      if (x >=0)
      {
        x = metadata_key_float2 (focused_no, "x", 0.0f);
        x += e->delta_x / ctx_width (e->ctx);
        metadata_set_float2 (focused_no, "x", x);
        ctx_set_dirty (e->ctx, 1);
      }
      float y = metadata_key_float2 (focused_no, "y", -10000);
      if (y >=0)
      {
        y = metadata_key_float2 (focused_no, "y", 0.0f);
        y += e->delta_y / ctx_width (e->ctx);
        metadata_set_float2 (focused_no, "y", y);
        ctx_set_dirty (e->ctx, 1);
      }
      e->stop_propagate = 1;
      break;
    case CTX_DRAG_RELEASE:
      e->stop_propagate = 1;
      break;
    default:
      break;
  }
}

static void item_tap_and_hold (CtxEvent *e, void *d1, void *d2)
{
  //fprintf (stderr, "tap and hold %i\n", e->type);
  e->stop_propagate = 1;
}

static int viewer_load_next_handler = 0;

static void deactivate_viewer (CtxEvent *e, void *d1, void *d2)
{

  if (viewer_load_next_handler!=0)
    ctx_remove_idle (ctx, viewer_load_next_handler);
  viewer_load_next_handler = 0;

  while (clients)
    ctx_client_remove (ctx, clients->data);
  active = NULL;
  ctx_set_dirty (e->ctx, 1);
}

static int item_get_level (int no)
{
  int level = 0;
  for (int i = 0; i <= no; i++)
  {
    int atom = item_get_type_atom (i);
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


static int items_to_move (int no)
{
  int count = 1;
  int self_level = item_get_level (no);
  int level;

  do {
    no ++;
    level = item_get_level (no);
    if (level > self_level) count++;
  } while (level > self_level);
  if (count > 1) count ++;
  
  return count;
}

static void item_duplicate(CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  int count = items_to_move (no);

  int insert_pos = no + count;
  for (int i = 0; i < count; i ++)
  {
    metadata_insert (insert_pos + i, files->items[no + i]);
    int keys = metadata_item_key_count2 (no + i);
    for (int k = 0; k < keys; k++)
    {
      char *key = metadata_key_name2 (no + i, k);
      if (key)
      {
        char *val = metadata_key_string2 (no + i, key);
        if (val)
        {
          metadata_set2 (insert_pos + i, key, val);
          //itk->x += level * em * 3 + em;
          //itk_labelf (itk, "%s=%s", key, val);
          free (val);
        }
        free (key);
      }
    }

  }
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
  e->stop_propagate = 1;
}

static void item_delete (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  int count = items_to_move (no);

  //int virtual = (item_get_type_atom (no) == CTX_ATOM_TEXT);
  //if (virtual)
  //
//  fprintf (stderr, "items to remove %i\n", count);
  CtxAtom pre_atom = item_get_type_atom (no-1);
  CtxAtom post_atom = item_get_type_atom (no+count);
  for (int i = 0; i < count; i++)
  {
    metadata_remove (no);
  }

#if 1
  if (pre_atom == CTX_ATOM_STARTGROUP &&
      post_atom == CTX_ATOM_ENDGROUP)
  {
 //       fprintf (stderr, "removed group\n");
    metadata_remove (no-1);
    metadata_remove (no-1);
    layout_find_item = no-2;
    itk->focus_no = -1;
  }
  //se
#endif
  //layout_find_item = no;

  text_edit = TEXT_EDIT_OFF;
  metadata_dirt();

  if (e)
  {
    ctx_set_dirty (e->ctx, 1);
    e->stop_propagate = 1;
  }
}

void itk_focus (ITK *itk, int dir);

#if 0
static void move_after_next_sibling (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  //char *new_path;
  //if (!strcmp (files->items[no], ".."))dd
  if (no<files->count-1)
  {
    metadata_swap (no, no+1);
    metadata_dirt();
    ctx_set_dirty (e->ctx, 1);
    itk_focus (itk, 1);
  }
}
#endif


static void grow_height (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float height = metadata_key_float2 (no, "height", -100.0);
  height += 0.01;
  if (height > 1.2) height = 1.2;
  metadata_set_float2 (no, "height", height);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void shrink_height (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float height = metadata_key_float2 (no, "height", -100.0);
  height -= 0.01;
  if (height < 0.01) height = 0.01;
  metadata_set_float2 (no, "height", height);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void grow_width (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float width = metadata_key_float2 (no, "width", -100.0);
  width += 0.01;
  if (width > 1.5) width = 1.5;
  metadata_set_float2 (no, "width", width);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void shrink_width (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float width = metadata_key_float2 (no, "width", -100.0);
  width -= 0.01;
  if (width < 0.0) width = 0.01;
  metadata_set_float2 (no, "width", width);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_left (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float x = metadata_key_float2 (no, "x", -100.0);
  x -= 0.01;
  if (x < 0.0) x = 0.0;
  metadata_set_float2 (no, "x", x);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_right (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float x = metadata_key_float2 (no, "x", -100.0);
  x += 0.01;
  if (x < 0) x = 0;
  metadata_set_float2 (no, "x", x);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_up (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float y = metadata_key_float2 (no, "y", -100.0);
  y -= 0.01;
  if (y< 0) y = 0.01;
  metadata_set_float2 (no, "y", y);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_down (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float y = metadata_key_float2 (no, "y", -100.0);
  y += 0.01;
  if (y< 0) y = 0.01;
  metadata_set_float2 (no, "y", y);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

#if 0
static void move_before_previous_sibling (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  if (no>0)
  {
    metadata_swap (no, no-1);
    metadata_dirt ();
    ctx_set_dirty (e->ctx, 1);
    itk_focus (itk, -1);
  }
}
#endif

static int
dir_prev (int i)
{
 int pos = i;
 int again = 0;

 do {
   pos -= 1;
   int atom = item_get_type_atom (pos);
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


static int
dir_next (int i)
{
 int pos = i;
 int again = 0;

 do {
   pos += 1;
   int atom = item_get_type_atom (pos);
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

static int
dir_prev_sibling (int i)
{
  int pos = i;
  int start_level = 0;
  int level = 0; // not absolute level, but relative level balance
  pos --;
  int atom = item_get_type_atom (pos);
  if (atom == CTX_ATOM_ENDGROUP)
          level ++;
  else if (atom == CTX_ATOM_STARTGROUP)
          level --;
  while (level > start_level)
  {
    pos--;
    atom = item_get_type_atom (pos);
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
    atom = item_get_type_atom (pos);
  }
  if (level < start_level || pos < 0)
  {
     return -1;
  }
  return pos;
}

static int
dir_next_sibling (int i)
{
  int start_level = 0;
  int level = 0;

  int atom;
 
  i++;
  atom = item_get_type_atom (i);
  if (atom == CTX_ATOM_ENDGROUP)
  {
    return -1;
  }
  if (atom == CTX_ATOM_STARTGROUP)
  {
    level++;

    while (level > start_level && i < files->count)
    {
      i++;
      atom = item_get_type_atom (i);
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
       atom = item_get_type_atom (i);
    }
  }
  while (atom == CTX_ATOM_NEWPAGE)
  {
    i++;
    atom = item_get_type_atom (i);
  }

  if (level != start_level || i >= files->count)
  {
     return -1;
  }
  return i;
}

static void
move_after_next_sibling (CtxEvent *event, void *a, void *b)
{
  if (dir_next_sibling (focused_no) < 0)
  {
    event->stop_propagate = 1;
    return;
  }

  //for (int i = 0; i < count; i++)
  {
  int start_no    = focused_no;
  int count = items_to_move (start_no);
  
  int start_level = 0;
  int level = 0;
  int did_skips = 0;
  int did_foo =0;

  if (item_get_type_atom (focused_no + count) == CTX_ATOM_ENDGROUP)
  {
    event->stop_propagate=1;
    return;
  }

  if (item_get_type_atom (focused_no + count + 1) == CTX_ATOM_STARTGROUP &&
      item_get_type_atom (focused_no + count) == CTX_ATOM_TEXT)
  {
    focused_no+=count;
    did_foo = 1;
  }

  focused_no++;
  int atom = item_get_type_atom (focused_no);
  if (atom == CTX_ATOM_ENDGROUP)
  {
    focused_no--;
    event->stop_propagate=1;
    return;
  }
  if (atom == CTX_ATOM_STARTGROUP)
     level++;

  while (level > start_level)
  {
    focused_no++;
    atom = item_get_type_atom (focused_no);
    if (atom == CTX_ATOM_STARTGROUP)
      {
        level++;
      }
    else if (atom == CTX_ATOM_ENDGROUP)
      {
        level--;
      }
    else
    {
      did_skips = 1;
    }
  }
  level++;
  if (item_get_type_atom (focused_no) == CTX_ATOM_ENDGROUP)
  {
    level--;
    focused_no++;
  }
  focused_no++;

  if (level < start_level)
  {
     focused_no = start_no;
  }
  else
  {
    if (did_skips && did_foo) focused_no--;

    //focused_no++;

    for (int i = 0; i < count; i ++)
    {
      metadata_insert (focused_no, "a");
      metadata_dirt ();
      metadata_swap (focused_no, start_no);
      metadata_dirt ();
      metadata_remove (start_no);
      metadata_dirt ();
    }
    focused_no-=count;
  }
  }

  layout_find_item = focused_no;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}


static void
move_before_previous_sibling (CtxEvent *event, void *a, void *b)
{
  int start_no = focused_no;
  
  int level = 0;
  int start_level = 0;
  int did_skips = 0;

  if (dir_prev_sibling (focused_no) < 0)
  {
     event->stop_propagate = 1;
     return;
  }

  focused_no--;
  int atom = item_get_type_atom (focused_no);
  if (atom == CTX_ATOM_ENDGROUP)
     level++;
  else if (atom == CTX_ATOM_STARTGROUP)
     level--;

  while (level > start_level)
  {
    focused_no--;
    atom = item_get_type_atom (focused_no);
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
      did_skips = 1;
    }
  }
  if (level < start_level)
  {
     focused_no = start_no;
  }
  else
  {
    int count = items_to_move (start_no);
    if (!did_skips) focused_no++;
    for (int i = 0; i < count; i ++)
    {
      metadata_insert (focused_no-1 + i, "b");
      metadata_swap (start_no+1 +i, focused_no-1 +i);
      metadata_remove (start_no+1 +i);
      metadata_dirt ();
    }
    focused_no--;
  }

  layout_find_item = focused_no;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
make_sibling_of_parent (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  int level = item_get_level (no);

  int count = items_to_move (no);

  if (level>0)
  {
    int target = no;
    int target_level;
    do {
      target++;
      target_level = item_get_level (target);
    } while (target_level >= level);

    int remove_level = 0;
    
    if (item_get_type_atom (no-1) == CTX_ATOM_STARTGROUP &&
        item_get_type_atom (no+count) == CTX_ATOM_ENDGROUP)
      remove_level = 1;

    for (int i = 0; i < count; i ++)
    {
      metadata_insert (target+1+i, "c");
      metadata_swap (no+i, target+1+i);
    }

    if (remove_level)
    {
      for (int i = 0; i < count + 2; i++)
        metadata_remove (no-1);
      layout_find_item = no - 1;
    }
    else
    {
      for (int i = 0; i < count; i++)
        metadata_remove (no);
      layout_find_item = target - count;
    }

    itk->focus_no = -1;

    metadata_dirt ();
    ctx_set_dirty (e->ctx, 1);

  }
}

static void
make_child_of_previous (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  if (no < 1)
    return;

  {
    int count = items_to_move (no);
    int target = no-1;
    int atom = item_get_type_atom (target);
    if (atom == CTX_ATOM_STARTGROUP)
       return;
    if (atom == CTX_ATOM_ENDGROUP)
    {
      for (int i = 0; i < count; i++)
      {
        metadata_insert (target+i, "d");
        metadata_swap (no+1+i, target+i);
        metadata_remove (no+1+i);
      }
    }
    else
    {
      // insert new group
      target = no;
      metadata_insert (target, "e");
      metadata_set2 (target, "type", "ctx/endgroup");
      for (int i = 0; i <count;i++)
      metadata_insert (target, "f");

      metadata_insert (target, "g");
      metadata_set2 (target, "type", "ctx/startgroup");

      for (int i = 0; i < count; i++)
      {
        metadata_swap (no+count+2, target+1+i);
        metadata_remove (no+count+2);
      }
    }

#if 0
    layout_find_item = target;
    itk->focus_no = -1;
#else
    //itk->focus_no -= (target-focused_no);
    focused_no = target;
    //layout_find_item = focused_no;
#endif

    metadata_dirt ();
    ctx_set_dirty (e->ctx, 1);
  }
}

void
item_cycle_bullet (CtxEvent *event, void *a, void *b)
{
  int bullet = metadata_key_int2 (focused_no, "bullet", CTX_BULLET_NONE);
  switch (bullet)
  {
    case CTX_BULLET_NONE:
      metadata_set_float2 (focused_no, "bullet", CTX_BULLET_BULLET);
      break;
    case CTX_BULLET_BULLET:
      metadata_set_float2 (focused_no, "bullet", CTX_BULLET_NUMBERS);
      break;
    case CTX_BULLET_NUMBERS:
      metadata_set_float2 (focused_no, "bullet", CTX_BULLET_TODO);
      break;
    case CTX_BULLET_TODO:
      metadata_set_float2 (focused_no, "bullet", CTX_BULLET_DONE);
      break;
    case CTX_BULLET_DONE:
      metadata_unset2 (focused_no, "bullet");
      break;
  }
  metadata_dirt ();

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void
item_cycle_heading (CtxEvent *event, void *a, void *b)
{
  char *klass = metadata_key_string2(focused_no, "class");

  if (!klass) klass = strdup ("");

  if (!strcmp (klass, "h1"))
    metadata_set2 (focused_no, "class", "h2");
  else if (!strcmp (klass, "h2"))
    metadata_set2 (focused_no, "class", "h3");
  else if (!strcmp (klass, "h3"))
    metadata_set2 (focused_no, "class", "h4");
  else if (!strcmp (klass, "h4"))
    metadata_set2 (focused_no, "class", "h5");
  else if (!strcmp (klass, "h5"))
    metadata_set2 (focused_no, "class", "h6");
  else if (!strcmp (klass, "h6"))
    metadata_unset2 (focused_no, "class");
  else
    metadata_set2 (focused_no, "class", "h1");
  metadata_dirt ();

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void draw_folder (Ctx *ctx, float x, float y, float w, float h)
{
  ctx_save (ctx);
  ctx_move_to (ctx, x + w * 0.24, y + h * 0.066);
  ctx_rel_line_to (ctx, w * 0.33, 0);
  ctx_rel_line_to (ctx, w * 0.016667, h * -.05);
  ctx_rel_line_to (ctx, w * 0.15, 0);
  ctx_rel_line_to (ctx, 0, h * 0.55);
  ctx_rel_line_to (ctx, w * -0.5, 0);
  ctx_close_path (ctx);
  ctx_line_width (ctx, w * 0.0133);
  ctx_rgb (ctx, 0.5, 0.4, 0.2);
  ctx_fill (ctx);
  ctx_restore (ctx);
}

static void draw_img (ITK *itk, float x, float y, float w, float h, const char *path)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  int imgw, imgh;
    float target_width = 
      (w - (layout_config.padding_left + layout_config.padding_right) * em );
    float target_height = 
      (h - (layout_config.padding_top + layout_config.padding_bottom) * em );
  //ctx_begin_path (ctx);
  //

  if ((target_width * dir_scale > 250 || target_height * dir_scale > 250))
  {
    char reteid[65]="";
    ctx_texture_load (ctx, path, &imgw, &imgh, reteid);
    //fprintf (stderr, "%f %f %i %i %s\n", target_width, target_height, imgw, imgh, reteid);
    if (reteid[0])
    {
#if 0
      ctx_draw_texture (ctx, reteid, x + layout_config.padding * em, y + layout_config.padding * em, target_width, target_height);
#else
      ctx_rectangle (ctx, x + layout_config.padding_left * em, y + layout_config.padding_top*em, w - (layout_config.padding_left + layout_config.padding_right) * em, h - (layout_config.padding_top + layout_config.padding_bottom) * em);
      ctx_save (ctx);
      ctx_translate (ctx, x + layout_config.padding_left * em, y + layout_config.padding_top * em);
      ctx_scale (ctx, (target_width)/ imgw,
                      (target_height) / imgh);
      ctx_texture (ctx, reteid, 0,0);
      ctx_fill (ctx);
      ctx_restore (ctx);
     #endif
    }
    return;
  }
  else
  {
  char *thumb_path = ctx_get_thumb_path (path);
  if (access (thumb_path, F_OK) != -1)
  {
    char reteid[65]="";
      ctx_save (ctx);
    ctx_texture_load (ctx, thumb_path, &imgw, &imgh, reteid);
    if (reteid[0])
    {
#if 0
      ctx_draw_texture (ctx, reteid, x, y, w, h);
#else
      ctx_rectangle (ctx, x + layout_config.padding_left * em, y + layout_config.padding_top*em, w - (layout_config.padding_left+layout_config.padding_right) * em, h - (layout_config.padding_top+layout_config.padding_bottom) * em);
      ctx_translate (ctx, x + layout_config.padding_left * em, y + layout_config.padding_top * em);
      ctx_scale (ctx, (target_width)/ imgw,
                      (target_height) / imgh);
      ctx_texture (ctx, reteid, 0,0);
      ctx_fill (ctx);
     #endif
  //  free (thumb_path);
    }
      ctx_restore (ctx);
    return;
  }
  else
  {
    queue_thumb (path, thumb_path);
  }
  }

  {
    ctx_save (ctx);
    ctx_arc (ctx, x + w * 0.5, y + h * 0.3, w * 0.2, 0.0, 6.3, 0);
    static float a = 0.4;
    a += 0.6124*0.2;
    if (a >= 0.6) a-=0.2;
    ctx_rgba (ctx, 1, 1, 1, a);
    ctx_fill (ctx);
    ctx_restore (ctx);
  }
}

static void draw_doc (Ctx *ctx, float x, float y, float w, float h)
{
  ctx_save (ctx);
  ctx_move_to (ctx, x + w * 0.34, y + h * 0.05);
  ctx_rel_line_to (ctx, w * 0.33, 0);
  ctx_rel_line_to (ctx, 0, h * 0.6);
  ctx_rel_line_to (ctx, w * -0.33, 0);
  ctx_close_path (ctx);
  ctx_line_width (ctx, w * 0.0133);
  ctx_rgb (ctx, .9,.9,.9);
  ctx_preserve (ctx);
  ctx_fill (ctx);
  //ctx_stroke_source (ctx);
  ctx_rgb (ctx, 0,0,0);
  ctx_stroke (ctx);
  ctx_restore (ctx);
}

const char *viewer_media_type = NULL;

void ctx_client_lock (CtxClient *client);
void ctx_client_unlock (CtxClient *client);

static void dir_handle_event (Ctx *ctx, CtxEvent *ctx_event, const char *event)
{
  if (!active)
    return;
  VT *vt = active->vt;
  CtxClient *client = vt_get_client (vt);
  ctx_client_lock (client);
  int media_class = ctx_media_type_class (viewer_media_type);

  if (!strcmp (event, "control-escape") ||
      !strcmp (event, "alt-up"))
  {
    ctx_client_unlock (client);
    deactivate_viewer (ctx_event, NULL, NULL);
    return;
  } 
  if (media_class == CTX_MEDIA_TYPE_IMAGE && (!strcmp (event, "space")))
  {
    ctx_client_unlock (client);
    //itk->focus_no ++;
    itk->focus_no ++;
    item_activate (NULL, (void*)(size_t)(viewer_no+1), NULL);
    ctx_set_dirty (ctx, 1);

    return;
  }
  if ((media_class == CTX_MEDIA_TYPE_IMAGE || 
      media_class == CTX_MEDIA_TYPE_VIDEO)&& (!strcmp (event, "page-down")))
  {
    ctx_client_unlock (client);
    itk->focus_no ++;
    item_activate (NULL, (void*)(size_t)viewer_no+1, NULL);
    ctx_set_dirty (ctx, 1);

    return;
  }
  if ((media_class == CTX_MEDIA_TYPE_IMAGE || 
      media_class == CTX_MEDIA_TYPE_VIDEO)&& (!strcmp (event, "page-up")))
  {
    ctx_client_unlock (client);
    //itk->focus_no ++;
    itk->focus_no --;
    item_activate (NULL, (void*)(size_t)viewer_no-1, NULL);
    ctx_set_dirty (ctx, 1);

    return;
  }
  else
  {
    if (vt)
      vt_feed_keystring (vt, ctx_event, event);
  }
  ctx_client_unlock (client);
}

static void dir_key_any (CtxEvent *event, void *userdata, void *userdata2)
{
  switch (event->type)
  {
    case CTX_KEY_PRESS:
      dir_handle_event (ctx, event, event->string);
      break;
    case CTX_KEY_DOWN:
      {
        char buf[1024];
        snprintf (buf, sizeof(buf)-1, "keydown %i %i", event->unicode, event->state);
        dir_handle_event (ctx, event, buf);
      }
      break;
    case CTX_KEY_UP:
      {
        char buf[1024];
        snprintf (buf, sizeof(buf)-1, "keyup %i %i", event->unicode, event->state);
        dir_handle_event (ctx, event, buf);
      }
      break;
    default:
      break;
  }
}

static void goto_link (CtxEvent *event, void *data1, void *data2)
{
  set_location (data1);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}

static void dir_revert (CtxEvent *event, void *data1, void *data2)
{
  // XXX: NYI
  metadata_dirt();
}

static void dir_select_item (CtxEvent *event, void *data1, void *data2)
{
   itk->focus_no = (size_t)data1;
   ctx_set_dirty (event->ctx, 1);
   event->stop_propagate = 1;
}

/*
 * this is the core text layout with linebreaking work function it is used
 * in two places and we do not want to repeat the logic
 */

static void layout_text (Ctx *ctx, float x, float y, const char *d_name,
                         float space_width, float wrap_width, float line_height,
                         int sel_start, 
                         int sel_end, 
                         int print, float *ret_x, float *ret_y,
                         int *prev_line, int *next_line,
                         int visible_markup,
                         int is_focused)
{
  char word[1024]="";
  int wlen = 0;
  const char *p;
  int pos = 0;
  float x0 = x;
  int link_no = 0;
  int was_no = 0;
  int cursor_drawn = 0;
  int compute_neighbor_lines = 0;
  if (prev_line || next_line) compute_neighbor_lines = 1;

  y += line_height;
  if (sel_start < 0)
    cursor_drawn = 1;

  int pot_cursor=-10;
  int best_end_cursor=-10;

  int in_link = 0;
  int was_in_link = 0;

  for (p = d_name; p == d_name || p[-1]; p++)
  {
    switch (*p)
    {
      case ' ':
      case '\0':
      case '\n':
    {
      word[wlen]=0;
      int wlen_utf8 = ctx_utf8_strlen (word);
      float word_width = ctx_text_width (itk->ctx, word);

      if (sel_start == TEXT_EDIT_FIND_CURSOR_FIRST_ROW)
      {
        if (pot_cursor < 0 && (x + word_width > text_edit_desired_x || *p == '\0'))
        {
           float excess =  (x + word_width) - text_edit_desired_x;
           int removed = 0;
            for (int g = wlen - 1; excess > 0.0f && g >= 0; g--)
            {
              char tmp[10]="a";
              tmp[0] = word[g];
              excess -= ctx_text_width (itk->ctx, tmp);
              removed ++;
           }
           pot_cursor = pos - removed;
           if (pot_cursor > (int)strlen (d_name))
              pot_cursor=strlen(d_name);
           sel_start = text_edit = pot_cursor;
        }
      }
      if (sel_start == TEXT_EDIT_FIND_CURSOR_LAST_ROW)
      {
        if (pot_cursor < 0 && (x + word_width > text_edit_desired_x || *p == '\0'))
        {
           float excess =  (x + word_width) - text_edit_desired_x;
           int removed = 0;
            for (int g = wlen - 1; excess > 0.0f && g >= 0; g--)
            {
              char tmp[10]="a";
              tmp[0] = word[g];
              excess -= ctx_text_width (itk->ctx, tmp);
              removed ++;
            }
            pot_cursor = pos - removed;
            best_end_cursor = pot_cursor;
        }
      }

      if (compute_neighbor_lines && text_edit_desired_x > 0)
      {
        if (pot_cursor < 0 && (x + word_width > text_edit_desired_x || *p == '\0'))
        {
           float excess =  (x + word_width) - text_edit_desired_x;
           int removed = 0;
            for (int g = wlen - 1; excess > 0.0f && g >= 0; g--)
            {
              char tmp[10]="a";
              tmp[0] = word[g];
              excess -= ctx_text_width (itk->ctx, tmp);
              removed ++;
           }
           pot_cursor = pos - removed;

          if (pot_cursor < sel_start - 1)
          {
            *prev_line = pot_cursor;
            *next_line = -1;
          }
          else if (pot_cursor > sel_start + 1)
          {
            *next_line = pot_cursor;
            compute_neighbor_lines = 0;
          }
        }
      }

      if (x + word_width - x0 > wrap_width)
      {
        x = x0;
        y = y + line_height;
        pot_cursor = -10;
      }
      {
        if (!cursor_drawn && pos >= sel_start)
        {
          int seg = wlen_utf8 - (pos-sel_start);
          char tmp[seg*2+10];
          if (print)
          ctx_save (ctx);
          int o = 0;
          for (int i = 0; i < seg; i++)
          {
            int ul = ctx_utf8_len (word[o]);
            memcpy (&tmp[o], &word[o], ul);
            o+=ul;
          }
          tmp[o]=0;
          //memcpy (tmp, word, seg);
          if (print)
            ctx_rgb (itk->ctx, 1,0,0);
          float cursor_x = x + ctx_text_width (itk->ctx, tmp);
          if (text_edit_desired_x < 0)
            text_edit_desired_x = cursor_x;
      if (print)
      {
          ctx_rectangle (itk->ctx,
                    cursor_x-1, y - line_height, 2, line_height * 1.2);
          ctx_fill (itk->ctx);
          ctx_restore (ctx);
      }
       cursor_drawn = 1;
        }
      if (print)
      {
        if (was_in_link)
        {
          ctx_save (itk->ctx);
          ctx_begin_path (itk->ctx);
          ctx_rectangle (itk->ctx, x - ctx_text_width (itk->ctx, " ")/2, y - itk->font_size, ctx_text_width (itk->ctx, word) + ctx_text_width (itk->ctx, " "), itk->font_size * 1.2);
          char *href = string_link_no (d_name, was_no);
          ctx_listen_with_finalize (itk->ctx, CTX_CLICK,
                          goto_link, href, NULL, (void*)free, NULL);
          ctx_listen_set_cursor (itk->ctx, CTX_CURSOR_HAND);
          ctx_begin_path (itk->ctx);

          if (was_in_link > 1)
          {
            ctx_rgba (itk->ctx, 1, 1, 1, 0.8);
            ctx_rectangle (itk->ctx, x - ctx_text_width (itk->ctx, " ")/2, y - itk->font_size, ctx_text_width (itk->ctx, word) + ctx_text_width (itk->ctx, " "), itk->font_size * 1.2);
            ctx_fill (itk->ctx);
            ctx_rgba (itk->ctx, 0,0,0,1.0);
          }
          else
          {
            ctx_rgba (itk->ctx, 1, 1, 0.3, 1);
            ctx_rectangle (itk->ctx, x - ctx_text_width (itk->ctx, " ")/2, y + itk->font_size * 0.1, ctx_text_width (itk->ctx, word) + ctx_text_width (itk->ctx, " "), itk->font_size * 0.05);
            ctx_fill (itk->ctx);
          }


          ctx_move_to (itk->ctx, x, y);
          ctx_text (itk->ctx, word);
          ctx_restore (itk->ctx);
        }
        else
        {
        ctx_move_to (itk->ctx, x, y);
        ctx_text (itk->ctx, word);
        }
      }
      }

      x += word_width + space_width;
      wlen=0;
      if (*p == '\n')
      {
        x = x0;
        y = y + line_height;
        pot_cursor = -10;
      }
    }
    break;
        if (!visible_markup)
          break;
        /* FALLTHROUGH */
      default:

      if (visible_markup)
      {
        if (wlen < 1000)
          word[wlen++]=*p;
      }
      else
      {
        if (*p !='[' && *p!=']')
        if (wlen < 1000 && (in_link <= 1))
          word[wlen++]=*p;
      }
    }

    was_in_link = (in_link != 0);
    if (was_in_link && is_focused) was_in_link += (link_no == layout_focused_link);
    was_no = link_no;

    switch (*p)
    {
      case '[':
      case ']':
        if (*p == '[')
        {
          in_link++;
        }
        else if (*p == ']')
        {
          in_link--;
          link_no ++;
        }
        break;
      case '(':
        if (p != d_name && p[-1] == ']')
        {
                link_no--;
        in_link+=2;
        if (!visible_markup)
          word[--wlen]=0;
        }
        break;
      case ')':
        if (in_link > 1)
        {
                link_no++;
          in_link-=2;
        }
        break;
    }
    if ((*p & 0xc0) != 0x80) pos++;
  }

  if (sel_start == TEXT_EDIT_FIND_CURSOR_FIRST_ROW)
  {
      text_edit = strlen (d_name);
  }
  if (sel_start == TEXT_EDIT_FIND_CURSOR_LAST_ROW)
  {
    if (best_end_cursor >=0)
      text_edit = best_end_cursor;
    else
      text_edit = strlen (d_name);
  }

  if (ret_x)
    *ret_x = x;
  if (ret_y)
    *ret_y = y;
}

void text_edit_stop (CtxEvent *event, void *a, void *b)
{
  text_edit = TEXT_EDIT_OFF;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_return (CtxEvent *event, void *a, void *b)
{
  char *str = strdup (files->items[focused_no]);
  metadata_insert (focused_no+1, files->items[focused_no]);
  metadata_dirt ();

  metadata_rename (focused_no+1, str + text_edit);
  str[text_edit]=0;
  metadata_rename (focused_no, str);

  metadata_dirt ();
  free (str);

  ctx_set_dirty (event->ctx, 1);
#if 1
  focused_no++;
  layout_find_item = focused_no;
  itk->focus_no++;
#else
  layout_find_item = focused_no + 1;
  itk->focus_no = -1;
#endif
  text_edit = 0;
  event->stop_propagate=1;
}

void text_edit_backspace (CtxEvent *event, void *a, void *b)
{
  if (focused_no>=0)
  {
    if (text_edit>0)
    {
      CtxString *str = ctx_string_new (files->items[focused_no]);
      ctx_string_remove (str, text_edit-1);
      metadata_rename (focused_no, str->str);
      ctx_string_free (str, 1);
      text_edit--;
      metadata_dirt ();
    }
    else if (text_edit == 0 && focused_no > 0 &&
            (item_get_type_atom (focused_no-1) == CTX_ATOM_TEXT)
            )
    {
      CtxString *str = ctx_string_new (files->items[focused_no-1]);
      text_edit = strlen (str->str);
      ctx_string_append_str (str, files->items[focused_no]);
      metadata_rename (focused_no-1, str->str);
      ctx_string_free (str, 1);
      metadata_remove (focused_no);
      focused_no--;
      itk->focus_no--;
      metadata_dirt ();
    }
    save_metadata();
  }
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_delete (CtxEvent *event, void *a, void *b)
{
  if (focused_no>=0){
    CtxString *str = ctx_string_new (files->items[focused_no]);
    if (text_edit == (int)strlen(str->str))
    {
      if (item_get_type_atom (focused_no+1) == CTX_ATOM_TEXT)
      {
        ctx_string_append_str (str, files->items[focused_no+1]);
        metadata_remove (focused_no+1);
      }
    }
    else
    {
      ctx_string_remove (str, text_edit);
    }
    metadata_rename (focused_no, str->str);
    ctx_string_free (str, 1);
    metadata_dirt ();
    save_metadata();
  }
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}


void text_edit_shift_return (CtxEvent *event, void *a, void *b)
{
  if (focused_no>=0){
    const char *insertedA = "\\";
    const char *insertedB = "n";
    CtxString *str = ctx_string_new (files->items[focused_no]);
    ctx_string_insert_utf8 (str, text_edit, insertedB);
    ctx_string_insert_utf8 (str, text_edit, insertedA);
    metadata_rename (focused_no, str->str);
    ctx_string_free (str, 1);
    text_edit++;
  }
  metadata_dirt ();
  save_metadata();
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_ignore (CtxEvent *event, void *a, void *b)
{
  event->stop_propagate=1;
}

void text_edit_any (CtxEvent *event, void *a, void *b)
{
  const char *inserted = event->string;
  if (!strcmp (inserted, "space")) inserted = " ";
  if (ctx_utf8_strlen (inserted) > 1)
    return;
  if (focused_no>=0){

    CtxString *str = ctx_string_new (files->items[focused_no]);
    ctx_string_insert_utf8 (str, text_edit, inserted);
    metadata_rename (focused_no, str->str);
    ctx_string_free (str, 1);
    text_edit++;
  }
  metadata_dirt ();
  save_metadata();
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_right (CtxEvent *event, void *a, void *b)
{
  int len = ctx_utf8_strlen (files->items[focused_no]);
  text_edit_desired_x = -100;

  text_edit ++;

  if (text_edit>len)
  {
    if (item_get_type_atom (focused_no+1) == CTX_ATOM_TEXT)
    {
      text_edit=0;
      layout_find_item = focused_no  + 1;
      itk->focus_no = -1;
    }
    else
    {
      text_edit=len;
    }
  }
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_left (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;

  text_edit--;

  if (text_edit < 0)
  {
    if (files->items[focused_no][0]==0 &&
        item_get_type_atom (focused_no-1) == CTX_ATOM_STARTGROUP &&
        item_get_type_atom (focused_no+1) == CTX_ATOM_ENDGROUP)
    {
      text_edit = 0;
      item_delete (event, (void*)(size_t)focused_no, NULL);

      //layout_find_item = focused_no-1;
      //itk->focus_no = -1;
      metadata_dirt();
    }
    else if (item_get_type_atom (focused_no-1) == CTX_ATOM_TEXT)

    {
      text_edit=ctx_utf8_strlen(files->items[focused_no-1]);
      layout_find_item = focused_no -1;
      itk->focus_no = -1;
    }
    else
    {
      text_edit = 0;
    }
  }
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_control_left (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;

  if (text_edit == 0)
  {
    if (item_get_type_atom (focused_no-1) == CTX_ATOM_TEXT)
    {
      text_edit=ctx_utf8_strlen(files->items[focused_no-1]);
      focused_no--;
      itk->focus_no--;
    }
    else
    {
      text_edit = 0;
    }
  }
  else
  {
    char *text = metadata_item_name (focused_no); 
    if (text_edit >0 && text[text_edit-1]==' ')  // XXX should be utf8 aware
      text_edit--;
    while (text_edit>0 && text[text_edit-1]!=' ') 
      text_edit--;
    free (text);
  }

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_control_right (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;

  {
    char *text = metadata_item_name (focused_no); 
    if (text[text_edit]==' ')  // XXX should be utf8 aware
      text_edit++;
    while (text[text_edit] && text[text_edit]!=' ') 
      text_edit++;
    free (text);
  }

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_home (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;
  text_edit = 0;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void text_edit_end (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;
  text_edit = ctx_utf8_strlen (files->items[focused_no]);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

int prev_line_pos = 0;
int next_line_pos = 0;

void text_edit_up (CtxEvent *event, void *a, void *b)
{
  if (prev_line_pos >= 0)
  {
    text_edit = prev_line_pos;
    ctx_set_dirty (event->ctx, 1);
    event->stop_propagate=1;
    return;
  }

  if (item_get_type_atom (focused_no-1) != CTX_ATOM_TEXT && 
      item_get_type_atom (focused_no-1) != CTX_ATOM_ENDGROUP)
  {
    event->stop_propagate=1;
    return;
  }
  //text_edit=strlen(files->items[focused_no-1]);
  //
  //

  if (files->items[focused_no][0]==0 &&
      (focused_no+1 >= files->count ||
      item_get_type_atom (focused_no+1) == CTX_ATOM_ENDGROUP))
    {
      text_edit = 0;
      int next_focus = dir_prev_sibling (focused_no);
      item_delete (event, (void*)(size_t)focused_no, NULL);
      focused_no = next_focus+1;
      metadata_dirt();
    }

  text_edit = TEXT_EDIT_FIND_CURSOR_LAST_ROW;
  layout_find_item = focused_no - 1;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}



void dir_zoom_in (CtxEvent *event, void *a, void *b)
{
  dir_scale *= 1.1f;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_zoom_out (CtxEvent *event, void *a, void *b)
{
  dir_scale /= 1.1f;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_zoom_reset (CtxEvent *event, void *a, void *b)
{
  dir_scale = 1.0f;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_font_up (CtxEvent *event, void *a, void *b)
{
  itk->font_size *= 1.05f;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_font_down (CtxEvent *event, void *a, void *b)
{
  itk->font_size /= 1.05f;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_set_page (CtxEvent *event, void *a, void *b)
{
  layout_show_page = (size_t)a;
  if (layout_show_page < 0) layout_show_page = 0;
  itk->focus_no = 0;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_prev_page (CtxEvent *event, void *a, void *b)
{
   dir_set_page (event, (void*)(size_t)(layout_show_page-1), NULL);
}
void dir_next_page (CtxEvent *event, void *a, void *b)
{
   if (layout_last_page > layout_show_page)
   dir_set_page (event, (void*)(size_t)(layout_show_page+1), NULL);
}


int item_context_active = 0;

static void
make_tail_entry ()
{
  metadata_insert (focused_no+items_to_move(focused_no), "");
  metadata_dirt ();
  layout_find_item = focused_no = dir_next_sibling (focused_no);
  itk->focus_no = -1;
  text_edit = 0;
}


void text_edit_down (CtxEvent *event, void *a, void *b)
{
  if (next_line_pos >= 0)
  {
    text_edit = next_line_pos;
    ctx_set_dirty (event->ctx, 1);
    event->stop_propagate=1;
    return;
  }

  if (item_get_type_atom (focused_no+1) != CTX_ATOM_TEXT)
  {
    if (files->items[focused_no][0])
      make_tail_entry ();
    ctx_set_dirty (event->ctx, 1);
    event->stop_propagate=1;
    return;
  }
  text_edit = TEXT_EDIT_FIND_CURSOR_FIRST_ROW;

  layout_find_item = focused_no + 1;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
  // -- - //
}



static void
focus_next_link (CtxEvent *event, void *a, void *b)
{
  layout_focused_link ++;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
focus_previous_link (CtxEvent *event, void *a, void *b)
{
  layout_focused_link --;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
focus_next_sibling (CtxEvent *event, void *a, void *b)
{
  layout_focused_link = -1;
  if (dir_next_sibling (focused_no) < 0)
  {
    make_tail_entry ();
    ctx_set_dirty (event->ctx, 1);
    event->stop_propagate=1;
    return;
  }

  layout_find_item = focused_no = dir_next_sibling (focused_no);
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static int item_get_list_index (int i)
{
  int pos = i;
  int count = 0;

  while (pos >= 0 &&
         metadata_key_int2(pos, "bullet", CTX_BULLET_NONE) == CTX_BULLET_NUMBERS)
  {
     pos = dir_prev_sibling (pos);
     count++;
  }

  return count;
}


static void
focus_next (CtxEvent *event, void *a, void *b)
{
  layout_focused_link = -1;
  int pos = dir_next (focused_no);
  if (pos >= 0)
  {
    layout_find_item = focused_no = pos;
    itk->focus_no = -1;
    ctx_set_dirty (event->ctx, 1);
  }
  event->stop_propagate=1;
}

static void
focus_previous (CtxEvent *event, void *a, void *b)
{
  layout_focused_link = -1;
  int pos = dir_prev (focused_no);
  if (pos >= 0)
  {
    layout_find_item = focused_no = pos;
    itk->focus_no = -1;
    ctx_set_dirty (event->ctx, 1);
  }
  event->stop_propagate=1;
}

static void
focus_previous_sibling (CtxEvent *event, void *a, void *b)
{
  layout_focused_link = -1;
  int pos = dir_prev_sibling (focused_no);
  if (pos >= 0)
  {
    layout_find_item = focused_no = pos;
    itk->focus_no = -1;
    ctx_set_dirty (event->ctx, 1);
  }
  event->stop_propagate=1;
}

static void
tool_rect_drag (CtxEvent *e, void *d1, void *d2)
{
  static float x0 = 0;
  static float y0 = 0;
  /* we default to adding new items at end of page, with layers dialog open
   * a cursor for specifying location is more easily seen.
   *
   * This cursor is different from flow cursor/position.
   */
  switch (e->type)
  {
    case CTX_DRAG_PRESS:
       fprintf (stderr, "rect drag %f %f\n", e->x, e->y);
       metadata_insert (focused_no, "");
       metadata_set2 (focused_no, "type", "ctx/rectangle");
       x0 = e->x;
       y0 = e->y;
       metadata_set_float2 (focused_no, "x", x0 / itk->width);
       metadata_set_float2 (focused_no, "y", y0 / itk->width);
       metadata_dirt ();
       break;
    case CTX_DRAG_MOTION:

       if (e->x > x0)
       {
         metadata_set_float2 (focused_no, "x", x0 / itk->width);
         metadata_set_float2 (focused_no, "width", (e->x-x0) / itk->width);
       }
       else
       {
         metadata_set_float2 (focused_no, "x", e->x / itk->width);
         metadata_set_float2 (focused_no, "width", (x0-e->x) / itk->width);
       }

       if (e->y > y0)
       {
         metadata_set_float2 (focused_no, "y", y0 / itk->width);
         metadata_set_float2 (focused_no, "height", (e->y-y0) / itk->width);
       }
       else
       {
         metadata_set_float2 (focused_no, "y", e->y / itk->width);
         metadata_set_float2 (focused_no, "height", (y0-e->y) / itk->width);
       }

       metadata_dirt ();
       ctx_set_dirty (e->ctx, 1);
       break;
    case CTX_DRAG_RELEASE:
       metadata_dirt ();
       break;
    default:
       break;
  }
  e->stop_propagate = 1;
}

static void
set_tool_no (CtxEvent *event, void *a, void *b)
{
  tool_no = (size_t)(a);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
dir_parent (CtxEvent *event, void *a, void *b)
{
  layout_focused_link = -1;
  int start_no = focused_no;
  
  int level = 1;

  CtxAtom atom;
  while (level > 0 && focused_no >= 0)
  {
    focused_no--;
    atom = item_get_type_atom (focused_no);
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
  if (metadata_key_int2 (focused_no, "was-folded", 0))
  {
    metadata_set_float2 (focused_no, "folded", 1.0);
    metadata_unset2 (focused_no, "was-folded");
  }
  focused_no--;
  if (focused_no < 0)
  {
     focused_no = start_no;
  }

  layout_find_item = focused_no;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}


static void
dir_enter_children (CtxEvent *event, void *a, void *b)
{
  int start_no = focused_no;
  layout_focused_link = -1;
  
  focused_no++;
  CtxAtom  atom = item_get_type_atom (focused_no);

  if (atom == CTX_ATOM_STARTGROUP)
  {
    if (metadata_key_int2(focused_no, "folded", 0))
    {
      metadata_set_float2 (focused_no, "was-folded", 1);
      metadata_set_float2 (focused_no, "folded", 0);
    }
    focused_no++;
  }
  else
  {
     focused_no = start_no;

     metadata_insert(focused_no+1, "j");
     metadata_set2(focused_no+1, "type", "ctx/startgroup");

     metadata_insert(focused_no+2, "");
     text_edit = 0;

     metadata_insert(focused_no+3, "l");
     metadata_set2(focused_no+3, "type", "ctx/endgroup");
     focused_no++;
  }
  metadata_dirt();

  layout_find_item = focused_no;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void item_properties (CtxEvent *event, void *a, void *b)
{
  if (item_context_active)
  {
    item_context_active = 0;
  }
  else
  {
    item_context_active = 1;
  }
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate = 1;
}
#define CTX_MAX_LAYOUT_BOXES    16

typedef struct CtxRectangle
{
  float x;
  float y;
  float width;
  float height;
} CtxRectangle;

static CtxRectangle layout_box[CTX_MAX_LAYOUT_BOXES];
int layout_box_count = 0;
int layout_box_no = 0;


static CtxControl *focused_control = NULL;


static void dir_layout (ITK *itk, Files *files)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  float prev_height = layout_config.height;
  float row_max_height = 0;
  int level = 0;
  int is_folded = 0;

  layout_box_count = 0;
  layout_box_no = 0;
  layout_box[0].x = 0.1;
  layout_box[0].y = 0.02;
  layout_box[0].width = 0.8;
  layout_box[0].height = 4000.0;

  metadata_cache_no = -3;
  prev_line_pos = -1;
  next_line_pos = -1;

#if 0
  if (cbox_x < 0)
  {
    cbox_x = 0.0f;
    cbox_y = 0.02f;
    cbox_width = 1.0f;
    cbox_height= 1000.0f;
  }
#endif

#if 0
  ctx_save (itk->ctx);
  ctx_begin_path (itk->ctx);
  ctx_rectangle (itk->ctx, cbox_x * itk->width, cbox_y * itk->width,
                 itk->width * cbox_width, itk->width * cbox_height);
  ctx_rgba (itk->ctx, 0.4, 0.0, 0.0,0.3);
  ctx_fill (itk->ctx);
  ctx_restore (itk->ctx);
#endif

  focused_no = -1;

  float saved_x0 = itk->x0;
  float saved_width = itk->width;
  float saved_y = itk->y;
  float space_width = ctx_text_width (itk->ctx, " ");


  float y1;
  layout_box_no = 0;
  layout_page_no = 0;

  itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
  itk->y           = layout_box[layout_box_no].y * saved_width;
  itk->width       = layout_box[layout_box_no].width * saved_width ;
  y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;
  ctx_save (itk->ctx);
  ctx_font_size (itk->ctx, itk->font_size);


  if (!layout_config.outliner)
  {
  if (tool_no == 1)
  {
    ctx_rectangle (itk->ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
    ctx_listen (itk->ctx, CTX_DRAG, tool_rect_drag, NULL, NULL);

    ctx_begin_path (itk->ctx);
  }
  }

  if (y1 < 100) y1 = itk->height;

  int printing = (layout_page_no == layout_show_page);
  if (layout_config.outliner)
     printing = 1;

  for (int i = 0; i < files->count; i++)
  {
      if (layout_find_item == i)
      {

         if (!printing)
         {
           layout_show_page = layout_page_no; // change to right page
           ctx_set_dirty (itk->ctx, 1); // queue another redraw
                                        // of the right page we'll find it then
         }
         else
         {
           itk->focus_no = itk->control_no;
           focused_no = i;
           layout_find_item = -1;
         }
      }

      if (itk->control_no == itk->focus_no && layout_find_item < 0)
      {
        focused_no = i;
      }

      const char *d_name = files->items[i];
      float width = 0;
      float height = 0;
      
      int hidden = 0;
      CtxAtom atom = item_get_type_atom (i);

      switch (atom)
      {
        case CTX_ATOM_RECTANGLE:
        case CTX_ATOM_TEXT:
        case CTX_ATOM_FILE:
        case CTX_ATOM_CTX:
          break;
        case CTX_ATOM_LAYOUTBOX:
          break;
        case CTX_ATOM_STARTGROUP:
          if (!layout_config.codes) hidden = 1;
          level ++;
          {
            int folded = metadata_key_int2(i, "folded", 0);

            if (folded && ! is_folded) is_folded = level;
          }
          break;
        case CTX_ATOM_ENDGROUP:
          if (is_folded == level)
          {
            is_folded = 0;
          }
          if (!layout_config.codes) hidden = 1;
          level --;
          break;
        case CTX_ATOM_NEWPAGE:
        case CTX_ATOM_STARTPAGE:
          layout_box_count = 0;
          layout_box_no = 0;
          layout_box[0].x = 0.05;
          layout_box[0].y = 0.02;
          layout_box[0].width = 0.9;
          layout_box[0].height = 4000.0;

          if (!layout_config.codes)
             hidden = 1;
          break;
      }

      if (layout_config.hide_non_file)
      if (atom != CTX_ATOM_FILE) hidden = 1;

      if (is_folded)
        hidden = 1;

      int label = metadata_key_int2 (i, "label", -1234);
      if (label == -1234) {
        if (atom == CTX_ATOM_TEXT)
          label = 0;
        else
          label = layout_config.label;
      }

      int gotpos = 0;
      char *xstr = metadata_key_string2 (i, "x");
      char *ystr = metadata_key_string2 (i, "y");
      float origin_x = metadata_key_float2(i, "origin-x", 0.0);
      float origin_y = metadata_key_float2(i, "origin-y", 0.0);


      float padding_left = metadata_key_float2 (i, "padding-left", layout_config.padding_left);
      float padding_right = metadata_key_float2 (i, "padding-right", layout_config.padding_right);
      float padding_top = metadata_key_float2 (i, "padding-top", layout_config.padding_top);
      float padding_bottom = metadata_key_float2 (i, "padding-bottom", layout_config.padding_bottom);

      //padding_left += level * layout_config.level_indent;

      float x = metadata_key_float2 (i, "x", 0.0);
      float y = metadata_key_float2 (i, "y", 0.0);
      //float opacity = metadata_key_float2 (i, "opacity", 1.0f);

      if (xstr)
      {
        free (xstr);
        gotpos = 1;
      }
      if (ystr)
      {
        free (ystr);
        gotpos = 1;
      }

      if (layout_config.fixed_pos)
        gotpos = 0;

      {
        width  = metadata_key_float2 (i, "width", -1000.0);
        if (width < 0 || layout_config.fixed_size)
          width = 
            layout_config.fill_width? itk->width * 1.0:
            layout_config.width * em;
        else {
          width *= saved_width;
        }
      }

      {
        height = metadata_key_float2 (i, "height", -1000.0);
        if (height < 0 || layout_config.fixed_size)  height =
          layout_config.fill_height? itk->height * 1.0:
          layout_config.height * em;
        else
          height *= saved_width;
      }

      x -= (origin_x * width / saved_width);
      y -= (origin_y * height / saved_width);



      int virtual = (item_get_type_atom (i) == CTX_ATOM_TEXT);

      if (virtual)
        atom = CTX_ATOM_TEXT;
      if (layout_config.outliner)
      {
        gotpos = 0;
        virtual = 1;
      }

      if (layout_config.stack_horizontal && layout_config.stack_vertical)
      {
      if (itk->x + width  > itk->x0 + itk->width || atom == CTX_ATOM_NEWPAGE || atom == CTX_ATOM_STARTPAGE) //panel->x + itk->panel->width)
      {
          itk->x = itk->x0;
          if (layout_config.stack_vertical)
          {
            itk->y += row_max_height;
            row_max_height = 0;
          }
          if (itk->y + height > y1 || atom == CTX_ATOM_NEWPAGE || atom == CTX_ATOM_STARTPAGE)
          {
            if (layout_box_count > layout_box_no+1 && atom != CTX_ATOM_NEWPAGE && atom != CTX_ATOM_STARTPAGE)
            {
              layout_box_no++;

              itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
              itk->y           = layout_box[layout_box_no].y * saved_width;
              itk->width       = layout_box[layout_box_no].width * saved_width ;
              y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;
            }
            else
            {
              layout_box_no = 0;

              itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
              itk->y           = layout_box[layout_box_no].y * saved_width;
              itk->width       = layout_box[layout_box_no].width * saved_width ;
              y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;

              layout_page_no++;
              printing = (layout_page_no == layout_show_page);
              layout_last_page = layout_page_no;
            }
          }
          itk->x  += level * em * layout_config.level_indent;
      }
      }

      if (atom == CTX_ATOM_LAYOUTBOX)
      {
         if (layout_box_count < CTX_MAX_LAYOUT_BOXES)
         {
           layout_box[layout_box_count].x = x;
           layout_box[layout_box_count].y = y;
           layout_box[layout_box_count].width = width / saved_width;
           layout_box[layout_box_count].height = height / saved_width;
           layout_box_count++;

           if (layout_box_count == 1)
           {
             layout_box_no = 0;
             itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
             itk->y           = layout_box[layout_box_no].y * saved_width;
             itk->width       = layout_box[layout_box_no].width * saved_width ;
             y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;
           }
         }
      }


  if (layout_config.outliner)
  {
    label = 0;
    atom = CTX_ATOM_TEXT;
    hidden = 0;
    printing = 1;
    layout_config.codes = 1;
  }
  if (layout_config.codes)
  {
      switch (atom)
      {
        case CTX_ATOM_RECTANGLE: d_name = "rectangle"; break;
        case CTX_ATOM_TEXT:      break;
        case CTX_ATOM_FILE: break;
        case CTX_ATOM_CTX:       d_name = "ctx"; break;
        case CTX_ATOM_LAYOUTBOX: d_name = "layoutbox"; break;
        case CTX_ATOM_STARTGROUP: d_name = "startgroup";
                                  if (layout_config.outliner)
                                    level--; 
                                  break;
        case CTX_ATOM_ENDGROUP:   d_name = "endgroup"; break;
        case CTX_ATOM_NEWPAGE:    d_name = "newpage"; break;
        case CTX_ATOM_STARTPAGE:  d_name = "startpage"; break;
      }
    if ((atom != CTX_ATOM_FILE &&
         atom != CTX_ATOM_RECTANGLE) || layout_config.outliner)
    {
      atom = CTX_ATOM_TEXT;
      label = 0;
    ///  height = em * 1.5;
    }
  }

      if (!hidden)
      {
              //itk->x += level * em * 4;
      float saved_x = itk->x;
      float saved_y = itk->y;
      if (gotpos)
      {
        x *= saved_width;
        y *= saved_width;
        itk->x = x;
        itk->y = y;
      }

      float sx = itk->x,sy = itk->y;
      ctx_user_to_device (itk->ctx, &sx, &sy);


      if (virtual &&  !gotpos)
      {
      }
      if (virtual)
      {
        if (!gotpos)
        {
          width = itk->width - (padding_left+padding_right)*em;
           
          if (layout_config.stack_vertical && itk->x != itk->x0)
            itk->y += row_max_height;

          itk->x = itk->x0 + level * layout_config.level_indent * em;
          /* measure height, and snap cursor */
          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       space_width, width - em * level * layout_config.level_indent, em,
                       i == focused_no ? text_edit : -1,
                       i == focused_no ? text_edit + 2: -1,
                       0, NULL, &height,
                       NULL, NULL,
                       (i == focused_no && text_edit >= 0),
                       i == focused_no);
          height = height - itk->y + em * 0.5;
          row_max_height = height;
        }
        //fprintf (stderr, "%f\n", height);
      }

      CtxControl *c = NULL;
      if (printing)
      {
        ctx_begin_path (itk->ctx);
        c = itk_add_control (itk, UI_LABEL, "foo",
        itk->x - em * padding_left, itk->y - em * padding_top,
        width + em * (padding_left+padding_right),
        height + em * (padding_top+padding_bottom));
        if (focused_no == i)
           focused_control = c;
      }

      if (!active && printing && sy + height > 0 && sy < ctx_height (itk->ctx))
      {
              //ctx_rgb(itk->ctx,1,0,0);
              //ctx_fill(itk->ctx);
        struct stat stat_buf;
        char *newpath = malloc (strlen(files->path)+strlen(d_name) + 2);
        if (!strcmp (files->path, PATH_SEP))
          sprintf (newpath, "%s%s", PATH_SEP, d_name);
        else
          sprintf (newpath, "%s%s%s", files->path, PATH_SEP, d_name);
        int focused = 0;

        const char *media_type = "inline/text";
        
        if (lstat (newpath, &stat_buf) == 0)
          media_type = ctx_path_get_media_type (newpath);

        if (!strcmp (media_type, "inode/directory") &&
                      !layout_config.list_data &&
                      !layout_config.outliner)
        {
          label = 1;
        }

        if (!strcmp (media_type, "inline/text"))
        {
           if (atom == CTX_ATOM_RECTANGLE)
           {
             media_type = "ctx/rectangle";
           }
           else if (atom == CTX_ATOM_CTX)
           {
             media_type = "ctx/ctx";
           }
           else if (atom == CTX_ATOM_TEXT)
           {
           }
           else if (atom == CTX_ATOM_FILE)
           {
           }
        }


        switch (tool_no)
        {
                case 0:
        if (c->no == itk->focus_no && layout_find_item < 0)
        {
          focused = 1;
          //fprintf (stderr, "\n{%i %i %i}\n", c->no, itk->focus_no, i);
          //viewer_load_path (newpath, files->items[i]);
          ctx_begin_path (itk->ctx);
          ctx_rectangle (itk->ctx, c->x, c->y, c->width, c->height);
          ctx_listen (itk->ctx, CTX_TAP_AND_HOLD, item_activate, (void*)(size_t)i, NULL);
          ctx_listen (itk->ctx, CTX_DRAG, item_drag, (void*)(size_t)i, NULL);
          //ctx_listen (itk->ctx, CTX_TAP_AND_HOLD, item_tap_and_hold, (void*)(size_t)i, NULL);


          ctx_begin_path (itk->ctx);
          //ctx_rgb(itk->ctx,1,0,0);
          //ctx_fill(itk->ctx);

          if (!active)
          {
          if (!is_text_editing())
          {

            if (history)
            ctx_add_key_binding (ctx, "alt-left", NULL, "back", history_back, NULL);
            if (future)
            ctx_add_key_binding (ctx, "alt-right", NULL, "forward", history_forward, NULL);

            ctx_add_key_binding (ctx, "alt-up", NULL, "go to parent",
                          dir_go_parent,
                          (void*)((size_t)i));

            ctx_add_key_binding (ctx, "alt-down", NULL, "enter item",
                          item_activate,
                          (void*)((size_t)i));


            ctx_add_key_binding (ctx, "control-d", NULL, "duplicate item",
                          item_duplicate,
                          (void*)((size_t)i));

            ctx_add_key_binding (ctx, "delete", NULL, "remove item",
                          item_delete,
                          (void*)((size_t)i));

            if (!layout_config.outliner)
            {
               ctx_add_key_binding (ctx, "alt-return", NULL, "item properties",
                          item_properties,
                          (void*)((size_t)i));


            if (layout_focused_link >= 0)
            {
            ctx_add_key_binding (ctx, "return", NULL, "follow link",
                          dir_follow_link,
                          (void*)((size_t)i));
            }
            else
            ctx_add_key_binding (ctx, "return", NULL, "activate/edit",
                          item_activate,
                          (void*)((size_t)i));
            

            {
              int bullet = metadata_key_int2 (i, "bullet", CTX_BULLET_NONE);
              const char *label = "cycle bullet";
              switch (bullet)
              {
                case CTX_BULLET_NONE:   label = "make bullet"; break;
                case CTX_BULLET_BULLET: label = "make numbered"; break;
                case CTX_BULLET_NUMBERS: label = "make todo"; break;
                case CTX_BULLET_TODO:   label = "mark done"; break;
                case CTX_BULLET_DONE:   label = "no bullet"; break;
              }
              ctx_add_key_binding (ctx, "control-t", NULL, label,
                          item_cycle_bullet,
                          (void*)((size_t)i));
            }


            {
              const char *label = "make heading";
              char *klass = metadata_key_string2 (i, "class");
              if (klass)
              {
                label = "cycle heading";
                free (klass);
              }
              else
              {

              }
              ctx_add_key_binding (ctx, "control-h", NULL, label,
                          item_cycle_heading, NULL);
            }

            ctx_add_key_binding (ctx, "insert", NULL, "insert text",
                          dir_insert, NULL);
            }
          }


          if (!layout_config.outliner && !is_text_editing ())
          {

          ctx_add_key_binding (ctx, "control-page-down", NULL, "move after next sibling", move_after_next_sibling, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-page-up", NULL, "move before previous sibling", move_before_previous_sibling, 
                         (void*)((size_t)i));

          if (gotpos)
          {
          ctx_add_key_binding (ctx, "shift-control-down", NULL, "grow height", grow_height, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "shift-control-up", NULL, "shrink height", shrink_height, 
                         (void*)((size_t)i));

          ctx_add_key_binding (ctx, "shift-control-left", NULL, "shrink width", shrink_width, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "shift-control-right", NULL, "grow width", grow_width, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-left", NULL, "move left", move_left, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-right", NULL, "move right", move_right, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-up", NULL, "move up", move_up, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-down", NULL, "move down", move_down, 
                         (void*)((size_t)i));
          }
          else
          {
            ctx_add_key_binding (ctx, "control-down", NULL,
                            "move after next sibling", move_after_next_sibling, (void*)((size_t)i));
            ctx_add_key_binding (ctx, "control-up", NULL,
                            "move before previous sibling", move_before_previous_sibling, (void*)((size_t)i));

            ctx_add_key_binding (ctx, "control-left", NULL,
                            "make sibling of parent", make_sibling_of_parent, 
                           (void*)((size_t)i));

            ctx_add_key_binding (ctx, "control-right", NULL,
                            "make child of previous", make_child_of_previous, 
                           (void*)((size_t)i));

          }
          }


           if (!is_text_editing())
           {
               if (metadata_key_int2(i + 1, "folded", -1)>0)
               {
               ctx_add_key_binding (ctx, "+", NULL, "expand",
                          outline_expand,
                          (void*)((size_t)i));
               ctx_add_key_binding (ctx, "=", NULL, "expand",
                          outline_expand,
                          (void*)((size_t)i));
               }
               else if (item_get_type_atom (i + 1) == CTX_ATOM_STARTGROUP)
               {
               ctx_add_key_binding (ctx, "-", NULL, "fold",
                          outline_collapse,
                          (void*)((size_t)i));
               }
           }

          }
          //itk_labelf (itk, "%s\n", ctx_path_get_media_type (newpath));
        }
        else
        {
          ctx_listen (itk->ctx, CTX_PRESS, dir_select_item, (void*)(size_t)c->no, NULL);
        }
        break;
        }

      ctx_begin_path (ctx);
      ctx_gray (ctx, 0.0);

        if (atom == CTX_ATOM_TEXT)
        {
          ctx_save (itk->ctx);

          //if (c->no == itk->focus_no)
          //  ctx_rgb (itk->ctx, 1, 0, text_edit>=0?0:1);
          //else
          //  ctx_rgb (itk->ctx, 1, 0, 1);
          ctx_gray (itk->ctx, 0.95);


          if (c->no == itk->focus_no && layout_find_item < 0)
          {
          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       space_width, width - em * level * layout_config.level_indent, em,
                       text_edit,text_edit,
                       1, NULL, NULL,
                       &prev_line_pos, &next_line_pos,
                       (i == focused_no && text_edit >= 0),
                       1);
          }
          else
          {

          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       space_width, width - em * level * layout_config.level_indent, em,
                       -1, -1,
                       1, NULL, NULL,
                       NULL, NULL,
                       0,
                       0);
          }

          int bullet = metadata_key_int2 (i, "bullet", CTX_BULLET_NONE);
          if (bullet != CTX_BULLET_NONE)
          {
             float x = itk->x - em * 0.7 ;//+ level * em * layout_config.level_indent;
             switch (bullet)
             {
               case CTX_BULLET_NONE:
               break;
               case CTX_BULLET_BULLET:
               ctx_move_to (itk->ctx, x, itk->y + em);
               ctx_text (itk->ctx, "-");
               break;
               case CTX_BULLET_NUMBERS:
               ctx_move_to (itk->ctx, x + em * 0.5, itk->y + em);
               {
                 char buf[64]="";
                 sprintf (buf, "%i", item_get_list_index (i));
                 ctx_save (itk->ctx);
                 ctx_text_align (itk->ctx, CTX_TEXT_ALIGN_RIGHT);
                 ctx_text (itk->ctx, buf);
                 ctx_restore (itk->ctx);
               }
               break;
               case CTX_BULLET_DONE:
               ctx_move_to (itk->ctx, x, itk->y + em);
               ctx_text (itk->ctx, "☑"); //☒
               break;
               case CTX_BULLET_TODO:
               ctx_move_to (itk->ctx, x, itk->y + em);
               ctx_text (itk->ctx, "☐");
               break;
             }
          }
          {
            int folded = metadata_key_int2 (i+1, "folded", -3);
            if (folded > 0)
            {
               float x = itk->x - em * 0.5;//
               if (bullet != CTX_BULLET_NONE) x -= em * 0.6;
               ctx_move_to (itk->ctx, x, itk->y + em);
               ctx_text (itk->ctx, "▶");//▷▽▼");
            }
          }


          //if (c->no == itk->focus_no)
          //fprintf (stderr, "%f %i %i %i\n", text_edit_desired_x, text_edit, prev_line, next_line);

          ctx_restore (itk->ctx);
        }
      else
        if (atom == CTX_ATOM_RECTANGLE)
        {
          char *fill       = metadata_key_string2(i, "fill");
          char *stroke     = metadata_key_string2(i, "stroke");
          float line_width = metadata_key_float2(i, "line-width", 1.0);
          //float opacity    = metadata_key_float2(i, "opacity", 1.0);

          ctx_rectangle (itk->ctx, itk->x, itk->y, width, height);
          if (fill)
          {
            ctx_color (itk->ctx, fill);
            if (stroke)
              ctx_preserve (itk->ctx);
            ctx_fill (itk->ctx);
            free (fill);
          }
        
          if (stroke)
          {
            ctx_color (itk->ctx, stroke);
            ctx_line_width (itk->ctx, line_width);
            ctx_stroke (itk->ctx);
            free (stroke);
          }
        }
        else if (atom == CTX_ATOM_LAYOUTBOX)
        {
        }
        else if (atom == CTX_ATOM_CTX)
        {
          ctx_save (itk->ctx);
          ctx_translate (itk->ctx, itk->x, itk->y);
          ctx_parse (itk->ctx, d_name);
          ctx_restore (itk->ctx);
        }
        else if (ctx_media_type_class (media_type) == CTX_MEDIA_TYPE_IMAGE)
        {
          if (gotpos) label = 0;
          draw_img (itk, itk->x, itk->y, width, height, newpath);
          if (c->no == itk->focus_no && layout_find_item < 0 && gotpos)
          {
             float em = itk->font_size;
             int resize_dim = 4;

             ctx_arc (itk->ctx, itk->x + width * origin_x,
                           itk->y + height * origin_y,
                           0.5 * em,
                           0.0,
                           M_PI * 2, 0);
             ctx_rgba (itk->ctx, 1, 1,0, 0.9);
             ctx_fill (itk->ctx);

             ctx_rectangle (itk->ctx, itk->x + resize_dim * em, itk->y, width - resize_dim * 2 * em, resize_dim * em);
             ctx_rgba (itk->ctx, 0,0,0, 0.5);
             ctx_fill (itk->ctx);


             ctx_rectangle (itk->ctx, itk->x + width - resize_dim * em,
                                      itk->y + height - resize_dim * em,
                                      resize_dim * em, resize_dim * em);
             ctx_rgba (itk->ctx, 0,0,0, 0.5);
             ctx_fill (itk->ctx);
          }
        }
        else if (!strcmp (media_type, "inode/directory"))
        {
          draw_folder (ctx, itk->x, itk->y, width, height);
        }
        else
        {
          draw_doc (ctx, itk->x, itk->y, width, height);
        }

      if (layout_config.list_data)
      {

        ctx_save (itk->ctx);
        ctx_gray (itk->ctx, 1.0);

        ctx_move_to (itk->ctx, itk->x + width + em * 0.5, itk->y + em * (1 + layout_config.padding_top) );
        ctx_text (itk->ctx, d_name);

        if (strcmp (media_type, "inode/directory"))
        {
          char buf[1024];
          ctx_move_to (itk->ctx, itk->x + itk->width - em * 15.5, itk->y + em * (1 + layout_config.padding_top) );
          ctx_save (itk->ctx);
          ctx_text_align (itk->ctx, CTX_TEXT_ALIGN_RIGHT);
          sprintf (buf, "%li", (long int)stat_buf.st_size);
          ctx_text (itk->ctx, buf);
          ctx_restore (itk->ctx);
        }

        ctx_move_to (itk->ctx, itk->x + itk->width - em * 15.0, itk->y + em * (1 + layout_config.padding_top) );
        ctx_text (itk->ctx, media_type);

        ctx_restore (itk->ctx);

      }

      free (newpath);

      if (label)
      {
      char *title = malloc (strlen (files->items[i]) + 32);
      strcpy (title, files->items[i]);
      int title_len = strlen (title);
      {
        int wraplen = 10;
        int lines = (title_len + wraplen - 1)/ wraplen;

        if (!focused &&  lines > 2) lines = 2;

        ctx_rectangle (itk->ctx, itk->x, itk->y + height - em * (lines + 1),
                        width, em * (lines + 0.5));
        ctx_rgba (itk->ctx, 0,0,0,0.6);
        ctx_fill (itk->ctx);

        for (int i = 0; i < lines; i++)
        {
          ctx_move_to (itk->ctx, itk->x + em * 3, itk->y + height + em * i - lines * em);
          ctx_rgba (itk->ctx, 1,1,1,0.6);
          ctx_save (itk->ctx);
          ctx_text_align (itk->ctx, CTX_TEXT_ALIGN_CENTER);

          if (i * wraplen + wraplen < title_len)
          {
            int tmp = title[i * wraplen + wraplen];
            title[i * wraplen + wraplen] = 0;
            ctx_text (itk->ctx, &title[i * wraplen]);
            title[i * wraplen + wraplen] = tmp;
          }
          else
          {
            ctx_text (itk->ctx, &title[i * wraplen]);
          }
          ctx_restore (itk->ctx);
        }
      }
        free (title);
      }
      }

      prev_height = height;
      if (prev_height > row_max_height && !gotpos)
        row_max_height = prev_height;



      if (gotpos)
      {
        itk->x = saved_x;
        itk->y = saved_y;
      }
      else
      {
        itk->x = saved_x + width;
        if ((virtual) || 
            !layout_config.stack_horizontal)
        {
          itk->x = itk->x0;
          if (layout_config.stack_vertical)
          {
            itk->y += row_max_height;
            row_max_height = 0;
          }
#if 1
          if (itk->y > y1 )
          {
            if (layout_box_count > layout_box_no+1)
            {
              layout_box_no++;

             itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
             itk->y           = layout_box[layout_box_no].y * saved_width;
             itk->width       = layout_box[layout_box_no].width * saved_width ;
             y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;
            }
            else
            {
              layout_box_no = 0;

             itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
             itk->y           = layout_box[layout_box_no].y * saved_width;
             itk->width       = layout_box[layout_box_no].width * saved_width ;
             y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;

             layout_page_no++;
             layout_last_page = layout_page_no;
             printing = (layout_page_no == layout_show_page);
            }

          }

#endif
          itk->x += level * layout_config.level_indent * em;
        }
      }


      if (layout_config.outliner)
      {
  int keys = metadata_item_key_count2 (i);
  for (int k = 0; k < keys; k++)
  {
    char *key = metadata_key_name2 (i, k);
    if (key)
    {
      char *val = metadata_key_string2 (i, key);
      if (val)
      {
        itk->x += level * em * 3 + em;
        itk_labelf (itk, "%s=%s", key, val);
        free (val);
      }
      free (key);
    }
  }

      if (!strcmp (d_name, "startgroup"))
      {
        level++;
      }
      }

      }
  }

  ctx_restore (itk->ctx);

  itk->x0    = saved_x0;
  itk->width = saved_width;
  itk->y     = saved_y;


}

static void empty_thumb_queue (void)
{
  while (thumb_queue)
  {
    ThumbQueueItem *item = thumb_queue->data;
    ctx_list_remove (&thumb_queue, item);
    free (item->path);
    free (item->thumbpath);
    free (item);
  }
}

static int thumb_monitor (Ctx *ctx, void *data)
{
  static pid_t thumb_pid = 0;
  int    initial_args = 2;
  char *argv[100]={"ctx", "thumb", };

  if (thumb_queue == NULL)
    return 1;

  if (thumb_pid == 0 || kill (thumb_pid, 0) == -1)
  {
    int batch_size = 16;
    int count = initial_args;
    for (CtxList *iter = thumb_queue;iter && count < batch_size + initial_args; iter = iter->next)
    {
      ThumbQueueItem *item = iter->data;
      if (access (item->thumbpath, F_OK) == -1)
      {
        argv[count++] = item->path;
        argv[count] = NULL;
      }
    }
    if (count != initial_args)
    {
      thumb_pid = fork();
      if (thumb_pid == 0)
      {
        return execvp (argv[0], argv);
      }
    }
    else
    {
      thumb_pid = 0;
    }
    empty_thumb_queue ();
    ctx_set_dirty (ctx, 1);
  }
  else
  {
    int status = 0;
    waitpid(0, &status, WNOHANG); // collect finished thumb generators
    ctx_set_dirty (ctx, 1);
  }
  return 1;
}

extern float font_size;
int  ctx_clients_draw (Ctx *ctx);
void ctx_clients_handle_events (Ctx *ctx);

static char *viewer_loaded_path = NULL;
static char *viewer_loaded_name = NULL;

int viewer_load_next (Ctx *ctx, void *data1)
{
  if (viewer_no+1 >= metadata_count())
    return 0;
  //fprintf (stderr, "next!\n");
  itk->focus_no ++;
  item_activate (NULL, (void*)(size_t)(viewer_no+1), NULL);
  ctx_set_dirty (ctx, 1);
  return 0;
}

void viewer_load_path (const char *path, const char *name)
{
  if (path && viewer_loaded_path && !strcmp (viewer_loaded_path, path) && clients)
  {
    return;
  }
  if (viewer_loaded_path)
  {
    while (clients)
      ctx_client_remove (ctx, clients->data);
    active = NULL;
    ctx_set_dirty (ctx, 1);
    free (viewer_loaded_path);
    viewer_loaded_path = NULL;
  }

  if (viewer_loaded_name) free (viewer_loaded_name);
  viewer_loaded_name = strdup (name);

  int no = metadata_item_to_no (name);

  float duration = 10.0;
  float in = metadata_key_float2 (no, "in", -1);
  float out = metadata_key_float2 (no, "out", -1);
  if (out > 0 && in > 0)
  {
    duration = out - in;
  }

  if (viewer_load_next_handler!=0)
    ctx_remove_idle (ctx, viewer_load_next_handler);
  viewer_load_next_handler = 0;

  //fprintf (stderr, "%f\n", duration);
  viewer_load_next_handler = ctx_add_timeout (ctx, 1000 * duration, viewer_load_next, NULL);

  if (path)
  {
    viewer_loaded_path = strdup (path);
    const char *media_type = ctx_path_get_media_type (path);
    viewer_media_type = media_type;
    CtxMediaTypeClass media_type_class = ctx_media_type_class (media_type);
    char *escaped_path = malloc (strlen (path) * 2 + 2);
    char *command = malloc (32 + strlen (path) * 2 + 64);
    int j = 0;
    for (int i = 0; path[i]; i++)
    {
      switch (path[i])
      {
        case ' ':
        case '`':
        case '$':
        case '[':
        case ']':
        case '\'':
        case '"':
          escaped_path[j++]='\\';
          escaped_path[j++]=path[i];
          break;
        default:
          escaped_path[j++]=path[i];
          break;
      }
    }
    escaped_path[j]=0;
    command[0]=0;
    if (!strcmp (media_type, "inode/directory"))
    {
       //fprintf (stderr, "is dir\n");
       return;
       //sprintf (command, "du -h '%s'", path);
    }

    char *basname = get_basename (path);

    if (!command[0])
    if (media_type_class == CTX_MEDIA_TYPE_TEXT)
    {
      sprintf (command, "vim +1 -R %s", escaped_path);
    }
    free (basname);
   
    if (!command[0])
    {
    if (media_type_class == CTX_MEDIA_TYPE_IMAGE)
    {
      sprintf (command, "ctx %s", escaped_path);
    }
    else if (!strcmp (media_type, "video/mpeg"))
    {
      sprintf (command, "ctx -s %s", escaped_path);
    }
    else if (media_type_class == CTX_MEDIA_TYPE_AUDIO)
    {
      sprintf (command, "ctx-audioplayer %s", escaped_path);
    }
    }

    if (!command[0])
    {
      sprintf (command, "sh -c 'xxd %s | vim -R -'", escaped_path);
    }

    if (command[0])
    {
      //fprintf (stderr, "ctx-dir:%f\n", itk->font_size);
      active = ctx_client_new (ctx, command,
        0, 0, ctx_width(ctx), ctx_height(ctx), 0);
    //fprintf (stderr, "[%s]\n", command);
#if 0
      fprintf (stderr, "run:%s %i %i %i %i,   %i\n", command,
        (int)ctx_width(ctx)/2, (int)0, (int)ctx_width(ctx)/2, (int)(ctx_height(ctx)-font_size*2), 0);
#endif
    }
    else
    {
      active = NULL;
    }

    free (escaped_path);
    free (command);
  }
  else
  {
    active = NULL;
  }
}

static void dir_ignore (CtxEvent *e, void *d1, void *d2)
{
  e->stop_propagate = 1;
}

static void dir_backspace (CtxEvent *e, void *d1, void *d2)
{
  ctx_set_dirty (e->ctx, 1);
  e->stop_propagate = 1;
  if (commandline_cursor_start != commandline_cursor_end)
  {
    int c_s = commandline_cursor_start;
    int c_e = commandline_cursor_end;
    if (c_s > c_e)
    {
      c_e = commandline_cursor_start;
      c_s = commandline_cursor_end;
    }
    for (int i = c_s; i < c_e; i++)
      ctx_string_remove (commandline, c_s);
    commandline_cursor_start = c_s;
    return;
  }

  if (commandline_cursor_start == 0) return;

  ctx_string_remove (commandline, commandline_cursor_start-1);
  commandline_cursor_start --;
  commandline_cursor_end = commandline_cursor_start;
}

static void dir_run_commandline (CtxEvent *e, void *d1, void *d2)
{
  CtxString *word = ctx_string_new ("");
  ctx_set_dirty (e->ctx, 1);
  e->stop_propagate = 1;

  for (int i = 0; commandline->str[i] && commandline->str[i] != ' '; i++)
  {
    ctx_string_append_byte (word, commandline->str[i]);
  }

  if (!strcmp (word->str, "cd.."))
  {
    dir_go_parent (e, d1, d2);
    layout_show_page = 0;
    itk_panels_reset_scroll (itk);
  }
  else if (!strcmp (word->str, "cd"))
  {
    char *arg = &commandline->str[2];
    if (*arg) arg++;

    if (*arg == 0)
    {
      set_location (getenv ("HOME"));
      layout_find_item = 0;
    }
    else if (!strcmp (arg, ".."))
    {
      dir_go_parent (e, d1, d2);
    }
    else
    {
      if (arg[0] == '/')
      {
        set_location (arg);
      }
      else
      {
        char *new_path = ctx_strdup_printf ("%s/%s", files->path, arg);
        set_location (new_path);
        free (new_path);
      }
      layout_find_item = 0;
    }
    layout_show_page = 0;
    itk_panels_reset_scroll (itk);
  }
  else
  {
     // all other code should be doing absolute paths,
     // this makes it easier to launch commands
     chdir (files->path);
     system (commandline->str);
     metadata_dirt();
  }

  ctx_string_set (commandline, "");
  commandline_cursor_end =
  commandline_cursor_start = 0;

  ctx_string_free (word, 1);
}

int editing_location = 0;
static void dir_location (CtxEvent *e, void *d1, void *d2)
{
  editing_location = 1;
  ctx_string_set (commandline, files->title?files->title:files->path);
  commandline_cursor_end = 0;
  commandline_cursor_start = strlen (commandline->str);
  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}

static void dir_location_escape (CtxEvent *e, void *d1, void *d2)
{
  editing_location = 0;
  commandline_cursor_end =
  commandline_cursor_start = 0;
  ctx_string_set (commandline, "");
  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}



static void dir_location_return (CtxEvent *e, void *d1, void *d2)
{
  editing_location = 0;
  set_location (commandline->str);
  ctx_string_set (commandline, "");
  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}

// tab

static void dir_location_left (CtxEvent *e, void *d1, void *d2)
{
  int c_start = commandline_cursor_start;
  int c_end   = commandline_cursor_end;
  if (c_end < c_start)
  {
    c_end   = commandline_cursor_start;
    c_start = commandline_cursor_end;
  }
  if (c_start == c_end) 
    commandline_cursor_start = c_start - 1;
  else
    commandline_cursor_start = c_start;

  if (commandline_cursor_start < 0) commandline_cursor_start = 0;
  commandline_cursor_end = commandline_cursor_start;
  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}

static void dir_location_right (CtxEvent *e, void *d1, void *d2)
{
  int c_start = commandline_cursor_start;
  int c_end   = commandline_cursor_end;
  if (c_end < c_start)
  {
    c_end   = commandline_cursor_start;
    c_start = commandline_cursor_end;
  }
  if (c_start == c_end) 
    commandline_cursor_start = c_end + 1;
  else
    commandline_cursor_start = c_end;
  if (commandline_cursor_start > (int)strlen (commandline->str))
    commandline_cursor_start = strlen (commandline->str);
  commandline_cursor_end = commandline_cursor_start;
  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}

static void dir_location_extend_sel_right (CtxEvent *e, void *d1, void *d2)
{
  commandline_cursor_end ++;
  if (commandline_cursor_end > (int) strlen (commandline->str))
    commandline_cursor_end = strlen (commandline->str);

  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}

static void dir_location_select_all (CtxEvent *e, void *d1, void *d2)
{
  commandline_cursor_start = 0;
  commandline_cursor_end = strlen (commandline->str);

  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}

static void dir_location_extend_sel_left (CtxEvent *e, void *d1, void *d2)
{
  commandline_cursor_end --;
  if (commandline_cursor_end < 0) commandline_cursor_end = 0;

  e->stop_propagate = 1;
  ctx_set_dirty (e->ctx, 1);
}

static void dir_any (CtxEvent *e, void *d1, void *d2)
{
  const char *str = e->string;
  e->stop_propagate = 1;

  if (!strcmp (str, "space"))
  {
    str = " ";
  }

  if (commandline_cursor_start != commandline_cursor_end)
  {
    int c_s = commandline_cursor_start;
    int c_e = commandline_cursor_end;
    if (c_s > c_e)
    {
      c_e = commandline_cursor_start;
      c_s = commandline_cursor_end;
    }
    for (int i = c_s; i < c_e; i++)
      ctx_string_remove (commandline, c_s);
    commandline_cursor_start = c_s;
  }

  if (ctx_utf8_strlen (str) == 1)
  {
    ctx_string_insert_utf8 (commandline, commandline_cursor_start, str);
    ctx_set_dirty (e->ctx, 1);
    commandline_cursor_start += strlen (str);
    commandline_cursor_end = commandline_cursor_start;
  }
}

static int card_files (ITK *itk_, void *data)
{
  itk = itk_;
  ctx = itk->ctx;
  //float em = itk_em (itk);
  //float row_height = em * 1.2;
  static int first = 1;
  if (first)
  {
    ctx_add_timeout (ctx, 1000, thumb_monitor, NULL);
    font_size = itk->font_size;
    first = 0;
  }
  //thumb_monitor (ctx, NULL);
  save_metadata();

  ctx_add_key_binding (ctx, "F1", NULL, "toggle keybinding help",
                       toggle_keybindings_display,
                       NULL);

  itk_panel_start (itk, "", 0,0, ctx_width(ctx),
                  ctx_height (ctx));

  if (dir_scale != 1.0f)
     ctx_scale (itk->ctx, dir_scale, dir_scale);


    if (!is_text_editing())
    {
          ctx_add_key_binding (ctx, "control-+", NULL, "zoom in",
                          dir_zoom_in,
                          NULL);
          ctx_add_key_binding (ctx, "control-=", NULL, "zoom in",
                          dir_zoom_in,
                          NULL);
          ctx_add_key_binding (ctx, "control--", NULL, "zoom out",
                          dir_zoom_out,
                          NULL);
          ctx_add_key_binding (ctx, "control-0", NULL, "zoom reset",
                          dir_zoom_reset,
                          NULL);

          ctx_add_key_binding (ctx, "shift-control-+", NULL, "increase font size",
                          dir_font_up,
                          NULL);
          ctx_add_key_binding (ctx, "shift-control-=", NULL, "increase font size",
                          dir_font_up,
                          NULL);
          ctx_add_key_binding (ctx, "shift-control--", NULL, "decrease font size",
                          dir_font_down,
                          NULL);
          //ctx_add_key_binding (ctx, "control-0", NULL, "reset font size",
         //                 dir_zoom_reset,
         //                 NULL);
    }


  if (!files->n)
  {
    itk_labelf (itk, "no items\n");
  }
  else
  {
    if (!is_text_editing())
    {

    ctx_add_key_binding (ctx, "control-r", NULL, 
                    layout_config.codes?"hide codes":"reveal codes", toggle_reveal_codes, NULL);
    ctx_add_key_binding (ctx, "control-1", NULL, "debug outline view", set_outline, NULL);
    ctx_add_key_binding (ctx, "control-2", NULL, "layout view", set_layout, NULL);
    ctx_add_key_binding (ctx, "control-3", NULL, "list view", set_list, NULL);
    ctx_add_key_binding (ctx, "control-4", NULL, "folder view", set_grid, NULL);
    }

    //if (layout_config.outliner)
    //  outliner_layout (itk, files);
    //else
      dir_layout (itk, files);

  }

  if (!is_text_editing())
  {
    if (layout_show_page < layout_last_page)
    ctx_add_key_binding (ctx, "page-down", NULL, "next page",
                         dir_next_page, NULL);
    if (layout_show_page > 0)
      ctx_add_key_binding (ctx, "page-up", NULL, "previous page",
                           dir_prev_page, NULL);
  }


  if (!active && text_edit <= TEXT_EDIT_OFF && !layout_config.outliner)
  {


          if (commandline->str[0])
          {
            ctx_add_key_binding (ctx, "backspace", NULL, "remove char",
                            dir_backspace,
                            NULL);
            ctx_add_key_binding (ctx, "return", NULL, "run commandline",
                            dir_run_commandline,
                            NULL);
          ctx_add_key_binding (ctx, "left", NULL, "add char", dir_location_left, NULL);
          ctx_add_key_binding (ctx, "right", NULL, "add char", dir_location_right, NULL);

          ctx_add_key_binding (ctx, "control-a", NULL, "add char", dir_location_select_all, NULL);

          ctx_add_key_binding (ctx, "shift-left", NULL, "add char", dir_location_extend_sel_left, NULL);
          ctx_add_key_binding (ctx, "shift-right", NULL, "add char", dir_location_extend_sel_right, NULL);
            ctx_add_key_binding (ctx, "up", NULL, NULL,
                            dir_ignore,
                            NULL);
            ctx_add_key_binding (ctx, "down", NULL, NULL,
                            dir_ignore,
                            NULL);
          }

          ctx_add_key_binding (ctx, "any", NULL, "add char", dir_any, NULL);
          if (editing_location)
          {
            ctx_add_key_binding (ctx, "escape", NULL, "stop editing location", dir_location_escape, NULL);
            ctx_add_key_binding (ctx, "return", NULL, "confirm new location", dir_location_return, NULL);
          }
          ctx_add_key_binding (ctx, "control-l", NULL, "location entry", dir_location, NULL);

          if (item_get_type_atom (focused_no) == CTX_ATOM_TEXT &&
              metadata_key_float2(focused_no, "x", -1234.0) == -1234.0 &&
              !is_text_editing())
          {

            if (layout_focused_link >= 0)
            ctx_add_key_binding (ctx, "up", NULL, "focus previous link",
                          focus_previous_link,
                          NULL);
            else

            ctx_add_key_binding (ctx, "up", NULL, "focus previous sibling",
                          focus_previous_sibling,
                          NULL);

            //fprintf (stderr, "%i \n", dir_item_count_links (focused_no));

            if (focused_no >=0 &&
                layout_focused_link < dir_item_count_links (focused_no)-1)
            ctx_add_key_binding (ctx, "down", NULL, "focus next link",
                          focus_next_link,
                          NULL);
            else
            ctx_add_key_binding (ctx, "down", NULL, "focus next sibling",
                          focus_next_sibling,
                          NULL);

            if (0)
            {
            ctx_add_key_binding (ctx, "left", NULL, "focus prev",
                          focus_previous,
                          NULL);

            ctx_add_key_binding (ctx, "right", NULL, "focus next",
                            focus_next,
                          NULL);
            }
            else
            {
            ctx_add_key_binding (ctx, "left", NULL, "focus parent",
                          dir_parent,
                          NULL);

            if (layout_focused_link >= 0)
            ctx_add_key_binding (ctx, "right", NULL, "follow link",
                          dir_follow_link, NULL);
            else

            ctx_add_key_binding (ctx, "right", NULL, "enter children",
                          dir_enter_children,
                          NULL);
            }
          }
  }


#if 1
      if (!active && text_edit>TEXT_EDIT_OFF)
      {
          ctx_add_key_binding (ctx, "tab", NULL, NULL,
                          text_edit_ignore,
                          NULL);
          ctx_add_key_binding (ctx, "shift-tab", NULL, NULL,
                          text_edit_ignore,
                          NULL);

          ctx_add_key_binding (ctx, "control-left", NULL, "previous word",
                          text_edit_control_left,
                          NULL);
          ctx_add_key_binding (ctx, "control-right", NULL, "next word",
                          text_edit_control_right,
                          NULL);
          ctx_add_key_binding (ctx, "left", NULL, "previous char",
                          text_edit_left,
                          NULL);
          ctx_add_key_binding (ctx, "up", NULL, "previous line",
                          text_edit_up,
                          NULL);

          ctx_add_key_binding (ctx, "down", NULL, "next line",
                          text_edit_down,
                          NULL);
          ctx_add_key_binding (ctx, "right", NULL, "next char",
                          text_edit_right,
                          NULL);
          ctx_add_key_binding (ctx, "escape", NULL, "stop editing",
                          text_edit_stop,
                          NULL);
          ctx_add_key_binding (ctx, "control-return", NULL, "stop editing",
                          text_edit_stop,
                          NULL);
          ctx_add_key_binding (ctx, "shift-return", NULL, "hard newline",
                            text_edit_shift_return,
                            NULL);

          ctx_add_key_binding (ctx, "any", NULL, "insert character",
                          text_edit_any,
                          NULL);

          ctx_add_key_binding (ctx, "home", NULL, "go to start of paragraph",
                          text_edit_home,
                          NULL);
          ctx_add_key_binding (ctx, "end", NULL, "go to end of paragraph",
                          text_edit_end,
                          NULL);

          ctx_add_key_binding (ctx, "return", NULL, "split paragraph",
                          text_edit_return,
                          NULL);

          if (text_edit == 0)
          ctx_add_key_binding (ctx, "backspace", NULL, "merge with preceding paragraph",
                          text_edit_backspace,
                          NULL);
          else
          ctx_add_key_binding (ctx, "backspace", NULL, "remove preceding character",
                          text_edit_backspace,
                          NULL);
          ctx_add_key_binding (ctx, "delete", NULL, "remove character",
                          text_edit_delete,
                          NULL);

      }
#endif

  if (item_context_active && focused_control && focused_no>=0)
  {
    char *choices[]={
    "make absolute positioned (ctrl-space)",
    "move up (ctrl-up)",
    "move down (ctrl-down)",
    "indent (ctrl-right)",
    "outdent (ctrl-left)",
            NULL
    };

    float em = itk->font_size;
    float width = em * 20;
    float x = focused_control->x + focused_control->width;
    float y = focused_control->y;
    float height = em * 9;

    if (width < focused_control->width) x-= width;
    else
    {
       if (x + width > itk->width)
       {
         x = focused_control->x - width;
       }
    }

    ctx_begin_path (itk->ctx);
    ctx_rgba (itk->ctx, 0.0,0.0,0.0, 0.4);
    ctx_rectangle (itk->ctx, 0, 0, ctx_width (itk->ctx), ctx_height (itk->ctx));
    ctx_fill (itk->ctx);
    ctx_rgba (itk->ctx, 0.0,0.0,0.0, 0.8);
    ctx_rectangle (itk->ctx, x, y, width, height);

    ctx_fill (itk->ctx);

    for (int i =0; choices[i]; i++)
    {
      if (i + 1 == item_context_active)
         ctx_rgb(itk->ctx, 1,1,1);
      else
         ctx_rgb(itk->ctx, 0.7,0.7,0.7);
      y+=em;

      ctx_move_to (itk->ctx, x+em, y);
      ctx_text( itk->ctx, choices[i]);
    }


#if 0
  itk_panel_start (itk, "prop",
                  
       focused_control->x + focused_control->width, focused_control->y, 200, 200);
                  
       //           ctx_width(ctx)/4, itk->font_size*1, ctx_width(ctx)/4, itk->font_size*8);
  int keys = metadata_item_key_count (files->items[focused_no]);
  itk_labelf (itk, "%s - %i", files->items[focused_no], keys);
  for (int k = 0; k < keys; k++)
  {
    char *key = metadata_key_name (files->items[focused_no], k);
    if (key)
    {
      char *val = metadata_key_string (files->items[focused_no], key);
      if (val)
      {
        itk_labelf (itk, "%s=%s", key, val);
        free (val);
      }
      free (key);
    }
  }
  itk_panel_end (itk);
#endif

    ctx_fill (itk->ctx);
  }

  itk_panel_end (itk);


#if 0
  {
    itk_panel_start (itk, "layout_config", 0,ctx_height(ctx)*0.8, ctx_width(ctx)/2, ctx_height (ctx) * 0.2);
    itk_toggle (itk, "stack horizontal", &layout_config.stack_horizontal);
    itk_sameline (itk);
    itk_toggle (itk, "stack vertical", &layout_config.stack_vertical);
    itk_toggle (itk, "fill width", &layout_config.fill_width);
    itk_sameline (itk);
    itk_toggle (itk, "fill height", &layout_config.fill_height);
    itk_toggle (itk, "fixed size", &layout_config.fixed_size);
    itk_sameline (itk);
    itk_toggle (itk, "fixed pos", &layout_config.fixed_pos);
    itk_sameline (itk);
    itk_toggle (itk, "label", &layout_config.label);

    itk_panel_end (itk);
  }
#endif

  if (active)
  {
    ctx_listen (ctx, CTX_KEY_PRESS, dir_key_any, NULL, NULL);
    ctx_listen (ctx, CTX_KEY_DOWN,  dir_key_any, NULL, NULL);
    ctx_listen (ctx, CTX_KEY_UP,    dir_key_any, NULL, NULL);
  }
  else
  {
    if (viewer_load_next_handler!=0)
      ctx_remove_idle (ctx, viewer_load_next_handler);
    viewer_load_next_handler=0;
  }

  if (tool_no == 0 && !active)
  {
    float em = itk->font_size;
    ctx_save (ctx);
    ctx_rectangle (ctx, 3 * em, 0, ctx_width (ctx) - 3 * em, 3 * em);
    ctx_rgba (ctx, 1,1,1, 0.1);
    ctx_fill (ctx);

    if (editing_location)
    {
      char *copy = strdup (commandline->str);
      char tmp;
      float sel_start = 0.0;
      float sel_end = 0.0;
      int c_start = commandline_cursor_start;
      int c_end   = commandline_cursor_end;

      if (c_end < c_start)
      {
        c_end   = commandline_cursor_start;
        c_start = commandline_cursor_end;
      }

      tmp = copy[c_start];
      copy[c_start] = 0;
      sel_start = ctx_text_width (ctx, copy) - 1;
      copy[c_start] = tmp;

      tmp = copy[c_end];
      copy[c_end] = 0;
      sel_end = ctx_text_width (ctx, copy) + 1;
      copy[c_end] = tmp;

      free (copy);

      ctx_rectangle (ctx, 3.4 * em + sel_start, 1.5 * em - em,
                          sel_end-sel_start,em);
      if (c_start==c_end)
        ctx_rgba (ctx, 1,1, 0.2, 1);
      else
        ctx_rgba (ctx, 0.5,0, 0, 1);
      ctx_fill (ctx);
      ctx_move_to (ctx, 3.4 * em, 1.5 * em);
      ctx_rgba (ctx, 1,1,1, 0.6);
      ctx_text (ctx, commandline->str);
    }
    else
    {
      ctx_rgba (ctx, 1,1,1, 0.6);
      ctx_move_to (ctx, 3.4 * em, 1.5 * em);
      if (files->title)
        ctx_text (ctx, files->title);
      else
        ctx_text (ctx, files->path);
    }
    ctx_restore (ctx);
  }

  if (!editing_location && text_edit == TEXT_EDIT_OFF && commandline->str[0]){
    float em = itk->font_size;
    ctx_save (ctx);

    char *copy = strdup (commandline->str);
      float sel_start = 0.0;
      float sel_end = 0.0;
      int c_start = commandline_cursor_start;
      int c_end   = commandline_cursor_end;
      char tmp;

      if (c_end < c_start)
      {
        c_end   = commandline_cursor_start;
        c_start = commandline_cursor_end;
      }

      tmp = copy[c_start];
      copy[c_start] = 0;
      sel_start = ctx_text_width (ctx, copy) - 1;
      copy[c_start] = tmp;

      tmp = copy[c_end];
      copy[c_end] = 0;
      sel_end = ctx_text_width (ctx, copy) + 1;
      copy[c_end] = tmp;

      free (copy);

      ctx_rectangle (ctx, 3.4 * em + sel_start, ctx_height (ctx) - 1.4 * em,
                          sel_end-sel_start,em * 1.3);
      if (c_start==c_end)
        ctx_rgba (ctx, 1,1, 0.2, 1);
      else
        ctx_rgba (ctx, 0.5,0, 0, 1);
      ctx_fill (ctx);
    ctx_move_to (ctx, 3.4 * em, ctx_height (ctx) - 0.5 * em);
    ctx_rgba (ctx, 1,1,1, 0.6);

    ctx_text (ctx, commandline->str);
    ctx_restore (ctx);
  }

  // toolbar
  {
    float em = itk->font_size;
    ctx_rectangle (ctx, 0, 0, 3 * em, ctx_height (ctx));
    ctx_rgba (ctx, 1,1,1, 0.1);
    ctx_fill (ctx);

    for (int i = 0; i < 5; i ++)
    {
      ctx_rectangle (ctx, 0.5 * em, (3 * i + 0.5) * em,  2 * em, 2 * em);
      ctx_listen (ctx, CTX_CLICK, set_tool_no, (void*)((size_t)i), NULL);
      if (i == tool_no)
        ctx_rgba (ctx, 1,1,1, 0.3);
      else
        ctx_rgba (ctx, 1,1,1, 0.05);
      ctx_fill (ctx);
    }
  }

  // pages
  if (tool_no == 0)
  {
    float em = itk->font_size * 1.4;
/*
    ctx_rectangle (ctx, ctx_width (ctx) - 3 * em, 0, 3 * em, ctx_height (ctx));
    ctx_rgba (ctx, 1,1,1, 0.1);
    ctx_fill (ctx);
    */

    for (int i = 0; i < layout_last_page + 1; i ++)
    {
      ctx_rectangle (ctx, ctx_width (ctx) - 3 * em + 0.5 * em, (3 * (1+i) + 0.5) * em,  2 * em, 2 * em);
      ctx_listen (ctx, CTX_CLICK, dir_set_page, (void*)((size_t)i), NULL);
      if (i == layout_show_page)
        ctx_rgba (ctx, 1,1,1, 0.3);
      else
        ctx_rgba (ctx, 1,1,1, 0.05);
      ctx_fill (ctx);
    }

    ctx_rgba (ctx, 1,1,1, 0.025);
    ctx_rectangle (ctx, ctx_width (ctx) - 3 * em + 0.5 * em, (3 * (layout_last_page+1+1) + 0.5) * em,  2 * em, 2 * em);
      ctx_fill (ctx);
  }


  if (clients && active)
  {
    ctx_font_size (ctx, itk->font_size);
    ctx_clients_draw (ctx);
    ctx_clients_handle_events (ctx);
  }

  if (show_keybindings && !active)
  {
    float bindings_height = ctx_height (ctx) * 0.3;
    float bindings_pos = ctx_height (ctx) - bindings_height;
    float em = itk->font_size;
    ctx_save (ctx);
    ctx_font_size (ctx, em);
    ctx_line_width (ctx, em*0.05);
    ctx_rgba (ctx, 0,0,0,0.6);
    ctx_rectangle (ctx, 0, bindings_pos, ctx_width (ctx), bindings_height);
    ctx_fill (ctx);
    float x = em;
    float y = bindings_pos + 1.4 * em;
    ctx_rgba (ctx, 1,1,1,0.5);
    ctx_move_to (ctx, x, y);
    CtxBinding *binding = ctx_get_bindings (ctx);

    float max_x = x;

    for (int i = 0; binding[i].nick; i++)
    {
      int found = 0;
      for (int j = i + 1; binding[j].nick; j++)
        if (!strcmp (binding[j].nick, binding[i].nick))
          found = 1;
      if (found)
        continue;
      if (binding[i].label)
      {

        float tx = x;
        ctx_rgba (ctx, 1,1,0,0.9);
        {
          char *n = &binding[i].nick[0];

          if (!strncmp (n, "alt-", 4))
          {
            n+= 4;
            const char *str = "Alt";
            float w = ctx_text_width (ctx, str);
            ctx_move_to (ctx, tx, y);
            ctx_text (ctx, str);
            ctx_rectangle (ctx, tx- 0.1 * em , y - 0.9 * em, w + 0.3*em, 1.1*em);
            ctx_stroke (ctx);
            tx += w + 0.7 * em;
          }

          if (!strncmp (n, "shift-", 6))
          {
            n+= 6;
            const char *str = "Shift";
            float w = ctx_text_width (ctx, str);
            ctx_move_to (ctx, tx, y);
            ctx_text (ctx, str);
            ctx_rectangle (ctx, tx- 0.1 * em , y - 0.9 * em, w + 0.3*em, 1.1*em);
            ctx_stroke (ctx);
            tx += w + 0.7 * em;

          }

          if (!strncmp (n, "control-", 8))
          {
            n+= 8;
            const char *str = "Ctrl";
            float w = ctx_text_width (ctx, str);
            ctx_move_to (ctx, tx, y);
            ctx_text (ctx, str);
            ctx_rectangle (ctx, tx- 0.1 * em , y - 0.9 * em, w + 0.3*em, 1.1*em);
            ctx_stroke (ctx);
            tx += w + 0.7 * em;
          }

          if (!strcmp (n, "up")) { n = "↑"; }
          else if (!strcmp (n, "down"))    { n = "↓"; }
          else if (!strcmp (n, "left"))    { n = "←"; }
          else if (!strcmp (n, "right"))   { n = "→"; }
          else if (!strcmp (n, "tab"))     { n = "Tab"; }
          else if (!strcmp (n, "return"))  { n = "Enter"; }
          //else if (!strcmp (n, "return"))  { n = "⏎"; }
          else if (!strcmp (n, "backspace")) { n = "⌫"; }
          //else if (!strcmp (n, "backspace")) { n = "Backspace"; }
          else if (!strcmp (n, "page-down")) { n = "PgDn"; }
          else if (!strcmp (n, "page-up")) { n = "PgUpn"; }
          else if (!strcmp (n, "delete"))  { n = "Del"; }
          else if (!strcmp (n, "insert"))  { n = "Ins"; }
          else if (!strcmp (n, "home"))    { n = "Home"; }
          else if (!strcmp (n, "end"))     { n = "End"; }
          else if (!strcmp (n, "space"))   { n = "Space"; }
          else if (!strcmp (n, "escape"))  { n = "Esc"; }

          {
            char *str = strdup (n);

            if (str[1]==0)
            {
               if (str[0]>='a' && str[0]<='z')
                 str[0] += ('A'-'a');

            }
            float w = ctx_text_width (ctx, str);

            ctx_move_to (ctx, tx, y);
            ctx_text (ctx, str);
            ctx_rectangle (ctx, tx- 0.1 * em , y - 0.9 * em, w + 0.3*em, 1.1*em);
            ctx_stroke (ctx);
            tx += w + 0.7 * em;
            free (str);
          }


        }
        ctx_rgba (ctx, 1,1,1,0.9);
        ctx_move_to (ctx, tx, y);
        ctx_text (ctx, binding[i].label);

        tx += ctx_text_width (ctx, binding[i].label);
        if (tx > max_x) max_x = tx;

        y += 1.4 * em;

        if (y > ctx_height (ctx) - em)
        {
           x = max_x + 1.0 * em;
           y = bindings_pos + 1.4 * em;
        }
      }
    }
    ctx_restore (ctx);
  }

  return 0;
}

void ctx_clients_signal_child (int signum);

int ctx_dir_main (int argc, char **argv)
{
  char *path = argv[1];
  if (path && strchr (path, ':'))
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }
  commandline = ctx_string_new ("");

  signal (SIGCHLD, ctx_clients_signal_child);

  set_layout (NULL, NULL, NULL);
  set_location (path);
  itk_main (card_files, NULL);

  ctx_string_free (commandline, 1);

  while (clients)
    ctx_client_remove (ctx, clients->data);
  return 0;
}
