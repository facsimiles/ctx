//
// todo
//   custom order , size / time orders
//      .ctxindex - with attributes as well
//                - skip per file meta-data
//   layout view
//      paginated mode
//      flow templates
//
// a file called ctx-template , searched in cur dir,
// ancestor dirs, and ctx folder.
// inserting a template, should be followed by newpage
// permitting chapter start to be template,newpage,template full-chapter,
// causing only the first bit of chapter to be styled this way.

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
};

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
  float padding;// ..
  float margin; // .. collapsed for inner

  int   fixed_size;
  int   fixed_pos;
  int   list_data;
} LayoutConfig;

LayoutConfig layout_config = {
  1,1,0,0, 1,0,
  4, 4, 0, 0.5,
  0, 0, 0
};


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
//int viewer_active = 0;

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

static CtxDirView view_type = CTX_DIR_LAYOUT;

static void set_layout (CtxEvent *e, void *d1, void *d2)
{
  view_type = CTX_DIR_LAYOUT;
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
  layout_config.padding = 0.5;
  layout_config.margin = 0.0;
  layout_config.fixed_size = 0;
  layout_config.fixed_pos = 0;
  layout_config.label = 0;
  layout_config.list_data = 0;
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
}

static void set_grid (CtxEvent *e, void *d1, void *d2)
{
  set_layout (e, d1, d2);
  layout_config.fixed_size = 1;
  layout_config.fixed_pos = 1;
  layout_config.stack_horizontal = 1;
  layout_config.stack_vertical = 1;
  layout_config.label = 1;
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
    //view_type = CTX_VIEW_GRID;
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


int ctx_handle_img (Ctx *ctx, const char *path);

ITK *itk = NULL;
static void item_activate (CtxEvent *e, void *d1, void *d2)
{
  //CtxEvent *e = &event; // we make a copy to permit recursion
  int no = (size_t)(d1);

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
#if 0
    if (ctx_renderer_is_ctx (e->ctx))
    {
      //fprintf (stderr, "launch \"%s\"!\n", new_path);
      fprintf (stdout, "\e_C;vim \"%s\"\e\\", new_path);
    }
    else
    {
      char *cmd = ctx_strdup_printf ("ctx 'vim \"%s\"'&", new_path);
      system (cmd);
    }
#endif
  }
  free (new_path);

  ctx_set_dirty (e->ctx, 1);
  e->stop_propagate = 1;
}

static int metadata_dirty = 0;

static void move_item_down (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  //char *new_path;
  //if (!strcmp (files->items[no], ".."))dd
  if (no<files->count-1)
  {
    metadata_swap (no, no+1);
    metadata_dirty ++;
    ctx_set_dirty (e->ctx, 1);
    itk_focus (itk, 1);
  }
}

static void grow_height (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float height = metadata_key_float (item_name, "height");
  if (height < 1) height = 100;
  height += 20;
  metadata_set_float (item_name, "height", height);
  fprintf (stderr, "%f\n", height);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void shrink_height (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float height = metadata_key_float (item_name, "height");
  height -= 20;
  if (height < 1) height = 20;
  metadata_set_float (item_name, "height", height);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void grow_width (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float width = metadata_key_float (item_name, "width");
  if (width < 1) width = 100;
  width += 20;
  metadata_set_float (item_name, "width", width);
  fprintf (stderr, "%f\n", width);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void shrink_width (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float width = metadata_key_float (item_name, "width");
  width -= 20;
  if (width < 1) width = 20;
  metadata_set_float (item_name, "width", width);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void move_left (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float x = metadata_key_float (item_name, "x");
  x -= 20;
  if (x < 1) x = 20;
  metadata_set_float (item_name, "x", x);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void move_right (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float x = metadata_key_float (item_name, "x");
  x += 20;
  if (x < 1) x = 20;
  metadata_set_float (item_name, "x", x);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void move_up (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float y = metadata_key_float (item_name, "y");
  y -= 20;
  if (y< 1) y = 20;
  metadata_set_float (item_name, "y", y);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void deactivate_viewer (CtxEvent *e, void *d1, void *d2)
{
  while (clients)
    ctx_client_remove (ctx, clients->data);
  active = NULL;
  ctx_set_dirty (e->ctx, 1);
}

static void move_down (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *item_name = metadata_item_name (no);
  float y = metadata_key_float (item_name, "y");
  y += 20;
  if (y< 1) y = 20;
  metadata_set_float (item_name, "y", y);
  metadata_dirty ++;
  ctx_set_dirty (e->ctx, 1);
}

static void move_item_up (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *new_path;
  if (no>0)
  {
    metadata_swap (no, no-1);
    metadata_dirty ++;
    ctx_set_dirty (e->ctx, 1);
    itk_focus (itk, -1);
  }
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
      (w - layout_config.padding * em * 2);
    float target_height = 
      (h - layout_config.padding * em * 2);
  //ctx_begin_path (ctx);

  if ((target_width > 250 || target_height > 250))
  {
    char reteid[65]="";
    ctx_texture_load (ctx, path, &imgw, &imgh, reteid);
    //fprintf (stderr, "%f %f %i %i %s\n", target_width, target_height, imgw, imgh, reteid);
    if (reteid[0])
    {
#if 1
      ctx_draw_texture (ctx, reteid, x + layout_config.padding * em, y + layout_config.padding * em, target_width, target_height);
#else
      ctx_rectangle (ctx, x + layout_config.padding * em, y + layout_config.padding*em, w - layout_config.padding * 2 * em, h - layout_config.padding * 2 * em);
      ctx_save (ctx);
      ctx_translate (ctx, x + layout_config.padding * em, y + layout_config.padding * em);
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
      ctx_rectangle (ctx, x + layout_config.padding * em, y + layout_config.padding*em, w - layout_config.padding * 2 * em, h - layout_config.padding * 2 * em);
      ctx_translate (ctx, x + layout_config.padding * em, y + layout_config.padding * em);
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

static int media_type_is_image (const char *media_type)
{
  if (media_type[0]!='i')return 0;
  if (media_type[1]!='m')return 0;
  if (media_type[2]!='a')return 0;
  if (media_type[3]!='g')return 0;
  if (media_type[4]!='e')return 0;
  return 1;
}

static int media_type_is_text (const char *media_type)
{
  if (media_type[0]!='t')return 0;
  if (media_type[1]!='e')return 0;
  if (media_type[2]!='x')return 0;
  if (media_type[3]!='t')return 0;
  return 1;
}

static int media_type_is_video (const char *media_type)
{
  if (media_type[0]!='v')return 0;
  if (media_type[1]!='i')return 0;
  if (media_type[2]!='d')return 0;
  if (media_type[3]!='e')return 0;
  if (media_type[4]!='o')return 0;
  return 1;
}

static int media_type_is_audio (const char *media_type)
{
  if (media_type[0]!='a')return 0;
  if (media_type[1]!='u')return 0;
  if (media_type[2]!='d')return 0;
  if (media_type[3]!='i')return 0;
  if (media_type[4]!='o')return 0;
  return 1;
}

static int dir_handle_event (Ctx *ctx, CtxEvent *ctx_event, const char *event)
{
  if (!active)
    return;
  VT *vt = active->vt;
  CtxClient *client = vt_get_client (vt);
  ctx_client_lock (client);
  if (!strcmp (event, "control-escape"))
  {
    ctx_client_unlock (client);
    deactivate_viewer (ctx_event, NULL, NULL);
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

static void dir_layout (ITK *itk, Files *files)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);

  for (int i = 0; i < files->count; i++)
  {
    if ((files->items[i][0] == '.' &&
         files->items[i][1] == '.') ||
        (files->items[i][0] != '.')
       )
    {
      const char *d_name = files->items[i];
      float width = 0;
      float height = 0;
      int label = metadata_key_int (d_name, "layout");
      if (label == -1234) label = layout_config.label;

      int gotpos = 0;
      char *xstr = metadata_key_string (d_name, "x");
      char *ystr = metadata_key_string (d_name, "y");

      float x = metadata_key_float (d_name, "x");
      float y = metadata_key_float (d_name, "y");
      float opacity = metadata_key_float (d_name, "opacity");
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
      float saved_x = itk->x;
      float saved_y = itk->y;

      if (layout_config.fixed_pos)
        gotpos = 0;
      if (gotpos)
      {
        itk->x = x;
        itk->y = y;
      }
      float sx = itk->x,sy = itk->y;
      ctx_user_to_device (itk->ctx, &sx, &sy);


      {
        width  = metadata_key_float (d_name, "width");
        if (width < 4 || layout_config.fixed_size) width = 
          layout_config.fill_width? itk->width * 1.0:
          layout_config.width * em;
      }
      {
        height = metadata_key_float (d_name, "height");
        if (height < 4 || layout_config.fixed_size)  height =
          layout_config.fill_height? itk->height * 1.0:
          layout_config.height * em;
      }

      CtxControl *c = itk_add_control (itk, UI_LABEL, "foo", itk->x, itk->y, width, height);


      if (!active && sy + height > 0 && sy < ctx_height (itk->ctx))
      {
        struct stat stat_buf;
        char *newpath = malloc (strlen(files->path)+strlen(d_name) + 2);
        if (!strcmp (files->path, PATH_SEP))
          sprintf (newpath, "%s%s", PATH_SEP, d_name);
        else
          sprintf (newpath, "%s%s%s", files->path, PATH_SEP, d_name);
        lstat (newpath, &stat_buf);
        int focused = 0;
        const char *media_type = ctx_path_get_media_type (newpath);

        if (c->no == itk->focus_no)
        {
          focused = 1;
          //viewer_load_path (newpath, files->items[i]);

          ctx_add_key_binding (ctx, "return", NULL, NULL,
                          item_activate,
                          (void*)((size_t)i));
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

          ctx_add_key_binding (ctx, "control-left", NULL, NULL, move_left, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-right", NULL, NULL, move_right, 
                         (void*)((size_t)i));

          ctx_add_key_binding (ctx, "control-up", NULL, NULL, move_up, 
                         (void*)((size_t)i));
          ctx_add_key_binding (ctx, "control-down", NULL, NULL, move_down, 
                         (void*)((size_t)i));
          //itk_labelf (itk, "%s\n", ctx_path_get_media_type (newpath));
        }

      ctx_begin_path (ctx);
      ctx_gray (ctx, 0.0);

      if (S_ISDIR(stat_buf.st_mode))
      {
        draw_folder (ctx, itk->x, itk->y, width, height);
      }
      else
      {
        if (media_type_is_image (media_type))
        {
          draw_img (itk, itk->x, itk->y, width, height, newpath);
        }
        else
        {
          draw_doc (ctx, itk->x, itk->y, width, height);
        }
        if (layout_config.list_data)
        {
          char buf[1024];
          ctx_move_to (itk->ctx, itk->x + width, itk->y + em * (1 + layout_config.padding) );
          sprintf (buf, "%s %i %i", d_name, stat_buf.st_size, files->namelist[i]->d_type);
          ctx_font_size (itk->ctx, em);
          ctx_gray (itk->ctx, 1.0);
          ctx_save (itk->ctx);
          ctx_text (itk->ctx, buf);
          ctx_restore (itk->ctx);

        }


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
      if (active)
      {
          ctx_listen (ctx, CTX_KEY_PRESS, dir_key_any, NULL, NULL);
          ctx_listen (ctx, CTX_KEY_DOWN, dir_key_any, NULL, NULL);
          ctx_listen (ctx, CTX_KEY_UP, dir_key_any, NULL, NULL);
      }

      if (gotpos)
      {
        itk->x = saved_x;
        itk->y = saved_y;
      }
      else
      {
        itk->x = saved_x + width;
        if (!layout_config.stack_horizontal ||
            itk->x + em * 5 > itk->panel->x + itk->panel->width)
        {
          itk->x = itk->x0;
          if (layout_config.stack_vertical)
            itk->y += height;
        }
      }
    }
  }
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
    ctx_set_dirty (ctx, 1);
    free (viewer_loaded_path);
    viewer_loaded_path = NULL;
  }

  if (viewer_loaded_name) free (viewer_loaded_name);
  viewer_loaded_name = strdup (name);

  if (path)
  {
    viewer_loaded_path = strdup (path);
    const char *media_type = ctx_path_get_media_type (path);
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
    const char *suffix = get_suffix (path);
    command[0]=0;
    if (!strcmp (media_type, "inode/directory"))
    {
       //fprintf (stderr, "is dir\n");
       return;
       //sprintf (command, "du -h '%s'", path);
    }

    char *basname = get_basename (path);

    if (!command[0])
    if (media_type_is_text (media_type))
    {
      sprintf (command, "vim +1 -R %s", escaped_path);
    }
    free (basname);
   
    if (!command[0])
    {
    if (media_type_is_image (media_type))
    {
      sprintf (command, "ctx %s", escaped_path);
    }
    else if (!strcmp (media_type, "video/mpeg"))
    {
      sprintf (command, "ctx -s %s", escaped_path);
    }
    else if (media_type_is_audio (media_type))
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
  Ctx *_ctx = itk->ctx;
  ctx = _ctx;
  //float em = itk_em (itk);
  //float row_height = em * 1.2;
  static int first = 1;
  if (first)
  {
    ctx_add_timeout (ctx, 1000, thumb_monitor, NULL);
    font_size = itk->font_size;
    //itk->font_size = font_size;
    //viewer_load_path ("/home/pippin/src/ctx/media/traffic.gif", "traffic.gif");
    first = 0;
  }
  //thumb_monitor (ctx, NULL);

  if (metadata_dirty)
  {
    metadata_save ();
    dm_set_path (files, files->path);
    metadata_dirty = 0;
  }

  itk_panel_start (itk, "files", 0,0, ctx_width(ctx),
                  
    view_type == CTX_DIR_LAYOUT ?
                  ctx_height (ctx) * 0.8 :
                  ctx_height (ctx));

  if (!files->n)
  {
    itk_labelf (itk, "no files\n");
  }
  else
  {
    ctx_add_key_binding (ctx, "F2", NULL, NULL, set_list, NULL);
    ctx_add_key_binding (ctx, "F3", NULL, NULL, set_grid, NULL);
    ctx_add_key_binding (ctx, "F4", NULL, NULL, set_layout, NULL);

    dir_layout (itk, files);
  }

  itk_panel_end (itk);

  if (view_type == CTX_DIR_LAYOUT)
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

  if (clients && active)
  {
    ctx_font_size (ctx, itk->font_size);
    ctx_clients_draw (ctx);
    //ctx_set_dirty (ctx, 1);
    ctx_clients_handle_events (ctx);
  }
  else
  {
    //viewer_active = 0;
  }

#if 0
  itk_panel_start (itk, "prop", ctx_width(ctx)/4, itk->font_size*1, ctx_width(ctx)/4, itk->font_size*8);
  int keys = metadata_item_key_count (viewer_loaded_name);
  itk_labelf (itk, "%s - %i", viewer_loaded_name, keys);
  for (int k = 0; k < keys; k++)
  {
    char *key = metadata_key_name (viewer_loaded_name, k);
    if (key)
    {
      char *val = metadata_key_string (viewer_loaded_name, key);
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

  view_type = CTX_DIR_LAYOUT;
  set_layout (NULL, NULL, NULL);
  dm_set_path (files, path?path:"./");
  itk_main (card_files, NULL);
  while (clients)
    ctx_client_remove (ctx, clients->data);
  return 0;
}
