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
#include "argvs.h"
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>

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

  int hide_non_file;
  float margin_top; // page_margin
  int monospace;
  float line_height;
} LayoutConfig;

LayoutConfig layout_config = {
  1,1,0,0, 1,0,
  4, 4, 0, 0.5,
  0, 0, 0, 3,
  1
};

int text_editor = 0; /* global variable that changes how some loading/saving
                        is done - we are not expected to run within a regular
                        instance but our own process
                      */
static int vim_keys = 0;
int show_keybindings = 0;
int focused_no = -1;
int layout_page_no = 0;
int layout_show_page = 0;
int layout_last_page = 0;
float dir_scale = 1.0f;

unsigned long viewer_slide_start = 0;
unsigned long viewer_slide_trigger = 0;

long viewer_slide_duration = 0;

int viewer_slideshow = 0; // slide-show / video theater mode active

// TODO global variables that should be enclosed in a struct

typedef struct _CtxSHA1 CtxSHA1;
CtxSHA1 *ctx_sha1_new (void);
void ctx_sha1_free    (CtxSHA1 *sha1);
int  ctx_sha1_process (CtxSHA1 *sha1, const unsigned char * msg, unsigned long len);
int  ctx_sha1_done    (CtxSHA1 *sha1, unsigned char *out);

extern Ctx *ctx;

#include "diz.h"

#define PATH_SEP "/"

#define TEXT_EDIT_OFF                   -10
#define TEXT_EDIT_FIND_CURSOR_FIRST_ROW -9
#define TEXT_EDIT_FIND_CURSOR_LAST_ROW  -8

#define DIR_LOG_ERROR    0
#define DIR_LOG_MESSAGE  1
#define DIR_LOG_WARNING  2
#define DIR_LOG_INFO     3
#define DIR_LOG_DETAIL   4

static int dir_log_level = DIR_LOG_ERROR;

static inline void dir_log (int level, const char *format, ...)
{
  char *log_label[] = {"  ERROR",
                       "MESSAGE",
                       "WARNING",
                       "   INFO",
                       " DETAIL"};
  if (level > dir_log_level)
    return;
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = (char*)malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);

  if (level < 0) level = 0;
  if (level > DIR_LOG_DETAIL) level = DIR_LOG_DETAIL;
  fprintf (stderr, "%s %s\n", log_label[level], buffer);
  free (buffer);
}

#define DIR_MESSAGE(args...) dir_log(DIR_LOG_MESSAGE, args)
#define DIR_INFO(args...)    dir_log(DIR_LOG_INFO, args)
#define DIR_ERROR(args...)   dir_log(DIR_LOG_ERROR, args)
#define DIR_WARNING(args...) dir_log(DIR_LOG_WARNING, args)
#define DIR_DETAIL(args...)  dir_log(DIR_LOG_DETAIL,  args)


CtxList *thumb_queue = NULL;
typedef struct ThumbQueueItem
{
  char *path;
  char *thumbpath;
} ThumbQueueItem;

static void queue_thumb (const char *path, const char *thumbpath)
{
  ThumbQueueItem *item;
  DIR_INFO("queue thumb ", path);
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

char *ctx_get_thumb_path (const char *path) // XXX add dim as arg?
{
  char *rp = realpath (path, NULL);
  char *dname = get_dirname (rp);
  char *bname = get_basename (rp);
  char *ret = ctx_strdup_printf ("%s/.ctx/256x256/%s", dname, bname);
  free (dname);
  free (bname);
  free (rp);
  return ret;
}

static void user_data_free (CtxClient *client, void *user_data)
{
  free (user_data);
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

typedef enum {
  CTX_DIR_LAYOUT,
  CTX_DIR_IMAGE
}
CtxDirView;

static int layout_find_item = -1; // -1 = found
ITK *itk = NULL;

int ctx_vt_had_alt_screen (VT *vt);

int stuff_stdout_is_running (void)
{
  CtxClient *client = ctx_client_find (ctx, "stdout");
  if (!client) return 0;
  if (ctx_client_flags (client) & ITK_CLIENT_FINISHED)
  {
     if (ctx_client_vt (client))
     {
       if (ctx_vt_had_alt_screen (ctx_client_vt (client)))
       {
          ctx_client_remove (itk->ctx, client);
       }
     }
     return 0;
  }
  return 1;
}

static void set_layout (CtxEvent *e, void *d1, void *d2)
{
  if (e)
  ctx_queue_draw (e->ctx);
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
  layout_config.padding_top = 0.25f;
  layout_config.padding_bottom = 0.25f;
  layout_config.margin_top = 3.5f;

  layout_config.use_layout_boxes = 1;
  layout_config.level_indent = 2.5;
  layout_config.outliner = 0;
  layout_config.monospace = 0;
  layout_config.line_height = 1.0;

  layout_config.hide_non_file = 0;
  if (focused_no>=0)
  {
    layout_find_item = focused_no;
    if (itk)
      itk->focus_no = -1;
  }
};

static void set_text_edit (CtxEvent *e, void *d1, void *d2)
{
  set_layout (e, d1, d2);
  layout_config.monospace = 1;
  layout_config.padding_left   = 0.0f;
  layout_config.padding_top    = 0.0f;
  layout_config.padding_bottom = 0.0f;
  layout_config.padding_right  = 0.0f;
  layout_config.margin_top    = 0.0f;
}

#if 0
static void set_outline (CtxEvent *e, void *d1, void *d2)
{
  set_layout (e, d1, d2);
  layout_config.outliner = 1;
} 
#endif

static void set_list (CtxEvent *e, void *d1, void *d2)
{
  if (e)
    ctx_queue_draw (e->ctx);
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



int dir_view (COMMAND_ARGS) /* "view", 1, "<view>", "change view mode for category" */
{
   if (!strcmp (argv[1], "grid")) set_grid (NULL, NULL, NULL);
   if (!strcmp (argv[1], "list")) set_list (NULL, NULL, NULL);
   if (!strcmp (argv[1], "layout")) set_layout (NULL, NULL, NULL);
   //if (!strcmp (argv[1], "outline")) set_outline (NULL, NULL, NULL);
   if (!strcmp (argv[1], "text-edit")) set_text_edit (NULL, NULL, NULL);
   return 0;
}

typedef enum CtxBullet {
  CTX_BULLET_NONE = 0,
  CTX_BULLET_BULLET,
  CTX_BULLET_NUMBERS,
  CTX_BULLET_TODO,
  CTX_BULLET_DONE,
} CtxBullet;

Diz *diz = NULL;


static CtxControl *focused_control = NULL;

int diz_dir_is_in_flow (Diz *diz, int no)
{
   return (diz_dir_has_key (diz, no, "x") == 0 &&
           diz_dir_has_key (diz, no, "y") == 0);
}

int item_unflow (COMMAND_ARGS) /* "unflow", 0, "", "moves item out of flow and static on page" */
{
  if (diz_dir_has_children (diz, focused_no))
  {
    fprintf (stderr, "not unflowing with children\n");
    return -1;
  }

  if (focused_control)
  {
     diz_dir_set_float (diz, focused_no, "x",
                  focused_control->x / itk->width);
     diz_dir_set_float (diz, focused_no, "y",
                  focused_control->y / itk->width);

     diz_dir_set_float (diz, focused_no, "width",
                  focused_control->width / itk->width);
     diz_dir_set_float (diz, focused_no, "height",
                  focused_control->height / itk->width);
     diz_dir_dirt (diz);

#if 0
     int insertion_point = diz_dir_prev_sibling (diz, focused_no);
     while (insertion_point &&
            diz_dir_type_atom (diz, insertion_point) != CTX_ATOM_NEWPAGE &&
            diz_dir_is_in_flow (diz, insertion_point))
     {
       if (diz_dir_prev_sibling (diz, insertion_point)>=0)
         insertion_point = diz_dir_prev_sibling (diz, insertion_point);
       else if(diz_dir_get_parent (diz, insertion_point)>=0)
         insertion_point = diz_dir_get_parent (diz, insertion_point);
       else
         insertion_point = 0;
     }
     if (diz_dir_type_atom (diz, insertion_point) == CTX_ATOM_NEWPAGE)
        insertion_point = diz_dir_next_sibling (diz, insertion_point);
     diz_dir_insert (diz, insertion_point, "temp");
     diz_dir_dirt (diz);
     diz_dir_swap (diz, insertion_point, focused_no+1);
     diz_dir_dirt (diz);
     diz_dir_remove (diz, focused_no+1);
     diz_dir_dirt (diz);
#else
     int insertion_point = focused_no+1;
     while (insertion_point < diz->count &&
            diz_dir_type_atom (diz, insertion_point) != CTX_ATOM_NEWPAGE &&
            diz_dir_is_in_flow (diz, insertion_point))
     {
         insertion_point++;
     }
     //if (diz_dir_type_atom (diz, insertion_point) == CTX_ATOM_NEWPAGE)
     //   insertion_point = diz_dir_next_sibling (diz, insertion_point);
     diz_dir_insert (diz, insertion_point, "temp");
     diz_dir_dirt (diz);
     diz_dir_swap (diz, insertion_point, focused_no);
     diz_dir_dirt (diz);
     diz_dir_remove (diz, focused_no);
     diz_dir_dirt (diz);
#endif
     focused_no = insertion_point-1;
     layout_find_item = focused_no;
     itk->focus_no = -1;
  }
     

  return 0;
}

int item_reflow (COMMAND_ARGS) /* "reflow", 0, "", "moves item out of flow and static on page" */
{
  diz_dir_unset (diz, focused_no, "x");
  diz_dir_unset (diz, focused_no, "y");
  // not needed - but tidiest to do a fullish reset to dynamic size
    diz_dir_unset (diz, focused_no, "width");
    diz_dir_unset (diz, focused_no, "height");
  return 0;
}

static void drop_item_renderers (Ctx *ctx)
{
  again:
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (!(ctx_client_flags (client) & ITK_CLIENT_LAYER2))
    {
      ctx_client_remove (ctx, client);
      goto again;
    }
  }
}

static void save_metadata (void)
{ 
  if (diz->dirty)
  {
    diz_dir_save (diz);
    diz_dir_set_path (diz, diz->path);
    drop_item_renderers (ctx);
    diz->dirty = 0;
  }
}

const char *get_suffix (const char *path)
{
  const char *last_dot = strrchr (path, '.');
  const char *last_slash = strrchr (path, '/');

  if (last_slash > last_dot) return "";
  if (last_dot)
return last_dot + 1;
return "";
}

int ctx_handle_img (Ctx *ctx, const char *path);

static int text_edit = TEXT_EDIT_OFF;

//static int nav_edit = 4;

static float text_edit_desired_x = -123;

CtxString *commandline = NULL;
int        commandline_cursor_start = 0;
int        commandline_cursor_end = 0;

static inline int is_text_editing (void)
{
  return (text_edit != TEXT_EDIT_OFF) ||
         (commandline->str[0]!=0);
}


int dir_insert (COMMAND_ARGS) /* "insert-text", 0, "", "" */
{
  diz_dir_insert (diz, focused_no, "");
  text_edit = 0;
  diz_dir_dirt (diz);
  return 0;
}

int dir_insert_newpage (COMMAND_ARGS) /* "insert-newpage", 0, "", "" */
{
  diz_dir_insert (diz, focused_no, "");
  diz_dir_set_string (diz, focused_no, "type", "newpage");
  text_edit = 0;
  diz_dir_dirt (diz);
  return 0;
}

static int path_is_executable (const char *path)
{
  struct stat stat_buf;
  if (!path || path[0]==0) return 0;
  lstat (path, &stat_buf);
  return stat_buf.st_mode & 0x1;
  //return S_ISDIR (stat_buf.st_mode);
}


static int path_is_dir (const char *path)
{
  struct stat stat_buf;
  if (!path || path[0]==0) return 0;
  lstat (path, &stat_buf);
  return S_ISDIR (stat_buf.st_mode);
}

static int path_exist (const char *path)
{
  struct stat stat_buf;
  if (!path || path[0]==0) return 0;
  return (lstat (path, &stat_buf) == 0);
}

static char *diz_dir_wiki_path (const char *path);

static CtxList *history = NULL;
static CtxList *future = NULL;

static void _set_location (const char *location)
{
  char *tpath = NULL;
  save_metadata();


  if (location[0] == '~')
  {
    char *tpath = ctx_strdup_printf ("%s/%s", getenv("HOME"), &location[2]);
    location = tpath;
  }


  if (location[0] == '/' || location[0] == '.')
  {
    char *loc = (char*)location;
    if (location[0] == '.' && location[1] == '/')
    {
      if (diz->path)
      loc = ctx_strdup_printf ("%s/%s", diz->path, location+2);
      else
      loc = realpath (location+2, NULL);
    }

    if (!path_exist(loc))
    {
    //  char *last_slash = strrchr(loc,'/');
    //  *last_slash = 0;
      mkdir_ancestors (loc, 0777);
    //  *last_slash = '/';
      if (loc[strlen(loc)-1]=='/')
      {
      }
      else
      {
        fprintf (stderr, "should make file %s\n", loc);
        FILE *f = fopen (loc, "w");
        fclose (f);
      }
    }
    
    {

    if (path_is_dir (loc))
    {
      //fprintf (stderr, "%i:%s\n", __LINE__, loc);
      if (loc[strlen(loc)-1]=='/')
        loc[strlen(loc)-1]='\0';
      diz_dir_set_path (diz, loc);
      drop_item_renderers (ctx);
      focused_no = -1;
      layout_find_item = 0;
      text_editor = 0;
    }
    else
    {
      text_editor = 1;
      set_text_edit (NULL, NULL, NULL);
      focused_no = 0;
      layout_find_item = focused_no;
      focused_no = -1;
      layout_find_item = 0;
      diz_dir_set_path_text_editor (diz, loc);
    }
    }
    if (loc != location) free (loc);
  } else
  {
    tpath = diz_dir_wiki_path (location);
    text_editor = 0;
    diz_dir_set_path (diz, tpath);
    diz_dir_set_name (diz, -1, location);
    save_metadata ();
    drop_item_renderers (ctx);
    focused_no = 0;
    layout_find_item = 0;
  }
  itk_panels_reset_scroll (itk);
  if (tpath)
    free (tpath);
}

int cmd_history (COMMAND_ARGS) /* "history", 1, "<forward|back>", "moved history forward or back" */
{
  if (!strcmp (argv[1], "back"))
  {
    if (history == NULL)
      return -1;
    char *location = history->data;
    ctx_list_remove (&history, location);

    if (diz->title)
      ctx_list_prepend (&future, strdup (diz->title));
    else
      ctx_list_prepend (&future, strdup (diz->path));

    _set_location (location);

    free (location);
  }
  else if (!strcmp (argv[1], "forward"))
  {
    if (future == NULL)
      return -1;

    char *location = future->data;
    ctx_list_remove (&future, location);

    if (diz->title)
      ctx_list_prepend (&history, strdup (diz->title));
    else
      ctx_list_prepend (&history, strdup (diz->path));

    _set_location (location);
    free (location);
  }
  return 0;
}

static void set_location (const char *location)
{
  if (!text_editor)
  {
    while (future)
    {
      free (future->data);
      ctx_list_remove (&future, future->data);
    }
    if (diz->path)
    {
      if (diz->title)
        ctx_list_prepend (&history, strdup (diz->title));
      else
        ctx_list_prepend (&history, strdup (diz->path));
    }
  }
  _set_location (location);
  focused_no = -1;
  layout_find_item = -1;
}

int cmd_go_parent (COMMAND_ARGS) /* "go-parent", 0, "", "" */
{
  char *old_path = strdup (diz->path);
  char *new_path = get_dirname (diz->path);
  set_location (new_path);

  layout_find_item = diz_dir_name_to_no (diz, strrchr (old_path, '/')+1);
  free (old_path);

  itk->focus_no = -1;
  free (new_path);
  return 0;
}

int toggle_fullscreen (COMMAND_ARGS) /* "toggle-fullscreen", 0, "", "" */
{
  if (ctx_get_fullscreen (ctx))
    ctx_set_fullscreen (ctx, 0);
  else
    ctx_set_fullscreen (ctx, 1);
  return 0;
}

int toggle_keybindings_display (COMMAND_ARGS) /* "toggle-keybindings-display", 0, "", ""*/
{
  show_keybindings = !show_keybindings;
  return 0;
}

int outline_expand (COMMAND_ARGS) /* "outline-expand", 0, "", "" */
{
  int no = focused_no;
  if (diz_dir_type_atom (diz, no+1) != CTX_ATOM_STARTGROUP)
    return -1;

  diz_dir_unset (diz, no, "folded");
  diz_dir_dirt (diz);
  return 0;
}

int outline_collapse (COMMAND_ARGS) /* "outline-fold", 0, "", "" */
{
  int no = focused_no;
  if (diz_dir_type_atom (diz, no+1) != CTX_ATOM_STARTGROUP)
    return 0;

  diz_dir_set_float (diz, no, "folded", 1);
  diz_dir_dirt (diz);
  return 0;
}

static int layout_focused_link = -1;

static inline int char_is_digit (char c) {
  return c >= '0' && c <= '9';
}
static inline int char_is_day_digit (char c) {
  return c >= '0' && c <= '9';
}

static int word_is_date (const char *word)
{
  if (!char_is_digit(word[0])) return 0;
  if (!char_is_digit(word[1])) return 0;
  if (!char_is_digit(word[2])) return 0;
  if (!char_is_digit(word[3])) return 0;
  //if (word[4]==0) return 1;
  if (word[4]!='-') return 0;
  if (!(word[5]>='0' && word[5]<='1')) return 0;
  if (!char_is_digit(word[6])) return 0;
  if (word[7]==0) return 1;
  if (word[7]!='-') return 0;
  if (!(word[8]>='0' && word[8]<='3')) return 0;
  if (!char_is_digit(word[9])) return 0;
  if (word[10]==0) return 1;
  return 0;
}

static int string_count_links (const char *name)
{
  int count = 0;
  char *p = name;
  int in_link = 0;
  CtxString *word = ctx_string_new ("");
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
      case ' ':
        if (word_is_date (word->str))
          count ++;
        ctx_string_set (word, "");
        break;
      default:
        ctx_string_append_byte (word, *p);
    }
    p++;
  }
  if (word_is_date (word->str))
    count ++;
  ctx_string_free (word, 1);
  return count;
}

static char *string_link_no (const char *string, int link_no)
{
  CtxString *str = ctx_string_new ("");
  CtxString *str_secondary = ctx_string_new ("");

  const char *p = string;
  int in_link = 0;
  int count = 0;
  CtxString *word = ctx_string_new ("");
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

        if (*p == '\0' || *p == ' ')
        {
          if (word_is_date (word->str) && !in_link)
          {
            count++;
            if (link_no == count-1)
            {
              ctx_string_free (str, 1);
              ctx_string_free (str_secondary, 1);
              return ctx_string_dissolve (word);
            }
          }
          ctx_string_set(word, "");
        }
        else
        {
          ctx_string_append_byte (word, *p);
        }


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
          ctx_string_free (word, 1);
          return ret;
        }
        break;
    }
    p++;
  }
  ctx_string_free (str_secondary, 1);
  ctx_string_free (str, 1);
  ctx_string_free (word, 1);
  return NULL;
}

static int dir_item_count_links (int i)
{
  int count = 0;
  char *name = diz_dir_get_name (diz, i);
  count = string_count_links (name);
  free (name);
  return count;
}

static char *dir_item_link_no (int item, int link_no)
{
  char *name = diz_dir_get_name (diz, item);
  char *ret = string_link_no (name, link_no);
  free (name);
  return ret;
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
    mkdir (val, 0777);
  }
  return val;
}

static char *diz_dir_wiki_path (const char *wiki_title)
{
  char *path;
  char *hex="0123456789abcdef";
  unsigned char hash[40];
  unsigned char hash_hex[51];
  CtxSHA1 *sha1 = ctx_sha1_new ();
  ctx_sha1_process (sha1, (uint8_t*)wiki_title, strlen (wiki_title));
  ctx_sha1_done (sha1, hash);
  ctx_sha1_free (sha1);
  for (int j = 0; j < 20; j++)
  {
    hash_hex[j*2+0]=hex[hash[j]/16];
    hash_hex[j*2+1]=hex[hash[j]%16];
  }
  hash_hex[40]=0;
  path = ctx_strdup_printf ("%s/%s", ctx_basedir(), hash_hex);
  mkdir (path, 0777);
  return path;
}

int dir_visit_symlink (COMMAND_ARGS) /* "visit-symlink", 0, "", "" */
{
  char *target = diz_dir_get_string (diz, focused_no, "target");
  if (target)
  {
    set_location (target);
    free (target);
  }
  layout_focused_link = -1;
  return 0;
}

int dir_follow_link (COMMAND_ARGS) /* "follow-link", 0, "", "" */
{
  char *target = dir_item_link_no (focused_no, layout_focused_link);
  if (target)
  {
    set_location (target);
  }
  layout_focused_link = -1;
  return 0;
}

int cmd_edit_text_home (COMMAND_ARGS) /* "item-edit-text", 0, "", "edit start of line" */
{
  int virtual = (diz_dir_type_atom (diz, focused_no) == CTX_ATOM_TEXT);
  if (!virtual) return 0;

  text_edit = 0;
  return 0;
}

int cmd_edit_text_end (COMMAND_ARGS) /* "item-edit-text-end", 0, "", "edit end of line" */
{
  int virtual = (diz_dir_type_atom (diz, focused_no) == CTX_ATOM_TEXT);
  if (!virtual) return 0;
  char *name = diz_dir_get_name (diz, focused_no);
  int pos = 0;
  if (name)
  {
    pos = strlen (name);
    free (name);
  }
  text_edit = pos;
  return 0;
}

int cmd_activate (COMMAND_ARGS) /* "activate", 0, "", "activate item" */
{
  int no = focused_no;

  if (diz_dir_type_atom (diz, no) == CTX_ATOM_TEXT)
  {
    if (diz_dir_type_atom (diz, no+1) == CTX_ATOM_ENDGROUP||
        no >= diz_dir_count (diz)-1)
      argvs_eval ("item-edit-text-end");
    else
      argvs_eval ("item-edit-text");
    return 0;
  }

  char *full_path;
  char *name = diz_dir_get_name (diz, no);

  if (!strcmp (name, ".."))
  {
    argvs_eval ("go-parent");
    free (name);
    return 0;
  }
  else
  {
    full_path = ctx_strdup_printf ("%s/%s", diz->path, name);
  }
  const char *media_type = ctx_path_get_media_type (full_path); 
  int runnable = diz_dir_get_int (diz, no, "runnable", 0);

  if (runnable)
  {
    viewer_load_path (full_path, name);
  }
  else if (ctx_media_type_is_text (media_type))
  {
    set_location (full_path);
    itk->focus_no = 0;
  }
  else if (!strcmp(media_type, "inode/directory"))
  {
    set_location (full_path);
    itk->focus_no = 0;
  }
  else
  {
    viewer_load_path (full_path, name);
  }
  free (name);
  free (full_path);
  return 0;
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
      {
      float x = diz_dir_get_float (diz, focused_no, "x", -10000);
      if (x >=0)
      {
        x = diz_dir_get_float (diz, focused_no, "x", 0.0f);
        x += e->delta_x / ctx_width (e->ctx);
        diz_dir_set_float (diz, focused_no, "x", x);
        ctx_queue_draw (e->ctx);
      }
      float y = diz_dir_get_float (diz, focused_no, "y", -10000);
      if (y >=0)
      {
        y = diz_dir_get_float (diz, focused_no, "y", 0.0f);
        y += e->delta_y / ctx_width (e->ctx);
        diz_dir_set_float (diz, focused_no, "y", y);
        ctx_queue_draw (e->ctx);
      }
      e->stop_propagate = 1;
      }
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

void
ui_run_command (CtxEvent *event, void *data1, void *data2)
{
  char *commandline = data1;
  if (event)
    event->stop_propagate = 1;
  while (commandline && commandline[strlen(commandline)-1]==' ')
    commandline[strlen(commandline)-1]=0;

  ctx_queue_draw (event->ctx);
  argvs_eval (commandline);
}

CtxClient *viewer = NULL;
static char *viewer_loaded_path = NULL;
static char *viewer_loaded_name = NULL;
static int viewer_was_live = 0;

static void deactivate_viewer (CtxEvent *e, void *d1, void *d2)
{
#if 0
  if (viewer)
  {
    for (CtxList *c = clients; c; c = c?c->next:NULL)
    {
      if (c->data == viewer)
      {
        //int was_live = 1;
        //if (viewer_was_live)
        //{
          ctx_client_resize (ctx_client_id (viewer), 2, 2); // just to get it out of the way
        //}
        //else
        //{
          //ctx_client_remove (ctx, viewer);
        //}
        c = NULL;
      }
    }
    viewer = NULL;
  }
#else


  CtxList *to_remove = NULL;
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (ctx_client_flags (client) & ITK_CLIENT_PRELOAD)
    {
      ctx_list_prepend (&to_remove, client);
    }
  }
  while (to_remove)
  {
    ctx_client_remove (ctx, to_remove->data);
    ctx_list_remove (&to_remove, to_remove->data);
  }

  viewer = NULL;
#endif
  if (viewer_loaded_path)
          free (viewer_loaded_path);
  viewer_loaded_path = NULL;
  ctx_queue_draw (e->ctx);
}

int cmd_duplicate (COMMAND_ARGS) /* "item-duplicate", 0, "", "duplicate item" */
{
  int no = focused_no;
  int count = diz_dir_measure_chunk (diz, no);

  int insert_pos = no + count;
  for (int i = 0; i < count; i ++)
  {
    char *name = diz_dir_get_name (diz, no + i);
    diz_dir_insert (diz, insert_pos + i, name);
    free (name);
    int keys = diz_dir_key_count (diz, no + i);
    for (int k = 0; k < keys; k++)
    {
      char *key = diz_dir_key_name (diz, no + i, k);
      if (key)
      {
        char *val = diz_dir_get_string (diz, no + i, key);
        if (val)
        {
          diz_dir_set_string (diz, insert_pos + i, key, val);
          //itk->x += level * em * 3 + em;
          //itk_labelf (itk, "%s=%s", key, val);
          free (val);
        }
        free (key);
      }
    }
  }
  diz_dir_dirt (diz);
  return 0;
}

void dir_set_page (CtxEvent *event, void *a, void *b);
int cmd_remove (COMMAND_ARGS) /* "remove", 0, "", "remove item" */
{
  int no = focused_no;
  int count = diz_dir_measure_chunk (diz, no);

  //int virtual = (diz_dir_type_atom (diz, no) == CTX_ATOM_TEXT);
  //if (virtual)
  //
  DIR_DETAIL("items to remove %i", count);
  CtxAtom pre_atom = diz_dir_type_atom (diz, no-1);
  CtxAtom atom = diz_dir_type_atom (diz, no);
  CtxAtom post_atom = diz_dir_type_atom (diz, no+count);

  int was_last = 0;

  if (no + count >= diz->count) was_last = 1;

  for (int i = 0; i < count; i++)
  {
    diz_dir_remove (diz, no);
  }

#if 1
  if (pre_atom == CTX_ATOM_STARTGROUP &&
      post_atom == CTX_ATOM_ENDGROUP)
  {
    DIR_DETAIL("removed group");
    diz_dir_remove (diz, no-1);
    diz_dir_dirt(diz);
    diz_dir_remove (diz, no-1);
    diz_dir_dirt(diz);
    layout_find_item = no-2-was_last;
    itk->focus_no = -1;
  }
  else if (atom == CTX_ATOM_NEWPAGE)
  {
    layout_show_page --;
    layout_find_item = focused_no-1;
    itk->focus_no = -1;
  }
  else
  {
    if (was_last)
      layout_find_item = no - 1;
  }
#endif

  text_edit = TEXT_EDIT_OFF;
  diz_dir_dirt(diz);
  return 0;
}

void itk_focus (ITK *itk, int dir);

#if 0
static void move_after_next_sibling (CtxEvent *e, void *d1, void *d2)
{
  int no = (size_t)(d1);
  //char *full_path;
  //if (!strcmp (diz->items[no], ".."))dd
  if (no<diz->count-1)
  {
    diz_dir_swap (no, no+1);
    diz_dir_dirt(diz);
    ctx_queue_draw (e->ctx);
    itk_focus (itk, 1);
  }
}
#endif


static void grow_height (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float height = diz_dir_get_float (diz, no, "height", -100.0);
  height += 0.01;
  if (height > 1.2) height = 1.2;
  diz_dir_set_float (diz, no, "height", height);
  diz_dir_dirt (diz);
  ctx_queue_draw (e->ctx);
}

static void shrink_height (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float height = diz_dir_get_float (diz, no, "height", -100.0);
  height -= 0.01;
  if (height < 0.01) height = 0.01;
  diz_dir_set_float (diz, no, "height", height);
  diz_dir_dirt (diz);
  ctx_queue_draw (e->ctx);
}

static void grow_width (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float width = diz_dir_get_float (diz, no, "width", -100.0);
  width += 0.01;
  if (width > 1.5) width = 1.5;
  diz_dir_set_float (diz, no, "width", width);
  diz_dir_dirt (diz);
  ctx_queue_draw (e->ctx);
}

static void shrink_width (CtxEvent *e, void *d1, void *d2)
{
  int no = focused_no;
  float width = diz_dir_get_float (diz, no, "width", -100.0);
  width -= 0.01;
  if (width < 0.0) width = 0.01;
  diz_dir_set_float (diz, no, "width", width);
  diz_dir_dirt (diz);
  ctx_queue_draw (e->ctx);
}

int cmd_move (COMMAND_ARGS) /* "item-move-pos", 1, "<up|left|right|down>", "shift items position" */
{
  int no = focused_no;
  float x = diz_dir_get_float (diz, no, "x", -100.0);
  float y = diz_dir_get_float (diz, no, "y", -100.0);
  if (!strcmp (argv[1], "up"))
  {
    y -= 0.01;
    if (y< 0) y = 0.01;
    diz_dir_set_float (diz, no, "y", y);
    diz_dir_dirt (diz);
  }
  else if (!strcmp (argv[1], "left"))
  {
    x -= 0.01;
    if (x < 0.0) x = 0.0;
    diz_dir_set_float (diz, no, "x", x);
    diz_dir_dirt (diz);
  }
  else if (!strcmp (argv[1], "down"))
  {
    y += 0.01;
    if (y< 0) y = 0.01;
    diz_dir_set_float (diz, no, "y", y);
    diz_dir_dirt (diz);
  }
  else if (!strcmp (argv[1], "right"))
  {
    x += 0.01;
    if (x < 0) x = 0;
    diz_dir_set_float (diz, no, "x", x);
    diz_dir_dirt (diz);
  }
  return 0;
}

int move_after_next_sibling (COMMAND_ARGS) /* "move-after-next-sibling", 0, "", "" */
{
  int start_no    = focused_no;
  if (diz_dir_next_sibling (diz, focused_no) < 0)
  {
    return -1;
  }

  //for (int i = 0; i < count; i++)
  {
  int count = diz_dir_measure_chunk (diz, start_no);
  
  int start_level = 0;
  int level = 0;
  int did_skips = 0;
  int did_foo =0;

  if (diz_dir_type_atom (diz, focused_no + count) == CTX_ATOM_ENDGROUP)
  {
    return -1;
  }

  if (diz_dir_type_atom (diz, focused_no + count + 1) == CTX_ATOM_STARTGROUP &&
      diz_dir_type_atom (diz, focused_no + count) == CTX_ATOM_TEXT)
  {
    focused_no+=count;
    did_foo = 1;
  }

  focused_no++;
  int atom = diz_dir_type_atom (diz, focused_no);
  if (atom == CTX_ATOM_ENDGROUP)
  {
    focused_no--;
    return -1;
  }
  if (atom == CTX_ATOM_STARTGROUP)
     level++;

  while (level > start_level)
  {
    focused_no++;
    atom = diz_dir_type_atom (diz, focused_no);
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

// the global field gets both start/end times and x/y positions
// of vectors, within a clip local time applies similarily
//
// thus text needs to be extended to exist over multiple frames

  level++;
  if (diz_dir_type_atom (diz, focused_no) == CTX_ATOM_ENDGROUP)
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
      diz_dir_insert (diz, focused_no, "a");
      diz_dir_dirt (diz);
      diz_dir_swap (diz, focused_no, start_no);
      diz_dir_dirt (diz);
      diz_dir_remove (diz, start_no);
      diz_dir_dirt (diz);
    }
    focused_no-=count;
  }
  }

  layout_find_item = focused_no;
  if (itk)
    itk->focus_no = -1;

  return 0;
}

int move_before_previous_sibling (COMMAND_ARGS) /* "move-before-previous-sibling", 0, "", "" */
{
  int start_no    = focused_no;
  int level       = 0;
  int start_level = 0;
  int did_skips   = 0;

  if (diz_dir_prev_sibling (diz, focused_no) < 0)
  {
     return -1;
  }

  focused_no--;
  int atom = diz_dir_type_atom (diz, focused_no);
  if (atom == CTX_ATOM_ENDGROUP)
     level++;
  else if (atom == CTX_ATOM_STARTGROUP)
     level--;

  while (level > start_level)
  {
    focused_no--;
    atom = diz_dir_type_atom (diz, focused_no);
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
    if (did_skips) focused_no --;
    int count = diz_dir_measure_chunk (diz, start_no);
    //fprintf (stderr, "!%i %i %i!", start_no, focused_no, count);
    for (int i = 0; i < count; i ++)
    {
      diz_dir_insert (diz, focused_no + i , "b");
      diz_dir_swap (diz, start_no + i +1, focused_no+i);
      diz_dir_remove (diz, start_no+i +1);
    }
    //fprintf (stderr, "\n");
    //if (did_skips)
    //focused_no++;
    diz_dir_dirt (diz);
  }

  layout_find_item = focused_no;
  if (itk)
  itk->focus_no = -1;
  return 0;
}

int make_sibling_of_parent (COMMAND_ARGS) /* "make-sibling-of-parent", 0, "", "" */
{
  int no = focused_no;
  int level = diz_dir_item_get_level (diz, no);

  int count = diz_dir_measure_chunk (diz, no);

  if (level>0)
  {
    int target = no;
    int target_level;
    do {
      target++;
      target_level = diz_dir_item_get_level (diz, target);
    } while (target_level >= level);

    int remove_level = 0;
    
    if (diz_dir_type_atom (diz, no-1) == CTX_ATOM_STARTGROUP &&
        diz_dir_type_atom (diz, no+count) == CTX_ATOM_ENDGROUP)
      remove_level = 1;

    for (int i = 0; i < count; i ++)
    {
      diz_dir_insert (diz, target+1+i, "c");
      diz_dir_swap (diz, no+i, target+1+i);
    }

    if (remove_level)
    {
      for (int i = 0; i < count + 2; i++)
        diz_dir_remove (diz, no-1);
      layout_find_item = no - 1;
    }
    else
    {
      for (int i = 0; i < count; i++)
        diz_dir_remove (diz, no);
      layout_find_item = target - count;
    }

    itk->focus_no = -1;

    diz_dir_dirt (diz);
  }
  return 0;
}

int make_child_of_previous (COMMAND_ARGS) /* "make-child-of-previous", 0, "", "" */
{
  int no = focused_no;
  if (no < 1)
    return -1;

  int prev_no = diz_dir_prev_sibling (diz, no);
  int was_folded = 0;
  if (diz_dir_has_children (diz, prev_no)
      && diz_dir_get_int (diz, prev_no, "folded", 0))
  {
      diz_dir_unset (diz, prev_no, "folded");
      was_folded = 1;
  }

  {
    int count = diz_dir_measure_chunk (diz, no);
    int target = no-1;
    int atom = diz_dir_type_atom (diz, target);
    if (atom == CTX_ATOM_STARTGROUP)
       return -1;
    if (atom == CTX_ATOM_ENDGROUP)
    {
      for (int i = 0; i < count; i++)
      {
        diz_dir_insert (diz, target+i, "d");
        diz_dir_swap (diz, no+1+i, target+i);
        diz_dir_remove (diz, no+1+i);
      }
    }
    else
    {
      // insert new group
      target = no;
      diz_dir_insert (diz, target, "");
      diz_dir_set_string (diz, target, "type", "endgroup");
      for (int i = 0; i <count;i++)
      diz_dir_insert (diz, target, "f");

      diz_dir_insert (diz, target, "");
      diz_dir_set_string (diz, target, "type", "startgroup");

      for (int i = 0; i < count; i++)
      {
        diz_dir_swap (diz, no+count+2, target+1+i);
        diz_dir_remove (diz, no+count+2);
      }
    }

#if 0
    layout_find_item = target;
    itk->focus_no = -1;
#else
    //itk->focus_no -= (target-focused_no);
    focused_no = target;

    if (was_folded)
    {
      int temp = focused_no + 1;
      while (temp > 0)
      {
        focused_no = temp;
        temp = diz_dir_next_sibling (diz, temp);
      }
      layout_find_item = target;
      itk->focus_no = -1;
    }
    //layout_find_item = focused_no;
#endif

    diz_dir_dirt (diz);
  }
  return 0;
}

int item_cycle_bullet(COMMAND_ARGS) /* "cycle-bullet", 0, "", "" */
{
  int bullet = diz_dir_get_int (diz, focused_no, "bullet", CTX_BULLET_NONE);
  switch (bullet)
  {
    case CTX_BULLET_NONE:
      diz_dir_set_float (diz, focused_no, "bullet", CTX_BULLET_BULLET);
      break;
    case CTX_BULLET_BULLET:
      diz_dir_set_float (diz, focused_no, "bullet", CTX_BULLET_NUMBERS);
      break;
    case CTX_BULLET_NUMBERS:
      diz_dir_set_float (diz, focused_no, "bullet", CTX_BULLET_TODO);
      break;
    case CTX_BULLET_TODO:
      diz_dir_set_float (diz, focused_no, "bullet", CTX_BULLET_DONE);
      break;
    case CTX_BULLET_DONE:
      diz_dir_unset (diz, focused_no, "bullet");
      break;
  }
  diz_dir_dirt (diz);
  return 0;
}

int item_cycle_heading (COMMAND_ARGS) /* "cycle-heading", 0, "", "" */
{
  char *klass = diz_dir_get_string (diz, focused_no, "tag");

  if (!klass) klass = strdup ("");

  if (!strcmp (klass, "h1"))
    diz_dir_set_string (diz, focused_no, "tag", "h2");
  else if (!strcmp (klass, "h2"))
    diz_dir_set_string (diz, focused_no, "tag", "h3");
  else if (!strcmp (klass, "h3"))
    diz_dir_set_string (diz, focused_no, "tag", "h4");
  else if (!strcmp (klass, "h4"))
    diz_dir_set_string (diz, focused_no, "tag", "h5");
  else if (!strcmp (klass, "h5"))
    diz_dir_set_string (diz, focused_no, "tag", "h6");
  else if (!strcmp (klass, "h6"))
    diz_dir_unset (diz, focused_no, "tag");
  else
    diz_dir_set_string (diz, focused_no, "tag", "h1");
  diz_dir_dirt (diz);
  return 0;
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

static void draw_img (ITK *itk, float x, float y, float w, float h, const char *path, int fit)
{
  Ctx *ctx = itk->ctx;
  float em = itk_em (itk);
  int imgw, imgh;
    float target_width = 
      (w - (layout_config.padding_left + layout_config.padding_right) * em );
    float target_height = 
      (h - (layout_config.padding_top + layout_config.padding_bottom) * em );
  //ctx_begin_path (ctx);

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
      float scale_w = (target_width)/imgw;
      float scale_h = (target_height)/imgh;
      if (fit)
      {
        if (scale_w < scale_h)
        {
          scale_h = scale_w;
        }
        else
        {
          scale_w = scale_h;
          ctx_translate (ctx, 
                   (target_width - imgw * scale_w)/2, 0.0);
        }
      }
      ctx_scale (ctx, scale_w, scale_h);

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
    struct stat thumb_stat;
    struct stat path_stat;

    lstat (path, &path_stat);
    lstat (thumb_path, &thumb_stat);

    if (thumb_stat.st_mtime <
        path_stat.st_mtime)
    {
      //fprintf (stderr, "requeing %s\n", path);
      ctx_drop_eid (ctx, thumb_path);
      unlink (thumb_path);
      queue_thumb (path, thumb_path);
    }
    else
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
      float scale_w = (target_width)/imgw;
      float scale_h = (target_height)/imgh;
      if (fit)
      {
        if (scale_w < scale_h)
        {
          scale_h = scale_w;
        }
        else
        {
          scale_w = scale_h;
          ctx_translate (ctx, 
                   (target_width - imgw * scale_w)/2, 0.0);
        }
      }
      ctx_scale (ctx, scale_w, scale_h);
      ctx_texture (ctx, reteid, 0,0);
      ctx_fill (ctx);
     #endif
      free (thumb_path);
    }
      ctx_restore (ctx);
    return;
    }
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

int viewer_load_next (Ctx *ctx, void *data1)
{
  if (focused_no+1 >= diz_dir_count(diz))
    return 0;
  if (viewer && viewer_media_type && viewer_media_type[0]=='v')
  {
    ctx_client_feed_keystring (viewer, NULL, "space");
  }

  focused_no++;
  layout_find_item = focused_no;
  argvs_eval ("activate");
  ctx_queue_draw (ctx);
  return 0;
}

void ctx_client_lock (CtxClient *client);
void ctx_client_unlock (CtxClient *client);

static void dir_handle_event (Ctx *ctx, CtxEvent *ctx_event, const char *event)
{
  if(stuff_stdout_is_running())
  {
    CtxClient *client = ctx_client_find (ctx, "stdout");
    if (!client)
      return;
    ctx_client_lock (client);
    ctx_client_feed_keystring (client, ctx_event, event);
    ctx_client_unlock (client);
    return;
  }

  if (!viewer)
    return;
  CtxClient *client = viewer;
  ctx_client_lock (client);
  int media_class = ctx_media_type_class (viewer_media_type);

  if (!strcmp (event, "control-escape") ||
      !strcmp (event, "alt-up"))
  {
    ctx_client_unlock (client);
    deactivate_viewer (ctx_event, NULL, NULL);
    return;
  }

#if 1
  if ((media_class == CTX_MEDIA_TYPE_IMAGE || 
       media_class == CTX_MEDIA_TYPE_VIDEO)&& (!strcmp (event, "space")))
  {
    ctx_client_unlock (client);
    viewer_slideshow = !viewer_slideshow;
    ctx_client_feed_keystring (client, ctx_event, event);
    ctx_event->stop_propagate=1;
    return;
  }
#endif

  if ((media_class == CTX_MEDIA_TYPE_IMAGE || 
      media_class  == CTX_MEDIA_TYPE_VIDEO) && (!strcmp (event, "page-down")))
  {
    ctx_client_unlock (client);
#if 1
    viewer_load_next (ctx_event->ctx, NULL);
#else
    focused_no++;
    layout_find_item = focused_no;
    argvs_eval ("activate");
    ctx_queue_draw (ctx);
#endif
    ctx_event->stop_propagate = 1;

    return;
  }
  if ((media_class == CTX_MEDIA_TYPE_IMAGE || 
      media_class == CTX_MEDIA_TYPE_VIDEO)&& (!strcmp (event, "page-up")))
  {
    ctx_client_unlock (client);
    focused_no --;
    layout_find_item = focused_no;
    argvs_eval ("activate");
    ctx_queue_draw (ctx);
    ctx_event->stop_propagate = 1;

    return;
  }
  else
  {
    ctx_client_feed_keystring (client, ctx_event, event);
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
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void dir_revert (CtxEvent *event, void *data1, void *data2)
{
  // XXX: NYI
  diz_dir_dirt(diz);
}

int editing_location = 0;
static void dir_location_escape (CtxEvent *e, void *d1, void *d2);

static void dir_select_item (CtxEvent *event, void *data1, void *data2)
{
   if ((size_t)data1 == (size_t)-1)
   {
     focused_no = -1;
     itk->focus_no = 0;
   }
   else
   {
     itk->focus_no = (size_t)data1;
     layout_find_item = -1;
   }
   if (editing_location)
   {
     dir_location_escape (event, NULL, NULL);
   } 
   itk_entry_commit (itk);
   ctx_queue_draw (event->ctx);
   event->stop_propagate = 1;
}

/*
 * this is the core text layout with linebreaking work function it is used
 * in two places and we do not want to repeat the logic as it would quickly
 * go out of sync leading to bad bugs.
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
  if (d_name == NULL)d_name="";
  char word[1024]="";
  int wlen = 0;
  const char *p;
  int pos = 0;
  float x0 = x;
  int link_no = 0;
  int was_no = 0;
  int cursor_drawn = 0;
  int compute_neighbor_lines = 0;
  if (text_editor) visible_markup=1;
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

      if (word_is_date (word))
      {
         was_in_link=1;
         if (is_focused && layout_focused_link == link_no) was_in_link++;
         link_no++;//fprintf (stderr, "[%s]",word);
      }

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
                    cursor_x-1, y - line_height * 0.8f,
                    text_editor?space_width:2, line_height*0.9f);
          ctx_fill (itk->ctx);
          ctx_restore (ctx);
      }
       cursor_drawn = 1;
        }
      if (print)
      {
        if (was_in_link   && !text_editor )
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
    if (was_in_link && is_focused)
       was_in_link += (link_no == layout_focused_link);
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
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void text_edit_return (CtxEvent *event, void *a, void *b)
{
  char *str = diz_dir_get_name (diz, focused_no);
  diz_dir_insert (diz, focused_no+1, str + text_edit);
  str[text_edit]=0;
  diz_dir_set_name (diz, focused_no, str);

  if (diz_dir_get_int (diz, focused_no, "bullet", 0))
  {
    diz_dir_set_float (diz, focused_no+1, "bullet",
                      diz_dir_get_int (diz, focused_no, "bullet", 0));
  }

  diz_dir_dirt (diz);
  free (str);

  ctx_queue_draw (event->ctx);
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
      char *name = diz_dir_get_name (diz, focused_no);
      CtxString *str = ctx_string_new (name);
      free (name);
      ctx_string_remove (str, text_edit-1);
      diz_dir_set_name (diz, focused_no, str->str);
      ctx_string_free (str, 1);
      text_edit--;
      diz_dir_dirt (diz);
    }
    else if (text_edit == 0 && focused_no > 0 &&
            (diz_dir_type_atom (diz, focused_no-1) == CTX_ATOM_TEXT)
            )
    {
      char *name_prev = diz_dir_get_name (diz, focused_no-1);
      char *name = diz_dir_get_name (diz, focused_no);
      CtxString *str = ctx_string_new (name_prev);
      text_edit = strlen (str->str);
      ctx_string_append_str (str, name);
      free (name);
      free (name_prev);
      diz_dir_set_name (diz, focused_no-1, str->str);
      ctx_string_free (str, 1);
      diz_dir_remove (diz, focused_no);
      focused_no--;
      itk->focus_no--;
      diz_dir_dirt (diz);
    }
  }
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}


void text_edit_delete (CtxEvent *event, void *a, void *b)
{
  if (focused_no>=0){

    char *name = diz_dir_get_name (diz, focused_no);
    CtxString *str = ctx_string_new (name);
    free (name);
    if (text_edit == (int)strlen(str->str))
    {
      if (diz_dir_type_atom (diz, focused_no+1) == CTX_ATOM_TEXT)
      {
        char *name_next = diz_dir_get_name (diz, focused_no+1);
        ctx_string_append_str (str, name_next);
        free (name_next);
        diz_dir_remove (diz, focused_no+1);
      }
    }
    else
    {
      ctx_string_remove (str, text_edit);
    }
    diz_dir_set_name (diz, focused_no, str->str);
    ctx_string_free (str, 1);
    diz_dir_dirt (diz);
  }
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

int dir_join (COMMAND_ARGS) /* "join-with-next", 0, "", "" */
{
  if (focused_no>=0){
    char *name = diz_dir_get_name (diz, focused_no);
    CtxString *str = ctx_string_new (name);
    free (name);
    if (diz_dir_type_atom (diz, focused_no+1) == CTX_ATOM_TEXT)
    {
      char *name_next = diz_dir_get_name (diz, focused_no+1);
      if (name[strlen(name)-1]!=' ')
        ctx_string_append_str (str, " ");
      ctx_string_append_str (str, name_next);
      free (name_next);
      diz_dir_remove (diz, focused_no+1);
    }
    diz_dir_set_name (diz, focused_no, str->str);
    ctx_string_free (str, 1);
    diz_dir_dirt (diz);
  }
  return 0;
}

void text_edit_shift_return (CtxEvent *event, void *a, void *b)
{
  if (focused_no>=0){
    const char *insertedA = "\\";
    const char *insertedB = "n";
    char *name = diz_dir_get_name (diz, focused_no);
    CtxString *str = ctx_string_new (name);
    free (name);
    ctx_string_insert_utf8 (str, text_edit, insertedB);
    ctx_string_insert_utf8 (str, text_edit, insertedA);
    diz_dir_set_name (diz, focused_no, str->str);
    ctx_string_free (str, 1);
    text_edit++;
  }
  diz_dir_dirt (diz);
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void ignore_and_stop_propagate (CtxEvent *event, void *a, void *b)
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
    char *name = diz_dir_get_name (diz, focused_no);
    CtxString *str = ctx_string_new (name);
    free (name);
    ctx_string_insert_utf8 (str, text_edit, inserted);
    diz_dir_set_name (diz, focused_no, str->str);
    ctx_string_free (str, 1);
    text_edit++;
  }
  diz_dir_dirt (diz);
  //save_metadata();
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void text_edit_right (CtxEvent *event, void *a, void *b)
{
  if (focused_no<0)
  {
    event->stop_propagate = 1;
    return;
  }
  char *name = diz_dir_get_name (diz, focused_no);
  int len = ctx_utf8_strlen (name);
  free (name);
  text_edit_desired_x = -100;

  text_edit ++;

  if (text_edit>len)
  {
    if (focused_no + 1 < diz->count &&
        diz_dir_type_atom (diz, focused_no+1) == CTX_ATOM_TEXT)
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
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void text_edit_left (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;

  text_edit--;

  if (focused_no < 0)
  {
     event->stop_propagate = 1;
     return;
  }

  if (text_edit < 0)
  {
    char *name = diz_dir_get_name (diz, focused_no);
    if (name[0]==0 &&
        diz_dir_type_atom (diz, focused_no-1) == CTX_ATOM_STARTGROUP &&
        diz_dir_type_atom (diz, focused_no+1) == CTX_ATOM_ENDGROUP)
    {
      text_edit = 0;
      argvs_eval ("remove");

      //layout_find_item = focused_no-1;
      //itk->focus_no = -1;
      diz_dir_dirt(diz);
    }
    else if (focused_no>0 && 
             diz_dir_type_atom (diz, focused_no-1) == CTX_ATOM_TEXT)

    {
      char *name = diz_dir_get_name (diz, focused_no-1);
      text_edit=ctx_utf8_strlen(name);
      free (name);
      layout_find_item = focused_no -1;
      itk->focus_no = -1;
    }
    else
    {
      text_edit = 0;
    }
    free (name);
  }
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void text_edit_control_left (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;

  if (text_edit == 0)
  {
    if (diz_dir_type_atom (diz, focused_no-1) == CTX_ATOM_TEXT)
    {
      char *name = diz_dir_get_name (diz, focused_no-1);
      text_edit=ctx_utf8_strlen(name);
      free (name);
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
    char *text = diz_dir_get_name (diz, focused_no); 
    if (text_edit >0 && text[text_edit-1]==' ')  // XXX should be utf8 aware
      text_edit--;
    while (text_edit>0 && text[text_edit-1]!=' ') 
      text_edit--;
    free (text);
  }

  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void text_edit_control_right (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;

  {
    char *text = diz_dir_get_name (diz, focused_no); 
    if (text[text_edit]==' ')  // XXX should be utf8 aware
      text_edit++;
    while (text[text_edit] && text[text_edit]!=' ') 
      text_edit++;
    free (text);
  }

  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void text_edit_home (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;
  text_edit = 0;
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

void text_edit_end (CtxEvent *event, void *a, void *b)
{
  text_edit_desired_x = -100;
  char *name = diz_dir_get_name (diz, focused_no);
  text_edit = ctx_utf8_strlen (name);
  if (event)
  {
    ctx_queue_draw (event->ctx);
    event->stop_propagate=1;
  }
}

int prev_line_pos = 0;
int next_line_pos = 0;
int was_editing_before_tail = 0;

void text_edit_up (CtxEvent *event, void *a, void *b)
{
  if (focused_no < 0){
    event->stop_propagate=1;
    return;
  }

  if (prev_line_pos >= 0)
  {
    text_edit = prev_line_pos;
    ctx_queue_draw (event->ctx);
    event->stop_propagate=1;
    return;
  }

#if 0
  if (diz_dir_type_atom (diz, focused_no-1) != CTX_ATOM_TEXT && 
      diz_dir_type_atom (diz, focused_no-1) != CTX_ATOM_ENDGROUP)
  {
    event->stop_propagate=1;
    return;
  }
  //text_edit=strlen(diz->items[focused_no-1]);
  //
  //
  char *name = diz_dir_get_name (diz, focused_no);

  if (name[0]==0 &&
      (focused_no+1 >= diz->count ||
      diz_dir_type_atom (diz, focused_no+1) == CTX_ATOM_ENDGROUP))
    {
      int next_focus = dir_prev_sibling (diz, focused_no);
      argvs_eval ("remove");
      if (was_editing_before_tail)
      {
        text_edit = TEXT_EDIT_FIND_CURSOR_LAST_ROW;
      }
      else
        text_edit = TEXT_EDIT_OFF;
      focused_no = next_focus+1;
      diz_dir_dirt(diz);
  free (name);
    }
  else
#else
    focused_no--;
    while (diz_dir_type_atom (diz, focused_no) != CTX_ATOM_TEXT)
      focused_no --;
#endif
    {
      text_edit = TEXT_EDIT_FIND_CURSOR_LAST_ROW;
    }
  layout_find_item = focused_no;
  itk->focus_no = -1;

  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
}

int dir_zoom (COMMAND_ARGS) /* "zoom", 1, "<in|out|val>", "" */
{
  if (!strcmp (argv[1], "in"))
    dir_scale *= 1.1f;
  else if (!strcmp (argv[1], "out"))
    dir_scale /= 1.1f;
  else {
    dir_scale = atof (argv[1]);
    if (dir_scale <= 0.001) dir_scale = 1.0f;
  }
  //drop_item_renderers (itk->ctx);
  return 0;
}

void dir_font_up (CtxEvent *event, void *a, void *b)
{
  itk->font_size *= 1.05f;
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
  //drop_item_renderers (itk->ctx);
}

void dir_font_down (CtxEvent *event, void *a, void *b)
{
  itk->font_size /= 1.05f;
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
  //drop_item_renderers (itk->ctx);
}

int dir_font_size (COMMAND_ARGS) /* "font-size", 1, "<up|down|val>", "" */
{
  if (!strcmp (argv[1], "up"))
    itk->font_size *= 1.05f;
  else if (!strcmp (argv[1], "down"))
    itk->font_size /= 1.05f;
  else
    itk->font_size = atof (argv[1]);
  return 0;
}

void dir_set_page (CtxEvent *event, void *a, void *b)
{
  layout_show_page = (size_t)(a);
  if (layout_show_page < 0)
    layout_show_page = 0;
  if (layout_last_page < layout_show_page)
    layout_show_page = layout_last_page;
  itk->focus_no = 0;
  if (event)
  {
    ctx_queue_draw (event->ctx);
    event->stop_propagate=1;
  }
}

int cmd_page (COMMAND_ARGS) /* "page", 1, "<next|prev|previous|integer>", "" */
{
  int target = layout_show_page;
  if (!strcmp (argv[1], "next"))
  {
    target++;
  }
  else if ((!strcmp (argv[1], "prev")) || (!strcmp (argv[1], "previous")))
  {
    target--;
  }
  else
  {
    target = atoi(argv[1]);
  }
  dir_set_page (NULL, (void*)(size_t)(target), NULL);
  return 0;
}

int item_context_active = 0;
int item_context_choice = 0;

static void
make_tail_entry (Diz *diz)
{
  if (focused_no == -1)
  {
    diz_dir_insert (diz, 0, "_");
    focused_no = 0;
    layout_find_item = focused_no = focused_no;
  }
  else
  {
  diz_dir_insert (diz, focused_no+diz_dir_measure_chunk(diz,focused_no), "");
  layout_find_item = focused_no = diz_dir_next_sibling (diz, focused_no);
  }
  diz_dir_dirt (diz);
  itk->focus_no = -1;
  was_editing_before_tail = (text_edit != TEXT_EDIT_OFF);
  text_edit = 0;
}


void text_edit_down (CtxEvent *event, void *a, void *b)
{
  ctx_queue_draw (event->ctx);
  event->stop_propagate=1;
  if (next_line_pos >= 0)
  {
    text_edit = next_line_pos;
    return;
  }

#if 0
  if (diz_dir_type_atom (diz, focused_no+1) != CTX_ATOM_TEXT)
  {
    char *str = diz_dir_get_name (diz, focused_no);
    if (str[0])
      make_tail_entry (diz);
    free (str);
    return;
  }
#else
  focused_no ++;
  while (diz_dir_type_atom (diz, focused_no) != CTX_ATOM_TEXT)
    focused_no ++;
#endif
  text_edit = TEXT_EDIT_FIND_CURSOR_FIRST_ROW;

  layout_find_item = focused_no;
  itk->focus_no = -1;
  // -- - //
}

int focus_next_link (COMMAND_ARGS) /* "focus-next-link", 0, "", "" */
{
  layout_focused_link ++;
  return 0;
}

int focus_previous_link (COMMAND_ARGS) /* "focus-previous-link", 0, "", "" */
{
  layout_focused_link --;
  return 0;
}

int focus_next_sibling (COMMAND_ARGS) /* "focus-next-sibling", 0, "", "" */
{
  layout_focused_link = -1;
  if (diz_dir_next_sibling (diz, focused_no) < 0)
  {
    make_tail_entry (diz);
    return -1;
  }

  layout_find_item = focused_no = diz_dir_next_sibling (diz, focused_no);
  itk->focus_no = -1;
  return 0;
}

static int item_get_list_index (Diz *diz, int i)
{
  int pos = i;
  int count = 0;

  while (pos >= 0 &&
         diz_dir_get_int (diz, pos, "bullet", CTX_BULLET_NONE) == CTX_BULLET_NUMBERS)
  {
     pos = diz_dir_prev_sibling (diz, pos);
     count++;
  }

  return count;
}

int focus_no (COMMAND_ARGS) /* "focus-no", 1, "", "" */
{
  layout_focused_link = -1;
  focused_no = atoi(argv[1]);
  return 0;
}


int focus_next (COMMAND_ARGS) /* "focus-next", 0, "", "" */
{
  layout_focused_link = -1;
  int pos = diz_dir_next (diz, focused_no);
  if (pos >= 0)
  {
    layout_find_item = focused_no = pos;
    itk->focus_no = -1;
  }
  return 0;
}

int focus_previous (COMMAND_ARGS) /* "focus-previous", 0, "", "" */
{
  layout_focused_link = -1;
  int pos = diz_dir_prev (diz, focused_no);
  if (pos >= 0)
  {
    layout_find_item = focused_no = pos;
    itk->focus_no = -1;
  }
  return 0;
}

int focus_previous_sibling (COMMAND_ARGS) /* "focus-previous-sibling", 0, "", "" */
{
  layout_focused_link = -1;
  int pos = diz_dir_prev_sibling (diz, focused_no);
  if (pos >= 0)
  {
    layout_find_item = focused_no = pos;
    itk->focus_no = -1;
  }
  return 0;
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
       diz_dir_insert (diz, focused_no, "");
       diz_dir_set_string (diz, focused_no, "type", "rectangle");
       x0 = e->x;
       y0 = e->y;
       diz_dir_set_float (diz, focused_no, "x", x0 / itk->width);
       diz_dir_set_float (diz, focused_no, "y", y0 / itk->width);
       diz_dir_dirt (diz);
       break;
    case CTX_DRAG_MOTION:

       if (e->x > x0)
       {
         diz_dir_set_float (diz, focused_no, "x", x0 / itk->width);
         diz_dir_set_float (diz, focused_no, "width", (e->x-x0) / itk->width);
       }
       else
       {
         diz_dir_set_float (diz, focused_no, "x", e->x / itk->width);
         diz_dir_set_float (diz, focused_no, "width", (x0-e->x) / itk->width);
       }

       if (e->y > y0)
       {
         diz_dir_set_float (diz, focused_no, "y", y0 / itk->width);
         diz_dir_set_float (diz, focused_no, "height", (e->y-y0) / itk->width);
       }
       else
       {
         diz_dir_set_float (diz, focused_no, "y", e->y / itk->width);
         diz_dir_set_float (diz, focused_no, "height", (y0-e->y) / itk->width);
       }

       diz_dir_dirt (diz);
       ctx_queue_draw (e->ctx);
       break;
    case CTX_DRAG_RELEASE:
       diz_dir_dirt (diz);
       break;
    default:
       break;
  }
  e->stop_propagate = 1;
}

static ITKPanel *panel = NULL;
static void dir_location (CtxEvent *e, void *d1, void *d2)
{
  editing_location = 1;

  ctx_string_set (commandline, diz->title?diz->title:diz->path);
  commandline_cursor_end = 0;
  commandline_cursor_start = strlen (commandline->str);
  //if (e)
  {
      if (itk->focus_label)
    free (itk->focus_label);
    itk->focus_label = NULL;

    itk_panels_reset_scroll (itk);
    itk->focus_no = 0;
    focused_no = -1;
    layout_find_item = focused_no;
    if(e)
    e->stop_propagate = 1;
    ctx_queue_draw (itk->ctx);
  }
}

int edit_location (COMMAND_ARGS) /* "location-edit", 0, "", "" */
{
   dir_location (NULL, NULL, NULL);
   return 0;
}

int cmd_quit (COMMAND_ARGS) /* "quit", 0, "", "" */
{
   ctx_quit (ctx);
   return 0;
}

static void dir_location_escape (CtxEvent *e, void *d1, void *d2)
{
  editing_location = 0;
  commandline_cursor_end =
  commandline_cursor_start = 0;
  ctx_string_set (commandline, "");
  if (e)
  {
    e->stop_propagate = 1;
    ctx_queue_draw (e->ctx);
  }
}

int dir_parent (COMMAND_ARGS) /* "outline-focus-parent", 0, "", "" */
{
  layout_focused_link = -1;
  int start_no = focused_no;
  
  int level = 1;

  CtxAtom atom;
  while (level > 0 && focused_no >= 0)
  {
    focused_no--;
    atom = diz_dir_type_atom (diz, focused_no);
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

  if (diz_dir_get_int (diz, focused_no-1, "was-folded", 0))
  {
    diz_dir_set_float (diz, focused_no-1, "folded", 1.0);
    diz_dir_unset (diz, focused_no-1, "was-folded");
  }
  focused_no--;
  if (focused_no < 0)
  {
     focused_no = start_no;
  }

  layout_find_item = focused_no;
  itk->focus_no = -1;

  return 0;
}

int dir_set (COMMAND_ARGS) /* "set", 2, "<key> <value>", "" */
{
  diz_dir_set_string (diz, focused_no, argv[1], argv[2]);
  return 0;
}

int dir_unset (COMMAND_ARGS) /* "unset", 1, "<key>", "unset the specified key on focused item" */
{
  diz_dir_unset (diz, focused_no, argv[1]);
  if (!strcmp (argv[1], "live"))
     drop_item_renderers (ctx);
  return 0;
}

int dir_set_name (COMMAND_ARGS) /* "set-name", 2, "<no> <name>", "" */
{
  diz_dir_set_name (diz, atoi(argv[1]), argv[2]);
  return 0;
}

int dir_load (COMMAND_ARGS) /* "load-path", 1, "<path>", "" */
{
  diz_dir_set_path (diz, argv[1]);
  return 0;
}


char *dir_exec_command (const char *commandline){
  FILE *fp;
  char path[1024];
  CtxString *str = ctx_string_new ("");

  /* Open the command for reading. */
  chdir (diz->path);
  fp = popen(commandline, "r");
  if (fp == NULL) {
    ctx_string_set (str, "Failed to run command\n");
    return ctx_string_dissolve (str);
  }

  while (fgets(path, sizeof(path), fp) != NULL) {
    ctx_string_append_str (str, path);
  }

  pclose(fp);

  return ctx_string_dissolve (str);
}


int run_command (COMMAND_ARGS) /* "run-command", 0, "", "" */
{
   char *commandline = diz_dir_get_name (diz, focused_no);
   
   diz_dir_remove_children (diz, focused_no);
   diz_dir_unset (diz, focused_no, "folded");


     diz_dir_insert(diz, focused_no+1, "");
     diz_dir_set_string (diz, focused_no+1, "type", "startgroup");

     char *full_commandline = ctx_strdup_printf ("%s %s", commandline, " 2>&1");
     char *result;
     if (commandline[0]=='$')
       result = dir_exec_command(full_commandline+1);
     else
       result = dir_exec_command(full_commandline);
     free (full_commandline);
     if (result[strlen(result)-1]=='\n')
       result[strlen(result)-1]=0;
     diz_dir_insert(diz, focused_no+2, result);
     free (result);

     diz_dir_insert(diz, focused_no+3, "");
     diz_dir_set_string (diz, focused_no+3, "type", "endgroup");
     diz_dir_dirt (diz);
   free (commandline);
   return 0;
}

int dir_dump (COMMAND_ARGS) /* "dump", 0, "", "" */
{
   diz_dir_dump (diz);
   return 0;
}

int stuffcmd_main (int argc, char **argv)
{
  diz = diz_dir_new ();
  char *script = NULL;
  long length = 0;
  char *rp = realpath (argv[1], NULL);
  if (rp)
  {
    ctx_get_contents (rp, (uint8_t**)&script, &length);
    if (script)
    {
      char *startline = script;
      char *endline = strchr(script, '\n');
      while (endline && *endline)
      {
        *endline = '\0';
        fprintf (stdout, "%s\n", startline);
        argvs_eval (startline);
        startline = endline + 1;
        endline = strchr (startline, '\n');
      }
      free (script);
    }
    free (rp);
  }
  return 0;
}

int stuff_realpath_main (int argc, char **argv)
{
  const char *category = argv[1];
  if (category)
  {
    if (category[0] == '/' || category[0] == '.')
      fprintf (stdout, "%s\n", realpath(category, NULL));
    else
      fprintf (stdout, "%s\n", diz_dir_wiki_path (category));
    return 0;
  }
  return 1;
}

int stuff_ls_main (int argc, char **argv)
{
  const char *path = ".";
  if (argv[1]) path = argv[1];
  Diz *diz = diz_dir_new ();
  diz_dir_set_path (diz, path);
  diz_dir_dump (diz);
  return 1;
}

int dir_enter_children (COMMAND_ARGS) /* "outline-focus-first-child", 0, "", "" */
{
  int start_no = focused_no;
  layout_focused_link = -1;
  
  focused_no++;
  CtxAtom  atom = diz_dir_type_atom (diz, focused_no);

  if (atom == CTX_ATOM_STARTGROUP)
  {
    if (diz_dir_get_int (diz, focused_no-1, "folded", 0))
    {
      diz_dir_set_float (diz, focused_no-1, "was-folded", 1);
      diz_dir_set_float (diz, focused_no-1, "folded", 0);
    }
    focused_no++;
  }
  else
  {
     focused_no = start_no;

     diz_dir_insert(diz, focused_no+1, "");
     diz_dir_set_string (diz, focused_no+1, "type", "startgroup");

     diz_dir_insert(diz, focused_no+2, "");
     text_edit = 0;

     diz_dir_insert(diz, focused_no+3, "");
     diz_dir_set_string (diz, focused_no+3, "type", "endgroup");

     focused_no++;
  }
  diz_dir_dirt(diz);

  layout_find_item = focused_no;
  itk->focus_no = -1;
  return 0;
}

int cmd_show_context (COMMAND_ARGS) /* "context-menu-show", 0, "", "" */
{
  item_context_active = 1;
  item_context_choice = 0;
  return 0;
}

int cmd_hide_context (COMMAND_ARGS) /* "context-menu-hide", 0, "", "" */
{
  item_context_active = 0;
  item_context_choice = 0;
  return 0;
}


int cmd_toggle_context (COMMAND_ARGS) /* "toggle-context", 0, "", "" */
{
  if (item_context_active)
  {
    item_context_active = 0;
  }
  else
  {
    item_context_active = 1;
    item_context_choice = 0;
  }
  return 0;
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


static void escape_path (const char *path, char *escaped_path)
{
    int j = 0;
    for (int i = 0; path[i]; i++)
    {
      switch (path[i])
      {
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
}

static char **ctx_str_list_to_pasv (CtxList **list)
{
  int count = ctx_list_length (*list);
  int size = 0;

  char **ret;
  char  *str;

  for (CtxList *l = *list; l; l=l->next)
  {
    char *data = l->data;
    size +=  sizeof(char*);
    size +=  strlen (data);
    size +=  1;
  }
  size += sizeof (void*); // NULL

  int no = 0;
  ret = calloc (1, size);

  str = ((char*)ret)+(count + 1) * sizeof (void*);

  while (*list)
  {
    char *data = (*list)->data;
    ret[no] = str;
    strcpy (ret[no++], data);
    str += strlen (data) + 1;
    ctx_list_remove (list, data);
    free (data);
  }
  return ret;
}

char **dir_get_viewer_argv (const char *path, int no)
{
  CtxList *args = NULL;
  const char *media_type = ctx_path_get_media_type (path);
  CtxMediaTypeClass media_type_class = ctx_media_type_class (media_type);
  char *escaped_path = malloc (strlen (path) * 2 + 20);
  escape_path (path, escaped_path);
  float in = diz_dir_get_float (diz, no, "in", 0.0f);
  //float out = diz_dir_get_float (diz, no, "out", 0.0f);
  int runnable = diz_dir_get_int (diz, no, "runnable", 0);
  if (!strcmp (media_type, "inode/directory"))
  {
    //fprintf (stderr, "is dir\n");
    return NULL;
  }

  if (!args)
  if (path_is_executable (path) && runnable)//!strcmp (get_suffix(path), "ctx"))
  {
    ctx_list_append (&args, strdup (path));
  }

  char *basname = get_basename (path);

  if (!args)
  if (media_type_class == CTX_MEDIA_TYPE_TEXT)
  {
    ctx_list_append (&args, strdup ("ctx"));
    ctx_list_append (&args, strdup ("-E"));
    ctx_list_append (&args, strdup (path));
  }
  free (basname);
  
  if (!args)
  {
  if (media_type_class == CTX_MEDIA_TYPE_IMAGE)
  {
    ctx_list_append (&args, strdup ("ctx"));
    ctx_list_append (&args, strdup (path));
  }
  else if (!strcmp (media_type, "video/mpeg"))
  {
    ctx_list_append (&args, strdup ("ctx"));
    ctx_list_append (&args, strdup (path));
    //ctx_list_append (&args, strdup ("--paused"));
    ctx_list_append (&args, strdup ("--seek-to"));
    ctx_list_append (&args, ctx_strdup_printf ("%.2f", in));
  }
  else if (media_type_class == CTX_MEDIA_TYPE_AUDIO)
  {
    ctx_list_append (&args, strdup ("ctx-audioplayer"));
    ctx_list_append (&args, strdup (path));
  }
  else
  {
    ctx_list_append (&args, strdup ("ctx"));
    ctx_list_append (&args, strdup ("hexview"));
    ctx_list_append (&args, strdup (path));
  }

  }

//  args = NULL;
//   ctx_list_append (&args, strdup ("/bin/dash"));

#if 0
  if (argc)
  {

#if 0
  ctx_list_prepend (&args, strdup("/"));
  ctx_list_prepend (&args, strdup("/"));
  ctx_list_prepend (&args, strdup("--ro-bind"));
#else
//  char *foo = strdup (path);
//  ctx_list_prepend (&args, strdup(dirname (foo)));
//  ctx_list_prepend (&args, strdup(dirname (foo)));
//  free (foo);
//  ctx_list_prepend (&args, strdup("--ro-bind"));

//  ctx_list_prepend (&args, strdup("/dev"));
//  ctx_list_prepend (&args, strdup("/dev"));
//  ctx_list_prepend (&args, strdup("--dev-bind"));

  ctx_list_prepend (&args, strdup("/lib64"));
  ctx_list_prepend (&args, strdup("/lib64"));
  ctx_list_prepend (&args, strdup("--ro-bind"));

  ctx_list_prepend (&args, strdup("/lib"));
  ctx_list_prepend (&args, strdup("/lib"));
  ctx_list_prepend (&args, strdup("--ro-bind"));

  ctx_list_prepend (&args, strdup("/home")); // XXX drop the need for this
  ctx_list_prepend (&args, strdup("/home")); // in ctx!
  ctx_list_prepend (&args, strdup("--ro-bind"));

  ctx_list_prepend (&args, strdup("/bin"));
  ctx_list_prepend (&args, strdup("/bin"));
  ctx_list_prepend (&args, strdup("--ro-bind"));
#endif

  ctx_list_prepend (&args, strdup("256"));
  ctx_list_prepend (&args, strdup("--gid"));
  ctx_list_prepend (&args, strdup("512"));
  ctx_list_prepend (&args, strdup("--uid"));
  ctx_list_prepend (&args, strdup("ctx"));
  ctx_list_prepend (&args, strdup("CTX_BACKEND"));
  ctx_list_prepend (&args, strdup("--setenv"));

  ctx_list_prepend (&args, strdup("localhost"));
  ctx_list_prepend (&args, strdup("--hostname"));

  ctx_list_prepend (&args, strdup("--unshare-all"));

  ctx_list_prepend (&args, strdup("bwrap"));
  }
#endif

  if (!args)
    return NULL;
  return ctx_str_list_to_pasv (&args);
}

static void ui_visit_tag (CtxEvent *event, void *a, void *b)
{
  set_location ((char*)a);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}


static char *diz_dir_tag_child_item_path (const char *tag, const char *path)
{
  char *hex="0123456789abcdef";
  unsigned char hash[40];
  unsigned char hash_hex[51];
  CtxSHA1 *sha1 = ctx_sha1_new ();
  ctx_sha1_process (sha1, (uint8_t*)diz->path, strlen (diz->path));
  ctx_sha1_done (sha1, hash);
  ctx_sha1_free (sha1);
  for (int j = 0; j < 20; j++)
  {
    hash_hex[j*2+0]=hex[hash[j]/16];
    hash_hex[j*2+1]=hex[hash[j]%16];
  }
  hash_hex[40]=0;
  char *link_target = ctx_strdup_printf ("%s/%s",
                      diz_dir_wiki_path (tag),
                      hash_hex);
  return link_target;
}

static void ui_remove_tag (CtxEvent *event, void *a, void *b)
{
  diz_dir_unset_value (diz, -1, "parent", (char*)a);

  char *link_target = diz_dir_tag_child_item_path ((char*)a, diz->path);
  unlink (link_target);
  free (link_target);

  diz_dir_dirt (diz);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;
}

static void stuff_draw_bullet (ITK *itk, Diz *diz, int i)
{
   float em = itk->font_size;
int bullet = diz_dir_get_int (diz, i, "bullet", CTX_BULLET_NONE);
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
       sprintf (buf, "%i", item_get_list_index (diz, i));
       ctx_save (itk->ctx);
       ctx_text_align (itk->ctx, CTX_TEXT_ALIGN_RIGHT);
       ctx_text (itk->ctx, buf);
       ctx_restore (itk->ctx);
     }
     break;
     case CTX_BULLET_DONE:
     ctx_move_to (itk->ctx, x, itk->y + em);
     ctx_text (itk->ctx, "[v]");//☑"); //☒
     break;
     case CTX_BULLET_TODO:
     ctx_move_to (itk->ctx, x, itk->y + em);
     ctx_text (itk->ctx, "[ ]");//☐");
     break;
   }
}
{
  int folded = diz_dir_get_int (diz, i, "folded", -3);
  if (folded > 0)
  {
     float x = itk->x - em * 0.5;//
     if (bullet != CTX_BULLET_NONE) x -= em * 0.6;
     ctx_move_to (itk->ctx, x, itk->y + em);
     ctx_text (itk->ctx, "-");//▷▶▽▼");
  }
}

}

#define BIND_KEY(key, command, help) \
            do {ctx_add_key_binding (ctx, key, NULL, help, ui_run_command, command);} while(0)

#define MAX_CONTEXT_MENU_ITEMS 40
static char *context_menu[MAX_CONTEXT_MENU_ITEMS * 3];
int context_menu_items = 0;

static void add_context_binding (int active_context,
                                 const char *label,
                                 const char *command,
                                 const char *key)
{
  if (item_context_active)
    return;
  if (active_context)
  {
    context_menu[context_menu_items * 3 + 0] = strdup (label);
    context_menu[context_menu_items * 3 + 1] = strdup (command);
    context_menu[context_menu_items * 3 + 2] = strdup (key);
    if (context_menu_items < MAX_CONTEXT_MENU_ITEMS)
      context_menu_items++;
  }
  if (strcmp (key, "escape"))
    BIND_KEY (key, (char*)command, label);
}

static void layout_box_defaults (CtxRectangle *rectangle)
{
  rectangle->x = 0.05;
  rectangle->y = 0.02;
  rectangle->width = 0.9;
  rectangle->height = 4000.0;
}

static void dir_layout (ITK *itk, Diz *diz)
{
  Ctx *ctx = itk->ctx;

  //printf ("%s\n", diz->path);

  float em = itk_em (itk);
  float prev_height = layout_config.height;
  float row_max_height = 0;
  int level = 0;
  int is_folded = 0;

  layout_box_count = 0;
  layout_box_no = 0;
#if 0
  layout_box[0].x = 0.05;
  layout_box[0].y = (layout_config.margin_top * em) / itk->width;
  layout_box[0].width = 0.9;
  layout_box[0].height = 4000.0;
#endif
  layout_box_defaults (&layout_box[0]);

  diz->metadata_cache_no = -3;
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
  //float saved_y = itk->y;


  float y1;
  layout_box_no = 0;
  layout_page_no = 0;

  itk->x0 = itk->x = layout_box[layout_box_no].x * saved_width;
  itk->y           = layout_box[layout_box_no].y * saved_width;
  itk->width       = layout_box[layout_box_no].width * saved_width ;
  y1 = (layout_box[layout_box_no].y + layout_box[layout_box_no].height) * saved_width;
  ctx_save (itk->ctx);
  if (layout_config.monospace)
          ctx_font (itk->ctx, "mono");
  float space_width = ctx_text_width (itk->ctx, " ");
  ctx_font_size (itk->ctx, itk->font_size);
      int location_active = (focused_no == -1 && itk->focus_no == 0);

            if (!is_text_editing() && !viewer &&
              !item_context_active
                            )
            {

              if (history)
                BIND_KEY("alt-left", "history back", "back");
              if (future)
                BIND_KEY("alt-right", "history forward", "forward");

              if (!text_editor)
                BIND_KEY ("alt-down",  "activate",  "enter item");
            }
            else
            {

              if (!is_text_editing() && !viewer && !text_editor)
                BIND_KEY ("alt-down",  "activate",  "enter item");
            }
            add_context_binding (location_active, "go to parent", "go-parent", "alt-up");
            
            // as "alt-up",    "go-parent",    "go to parent");

  if (!layout_config.outliner)
  {
#if 0
  if (tool_no == STUFF_TOOL_RECTANGLE)
  {
    ctx_rectangle (itk->ctx, 0, 0, ctx_width (ctx), ctx_height (ctx));
    ctx_listen (itk->ctx, CTX_DRAG, tool_rect_drag, NULL, NULL);

    ctx_begin_path (itk->ctx);
  }
#endif
  }

  if (y1 < 100) y1 = itk->height;

  int last_page = 0;
  int printing = (layout_page_no == layout_show_page);

  if (layout_show_page == 0)
  {
    if (printing)
    {
      // allocate room for and key-nav/focus rect
      // for title - up front, to maintain focus
      // order
      //CtxControl *title_control =
      itk_add_control (itk, UI_LABEL, "title",
                       itk->x0 - em * 0.5f,
                       itk->font_size * 0,
                       itk->width,
                       itk->font_size * 2.5);
      ctx_listen (itk->ctx, CTX_PRESS, dir_select_item, (void*)((size_t)(-1)), NULL);
    }
  
    itk->y = 2.0 * itk->font_size;
    int parents = diz_dir_value_count (diz, -1, "parent");
    float tx = itk->x0;// + itk->font_size / 2;
    for (int i = 0; i < parents; i++)
    {
      char *parent = diz_dir_get_string_no (diz, -1, "parent", i);
      ctx_begin_path (itk->ctx);
      float width = ctx_text_width (itk->ctx, parent);
      if (itk->focus_no == itk->control_no)
      {
        char *dup_par = strdup (parent);
        ctx_add_key_binding_full (ctx, "return", NULL, "view category", ui_visit_tag, dup_par, free, dup_par);
        dup_par = strdup (parent);
        ctx_add_key_binding_full (ctx, "alt-down", NULL, "view category", ui_visit_tag, dup_par, free, dup_par);
        dup_par = strdup (parent);
        ctx_add_key_binding_full (ctx, "delete", NULL, "remove tag", ui_remove_tag, dup_par, free, dup_par);
      }
  
      CtxControl *c = itk_add_control (itk, UI_LABEL, "tag",
                       tx,
                       itk->font_size * 2.5,
                       itk->font_size + width,
                       itk->font_size * 1);
      ctx_listen (itk->ctx, CTX_PRESS, dir_select_item, (void*)(size_t)c->no, NULL);
      ctx_font_size (itk->ctx, itk->font_size);
      ctx_rgb (itk->ctx, 1, 1, 0);
      ctx_move_to (itk->ctx, tx + 0.5 * itk->font_size,
                   itk->font_size * 3.3);
      tx += width + itk->font_size;
      ctx_text (itk->ctx, parent);
      free (parent);
    }
  
    {
      ctx_begin_path (itk->ctx);
      static char tag[100]="";
  
      if (itk->focus_no == itk->control_no || 1)
      {
        float width = ctx_text_width (itk->ctx, "aaaaaaaaaaa");
        itk->x = tx + itk->font_size * 0.2;
        itk->y+= itk->font_size * 0.5;
        float w = itk->width;
        itk->width = width;
        if (itk_entry (itk, "", "+", tag, sizeof(tag)-1, NULL, NULL))
        {
          diz_dir_add_string_unique (diz, -1, "parent", tag);
          char *link_path = diz_dir_tag_child_item_path (tag, diz->path);
          symlink (diz->path, link_path);
          free (link_path);
          diz_dir_dirt (diz);
          tag[0]=0;
          ctx_queue_draw (itk->ctx);
        }
        itk->y-= itk->font_size * 0.5;
        itk->width = w;
      }
      else
      {
        float width = ctx_text_width (itk->ctx, " + ");
        //CtxControl *c = 
                itk_add_control (itk, UI_LABEL, "tag",
                        tx,
                        itk->font_size * 2.5,
                        itk->font_size + width,
                        itk->font_size * 1);
        ctx_font_size (itk->ctx, itk->font_size);
        ctx_rgb (itk->ctx, 1, 1, 0);
        ctx_move_to (itk->ctx, tx + 0.5 * itk->font_size,
                     itk->font_size * 3.3);
        ctx_text (itk->ctx, "+ ");
      }
    }
  }
  itk->y = 4.0 * itk->font_size;

  if (layout_config.outliner)
     printing = 1;

  for (int i = 0; i < diz->count; i++)
  {
      char *d_name = diz_dir_get_name (diz, i);
      if (layout_find_item == i)
      {
         if (!printing)
         {
           layout_show_page = layout_page_no; // change to right page
           ctx_queue_draw (itk->ctx); // queue another redraw
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

      float width = 0;
      float height = 0;
      
      int hidden = 0;
      CtxAtom atom = diz_dir_type_atom (diz, i);

      switch (atom)
      {
        case CTX_ATOM_RECTANGLE:
        case CTX_ATOM_TEXT:
        case CTX_ATOM_FILE:
        case CTX_ATOM_SYMLINK:
        case CTX_ATOM_CTX:
          break;
        case CTX_ATOM_LAYOUTBOX:
          break;
        case CTX_ATOM_STARTGROUP:
          hidden = 1;
          level ++;
          {
            int folded = diz_dir_get_int (diz, i-1, "folded", 0);

            if (folded && ! is_folded) is_folded = level;
          }
          break;
        case CTX_ATOM_ENDGROUP:
          if (is_folded == level)
          {
            is_folded = 0;
          }
          hidden = 1;
          level --;
          break;
        case CTX_ATOM_STARTPAGE:
          hidden = 1;
          /*FALLTHROUGH*/
        case CTX_ATOM_NEWPAGE:
          layout_box_count = 0;
          layout_box_no = 0;
          // XXX need better default for new page
          layout_box_defaults(&layout_box[0]);
          break;
      }

      if (layout_config.hide_non_file)
      if (atom != CTX_ATOM_FILE &&
          atom != CTX_ATOM_SYMLINK) hidden = 1;

      if (is_folded)
        hidden = 1;

      int label = 0;
      int gotpos = 0;
      int gotdim = 0;
      char *xstr = NULL;
      char *ystr = NULL;
      char *wstr = NULL;
      char *hstr = NULL;
      float origin_x = 0.0;
      float origin_y = 0.0;

      float padding_left = layout_config.padding_left;
      float padding_right = layout_config.padding_right;
      float padding_top = layout_config.padding_top;
      float padding_bottom = layout_config.padding_bottom;

      //padding_left += level * layout_config.level_indent;

      float x = 0.0;
      float y = 0.0;
      
      if (!text_editor)
      {

      label = diz_dir_get_int (diz, i, "show-label", -1234);
      if (label == -1234) {
        if (atom == CTX_ATOM_TEXT || atom == CTX_ATOM_SYMLINK)
          label = 0;
        else
          label = layout_config.label;
      }

      xstr     = diz_dir_get_string (diz, i, "x");
      ystr     = diz_dir_get_string (diz, i, "y");
      wstr     = diz_dir_get_string (diz, i, "width");
      hstr     = diz_dir_get_string (diz, i, "height");
      origin_x = diz_dir_get_float (diz, i, "origin-x", 0.0);
      origin_y = diz_dir_get_float (diz, i, "origin-y", 0.0);


      padding_left = diz_dir_get_float (diz, i, "padding-left", layout_config.padding_left);
      padding_right = diz_dir_get_float (diz, i, "padding-right", layout_config.padding_right);
      padding_top = diz_dir_get_float (diz, i, "padding-top", layout_config.padding_top);
      padding_bottom = diz_dir_get_float (diz, i, "padding-bottom", layout_config.padding_bottom);

      //padding_left += level * layout_config.level_indent;

      x = diz_dir_get_float (diz, i, "x", 0.0);
      y = diz_dir_get_float (diz, i, "y", 0.0);
      //float opacity = diz_dir_get_float (i, "opacity", 1.0f);
      }

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
      if (wstr)
      {
        free (wstr);
        gotdim = 1;
      }
      if (hstr)
      {
        free (hstr);
        gotdim = 1;
      }

      if (layout_config.fixed_pos)
        gotpos = 0;

      {
        if (text_editor)
          width = -1000.0;
        else
          width  = diz_dir_get_float (diz, i, "width", -1000.0);
        if (width < 0 || layout_config.fixed_size)
          width = 
            layout_config.fill_width? itk->width * 1.0:
            layout_config.width * em;
        else {
          width *= saved_width;
        }
      }

      {
        if (text_editor)
          height = -1000.0;
        else
          height = diz_dir_get_float (diz, i, "height", -1000.0);
        if (height < 0 || layout_config.fixed_size)  height =
          layout_config.fill_height? itk->height * 1.0:
          layout_config.height * em;
        else
          height *= saved_width;
      }

      x -= (origin_x * width / saved_width);
      y -= (origin_y * height / saved_width);



      int virtual = (diz_dir_type_atom (diz, i) == CTX_ATOM_TEXT);

      if (virtual)
        atom = CTX_ATOM_TEXT;
      if (layout_config.outliner)
      {
        gotpos = 0;
        virtual = 1;
      }

      if (atom == CTX_ATOM_SYMLINK)
        virtual = 1;

      if (layout_config.stack_horizontal && layout_config.stack_vertical)
      {
      if (itk->x + width  > itk->x0 + itk->width ||atom == CTX_ATOM_NEWPAGE || atom == CTX_ATOM_STARTPAGE) //panel->x + itk->panel->width)
      {
          itk->x = itk->x0;
          if (layout_config.stack_vertical)
          {
            itk->y += row_max_height;
            row_max_height = 0;
          }
          if (itk->y + height > y1 ||
              atom == CTX_ATOM_NEWPAGE ||
              atom == CTX_ATOM_STARTPAGE)
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
              last_page = layout_page_no;
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
      else if (atom == CTX_ATOM_NEWPAGE)
      {
         width = itk->width;
         height = 1;
      }

  if (layout_config.outliner)
  {
    label = 0;
    atom = CTX_ATOM_TEXT;
    hidden = 0;
    printing = 1;
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
          //width -= em * level * layout_config.level_indent;
          /* measure height, and snap cursor */
          layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                       space_width, width - em * level * layout_config.level_indent, em,
                       i == focused_no ? text_edit : -1,
                       i == focused_no ? text_edit + 2: -1,
                       0, NULL, &height,
                       NULL, NULL,
                       (i == focused_no && text_edit >= 0),
                       i == focused_no);
          height = height - itk->y;// + em;// * 0.5;
          row_max_height = height;
        }
      }

      CtxControl *c = NULL;
      if (printing)
      {
        ctx_begin_path (itk->ctx);
        c = itk_add_control (itk, UI_LABEL, "item",
          itk->x - em * (padding_left * 2), itk->y, // - em * padding_top,
          width + em * (padding_left*2+padding_right)
           - em * level * layout_config.level_indent
          ,
          height + em * (padding_top+padding_bottom));
        if (focused_no == i)
           focused_control = c;
      }

      if (!viewer && printing && sy + height > 0 && sy < ctx_height (itk->ctx))
      {
              //ctx_rgb(itk->ctx,1,0,0);
              //ctx_fill(itk->ctx);
        struct stat stat_buf;
        char *newpath = malloc (strlen(diz->path)+strlen(d_name) + 2);
        if (!strcmp (diz->path, PATH_SEP))
          sprintf (newpath, "%s%s", PATH_SEP, d_name);
        else
          sprintf (newpath, "%s%s%s", diz->path, PATH_SEP, d_name);
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
             media_type = "rectangle";
           }
           else if (atom == CTX_ATOM_CTX)
           {
             media_type = "ctx";
           }
           else if (atom == CTX_ATOM_TEXT)
           {
           }
           else if (atom == CTX_ATOM_FILE)
           {
           }
           else if (atom == CTX_ATOM_SYMLINK)
           {
           }
        }


        if (c->no == itk->focus_no && layout_find_item < 0)
        {

          if (dir_item_count_links (focused_no) == 0)
           layout_focused_link = -1;

          focused = 1;
          //fprintf (stderr, "\n{%i %i %i}\n", c->no, itk->focus_no, i);
          ctx_begin_path (itk->ctx);
          ctx_rectangle (itk->ctx, c->x, c->y, c->width, c->height);
         
          //ctx_listen (itk->ctx, CTX_TAP_AND_HOLD, item_activate, NULL);
          ctx_listen (itk->ctx, CTX_DRAG, item_drag, (void*)(size_t)i, NULL);
          //ctx_listen (itk->ctx, CTX_TAP_AND_HOLD, item_tap_and_hold, (void*)(size_t)i, NULL);


              //BIND_KEY ("insert", "insert-text", "insert text item");
          add_context_binding (1, "insert text item", "insert-text", "insert");
          add_context_binding (1, "insert new page", "insert-newpage", "control-insert");
          ctx_begin_path (itk->ctx);
          //ctx_rgb(itk->ctx,1,0,0);
          //ctx_fill(itk->ctx);

          if (!viewer)
          {


           if (gotpos)
           {
              add_context_binding (1, "reflow", "reflow", "control-f");
           }
           else
           {
              add_context_binding (1, "unflow", "unflow", "control-f");
           }


          if (!is_text_editing()
              && !item_context_active)
          {
            add_context_binding (1, "duplicate", "item-duplicate", "control-d");
            add_context_binding (1, "remove", "remove", "delete");

            if (!layout_config.outliner)
            {

               if (layout_focused_link >= 0)
               {
                 BIND_KEY ("return", "follow-link", "follow link");
                 BIND_KEY ("alt-down", "follow-link", "follow link");
               }
               else
              {
                if (text_editor)
                  BIND_KEY ("return", "item-edit-text", "edit paragraph");
                else
                {
                  if (atom == CTX_ATOM_SYMLINK)
                    BIND_KEY ("return", "visit-symlink", "visit");
                  else
                    BIND_KEY ("return", "activate", "activate/edit");
                }
              }

            if (!text_editor)
            {
              int bullet = diz_dir_get_int (diz, i, "bullet", CTX_BULLET_NONE);
              const char *label = "cycle bullet";
              switch (bullet)
              {
                case CTX_BULLET_NONE:    label = "make bullet"; break;
                case CTX_BULLET_BULLET:  label = "make numbered"; break;
                case CTX_BULLET_NUMBERS: label = "make todo"; break;
                case CTX_BULLET_TODO:    label = "mark done"; break;
                case CTX_BULLET_DONE:    label = "no bullet"; break;
              }
              add_context_binding (1, label, "cycle-bullet", "control-t");
            }

            if (atom == CTX_ATOM_TEXT)
              add_context_binding (1, "join with next paragraph", "join-with-next", "control-j");

            if (!text_editor)
            {
              const char *label = "make heading";
              char *klass = diz_dir_get_string (diz, i, "tag");
              if (klass)
              {
                label = "cycle heading";
                free (klass);
              }
              else
              {

              }
              if (atom == CTX_ATOM_TEXT)
                add_context_binding (1, label, "cycle-heading", "control-h");
            }

              if (text_editor || vim_keys)
              {
                BIND_KEY ("i", "item-edit-text", "insert text");
                BIND_KEY ("A", "item-edit-text-end", "insert text");
              }
            }
          }
          if (!text_editor && !editing_location
                           && !is_text_editing () 
                           && layout_focused_link < 0)
          {
            if (item_context_active)
              BIND_KEY ("alt-return", "context-menu-hide", "item properties");
            else
              BIND_KEY ("alt-return", "context-menu-show", "item properties");
          }


          if (!layout_config.outliner
              && !is_text_editing ()
              && !item_context_active)
          {
            add_context_binding (1, "move after next sibling",
                                    "move-after-next-sibling",
                                    "control-page-down");
            add_context_binding (1, "move before previous sibling",
                                    "move-before-previous-sibling",
                                    "control-page-up");

            ctx_add_key_binding (ctx, "shift-control-down", NULL, "grow height", grow_height, NULL);
            ctx_add_key_binding (ctx, "shift-control-up", NULL, "shrink height", shrink_height, NULL);

            ctx_add_key_binding (ctx, "shift-control-left", NULL, "shrink width", shrink_width, NULL);
            ctx_add_key_binding (ctx, "shift-control-right", NULL, "grow width", grow_width, NULL);

          if (gotpos)
          {
          BIND_KEY("control-left", "item-move-pos left", "item-move-pos left");
          BIND_KEY("control-up", "item-move-pos up", "item-move-pos up");
          BIND_KEY("control-right", "item-move-pos right", "item-move-pos right");
          BIND_KEY("control-down", "item-move-pos down", "item-move-pos down");

          }
          else
          {
#if 0
            add_context_binding (1,"move after next sibling",
                                 "move-after-next-sibling",
                                 "control-down");
            add_context_binding (1,"move before previous sibling",
                                 "move-before-previous-sibling",
                                 "control-up");
#endif
            BIND_KEY ("control-down",
                      "move-after-next-sibling",
                      "move after next sibling");
            BIND_KEY ("control-up",
                      "move-before-previous-sibling",
                      "move before previoue sibling");

            if (!text_editor)
            {
#if 0
              BIND_KEY ("control-left", "make-sibling-of-parent", "make sibling of parent");

              BIND_KEY ("control-right", "make-child-of-previous", "make child of previous");
#else
              add_context_binding (1,
                              "demote",
                              "make-sibling-of-parent",
                              "control-left");
              add_context_binding (1,
                              "promote",
                              "make-child-of-previous",
                              "control-right");
#endif
            }

          }


          if (atom == CTX_ATOM_FILE && strcmp (media_type, "inode/directory"))
           {
                   // XXX only if executable
            int runnable = diz_dir_get_int (diz, focused_no, "runnable", 0);
            if (runnable)
            {
              add_context_binding (1,
                            "make non runnable",
                            "unset runnable",
                            "control-r");
              int live = diz_dir_get_int (diz, focused_no, "live", 0);
              if (live)
              add_context_binding (1,
                            "make unlive",
                            "unset live",
                            "control-l");
              else
              add_context_binding (1,
                            "make live",
                            "set live 1",
                            "control-l");
            }
            else
            add_context_binding (1,
                            "make runnable",
                            "set runnable 1",
                            "control-r");
                            
           }
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
          ctx_gray (itk->ctx, 0.95);

          if (c->no == itk->focus_no && layout_find_item < 0)
          {
            layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                         space_width, width - em * level * layout_config.level_indent, em,
                         text_edit,text_edit,
                         1, NULL, NULL,
                         &prev_line_pos, &next_line_pos,
                         (i == focused_no && text_edit >= 0), 1);
          }
          else
          {
            layout_text (itk->ctx, itk->x + padding_left * em, itk->y, d_name,
                         space_width, width - em * level * layout_config.level_indent, em,
                         -1, -1,
                         1, NULL, NULL,
                         NULL, NULL,
                         0, 0);
          }

          stuff_draw_bullet (itk, diz, i);

          ctx_restore (itk->ctx);
          if (i == focused_no && d_name[0]=='$')
          {
            add_context_binding (1,
                            "run command",
                            "run-command",
                            "control-return");
          }
        }
        else if (atom == CTX_ATOM_SYMLINK)
        {
          ctx_save (itk->ctx);

          char *target = diz_dir_get_string (diz, i, "target");
          Diz *tdiz = diz_dir_new ();
          diz_dir_set_path (tdiz, target);
          char *label = strdup (tdiz->title?tdiz->title:"xx");
          diz_dir_destroy (tdiz);

                  
          ctx_gray (itk->ctx, 0.95);

          if (c->no == itk->focus_no && layout_find_item < 0)
          {
            layout_text (itk->ctx, itk->x + padding_left * em, itk->y, label,
                         space_width, width - em * level * layout_config.level_indent, em,
                         text_edit,text_edit,
                         1, NULL, NULL,
                         &prev_line_pos, &next_line_pos,
                         (i == focused_no && text_edit >= 0), 1);
          }
          else
          {
            layout_text (itk->ctx, itk->x + padding_left * em, itk->y, label,
                         space_width, width - em * level * layout_config.level_indent, em,
                         -1, -1,
                         1, NULL, NULL,
                         NULL, NULL,
                         0, 0);
          }

          stuff_draw_bullet (itk, diz, i);

          ctx_restore (itk->ctx);
        }
      else
        if (atom == CTX_ATOM_RECTANGLE)
        {
          char *fill       = diz_dir_get_string (diz, i, "fill");
          char *stroke     = diz_dir_get_string (diz, i, "stroke");
          float line_width = diz_dir_get_float  (diz, i, "line-width", 1.0);
          //float opacity    = diz_dir_get_float (diz, i, "opacity", 1.0);

          if (gotpos) label = 0;
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
        else if (atom == CTX_ATOM_NEWPAGE)
        {
#if 0
           ctx_rgb(itk->ctx, 1.0,0,0);
           ctx_rectangle (itk->ctx, itk->x, itk->y, itk->width, 2);
           ctx_fill (itk->ctx);
#endif
           ctx_save (itk->ctx);
           ctx_move_to (itk->ctx, itk->width/2, itk->y - itk->font_size * 0.25f);
           ctx_gray (itk->ctx, 0.8);
           char buf[128];
           sprintf (buf, "page %i/%i", layout_show_page+1, layout_last_page+1);
           ctx_text (itk->ctx, buf);
           ctx_restore (itk->ctx);
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
        else if (diz_dir_get_int (diz, i, "live", 0))
          {
            char *client_name = ctx_strdup_printf ("%s-%i", newpath, i);
            CtxClient *client = ctx_client_find (ctx, client_name);
            free (client_name);
            float live_font_factor = 0.33;
            if (!client)
            {
#if 0
              // destory all clients that are not this one //
              CtxList *to_remove = NULL;
              for (CtxList *l = clients; l; l = l->next)
              {
                CtxClient *client = l->data;
                if (!(ctx_client_flags (client) & ITK_CLIENT_LAYER2))
                {
                  ctx_list_prepend (&to_remove, client);
                }
              }
              while (to_remove)
              {
                ctx_client_remove (ctx, to_remove->data);
                ctx_list_remove (&to_remove, to_remove->data);
              }
#endif

              char **command = dir_get_viewer_argv (newpath, i);
              if (command)
              {
                client = ctx_client_new_argv (ctx, command,
                  itk->x, itk->y, width, height,
                  itk->font_size * live_font_factor,
                  ITK_CLIENT_PRELOAD,
                  ctx_strdup_printf ("%s-%i", newpath, i), user_data_free);
                free (command);
              }
            }
            else
            {
              ctx_client_move (ctx, ctx_client_id (client), itk->x, itk->y);
              ctx_client_set_font_size (ctx, ctx_client_id (client), itk->font_size * live_font_factor);
              ctx_client_resize (ctx, ctx_client_id (client), width, height);

            }
          }

        else if (ctx_media_type_class (media_type) == CTX_MEDIA_TYPE_IMAGE)
        {
          if (gotpos) label = 0;

          draw_img (itk, itk->x, itk->y, width, height, newpath, gotdim?0:1);
          if (c->no == itk->focus_no && layout_find_item < 0 && gotpos)
          {
             float em = itk->font_size;
             int resize_dim = 4;

             ctx_arc (itk->ctx, itk->x + width * origin_x,
                           itk->y + height * origin_y,
                           0.5f * em,
                           0.0,
                           M_PI * 2, 0);
             ctx_rgba (itk->ctx, 1, 1,0, 0.9f);
             ctx_fill (itk->ctx);

             ctx_rectangle (itk->ctx, itk->x + resize_dim * em, itk->y, width - resize_dim * 2 * em, resize_dim * em);
             ctx_rgba (itk->ctx, 0,0,0, 0.5f);
             ctx_fill (itk->ctx);


             ctx_rectangle (itk->ctx, itk->x + width - resize_dim * em,
                                      itk->y + height - resize_dim * em,
                                      resize_dim * em, resize_dim * em);
             ctx_rgba (itk->ctx, 0,0,0, 0.5f);
             ctx_fill (itk->ctx);
          }
        }

        else if (ctx_media_type_class (media_type) == CTX_MEDIA_TYPE_TEXT ||
                 ctx_media_type_class (media_type) == CTX_MEDIA_TYPE_VIDEO ||
                 ctx_media_type_class (media_type) == CTX_MEDIA_TYPE_APPLICATION
                )
        {
          if (gotpos) label = 0;

          draw_img (itk, itk->x, itk->y, width, height, newpath, gotdim?0:1);
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

      if (label && d_name[0])
      {
      char *title = malloc (strlen (d_name) + 32);
      strcpy (title, d_name);
      int title_len = strlen (title);
      {
        int wraplen = 10;
        int lines = (title_len + wraplen - 1)/ wraplen;

        if (!focused &&  lines > 2) lines = 2;

        ctx_rectangle (itk->ctx, itk->x, itk->y + height - em * (lines + 1),
                        width, em * (lines + 0.5));
        ctx_rgba (itk->ctx, 0,0,0,0.4);
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
            itk->y += row_max_height + layout_config.padding_bottom * em;
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
             last_page = layout_page_no;
             printing = (layout_page_no == layout_show_page);
            }

          }

#endif
          itk->x += level * layout_config.level_indent * em;
        }
      }

      if (layout_config.outliner)
      {
  int keys = diz_dir_key_count (diz, i);
  for (int k = 0; k < keys; k++)
  {
    char *key = diz_dir_key_name (diz, i, k);
    if (key)
    {
      char *val = diz_dir_get_string (diz, i, key);
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
      free (d_name);
  }
  layout_last_page = last_page;

  if (diz->count <= 0)
  {
      CtxControl *c = NULL;
      if (printing)
      {
        float width = itk->width;
        float height = em;

        ctx_begin_path (itk->ctx);
        c = itk_add_control (itk, UI_LABEL, "item",
          itk->x - em * (layout_config.padding_left * 2), itk->y, // - em * padding_top,
          width + em * (layout_config.padding_left*2+layout_config.padding_right),
          height + em * (layout_config.padding_top+layout_config.padding_bottom));
        if (itk->focus_no == c->no)
        {
          itk_labelf (itk, "press return to add item");
          //ctx_add_key_binding (ctx, "return", NULL, "edit first item", dir_location, NULL);
          BIND_KEY ("return", "insert-text", "insert text item");
        }
        else
        itk_labelf (itk, "no items");
      }
  }

  ctx_restore (itk->ctx);

  // draw title
  if (layout_show_page == 0)
  {
  ctx_save (itk->ctx);
  ctx_font_size (itk->ctx, itk->font_size * 2);
  //int printing = (layout_page_no == layout_show_page);

  if (editing_location)
  {
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

      ctx_rectangle (ctx, itk->x0 + sel_start, 1.0 * em - em,
                          sel_end-sel_start,em * 2.5);
      if (c_start==c_end)
        ctx_rgba (ctx, 1,1, 0.2, 1);
      else
        ctx_rgba (ctx, 0.5,0, 0, 1);
      ctx_fill (ctx);
      ctx_move_to (itk->ctx, itk->x0, 2 * em);
      ctx_rgba (ctx, 1,1,1, 1);
      ctx_text (ctx, commandline->str);
    }
  }
  else
  {
    ctx_rgba (itk->ctx, 1,1,1,0.8);
    ctx_move_to (itk->ctx, itk->x0, 2 * em);

    char *title = diz_dir_get_string (diz, -1, "title");

    if (title)
    {
      ctx_text (ctx, title);
      free (title);
    }
    else
    if (diz->title)
      ctx_text (ctx, diz->title);
    else
      ctx_text (ctx, diz->path);
  }
  ctx_restore (itk->ctx);
  }


  itk->x0    = saved_x0;
  itk->width = saved_width;
//  itk->y     = saved_y;

  if (focused_no == -1 && !editing_location && itk->focus_no == 0)
  {
    BIND_KEY ("alt-return", "toggle-context", "document properties");
    ctx_add_key_binding (ctx, "return", NULL, "edit location", dir_location, NULL);
    ctx_add_key_binding (ctx, "alt-down", NULL, NULL, dir_location, NULL);
  }
  BIND_KEY ("resize-event", "", NULL); // causes a queue-draw
                                    

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
    int batch_size = 2;
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
    ctx_queue_draw (ctx);
  }
  else
  {
    int status = 0;
    waitpid(0, &status, WNOHANG); // collect finished thumb generators
    ctx_queue_draw (ctx);
  }
  return 1;
}

extern float font_size;

int viewer_pre_next (Ctx *ctx, void *data1)
{
  if (focused_no+1 >= diz_dir_count(diz))
    return 0;
  //focused_no++;
  //layout_find_item = focused_no;

  char *name;
  char *path;
  char *pathA;
  CtxClient *client = NULL;
  name = diz_dir_get_name (diz, focused_no);
  path = ctx_strdup_printf ("%s/%s", diz->path, name);

  char *client_cur = ctx_strdup_printf ("%s-%i", path, focused_no);
  free (name);

  name = diz_dir_get_name (diz, focused_no+1);
  pathA = ctx_strdup_printf ("%s/%s", diz->path, name);
  free (name);

  float pre_thumb_size = 2.0;

  char *client_a = ctx_strdup_printf ("%s-%i", pathA, focused_no+1);
#if 0
  if ((client=ctx_client_find (ctx, client_a)))
  {
     fprintf (stderr, "reusing %s\n", client_a);
     ctx_client_move (ctx, ctx_client_id (client), 
          ctx_width (ctx) - itk->font_size * pre_thumb_size,
          ctx_height (ctx) - itk->font_size * pre_thumb_size);
     ctx_client_resize (ctx, ctx_client_id (client), 
          itk->font_size * pre_thumb_size,
          itk->font_size * pre_thumb_size);
  }
  else
#endif
  {
    char **command = dir_get_viewer_argv (pathA, focused_no+1);
    if (command)
    {
     fprintf (stderr, "preloading %s\n", client_a);
     //fprintf (stderr, "%s\n", command);
     client= ctx_client_new_argv (ctx, command,
          ctx_width (ctx) - itk->font_size * pre_thumb_size,
          ctx_height (ctx) - itk->font_size * pre_thumb_size,
          itk->font_size * pre_thumb_size,
          itk->font_size * pre_thumb_size,
          itk->font_size, ITK_CLIENT_PRELOAD, strdup (client_a), user_data_free);
      free (command);
    }
  }
  if (client)
    ctx_client_raise_top (ctx, ctx_client_id (client));

#if 1
  CtxList *to_remove = NULL;
  for (CtxList *l = ctx_clients (ctx); l; l = l->next)
  {
    CtxClient *client = l->data;
    if (ctx_client_flags (client) & ITK_CLIENT_PRELOAD)
    {
      if (! ( (!strcmp (client_cur, ctx_client_userdata (client))) ||
              (!strcmp (client_a, ctx_client_userdata (client)))))
      {
         ctx_list_prepend (&to_remove, client);
      }
    }
  }
  while (to_remove)
  {
    ctx_client_remove (ctx, to_remove->data);
    ctx_list_remove (&to_remove, to_remove->data);
  }
#endif

  free (client_a);
  free (client_cur);
  return 0;
}

static int viewer_space (Ctx *ctx, void *a)
{
  ctx_client_feed_keystring (viewer, NULL, "space");
  return 0;
}

void viewer_load_path (const char *path, const char *name)
{
  viewer_slide_start = ctx_ticks ();
  if (viewer_loaded_path)
  {
    ctx_queue_draw (ctx);
    free (viewer_loaded_path);
    viewer_loaded_path = NULL;
  }

  if (viewer_loaded_name) free (viewer_loaded_name);
  viewer_loaded_name = strdup (name);

  int no = focused_no;//diz_dir_name_to_no (diz, name);


  if (path)
  {
    viewer_loaded_path = strdup (path);

    // not entirely precise?
    // and does not permit override through diz-data XXX
    viewer_media_type = ctx_path_get_media_type (path);

    char **command = dir_get_viewer_argv (path, no);
   

    if (command)
    {
      //fprintf (stderr, "ctx-dir:%f\n", itk->font_size);
      char *client_name = ctx_strdup_printf ("%s-%i", path, no);
      if ((viewer=ctx_client_find (ctx, client_name)))
      {
        //fprintf (stderr, "reloading %s\n", client_name);
        if (ctx_client_flags (viewer) & ITK_CLIENT_PRELOAD)
        {
          viewer_was_live = 0;
        }
        else
        {
          viewer_was_live = 1;
        }
        ctx_client_set_font_size (ctx, ctx_client_id (viewer), itk->font_size);
        ctx_client_move (ctx, ctx_client_id (viewer), 0, 0);
        ctx_client_resize (ctx, ctx_client_id (viewer), ctx_width (ctx), ctx_height (ctx));
        ctx_client_feed_keystring (viewer, NULL, "space");
      }
      else
      {
        viewer_was_live = 0;
        //fprintf (stderr, "loading %s\n", client_name);
        //fprintf (stderr, "%s\n", command);
        viewer = ctx_client_new_argv (ctx, command,
          0, 0, ctx_width(ctx), ctx_height(ctx), itk->font_size, ITK_CLIENT_PRELOAD, strdup (client_name), user_data_free);
        //if (viewer_media_type[0] == 'v')
        //ctx_add_timeout (ctx, 1000 * 0.2, viewer_space, NULL);
      }
      free (client_name);
      ctx_client_raise_top (ctx, ctx_client_id (viewer));

    //fprintf (stderr, "[%s]\n", command);
#if 0
      fprintf (stderr, "run:%s %i %i %i %i,   %i\n", command,
        (int)ctx_width(ctx)/2, (int)0, (int)ctx_width(ctx)/2, (int)(ctx_height(ctx)-font_size*2), 0);
#endif
      free (command);

    }
    else
    {
      viewer = NULL;
    }
  }
  else
  {
    viewer = NULL;
  }


  float duration = 5.0;
  float in = diz_dir_get_float (diz, no, "in", -1);
  float out = diz_dir_get_float (diz, no, "out", -1);
  if (out > 0)
  {
    duration = out - in;
  }

  viewer_slide_duration = 1000 * 1000 * duration;

  viewer_pre_next (ctx, NULL); /* on initial opening of viewer mode we spawn both this image and the
                                */
}

static void dir_ignore (CtxEvent *e, void *d1, void *d2)
{
  e->stop_propagate = 1;
}

static void dir_backspace (CtxEvent *e, void *d1, void *d2)
{
  ctx_queue_draw (e->ctx);
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
  ctx_queue_draw (e->ctx);
  e->stop_propagate = 1;

  for (int i = 0; commandline->str[i] && commandline->str[i] != ' '; i++)
  {
    ctx_string_append_byte (word, commandline->str[i]);
  }

  if (!strcmp (word->str, "cd.."))
  {
    argvs_eval ("go-parent");
    layout_show_page = 0;
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
      argvs_eval ("go-parent");
    }
    else
    {
      if (arg[0] == '/')
      {
        set_location (arg);
      }
      else
      {
        char *full_path = ctx_strdup_printf ("%s/%s", diz->path, arg);
        set_location (full_path);
        free (full_path);
      }
      layout_find_item = 0;
    }
    layout_show_page = 0;
  }
  else
  {
     // all other code should be doing absolute paths,
     // this makes it easier to launch commands
     chdir (diz->path);
    // system (commandline->str);

    CtxClient *old = ctx_client_find (ctx, "stdout");
    if (old)
      ctx_client_remove (ctx, old);

    int terminal_height = 24;

    ctx_client_new (ctx, commandline->str, 10, ctx_height(ctx)-font_size*(terminal_height+1), font_size*42, font_size*terminal_height,
                    itk->font_size,
                    ITK_CLIENT_LAYER2|ITK_CLIENT_KEEP_ALIVE|ITK_CLIENT_TITLEBAR, strdup("stdout"), user_data_free);

     diz_dir_dirt(diz);
     save_metadata();
  }

  ctx_string_set (commandline, "");
  commandline_cursor_end =
  commandline_cursor_start = 0;

  ctx_string_free (word, 1);
}



static void dir_location_return (CtxEvent *e, void *d1, void *d2)
{
  editing_location = 0;
  set_location (commandline->str);
  ctx_string_set (commandline, "");
  //        ctx_listen (itk->ctx, CTX_PRESS, dir_select_item, (void*)(size_t)c->no, NULL);

  e->stop_propagate = 1;
  ctx_queue_draw (e->ctx);
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
  ctx_queue_draw (e->ctx);
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
  ctx_queue_draw (e->ctx);
}

static void dir_location_extend_sel_right (CtxEvent *e, void *d1, void *d2)
{
  commandline_cursor_end ++;
  if (commandline_cursor_end > (int) strlen (commandline->str))
    commandline_cursor_end = strlen (commandline->str);

  e->stop_propagate = 1;
  ctx_queue_draw (e->ctx);
}

static void dir_location_select_all (CtxEvent *e, void *d1, void *d2)
{
  commandline_cursor_start = 0;
  commandline_cursor_end = strlen (commandline->str);

  e->stop_propagate = 1;
  ctx_queue_draw (e->ctx);
}

static void item_context_choice_prev (CtxEvent *e, void *d1, void *d2)
{
  item_context_choice --;
  if (item_context_choice < 0) item_context_choice = 0;
  e->stop_propagate = 1;
  ctx_queue_draw (e->ctx);
}

static void item_context_choice_next (CtxEvent *e, void *d1, void *d2)
{
  item_context_choice ++;
  if (item_context_choice >= context_menu_items)
    item_context_choice = context_menu_items-1;

  e->stop_propagate = 1;
  ctx_queue_draw (e->ctx);
}

static void item_context_make_choice (CtxEvent *e, void *d1, void *d2)
{
  //char **choices = d1;
  argvs_eval (context_menu[item_context_choice*3+1]);
  item_context_active = 0;
  e->stop_propagate = 1;
  ctx_queue_draw (e->ctx);
}

static void dir_location_extend_sel_left (CtxEvent *e, void *d1, void *d2)
{
  commandline_cursor_end --;
  if (commandline_cursor_end < 0) commandline_cursor_end = 0;

  e->stop_propagate = 1;
  ctx_queue_draw (e->ctx);
}

static void dir_any (CtxEvent *e, void *d1, void *d2)
{
  const char *str = e->string;
  
  if (!editing_location)
       return; // generic commandline will move to tooldock
  
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
    ctx_queue_draw (e->ctx);
    commandline_cursor_start += strlen (str);
    commandline_cursor_end = commandline_cursor_start;
  }
}

static int malloc_trim_cb (Ctx *ctx, void *data)
{
#if GNU_C
  malloc_trim (64*1024);
#endif
  return 1;
}

static int dirtcb = 0;
static int delayed_dirt (Ctx *ctx, void *data)
{
  ctx_queue_draw (ctx);
  dirtcb = 0;
  return 0;
}

static int activate_from_start = 0;

static void clear_context_menu (void)
{
  for (int i = 0; i < context_menu_items * 3; i++)
  {
    if (context_menu[i]) { free (context_menu[i]);
       context_menu[i] = NULL;
    }
  }
  context_menu_items=0;
}

static void drop_event (CtxEvent *event, void *a, void *b)
{
        fprintf (stderr, "dropped :) %s\n", event->string);
}

static int card_files (ITK *itk_, void *data)
{
  itk = itk_;
  ctx = itk->ctx;
  //float em = itk_em (itk);
  //float row_height = em * 1.2;
  static int first = 1;

  int location_active = (focused_no == -1 && itk->focus_no == 0);
  if (item_context_active == 0)
  {
    clear_context_menu ();
    add_context_binding (1, "close", "", "escape");
  }
  else
  {
    for (int i = 0; i < context_menu_items; i++)
    {
      BIND_KEY(context_menu[i*3+2], context_menu[i*3+1], context_menu[i*3]);
    }
  }

  if (viewer)
  {
    int found = 0;
    for (CtxList *l = ctx_clients (ctx); l; l = l->next)
    {
      if (l->data == viewer)
        found = 1;
    }
    if (!found)
      viewer = NULL;
  }

  if (first)
  {
#if GNU_C
    ctx_add_timeout (ctx, 1000 * 200, malloc_trim_cb, NULL);
#endif
    ctx_add_timeout (ctx, 250, thumb_monitor, NULL);
    font_size = itk->font_size;
    first = 0;

#if 1
    //ctx_client_new (ctx, "bashtop", font_size * 5, font_size * 40, font_size*42, font_size*24, 0, strdup("boo2"), user_data_free);
#endif

    if (activate_from_start)
       argvs_eval ("activate");
  }
  //thumb_monitor (ctx, NULL);

  BIND_KEY ("F1", "toggle-keybindings-display", "toggle keybinding help");

  panel = itk_panel_start (itk, "", 0,0, ctx_width (ctx),
                                         ctx_height (ctx));

  if (dir_scale != 1.0f)
     ctx_scale (itk->ctx, dir_scale, dir_scale);


#if 0
    add_context_binding (location_active, "Zoom in", "zoom in", "shift-control-+");
    add_context_binding (location_active, "Zoom out", "zoom out", "shift-control--");
    add_context_binding (location_active, "Zoom reset", "zoom 1.0", "shift-control-0");
#endif
    add_context_binding (location_active, "larger text", "font-size up", "control-+");
    add_context_binding (location_active, "smaller text", "font-size down", "control--");
    add_context_binding (location_active, "reset text size", "font-size 14.0", "control-0");

    if (!is_text_editing() &&
        !item_context_active)
    {
      BIND_KEY ("shift-control-+", "zoom in", "zoom in");
      BIND_KEY ("shift-control-=", "zoom in", "zoom in");
      BIND_KEY ("shift-control--", "zoom out", "zoom out");
      BIND_KEY ("shift-control-0", "zoom 1.0", "zoom reset");

      BIND_KEY ("control-+", "font-size up", "larger text");
      BIND_KEY ("control-=", "font-size up", "larger text");
      BIND_KEY ("control--", "font-size down", "smaller text");
      BIND_KEY ("control-0", "font-size 14.0", "reset text size");

#if 0
      ctx_add_key_binding (ctx, "shift-control-+", NULL, "increase font size",
                      dir_font_up,
                      NULL);
      ctx_add_key_binding (ctx, "shift-control-=", NULL, "increase font size",
                      dir_font_up,
                      NULL);
      ctx_add_key_binding (ctx, "shift-control--", NULL, "decrease font size",
                      dir_font_down,
                      NULL);
#endif
      //ctx_add_key_binding (ctx, "control-0", NULL, "reset font size",
      //                 dir_zoom_reset,
      //                 NULL);
    }


  //if (!n)
  //{
  //  itk_labelf (itk, "no items\n");
 // }
 // else
  {
#if 1
    if (ctx_get_fullscreen (ctx))
      add_context_binding (location_active, "windowed", "toggle-fullscreen", "F11");
    else
      add_context_binding (location_active, "fullscreen", "toggle-fullscreen", "F11");
    if (!is_text_editing()
        && !text_editor
        && !item_context_active)
    {
      add_context_binding (location_active, "layout view", "view layout", "control-1");
      add_context_binding (location_active, "list view", "view list", "control-2");
      add_context_binding (location_active, "folder view", "view grid", "control-3");
      add_context_binding (location_active, "text-edit view", "view text-edit", "control-4");
    }
#endif

    if (!viewer)
      dir_layout (itk, diz);

    if (ctx_clients (ctx))
    {
      ctx_font_size (ctx, itk->font_size);
      ctx_clients_draw (ctx, 0);
    }
  }

  if (!is_text_editing() &&
      !item_context_active)
  {
    if (layout_show_page < layout_last_page)
      BIND_KEY("page-down", "page next", "next page");
    if (layout_show_page > 0)
      BIND_KEY("page-up", "page previous", "previous page");
  }

  if (!viewer
      && text_edit <= TEXT_EDIT_OFF
      && !layout_config.outliner 
      && !item_context_active)
  {

          if (commandline->str[0])
          {
            ctx_add_key_binding (ctx, "backspace", NULL, "remove char",
                            dir_backspace,
                            NULL);
            ctx_add_key_binding (ctx, "return", NULL, "run commandline",
                            dir_run_commandline,
                            NULL);

          ctx_add_key_binding (ctx, "left", NULL, "move cursor left", dir_location_left, NULL);
          ctx_add_key_binding (ctx, "right", NULL, "move cursor right", dir_location_right, NULL);

          ctx_add_key_binding (ctx, "control-a", NULL, "add char", dir_location_select_all, NULL);

          ctx_add_key_binding (ctx, "shift-left", NULL, "extend selection left", dir_location_extend_sel_left, NULL);
          ctx_add_key_binding (ctx, "shift-right", NULL, "extend selection right", dir_location_extend_sel_right, NULL);
#if 1
          BIND_KEY ("up", "", "<ignored>");
          BIND_KEY ("down", "", "<ignored>");
#else
            ctx_add_key_binding (ctx, "up", NULL, NULL,
                            dir_ignore,
                            NULL);
            ctx_add_key_binding (ctx, "down", NULL, NULL,
                            dir_ignore,
                            NULL);
#endif
          }

          if (!item_context_active)
            ctx_add_key_binding (ctx, "any", NULL, NULL, dir_any, NULL);
          if (editing_location)
          {
            ctx_add_key_binding (ctx, "escape", NULL, "stop editing location", dir_location_escape, NULL);
            ctx_add_key_binding (ctx, "return", NULL, "confirm new location", dir_location_return, NULL);
            ctx_add_key_binding (ctx, "down", NULL, NULL, dir_location_return, NULL);
            ctx_add_key_binding (ctx, "tab", NULL, NULL, ui_run_command, "");
            ctx_add_key_binding (ctx, "shift-tab", NULL, NULL, ui_run_command, "");
            ctx_add_key_binding (ctx, "alt-up", NULL, NULL, ui_run_command, "");
          }
          if (!editing_location)
          {
            add_context_binding (location_active, "change location", "location-edit", "control-l");
            add_context_binding (location_active, "quit", "quit", "control-q");
          }

          if (diz_dir_type_atom (diz, focused_no) == CTX_ATOM_TEXT
              && diz_dir_get_float (diz, focused_no, "x", -1234.0) == -1234.0
              && !is_text_editing()
              && !item_context_active)
          {

            if (layout_focused_link >= 0)
              BIND_KEY ("up", "focus-previous-link", "focus previous link");
            else
            {
              //BIND_KEY ("up", "focus-previous", "focus previous");
              if (vim_keys)
              BIND_KEY ("k", "focus-previous", "focus previous");
            }

            //fprintf (stderr, "%i \n", dir_item_count_links (focused_no));

            if (focused_no >=0 &&
                layout_focused_link < dir_item_count_links (focused_no)-1)
              BIND_KEY ("down", "focus-next-link", "focus next link");
            else
            {
              //BIND_KEY ("down", "focus-next", "focus next");
            }
            if (vim_keys)
              BIND_KEY ("j", "focus-next", "focus next");

            if (!text_editor && focused_no >= 0)
            {
              if (diz_dir_has_children (diz, focused_no) && 
                  !diz_dir_get_int (diz, focused_no, "folded", 0))
                BIND_KEY ("left", "outline-fold", "fold");
              if (layout_focused_link >= 0)
                 BIND_KEY ("right", "follow-link", "follow link");
              else
              {
                if (diz_dir_has_children (diz, focused_no))
                {
                  if (diz_dir_get_int (diz, focused_no, "folded", 0))
                    BIND_KEY ("right", "outline-expand", "expand");
                }
                else
                  BIND_KEY ("right", "outline-focus-first-child", "enter children");
                  // enter-children creates child if it doesnt exist
              }

              if (vim_keys)
              {
                BIND_KEY ("h", "outline-fold", "fold");
                if (layout_focused_link >= 0)
                   BIND_KEY ("l", "follow-link", "follow link");
                else
                {
                  if (diz_dir_has_children (diz, focused_no))
                    BIND_KEY ("l", "outline-expand", "expand");
                  else
                    BIND_KEY ("l", "outline-focus-first-child", "enter children");
                  // enter-children creates child if it doesnt exist
                }
              }
            }
          }
  }


#if 1
      if (!viewer &&
          text_edit>TEXT_EDIT_OFF
          && !item_context_active)
      {
          ctx_add_key_binding (ctx, "tab", NULL, NULL,
                          ignore_and_stop_propagate,
                          NULL);
          ctx_add_key_binding (ctx, "shift-tab", NULL, NULL,
                          ignore_and_stop_propagate,
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
          ctx_add_key_binding (ctx, "alt-up", NULL, "stop editing",
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

          ctx_add_key_binding (ctx, "control-a", NULL, "go to start of paragraph",
                          text_edit_home,
                          NULL);
          ctx_add_key_binding (ctx, "control-e", NULL, "go to end of paragraph",
                          text_edit_end,
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

  if (item_context_active && focused_no>=-1)
  {
    float em = itk->font_size;
    float width = em * 20;
    float x;
    float y;
    float height = em * (context_menu_items + 3);

    if (focused_control && focused_no > -1)
    {
    x = focused_control->x + focused_control->width;
    y = focused_control->y;
    if (width < focused_control->width)
    {
       // menu thinner than item itself, place menu on item        
       x = focused_control->x + focused_control->width/2 - width/2;
    }
    else
    {
       if (x + width > itk->width)
       {
         x = focused_control->x - width;
       }
    }
    }
    else
    {
      x = 10 * em;
      y = 4 * em;
    }

    if (y + height - itk->panel->scroll> ctx_height (ctx))
        y = ctx_height (ctx)-height + itk->panel->scroll;

    ctx_add_key_binding (ctx, "up", NULL, "previous choice",
                    item_context_choice_prev,
                    NULL);
    ctx_add_key_binding (ctx, "down", NULL, "next choice",
                    item_context_choice_next,
                    NULL);
    ctx_add_key_binding (ctx, "return", NULL, "make choice",
                    item_context_make_choice,
                    NULL);
    ctx_add_key_binding (ctx, "left", NULL, NULL,
                    ignore_and_stop_propagate,
                    NULL);
    ctx_add_key_binding (ctx, "right", NULL, NULL,
                    ignore_and_stop_propagate,
                    NULL);
    ctx_add_key_binding (ctx, "delete", NULL, NULL,
                    ignore_and_stop_propagate,
                    NULL);
    ctx_add_key_binding (ctx, "tab", NULL, NULL,
                    ignore_and_stop_propagate,
                    NULL);
    ctx_add_key_binding (ctx, "shift-tab", NULL, NULL,
                    ignore_and_stop_propagate,
                    NULL);

    ctx_begin_path (itk->ctx);
    ctx_rgba (itk->ctx, 0.0,0.0,0.0, 0.4);
    ctx_rectangle (itk->ctx, 0, itk->panel->scroll,
                   ctx_width (itk->ctx), ctx_height (itk->ctx));
    ctx_fill (itk->ctx);
    ctx_rgba (itk->ctx, 0.0, 0.0, 0.0, 0.8);
    ctx_rectangle (itk->ctx, x, y, width, height);

    ctx_fill (itk->ctx);

    for (int i = 0; context_menu[i]; i+=3)
    {
      if (i == item_context_choice * 3)
         ctx_rgb(itk->ctx, 1,1,1);
      else
         ctx_rgb(itk->ctx, 0.7,0.7,0.7);
      y+=em;
      ctx_move_to (itk->ctx, x+em, y);
      ctx_text( itk->ctx, context_menu[i]);
      ctx_save( itk->ctx);
      ctx_global_alpha( itk->ctx, 0.4);
      ctx_text( itk->ctx, "(");
      ctx_text( itk->ctx, context_menu[i+2]);
      ctx_text( itk->ctx, ")");
      ctx_restore( itk->ctx);
    }
         ctx_rgb(itk->ctx, 0.7,0.7,0.7);
    {
      int keys = diz_dir_key_count (diz, focused_no);

      for (int k = 0; k < keys; k++)
      {
         char *key = diz_dir_key_name (diz, focused_no, k);
         char *value = diz_dir_get_string (diz, focused_no, key);
         char tmp[strlen(key)+strlen(value)+1+1];
         sprintf (tmp, "%s=%s", key, value);
         y+=em;
         ctx_move_to (itk->ctx, x+em, y);
         ctx_text (itk->ctx, tmp);
      }
    }

    if (focused_no>=0)
    {
      char *name = diz_dir_get_name (diz, focused_no);
      char *full_path = ctx_strdup_printf ("%s/%s", diz->path, name);
      y+=em;
      ctx_move_to (itk->ctx, x+em, y);
      ctx_text (itk->ctx, ctx_path_get_media_type (full_path));
      free (full_path);
      free (name);
    }


#if 0
  itk_panel_start (itk, "prop",
                  
       focused_control->x + focused_control->width, focused_control->y, 200, 200);
                  
       //           ctx_width(ctx)/4, itk->font_size*1, ctx_width(ctx)/4, itk->font_size*8);
  int keys = diz_dir_key_count (diz->items[focused_no]);
  itk_labelf (itk, "%s - %i", diz->items[focused_no], keys);
  for (int k = 0; k < keys; k++)
  {
    char *key = diz_dir_key_name (diz->items[focused_no], k);
    if (key)
    {
      char *val = diz_dir_get_string (diz->items[focused_no], key);
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
    itk_toggle (itk, "show-label", &layout_config.label);

    itk_panel_end (itk);
  }
#endif

  static unsigned long viewer_slide_prev_time = 0;
  unsigned long viewer_slide_time = ctx_ticks ();
  long elapsed = viewer_slide_time - viewer_slide_prev_time;
  viewer_slide_prev_time = viewer_slide_time;


  if (viewer || stuff_stdout_is_running ())
  {
    ctx_listen (ctx, CTX_KEY_PRESS, dir_key_any, NULL, NULL);
    ctx_listen (ctx, CTX_KEY_DOWN,  dir_key_any, NULL, NULL);
    ctx_listen (ctx, CTX_KEY_UP,    dir_key_any, NULL, NULL);

    if (viewer_slideshow && viewer_slide_prev_time)
    {
      float fade_duration = diz_dir_get_float (diz, focused_no, "fade", 0.0f);
      long int fade_ticks = 1000 * 1000 * fade_duration;

      if (viewer_slide_duration >= 0)
      {
        viewer_slide_duration -= elapsed;
        if (viewer_slide_duration < 0)
        {
          viewer_load_next (ctx, NULL);   
        }
#if 1
        else if (viewer_slide_duration < fade_ticks)
        {
          float opacity = (fade_ticks - viewer_slide_duration) / (1.0 * fade_ticks);
          char *name = diz_dir_get_name (diz, focused_no+1);
          char *pathA = ctx_strdup_printf ("%s/%s", diz->path, name);
          free (name);
          char *client_a = ctx_strdup_printf ("%s-%i", pathA, focused_no+1);
          free (pathA);
          CtxClient *client;
          if ((client=ctx_client_find (ctx, client_a)))
          {
            ctx_client_move (ctx, ctx_client_id (client), 0, 0);
            ctx_client_resize (ctx, ctx_client_id (client), ctx_width (ctx), ctx_height (ctx));
            ctx_client_raise_top (ctx, ctx_client_id (client));
            ctx_client_set_opacity (ctx, ctx_client_id (client), opacity);
            //if (!dirtcb)
            //  dirtcb = ctx_add_timeout (ctx, 50, delayed_dirt, NULL);
            ctx_queue_draw (ctx);
          }
          else
          {
            if (!dirtcb)
              dirtcb = ctx_add_timeout (ctx, 200, delayed_dirt, NULL);
          }
        }
#endif
        else
        { /* to keep events being processed, without it we need to
             wiggle mouse on images
           */
          if (!dirtcb)
            dirtcb = ctx_add_timeout (ctx, 250, delayed_dirt, NULL);
        }
      }
    }
  }
  else
  {
  }


  if (viewer) 
  {
    ctx_font_size (ctx, itk->font_size);
    ctx_clients_draw (ctx, 0);
    int found = 0;
    for (CtxList *c = ctx_clients (ctx); c; c = c->next)
    {
      if (c->data == viewer)
        found = 1;
    }
    if (!found)
    {
      CtxEvent fake_event;
      fake_event.ctx = ctx;
      deactivate_viewer (&fake_event, NULL, NULL);
      viewer = NULL;
    }

  }
  else
  {

    for (CtxList *c = ctx_clients (ctx); c; c = c->next)
    {
      CtxClient *client = c->data;
      if (ctx_client_flags (client) & ITK_CLIENT_LAYER2)
      {
        //VT *vt = ctx_client_vt (client);
        //int line_count = vt_get_cursor_y (vt) + 1;
        //line_count += vt_get_scrollback_lines (vt);

        //fprintf (stderr, "%p %i %i\n", vt, ctx_client_id (client),  line_count);
          
        //if (line_count > 2)
        //ctx_client_resize (ctx_client_id (client), client->width,
        //                   itk->font_size * (line_count));
      }
    }

    ctx_clients_draw (ctx, 1);
  }

#if 0
  if (!text_editor)
  {
  ctx_save (ctx);
  ctx_translate (ctx, toolbar_x, toolbar_y);

  // toolbar
  {
    float em = itk->font_size;

    int n_tools = 5;

    if (viewer) n_tools = 3;

    if (tool_no == 1) n_tools = 2;

    ctx_rectangle (ctx, 0, 0, 2.5 * (n_tools + 0.25) * em, 3 * em);
    ctx_rgba (ctx, 1,1,1, 0.1);
    ctx_fill (ctx);

    for (int i = 0; i < n_tools; i ++)
    {
      ctx_rectangle (ctx, (2.5 * i + 0.5) * em,
                          0.5 * em,
                          2 * em, 2 * em);
      ctx_listen (ctx, CTX_DRAG, tool_drag, (void*)((size_t)i), NULL);
      if (i == tool_no)
        ctx_rgba (ctx, 1,1,1, 0.3);
      else
        ctx_rgba (ctx, 1,1,1, 0.05);
      ctx_fill (ctx);
    }

  }

  if (tool_no == STUFF_TOOL_LOCATION)
  {
    float em = itk->font_size;
    ctx_save (ctx);
    ctx_rectangle (ctx, 6 * em, 0, ctx_width (ctx) - 3 * em, 3 * em);
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

      ctx_rectangle (ctx, 6.4 * em + sel_start, 1.5 * em - em,
                          sel_end-sel_start,em);
      if (c_start==c_end)
        ctx_rgba (ctx, 1,1, 0.2, 1);
      else
        ctx_rgba (ctx, 0.5,0, 0, 1);
      ctx_fill (ctx);
      ctx_move_to (ctx, 6.4 * em, 1.5 * em);
      ctx_rgba (ctx, 1,1,1, 0.6);
      ctx_text (ctx, commandline->str);
    }
    else
    {
      ctx_rgba (ctx, 1,1,1, 0.6);
      ctx_move_to (ctx, 6.4 * em, 1.5 * em);
      if (diz->title)
        ctx_text (ctx, diz->title);
      else
        ctx_text (ctx, diz->path);
    }
    ctx_restore (ctx);
  }
  // pages
  if (tool_no == STUFF_TOOL_LOCATION)
  {
    float em = itk->font_size * 1.4;
/*
    ctx_rectangle (ctx, ctx_width (ctx) - 3 * em, 0, 3 * em, ctx_height (ctx));
    ctx_rgba (ctx, 1,1,1, 0.1);
    ctx_fill (ctx);
    */

    for (int i = 0; i < layout_last_page + 1; i ++)
    {
      ctx_rectangle (ctx, 0.5 * em, (3 * (1+i) + 0.5) * em,  2 * em, 2 * em);
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
  ctx_restore (ctx);
  }
#endif

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


  if (show_keybindings && !viewer)
  {
    float bindings_height = ctx_height (ctx) * 0.3;
    float bindings_pos = ctx_height (ctx) - bindings_height;
    float em = itk->font_size * 0.66;
    ctx_save (ctx);
    //ctx_font (ctx, "regular");
    ctx_font_size (ctx, em);
    ctx_line_width (ctx, em*0.05);
    ctx_rgba (ctx, 0,0,0,0.9);
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
          //else if (!strcmp (n, "backspace")) { n = "⌫"; }
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

  // drop target
  ctx_begin_path (ctx);
  ctx_rectangle (ctx, 0,0, ctx_width (ctx), ctx_height (ctx));
  ctx_listen (ctx, CTX_DROP, drop_event, NULL, NULL);
  ctx_begin_path (ctx);
  return 1;
}


int stuff_main (int argc, char **argv)
{
  //setenv ("CTX_SHAPE_CACHE", "0", 1);
  diz = diz_dir_new ();
  const char *path = NULL;
  
  for (int i = 1; argv[i]; i++)
  {
    if (argv[i][0]!='-')
    {
      if (!path) path = argv[i];
    }
    else
    {
      if (!strcmp (argv[i], "-E"))
      {
        text_editor = 1;
      }
    }
  }

  if (!path)
  {
    fprintf (stderr, "need a path argument");
    exit(1);
  }

  if (path && strchr (path, ':'))
  {
    path = strchr (path, ':') + 1;
    if (path[1] == '/') path++;
    if (path[1] == '/') path++;
  }
  commandline = ctx_string_new ("");

  if (text_editor)
  {
    set_text_edit (NULL, NULL, NULL);
    set_location (path);
    focused_no = 0;
    layout_find_item = focused_no;
  }
  else
  {
    set_layout (NULL, NULL, NULL);

    if (path_is_dir (path))
    {
      set_location (path);
      focused_no = -1;
      layout_find_item = focused_no;
    }
    else
    {

    char *dir = strdup (path);
    char *name = NULL;
    if (strrchr (dir, '/'))
    {
      name = strrchr (dir, '/')+1;
      strrchr (dir, '/')[0] = 0;
    }
    else
    {
      name = NULL;
    }
    set_location (dir);
    focused_no = diz_dir_name_to_no (diz, name);
    layout_find_item = focused_no;

    if (name && name[0])
    {
      activate_from_start = 1;
    }
    free (dir);
    }
  }

  itk_main (card_files, NULL);


  //fprintf (stderr, "parents: %i\n",
  //                diz_dir_value_count (diz, -1, "parent"));

  save_metadata ();

  ctx_string_free (commandline, 1);
  diz_dir_destroy (diz);

  return 0;
}


#include <libgen.h>


void ctx_screenshot (Ctx *ctx, const char *path);

int stuff_make_thumb (const char *src_path, const char *dst_path)
{
   diz = diz_dir_new ();
   ctx_init (NULL, NULL);
   int width = 256;
   int height = 256;
   float font_size = height * 0.075;
   float live_font_factor = 1.0;
   Ctx *ctx = ctx_new (width, height, "headless");
   // XXX : broken/glitched thumbnails with "drawlist" it should
   // be possible to use just that though
   char *dir = dirname (strdup(src_path));
   char *base = strdup(basename (strdup(src_path)));
   diz_dir_set_path (diz, dir);
   drop_item_renderers (ctx);
   char **command = dir_get_viewer_argv (src_path, diz_dir_name_to_no (diz, base));

   if (!command)
      return -1;

   CtxClient *client = ctx_client_new_argv (ctx, command,
                  0, 0, width, height,
                  font_size * live_font_factor,
                  ITK_CLIENT_PRELOAD,
                  NULL, NULL);

   int count = 0;
   int got_content = 0;
   int slept_time = 0;
   int sleep_time = 500;
   for (int i = 0; slept_time < 1000 * 3500 && got_content < 2; i ++)
   {
     //CtxEvent *event = NULL;
     ctx_start_frame (ctx);
     ctx_rectangle (ctx, 0, 0, width, height);
     ctx_gray (ctx, 0.0f);
     ctx_fill (ctx);
     ctx_clients_draw (ctx, 0);
     ctx_get_drawlist (ctx, &count);
     if (count > 120 || got_content) // formulated like this,
        got_content++;                // to catch the tiny size of correctly cached
     ctx_end_frame (ctx);             // textures

     while(ctx_get_event (ctx));
     ctx_clients_handle_events (ctx);
     sleep_time *= 1.5;
     usleep (sleep_time);
     slept_time += sleep_time;
   }
   ctx_screenshot (ctx, dst_path);
   ctx_client_remove (ctx, client);
   ctx_destroy (ctx);
   diz_dir_destroy (diz);

   return 0;
}
