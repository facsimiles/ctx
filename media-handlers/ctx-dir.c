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
#include "../terminal/vt.h"
#include "../terminal/ctx-clients.h"

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

void viewer_load_path (const char *path);

static int custom_sort (const struct dirent **a,
                        const struct dirent **b)
{
  if ((*a)->d_type != (*b)->d_type)
  {
    return ((*a)->d_type - (*b)->d_type);
  }
  return strcmp ((*a)->d_name , (*b)->d_name);
}

void dm_set_path (Files *files, const char *path)
{
  char *resolved_path = realpath (path, NULL);

  if (files->path)
    free (files->path);
  if (files->namelist)
    free (files->namelist);
  files->namelist = NULL;
  files->path = resolved_path;
  files->n = scandir (files->path, &files->namelist, NULL, custom_sort);
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
static void print_curr (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);

  char *new_path;
 
  if (!strcmp (files->namelist[no]->d_name, ".."))
  {
    new_path = get_dirname (files->path);
  }
  else
  {
    new_path = ctx_strdup_printf ("%s/%s", files->path, 
                                  files->namelist[no]->d_name);
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

static void files_list (ITK *itk, Files *files)
{
  float em = itk_em (itk);
  for (int i = 0; i < files->n; i++)
  {
    if ((files->namelist[i]->d_name[0] == '.' &&
         files->namelist[i]->d_name[1] == '.') ||
        (files->namelist[i]->d_name[0] != '.')
       )
    {
      struct stat stat_buf;

      float sx = itk->x,sy = itk->y;
      ctx_user_to_device (itk->ctx, &sx, &sy);

      CtxControl *c = itk_add_control (itk, UI_LABEL, "foo", itk->x, itk->y, itk->width, em * itk->rel_ver_advance);
      if (sy > 0 && sy < ctx_height (itk->ctx))
      {

      char *newpath = malloc (strlen(files->path)+strlen(files->namelist[i]->d_name) + 2);
      if (!strcmp (files->path, PATH_SEP))
        sprintf (newpath, "%s%s", PATH_SEP, files->namelist[i]->d_name);
      else
        sprintf (newpath, "%s%s%s", files->path, PATH_SEP, files->namelist[i]->d_name);
      lstat (newpath, &stat_buf);

      free (newpath);
      if (c->no == itk->focus_no)
        ctx_add_key_binding (itk->ctx, "return", NULL, NULL, print_curr, (void*)((size_t)i));
      itk_labelf (itk, "%s\n", files->namelist[i]->d_name);
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
  ctx_rel_line_to (ctx, 0, h * 0.62);
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
    ctx_gray (ctx, 1);
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

int is_grid = 1;

static void set_list (CtxEvent *e, void *d1, void *d2)
{
  is_grid = 0;
  ctx_set_dirty (e->ctx, 1);
}
static void set_grid (CtxEvent *e, void *d1, void *d2)
{
  is_grid = 1;
  ctx_set_dirty (e->ctx, 1);
}

static int filename_is_image (const char *filename)
{
  if (strstr (filename, ".png")) return 1;
  if (strstr (filename, ".jpg")) return 1;
  if (strstr (filename, ".JPG")) return 1;
  if (strstr (filename, ".JPEG")) return 1;
  if (strstr (filename, ".gif")) return 1;
  if (strstr (filename, ".GIF")) return 1;
  //if (strstr (filename, ".tif")) return 1;
  //if (strstr (filename, ".tiff")) return 1;
  //if (strstr (filename, ".TIF")) return 1;
  if (strstr (filename, ".jpeg")) return 1;
  if (strstr (filename, ".PNG")) return 1;
  return 0;
}

static void files_grid (ITK *itk, Files *files)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);


  for (int i = 0; i < files->n; i++)
  {
    if ((files->namelist[i]->d_name[0] == '.' &&
         files->namelist[i]->d_name[1] == '.') ||
        (files->namelist[i]->d_name[0] != '.')
       )
    {
      float sx = itk->x,sy = itk->y;
      ctx_user_to_device (itk->ctx, &sx, &sy);
      CtxControl *c = itk_add_control (itk, UI_LABEL, "foo", itk->x, itk->y, em * 6, em * 4);
      float saved_x = itk->x;
      if (sy + em * 4 > 0 && sy < ctx_height (itk->ctx))
      {


      struct stat stat_buf;
      const char *d_name = files->namelist[i]->d_name;
      char *newpath = malloc (strlen(files->path)+strlen(d_name) + 2);
      if (!strcmp (files->path, PATH_SEP))
        sprintf (newpath, "%s%s", PATH_SEP, d_name);
      else
        sprintf (newpath, "%s%s%s", files->path, PATH_SEP, d_name);
      lstat (newpath, &stat_buf);

      if (c->no == itk->focus_no)
      {
        viewer_load_path (newpath);

        ctx_add_key_binding (ctx, "return", NULL, NULL, print_curr, (void*)((size_t)i));
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

      ctx_move_to (itk->ctx, itk->x + em * 3, itk->y + em * 3.5);
      ctx_font_size (itk->ctx, em * 0.6);// * 0.9);
      ctx_gray (itk->ctx, 0.8);
      ctx_save (itk->ctx);
      ctx_text_align (itk->ctx, CTX_TEXT_ALIGN_CENTER);
      ctx_text (itk->ctx, files->namelist[i]->d_name);
      ctx_restore (itk->ctx);
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
    int batch_size = 5;
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
int  ctx_clients_draw (ITK *itk, Ctx *ctx);
void ctx_clients_handle_events (Ctx *ctx);

void viewer_load_path (const char *path)
{
  static char *loaded_path = NULL;
  if (path && loaded_path && !strcmp (loaded_path, path))
  {
    return;
  }
  if (loaded_path)
  {
    while (clients)
      ctx_client_remove (ctx, clients->data);
    ctx_set_dirty (ctx, 1);
    free (loaded_path);
    loaded_path = NULL;
  }
  if (path)
  {
    loaded_path = strdup (path);
    char *command = malloc (5 + strlen (path) + 16);
    const char *suffix = get_suffix (path);
    command[0]=0;
    if (ctx_path_is_dir (path))
    {
       fprintf (stderr, "is dir\n");
       return;
    }

    char *basname = get_basename (path);

    if (!strcmp (suffix, "c")||
        !strcmp (suffix, "txt")||
        !strcmp (suffix, "h")||
        !strcmp (suffix, "sh")||
        !strcmp (suffix, "ctx")||
        !strcmp (suffix, "md")||
        !strcmp (basname, "Makefile")||
        !strcmp (basname, "README")||
        !strcmp (basname, "ReadMe")
        )
    {
      sprintf (command, "vim -R %s", path);
    }
    free (basname);
   

    if (!strcmp (suffix, "png")||
        !strcmp (suffix, "gif")||
        !strcmp (suffix, "jpg")||
        !strcmp (suffix, "mpg")||
        !strcmp (suffix, "MPG")||
        !strcmp (suffix, "JPG")||
        !strcmp (suffix, "PNG")||
        !strcmp (suffix, "GIF")
        )
    {
      sprintf (command, "ctx %s", path);
    }
    if (command[0])
    {
      ctx_client_new (command,
        ctx_width(ctx)/2, 0, ctx_width(ctx)/2, ctx_height(ctx)-font_size*2, 0);
      fprintf (stderr, "run:%s %i %i %i %i,   %i\n", command,
        (int)ctx_width(ctx)/2, (int)0, (int)ctx_width(ctx)/2, (int)(ctx_height(ctx)-font_size*2), 0);
    }
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
    ctx_add_timeout (ctx, 250, thumb_monitor, NULL);
    font_size = itk->font_size;
    viewer_load_path ("/home/pippin/src/ctx/media/traffic.gif");
    first = 0;
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

    if (is_grid)
      files_grid (itk, files);
    else
      files_list (itk, files);
  }

  itk_panel_end (itk);

  if (clients)
  {
    ctx_clients_draw (itk, ctx);
    //ctx_set_dirty (ctx, 1);
    ctx_clients_handle_events (ctx);
  }

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
