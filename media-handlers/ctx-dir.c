//
// todo
//   custom order , size / time orders
//      .ctxindex - with attributes as well
//                - skip per file meta-data
//   layout view
//      paginated mode
//      flow templates
//
//
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

static int ctx_path_is_dir (const char *path)
{
  struct stat stat_buf;
  lstat (path, &stat_buf);
  return S_ISDIR (stat_buf.st_mode);
}

ITK *itk = NULL;
static void item_activate (CtxEvent *e, void *d1, void *d2)
{
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
  if (ctx_path_is_dir (new_path))
  {
    dm_set_path (files, new_path);
    itk->focus_no = 0;
  }
  else
  {
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
  }
  free (new_path);

  ctx_set_dirty (e->ctx, 1);
}

static int metadata_dirty = 0;

static void move_item_down (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  char *new_path;
  //if (!strcmp (files->items[no], ".."))dd
  if (no<files->count-1)
  {
    metadata_swap (no, no+1);
    metadata_dirty ++;
    ctx_set_dirty (e->ctx, 1);
    itk_focus (itk, 1);
  }
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

static void files_list (ITK *itk, Files *files)
{
  float em = itk_em (itk);
  for (int i = 0; i < files->count; i++)
  {
    if ((files->items[i][0] == '.' &&
         files->items[i][1] == '.') ||
        (files->items[i][0] != '.')
       )
    {
      struct stat stat_buf;

      float sx = itk->x,sy = itk->y;
      ctx_user_to_device (itk->ctx, &sx, &sy);

      CtxControl *c = itk_add_control (itk, UI_LABEL, "foo", itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
      if (sy > 0 && sy < ctx_height (itk->ctx))
      {

      char *newpath = malloc (strlen(files->path)+strlen(files->items[i]) + 2);
      if (!strcmp (files->path, PATH_SEP))
        sprintf (newpath, "%s%s", PATH_SEP, files->items[i]);
      else
        sprintf (newpath, "%s%s%s", files->path, PATH_SEP, files->items[i]);
      lstat (newpath, &stat_buf);


      if (c->no == itk->focus_no)
      {
        viewer_load_path (newpath, files->items[i]);

        ctx_add_key_binding (ctx, "return", NULL, NULL, item_activate, (void*)((size_t)i));
        ctx_add_key_binding (ctx, "control-down", NULL, NULL, move_item_down, 
                         (void*)((size_t)i));
        ctx_add_key_binding (ctx, "control-up", NULL, NULL, move_item_up, 
                         (void*)((size_t)i));
      }
      free (newpath);

      itk_labelf (itk, "%s\n", files->items[i]);
      itk_sameline (itk);
      itk->x = itk->x0 + itk_em (itk) * 10;
      if (S_ISDIR (stat_buf.st_mode))
      itk_labelf (itk, "[DIR] %i", stat_buf.st_size, files->namelist[i]->d_type);
      else
      itk_labelf (itk, "%i %i", stat_buf.st_size, files->namelist[i]->d_type);
      }
      else
      {
        itk_labelf (itk, "");
      }
    }
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

static void draw_img (Ctx *ctx, float x, float y, float w, float h, const char *path)
{
  int imgw, imgh;
  char *thumb_path = ctx_get_thumb_path (path);
  if (access (thumb_path, F_OK) != -1)
  {
    char reteid[65]="";
    ctx_texture_load (ctx, thumb_path, &imgw, &imgh, reteid);
    if (reteid[0])
    {
#if 0
      ctx_draw_texture (ctx, reteid, x, y, w, h);
#else
      ctx_rectangle (ctx, x + w * 0.2, y + h * 0.1, w * 0.5, h * 0.7);
      ctx_save (ctx);
      ctx_translate (ctx, x + w * 0.2, y + h * 0.1);
      ctx_scale (ctx, w * 0.5/ imgw, h * 0.5 / imgh);
      ctx_texture (ctx, reteid, 0,0);
      ctx_fill (ctx);
      ctx_restore (ctx);
     #endif
      return;
    }
  }
  else
  {
    queue_thumb (path, thumb_path);
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

typedef enum {
  CTX_DIR_LIST,
  CTX_DIR_GRID,
  CTX_DIR_LAYOUT
}
CtxDirView;

static CtxDirView view_type = 1;


static void set_layout (CtxEvent *e, void *d1, void *d2)
{
  view_type = CTX_DIR_LAYOUT;
  ctx_set_dirty (e->ctx, 1);
}
static void set_list (CtxEvent *e, void *d1, void *d2)
{
  view_type = CTX_DIR_LIST;
  ctx_set_dirty (e->ctx, 1);
}
static void set_grid (CtxEvent *e, void *d1, void *d2)
{
  view_type = CTX_DIR_GRID;
  ctx_set_dirty (e->ctx, 1);
}

static int filename_is_image (const char *filename)
{
  if (strstr (filename, ".png"))  return 1;
  if (strstr (filename, ".jpg"))  return 1;
  if (strstr (filename, ".JPG"))  return 1;
  if (strstr (filename, ".JPEG")) return 1;
  if (strstr (filename, ".gif"))  return 1;
  if (strstr (filename, ".GIF"))  return 1;
  //if (strstr (filename, ".tif")) return 1;
  //if (strstr (filename, ".tiff")) return 1;
  //if (strstr (filename, ".TIF")) return 1;
  if (strstr (filename, ".jpeg")) return 1;
  if (strstr (filename, ".PNG"))  return 1;
  return 0;
}

static void files_grid (ITK *itk, Files *files)
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
      float sx = itk->x,sy = itk->y;
      ctx_user_to_device (itk->ctx, &sx, &sy);
      CtxControl *c = itk_add_control (itk, UI_LABEL, "foo", itk->x, itk->y, em * 6, em * 4);
      float saved_x = itk->x;
      if (sy + em * 4 > 0 && sy < ctx_height (itk->ctx))
      {


      struct stat stat_buf;
      const char *d_name = files->items[i];
      char *newpath = malloc (strlen(files->path)+strlen(d_name) + 2);
      if (!strcmp (files->path, PATH_SEP))
        sprintf (newpath, "%s%s", PATH_SEP, d_name);
      else
        sprintf (newpath, "%s%s%s", files->path, PATH_SEP, d_name);
      lstat (newpath, &stat_buf);
      int focused = 0;

      if (c->no == itk->focus_no)
      {
        focused = 1;
        viewer_load_path (newpath, files->items[i]);

        ctx_add_key_binding (ctx, "return", NULL, NULL, item_activate, (void*)((size_t)i));
        ctx_add_key_binding (ctx, "control-down", NULL, NULL, move_item_down, 
                         (void*)((size_t)i));
        ctx_add_key_binding (ctx, "control-up", NULL, NULL, move_item_up, 
                         (void*)((size_t)i));
      }

      ctx_begin_path (ctx);
      ctx_gray (ctx, 0.0);

      if (S_ISDIR(stat_buf.st_mode))
      {
        draw_folder (ctx, itk->x, itk->y, em * 6, em * 4);
      }
      else
      {
        if (filename_is_image (d_name))
        {
          draw_img (ctx, itk->x, itk->y, em * 6, em * 4, newpath);
        }
        else
        {
          draw_doc (ctx, itk->x, itk->y, em * 6, em *  4);
        }
      }
      free (newpath);

      char *title = malloc (strlen (files->items[i]) + 32);
      strcpy (title, files->items[i]);
      int title_len = strlen (title);

      {
        int wraplen = 12;
        int lines = (title_len + wraplen - 1)/ wraplen;

        if (!focused &&  lines > 2) lines = 2;

        for (int i = 0; i < lines; i++)
        {
        ctx_move_to (itk->ctx, itk->x + em * 3, itk->y + em * (4.0 + i) - lines/2.0 * em);
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
      itk->x = saved_x + em * 6;
      if (itk->x + em * 5 > itk->panel->x + itk->panel->width)
      {
        itk->x = itk->x0;
        itk->y += em * 4;
      }
    }
  }
}

static void files_layout (ITK *itk, Files *files)
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
      float sx = itk->x,sy = itk->y;
      ctx_user_to_device (itk->ctx, &sx, &sy);
      CtxControl *c = itk_add_control (itk, UI_LABEL, "foo", itk->x, itk->y, em * 6, em * 4);
      float saved_x = itk->x;
      if (sy + em * 4 > 0 && sy < ctx_height (itk->ctx))
      {


      struct stat stat_buf;
      const char *d_name = files->items[i];
      char *newpath = malloc (strlen(files->path)+strlen(d_name) + 2);
      if (!strcmp (files->path, PATH_SEP))
        sprintf (newpath, "%s%s", PATH_SEP, d_name);
      else
        sprintf (newpath, "%s%s%s", files->path, PATH_SEP, d_name);
      lstat (newpath, &stat_buf);
      int focused = 0;

      if (c->no == itk->focus_no)
      {
        focused = 1;
        viewer_load_path (newpath, files->items[i]);

        ctx_add_key_binding (ctx, "return", NULL, NULL, item_activate, (void*)((size_t)i));
        ctx_add_key_binding (ctx, "control-down", NULL, NULL, move_item_down, 
                         (void*)((size_t)i));
        ctx_add_key_binding (ctx, "control-up", NULL, NULL, move_item_up, 
                         (void*)((size_t)i));
      }

      ctx_begin_path (ctx);
      ctx_gray (ctx, 0.0);

      if (S_ISDIR(stat_buf.st_mode))
      {
        draw_folder (ctx, itk->x, itk->y, em * 6, em * 4);
      }
      else
      {
        if (filename_is_image (d_name))
        {
          draw_img (ctx, itk->x, itk->y, em * 6, em * 4, newpath);
        }
        else
        {
          draw_doc (ctx, itk->x, itk->y, em * 6, em *  4);
        }
      }
      free (newpath);

      char *title = malloc (strlen (files->items[i]) + 32);
      strcpy (title, files->items[i]);
      int title_len = strlen (title);

      {
        int wraplen = 12;
        int lines = (title_len + wraplen - 1)/ wraplen;

        if (!focused &&  lines > 2) lines = 2;

        for (int i = 0; i < lines; i++)
        {
        ctx_move_to (itk->ctx, itk->x + em * 3, itk->y + em * (4.0 + i) - lines/2.0 * em);
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
      //itk->x = saved_x + em * 6;
      //if (itk->x + em * 5 > itk->panel->x + itk->panel->width)
      {
        itk->x = itk->x0;
        itk->y += em * 4;
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
  if (path && viewer_loaded_path && !strcmp (viewer_loaded_path, path))
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
    if (ctx_path_is_dir (path))
    {
       //fprintf (stderr, "is dir\n");
       return;
       //sprintf (command, "du -h '%s'", path);
    }

    char *basname = get_basename (path);

    if (!command[0])
    if (!strcmp (suffix, "c")||
        !strcmp (suffix, "txt")||
        !strcmp (suffix, "h")||
        !strcmp (suffix, "html")||
        !strcmp (suffix, "sh")||
        !strcmp (suffix, "ctx")||
        !strcmp (suffix, "md")||
        !strcmp (basname, "Makefile")||
        !strcmp (basname, "README")||
        !strcmp (basname, "ReadMe")
        )
    {
      sprintf (command, "vim +1 -R %s", escaped_path);
    }
    free (basname);
   
    if (!command[0])
    {
    if (!strcmp (suffix, "png")||
        !strcmp (suffix, "gif")||
        !strcmp (suffix, "jpg")||
        !strcmp (suffix, "JPG")||
        !strcmp (suffix, "PNG")||
        !strcmp (suffix, "GIF")
        )
    {
      sprintf (command, "ctx %s", escaped_path);
    }
    else if (!strcmp (suffix, "mpg")||
          !strcmp (suffix, "MPG"))
    {
      sprintf (command, "ctx -s %s", escaped_path);
    }
    else if (!strcmp (suffix, "mp3")||
          !strcmp (suffix, "MP3") ||
          !strcmp (suffix, "ogg") ||
          !strcmp (suffix, "OGG") ||
          !strcmp (suffix, "WAV") ||
          !strcmp (suffix, "wav"))
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
      ctx_client_new (ctx, command,
        ctx_width(ctx)/2, 0, ctx_width(ctx)/2, ctx_height(ctx)-font_size*2, 0);
    //fprintf (stderr, "[%s]\n", command);
#if 0
      fprintf (stderr, "run:%s %i %i %i %i,   %i\n", command,
        (int)ctx_width(ctx)/2, (int)0, (int)ctx_width(ctx)/2, (int)(ctx_height(ctx)-font_size*2), 0);
#endif
    }

    free (escaped_path);
    free (command);
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

  itk_panel_start (itk, "files", 0,0, ctx_width(ctx)/2, ctx_height (ctx));

  if (!files->n)
  {
    itk_labelf (itk, "no files\n");
  }
  else
  {
    ctx_add_key_binding (ctx, "F1", NULL, NULL, set_list, NULL);
    ctx_add_key_binding (ctx, "F2", NULL, NULL, set_grid, NULL);
    ctx_add_key_binding (ctx, "F3", NULL, NULL, set_layout, NULL);

    switch (view_type)
    {
      case CTX_DIR_GRID:
        files_grid (itk, files);
        break;
      case CTX_DIR_LIST:
        files_list (itk, files);
        break;
      case CTX_DIR_LAYOUT:
        files_layout (itk, files);
        break;
    }
  }

  itk_panel_end (itk);

  if (clients)
  {
    ctx_font_size (ctx, itk->font_size);
    ctx_clients_draw (ctx);
    //ctx_set_dirty (ctx, 1);
    ctx_clients_handle_events (ctx);
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

int ctx_dir_main (int argc, char **argv)
{
  char *path = argv[1];
  if (path && strchr (path, ':'))
  {
    path = strchr (path, ':');
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }

  dm_set_path (files, path?path:"./");
  itk_main (card_files, NULL);
  while (clients)
    ctx_client_remove (ctx, clients->data);
  return 0;
}
