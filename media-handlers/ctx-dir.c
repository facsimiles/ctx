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

  int use_layout_boxes;
} LayoutConfig;

LayoutConfig layout_config = {
  1,1,0,0, 1,0,
  4, 4, 0, 0.5,
  0, 0, 0,
  1
};

int focused_no = -1;
int layout_page_no = 0;
int layout_show_page = 0;
int layout_last_page = 0;

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
  layout_config.label = 0;
  layout_config.list_data = 0;
  layout_config.padding_left = 0.5f;
  layout_config.padding_right = 0.5f;
  layout_config.padding_top = 0.5f;
  layout_config.padding_bottom = 0.5f;
  layout_config.use_layout_boxes = 1;
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
      metadata_insert(-1, name);
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

ITK *itk = NULL;
static void item_activate (CtxEvent *e, void *d1, void *d2)
{
  //CtxEvent *e = &event; // we make a copy to permit recursion
  int no = (size_t)(d1);
  viewer_no = no;
  int virtual = (metadata_key_int2 (no, "virtual") > 0);

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
    new_path = get_dirname (files->path);
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

  int virtual = metadata_key_int2 (no, "virtual");
  if (virtual <0) virtual = 0;

  if (virtual)
  {
    metadata_remove (no);
    text_edit = TEXT_EDIT_OFF;
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

  if (!strcmp (event, "control-escape"))
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
  char *p;
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
    if (*p == ' ' || *p == '\0')
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
           if (pot_cursor > strlen (d_name))
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
          ctx_rectangle (itk->ctx,
                    cursor_x-1, y - line_height, 2, line_height * 1.2);
      if (print)
          ctx_fill (itk->ctx);
          cursor_drawn = 1;
      if (print)
          ctx_restore (ctx);
        }
      if (print)
        ctx_move_to (itk->ctx, x, y);
      if (print)
        ctx_text (itk->ctx, word);
      }

      x += word_width + space_width;
      wlen=0;
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
  metadata_set_float2 (focused_no+1, "virtual", 1);
  metadata_dirt ();

  metadata_rename (focused_no+1, str + text_edit);
  str[text_edit]=0;
  metadata_rename (focused_no, str);

  free (str);

  ctx_set_dirty (event->ctx, 1);
  itk->focus_no++;
  focused_no++;
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
             (metadata_key_int2 (focused_no-1, "virtual")>0)
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
    if (text_edit == strlen(str->str))
    {
      if ((metadata_key_int2 (focused_no+1, "virtual")>0))
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
    if (metadata_key_int2 (focused_no+1, "virtual")>0)
    {
      text_edit=0;
      focused_no++;
      itk->focus_no++;
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
    if (metadata_key_int2 (focused_no-1, "virtual")>0)
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

  if (metadata_key_int2 (focused_no-1, "virtual")<=0)
  {
    event->stop_propagate=1;
    return;
  }
  //text_edit=strlen(files->items[focused_no-1]);
  text_edit = TEXT_EDIT_FIND_CURSOR_LAST_ROW;
  focused_no--;
  itk->focus_no--;

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

void dir_prev_page (CtxEvent *event, void *a, void *b)
{
  layout_show_page --;
  if (layout_show_page < 0) layout_show_page = 0;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

void dir_next_page (CtxEvent *event, void *a, void *b)
{
  layout_show_page ++;
  //if (layout_show_page > layout_last_page) layout_show_page = layout_last_page;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
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

  if (metadata_key_int2 (focused_no+1, "virtual")<=0)
  {
    event->stop_propagate=1;
    return;
  }
  text_edit = TEXT_EDIT_FIND_CURSOR_FIRST_ROW;
  focused_no++;
  itk->focus_no++;
  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
  // -- - //
}


int item_context_active = 0;

int item_outliner = 0; // with 0 only virtual items get outliner event handling

/* */
static void
item_outliner_down (CtxEvent *event, void *a, void *b)
{
  int start_level = metadata_key_int2 (focused_no, "level");
  if (start_level <0)start_level = 0;

  int start_no = focused_no;
  int start_focus = itk->focus_no;
  
  int level;
  focused_no++;
  itk->focus_no++;
  level = metadata_key_int2 (focused_no, "level");
  if (level < 0) level = 0;
  while (level > start_level)
  {
    focused_no++;
    itk->focus_no++;
    level = metadata_key_int2 (focused_no, "level");
    if (level < 0) level = 0;
  }
  if (level < start_level)
  {
     focused_no = start_no;
     itk->focus_no = start_focus;
  }

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
item_outliner_up (CtxEvent *event, void *a, void *b)
{
  int start_level = metadata_key_int2 (focused_no, "level");
  if (start_level <0)start_level = 0;

  int start_no = focused_no;
  int start_focus = itk->focus_no;
  
  int level;
  focused_no--;
  itk->focus_no--;
  level = metadata_key_int2 (focused_no, "level");
  if (level < 0) level = 0;
  while (level > start_level)
  {
    focused_no--;
    itk->focus_no--;
    level = metadata_key_int2 (focused_no, "level");
    if (level < 0) level = 0;
  }
  if (level < start_level)
  {
     focused_no = start_no;
     itk->focus_no = start_focus;
  }

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
item_outliner_left (CtxEvent *event, void *a, void *b)
{
  int start_level = metadata_key_int2 (focused_no, "level");
  if (start_level <0)start_level = 0;

  int start_no = focused_no;
  int start_focus = itk->focus_no;
  
  int level;
  focused_no--;
  itk->focus_no--;
  level = metadata_key_int2 (focused_no, "level");
  if (level < 0) level = 0;
  while (level > start_level - 1 && focused_no >= 0)
  {
    focused_no--;
    itk->focus_no--;
    level = metadata_key_int2 (focused_no, "level");
    if (level < 0) level = 0;
  }
  if (level < start_level - 1 || focused_no < 0)
  {
     focused_no = start_no;
     itk->focus_no = start_focus;
  }

  ctx_set_dirty (event->ctx, 1);
  event->stop_propagate=1;
}

static void
item_outliner_right (CtxEvent *event, void *a, void *b)
{
  int start_level = metadata_key_int2 (focused_no, "level");
  if (start_level <0)start_level = 0;

  int start_no = focused_no;
  int start_focus = itk->focus_no;
  
  int level;
  focused_no++;
  itk->focus_no++;
  level = metadata_key_int2 (focused_no, "level");
  if (level < 0) level = 0;

  if (level == start_level + 1)
  {
          
  }
  else
  {
          focused_no = start_no;
          itk->focus_no = start_focus;
          // create empty place holder node
  }

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

  layout_box_count = 0;
  layout_box_no = 0;
  layout_box[0].x = 0.05;
  layout_box[0].y = 0.02;
  layout_box[0].width = 0.9;
  layout_box[0].height = 0.4;
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

  float y0, y1;
  layout_box_no = 0;
  layout_page_no = 0;

  itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
  itk->y           = layout_box[layout_box_no].y * saved_width;
  itk->width       = layout_box[layout_box_no].width * saved_width ;
  y0 = layout_box[layout_box_no].y * saved_width;
  y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;

  if (y1 < 100) y1 = itk->height;

  int printing = (layout_page_no == layout_show_page);

  for (int i = 0; i < files->count; i++)
  {
    if ((files->items[i][0] == '.' && files->items[i][1] == '.') ||
        (files->items[i][0] != '.') // skipping dot files
       )
    {
      if (itk->control_no == itk->focus_no)
      {
        focused_no = i;
      }

      const char *d_name = files->items[i];
      float width = 0;
      float height = 0;
      
      int hidden = 0;
      int is_contentbox = 0;
      int is_newpage = 0;
      {
      const char *type = metadata_key_string2 (i, "type");
      if (type)
      {
         if (!strcmp (type, "ctx/contentbox") && layout_config.use_layout_boxes)
           is_contentbox = 1;
         if (!strcmp (type, "ctx/newpage"))
         {
           is_newpage = 1;
           hidden = 1;
         }
         free (type);
      }
      }

      int label = metadata_key_int2 (i, "label");
      int level = metadata_key_int2 (i, "level");
      if (level == -1234) level = 0;
      if (label == -1234) label = layout_config.label;


      int gotpos = 0;
      char *xstr = metadata_key_string2 (i, "x");
      char *ystr = metadata_key_string2 (i, "y");

      float padding_left = metadata_key_float2 (i, "padding-left");
      if (padding_left == -1234.0f) padding_left = layout_config.padding_left;
      float padding_right = metadata_key_float2 (i, "padding-right");
      if (padding_right == -1234.0f) padding_right = layout_config.padding_right;

      padding_left += level * 3;

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

      int virtual = metadata_key_int2 (i, "virtual");
      if (virtual < 0) virtual = 0;

      if (layout_config.stack_horizontal && layout_config.stack_vertical)
      {
      if (itk->x + width  > itk->x0 + itk->width || is_newpage) //panel->x + itk->panel->width)
      {
          itk->x = itk->x0;
          if (layout_config.stack_vertical)
          {
            itk->y += row_max_height;
            row_max_height = 0;
          }
          if (itk->y + height > y1 || is_newpage)
          {
            if (layout_box_count > layout_box_no+1 && ! is_newpage)
            {
              layout_box_no++;

             itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
             itk->y           = layout_box[layout_box_no].y * saved_width;
             itk->width       = layout_box[layout_box_no].width * saved_width ;
             y0 = layout_box[layout_box_no].y * saved_width;
             y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;
            }
            else
            {
              layout_box_no = 0;

             itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
             itk->y           = layout_box[layout_box_no].y * saved_width;
             itk->width       = layout_box[layout_box_no].width * saved_width ;
             y0 = layout_box[layout_box_no].y * saved_width;
             y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;

             layout_page_no++;
             printing = (layout_page_no == layout_show_page);
             layout_last_page = layout_page_no;
            }
          }
      }
      }


      if (is_contentbox)
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
             y0 = layout_box[layout_box_no].y * saved_width;
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
          ctx_font_size (itk->ctx, itk->font_size);
           
          if (layout_config.stack_vertical && itk->x != itk->x0)
            itk->y += row_max_height;

          itk->x = itk->x0;
          /* measure height, and snap cursor */
          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       em * 0.25, width, em,
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


        if (c->no == itk->focus_no)
        {
          focused = 1;
          //fprintf (stderr, "\n{%i %i %i}\n", c->no, itk->focus_no, i);
          //viewer_load_path (newpath, files->items[i]);
          ctx_begin_path (itk->ctx);
          ctx_rectangle (itk->ctx, c->x, c->y, c->width, c->height);
          ctx_listen (itk->ctx, CTX_CLICK, item_activate, (void*)(size_t)i, NULL);
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

        if (virtual)
        {
          ctx_save (itk->ctx);
          ctx_font_size (itk->ctx, em);

          //if (c->no == itk->focus_no)
          //  ctx_rgb (itk->ctx, 1, 0, text_edit>=0?0:1);
          //else
          //  ctx_rgb (itk->ctx, 1, 0, 1);
          ctx_gray (itk->ctx, 0.95);


          if (c->no == itk->focus_no)
          {
          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       em * 0.25, width, em,
                       text_edit,text_edit,
                       1, NULL, NULL,
                       &prev_line_pos, &next_line_pos);
          }
          else
          {

          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       em * 0.25, width, em,
                       -1, -1,
                       1, NULL, NULL,
                       NULL, NULL);
          }

          int todo = metadata_key_int2 (i, "todo");
          if (todo >= 0)
          {
             if (todo)
             {
               ctx_move_to (itk->ctx, itk->x - em * 0.5, itk->y + em);
               ctx_text (itk->ctx, "X");
             }
             else
             {
               ctx_move_to (itk->ctx, itk->x - em * 0.5, itk->y + em);
               ctx_text (itk->ctx, "O");
             }
          }
          //if (c->no == itk->focus_no)
          //fprintf (stderr, "%f %i %i %i\n", text_edit_desired_x, text_edit, prev_line, next_line);

          ctx_restore (itk->ctx);
        }
      else if (!strcmp (media_type, "inode/directory"))
      {
        draw_folder (ctx, itk->x, itk->y, width, height);
      }
      else
      {
        if (ctx_media_type_class (media_type) == CTX_MEDIA_TYPE_IMAGE)
        {
          draw_img (itk, itk->x, itk->y, width, height, newpath);
        }
        else
        {
          draw_doc (ctx, itk->x, itk->y, width, height);
        }
      }
      if (layout_config.list_data)
      {

        ctx_save (itk->ctx);
        ctx_font_size (itk->ctx, em);
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
          ctx_font_size (itk->ctx, em); //em * 0.6);// * 0.9);
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
             y0 = layout_box[layout_box_no].y * saved_width;
             y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;
            }
            else
            {
              layout_box_no = 0;

             itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
             itk->y           = layout_box[layout_box_no].y * saved_width;
             itk->width       = layout_box[layout_box_no].width * saved_width ;
             y0 = layout_box[layout_box_no].y * saved_width;
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
      ctx_font_size (ctx, itk->font_size);
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

  itk_panel_start (itk, "files", 0,0, ctx_width(ctx),
                  ctx_height (ctx));

  if (dir_scale != 1.0f)
     ctx_scale (itk->ctx, dir_scale, dir_scale);

  if (!files->n)
  {
    itk_labelf (itk, "no files\n");
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
          ctx_add_key_binding (ctx, "+", NULL, NULL,
                          dir_zoom_in,
                          NULL);
          ctx_add_key_binding (ctx, "=", NULL, NULL,
                          dir_zoom_in,
                          NULL);
          ctx_add_key_binding (ctx, "-", NULL, NULL,
                          dir_zoom_out,
                          NULL);
#endif

          ctx_add_key_binding (ctx, "page-down", NULL, NULL,
                          dir_next_page,
                          NULL);
          ctx_add_key_binding (ctx, "page-up", NULL, NULL,
                          dir_prev_page,
                          NULL);

          int virtual = (metadata_key_int2 (focused_no, "virtual") > 0);
          if (item_outliner || virtual)
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

  if (clients && active)
  {
    ctx_font_size (ctx, itk->font_size);
    ctx_clients_draw (ctx);
    ctx_clients_handle_events (ctx);
  }

  return 0;
}

extern int _ctx_max_threads;
extern int _ctx_enable_hash_cache;

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
