/* DEC terminals/xterm family terminal with ANSI, utf8, vector graphics and
 * audio.
 *
 * Copyright (c) 2014, 2016, 2018, 2020 Øyvind Kolås <pippin@gimp.org>
 *
 * Adhering to the standards with modern extensions.
 *
 * Features:
 *     vt100 - 101 points on scoresheet
 *     UTF8, cp437
 *     dim, bold, strikethrough, underline, italic, reverse
 *     ANSI colors, 256 colors (non-redefineable), 24bit color
 *     realtime audio transmission
 *     raster sprites (kitty spec)
 *     vector graphics
 *     vt320 - horizontal margins
 *     proportional fonts
 *
 *     BBS/ANSI-art mode
 *
 * 8bit clean
 *
 *
 * Todo:
 *     DECCIR - cursor state report https://vt100.net/docs/vt510-rm/DECCIR.html
 *     make absolute positioning take proportional into account
 *     HTML / PNG / SVG / PDF export of scrollback / screen
 *     sixels
 *
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>

#include "ctx.h"


#define CTX_VT_USE_FRAMEDIFF 0  // is a larger drain than neccesary when everything is per-byte?
                                // is anyways currently disabled also in ctx

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

//#include "vt-line.h"
//#include "vt.h"
//#include "ctx-clients.h"



#define VT_LOG_INFO     (1<<0)
#define VT_LOG_CURSOR   (1<<1)
#define VT_LOG_COMMAND  (1<<2)
#define VT_LOG_WARNING  (1<<3)
#define VT_LOG_ERROR    (1<<4)
#define VT_LOG_INPUT    (1<<5)
#define VT_LOG_ALL       0xff

static int vt_log_mask = VT_LOG_INPUT;
//static int vt_log_mask = VT_LOG_WARNING | VT_LOG_ERROR;// | VT_LOG_COMMAND;// | VT_LOG_INFO | VT_LOG_COMMAND;
//static int vt_log_mask = VT_LOG_WARNING | VT_LOG_ERROR | VT_LOG_INFO | VT_LOG_COMMAND | VT_LOG_INPUT;
//static int vt_log_mask = VT_LOG_ALL;

#if 0
#define vt_log(domain, fmt, ...)

#define VT_input(str, ...)
#define VT_info(str, ...)
#define VT_command(str, ...)
#define VT_cursor(str, ...)
#define VT_warning(str, ...)
#define VT_error(str, ...)
#else
#define vt_log(domain, line, a...) \
        do {fprintf (stderr, "%i %s ", line, domain);fprintf(stderr, ##a);fprintf(stderr, "\n");}while(0)
#define VT_info(a...) if (vt_log_mask & VT_LOG_INFO) vt_log ("INFO  ", __LINE__, ##a)
#define VT_input(a...) if (vt_log_mask & VT_LOG_INPUT) vt_log ("INPUT ", __LINE__, ##a)
#define VT_command(a...) if (vt_log_mask & VT_LOG_COMMAND) vt_log ("CMD   ", __LINE__, ##a)
#define VT_cursor(a...) if (vt_log_mask & VT_LOG_CURSOR) vt_log ("CURSOR",__LINE__, ##a)
#define VT_warning(a...) if (vt_log_mask & VT_LOG_WARNING) vt_log ("WARN  ",__LINE__, ##a)
#define VT_error(a...) if (vt_log_mask & VT_LOG_ERROR) vt_log ("ERROR",__LINE__, ##a)

#endif

#ifndef MIN
#define MIN(a,b)  ((a)<(b)?(a):(b))
#endif

static void vt_state_neutral      (VT *vt, int byte);
static void vt_state_esc          (VT *vt, int byte);
static void vt_state_osc          (VT *vt, int byte);
static void vt_state_apc          (VT *vt, int byte);
static void vt_state_apc_generic  (VT *vt, int byte);
static void vt_state_sixel        (VT *vt, int byte);
static void vt_state_esc_sequence (VT *vt, int byte);
static void vt_state_esc_foo      (VT *vt, int byte);
static void vt_state_swallow      (VT *vt, int byte);
static void vt_state_ctx          (VT *vt, int byte);
static void vt_state_vt52         (VT *vt, int byte);

#if 0
/* barebones linked list */

typedef struct _CtxList CtxList;
struct _CtxList
{
  void *data;
  CtxList *next;
};

static inline int ctx_list_length (CtxList *list)
{
  int length = 0;
  for (CtxList *l = list; l; l = l->next, length++);
  return length;
}

static inline void ctx_list_prepend (CtxList **list, void *data)
{
  CtxList *new_=calloc (sizeof (CtxList), 1);
  new_->next = *list;
  new_->data = data;
  *list = new_;
}

static inline void *ctx_list_last (CtxList *list)
{
  if (list)
    {
      CtxList *last;
      for (last = list; last->next; last=last->next);
      return last->data;
    }
  return NULL;
}

static inline void ctx_list_append (CtxList **list, void *data)
{
  CtxList *new_= calloc (sizeof (CtxList), 1);
  new_->data=data;
  if (*list)
    {
      CtxList *last;
      for (last = *list; last->next; last=last->next);
      last->next = new_;
      return;
    }
  *list = new_;
  return;
}

static inline void ctx_list_remove (CtxList **list, void *data)
{
  CtxList *iter, *prev = NULL;
  if ( (*list)->data == data)
    {
      prev = (void *) (*list)->next;
      free (*list);
      *list = prev;
      return;
    }
  for (iter = *list; iter; iter = iter->next)
    if (iter->data == data)
      {
        prev->next = iter->next;
        free (iter);
        break;
      }
    else
      { prev = iter; }
}

static inline void
ctx_list_insert_before (CtxList **list, CtxList *sibling,
                       void *data)
{
  if (*list == NULL || *list == sibling)
    {
      ctx_list_prepend (list, data);
    }
  else
    {
      CtxList *prev = NULL;
      for (CtxList *l = *list; l; l=l->next)
        {
          if (l == sibling)
            { break; }
          prev = l;
        }
      if (prev)
        {
          CtxList *new_=calloc (sizeof (CtxList), 1);
          new_->next = sibling;
          new_->data = data;
          prev->next=new_;
        }
    }
}
#endif


typedef enum
{
  STYLE_REVERSE         = 1 << 0,
  STYLE_BOLD            = 1 << 1,
  STYLE_BLINK           = 1 << 2,
  STYLE_UNDERLINE       = 1 << 3,
  STYLE_DIM             = 1 << 4,
  STYLE_HIDDEN          = 1 << 5,
  STYLE_ITALIC          = 1 << 6,
  STYLE_UNDERLINE_VAR   = 1 << 7,
  STYLE_STRIKETHROUGH   = 1 << 8,
  STYLE_OVERLINE        = 1 << 9,
  STYLE_BLINK_FAST      = 1 << 10,
  STYLE_PROPORTIONAL    = 1 << 11,
  STYLE_FG_COLOR_SET    = 1 << 12,
  STYLE_BG_COLOR_SET    = 1 << 13,
  STYLE_FG24_COLOR_SET  = 1 << 14,
  STYLE_BG24_COLOR_SET  = 1 << 15,
  //STYLE_NONERASABLE     = 1 << 16  // needed for selective erase
} TerminalStyle;

typedef struct Image
{
  int kitty_format;
  int width;
  int height;
  int id;
  int eid_no;
  int size;
  uint8_t *data;
} Image;

#define MAX_IMAGES 128

static Image image_db[MAX_IMAGES]= {{0,},};

static Image *image_query (int id)
{
  for (int i = 0; i < MAX_IMAGES; i++)
    {
      Image *image = &image_db[i];
      if (image->id == id)
        { return image; }
    }
  return NULL;
}

static int image_eid_no = 0;

static CtxList *ctx_vts;
static Image *image_add (int width,
                         int height,
                         int id,
                         int format,
                         int size,
                         uint8_t *data)
{
  // look for id if id is not 0
  Image *image;
  for (int i = 0; i < MAX_IMAGES; i++)
    {
      image = &image_db[i];
      if (image->data == NULL)
        { break; }
    }
  if (image->data)
    {
      // not a good eviction strategy
      image = &image_db[random() %MAX_IMAGES];
    }
  if (image->data)
    { free (image->data); }
  image->kitty_format = format;
  image->width  = width;
  image->height = height;
  image->id     = id;
  image->size   = size;
  image->data   = data;
  image->eid_no = image_eid_no++;
  return image;
}

void vtpty_resize (void *data, int cols, int rows, int px_width, int px_height)
{
  VtPty *vtpty = data;
  struct winsize ws;
  ws.ws_row = rows;
  ws.ws_col = cols;
  ws.ws_xpixel = px_width;
  ws.ws_ypixel = px_height;
  ioctl (vtpty->pty, TIOCSWINSZ, &ws);
}

ssize_t vtpty_write (void *data, const void *buf, size_t count)
{
  VtPty *vtpty = data;
  return write (vtpty->pty, buf, count);
}

ssize_t vtpty_read (void  *data, void *buf, size_t count)
{
  VtPty *vtpty = data;
  return read (vtpty->pty, buf, count);
}

int vtpty_waitdata (void  *data, int timeout)
{
  VtPty *vtpty = data;
  struct timeval tv;
  fd_set fdset;
  FD_ZERO (&fdset);
  FD_SET (vtpty->pty, &fdset);
  tv.tv_sec = 0;
  tv.tv_usec = timeout;
  tv.tv_sec  = timeout / 1000000;
  tv.tv_usec = timeout % 1000000;
  if (select (vtpty->pty+1, &fdset, NULL, NULL, &tv) == -1)
    {
      perror ("select");
      return 0;
    }
  if (FD_ISSET (vtpty->pty, &fdset) )
    {
      return 1;
    }
  return 0;
}


/* on current line */
static int vt_col_to_pos (VT *vt, int col)
{
  int pos = col;
  if (vt->current_line->contains_proportional)
    {
      Ctx *ctx = _ctx_new_drawlist (vt->width, vt->height);
      ctx_font (ctx, "regular");
      ctx_font_size (ctx, vt->font_size);
      int x = 0;
      pos = 0;
      int prev_prop = 0;
      while (x <= col * vt->cw)
        {
          if (vt_line_get_style (vt->current_line, pos) & STYLE_PROPORTIONAL)
            {
              x += ctx_glyph_width (ctx, vt_line_get_unichar (vt->current_line, pos) );
              prev_prop = 1;
            }
          else
            {
              if (prev_prop)
                {
                  int new_cw = vt->cw - ( (x % vt->cw) );
                  if (new_cw < vt->cw*3/2)
                    { new_cw += vt->cw; }
                  x += new_cw;
                }
              else
                {
                  x += vt->cw;
                }
              prev_prop = 0;
            }
          pos ++;
        }
      pos --;
      ctx_free (ctx);
    }
  return pos;
}

static int vt_margin_left (VT *vt)
{
  int left = vt->left_right_margin_mode?vt->margin_left:1;
  return vt_col_to_pos (vt, left);
}

#define VT_MARGIN_LEFT vt_margin_left(vt)

static int vt_margin_right (VT *vt)
{
  int right = vt->left_right_margin_mode?vt->margin_right:vt->cols;
  return vt_col_to_pos (vt, right);
}

#define VT_MARGIN_RIGHT vt_margin_right(vt)

static void vtcmd_reset_to_initial_state (VT *vt, const char *sequence);
int vt_set_prop (VT *vt, uint32_t key_hash, const char *val);
uint32_t ctx_strhash (const char *utf8);

static void vt_set_title (VT *vt, const char *new_title)
{
  if (vt->inert) return;
  if (vt->title)
    { free (vt->title); }
  vt->title = strdup (new_title);
  vt_set_prop (vt, ctx_strhash ("title"), (char*)new_title);
}

const char *vt_get_title (VT *vt)
{
  return vt->title;
}

static void vt_run_command (VT *vt, const char *command, const char *term);
static void vtcmd_set_top_and_bottom_margins (VT *vt, const char *sequence);
static void vtcmd_set_left_and_right_margins (VT *vt, const char *sequence);
static void _vt_move_to (VT *vt, int y, int x);

static void vtcmd_clear (VT *vt, const char *sequence)
{
  while (vt->lines)
    {
      vt_line_free (vt->lines->data, 1);
      ctx_list_remove (&vt->lines, vt->lines->data);
    }
  vt->lines = NULL;
  vt->line_count = 0;

  if (1)
  { /* TODO: detect if this is neccesary.. due to images present
             in lines in scrollback */
    for (int i=0; i<vt->rows; i++)
    {
      vt->current_line = vt_line_new_with_size ("", vt->cols);
      ctx_list_prepend (&vt->scrollback, vt->current_line);
      vt->scrollback_count++;
    }
  }

  /* populate lines */
  for (int i=0; i<vt->rows; i++)
    {
      vt->current_line = vt_line_new_with_size ("", vt->cols);
      ctx_list_prepend (&vt->lines, vt->current_line);
      vt->line_count++;
    }
}

#define set_fg_rgb(r, g, b) \
    vt->cstyle ^= (vt->cstyle & (((uint64_t)((1l<<24)-1))<<16));\
    vt->cstyle |=  ((uint64_t)(r)<<16);\
    vt->cstyle |=  ((uint64_t)(g)<<(16+8));\
    vt->cstyle |=  ((uint64_t)(b)<<(16+8+8));\
    vt->cstyle |= STYLE_FG_COLOR_SET;\
    vt->cstyle |= STYLE_FG24_COLOR_SET;\

#define set_bg_rgb(r, g, b) \
    vt->cstyle ^= (vt->cstyle & (((uint64_t)((1l<<24)-1))<<40));\
    vt->cstyle |=  ((uint64_t)(r)<<40);\
    vt->cstyle |=  ((uint64_t)(g)<<(40+8));\
    vt->cstyle |=  ((uint64_t)(b)<<(40+8+8));\
    vt->cstyle |= STYLE_BG_COLOR_SET;\
    vt->cstyle |= STYLE_BG24_COLOR_SET;\

#define set_fg_idx(idx) \
    vt->cstyle ^= (vt->cstyle & (((uint64_t)((1l<<24)-1))<<16));\
    vt->cstyle ^= (vt->cstyle & STYLE_FG24_COLOR_SET);\
    vt->cstyle |=  ((idx)<<16);\
    vt->cstyle |= STYLE_FG_COLOR_SET;

#define set_bg_idx(idx) \
    vt->cstyle ^= (vt->cstyle & (((uint64_t)((1l<<24)-1))<<40));\
    vt->cstyle ^= (vt->cstyle & STYLE_BG24_COLOR_SET);\
    vt->cstyle |= ((int64_t)(idx)<<40) ;\
    vt->cstyle |= STYLE_BG_COLOR_SET;


static void _vt_compute_cw_ch (VT *vt)
{
  vt->cw = (vt->font_size / vt->line_spacing * vt->scale_x) + 0.99;
  vt->ch = vt->font_size;
}

static void vtcmd_set_132_col (VT  *vt, int set)
{
  // this should probably force the window as well
  if (set == 0 && vt->scale_x == 1.0f) return;
  if (set == 1 && vt->scale_x != 1.0f) return;
  if (set) // 132 col
    {
      vt->scale_x = 74.0/132.0; // show all - po
      //vt->scale_x = 80.0/132.0;
      vt->scale_y = 1.0;
      _vt_compute_cw_ch (vt);
      vt_set_term_size (vt, vt->cols * 132/80.0, vt->rows);
    }
  else // 80 col
    {
      vt->scale_x = 1.0;
      vt->scale_y = 1.0;
      _vt_compute_cw_ch (vt);
      vt_set_term_size (vt, vt->cols * 80/132.0, vt->rows);
    }
}

static void vt_line_feed (VT *vt);
static void vt_carriage_return (VT *vt);

static int vt_trimlines (VT *vt, int max);
static void vtcmd_reset_to_initial_state (VT *vt, const char *sequence)
{
  VT_info ("reset %s", sequence);
  if (getenv ("VT_DEBUG") )
    { vt->debug = 1; }
  vtcmd_clear (vt, sequence);
  vt->encoding = 0;
  vt->bracket_paste = 0;
  vt->ctx_events = 0;
  vt->cr_on_lf = 0;
  vtcmd_set_top_and_bottom_margins (vt, "[r");
  vtcmd_set_left_and_right_margins (vt, "[s");
  vt->autowrap               = 1;
  vt->justify                = 0;
  vt->cursor_visible         = 1;
  vt->charset[0]             = 0;
  vt->charset[1]             = 0;
  vt->charset[2]             = 0;
  vt->charset[3]             = 0;
  vt->bell                   = 3;
  vt->scale_x                = 1.0;
  vt->scale_y                = 1.0;
  vt->saved_x                = 1;
  vt->saved_y                = 1;
  vt->saved_style            = 1;
  vt->reverse_video          = 0;
  vt->cstyle                 = 0;
  vt->keyrepeat              = 1;
  vt->cursor_key_application = 0;
  vt->argument_buf_len       = 0;
  vt->argument_buf[0]        = 0;
  vt->vtpty.done             = 0;
  vt->result                 = -1;
  vt->state                  = vt_state_neutral;
  vt->scroll_on_output       = 0;
  vt->scroll_on_input        = 1;
  vt->unit_pixels            = 0;
  vt->mouse                  = 0;
  vt->mouse_drag             = 0;
  vt->mouse_all              = 0;
  vt->mouse_decimal          = 0;
  _vt_compute_cw_ch (vt);
  for (int i = 0; i < MAX_COLS; i++)
    { vt->tabs[i] = i % 8 == 0? 1 : 0; }
  _vt_move_to (vt, vt->margin_top, vt->cursor_x);
  vt_carriage_return (vt);
  //if (vt->ctx)
  //  { ctx_reset (vt->ctx); }
  vt->audio.bits = 8;
  vt->audio.channels = 1;
  vt->audio.type = 'u';
  vt->audio.samplerate = 8000;
  vt->audio.buffer_size = 1024;
  vt->audio.encoding = 'a';
  vt->audio.compression = '0';
  vt->audio.mic = 0;
  while (vt->scrollback)
    {
      vt_line_free (vt->scrollback->data, 1);
      ctx_list_remove (&vt->scrollback, vt->scrollback->data);
    }
  vt->scrollback_count = 0;
}

void vt_set_font_size (VT *vt, float font_size)
{
  vt->font_size = font_size;
  _vt_compute_cw_ch (vt);
}

float       vt_get_font_size      (VT *vt)
{
  return vt->font_size;
}

void vt_set_line_spacing (VT *vt, float line_spacing)
{
  vt->line_spacing = line_spacing;
  _vt_compute_cw_ch (vt);
}


static void ctx_clients_signal_child (int signum)
{
  pid_t pid;
  int   status;
  if ( (pid = waitpid (-1, &status, WNOHANG) ) != -1)
    {
      if (pid)
        {
          for (CtxList *l = ctx_vts; l; l=l->next)
            {
              VtPty *vt = l->data;
              if (vt->pid == pid)
                {
                  vt->done = 1;
                  //vt->result = status;
                }
            }
        }
    }
}

static void vt_init (VT *vt, int width, int height, float font_size, float line_spacing, int id, int can_launch)
{
  static int signal_installed = 0;
  if (!signal_installed)
  {
    signal (SIGCHLD,ctx_clients_signal_child);
    signal_installed = 1;
  }
  vt->id                 = id;
  vt->lastx              = -1;
  vt->lasty              = -1;
  vt->state              = vt_state_neutral;
  vt->smooth_scroll      = 0;
  vt->can_launch         = can_launch;
  vt->scroll_offset      = 0.0;
  vt->waitdata           = vtpty_waitdata;
  vt->read               = vtpty_read;
  vt->write              = vtpty_write;
  vt->resize             = vtpty_resize;
  vt->font_to_cell_scale = 0.98;
  vt->cursor_visible     = 1;
  vt->lines              = NULL;
  vt->line_count         = 0;
  vt->current_line       = NULL;
  vt->cols               = 0;
  vt->rows               = 0;

  vt->scrollback_limit   = DEFAULT_SCROLLBACK;
  vt->argument_buf_len   = 0;
  vt->argument_buf_cap   = 64;
  vt->argument_buf       = malloc (vt->argument_buf_cap);
  vt->argument_buf[0]    = 0;
  vt->vtpty.done         = 0;
  vt->result             = -1;
  vt->line_spacing       = 1.0;
  vt->scale_x            = 1.0;
  vt->scale_y            = 1.0;
  vt->fg_color[0] = 216;
  vt->fg_color[1] = 216;
  vt->fg_color[2] = 216;
  vt->bg_color[0] = 0;
  vt->bg_color[1] = 0;
  vt->bg_color[2] = 0;
}

static pid_t
vt_forkpty (int  *amaster,
            char *aname,
            const struct termios *termp,
            const struct winsize *winsize)
{
  pid_t pid;
  int master = posix_openpt (O_RDWR|O_NOCTTY);
  int slave;

  if (master < 0)
    return -1;
  if (grantpt (master) != 0)
    return -1;
  if (unlockpt (master) != 0)
    return -1;
#if 0
  char name[1024];
  if (ptsname_r (master, name, sizeof(name)-1))
    return -1;
#else
  char *name = NULL;
  if ((name = ptsname (master)) == NULL)
    return -1;
#endif

  slave = open(name, O_RDWR|O_NOCTTY);

  if (termp)   tcsetattr(slave, TCSAFLUSH, termp);
  if (winsize) ioctl(slave, TIOCSWINSZ, winsize);

  pid = fork();
  if (pid < 0)
  {
    return pid;
  } else if (pid == 0)
  {
    close (master);
    setsid ();
    dup2 (slave, STDIN_FILENO);
    dup2 (slave, STDOUT_FILENO);
    dup2 (slave, STDERR_FILENO);

    close (slave);
    return 0;
  }
  ioctl (slave, TIOCSCTTY, NULL);
  close (slave);
  *amaster = master;
  return pid;
}

static void
ctx_child_prepare_env (int was_pidone, const char *term)
{
  if (was_pidone)
  {
    if (setuid(1000)) fprintf (stderr, "setuid failed\n");
  }
  else
  {
    for (int i = 3; i<768; i++) { close (i); } /*hack, trying to close xcb */
  }
  unsetenv ("TERM");
  unsetenv ("COLUMNS");
  unsetenv ("LINES");
  unsetenv ("TERMCAP");
  unsetenv ("COLOR_TERM");
  unsetenv ("COLORTERM");
  unsetenv ("VTE_VERSION");
  unsetenv ("CTX_BACKEND");
  //setenv ("TERM", "ansi", 1);
  //setenv ("TERM", "vt102", 1);
  //setenv ("TERM", "vt100", 1);
  // setenv ("TERM", term?term:"xterm", 1);
  setenv ("TERM", term?term:"xterm-256color", 1);
  setenv ("COLORTERM", "truecolor", 1);
  //setenv ("CTX_VERSION", "0", 1);
  setenv ("CTX_BACKEND", "ctx", 1); // speeds up launching of clients
}

void _ctx_add_listen_fd (int fd);
void _ctx_remove_listen_fd (int fd);

#ifdef EMSCRIPTEN

#define EM_BUFSIZE 81920

char em_inbuf[EM_BUFSIZE]="";
char em_outbuf[EM_BUFSIZE]="";
int em_in_len = 0;
int em_in_pos = 0;
int em_in_read_pos = 0;
EMSCRIPTEN_KEEPALIVE int em_out_len = 0;
int em_out_pos = 0;

ssize_t em_write (void *s, const void *buf, size_t count)
{
  const char *src = (const char*)buf;
  int i;
  for (i = 0; i < count && em_out_len < EM_BUFSIZE; i ++)
  {
    em_outbuf[em_out_pos++] = src[i];
    em_out_len++;
    if (em_out_pos >= EM_BUFSIZE)em_out_pos = 0;
  }
  if (em_out_len >= EM_BUFSIZE)
    printf ("em_outbuf overflow\n");
  else
  EM_ASM({
    console.log('a a ' + UTF8ToString($1));
    ws.send(new Blob([UTF8ToString($0)]));
    }, src
       );

  return i;
}

EMSCRIPTEN_KEEPALIVE
ssize_t em_buffer (void *s, const void *buf, size_t count)
{
  const char *src = (const char*)buf;
  int i;
  for (i = 0; i < count && em_in_len < EM_BUFSIZE; i ++)
  {
    em_inbuf[em_in_pos++] = src[i];
    em_in_len++;
    if (em_in_pos >= EM_BUFSIZE)em_in_pos = 0;
    if (src[i]=='\n')
    {
      em_inbuf[em_in_pos++] = '\r';
      em_in_len++;
      if (em_in_pos >= EM_BUFSIZE)em_in_pos = 0;
    }
  }
  if (em_in_len >= EM_BUFSIZE)
    printf ("em_inbuf overflow\n");
  return i;
}

ssize_t em_read    (void *serial_obj, void *buf, size_t count)
{
  char *dst = (char*)buf;
  if (em_in_len)
  {
    *dst = em_inbuf[em_in_read_pos++];
    --em_in_len;
    if (em_in_read_pos>=EM_BUFSIZE)em_in_read_pos = 0;
    return 1;
  }
  return 0;
}
  int     em_waitdata (void *serial_obj, int timeout)
{
  return em_in_len;
}

  void    em_resize  (void *serial_obj, int cols, int rows, int px_width, int px_height)
{
}


#endif


static void vt_run_argv (VT *vt, char **argv, const char *term)
{
#ifdef EMSCRIPTEN
        vt->read = em_read;
        vt->write = em_write;
        vt->waitdata = em_waitdata;
        vt->resize = em_resize;

        printf ("aaa?\n");
#else
  struct winsize ws;
  //signal (SIGCHLD,signal_child);
#if 0
  int was_pidone = (getpid () == 1);
#else
  int was_pidone = 0; // do no special treatment, all child processes belong
                      // to root
#endif
  signal (SIGINT,SIG_DFL);
  ws.ws_row = vt->rows;
  ws.ws_col = vt->cols;
  ws.ws_xpixel = ws.ws_col * vt->cw;
  ws.ws_ypixel = ws.ws_row * vt->ch;
  vt->vtpty.pid = vt_forkpty (&vt->vtpty.pty, NULL, NULL, &ws);
  if (vt->vtpty.pid == 0)
    {
      ctx_child_prepare_env (was_pidone, term);

      execvp (argv[0], (char**)argv);
      exit (0);
    }
  else if (vt->vtpty.pid < 0)
    {
      VT_error ("forkpty failed (%s)", argv[0]);
      return;
    }
  fcntl(vt->vtpty.pty, F_SETFL, O_NONBLOCK|O_NOCTTY);
  _ctx_add_listen_fd (vt->vtpty.pty);
#endif
}


VT *vt_new_argv (char **argv, int width, int height, float font_size, float line_spacing, int id, int can_launch)
{
  VT *vt                 = calloc (sizeof (VT), 1);
  vt_init (vt, width, height, font_size, line_spacing, id, can_launch);
  vt_set_font_size (vt, font_size);
  vt_set_line_spacing (vt, line_spacing);
  if (argv)
    {
      vt_run_argv (vt, argv, NULL);
    }
  if (width <= 0) width = 640;
  if (height <= 0) width = 480;
  vt_set_px_size (vt, width, height);

  vtcmd_reset_to_initial_state (vt, NULL);
  //vt->ctx = ctx_new ();
  ctx_list_prepend (&ctx_vts, vt);
  return vt;
}

static char *string_chop_head (char *orig) /* return pointer to reset after arg */
{
  int j=0;
  int eat=0; /* number of chars to eat at start */

  if(orig)
    {
      int got_more;
      char *o = orig;
      while(o[j] == ' ')
        {j++;eat++;}

      if (o[j]=='"')
        {
          eat++;j++;
          while(o[j] != '"' &&
                o[j] != 0)
            j++;
          o[j]='\0';
          j++;
        }
      else if (o[j]=='\'')
        {
          eat++;j++;
          while(o[j] != '\'' &&
                o[j] != 0)
            j++;
          o[j]='\0';
          j++;
        }
      else
        {
          while(o[j] != ' ' &&
                o[j] != 0 &&
                o[j] != ';')
            j++;
        }
      if (o[j] == 0 ||
          o[j] == ';')
        got_more = 0;
      else
        got_more = 1;
      o[j]=0; /* XXX: this is where foo;bar won't work but foo ;bar works*/

      if(eat)
       {
         int k;
         for (k=0; k<j-eat; k++)
           orig[k] = orig[k+eat];
       }
      if (got_more)
        return &orig[j+1];
    }
  return NULL;
}


VT *vt_new (const char *command, int width, int height, float font_size, float line_spacing, int id, int can_launch)
{
  char *cargv[32];
  int   cargc;
  char *rest, *copy;
  copy = calloc (strlen (command)+2, 1);
  strcpy (copy, command);
  rest = copy;
  cargc = 0;
  while (rest && cargc < 30 && rest[0] != ';')
  {
    cargv[cargc++] = rest;
    rest = string_chop_head (rest);
  }
  cargv[cargc] = NULL;
  return vt_new_argv ((char**)cargv, width, height, font_size, line_spacing, id, can_launch);
}


int vt_cw (VT *vt)
{
  return vt->cw;
}

int vt_ch (VT *vt)
{
  return vt->ch;
}

static int vt_trimlines (VT *vt, int max)
{
  CtxList *chop_point = NULL;
  CtxList *l;
  int i;
  if (vt->line_count < max)
    { 
      return 0;
    }
  for (l = vt->lines, i = 0; l && i < max-1; l = l->next, i++);
  if (l)
    {
      chop_point = l->next;
      l->next = NULL;
    }
  while (chop_point)
    {
      if (vt->in_alt_screen)
        {
          vt_line_free (chop_point->data, 1);
        }
      else
        {
          ctx_list_prepend (&vt->scrollback, chop_point->data);
          vt->scrollback_count ++;
        }
      ctx_list_remove (&chop_point, chop_point->data);
      vt->line_count--;
    }
  if (vt->scrollback_count > vt->scrollback_limit + 1024)
    {
      CtxList *l = vt->scrollback;
      int no = 0;
      while (l && no < vt->scrollback_limit)
        {
          l = l->next;
          no++;
        }
      chop_point = NULL;
      if (l)
        {
          chop_point = l->next;
          l->next = NULL;
        }
      while (chop_point)
        {
          vt_line_free (chop_point->data, 1);
          ctx_list_remove (&chop_point, chop_point->data);
          vt->scrollback_count --;
        }
    }
  return 0;
}

static void vt_rewrap_pair (VT *vt, VtLine *topline, VtLine *bottomline, int max_col)
{
  int toplen = 0;

  while ((toplen = vt_line_get_utf8length (topline)) > max_col)
  {
     uint32_t unichar = vt_line_get_unichar (topline, toplen-1);
     uint32_t style =  vt_line_get_style (topline, toplen-1);
     vt_line_insert_unichar (bottomline, 0, unichar);
     vt_line_remove (topline, toplen-1);
     vt_line_set_style (bottomline, 0, style);
  }

  while (vt_line_get_length (bottomline) &&
         (toplen = vt_line_get_utf8length (topline)) < max_col)
  {
     uint32_t unichar = vt_line_get_unichar (bottomline, 0);
     uint32_t style =  vt_line_get_style (bottomline, 0);
     vt_line_append_unichar (topline, unichar);
     vt_line_set_style (topline, toplen, style);
     vt_line_remove (bottomline, 0);
  }
}

static void vt_rewrap (VT *vt, int max_col)
{
  if (max_col < 8) max_col = 8;
  CtxList *list = NULL;

  for (CtxList *l = vt->lines; l;)
  {
    CtxList *next = l->next;
    ctx_list_prepend (&list, l->data);
    ctx_list_remove (&vt->lines, l->data);
    l = next;
  }
  for (CtxList *l = vt->scrollback; l;)
  {
    CtxList *next = l->next;
    ctx_list_prepend (&list, l->data);
    ctx_list_remove (&vt->scrollback, l->data);
    l = next;
  }

  for (CtxList *l = list; l; l = l->next)
    {
      VtLine *line = l->data;
      VtLine *next = l->next ?l->next->data:NULL;

      if (vt_line_get_utf8length (line) >= max_col || (next && next->wrapped))
      {
        if (!next)
        {
          ctx_list_append (&list, vt_line_new (""));
          next = l->next->data;
          next->wrapped = 1;
        }
        else if (!next->wrapped)
        {
          ctx_list_insert_before (&list, l->next, vt_line_new (""));
          next = l->next->data;
          next->wrapped = 1;
        } 
        vt_rewrap_pair (vt, line, next, max_col);
        if (vt_line_get_utf8length (next) == 0)
          ctx_list_remove (&list, l->next->data);
      }
    }

  int rows = vt->rows;
  int total_rows = ctx_list_length (list);

  int scrollback_rows = total_rows - rows;

  int c = 0;
  CtxList *l;
  for (l = list; l && c < scrollback_rows;)
  {
    CtxList *next = l->next;
    ctx_list_prepend (&vt->scrollback, l->data);
    ctx_list_remove (&list, l->data);
    l = next;
    c++;
  }
  for (; l ;)
  {
    CtxList *next = l->next;
    ctx_list_prepend (&vt->lines, l->data);
    ctx_list_remove (&list, l->data);
    l = next;
    c++;
  }
}

void vt_set_term_size (VT *vt, int icols, int irows)
{
  if (vt->rows == irows && vt->cols == icols)
    return;

  if (vt->state == vt_state_ctx)
  {
    // we should queue a pending resize instead,
    // .. or set a flag indicating that the last
    // rendered frame is discarded?
    return;
  }

  if(1)vt_rewrap (vt, icols);

  while (irows > vt->rows)
    {
      if (vt->scrollback_count && vt->scrollback)
        {
          vt->scrollback_count--;
          ctx_list_append (&vt->lines, vt->scrollback->data);
          ctx_list_remove (&vt->scrollback, vt->scrollback->data);
          vt->cursor_y++;
        }
      else
        {
          ctx_list_prepend (&vt->lines, vt_line_new_with_size ("", vt->cols) );
        }
      vt->line_count++;
      vt->rows++;
    }
  while (irows < vt->rows)
    {
      vt->cursor_y--;
      vt->rows--;
    }
  vt->rows = irows;
  vt->cols = icols;
  vt_resize (vt, vt->cols, vt->rows, vt->width, vt->height);
  vt_trimlines (vt, vt->rows);
  vt->margin_top     = 1;
  vt->margin_left    = 1;
  vt->margin_bottom  = vt->rows;
  vt->margin_right   = vt->cols;
  _vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  ctx_client_rev_inc (vt->client);
  VT_info ("resize %i %i", irows, icols);
  if (vt->ctxp)
    ctx_parser_free (vt->ctxp);
  vt->ctxp = NULL;
}

void vt_set_px_size (VT *vt, int width, int height)
{
  int cols = width / vt->cw;
  int rows = height / vt->ch;
  vt->width = width;
  vt->height = height;
  vt_set_term_size (vt, cols, rows);
}

static void vt_argument_buf_reset (VT *vt, const char *start)
{
  if (start)
    {
      strcpy (vt->argument_buf, start);
      vt->argument_buf_len = strlen (start);
    }
  else
    { vt->argument_buf[vt->argument_buf_len=0]=0; }
}

static inline void vt_argument_buf_add (VT *vt, int ch)
{
  if (vt->argument_buf_len + 1 >= 1024 * 1024 * 16)
    return; 
  //
  if (vt->argument_buf_len + 1 >=
      vt->argument_buf_cap)
    {
      vt->argument_buf_cap = vt->argument_buf_cap * 2;
      vt->argument_buf = realloc (vt->argument_buf, vt->argument_buf_cap);
    }
  vt->argument_buf[vt->argument_buf_len] = ch;
  vt->argument_buf[++vt->argument_buf_len] = 0;
}

static void
_vt_move_to (VT *vt, int y, int x)
{
  int i;
  x = x < 1 ? 1 : (x > vt->cols ? vt->cols : x);
  y = y < 1 ? 1 : (y > vt->rows ? vt->rows : y);
  vt->at_line_home = 0;
  vt->cursor_x = x;
  vt->cursor_y = y;
  i = vt->rows - y;
  CtxList *l;
  for (l = vt->lines; l && i >= 1; l = l->next, i--);
  if (l)
    {
      vt->current_line = l->data;
    }
  else
    {
      for (; i > 0; i--)
        {
          vt->current_line = vt_line_new_with_size ("", vt->cols);
          ctx_list_append (&vt->lines, vt->current_line);
          vt->line_count++;
        }
    }
  VT_cursor ("%i,%i (_vt_move_to)", y, x);
  ctx_client_rev_inc (vt->client);
}

static void vt_scroll (VT *vt, int amount);

static void _vt_add_str (VT *vt, const char *str)
{
  int logical_margin_right = VT_MARGIN_RIGHT;
  if (vt->cstyle & STYLE_PROPORTIONAL)
    { vt->current_line->contains_proportional = 1; }
  if (vt->cursor_x > logical_margin_right)
    {
      if (vt->autowrap)
        {
          int chars = 0;
          int old_x = vt->cursor_x;
          VtLine *old_line = vt->current_line;
          if (vt->justify && str[0] != ' ')
            {
              while (old_x-1-chars >1 && vt_line_get_unichar (vt->current_line,
                     old_x-1-chars) !=' ')
                {
                  chars++;
                }
              chars--;
              if (chars > (vt->margin_right - vt->margin_left) * 3 / 2)
                { chars = 0; }
            }
          if (vt->cursor_y == vt->margin_bottom)
            {
              vt_scroll (vt, -1);
            }
          else
            {
              _vt_move_to (vt, vt->cursor_y+1, 1);
            }
          vt->current_line->wrapped=1;
          vt_carriage_return (vt);
          for (int i = 0; i < chars; i++)
            {
              vt_line_set_style (vt->current_line, vt->cursor_x-1, vt->cstyle);
              vt_line_replace_unichar (vt->current_line, vt->cursor_x - 1,
                                         vt_line_get_unichar (old_line, old_x-1-chars+i) );
              vt->cursor_x++;
            }
          for (int i = 0; i < chars; i++)
            {
              vt_line_replace_unichar (old_line, old_x-1-chars+i, ' ');
            }
          if (str[0] == ' ')
            return;
        }
      else
        {
          vt->cursor_x = logical_margin_right;
        }
    }
  if (vt->insert_mode)
    {
      vt_line_insert_utf8 (vt->current_line, vt->cursor_x - 1, str);
      while (vt->current_line->string.utf8_length > logical_margin_right)
        { vt_line_remove (vt->current_line, logical_margin_right); }
    }
  else
    {
      vt_line_replace_utf8 (vt->current_line, vt->cursor_x - 1, str);
    }
  vt_line_set_style (vt->current_line, vt->cursor_x-1, vt->cstyle);
  vt->cursor_x += 1;
  vt->at_line_home = 0;
  ctx_client_rev_inc (vt->client);
}

static void _vt_backspace (VT *vt)
{
  if (vt->current_line)
    {
      vt->cursor_x --;
      if (vt->cursor_x == VT_MARGIN_RIGHT) { vt->cursor_x--; }
      if (vt->cursor_x < VT_MARGIN_LEFT)
        {
          vt->cursor_x = VT_MARGIN_LEFT;
          vt->at_line_home = 1;
        }
      VT_cursor ("backspace");
    }
  ctx_client_rev_inc (vt->client);
}

static void vtcmd_set_top_and_bottom_margins (VT *vt, const char *sequence)
{
  int top = 1, bottom = vt->rows;
  /* w3m issues this; causing reset of cursor position, why it is issued
   * is unknown 
   */
  if (!strcmp (sequence, "[?1001r"))
    return;
  if (strlen (sequence) > 2)
    {
      sscanf (sequence, "[%i;%ir", &top, &bottom);
    }
  VT_info ("margins: %i %i", top, bottom);
  if (top <1) { top = 1; }
  if (top > vt->rows) { top = vt->rows; }
  if (bottom > vt->rows) { bottom = vt->rows; }
  if (bottom < top) { bottom = top; }
  vt->margin_top = top;
  vt->margin_bottom = bottom;
#if 0
  _vt_move_to (vt, top, 1);
#endif
  vt_carriage_return (vt);
  VT_cursor ("%i, %i (home)", top, 1);
}
static void vtcmd_save_cursor_position (VT *vt, const char *sequence);

static void vtcmd_set_left_and_right_margins (VT *vt, const char *sequence)
{
  int left = 1, right = vt->cols;
  if (!vt->left_right_margin_mode)
    {
      vtcmd_save_cursor_position (vt, sequence);
      return;
    }
  if (strlen (sequence) > 2)
    {
      sscanf (sequence, "[%i;%is", &left, &right);
    }
  VT_info ("hor margins: %i %i", left, right);
  if (left <1) { left = 1; }
  if (left > vt->cols) { left = vt->cols; }
  if (right > vt->cols) { right = vt->cols; }
  if (right < left) { right = left; }
  vt->margin_left = left;
  vt->margin_right = right;
  _vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  vt_carriage_return (vt);
  //VT_cursor ("%i, %i (home)", left, 1);
}

static inline int parse_int (const char *arg, int def_val)
{
  if (!isdigit (arg[1]) || strlen (arg) == 2)
    { return def_val; }
  return atoi (arg+1);
}


static void vtcmd_set_line_home (VT *vt, const char *sequence)
{
  int val = parse_int (sequence, 1);
  char buf[256];
  vt->left_right_margin_mode = 1;
  sprintf (buf, "[%i;%it", val, vt->margin_right);
  vtcmd_set_left_and_right_margins (vt, buf);
}

static void vtcmd_set_line_limit (VT *vt, const char *sequence)
{
  int val = parse_int (sequence, 0);
  char buf[256];
  vt->left_right_margin_mode = 1;
  if (val < vt->margin_left) { val = vt->margin_left; }
  sprintf (buf, "[%i;%it", vt->margin_left, val);
  vtcmd_set_left_and_right_margins (vt, buf);
}

static void vt_scroll (VT *vt, int amount)
{
  int remove_no, insert_before;
  VtLine *string = NULL;
  if (amount == 0) { amount = 1; }
  if (amount < 0)
    {
      remove_no = vt->margin_top;
      insert_before = vt->margin_bottom;
    }
  else
    {
      remove_no = vt->margin_bottom;
      insert_before = vt->margin_top;
    }
  CtxList *l;
  int i;
  for (i=vt->rows, l = vt->lines; i > 0 && l; l=l->next, i--)
    {
      if (i == remove_no)
        {
          string = l->data;
          ctx_list_remove (&vt->lines, string);
          break;
        }
    }
  if (string)
    {
      if (!vt->in_alt_screen &&
          (vt->margin_top == 1 && vt->margin_bottom == vt->rows) )
        {
          ctx_list_prepend (&vt->scrollback, string);
          vt->scrollback_count ++;
        }
      else
        {
          vt_line_free (string, 1);
        }
    }
  string = vt_line_new_with_size ("", vt->cols/4);
  if (amount > 0 && vt->margin_top == 1)
    {
      ctx_list_append (&vt->lines, string);
    }
  else
    {
      for (i=vt->rows, l = vt->lines; l; l=l->next, i--)
        {
          if (i == insert_before)
            {
              ctx_list_insert_before (&vt->lines, l, string);
              break;
            }
        }
      if (i != insert_before)
        {
          ctx_list_append (&vt->lines, string);
        }
    }
  vt->current_line = string;
  /* not updating line count since we should always remove one and add one */
  if (vt->smooth_scroll)
    {
      if (amount < 0)
        {
          vt->scroll_offset = -1.0;
          vt->in_smooth_scroll = -1;
        }
      else
        {
          vt->scroll_offset = 1.0;
          vt->in_smooth_scroll = 1;
        }
    }

  {
    vt->select_begin_row += amount;
    vt->select_end_row += amount;
    vt->select_start_row += amount;
  }
}

typedef struct Sequence
{
  const char *prefix;
  char        suffix;
  void (*vtcmd) (VT *vt, const char *sequence);
  uint32_t    compat;
} Sequence;

static void vtcmd_cursor_position (VT *vt, const char *sequence)
{
  int y = 1, x = 1;
  const char *semi;
  if (sequence[0] != 'H' && sequence[0] != 'f')
    {
      y = parse_int (sequence, 1);
      if ( (semi = strchr (sequence, ';') ) )
        {
          x = parse_int (semi, 1);
        }
    }
  if (x == 0) { x = 1; }
  if (y == 0) { y = 1; }
  if (vt->origin)
    {
      y += vt->margin_top - 1;
      _vt_move_to (vt, y, vt->cursor_x);
      x += VT_MARGIN_LEFT - 1;
    }
  VT_cursor ("%i %i CUP", y, x);
  _vt_move_to (vt, y, x);
}


static void vtcmd_horizontal_position_absolute (VT *vt, const char *sequence)
{
  int x = parse_int (sequence, 1);
  if (x<=0) { x = 1; }
  _vt_move_to (vt, vt->cursor_y, x);
}

static void vtcmd_goto_row (VT *vt, const char *sequence)
{
  int y = parse_int (sequence, 1);
  _vt_move_to (vt, y, vt->cursor_x);
}

static void vtcmd_cursor_forward (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) { n = 1; }
  for (int i = 0; i < n; i++)
    {
      vt->cursor_x++;
    }
  if (vt->cursor_x > VT_MARGIN_RIGHT)
    { vt->cursor_x = VT_MARGIN_RIGHT; }
}

static void vtcmd_cursor_backward (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) { n = 1; }
  for (int i = 0; i < n; i++)
    {
      vt->cursor_x--;
    }
  if (vt->cursor_x < VT_MARGIN_LEFT)
    {
      vt->cursor_x = VT_MARGIN_LEFT; // should this wrap??
      vt->at_line_home = 1;
    }
}

static void vtcmd_reverse_index (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) { n = 1; }
  for (int i = 0; i < n; i++)
    {
      if (vt->cursor_y == vt->margin_top)
        {
          vt_scroll (vt, 1);
          _vt_move_to (vt, vt->margin_top, vt->cursor_x);
        }
      else
        {
          _vt_move_to (vt, vt->cursor_y-1, vt->cursor_x);
        }
    }
}

static void vtcmd_cursor_up (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) { n = 1; }
  for (int i = 0; i < n; i++)
    {
      if (vt->cursor_y == vt->margin_top)
        {
          //_vt_move_to (vt, 1, vt->cursor_x);
        }
      else
        {
          _vt_move_to (vt, vt->cursor_y-1, vt->cursor_x);
        }
    }
}

static void vtcmd_back_index (VT *vt, const char *sequence)
{
  // XXX implement
}

static void vtcmd_forward_index (VT *vt, const char *sequence)
{
  // XXX implement
}

static void vtcmd_index (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) { n = 1; }
  for (int i = 0; i < n; i++)
    {
      if (vt->cursor_y == vt->margin_bottom)
        {
          vt_scroll (vt, -1);
          _vt_move_to (vt, vt->margin_bottom, vt->cursor_x);
        }
      else
        {
          _vt_move_to (vt, vt->cursor_y + 1, vt->cursor_x);
        }
    }
}

static void vtcmd_cursor_down (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) { n = 1; }
  for (int i = 0; i < n; i++)
    {
      if (vt->cursor_y >= vt->margin_bottom)
        {
          _vt_move_to (vt, vt->margin_bottom, vt->cursor_x);
        }
      else
        {
          _vt_move_to (vt, vt->cursor_y + 1, vt->cursor_x);
        }
    }
}

static void vtcmd_next_line (VT *vt, const char *sequence)
{
  vtcmd_index (vt, sequence);
  _vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  vt_carriage_return (vt);
  vt->cursor_x = VT_MARGIN_LEFT;
}

static void vtcmd_cursor_preceding_line (VT *vt, const char *sequence)
{
  vtcmd_cursor_up (vt, sequence);
  _vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  vt->cursor_x = VT_MARGIN_LEFT;
}

static void vtcmd_erase_in_line (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 0);
  switch (n)
    {
      case 0: // clear to end of line
        {
          char *p = (char *) mrg_utf8_skip (vt->current_line->string.str, vt->cursor_x-1);
          if (p) { *p = 0; }
          // XXX : this is chopping lines
          for (int col = vt->cursor_x; col <= VT_MARGIN_RIGHT; col++)
            { vt_line_set_style (vt->current_line, col - 1, vt->cstyle); }
          vt->current_line->string.length = strlen (vt->current_line->string.str);
          vt->current_line->string.utf8_length = ctx_utf8_strlen (vt->current_line->string.str);
        }
        break;
      case 1: // clear from beginning to cursor
        {
          for (int col = VT_MARGIN_LEFT; col <= vt->cursor_x; col++)
            {
              vt_line_replace_utf8 (vt->current_line, col-1, " ");
            }
          for (int col = VT_MARGIN_LEFT; col <= vt->cursor_x; col++)
            { vt_line_set_style (vt->current_line, col-1, vt->cstyle); }
          vt->current_line->string.length = strlen (vt->current_line->string.str);
          vt->current_line->string.utf8_length = ctx_utf8_strlen (vt->current_line->string.str); // should be a nop
        }
        break;
      case 2: // clear entire line
        for (int col = VT_MARGIN_LEFT; col <= VT_MARGIN_RIGHT; col++)
          { vt_line_set_style (vt->current_line, col-1, vt->cstyle); }
        vt_line_set (vt->current_line, ""); // XXX not all
        break;
    }
}

static void vtcmd_erase_in_display (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 0);
  switch (n)
    {
      case 0: // clear to end of screen
        {
          char *p = (char *) mrg_utf8_skip (vt->current_line->string.str, vt->cursor_x-1);
          if (p) { *p = 0; }
          vt->current_line->string.length = strlen (vt->current_line->string.str);
          vt->current_line->string.utf8_length = ctx_utf8_strlen (vt->current_line->string.str);
        }
        for (int col = vt->cursor_x; col <= VT_MARGIN_RIGHT; col++)
          { vt_line_set_style (vt->current_line, col-1, vt->cstyle); }
        {
          CtxList *l;
          int no = vt->rows;
          for (l = vt->lines; l->data != vt->current_line; l = l->next, no--)
            {
              VtLine *buf = l->data;
              buf->string.str[0] = 0;
              buf->string.length = 0;
              buf->string.utf8_length = 0;
              for (int col = 1; col <= vt->cols; col++)
                { vt_line_set_style (buf, col-1, vt->cstyle); }
            }
        }
        break;
      case 1: // clear from beginning to cursor
        {
          for (int col = 1; col <= vt->cursor_x; col++)
            {
              vt_line_replace_utf8 (vt->current_line, col-1, " ");
              vt_line_set_style (vt->current_line, col-1, vt->cstyle);
            }
        }
        {
          CtxList *l;
          int there_yet = 0;
          int no = vt->rows;
          for (l = vt->lines; l; l = l->next, no--)
            {
              VtLine *buf = l->data;
              if (there_yet)
                {
                  buf->string.str[0] = 0;
                  buf->string.length = 0;
                  buf->string.utf8_length = 0;
                  for (int col = 1; col <= vt->cols; col++)
                    { vt_line_set_style (buf, col-1, vt->cstyle); }
                }
              if (buf == vt->current_line)
                {
                  there_yet = 1;
                }
            }
        }
        break;
      case 3: // also clear scrollback
        while (vt->scrollback)
        {
           vt_line_free (vt->scrollback->data, 1);
           ctx_list_remove (&vt->scrollback, vt->scrollback->data);
         }
        vt->scrollback_count = 0;
        /* FALLTHROUGH */
      case 2: // clear entire screen but keep cursor;
        {
          int tx = vt->cursor_x;
          int ty = vt->cursor_y;
          vtcmd_clear (vt, "");
          _vt_move_to (vt, ty, tx);
          for (CtxList *l = vt->lines; l; l = l->next)
            {
              VtLine *line = l->data;
              for (int col = 1; col <= vt->cols; col++)
                { vt_line_set_style (line, col-1, vt->cstyle); }
            }
        }
        break;
    }
}

static void vtcmd_screen_alignment_display (VT *vt, const char *sequence)
{
  for (int y = 1; y <= vt->rows; y++)
    {
      _vt_move_to (vt, y, 1);
      for (int x = 1; x <= vt->cols; x++)
        {
          _vt_add_str (vt, "E");
        }
    }
}

#if 0
static int find_idx (int r, int g, int b)
{
  r = r / 255.0 * 5;
  g = g / 255.0 * 5;
  b = b / 255.0 * 5;
  return 16 + r * 6 * 6 + g * 6 + b;
}
#endif

static void vtcmd_set_graphics_rendition (VT *vt, const char *sequence)
{
  const char *s = sequence;
  if (s[0]) { s++; }
  while (s && *s)
    {
      int n = parse_int (s - 1, 0); // works until color
      // both fg and bg could be set in 256 color mode FIXME
      //
      /* S_GR@38@Set forground color@foo bar baz@ */
      if (n == 38) // set foreground
        {
          s = strchr (s, ';');
          if (!s)
            {
              VT_warning ("incomplete [38m expected ;  %s", sequence);
              return;
            }
          n = parse_int (s, 0);
          if (n == 5)
            {
              s++;
              if (strchr (s, ';') )
                { s = strchr (s, ';'); }
              else
                { s = strchr (s, ':'); }
              if (s)
                {
                  n = parse_int (s, 0);
                  set_fg_idx (n);
                  s++;
                  while (*s && *s >= '0' && *s <='9') { s++; }
                }
            }
          else if (n == 2)
            {
              int r = 0, g = 0, b = 0;
              s++;
              if (strchr (s, ';') )
                {
                  s = strchr (s, ';');
                  if (s)
                    { sscanf (s, ";%i;%i;%i", &r, &g, &b); }
                }
              else
                {
                  s = strchr (s, ':');
                  if (s)
                    { sscanf (s, ":%i:%i:%i", &r, &g, &b); }
                }
              if (s)
              for (int i = 0; i < 3; i++)
                {
                  if (*s)
                    {
                      s++;
                      while (*s && *s >= '0' && *s <='9') { s++; }
                    }
                }
              set_fg_rgb (r,g,b);
            }
          else
            {
              VT_warning ("unhandled %s %i", sequence, n);
              return;
            }
          //return; // XXX we should continue, and allow further style set after complex color
        }
      else if (n == 48) // set background
        {
          s = strchr (s, ';');
          if (!s)
            {
              VT_warning ("incomplete [38m expected ;  %s", sequence);
              return;
            }
          n = parse_int (s, 0);
          if (n == 5)
            {
              s++;
              if (strchr (s, ';') )
                { s = strchr (s, ';'); }
              else
                { s = strchr (s, ':'); }
              if (s)
                { n = parse_int (s, 0); }
              set_bg_idx (n);
              if (s)
                {
                  s++;
                  while (*s && *s >= '0' && *s <='9') { s++; }
                }
            }
          else if (n == 2)
            {
              int r = 0, g = 0, b = 0;
              s++;
              if (strchr (s, ';') )
                {
                  s = strchr (s, ';');
                  if (s)
                    { sscanf (s, ";%i;%i;%i", &r, &g, &b); }
                }
              else
                {
                  s = strchr (s, ':');
                  if (s)
                    { sscanf (s, ":%i:%i:%i", &r, &g, &b); }
                }
              if (s)
              for (int i = 0; i < 3; i++)
                {
                  s++;
                  while (*s >= '0' && *s <='9') { s++; }
                }
              set_bg_rgb (r,g,b);
            }
          else
            {
              VT_warning ("unhandled %s %i", sequence, n);
              return;
            }
          //return; // we XXX should continue, and allow further style set after complex color
        }
      else
        switch (n)
          {
            case 0: /* SGR@0@Style reset@@ */
              if (vt->cstyle & STYLE_PROPORTIONAL)
                { vt->cstyle = STYLE_PROPORTIONAL; }
              else
                { vt->cstyle = 0; }
              break;
            case 1: /* SGR@@Bold@@ */
              vt->cstyle |= STYLE_BOLD;
              break;
            case 2: /* SGR@@Dim@@ */
              vt->cstyle |= STYLE_DIM;
              break;
            case 3: /* SGR@@Italic@@ */
              vt->cstyle |= STYLE_ITALIC;
              break;
            case 4: /* SGR@@Underscore@@ */
                    /* SGR@4:2@Double underscore@@ */
                    /* SGR@4:3@Curvy underscore@@ */
              if (s[1] == ':')
                {
                  switch (s[2])
                    {
                      case '0':
                        break;
                      case '1':
                        vt->cstyle |= STYLE_UNDERLINE;
                        break;
                      case '2':
                        vt->cstyle |= STYLE_UNDERLINE|
                                      STYLE_UNDERLINE_VAR;
                        break;
                      default:
                      case '3':
                        vt->cstyle |= STYLE_UNDERLINE_VAR;
                        break;
                    }
                }
              else
                {
                  vt->cstyle |= STYLE_UNDERLINE;
                }
              break;
            case 5: /* SGR@@Blink@@ */
              vt->cstyle |= STYLE_BLINK;
              break;
            case 6: /* SGR@@Blink Fast@@ */
              vt->cstyle |= STYLE_BLINK_FAST;
              break;
            case 7: /* SGR@@Reverse@@ */
              vt->cstyle |= STYLE_REVERSE;
              break;
            case 8: /* SGR@@Hidden@@ */
              vt->cstyle |= STYLE_HIDDEN;
              break;
            case 9: /* SGR@@Strikethrough@@ */
              vt->cstyle |= STYLE_STRIKETHROUGH;
              break;
            case 10: /* SGR@@Font 0@@ */
              break;
            case 11: /* SGR@@Font 1@@ */
              break;
            case 12: /* SGR@@Font 2(ignored)@@ */
            case 13: /* SGR@@Font 3(ignored)@@ */
            case 14: /* SGR@@Font 4(ignored)@@ */
              break;
            case 22: /* SGR@@Bold off@@ */
              vt->cstyle ^= (vt->cstyle & STYLE_BOLD);
              vt->cstyle ^= (vt->cstyle & STYLE_DIM);
              break;
            case 23: /* SGR@@Italic off@@ */
              vt->cstyle ^= (vt->cstyle & STYLE_ITALIC);
              break;
            case 24: /* SGR@@Underscore off@@ */
              vt->cstyle ^= (vt->cstyle & (STYLE_UNDERLINE|STYLE_UNDERLINE_VAR) );
              break;
            case 25: /* SGR@@Blink off@@ */
              vt->cstyle ^= (vt->cstyle & STYLE_BLINK);
              vt->cstyle ^= (vt->cstyle & STYLE_BLINK_FAST);
              break;
            case 26: /* SGR@@Proportional spacing @@ */
              vt->cstyle |= STYLE_PROPORTIONAL;
              break;
            case 27: /* SGR@@Reverse off@@ */
              vt->cstyle ^= (vt->cstyle & STYLE_REVERSE);
              break;
            case 28: /* SGR@@Hidden off@@ */
              vt->cstyle ^= (vt->cstyle & STYLE_HIDDEN);
              break;
            case 29: /* SGR@@Strikethrough off@@ */
              vt->cstyle ^= (vt->cstyle & STYLE_STRIKETHROUGH);
              break;
            case 30: /* SGR@@black text color@@ */
              set_fg_idx (0);
              break;
            case 31: /* SGR@@red text color@@ */
              set_fg_idx (1);
              break;
            case 32: /* SGR@@green text color@@ */
              set_fg_idx (2);
              break;
            case 33: /* SGR@@yellow text color@@ */
              set_fg_idx (3);
              break;
            case 34: /* SGR@@blue text color@@ */
              set_fg_idx (4);
              break;
            case 35: /* SGR@@magenta text color@@ */
              set_fg_idx (5);
              break;
            case 36: /* SGR@@cyan text color@@ */
              set_fg_idx (6);
              break;
            case 37: /* SGR@@light gray text color@@ */
              set_fg_idx (7);
              break;
          /* SGR@38;5;Pn@256 color index foreground color@where Pn is 0-15 is system colors 16-(16+6*6*6) is a 6x6x6  RGB cube and in the end a grayscale without white and black.@ */
              /* SGR@38;2;Pr;Pg;Pb@24 bit RGB foreground color each of Pr Pg and Pb have 0-255 range@@ */
            case 39: /* SGR@@default text color@@ */
              set_fg_idx (vt->reverse_video?0:15);
              vt->cstyle ^= (vt->cstyle & STYLE_FG_COLOR_SET);
              break;
            case 40: /* SGR@@black background color@@ */
              set_bg_idx (0);
              break;
            case 41: /* SGR@@red background color@@ */
              set_bg_idx (1);
              break;
            case 42: /* SGR@@green background color@@ */
              set_bg_idx (2);
              break;
            case 43: /* SGR@@yellow background color@@ */
              set_bg_idx (3);
              break;
            case 44: /* SGR@@blue background color@@ */
              set_bg_idx (4);
              break;
            case 45: /* SGR@@magenta background color@@ */
              set_bg_idx (5);
              break;
            case 46: /* SGR@@cyan background color@@ */
              set_bg_idx (6);
              break;
            case 47: /* SGR@@light gray background color@@ */
              set_bg_idx (7);
              break;

          /* SGR@48;5;Pn@256 color index background color@where Pn is 0-15 is system colors 16-(16+6*6*6) is a 6x6x6  RGB cube and in the end a grayscale without white and black.@ */
          /* SGR@48;2;Pr;Pg;Pb@24 bit RGB background color@Where Pr Pg and Pb have 0-255 range@ */

            case 49: /* SGR@@default background color@@ */
              set_bg_idx (vt->reverse_video?15:0);
              vt->cstyle ^= (vt->cstyle & STYLE_BG_COLOR_SET);
              break;
            case 50: /* SGR@@Proportional spacing off @@ */
              vt->cstyle ^= (vt->cstyle & STYLE_PROPORTIONAL);
              break;
            // 51 : framed
            // 52 : encircled
            case 53: /* SGR@@Overlined@@ */
              vt->cstyle |= STYLE_OVERLINE;
              break;
            case 55: /* SGR@@Not Overlined@@ */
              vt->cstyle ^= (vt->cstyle&STYLE_OVERLINE);
              break;
            case 90: /* SGR@@dark gray text color@@ */
              set_fg_idx (8);
              break;
            case 91: /* SGR@@light red text color@@ */
              set_fg_idx (9);
              break;
            case 92: /* SGR@@light green text color@@ */
              set_fg_idx (10);
              break;
            case 93: /* SGR@@light yellow text color@@ */
              set_fg_idx (11);
              break;
            case 94: /* SGR@@light blue text color@@ */
              set_fg_idx (12);
              break;
            case 95: /* SGR@@light magenta text color@@ */
              set_fg_idx (13);
              break;
            case 96: /* SGR@@light cyan text color@@ */
              set_fg_idx (14);
              break;
            case 97: /* SGR@@white text color@@ */
              set_fg_idx (15);
              break;
            case 100: /* SGR@@dark gray background color@@ */
              set_bg_idx (8);
              break;
            case 101: /* SGR@@light red background color@@ */
              set_bg_idx (9);
              break;
            case 102: /* SGR@@light green background color@@ */
              set_bg_idx (10);
              break;
            case 103: /* SGR@@light yellow background color@@ */
              set_bg_idx (11);
              break;
            case 104: /* SGR@@light blue background color@@ */
              set_bg_idx (12);
              break;
            case 105: /* SGR@@light magenta background color@@ */
              set_bg_idx (13);
              break;
            case 106: /* SGR@@light cyan background color@@ */
              set_bg_idx (14);
              break;
            case 107: /* SGR@@white background color@@ */
              set_bg_idx (15);
              break;
            default:
              VT_warning ("unhandled style code %i in sequence \\033%s\n", n, sequence);
              return;
          }
      while (s && *s && *s != ';') { s++; }
      if (s && *s == ';') { s++; }
    }
}

static void vtcmd_ignore (VT *vt, const char *sequence)
{
  VT_info ("ignoring sequence %s", sequence);
}

static void vtcmd_clear_all_tabs (VT *vt, const char *sequence)
{
  memset (vt->tabs, 0, sizeof (vt->tabs) );
}

static void vtcmd_clear_current_tab (VT *vt, const char *sequence)
{
  vt->tabs[ (int) (vt->cursor_x-1)] = 0;
}

static void vtcmd_horizontal_tab_set (VT *vt, const char *sequence)
{
  vt->tabs[ (int) vt->cursor_x-1] = 1;
}

static void vtcmd_save_cursor_position (VT *vt, const char *sequence)
{
  vt->saved_x = vt->cursor_x;
  vt->saved_y = vt->cursor_y;
}

static void vtcmd_restore_cursor_position (VT *vt, const char *sequence)
{
  _vt_move_to (vt, vt->saved_y, vt->saved_x);
}


static void vtcmd_save_cursor (VT *vt, const char *sequence)
{
  vt->saved_style   = vt->cstyle;
  vt->saved_origin  = vt->origin;
  vtcmd_save_cursor_position (vt, sequence);
  for (int i = 0; i < 4; i++)
    { vt->saved_charset[i] = vt->charset[i]; }
}

static void vtcmd_restore_cursor (VT *vt, const char *sequence)
{
  vtcmd_restore_cursor_position (vt, sequence);
  vt->cstyle  = vt->saved_style;
  vt->origin  = vt->saved_origin;
  for (int i = 0; i < 4; i++)
    { vt->charset[i] = vt->saved_charset[i]; }
}

static void vtcmd_erase_n_chars (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
    {
      vt_line_replace_utf8 (vt->current_line, vt->cursor_x - 1 + n, " ");
      vt_line_set_style (vt->current_line, vt->cursor_x + n - 1, vt->cstyle);
    }
}

static void vtcmd_delete_n_chars (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  int count = n;
  while (count--)
    {
      vt_line_remove (vt->current_line, vt->cursor_x - 1);
    }
}

static void vtcmd_delete_n_lines (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  for (int a = 0; a < n; a++)
    {
      int i;
      CtxList *l;
      VtLine *string = vt->current_line;
      vt_line_set (string, "");
      ctx_list_remove (&vt->lines, vt->current_line);
      for (i=vt->rows, l = vt->lines; l; l=l->next, i--)
        {
          if (i == vt->margin_bottom)
            {
              vt->current_line = string;
              ctx_list_insert_before (&vt->lines, l, string);
              break;
            }
        }
      _vt_move_to (vt, vt->cursor_y, vt->cursor_x); // updates current_line
    }
}

static void vtcmd_insert_character (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
    {
      vt_line_insert_utf8 (vt->current_line, vt->cursor_x-1, " ");
    }
  while (vt->current_line->string.utf8_length > vt->cols)
    { vt_line_remove (vt->current_line, vt->cols); }
  // XXX update style
}

static void vtcmd_scroll_up (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n == 0) { n = 1; }
  while (n--)
    { vt_scroll (vt, -1); }
}

static void vtcmd_scroll_down (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n == 0) { n = 1; }
  while (n--)
    { vt_scroll (vt, 1); }
}

static void vtcmd_insert_blank_lines (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n == 0) { n = 1; }
  {
    int st = vt->margin_top;
    int sb = vt->margin_bottom;
    vt->margin_top = vt->cursor_y;
    while (n--)
      {
        vt_scroll (vt, 1);
      }
    vt->margin_top = st;
    vt->margin_bottom = sb;
  }
}

static void vtcmd_set_default_font (VT *vt, const char *sequence)
{
  vt->charset[0] = 0;
}

static void vtcmd_set_alternate_font (VT *vt, const char *sequence)
{
  vt->charset[0] = 1;
}


static void vt_ctx_exit (void *data)
{
  VT *vt = data;
  vt->state = vt_state_neutral;
  ctx_client_rev_inc (vt->client);
  if (!vt->current_line)
    return;
#if 0
  fprintf (stderr, "\n");
  if (vt->current_line->prev)
  fprintf (stderr, "---prev(%i)----\n%s", (int)strlen(vt->current_line->prev),vt->current_line->prev);
  fprintf (stderr, "---new(%i)----\n%s", (int)strlen(vt->current_line->frame->str),vt->current_line->frame->str);
  fprintf (stderr, "--------\n");
#endif

#if CTX_VT_USE_FRAME_DIFF
  if (vt->current_line->prev)
    free (vt->current_line->prev);
  vt->current_line->prev = NULL;
  if (vt->current_line->frame)
  {
    vt->current_line->prev = vt->current_line->frame->str;
    vt->current_line->prev_length = vt->current_line->frame->length;

    ctx_string_free (vt->current_line->frame, 0);
    vt->current_line->frame = NULL;
  }
#endif

  void *tmp = vt->current_line->ctx;
  vt->current_line->ctx = vt->current_line->ctx_copy;
  vt->current_line->ctx_copy = tmp;

  _ctx_set_frame (vt->current_line->ctx, _ctx_frame (vt->current_line->ctx) + 1);
  _ctx_set_frame (vt->current_line->ctx_copy, _ctx_frame (vt->current_line->ctx));
#if 1
  if (vt->ctxp) // XXX: ugly hack to aid double buffering
    ((void**)vt->ctxp)[0]= vt->current_line->ctx;
#endif

  //ctx_parser_free (vt->ctxp);
  //vt->ctxp = NULL;
}
#if 0
#define CTX_x            CTX_STRH('x',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_y            CTX_STRH('y',0,0,0,0,0,0,0,0,0,0,0,0,0)
#define CTX_lower_bottom CTX_STRH('l','o','w','e','r','-','b','o','t','t','o','m',0,0)
#define CTX_lower        CTX_STRH('l','o','w','e','r',0,0,0,0,0,0,0,0,0)
#define CTX_raise        CTX_STRH('r','a','i','s','e',0,0,0,0,0,0,0,0,0)
#define CTX_raise_top    CTX_STRH('r','a','i','s','e','-','t','o','p',0,0,0,0,0)
#define CTX_terminate    CTX_STRH('t','e','r','m','i','n','a','t','e',0,0,0,0,0)
#define CTX_maximize     CTX_STRH('m','a','x','i','m','i','z','e',0,0,0,0,0,0)
#define CTX_unmaximize   CTX_STRH('u','n','m','a','x','i','m','i','z','e',0,0,0,0)
#define CTX_width        CTX_STRH('w','i','d','t','h',0,0,0,0,0,0,0,0,0)
#define CTX_title        CTX_STRH('t','i','t','l','e',0,0,0,0,0,0,0,0,0)
#define CTX_action       CTX_STRH('a','c','t','i','o','n',0,0,0,0,0,0,0,0)
#define CTX_height       CTX_STRH('h','e','i','g','h','t',0,0,0,0,0,0,0,0)
#endif


static int vt_get_prop (VT *vt, const char *key, const char **val, int *len)
{
#if 0
  uint32_t key_hash = ctx_strhash (key);
  char str[4096]="";
  fprintf (stderr, "%s: %s %i\n", __FUNCTION__, key, key_hash);
  CtxClient *client = ctx_client_by_id (ct->id);
  if (!client)
    return 0;
  switch (key_hash)
  {
    case CTX_title:
      sprintf (str, "setkey %s %s\n", key, client->title);
      break;
    case CTX_x:      
      sprintf (str, "setkey %s %i\n", key, client->x);
      break;
    case CTX_y:    
      sprintf (str, "setkey %s %i\n", key, client->y);
      break;
    case CTX_width:
      sprintf (str, "setkey %s %i\n", key, client->width);
      break;
    case CTX_height:
      sprintf (str, "setkey %s %i\n", key, client->width);
      break;
    default:
      sprintf (str, "setkey %s undefined\n", key);
      break;
  }
  if (str[0])
  {
    vtpty_write ((void*)ct, str, strlen (str));
    fprintf (stderr, "%s", str);
  }
#endif
  return 0;
}

static void vtcmd_set_mode (VT *vt, const char *sequence)
{
  int set = 1;
  if (sequence[strlen (sequence)-1]=='l')
    { set = 0; }
  if (sequence[1]=='?')
    {
      int qval;
      sequence++;
qagain:
      qval = parse_int (sequence, 1);
      switch (qval)
        {
          case 1: /*MODE;DECCKM;Cursor key mode;Application;Cursor;*/
            vt->cursor_key_application = set;
            break;
          case 2: /*MODE;DECANM;VT52 emulation;on;off; */
            if (set==0)
              { vt->state = vt_state_vt52; }
            break;
          case 3: /*MODE;DECCOLM;Column mode;132 columns;80 columns;*/
            vtcmd_set_132_col (vt, set);
            break; // set 132 col
          case 4: /*MODE;DECSCLM;Scrolling mode;smooth;jump;*/
            vt->smooth_scroll = set;
            break; // set 132 col
          case 5: /*MODE;DECSCNM;Screen mode;Reverse;Normal;*/
            vt->reverse_video = set;
            break;
          case 6: /*MODE;DECOM;Origin mode;Relative;Absolute;*/
            vt->origin = set;
            if (set)
              {
                _vt_move_to (vt, vt->margin_top, 1);
                vt_carriage_return (vt);
              }
            else
              { _vt_move_to (vt, 1, 1); }
            break;
          case 7: /*MODE;DECAWM;Autowrap;on;off;*/
            vt->autowrap = set;
            break;
          case 8: /*MODE;DECARM;Auto repeat;on;off;*/
            vt->keyrepeat = set;
            break;
          // case 9: // send mouse x & y on button press 

          // 10 - Block DECEDM
          // 18 - Print form feed  DECPFF  default off
          // 19 - Print extent fullscreen DECPEX  default on
          case 12:
            vtcmd_ignore (vt, sequence);
            break; // blinking_cursor
          case 25:/*MODE;DECTCEM;Cursor visible;on;off; */
            vt->cursor_visible = set;
            break;
          case 30: // from rxvt - show/hide scrollbar
            break;
          case 34: // DECRLM - right to left mode
            break;
          case 38: // DECTEK - enter tektronix mode
            break;
          case 60: // horizontal cursor coupling
          case 61: // vertical cursor coupling
            break;
          case 69:/*MODE;DECVSSM;Left right margin mode;on;off; */
            vt->left_right_margin_mode = set;
            break;
          case 80:/* DECSDM Sixel scrolling */
            break;
          case 437:/*MODE;;Encoding/cp437mode;cp437;utf8; */
            vt->encoding = set ? 1 : 0;
            break;
          case 1000:/*MODE;;Mouse reporting;on;off;*/
            vt->mouse = set;
            break;
          case 1001:
          case 1002:/*MODE;;Mouse drag;on;off;*/
            vt->mouse_drag = set;
            break;
          case 1003:/*MODE;;Mouse all;on;off;*/
            vt->mouse_all = set;
            break;
          case 1006:/*MODE;;Mouse decimal;on;off;*/
            vt->mouse_decimal = set;
            break;
          case 47:
          case 1047:
          //case 1048:
          case 1049:/*MODE;;Alt screen;on;off;*/
            if (set)
              {
                if (vt->in_alt_screen)
                  {
                  }
                else
                  {
                    vtcmd_save_cursor (vt, "");
                    vt->saved_lines = vt->lines;
                    vt->saved_line_count = vt->line_count;
                    vt->line_count = 0;
                    vt->lines = NULL;
                    for (int i = 0; i <  vt->rows; i++)
                      {
                        vt->current_line = vt_line_new_with_size ("", vt->cols);
                        ctx_list_append (&vt->lines, vt->current_line);
                        vt->line_count++;
                      }
                    vt->in_alt_screen = 1;
                    vt->had_alt_screen = 1;
                    vt_line_feed (vt);
                    _vt_move_to (vt, 1, 1);
                    vt_carriage_return (vt);
                  }
              }
            else
              {
                if (vt->in_alt_screen)
                  {
                    while (vt->lines)
                      {
                        vt_line_free (vt->lines->data, 1);
                        ctx_list_remove (&vt->lines, vt->lines->data);
                      }
                    vt->line_count = vt->saved_line_count;
                    vt->lines = vt->saved_lines;
                    vtcmd_restore_cursor (vt, "");
                    vt->saved_lines = NULL;
                    vt->in_alt_screen = 0;
                  }
                else
                  {
                  }
              }
            break; // alt screen
          case 1010: /*MODE;;scroll on output;on;off; */ //rxvt
            vt->scroll_on_output = set;
            break;
          case 1011:/*MODE:;scroll on input;on;off; */ //rxvt)
            vt->scroll_on_input = set;
            break;
          case 2004:/*MODE;;bracketed paste;on;off; */
            vt->bracket_paste = set;
            break;
          case 201:/*MODE;;ctx-events;on;off;*/
            vt->ctx_events = set;
            break;
          
          case 200:/*MODE;;ctx vector graphics mode;on;;*/
            if (set)
              {
                if (!vt->current_line->ctx)
                  {
                    vt->current_line->ctx = ctx_new (vt->width, vt->height, "drawlist");
                    vt->current_line->ctx_copy = ctx_new (vt->width, vt->height, "drawlist");
                    ctx_set_texture_cache (vt->current_line->ctx_copy, vt->current_line->ctx);
                    _ctx_set_transformation (vt->current_line->ctx, 0);
                    _ctx_set_transformation (vt->current_line->ctx_copy, 0);

                    //ctx_set_texture_cache (vt->current_line->ctx, vt->current_line->ctx_copy);
                    //ctx_set_texture_cache (vt->current_line->ctx_copy, vt->current_line->ctx);
#if CTX_VT_USE_FRAME_DIFF
                    vt->current_line->frame = ctx_string_new ("");
#endif
                  }
                if (vt->ctxp)
                  ctx_parser_free (vt->ctxp);

                vt->ctxp = ctx_parser_new (vt->current_line->ctx,
                                           vt->cols * vt->cw, vt->rows * vt->ch,
                                           vt->cw, vt->ch, vt->cursor_x, vt->cursor_y,
                                           (void*)vt_set_prop, (void*)vt_get_prop, vt, vt_ctx_exit, vt);
                vt->utf8_holding[vt->utf8_pos=0]=0; // XXX : needed?
                vt->state = vt_state_ctx;
              }
            break;
          default:
            VT_warning ("unhandled CSI ? %i%s", qval, set?"h":"l");
            return;
        }
      if (strchr (sequence + 1, ';') )
        {
          sequence = strchr (sequence + 1, ';');
          goto qagain;
        }
    }
  else
    {
      int val;
again:
      val = parse_int (sequence, 1);
      switch (val)
        {
          case 1:/*GATM - transfer enhanced data */
          case 2:/*KAM - keyboard action mode */
            break;
          case 3:/*CRM - control representation mode */
            /* show control chars? */
            break;
          case 4:/*MODE2;IRM;Insert Mode;Insert;Replace; */
            vt->insert_mode = set;
            break;
          case 9: /* interlace mode */
            break;
          case 12:/*MODE2;SRM;Local echo;on;off; */
            vt->echo = set;
            break;
          case 20:/*MODE2;LNM;Carriage Return on LF/Newline;on;off;*/
            ;
            vt->cr_on_lf = set;
            break;
          case 21: // GRCM - whether SGR accumulates or a reset on each command
            break;
          case 32: // WYCRTSAVM - screen saver
            break;
          case 33: // WYSTCURM  - steady cursor
            break;
          case 34: // WYULCURM  - underline cursor
            break;
          default:
            VT_warning ("unhandled CSI %ih", val);
            return;
        }
      if (strchr (sequence, ';') && sequence[0] != ';')
        {
          sequence = strchr (sequence, ';');
          goto again;
        }
    }
}

static void vtcmd_request_mode (VT *vt, const char *sequence)
{
  char buf[64]="";
  if (sequence[1]=='?')
    {
      int qval;
      sequence++;
      qval = parse_int (sequence, 1);
      int is_set = -1; // -1 undefiend   0 reset 1 set  1000 perm_reset  1001 perm_set
      switch (qval)
        {
          case 1:
            is_set = vt->cursor_key_application;
            break;
          case 2: /*VT52 emulation;;enable; */
          //if (set==0) vt->in_vt52 = 1;
            is_set = 1001;
            break;
          case 3:
            is_set = 0;
            break;
          case 4:
            is_set = vt->smooth_scroll;
            break;
          case 5:
            is_set = vt->reverse_video;
            break;
          case 6:
            is_set = vt->origin;
            break;
          case 7:
            is_set = vt->autowrap;
            break;
          case 8:
            is_set = vt->keyrepeat;
            break;
          case 9: // should be dynamic
            is_set = 1000;
            break;
          case 10:
          case 11:
          case 12:
          case 13:
          case 14:
          case 16:
          case 18:
          case 19:
            is_set = 1000;
            break;
          case 25:
            is_set = vt->cursor_visible;
            break;
          case 45:
            is_set = 1000;
            break;
          case 47:
            is_set = vt->in_alt_screen;
            break;
          case 69:
            is_set = vt->left_right_margin_mode;
            break;
          case 437:
            is_set = vt->encoding;
            break;
          case 1000:
            is_set = vt->mouse;
            break;
            // 1001 hilite tracking
          case 1002:
            is_set = vt->mouse_drag;
            break;
          case 1003:
            is_set = vt->mouse_all;
            break;
          case 1006:
            is_set = vt->mouse_decimal;
            break;
          case 201:
            is_set = vt->ctx_events;
            break;
          case 2004:
            is_set = vt->bracket_paste;
            break;
          case 1010: // scroll to bottom on tty output (rxvt)
            is_set = vt->scroll_on_output;
            break;
          case 1011: // scroll to bottom on key press (rxvt)
            is_set = vt->scroll_on_input;
            break;
          case 1049:
            is_set = vt->in_alt_screen;
            break;
            break;
          case 200:/*ctx protocol;On;;*/
            is_set = (vt->state == vt_state_ctx);
            break;
          case 80:/* DECSDM Sixel scrolling */
          case 30: // from rxvt - show/hide scrollbar
          case 34: // DECRLM - right to left mode
          case 60: // horizontal cursor coupling
          case 61: // vertical cursor coupling
          default:
            break;
        }
      switch (is_set)
      {
        case 0:
          sprintf (buf, "\033[?%i;%i$y", qval, 2);
          break;
        case 1:
          { sprintf (buf, "\033[?%i;%i$y", qval, 1); }
          break;
        case 1000:
          sprintf (buf, "\033[?%i;%i$y", qval, 4);
          break;
        case 1001:
          sprintf (buf, "\033[?%i;%i$y", qval, 3);
          break;
        case -1:
          { sprintf (buf, "\033[?%i;%i$y", qval, 0); }
      }
    }
  else
    {
      int val;
      val = parse_int (sequence, 1);
      switch (val)
        {
          case 1:
            sprintf (buf, "\033[%i;%i$y", val, 0);
            break;
          case 2:/* AM - keyboard action mode */
            sprintf (buf, "\033[%i;%i$y", val, 0);
            break;
          case 3:/*CRM - control representation mode */
            sprintf (buf, "\033[%i;%i$y", val, 0);
            break;
          case 4:/*Insert Mode;Insert;Replace; */
            sprintf (buf, "\033[%i;%i$y", val, vt->insert_mode?1:2);
            break;
          case 9: /* interlace mode */
            sprintf (buf, "\033[%i;%i$y", val, 1);
            break;
          case 12:
            sprintf (buf, "\033[%i;%i$y", val, vt->echo?1:2);
            break;
          case 20:/*Carriage Return on LF/Newline;on;off;*/
            ;
            sprintf (buf, "\033[%i;%i$y", val, vt->cr_on_lf?1:2);
            break;
          case 21: // GRCM - whether SGR accumulates or a reset on each command
          default:
            sprintf (buf, "\033[%i;%i$y", val, 0);
        }
    }
  if (buf[0])
    { vt_write (vt, buf, strlen (buf) ); }
}

static void vtcmd_set_t (VT *vt, const char *sequence)
{
  /* \e[21y is request title - allows inserting keychars */
  if      (!strcmp (sequence,  "[1t")) { ctx_client_unshade (vt->root_ctx, vt->id); }
  else if (!strcmp (sequence,  "[2t")) { ctx_client_shade (vt->root_ctx, vt->id); } 
  else if (!strncmp (sequence, "[3;", 3)) {
    int x=0,y=0;
    sscanf (sequence, "[3;%i;%ir", &y, &x);
    ctx_client_move (vt->root_ctx, vt->id, x, y);
  }
  else if (!strncmp (sequence, "[4;", 3))
  {
    int width = 0, height = 0;
    sscanf (sequence, "[4;%i;%ir", &height , &width);
    if (width < 0) width = vt->cols * vt->cw;
    if (height < 0) height = vt->rows * vt->ch;
    if (width == 0) width = ctx_width (vt->root_ctx);
    if (height == 0) height = ctx_height (vt->root_ctx);
    ctx_client_resize (vt->root_ctx, vt->id, width, height);
  }
  else if (!strcmp (sequence, "[5t") ) { ctx_client_raise_top (vt->root_ctx, vt->id); } 
  else if (!strcmp (sequence, "[6t") ) { ctx_client_lower_bottom (vt->root_ctx, vt->id); } 
  else if (!strcmp (sequence, "[7t") ) { 
          ctx_client_rev_inc (vt->client);
          /* refresh */ }
  else if (!strncmp (sequence, "[8;", 3) )
  {
    int cols = 0, rows = 0;
    sscanf (sequence, "[8;%i;%ir", &rows, &cols);
    if (cols < 0) cols = vt->cols;
    if (rows < 0) rows = vt->rows;
    if (cols == 0) cols = ctx_width (vt->root_ctx) / vt->cw;
    if (rows == 0) rows = ctx_height (vt->root_ctx) / vt->ch;
    ctx_client_resize (vt->root_ctx, vt->id, cols * vt->cw, rows * vt->ch);
  }
  else if (!strcmp (sequence, "[9;0t") ) { ctx_client_unmaximize (vt->root_ctx, vt->id); } 
  else if (!strcmp (sequence, "[9;1t") ) { ctx_client_maximize (vt->root_ctx, vt->id);} 

  /* should actually be full-screen */
  else if (!strcmp (sequence, "[10;0t") ) { ctx_client_unmaximize (vt->root_ctx, vt->id); } 
  else if (!strcmp (sequence, "[10;1t") ) { ctx_client_maximize (vt->root_ctx, vt->id);} 
  else if (!strcmp (sequence, "[10;2t") ) { ctx_client_toggle_maximized (vt->root_ctx, vt->id);} 

  else if (!strcmp (sequence, "[11t") )  /* report window state  */
    {
      char buf[128];
      if (ctx_client_is_iconified (vt->root_ctx, vt->id))
        sprintf (buf, "\033[2t");
      else
        sprintf (buf, "\033[1t");
      vt_write (vt, buf, strlen (buf) );
    }
  else if (!strcmp (sequence, "[13t") ) /* request terminal position */
    {
      char buf[128];
      sprintf (buf, "\033[3;%i;%it", ctx_client_y (vt->root_ctx, vt->id), ctx_client_x (vt->root_ctx, vt->id));
      vt_write (vt, buf, strlen (buf) );
    }
  else if (!strcmp (sequence, "[14t") ) /* request terminal dimensions in px */
    {
      char buf[128];
      sprintf (buf, "\033[4;%i;%it", vt->rows * vt->ch, vt->cols * vt->cw);
      vt_write (vt, buf, strlen (buf) );
    }
  else if (!strcmp (sequence, "[15t") ) /* request root dimensions in px */
    {
      char buf[128];
      sprintf (buf, "\033[5;%i;%it", ctx_height (vt->root_ctx), ctx_width(vt->root_ctx));
      vt_write (vt, buf, strlen (buf) );
    }
  else if (!strcmp (sequence, "[16t") ) /* request char dimensions in px */
    {
      char buf[128];
      sprintf (buf, "\033[6;%i;%it", vt->ch, vt->cw);
      vt_write (vt, buf, strlen (buf) );
    }
  else if (!strcmp (sequence, "[18t") ) /* request terminal dimensions */
    {
      char buf[128];
      sprintf (buf, "\033[8;%i;%it", vt->rows, vt->cols);
      vt_write (vt, buf, strlen (buf) );
    }
  else if (!strcmp (sequence, "[19t") ) /* request root window size in char */
    {
      char buf[128];
      sprintf (buf, "\033[9;%i;%it", ctx_height(vt->root_ctx)/vt->ch, ctx_width (vt->root_ctx)/vt->cw);
      vt_write (vt, buf, strlen (buf) );
    }

#if 0
  {"[", 's',  foo, VT100}, /*args:PnSP id:DECSWBV Set warning bell volume */
#endif
  else if (sequence[strlen (sequence)-2]==' ') /* DECSWBV */
    {
      int val = parse_int (sequence, 0);
      if (val <= 1) { vt->bell = 0; }
      if (val <= 8) { vt->bell = val; }
    }
  else
    {
      // XXX: X for ints >=24 resize to that number of lines
      VT_info ("unhandled subsequence %s", sequence);
    }
}

static void _vt_htab (VT *vt)
{
  do
    {
      vt->cursor_x ++;
    }
  while (vt->cursor_x < VT_MARGIN_RIGHT &&  ! vt->tabs[ (int) vt->cursor_x-1]);
  if (vt->cursor_x > VT_MARGIN_RIGHT)
    { vt->cursor_x = VT_MARGIN_RIGHT; }
}

static void _vt_rev_htab (VT *vt)
{
  do
    {
      vt->cursor_x--;
    }
  while ( vt->cursor_x > 1 && ! vt->tabs[ (int) vt->cursor_x-1]);
  if (vt->cursor_x < VT_MARGIN_LEFT)
    { vt_carriage_return (vt); }
}

static void vtcmd_insert_n_tabs (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
    {
      _vt_htab (vt);
    }
}

static void vtcmd_rev_n_tabs (VT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
    {
      _vt_rev_htab (vt);
    }
}

static void vtcmd_set_double_width_double_height_top_line
(VT *vt, const char *sequence)
{
  vt->current_line->double_width         = 1;
  vt->current_line->double_height_top    = 1;
  vt->current_line->double_height_bottom = 0;
}
static void vtcmd_set_double_width_double_height_bottom_line
(VT *vt, const char *sequence)
{
  vt->current_line->double_width         = 1;
  vt->current_line->double_height_top    = 0;
  vt->current_line->double_height_bottom = 1;
}
static void vtcmd_set_single_width_single_height_line
(VT *vt, const char *sequence)
{
  vt->current_line->double_width         = 0;
  vt->current_line->double_height_top    = 0;
  vt->current_line->double_height_bottom = 0;
}
static void
vtcmd_set_double_width_single_height_line
(VT *vt, const char *sequence)
{
  vt->current_line->double_width         = 1;
  vt->current_line->double_height_top    = 0;
  vt->current_line->double_height_bottom = 0;
}

static void vtcmd_set_led (VT *vt, const char *sequence)
{
  int val = 0;
  //fprintf (stderr, "%s\n", sequence);
  for (const char *s = sequence; *s; s++)
    {
      switch (*s)
        {
          case '0': val = 0; break;
          case '1': val = 1; break;
          case '2': val = 2; break;
          case '3': val = 3; break;
          case '4': val = 4; break;
          case ';':
          case 'q':
            if (val == 0)
              { vt->leds[0] = vt->leds[1] = vt->leds[2] = vt->leds[3] = 0; }
            else
              { vt->leds[val-1] = 1; }
            val = 0;
            break;
        }
    }
}

static void vtcmd_char_at_cursor (VT *vt, const char *sequence)
{
  char *buf="";
  vt_write (vt, buf, strlen (buf) );
}

static void vtcmd_DECELR (VT *vt, const char *sequence)
{
  int ps1 = parse_int (sequence, 0);
  int ps2 = 0;
  const char *s = strchr (sequence, ';');
  if (ps1) {/* unused */};
  if (s)
    { ps2 = parse_int (s, 0); }
  if (ps2 == 1)
    { vt->unit_pixels = 1; }
  else
    { vt->unit_pixels = 0; }
}

static void vtcmd_graphics (VT *vt, const char *sequence)
{
  fprintf (stderr, "gfx intro [%s]\n",sequence); // maybe implement such as well?
}
void vt_audio_task (VT *vt, int click);

#if CTX_TILED
static void ctx_show_frame (Ctx *ctx, int block)
{
  CtxTiled *tiled = (CtxTiled*)(ctx->backend);
  tiled->show_frame (tiled, block);
}
#endif

static void ctx_wait_frame (Ctx *ctx, VT *vt)
{
#if CTX_TILED
  if (ctx_backend_is_tiled (ctx))
  {
    CtxTiled *tiled = (CtxTiled*)(ctx->backend);
    int max_wait    = 500;
    //int wait_frame  = tiled->frame;  // tiled->frame and tiled->render_frame are expected
                                       // to be equal, unless something else has timed out
    int wait_frame  = tiled->render_frame;
    ctx_show_frame (ctx, 0);
    while (wait_frame > tiled->shown_frame &&
           max_wait-- > 0)
    {
#if CTX_AUDIO
      usleep (5);
      vt_audio_task (vt, 0);
#else
      usleep (5);
#endif
      ctx_show_frame (ctx, 0);
    }
#if 1
    if (max_wait > 0)
    {}//fprintf (stderr, "[%i]", max_wait);
    else
      fprintf (stderr, "[wait-drop]");
#endif
  }
#endif
}

static void vtcmd_report (VT *vt, const char *sequence)
{
  char buf[64]="";
  if (!strcmp (sequence, "[5n") ) // DSR device status report
    {
      ctx_wait_frame (vt->root_ctx, vt);
      sprintf (buf, "\033[0n"); // we're always OK :)
    }
  else if (!strcmp (sequence, "[?15n") ) // printer status
    {
      sprintf (buf, "\033[?13n"); // no printer
    }
  else if (!strcmp (sequence, "[?26n") ) // keyboard dialect
    {
      sprintf (buf, "\033[?27;1n"); // north american/ascii
    }
  else if (!strcmp (sequence, "[?25n") ) // User Defined Key status
    {
      sprintf (buf, "\033[?21n"); // locked
    }
#if 0
  {"[6n", 0, },  /* id:DSR  cursor position report, yields a reply <tt>\e[Pl;PcR</tt> */
#endif
  else if (!strcmp (sequence, "[6n") ) // DSR cursor position report
    {
      sprintf (buf, "\033[%i;%iR", vt->cursor_y - (vt->origin? (vt->margin_top - 1) :0), (int) vt->cursor_x - (vt->origin? (VT_MARGIN_LEFT-1) :0) );
    }
  else if (!strcmp (sequence, "[?6n") ) // DECXPR extended cursor position report
    {
#if 0
  {"[?6n", 0, },  /* id:DEXCPR  extended cursor position report, yields a reply <tt>\e[Pl;PcR</tt> */
#endif
      sprintf (buf, "\033[?%i;%i;1R", vt->cursor_y - (vt->origin? (vt->margin_top - 1) :0), (int) vt->cursor_x - (vt->origin? (VT_MARGIN_LEFT-1) :0) );
    }
  else if (!strcmp (sequence, "[>c") )
    {
      sprintf (buf, "\033[>23;01;1c");
    }
  else if (sequence[strlen (sequence)-1]=='c') // device attributes
    {
      //buf = "\033[?1;2c"; // what rxvt reports
      //buf = "\033[?1;6c"; // VT100 with AVO ang GPO
      //buf = "\033[?2c";     // VT102
      sprintf (buf, "\033[?63;14;4;22c");
    }
  else if (sequence[strlen (sequence)-1]=='x') // terminal parameters
    {
      if (!strcmp (sequence, "[1x") )
        { sprintf (buf, "\033[3;1;1;120;120;1;0x"); }
      else
        { sprintf (buf, "\033[2;1;1;120;120;1;0x"); }
    }
  if (buf[0])
    { vt_write (vt, buf, strlen (buf) );
    }
}

static char *charmap_cp437[]=
{
  " ","☺","☻","♥","♦","♣","♠","•","◘","○","◙","♂","♀","♪","♫","☼",
  "►","◄","↕","‼","¶","§","▬","↨","↑","↓","→","←","∟","↔","▲","▼",
  " ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/",
  "0","1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
  "@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
  "P","Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
  "`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o",
  "p","q","r","s","t","u","v","w","x","y","z","{","|","}","~","⌂",
  "Ç","ü","é","â","ä","à","å","ç","ê","ë","è","ï","î","ì","Ä","Å",
  "É","æ","Æ","ô","ö","ò","û","ù","ÿ","Ö","Ü","¢","£","¥","₧","ƒ",
  "á","í","ó","ú","ñ","Ñ","ª","º","¿","⌐","¬","½","¼","¡","«","»",
  "░","▒","▓","│","┤","╡","╢","╖","╕","╣","║","╗","╝","╜","╛","┐",
  "└","┴","┬","├","─","┼","╞","╟","╚","╔","╩","╦","╠","═","╬","╧",
  "╨","╤","╥","╙","╘","╒","╓","╫","╪","┘","┌","█","▄","▌","▐","▀",
  "α","ß","Γ","π","Σ","σ","µ","τ","Φ","Θ","Ω","δ","∞","φ","ε","∩",
  "≡","±","≥","≤","⌠","⌡","÷","≈","°","∙","·","√","ⁿ","²","■"," "
};


static char *charmap_graphics[]=
{
  " ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/","0",
  "1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
  "@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
  "Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
  "◆","▒","␉","␌","␍","␊","°","±","␤","␋","┘","┐","┌","└","┼","⎺","⎻",
  "─","⎼","⎽","├","┤","┴","┬","│","≤","≥","π","≠","£","·"," "
};

static char *charmap_uk[]=
{
  " ","!","\"","£","$","%","&","'","(",")","*","+",",","-",".","/","0",
  "1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
  "@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
  "Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
  "`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p",
  "q","r","s","t","u","v","w","x","y","z","{","|","}","~"," "
};

static char *charmap_ascii[]=
{
  " ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/","0",
  "1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
  "@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
  "Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
  "`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p",
  "q","r","s","t","u","v","w","x","y","z","{","|","}","~"," "
};

static void vtcmd_justify (VT *vt, const char *sequence)
{
  int n = parse_int (vt->argument_buf, 0);
  switch (n)
    {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        vt->justify = n;
        break;
      default:
        vt->justify = 0;
    }
}

static void vtcmd_sixel_related_req (VT *vt, const char *sequence)
{
  fprintf (stderr, "it happens!\n");
}

static void vtcmd_set_charmap (VT *vt, const char *sequence)
{
  int slot = 0;
  int set = sequence[1];
  if (sequence[0] == ')') { slot = 1; }
  if (set == 'G') { set = 'B'; }
  vt->charset[slot] = set;
}
#if 0

CSI Pm ' }    '
Insert Ps Column (s) (default = 1) (DECIC), VT420 and up.

CSI Pm ' ~    '
Delete Ps Column (s) (default = 1) (DECDC), VT420 and up.


in text.  When bracketed paste mode is set, the program will receive:
ESC [ 2 0 0 ~,
      followed by the pasted text, followed by
      ESC [ 2 0 1 ~ .


            CSI I
            when the terminal gains focus, and CSI O  when it loses focus.
#endif

#define COMPAT_FLAG_LEVEL_0   (1<<1)
#define COMPAT_FLAG_LEVEL_1   (1<<2)
#define COMPAT_FLAG_LEVEL_2   (1<<3)
#define COMPAT_FLAG_LEVEL_3   (1<<4)
#define COMPAT_FLAG_LEVEL_4   (1<<5)
#define COMPAT_FLAG_LEVEL_5   (1<<6)

#define COMPAT_FLAG_LEVEL_102 (1<<7)

#define COMPAT_FLAG_ANSI      (1<<8)
#define COMPAT_FLAG_OBSOLETE  (1<<9)

#define COMPAT_FLAG_ANSI_COLOR (1<<10)
#define COMPAT_FLAG_256_COLOR  (1<<11)
#define COMPAT_FLAG_24_COLOR   (1<<12)

#define COMPAT_FLAG_MOUSE_REPORT (1<<13)

#define COMPAT_FLAG_AUDIO      (1<<14)
#define COMPAT_FLAG_GRAPHICS   (1<<15)

#define ANSI    COMPAT_FLAG_ANSI
#define OBS     COMPAT_FLAG_OBSOLETE

#define VT100   (COMPAT_FLAG_LEVEL_0|COMPAT_FLAG_LEVEL_1)
#define VT102   (VT100|COMPAT_FLAG_LEVEL_102)
#define VT200   (VT102|COMPAT_FLAG_LEVEL_2)
#define VT300   (VT200|COMPAT_FLAG_LEVEL_3)
#define VT400   (VT300|COMPAT_FLAG_LEVEL_4)
#define VT220   VT200
#define VT320   VT300

#define XTERM   (VT400|COMPAT_FLAG_24_COLOR|COMPAT_FLAG_256_COLOR|COMPAT_FLAG_ANSI_COLOR)

#define VT2020  (XTERM|COMPAT_FLAG_GRAPHICS|COMPAT_FLAG_AUDIO)


            static Sequence sequences[]=
  {
    /*
      prefix suffix  command */
    //{"B",  0,  vtcmd_break_permitted},
    //{"C",  0,  vtcmd_nobreak_here},
    {"D", 0,    vtcmd_index, VT100}, /* args: id:IND Index  */
    {"E",  0,   vtcmd_next_line}, /* ref:none id:  Next line */
    {"_", 'G',  vtcmd_graphics},
    {"H",   0,  vtcmd_horizontal_tab_set, VT100}, /* id:HTS Horizontal Tab Set */

    //{"I",  0,  vtcmd_char_tabulation_with_justification},
    //{"K",  0,  PLD partial line down
    //{"L",  0,  PLU partial line up
    {"M",  0,   vtcmd_reverse_index, VT100}, /* ref:none id:RI Reverse Index */
    //{"N",  0,  vtcmd_ignore}, /* Set Single Shift 2 - SS2*/
    //{"O",  0,  vtcmd_ignore}, /* Set Single Shift 3 - SS3*/

#if 0
    {"[0F", 0, vtcmd_justify, ANSI}, /* ref:none id:JFY disable justification and wordwrap  */ // needs special link to ANSI standard
    {"[1F", 0, vtcmd_justify, ANSI}, /* ref:none id:JFY enable wordwrap  */
#endif

    /* these need to occur before vtcmd_preceding_line to have precedence */
    {"[0 F", 0, vtcmd_justify, ANSI},
    {"[1 F", 0, vtcmd_justify, ANSI},
    {"[2 F", 0, vtcmd_justify},
    {"[3 F", 0, vtcmd_justify},
    {"[4 F", 0, vtcmd_justify},
    {"[5 F", 0, vtcmd_justify},
    {"[6 F", 0, vtcmd_justify},
    {"[7 F", 0, vtcmd_justify},
    {"[8 F", 0, vtcmd_justify},
// XXX missing DECIC DECDC  insert and delete column
    {"[", 'A', vtcmd_cursor_up, VT100},   /* args:Pn    id:CUU Cursor Up */
    {"[",  'B', vtcmd_cursor_down, VT100}, /* args:Pn    id:CUD Cursor Down */
    {"[",  'C', vtcmd_cursor_forward, VT100}, /* args:Pn id:CUF Cursor Forward */
    {"[",  'D', vtcmd_cursor_backward, VT100}, /* args:Pn id:CUB Cursor Backward */
    {"[",  'j', vtcmd_cursor_backward, ANSI}, /* args:Pn ref:none id:HPB Horizontal Position Backward */
    {"[",  'k', vtcmd_cursor_up, ANSI}, /* args:Pn ref:none id:VPB Vertical Position Backward */
    {"[",  'E', vtcmd_next_line, VT100}, /* args:Pn id:CNL Cursor Next Line */
    {"[",  'F', vtcmd_cursor_preceding_line, VT100}, /* args:Pn id:CPL Cursor Preceding Line */
    {"[",  'G', vtcmd_horizontal_position_absolute}, /* args:Pn id:CHA Cursor Horizontal Absolute */
    {"[",  'H', vtcmd_cursor_position, VT100}, /* args:Pl;Pc id:CUP Cursor Position */
    {"[",  'I', vtcmd_insert_n_tabs}, /* args:Pn id:CHT Cursor Horizontal Forward Tabulation */
    {"[",  'J', vtcmd_erase_in_display, VT100}, /* args:Ps id:ED Erase in Display */
    {"[",  'K', vtcmd_erase_in_line, VT100}, /* args:Ps id:EL Erase in Line */
    {"[",  'L', vtcmd_insert_blank_lines, VT102}, /* args:Pn id:IL Insert Line */
    {"[",  'M', vtcmd_delete_n_lines, VT102}, /* args:Pn id:DL Delete Line   */
    // [ N is EA - Erase in field
    // [ O is EA - Erase in area
    {"[",  'P', vtcmd_delete_n_chars, VT102}, /* args:Pn id:DCH Delete Character */
    // [ Q is SEE - Set editing extent
    // [ R is CPR - active cursor position report
    {"[?", 'S', vtcmd_sixel_related_req},
    {"[",  'S', vtcmd_scroll_up, VT100},   /* args:Pn id:SU Scroll Up */
    {"[",  'T', vtcmd_scroll_down, VT100}, /* args:Pn id:SD Scroll Down */
    {"[",/*SP*/'U', vtcmd_set_line_home, ANSI}, /* args:PnSP id=SLH Set Line Home */
    {"[",/*SP*/'V', vtcmd_set_line_limit, ANSI},/* args:PnSP id=SLL Set Line Limit */
    // [ W is cursor tabulation control
    // [ Pn Y  - cursor line tabulation
    //
    {"[",  'X', vtcmd_erase_n_chars}, /* args:Pn id:ECH Erase Character */
    {"[",  'Z', vtcmd_rev_n_tabs},    /* args:Pn id:CBT Cursor Backward Tabulation */
    {"[",  '^', vtcmd_scroll_down}  , /* muphry alternate from ECMA */
    {"[",  '@', vtcmd_insert_character, VT102}, /* args:Pn id:ICH Insert Character */

    {"[",  'a', vtcmd_cursor_forward, ANSI}, /* args:Pn id:HPR Horizontal Position Relative */
    {"[",  'b', vtcmd_cursor_forward, ANSI}, /* REP previous char XXX incomplete */
    {"[",  'c', vtcmd_report}, /* ref:none id:DA args:... Device Attributes */
    {"[",  'd', vtcmd_goto_row},       /* args:Pn id:VPA Vertical Position Absolute  */
    {"[",  'e', vtcmd_cursor_down},    /* args:Pn id:VPR Vertical Position Relative */
    {"[",  'f', vtcmd_cursor_position, VT100}, /* args:Pl;Pc id:HVP Cursor Position */
    {"[g", 0,   vtcmd_clear_current_tab, VT100}, /* id:TBC clear current tab */
    {"[0g", 0,  vtcmd_clear_current_tab, VT100}, /* id:TBC clear current tab */
    {"[3g", 0,  vtcmd_clear_all_tabs, VT100},    /* id:TBC clear all tabs */
    {"[",  'm', vtcmd_set_graphics_rendition, VT100}, /* args:Ps;Ps;.. id:SGR Select Graphics Rendition */
    {"[",  'n', vtcmd_report, VT200}, /* id:DSR args:... CPR Cursor Position Report  */
    {"[",  'r', vtcmd_set_top_and_bottom_margins, VT100}, /* args:Pt;Pb id:DECSTBM Set Top and Bottom Margins */
#if 0
    // handled by set_left_and_right_margins - in if 0 to be documented
    {"[s",  0,  vtcmd_save_cursor_position, VT100}, /*ref:none id:SCP Save Cursor Position */
#endif
    {"[u",  0,  vtcmd_restore_cursor_position, VT100}, /*ref:none id:RCP Restore Cursor Position */
    {"[",  's', vtcmd_set_left_and_right_margins, VT400}, /* args:Pl;Pr id:DECSLRM Set Left and Right Margins */
    {"[",  '`', vtcmd_horizontal_position_absolute, ANSI},  /* args:Pn id:HPA Horizontal Position Absolute */

    {"[",  'h', vtcmd_set_mode, VT100},   /* args:Pn[;...] id:SM Set Mode */
    {"[",  'l', vtcmd_set_mode, VT100}, /* args:Pn[;...]  id:RM Reset Mode */
    {"[",  't', vtcmd_set_t},
    {"[",  'q', vtcmd_set_led, VT100}, /* args:Ps id:DECLL Load LEDs */
    {"[",  'x', vtcmd_report}, /* ref:none id:DECREQTPARM */
    {"[",  'z', vtcmd_DECELR}, /* ref:none id:DECELR set locator res  */

    {"5",   0,  vtcmd_char_at_cursor, VT300}, /* ref:none id:DECXMIT */
    {"6",   0,  vtcmd_back_index, VT400}, /* id:DECBI Back index (hor. scroll) */
    {"7",   0,  vtcmd_save_cursor, VT100}, /* id:DECSC Save Cursor */
    {"8",   0,  vtcmd_restore_cursor, VT100}, /* id:DECRC Restore Cursor */
    {"9",   0,  vtcmd_forward_index, VT400}, /* id:DECFI Forward index (hor. scroll)*/

    //{"Z", 0,  vtcmd_device_attributes},
    //{"%G",0,  vtcmd_set_default_font}, // set_alternate_font


    {"(0",  0,   vtcmd_set_charmap},
    {"(1",  0,   vtcmd_set_charmap},
    {"(2",  0,   vtcmd_set_charmap},
    {"(A",  0,   vtcmd_set_charmap},
    {"(B",  0,   vtcmd_set_charmap},
    {")0",  0,   vtcmd_set_charmap},
    {")1",  0,   vtcmd_set_charmap},
    {")2",  0,   vtcmd_set_charmap},
    {")A",  0,   vtcmd_set_charmap},
    {")B",  0,   vtcmd_set_charmap},
    {"%G",  0,   vtcmd_set_charmap},

    {"#3",  0,   vtcmd_set_double_width_double_height_top_line, VT100}, /*id:DECDHL Top half of double-width, double-height line */
    {"#4",  0,   vtcmd_set_double_width_double_height_bottom_line, VT100}, /*id:DECDHL Bottom half of double-width, double-height line */
    {"#5",  0,   vtcmd_set_single_width_single_height_line, VT100}, /* id:DECSWL Single-width line */
    {"#6",  0,   vtcmd_set_double_width_single_height_line, VT100}, /* id:DECDWL Double-width line */

    {"#8",  0,   vtcmd_screen_alignment_display, VT100}, /* id:DECALN Screen Alignment Pattern */
    {"=",   0,   vtcmd_ignore},  // keypad mode change
    {">",   0,   vtcmd_ignore},  // keypad mode change
    {"c",   0,   vtcmd_reset_to_initial_state, VT100}, /* id:RIS Reset to Initial State */
    {"[!", 'p',  vtcmd_ignore},       // soft reset?
    {"[",  'p',  vtcmd_request_mode}, /* args:Pa$ id:DECRQM Request ANSI Mode */
#if 0
    {"[?",  'p',  vtcmd_request_mode}, /* args:Pd$ id:DECRQM Request DEC Mode */
#endif

    {NULL, 0, NULL}
  };

  static void handle_sequence (VT *vt, const char *sequence)
{
  int i0 = strlen (sequence)-1;
  int i;
  ctx_client_rev_inc (vt->client);
  for (i = 0; sequences[i].prefix; i++)
    {
      if (!strncmp (sequence, sequences[i].prefix, strlen (sequences[i].prefix) ) )
        {
          if (! (sequences[i].suffix && (sequence[i0] != sequences[i].suffix) ) )
            {
              VT_command ("%s", sequence);
              sequences[i].vtcmd (vt, sequence);
              return;
            }
        }
    }
#ifndef ASANBUILD
  VT_warning ("unhandled: %c%c%c%c%c%c%c%c%c\n", sequence[0], sequence[1], sequence[2], sequence[3], sequence[4], sequence[5], sequence[6], sequence[7], sequence[8]);
#endif
}

static void vt_line_feed (VT *vt)
{
  int was_home = vt->at_line_home;
  if (vt->margin_top == 1 && vt->margin_bottom == vt->rows)
    {
      if (vt->lines == NULL ||
          (vt->lines->data == vt->current_line && vt->cursor_y != vt->rows) )
        {
          vt->current_line = vt_line_new_with_size ("", vt->cols);
          ctx_list_prepend (&vt->lines, vt->current_line);
          vt->line_count++;
        }
      if (vt->cursor_y >= vt->margin_bottom)
        {
          vt->cursor_y = vt->margin_bottom;
          vt_scroll (vt, -1);
        }
      else
        {
          vt->cursor_y++;
        }
    }
  else
    {
      if (vt->lines->data == vt->current_line &&
          (vt->cursor_y != vt->margin_bottom) && 0)
        {
          vt->current_line = vt_line_new_with_size ("", vt->cols);
          ctx_list_prepend (&vt->lines, vt->current_line);
          vt->line_count++;
        }
      vt->cursor_y++;
      if (vt->cursor_y > vt->margin_bottom)
        {
          vt->cursor_y = vt->margin_bottom;
          vt_scroll (vt, -1);
        }
    }
  _vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  if (vt->cr_on_lf)
    { vt_carriage_return (vt); }
  vt_trimlines (vt, vt->rows);
  if (was_home)
    { vt_carriage_return (vt); }
}

//#include "vt-encodings.h"

#if CTX_SDL
static void vt_state_apc_audio (VT *vt, int byte)
{
  if ( (byte < 32) && ( (byte < 8) || (byte > 13) ) )
    {
      vt_audio (vt, vt->argument_buf);
      vt->state = ( (byte == 27) ?  vt_state_swallow : vt_state_neutral);
    }
  else
    {
      vt_argument_buf_add (vt, byte);
    }
}

#else

void vt_audio_task (VT *vt, int click)
{
}

void vt_bell (VT *vt)
{
}
static void vt_state_apc_audio (VT *vt, int byte)
{
  vt->state = vt_state_apc_generic;
}

#endif

static void
vt_carriage_return (VT *vt)
{
  _vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  vt->cursor_x = VT_MARGIN_LEFT;
  vt->at_line_home = 1;
}

/* if the byte is a non-print control character, handle it and return 1
 * oterhwise return 0*/
static int _vt_handle_control (VT *vt, int byte)
{
  /* the big difference between ANSI-BBS mode and VT100+ mode is that
   * most C0 characters are printable
   */
  if (CTX_UNLIKELY(vt->encoding == 1)) // this codepage is for ansi-bbs use
    switch (byte)
      {
        case '\0':
          return 1;
        case 1:    /* SOH start of heading */
        case 2:    /* STX start of text */
        case 3:    /* ETX end of text */
        case 4:    /* EOT end of transmission */
        case 5:    /* ENQuiry */
        case 6:    /* ACKnolwedge */
        case '\v': /* VT vertical tab */
        case '\f': /* VF form feed */
        case 14: /* SO shift in - alternate charset */
        case 15: /* SI shift out - (back to normal) */
        case 16: /* DLE data link escape */
        case 17: /* DC1 device control 1 - XON */
        case 18: /* DC2 device control 2 */
        case 19: /* DC3 device control 3 - XOFF */
        case 20: /* DC4 device control 4 */
        case 21: /* NAK negative ack */
        case 22: /* SYNchronous idle */
        case 23: /* ETB end of trans. blk */
        case 24: /* CANcel (vt100 aborts sequence) */
        case 25: /* EM  end of medium */
        case 26: /* SUB stitute */
        case 28: /* FS file separator */
        case 29: /* GS group separator */
        case 30: /* RS record separator */
        case 31: /* US unit separator */
          _vt_add_str (vt, charmap_cp437[byte]);
          return 1;
      }
  switch (byte)
    {
      case '\0':
      case 1:    /* SOH start of heading */
      case 2:    /* STX start of text */
      case 3:    /* ETX end of text */
      case 4:    /* EOT end of transmission */
      case 6:    /* ACKnolwedge */
        return 1;
      case 5:    /* ENQuiry */
        {
          const char *reply = getenv ("TERM_ENQ_REPLY");
          if (reply)
            {
              char *copy = strdup (reply);
              for (uint8_t *c = (uint8_t *) copy; *c; c++)
                {
                  if (*c < ' ' || * c > 127) { *c = 0; }
                }
              vt_write (vt, reply, strlen (reply) );
              free (copy);
            }
        }
        return 1;
      case '\a': /* BELl */
        vt_bell (vt);
        return 1;
      case '\b': /* BS */
        _vt_backspace (vt);
        return 1;
      case '\t': /* HT tab */
        _vt_htab (vt);
        return 1;
      case '\v': /* VT vertical tab */
      case '\f': /* VF form feed */
      case '\n': /* LF line ffed */
        vt_line_feed (vt);
        return 1;
      case '\r': /* CR carriage return */
        vt_carriage_return (vt);
        return 1;
      case 14: /* SO shift in - alternate charset */
        vt->shifted_in = 1;  // XXX not in vt52
        return 1;
      case 15: /* SI shift out - (back to normal) */
        vt->shifted_in = 0;
        return 1;
      case 16: /* DLE data link escape */
      case 17: /* DC1 device control 1 - XON */
      case 18: /* DC2 device control 2 */
      case 19: /* DC3 device control 3 - XOFF */
      case 20: /* DC4 device control 4 */
      case 21: /* NAK negative ack */
      case 22: /* SYNchronous idle */
      case 23: /* ETB end of trans. blk */
      case 24: /* CANcel (vt100 aborts sequence) */
      case 25: /* EM  end of medium */
      case 26: /* SUB stitute */
        _vt_add_str (vt, "¿");  // in vt52? XXX
        return 1;
      case 27: /* ESCape */
        return 0;
        break;
      case 28: /* FS file separator */
      case 29: /* GS group separator */
      case 30: /* RS record separator */
      case 31: /* US unit separator */
      case 127: /* DEL */
        return 1;
      default:
        return 0;
    }
}

void vt_open_log (VT *vt, const char *path)
{
  unlink (path);
  vt->log = fopen (path, "w");
}

/* the function shared by sixels, kitty mode and iterm2 mode for
 * doing inline images. it attaches an image to the current line
 */
static void display_image (VT *vt, Image *image,
                           int col,
                           float xoffset,
                           float yoffset,
                           int rows,
                           int cols,
                           int subx,
                           int suby,
                           int subw,
                           int subh
                          )
{
  int i = 0;
  for (i = 0; vt->current_line->images[i] && i < 4; i++)
  {
     if (vt->current_line->image_col[i] == vt->cursor_x)
       break;
  }
  //for (i = 0; vt->current_line->images[i] && i < 4; i++);
  if (i >= 4) { i = 3; }
  /* this needs a struct and dynamic allocation */
  vt->current_line->images[i] = image;
  vt->current_line->image_col[i] = vt->cursor_x;
  vt->current_line->image_X[i] = xoffset;
  vt->current_line->image_Y[i] = yoffset;
  vt->current_line->image_subx[i] = subx;
  vt->current_line->image_suby[i] = suby;
  vt->current_line->image_subw[i] = subw;
  vt->current_line->image_subh[i] = subh;
  vt->current_line->image_rows[i] = rows;
  vt->current_line->image_cols[i] = cols;
}

static int vt_gfx_pending=0;

void vt_gfx (VT *vt, const char *command)
{
  const char *payload = NULL;
  char key = 0;
  int  value;
  int  pos = 1;
  if (vt->gfx.multichunk == 0)
    {
      memset (&vt->gfx, 0, sizeof (GfxState) );
      vt->gfx.action='t';
      vt->gfx.transmission='d';
    }
  while (command[pos] != ';')
    {
      pos ++; // G or ,
      if (command[pos] == ';') { break; }
      key = command[pos];
      pos++;
      if (command[pos] == ';') { break; }
      pos ++; // =
      if (command[pos] == ';') { break; }
      if (command[pos] >= '0' && command[pos] <= '9')
        { value = atoi (&command[pos]); }
      else
        { value = command[pos]; }
      while (command[pos] &&
             command[pos] != ',' &&
             command[pos] != ';') { pos++; }
      switch (key)
        {
          case 'a':
            vt->gfx.action = value;
            break;
          case 'd':
            vt->gfx.delete = value;
            break;
          case 'i':
            vt->gfx.id = value;
            break;
          case 'S':
            vt->gfx.buf_size = value;
            break;
          case 's':
            vt->gfx.buf_width = value;
            break;
          case 'v':
            vt->gfx.buf_height = value;
            break;
          case 'f':
            vt->gfx.format = value;
            break;
          case 'm':
            vt->gfx.multichunk = value;
            break;
          case 'o':
            vt->gfx.compression = value;
            break;
          case 't':
            vt->gfx.transmission = value;
            break;
          case 'x':
            vt->gfx.x = value;
            break;
          case 'y':
            vt->gfx.y = value;
            break;
          case 'w':
            vt->gfx.w = value;
            break;
          case 'h':
            vt->gfx.h = value;
            break;
          case 'X':
            vt->gfx.x_cell_offset = value;
            break;
          case 'Y':
            vt->gfx.y_cell_offset = value;
            break;
          case 'c':
            vt->gfx.columns = value;
            break;
          case 'r':
            vt->gfx.rows = value;
            break;
          case 'z':
            vt->gfx.z_index = value;
            break;
        }
    }
  payload = &command[pos+1];
  {
    int chunk_size = strlen (payload);
    int old_size = vt->gfx.data_size;
    // accumulate incoming data
    if (vt->gfx.data == NULL)
      {
        vt->gfx.data_size = chunk_size;
        vt->gfx.data = malloc (vt->gfx.data_size + 1);
      }
    else
      {
        vt->gfx.data_size += chunk_size;
        vt->gfx.data = realloc (vt->gfx.data, vt->gfx.data_size + 1);
      }
    memcpy (vt->gfx.data + old_size, payload, chunk_size);
    vt->gfx.data[vt->gfx.data_size]=0;
  }
  if (vt->gfx.multichunk == 0)
    {
      if (vt->gfx.transmission != 'd') /* */
        {
          char buf[256];
          sprintf (buf, "\033_Gi=%i;only direct transmission supported\033\\",
                   vt->gfx.id);
          vt_write (vt, buf, strlen (buf) );
          goto cleanup;
        }
      {
        int bin_length = vt->gfx.data_size;
        uint8_t *data2 = malloc (vt->gfx.data_size);
        bin_length = ctx_base642bin ( (char *) vt->gfx.data,
                                     &bin_length,
                                     data2);
        memcpy (vt->gfx.data, data2, bin_length + 1);
        vt->gfx.data_size = bin_length;
        free (data2);
      }
      if (vt->gfx.buf_width)
        {
          // implicit buf_size
          vt->gfx.buf_size = vt->gfx.buf_width * vt->gfx.buf_height *
                             (vt->gfx.format == 24 ? 3 : 4);
        }
      if (vt->gfx.compression == 'z')
        {
          //vt->gfx.buf_size)
          unsigned char *data2 = malloc (vt->gfx.buf_size + 1);
          /* if a buf size is set (rather compression, but
           * this works first..) then */
#ifndef EMSCRIPTEN
      unsigned long
#else
      unsigned int
#endif
          actual_uncompressed_size = vt->gfx.buf_size;
          int z_result = uncompress (data2, &actual_uncompressed_size,
                                     vt->gfx.data,
                                     vt->gfx.data_size);
          if (z_result != Z_OK)
            {
              char buf[256]= "\033_Go=z;zlib error\033\\";
              vt_write (vt, buf, strlen (buf) );
              goto cleanup;
            }
          free (vt->gfx.data);
          vt->gfx.data = data2;
          vt->gfx.data_size = actual_uncompressed_size;
          vt->gfx.compression = 0;
        }
#ifdef STBI_INCLUDE_STB_IMAGE_H
      if (vt->gfx.format == 100)
        {
          int channels;
          uint8_t *new_data = stbi_load_from_memory (vt->gfx.data, vt->gfx.data_size, &vt->gfx.buf_width, &vt->gfx.buf_height, &channels, 4);
          if (!new_data)
            {
              char buf[256]= "\033_Gf=100;image decode error\033\\";
              vt_write (vt, buf, strlen (buf) );
              goto cleanup;
            }
          vt->gfx.format = 32;
          free (vt->gfx.data);
          vt->gfx.data = new_data;
          vt->gfx.data_size= vt->gfx.buf_width * vt->gfx.buf_height * 4;
        }
#endif
      Image *image = NULL;
      switch (vt->gfx.action)
        {
          case 't': // transfer
          case 'T': // transfer and present
            switch (vt->gfx.format)
              {
                case 24:
                case 32:
                  image = image_add (vt->gfx.buf_width, vt->gfx.buf_height, vt->gfx.id,
                                     vt->gfx.format, vt->gfx.data_size, vt->gfx.data);
                  vt->gfx.data = NULL;
                  vt->gfx.data_size=0;
                  break;
              }
            if (vt->gfx.action == 't')
              { break; }
          // fallthrough
          case 'p': // present
            if (!image && vt->gfx.id)
              { image = image_query (vt->gfx.id); }
            if (image)
              {
                display_image (vt, image, vt->cursor_x, vt->gfx.rows, vt->gfx.columns,
                               vt->gfx.x_cell_offset * 1.0 / vt->cw,
                               vt->gfx.y_cell_offset * 1.0 / vt->ch,
                               vt->gfx.x,
                               vt->gfx.y,
                               vt->gfx.w,
                               vt->gfx.h);
                int right = (image->width + (vt->cw-1) ) /vt->cw;
                int down = (image->height + (vt->ch-1) ) /vt->ch;
                for (int i = 0; i<down - 1; i++)
                  { vtcmd_index (vt, " "); }
                for (int i = 0; i<right; i++)
                  { vtcmd_cursor_forward (vt, " "); }
              }
            break;
          case 'q': // query
            if (image_query (vt->gfx.id) )
              {
                char buf[256];
                sprintf (buf, "\033_Gi=%i;OK\033\\", vt->gfx.id);
                vt_write (vt, buf, strlen (buf) );
              }
            break;
          case 'd': // delete
            {
              int row = vt->rows; // probably not right at start of session XXX
              for (CtxList *l = vt->lines; l; l = l->next, row --)
                {
                  VtLine *line = l->data;
                  for (int i = 0; i < 4; i ++)
                    {
                      int free_resource = 0;
                      int match = 0;
                      if (line->images[i])
                        switch (vt->gfx.delete)
                          {
                            case 'A':
                              free_resource = 1;
                              /* FALLTHROUGH */
                            case 'a': /* all images visible on screen */
                              match = 1;
                              break;
                            case 'I':
                              free_resource = 1;
                              /* FALLTHROUGH */
                            case 'i': /* all images with specified id */
                              if ( ( (Image *) (line->images[i]) )->id == vt->gfx.id)
                                { match = 1; }
                              break;
                            case 'P':
                              free_resource = 1;
                              /* FALLTHROUGH */
                            case 'p': /* all images intersecting cell
          specified with x and y */
                              if (line->image_col[i] == vt->gfx.x &&
                                  row == vt->gfx.y)
                                { match = 1; }
                              break;
                            case 'Q':
                              free_resource = 1;
                              /* FALLTHROUGH */
                            case 'q': /* all images with specified cell (x), row(y) and z */
                              if (line->image_col[i] == vt->gfx.x &&
                                  row == vt->gfx.y)
                                { match = 1; }
                              break;
                            case 'Y':
                              free_resource = 1;
                              /* FALLTHROUGH */
                            case 'y': /* all images with specified row (y) */
                              if (row == vt->gfx.y)
                                { match = 1; }
                              break;
                            case 'X':
                              free_resource = 1;
                              /* FALLTHROUGH */
                            case 'x': /* all images with specified column (x) */
                              if (line->image_col[i] == vt->gfx.x)
                                { match = 1; }
                              break;
                            case 'Z':
                              free_resource = 1;
                              /* FALLTHROUGH */
                            case 'z': /* all images with specified z-index (z) */
                              break;
                          }
                      if (match)
                        {
                          line->images[i] = NULL;
                          if (free_resource)
                            {
                              // XXX : NYI
                            }
                        }
                    }
                }
            }
            break;
        }
cleanup:
      if (vt->gfx.data)
        { free (vt->gfx.data); }
      vt->gfx.data = NULL;
      vt->gfx.data_size=0;
      vt->gfx.multichunk=0;
      vt_gfx_pending = 0;
    }
  else
     vt_gfx_pending = 1;
}

static void vt_state_vt52 (VT *vt, int byte)
{
  /* in vt52 mode, utf8_pos being non 0 means we got ESC prior */
  switch (vt->utf8_pos)
    {
      case 0:
        if (_vt_handle_control (vt, byte) == 0)
          switch (byte)
            {
              case 27: /* ESC */
                vt->utf8_pos = 1;
                break;
              default:
                {
                  char str[2] = {byte & 127, 0};
                  /* we're not validating utf8, and our utf8 manipulation
                   * functions are not robust against malformed utf8,
                   * hence we strip to ascii
                   */
                  _vt_add_str (vt, str);
                }
                break;
            }
        break;
      case 1:
        vt->utf8_pos = 0;
        switch (byte)
          {
            case 'A':
              vtcmd_cursor_up (vt, " ");
              break;
            case 'B':
              vtcmd_cursor_down (vt, " ");
              break;
            case 'C':
              vtcmd_cursor_forward (vt, " ");
              break;
            case 'D':
              vtcmd_cursor_backward (vt, " ");
              break;
            case 'F':
              vtcmd_set_alternate_font (vt, " ");
              break;
            case 'G':
              vtcmd_set_default_font (vt, " ");
              break;
            case 'H':
              _vt_move_to (vt, 1, 1);
              break;
            case 'I':
              vtcmd_reverse_index (vt, " ");
              break;
            case 'J':
              vtcmd_erase_in_display (vt, "[0J");
              break;
            case 'K':
              vtcmd_erase_in_line (vt, "[0K");
              break;
            case 'Y':
              vt->utf8_pos = 2;
              break;
            case 'Z':
              vt_write (vt, "\033/Z", 3);
              break;
            case '<':
              vt->state = vt_state_neutral;
              break;
            default:
              break;
          }
        break;
      case 2:
        _vt_move_to (vt, byte - 31, vt->cursor_x);
        vt->utf8_pos = 3;
        break;
      case 3:
        _vt_move_to (vt, vt->cursor_y, byte - 31);
        vt->utf8_pos = 0;
        break;
    }
}

static void vt_sixels (VT *vt, const char *sixels)
{
  uint8_t colors[256][3];
  int width = 0;
  int height = 0;
  int x = 0;
  int y = 0;
  Image *image;
  uint8_t *pixels = NULL;
  int repeat = 1;
  const char *p = sixels;
  int pal_no = 0;
#if 0
  for (; *p && *p != ';'; p++);
  if (*p == ';') { p ++; }
  printf ("%i:[%c]%i\n", __LINE__, *p, atoi (p) );
  // should be 0
  for (; *p && *p != ';'; p++);
  if (*p == ';') { p ++; }
  printf ("%i:[%c]%i\n", __LINE__, *p, atoi (p) );
  // if 1 then transparency is enabled - otherwise use bg color
  for (; *p && *p != 'q'; p++);
#endif
  //for (; *p && *p != '"'; p++);
  while (*p && *p != 'q') { p++; }
  if (*p == 'q') { p++; }
  if (*p == '"') { p++; }
  //printf ("%i:[%c]%i\n", __LINE__, *p, atoi (p));
  for (; *p && *p != ';'; p++);
  if (*p == ';') { p ++; }
  //printf ("%i:[%c]%i\n", __LINE__, *p, atoi (p));
  for (; *p && *p != ';'; p++);
  if (*p == ';') { p ++; }
  width = atoi (p);
  for (; *p && *p != ';'; p++);
  if (*p == ';') { p ++; }
  height = atoi (p);
  if (width * height > 2048 * 2048)
    return;
  if (width <= 0 || height <=0)
    {
      width = 0;
      height = 0;
      // XXX  : a copy paste dry-run
      for (const char *t=p; *t; t++)
        {
          if (*t == '#')
            {
              t++;
              while (*t && *t >= '0' && *t <= '9') { t++; }
              if (*t == ';')
                {
                  for (; *t && *t != ';'; t++);
                  if (*t == ';') { t ++; }
                  for (; *t && *t != ';'; t++);
                  if (*t == ';') { t ++; }
                  for (; *t && *t != ';'; t++);
                  if (*t == ';') { t ++; }
                  for (; *t && *t != ';'; t++);
                  if (*t == ';') { t ++; }
                  while (*t && *t >= '0' && *t <= '9') { t++; }
                  t--;
                }
              else
                {
                  t--;
                }
            }
          else if (*t == '$') // carriage return
            {
              if (x > width) { width = x; }
              x = 0;
            }
          else if (*t == '-') // line feed
            {
              y += 6;
              x = 0;
            }
          else if (*t == '!') // repeat
            {
              t++;
              repeat = atoi (t);
              while (*t && *t >= '0' && *t <= '9') { t++; }
              t--;
            }
          else if (*t >= '?' && *t <= '~') /* sixel data */
            {
              x += repeat;
              repeat = 1;
            }
        }
      height = y;
    }
  x = 0;
  y = 0;
  pixels = calloc (width * (height + 6), 4);
  image = image_add (width, height, 0,
                     32, width*height*4, pixels);
  uint8_t *dst = pixels;
  for (; *p; p++)
    {
      if (*p == '#')
        {
          p++;
          pal_no = atoi (p);
          if (pal_no < 0 || pal_no > 255) { pal_no = 255; }
          while (*p && *p >= '0' && *p <= '9') { p++; }
          if (*p == ';')
            {
              /* define a palette */
              for (; *p && *p != ';'; p++);
              if (*p == ';') { p ++; }
              // color_model , 2 is rgb
              for (; *p && *p != ';'; p++);
              if (*p == ';') { p ++; }
              colors[pal_no][0] = atoi (p) * 255 / 100;
              for (; *p && *p != ';'; p++);
              if (*p == ';') { p ++; }
              colors[pal_no][1] = atoi (p) * 255 / 100;
              for (; *p && *p != ';'; p++);
              if (*p == ';') { p ++; }
              colors[pal_no][2] = atoi (p) * 255 / 100;
              while (*p && *p >= '0' && *p <= '9') { p++; }
              p--;
            }
          else
            {
              p--;
            }
        }
      else if (*p == '$') // carriage return
        {
          x = 0;
          dst = &pixels[ (4 * width * y)];
        }
      else if (*p == '-') // line feed
        {
          y += 6;
          x = 0;
          dst = &pixels[ (4 * width * y)];
        }
      else if (*p == '!') // repeat
        {
          p++;
          repeat = atoi (p);
          while (*p && *p >= '0' && *p <= '9') { p++; }
          p--;
        }
      else if (*p >= '?' && *p <= '~') /* sixel data */
        {
          int sixel = (*p) - '?';
          if (x + repeat <= width && y < height)
            {
              for (int bit = 0; bit < 6; bit ++)
                {
                  if (sixel & (1 << bit) )
                    {
                      for (int u = 0; u < repeat; u++)
                        {
                          for (int c = 0; c < 3; c++)
                            {
                              dst[ (bit * width * 4) + u * 4 + c] = colors[pal_no][c];
                              dst[ (bit * width * 4) + u * 4 + 3] = 255;
                            }
                        }
                    }
                }
            }
          x   += repeat;
          dst += (repeat * 4);
          repeat = 1;
        }
    }
  if (image)
    {
      display_image (vt, image, vt->cursor_x, 0,0, 0.0, 0.0, 0,0,0,0);
      int right = (image->width + (vt->cw-1) ) /vt->cw;
      int down = (image->height + (vt->ch-1) ) /vt->ch;
      for (int i = 0; i<down - 1; i++)
        { vtcmd_index (vt, " "); }
      for (int i = 0; i<right; i++)
        { vtcmd_cursor_forward (vt, " "); }
      vt_line_feed (vt);
      vt_carriage_return (vt);
    }
  ctx_client_rev_inc (vt->client);
}

static inline void vt_ctx_unrled (VT *vt, char byte)
{
#if CTX_VT_USE_FRAMEDIFF
  ctx_string_append_byte (vt->current_line->frame, byte);
#endif
  ctx_parser_feed_byte (vt->ctxp, byte);
}

static void vt_state_ctx (VT *vt, int byte)
{
#if 0
  //fprintf (stderr, "%c", byte);
  if (CTX_UNLIKELY(byte == CTX_CODEC_CHAR))
  {
    if (CTX_UNLIKELY(vt->in_prev_match))
    {
      char *prev = vt->current_line->prev;
      int prev_length = vt->current_line->prev_length;
      int start = atoi (vt->reference);
      int len = 0;
      if (strchr (vt->reference, ' '))
        len = atoi (strchr (vt->reference, ' ')+1);

      //fprintf (stderr, "%i-%i:", start, len);

      if (start < 0) start = 0;
      if (start >= (prev_length))start = prev_length-1;
      if (len + start > prev_length)
        len = prev_length - start;

      //fprintf (stderr, "%i-%i\n", start, len);

      if (CTX_UNLIKELY (start == 0 && len == 0))
      {
        vt_ctx_unrled (vt, CTX_CODEC_CHAR);
      }
      else
      {
        if (prev)
        for (int i = 0; i < len && start + i < prev_length; i++)
        {
          vt_ctx_unrled (vt, prev[start + i]);
        }
      }
      vt->ref_len = 0;
      vt->reference[0]=0;
      vt->in_prev_match = 0;
    }
    else
    {
      vt->reference[0]=0;
      vt->ref_len = 0;
      vt->in_prev_match = 1;
    }
  }
  else
  {
    if (CTX_UNLIKELY(vt->in_prev_match))
    {
      if (vt->ref_len < 15)
      {
        vt->reference[vt->ref_len++]=byte;
        vt->reference[vt->ref_len]=0;
      }
    }
    else
    {
      vt_ctx_unrled (vt, byte);
    }
  }
#else
      vt_ctx_unrled (vt, byte);
#endif
}

static int vt_decoder_feed (VT *vt, int byte)
{
  int encoding = vt->encoding;
  switch (encoding)
    {
      case 0: /* utf8 */
        if (!vt->utf8_expected_bytes)
          {
            vt->utf8_expected_bytes = mrg_utf8_len (byte) - 1;
            vt->utf8_pos = 0;
          }
        if (vt->utf8_expected_bytes)
          {
            vt->utf8_holding[vt->utf8_pos++] = byte;
            vt->utf8_holding[vt->utf8_pos] = 0;
            if (vt->utf8_pos == vt->utf8_expected_bytes + 1)
              {
                vt->utf8_expected_bytes = 0;
                vt->utf8_pos = 0;
              }
            else
              {
                return 1;
              }
          }
        else
          {
            vt->utf8_holding[0] = byte;
            vt->utf8_holding[0] &= 127;
            vt->utf8_holding[1] = 0;
            if (vt->utf8_holding[0] == 0)
              { vt->utf8_holding[0] = 32; }
          }
        break;
      case 1:
        if ( ! (byte>=0 && byte < 256) )
          { byte = 255; }
        strcpy ( (char *) &vt->utf8_holding[0], &charmap_cp437[byte][0]);
        vt->utf8_expected_bytes = mrg_utf8_len (byte) - 1; // ?
        break;
      default:
        vt->utf8_holding[0] = byte & 127;
        vt->utf8_holding[1] = 0;
        break;
    }
  return 0;
}

static void vt_state_swallow (VT *vt, int byte)
{
  vt->state = vt_state_neutral;
}

static int vt_decode_hex_digit (char digit)
{
  if (digit >= '0' && digit <='9')
    return digit - '0';
  if (digit >= 'a' && digit <='f')
    return digit - 'a' + 10;
  if (digit >= 'A' && digit <='F')
    return digit - 'A' + 10;
  return 0;
}

static int vt_decode_hex (const char *two_digits)
{
  return vt_decode_hex_digit (two_digits[0]) * 16 +
         vt_decode_hex_digit (two_digits[1]);
}

static uint8_t palettes[][16][3]=
{
  {
{0, 0, 0},
{160, 41, 41},
{74, 160, 139},
{135, 132, 83},
{36, 36, 237},
{171, 74, 223},
{59, 107, 177},
{195, 195, 195},
{111, 111, 111},
{237, 172, 130},
{153, 237, 186},
{233, 216, 8},
{130, 180, 237},
{214, 111, 237},
{29, 225, 237},
{255, 255, 255},

  },

  {
    {0, 0, 0},
    {127, 0, 0},
    {90, 209, 88},
    {136, 109, 0},
    {3, 9, 235},
    {90, 4, 150},
    {43, 111, 150},
    {178, 178, 178},
    {87, 87, 87},
    {193, 122, 99},
    {110, 254, 174},
    {255, 200, 0},
    {10, 126, 254},
    {146, 155, 249},
    {184, 208, 254},
    {255, 255, 255},

  },{
    {0, 0, 0},
    {147, 53, 38},
    {30, 171, 82},
    {188, 153, 0},
    {32, 71, 193},
    {236, 49, 188},
    {42, 182, 253},
    {149, 149, 149},
    {73, 73, 73},
    {210, 36, 0},
    {96, 239, 97},
    {247, 240, 2},
    {93, 11, 249},
    {222, 42, 255},
    {11, 227, 255},
    {233, 235, 235},
  },


  { {0, 0, 0},{97, 27, 0},{129, 180, 0},{127, 100, 0},{44, 15, 255},{135, 10, 167},{20, 133, 164},{174, 174, 174},{71, 71, 71},{167, 114, 90},{162, 214, 127},{255, 251, 83},{118, 77, 253},{192, 121, 255},{14, 217, 255},{255, 255, 255},
  },{


#if 0
    {
      {0, 0, 0},
      {144, 0, 0},
      {9, 154, 9},
      {255, 137, 113},
      {3, 0, 255},
      {56, 0, 132},
      {0, 131, 131},
      {204, 204, 204},
      {127, 127, 127},
      {255, 33, 0},
      {113, 255, 88},
      {255, 236, 8},
      {1, 122, 255},
      {235, 0, 222},
      {0, 217, 255},
      {255, 255, 255},
    },{
#endif


    {0, 0, 0},
    {139, 0, 0},
    {9, 154, 9},
    {255, 137, 113},
    {3, 0, 255},
    {56, 0, 132},
    {0, 111, 111},
    {204, 204, 204},
    {127, 127, 127},
    {255, 33, 0},
    {118, 255, 92},
    {255, 230, 15},
    {1, 122, 255},
    {232, 0, 220},
    {1, 217, 255},
    {255, 255, 255},
  },
  {

    {0, 0, 0},
    {191, 0, 0},
    {3, 187, 0},
    {254, 212, 0},
    {0, 0, 255},
    {80, 0, 128},
    {0, 156, 255},
    {166, 166, 166},
    {84, 84, 84},
    {255, 62, 0},
    {85, 255, 143},
    {255, 255, 0},
    {67, 80, 255},
    {243, 70, 255},
    {30, 255, 222},
    {255, 255, 255},
  },
  {
    /* */
    { 32, 32, 32}, // 0 - background (black)
    {165, 15, 21}, // 1               red
    { 95,130, 10}, // 2               green
    {205,145, 60}, // 3               yellow
    { 49,130,189}, // 4               blue
    {120, 40,160}, // 5               magenta
    {120,230,230}, // 6               cyan
    {196,196,196},// 7                light-gray
    { 85, 85, 85},// 8                dark gray

    {251,106, 74},// 9                light red
    {130,215,140},// 10               light green
    {255,255,  0},// 11               light yellow
    {107,174,214},// 12               light blue
    {215,130,160},// 13               light magenta
    {225,255,245},// 14               light cyan
    {255,255,255},// 15 - foreground (white)
  },{
    /* */
    { 32, 32, 32}, // 0 - background (black)
    {160,  0,  0}, // 1               red
    {  9,233,  0}, // 2               green
    {220,110, 44}, // 3               yellow
    {  0,  0,200}, // 4               blue
    { 90,  0,130}, // 5               magenta
    {  0,156,180}, // 6               cyan
    {196,196,196}, // 7                light-gray
    { 85, 85, 85}, // 8                dark gray

    {240, 60, 40}, // 9                light red
    {170,240, 80}, // 10               light green
    {248,248,  0}, // 11               light yellow
    {  0, 40,255}, // 12               light blue
    {204, 62,214}, // 13               light magenta
    { 10,234,254}, // 14               light cyan
    {255,255,255}, // 15 - foreground (white)
  },
  /* inspired by DEC */
  { {  0,  0,  0}, // 0 - background  black
    {150, 10, 10}, // 1               red
    { 21,133,  0}, // 2               green

    {103,103, 24}, // 3               yellow
    { 44, 44,153}, // 4               blue
    {123, 94,183}, // 5               magenta

    { 20,183,193}, // 6               cyan

    {177,177,177},// 7                light-gray
    {100,100,100},// 8                dark gray

    {244, 39, 39},// 9                light red
    { 61,224, 81},// 10               light green
    {255,255,  0},// 11               light yellow
    { 61, 61,244},// 12               light blue
    {240, 11,240},// 13               light magenta
    { 61,234,234},// 14               light cyan

    {255,255,255},// 15 - foreground  white
  },
};

static void vt_state_osc (VT *vt, int byte)
{
  // https://ttssh2.osdn.jp/manual/4/en/about/ctrlseq.html
  // and in "\033\" rather than just "\033", this would cause
  // a stray char
  //if (byte == '\a' || byte == 27 || byte == 0 || byte < 32)
  if ( (byte < 32) && ( (byte < 8) || (byte > 13) ) )
    {
      int n = parse_int (vt->argument_buf, 0);
      switch (n)
        {
          case 0:
          case 1:
          case 2:
#if 0
    {"]0;New_title\e\",  0, , }, /* id: set window title */ "
#endif
            vt_set_title (vt, vt->argument_buf + 3);
            break;
          case 4: // set palette entry
            {
            int color_no = parse_int (vt->argument_buf + 2, 0);
            char *rest = vt->argument_buf + 3;
            rest = strchr (rest, ';');

            if (rest++)
            if (strlen(rest)>10 &&
                rest[0] == 'r' &&
                rest[1] == 'g' &&
                rest[2] == 'b' &&
                rest[3] == ':' &&
                rest[6] == '/' &&
                rest[9] == '/')
            {
              int red = vt_decode_hex (&rest[4]);
              int green = vt_decode_hex (&rest[7]);
              int blue = vt_decode_hex (&rest[10]);
          //  fprintf (stderr, "set color:%i  %i %i %i\n", color_no, red, green, blue);
              if (color_no >= 0 && color_no <= 15)
              {
                palettes[0][color_no][0]=red;
                palettes[0][color_no][1]=green;
                palettes[0][color_no][2]=blue;
              }
            }
            }
            break;
          case 12: // text cursor color
            break;
          case 17: // highlight color
            break;
          case 19: // ??
            break;

          case 10: // text fg
#if 0
#if 0
    {"]11;",  0, , }, /* id: set foreground color */
#endif
            {
              /* request current foreground color, xterm does this to
                 determine if it can use 256 colors, when this test fails,
                 it still mixes in color 130 together with stock colors
               */
              char buf[128];
              sprintf (buf, "\033]10;rgb:%2x/%2x/%2x\033\\",
                       vt->fg_color[0], vt->fg_color[1], vt->fg_color[2]);
              vt_write (vt, buf, strlen (buf) );
            }
#endif
            break;
          case 11: // text bg
#if 0
    {"]11;",  0, , }, /* id: get background color */
            {
              /* get background color */
              char buf[128];
              sprintf (buf, "\033]11;rgb:%2x/%2x/%2x\033\\",
                       vt->bg_color[0], vt->bg_color[1], vt->bg_color[2]);
              vt_write (vt, buf, strlen (buf) );
            }
#endif
            break;
#if 0
    {"]1337;key=value:base64data\b\",  0, vtcmd_erase_in_line, VT100}, /* args:keyvalue id: iterm2 graphics */ "
#endif
#ifdef STBI_INCLUDE_STB_IMAGE_H
          case 1337:
            if (!strncmp (&vt->argument_buf[6], "File=", 5) )
              {
                {
                  /* iTerm2 image protocol */
                  int width = 0;
                  int height = 0;
                  int file_size = 0;
                  int show_inline = 0;
                  int preserve_aspect = 1;
                  char *name = NULL;
                  char *p = &vt->argument_buf[11];
                  char key[128]="";
                  char value[128]="";
                  int in_key=1;
                  if (preserve_aspect) {}; /* XXX : NYI */
                  for (; *p && *p!=':'; p++)
                    {
                      if (in_key)
                        {
                          if (*p == '=')
                            { in_key = 0; }
                          else
                            {
                              if (strlen (key) < 124)
                                {
                                  key[strlen (key)+1] = 0;
                                  key[strlen (key)] = *p;
                                }
                            }
                        }
                      else
                        {
                          if (*p == ';')
                            {
                              if (!strcmp (key, "name") )
                                {
                                  name = strdup (value);
                                }
                              else if (!strcmp (key, "width") )
                                {
                                  width = atoi (value);
                                  if (strchr (value, 'x') )
                                    { /* pixels */ }
                                  else if (strchr (value, '%') )
                                    {
                                      /* percent */
                                      width = width / 100.0 * (vt->cw * vt->cols);
                                    }
                                  else
                                    { /* chars */ width = width * vt->cw; }
                                }
                              else if (!strcmp (key, "height") )
                                {
                                  height = atoi (value);
                                  if (strchr (value, 'x') )
                                    { /* pixels */ }
                                  else if (strchr (value, '%') )
                                    {
                                      /* percent */
                                      height = height / 100.0 * (vt->ch * vt->rows);
                                    }
                                  else
                                    { /* chars */ height = height * vt->ch; }
                                }
                              else if (!strcmp (key, "preserveAspectRatio") )
                                {
                                  preserve_aspect = atoi (value);
                                }
                              else if (!strcmp (key, "inline") )
                                {
                                  show_inline = atoi (value);
                                }
                              key[0]=0;
                              value[0]=0;
                              in_key = 1;
                            }
                          else
                            {
                              if (strlen (value) < 124)
                                {
                                  value[strlen (value)+1] = 0;
                                  value[strlen (value)] = *p;
                                }
                            }
                        }
                    }
                  if (key[0])
                    {
                      // code-dup
                      if (!strcmp (key, "name") )
                        {
                          name = strdup (value);
                        }
                      else if (!strcmp (key, "width") )
                        {
                          width = atoi (value);
                          if (strchr (value, 'x') )
                            { /* pixels */ }
                          else if (strchr (value, '%') )
                            {
                              /* percent */
                              width = width / 100.0 * (vt->cw * vt->cols);
                            }
                          else
                            { /* chars */ width = width * vt->cw; }
                        }
                      else if (!strcmp (key, "height") )
                        {
                          height = atoi (value);
                          if (strchr (value, 'x') )
                            { /* pixels */ }
                          else if (strchr (value, '%') )
                            {
                              /* percent */
                              height = height / 100.0 * (vt->ch * vt->rows);
                            }
                          else
                            { /* chars */ height = height * vt->ch; }
                        }
                      else if (!strcmp (key, "preserveAspectRatio") )
                        {
                          preserve_aspect = atoi (value);
                        }
                      else if (!strcmp (key, "inline") )
                        {
                          show_inline = atoi (value);
                        }
                    }
                  if (*p == ':')
                    {
                      p++;
                    }
                  if (0)
                    fprintf (stderr, "%s %i %i %i %i{%s\n", name?name:"",
                             width, height, file_size, show_inline,
                             p);
                  Image *image = NULL;
                  {
                    int bin_length = vt->argument_buf_len;
                    uint8_t *data2 = malloc (bin_length);
                    bin_length = ctx_base642bin ( (char *) p,
                                                 &bin_length,
                                                 data2);
                    int channels = 4;
                    int buf_width = 0;
                    int buf_height = 0;
                    uint8_t *new_data = stbi_load_from_memory (data2, bin_length, &buf_width, &buf_height, &channels, 4);
                    free (data2);
                    if (new_data)
                      {
                        image = image_add (buf_width, buf_height, 0,
                                           32, buf_width*buf_height*4, new_data);
                      }
                    else
                      {
                        fprintf (stderr, "image decoding problem %s\n", stbi_failure_reason());
                        fprintf (stderr, "len: %i\n", bin_length);
                      }
                  }
                  if (image)
                    {
                      display_image (vt, image, vt->cursor_x, 0,0, 0.0, 0.0, 0,0,0,0);
                      int right = (image->width + (vt->cw-1) ) /vt->cw;
                      int down = (image->height + (vt->ch-1) ) /vt->ch;
                      for (int i = 0; i<down - 1; i++)
                        { vtcmd_index (vt, " "); }
                      for (int i = 0; i<right; i++)
                        { vtcmd_cursor_forward (vt, " "); }
                    }
                }
              }
            break;
#endif
          case 104:
            break;
          case 8:
            fprintf (stderr, "unhandled OSC 8, hyperlink\n");
            break;
          default:
            fprintf (stderr, "unhandled OSC %i\n", n);
            break;
        }
      if (byte == 27)
        {
          vt->state = vt_state_swallow;
        }
      else
        {
          vt->state = vt_state_neutral;
        }
    }
  else
    {
      vt_argument_buf_add (vt, byte);
    }
}


static void vt_state_sixel (VT *vt, int byte)
{
  // https://ttssh2.osdn.jp/manual/4/en/about/ctrlseq.html
  // and in "\033\" rather than just "\033", this would cause
  // a stray char
  if ( (byte < 32) && ( (byte < 8) || (byte > 13) ) )
    {
      vt_sixels (vt, vt->argument_buf);
      if (byte == 27)
        {
          vt->state = vt_state_swallow;
        }
      else
        {
          vt->state = vt_state_neutral;
        }
    }
  else
    {
      vt_argument_buf_add (vt, byte);
      //fprintf (stderr, "\r%i ", vt->argument_buf_len);
    }
}

//void add_tab (Ctx *ctx, const char *commandline, int can_launch);
//void vt_screenshot (const char *output_path);

static void vt_state_apc_generic (VT *vt, int byte)
{
  if ( (byte < 32) && ( (byte < 8) || (byte > 13) ) )
    {
      if (vt->argument_buf[1] == 'G') /* graphics - from kitty */
        {
          vt_gfx (vt, vt->argument_buf);
        }
      else if (vt->argument_buf[1] == 'C') /* launch command */
      {
        if (vt->can_launch)
        {
          int   can_launch = 0;
          int   no_title = 0;
          int   no_move = 0;
          int   no_resize = 0;
          int   layer = 0;
  // escape subsequent arguments so that we dont have to pass a string?
          float x = -1.0;
          float y = -1.0;
          int   z = 0;
          float width = -1.0;
          float height = -1.0;

          for (int i=2; vt->argument_buf[i]; i++)
          {
            if (!strncmp (&vt->argument_buf[i], "can_launch=1", strlen ("can_launch=1")))
              can_launch = 1;
            if (!strncmp (&vt->argument_buf[i], "no_title=1", strlen("no_title=1")))
              no_title = 1;
            if (!strncmp (&vt->argument_buf[i], "no_move=1", strlen("no_move=1")))
              no_move = 1;
            else if (!strncmp (&vt->argument_buf[i], "z=", 2))
              z=atoi(&vt->argument_buf[i]+strlen("z="));
            else if (!strncmp (&vt->argument_buf[i], "x=", 2))
              x=atof(&vt->argument_buf[i]+strlen("x="));
            else if (!strncmp (&vt->argument_buf[i], "y=", 2))
              y=atof(&vt->argument_buf[i]+strlen("y="));
            else if (!strncmp (&vt->argument_buf[i], "width=", 6))
              width=atof(&vt->argument_buf[i]+strlen("width="));
            else if (!strncmp (&vt->argument_buf[i], "height=", 7))
              height=atof(&vt->argument_buf[i]+strlen("height="));
          }

          if (width + no_resize + layer + height + x + y + no_title + no_move + z + can_launch) {};

          char *sep = strchr(vt->argument_buf, ';');
          if (sep)
          {
            //fprintf (stderr, "[%s]", sep +  1);
            if (!strncmp (sep + 1, "fbsave", 6))
            {
              // vt_screenshot (sep + 8);
            }
            else
            {
          //  add_tab (ctx, sep + 1, can_launch);
            }
          }
        }

      }
      vt->state = ( (byte == 27) ?  vt_state_swallow : vt_state_neutral);
    }
  else
    {
      vt_argument_buf_add (vt, byte);
    }
}

#if 0
    {"_G..\e\", 0, vtcmd_delete_n_chars, VT102}, /* ref:none id: <a href='https://sw.kovidgoyal.net/kitty/graphics-protocol.html'>kitty graphics</a> */ "
    {"_A..\e\", 0, vtcmd_delete_n_chars, VT102}, /* id:  <a href='https://github.com/hodefoting/atty/'>atty</a> audio input/output */ "
    {"_C..\e\", 0, vtcmd_delete_n_chars, VT102}, /* id:  run command */ "
#endif
static void vt_state_apc (VT *vt, int byte)
{
  if (byte == 'A')
    {
      vt_argument_buf_add (vt, byte);
      vt->state = vt_state_apc_audio;
    }
  else if ( (byte < 32) && ( (byte < 8) || (byte > 13) ) )
    {
      vt->state = ( (byte == 27) ?  vt_state_swallow : vt_state_neutral);
    }
  else
    {
      vt_argument_buf_add (vt, byte);
      vt->state = vt_state_apc_generic;
    }
}

static void vt_state_esc_foo (VT *vt, int byte)
{
  vt_argument_buf_add (vt, byte);
  vt->state = vt_state_neutral;
  handle_sequence (vt, vt->argument_buf);
}

static void vt_state_esc_sequence (VT *vt, int byte)
{
  if (_vt_handle_control (vt, byte) == 0)
    {
      if (byte == 27)
        {
        }
      else if (byte >= '@' && byte <= '~')
        {
          vt_argument_buf_add (vt, byte);
          vt->state = vt_state_neutral;
          handle_sequence (vt, vt->argument_buf);
        }
      else
        {
          vt_argument_buf_add (vt, byte);
        }
    }
}

static void vt_state_esc (VT *vt, int byte)
{
  if (_vt_handle_control (vt, byte) == 0)
    switch (byte)
      {
        case 27: /* ESCape */
          break;
        case ')':
        case '#':
        case '(':
          {
            char tmp[]= {byte, '\0'};
            vt_argument_buf_reset (vt, tmp);
            vt->state = vt_state_esc_foo;
          }
          break;
        case '[':
        case '%':
        case '+':
        case '*':
          {
            char tmp[]= {byte, '\0'};
            vt_argument_buf_reset (vt, tmp);
            vt->state = vt_state_esc_sequence;
          }
          break;

#if 0
    {"Psixel_data\e\",  0, , }, /* id: sixels */ "
#endif

        case 'P':
          {
            char tmp[]= {byte, '\0'};
            vt_argument_buf_reset (vt, tmp);
            vt->state = vt_state_sixel;
          }
          break;
        case ']':
          {
            char tmp[]= {byte, '\0'};
            vt_argument_buf_reset (vt, tmp);
            vt->state = vt_state_osc;
          }
          break;
        case '^':  // privacy message
        case '_':  // APC
        case 'X':  // SOS
          {
            char tmp[]= {byte, '\0'};
            vt_argument_buf_reset (vt, tmp);
            vt->state = vt_state_apc;
          }
          break;
        default:
          {
            char tmp[]= {byte, '\0'};
            tmp[0]=byte;
            vt->state = vt_state_neutral;
            handle_sequence (vt, tmp);
          }
          break;
      }
}

static void vt_state_neutral (VT *vt, int byte)
{
  if (CTX_UNLIKELY(_vt_handle_control (vt, byte) != 0))
    return;
  if (CTX_LIKELY(byte != 27))
  {
    if (vt_decoder_feed (vt, byte) )
      return;
    if (vt->charset[vt->shifted_in] != 0 &&
        vt->charset[vt->shifted_in] != 'B')
      {
        char **charmap;
        switch (vt->charset[vt->shifted_in])
          {
            case 'A':
              charmap = charmap_uk;
              break;
            case 'B':
              charmap = charmap_ascii;
              break;
            case '0':
              charmap = charmap_graphics;
              break;
            case '1':
              charmap = charmap_cp437;
              break;
            case '2':
              charmap = charmap_graphics;
              break;
            default:
              charmap = charmap_ascii;
              break;
          }
        if ( (vt->utf8_holding[0] >= ' ') && (vt->utf8_holding[0] <= '~') )
          {
            _vt_add_str (vt, charmap[vt->utf8_holding[0]-' ']);
          }
      }
    else
      {
        // ensure vt->utf8_holding contains a valid utf8
        uint32_t codepoint;
        uint32_t state = 0;
        for (int i = 0; vt->utf8_holding[i]; i++)
          { utf8_decode (&state, &codepoint, vt->utf8_holding[i]); }
        if (state != UTF8_ACCEPT)
          {
            /* otherwise mangle it so that it does */
            vt->utf8_holding[0] &= 127;
            vt->utf8_holding[1] = 0;
            if (vt->utf8_holding[0] == 0)
              { vt->utf8_holding[0] = 32; }
          }
        _vt_add_str (vt, (char *) vt->utf8_holding);
      }
  }
  else // ESCape
  {
    vt->state = vt_state_esc;
  }
}

int vt_poll (VT *vt, int timeout)
{
  if (!vt) return 0;
  int read_size = sizeof (vt->buf);
  int got_data = 0;

  // read_size 1m1.142s
  // read_size*10  52s
  // read_size*5   53.8s
  // read_size*4   53.78s
  // read_size*3   .....s
  // read_size*2   56.99s
  int remaining_chars = read_size * 3;// * 100;
  int len = 0;
  vt_audio_task (vt, 0);
#if 1
  if (vt->cursor_visible && vt->smooth_scroll)
    {
      remaining_chars = vt->cols / 2;
    }
#endif
  read_size = MIN (read_size, remaining_chars);
  long start_ticks = ctx_ticks ();
  long ticks = start_ticks;
  while (remaining_chars > 0 &&
         vt_waitdata (vt, 0) &&
         ( ticks - start_ticks < timeout ||  vt->state == vt_state_ctx))
    {
  if (vt->in_smooth_scroll)
    {
      remaining_chars = 1;
      // XXX : need a bail condition -
      // /// so that we can stop accepting data until autowrap or similar
    }
      len = vt_read (vt, vt->buf, read_size);
      if (len >0)
      {
     // fwrite (vt->buf, len, 1, vt->log);
     // fwrite (vt->buf, len, 1, stdout);
      }
      for (int i = 0; i < len; i++)
        { vt->state (vt, vt->buf[i]); }
      // XXX allow state to break out in ctx mode on flush
      got_data+=len;
      remaining_chars -= len;
      if (vt->state == vt_state_ctx) {
         if (remaining_chars < read_size)
         {
           remaining_chars = read_size * 2;
         }
      }
      vt_audio_task (vt, 0);
      ticks = ctx_ticks ();
    }
  if (got_data < 0)
    {
      if (kill (vt->vtpty.pid, 0) != 0)
        {
          vt->vtpty.done = 1;
        }
    }
  return got_data;
}

/******/

static const char *keymap_vt52[][2]=
{
  {"up",    "\033A" },
  {"down",  "\033B" },
  {"right", "\033C" },
  {"left",  "\033D" },
};

static const char *keymap_application[][2]=
{
  {"up",    "\033OA" },
  {"down",  "\033OB" },
  {"right", "\033OC" },
  {"left",  "\033OD" },
};

static const char *keymap_general[][2]=
{
  {"up",             "\033[A"},
  {"down",           "\033[B"},
  {"right",          "\033[C"},
  {"left",           "\033[D"},
  {"end",            "\033[F"},
  {"home",           "\033[H"},
  {"shift-up",       "\033[1;2A"},
  {"shift-down",     "\033[1;2B"},
  {"shift-right",    "\033[1;2C"},
  {"shift-left",     "\033[1;2D"},
  {"alt-a",          "\033a"},
  {"alt-b",          "\033b"},
  {"alt-c",          "\033c"},
  {"alt-d",          "\033d"},
  {"alt-e",          "\033e"},
  {"alt-f",          "\033f"},
  {"alt-g",          "\033g"},
  {"alt-h",          "\033h"},
  {"alt-i",          "\033i"},
  {"alt-j",          "\033j"},
  {"alt-k",          "\033k"},
  {"alt-l",          "\033l"},
  {"alt-m",          "\033m"},
  {"alt-n",          "\033n"},
  {"alt-o",          "\033o"},
  {"alt-p",          "\033p"},
  {"alt-q",          "\033q"},
  {"alt-r",          "\033r"},
  {"alt-s",          "\033s"},
  {"alt-t",          "\033t"},
  {"alt-u",          "\033u"},
  {"alt-v",          "\033v"},
  {"alt-w",          "\033w"},
  {"alt-x",          "\033x"},
  {"alt-y",          "\033y"},
  {"alt-z",          "\033z"},
  {"alt- ",          "\033 "},
  {"alt-space",      "\033 "},
  {"alt-0",          "\0330"},
  {"alt-1",          "\0331"},
  {"alt-2",          "\0332"},
  {"alt-3",          "\0333"},
  {"alt-4",          "\0334"},
  {"alt-5",          "\0335"},
  {"alt-6",          "\0336"},
  {"alt-7",          "\0337"},
  {"alt-8",          "\0338"},
  {"alt-9",          "\0339"},
  {"alt-return",     "\033\r"},
  {"alt-backspace",  "\033\177"},
  {"alt-up",         "\033[1;3A"},
  {"alt-down",       "\033[1;3B"},
  {"alt-right",      "\033[1;3C"},
  {"alt-left",       "\033[1;3D"},
  {"shift-alt-up",   "\033[1;4A"},
  {"shift-alt-down", "\033[1;4B"},
  {"shift-alt-right","\033[1;4C"},
  {"shift-alt-left", "\033[1;4D"},
  {"control-space",  "\000"},
  {"control-up",     "\033[1;5A"},
  {"control-down",   "\033[1;5B"},
  {"control-right",  "\033[1;5C"},
  {"control-left",   "\033[1;5D"},
  {"shift-control-up",    "\033[1;6A"},
  {"shift-control-down",  "\033[1;6B"},
  {"shift-control-right", "\033[1;6C"},
  {"shift-control-left",  "\033[1;6D"},
  {"insert",         "\033[2~"},
  {"delete",         "\033[3~"},
  {"control-delete", "\033[3,5~"},
  {"shift-delete",   "\033[3,2~"},
  {"control-shift-delete",  "\033[3,6~"},
  {"page-up",        "\033[5~"},
  {"page-down",     "\033[6~"},
  {"return",         "\r"},
  {"shift-tab",      "\033Z"},
  {"shift-return",   "\r"},
  {"control-return", "\r"},
  {"space",          " "},
  {"shift-space",    " "},
  {"control-a",      "\001"},
  {"control-b",      "\002"},
  {"control-c",      "\003"},
  {"control-d",      "\004"},
  {"control-e",      "\005"},
  {"control-f",      "\006"},
  {"control-g",      "\007"},
  {"control-h",      "\010"},
  {"control-i",      "\011"},
  {"control-j",      "\012"},
  {"control-k",      "\013"},
  {"control-l",      "\014"},
  {"control-m",      "\015"},
  {"control-n",      "\016"},
  {"control-o",      "\017"},
  {"control-p",      "\020"},
  {"control-q",      "\021"},
  {"control-r",      "\022"},
  {"control-s",      "\023"},
  {"control-t",      "\024"},
  {"control-u",      "\025"},
  {"control-v",      "\026"},
  {"control-w",      "\027"},
  {"control-x",      "\030"},
  {"control-y",      "\031"},
  {"control-z",      "\032"},
  {"escape",         "\033"},
  {"tab",            "\t"},
  {"backspace",      "\177"},
  {"control-backspace", "\177"},
  {"shift-backspace","\177"},
  {"shift-tab",      "\033[Z"},

  {"control-F1",     "\033[>11~"},
  {"control-F2",     "\033[>12~"},
  {"control-F3",     "\033[>13~"},
  {"control-F4",     "\033[>14~"},
  {"control-F5",     "\033[>15~"},

  {"shift-F1",       "\033[?11~"},
  {"shift-F2",       "\033[?12~"},
  {"shift-F3",       "\033[?13~"},
  {"shift-F4",       "\033[?14~"},
  {"shift-F5",       "\033[?15~"},

  {"F1",             "\033[11~"},  // hold screen   // ESC O P
  {"F2",             "\033[12~"},  // print screen  //       Q
  {"F3",             "\033[13~"},  // set-up                 R
  {"F4",             "\033[14~"},  // data/talk              S
  {"F5",             "\033[15~"},  // break
  {"F6",             "\033[17~"},
  {"F7",             "\033[18~"},
  {"F8",             "\033[19~"},
  {"F9",             "\033[20~"},
  {"F10",            "\033[21~"},
  {"F11",            "\033[22~"},
  {"F12",            "\033[23~"},
  {"control-/",       "\037"},
  {"shift-control-/", "\037"},
  {"control-[",       "\033"},
  {"control-]",       "\035"},
  {"shift-control-[", "\033"},
  {"shift-control-]", "\031"},
  {"shift-control-`", "\036"},
  {"control-'",       "'"},
  {"shift-control-'", "'"},
  {"control-;",       ";"},
  {"shift-control-;", ";"},
  {"control-.",       "."},
  {"shift-control-.", "."},
  {"control-,",       ","},
  {"shift-control-,", ","},
  {"control-\\",      "\034"},
  {"control-1",       "1"},
  {"control-3",       "\033"},
  {"control-4",       "\034"},
  {"control-5",       "\035"},
  {"control-6",       "\036"},
  {"shift-control-6", "\036"},
  {"control-7",       "\037"},
  {"shift-control-7", "\036"},
  {"control-8",       "\177"},
  {"control-9",       "9"},


};

void ctx_client_lock (CtxClient *client);
void ctx_client_unlock (CtxClient *client);

void vt_feed_keystring (VT *vt, CtxEvent *event, const char *str)
{
  if (vt->ctx_events)
  {
    if (!strcmp (str, "control-l") )
    {
      vt->ctx_events = 0;
      return;
    }
    vt_write (vt, str, strlen (str) );
    vt_write (vt, "\n", 1);
    return;
  }
  if (!strncmp (str, "keyup",   5)) return;
  if (!strncmp (str, "keydown", 7)) return;

  if (!strcmp (str, "capslock")) return;

#if 0
  if (!strstr (str, "-page"))
    vt_set_scroll (vt, 0);
#endif

  if (!strcmp (str, "idle") )
     return;
  else if (!strcmp (str, "shift-control-home"))
    {
      vt_set_scroll (vt, vt->scrollback_count);
      ctx_client_rev_inc (vt->client);
      return;
    }
  else if (!strcmp (str, "shift-control-end"))
    {
      int new_scroll = 0;
      vt_set_scroll (vt, new_scroll);
      ctx_client_rev_inc (vt->client);
      return;
    }
  else if (!strcmp (str, "shift-control-down"))
    {
      int new_scroll = vt_get_scroll (vt) - 1;
      vt_set_scroll (vt, new_scroll);
      ctx_client_rev_inc (vt->client);
      return;
    }
  else if (!strcmp (str, "shift-control-up"))
    {
      int new_scroll = vt_get_scroll (vt) + 1;
      vt_set_scroll (vt, new_scroll);
      ctx_client_rev_inc (vt->client);
      return;
    }
  else if (!strcmp (str, "shift-page-up") ||
           !strcmp (str, "shift-control-page-up"))
    {
      int new_scroll = vt_get_scroll (vt) + vt_get_rows (vt) /2;
      vt_set_scroll (vt, new_scroll);
      ctx_client_rev_inc (vt->client);
      return;
    }
  else if (!strcmp (str, "shift-page-down") ||
           !strcmp (str, "shift-control-page-down"))
    {
      int new_scroll = vt_get_scroll (vt) - vt_get_rows (vt) /2;
      if (new_scroll < 0) { new_scroll = 0; }
      vt_set_scroll (vt, new_scroll);
      ctx_client_rev_inc (vt->client);
      return;
    }
  else if (!strcmp (str, "shift-control--") ||
           !strcmp (str, "control--") )
    {
      float font_size = vt_get_font_size (vt);
      //font_size /= 1.15;
      font_size -=2;//= roundf (font_size);
      if (font_size < 2) { font_size = 2; }
      vt_set_font_size (vt, font_size);
      vt_set_px_size (vt, vt->width, vt->height);
      return;
    }
  else if (!strcmp (str, "shift-control-=") ||
           !strcmp (str, "control-=") )
    {
      float font_size = vt_get_font_size (vt);
      float old = font_size;
      //font_size *= 1.15;
      //
      //font_size = roundf (font_size);
      font_size+=2;

      if (old == font_size) { font_size = old+1; }
      if (font_size > 200) { font_size = 200; }
      vt_set_font_size (vt, font_size);
      vt_set_px_size (vt, vt->width, vt->height);

      return;
    }
  else if (!strcmp (str, "shift-control-r") )
    {
      vt_open_log (vt, "/tmp/ctx-vt");
      return;
    }
  else if (!strcmp (str, "shift-control-l") )
    {
      vt_set_local (vt, !vt_get_local (vt) );
      return;
    }
  else if (str[0]=='p' && str[1] != 0 && str[2] == ' ')
    {
      int cw = vt_cw (vt);
      int ch = vt_ch (vt);
      if (!strncmp (str, "pm", 2))
        {
          int x = 0, y = 0;
          char *s = strchr (str, ' ');
          if (s)
            {
              x = atoi (s);
              s = strchr (s + 1, ' ');
              if (s)
                {
                  y = atoi (s);
                  vt_mouse (vt, event, VT_MOUSE_MOTION, 1, x/cw + 1, y/ch + 1, x, y);
                }
            }
        }
      else if (!strncmp (str, "pp", 2))
        {
          int x = 0, y = 0, b = 0;
          char *s = strchr (str, ' ');
          if (s)
            {
              x = atoi (s);
              s = strchr (s + 1, ' ');
              if (s)
                {
                  y = atoi (s);
                  s = strchr (s + 1, ' ');
                  if (s)
                  {
                    b = atoi (s);
                  }
                  vt_mouse (vt, event, VT_MOUSE_PRESS, b, x/cw + 1, y/ch + 1, x, y);
                }
            }
          //clients[active].drawn_rev = 0;
        }
      else if (!strncmp (str, "pd", 2))
        {
          int x = 0, y = 0, b = 0; // XXX initialize B
          char *s = strchr (str, ' ');
          if (s)
            {
              x = atoi (s);
              s = strchr (s + 1, ' ');
              if (s)
                {
                  y = atoi (s);
                  if (s)
                  {
                    b = atoi (s);
                  }
                  vt_mouse (vt, event, VT_MOUSE_DRAG, b, x/cw + 1, y/ch + 1, x, y);
                }
            }
          //clients[active].drawn_rev = 0;
        }
      else if (!strncmp (str, "pr", 2))
        {
          int x = 0, y = 0, b = 0;
          char *s = strchr (str, ' ');
          if (s)
            {
              x = atoi (s);
              s = strchr (s + 1, ' ');
              if (s)
                {
                  y = atoi (s);
                  s = strchr (s + 1, ' ');
                  if (s)
                  {
                    b = atoi (s);
                  }
                  vt_mouse (vt, event, VT_MOUSE_RELEASE, b, x/cw + 1, y/ch + 1, x, y);
                }
            }
          //clients[active].drawn_rev = 0;
          // queue-draw
        }
      return;
    }

  if (vt->scroll_on_input)
  {
    vt->scroll = 0.0;
  }


  if (vt->state == vt_state_vt52)
    {
      for (unsigned int i = 0; i<sizeof (keymap_vt52) /sizeof (keymap_vt52[0]); i++)
        if (!strcmp (str, keymap_vt52[i][0]) )
          { str = keymap_vt52[i][1]; goto done; }
    }
  else
    {
      if (vt->cursor_key_application)
        {
          for (unsigned int i = 0; i<sizeof (keymap_application) /sizeof (keymap_application[0]); i++)
            if (!strcmp (str, keymap_application[i][0]) )
              { str = keymap_application[i][1]; goto done; }
        }
    }


  if (!strcmp (str, "return") )
    {
      if (vt->cr_on_lf)
        { str = "\r\n"; }
      else
        { str = "\r"; }
      goto done;
    }
  if (!strcmp (str, "control-space") ||
      !strcmp (str, "control-`") ||
      !strcmp (str, "control-2") ||
      !strcmp (str, "shift-control-2") ||
      !strcmp (str, "shift-control-space") )
    {
      str = "\0\0";
      vt_write (vt, str, 1);
      return;
    }
  for (unsigned int i = 0; i< sizeof (keymap_general) /
                              sizeof (keymap_general[0]); i++)
    if (!strcmp (str, keymap_general[i][0]) )
      {
        str = keymap_general[i][1];
        break;
      }
done:
  if (strlen (str) )
    {
      if (vt->local_editing)
        {
          for (int i = 0; str[i]; i++)
            {
              vt->state (vt, str[i]);
            }
        }
      else
        {
          vt_write (vt, str, strlen (str) );
        }
    }
}


void vt_paste (VT *vt, const char *str)
{
  if (vt->bracket_paste)
    {
      vt_write (vt, "\033[200~", 6);
    }
  vt_feed_keystring (vt, NULL, str);
  if (vt->bracket_paste)
    {
      vt_write (vt, "\033[201~", 6);
    }
}

const char *ctx_find_shell_command (void)
{
#ifdef EMSCRIPTEN
  return NULL;  
#else
  if (access ("/.flatpak-info", F_OK) != -1)
  {
    static char ret[512];
    char buf[256];
    FILE *fp = popen("flatpak-spawn --host getent passwd $USER|cut -f 7 -d :", "r");
    if (fp)
    {
      while (fgets (buf, sizeof(buf), fp) != NULL)
      {
        if (buf[strlen(buf)-1]=='\n')
          buf[strlen(buf)-1]=0;
        sprintf (ret, "flatpak-spawn --env=TERM=xterm --host %s", buf);
      }
      pclose (fp);
      return ret;
    }
  }

  if (getenv ("SHELL"))
  {
    return getenv ("SHELL");
  }
  int i;
  const char *command = NULL;
  struct stat stat_buf;
  static char *alts[][2] =
  {
    {"/bin/bash",     "/bin/bash"},
    {"/usr/bin/bash", "/usr/bin/bash"},
    {"/bin/sh",       "/bin/sh"},
    {"/usr/bin/sh",   "/usr/bin/sh"},
    {NULL, NULL}
  };
  for (i = 0; alts[i][0] && !command; i++)
    {
      lstat (alts[i][0], &stat_buf);
      if (S_ISREG (stat_buf.st_mode) || S_ISLNK (stat_buf.st_mode) )
        { command = alts[i][1]; }
    }
  return command;
#endif
}




static void vt_run_command (VT *vt, const char *command, const char *term)
{
#ifdef EMSCRIPTEN
        printf ("run command %s\n", command);
#else
  struct winsize ws;
  //signal (SIGCHLD,signal_child);
#if 0
  int was_pidone = (getpid () == 1);
#else
  int was_pidone = 0; // do no special treatment, all child processes belong
                      // to root
#endif
  signal (SIGINT,SIG_DFL);
  ws.ws_row = vt->rows;
  ws.ws_col = vt->cols;
  ws.ws_xpixel = ws.ws_col * vt->cw;
  ws.ws_ypixel = ws.ws_row * vt->ch;
  vt->vtpty.pid = vt_forkpty (&vt->vtpty.pty, NULL, NULL, &ws);
  if (vt->vtpty.pid == 0)
    {
      ctx_child_prepare_env (was_pidone, term);
      exit (0);
    }
  else if (vt->vtpty.pid < 0)
    {
      VT_error ("forkpty failed (%s)", command);
      return;
    }
  fcntl(vt->vtpty.pty, F_SETFL, O_NONBLOCK|O_NOCTTY);
  _ctx_add_listen_fd (vt->vtpty.pty);
#endif
}


void vt_destroy (VT *vt)
{
  while (vt->lines)
    {
      vt_line_free (vt->lines->data, 1);
      ctx_list_remove (&vt->lines, vt->lines->data);
      vt->line_count--;
    }
  while (vt->scrollback)
    {
      vt_line_free (vt->scrollback->data, 1);
      ctx_list_remove (&vt->scrollback, vt->scrollback->data);
    }
  if (vt->ctxp)
    ctx_parser_free (vt->ctxp);
  //if (vt->ctx)
  //  { ctx_free (vt->ctx); }
  free (vt->argument_buf);
  ctx_list_remove (&ctx_vts, vt);
  kill (vt->vtpty.pid, 9);
  _ctx_remove_listen_fd (vt->vtpty.pty);
  close (vt->vtpty.pty);
#if 1
  if (vt->title)
    free (vt->title);
#endif
  free (vt);
}

int vt_get_line_count (VT *vt)
{
  int max_pop = 0;
  int no = 0;
  for (CtxList *l = vt->lines; l; l = l->next, no++)
  {
    CtxString *str = l->data;
    if (str->str[0]) max_pop = no;
  }
  return max_pop + 1;
}

const char *vt_get_line (VT *vt, int no)
{
  if (no >= vt->rows)
  {
    CtxList *l = ctx_list_nth (vt->scrollback, no - vt->rows);
    if (!l)
      { 
         return "";
      }
    CtxString *str = l->data;
    return str->str;
  }
  else
  {
    CtxList *l = ctx_list_nth (vt->lines, no);
    if (!l)
      { 
         return "-";
      }
    CtxString *str = l->data;
    return str->str;
  }
}

int vt_line_is_continuation (VT *vt, int no)
{
  if (no >= vt->rows)
  {
    CtxList *l = ctx_list_nth (vt->scrollback, no - vt->rows);
    if (!l)
      { 
         return 1;
      }
    VtLine *line = l->data;
    return line->wrapped;
  }
  else
  {
    CtxList *l = ctx_list_nth (vt->lines, no);
    if (!l)
      { 
         return 1;
      }
    VtLine *line = l->data;
    return line->wrapped;
  }
}

int vt_get_cols (VT *vt)
{
  return vt->cols;
}

int vt_get_rows (VT *vt)
{
  return vt->rows;
}

int vt_get_cursor_x (VT *vt)
{
  return vt->cursor_x;
}

int vt_get_cursor_y (VT *vt)
{
  return vt->cursor_y;
}

static void draw_braille_bit (Ctx *ctx, float x, float y, float cw, float ch, int u, int v)
{
  ctx_rectangle (ctx, 0.167 * cw + x + u * cw * 0.5,
                 y - ch + 0.080 * ch + v * ch * 0.25,
                 0.33 *cw, 0.33 * cw);
}

static void draw_sextant_bit (Ctx *ctx, float x, float y, float cw, float ch, int u, int v)
{
  ctx_rectangle (ctx,  x + u * cw * 0.5,
                       y - ch + v * ch * 0.3333,
                       0.5 *cw, 0.34 * ch);
}

int vt_special_glyph (Ctx *ctx, VT *vt, float x, float y, int cw, int ch, int unichar)
{
  switch (unichar)
    {
      case 0x2594: // UPPER_ONE_EIGHT_BLOCK
        ctx_begin_path (ctx);
        {
          float factor = 1.0f/8.0f;
          ctx_rectangle (ctx, x, y - ch, cw, ch * factor);
          ctx_fill (ctx);
        }
        return 0;
      case 0x2581: // LOWER_ONE_EIGHT_BLOCK:
        ctx_begin_path (ctx);
        {
          float factor = 1.0f/8.0f;
          ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
          ctx_fill (ctx);
        }
        return 0;
      case 0x2582: // LOWER_ONE_QUARTER_BLOCK:
        ctx_begin_path (ctx);
        {
          float factor = 1.0f/4.0f;
          ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
          ctx_fill (ctx);
        }
        return 0;
      case 0x2583: // LOWER_THREE_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        {
          float factor = 3.0f/8.0f;
          ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
          ctx_fill (ctx);
        }
        return 0;
      case 0x2585: // LOWER_FIVE_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        {
          float factor = 5.0f/8.0f;
          ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
          ctx_fill (ctx);
        }
        return 0;
      case 0x2586: // LOWER_THREE_QUARTERS_BLOCK:
        ctx_begin_path (ctx);
        {
          float factor = 3.0f/4.0f;
          ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
          ctx_fill (ctx);
        }
        return 0;
      case 0x2587: // LOWER_SEVEN_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        {
          float factor = 7.0f/8.0f;
          ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
          ctx_fill (ctx);
        }
        return 0;
      case 0x2589: // LEFT_SEVEN_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw*7/8, ch);
        ctx_fill (ctx);
        return 0;
      case 0x258A: // LEFT_THREE_QUARTERS_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw*3/4, ch);
        ctx_fill (ctx);
        return 0;
      case 0x258B: // LEFT_FIVE_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw*5/8, ch);
        ctx_fill (ctx);
        return 0;
      case 0x258D: // LEFT_THREE_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw*3/8, ch);
        ctx_fill (ctx);
        return 0;
      case 0x258E: // LEFT_ONE_QUARTER_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw/4, ch);
        ctx_fill (ctx);
        return 0;
      case 0x258F: // LEFT_ONE_EIGHT_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw/8, ch);
        ctx_fill (ctx);
        return 0;
      case 0x258C: // HALF_LEFT_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw/2, ch);
        ctx_fill (ctx);
        return 0;
      case 0x2590: // HALF_RIGHT_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2, y - ch, cw/2, ch);
        ctx_fill (ctx);
        return 0;
      case 0x1fb8f: // VT_RIGHT_SEVEN_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw*1/8, y - ch, cw*7/8, ch);
        ctx_fill (ctx);
        return 0;
      case 0x1fb8d: // VT_RIGHT_FIVE_EIGHTS_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw*3/8, y - ch, cw*5/8, ch);
        ctx_fill (ctx);
        return 0;
      case 0x1fb8b: // VT_RIGHT_ONE_QUARTER_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw*3/4, y - ch, cw/4, ch);
        ctx_fill (ctx);
        return 0;
      case 0x1fb8e: // VT_RIGHT_THREE_QUARTER_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw*1/4, y - ch, cw*3/4, ch);
        ctx_fill (ctx);
        return 0;
      case 0x2595: // VT_RIGHT_ONE_EIGHT_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw*7/8, y - ch, cw/8, ch);
        ctx_fill (ctx);
        return 0;
      case 0x2580: // HALF_UP_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x2584: // _HALF_DOWN_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch/2, cw, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x2596: // _QUADRANT LOWER LEFT
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch/2, cw/2, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x2597: // _QUADRANT LOWER RIGHT
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x+cw/2, y - ch/2, cw/2, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x2598: // _QUADRANT UPPER LEFT
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw/2, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x259D: // _QUADRANT UPPER RIGHT
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2, y - ch, cw/2, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x2599: // _QUADRANT UPPER LEFT AND LOWER LEFT AND LOWER RIGHT
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2598);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2596);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2597);
        return 0;
      case 0x259A: // _QUADRANT UPPER LEFT AND LOWER RIGHT
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2598);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2597);
        return 0;
      case 0x259B: // _QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER LEFT
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2598);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x259D);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2596);
        return 0;
      case 0x259C: // _QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2598);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x259D);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2597);
        return 0;
      case 0x259E: // _QUADRANT UPPER RIGHT AND LOWER LEFT
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x259D);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2596);
        return 0;
      case 0x259F: // _QUADRANT UPPER RIGHT AND LOWER LEFT AND LOWER RIGHT
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x259D);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2596);
        vt_special_glyph (ctx, vt, x, y, cw, ch, 0x2597);
        return 0;
      case 0x2588: // FULL_BLOCK:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw, ch);
        ctx_fill (ctx);
        return 0;
      case 0x2591: // LIGHT_SHADE:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw, ch);
        ctx_save (ctx);
        ctx_global_alpha (ctx, 0.25);
        ctx_fill (ctx);
        ctx_restore (ctx);
        return 0;
      case 0x2592: // MEDIUM_SHADE:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw, ch);
        ctx_save (ctx);
        ctx_global_alpha (ctx, 0.5);
        ctx_fill (ctx);
        ctx_restore (ctx);
        return 0;
      case 0x2593: // DARK SHADE:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch, cw, ch);
        ctx_save (ctx);
        ctx_global_alpha (ctx, 0.75);
        ctx_fill (ctx);
        ctx_restore (ctx);
        return 0;
      case 0x23BA: //HORIZONTAL_SCANLINE-1
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x,      y - ch + ch*0.1 - ch * 0.1,
                       cw, ch * 0.1);
        ctx_fill (ctx);
        return 0;
      case 0x23BB: //HORIZONTAL_SCANLINE-3
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x,      y - ch + ch*0.3 - ch * 0.075,
                       cw, ch * 0.1);
        ctx_fill (ctx);
        return 0;
      case 0x23BC: //HORIZONTAL_SCANLINE-7
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x,      y - ch + ch*0.7 - ch * 0.025,
                       cw, ch * 0.1);
        ctx_fill (ctx);
        return 0;
      case 0x23BD: //HORIZONTAL_SCANLINE-9
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x,      y - ch + ch*0.9 + ch * 0.0,
                       cw, ch * 0.1);
        ctx_fill (ctx);
        return 0;
      case 0x2500: //VT_BOX_DRAWINGS_LIGHT_HORIZONTAL // and scanline 5
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
        ctx_fill (ctx);
        return 0;
      case 0x2212: // minus -sign
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw * 0.1, y - ch/2 - ch * 0.1 / 2, cw * 0.8, ch * 0.1);
        ctx_fill (ctx);
        return 0;
      case 0x2502: // VT_BOX_DRAWINGS_LIGHT_VERTICAL:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch + 1);
        ctx_fill (ctx);
        return 0;
      case 0x250c: //VT_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, ch * 0.1, ch/2 + ch*0.1);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, cw/2+ ch * 0.1, ch*0.1);
        ctx_fill (ctx);
        return 0;
      case 0x2510: //VT_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, ch * 0.1, ch/2 + ch*0.1);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x, y - ch/2 - ch*0.1/2, cw/2+ ch * 0.1/2, ch*0.1);
        ctx_fill (ctx);
        return 0;
      case 0x2514: //VT_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch/2+ch*0.1/2);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, cw/2 + ch * 0.1, ch*0.1);
        ctx_fill (ctx);
        return 0;
      case 0x2518: //VT_BOX_DRAWINGS_LIGHT_UP_AND_LEFT:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch/2+ ch*0.1/2);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x, y - ch/2-ch*0.1/2, cw/2+ch * 0.1/2, ch*0.1);
        ctx_fill (ctx);
        return 0;
      case 0x251C: //VT_BOX_DRAWINGS_LIGHT_VERTICAL_AND_RIGHT:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2-ch*0.1/2, cw/2+ch * 0.1, ch*0.1);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
        ctx_fill (ctx);
        return 0;
      case 0x2524: //VT_BOX_DRAWINGS_LIGHT_VERTICAL_AND_LEFT:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x, y - ch/2-ch*0.1/2, cw/2+ch * 0.1/2, ch*0.1);
        ctx_fill (ctx);
        return 0;
      case 0x252C: // VT_BOX_DRAWINGS_LIGHT_DOWN_AND_HORIZONTAL:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2-ch*0.1/2, ch * 0.1, ch/2+ch*0.1);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
        ctx_fill (ctx);
        return 0;
      case 0x2534: // VT_BOX_DRAWINGS_LIGHT_UP_AND_HORIZONTAL:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch /2+ch*0.1/2);
        ctx_fill (ctx);
        return 0;
      case 0x253C: // VT_BOX_DRAWINGS_LIGHT_VERTICAL_AND_HORIZONTAL:
        ctx_begin_path (ctx);
        ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
        ctx_fill (ctx);
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
        ctx_fill (ctx);
        return 0;
      case 0xe0a0: // PowerLine branch
        ctx_save (ctx);
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x+cw/2, y - 0.15 * ch);
        ctx_rel_line_to (ctx, -cw/3, -ch * 0.7);
        ctx_rel_line_to (ctx, cw/2, 0);
        ctx_rel_line_to (ctx, -cw/3, ch * 0.7);
        ctx_line_width (ctx, cw * 0.25);
        ctx_stroke (ctx);
        ctx_restore (ctx);
        break;
      // case 0xe0a1: // PowerLine LN
      // case 0xe0a2: // PowerLine Lock
      case 0xe0b0: // PowerLine left solid
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_line_to (ctx, 0, -ch);
        ctx_rel_line_to (ctx, cw, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0xe0b1: // PowerLine left line
        ctx_save (ctx);
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y - ch * 0.1);
        ctx_rel_line_to (ctx, cw * 0.9, -ch/2 * 0.8);
        ctx_rel_line_to (ctx, -cw * 0.9, -ch/2 * 0.8);
        ctx_line_width (ctx, cw * 0.2);
        ctx_stroke (ctx);
        ctx_restore (ctx);
        return 0;
      case 0xe0b2: // PowerLine Right solid
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_move_to (ctx, cw, 0);
        ctx_rel_line_to (ctx, -cw, -ch/2);
        ctx_rel_line_to (ctx, cw, -ch/2);
        ctx_fill (ctx);
        return 0;
      case 0xe0b3: // PowerLine right line
        ctx_save (ctx);
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y - ch * 0.1);
        ctx_rel_move_to (ctx, cw, 0);
        ctx_rel_line_to (ctx, -cw * 0.9, -ch/2 * 0.8);
        ctx_rel_line_to (ctx,  cw * 0.9, ch/2 * 0.8);
        ctx_line_width (ctx, cw * 0.2);
        ctx_stroke (ctx);
        ctx_restore (ctx);
        return 0;
        /*
      case 0x1fb70: // left triangular one quarter block
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_line_to (ctx, 0, -ch);
        ctx_rel_line_to (ctx, cw/2, -ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x1fb72: // right triangular one quarter block
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_move_to (ctx, cw/2, -ch/2);
        ctx_rel_line_to (ctx, cw/2, -ch/2);
        ctx_rel_line_to (ctx, 0, ch);
        ctx_fill (ctx);
        return 0;
      case 0x1fb73: // lower triangular one quarter block
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_line_to (ctx, cw/2, -ch/2);
        ctx_rel_line_to (ctx, cw/2, ch/2);
        ctx_fill (ctx);
        return 0;
      case 0x1fb71: // upper triangular one quarter block
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_move_to (ctx, cw/2, -ch/2);
        ctx_rel_line_to (ctx, -cw/2, -ch/2);
        ctx_rel_line_to (ctx, cw, 0);
        ctx_fill (ctx);
        return 0;
        */
      case 0x25E2: // VT_BLACK_LOWER_RIGHT_TRIANGLE:
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_line_to (ctx, cw, -ch);
        ctx_rel_line_to (ctx, 0, ch);
        ctx_fill (ctx);
        return 0;
      case 0x25E3: //  VT_BLACK_LOWER_LEFT_TRIANGLE:
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_line_to (ctx, 0, -ch);
        ctx_rel_line_to (ctx, cw, ch);
        ctx_fill (ctx);
        return 0;
      case 0x25E4: // tri
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y);
        ctx_rel_line_to (ctx, 0, -ch);
        ctx_rel_line_to (ctx, cw, 0);
        ctx_fill (ctx);
        return 0;
      case 0x25E5: // tri
        ctx_begin_path (ctx);
        ctx_move_to (ctx, x, y - ch);
        ctx_rel_line_to (ctx, cw, 0);
        ctx_rel_line_to (ctx, 0, ch);
        ctx_fill (ctx);
        return 0;
      case 0x2800:
      case 0x2801:
      case 0x2802:
      case 0x2803:
      case 0x2804:
      case 0x2805:
      case 0x2806:
      case 0x2807:
      case 0x2808:
      case 0x2809:
      case 0x280A:
      case 0x280B:
      case 0x280C:
      case 0x280D:
      case 0x280E:
      case 0x280F:
      case 0x2810:
      case 0x2811:
      case 0x2812:
      case 0x2813:
      case 0x2814:
      case 0x2815:
      case 0x2816:
      case 0x2817:
      case 0x2818:
      case 0x2819:
      case 0x281A:
      case 0x281B:
      case 0x281C:
      case 0x281D:
      case 0x281E:
      case 0x281F:
      case 0x2820:
      case 0x2821:
      case 0x2822:
      case 0x2823:
      case 0x2824:
      case 0x2825:
      case 0x2826:
      case 0x2827:
      case 0x2828:
      case 0x2829:
      case 0x282A:
      case 0x282B:
      case 0x282C:
      case 0x282D:
      case 0x282E:
      case 0x282F:
      case 0x2830:
      case 0x2831:
      case 0x2832:
      case 0x2833:
      case 0x2834:
      case 0x2835:
      case 0x2836:
      case 0x2837:
      case 0x2838:
      case 0x2839:
      case 0x283A:
      case 0x283B:
      case 0x283C:
      case 0x283D:
      case 0x283E:
      case 0x283F:
        ctx_begin_path (ctx);
        {
          int bit_pattern = unichar - 0x2800;
          int bit = 0;
          int u = 0;
          int v = 0;
          for (bit = 0; bit < 6; bit++)
            {
              if (bit_pattern & (1<<bit) )
                {
                  draw_braille_bit (ctx, x, y, cw, ch, u, v);
                }
              v++;
              if (v > 2)
                {
                  v = 0;
                  u++;
                }
            }
        }
        ctx_fill (ctx);
        return 0;
      case 0x2840:
      case 0x2841:
      case 0x2842:
      case 0x2843:
      case 0x2844:
      case 0x2845:
      case 0x2846:
      case 0x2847:
      case 0x2848:
      case 0x2849:
      case 0x284A:
      case 0x284B:
      case 0x284C:
      case 0x284D:
      case 0x284E:
      case 0x284F:
      case 0x2850:
      case 0x2851:
      case 0x2852:
      case 0x2853:
      case 0x2854:
      case 0x2855:
      case 0x2856:
      case 0x2857:
      case 0x2858:
      case 0x2859:
      case 0x285A:
      case 0x285B:
      case 0x285C:
      case 0x285D:
      case 0x285E:
      case 0x285F:
      case 0x2860:
      case 0x2861:
      case 0x2862:
      case 0x2863:
      case 0x2864:
      case 0x2865:
      case 0x2866:
      case 0x2867:
      case 0x2868:
      case 0x2869:
      case 0x286A:
      case 0x286B:
      case 0x286C:
      case 0x286D:
      case 0x286E:
      case 0x286F:
      case 0x2870:
      case 0x2871:
      case 0x2872:
      case 0x2873:
      case 0x2874:
      case 0x2875:
      case 0x2876:
      case 0x2877:
      case 0x2878:
      case 0x2879:
      case 0x287A:
      case 0x287B:
      case 0x287C:
      case 0x287D:
      case 0x287E:
      case 0x287F:
        ctx_begin_path (ctx);
        draw_braille_bit (ctx, x, y, cw, ch, 0, 3);
        {
          int bit_pattern = unichar - 0x2840;
          int bit = 0;
          int u = 0;
          int v = 0;
          for (bit = 0; bit < 6; bit++)
            {
              if (bit_pattern & (1<<bit) )
                {
                  draw_braille_bit (ctx, x, y, cw, ch, u, v);
                }
              v++;
              if (v > 2)
                {
                  v = 0;
                  u++;
                }
            }
        }
        ctx_fill (ctx);
        return 0;
      case 0x2880:
      case 0x2881:
      case 0x2882:
      case 0x2883:
      case 0x2884:
      case 0x2885:
      case 0x2886:
      case 0x2887:
      case 0x2888:
      case 0x2889:
      case 0x288A:
      case 0x288B:
      case 0x288C:
      case 0x288D:
      case 0x288E:
      case 0x288F:
      case 0x2890:
      case 0x2891:
      case 0x2892:
      case 0x2893:
      case 0x2894:
      case 0x2895:
      case 0x2896:
      case 0x2897:
      case 0x2898:
      case 0x2899:
      case 0x289A:
      case 0x289B:
      case 0x289C:
      case 0x289D:
      case 0x289E:
      case 0x289F:
      case 0x28A0:
      case 0x28A1:
      case 0x28A2:
      case 0x28A3:
      case 0x28A4:
      case 0x28A5:
      case 0x28A6:
      case 0x28A7:
      case 0x28A8:
      case 0x28A9:
      case 0x28AA:
      case 0x28AB:
      case 0x28AC:
      case 0x28AD:
      case 0x28AE:
      case 0x28AF:
      case 0x28B0:
      case 0x28B1:
      case 0x28B2:
      case 0x28B3:
      case 0x28B4:
      case 0x28B5:
      case 0x28B6:
      case 0x28B7:
      case 0x28B8:
      case 0x28B9:
      case 0x28BA:
      case 0x28BB:
      case 0x28BC:
      case 0x28BD:
      case 0x28BE:
      case 0x28BF:
        ctx_begin_path (ctx);
        draw_braille_bit (ctx, x, y, cw, ch, 1, 3);
        {
          int bit_pattern = unichar - 0x2880;
          int bit = 0;
          int u = 0;
          int v = 0;
          for (bit = 0; bit < 6; bit++)
            {
              if (bit_pattern & (1<<bit) )
                {
                  draw_braille_bit (ctx, x, y, cw, ch, u, v);
                }
              v++;
              if (v > 2)
                {
                  v = 0;
                  u++;
                }
            }
        }
        ctx_fill (ctx);
        return 0;
      case 0x28C0:
      case 0x28C1:
      case 0x28C2:
      case 0x28C3:
      case 0x28C4:
      case 0x28C5:
      case 0x28C6:
      case 0x28C7:
      case 0x28C8:
      case 0x28C9:
      case 0x28CA:
      case 0x28CB:
      case 0x28CC:
      case 0x28CD:
      case 0x28CE:
      case 0x28CF:
      case 0x28D0:
      case 0x28D1:
      case 0x28D2:
      case 0x28D3:
      case 0x28D4:
      case 0x28D5:
      case 0x28D6:
      case 0x28D7:
      case 0x28D8:
      case 0x28D9:
      case 0x28DA:
      case 0x28DB:
      case 0x28DC:
      case 0x28DD:
      case 0x28DE:
      case 0x28DF:
      case 0x28E0:
      case 0x28E1:
      case 0x28E2:
      case 0x28E3:
      case 0x28E4:
      case 0x28E5:
      case 0x28E6:
      case 0x28E7:
      case 0x28E8:
      case 0x28E9:
      case 0x28EA:
      case 0x28EB:
      case 0x28EC:
      case 0x28ED:
      case 0x28EE:
      case 0x28EF:
      case 0x28F0:
      case 0x28F1:
      case 0x28F2:
      case 0x28F3:
      case 0x28F4:
      case 0x28F5:
      case 0x28F6:
      case 0x28F7:
      case 0x28F8:
      case 0x28F9:
      case 0x28FA:
      case 0x28FB:
      case 0x28FC:
      case 0x28FD:
      case 0x28FE:
      case 0x28FF:
        ctx_begin_path (ctx);
        draw_braille_bit (ctx, x, y, cw, ch, 0, 3);
        draw_braille_bit (ctx, x, y, cw, ch, 1, 3);
        {
          int bit_pattern = unichar - 0x28C0;
          int bit = 0;
          int u = 0;
          int v = 0;
          for (bit = 0; bit < 6; bit++)
            {
              if (bit_pattern & (1<<bit) )
                {
                  draw_braille_bit (ctx, x, y, cw, ch, u, v);
                }
              v++;
              if (v > 2)
                {
                  v = 0;
                  u++;
                }
            }
        }
        ctx_fill (ctx);
        return 0;
      case 0x1fb00:
      case 0x1fb01:
      case 0x1fb02:
      case 0x1fb03:
      case 0x1fb04:
      case 0x1fb05:
      case 0x1fb06:
      case 0x1fb07:
      case 0x1fb08:
      case 0x1fb09:
      case 0x1fb0a:
      case 0x1fb0b:
      case 0x1fb0c:
      case 0x1fb0d:
      case 0x1fb0e:
      case 0x1fb0f:
      case 0x1fb10:
      case 0x1fb11:
      case 0x1fb12:
      case 0x1fb13:
      case 0x1fb14:
      case 0x1fb15:
      case 0x1fb16:
      case 0x1fb17:
      case 0x1fb18:
      case 0x1fb19:
      case 0x1fb1a:
      case 0x1fb1b:
      case 0x1fb1c:
      case 0x1fb1d:
      case 0x1fb1e:
      case 0x1fb1f:
      case 0x1fb20:
      case 0x1fb21:
      case 0x1fb22:
      case 0x1fb23:
      case 0x1fb24:
      case 0x1fb25:
      case 0x1fb26:
      case 0x1fb27:
      case 0x1fb28:
      case 0x1fb29:
      case 0x1fb2a:
      case 0x1fb2b:
      case 0x1fb2c:
      case 0x1fb2d:
      case 0x1fb2e:
      case 0x1fb2f:
      case 0x1fb30:
      case 0x1fb31:
      case 0x1fb32:
      case 0x1fb33:
      case 0x1fb34:
      case 0x1fb35:
      case 0x1fb36:
      case 0x1fb37:
      case 0x1fb38:
      case 0x1fb39:
      case 0x1fb3a:
      case 0x1fb3b:

        {
          ctx_begin_path (ctx);
          uint32_t bitmask = (unichar - 0x1fb00) + 1;
          if (bitmask > 20) bitmask ++;
          if (bitmask > 41) bitmask ++;
          int bit = 0;
          for (int v = 0; v < 3; v ++)
          for (int u = 0; u < 2; u ++, bit ++)
          {
            if (bitmask & (1<<bit))
            {
              draw_sextant_bit (ctx, x, y, cw, ch, u, v);
            }
          }
          ctx_fill (ctx);
          return 0;
        }
        break;
      case 0x1fb3c:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch / 3.0f);
          ctx_rel_line_to (ctx, cw/2, ch/3.0f);
          ctx_fill (ctx);
          return 0;
        }
        break;
      case 0x1fb3d:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/3.0f);
          ctx_rel_line_to (ctx, cw, ch/3.0f);
          ctx_fill (ctx);
          return 0;
        }
        break;
      case 0x1fb3e:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0,   -ch/3.0f * 2);
          ctx_rel_line_to (ctx, cw/2, ch/3.0f * 2);
          ctx_fill (ctx);
          return 0;
        }
        break;
      case 0x1fb3f:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0,  -ch/3.0f * 2);
          ctx_rel_line_to (ctx, cw, ch/3.0f * 2);
          ctx_fill (ctx);
          return 0;
        }
        break;
      case 0x1fb40:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, ch);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb41:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw/2, -ch/3.0);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, 0.0, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb42:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw, -ch/3.0);
          ctx_rel_line_to (ctx, 0.0, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb43:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/3.0f);
          ctx_rel_line_to (ctx, cw/2, -ch/3.0f*2.0f);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, 0.0, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb44:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, cw, -ch/3.0 * 2);
          ctx_rel_line_to (ctx, 0.0, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb45:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, cw/2, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, 0.0, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb46:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, cw, -ch/3.0);
          ctx_rel_line_to (ctx, 0.0, ch/3.0*2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb47:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, cw/2, -ch/3.0);
          ctx_rel_line_to (ctx, 0.0, ch/3.0);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb48:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, cw, -ch/3.0);
          ctx_rel_line_to (ctx, 0.0, ch/3.0);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb49:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, cw/2, -ch/3.0*2);
          ctx_rel_line_to (ctx, 0.0, ch/3.0*2);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb4a:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, cw, -ch/3.0*2);
          ctx_rel_line_to (ctx, 0.0, ch/3.0*2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb4b:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, cw/2, ch/3);
          ctx_rel_line_to (ctx, 0, ch/3.0*2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb4c:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, ch/3);
          ctx_rel_line_to (ctx, 0, ch/3.0*2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb4d:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, ch/3);
          ctx_rel_line_to (ctx, 0, ch/3.0*2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb4e:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, cw/2, ch/3 *  2);
          ctx_rel_line_to (ctx, 0, ch/3.0);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb4f:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, ch/3 *  2);
          ctx_rel_line_to (ctx, 0, ch/3.0);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb50:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, cw/2, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb51:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw, ch/3.0);
          ctx_rel_line_to (ctx, 0, ch/3.0);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb52:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb53:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw, -ch/3.0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb54:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb55:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw, -ch/3.0*2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb56:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_rel_line_to (ctx, -cw/2, -ch);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb57:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, 0, -ch/3);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, -cw/2, ch/3);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb58:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, 0, -ch/3);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, -cw, ch/3);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb59:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, -cw/2, ch/3 * 2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb5a:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0);
          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, -cw, ch/3 * 2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb5b:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, -cw/2, ch);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb5c:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3);

          ctx_rel_line_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3);
          ctx_rel_line_to (ctx, -cw, ch/3);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb5d:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3 * 2);
          ctx_rel_line_to (ctx, -cw/2, ch/3);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb5e:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3 * 2);
          ctx_rel_line_to (ctx, -cw, ch/3);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb5f:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3);
          ctx_rel_line_to (ctx, -cw/2, ch/3*2);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb60:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3);
          ctx_rel_line_to (ctx, -cw, ch/3*2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb61:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, -cw/2, ch);
          ctx_rel_line_to (ctx, -cw/2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb62:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/2, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, 0, ch/3);
          ctx_rel_line_to (ctx, -cw/2, -ch/3);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb63:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3);
          ctx_rel_line_to (ctx, -cw, -ch/3);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb64:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/2, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, 0, ch/3*2);
          ctx_rel_line_to (ctx, -cw/2, -ch/3*2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb65:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3*2);
          ctx_rel_line_to (ctx, -cw, -ch/3*2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb66:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/2, -ch);
          ctx_rel_line_to (ctx, cw/2, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw/2, -ch);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb67:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/3.0*2);
          ctx_rel_line_to (ctx, 0, -ch/3);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/3.0*2);
          ctx_rel_line_to (ctx, -cw, -ch/3.0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb68:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, cw/2, -ch/2);
          ctx_rel_line_to (ctx, -cw/2, -ch/2);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb69:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, ch/2);
          ctx_rel_line_to (ctx, cw/2, -ch/2);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb6a:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, -cw/2, ch/2);
          ctx_rel_line_to (ctx, cw/2, ch/2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb6b:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw/2, -ch/2);
          ctx_rel_line_to (ctx, -cw/2, ch/2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb6c:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, ch/2);
          ctx_rel_line_to (ctx, -cw/2, ch/2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb6d:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, -cw/2, ch/2);
          ctx_rel_line_to (ctx, -cw/2, -ch/2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb6e:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw, -ch);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw/2, -ch/2);
          ctx_rel_line_to (ctx, cw/2, -ch/2);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb6f:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, cw/2, -ch/2);
          ctx_rel_line_to (ctx, cw/2, ch/2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb82:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/8 * 2);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_move_to (ctx, 0, ch/8 * 2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb83:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/8 * 3);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_move_to (ctx, 0, ch/8 * 3);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb84:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/8 * 5);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_move_to (ctx, 0, ch/8 * 5);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb85:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/8 * 6);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_move_to (ctx, 0, ch/8 * 6);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb86:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, 0, -ch/8 * 7);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_rel_move_to (ctx, 0, ch/8 * 7);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb87:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/8*6, 0);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_move_to (ctx, cw/8*2, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_move_to (ctx, -cw/8*2, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb88:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/8*5, 0);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_move_to (ctx, cw/8*3, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_move_to (ctx, -cw/8*3, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb89:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/8*3, 0);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_move_to (ctx, cw/8*5, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_move_to (ctx, -cw/8*5, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb8a:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_move_to (ctx, cw/8*2, 0);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_move_to (ctx, cw/8*6, 0);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_move_to (ctx, -cw/8*6, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb97:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch/4);
          ctx_rel_move_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/4);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_close_path (ctx);
          ctx_move_to (ctx, 0, -ch/2);
          ctx_rel_line_to (ctx, 0, -ch/4);
          ctx_rel_move_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, 0, ch/4);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb9a:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, cw/2, -ch/2);
          ctx_rel_line_to (ctx, -cw/2, -ch/2);
          ctx_rel_move_to (ctx, cw, 0);
          ctx_rel_line_to (ctx, -cw/2, ch/2);
          ctx_rel_line_to (ctx, cw/2, ch/2);
          ctx_rel_line_to (ctx, -cw, 0);
          ctx_fill (ctx);
          return 0;
        }
      case 0x1fb9b:
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x, y);
          ctx_rel_line_to (ctx, 0, -ch);
          ctx_rel_line_to (ctx, cw/2, ch/2);
          ctx_rel_line_to (ctx, cw/2, -ch/2);
          ctx_rel_line_to (ctx, 0, ch);
          ctx_rel_line_to (ctx, -cw/2, -ch/2);
          ctx_rel_line_to (ctx, -cw/2, ch/2);
          ctx_fill (ctx);
          return 0;
        }

    }
  return -1;
}

void vt_ctx_glyph (Ctx *ctx, VT *vt, float x, float y, int unichar, int bold, float scale_x, float scale_y, float offset_y)
{
  int did_save = 0;
  if (unichar <= ' ')
    return;
  scale_x *= vt->scale_x;
  scale_y *= vt->scale_y;

  CtxBackendType backend_type = ctx_backend_type (ctx);

  if (backend_type != CTX_BACKEND_TERM)
  {
    // TODO : use our own special glyphs when glyphs are not passed through
    if (!vt_special_glyph (ctx, vt, x, y + offset_y * vt->ch, vt->cw * scale_x, vt->ch * scale_y, unichar) )
      return;
  }

  if (scale_x != 1.0 || scale_y != 1.0)
    {
      if (!did_save)
      {
        ctx_save (ctx);
        did_save = 1;
      }

      ctx_translate (ctx, x, y);
      ctx_scale (ctx, scale_x, scale_y);
      ctx_translate (ctx, -x, -y);
    }
  if (offset_y != 0.0f)
  {
    if (!did_save)
    {
      ctx_save (ctx);
      did_save = 1;
    }
    ctx_translate (ctx, 0, vt->font_size * offset_y);
  }
  y -= vt->font_size * 0.22;
  if (bold
      && backend_type != CTX_BACKEND_TERM)
    {
      ctx_move_to (ctx, x - vt->font_size/30.0, y);
      //ctx_line_width (ctx, vt->font_size/30.0);
      ctx_glyph (ctx, unichar, 0);
    }
  ctx_move_to (ctx, x, y);
  ctx_glyph (ctx, unichar, 0);
  if (did_save)
    ctx_restore (ctx);
}

//static uint8_t palette[256][3];

/* optimized for ANSI ART - and avoidance of human metamers
 * among color deficient vision - by distributing and pertubating
 * until all 64 combinations - sans self application, have
 * likely to be discernable by humans.
 */


void vt_ctx_get_color (VT *vt, int no, int intensity, uint8_t *rgba)
{
  uint8_t r = 0, g = 0, b = 0;
  if (no < 16 && no >= 0)
    {
      switch (intensity)
        {
          case 0:
            no = 0;
            break;
          case 1:
            // 15 becomes 7
            if (no == 15) { no = 8; }
            else if (no > 8) { no -= 8; }
            break;
          case 2:
            /* give the normal color special treatment, and in really normal
             * cirumstances it is the dim variant of foreground that is used
             */
            if (no == 15) { no = 7; }
            break;
          case 3:
          case 4:
            if (no < 8)
              { no += 8; }
            break;
          default:
            break;
        }
      r = palettes[vt->palette_no][no][0];
      g = palettes[vt->palette_no][no][1];
      b = palettes[vt->palette_no][no][2];
    }
  else if (no < 16 + 6*6*6)
    {
      no = no-16;
      b = (no % 6) * 255 / 5;
      no /= 6;
      g = (no % 6) * 255 / 5;
      no /= 6;
      r = (no % 6) * 255 / 5;
    }
  else
    {
      int gray = no - (16 + 6*6*6);
      float val = gray * 255 / 24;
      r = g = b = val;
    }
  rgba[0]=r;
  rgba[1]=g;
  rgba[2]=b;
  rgba[3]=255;
}

int vt_keyrepeat (VT *vt)
{
  return vt->keyrepeat;
}

static void vt_flush_bg (VT *vt, Ctx *ctx)
{
  if (vt->bg_active)
  {
    ctx_rgba8 (ctx, vt->bg_rgba[0], vt->bg_rgba[1], vt->bg_rgba[2], vt->bg_rgba[3]);
    ctx_rectangle (ctx, vt->bg_x0, vt->bg_y0, vt->bg_width, vt->bg_height);
    ctx_fill (ctx);
    vt->bg_active = 0;
  }
}

static void vt_draw_bg (VT *vt, Ctx *ctx,
                        float x0, float y0,
                        float width, float height,
                        uint8_t *rgba)
{
   int same_color = !memcmp(rgba, vt->bg_rgba, 4);
   if (vt->bg_active && !same_color)
   {
     vt_flush_bg (vt, ctx);
   }

   if (vt->bg_active && same_color)
   {
     vt->bg_width += width;
   }
   else
   {
     memcpy (vt->bg_rgba, rgba, 4);
     vt->bg_active = 1;
     vt->bg_x0 = x0;
     vt->bg_y0 = y0;
     vt->bg_width = width;
     vt->bg_height = height;
   }
}

float vt_draw_cell (VT      *vt, Ctx *ctx,
                    int      row, int col, // pass 0 to force draw - like
                    float    x0, float y0, // for scrollback visible
                    uint64_t style,
                    uint32_t unichar,
                    int      dw, int dh,
                    int      in_smooth_scroll,
                    int      in_select,
                    int      is_fg)
// dw is 0 or 1
// dh is 0 1 or -1  1 is upper -1 is lower
{
  int on_white = vt->reverse_video;
  int color = 0;
  int bold = (style & STYLE_BOLD) != 0;
  int dim = (style & STYLE_DIM) != 0;
  int is_hidden = (style & STYLE_HIDDEN) != 0;
  int proportional = (style & STYLE_PROPORTIONAL) != 0;
  int fg_set = (style & STYLE_FG_COLOR_SET) != 0;
  int bg_intensity = 0;
  int fg_intensity = 2;
  int reverse = ( (style & STYLE_REVERSE) != 0) ^ in_select;
  int blink = ( (style & STYLE_BLINK) != 0);
  int blink_fast = ( (style & STYLE_BLINK_FAST) != 0);
  int cw = vt->cw;
  int ch = vt->ch;
  if (proportional)
    {
      if (vt->font_is_mono)
        {
          ctx_font (ctx, "regular");
          vt->font_is_mono = 0;
        }
      cw = ctx_glyph_width (ctx, unichar);
    }
  else
    {
      if (vt->font_is_mono == 0)
        {
          ctx_font (ctx, "mono");
          vt->font_is_mono = 1;
          if (col > 1)
            {
              int x = x0;
              int new_cw = cw - ( (x % cw) );
              if (new_cw < cw*3/2)
                { new_cw += cw; }
              cw = new_cw;
            }
        }
    }
  float scale_x  = 1.0f;
  float scale_y  = 1.0f;
  float offset_y = 0.0f;
  if (dw)
    {
      scale_x = 2.0f;
    }
  if (dh)
    {
      scale_y = 2.0f;
    }
  if (dh == 1)
    {
      offset_y = 0.5f;
    }
  else if (dh == -1)
    {
      offset_y =  0.0f;
    }
  if (in_smooth_scroll)
    {
      offset_y -= vt->scroll_offset / (dh?2:1);
    }
  cw *= scale_x;
  if (blink_fast)
    {
      if ( (vt->blink_state % 2) == 0)
        { blink = 1; }
      else
        { blink = 0; }
    }
  else if (blink)
    {
      if ( (vt->blink_state % 10) < 5)
        { blink = 1; }
      else
        { blink = 0; }
    }
  /*
     from the vt100 technical-manual:

     "Reverse characters [..] normally have dim backgrounds with
     black characters so that large white spaces have the same impact
     on the viewer's eye as the smaller brighter white areas of
     normal characters. Bold and reverse asserted together give a
     background of normal intensity. Blink applied to nonreverse
     characters causes them to alternate between their usual
     intensity and the next lower intensity. (Normal characters vary
     between normal and dim intensity. Bold characters vary between
     bright and normal intensity.) Blink applied to a reverse
     character causes that character to alternate between normal and
     reverse video representations of that character."

     This is in contrast with how the truth table appears to be
     meant used, since it uses a reverse computed as the xor of
     the global screen reverse and the reverse attribute of the
     cell.

     To fulfil the more asthethic resulting from implementing the
     text, and would be useful to show how the on_bright background
     mode of the vt100 actually displays the vttest.

     */
  if (on_white)
    {
          if (bold)
            {
              bg_intensity =           2;
              fg_intensity = blink?1:  0;
            }
          else if (dim)
            {
              bg_intensity =           2;
              fg_intensity = blink?3:  1;
            }
          else
            {
              bg_intensity =           2;
              fg_intensity = blink?1:  0;
            }
          if (fg_set)
            {
              fg_intensity = blink?2:3;
            }
    }
  else /* bright on dark */
    {
          if (bold)
            {
              bg_intensity =           0;
              fg_intensity = blink?2:  3;
            }
          else if (dim)
            {
              bg_intensity =           0;
              fg_intensity = blink?0:  1;
            }
          else
            {
              bg_intensity =           0;
              fg_intensity = blink?1:  2;
            }
    }
  uint8_t bg_rgb[4]= {0,0,0,255};
  uint8_t fg_rgb[4]= {255,255,255,255};
  {
      //ctx_begin_path (ctx);
      if (style &  STYLE_BG24_COLOR_SET)
        {
          uint64_t temp = style >> 40;
          bg_rgb[0] = temp & 0xff;
          temp >>= 8;
          bg_rgb[1] = temp & 0xff;
          temp >>= 8;
          bg_rgb[2] = temp & 0xff;
#if 0
          if (dh)
          {
             bg_rgb[0] = 
             bg_rgb[1] =
             bg_rgb[2] = 30;
          }
#endif
        }
      else
        {
          if (style & STYLE_BG_COLOR_SET)
            {
              color = (style >> 40) & 255;
              bg_intensity = -1;
              vt_ctx_get_color (vt, color, bg_intensity, bg_rgb);
            }
          else
            {
              switch (bg_intensity)
                {
                  case 0:
                    for (int i = 0; i <3 ; i++)
                      { bg_rgb[i] = vt->bg_color[i]; }
                    break;
                  case 1:
                    for (int i = 0; i <3 ; i++)
                      { bg_rgb[i] = vt->bg_color[i] * 0.5 + vt->fg_color[i] * 0.5; }
                    break;
                  case 2:
                    for (int i = 0; i <3 ; i++)
                      { bg_rgb[i] = vt->bg_color[i] * 0.05 + vt->fg_color[i] * 0.95; }
                    break;
                  case 3:
                    for (int i = 0; i <3 ; i++)
                      { bg_rgb[i] = vt->fg_color[i]; }
                    break;
                }
            }
        }
  }
  if (style & STYLE_FG24_COLOR_SET)
    {
      uint64_t temp = style >> 16;
      fg_rgb[0] = temp & 0xff;
      temp >>= 8;
      fg_rgb[1] = temp & 0xff;
      temp >>= 8;
      fg_rgb[2] = temp & 0xff;
    }
  else
    {
      if ( (style & STYLE_FG_COLOR_SET) == 0)
        {
          switch (fg_intensity)
            {
              case 0:
                for (int i = 0; i <3 ; i++)
                  { fg_rgb[i] = vt->bg_color[i] * 0.7 + vt->fg_color[i] * 0.3; }
                break;
              case 1:
                for (int i = 0; i <3 ; i++)
                  { fg_rgb[i] = vt->bg_color[i] * 0.5 + vt->fg_color[i] * 0.5; }
                break;
              case 2:
                for (int i = 0; i <3 ; i++)
                  { fg_rgb[i] = vt->bg_color[i] * 0.20 + vt->fg_color[i] * 0.80; }
                break;
              case 3:
                for (int i = 0; i <3 ; i++)
                  { fg_rgb[i] = vt->fg_color[i]; }
            }
        }
      else
        {
          color = (style >> 16) & 255;
          vt_ctx_get_color (vt, color, fg_intensity, fg_rgb);
        }
  }

  if (reverse)
  {
    for (int c = 0; c < 3; c ++)
    {
      int t = bg_rgb[c];
      bg_rgb[c] = fg_rgb[c];
      fg_rgb[c] = t;
    }
  }

  if (is_fg ||
      ((!on_white) && bg_rgb[0]==0 && bg_rgb[1]==0 && bg_rgb[2]==0) ||
      ((on_white) && bg_rgb[0]==255 && bg_rgb[1]==255 && bg_rgb[2]==255))
          /* these comparisons are not entirely correct, when on dark background we assume black to
           * be default and non-set, even when theme might differ
           */
  {
    /* skipping draw of background */
  }
  else
  {
    if (dh)
    {
      vt_draw_bg (vt, ctx, ctx_floorf(x0),
         ctx_floorf(y0 - ch - ch * (vt->scroll_offset)), cw, ch, bg_rgb);
    }
    else
    {
      vt_draw_bg (vt, ctx, x0, y0 - ch + ch * offset_y, cw, ch, bg_rgb);
    }
  }

  if (!is_fg)
    return cw;

  int italic        = (style & STYLE_ITALIC) != 0;
  int strikethrough = (style & STYLE_STRIKETHROUGH) != 0;
  int overline      = (style & STYLE_OVERLINE) != 0;
  int underline     = (style & STYLE_UNDERLINE) != 0;
  int underline_var = (style & STYLE_UNDERLINE_VAR) != 0;
  if (dh == 1)
    {
      underline = underline_var = 0;
    }
  int double_underline = 0;
  int curved_underline = 0;
  if (underline_var)
    {
      if (underline)
        {
          double_underline = 1;
        }
      else
        {
          curved_underline = 1;
        }
    }

  int has_underline = (underline || double_underline || curved_underline);

  if (unichar == ' ' && !has_underline)
    is_hidden = 1;

  if (!is_hidden)
    {

      ctx_rgba8 (ctx, fg_rgb[0], fg_rgb[1], fg_rgb[2], 255);


      if (italic)
        {
          ctx_save (ctx);
          ctx_translate (ctx, (x0 + cw/3), (y0 + vt->ch/2) );
          ctx_scale (ctx, 0.9, 0.9);
          ctx_rotate (ctx, 0.15);
          ctx_translate (ctx, - (x0 + cw/3), - (y0 + vt->ch/2) );
        }
      vt_ctx_glyph (ctx, vt, x0, y0, unichar, bold, scale_x, scale_y, offset_y);
      if (italic)
        {
          ctx_restore (ctx);
        }
      if (curved_underline)
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x0, y0 - vt->font_size * 0.07 - vt->ch * vt->scroll_offset);
          ctx_rel_line_to (ctx, (cw+2) /3, -vt->ch * 0.05);
          ctx_rel_line_to (ctx, (cw+2) /3, vt->ch * 0.1);
          ctx_rel_line_to (ctx, (cw+2) /3, -vt->ch * 0.05);
          //ctx_rel_line_to (ctx, cw, 0);
          ctx_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.050:0.04) );
          ctx_stroke (ctx);
        }
      else if (double_underline)
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x0, y0 - vt->font_size * 0.130 - vt->ch * vt->scroll_offset);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_move_to (ctx, x0, y0 - vt->font_size * 0.030 - vt->ch * vt->scroll_offset);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.050:0.04) );
          ctx_stroke (ctx);
        }
      else if (underline)
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x0, y0 - vt->font_size * 0.07 - vt->ch * vt->scroll_offset);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.075:0.05) );
          ctx_stroke (ctx);
        }
      if (overline)
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x0, y0 - vt->font_size * 0.94 - vt->ch * vt->scroll_offset);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.075:0.05) );
          ctx_stroke (ctx);
        }
      if (strikethrough)
        {
          ctx_begin_path (ctx);
          ctx_move_to (ctx, x0, y0 - vt->font_size * 0.43 - vt->ch * vt->scroll_offset);
          ctx_rel_line_to (ctx, cw, 0);
          ctx_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.075:0.05) );
          ctx_stroke (ctx);
        }
    }
  return cw;
}

int vt_has_blink (VT *vt)
{
  if (!vt) return 0;
  return (vt->in_smooth_scroll ?  10 : 0);
  //return vt->has_blink + (vt->in_smooth_scroll ?  10 : 0);
}

//extern int enable_terminal_menu;
//

//void ctx_set_popup (Ctx *ctx, void (*popup)(Ctx *ctx, void *data), void *popup_data);

static char *primary = NULL;
static void scrollbar_drag (CtxEvent *event, void *data, void *data2);
static int scrollbar_down = 0;

void ctx_client_mouse_event (CtxEvent *event, void *data, void *data2)
{
  CtxClient *client = data;
  if (!client)
  {
    event->stop_propagate = 1;
    return;
  }
  VT *vt = client->vt;

  float  x = event->x;
  float  y = event->y;
  int device_no = event->device_no;
  char buf[128]="";

  if (vt)
  {
  if ((!vt->in_alt_screen) &&
      (event->x > vt->width - vt->cw * 1.5 || scrollbar_down) &&
      (event->type == CTX_DRAG_MOTION ||
      event->type == CTX_DRAG_PRESS ||
      event->type == CTX_DRAG_RELEASE))
    return scrollbar_drag (event, data, data2);
  switch (event->type)
  {
    case CTX_MOTION:
    case CTX_DRAG_MOTION:
      //if (event->device_no==1)
      {
        sprintf (buf, "pm %.0f %.0f %i", x, y, device_no);
//      ctx_queue_draw (event->ctx);
        ctx_client_lock (client);
        vt_feed_keystring (vt, event, buf);
        ctx_client_unlock (client);
//      vt->rev++;
      }
      break;
    case CTX_DRAG_PRESS:
      if (event->device_no==2)
      {
        if (primary)
        {
          if (vt)
            vt_paste (vt, primary);
        }
      }
      else if (event->device_no==3 && !vt->in_alt_screen)
      {
        vt->popped = 1;
      }
      else
      {
        sprintf (buf, "pp %.0f %.0f %i", x, y, device_no);
        ctx_client_lock (client);
        vt_feed_keystring (vt, event, buf);
        ctx_client_unlock (client);
//      ctx_queue_draw (event->ctx);
//      vt->rev++;
      }
      break;
    case CTX_DRAG_RELEASE:
      if (event->device_no==3 && !vt->in_alt_screen)
      {
        vt->popped = 0;
      }
        ctx_queue_draw (event->ctx);
        sprintf (buf, "pr %.0f %.0f %i", x, y, device_no);
        ctx_client_lock (client);
        vt_feed_keystring (vt, event, buf);
        ctx_client_unlock (client);
      break;
    default:
      // we should not stop propagation
      return;
      break;
  }
  }
  else
  {
     CtxEvent *copy = ctx_event_copy (event);
     ctx_list_append (&client->ctx_events, copy);
  }
  event->stop_propagate = 1;
//vt->rev++;
}

void vt_mouse_event (CtxEvent *event, void *data, void *data2)
{
  VT   *vt = data;
  CtxClient *client = vt_get_client (vt);
  if (!client)
  {
    event->stop_propagate = 1;
    return;
  }
  float  x = event->x;
  float  y = event->y;
  int device_no = event->device_no;
  char buf[128]="";
  if ((!vt->in_alt_screen) &&
      (event->x > vt->width - vt->cw * 1.5 || scrollbar_down) &&
      (event->type == CTX_DRAG_MOTION ||
      event->type == CTX_DRAG_PRESS ||
      event->type == CTX_DRAG_RELEASE))
    return scrollbar_drag (event, data, data2);
  switch (event->type)
  {
    case CTX_MOTION:
    case CTX_DRAG_MOTION:
      //if (event->device_no==1)
      {
        sprintf (buf, "pm %.0f %.0f %i", x, y, device_no);
//      ctx_queue_draw (event->ctx);
        ctx_client_lock (client);
        vt_feed_keystring (vt, event, buf);
        ctx_client_unlock (client);
//      vt->rev++;
      }
      break;
    case CTX_DRAG_PRESS:
      if (event->device_no==2)
      {
        if (primary)
        {
          if (vt)
            vt_paste (vt, primary);
        }
      }
      else if (event->device_no==3 && !vt->in_alt_screen)
      {
        vt->popped = 1;
      }
      else
      {
        sprintf (buf, "pp %.0f %.0f %i", x, y, device_no);
        ctx_client_lock (client);
        vt_feed_keystring (vt, event, buf);
        ctx_client_unlock (client);
//      ctx_queue_draw (event->ctx);
//      vt->rev++;
      }
      break;
    case CTX_DRAG_RELEASE:
      if (event->device_no==3 && !vt->in_alt_screen)
      {
        vt->popped = 0;
      }
        ctx_queue_draw (event->ctx);
        sprintf (buf, "pr %.0f %.0f %i", x, y, device_no);
        ctx_client_lock (client);
        vt_feed_keystring (vt, event, buf);
        ctx_client_unlock (client);
      break;
    default:
      // we should not stop propagation
      return;
      break;
  }
  event->stop_propagate = 1;
//vt->rev++;
}
static int scrollbar_focused = 0;
#if 0
static void scrollbar_enter (CtxEvent *event, void *data, void *data2)
{
  VT *vt = data;
  vt->rev++;
  scrollbar_focused = 1;
}

static void scrollbar_leave (CtxEvent *event, void *data, void *data2)
{
  VT *vt = data;
  vt->rev++;
  scrollbar_focused = 0;
}
#endif

int ctx_vt_had_alt_screen (VT *vt)
{
  return vt?vt->had_alt_screen:0;
}

static void scrollbar_drag (CtxEvent *event, void *data, void *data2)
{
  VT *vt = data;
  float disp_lines = vt->rows;
  float tot_lines = vt->line_count + vt->scrollback_count;

  vt->scroll = tot_lines - disp_lines - (event->y*1.0/ ctx_client_height (vt->root_ctx, vt->id)) * tot_lines + disp_lines/2;
  if (vt->scroll < 0) { vt->scroll = 0.0; }
  if (vt->scroll > vt->scrollback_count) { vt->scroll = vt->scrollback_count; }
  ctx_client_rev_inc (vt->client);
  ctx_queue_draw (event->ctx);
  event->stop_propagate = 1;

  switch (event->type)
  {
    case CTX_DRAG_PRESS:
      scrollbar_down = 1;
      break;
    case CTX_DRAG_RELEASE:
      scrollbar_down = 0;
      break;
    default:
      break;
  }
}

#if 0
static void scroll_handle_drag (CtxEvent *event, void *data, void *data2)
{
  VT *vt = data;
  float tot_lines = vt->line_count + vt->scrollback_count;
  if (event->type == CTX_DRAG_MOTION)
  {
    vt->scroll -= (event->delta_y * tot_lines) / (vt->rows * vt->ch);
  }
  if (vt->scroll < 0) { vt->scroll = 0.0; }
  if (vt->scroll > vt->scrollback_count) { vt->scroll = vt->scrollback_count; }
  vt->rev++;
  event->stop_propagate = 1;
}
#endif

#if 0
static void test_popup (Ctx *ctx, void *data)
{
  VT *vt = data;

  float x = ctx_client_x (vt->root_ctx, vt->id);
  float y = ctx_client_y (vt->root_ctx, vt->id);
  ctx_rectangle (ctx, x, y, 100, 100);
  ctx_rgb (ctx, 1,0,0);
  ctx_fill (ctx);
}
#endif

void itk_style_color (Ctx *ctx, const char *name); // only itk fun used in vt

void vt_use_images (VT *vt, Ctx *ctx)
{
  /*  this is a call intended for minimized/shaded fully obscured
   *  clients to make sure their textures are kept alive
   *  in the server
   */
  //float x0=0;
  float y0=0;
  //vt->has_blink = 0;
  //vt->blink_state++;

  ctx_save (ctx);

  {
    /* draw graphics */
    for (int row = ((vt->scroll!=0.0f)?vt->scroll:0);
         row < (vt->scroll) + vt->rows * 4;
         row ++)
    {
       CtxList *l = ctx_list_nth (vt->lines, row);
       float y = y0 + vt->ch * (vt->rows - row);

       if (row >= vt->rows && !vt->in_alt_screen)
         {
           l = ctx_list_nth (vt->scrollback, row-vt->rows);
         }

       if (l && y <= (vt->rows - vt->scroll) *  vt->ch)
         {
           VtLine *line = l->data;
           if (line->ctx_copy)
             {
               ctx_render_ctx_textures (line->ctx_copy, ctx);
             }
         }
    }
  }
  ctx_restore (ctx);
}


void ctx_client_register_events (CtxClient *client, Ctx *ctx, double x0, double y0)
{
  ctx_begin_path (ctx);
  ctx_save (ctx);
  ctx_translate (ctx, x0, y0);
  ctx_rectangle (ctx, 0, 0, client->width, client->height);
  ctx_listen (ctx, CTX_DRAG,   ctx_client_mouse_event, client, NULL);
  ctx_listen (ctx, CTX_MOTION, ctx_client_mouse_event, client, NULL);
  ctx_begin_path (ctx);
  ctx_restore (ctx);
}

#if 0
void vt_register_events (VT *vt, Ctx *ctx, double x0, double y0)
{
  ctx_begin_path (ctx);
  ctx_save (ctx);
  ctx_translate (ctx, x0, y0);
  ctx_rectangle (ctx, 0, 0, vt->cols * vt->cw, vt->rows * vt->ch);
  ctx_listen (ctx, CTX_DRAG,   vt_mouse_event, vt, NULL);
  ctx_listen (ctx, CTX_MOTION, vt_mouse_event, vt, NULL);
  ctx_begin_path (ctx);
  ctx_restore (ctx);
}
#endif

void vt_draw (VT *vt, Ctx *ctx, double x0, double y0)
{
  ctx_begin_path (ctx);
  ctx_save (ctx);
  ctx_translate (ctx, x0, y0);
  if (getenv ("CTX_STAR_WARS"))
  ctx_apply_transform (ctx, 0.3120, -0.666, 700.,
                            0.0000, 0.015,  200.0,
                            0.00, -0.0007, 1.0);
  x0 = 0;
  y0 = 0;
  ctx_font (ctx, "mono");
  vt->font_is_mono = 0;
  ctx_font_size (ctx, vt->font_size * vt->font_to_cell_scale);
  vt->has_blink = 0;
  vt->blink_state++;
#if 0
  int cursor_x_px = 0;
  int cursor_y_px = 0;
  int cursor_w = vt->cw;
  int cursor_h = vt->ch;
  cursor_x_px = x0 + (vt->cursor_x - 1) * vt->cw;
  cursor_y_px = y0 + (vt->cursor_y - 1) * vt->ch;
  cursor_w = vt->cw;
  cursor_h = vt->ch;
#endif
  ctx_save (ctx);
  //if (vt->scroll || full)
    {
      ctx_begin_path (ctx);
#if 1
      ctx_rectangle (ctx, 0, 0, vt->width, //(vt->cols) * vt->cw,
                     (vt->rows) * vt->ch);
      if (vt->reverse_video)
        {
          itk_style_color (ctx, "terminal-bg-reverse");
          ctx_fill  (ctx);
        }
      else
        {
          itk_style_color (ctx, "terminal-bg");
          ctx_fill  (ctx);
        }
#endif
      if (vt->scroll != 0.0f)
        ctx_translate (ctx, 0.0, vt->ch * vt->scroll);
    }
  /* draw terminal lines */
   {
     for (int row = (vt->scroll!=0.0f)?vt->scroll:0; row < (vt->scroll) + vt->rows; row ++)
       {
         CtxList *l = ctx_list_nth (vt->lines, row);
         float y = y0 + vt->ch * (vt->rows - row);
         if (row >= vt->rows)
           {
             l = ctx_list_nth (vt->scrollback, row-vt->rows);
           }
         if (l && y <= (vt->rows - vt->scroll) *  vt->ch)
           {
             VtLine *line = l->data;
             int r = vt->rows - row;
             const char *data = line->string.str;

             vt->bg_active = 0;
             for (int is_fg = 0; is_fg < 2; is_fg++)
             {
               const char *d = data;
               float x = x0;
               uint64_t style = 0;
               uint32_t unichar = 0;
               int in_scrolling_region = vt->in_smooth_scroll &&
                   ((r >= vt->margin_top && r <= vt->margin_bottom) || r <= 0);
               if (is_fg)
                  vt_flush_bg (vt, ctx);
  
               for (int col = 1; col <= vt->cols * 1.33 && x < vt->cols * vt->cw; col++)
                 {
                   int c = col;
                   int real_cw;
                   int in_selected_region = 0;
                   //if (vt->in_alt_screen == 0)
                   {
                   if (r > vt->select_start_row && r < vt->select_end_row)
                     {
                       in_selected_region = 1;
                     }
                   else if (r == vt->select_start_row)
                     {
                       if (col >= vt->select_start_col) { in_selected_region = 1; }
                       if (r == vt->select_end_row)
                         {
                           if (col > vt->select_end_col) { in_selected_region = 0; }
                         }
                     }
                   else if (r == vt->select_end_row)
                     {
                       in_selected_region = 1;
                       if (col > vt->select_end_col) { in_selected_region = 0; }
                     }
                   }
                   if (vt->select_active == 0) in_selected_region = 0;
                   style = vt_line_get_style (line, col-1);
                   unichar = d?ctx_utf8_to_unichar (d) :' ';
  
                   int is_cursor = 0;
                   if (vt->cursor_x == col && vt->cursor_y == vt->rows - row && vt->cursor_visible)
                      is_cursor = 1;
  
                   real_cw=vt_draw_cell (vt, ctx, r, c, x, y, style, unichar,
                                         line->double_width,
                                         line->double_height_top?1:
                                         line->double_height_bottom?-1:0,
                                         in_scrolling_region,
                                         in_selected_region ^ is_cursor, is_fg);
                   if (r == vt->cursor_y && col == vt->cursor_x)
                     {
#if 0
                       cursor_x_px = x;
#endif
                     }
                   x+=real_cw;
                   if (style & STYLE_BLINK ||
                       style & STYLE_BLINK_FAST)
                     {
                       vt->has_blink = 1;
                     }
                   if (d)
                     {
                       d = mrg_utf8_skip (d, 1);
                       if (!*d) { d = NULL; }
                     }
                 }
             }
#if 0
             if (line->wrapped)
             {
               ctx_rectangle (ctx, x0, y, 10, 10);
               ctx_rgb (ctx, 1,0,0);
               ctx_fill (ctx);
             }
#endif
          }
      }
  }

#if 0
  /* draw cursor (done inline with fg/bg reversing, some cursor styles might need
   * additional drawing though
   */
  if (vt->cursor_visible)
    {
    //  ctx_rgba (ctx, 0.9, 0.8, 0.0, 0.5333);
      ctx_rgba (ctx, 1.0,1.0,1.0,1.0);
      ctx_begin_path (ctx);
      ctx_rectangle (ctx,
                     cursor_x_px, cursor_y_px,
                     cursor_w, cursor_h);
      ctx_fill (ctx);
    }
#endif


  {
    /* draw graphics */
     for (int row = ((vt->scroll!=0.0f)?vt->scroll:0); row < (vt->scroll) + vt->rows * 4; row ++)
      {
        CtxList *l = ctx_list_nth (vt->lines, row);
        float y = y0 + vt->ch * (vt->rows - row);

        if (row >= vt->rows && !vt->in_alt_screen)
          {
            l = ctx_list_nth (vt->scrollback, row-vt->rows);
          }

        if (l && y <= (vt->rows - vt->scroll) *  vt->ch)
          {
            VtLine *line = l->data;
            {
            for (int i = 0; i < 4; i++)
              {
                Image *image = line->images[i];
                if (image)
                  {
                    int u = (line->image_col[i]-1) * vt->cw + (line->image_X[i] * vt->cw);
                    int v = y - vt->ch + (line->image_Y[i] * vt->ch);
                //  int rows = (image->height + (vt->ch-1) ) /vt->ch;
                //
                //
                    if (v + image->height +vt->scroll * vt->ch > 0.0 &&
                        image->width && image->height /* some ghost images appear with these */
                        )
                    {
                    ctx_save (ctx);
                    ctx_rectangle (ctx, x0, y0 - vt->scroll * vt->ch, vt->cw * vt->cols,
                                    vt->ch * vt->rows);
                    ctx_clip (ctx);
                    char texture_n[65]; 

                    sprintf (texture_n, "vtimg%i", image->eid_no);
                    ctx_rectangle (ctx, u, v, image->width, image->height);
                    ctx_translate (ctx, u, v);

                    //replace this texture_n with NULL to
                    // be content addressed - but bit slower
                    ctx_define_texture (ctx, texture_n, image->width,
                                        image->height,
                                        0,
                                        image->kitty_format == 32 ?
                                                 CTX_FORMAT_RGBA8 :
                                                 CTX_FORMAT_RGB8,
                                        image->data, texture_n);
                    ctx_fill (ctx);

                    ctx_restore (ctx);
                    }
                  }
              }

            if (line->ctx_copy)
              {
                //fprintf (stderr, " [%i]\n", _ctx_frame (ctx));
                //ctx_render_stream (line->ctx_copy, stderr, 1);

                ctx_begin_path (ctx);
                ctx_save (ctx);
                ctx_rectangle (ctx, x0, y0 - vt->scroll * vt->ch, vt->cw * vt->cols,
                                    vt->ch * vt->rows);
                ctx_clip (ctx);
                ctx_translate (ctx, 0.0, y - vt->ch);

                //(vt->rows-row-1) * (vt->ch) );
                //float factor = vt->cols * vt->cw / 1000.0;
                //ctx_scale (ctx, factor, factor);
                //
                ctx_render_ctx (line->ctx_copy, ctx);
                ctx_restore (ctx);
              }
            }
          }
    //  y -= vt->ch;
      }
  }


  for (int i = 0; i < 4; i++)
    {
      if (vt->leds[i])
        {
          ctx_rgba (ctx, .5,1,.5,0.8);
          ctx_rectangle (ctx, vt->cw * i + vt->cw * 0.25, vt->ch * 0.25, vt->cw/2, vt->ch/2);
          ctx_fill (ctx);
        }
    }
  ctx_restore (ctx);
//#define SCROLL_SPEED 0.25;
#define SCROLL_SPEED 0.001;
  if (vt->in_smooth_scroll)
   {
     if (vt->in_smooth_scroll<0)
       {
         vt->scroll_offset += SCROLL_SPEED;
         if (vt->scroll_offset >= 0.0)
           {
             vt->scroll_offset = 0;
             vt->in_smooth_scroll = 0;
             ctx_client_rev_inc (vt->client);
           }
       }
     else
       {
         vt->scroll_offset -= SCROLL_SPEED;
         if (vt->scroll_offset <= 0.0)
           {
             vt->scroll_offset = 0;
             vt->in_smooth_scroll = 0;
             ctx_client_rev_inc (vt->client);
           }
       }
   }

    /* scrollbar */
    if (!vt->in_alt_screen)
    {
      float disp_lines = vt->rows;
      float tot_lines = vt->line_count + vt->scrollback_count;
      float offset = (tot_lines - disp_lines - vt->scroll) / tot_lines;
      float win_len = disp_lines / tot_lines;

#if 0
      ctx_rectangle (ctx, (vt->cols *vt->cw), 0, 
                       (vt->width) - (vt->cols * vt->cw),
                       vt->rows *  vt->ch);
      ctx_rgb (ctx,1,0,0);
      ctx_fill (ctx);
#endif

      ctx_rectangle (ctx, (vt->width) - vt->cw * 1.5,
                     0, 1.5 * vt->cw,
                     vt->rows * vt->ch);
      //ctx_listen (ctx, CTX_DRAG,  scrollbar_drag, vt, NULL);
      //ctx_listen (ctx, CTX_ENTER, scrollbar_enter, vt, NULL);
      //ctx_listen (ctx, CTX_LEAVE, scrollbar_leave, vt, NULL);
      if (vt->scroll != 0 || scrollbar_focused)
        ctx_rgba (ctx, 0.5, 0.5, 0.5, .25);
      else
        ctx_rgba (ctx, 0.5, 0.5, 0.5, .10);
      ctx_fill (ctx);
      ctx_round_rectangle (ctx, (vt->width) - vt->cw * 1.5,
                           offset * vt->rows * vt->ch, (1.5-0.2) * vt->cw,
                           win_len * vt->rows * vt->ch,
                           vt->cw * 1.5 /2);
      //ctx_listen (ctx, CTX_DRAG, scroll_handle_drag, vt, NULL);
      if (vt->scroll != 0 || scrollbar_focused)
        ctx_rgba (ctx, 1, 1, 1, .25);
      else
        ctx_rgba (ctx, 1, 1, 1, .10);
      ctx_fill (ctx);
    }

  if (getenv ("CTX_STAR_WARS"))
  ctx_apply_transform (ctx, 0.3120, -0.666, 700.,
                            0.0000, 0.015,  200.0,
                            0.00, -0.0007, 1.0);
    ctx_rectangle (ctx, 0, 0, vt->cols * vt->cw, vt->rows * vt->ch);
    ctx_listen (ctx, CTX_DRAG,   vt_mouse_event, vt, NULL);
    ctx_listen (ctx, CTX_MOTION, vt_mouse_event, vt, NULL);
    ctx_begin_path (ctx);

    ctx_restore (ctx);

    if (vt->popped)
    {
       //ctx_set_popup (ctx, test_popup, vt);
    }
}


int vt_is_done (VT *vt)
{
  return vt->vtpty.done;
}

int vt_get_result (VT *vt)
{
  /* we could block - at least for a while, here..? */
  return vt->result;
}

void vt_set_scrollback_lines (VT *vt, int scrollback_lines)
{
  vt->scrollback_limit = scrollback_lines;
}

int  vt_get_scrollback_lines (VT *vt)
{
  return vt->scrollback_limit;
}

void vt_set_scroll (VT *vt, int scroll)
{
  if (vt->scroll == scroll)
    return;
  vt->scroll = scroll;
  if (vt->scroll > ctx_list_length (vt->scrollback) )
    { vt->scroll = ctx_list_length (vt->scrollback); }
  if (vt->scroll < 0)
    { vt->scroll = 0; }
}

int vt_get_scroll (VT *vt)
{
  return vt->scroll;
}

char *
vt_get_selection (VT *vt)
{
  CtxString *str = ctx_string_new ("");
  char *ret;
  for (int row = vt->select_start_row; row <= vt->select_end_row; row++)
    {
      const char *line_str = vt_get_line (vt, vt->rows - row);
      int col = 1;
      for (const char *c = line_str; *c; c = mrg_utf8_skip (c, 1), col ++)
        {
          if (row == vt->select_end_row && col > vt->select_end_col)
            { continue; }
          if (row == vt->select_start_row && col < vt->select_start_col)
            { continue; }
          ctx_string_append_utf8char (str, c);
        }
      if (row < vt->select_end_row && !vt_line_is_continuation (vt, vt->rows-row-1))
      {
        _ctx_string_append_byte (str, '\n');
      }
    }
  ret = str->str;
  ctx_string_free (str, 0);
  return ret;
}

int vt_get_local (VT *vt)
{
  return vt->local_editing;
}

void vt_set_local (VT *vt, int local)
{
  vt->local_editing = local;
}

static unsigned long prev_press_time = 0;
static int short_count = 0;


void terminal_set_primary (const char *text)
{
  if (primary) free (primary);
  primary = NULL;
  if (text) primary = strdup (text);
}

void terminal_long_tap (Ctx *ctx, VT *vt);
static int long_tap_cb_id = 0;
static int single_tap (Ctx *ctx, void *data)
{
#if 0 // XXX
  VT *vt = data;
  if (short_count == 0 && !vt->select_active)
    terminal_long_tap (ctx, vt);
#endif
  return 0;
}

void vt_mouse (VT *vt, CtxEvent *event, VtMouseEvent type, int button, int x, int y, int px_x, int px_y)
{
 char buf[64]="";
 int button_state = 0;
 ctx_client_rev_inc (vt->client);
 ctx_ticks();
 if ((! (vt->mouse | vt->mouse_all | vt->mouse_drag)) ||
     (event && (event->state & CTX_MODIFIER_STATE_SHIFT)))
   {
     // regular mouse select, this is incomplete
     // fully ignorant of scrollback for now
     //
     if (type == VT_MOUSE_PRESS)
       {
         vt->cursor_down = 1;
         vt->select_begin_col = x;
         vt->select_begin_row = y - (int)vt->scroll;
         vt->select_start_col = x;
         vt->select_start_row = y - (int)vt->scroll;
         vt->select_end_col = x;
         vt->select_end_row = y - (int)vt->scroll;
         vt->select_active = 0;
         if (long_tap_cb_id)
           {
             ctx_remove_idle (vt->root_ctx, long_tap_cb_id);
             long_tap_cb_id = 0;
           }
         
         if ((ctx_ticks () - prev_press_time) < 1000*300 &&
             abs(px_x - vt->select_begin_x) + 
             abs(px_y - vt->select_begin_y) < 8)
         {
           short_count++;
           switch (short_count)
           {
             case 1:
             {
               /* extend selection until space, XXX  should handle utf8 instead of ascii here!  */

               int hit_space = 0;
           
               while (vt->select_start_col > 1 && !hit_space)
               {
                 vt->select_start_col --;
                 char *sel = vt_get_selection (vt);
                 if (sel[0] == ' ' || sel[0] == '\0')
                   hit_space = 1;
                 free (sel);
               }
               if (hit_space)
                 vt->select_start_col++;

               hit_space = 0;
               while ((hit_space == 0) &&
                      (vt->select_end_col < vt->cols))
               {
                 vt->select_end_col ++;
                 char *sel = vt_get_selection (vt);
                 int len = strlen(sel);
                 if (sel[len-1]==' ')
                   hit_space = 1;
                 free (sel);
               }
               if (hit_space)
                 vt->select_end_col--;

               vt->select_active = 1;

               { char *sel = vt_get_selection (vt);
                 if (sel)
                 {
                    terminal_set_primary (sel);
                    free (sel);
                 }
               }
               }
               break;
             case 2:
               vt->select_start_col = 1;
               vt->select_end_col = vt->cols;
               vt->select_active = 1;
               {
                 char *sel = vt_get_selection (vt);
                 if (sel){
                    terminal_set_primary (sel);
                    free (sel);
                 }
               }
               break;
             case 3:
               short_count = 0;
               vt->select_start_col = 
               vt->select_end_col = vt->select_begin_col;
               vt->select_active = 0;
               terminal_set_primary ("");
               break;
           }
         }
         else
         {
           if (vt->root_ctx && short_count == 0)
             long_tap_cb_id = ctx_add_timeout (vt->root_ctx, 1000, single_tap, vt);
           short_count = 0;
           //vt->select_start_col = 
           //vt->select_end_col = vt->select_begin_col;
         }
         vt->select_begin_x = px_x;
         vt->select_begin_y = px_y;
         prev_press_time = ctx_ticks ();
         ctx_client_rev_inc (vt->client);
       }
     else if (type == VT_MOUSE_RELEASE)
       {
         if (long_tap_cb_id)
           {
             ctx_remove_idle (vt->root_ctx, long_tap_cb_id);
             long_tap_cb_id = 0;
           }
         vt->cursor_down = 0;
       }
     else if (type == VT_MOUSE_MOTION && vt->cursor_down)
       {
         int row = y - (int)vt->scroll;
         int col = x;
         if ((row > vt->select_begin_row) ||
             ((row == vt->select_begin_row) && (col >= vt->select_begin_col)))
         {
           vt->select_start_col = vt->select_begin_col;
           vt->select_start_row = vt->select_begin_row;
           vt->select_end_col = col;
           vt->select_end_row = row;
         }
         else
         {
           vt->select_start_col = col;
           vt->select_start_row = row;
           vt->select_end_col = vt->select_begin_col;
           vt->select_end_row = vt->select_begin_row;
         }
         if (vt->select_end_row == vt->select_start_row &&
             abs (vt->select_begin_x - px_x) < vt->cw/2)
         {
           vt->select_active = 0;
         }
         else
         {
           vt->select_active = 1;
           char *selection = vt_get_selection (vt);
           if (selection)
           {
             terminal_set_primary (selection);
             free (selection);
           }
         }

         if (y < 1)
         {
           vt->scroll += 1.0f;
           if (vt->scroll > vt->scrollback_count)
             vt->scroll = vt->scrollback_count;
         }
         else if (y > vt->rows)
         {
           vt->scroll -= 1.0f;
           if (vt->scroll < 0)
             vt->scroll = 0.0f;
         }

         ctx_client_rev_inc (vt->client);
       }
     return;
   }
 if (type == VT_MOUSE_MOTION)
   { button_state = 3; }

 if (vt->unit_pixels && vt->mouse_decimal)
   {
     x = px_x;
     y = px_y;
   }
 switch (type)
   {
     case VT_MOUSE_MOTION:
       if (!vt->mouse_all)
         return;
       if (x==vt->lastx && y==vt->lasty)
         return;
       vt->lastx = x;
       vt->lasty = y;
   //  sprintf (buf, "\033[<35;%i;%iM", x, y);
       break;
     case VT_MOUSE_RELEASE:
       if (vt->mouse_decimal == 0)
         button_state = 3;
       break;
     case VT_MOUSE_PRESS:
       button_state = 0;
       break;
     case VT_MOUSE_DRAG: // XXX not really used - remove
       if (! (vt->mouse_all || vt->mouse_drag) )
         return;
       button_state = 32;
       break;
   }
 // todo : mix in ctrl/meta state
 if (vt->mouse_decimal)
   {
     sprintf (buf, "\033[<%i;%i;%i%c", button_state, x, y, type == VT_MOUSE_RELEASE?'m':'M');
   }
 else
   { 
     sprintf (buf, "\033[M%c%c%c", button_state + 32, x + 32, y + 32);
   }
 if (buf[0])
   {
     vt_write (vt, buf, strlen (buf) );
     fflush (NULL);
   }
}

pid_t vt_get_pid (VT *vt)
{
  return vt->vtpty.pid;
}

void vt_set_ctx (VT *vt, Ctx *ctx)
{
  vt->root_ctx = ctx;
}
