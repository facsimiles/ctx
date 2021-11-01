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
  float margin; // .. collapsed for inner

  int   fixed_size;
  int   fixed_pos;
  int   list_data;

  float padding_left;
  float padding_right;
  float padding_top;
  float padding_bottom;
  float level_indent;
  int use_layout_boxes;
} LayoutConfig;

LayoutConfig layout_config = {
  1,1,0,0, 1,0,
  4, 4, 0, 0.5,
  0, 0, 0, 3,
  1
};

int focused_no = -1;
int layout_page_no = 0;
int layout_show_page = 0;
int layout_last_page = 0;

int tool_no = -1;

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
  layout_config.level_indent = 2.5;
};


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
}

typedef enum CtxAtom {
 CTX_ATOM_TEXT = 0,
 CTX_ATOM_LAYOUTBOX,
 CTX_ATOM_NEWPAGE,
 CTX_ATOM_STARTGROUP,
 CTX_ATOM_ENDGROUP,
 CTX_ATOM_RECTANGLE,
 CTX_ATOM_CTX,
 CTX_ATOM_FILE
} CtxAtom;

CtxAtom item_get_type_atom (int i)
{
char *type = metadata_key_string2 (i, "type");
if (type)
{
   if (!strcmp (type, "ctx/layoutbox") && layout_config.use_layout_boxes)
     return CTX_ATOM_LAYOUTBOX;
   else if (!strcmp (type, "ctx/newpage"))    return CTX_ATOM_NEWPAGE;
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

void dm_set_path (Files *files, const char *path)
{
  char *resolved_path = realpath (path, NULL);

  if (files->path)
    free (files->path);
  files->path = resolved_path;

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

  for (int i = 0; i < files->n; i++)
  {
    int found = 0;
    const char *name = files->namelist[i]->d_name;
    for (int j = 0; j < files->count; j++)
    {
      if (!strcmp (name, files->items[j]))
      { found = 1;
      }
    }
    if (!found && (name[0] != '.' || (name[0] == '.' && name[1]=='.')))
    {
      int n = metadata_insert(-1, name);
      metadata_set2 (n, "type", "ctx/file");
      files->items[files->count++] = strdup (name);
    }
  }
}

Files file_state;
Files *files = &file_state;

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

static int layout_find_item = -1;
ITK *itk = NULL;
static void dir_go_parent (CtxEvent *e, void *d1, void *d2)
{
  char *old_path = strdup (files->path);
  char *new_path = get_dirname (files->path);
  const char *media_type = ctx_path_get_media_type (new_path); 
  dm_set_path (files, new_path);

  layout_find_item = metadata_item_to_no (strrchr (old_path, '/')+1);
  fprintf (stderr, "%i, %s\n", layout_find_item, strrchr (old_path, '/')+1);


  free (old_path);

  itk->focus_no = -1;
  free (new_path);
  if (e)
  {
    ctx_set_dirty (e->ctx, 1);
    e->stop_propagate = 1;
  }
}

static void item_activate (CtxEvent *e, void *d1, void *d2)
{
  //CtxEvent *e = &event; // we make a copy to permit recursion
  int no = (size_t)(d1);
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
    dm_set_path (files, new_path);
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
  fprintf (stderr, "drag %i\n", e->type);
  switch (e->type)
  {
    case CTX_DRAG_PRESS:
    case CTX_DRAG_MOTION:
    //case CTX_DRAG_RELEASE:
      e->stop_propagate = 1;
      break;
    default:
      break;
  }
}

static void item_tap_and_hold (CtxEvent *e, void *d1, void *d2)
{
  fprintf (stderr, "tap and hold %i\n", e->type);
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

static int metadata_dirty = 0;
static void metadata_dirt(void)
{
  metadata_dirty++;
  metadata_cache_no=-3;
}


static void item_delete (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);

  //int virtual = (item_get_type_atom (no) == CTX_ATOM_TEXT);
  //if (virtual)
  {
    CtxAtom pre_atom = item_get_type_atom (no-1);
    CtxAtom post_atom = item_get_type_atom (no+1);

    if (pre_atom == CTX_ATOM_STARTGROUP &&
        post_atom == CTX_ATOM_ENDGROUP)
    {
      metadata_remove (no-1);
      metadata_remove (no-1);
      layout_find_item = no-2;
    }
    else
    {
      metadata_remove (no);
      layout_find_item = no;
    }
    text_edit = TEXT_EDIT_OFF;
    itk->focus_no = -1;

    metadata_dirt();
    ctx_set_dirty (e->ctx, 1);
    e->stop_propagate = 1;
    return;
  }

  if (e)
  {
    ctx_set_dirty (e->ctx, 1);
    e->stop_propagate = 1;
  }
}


void itk_focus (ITK *itk, int dir);

#if 0
static void move_item_down (CtxEvent *e, void *d1, void *d2)
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

static void
move_item_down (CtxEvent *event, void *a, void *b)
{
  int skip = 0;

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
      metadata_insert (focused_no, "fnord");
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

static void grow_height (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float height = metadata_key_float2 (no, "height");
  height += 0.01;
  if (height > 1.2) height = 1.2;
  metadata_set_float2 (no, "height", height);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void shrink_height (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float height = metadata_key_float2 (no, "height");
  height -= 0.01;
  if (height < 0.01) height = 0.01;
  metadata_set_float2 (no, "height", height);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void grow_width (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float width = metadata_key_float2 (no, "width");
  width += 0.01;
  if (width > 1.5) width = 1.5;
  metadata_set_float2 (no, "width", width);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void shrink_width (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float width = metadata_key_float2 (no, "width");
  width -= 0.01;
  if (width < 0.0) width = 0.01;
  metadata_set_float2 (no, "width", width);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_left (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float x = metadata_key_float2 (no, "x");
  x -= 0.01;
  if (x < 0.0) x = 0.0;
  metadata_set_float2 (no, "x", x);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_right (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float x = metadata_key_float2 (no, "x");
  x += 0.01;
  if (x < 0) x = 0;
  metadata_set_float2 (no, "x", x);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_up (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float y = metadata_key_float2 (no, "y");
  y -= 0.01;
  if (y< 0) y = 0.01;
  metadata_set_float2 (no, "y", y);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

static void move_down (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  float y = metadata_key_float2 (no, "y");
  y += 0.01;
  if (y< 0) y = 0.01;
  metadata_set_float2 (no, "y", y);
  metadata_dirt ();
  ctx_set_dirty (e->ctx, 1);
}

#if 0
static void move_item_up (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  if (no>0)
  {
    metadata_swap (no, no-1);
    metadata_dirt ();
    ctx_set_dirty (e->ctx, 1);
    itk_focus (itk, -1);
  }
}
#endif



static void
move_item_up (CtxEvent *event, void *a, void *b)
{
  int start_no = focused_no;
  
  int level = 0;
  int start_level = 0;
  int did_skips = 0;

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
      metadata_insert (focused_no-1 + i, "");
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


static void move_item_left (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
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
      metadata_insert (target+1+i, "a");
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

static void move_item_right (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
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
        metadata_insert (target+i, "a");
        metadata_swap (no+1+i, target+i);
        metadata_remove (no+1+i);
      }
    }
    else
    {
      // insert new group
      target = no;
      metadata_insert (target, "a");
      metadata_set2 (target, "type", "ctx/endgroup");
      for (int i = 0; i <count;i++)
      metadata_insert (target, "b");

      metadata_insert (target, "c");
      metadata_set2 (target, "type", "ctx/startgroup");

      for (int i = 0; i < count; i++)
      {
        metadata_swap (no+count+2, target+1+i);
        metadata_remove (no+count+2);
      }
    }

    layout_find_item = target;
    itk->focus_no = -1;

    metadata_dirt ();
    ctx_set_dirty (e->ctx, 1);
  }
}

void item_toggle_todo (CtxEvent *event, void *a, void *b)
{
  int todo = metadata_key_int2 (focused_no, "todo");
  switch (todo)
  {
    default:
      metadata_set_float2 (focused_no, "todo", 0);
      break;
    case 0:
      metadata_set_float2 (focused_no, "todo", 1);
      break;
    case 1:
      metadata_unset2 (focused_no, "todo");
      break;
  }
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

  if ((target_width > 250 || target_height > 250))
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
                         int *prev_line, int *next_line)
{
  char word[1024]="";
  int wlen = 0;
  const char *p;
  int pos = 0;
  float x0 = x;

  int cursor_drawn = 0;
  int compute_neighbor_lines = 0;
  if (prev_line || next_line) compute_neighbor_lines = 1;

  y += line_height;
  if (sel_start < 0)
    cursor_drawn = 1;


  int pot_cursor=-10;
  int best_end_cursor=-10;

  for (p = d_name; p == d_name || p[-1]; p++)
  {
    if (*p == ' ' || *p == '\0' || *p == '\n')
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
        ctx_move_to (itk->ctx, x, y);
        ctx_text (itk->ctx, word);
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
    else
    {
      if (wlen < 1000)
        word[wlen++]=*p;
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
static void save_metadata(void)
{  if (metadata_dirty)
   {
     metadata_save ();
     dm_set_path (files, files->path);
     metadata_dirty = 0;
   }

}


void text_edit_return (CtxEvent *event, void *a, void *b)
{
  char *str = strdup (files->items[focused_no]);
  metadata_insert (focused_no+1, files->items[focused_no]);
  metadata_dirt ();

  metadata_rename (focused_no+1, str + text_edit);
  str[text_edit]=0;
  metadata_rename (focused_no, str);

  free (str);

  ctx_set_dirty (event->ctx, 1);
  layout_find_item = focused_no + 1;
  itk->focus_no = -1;
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

void text_edit_any (CtxEvent *event, void *a, void *b)
{
  const char *inserted = event->string;
  if (!strcmp (inserted, "space")) inserted = " ";
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
      item_delete (event, (void*)focused_no, NULL);

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

  if (item_get_type_atom (focused_no-1) != CTX_ATOM_TEXT)
  {
    event->stop_propagate=1;
    return;
  }
  //text_edit=strlen(files->items[focused_no-1]);
  text_edit = TEXT_EDIT_FIND_CURSOR_LAST_ROW;
  layout_find_item = focused_no - 1;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}


float dir_scale = 1.0f;

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
  itk->font_size *= 1.1f;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_font_down (CtxEvent *event, void *a, void *b)
{
  itk->font_size /= 1.1f;
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
   dir_set_page (event, (void*)(layout_show_page-1), NULL);
}
void dir_next_page (CtxEvent *event, void *a, void *b)
{
   dir_set_page (event, (void*)(layout_show_page+1), NULL);
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

int item_context_active = 0;

int item_outliner = 0; // with 0 only virtual items get outliner event handling


static void
item_outliner_down (CtxEvent *event, void *a, void *b)
{
  int start_no = focused_no;
  
  int start_level = 0;
  int level = 0;

  int atom = item_get_type_atom (focused_no+1);
  if (atom == CTX_ATOM_ENDGROUP)
  {
    event->stop_propagate=1;
    ctx_set_dirty (event->ctx, 1);
    return;
  }
  if (atom == CTX_ATOM_STARTGROUP)
     level++;
  focused_no++;

  while (level > start_level && focused_no < files->count)
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
  }
  level++;
  while (item_get_type_atom (focused_no) == CTX_ATOM_ENDGROUP)
  {
    level--;
    focused_no++;
  }

  if (level < start_level)
  {
     focused_no = start_no;
  }

  layout_find_item = focused_no;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
item_outliner_up (CtxEvent *event, void *a, void *b)
{
  int start_no = focused_no;
  
  int level = 0;
  int start_level = 0;

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
    }
  }
  while (atom == CTX_ATOM_STARTGROUP || atom == CTX_ATOM_LAYOUTBOX)
  {
    focused_no--;
    atom = item_get_type_atom (focused_no);
  }
  if (level < start_level || focused_no <= 0)
  {
     focused_no = start_no;
  }

  layout_find_item = focused_no;
  itk->focus_no = -1;

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}


static void
set_tool_no (CtxEvent *event, void *a, void *b)
{
  tool_no = (size_t)(a);
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}


static void
item_outliner_left (CtxEvent *event, void *a, void *b)
{
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
item_outliner_right (CtxEvent *event, void *a, void *b)
{
  int start_no = focused_no;
  
  focused_no++;
  CtxAtom  atom = item_get_type_atom (focused_no);

  if (atom == CTX_ATOM_STARTGROUP)
  {
    focused_no++;
  }
  else
  {
     focused_no = start_no;

     metadata_insert(focused_no+1, "");
     metadata_set2(focused_no+1, "type", "ctx/startgroup");

     metadata_insert(focused_no+2, "");
     text_edit = 0;

     metadata_insert(focused_no+3, "");
     metadata_set2(focused_no+3, "type", "ctx/endgroup");
     metadata_dirt();
     focused_no++;
  }

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

  layout_box_count = 0;
  layout_box_no = 0;
  layout_box[0].x = 0.05;
  layout_box[0].y = 0.02;
  layout_box[0].width = 0.9;
  layout_box[0].height = 4000.0;
#if 0
  float cbox_x = metadata_key_float (".contentBox0", "x");
  float cbox_y = metadata_key_float (".contentBox0", "y");
  float cbox_width = metadata_key_float (".contentBox0", "width");
  float cbox_height = metadata_key_float (".contentBox0", "height");
#endif

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

  if (y1 < 100) y1 = itk->height;

  int printing = (layout_page_no == layout_show_page);

  for (int i = 0; i < files->count; i++)
  {
    if ((files->items[i][0] == '.' && files->items[i][1] == '.') ||
        (files->items[i][0] != '.') // skipping dot files
       )
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
          hidden = 1;
          level ++;
          break;
        case CTX_ATOM_ENDGROUP:
          hidden = 1;
          level --;
          break;
        case CTX_ATOM_NEWPAGE:
          hidden = 1;
          break;
      }

      int label = metadata_key_int2 (i, "label");
      if (label == -1234) {
        if (atom == CTX_ATOM_TEXT)
          label = 0;
        else
          label = layout_config.label;
      }


      int gotpos = 0;
      char *xstr = metadata_key_string2 (i, "x");
      char *ystr = metadata_key_string2 (i, "y");

      float padding_left = metadata_key_float2 (i, "padding-left");
      if (padding_left == -1234.0f) padding_left = layout_config.padding_left;
      float padding_right = metadata_key_float2 (i, "padding-right");
      if (padding_right == -1234.0f) padding_right = layout_config.padding_right;

      padding_left += level * layout_config.level_indent;

      float x = metadata_key_float2 (i, "x");
      float y = metadata_key_float2 (i, "y");
      float opacity = metadata_key_float2 (i, "opacity");
      if (opacity == -1234.0f) opacity = 1.0f;

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
        width  = metadata_key_float2 (i, "width");
        if (width < 0 || layout_config.fixed_size)
          width = 
            layout_config.fill_width? itk->width * 1.0:
            layout_config.width * em;
        else {
          width *= saved_width;
        }
      }

      {
        height = metadata_key_float2 (i, "height");
        if (height < 0 || layout_config.fixed_size)  height =
          layout_config.fill_height? itk->height * 1.0:
          layout_config.height * em;
        else
          height *= saved_width;
      }

      int virtual = (item_get_type_atom (i) == CTX_ATOM_TEXT);

      if (virtual)
        atom = CTX_ATOM_TEXT;

      if (layout_config.stack_horizontal && layout_config.stack_vertical)
      {
      if (itk->x + width  > itk->x0 + itk->width || atom == CTX_ATOM_NEWPAGE) //panel->x + itk->panel->width)
      {
          itk->x = itk->x0;
          if (layout_config.stack_vertical)
          {
            itk->y += row_max_height;
            row_max_height = 0;
          }
          if (itk->y + height > y1 || atom == CTX_ATOM_NEWPAGE)
          {
            if (layout_box_count > layout_box_no+1 && atom != CTX_ATOM_NEWPAGE)
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

      if (!hidden)
      {
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

          itk->x = itk->x0;
          /* measure height, and snap cursor */
          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       space_width, width, em,
                       i == focused_no ? text_edit : -1,
                       i == focused_no ? text_edit + 2: -1,
                       0, NULL, &height,
                       NULL, NULL);
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
        itk->x, itk->y,
        width + em * (padding_left+padding_right),
        height);
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

        if (!strcmp (media_type, "inode/directory") && !layout_config.list_data)
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

        if (c->no == itk->focus_no && layout_find_item < 0)
        {
          focused = 1;
          //fprintf (stderr, "\n{%i %i %i}\n", c->no, itk->focus_no, i);
          //viewer_load_path (newpath, files->items[i]);
          ctx_begin_path (itk->ctx);
          ctx_rectangle (itk->ctx, c->x, c->y, c->width, c->height);
          ctx_listen (itk->ctx, CTX_DRAG, item_drag, (void*)(size_t)i, NULL);
          ctx_listen (itk->ctx, CTX_TAP_AND_HOLD, item_activate, (void*)(size_t)i, NULL);
          //ctx_listen (itk->ctx, CTX_TAP_AND_HOLD, item_tap_and_hold, (void*)(size_t)i, NULL);


          ctx_begin_path (itk->ctx);
          //ctx_rgb(itk->ctx,1,0,0);
          //ctx_fill(itk->ctx);

          if (!active)
          {
          if (text_edit < 0)
          {
            ctx_add_key_binding (ctx, "alt-return", NULL, NULL,
                          item_properties,
                          (void*)((size_t)i));

            ctx_add_key_binding (ctx, "alt-up", NULL, NULL,
                          dir_go_parent,
                          (void*)((size_t)i));


            ctx_add_key_binding (ctx, "alt-down", NULL, NULL,
                          item_activate,
                          (void*)((size_t)i));

            ctx_add_key_binding (ctx, "return", NULL, NULL,
                          item_activate,
                          (void*)((size_t)i));
            ctx_add_key_binding (ctx, "delete", NULL, NULL,
                          item_delete,
                          (void*)((size_t)i));
            ctx_add_key_binding (ctx, "control-x", NULL, NULL,
                          item_delete,
                          (void*)((size_t)i));

            ctx_add_key_binding (ctx, "control-t", NULL, NULL,
                          item_toggle_todo,
                          (void*)((size_t)i));
          }
          ctx_add_key_binding (ctx, "control-page-down", NULL, NULL, move_item_down, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-page-up", NULL, NULL, move_item_up, 
                         (void*)((size_t)i));

          ctx_add_key_binding (ctx, "shift-control-down", NULL, NULL, grow_height, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "shift-control-up", NULL, NULL, shrink_height, 
                         (void*)((size_t)i));

          ctx_add_key_binding (ctx, "shift-control-left", NULL, NULL, shrink_width, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "shift-control-right", NULL, NULL, grow_width, 
                         (void*)((size_t)i));

          if (gotpos)
          {
          ctx_add_key_binding (ctx, "control-left", NULL, NULL, move_left, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-right", NULL, NULL, move_right, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-up", NULL, NULL, move_up, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-down", NULL, NULL, move_down, 
                         (void*)((size_t)i));
          }
          else
          {
            ctx_add_key_binding (ctx, "control-down", NULL, NULL, move_item_down, 
                           (void*)((size_t)i));
            ctx_add_key_binding (ctx, "control-up", NULL, NULL, move_item_up, 
                           (void*)((size_t)i));

            ctx_add_key_binding (ctx, "control-left", NULL, NULL, move_item_left, 
                           (void*)((size_t)i));

            ctx_add_key_binding (ctx, "control-right", NULL, NULL, move_item_right, 
                           (void*)((size_t)i));

          }

          }
          //itk_labelf (itk, "%s\n", ctx_path_get_media_type (newpath));
        }
        else
        {
          ctx_listen (itk->ctx, CTX_PRESS, dir_select_item, (void*)(size_t)c->no, NULL);
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
                       space_width, width, em,
                       text_edit,text_edit,
                       1, NULL, NULL,
                       &prev_line_pos, &next_line_pos);
          }
          else
          {

          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       space_width, width, em,
                       -1, -1,
                       1, NULL, NULL,
                       NULL, NULL);
          }

          int todo = metadata_key_int2 (i, "todo");
          if (todo >= 0)
          {
             float x = itk->x - em * 0.5 + level * em * layout_config.level_indent;
             if (todo)
             {
               ctx_move_to (itk->ctx, x, itk->y + em);
               ctx_text (itk->ctx, "X");
             }
             else
             {
               ctx_move_to (itk->ctx, x, itk->y + em);
               ctx_text (itk->ctx, "O");
             }
          }
          //if (c->no == itk->focus_no)
          //fprintf (stderr, "%f %i %i %i\n", text_edit_desired_x, text_edit, prev_line, next_line);

          ctx_restore (itk->ctx);
        }
      else
        if (atom == CTX_ATOM_RECTANGLE)
        {
          char *fill = metadata_key_string2(i, "fill");
          char *stroke = metadata_key_string2(i, "stroke");
          float line_width = metadata_key_float2(i, "line-width");
          float opacity = metadata_key_float2(i, "opacity");

          if (opacity < 0) opacity = 1.0;
          if (line_width < 0) line_width = 1.0;

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
          draw_img (itk, itk->x, itk->y, width, height, newpath);
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
        int wraplen = 12;
        int lines = (title_len + wraplen - 1)/ wraplen;

        if (!focused &&  lines > 2) lines = 2;

        for (int i = 0; i < lines; i++)
        {
          ctx_move_to (itk->ctx, itk->x + em * 3, itk->y + height + em * i - lines/2.0 * em);
          ctx_gray (itk->ctx, 0.8);
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
        }
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
  fprintf (stderr, "next!\n");
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

  float duration = 10.0;
  float in = metadata_key_float (name, "in");
  float out = metadata_key_float (name, "out");
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

  itk_panel_start (itk, "", 0,0, ctx_width(ctx),
                  ctx_height (ctx));

  if (dir_scale != 1.0f)
     ctx_scale (itk->ctx, dir_scale, dir_scale);

  if (!files->n)
  {
    itk_labelf (itk, "no items\n");
  }
  else
  {
    ctx_add_key_binding (ctx, "F1", NULL, NULL, set_list, NULL);
    ctx_add_key_binding (ctx, "F2", NULL, NULL, set_grid, NULL);
    ctx_add_key_binding (ctx, "F3", NULL, NULL, set_layout, NULL);

    dir_layout (itk, files);


  }


  if (!active && text_edit <= TEXT_EDIT_OFF)
  {

#if 0
          ctx_add_key_binding (ctx, "+", NULL, NULL,
                          dir_font_up,
                          NULL);
          ctx_add_key_binding (ctx, "=", NULL, NULL,
                          dir_font_up,
                          NULL);
          ctx_add_key_binding (ctx, "-", NULL, NULL,
                          dir_font_down,
                          NULL);
#else
          ctx_add_key_binding (ctx, "control-+", NULL, NULL,
                          dir_zoom_in,
                          NULL);
          ctx_add_key_binding (ctx, "control-=", NULL, NULL,
                          dir_zoom_in,
                          NULL);
          ctx_add_key_binding (ctx, "control--", NULL, NULL,
                          dir_zoom_out,
                          NULL);
          ctx_add_key_binding (ctx, "control-0", NULL, NULL,
                          dir_zoom_reset,
                          NULL);
#endif

          ctx_add_key_binding (ctx, "page-down", NULL, NULL,
                          dir_next_page,
                          NULL);
          ctx_add_key_binding (ctx, "page-up", NULL, NULL,
                          dir_prev_page,
                          NULL);

          if (item_outliner || item_get_type_atom (focused_no) == CTX_ATOM_TEXT)
          {
          ctx_add_key_binding (ctx, "up", NULL, NULL,
                          item_outliner_up,
                          NULL);
          ctx_add_key_binding (ctx, "down", NULL, NULL,
                          item_outliner_down,
                          NULL);
          ctx_add_key_binding (ctx, "left", NULL, NULL,
                          item_outliner_left,
                          NULL);
          ctx_add_key_binding (ctx, "right", NULL, NULL,
                          item_outliner_right,
                          NULL);
          }
  }


#if 1
      if (!active && text_edit>TEXT_EDIT_OFF)
      {

          ctx_add_key_binding (ctx, "control-left", NULL, NULL,
                          text_edit_control_left,
                          NULL);
          ctx_add_key_binding (ctx, "control-right", NULL, NULL,
                          text_edit_control_right,
                          NULL);
          ctx_add_key_binding (ctx, "left", NULL, NULL,
                          text_edit_left,
                          NULL);
          ctx_add_key_binding (ctx, "up", NULL, NULL,
                          text_edit_up,
                          NULL);

          ctx_add_key_binding (ctx, "down", NULL, NULL,
                          text_edit_down,
                          NULL);
          ctx_add_key_binding (ctx, "right", NULL, NULL,
                          text_edit_right,
                          NULL);
          ctx_add_key_binding (ctx, "escape", NULL, NULL,
                          text_edit_stop,
                          NULL);
          ctx_add_key_binding (ctx, "shift-return", NULL, NULL,
                            text_edit_shift_return,
                            NULL);

          ctx_add_key_binding (ctx, "unhandled", NULL, NULL,
                          text_edit_any,
                          NULL);

          ctx_add_key_binding (ctx, "home", NULL, NULL,
                          text_edit_home,
                          NULL);
          ctx_add_key_binding (ctx, "end", NULL, NULL,
                          text_edit_end,
                          NULL);

          ctx_add_key_binding (ctx, "return", NULL, NULL,
                          text_edit_return,
                          NULL);
          ctx_add_key_binding (ctx, "backspace", NULL, NULL,
                          text_edit_backspace,
                          NULL);
          ctx_add_key_binding (ctx, "delete", NULL, NULL,
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
  {
    float em = itk->font_size * 1.4;
/*
    ctx_rectangle (ctx, ctx_width (ctx) - 3 * em, 0, 3 * em, ctx_height (ctx));
    ctx_rgba (ctx, 1,1,1, 0.1);
    ctx_fill (ctx);
    */

    for (int i = 0; i < layout_last_page + 1; i ++)
    {
      ctx_rectangle (ctx, ctx_width (ctx) - 3 * em + 0.5 * em, (3 * i + 0.5) * em,  2 * em, 2 * em);
      ctx_listen (ctx, CTX_CLICK, dir_set_page, (void*)((size_t)i), NULL);
      if (i == layout_show_page)
        ctx_rgba (ctx, 1,1,1, 0.3);
      else
        ctx_rgba (ctx, 1,1,1, 0.05);
      ctx_fill (ctx);
    }

    ctx_rgba (ctx, 1,1,1, 0.025);
    ctx_rectangle (ctx, ctx_width (ctx) - 3 * em + 0.5 * em, (3 * (layout_last_page+1) + 0.5) * em,  2 * em, 2 * em);
      ctx_fill (ctx);
  }


  if (clients && active)
  {
    ctx_font_size (ctx, itk->font_size);
    ctx_clients_draw (ctx);
    ctx_clients_handle_events (ctx);
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

  signal (SIGCHLD, ctx_clients_signal_child);

  set_layout (NULL, NULL, NULL);
  dm_set_path (files, path?path:"./");
  itk_main (card_files, NULL);
  while (clients)
    ctx_client_remove (ctx, clients->data);
  return 0;
}
