/* vt2020 - xterm and DEC terminal, with ANSI, utf8, graphics and audio
 *
 * Copyright (c) 2014, 2016, 2018, 2020 Øyvind Kolås <pippin@gimp.org>
 *
 * Adhering to the standards with modern extensions.
 *
 * Features:
 *     vt100 - 101 points on scoresheet
 *     UTF8, cp437
 *     dim, bold, strikethrough, underline, italic, reverse
 *     ANSI colors, 256 colors (non-redfinable), 24bit color
 *     vector graphics
 *     raster sprites (kitty spec)
 *     vt320 - horizontal margins
 *     wordwrap (extension)
 *     proportional fonts
 *     BBS/ANSI-art mode
 *     audio playback
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

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
#include <zlib.h>

#include <pty.h>

#include "ctx.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vt-line.h"
#include "vt.h"

#define VT_LOG_INFO     (1<<0)
#define VT_LOG_CURSOR   (1<<1)
#define VT_LOG_COMMAND  (1<<2)
#define VT_LOG_WARNING  (1<<3)
#define VT_LOG_ERROR    (1<<4)
#define VT_LOG_INPUT    (1<<5)
#define VT_LOG_ALL       0xff

//static int vt_log_mask = VT_LOG_INPUT;
static int vt_log_mask = VT_LOG_WARNING | VT_LOG_ERROR | VT_LOG_INFO;
//static int vt_log_mask = VT_LOG_WARNING | VT_LOG_ERROR | VT_LOG_INFO | VT_LOG_COMMAND | VT_LOG_INPUT;
//static int vt_log_mask = VT_LOG_ALL;

#if 0
  #define vt_log(domain, fmt, ...)

  #define VT_input(str, a...)
  #define VT_info(str, a...)
  #define VT_command(str, a...)
  #define VT_cursor(str, a...)
  #define VT_warning(str, a...)
  #define VT_error(str, a...)
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

#include "mmm.h"

/* barebones linked list */

typedef struct _VtList VtList;
struct _VtList {
  void *data;
  VtList *next;
};

static inline int vt_list_length (VtList *list)
{
  int length = 0;
  for (VtList *l = list; l; l = l->next, length++);
  return length;
}

static inline void vt_list_prepend (VtList **list, void *data)
{
  VtList *new_=calloc (sizeof (VtList), 1);
  new_->next = *list;
  new_->data = data;
  *list = new_;
}

static inline void *vt_list_last (VtList *list)
{
  if (list)
    {
      VtList *last;
      for (last = list; last->next; last=last->next);
      return last->data;
    }
  return NULL;
}

static inline void vt_list_append (VtList **list, void *data)
{
  VtList *new_= calloc (sizeof (VtList), 1);
  new_->data=data;
  if (*list)
    {
      VtList *last;
      for (last = *list; last->next; last=last->next);
      last->next = new_;
      return;
    }
  *list = new_;
  return;
}

static inline void vt_list_remove (VtList **list, void *data)
{
  VtList *iter, *prev = NULL;
  if ((*list)->data == data)
    {
      prev = (void*)(*list)->next;
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
      prev = iter;
}

static inline VtList *vt_list_nth (VtList *list, int no)
{
  while(no-- && list)
    list = list->next;
  return list;
}

static inline void
vt_list_insert_before (VtList **list, VtList *sibling,
                        void *data)
{
  if (*list == NULL || *list == sibling)
    {
      vt_list_prepend (list, data);
    }
  else
    {
      VtList *prev = NULL;
      for (VtList *l = *list; l; l=l->next)
        {
          if (l == sibling)
            break;
          prev = l;
        }
      if (prev) {
        VtList *new_=calloc(sizeof (VtList), 1);
        new_->next = sibling;
        new_->data = data;
        prev->next=new_;
      }
    }
}

typedef enum {
  TERMINAL_STATE_NEUTRAL          = 0,
  TERMINAL_STATE_GOT_ESC          = 1,
  TERMINAL_STATE_GOT_ESC_SQRPAREN = 2,
  TERMINAL_STATE_GOT_ESC_SEQUENCE = 3,
  TERMINAL_STATE_GOT_ESC_FOO      = 4,
  TERMINAL_STATE_SWALLOW          = 5,
} TerminalState;

#define MAX_COLS 2048 // used for tabstops

typedef enum {
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

typedef struct VtPty {
  int        pty;
  pid_t      pid;
} VtPty;

typedef struct Image
{
  int kitty_format;
  int width;
  int height;
  int id;
  int size;
  uint8_t *data;
} Image;

#define MAX_IMAGES 32

static Image image_db[MAX_IMAGES]={{0,},};

static Image *image_query (int id)
{
  for (int i = 0; i < MAX_IMAGES; i++)
  {
    Image *image = &image_db[i];
    if (image->id == id)
      return image;
  }
  return NULL;
}

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
      break;
  }

  if (image->data)
  {
     image = &image_db[random()%MAX_IMAGES];
  }

  if (image->data)
    free (image->data);

  image->kitty_format = format;
  image->width  = width;
  image->height = height;
  image->id     = id;
  image->size   = size;
  image->data   = data;

  return image;
}

typedef struct GfxState {
  int action;
  int id;
  int buf_width;
  int buf_height;
  int format;
  int compression;
  int transmission;
  int multichunk;
  int buf_size;
  int x;
  int y;
  int w;
  int h;
  int x_cell_offset;
  int y_cell_offset;
  int columns;
  int rows;
  int z_index;
  int delete;

  uint8_t *data;
  int   data_size;
} GfxState;

struct _MrgVT {
  char     *title;
  VtList   *lines;
  int       line_count;
  VtList   *scrollback;
  int       scrollback_count;
  int       leds[4];
  uint64_t  cstyle;

  uint8_t   fg_color[3];
  uint8_t   bg_color[3];

  uint64_t *set_style;
  uint32_t *set_unichar;
  int       in_scroll;
  int       smooth_scroll;
  float     scroll_offset;
  int       debug;
  int       bell;
  int       origin;
  int       at_line_home;
  int       charset[4];
  int       saved_charset[4];
  int       shifted_in;
  int       reverse_video;
  int       echo;
  int       bracket_paste;
  int       font_is_mono;
  int       palette_no;
  int       has_blink; // if any of the set characters are blinking
                       // updated on each draw of the screen

  TerminalState state;

  int unit_pixels;
  int mouse;
  int mouse_drag;
  int mouse_all;
  int mouse_decimal;
  int keyrepeat;

  long       rev;
  uint8_t    utf8_holding[64]; /* only 4 needed for utf8 - but it's purpose
                                 is also overloaded for ctx journal command
                                 buffering , and the bigger sizes for the ascii
				 ctx mode */

  int        encoding;  // 0 = utf8 1=pc vga 2=ascii

  int        insert_mode;
  int        autowrap;
  int        wordwrap;
  int        utf8_expected_bytes;
  int        utf8_pos;
  float      cursor_x;
  int        cursor_y;
  int        cols;
  int        rows;
  VtString *current_line;


  int        cr_on_lf;
  int        cursor_visible;
  int        saved_x;
  int        saved_y;
  uint32_t   saved_style;
  int        saved_origin;
  int        cursor_key_application;
  int        margin_top;
  int        margin_bottom;
  int        margin_left;
  int        margin_right;

  int        left_right_margin_mode;

  int        scrollback_limit;
  float      scroll;

#define MAX_ARGUMENT_BUF_LEN (8192 + 16)

  char       argument_buf[MAX_ARGUMENT_BUF_LEN];
  uint8_t    tabs[MAX_COLS];
  int        argument_buf_len;
  int        inert;

  int        done;
  int        result;

  int        in_vt52;
  int        in_ctx;
  int        in_pcm;
  int        in_ctx_ascii;

  float      font_to_cell_scale;
  float      font_size; // should maybe be integer?
  float      line_spacing; // char-aspect might be better variable to use..?
  int        cw; // cell width
  int        ch; // cell height
  float      scale_x;
  float      scale_y;

  int        ctx_pos;  // 1 is graphics above text, 0 or -1 is below text
  Ctx       *ctx;
  void      *mmm;

  int        blink_state;

  FILE      *log;
  ssize_t(*write) (void *serial_obj, const void *buf, size_t count);
  ssize_t(*read)  (void *serial_obj, void *buf, size_t count);
  int    (*waitdata)(void *serial_obj, int timeout);
  void  (*resize)(void *serial_obj, int cols, int rows, int px_width, int px_height);

  VtPty   vtpty;

  GfxState gfx;
};

/* on current line */
static int vt_col_to_pos (MrgVT *vt, int col)
{
  int pos = col;

  if (vt->current_line->contains_proportional)
  {
    Ctx *ctx = ctx_new ();
    ctx_set_font (ctx, "regular");
    ctx_set_font_size (ctx, vt->font_size);
    int x = 0;
    pos = 0;
    int prev_prop = 0;

    while (x <= col * vt->cw)
    {
      if (vt_string_get_style (vt->current_line, pos) & STYLE_PROPORTIONAL)
      {
	x += ctx_glyph_width (ctx, vt_string_get_unichar (vt->current_line, pos));
	prev_prop = 1;
      }
      else
      {
	if (prev_prop)
	{
	  int new_cw = vt->cw - ((x % vt->cw));
	  if (new_cw < vt->cw*3/2)
	    new_cw += vt->cw;
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

static int vt_margin_left (MrgVT *vt)
{
  int left = vt->left_right_margin_mode?vt->margin_left:1;
  return vt_col_to_pos (vt, left);
}

#define VT_MARGIN_LEFT vt_margin_left(vt)

static int vt_margin_right (MrgVT *vt)
{
  int right = vt->left_right_margin_mode?vt->margin_right:vt->cols;
  return vt_col_to_pos (vt, right);
}

#define VT_MARGIN_RIGHT vt_margin_right(vt)

static ssize_t vt_write (MrgVT *vt, const void *buf, size_t count)
{
  if (!vt->write) return 0;
  return vt->write (&vt->vtpty, buf, count);
}
static ssize_t vt_read (MrgVT *vt, void *buf, size_t count)
{
  if (!vt->read) return 0;
  return vt->read (&vt->vtpty, buf, count);
}
static int vt_waitdata (MrgVT *vt, int timeout)
{
  if (!vt->waitdata) return 0;
  return vt->waitdata (&vt->vtpty, timeout);
}
static void vt_resize (MrgVT *vt, int cols, int rows, int px_width, int px_height)
{
  if (vt->resize)
   vt->resize (&vt->vtpty, cols, rows, px_width, px_height);
}

void vtpty_resize (void *data, int cols, int rows, int px_width, int px_height)
{
  VtPty *vtpty = data;
  struct winsize ws;

  ws.ws_row = rows;
  ws.ws_col = cols;
  ws.ws_xpixel = px_width;
  ws.ws_ypixel = px_height;
  ioctl(vtpty->pty, TIOCSWINSZ, &ws);
}

static ssize_t vtpty_write (void *data, const void *buf, size_t count)
{
  VtPty *vtpty = data;
  return write (vtpty->pty, buf, count);
}

static ssize_t vtpty_read (void  *data, void *buf, size_t count)
{
  VtPty *vtpty = data;
  return read (vtpty->pty, buf, count);
}

static int vtpty_waitdata (void  *data, int timeout)
{
  VtPty *vtpty = data;
  struct timeval tv;
  fd_set fdset;
  FD_ZERO (&fdset);
  FD_SET(vtpty->pty, &fdset);

  tv.tv_sec = 0;
  tv.tv_usec = timeout;
  tv.tv_sec  = timeout / 1000000;
  tv.tv_usec = timeout % 1000000;

  if (select (vtpty->pty+1, &fdset, NULL, NULL, &tv) == -1){
    perror("select");
    return 0;
  }
  if (FD_ISSET(vtpty->pty, &fdset))
  {
    return 1;
  }
  return 0;
}


void ctx_vt_rev_inc (MrgVT *vt)
{
  vt->rev++;
}

long ctx_vt_rev (MrgVT *vt)
{
  return vt->rev;
}

static void vtcmd_reset_to_initial_state (MrgVT *vt, const char *sequence);

static void ctx_vt_set_title (MrgVT *vt, const char *new_title)
{
  if (vt->inert) return;
  if (vt->title)
    free (vt->title);
  vt->title = strdup (new_title);

#if 1
  if (vt->mmm)
     mmm_set_title (vt->mmm, new_title);
#endif
}

const char *ctx_vt_get_title (MrgVT *vt)
{
  return vt->title;
}

static VtList *vts = NULL;

static void ctx_vt_run_command (MrgVT *vt, const char *command);
static void vtcmd_set_top_and_bottom_margins (MrgVT *vt, const char *sequence);
static void vtcmd_set_left_and_right_margins (MrgVT *vt, const char *sequence);
static void _ctx_vt_move_to (MrgVT *vt, int y, int x);

static void vt_cell_cache_reset(MrgVT *vt, int row, int col)
{
  if (row < 0 || col < 0 || row > vt->rows || col > vt->cols)
    return;
  vt->set_unichar[row*vt->cols*2+col] = 0xfffff;
  vt->set_style[row*vt->cols*2+col] = 0xffffff;
}

static void vt_cell_cache_clear_row (MrgVT *vt, int row)
{
  if (!vt->set_style)
    return;
  for (int col = 0; col <= vt->cols; col++)
    vt_cell_cache_reset (vt, row, col);
}

static void vt_cell_cache_clear (MrgVT *vt)
{
  if (!vt->set_style)
    return;
  vt->rev++;
  for (int row = 0; row <= vt->rows; row++)
    vt_cell_cache_clear_row (vt, row);
}

static void vtcmd_clear (MrgVT *vt, const char *sequence)
{
  while (vt->lines)
  {
    vt_string_free (vt->lines->data, 1);
    vt_list_remove (&vt->lines, vt->lines->data);
  }

  vt->lines = NULL;
  vt->line_count = 0;

  /* populate lines */
  for (int i=0; i<vt->rows;i++)
  {
    vt->current_line = vt_string_new_with_size ("", vt->cols);
    vt_list_prepend (&vt->lines, vt->current_line);
    vt->line_count++;
  }

  vt_cell_cache_clear (vt); // should not be needed
}

#define set_fg_rgb(r, g, b) \
    vt->cstyle ^= (vt->cstyle & (((1l<<24)-1)<<16));\
    vt->cstyle |=  ((uint64_t)(r)<<16);\
    vt->cstyle |=  ((uint64_t)(g)<<(16+8));\
    vt->cstyle |=  ((uint64_t)(b)<<(16+8+8));\
    vt->cstyle |= STYLE_FG_COLOR_SET;\
    vt->cstyle |= STYLE_FG24_COLOR_SET;\

#define set_bg_rgb(r, g, b) \
    vt->cstyle ^= (vt->cstyle & (((1l<<24)-1)<<40));\
    vt->cstyle |=  ((uint64_t)(r)<<40);\
    vt->cstyle |=  ((uint64_t)(g)<<(40+8));\
    vt->cstyle |=  ((uint64_t)(b)<<(40+8+8));\
    vt->cstyle |= STYLE_BG_COLOR_SET;\
    vt->cstyle |= STYLE_BG24_COLOR_SET;\

#define set_fg_idx(idx) \
    vt->cstyle ^= (vt->cstyle & (((1l<<24)-1)<<16));\
    vt->cstyle ^= (vt->cstyle & STYLE_FG24_COLOR_SET);\
    vt->cstyle |=  ((idx)<<16);\
    vt->cstyle |= STYLE_FG_COLOR_SET;

#define set_bg_idx(idx) \
    vt->cstyle ^= (vt->cstyle & (((1l<<24)-1)<<40));\
    vt->cstyle ^= (vt->cstyle & STYLE_BG24_COLOR_SET);\
    vt->cstyle |= ((int64_t)(idx)<<40) ;\
    vt->cstyle |= STYLE_BG_COLOR_SET;


static void _ctx_vt_compute_cw_ch (MrgVT *vt)
{
  vt->cw = (vt->font_size / vt->line_spacing * vt->scale_x) + 0.99;
  vt->ch = vt->font_size;
}

static void vtcmd_set_132_col (MrgVT  *vt, int set)
{
  // this should probably force the window as well

  if (set == 0 && vt->scale_x == 1.0f) return;
  if (set == 1 && vt->scale_x != 1.0f) return;

  if (set) // 132 col
  {
    vt->scale_x = 80.0/132.0;
    vt->scale_y = 1.0;
    _ctx_vt_compute_cw_ch (vt);
    ctx_vt_set_term_size (vt, vt->cols * 132/80.0, vt->rows);
  }
  else // 80 col
  {
    vt->scale_x = 1.0;
    vt->scale_y = 1.0;
    _ctx_vt_compute_cw_ch (vt);
    ctx_vt_set_term_size (vt, vt->cols * 80/132.0, vt->rows);
  }
}

static void ctx_vt_line_feed (MrgVT *vt);
static void ctx_vt_carriage_return (MrgVT *vt);

static void vtcmd_reset_to_initial_state (MrgVT *vt, const char *sequence)
{
  VT_info ("reset %s", sequence);
  if (getenv ("VT_DEBUG"))
    vt->debug = 1;
  vtcmd_clear (vt, sequence);
  vt->encoding = 0;
  vt->bracket_paste = 0;
  vt->cr_on_lf = 0;
  vtcmd_set_top_and_bottom_margins (vt, "[r");
  vtcmd_set_left_and_right_margins (vt, "[s");
  vt->autowrap       = 1;
  vt->wordwrap       = 0;
  vt->cursor_visible = 1;
  vt->charset[0] = 0;
  vt->charset[1] = 0;
  vt->charset[2] = 0;
  vt->charset[3] = 0;
  vt->bell = 3;
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
  vt->done                   = 0;
  vt->result                 = -1;
  vt->state                  = TERMINAL_STATE_NEUTRAL,

  vt->unit_pixels   = 0;
  vt->mouse         = 0;
  vt->mouse_drag    = 0;
  vt->mouse_all     = 0;
  vt->mouse_decimal = 0;

  _ctx_vt_compute_cw_ch (vt);

  for (int i = 0; i < MAX_COLS; i++)
    vt->tabs[i] = i % 8 == 0? 1 : 0;

  _ctx_vt_move_to (vt, vt->margin_top, vt->cursor_x);
  ctx_vt_carriage_return (vt);

  if (vt->ctx)
    ctx_clear (vt->ctx);
}

void ctx_vt_set_font_size (MrgVT *vt, float font_size)
{
  vt->font_size = font_size;
  _ctx_vt_compute_cw_ch (vt);
  vt_cell_cache_clear (vt);
}

void ctx_vt_set_line_spacing (MrgVT *vt, float line_spacing)
{
  vt->line_spacing = line_spacing;
  _ctx_vt_compute_cw_ch (vt);
  vt_cell_cache_clear (vt);
}

MrgVT *ctx_vt_new (const char *command, int cols, int rows, float font_size, float line_spacing)
{
  MrgVT *vt              = calloc (sizeof (MrgVT), 1);
  vt->smooth_scroll = 0;
  vt->scroll_offset = 0.0;
  vt->waitdata = vtpty_waitdata;
  vt->read   = vtpty_read;
  vt->write  = vtpty_write;
  vt->resize = vtpty_resize;

  vt->font_to_cell_scale = 1.0;
  vt->cursor_visible     = 1;
  vt->lines              = NULL;
  vt->line_count         = 0;
  vt->current_line       = NULL;
  vt->cols               = 0;
  vt->rows               = 0;
  vt->scrollback_limit   = DEFAULT_SCROLLBACK;

  vt->argument_buf_len   = 0;
  vt->argument_buf[0]    = 0;
  vt->done               = 0;
  vt->result             = -1;
  vt->state              = TERMINAL_STATE_NEUTRAL,
  vt->line_spacing       = 1.0;
  vt->scale_x            = 1.0;
  vt->scale_y            = 1.0;

  ctx_vt_set_font_size (vt, font_size);
  ctx_vt_set_line_spacing (vt, line_spacing);

  if (command)
  {
    ctx_vt_run_command (vt, command);
  }

  if (cols <= 0) cols = DEFAULT_COLS;
  if (rows <= 0) cols = DEFAULT_ROWS;

  ctx_vt_set_term_size (vt, cols, rows);

  vt->fg_color[0] = 255;
  vt->fg_color[1] = 255;
  vt->fg_color[2] = 255;

  vt->bg_color[0] = 0;
  vt->bg_color[1] = 0;
  vt->bg_color[2] = 0;

  vtcmd_reset_to_initial_state (vt, NULL);

  //vt->ctx = ctx_new ();

  vt_list_prepend (&vts, vt);

  return vt;
}

int ctx_vt_cw (MrgVT *vt)
{
  return vt->cw;
}

int ctx_vt_ch (MrgVT *vt)
{
  return vt->ch;
}

void ctx_vt_set_mmm (MrgVT *vt, void *mmm)
{
  vt->mmm = mmm;
}

static int ctx_vt_trimlines (MrgVT *vt, int max)
{
  VtList *chop_point = NULL;
  VtList *l;
  int i;

  if (vt->line_count < max)
    return 0;

  for (l = vt->lines, i = 0; l && i < max-1; l = l->next, i++);

  if (l)
  {
    chop_point = l->next;
    l->next = NULL;
  }

  while (chop_point)
  {
    vt_list_prepend (&vt->scrollback, chop_point->data);
    vt->scrollback_count ++;
    vt_list_remove (&chop_point, chop_point->data);
    vt->line_count--;
  }

  if (vt->scrollback_count > vt->scrollback_limit + 128)
  {
    VtList *l = vt->scrollback;
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
      vt_string_free (chop_point->data, 1);
      vt_list_remove (&chop_point, chop_point->data);
      vt->scrollback_count --;
    }
  }

  return 0;
}


void ctx_vt_set_term_size (MrgVT *vt, int icols, int irows)
{
  vt_cell_cache_clear (vt);
  if (vt->rows == irows && vt->cols == icols)
    return;

  while (irows > vt->rows)
  {
    if (vt->scrollback_count)
    {
       vt->scrollback_count--;
       vt_list_append (&vt->lines, vt->scrollback->data);
       vt_list_remove (&vt->scrollback, vt->scrollback->data);
    }
    else
    {
       vt_list_append (&vt->lines, vt_string_new_with_size ("", vt->cols));
    }
    vt->cursor_y++;
    vt->rows++;
  }
  while (irows < vt->rows)
  {
    vt->cursor_y--;
    vt->rows--;
  }

  vt->rows = irows;
  vt->cols = icols;
  vt_resize (vt, vt->cols, vt->rows, vt->cols * vt->cw, vt->rows * vt->ch);

  ctx_vt_trimlines (vt, vt->rows);

  vt->margin_top     = 1;
  vt->margin_left    = 1;
  vt->margin_bottom  = vt->rows;
  vt->margin_right   = vt->cols;
  vt->rev++;

  if (vt->set_style)
    free (vt->set_style);
  if (vt->set_unichar)
    free (vt->set_unichar);
  vt->set_style = malloc (((1+vt->rows) * (1+vt->cols*2)) * sizeof (uint64_t));
  vt->set_unichar = malloc (((1+vt->rows) * (1+vt->cols*2)) * sizeof (uint32_t));


  VT_info ("resize %i %i", irows, icols);
}



static void ctx_vt_argument_buf_reset (MrgVT *vt, const char *start)
{
  if (start)
  {
    strcpy (vt->argument_buf, start);
    vt->argument_buf_len = strlen (start);
  }
  else
    vt->argument_buf[vt->argument_buf_len=0]=0;
}

static inline void ctx_vt_argument_buf_add (MrgVT *vt, int ch)
{
  if (vt->argument_buf_len < MAX_ARGUMENT_BUF_LEN-1)
  {
    vt->argument_buf[vt->argument_buf_len] = ch;
    vt->argument_buf[++vt->argument_buf_len] = 0;
  }
}

static void
_ctx_vt_move_to (MrgVT *vt, int y, int x)
{
  int i;
  x = x < 1 ? 1 : (x > vt->cols ? vt->cols : x);
  y = y < 1 ? 1 : (y > vt->rows ? vt->rows : y);

  vt->at_line_home = 0;
  vt->cursor_x = x;
  vt->cursor_y = y;

  i = vt->rows - y;

  VtList *l;
  for (l = vt->lines; l && i >= 1; l = l->next, i--);
  if (l)
  {
    vt->current_line = l->data;
  }
  else
  {
    for (; i > 0; i--)
      {
        vt->current_line = vt_string_new_with_size ("", vt->cols);
        vt_list_append (&vt->lines, vt->current_line);
        vt->line_count++;
      }
  }
  VT_cursor("%i,%i (_ctx_vt_move_to)", y, x);
}


static void vt_scroll (MrgVT *vt, int amount);

static void _ctx_vt_add_str (MrgVT *vt, const char *str)
{
  int logical_margin_right = VT_MARGIN_RIGHT;

  if (vt->cstyle & STYLE_PROPORTIONAL)
   vt->current_line->contains_proportional = 1;

  if (vt->cursor_x > logical_margin_right)
  {
    if (vt->autowrap) {
      int chars = 0;
      int old_x = vt->cursor_x;
      VtString *old_line = vt->current_line;

      if (vt->wordwrap && str[0] != ' ')
      {
        while (old_x-1-chars >1 && vt_string_get_unichar (vt->current_line,
			           old_x-1-chars)!=' ')
        {
	  chars++;
        }
        chars--;
	if (chars > (vt->margin_right - vt->margin_left) * 3 / 2)
	  chars = 0;
      }

      if (vt->cursor_y == vt->margin_bottom)
      {
        vt_scroll (vt, -1);
      }
      else
      {
        _ctx_vt_move_to (vt, vt->cursor_y+1, 1);
      }
      ctx_vt_carriage_return (vt);
      for (int i = 0; i < chars; i++)
      {
        vt_string_set_style (vt->current_line, vt->cursor_x-1, vt->cstyle);
        vt_string_replace_unichar (vt->current_line, vt->cursor_x - 1, 
	     vt_string_get_unichar (old_line, old_x-1-chars+i));
	vt->cursor_x++;
      }
      for (int i = 0; i < chars; i++)
      {
        vt_string_replace_unichar (old_line, old_x-1-chars+i, ' ');
      }
      if (str[0] == ' ')
	return;
    }
    else
    {
      vt->cursor_x = logical_margin_right;
    }
  }

  vt_string_set_style (vt->current_line, vt->cursor_x-1, vt->cstyle);
  if (vt->insert_mode)
   {
     vt_string_insert_utf8 (vt->current_line, vt->cursor_x - 1, str);
     while (vt->current_line->utf8_length > logical_margin_right)
        vt_string_remove_utf8 (vt->current_line, logical_margin_right);
   }
  else
   {
     vt_string_replace_utf8 (vt->current_line, vt->cursor_x - 1, str);
   }
  vt->cursor_x += 1;
  vt->at_line_home = 0;
}

static void _ctx_vt_backspace (MrgVT *vt)
{
  if (vt->current_line)
  {
    vt->cursor_x --;
    if (vt->cursor_x == VT_MARGIN_RIGHT) vt->cursor_x--;
    if (vt->cursor_x < VT_MARGIN_LEFT)
    {
      vt->cursor_x = VT_MARGIN_LEFT;
      vt->at_line_home = 1;
    }
    VT_cursor("backspace");
  }
}

static void vtcmd_set_top_and_bottom_margins (MrgVT *vt, const char *sequence)
{
  int top = 1, bottom = vt->rows;
  if (strlen (sequence) > 2)
  {
    sscanf (sequence, "[%i;%ir", &top, &bottom);
  }
  VT_info ("margins: %i %i", top, bottom);

  if (top <1) top = 1;
  if (top > vt->rows) top = vt->rows;
  if (bottom > vt->rows) bottom = vt->rows;
  if (bottom < top) bottom = top;

  vt->margin_top = top;
  vt->margin_bottom = bottom;
  _ctx_vt_move_to (vt, top, 1);
  ctx_vt_carriage_return (vt);
  VT_cursor ("%i, %i (home)", top, 1);
}
static void vtcmd_save_cursor_position (MrgVT *vt, const char *sequence);

static void vtcmd_set_left_and_right_margins (MrgVT *vt, const char *sequence)
{
  int left = 1, right = vt->cols;

  if (!vt->left_right_margin_mode)
  {
    vtcmd_save_cursor_position (vt, sequence); /* id:SCP Save Cursor Position */
    return;
  }

  if (strlen (sequence) > 2)
  {
    sscanf (sequence, "[%i;%is", &left, &right);
  }
  VT_info ("hor margins: %i %i", left, right);

  if (left <1) left = 1;
  if (left > vt->cols) left = vt->cols;
  if (right > vt->cols) right = vt->cols;
  if (right < left) right = left;

  vt->margin_left = left + 0;
  vt->margin_right = right - 0;
  _ctx_vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  ctx_vt_carriage_return (vt);
  //VT_cursor ("%i, %i (home)", left, 1);
}

static void vt_scroll (MrgVT *vt, int amount)
{
  int remove_no, insert_before;
  VtString *string = NULL;
  if (amount == 0) amount = 1;
  
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

  VtList *l;
  int i;

  for (i=vt->rows, l = vt->lines; i > 0 && l; l=l->next, i--)
  {
    if (i == remove_no)
    {
      string = l->data;
      vt_list_remove (&vt->lines, string);

      break;
    }
  }

  if (string)
  {
    if (vt->margin_top == 1 && vt->margin_bottom == vt->rows)
    {
      vt_list_prepend (&vt->scrollback, string);
      vt->scrollback_count ++;
    }
    else
    {
      vt_string_free (string, 1);
    }
  }

  string = vt_string_new_with_size ("", vt->cols/4);

  if (amount > 0 && vt->margin_top == 1)
  {
    vt_list_append (&vt->lines, string);
  }
  else
  {
    for (i=vt->rows, l = vt->lines; l; l=l->next, i--)
    {
      if (i == insert_before)
      {
        vt_list_insert_before (&vt->lines, l, string);
        break;
      }
    }

    if (i != insert_before)
    {
      vt_list_append (&vt->lines, string);
    }
  }

  vt->current_line = string;
  /* not updating line count since we should always remove one and add one */

  if (vt->smooth_scroll)
  {
    if (amount < 0)
    {
      vt->scroll_offset = -1.0;
      vt->in_scroll = -1;
    }
    else
    {
      vt->scroll_offset = 1.0;
      vt->in_scroll = 1;
    }
    vt_cell_cache_clear (vt);
  }
}

typedef struct Sequence {
  const char *prefix;
  char        suffix;
  void (*vtcmd) (MrgVT *vt, const char *sequence);
} Sequence;

static inline int parse_int (const char *arg, int def_val)
{
  if (!isdigit (arg[1]) || strlen (arg) == 2)
    return def_val;
  return atoi (arg+1);
}

static void vtcmd_cursor_position (MrgVT *vt, const char *sequence)
{
  int y = 1, x = 1;
  const char *semi;
  if (sequence[0] != 'H' && sequence[0] != 'f')
  {
    y = parse_int (sequence, 1);
    if ( (semi = strchr (sequence, ';')))
    {
      x = parse_int (semi, 1);
    }
  }
  if (x == 0) x = 1;
  if (y == 0) y = 1;

  if (vt->origin)
  {
    y += vt->margin_top - 1;
    _ctx_vt_move_to (vt, y, vt->cursor_x);
    x += VT_MARGIN_LEFT - 1;
  }

  VT_cursor("%i %i CUP", y, x);
  _ctx_vt_move_to (vt, y, x);
}


static void vtcmd_horizontal_position_absolute (MrgVT *vt, const char *sequence)
{
  int x = parse_int (sequence, 1);
  if (x<=0) x = 1;
  _ctx_vt_move_to (vt, vt->cursor_y, x);
}

static void vtcmd_goto_row (MrgVT *vt, const char *sequence)
{
  int y = parse_int (sequence, 1);
  _ctx_vt_move_to (vt, y, vt->cursor_x);
}

static void vtcmd_cursor_forward (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) n = 1;
  for (int i = 0; i < n; i++)
  {
    vt->cursor_x++;
  }
  if (vt->cursor_x > VT_MARGIN_RIGHT)
    vt->cursor_x = VT_MARGIN_RIGHT;
}

static void vtcmd_cursor_backward (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) n = 1;
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

static void vtcmd_reverse_index (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) n = 1;

  for (int i = 0; i < n; i++)
  {
    if (vt->cursor_y == vt->margin_top)
    {
      vt_scroll (vt, 1);
      _ctx_vt_move_to (vt, vt->margin_top, vt->cursor_x);
    }
    else
    {
      _ctx_vt_move_to (vt, vt->cursor_y-1, vt->cursor_x);
    }
  }
}

static void vtcmd_cursor_up (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) n = 1;

  for (int i = 0; i < n; i++)
  {
    if (vt->cursor_y == vt->margin_top)
    {
      //_ctx_vt_move_to (vt, 1, vt->cursor_x);
    }
    else
    {
      _ctx_vt_move_to (vt, vt->cursor_y-1, vt->cursor_x);
    }
  }
}

static void vtcmd_back_index (MrgVT *vt, const char *sequence)
{
	// XXX implement
}

static void vtcmd_forward_index (MrgVT *vt, const char *sequence)
{
	// XXX implement
}

static void vtcmd_index (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) n = 1;
  for (int i = 0; i < n; i++)
  {
    if (vt->cursor_y == vt->margin_bottom)
    {
      vt_scroll (vt, -1);
      _ctx_vt_move_to (vt, vt->margin_bottom, vt->cursor_x);
    }
    else
    {
      _ctx_vt_move_to (vt, vt->cursor_y + 1, vt->cursor_x);
    }
  }
}

static void vtcmd_cursor_down (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n==0) n = 1;
  for (int i = 0; i < n; i++)
  {
    if (vt->cursor_y >= vt->margin_bottom)
    {
      _ctx_vt_move_to (vt, vt->margin_bottom, vt->cursor_x);
    }
    else
    {
      _ctx_vt_move_to (vt, vt->cursor_y + 1, vt->cursor_x);
    }
  }
}

static void vtcmd_next_line (MrgVT *vt, const char *sequence)
{
  vtcmd_index (vt, sequence);
  _ctx_vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  ctx_vt_carriage_return (vt);
  vt->cursor_x = VT_MARGIN_LEFT;
}

static void vtcmd_cursor_preceding_line (MrgVT *vt, const char *sequence)
{
  vtcmd_cursor_up (vt, sequence);
  _ctx_vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  vt->cursor_x = VT_MARGIN_LEFT;
}

static void vtcmd_erase_in_line (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 0);

  switch (n)
  {
    case 0: // clear to end of line
      {
        char *p = (char*)mrg_utf8_skip (vt->current_line->str, vt->cursor_x-1);
        if (p) *p = 0;
	// XXX : this is chopping lines
	for (int col = vt->cursor_x; col <= VT_MARGIN_RIGHT; col++)
	  vt_string_set_style (vt->current_line, col - 1, vt->cstyle);
        vt->current_line->length = strlen (vt->current_line->str);
        vt->current_line->utf8_length = mrg_utf8_strlen (vt->current_line->str);
      }
      break;
    case 1: // clear from beginning to cursor
      {
	for (int col = VT_MARGIN_LEFT; col <= vt->cursor_x; col++)
        {
          vt_string_replace_utf8 (vt->current_line, col-1, " ");
        }

	for (int col = VT_MARGIN_LEFT; col <= vt->cursor_x; col++)
	  vt_string_set_style (vt->current_line, col-1, vt->cstyle);

        vt->current_line->length = strlen (vt->current_line->str);
        vt->current_line->utf8_length = mrg_utf8_strlen (vt->current_line->str); // should be a nop
      }
      break;
    case 2: // clear entire line
      for (int col = VT_MARGIN_LEFT; col <= VT_MARGIN_RIGHT; col++)
	  vt_string_set_style (vt->current_line, col-1, vt->cstyle);
      vt_string_set (vt->current_line, ""); // XXX not all
      break;
  }
}

static void vtcmd_erase_in_display (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 0);
  switch (n)
  {
    case 0: // clear to end of screen
      {
        char *p = (char*)mrg_utf8_skip (vt->current_line->str, vt->cursor_x-1);
        if (p) *p = 0;
        vt->current_line->length = strlen (vt->current_line->str);
        vt->current_line->utf8_length = mrg_utf8_strlen (vt->current_line->str);
      }
	for (int col = vt->cursor_x; col <= VT_MARGIN_RIGHT; col++)
	  vt_string_set_style (vt->current_line, col-1, vt->cstyle);

      {
        VtList *l;int no = vt->rows;
        for (l = vt->lines; l->data != vt->current_line; l = l->next, no--)
        {
          VtString *buf = l->data;
          buf->str[0] = 0;
          buf->length = 0;
          buf->utf8_length = 0;

          for (int col = 1; col <= vt->cols; col++)
	    vt_string_set_style (buf, col-1, vt->cstyle);
        }
      }
      break;
    case 1: // clear from beginning to cursor
      {
        for (int col = 1; col <= vt->cursor_x; col++)
        {
          vt_string_replace_utf8 (vt->current_line, col-1, " ");
	  vt_string_set_style (vt->current_line, col-1, vt->cstyle);
        }
      }
      {
        VtList *l;
        int there_yet = 0;
	int no = vt->rows;

        for (l = vt->lines; l; l = l->next, no--)
        {
          VtString *buf = l->data;
          if (there_yet)
          {
            buf->str[0] = 0;
            buf->length = 0;
            buf->utf8_length = 0;
            for (int col = 1; col <= vt->cols; col++)
	      vt_string_set_style (buf, col-1, vt->cstyle);
          }
          if (buf == vt->current_line)
          {
            there_yet = 1;
          }
        }
      }
      break;
    case 2: // clear entire screen but keep cursor;
      {
        int tx = vt->cursor_x;
        int ty = vt->cursor_y;
        vtcmd_clear (vt, "");
        _ctx_vt_move_to (vt, ty, tx);
        for (VtList *l = vt->lines; l; l = l->next)
	{
	  VtString *line = l->data;
          for (int col = 1; col <= vt->cols; col++)
	    vt_string_set_style (line, col-1, vt->cstyle);
	}
      }
      break;
  }
}

static void vtcmd_screen_alignment_display (MrgVT *vt, const char *sequence)
{
  for (int y = 1; y <= vt->rows; y++)
  {
    _ctx_vt_move_to (vt, y, 1);
    for (int x = 1; x <= vt->cols; x++)
      {
        _ctx_vt_add_str (vt, "E");
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

static void vtcmd_set_graphics_rendition (MrgVT *vt, const char *sequence)
{
  const char *s = sequence;
  if (s[0]) s++;
 
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
    /* SGR@38;5;n@\b256 color index foreground color@where n is 0-15 is system colors 16-(16+6*6*6) is a 6x6x6\n                RGB cube and in the end a grayscale without white and black.@ */
    if (n == 5)
    {
       s++;
       if (strchr (s, ';'))
         s = strchr (s, ';');
       else
         s = strchr (s, ':');
       if (s)
       n = parse_int (s, 0);
       set_fg_idx(n);
    }
    else if (n == 2)
    {
    /* SGR@38;2;50;70;180m@\b24 bit RGB foreground color@The example sets RGB the triplet 50 70 180@f@ */
      int r = 0, g = 0, b = 0;
      s++;
      if (strchr (s, ';'))
      {
        s = strchr (s, ';');
        if (s)
          sscanf (s, ";%i;%i;%i", &r, &g, &b);
      }
      else
      {
        s = strchr (s, ':');
        if (s)
          sscanf (s, ":%i:%i:%i", &r, &g, &b);
      }
      set_fg_rgb(r,g,b);
    }
    else
    {
      VT_warning ("unhandled %s %i", sequence, n);
      return;
    }
    return; // XXX we should continue, and allow further style set after complex color
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
    /* SGR@48;5;n@\b256 color index background color@where n is 0-15 is system colors 16-(16+6*6*6) is a 6x6x6\n                RGB cube and in the end a grayscale without white and black.@ */
    if (n == 5)
    {
       s++;
       if (strchr (s, ';'))
         s = strchr (s, ';');
       else
         s = strchr (s, ':');
       if (s)
       n = parse_int (s, 0);
       set_bg_idx(n);
    }
    /* SGR@48;2;50;70;180m@\b24 bit RGB background color@The example sets RGB the triplet 50 70 180@ */
    else if (n == 2)
    {
      int r = 0, g = 0, b = 0;
      s++;
      if (strchr (s, ';'))
      {
        s = strchr (s, ';');
        if (s)
          sscanf (s, ";%i;%i;%i", &r, &g, &b);
      }
      else
      {
        s = strchr (s, ':');
        if (s)
          sscanf (s, ":%i:%i:%i", &r, &g, &b);
      }
      set_bg_rgb(r,g,b);
    }
    else
    {
      VT_warning ("unhandled %s %i", sequence, n);
      return;
    }
    return; // we XXX should continue, and allow further style set after complex color
  }
  else
  switch (n)
  {
    case 0: /* SGR@0@Style reset@@ */
	if (vt->cstyle & STYLE_PROPORTIONAL)
	  vt->cstyle = STYLE_PROPORTIONAL;
	else
	  vt->cstyle = 0;
	break;
    case 1: /* SGR@@Bold@@ */         vt->cstyle |= STYLE_BOLD; break;
    case 2: /* SGR@@Dim@@ */          vt->cstyle |= STYLE_DIM; break; 
    case 3: /* SGR@@Rotalic@@ */      vt->cstyle |= STYLE_ITALIC; break; 
    case 4: /* SGR@@Underscore@@ */
        if (s[1] == ':')
	{
	  switch (s[2])
	  {
	     case '0': break;
	     case '1': vt->cstyle |= STYLE_UNDERLINE;
		       break;
	     case '2': vt->cstyle |= STYLE_UNDERLINE|
	                             STYLE_UNDERLINE_VAR;
		       break;
             default:
	     case '3': vt->cstyle |= STYLE_UNDERLINE_VAR;
		       break;
	  }
	}
	else
	{
	  vt->cstyle |= STYLE_UNDERLINE;
	}
	break;
    case 5: /* SGR@@Blink@@ */        vt->cstyle |= STYLE_BLINK; break;
    case 6: /* SGR@@Blink Fast@@ */   vt->cstyle |= STYLE_BLINK_FAST; break;
    case 7: /* SGR@@Reverse@@ */      vt->cstyle |= STYLE_REVERSE; break;
    case 8: /* SGR@@Hidden@@ */       vt->cstyle |= STYLE_HIDDEN; break;
    case 9: /* SGR@@Strikethrough@@ */vt->cstyle |= STYLE_STRIKETHROUGH; break;
    case 10: /* SGR@@Font 0@@ */      break;
    case 11: /* SGR@@Font 1@@ */      break;
    case 12: /* SGR@@Font 2(ignored)@@ */ 
    case 13: /* SGR@@Font 3(ignored)@@ */
    case 14: /* SGR@@Font 4(ignored)@@ */ break;
    case 22: /* SGR@@Bold off@@ */
      vt->cstyle ^= (vt->cstyle & STYLE_BOLD);
      vt->cstyle ^= (vt->cstyle & STYLE_DIM);
      break;
    case 23: /* SGR@@Italic off@@ */
      vt->cstyle ^= (vt->cstyle & STYLE_ITALIC);
      break;
    case 24: /* SGR@@Underscore off@@ */
      vt->cstyle ^= (vt->cstyle & (STYLE_UNDERLINE|STYLE_UNDERLINE_VAR));
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
    case 30: /* SGR@@black text color@@ */     set_fg_idx(0); break;
    case 31: /* SGR@@red text color@@ */       set_fg_idx(1); break;
    case 32: /* SGR@@green text color@@ */     set_fg_idx(2); break;
    case 33: /* SGR@@yellow text color@@ */    set_fg_idx(3); break;
    case 34: /* SGR@@blue text color@@ */      set_fg_idx(4); break;
    case 35: /* SGR@@magenta text color@@ */   set_fg_idx(5); break;
    case 36: /* SGR@@cyan text color@@ */      set_fg_idx(6); break;
    case 37: /* SGR@@light gray text color@@ */set_fg_idx(7); break;
    case 39: /* SGR@@default text color@@ */  
      set_fg_idx(vt->reverse_video?0:15);
      vt->cstyle ^= (vt->cstyle & STYLE_FG_COLOR_SET);
      break;
    case 40: /* SGR@@black background color@@ */     set_bg_idx(0); break;
    case 41: /* SGR@@red background color@@ */       set_bg_idx(1); break;
    case 42: /* SGR@@green background color@@ */     set_bg_idx(2); break;
    case 43: /* SGR@@yellow background color@@ */    set_bg_idx(3); break;
    case 44: /* SGR@@blue background color@@ */      set_bg_idx(4); break;
    case 45: /* SGR@@magenta background color@@ */   set_bg_idx(5); break;
    case 46: /* SGR@@cyan background color@@ */      set_bg_idx(6); break;
    case 47: /* SGR@@light gray background color@@ */set_bg_idx(7); break;

    case 49: /* SGR@@default background color@@ */  
      set_bg_idx(vt->reverse_video?15:0);
      vt->cstyle ^= (vt->cstyle & STYLE_BG_COLOR_SET);
      break;

    case 50: /* SGR@@Proportional spacing off @@ */
      vt->cstyle ^= (vt->cstyle & STYLE_PROPORTIONAL);
      break;
      // 51 : framed
      // 52 : encircled
    case 53: /* SGR@@Overlined@@ */
       vt->cstyle |= STYLE_OVERLINE; break;
    case 55: /* SGR@@Not Overlined@@ */
       vt->cstyle ^= (vt->cstyle&STYLE_OVERLINE); break;

    case 90: /* SGR@@dark gray text color@@ */       set_fg_idx(8); break;
    case 91: /* SGR@@light red text color@@ */       set_fg_idx(9); break;
    case 92: /* SGR@@light green text color@@ */     set_fg_idx(10); break;
    case 93: /* SGR@@light yellow text color@@ */    set_fg_idx(11); break;
    case 94: /* SGR@@light blue text color@@ */      set_fg_idx(12); break;
    case 95: /* SGR@@light magenta text color@@ */   set_fg_idx(13); break;
    case 96: /* SGR@@light cyan text color@@ */      set_fg_idx(14); break;
    case 97: /* SGR@@white text color@@ */           set_fg_idx(15); break;

    case 100: /* SGR@@dark gray background color@@ */set_bg_idx(8); break;
    case 101: /* SGR@@light red background color@@ */set_bg_idx(9); break;
    case 102: /* SGR@@light green background color@@ */set_bg_idx(10); break;
    case 103: /* SGR@@light yellow background color@@ */set_bg_idx(11); break;
    case 104: /* SGR@@light blue background color@@ */set_bg_idx(12); break;
    case 105: /* SGR@@light magenta background color@@ */set_bg_idx(13); break;
    case 106: /* SGR@@light cyan background color@@ */set_bg_idx(14); break;
    case 107: /* SGR@@white background color@@ */set_bg_idx(15); break;

    default:
      VT_warning ("unhandled style code %i in sequence \\e%s\n", n, sequence);
      return;

  }
    while(s && *s && *s != ';') s++;
    if (s && *s == ';') s++;
  }
}

static void vtcmd_ignore (MrgVT *vt, const char *sequence)
{
  VT_info("ignoring sequence %s", sequence);
}

static void vtcmd_clear_all_tabs (MrgVT *vt, const char *sequence)
{
  memset (vt->tabs, 0, sizeof (vt->tabs));
}

static void vtcmd_clear_current_tab (MrgVT *vt, const char *sequence)
{
  vt->tabs[(int)(vt->cursor_x-1)] = 0;
}

static void vtcmd_horizontal_tab_set (MrgVT *vt, const char *sequence)
{
  vt->tabs[(int)vt->cursor_x-1] = 1;
}

static void vtcmd_save_cursor_position (MrgVT *vt, const char *sequence)
{
  vt->saved_x = vt->cursor_x;
  vt->saved_y = vt->cursor_y;
}

static void vtcmd_restore_cursor_position (MrgVT *vt, const char *sequence)
{
  _ctx_vt_move_to (vt, vt->saved_y, vt->saved_x);
}


static void vtcmd_save_cursor (MrgVT *vt, const char *sequence)
{
  vt->saved_style   = vt->cstyle;
  vt->saved_origin  = vt->origin;
  vtcmd_save_cursor_position (vt, sequence);
  for (int i = 0; i < 4; i++)
    vt->saved_charset[i] = vt->charset[i];
}

static void vtcmd_restore_cursor (MrgVT *vt, const char *sequence)
{
  vtcmd_restore_cursor_position (vt, sequence);
  vt->cstyle  = vt->saved_style;
  vt->origin  = vt->saved_origin;
  for (int i = 0; i < 4; i++)
    vt->charset[i] = vt->saved_charset[i];
}

static void vtcmd_erase_n_chars (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
  {
     vt_string_replace_utf8 (vt->current_line, vt->cursor_x - 1 + n, " ");
     vt_string_set_style (vt->current_line, vt->cursor_x + n - 1, vt->cstyle);
  }
}

static void vtcmd_delete_n_chars (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  int count = n;
  while (count--)
  {
    vt_string_remove_utf8 (vt->current_line, vt->cursor_x - 1);
  }
  // XXX need style
}

static void vtcmd_delete_n_lines (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  for (int a = 0; a < n; a++)
  {
    int i;
    VtList *l;
    VtString *string = vt->current_line;
    vt_string_set (string, "");
    vt_list_remove (&vt->lines, vt->current_line);
    for (i=vt->rows, l = vt->lines; l; l=l->next, i--)
    {
      if (i == vt->margin_bottom)
      {
        vt->current_line = string;
        vt_list_insert_before (&vt->lines, l, string);
        break;
      }
    }
    _ctx_vt_move_to (vt, vt->cursor_y, vt->cursor_x); // updates current_line
  }
}

static void vtcmd_insert_character (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
  {
     vt_string_insert_utf8 (vt->current_line, vt->cursor_x-1, " ");
  }
  while (vt->current_line->utf8_length > vt->cols)
     vt_string_remove_utf8 (vt->current_line, vt->cols);
  // XXX update style
}

static void vtcmd_scroll_up (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n == 0) n = 1;
  while (n--)
    vt_scroll (vt, -1);
}

static void vtcmd_scroll_down (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n == 0) n = 1;
  while (n--)
    vt_scroll (vt, 1);
}

static void vtcmd_insert_blank_lines (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  if (n == 0) n = 1;

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

static void vtcmd_set_default_font (MrgVT *vt, const char *sequence)
{
  vt->charset[0] = 0;
}

static void vtcmd_set_alternate_font (MrgVT *vt, const char *sequence)
{
  vt->charset[0] = 1;
}

static void vtcmd_set_mode (MrgVT *vt, const char *sequence)
{
  int set = 1;
  if (sequence[strlen(sequence)-1]=='l')
    set = 0;

  if (sequence[1]=='?')
  {
    int qval;
    sequence++;
qagain:
    qval = parse_int (sequence, 1);
    switch (qval)
    {
     case 1: /*MODE;Cursor key mode;Application;Cursor;*/
	       vt->cursor_key_application = set;
	     break;
     case 2: /*MODE;VT52 emulation;;enable; */
	     if (set==0) vt->in_vt52 = 1;
	     break; 
     case 3: /*MODE;Column mode;132 columns;80 columns;*/
	     vtcmd_set_132_col (vt, set);
	     break; // set 132 col
     case 4: /*MODE;Smooth scroll;Smooth;Jump;*/
	     vt->smooth_scroll = set;
	     vt_cell_cache_clear (vt);
	     break; // set 132 col

     case 5: /*MODE;Screen mode;Reverse;Normal;*/
	     vt->reverse_video = set;
	     vt_cell_cache_clear (vt);
	     break;
     case 6: /*MODE;Origin mode;Relative;Absolute;*/
	     vt->origin = set;
	     if (set)
	     {
               _ctx_vt_move_to (vt, vt->margin_top, 1);
	       ctx_vt_carriage_return (vt);
	     }
	     else
               _ctx_vt_move_to (vt, 1, 1);
	     break;
     case 7: /*MODE;Wraparound;On;Off;*/
	     vt->autowrap = set; break;
     case 8: /*MODE;Auto repeat;On;Off;*/
	     vt->keyrepeat = set;
	     break;
     case 12:vtcmd_ignore (vt, sequence);break; // blinking_cursor


     case 30: // from rxvt - show/hide scrollbar
	     break;

     case 34: // DECRLM - right to left mode
	     break;

     case 25:/*MODE;Cursor visible;On;Off; */
	     vt->cursor_visible = set; 
	     break;
     case 60: // horizontal cursor coupling
     case 61: // vertical cursor coupling
	     break;

     case 69:/*MODE;Left right margin mode;On;Off; */
	     vt->left_right_margin_mode = set; 
	     break;

     case 80:/* DECSDM Sixel scrolling */
	     break;

     case 437:/*MODE;Encoding/cp437mode;cp437;utf8; */
	     vt->encoding = set ? 1 : 0;
	     break;

     case 1000:/*MODE;Mouse reporting;On;Off;*/
	     vt->mouse = set; break;
     case 1002:/*MODE;Mouse drag;On;Off;*/ 
	     vt->mouse_drag = set; break;
     case 1003:/*MODE;Mouse all;On;Off;*/ 
	     vt->mouse_all = set; break;
     case 1006:/*MODE;Mouse decimal;On;Off;*/ 
	     vt->mouse_decimal = set; break;
     //case 47:
     //case 1047:
     //case 1048:
     //case 1049: <- the one to implement
	//   vtcmd_reset_to_initial_state (vt, sequence);  
	   break; // alt screen
     case 1010: // scroll to bottom on tty output (rxvt)
	   break;
     case 1011: // scroll to bottom on key press (rxvt)
	   break;
     case 2004:  // set_bracketed_paste_mode
	   vt->bracket_paste = set;
	   break;
     case 2020: /*MODE;wordwrap;On;Off;*/
	     vt->wordwrap = set; break;

     case 4444:/*MODE;Audio;On;;*/
           vt->in_pcm=set; break;

     case 2222:/*MODE;Ctx;On;;*/
           vt->in_ctx=set;
	   break;
     case 7020:/*MODE;Ctx ascii;On;;*/
           vt->in_ctx_ascii = set;
	   break;

     default:
	   VT_warning ("unhandled CSI ? %ih", qval); return;
    }
    if (strchr (sequence + 1, ';'))
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
     case 2:/* AM - keyboard action mode */ break;
     case 3:/*CRM - control representation mode */
	     /* show control chars? */
	     break;
     case 4:/*MODE2;Insert Mode;Insert;Replace; */
	     vt->insert_mode = set; break;
     case 9: /* interlace mode */
	     break;
     case 12:/*MODE2;Local echo;On;Off; */
	     vt->echo = set; break;
     case 20:/*MODE2;Carriage Return on LF/Newline;On;Off;*/;
	     vt->cr_on_lf = set; break;
     case 21: // GRCM - whether SGR accumulates or a reset on each command
	     break;

     default: VT_warning ("unhandled CSI %ih", val); return;
    }
    if (strchr (sequence, ';') && sequence[0] != ';')
    {
      sequence = strchr (sequence, ';');
      goto again;
    }
  }
}

static void vtcmd_set_t (MrgVT *vt, const char *sequence)
{
  if (!strcmp (sequence, "[14t")) /* request terminal dimensions */
  {
    char buf[128];
    sprintf (buf, "\e[4;%i;%it", vt->rows * vt->ch, vt->cols * vt->cw);
    vt_write (vt, buf, strlen(buf));
  }
  else if (sequence[strlen (sequence)-2]==' ') /* DECSWBV */
  {
    int val = parse_int (sequence, 0);
    if (val <= 1) vt->bell = 0;
    if (val <= 8) vt->bell = val;
  }
  else
  {
    VT_info ("unhandled subsequence %s", sequence);
  }
}

static void _ctx_vt_htab (MrgVT *vt)
{
  do {
    vt->cursor_x ++;
  } while (vt->cursor_x < VT_MARGIN_RIGHT &&  ! vt->tabs[(int)vt->cursor_x-1]);
  if (vt->cursor_x > VT_MARGIN_RIGHT)
    vt->cursor_x = VT_MARGIN_RIGHT;
}

static void _ctx_vt_rev_htab (MrgVT *vt)
{
  do {
    vt->cursor_x--;
  } while ( ! vt->tabs[(int)vt->cursor_x-1] && vt->cursor_x > 1);
  if (vt->cursor_x < VT_MARGIN_LEFT)
    ctx_vt_carriage_return (vt);
}

static void vtcmd_insert_n_tabs (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
  {
    _ctx_vt_htab (vt);
  }
}

static void vtcmd_rev_n_tabs (MrgVT *vt, const char *sequence)
{
  int n = parse_int (sequence, 1);
  while (n--)
  {
    _ctx_vt_rev_htab (vt);
  }
}

static void vtcmd_set_double_width_double_height_top_line
 (MrgVT *vt, const char *sequence)
{
  vt->current_line->double_width = 1;
  vt->current_line->double_height_top = 1;
  vt->current_line->double_height_bottom = 0;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}
static void vtcmd_set_double_width_double_height_bottom_line
 (MrgVT *vt, const char *sequence)
{
  vt->current_line->double_width = 1;
  vt->current_line->double_height_top = 0;
  vt->current_line->double_height_bottom = 1;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}
static void vtcmd_set_single_width_single_height_line
 (MrgVT *vt, const char *sequence)
{
  vt->current_line->double_width = 0;
  vt->current_line->double_height_top = 0;
  vt->current_line->double_height_bottom = 0;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}
static void
vtcmd_set_double_width_single_height_line
 (MrgVT *vt, const char *sequence)
{
  vt->current_line->double_width = 1;
  vt->current_line->double_height_top = 0;
  vt->current_line->double_height_bottom = 0;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}

static void vtcmd_set_led (MrgVT *vt, const char *sequence)
{
  int val = 0;
  fprintf (stderr, "%s\n", sequence);
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
	    vt->leds[0] = vt->leds[1] = vt->leds[2] = vt->leds[3] = 0;
	  else
	    vt->leds[val-1] = 1;
	  val = 0;
	  break;
    }
  }
}

static void vtcmd_char_at_cursor (MrgVT *vt, const char *sequence)
{
  char *buf="";
  vt_write (vt, buf, strlen(buf));
}

static void vtcmd_DECELR (MrgVT *vt, const char *sequence)
{
  int ps1 = parse_int (sequence, 0);
  int ps2 = 0;
  const char *s = strchr(sequence, ';');
  if (ps1); // because it is unused
  if (s)
    ps2 = parse_int (s, 0);
  if (ps2 == 1)
    vt->unit_pixels = 1;
  else
    vt->unit_pixels = 0;

}

static void vtcmd_graphics (MrgVT *vt, const char *sequence)
{
  fprintf (stderr, "gfx intro [%s]\n" ,sequence);
}

static void vtcmd_report (MrgVT *vt, const char *sequence)
{
  char buf[64]="";
  if (!strcmp (sequence, "[?15n")) // printer status
  {
    sprintf (buf, "\e[?13n"); // no printer
  }
  else if (!strcmp (sequence, "[?26n")) // keyboard dialect 
  {
    sprintf (buf, "\e[?27;1n"); // north american/ascii
  }
  else if (!strcmp (sequence, "[?25n")) // User Defined Key status
  {
    sprintf (buf, "\e[?21n"); // locked
  }
  else if (!strcmp (sequence, "[6n")) // DSR cursor position report
  {
    sprintf (buf, "\033[%i;%iR", vt->cursor_y - (vt->origin?(vt->margin_top - 1):0), (int)vt->cursor_x - (vt->origin?(VT_MARGIN_LEFT-1):0));
  }
  else if (!strcmp (sequence, "[?6n")) // DECXPR extended cursor position report
  {
    sprintf (buf, "\033[?%i;%i;1R", vt->cursor_y - (vt->origin?(vt->margin_top - 1):0), (int)vt->cursor_x - (vt->origin?(VT_MARGIN_LEFT-1):0));
  }
  else if (!strcmp (sequence, "[5n")) // DSR decide status report
  {
    sprintf (buf, "\033[0n"); // we're always OK :)
  }
  else if (!strcmp (sequence, "[>c"))
  {
    sprintf (buf, "\033[>23;01;1c");
  }
  else if (sequence[strlen(sequence)-1]=='c') // device attributes
  {
    //buf = "\033[?1;2c"; // what rxvt reports
    //buf = "\033[?1;6c"; // VT100 with AVO ang GPO
    //buf = "\033[?2c";     // VT102
    sprintf (buf, "\033[?63;14;22c");
  }
  else if (sequence[strlen(sequence)-1]=='x') // terminal parameters
  {
    if (!strcmp (sequence, "[1x"))
      sprintf (buf, "\e[3;1;1;120;120;1;0x");
    else
      sprintf (buf, "\e[2;1;1;120;120;1;0x");
  }

  if (buf[0])
    vt_write (vt, buf, strlen(buf));
}

static char* charmap_cp437[]={
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
"≡","±","≥","≤","⌠","⌡","÷","≈","°","∙","·","√","ⁿ","²","■"," "};


static char* charmap_graphics[]={
" ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/","0",
"1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
"@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
"Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
"◆","▒","␉","␌","␍","␊","°","±","␤","␋","┘","┐","┌","└","┼","⎺","⎻",
"─","⎼","⎽","├","┤","┴","┬","│","≤","≥","π","≠","£","·"," "};

static char* charmap_uk[]={
" ","!","\"","£","$","%","&","'","(",")","*","+",",","-",".","/","0",
"1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
"@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
"Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
"`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p",
"q","r","s","t","u","v","w","x","y","z","{","|","}","~"," "};

static char* charmap_ascii[]={
" ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/","0",
"1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
"@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
"Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
"`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p",
"q","r","s","t","u","v","w","x","y","z","{","|","}","~"," "};

static void vtcmd_set_charmap (MrgVT *vt, const char *sequence)
{
  int slot = 0;
  int set = sequence[1];
  if (sequence[0] == ')') slot = 1;
  if (set == 'G') set = 'B';
  vt->charset[slot] = set;
}
#if 0

CSI Pm ' }    '
          Insert Ps Column(s) (default = 1) (DECIC), VT420 and up.

CSI Pm ' ~    '
          Delete Ps Column(s) (default = 1) (DECDC), VT420 and up.


in text.  When bracketed paste mode is set, the program will receive:
   ESC [ 2 0 0 ~ ,
followed by the pasted text, followed by
   ESC [ 2 0 1 ~ .


   CSI I
when the terminal gains focus, and CSI O  when it loses focus.
#endif

static Sequence sequences[]={
/*
  prefix suffix  command */
  //{"B",  0,  vtcmd_break_permitted},
  //{"C",  0,  vtcmd_nobreak_here},
  {"D",  0,  vtcmd_index}, /* id:IND Index */
  {"E",  0,  vtcmd_next_line},
  {"_", 'G',  vtcmd_graphics},

  //{"I",  0,  vtcmd_char_tabulation_with_justification},
  //{"K",  0,  PLD partial line down
  //{"L",  0,  PLU partial line up
  {"M",  0,  vtcmd_reverse_index}, /* id:RI Reverse Index */
  //{"N",  0,  vtcmd_ignore}, /* Set Single Shift 2 - SS2*/
  //{"O",  0,  vtcmd_ignore}, /* Set Single Shift 3 - SS3*/

  {"[",  'A', vtcmd_cursor_up},   /* args:Pn    id:CUU Cursor Up */
  {"[",  'B', vtcmd_cursor_down}, /* args:Pn    id:CUD Cursor Down */
  {"[",  'C', vtcmd_cursor_forward}, /* args:Pn id:CUF Cursor Forward */
  {"[",  'D', vtcmd_cursor_backward}, /* args:Pn id:CUB Cursor Backward */
  {"[",  'E', vtcmd_next_line}, /* args:Pn id:CNL Cursor Next Line */
  {"[",  'F', vtcmd_cursor_preceding_line}, /* args:Pn id:CPL Cursor Preceding Line */
  {"[",  'G', vtcmd_horizontal_position_absolute}, /* args:Pn id:CHA Cursor Horizontal Absolute */
  {"[",  'H', vtcmd_cursor_position}, /* args:Pl;Pc id:CUP Cursor Position */
  {"[",  'I', vtcmd_insert_n_tabs}, /* args:Pn id:CHT Cursor Horizontal Forward Tabulation */
  {"[",  'J', vtcmd_erase_in_display}, /* args:Ps id:ED Erase in Display */
  {"[",  'K', vtcmd_erase_in_line}, /* args:Ps id:EL Erase in Line */
  {"[",  'L', vtcmd_insert_blank_lines}, /* args:Pn id:IL Insert Line */
  {"[",  'M', vtcmd_delete_n_lines}, /* args:Pn id:DL Delete Line   */
  {"[",  'P', vtcmd_delete_n_chars}, /* args:Pn id:DCH Delete Character */
  {"[",  'X', vtcmd_erase_n_chars}, /* args:Pn id:ECH Erase Character */
  {"[",  'S', vtcmd_scroll_up}, /* args:Pn id:SU Scroll Up */
  {"[",  'T', vtcmd_scroll_down}, /* args:Pn id:SD Scroll Down */
  {"[",  '^', vtcmd_scroll_down}, /* muphry alternate from ECMA */
  {"[",  'Z', vtcmd_rev_n_tabs}, /* args:Pn id:CBT Cursor Backward Tabulation */
  {"[",  '@', vtcmd_insert_character}, /* args:Pn id:ICH Insert Character */

  {"[",  'a', vtcmd_cursor_forward}, /* args:Pn id:HPR Horizontal Position Relative */
  {"[",  'b', vtcmd_cursor_forward}, /* REP previous char */
  {"[",  'c', vtcmd_report}, /* id:DA Device Attributes */
  {"[",  'd', vtcmd_goto_row},       /* args:Pn id:VPA Vertical Position Absolute  */
  {"[",  'e', vtcmd_cursor_down},    /* args:Pn id:VPR Vertical Position Relative */
  {"[",  'f', vtcmd_cursor_position}, /* args:Pl;Pc id:CUP Cursor Position */
  {"[0g", 0,  vtcmd_clear_current_tab}, /* id:XXX clear current tab */
  {"[3g", 0,  vtcmd_clear_all_tabs},    /* id:XXX clear all tabs */
  {"[",  'm', vtcmd_set_graphics_rendition}, /* args:Ps;Ps;.. id:SGR Select Graphics Rendition */
  {"[",  'n', vtcmd_report},           /* id:DSR CPR Cursor Position Report  */
  {"[",  'r', vtcmd_set_top_and_bottom_margins}, /* args:Pt;Pb id:DECSTBM Set Top and Bottom Margins */
  // XXX: the save cursor position below conflicts with set left and right margins 
  {"[u",  0,  vtcmd_restore_cursor_position}, /*id:RCP Restore Cursor Position */
  {"[",  's', vtcmd_set_left_and_right_margins}, /* args:Pl;Pr id:DECSLRM Set Left and Right Margins */
  {"[",  '`', vtcmd_horizontal_position_absolute},  /* args:Pn id:HPA Horizontal Position Absolute */

  {"[",  'h', vtcmd_set_mode},   /* args:Ps;Ps;.. id:SM Set Mode */
  {"[",  'l', vtcmd_set_mode}, /* args:Ps;Ps;..  id:RM Reset Mode */
  {"[",  't', vtcmd_set_t}, 
  {"[",  'q', vtcmd_set_led}, /* args:Ps id:DECLL Load LEDs */
  {"[",  'x', vtcmd_report}, /* id:DECREQTPARM */
  {"[",  'z', vtcmd_DECELR}, /* id:DECELR set locator res  */

  {"5",   0,  vtcmd_char_at_cursor}, /* id:DECXMIT */
  {"6",   0,  vtcmd_back_index}, /* id:DECBI Back index */
  {"7",   0,  vtcmd_save_cursor}, /* id:DECSC Save Cursor */
  {"8",   0,  vtcmd_restore_cursor}, /* id:DECRC Restore Cursor */
  {"9",   0,  vtcmd_forward_index}, /* id:DECFI Forward index */
  {"H",   0,  vtcmd_horizontal_tab_set}, /* id:HTS Horizontal Tab Set */

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

  {"#3",  0,   vtcmd_set_double_width_double_height_top_line},
  {"#4",  0,   vtcmd_set_double_width_double_height_bottom_line},
  {"#5",  0,   vtcmd_set_single_width_single_height_line},
  {"#6",  0,   vtcmd_set_double_width_single_height_line},

  {"#8",  0,   vtcmd_screen_alignment_display}, /* id:DECALN Screen Alignment Display */
  {"=",   0,   vtcmd_ignore},  // keypad mode change
  {">",   0,   vtcmd_ignore},  // keypad mode change
  {"c",   0,   vtcmd_reset_to_initial_state}, /* id:RIS Reset to Initial State */
  {"[!", 'p',  vtcmd_ignore}, // soft reset?

  {NULL, 0, NULL}
};

static void handle_sequence (MrgVT *vt, const char *sequence)
{
  int i;
  for (i = 0; sequences[i].prefix; i++)
  {
    if (!strncmp (sequence, sequences[i].prefix, strlen(sequences[i].prefix)))
    {
      int i0;
      int mismatch = 0;
      i0 = strlen (sequence)-1;
      if (sequences[i].suffix && (sequence[i0] != sequences[i].suffix))
        mismatch = 1;
      if (!mismatch)
      {
	VT_command("%s", sequence);
        sequences[i].vtcmd (vt, sequence);
        return;
      }
    }
  }
  VT_warning ("unhandled: %c%c%c%c%c%c%c%c%c\n", sequence[0], sequence[1], sequence[2], sequence[3], sequence[4], sequence[5], sequence[6], sequence[7], sequence[8]);
}

static void ctx_vt_line_feed (MrgVT *vt)
{
  int was_home = vt->at_line_home;
  if (vt->margin_top == 1 && vt->margin_bottom == vt->rows)
  {
    if (vt->lines->data == vt->current_line && vt->cursor_y != vt->rows)
    {
      vt->current_line = vt_string_new_with_size ("", vt->cols);
      vt_list_prepend (&vt->lines, vt->current_line);
      vt->line_count++;
    }

    if (vt->cursor_y >= vt->margin_bottom){
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
      vt->current_line = vt_string_new_with_size ("", vt->cols);
      vt_list_prepend (&vt->lines, vt->current_line);
      vt->line_count++;
    }
    vt->cursor_y++; 
    if (vt->cursor_y > vt->margin_bottom)
    {
      vt->cursor_y = vt->margin_bottom;
      vt_scroll (vt, -1);
    }
  }
  _ctx_vt_move_to (vt, vt->cursor_y, vt->cursor_x);

  if (vt->cr_on_lf)
    ctx_vt_carriage_return (vt);

  ctx_vt_trimlines (vt, vt->rows);

  if (was_home)
    ctx_vt_carriage_return (vt);
}

static void
ctx_vt_carriage_return (MrgVT *vt)
{
  _ctx_vt_move_to (vt, vt->cursor_y, vt->cursor_x);
  vt->cursor_x = VT_MARGIN_LEFT;
  vt->at_line_home = 1;
}

static short MuLawDecompressTable[256] =
{
     -32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
     -23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
     -15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
     -11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316,
      -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
      -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
      -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
      -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
      -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
      -1372, -1308, -1244, -1180, -1116, -1052,  -988,  -924,
       -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
       -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
       -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260,
       -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132,
       -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64,
        -56,   -48,   -40,   -32,   -24,   -16,    -8,     -1,
      32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
      23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
      15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
      11900, 11388, 10876, 10364,  9852,  9340,  8828,  8316,
       7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140,
       5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
       3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,
       2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
       1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436,
       1372,  1308,  1244,  1180,  1116,  1052,   988,   924,
        876,   844,   812,   780,   748,   716,   684,   652,
        620,   588,   556,   524,   492,   460,   428,   396,
        372,   356,   340,   324,   308,   292,   276,   260,
        244,   228,   212,   196,   180,   164,   148,   132,
        120,   112,   104,    96,    88,    80,    72,    64,
         56,    48,    40,    32,    24,    16,     8,     0
};

void terminal_queue_pcm_sample (int16_t sample);

static unsigned char vt_bell_audio[] = {
#if 1
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf, 0xaf,
  0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
#else
  0x7e, 0xfe, 0x7e, 0x7d, 0x7e, 0x7e, 0x7e, 0x7d, 0x7e, 0x7e, 0x7e, 0xff,
  0xff, 0xfe, 0xfe, 0x7e, 0xff, 0xfe, 0xfd, 0xfd, 0xfe, 0xfe, 0xfd, 0xfd,
  0xfe, 0xfe, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7d, 0x7d,
  0xfe, 0x7e, 0x7e, 0x7e, 0x7e, 0xfd, 0xfd, 0x7e, 0x7e, 0xfd, 0xfe, 0xfe,
  0xfe, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0xfe, 0xfe, 0xff, 0xfe,
  0xfe, 0xfe, 0x7d, 0x7c, 0xfb, 0xfa, 0xfc, 0xfd, 0xfc, 0x76, 0x75, 0xfa,
  0xfb, 0x7b, 0xfc, 0xef, 0xf6, 0x77, 0x6d, 0x7b, 0xf8, 0x78, 0x78, 0xfa,
  0xf7, 0xfd, 0xfd, 0xfc, 0xfc, 0xfa, 0xf5, 0xf7, 0x7d, 0x7b, 0x78, 0x77,
  0x7c, 0x6f, 0x7b, 0xf5, 0xfb, 0x7b, 0x7c, 0x78, 0x76, 0xea, 0xf2, 0x6d,
  0xfd, 0xed, 0x7a, 0x6d, 0x6e, 0x71, 0xfe, 0x76, 0x6d, 0xfb, 0xef, 0x7e,
  0xfa, 0xef, 0xec, 0xed, 0xf8, 0xf0, 0xea, 0xf9, 0x70, 0x7c, 0x7c, 0x6b,
  0x6d, 0x75, 0xfb, 0xf1, 0xf9, 0xfe, 0xec, 0xea, 0x7c, 0x75, 0xff, 0xfb,
  0x7d, 0x77, 0x7a, 0x71, 0x6e, 0x6c, 0x6e, 0x7b, 0x7e, 0x7a, 0x7c, 0xf4,
  0xf9, 0x7b, 0x7b, 0xfa, 0xfe, 0x73, 0x79, 0xfe, 0x7b, 0x76, 0xfe, 0xf3,
  0xf9, 0x76, 0x77, 0x7e, 0x7e, 0x7d, 0x7c, 0xf9, 0xee, 0xf2, 0x7d, 0xf8,
  0xec, 0xee, 0xf7, 0xfa, 0xf7, 0xf6, 0xfd, 0x77, 0x75, 0x7b, 0xfa, 0xfe,
  0x78, 0x79, 0x7c, 0x76, 0x7e, 0xf7, 0xfb, 0xf5, 0xf6, 0x75, 0x6f, 0x74,
  0x6e, 0x6e, 0x6d, 0x6c, 0x7a, 0xf9, 0x75, 0x77, 0xf4, 0xf0, 0xf0, 0xf1,
  0xef, 0xf3, 0xf6, 0xfd, 0xfc, 0xfb, 0xfd, 0xfc, 0xf6, 0xf8, 0xfb, 0xf9,
  0xfa, 0xfd, 0xfb, 0xfc, 0x7a, 0x7c, 0x77, 0x75, 0x78, 0x7a, 0x7a, 0x78,
  0x7a, 0xfa, 0xf9, 0x7c, 0xff, 0xfb, 0x7d, 0x77, 0x73, 0x6c, 0x6e, 0x7b,
  0xfc, 0xfe, 0x7e, 0xfb, 0xf1, 0xeb, 0xee, 0xf6, 0xf6, 0xef, 0xf7, 0x7c,
  0x76, 0x76, 0x7b, 0x7a, 0x7b, 0x73, 0x73, 0x7c, 0x79, 0x70, 0x79, 0xfb,
  0xfd, 0xf8, 0xf9, 0xfc, 0xfc, 0xf8, 0xfb, 0xff, 0xfc, 0xf9, 0x75, 0x6f,
  0x74, 0xfe, 0xff, 0xfd, 0x7d, 0xf5, 0xef, 0xee, 0xf8, 0xfd, 0xfd, 0xf3,
  0xfa, 0xfe, 0xfe, 0x7c, 0x77, 0x7a, 0xfb, 0x79, 0x7e, 0x7b, 0xfd, 0x6d,
  0xfc, 0x7a, 0xf0, 0x74, 0xee, 0x79, 0xea, 0x79, 0xf9, 0x6d, 0xf7, 0x71,
  0x79, 0x76, 0x7c, 0x77, 0x6f, 0xf3, 0x6c, 0xe8, 0x67, 0xe3, 0x5e, 0xdc,
  0x58, 0xd8, 0x4e, 0xce, 0x46, 0xc5, 0x40, 0x67, 0xba, 0x49, 0xac, 0x26,
  0xba, 0x3e, 0xc5, 0xc8, 0x2b, 0xa8, 0x32, 0xbd, 0xe4, 0x3e, 0xb7, 0x3b,
  0xb7, 0x3a, 0x33, 0xab, 0x3f, 0xc8, 0x46, 0x5f, 0xb7, 0x69, 0xd4, 0x3d,
  0xc0, 0x4c, 0xf2, 0xdb, 0x3b, 0xdd, 0x69, 0xc5, 0x5f, 0xd8, 0xd8, 0xda,
  0xc6, 0x39, 0xba, 0x3f, 0x35, 0xb3, 0x3e, 0xbb, 0x4a, 0x4a, 0xe7, 0x60,
  0xae, 0x2c, 0xcb, 0x53, 0x45, 0xaf, 0x2a, 0xae, 0x3e, 0x4a, 0xae, 0x2a,
  0xad, 0x38, 0xcc, 0xbb, 0x36, 0xae, 0x2c, 0xc6, 0xce, 0x38, 0xb1, 0x2f,
  0xb9, 0x54, 0x7c, 0xb3, 0x28, 0xae, 0x3d, 0xcf, 0xbb, 0x2e, 0xb4, 0x41,
  0xc6, 0x78, 0x39, 0xbc, 0x41, 0xc8, 0x59, 0x5b, 0xc7, 0x43, 0xbc, 0x45,
  0xf3, 0xdc, 0x69, 0xd6, 0x48, 0xc9, 0x4e, 0xd9, 0x59, 0x61, 0xde, 0x4b,
  0xc9, 0x44, 0xc8, 0xf5, 0x43, 0xc5, 0x37, 0xba, 0x65, 0x4d, 0xc8, 0x31,
  0xaf, 0x47, 0xdb, 0xd6, 0x36, 0xad, 0x37, 0xbb, 0x61, 0x3a, 0xae, 0x2d,
  0xb4, 0x47, 0x49, 0xb2, 0x30, 0xac, 0x3a, 0xcd, 0xbc, 0x2e, 0xaf, 0x32,
  0xbd, 0xd7, 0x34, 0xaf, 0x32, 0xbb, 0x55, 0x4a, 0xb4, 0x30, 0xbb, 0x40,
  0xeb, 0xbf, 0x39, 0xba, 0x3a, 0xd6, 0xd3, 0x48, 0xc0, 0x3b, 0xce, 0x5e,
  0xe7, 0xd3, 0x46, 0xcb, 0x4c, 0xce, 0x74, 0x7e, 0x7e, 0x55, 0xcf, 0x44,
  0xc4, 0x5b, 0x7c, 0xd3, 0x3f, 0xbc, 0x44, 0xcb, 0xfa, 0x46, 0xb9, 0x37,
  0xb8, 0x51, 0x54, 0xbe, 0x33, 0xb1, 0x3d, 0xce, 0xc4, 0x34, 0xaf, 0x2f,
  0xbd, 0xf8, 0x37, 0xb0, 0x2d, 0xb1, 0x4c, 0x4a, 0xb3, 0x2c, 0xb0, 0x3c,
  0xe4, 0xbf, 0x2f, 0xaf, 0x35, 0xc0, 0xdb, 0x39, 0xb3, 0x31, 0xbb, 0x5d,
  0x4c, 0xb8, 0x37, 0xb9, 0x48, 0xe8, 0xc7, 0x3d, 0xba, 0x43, 0xce, 0xdd,
  0x52, 0xc6, 0x46, 0xce, 0x55, 0xdf, 0xe8, 0x52, 0xd5, 0x48, 0xca, 0x4d,
  0xef, 0x68, 0x4c, 0xc7, 0x42, 0xc2, 0x49, 0x78, 0xce, 0x3e, 0xb9, 0x3c,
  0xc8, 0xef, 0x43, 0xb7, 0x35, 0xb8, 0x4a, 0x53, 0xb8, 0x32, 0xaf, 0x3b,
  0xde, 0xc1, 0x34, 0xaf, 0x32, 0xc3, 0xde, 0x3b, 0xaf, 0x2e, 0xb6, 0x4e,
  0x48, 0xb4, 0x2e, 0xb2, 0x3d, 0xf0, 0xbf, 0x33, 0xb2, 0x37, 0xc8, 0xd9,
  0x3d, 0xb5, 0x36, 0xbc, 0x56, 0x4f, 0xbc, 0x39, 0xbc, 0x47, 0xf6, 0xcf,
  0x44, 0xbf, 0x46, 0xce, 0x68, 0x5b, 0xd0, 0x4a, 0xcc, 0x4d, 0xd3, 0x60,
  0x6a, 0xcf, 0x49, 0xc8, 0x45, 0xd0, 0x7b, 0x58, 0xc3, 0x3c, 0xbf, 0x48,
  0xe2, 0xc9, 0x3b, 0xb7, 0x39, 0xc5, 0xdb, 0x40, 0xb6, 0x31, 0xb9, 0x50,
  0x50, 0xb9, 0x2f, 0xb3, 0x3b, 0xdc, 0xbf, 0x33, 0xaf, 0x32, 0xc1, 0xd6,
  0x3b, 0xb0, 0x2f, 0xb8, 0x54, 0x4a, 0xb6, 0x30, 0xb4, 0x3f, 0xfd, 0xc0,
  0x36, 0xb5, 0x39, 0xcc, 0xd9, 0x41, 0xb9, 0x39, 0xc2, 0x59, 0x57, 0xc1,
  0x3e, 0xc2, 0x49, 0xe2, 0xd7, 0x4c, 0xcb, 0x47, 0xcf, 0x5b, 0xec, 0xe0,
  0x53, 0xcb, 0x4b, 0xca, 0x55, 0xf6, 0xdb, 0x48, 0xc0, 0x43, 0xc9, 0x5f,
  0x54, 0xc0, 0x3c, 0xbb, 0x43, 0xe8, 0xc8, 0x39, 0xb5, 0x39, 0xc6, 0xde,
  0x3d, 0xb4, 0x32, 0xba, 0x4f, 0x4c, 0xb9, 0x30, 0xb2, 0x3c, 0xec, 0xc1,
  0x33, 0xaf, 0x35, 0xc4, 0xd7, 0x3a, 0xb2, 0x31, 0xba, 0x56, 0x48, 0xb9,
  0x33, 0xb7, 0x44, 0x7e, 0xc3, 0x39, 0xb7, 0x3d, 0xcd, 0xe3, 0x42, 0xbd,
  0x3d, 0xc2, 0x58, 0x5d, 0xcb, 0x43, 0xc4, 0x4c, 0xd8, 0xf8, 0x58, 0xcd,
  0x4c, 0xcb, 0x4e, 0xda, 0x71, 0x5c, 0xcc, 0x46, 0xc4, 0x49, 0xdc, 0xdc,
  0x46, 0xbe, 0x3d, 0xc4, 0x59, 0x53, 0xbe, 0x38, 0xb8, 0x41, 0xe1, 0xc5,
  0x39, 0xb3, 0x38, 0xc4, 0xde, 0x3d, 0xb2, 0x32, 0xb9, 0x4e, 0x4b, 0xb7,
  0x30, 0xb3, 0x3d, 0xf2, 0xbf, 0x33, 0xb1, 0x36, 0xc9, 0xd9, 0x3a, 0xb4,
  0x33, 0xbc, 0x58, 0x49, 0xba, 0x36, 0xb9, 0x46, 0x7e, 0xc8, 0x3c, 0xba,
  0x3f, 0xcd, 0xe8, 0x4b, 0xc1, 0x41, 0xc7, 0x57, 0xfe, 0xd3, 0x4e, 0xc9,
  0x4d, 0xd0, 0x5e, 0x7c, 0xda, 0x4e, 0xca, 0x47, 0xcd, 0x5b, 0x68, 0xcc,
  0x40, 0xbf, 0x42, 0xd2, 0xe4, 0x42, 0xbd, 0x3a, 0xbf, 0x56, 0x50, 0xbd,
  0x36, 0xb6, 0x40, 0xe2, 0xc5, 0x36, 0xb2, 0x37, 0xc5, 0xde, 0x3c, 0xb3,
  0x32, 0xba, 0x52, 0x4a, 0xb7, 0x31, 0xb4, 0x3f, 0xef, 0xbf, 0x34, 0xb2,
  0x39, 0xc8, 0xd3, 0x3c, 0xb6, 0x37, 0xbe, 0x5c, 0x4c, 0xbd, 0x39, 0xbc,
  0x49, 0xf2, 0xcc, 0x3f, 0xbf, 0x44, 0xcf, 0xfd, 0x51, 0xca, 0x48, 0xcb,
  0x54, 0xe4, 0xeb, 0x57, 0xcf, 0x4d, 0xcc, 0x4f, 0xe0, 0xee, 0x51, 0xc7,
  0x44, 0xc6, 0x4f, 0x78, 0xcc, 0x3f, 0xbd, 0x3e, 0xce, 0xe5, 0x42, 0xba,
  0x38, 0xbe, 0x50, 0x4f, 0xbb, 0x35, 0xb6, 0x3e, 0xe8, 0xc2, 0x36, 0xb2,
  0x37, 0xc6, 0xda, 0x3c, 0xb3, 0x32, 0xba, 0x55, 0x4a, 0xb7, 0x33, 0xb5,
  0x41, 0x7e, 0xbf, 0x37, 0xb4, 0x3b, 0xcd, 0xd8, 0x3e, 0xb8, 0x39, 0xc2,
  0x5b, 0x4f, 0xc0, 0x3c, 0xbf, 0x4a, 0xee, 0xd1, 0x47, 0xc5, 0x47, 0xd0,
  0x68, 0x63, 0xd0, 0x4d, 0xcd, 0x4e, 0xd6, 0x67, 0x68, 0xd6, 0x4b, 0xc9,
  0x4a, 0xd1, 0x6e, 0x52, 0xc5, 0x3f, 0xc0, 0x4b, 0xfd, 0xcb, 0x3d, 0xba,
  0x3d, 0xcc, 0xe2, 0x41, 0xb8, 0x37, 0xbc, 0x53, 0x4e, 0xba, 0x34, 0xb6,
  0x3f, 0xee, 0xc1, 0x36, 0xb2, 0x38, 0xc8, 0xd6, 0x3c, 0xb3, 0x34, 0xbc,
  0x58, 0x49, 0xb9, 0x34, 0xb7, 0x44, 0x73, 0xc3, 0x38, 0xb7, 0x3d, 0xce,
  0xd9, 0x40, 0xbc, 0x3c, 0xc5, 0x5e, 0x55, 0xc6, 0x40, 0xc3, 0x4d, 0xe5,
  0xde, 0x4d, 0xca, 0x4b, 0xce, 0x5c, 0xfa, 0xe1, 0x54, 0xcd, 0x4d, 0xcd,
  0x56, 0xf3, 0xd9, 0x4a, 0xc4, 0x44, 0xcb, 0x67, 0x53, 0xc3, 0x3d, 0xbe,
  0x48, 0xf0, 0xca, 0x3c, 0xb8, 0x3b, 0xca, 0xdf, 0x3f, 0xb7, 0x36, 0xbc,
  0x54, 0x4c, 0xb9, 0x34, 0xb6, 0x40, 0xf7, 0xc1, 0x36, 0xb3, 0x39, 0xca,
  0xd6, 0x3c, 0xb4, 0x35, 0xbe, 0x5b, 0x49, 0xba, 0x36, 0xba, 0x47, 0x6f,
  0xc5, 0x3b, 0xba, 0x3f, 0xd2, 0xdd, 0x46, 0xbe, 0x3f, 0xc9, 0x5c, 0x5d,
  0xcc, 0x47, 0xc8, 0x4e, 0xdd, 0xf5, 0x5a, 0xd1, 0x4e, 0xcf, 0x52, 0xde,
  0x7d, 0x5c, 0xcf, 0x49, 0xc9, 0x4d, 0xdd, 0xde, 0x49, 0xc1, 0x3f, 0xc6,
  0x5d, 0x53, 0xc0, 0x3b, 0xbc, 0x46, 0xeb, 0xc8, 0x3b, 0xb7, 0x3b, 0xc8,
  0xde, 0x3e, 0xb6, 0x35, 0xbb, 0x57, 0x4c, 0xb9, 0x34, 0xb6, 0x42, 0xff,
  0xc1, 0x36, 0xb4, 0x3a, 0xcc, 0xd7, 0x3d, 0xb7, 0x37, 0xbf, 0x5e, 0x4a,
  0xbc, 0x38, 0xbc, 0x4a, 0x6e, 0xc9, 0x3e, 0xbe, 0x43, 0xd5, 0xe2, 0x4b,
  0xc5, 0x45, 0xcb, 0x5e, 0x6e, 0xd6, 0x4e, 0xcd, 0x51, 0xd7, 0x65, 0x74,
  0xdc, 0x54, 0xcd, 0x4d, 0xd1, 0x5e, 0x6b, 0xcf, 0x46, 0xc4, 0x47, 0xd7,
  0xe3, 0x48, 0xbe, 0x3d, 0xc3, 0x58, 0x54, 0xbe, 0x39, 0xba, 0x43, 0xee,
  0xc7, 0x3a, 0xb6, 0x3a, 0xc9, 0xdc, 0x3e, 0xb5, 0x35, 0xbd, 0x57, 0x4b,
  0xb9, 0x34, 0xb7, 0x43, 0x6f, 0xc1, 0x38, 0xb6, 0x3c, 0xcf, 0xd3, 0x3e,
  0xb8, 0x3a, 0xc3, 0x64, 0x4c, 0xbe, 0x3c, 0xbf, 0x4d, 0x72, 0xcc, 0x42,
  0xc1, 0x48, 0xd9, 0xed, 0x51, 0xcc, 0x4b, 0xcf, 0x5a, 0xef, 0xe8, 0x59,
  0xd3, 0x50, 0xd1, 0x58, 0xe1, 0xec, 0x56, 0xcc, 0x49, 0xca, 0x55, 0x7c,
  0xcf, 0x44, 0xbf, 0x43, 0xcf, 0xe5, 0x47, 0xbc, 0x3b, 0xbf, 0x56, 0x52,
  0xbe, 0x38, 0xb9, 0x42, 0xee, 0xc6, 0x39, 0xb6, 0x3a, 0xca, 0xdc, 0x3d,
  0xb6, 0x36, 0xbd, 0x59, 0x4a, 0xba, 0x35, 0xb9, 0x45, 0x6b, 0xc2, 0x39,
  0xb8, 0x3d, 0xd2, 0xd5, 0x3f, 0xbb, 0x3c, 0xc6, 0x69, 0x4f, 0xc1, 0x3f,
  0xc3, 0x50, 0x7d, 0xd0, 0x49, 0xc8, 0x4c, 0xd8, 0x7d, 0x5d, 0xd4, 0x4f,
  0xd2, 0x57, 0xde, 0x6e, 0x69, 0xda, 0x50, 0xcd, 0x4e, 0xd6, 0x71, 0x59,
  0xca, 0x44, 0xc5, 0x4d, 0xf3, 0xce, 0x41, 0xbd, 0x3f, 0xcd, 0xe5, 0x45,
  0xbb, 0x3a, 0xbf, 0x55, 0x51, 0xbd, 0x37, 0xb9, 0x43, 0xf1, 0xc5, 0x39,
  0xb6, 0x3b, 0xcc, 0xd9, 0x3d, 0xb7, 0x37, 0xbf, 0x5d, 0x49, 0xbb, 0x37,
  0xba, 0x48, 0x69, 0xc4, 0x3b, 0xba, 0x40, 0xd5, 0xd7, 0x42, 0xbd, 0x3f,
  0xc9, 0x69, 0x52, 0xc7, 0x44, 0xc7, 0x52, 0xfa, 0xda, 0x4f, 0xcd, 0x4f,
  0xd8, 0x66, 0x72, 0xdf, 0x59, 0xd4, 0x52, 0xd6, 0x5d, 0xef, 0xde, 0x4f,
  0xca, 0x49, 0xce, 0x64, 0x5a, 0xc8, 0x40, 0xc1, 0x4a, 0xec, 0xcd, 0x3f,
  0xbc, 0x3e, 0xcc, 0xe5, 0x43, 0xba, 0x39, 0xbe, 0x55, 0x4f, 0xbc, 0x37,
  0xb9, 0x43, 0xf8, 0xc4, 0x39, 0xb6, 0x3b, 0xcd, 0xd7, 0x3e, 0xb7, 0x38,
  0xc0, 0x60, 0x4b, 0xbc, 0x39, 0xbc, 0x4b, 0x6b, 0xc6, 0x3d, 0xbd, 0x43,
  0xd9, 0xda, 0x47, 0xc1, 0x42, 0xcd, 0x66, 0x5a, 0xcd, 0x48, 0xcc, 0x54,
  0xe9, 0xe9, 0x59, 0xd5, 0x54, 0xd5, 0x5c, 0xe4, 0xfb, 0x61, 0xd4, 0x4f,
  0xcd, 0x51, 0xdf, 0xe3, 0x4e, 0xc5, 0x45, 0xca, 0x5e, 0x5b, 0xc6, 0x3e,
  0xbf, 0x48, 0xea, 0xcc, 0x3e, 0xbb, 0x3d, 0xcb, 0xe4, 0x42, 0xba, 0x38,
  0xbe, 0x56, 0x4e, 0xbc, 0x37, 0xb9, 0x44, 0x7b, 0xc4, 0x39, 0xb7, 0x3c,
  0xcf, 0xd7, 0x3e, 0xb9, 0x3a, 0xc2, 0x62, 0x4c, 0xbd, 0x3b, 0xbe, 0x4d,
  0x6b, 0xc9, 0x3f, 0xbf, 0x46, 0xd9, 0xde, 0x4b, 0xc5, 0x47, 0xce, 0x63,
  0x65, 0xd3, 0x4e, 0xcf, 0x55, 0xdf, 0x74, 0x67, 0xdc, 0x55, 0xd3, 0x54,
  0xda, 0x68, 0x67, 0xd5, 0x4c, 0xca, 0x4d, 0xdc, 0xe7, 0x4d, 0xc4, 0x42,
  0xc8, 0x5b, 0x59, 0xc4, 0x3d, 0xbe, 0x47, 0xec, 0xcc, 0x3d, 0xba, 0x3d,
  0xcc, 0xe1, 0x42, 0xba, 0x39, 0xbf, 0x5a, 0x4f, 0xbc, 0x38, 0xba, 0x46,
  0x7d, 0xc5, 0x3b, 0xb9, 0x3e, 0xd0, 0xd8, 0x40, 0xbb, 0x3c, 0xc5, 0x63,
  0x4d, 0xc0, 0x3d, 0xc1, 0x4e, 0x6e, 0xcd, 0x44, 0xc4, 0x49, 0xdb, 0xec,
  0x50, 0xcc, 0x4a, 0xd1, 0x5c, 0x7b, 0xde, 0x56, 0xd2, 0x54, 0xd8, 0x62,
  0xf2, 0xe2, 0x58, 0xcf, 0x4e, 0xd0, 0x5d, 0x72, 0xd3, 0x4a, 0xc5, 0x49,
  0xd6, 0xe8, 0x4b, 0xc0, 0x3f, 0xc5, 0x5b, 0x58, 0xc2, 0x3c, 0xbd, 0x47,
  0xee, 0xca, 0x3d, 0xba, 0x3d, 0xcd, 0xdf, 0x41, 0xba, 0x3a, 0xc0, 0x5b,
  0x4d, 0xbd, 0x39, 0xbc, 0x48, 0x73, 0xc6, 0x3c, 0xbb, 0x3f, 0xd4, 0xd9,
  0x43, 0xbd, 0x3e, 0xc8, 0x67, 0x50, 0xc4, 0x40, 0xc4, 0x51, 0x7b, 0xd1,
  0x4a, 0xc8, 0x4d, 0xd9, 0xf6, 0x5c, 0xd0, 0x51, 0xd2, 0x5c, 0xe8, 0xf2,
  0x62, 0xd8, 0x55, 0xd4, 0x56, 0xe1, 0x7c, 0x59, 0xcf, 0x49, 0xcc, 0x53,
  0x7a, 0xd4, 0x46, 0xc4, 0x45, 0xd5, 0xef, 0x49, 0xc0, 0x3d, 0xc5, 0x57,
  0x55, 0xc2, 0x3b, 0xbd, 0x47, 0xed, 0xc9, 0x3d, 0xb9, 0x3e, 0xcc, 0xdb,
  0x43, 0xb9, 0x3b, 0xc0, 0x61, 0x4f, 0xbd, 0x3b, 0xbc, 0x4b, 0x75, 0xc6,
  0x3d, 0xbc, 0x43, 0xd7, 0xd9, 0x45, 0xbf, 0x40, 0xcc, 0x68, 0x54, 0xc9,
  0x44, 0xca, 0x53, 0x7d, 0xda, 0x4d, 0xce, 0x4f, 0xdb, 0x6d, 0x68, 0xdd,
  0x55, 0xd6, 0x56, 0xdc, 0x67, 0x71, 0xde, 0x53, 0xce, 0x4f, 0xd6, 0x6e,
  0x5c, 0xcc, 0x47, 0xc7, 0x4f, 0xef, 0xd0, 0x45, 0xbf, 0x44, 0xcf, 0xe6,
  0x48, 0xbe, 0x3d, 0xc3, 0x5a, 0x54, 0xc0, 0x3b, 0xbc, 0x47, 0xfa, 0xc9,
  0x3c, 0xba, 0x3e, 0xd0, 0xdc, 0x41, 0xbb, 0x3b, 0xc4, 0x5f, 0x4d, 0xbf,
  0x3c, 0xbf, 0x4c, 0x6d, 0xc9, 0x3f, 0xbe, 0x46, 0xda, 0xdc, 0x49, 0xc3,
  0x45, 0xce, 0x6b, 0x5b, 0xcd, 0x4a, 0xcd, 0x57, 0xee, 0xe4, 0x58, 0xd4,
  0x54, 0xd9, 0x60, 0xf1, 0xed, 0x5f, 0xd7, 0x53, 0xd3, 0x5a, 0xeb, 0xe1,
  0x52, 0xca, 0x4b, 0xce, 0x68, 0x5c, 0xc9, 0x44, 0xc4, 0x4e, 0xec, 0xce,
  0x43, 0xbe, 0x42, 0xcf, 0xe4, 0x47, 0xbd, 0x3c, 0xc3, 0x5b, 0x50, 0xbf,
  0x3b, 0xbd, 0x48, 0x73, 0xc8, 0x3c, 0xbb, 0x3f, 0xd4, 0xda, 0x41, 0xbc,
  0x3d, 0xc7, 0x67, 0x4d, 0xc0, 0x3d, 0xc2, 0x4f, 0x68, 0xcb, 0x42, 0xc2,
  0x4a, 0xdd, 0xdd, 0x4d, 0xc7, 0x4a, 0xd1, 0x6d, 0x65, 0xd3, 0x52, 0xd1,
  0x5b, 0xe3, 0xf8, 0x68, 0xdd, 0x5a, 0xd8, 0x59, 0xde, 0x6d, 0x68, 0xd9,
  0x4e, 0xce, 0x4f, 0xe1, 0xeb, 0x4e, 0xc9, 0x45, 0xcd, 0x5d, 0x58, 0xc9,
  0x3f, 0xc2, 0x4b, 0xf6, 0xce, 0x3f, 0xbd, 0x40, 0xcf, 0xe0, 0x45, 0xbc,
  0x3d, 0xc3, 0x5e, 0x51, 0xbf, 0x3c, 0xbd, 0x4b, 0x7b, 0xc7, 0x3e, 0xbb,
  0x42, 0xd4, 0xd6, 0x45, 0xbd, 0x3f, 0xc9, 0x6f, 0x50, 0xc3, 0x40, 0xc5,
  0x53, 0x6c, 0xce, 0x46, 0xc7, 0x4c, 0xe0, 0xeb, 0x51, 0xce, 0x4d, 0xd7,
  0x5f, 0x6c, 0xe3, 0x57, 0xd9, 0x57, 0xde, 0x64, 0xfd, 0xe9, 0x5b, 0xd5,
  0x52, 0xd5, 0x61, 0x78, 0xd6, 0x4d, 0xc9, 0x4e, 0xd8, 0xe5, 0x4e, 0xc4,
  0x44, 0xc9, 0x5f, 0x5a, 0xc6, 0x3f, 0xc0, 0x4b, 0xf5, 0xcd, 0x3f, 0xbd,
  0x40, 0xd2, 0xdf, 0x43, 0xbd, 0x3c, 0xc6, 0x5e, 0x4e, 0xbf, 0x3b, 0xbf,
  0x4c, 0x6b, 0xc9, 0x3e, 0xbd, 0x44, 0xda, 0xd8, 0x45, 0xbf, 0x42, 0xcc,
  0x75, 0x52, 0xc6, 0x45, 0xc8, 0x58, 0x76, 0xd2, 0x4c, 0xca, 0x51, 0xdd,
  0xed, 0x5d, 0xd3, 0x55, 0xd7, 0x61, 0xec, 0xef, 0x65, 0xdc, 0x58, 0xd7,
  0x5a, 0xe4, 0xfd, 0x5c, 0xd2, 0x4c, 0xcf, 0x57, 0x77, 0xd8, 0x49, 0xc7,
  0x48, 0xd8, 0xeb, 0x4b, 0xc3, 0x40, 0xc8, 0x5d, 0x57, 0xc4, 0x3e, 0xbf,
  0x4b, 0xf8, 0xca, 0x3f, 0xbc, 0x42, 0xd1, 0xd9, 0x45, 0xbc, 0x3e, 0xc6,
  0x6a, 0x4f, 0xbf, 0x3d, 0xc0, 0x4f, 0x67, 0xc9, 0x3f, 0xbf, 0x47, 0xdf,
  0xdb, 0x47, 0xc4, 0x44, 0xd1, 0x6c, 0x53, 0xcc, 0x48, 0xce, 0x58, 0x74,
  0xdc, 0x50, 0xd1, 0x54, 0xdf, 0x74, 0x6a, 0xde, 0x5b, 0xd9, 0x5d, 0xdd,
  0x6e, 0xfc, 0xdd, 0x5a, 0xcf, 0x54, 0xd6, 0x7b, 0x60, 0xcd, 0x4b, 0xc9,
  0x55, 0xf2, 0xd2, 0x48, 0xc3, 0x47, 0xd5, 0xe6, 0x4a, 0xc1, 0x3f, 0xc8,
  0x5d, 0x52, 0xc4, 0x3d, 0xc0, 0x4b, 0x71, 0xcb, 0x3e, 0xbd, 0x41, 0xd7,
  0xdc, 0x43, 0xbe, 0x3e, 0xc9, 0x6a, 0x4e, 0xc1, 0x3e, 0xc3, 0x52, 0x6a,
  0xca, 0x43, 0xc1, 0x4b, 0xdd, 0xda, 0x4c, 0xc6, 0x4a, 0xd1, 0x77, 0x5d,
  0xce, 0x4e, 0xcf, 0x5c, 0xf3, 0xe5, 0x5a, 0xd8, 0x58, 0xdd, 0x62, 0xfb,
  0xf4, 0x5e, 0xdc, 0x54, 0xd9, 0x5a, 0xf6, 0xea, 0x52, 0xce, 0x4c, 0xd4,
  0x67, 0x5c, 0xcc, 0x46, 0xc8, 0x50, 0xf8, 0xd0, 0x45, 0xc0, 0x46, 0xd3,
  0xe1, 0x49, 0xbf, 0x3f, 0xc6, 0x63, 0x54, 0xc1, 0x3e, 0xbf, 0x4d, 0x76,
  0xc9, 0x3f, 0xbd, 0x44, 0xd8, 0xda, 0x44, 0xbe, 0x3f, 0xcb, 0x6b, 0x4e,
  0xc4, 0x3f, 0xc7, 0x53, 0x66, 0xcd, 0x45, 0xc6, 0x4d, 0xe3, 0xdf, 0x4e,
  0xcb, 0x4d, 0xd5, 0x6f, 0x65, 0xd6, 0x55, 0xd4, 0x5e, 0xe5, 0xf1, 0x6b,
  0xdc, 0x5d, 0xd8, 0x5e, 0xdf, 0x79, 0x6d, 0xd9, 0x53, 0xcf, 0x56, 0xe3,
  0xe8, 0x52, 0xcb, 0x49, 0xcf, 0x61, 0x5a, 0xcb, 0x43, 0xc6, 0x4d, 0x7c,
  0xd1, 0x42, 0xc0, 0x43, 0xd6, 0xe3, 0x47, 0xbf, 0x3e, 0xc8, 0x63, 0x51,
  0xc1, 0x3e, 0xc0, 0x4e, 0x6f, 0xc9, 0x3f, 0xbe, 0x47, 0xd9, 0xd7, 0x47,
  0xbf, 0x43, 0xcc, 0x79, 0x51, 0xc6, 0x44, 0xc9, 0x58, 0x6a, 0xd0, 0x49,
  0xca, 0x4f, 0xe6, 0xea, 0x54, 0xd1, 0x50, 0xdb, 0x65, 0x6e, 0xe4, 0x5a,
  0xdc, 0x5a, 0xe1, 0x67, 0xfe, 0xea, 0x5d, 0xd7, 0x55, 0xd8, 0x63, 0x74,
  0xd8, 0x4f, 0xcb, 0x4f, 0xdc, 0xe5, 0x50, 0xc7, 0x47, 0xcc, 0x65, 0x5b,
  0xc8, 0x43, 0xc4, 0x4e, 0xfa, 0xcd, 0x42, 0xbf, 0x44, 0xd6, 0xdf, 0x46,
  0xbf, 0x3f, 0xc9, 0x64, 0x4f, 0xc3, 0x3e, 0xc3, 0x4f, 0x68, 0xcb, 0x40,
  0xc0, 0x48, 0xde, 0xd9, 0x48, 0xc2, 0x45, 0xcf, 0x7b, 0x55, 0xc9, 0x48,
  0xcb, 0x5c, 0x75, 0xd3, 0x4f, 0xcd, 0x56, 0xe0, 0xed, 0x5e, 0xd7, 0x58,
  0xdb, 0x63, 0xef, 0xf5, 0x65, 0xdf, 0x5a, 0xdb, 0x5b, 0xea, 0x7d, 0x5d,
  0xd6, 0x4e, 0xd3, 0x59, 0x73, 0xd9, 0x4b, 0xca, 0x4b, 0xdc, 0xeb, 0x4d,
  0xc6, 0x44, 0xcb, 0x61, 0x59, 0xc7, 0x41, 0xc2, 0x4e, 0xfb, 0xcc, 0x42,
  0xbe, 0x46, 0xd5, 0xdb, 0x47, 0xbe, 0x40, 0xc9, 0x6e, 0x50, 0xc2, 0x3f,
  0xc4, 0x52, 0x69, 0xcb, 0x42, 0xc3, 0x4a, 0xe2, 0xdc, 0x49, 0xc6, 0x48,
  0xd4, 0x71, 0x56, 0xcd, 0x4a, 0xcf, 0x5b, 0x73, 0xdd, 0x53, 0xd3, 0x57,
  0xe2, 0x78, 0x6a, 0xdf, 0x5d, 0xdb, 0x5e, 0xe1, 0x6f, 0x7a, 0xe0, 0x5b,
  0xd4, 0x57, 0xdb, 0x79, 0x5f, 0xd0, 0x4d, 0xcd, 0x58, 0xfd, 0xd6, 0x4a,
  0xc7, 0x4a, 0xd9, 0xe8, 0x4c, 0xc5, 0x42, 0xcb, 0x5f, 0x55, 0xc7, 0x3f,
  0xc4, 0x4e, 0x71, 0xcd, 0x41, 0xbf, 0x46, 0xd9, 0xdb, 0x47, 0xbf, 0x41,
  0xcb, 0x70, 0x51, 0xc4, 0x42, 0xc5, 0x57, 0x6b, 0xcc, 0x46, 0xc4, 0x4d,
  0xe0, 0xdb, 0x4d, 0xc8, 0x4c, 0xd5, 0x78, 0x5d, 0xd1, 0x4f, 0xd3, 0x5d,
  0xfb, 0xe6, 0x5a, 0xda, 0x59, 0xe1, 0x67, 0x7c, 0xf1, 0x5f, 0xde, 0x58,
  0xdc, 0x5e, 0xf9, 0xe8, 0x56, 0xd1, 0x4f, 0xd7, 0x6d, 0x5e, 0xce, 0x4a,
  0xcb, 0x55, 0xf7, 0xd3, 0x49, 0xc4, 0x4a, 0xd7, 0xe3, 0x4c, 0xc2, 0x43,
  0xca, 0x66, 0x56, 0xc5, 0x40, 0xc3, 0x4f, 0x74, 0xcc, 0x42, 0xc0, 0x47,
  0xdb, 0xdc, 0x47, 0xc1, 0x42, 0xce, 0x6e, 0x50, 0xc7, 0x43, 0xc9, 0x57,
  0x67, 0xcf, 0x48, 0xc9, 0x4e, 0xe6, 0xe0, 0x50, 0xcd, 0x4e, 0xd8, 0x6f,
  0x65, 0xd7, 0x55, 0xd7, 0x5f, 0xeb, 0xf3, 0x68, 0xde, 0x5e, 0xdc, 0x60,
  0xe5, 0x7b, 0x6b, 0xdc, 0x56, 0xd4, 0x59, 0xe8, 0xe8, 0x55, 0xcd, 0x4c,
  0xd3, 0x68, 0x5c, 0xcd, 0x47, 0xc9, 0x52, 0xfe, 0xd2, 0x47, 0xc4, 0x48,
  0xd8, 0xe2, 0x4a, 0xc2, 0x42, 0xcb, 0x68, 0x54, 0xc5, 0x40, 0xc4, 0x51,
  0x6e, 0xcc, 0x42, 0xc1, 0x49, 0xdd, 0xda, 0x49, 0xc3, 0x46, 0xcf, 0x7a,
  0x53, 0xc8, 0x46, 0xcb, 0x5b, 0x6a, 0xd2, 0x4b, 0xcc, 0x52, 0xe7, 0xe7,
  0x56, 0xd2, 0x53, 0xdc, 0x6a, 0x6d, 0xe2, 0x5b, 0xdc, 0x5e, 0xe5, 0x6d,
  0x7a, 0xea, 0x5f, 0xda, 0x59, 0xdc, 0x68, 0x70, 0xdb, 0x52, 0xcf, 0x53,
  0xe0, 0xe9, 0x53, 0xcb, 0x4a, 0xcf, 0x66, 0x5c, 0xcb, 0x46, 0xc7, 0x51,
  0xfb, 0xcf, 0x46, 0xc2, 0x48, 0xd8, 0xdf, 0x4a, 0xc2, 0x42, 0xcb, 0x69,
  0x53, 0xc5, 0x41, 0xc5, 0x53, 0x6b, 0xcc, 0x44, 0xc3, 0x4a, 0xe0, 0xdb,
  0x4a, 0xc5, 0x48, 0xd2, 0x78, 0x55, 0xcb, 0x49, 0xce, 0x5c, 0x6e, 0xd7,
  0x4f, 0xcf, 0x56, 0xe6, 0xef, 0x5d, 0xd8, 0x59, 0xdc, 0x67, 0xf6, 0xee,
  0x65, 0xdf, 0x5d, 0xdd, 0x61, 0xec, 0xf4, 0x5f, 0xd8, 0x54, 0xd6, 0x5f,
  0x78, 0xda, 0x4f, 0xcc, 0x4f, 0xde, 0xea, 0x50, 0xc9, 0x48, 0xce, 0x64,
  0x5a, 0xca, 0x45, 0xc7, 0x50, 0x7b, 0xcf, 0x45, 0xc3, 0x48, 0xda, 0xde,
  0x4a, 0xc2, 0x43, 0xcc, 0x6d, 0x53, 0xc6, 0x43, 0xc7, 0x56, 0x6a, 0xcd,
  0x46, 0xc5, 0x4d, 0xe2, 0xdc, 0x4c, 0xc8, 0x4b, 0xd5, 0x7a, 0x59, 0xce,
  0x4d, 0xd0, 0x5e, 0x76, 0xdc, 0x55, 0xd4, 0x5a, 0xe5, 0x7d, 0x68, 0xdf,
  0x5d, 0xde, 0x60, 0xe9, 0x73, 0x70, 0xe4, 0x5c, 0xd9, 0x59, 0xe1, 0x7a,
  0x60, 0xd5, 0x4f, 0xd1, 0x5b, 0x7e, 0xd9, 0x4d, 0xca, 0x4d, 0xdb, 0xe8,
  0x4f, 0xc8, 0x47, 0xcd, 0x66, 0x5a, 0xc9, 0x44, 0xc6, 0x51, 0x78, 0xce,
  0x45, 0xc3, 0x49, 0xdb, 0xdd, 0x49, 0xc3, 0x44, 0xce, 0x6f, 0x53, 0xc7,
  0x44, 0xc9, 0x57, 0x69, 0xce, 0x48, 0xc8, 0x4e, 0xe6, 0xde, 0x4e, 0xcb,
  0x4d, 0xd8, 0x78, 0x5d, 0xd2, 0x50, 0xd5, 0x5f, 0xfc, 0xe4, 0x5c, 0xda,
  0x5c, 0xe2, 0x6e, 0x7d, 0xeb, 0x63, 0xde, 0x5d, 0xde, 0x65, 0xf9, 0xe7,
  0x5a, 0xd5, 0x54, 0xdb, 0x6f, 0x5f, 0xd2, 0x4d, 0xce, 0x58, 0x7d, 0xd7,
  0x4b, 0xc9, 0x4c, 0xdb, 0xe7, 0x4e, 0xc6, 0x46, 0xcd, 0x67, 0x58, 0xc8,
  0x44, 0xc7, 0x53, 0x72, 0xce, 0x45, 0xc3, 0x4a, 0xdd, 0xdc, 0x4a, 0xc4,
  0x46, 0xcf, 0x76, 0x54, 0xc9, 0x46, 0xcb, 0x5a, 0x69, 0xd0, 0x4a, 0xcb,
  0x51, 0xe8, 0xe1, 0x52, 0xce, 0x50, 0xdb, 0x72, 0x63, 0xda, 0x56, 0xda,
  0x5f, 0xf1, 0xf3, 0x65, 0xe0, 0x5e, 0xe0, 0x63, 0xec, 0x7c, 0x6a, 0xde,
  0x59, 0xd8, 0x5c, 0xec, 0xe9, 0x58, 0xd0, 0x4f, 0xd7, 0x6c, 0x5f, 0xcf,
  0x4b, 0xcc, 0x56, 0xfc, 0xd5, 0x4a, 0xc7, 0x4b, 0xdb, 0xe4, 0x4d, 0xc6,
  0x46, 0xcd, 0x69, 0x57, 0xc8, 0x44, 0xc7, 0x54, 0x6e, 0xce, 0x46, 0xc5,
  0x4b, 0xdf, 0xdc, 0x4b, 0xc6, 0x48, 0xd2, 0x79, 0x55, 0xcb, 0x49, 0xcd,
  0x5d, 0x6c, 0xd4, 0x4d, 0xcd, 0x55, 0xe8, 0xe6, 0x58, 0xd3, 0x55, 0xdd,
  0x6e, 0x6d, 0xe0, 0x5d, 0xdd, 0x5f, 0xe9, 0x73, 0x75, 0xe9, 0x60, 0xdd,
  0x5c, 0xe0, 0x6c, 0x6e, 0xde, 0x55, 0xd4, 0x57, 0xe6, 0xeb, 0x56, 0xce,
  0x4d, 0xd4, 0x6a, 0x5e, 0xce, 0x49, 0xcb, 0x55, 0xfe, 0xd3, 0x49, 0xc6,
  0x4b, 0xdb, 0xe2, 0x4c, 0xc5, 0x46, 0xce, 0x6c, 0x56, 0xc8, 0x45, 0xc8,
  0x56, 0x6c, 0xce, 0x47, 0xc6, 0x4d, 0xe3, 0xdd, 0x4c, 0xc8, 0x4a, 0xd5,
  0x7a, 0x57, 0xcd, 0x4b, 0xcf, 0x5e, 0x6d, 0xd8, 0x50, 0xd1, 0x58, 0xe9,
  0xee, 0x5d, 0xd9, 0x5a, 0xde, 0x6a, 0xfe, 0xec, 0x65, 0xe0, 0x5f, 0xe0,
  0x67, 0xf0, 0xf1, 0x63, 0xda, 0x58, 0xda, 0x65, 0x78, 0xdc, 0x53, 0xcf,
  0x54, 0xe1, 0xeb, 0x54, 0xcc, 0x4b, 0xd1, 0x68, 0x5d, 0xcd, 0x48, 0xca,
  0x54, 0x7b, 0xd2, 0x48, 0xc6, 0x4b, 0xdc, 0xe0, 0x4c, 0xc6, 0x47, 0xcf,
  0x6e, 0x55, 0xc9, 0x45, 0xca, 0x58, 0x6b, 0xcf, 0x48, 0xc8, 0x4e, 0xe5,
  0xdd, 0x4e, 0xca, 0x4c, 0xd8, 0x7b, 0x5a, 0xcf, 0x4e, 0xd3, 0x60, 0x73,
  0xdd, 0x56, 0xd6, 0x5b, 0xe8, 0xfc, 0x67, 0xdf, 0x5e, 0xdf, 0x65, 0xed,
  0x7b, 0x6e, 0xe5, 0x5e, 0xdc, 0x5d, 0xe6, 0xff, 0x63, 0xd8, 0x53, 0xd5,
  0x5e, 0x7c, 0xdb, 0x4f, 0xcd, 0x50, 0xde, 0xea, 0x52, 0xcb, 0x4a, 0xcf,
  0x68, 0x5c, 0xcc, 0x47, 0xc9, 0x54, 0x79, 0xd1, 0x48, 0xc6, 0x4b, 0xdd,
  0xdf, 0x4c, 0xc6, 0x47, 0xd0, 0x6f, 0x56, 0xca, 0x47, 0xcb, 0x5a, 0x6b,
  0xd1, 0x4a, 0xca, 0x50, 0xe7, 0xdf, 0x50, 0xcd, 0x4f, 0xda, 0x79, 0x5e,
  0xd5, 0x52, 0xd7, 0x62, 0xff, 0xe4, 0x5c, 0xdb, 0x5d, 0xe5, 0x73, 0x77,
  0xea, 0x64, 0xe0, 0x5f, 0xe2, 0x6b, 0xfe, 0xe8, 0x5c, 0xd8, 0x58, 0xde,
  0x76, 0x63, 0xd5, 0x50, 0xd1, 0x5b, 0xff, 0xda, 0x4e, 0xcb, 0x4f, 0xdd,
  0xe9, 0x50, 0xca, 0x49, 0xcf, 0x69, 0x5b, 0xcb, 0x47, 0xc9, 0x55, 0x75,
  0xd1, 0x48, 0xc7, 0x4c, 0xde, 0xde, 0x4c, 0xc7, 0x49, 0xd2, 0x76, 0x57,
  0xcb, 0x49, 0xcd, 0x5c, 0x6b, 0xd3, 0x4c, 0xcd, 0x54, 0xe9, 0xe3, 0x54,
  0xcf, 0x52, 0xdc, 0x75, 0x64, 0xda, 0x58, 0xdb, 0x63, 0xf4, 0xee, 0x65,
  0xe0, 0x5f, 0xe3, 0x68, 0xf1, 0xf9, 0x6a, 0xe0, 0x5d, 0xdc, 0x60, 0xef,
  0xeb, 0x5b, 0xd5, 0x54, 0xda, 0x6d, 0x62, 0xd3, 0x4e, 0xcf, 0x59, 0xfd,
  0xd9, 0x4d, 0xca, 0x4e, 0xdd, 0xe8, 0x4f, 0xc9, 0x49, 0xcf, 0x69, 0x5a,
  0xcb, 0x47, 0xca, 0x56, 0x73, 0xd1, 0x49, 0xc7, 0x4d, 0xdf, 0xdf, 0x4d,
  0xc8, 0x4a, 0xd4, 0x77, 0x58, 0xcd, 0x4b, 0xcf, 0x5d, 0x6d, 0xd6, 0x4e,
  0xcf, 0x56, 0xea, 0xe9, 0x59, 0xd5, 0x56, 0xde, 0x6f, 0x6d, 0xdf, 0x5d,
  0xdd, 0x62, 0xeb, 0x7c, 0x71, 0xe8, 0x62, 0xdf, 0x60, 0xe5, 0x73, 0x6f,
  0xdf, 0x5a, 0xd7, 0x5c, 0xe9, 0xec, 0x5a, 0xd1, 0x50, 0xd7, 0x6c, 0x61,
  0xd1, 0x4c, 0xcd, 0x58, 0xfd, 0xd7, 0x4c, 0xc9, 0x4d, 0xdd, 0xe7, 0x4f,
  0xc9, 0x49, 0xcf, 0x6b, 0x59, 0xcb, 0x47, 0xcb, 0x57, 0x6f, 0xd1, 0x49,
  0xc9, 0x4e, 0xe2, 0xdf, 0x4e, 0xca, 0x4c, 0xd6, 0x78, 0x5a, 0xcf, 0x4d,
  0xd1, 0x5f, 0x70, 0xda, 0x52, 0xd3, 0x59, 0xe9, 0xef, 0x5e, 0xda, 0x5a,
  0xdf, 0x6b, 0x7b, 0xeb, 0x63, 0xe1, 0x61, 0xe5, 0x6b, 0xfa, 0xf0, 0x64,
  0xdd, 0x5b, 0xde, 0x68, 0x75, 0xdf, 0x56, 0xd4, 0x58, 0xe4, 0xed, 0x58,
  0xcf, 0x4e, 0xd5, 0x6a, 0x60, 0xcf, 0x4b, 0xcc, 0x57, 0xfd, 0xd6, 0x4b,
  0xc9, 0x4d, 0xdd, 0xe5, 0x4f, 0xc9, 0x49, 0xd0, 0x6d, 0x5a, 0xcc, 0x48,
  0xcc, 0x59, 0x6f, 0xd3, 0x4b, 0xca, 0x4f, 0xe5, 0xe1, 0x50, 0xcc, 0x4e,
  0xd9, 0x77, 0x5d, 0xd2, 0x4f, 0xd5, 0x60, 0x77, 0xde, 0x57, 0xd7, 0x5c,
  0xe8, 0xfa, 0x67, 0xdf, 0x5e, 0xe0, 0x67, 0xf0, 0xfc, 0x6d, 0xe5, 0x5f,
  0xdf, 0x61, 0xeb, 0xfb, 0x65, 0xdb, 0x57, 0xd9, 0x61, 0x7c, 0xde, 0x54,
  0xd0, 0x54, 0xe1, 0xed, 0x56, 0xcd, 0x4d, 0xd3, 0x69, 0x5f, 0xce, 0x4b,
  0xcc, 0x57, 0x7d, 0xd5, 0x4b, 0xc9, 0x4e, 0xde, 0xe4, 0x4f, 0xc9, 0x4a,
  0xd2, 0x6f, 0x5a, 0xcc, 0x4a, 0xcd, 0x5b, 0x6f, 0xd4, 0x4c, 0xcc, 0x52,
  0xe7, 0xe4, 0x53, 0xce, 0x50, 0xdb, 0x76, 0x60, 0xd6, 0x54, 0xd8, 0x62,
  0xff, 0xe5, 0x5d, 0xdc, 0x5e, 0xe7, 0x75, 0x73, 0xe9, 0x64, 0xe2, 0x63,
  0xe7, 0x6f, 0x7b, 0xe9, 0x5f, 0xdb, 0x5c, 0xe2, 0x78, 0x65, 0xd9, 0x54,
  0xd6, 0x5e, 0xfe, 0xdd, 0x51, 0xce, 0x52, 0xdf, 0xec, 0x54, 0xcd, 0x4c,
  0xd2, 0x69, 0x5d, 0xce, 0x4a, 0xcc, 0x57, 0x7a, 0xd5, 0x4b, 0xc9, 0x4e,
  0xdf, 0xe3, 0x4f, 0xca, 0x4b, 0xd4, 0x70, 0x5a, 0xcd, 0x4b, 0xce, 0x5c,
  0x6f, 0xd6, 0x4e, 0xce, 0x55, 0xe8, 0xe7, 0x57, 0xd2, 0x54, 0xdd, 0x73,
  0x66, 0xdb, 0x59, 0xdb, 0x63, 0xf5, 0xee, 0x65, 0xe0, 0x60, 0xe5, 0x6b,
  0xf7, 0xf6, 0x69, 0xe2, 0x5f, 0xdf, 0x65, 0xf5, 0xeb, 0x5d, 0xd8, 0x58,
  0xdd, 0x71, 0x65, 0xd7, 0x51, 0xd2, 0x5c, 0xfd, 0xdb, 0x4f, 0xcd, 0x50,
  0xde, 0xea, 0x53, 0xcc, 0x4c, 0xd2, 0x6a, 0x5d, 0xce, 0x4a, 0xcc, 0x58,
  0x78, 0xd4, 0x4b, 0xca, 0x4f, 0xe1, 0xe3, 0x4f, 0xcb, 0x4c, 0xd6, 0x74,
  0x5b, 0xcf, 0x4d, 0xd0, 0x5e, 0x70, 0xd9, 0x50, 0xd0, 0x58, 0xea, 0xeb,
  0x5b, 0xd6, 0x58, 0xdf, 0x6f, 0x6d, 0xe1, 0x5d, 0xde, 0x64, 0xed, 0xff,
  0x6e, 0xe8, 0x63, 0xe2, 0x64, 0xe9, 0x77, 0x6e, 0xe2, 0x5c, 0xdb, 0x5e,
  0xec, 0xed, 0x5c, 0xd5, 0x54, 0xda, 0x6d, 0x64, 0xd5, 0x4f, 0xd0, 0x5a,
  0xfd, 0xda, 0x4e, 0xcc, 0x4f, 0xde, 0xe9, 0x52, 0xcb, 0x4b, 0xd3, 0x6c,
  0x5c, 0xce, 0x4a, 0xcd, 0x5a, 0x74, 0xd5, 0x4c, 0xcb, 0x50, 0xe4, 0xe3,
  0x51, 0xcc, 0x4e, 0xd8, 0x77, 0x5c, 0xd1, 0x4f, 0xd4, 0x60, 0x72, 0xdc,
  0x55, 0xd5, 0x5b, 0xeb, 0xef, 0x5f, 0xdb, 0x5c, 0xe1, 0x6d, 0x7a, 0xeb,
  0x65, 0xe3, 0x64, 0xe8, 0x6e, 0xfc, 0xef, 0x66, 0xdf, 0x5e, 0xe0, 0x6b,
  0x76, 0xe0, 0x59, 0xd7, 0x5a, 0xe8, 0xee, 0x5a, 0xd2, 0x51, 0xd8, 0x6c,
  0x62, 0xd3, 0x4e, 0xcf, 0x5a, 0xff, 0xd9, 0x4e, 0xcc, 0x4f, 0xdf, 0xe7,
  0x51, 0xcb, 0x4c, 0xd4, 0x6e, 0x5c, 0xce, 0x4b, 0xce, 0x5b, 0x71, 0xd5,
  0x4d, 0xcd, 0x53, 0xe7, 0xe3, 0x53, 0xce, 0x50, 0xdb, 0x78, 0x5e, 0xd5,
  0x52, 0xd7, 0x63, 0x78, 0xdf, 0x5a, 0xd9, 0x5e, 0xea, 0xf9, 0x69, 0xe1,
  0x60, 0xe3, 0x6a, 0xf2, 0xfa, 0x6e, 0xe7, 0x63, 0xe1, 0x65, 0xed, 0xfa,
  0x67, 0xdd, 0x5a, 0xdc, 0x65, 0x7b, 0xdf, 0x57, 0xd4, 0x57, 0xe4, 0xee,
  0x59, 0xd0, 0x4f, 0xd6, 0x6b, 0x60, 0xd2, 0x4d, 0xce, 0x59, 0x7d, 0xd8,
  0x4d, 0xcc, 0x4f, 0xe0, 0xe7, 0x51, 0xcc, 0x4c, 0xd5, 0x6f, 0x5b, 0xce,
  0x4c, 0xcf, 0x5c, 0x70, 0xd7, 0x4e, 0xce, 0x55, 0xe9, 0xe5, 0x56, 0xd1,
  0x53, 0xdd, 0x78, 0x62, 0xd9, 0x57, 0xda, 0x66, 0x7e, 0xe6, 0x5e, 0xde,
  0x60, 0xe9, 0x78, 0x73, 0xea, 0x66, 0xe4, 0x66, 0xea, 0x71, 0x7a, 0xea,
  0x61, 0xde, 0x5e, 0xe5, 0x7a, 0x67, 0xdb, 0x57, 0xd8, 0x60, 0x7e, 0xde,
  0x54, 0xd1, 0x55, 0xe2, 0xed, 0x57, 0xcf, 0x4e, 0xd6, 0x6b, 0x5f, 0xd0,
  0x4c, 0xce, 0x5a, 0x7a, 0xd8, 0x4d, 0xcc, 0x50, 0xe3, 0xe5, 0x51, 0xcc,
  0x4d, 0xd7, 0x74, 0x5c, 0xcf, 0x4d, 0xd1, 0x5e, 0x6f, 0xd9, 0x50, 0xd0,
  0x58, 0xea, 0xe7, 0x59, 0xd4, 0x57, 0xde, 0x77, 0x68, 0xdd, 0x5b, 0xdd,
  0x66, 0xf7, 0xee, 0x67, 0xe3, 0x64, 0xe7, 0x6d, 0xf9, 0xf5, 0x6b, 0xe5,
  0x62, 0xe2, 0x68, 0xf6, 0xed, 0x5f, 0xdb, 0x5a, 0xdf, 0x72, 0x67, 0xd9,
  0x54, 0xd5, 0x5e, 0xfd, 0xdd, 0x52, 0xcf, 0x53, 0xe1, 0xed, 0x56, 0xce,
  0x4e, 0xd5, 0x6b, 0x5e, 0xd0, 0x4c, 0xce, 0x5a, 0x77, 0xd7, 0x4d, 0xcc,
  0x52, 0xe4, 0xe5, 0x53, 0xcd, 0x4e, 0xd8, 0x76, 0x5d, 0xd1, 0x4f, 0xd3,
  0x5f, 0x71, 0xdb, 0x53, 0xd4, 0x5a, 0xec, 0xec, 0x5d, 0xd9, 0x5a, 0xe1,
  0x72, 0x6e, 0xe3, 0x5f, 0xe0, 0x66, 0xef, 0xfe, 0x6f, 0xe9, 0x66, 0xe5,
  0x67, 0xec, 0x79, 0x70, 0xe5, 0x5e, 0xdd, 0x60, 0xee, 0xee, 0x5e, 0xd8,
  0x57, 0xdc, 0x6f, 0x67, 0xd8, 0x52, 0xd3, 0x5d, 0xfd, 0xdc, 0x51, 0xce,
  0x53, 0xe0, 0xeb, 0x55, 0xce, 0x4e, 0xd6, 0x6d, 0x5e, 0xd0, 0x4c, 0xcf,
  0x5b, 0x75, 0xd8, 0x4e, 0xcd, 0x53, 0xe6, 0xe5, 0x54, 0xce, 0x50, 0xda,
  0x78, 0x5e, 0xd4, 0x51, 0xd6, 0x63, 0x74, 0xdd, 0x57, 0xd7, 0x5d, 0xec,
  0xf0, 0x61, 0xdd, 0x5e, 0xe4, 0x6f, 0x7a, 0xec, 0x67, 0xe5, 0x67, 0xea,
  0x70, 0xfe, 0xf0, 0x68, 0xe2, 0x61, 0xe3, 0x6d, 0x77, 0xe4, 0x5c, 0xda,
  0x5d, 0xea, 0xef, 0x5d, 0xd6, 0x54, 0xdb, 0x6c, 0x65, 0xd6, 0x50, 0xd2,
  0x5c, 0xfe, 0xdc, 0x50, 0xce, 0x52, 0xe1, 0xea, 0x55, 0xcd, 0x4e, 0xd6,
  0x6e, 0x5e, 0xd0, 0x4d, 0xcf, 0x5d, 0x74, 0xd8, 0x4f, 0xce, 0x55, 0xe8,
  0xe6, 0x56, 0xd0, 0x53, 0xdc, 0x78, 0x60, 0xd7, 0x55, 0xd9, 0x65, 0x78,
  0xe2, 0x5b, 0xdb, 0x5f, 0xec, 0xfa, 0x69, 0xe3, 0x62, 0xe6, 0x6b, 0xf6,
  0xfa, 0x6e, 0xe9, 0x66, 0xe5, 0x68, 0xee, 0xfb, 0x69, 0xdf, 0x5d, 0xde,
  0x68, 0x7c, 0xe3, 0x5a, 0xd7, 0x5a, 0xe6, 0xef, 0x5c, 0xd3, 0x52, 0xd9,
  0x6c, 0x64, 0xd5, 0x4f, 0xd1, 0x5b, 0xff, 0xdb, 0x4f, 0xce, 0x53, 0xe2,
  0xe9, 0x55, 0xce, 0x4e, 0xd7, 0x70, 0x5e, 0xd1, 0x4e, 0xd1, 0x5e, 0x73,
  0xd9, 0x50, 0xd0, 0x58, 0xea, 0xe7, 0x58, 0xd3, 0x56, 0xde, 0x78, 0x64,
  0xdb, 0x59, 0xdc, 0x67, 0x7d, 0xe8, 0x5f, 0xdf, 0x62, 0xeb, 0x79, 0x72,
  0xeb, 0x67, 0xe7, 0x68, 0xec, 0x74, 0x79, 0xec, 0x64, 0xe0, 0x60, 0xe8,
  0x7a, 0x6a, 0xde, 0x5a, 0xdb, 0x63, 0xfe, 0xe1, 0x58, 0xd4, 0x58, 0xe4,
  0xef, 0x5a, 0xd2, 0x51, 0xd8, 0x6c, 0x63, 0xd4, 0x4f, 0xd0, 0x5c, 0x7d,
  0xda, 0x4f, 0xce, 0x53, 0xe3, 0xe8, 0x55, 0xce, 0x4f, 0xd9, 0x73, 0x5e,
  0xd2, 0x4f, 0xd4, 0x5f, 0x72, 0xdb, 0x53, 0xd3, 0x5a, 0xeb, 0xea, 0x5b,
  0xd7, 0x59, 0xe0, 0x75, 0x6a, 0xde, 0x5d, 0xde, 0x68, 0xf9, 0xef, 0x67,
  0xe5, 0x65, 0xe9, 0x6f, 0xfb, 0xf5, 0x6c, 0xe7, 0x64, 0xe5, 0x6a, 0xf8,
  0xee, 0x62, 0xdd, 0x5d, 0xe2, 0x73, 0x6a, 0xdc, 0x57, 0xd8, 0x5f, 0xfb,
  0xe0, 0x56, 0xd2, 0x57, 0xe2, 0xee, 0x59, 0xd1, 0x50, 0xd8, 0x6d, 0x62,
  0xd3, 0x4e, 0xd0, 0x5c, 0x7a, 0xda, 0x4f, 0xce, 0x54, 0xe5, 0xe8, 0x56,
  0xcf, 0x50, 0xda, 0x75, 0x5f, 0xd4, 0x51, 0xd6, 0x62, 0x74, 0xdd, 0x56,
  0xd6, 0x5c, 0xec, 0xed, 0x5e, 0xdb, 0x5c, 0xe3, 0x73, 0x6e, 0xe5, 0x60,
  0xe3, 0x68, 0xf1, 0xfd, 0x6f, 0xeb, 0x67, 0xe7, 0x69, 0xee, 0x7a, 0x71,
  0xe7, 0x61, 0xdf, 0x64, 0xef, 0xef, 0x60, 0xda, 0x5a, 0xde, 0x70, 0x69,
  0xda, 0x55, 0xd6, 0x5e, 0xfb, 0xde, 0x55, 0xd1, 0x56, 0xe2, 0xed, 0x59,
  0xd0, 0x50, 0xd8, 0x6d, 0x60, 0xd3, 0x4f, 0xd1, 0x5d, 0x79, 0xda, 0x50,
  0xcf, 0x56, 0xe7, 0xe8, 0x57, 0xd1, 0x53, 0xdc, 0x77, 0x60, 0xd7, 0x54,
  0xd8, 0x64, 0x76, 0xdf, 0x59, 0xd9, 0x5e, 0xed, 0xf2, 0x64, 0xde, 0x5f,
  0xe5, 0x6f, 0x79, 0xec, 0x68, 0xe7, 0x68, 0xec, 0x73, 0x7e, 0xf1, 0x6a,
  0xe5, 0x63, 0xe7, 0x6f, 0x77, 0xe7, 0x5e, 0xdc, 0x5e, 0xeb, 0xf2, 0x5f,
  0xd9, 0x57, 0xdc, 0x6d, 0x68, 0xd9, 0x53, 0xd5, 0x5d, 0xfc, 0xde, 0x53,
  0xd0, 0x55, 0xe3, 0xed, 0x58, 0xd0, 0x50, 0xd8, 0x6e, 0x60, 0xd3, 0x4f,
  0xd2, 0x5e, 0x77, 0xdb, 0x52, 0xd1, 0x57, 0xe8, 0xe9, 0x59, 0xd3, 0x55,
  0xdd, 0x78, 0x64, 0xd9, 0x57, 0xdb, 0x66, 0x7b, 0xe4, 0x5d, 0xdc, 0x61,
  0xec, 0xfa, 0x6b, 0xe4, 0x64, 0xe7, 0x6d, 0xf7, 0xf8, 0x6f, 0xea, 0x68,
  0xe8, 0x6a, 0xf2, 0xfa, 0x6b, 0xe3, 0x5f, 0xe1, 0x69, 0x7c, 0xe6, 0x5c,
  0xda, 0x5c, 0xe9, 0xf3, 0x5e, 0xd7, 0x55, 0xdb, 0x6c, 0x67, 0xd8, 0x52,
  0xd4, 0x5d, 0xfd, 0xdd, 0x53, 0xd0, 0x55, 0xe3, 0xec, 0x58, 0xd0, 0x50,
  0xd9, 0x6f, 0x61, 0xd4, 0x50, 0xd4, 0x5f, 0x77, 0xdc, 0x54, 0xd3, 0x59,
  0xea, 0xea, 0x5b, 0xd6, 0x58, 0xdf, 0x77, 0x67, 0xdc, 0x5a, 0xdd, 0x68,
  0xfe, 0xea, 0x62, 0xdf, 0x64, 0xeb, 0x7b, 0x73, 0xeb, 0x68, 0xe8, 0x6a,
  0xee, 0x77, 0x79, 0xed, 0x67, 0xe3, 0x64, 0xeb, 0x7c, 0x6b, 0xe0, 0x5c,
  0xdd, 0x65, 0xfe, 0xe5, 0x5a, 0xd8, 0x5a, 0xe6, 0xf3, 0x5d, 0xd5, 0x54,
  0xda, 0x6d, 0x67, 0xd7, 0x51, 0xd3, 0x5d, 0xfe, 0xdd, 0x53, 0xd0, 0x56,
  0xe5, 0xeb, 0x58, 0xd1, 0x52, 0xda, 0x71, 0x61, 0xd6, 0x52, 0xd6, 0x61,
  0x77, 0xdd, 0x56, 0xd5, 0x5b, 0xeb, 0xec, 0x5d, 0xd9, 0x5a, 0xe2, 0x75,
  0x6b, 0xe0, 0x5e, 0xe0, 0x68, 0xf9, 0xf0, 0x68, 0xe5, 0x66, 0xeb, 0x70,
  0xfe, 0xf4, 0x6c, 0xe9, 0x67, 0xe8, 0x6d, 0xfa, 0xef, 0x65, 0xdf, 0x5f,
  0xe6, 0x75, 0x6b, 0xde, 0x5a, 0xdb, 0x62, 0xfc, 0xe3, 0x59, 0xd6, 0x59,
  0xe5, 0xf2, 0x5c, 0xd4, 0x53, 0xda, 0x6d, 0x65, 0xd7, 0x51, 0xd4, 0x5e,
  0x7e, 0xdd, 0x53, 0xd1, 0x57, 0xe6, 0xeb, 0x59, 0xd2, 0x53, 0xdc, 0x75,
  0x62, 0xd7, 0x54, 0xd8, 0x64, 0x78, 0xdf, 0x59, 0xd8, 0x5d, 0xec, 0xee,
  0x60, 0xdc, 0x5d, 0xe4, 0x74, 0x70, 0xe6, 0x63, 0xe3, 0x6a, 0xf2, 0xfc,
  0x70, 0xeb, 0x69, 0xe9, 0x6b, 0xf0, 0x7d, 0x72, 0xe9, 0x64, 0xe3, 0x67,
  0xf2, 0xf2, 0x63, 0xdd, 0x5c, 0xe1, 0x70, 0x6b, 0xdd, 0x58, 0xd9, 0x5f,
  0xfb, 0xe2, 0x58, 0xd4, 0x58, 0xe5, 0xf1, 0x5b, 0xd4, 0x53, 0xda, 0x6d,
  0x64, 0xd6, 0x51, 0xd4, 0x5e, 0x7b, 0xdd, 0x54, 0xd2, 0x58, 0xe8, 0xeb,
  0x5a, 0xd4, 0x55, 0xdd, 0x76, 0x64, 0xd9, 0x57, 0xda, 0x65, 0x79, 0xe2,
  0x5b, 0xdb, 0x5f, 0xed, 0xf3, 0x66, 0xdf, 0x61, 0xe7, 0x71, 0x7a, 0xed,
  0x69, 0xe8, 0x6a, 0xed, 0x76, 0x7e, 0xf1, 0x6b, 0xe7, 0x66, 0xea, 0x71,
  0x77, 0xe9, 0x60, 0xde, 0x61, 0xed, 0xf5, 0x61, 0xdb, 0x5a, 0xde, 0x6e,
  0x6b, 0xdc, 0x57, 0xd8, 0x5f, 0xfb, 0xe1, 0x57, 0xd4, 0x57, 0xe5, 0xef,
  0x5b, 0xd3, 0x53, 0xda, 0x6e, 0x64, 0xd7, 0x52, 0xd5, 0x5f, 0x7b, 0xdd,
  0x55, 0xd3, 0x59, 0xe9, 0xeb, 0x5b, 0xd6, 0x58, 0xde, 0x77, 0x67, 0xdb,
  0x59, 0xdc, 0x68, 0x7d, 0xe6, 0x5f, 0xde, 0x63, 0xed, 0xfb, 0x6b, 0xe6,
  0x65, 0xe8, 0x6e, 0xfa, 0xf8, 0x6e, 0xeb, 0x69, 0xea, 0x6c, 0xf5, 0xfa,
  0x6c, 0xe5, 0x61, 0xe4, 0x6c, 0x7c, 0xe8, 0x5e, 0xdc, 0x5e, 0xea, 0xf4,
  0x60, 0xd9, 0x58, 0xdd, 0x6e, 0x6a, 0xdb, 0x55, 0xd7, 0x5e, 0xfc, 0xdf,
  0x56, 0xd3, 0x58, 0xe5, 0xee, 0x5b, 0xd3, 0x54, 0xdb, 0x6f, 0x64, 0xd7,
  0x53, 0xd7, 0x60, 0x79, 0xde, 0x56, 0xd5, 0x5b, 0xeb, 0xed, 0x5d, 0xd8,
  0x5a, 0xe1, 0x76, 0x69, 0xde, 0x5c, 0xde, 0x69, 0xfd, 0xeb, 0x64, 0xe2,
  0x66, 0xed, 0x7c, 0x74, 0xec, 0x6a, 0xe9, 0x6c, 0xef, 0x7a, 0x78, 0xee,
  0x68, 0xe6, 0x67, 0xed, 0x7c, 0x6d, 0xe3, 0x5e, 0xdf, 0x68, 0xfe, 0xe7,
  0x5d, 0xda, 0x5d, 0xe8, 0xf4, 0x5f, 0xd8, 0x57, 0xdc, 0x6e, 0x69, 0xda,
  0x55, 0xd6, 0x5e, 0xfd, 0xdf, 0x56, 0xd3, 0x58, 0xe6, 0xed, 0x5a, 0xd4,
  0x54, 0xdc, 0x72, 0x64, 0xd8, 0x55, 0xd8, 0x63, 0x78, 0xdf, 0x58, 0xd8,
  0x5d, 0xed, 0xee, 0x5f, 0xdb, 0x5c, 0xe4, 0x76, 0x6d, 0xe2, 0x5f, 0xe2,
  0x6a, 0xfa, 0xf1, 0x6a, 0xe7, 0x68, 0xec, 0x72, 0x7e, 0xf5, 0x6e, 0xea,
  0x69, 0xea, 0x6e, 0xfc, 0xf0, 0x68, 0xe2, 0x62, 0xe7, 0x77, 0x6d, 0xe0,
  0x5d, 0xdd, 0x65, 0xfb, 0xe6, 0x5b, 0xd9, 0x5b, 0xe7, 0xf4, 0x5e, 0xd7,
  0x56, 0xdc, 0x6e, 0x68, 0xd9, 0x54, 0xd6, 0x5f, 0xfe, 0xdf, 0x56, 0xd4,
  0x59, 0xe7, 0xed, 0x5b, 0xd5, 0x56, 0xdd, 0x73, 0x65, 0xda, 0x57, 0xda,
  0x65, 0x79, 0xe2, 0x5b, 0xda, 0x5f, 0xed, 0xf1, 0x63, 0xde, 0x5f, 0xe6,
  0x75, 0x72, 0xe8, 0x64, 0xe6, 0x6b, 0xf4, 0xfb, 0x70, 0xec, 0x6a, 0xeb,
  0x6d, 0xf3, 0x7e, 0x72, 0xeb, 0x66, 0xe5, 0x69, 0xf5, 0xf3, 0x66, 0xdf,
  0x5f, 0xe4, 0x73, 0x6d, 0xdf, 0x5b, 0xdc, 0x63, 0xfb, 0xe5, 0x5a, 0xd7,
  0x5a, 0xe6, 0xf3, 0x5d, 0xd7, 0x56, 0xdc, 0x6e, 0x67, 0xd9, 0x55, 0xd7,
  0x5f, 0x7e, 0xdf, 0x57, 0xd5, 0x5a, 0xe9, 0xed, 0x5c, 0xd6, 0x58, 0xde,
  0x76, 0x67, 0xdb, 0x59, 0xdc, 0x66, 0x7b, 0xe5, 0x5d, 0xdc, 0x61, 0xee,
  0xf6, 0x67, 0xe2, 0x62, 0xe8, 0x71, 0x7a, 0xee, 0x69, 0xe9, 0x6b, 0xef,
  0x78, 0x7c, 0xf0, 0x6c, 0xe9, 0x69, 0xec, 0x76, 0x77, 0xea, 0x63, 0xe1,
  0x65, 0xee, 0xf4, 0x65, 0xdd, 0x5d, 0xe1, 0x70, 0x6d, 0xde, 0x59, 0xda,
  0x61, 0xfb, 0xe4, 0x59, 0xd7, 0x5a, 0xe6, 0xf2, 0x5d, 0xd6, 0x56, 0xdc,
  0x6e, 0x67, 0xd9, 0x55, 0xd8, 0x61, 0x7c, 0xdf, 0x58, 0xd6, 0x5b, 0xea,
  0xee, 0x5d, 0xd8, 0x59, 0xe0, 0x76, 0x69, 0xdd, 0x5b, 0xde, 0x68, 0x7d,
  0xe9, 0x5f, 0xdf, 0x63, 0xee, 0xfc, 0x6c, 0xe7, 0x66, 0xe9, 0x6f, 0xfb,
  0xf7, 0x6e, 0xec, 0x6a, 0xec, 0x6e, 0xf8, 0xf9, 0x6d, 0xe7, 0x65, 0xe7,
  0x6e, 0x7c, 0xea, 0x61, 0xde, 0x61, 0xec, 0xf6, 0x64, 0xdc, 0x5b, 0xdf,
  0x6f, 0x6c, 0xdd, 0x58, 0xd9, 0x61, 0xfb, 0xe3, 0x59, 0xd6, 0x5a, 0xe7,
  0xf1, 0x5d, 0xd6, 0x56, 0xdd, 0x6f, 0x67, 0xda, 0x56, 0xd9, 0x62, 0x7c,
  0xe0, 0x59, 0xd8, 0x5d, 0xeb, 0xee, 0x5f, 0xda, 0x5b, 0xe2, 0x76, 0x6b,
  0xe0, 0x5d, 0xe0, 0x6a, 0xfe, 0xed, 0x65, 0xe4, 0x67, 0xee, 0x7b, 0x73,
  0xed, 0x6a, 0xeb, 0x6d, 0xf2, 0x7b, 0x77, 0xee, 0x6a, 0xe9, 0x6a, 0xef,
  0x7e, 0x6e, 0xe6, 0x61, 0xe2, 0x6a, 0xfe, 0xe9, 0x5f, 0xdc, 0x5f, 0xea,
  0xf7, 0x62, 0xdb, 0x5a, 0xde, 0x6e, 0x6c, 0xdc, 0x58, 0xd9, 0x60, 0xfc,
  0xe2, 0x59, 0xd6, 0x5a, 0xe7, 0xef, 0x5d, 0xd7, 0x57, 0xde, 0x72, 0x67,
  0xdb, 0x57, 0xda, 0x64, 0x7b, 0xe2, 0x5b, 0xda, 0x5e, 0xed, 0xf0, 0x61,
  0xdd, 0x5e, 0xe5, 0x77, 0x6e, 0xe4, 0x61, 0xe3, 0x6c, 0xf9, 0xf1, 0x6b,
  0xe8, 0x6a, 0xed, 0x75, 0x7e, 0xf5, 0x6e, 0xec, 0x6b, 0xec, 0x71, 0xfe,
  0xf1, 0x69, 0xe5, 0x65, 0xea, 0x78, 0x6e, 0xe4, 0x5e, 0xdf, 0x67, 0xfc,
  0xe9, 0x5e, 0xdb, 0x5d, 0xe9, 0xf6, 0x61, 0xda, 0x59, 0xde, 0x6e, 0x6b,
  0xdc, 0x57, 0xd9, 0x61, 0xfd, 0xe2, 0x59, 0xd7, 0x5b, 0xe9, 0xef, 0x5d,
  0xd8, 0x58, 0xdf, 0x73, 0x68, 0xdc, 0x59, 0xdc, 0x66, 0x7b, 0xe4, 0x5c,
  0xdc, 0x60, 0xee, 0xf4, 0x65, 0xdf, 0x60, 0xe7, 0x75, 0x73, 0xe9, 0x66,
  0xe7, 0x6c, 0xf5, 0xfa, 0x71, 0xed, 0x6b, 0xec, 0x6e, 0xf6, 0xfe, 0x73,
  0xec, 0x68, 0xe9, 0x6b, 0xf7, 0xf5, 0x68, 0xe2, 0x61, 0xe7, 0x73, 0x6e,
  0xe2, 0x5d, 0xde, 0x65, 0xfa, 0xe8, 0x5d, 0xda, 0x5d, 0xe8, 0xf6, 0x5f,
  0xd9, 0x59, 0xdd, 0x6f, 0x6a, 0xdc, 0x58, 0xd9, 0x62, 0xfe, 0xe2, 0x59,
  0xd7, 0x5c, 0xe9, 0xef, 0x5e, 0xd9, 0x5a, 0xdf, 0x75, 0x69, 0xdd, 0x5b,
  0xdd, 0x68, 0x7d, 0xe7, 0x5f, 0xde, 0x63, 0xee, 0xf7, 0x69, 0xe4, 0x64,
  0xe9, 0x72, 0x7b, 0xee, 0x6b, 0xea, 0x6c, 0xf0, 0x79, 0x7b, 0xf2, 0x6d,
  0xeb, 0x6b, 0xee, 0x76, 0x78, 0xec, 0x66, 0xe4, 0x67, 0xf0, 0xf7, 0x67,
  0xdf, 0x5e, 0xe4, 0x70, 0x6e, 0xe0, 0x5c, 0xdc, 0x63, 0xfa, 0xe7, 0x5c,
  0xd9, 0x5c, 0xe8, 0xf5, 0x5f, 0xd9, 0x58, 0xde, 0x6f, 0x6a, 0xdc, 0x58,
  0xda, 0x62, 0x7e, 0xe3, 0x5a, 0xd9, 0x5d, 0xeb, 0xf0, 0x5f, 0xdb, 0x5b,
  0xe2, 0x74, 0x6b, 0xdf, 0x5d, 0xdf, 0x69, 0x7e, 0xea, 0x63, 0xe1, 0x65,
  0xee, 0xfd, 0x6e, 0xe8, 0x68, 0xea, 0x6f, 0xfc, 0xf8, 0x6f, 0xed, 0x6c,
  0xed, 0x70, 0xf9, 0xf9, 0x6e, 0xe9, 0x68, 0xe9, 0x6f, 0x7c, 0xec, 0x64,
  0xe1, 0x64, 0xed, 0xf8, 0x66, 0xde, 0x5d, 0xe1, 0x6f, 0x6e, 0xdf, 0x5b,
  0xdc, 0x63, 0xfb, 0xe6, 0x5b, 0xd9, 0x5c, 0xe8, 0xf4, 0x5f, 0xd9, 0x59,
  0xde, 0x70, 0x6a, 0xdc, 0x59, 0xdb, 0x64, 0x7e, 0xe3, 0x5b, 0xda, 0x5e,
  0xec, 0xf0, 0x61, 0xdc, 0x5d, 0xe4, 0x75, 0x6d, 0xe2, 0x5f, 0xe1, 0x6b,
  0xfb, 0xee, 0x67, 0xe5, 0x68, 0xee, 0x7c, 0x75, 0xed, 0x6b, 0xec, 0x6e,
  0xf4, 0x7d, 0x77, 0xef, 0x6c, 0xea, 0x6b, 0xf1, 0xff, 0x6f, 0xe8, 0x64,
  0xe6, 0x6c, 0xff, 0xeb, 0x62, 0xdf, 0x61, 0xec, 0xf8, 0x65, 0xdd, 0x5c,
  0xe0, 0x6f, 0x6d, 0xdf, 0x5a, 0xdb, 0x63, 0xfa, 0xe6, 0x5b, 0xd9, 0x5c,
  0xe8, 0xf3, 0x5f, 0xda, 0x59, 0xdf, 0x71, 0x6a, 0xdd, 0x59, 0xdc, 0x65,
  0x7d, 0xe5, 0x5d, 0xdc, 0x5f, 0xed, 0xf3, 0x64, 0xde, 0x5f, 0xe6, 0x75,
  0x70, 0xe6, 0x63, 0xe5, 0x6c, 0xf8, 0xf3, 0x6c, 0xe9, 0x6a, 0xed, 0x76,
  0x7e, 0xf5, 0x6e, 0xec, 0x6c, 0xee, 0x74, 0xfe, 0xf2, 0x6b, 0xe8, 0x68,
  0xec, 0x7a, 0x6f, 0xe6, 0x61, 0xe2, 0x6a, 0xfc, 0xea, 0x60, 0xdd, 0x5f,
  0xea, 0xf8, 0x64, 0xdc, 0x5b, 0xdf, 0x6e, 0x6d, 0xde, 0x5a, 0xdb, 0x63,
  0xfb, 0xe5, 0x5b, 0xd9, 0x5d, 0xea, 0xf3, 0x5f, 0xda, 0x5a, 0xe0, 0x72,
  0x6b, 0xde, 0x5b, 0xdd, 0x67, 0x7e, 0xe7, 0x5e, 0xdd, 0x62, 0xee, 0xf6,
  0x68, 0xe1, 0x62, 0xe8, 0x74, 0x75, 0xeb, 0x68, 0xe8, 0x6c, 0xf6, 0xfb,
  0x72, 0xed, 0x6d, 0xed, 0x70, 0xf7, 0xfe, 0x74, 0xed, 0x6a, 0xea, 0x6d,
  0xf7, 0xf6, 0x6a, 0xe5, 0x64, 0xe9, 0x75, 0x70, 0xe5, 0x5f, 0xdf, 0x67,
  0xfa, 0xea, 0x5f, 0xdc, 0x5e, 0xe9, 0xf8, 0x63, 0xdc, 0x5b, 0xdf, 0x6f,
  0x6c, 0xde, 0x5a, 0xdb, 0x63, 0xfc, 0xe6, 0x5c, 0xda, 0x5e, 0xea, 0xf3,
  0x61, 0xdb, 0x5c, 0xe2, 0x74, 0x6c, 0xdf, 0x5d, 0xdf, 0x69, 0x7e, 0xe9,
  0x61, 0xdf, 0x64, 0xef, 0xfa, 0x6b, 0xe6, 0x65, 0xea, 0x72, 0x7b, 0xef,
  0x6c, 0xeb, 0x6d, 0xf1, 0x7a, 0x7a, 0xf3, 0x6e, 0xec, 0x6c, 0xef, 0x78,
  0x78, 0xed, 0x68, 0xe7, 0x6a, 0xf2, 0xf7, 0x6a, 0xe2, 0x61, 0xe6, 0x72,
  0x70, 0xe3, 0x5e, 0xde, 0x66, 0xfa, 0xe9, 0x5e, 0xdc, 0x5e, 0xe9, 0xf8,
  0x62, 0xdb, 0x5b, 0xdf, 0x6f, 0x6c, 0xde, 0x5a, 0xdc, 0x64, 0xfd, 0xe6,
  0x5c, 0xdb, 0x5e, 0xeb, 0xf4, 0x62, 0xdd, 0x5d, 0xe3, 0x74, 0x6d, 0xe2,
  0x5e, 0xe1, 0x69, 0xfd, 0xed, 0x65, 0xe3, 0x67, 0xee, 0xfe, 0x6f, 0xea,
  0x69, 0xeb, 0x70, 0xfb, 0xf7, 0x70, 0xed, 0x6d, 0xee, 0x72, 0xfa, 0xf9,
  0x6f, 0xeb, 0x6a, 0xeb, 0x71, 0x7c, 0xed, 0x67, 0xe3, 0x66, 0xee, 0xf9,
  0x69, 0xe0, 0x5f, 0xe4, 0x71, 0x71, 0xe2, 0x5d, 0xdd, 0x65, 0xf8, 0xe9,
  0x5e, 0xdb, 0x5e, 0xe8, 0xf7, 0x63, 0xdb, 0x5b, 0xdf, 0x70, 0x6d, 0xde,
  0x5b, 0xdc, 0x65, 0xfc, 0xe7, 0x5d, 0xdc, 0x5f, 0xec, 0xf5, 0x64, 0xde,
  0x5e, 0xe5, 0x73, 0x6f, 0xe5, 0x60, 0xe3, 0x6a, 0xfc, 0xf0, 0x68, 0xe7,
  0x69, 0xef, 0x7b, 0x75, 0xee, 0x6c, 0xec, 0x6f, 0xf5, 0x7d, 0x77, 0xef,
  0x6d, 0xec, 0x6d, 0xf3, 0xff, 0x70, 0xea, 0x66, 0xe8, 0x6e, 0xfe, 0xed,
  0x65, 0xe1, 0x64, 0xed, 0xfa, 0x68, 0xdf, 0x5e, 0xe2, 0x6f, 0x6f, 0xe1,
  0x5d, 0xdd, 0x65, 0xf9, 0xe9, 0x5e, 0xdb, 0x5e, 0xe9, 0xf6, 0x63, 0xdc,
  0x5b, 0xe0, 0x71, 0x6d, 0xdf, 0x5c, 0xdd, 0x66, 0xfe, 0xe9, 0x5f, 0xdd,
  0x61, 0xed, 0xf7, 0x67, 0xe0, 0x60, 0xe7, 0x74, 0x73, 0xe8, 0x64, 0xe6,
  0x6c, 0xf8, 0xf5, 0x6c, 0xeb, 0x6b, 0xee, 0x76, 0x7d, 0xf4, 0x6f, 0xed,
  0x6d, 0xef, 0x75, 0x7e, 0xf4, 0x6c, 0xe9, 0x6a, 0xee, 0x7a, 0x71, 0xe9,
  0x64, 0xe5, 0x6b, 0xfc, 0xed, 0x63, 0xdf, 0x62, 0xeb, 0xfa, 0x67, 0xde,
  0x5d, 0xe1, 0x6f, 0x6f, 0xe0, 0x5c, 0xdd, 0x64, 0xf9, 0xe8, 0x5e, 0xdb,
  0x5e, 0xea, 0xf6, 0x64, 0xdc, 0x5c, 0xe1, 0x72, 0x6d, 0xe0, 0x5d, 0xde,
  0x68, 0xfd, 0xea, 0x61, 0xdf, 0x63, 0xed, 0xf9, 0x6a, 0xe4, 0x63, 0xe9,
  0x74, 0x77, 0xec, 0x69, 0xe9, 0x6d, 0xf6, 0xfd, 0x73, 0xee, 0x6d, 0xee,
  0x72, 0xf9, 0xfd, 0x74, 0xee, 0x6c, 0xec, 0x6f, 0xf9, 0xf6, 0x6c, 0xe8,
  0x66, 0xeb, 0x76, 0x72, 0xe8, 0x62, 0xe2, 0x69, 0xfa, 0xec, 0x62, 0xde,
  0x60, 0xea, 0xfb, 0x66, 0xde, 0x5d, 0xe1, 0x6f, 0x6f, 0xe0, 0x5c, 0xdd,
  0x65, 0xfa, 0xe9, 0x5e, 0xdc, 0x5f, 0xeb, 0xf7, 0x64, 0xdd, 0x5d, 0xe3,
  0x72, 0x6e, 0xe2, 0x5e, 0xe0, 0x69, 0xfc, 0xec, 0x64, 0xe1, 0x65, 0xef,
  0xfc, 0x6c, 0xe7, 0x66, 0xeb, 0x72, 0x7c, 0xf1, 0x6c, 0xec, 0x6d, 0xf2,
  0x7b, 0x7a, 0xf2, 0x6e, 0xed, 0x6e, 0xf1, 0x7a, 0x79, 0xee, 0x6a, 0xe9,
  0x6c, 0xf4, 0xf9, 0x6b, 0xe5, 0x64, 0xe8, 0x73, 0x73, 0xe6, 0x60, 0xe0,
  0x68, 0xf9, 0xec, 0x61, 0xde, 0x5f, 0xea, 0xfa, 0x66, 0xde, 0x5d, 0xe1,
  0x6f, 0x6f, 0xe1, 0x5c, 0xdd, 0x66, 0xfb, 0xe9, 0x5e, 0xdd, 0x5f, 0xec,
  0xf7, 0x65, 0xde, 0x5e, 0xe5, 0x74, 0x6f, 0xe5, 0x60, 0xe2, 0x6a, 0xfc,
  0xee, 0x67, 0xe5, 0x68, 0xef, 0xfe, 0x71, 0xeb, 0x6a, 0xec, 0x72, 0xfc,
  0xf8, 0x71, 0xee, 0x6e, 0xef, 0x74, 0xfb, 0xf9, 0x70, 0xed, 0x6c, 0xed,
  0x73, 0x7d, 0xef, 0x69, 0xe6, 0x68, 0xef, 0xfa, 0x6b, 0xe3, 0x62, 0xe6,
  0x72, 0x73, 0xe5, 0x5f, 0xdf, 0x67, 0xf8, 0xeb, 0x60, 0xdd, 0x5f, 0xea,
  0xfb, 0x65, 0xde, 0x5d, 0xe2, 0x6f, 0x6e, 0xe1, 0x5d, 0xde, 0x66, 0xfd,
  0xea, 0x5f, 0xde, 0x61, 0xed, 0xf8, 0x67, 0xe0, 0x60, 0xe7, 0x74, 0x71,
  0xe8, 0x63, 0xe5, 0x6c, 0xfa, 0xf1, 0x6b, 0xe8, 0x6a, 0xef, 0x7c, 0x77,
  0xef, 0x6d, 0xed, 0x71, 0xf5, 0x7e, 0x79, 0xf0, 0x6e, 0xed, 0x6f, 0xf4,
  0xfe, 0x73, 0xeb, 0x69, 0xe9, 0x6f, 0xfd, 0xee, 0x67, 0xe4, 0x66, 0xed,
  0xfb, 0x6a, 0xe2, 0x60, 0xe5, 0x70, 0x72, 0xe5, 0x5e, 0xdf, 0x66, 0xf8,
  0xeb, 0x5f, 0xdd, 0x5f, 0xea, 0xfa, 0x65, 0xde, 0x5d, 0xe3, 0x6f, 0x6e,
  0xe2, 0x5d, 0xdf, 0x67, 0xfc, 0xeb, 0x60, 0xdf, 0x63, 0xed, 0xf9, 0x69,
  0xe3, 0x63, 0xe9, 0x74, 0x75, 0xea, 0x67, 0xe8, 0x6d, 0xf9, 0xf7, 0x6e,
  0xec, 0x6c, 0xef, 0x78, 0x7e, 0xf5, 0x71, 0xee, 0x6e, 0xf0, 0x77, 0xfe,
  0xf4, 0x6e, 0xeb, 0x6c, 0xef, 0x7b, 0x74, 0xea, 0x67, 0xe7, 0x6d, 0xfb,
  0xee, 0x66, 0xe1, 0x64, 0xec, 0xfc, 0x69, 0xe1, 0x5f, 0xe4, 0x6f, 0x72,
  0xe4, 0x5e, 0xdf, 0x66, 0xf9, 0xeb, 0x5f, 0xdd, 0x5f, 0xeb, 0xf9, 0x66,
  0xde, 0x5e, 0xe3, 0x71, 0x6f, 0xe4, 0x5f, 0xe0, 0x69, 0xfc, 0xec, 0x63,
  0xe1, 0x65, 0xef, 0xfb, 0x6b, 0xe6, 0x65, 0xea, 0x74, 0x79, 0xed, 0x6a,
  0xea, 0x6e, 0xf6, 0xfc, 0x73, 0xef, 0x6e, 0xef, 0x73, 0xf9, 0xfd, 0x75,
  0xef, 0x6d, 0xed, 0x70, 0xfa, 0xf7, 0x6e, 0xe9, 0x69, 0xec, 0x77, 0x74,
  0xe9, 0x65, 0xe5, 0x6b, 0xfa, 0xee, 0x64, 0xe0, 0x63, 0xec, 0xfc, 0x69,
  0xe0, 0x5f, 0xe3, 0x6f, 0x72, 0xe4, 0x5e, 0xdf, 0x66, 0xfa, 0xeb, 0x60,
  0xde, 0x60, 0xeb, 0xf9, 0x67, 0xdf, 0x5f, 0xe5, 0x72, 0x70, 0xe5, 0x60,
  0xe2, 0x6a, 0xfc, 0xee, 0x66, 0xe4, 0x67, 0xef, 0xfd, 0x6e, 0xe9, 0x68,
  0xec, 0x73, 0x7d, 0xf3, 0x6e, 0xed, 0x6e, 0xf3, 0x7b, 0x7b, 0xf5, 0x70,
  0xee, 0x6f, 0xf3, 0x7a, 0x7a, 0xef, 0x6c, 0xeb, 0x6d, 0xf5, 0xf9, 0x6d,
  0xe7, 0x67, 0xea, 0x75, 0x75, 0xe8, 0x63, 0xe3, 0x69, 0xf8, 0xed, 0x64,
  0xdf, 0x62, 0xeb, 0xfc, 0x69, 0xdf, 0x5f, 0xe3, 0x70, 0x71, 0xe3, 0x5e,
  0xdf, 0x67, 0xfa, 0xeb, 0x61, 0xdf, 0x61, 0xed, 0xfa, 0x68, 0xe1, 0x60,
  0xe7, 0x73, 0x71, 0xe7, 0x62, 0xe5, 0x6b, 0xfb, 0xf0, 0x69, 0xe7, 0x69,
  0xef, 0x7e, 0x72, 0xed, 0x6b, 0xed, 0x72, 0xfc, 0xf9, 0x72, 0xef, 0x6e,
  0xf1, 0x74, 0xfc, 0xf9, 0x71, 0xee, 0x6d, 0xee, 0x75, 0x7d, 0xf0, 0x6b,
  0xe8, 0x6b, 0xf0, 0xfa, 0x6d, 0xe6, 0x64, 0xe8, 0x72, 0x75, 0xe8, 0x62,
  0xe2, 0x69, 0xf8, 0xed, 0x63, 0xdf, 0x61, 0xeb, 0xfc, 0x68, 0xdf, 0x5f,
  0xe4, 0x70, 0x71, 0xe4, 0x5f, 0xe0, 0x68, 0xfb, 0xec, 0x62, 0xdf, 0x63,
  0xed, 0xfa, 0x69, 0xe3, 0x62, 0xe8, 0x74, 0x74, 0xea, 0x65, 0xe7, 0x6c,
  0xfa, 0xf4, 0x6c, 0xea, 0x6b, 0xef, 0x7a, 0x78, 0xf0, 0x6e, 0xee, 0x71,
  0xf6, 0xff, 0x79, 0xf2, 0x6f, 0xee, 0x70, 0xf5, 0xfd, 0x74, 0xed, 0x6b,
  0xec, 0x70, 0xfe, 0xf0, 0x69, 0xe7, 0x68, 0xef, 0xfd, 0x6c, 0xe5, 0x62,
  0xe7, 0x70, 0x74, 0xe7, 0x60, 0xe1, 0x67, 0xf9, 0xee, 0x62, 0xdf, 0x61,
  0xeb, 0xfb, 0x68, 0xe0, 0x5f, 0xe4, 0x70, 0x71, 0xe5, 0x5f, 0xe1, 0x69,
  0xfb, 0xed, 0x64, 0xe1, 0x65, 0xee, 0xfb, 0x6b, 0xe5, 0x65, 0xe9, 0x74,
  0x78, 0xec, 0x69, 0xe9, 0x6e, 0xf8, 0xf8, 0x6f, 0xec, 0x6d, 0xef, 0x77,
  0x7e, 0xf6, 0x72, 0xef, 0x70, 0xf2, 0x78, 0x7e, 0xf6, 0x6f, 0xed, 0x6d,
  0xf0, 0x7b, 0x75, 0xed, 0x69, 0xe9, 0x6e, 0xfb, 0xf0, 0x68, 0xe5, 0x66,
  0xee, 0xfd, 0x6b, 0xe4, 0x61, 0xe6, 0x70, 0x74, 0xe7, 0x60, 0xe1, 0x67,
  0xf8, 0xed, 0x63, 0xdf, 0x62, 0xeb, 0xfb, 0x69, 0xe0, 0x5f, 0xe5, 0x71,
  0x72, 0xe6, 0x61, 0xe2, 0x6a, 0xfa, 0xee, 0x66, 0xe3, 0x67, 0xee, 0xfc,
  0x6e, 0xe8, 0x67, 0xeb, 0x74, 0x7c, 0xef, 0x6c, 0xeb, 0x6e, 0xf6, 0xfd,
  0x75, 0xf0, 0x6f, 0xef, 0x74, 0xfa, 0xfd, 0x75, 0xf0, 0x6e, 0xef, 0x73,
  0xfb, 0xf8, 0x6e, 0xeb, 0x6a, 0xee, 0x78, 0x76, 0xeb, 0x67, 0xe7, 0x6c,
  0xfa, 0xf0, 0x67, 0xe3, 0x65, 0xed, 0xfe, 0x6b, 0xe3, 0x61, 0xe5, 0x6f,
  0x74, 0xe7, 0x60, 0xe1, 0x67, 0xf9, 0xee, 0x63, 0xdf, 0x62, 0xec, 0xfc,
  0x69, 0xe2, 0x60, 0xe6, 0x72, 0x73, 0xe8, 0x63, 0xe4, 0x6b, 0xfa, 0xef,
  0x68, 0xe5, 0x69, 0xef, 0xfe, 0x70, 0xea, 0x6a, 0xec, 0x74, 0x7e, 0xf3,
  0x6f, 0xed, 0x6f, 0xf3, 0x7c, 0x7b, 0xf4, 0x71, 0xef, 0x71, 0xf4, 0x7c,
  0x7a, 0xf0, 0x6e, 0xec, 0x6f, 0xf6, 0xfa, 0x6f, 0xea, 0x69, 0xeb, 0x76,
  0x77, 0xeb, 0x66, 0xe5, 0x6b, 0xf8, 0xef, 0x67, 0xe2, 0x64, 0xed, 0xfe,
  0x6b, 0xe3, 0x60, 0xe5, 0x6f, 0x74, 0xe7, 0x60, 0xe1, 0x68, 0xf9, 0xee,
  0x63, 0xe1, 0x63, 0xed, 0xfd, 0x6a, 0xe4, 0x62, 0xe8, 0x72, 0x75, 0xe9,
  0x65, 0xe6, 0x6c, 0xfa, 0xf2, 0x6b, 0xe8, 0x6a, 0xef, 0x7d, 0x74, 0xed,
  0x6d, 0xed, 0x73, 0xfb, 0xf9, 0x74, 0xef, 0x70, 0xf1, 0x77, 0xfd, 0xfa,
  0x73, 0xef, 0x6e, 0xef, 0x76, 0x7d, 0xf2, 0x6c, 0xeb, 0x6c, 0xf2, 0xfc,
  0x6e, 0xe9, 0x67, 0xea, 0x72, 0x77, 0xea, 0x65, 0xe4, 0x6a, 0xf8, 0xef,
  0x66, 0xe2, 0x63, 0xec, 0xff, 0x6b, 0xe3, 0x60, 0xe5, 0x70, 0x75, 0xe7,
  0x61, 0xe2, 0x69, 0xf9, 0xee, 0x65, 0xe2, 0x64, 0xed, 0xfd, 0x6c, 0xe5,
  0x64, 0xe9, 0x73, 0x77, 0xeb, 0x67, 0xe8, 0x6d, 0xf9, 0xf6, 0x6d, 0xeb,
  0x6c, 0xef, 0x7a, 0x7a, 0xf1, 0x6f, 0xee, 0x72, 0xf7, 0xff, 0x78, 0xf3,
  0x70, 0xef, 0x72, 0xf7, 0xfe, 0x75, 0xee, 0x6c, 0xed, 0x72, 0xfd, 0xf2,
  0x6b, 0xe8, 0x6a, 0xef, 0xfd, 0x6e, 0xe7, 0x65, 0xe9, 0x71, 0x78, 0xea,
  0x63, 0xe4, 0x69, 0xf7, 0xef, 0x66, 0xe1, 0x64, 0xec, 0xff, 0x6b, 0xe3,
  0x61, 0xe6, 0x70, 0x75, 0xe8, 0x62, 0xe3, 0x6a, 0xf9, 0xef, 0x67, 0xe3,
  0x66, 0xee, 0xfe, 0x6d, 0xe7, 0x66, 0xea, 0x74, 0x7a, 0xee, 0x6a, 0xea,
  0x6e, 0xf7, 0xfa, 0x70, 0xee, 0x6e, 0xf1, 0x77, 0x7e, 0xf7, 0x72, 0xf0,
  0x71, 0xf4, 0x79, 0x7d, 0xf6, 0x71, 0xee, 0x6f, 0xf2, 0x7c, 0x77, 0xee,
  0x6b, 0xeb, 0x6f, 0xfb, 0xf2, 0x6b, 0xe7, 0x68, 0xee, 0xff, 0x6e, 0xe6,
  0x64, 0xe7, 0x70, 0x78, 0xea, 0x63, 0xe3, 0x69, 0xf7, 0xef, 0x66, 0xe2,
  0x64, 0xec, 0xfe, 0x6b, 0xe4, 0x62, 0xe6, 0x71, 0x76, 0xe9, 0x63, 0xe4,
  0x6a, 0xf9, 0xf1, 0x68, 0xe5, 0x67, 0xef, 0x7e, 0x6f, 0xea, 0x68, 0xec,
  0x74, 0x7c, 0xf1, 0x6c, 0xec, 0x6f, 0xf6, 0xfe, 0x76, 0xf1, 0x6f, 0xf0,
  0x75, 0xfb, 0xfd, 0x76, 0xf1, 0x6f, 0xef, 0x74, 0xfb, 0xf9, 0x70, 0xec,
  0x6d, 0xee, 0x79, 0x78, 0xed, 0x6a, 0xe9, 0x6d, 0xf9, 0xf2, 0x6a, 0xe6,
  0x67, 0xed, 0x7e, 0x6d, 0xe6, 0x63, 0xe7, 0x70, 0x78, 0xe9, 0x63, 0xe3,
  0x69, 0xf7, 0xf0, 0x66, 0xe2, 0x64, 0xec, 0xfe, 0x6c, 0xe5, 0x63, 0xe7,
  0x71, 0x77, 0xea, 0x65, 0xe6, 0x6b, 0xf8, 0xf3, 0x6a, 0xe8, 0x69, 0xef,
  0x7e, 0x72, 0xec, 0x6b, 0xed, 0x74, 0xfe, 0xf5, 0x6f, 0xee, 0x6f, 0xf4,
  0x7c, 0x7b, 0xf5, 0x71, 0xf0, 0x72, 0xf6, 0x7b, 0x79, 0xf3, 0x6e, 0xee,
  0x6f, 0xf7, 0xfb, 0x6f, 0xec, 0x6b, 0xed, 0x76, 0x79, 0xed, 0x68, 0xe7,
  0x6c, 0xf7, 0xf2, 0x69, 0xe5, 0x66, 0xec, 0x7e, 0x6e, 0xe5, 0x63, 0xe7,
  0x6f, 0x78, 0xe9, 0x63, 0xe4, 0x69, 0xf7, 0xf0, 0x67, 0xe3, 0x65, 0xed,
  0xff, 0x6d, 0xe6, 0x65, 0xe9, 0x73, 0x78, 0xeb, 0x67, 0xe7, 0x6c, 0xf8,
  0xf5, 0x6d, 0xea, 0x6b, 0xf0, 0x7c, 0x76, 0xef, 0x6d, 0xee, 0x72, 0xfc,
  0xfb, 0x74, 0xf2, 0x70, 0xf3, 0x76, 0xfe, 0xfb, 0x75, 0xf0, 0x6f, 0xf1,
  0x77, 0x7d, 0xf4, 0x6e, 0xec, 0x6d, 0xf3, 0xfe, 0x70, 0xeb, 0x69, 0xeb,
  0x73, 0x7a, 0xed, 0x67, 0xe7, 0x6b, 0xf7, 0xf4, 0x69, 0xe5, 0x66, 0xec,
  0x7d, 0x6d, 0xe5, 0x63, 0xe6, 0x6f, 0x79, 0xea, 0x64, 0xe3, 0x6a, 0xf6,
  0xf0, 0x68, 0xe4, 0x66, 0xed, 0xff, 0x6e, 0xe7, 0x66, 0xea, 0x73, 0x7a,
  0xed, 0x69, 0xe9, 0x6d, 0xf8, 0xf8, 0x6f, 0xec, 0x6d, 0xf1, 0x7a, 0x7a,
  0xf3, 0x6f, 0xf0, 0x72, 0xf8, 0x7e, 0x78, 0xf5, 0x70, 0xf1, 0x72, 0xf9,
  0xff, 0x75, 0xf0, 0x6d, 0xef, 0x72, 0xfd, 0xf4, 0x6d, 0xeb, 0x6b, 0xf1,
  0xff, 0x70, 0xea, 0x67, 0xea, 0x72, 0x7a, 0xec, 0x66, 0xe5, 0x6a, 0xf6,
  0xf3, 0x68, 0xe4, 0x66, 0xec, 0x7d, 0x6e, 0xe6, 0x64, 0xe7, 0x70, 0x79,
  0xea, 0x65, 0xe4, 0x6b, 0xf6, 0xf2, 0x69, 0xe5, 0x68, 0xed, 0xff, 0x6f,
  0xe9, 0x68, 0xeb, 0x73, 0x7c, 0xef, 0x6c, 0xeb, 0x6e, 0xf6, 0xfc, 0x72,
  0xee, 0x6e, 0xf1, 0x77, 0xfe, 0xf8, 0x73, 0xf2, 0x71, 0xf5, 0x79, 0x7d,
  0xf7, 0x72, 0xef, 0x6f, 0xf4, 0x7b, 0x77, 0xf0, 0x6c, 0xec, 0x6f, 0xfb,
  0xf5, 0x6c, 0xe9, 0x6a, 0xef, 0x7d, 0x6f, 0xe9, 0x66, 0xe9, 0x70, 0x7a,
  0xec, 0x66, 0xe5, 0x6a, 0xf6, 0xf3, 0x69, 0xe4, 0x65, 0xec, 0x7d, 0x6e,
  0xe7, 0x64, 0xe8, 0x70, 0x79, 0xeb, 0x66, 0xe6, 0x6b, 0xf7, 0xf4, 0x6b,
  0xe7, 0x69, 0xee, 0x7d, 0x72, 0xeb, 0x6a, 0xec, 0x73, 0xff, 0xf2, 0x6e,
  0xed, 0x6f, 0xf5, 0xfe, 0x78, 0xf2, 0x70, 0xf1, 0x75, 0xfb, 0xfd, 0x77,
  0xf3, 0x70, 0xf1, 0x75, 0xfb, 0xfa, 0x73, 0xee, 0x6e, 0xf0, 0x78, 0x79,
  0xef, 0x6b, 0xeb, 0x6e, 0xf8, 0xf5, 0x6c, 0xe8, 0x69, 0xee, 0x7c, 0x6f,
  0xe9, 0x66, 0xe8, 0x6f, 0x7a, 0xec, 0x66, 0xe5, 0x6a, 0xf6, 0xf4, 0x69,
  0xe5, 0x66, 0xed, 0x7c, 0x6e, 0xe7, 0x65, 0xe9, 0x71, 0x7a, 0xec, 0x68,
  0xe7, 0x6c, 0xf7, 0xf6, 0x6c, 0xe9, 0x6b, 0xef, 0x7c, 0x75, 0xed, 0x6c,
  0xed, 0x74, 0xfd, 0xf7, 0x71, 0xef, 0x70, 0xf4, 0x7c, 0x7c, 0xf6, 0x74,
  0xf1, 0x73, 0xf6, 0x7d, 0x7b, 0xf3, 0x70, 0xef, 0x71, 0xf7, 0xfd, 0x72,
  0xee, 0x6c, 0xee, 0x75, 0x7a, 0xef, 0x6a, 0xea, 0x6d, 0xf7, 0xf6, 0x6b,
  0xe7, 0x68, 0xed, 0x7b, 0x6f, 0xe8, 0x66, 0xe9, 0x6f, 0x7b, 0xec, 0x66,
  0xe5, 0x6a, 0xf6, 0xf4, 0x6a, 0xe6, 0x67, 0xed, 0x7c, 0x6f, 0xe9, 0x66,
  0xe9, 0x72, 0x7a, 0xee, 0x69, 0xe9, 0x6d, 0xf8, 0xf8, 0x6e, 0xeb, 0x6c,
  0xf0, 0x7b, 0x78, 0xf0, 0x6e, 0xef, 0x74, 0xfa, 0xfb, 0x76, 0xf1, 0x71,
  0xf3, 0x78, 0xfd, 0xfa, 0x76, 0xf1, 0x71, 0xf3, 0x78, 0x7e, 0xf5, 0x6f,
  0xed, 0x6f, 0xf4, 0xfe, 0x72, 0xec, 0x6b, 0xec, 0x74, 0x7c, 0xee, 0x6a,
  0xe8, 0x6c, 0xf6, 0xf5, 0x6b, 0xe7, 0x67, 0xed, 0x7c, 0x6f, 0xe9, 0x65,
  0xe9, 0x6f, 0x7a, 0xed, 0x66, 0xe6, 0x6a, 0xf6, 0xf5, 0x6a, 0xe7, 0x68,
  0xee, 0x7c, 0x70, 0xea, 0x68, 0xeb, 0x72, 0x7c, 0xef, 0x6b, 0xeb, 0x6e,
  0xf7, 0xfa, 0x70, 0xee, 0x6d, 0xf1, 0x79, 0x7b, 0xf5, 0x70, 0xf0, 0x74,
  0xf7, 0x7e, 0x7a, 0xf4, 0x73, 0xf1, 0x75, 0xf9, 0xfe, 0x77, 0xf1, 0x6f,
  0xef, 0x74, 0xfc, 0xf6, 0x6f, 0xec, 0x6d, 0xf1, 0x7e, 0x73, 0xeb, 0x6a,
  0xeb, 0x73, 0x7d, 0xee, 0x69, 0xe7, 0x6b, 0xf5, 0xf6, 0x6b, 0xe7, 0x67,
  0xed, 0x7b, 0x6f, 0xe9, 0x66, 0xe9, 0x6f, 0x7b, 0xed, 0x67, 0xe7, 0x6b,
  0xf7, 0xf6, 0x6b, 0xe8, 0x68, 0xee, 0x7c, 0x72, 0xeb, 0x69, 0xec, 0x73,
  0x7d, 0xf2, 0x6d, 0xec, 0x6f, 0xf7, 0xfc, 0x75, 0xef, 0x6f, 0xf1, 0x78,
  0xfe, 0xf9, 0x75, 0xf2, 0x73, 0xf5, 0x7a, 0x7e, 0xf8, 0x74, 0xf1, 0x71,
  0xf4, 0x7c, 0x79, 0xf1, 0x6e, 0xee, 0x71, 0xfb, 0xf7, 0x6e, 0xeb, 0x6b,
  0xf0, 0x7c, 0x72, 0xeb, 0x69, 0xea, 0x71, 0x7d, 0xee, 0x68, 0xe7, 0x6b,
  0xf5, 0xf6, 0x6b, 0xe7, 0x68, 0xed, 0x7c, 0x70, 0xe9, 0x67, 0xe9, 0x70,
  0x7b, 0xed, 0x68, 0xe8, 0x6c, 0xf7, 0xf6, 0x6c, 0xe9, 0x6a, 0xef, 0x7b,
  0x74, 0xed, 0x6b, 0xed, 0x73, 0xff, 0xf5, 0x6f, 0xee, 0x6f, 0xf6, 0x7e,
  0x79, 0xf3, 0x71, 0xf2, 0x76, 0xfb, 0xfd, 0x78, 0xf5, 0x72, 0xf3, 0x76,
  0xfc, 0xfa, 0x74, 0xef, 0x6f, 0xf2, 0x79, 0x7a, 0xf1, 0x6d, 0xec, 0x6f,
  0xf9, 0xf7, 0x6d, 0xea, 0x6b, 0xee, 0x7c, 0x72, 0xeb, 0x68, 0xea, 0x70,
  0x7d, 0xee, 0x69, 0xe7, 0x6b, 0xf4, 0xf6, 0x6c, 0xe7, 0x68, 0xed, 0x7b,
  0x71, 0xea, 0x67, 0xea, 0x71, 0x7c, 0xee, 0x69, 0xe9, 0x6c, 0xf6, 0xf8,
  0x6e, 0xeb, 0x6b, 0xf0, 0x7a, 0x76, 0xef, 0x6d, 0xee, 0x73, 0xfd, 0xf9,
  0x72, 0xf0, 0x70, 0xf5, 0x7b, 0x7c, 0xf7, 0x73, 0xf3, 0x74, 0xf8, 0x7c,
  0x7b, 0xf5, 0x71, 0xf1, 0x72, 0xf9, 0xfd, 0x74, 0xee, 0x6d, 0xef, 0x77,
  0x7c, 0xf1, 0x6c, 0xeb, 0x6e, 0xf7, 0xf7, 0x6d, 0xe9, 0x6a, 0xee, 0x7c,
  0x72, 0xea, 0x68, 0xea, 0x70, 0x7d, 0xee, 0x69, 0xe7, 0x6b, 0xf5, 0xf6,
  0x6c, 0xe7, 0x69, 0xed, 0x7c, 0x72, 0xea, 0x68, 0xeb, 0x71, 0x7d, 0xef,
  0x6b, 0xea, 0x6d, 0xf7, 0xfa, 0x6f, 0xec, 0x6d, 0xf1, 0x7a, 0x7a, 0xf2,
  0x6f, 0xef, 0x74, 0xfa, 0xfc, 0x76, 0xf3, 0x72, 0xf4, 0x78, 0xfe, 0xfb,
  0x75, 0xf3, 0x72, 0xf4, 0x79, 0x7e, 0xf7, 0x70, 0xef, 0x70, 0xf5, 0xfe,
  0x75, 0xee, 0x6c, 0xee, 0x75, 0x7d, 0xf0, 0x6c, 0xea, 0x6d, 0xf7, 0xf8,
  0x6d, 0xe9, 0x69, 0xee, 0x7b, 0x73, 0xea, 0x68, 0xea, 0x70, 0x7d, 0xee,
  0x69, 0xe8, 0x6b, 0xf6, 0xf7, 0x6c, 0xe8, 0x69, 0xee, 0x7b, 0x73, 0xec,
  0x6a, 0xec, 0x71, 0x7d, 0xf2, 0x6c, 0xec, 0x6e, 0xf7, 0xfc, 0x73, 0xee,
  0x6e, 0xf1, 0x79, 0x7c, 0xf6, 0x72, 0xf1, 0x74, 0xf9, 0x7e, 0x7a, 0xf6,
  0x73, 0xf3, 0x75, 0xfa, 0xfe, 0x78, 0xf3, 0x70, 0xf1, 0x76, 0xfd, 0xf7,
  0x70, 0xed, 0x6e, 0xf3, 0x7e, 0x75, 0xed, 0x6c, 0xed, 0x73, 0x7e, 0xf0,
  0x6b, 0xea, 0x6c, 0xf6, 0xf8, 0x6d, 0xe9, 0x69, 0xee, 0x7b, 0x72, 0xeb,
  0x68, 0xea, 0x70, 0x7d, 0xee, 0x69, 0xe9, 0x6c, 0xf5, 0xf8, 0x6d, 0xea,
  0x6a, 0xef, 0x7b, 0x75, 0xed, 0x6b, 0xed, 0x72, 0xff, 0xf4, 0x6e, 0xed,
  0x6f, 0xf6, 0xfe, 0x75, 0xf1, 0x70, 0xf3, 0x78, 0xfe, 0xf9, 0x75, 0xf3,
  0x73, 0xf6, 0x7b, 0x7d, 0xf9, 0x74, 0xf2, 0x72, 0xf7, 0x7c, 0x7a, 0xf3,
  0x6f, 0xef, 0x73, 0xfb, 0xf8, 0x6f, 0xed, 0x6d, 0xf1, 0x7c, 0x75, 0xed,
  0x6b, 0xec, 0x73, 0x7d, 0xef, 0x6b, 0xe9, 0x6c, 0xf6, 0xf8, 0x6d, 0xe9,
  0x69, 0xee, 0x7a, 0x73, 0xeb, 0x69, 0xeb, 0x71, 0x7e, 0xef, 0x6b, 0xe9,
  0x6d, 0xf6, 0xf8, 0x6e, 0xea, 0x6c, 0xef, 0x7c, 0x77, 0xee, 0x6d, 0xee,
  0x74, 0xfe, 0xf7, 0x70, 0xef, 0x70, 0xf6, 0x7d, 0x78, 0xf5, 0x71, 0xf4,
  0x76, 0xfb, 0xfe, 0x78, 0xf6, 0x73, 0xf5, 0x76, 0xfd, 0xfb, 0x75, 0xf2,
  0x70, 0xf3, 0x7a, 0x7b, 0xf3, 0x6e, 0xed, 0x71, 0xf9, 0xf8, 0x6f, 0xec,
  0x6c, 0xef, 0x7c, 0x75, 0xec, 0x6a, 0xec, 0x71, 0x7e, 0xf0, 0x6a, 0xe9,
  0x6c, 0xf6, 0xf9, 0x6d, 0xe9, 0x69, 0xee, 0x7a, 0x73, 0xec, 0x69, 0xeb,
  0x71, 0x7e, 0xf0, 0x6b, 0xea, 0x6d, 0xf6, 0xfa, 0x6f, 0xec, 0x6d, 0xf0,
  0x7b, 0x79, 0xf0, 0x6e, 0xef, 0x75, 0xfc, 0xf9, 0x74, 0xf1, 0x72, 0xf6,
  0x7b, 0x7c, 0xf8, 0x75, 0xf4, 0x74, 0xf8, 0x7d, 0x7c, 0xf7, 0x73, 0xf2,
  0x74, 0xf9, 0xfd, 0x76, 0xf0, 0x6f, 0xf0, 0x77, 0x7d, 0xf3, 0x6d, 0xed,
  0x6f, 0xf8, 0xfa, 0x6f, 0xeb, 0x6b, 0xef, 0x7b, 0x75, 0xec, 0x6a, 0xeb,
  0x71, 0x7e, 0xf0, 0x6a, 0xe9, 0x6c, 0xf5, 0xf9, 0x6d, 0xea, 0x6a, 0xee,
  0x7a, 0x74, 0xed, 0x6a, 0xec, 0x71, 0x7e, 0xf2, 0x6c, 0xec, 0x6e, 0xf5,
  0xfc, 0x72, 0xed, 0x6e, 0xf1, 0x79, 0x7b, 0xf3, 0x70, 0xef, 0x74, 0xfa,
  0xfc, 0x78, 0xf3, 0x73, 0xf4, 0x79, 0xfc, 0xfa, 0x77, 0xf3, 0x74, 0xf4,
  0x7a, 0xfe, 0xf7, 0x73, 0xef, 0x71, 0xf7, 0x7e, 0x76, 0xef, 0x6e, 0xef,
  0x75, 0x7d, 0xf3, 0x6d, 0xec, 0x6e, 0xf7, 0xfa, 0x6e, 0xeb, 0x6b, 0xef,
  0x79, 0x74, 0xed, 0x69, 0xeb, 0x6f, 0x7e, 0xf1, 0x6a, 0xea, 0x6c, 0xf6,
  0xfa, 0x6e, 0xeb, 0x6a, 0xef, 0x7a, 0x76, 0xee, 0x6b, 0xed, 0x72, 0xfe,
  0xf4, 0x6e, 0xed, 0x6f, 0xf6, 0xfd, 0x75, 0xef, 0x6f, 0xf1, 0x79, 0x7e,
  0xf6, 0x75, 0xf1, 0x76, 0xf8, 0xfd, 0x7a, 0xf5, 0x74, 0xf2, 0x76, 0xf8,
  0x7e, 0x7a, 0xf6, 0x71, 0xec, 0x76, 0xfa, 0x7e, 0x7b, 0xf9, 0x72, 0xf5,
  0x7d, 0xfb, 0xfb, 0x76, 0xf9, 0x75, 0x7b, 0xf1, 0x77, 0xfe, 0x7a, 0xfb,
  0x7e, 0x73, 0xfa, 0x7e, 0xfb, 0x7d, 0x77, 0xf7, 0x7e, 0x7c, 0xfe, 0x78,
  0xfa, 0x7e, 0xff, 0x7e, 0x76, 0xf9, 0xfd, 0xf9, 0xfe, 0x75, 0xf1, 0x76,
  0x7c, 0xfc, 0x7a, 0xf3, 0x70, 0xfd, 0x7e, 0x7d, 0xf3, 0x70, 0xf7, 0x78,
  0x7e, 0xf9, 0x72, 0xef, 0x77, 0xfc, 0xfd, 0x75, 0xee, 0x72, 0xf7, 0xfb
#endif
};

static void ctx_vt_bell (MrgVT *vt)
{
  if (vt->bell < 2)
    return;
  for (int i = 0; i < sizeof (vt_bell_audio); i++)
    terminal_queue_pcm_sample (MuLawDecompressTable[vt_bell_audio[i]] * vt->bell / 8);
}

/* if the byte is a non-print control character, handle it and return 1
 * oterhwise return 0*/
static int _vt_handle_control (MrgVT *vt, int byte)
{
    /* the big difference between ANSI-BBS mode and VT100+ mode is that
     * most C0 characters are printable
     */
  if (vt->encoding == 1) // this codepage is for ansi-bbs use
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
          _ctx_vt_add_str (vt, charmap_cp437[byte]);
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
	       for (char *c = copy; *c; c++)
	       {
		 if (*c < ' ' || * c > 127) *c = 0;
	       }
	       vt_write (vt, reply, strlen (reply));
	       free (copy);
	     }
	   }
	   return 1;
        case '\a': /* BELl */    ctx_vt_bell (vt); return 1;
        case '\b': /* BS */     _ctx_vt_backspace (vt); return 1;
        case '\t': /* HT tab */ _ctx_vt_htab (vt); return 1;

        case '\v': /* VT vertical tab */
        case '\f': /* VF form feed */
        case '\n': /* LF line ffed */
          ctx_vt_line_feed (vt);
	  // XXX : if we are at left margin, keep it!
          return 1;
        case '\r': /* CR carriage return */
	  ctx_vt_carriage_return (vt);
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
            _ctx_vt_add_str (vt, "¿");  // in vt52? XXX
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
  }
  return 0;
}

void ctx_vt_open_log (MrgVT *vt, const char *path)
{
  unlink (path);
  vt->log = fopen (path, "w");
}

static const char *base64_map="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
static void bin2base64_group (const unsigned char *in, int remaining, char *out)
{
  unsigned char digit[4] = {0,0,64,64};
  int i;
  digit[0] = in[0] >> 2;
  digit[1] = ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4);
  if (remaining > 1)
    {
      digit[2] = ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6);
      if (remaining > 2)
        digit[3] = ((in[2] & 0x3f));
    }
  for (i = 0; i < 4; i++)
    out[i] = base64_map[digit[i]];
}

void
vt_bin2base64 (const void *bin,
               int         bin_length,
               char       *ascii)
{
  /* this allocation is a hack to ensure we always produce the same result,
   * regardless of padding data accidentally taken into account.
   */
  unsigned char *bin2 = calloc (bin_length + 4, 1);
  unsigned const char *p = bin2;
  int i;
  memcpy (bin2, bin, bin_length);
  for (i=0; i*3 < bin_length; i++)
   {
     int remaining = bin_length - i*3;
     bin2base64_group (&p[i*3], remaining, &ascii[i*4]);
   }
  free (bin2);
  ascii[i*4]=0;
}

static unsigned char base64_revmap[255];
static void base64_revmap_init (void)
{
  static int done = 0;
  if (done)
    return;

  for (int i = 0; i < 255; i ++)
    base64_revmap[i]=255;
  for (int i = 0; i < 64; i ++)
    base64_revmap[((const unsigned char*)base64_map)[i]]=i;
  /* include variants used in URI encodings for decoder */
  base64_revmap['-']=62;
  base64_revmap['_']=63;
  base64_revmap['+']=62;
  base64_revmap['/']=63;

  done = 1;
}

int
vt_base642bin (const char    *ascii,
               int           *length,
               unsigned char *bin)
{
  // XXX : it would be nice to transform this to be able to do
  //       the conversion in-place, reusing the allocation
  int i;
  int charno = 0;
  int outputno = 0;
  int carry = 0;
  base64_revmap_init ();
  for (i = 0; ascii[i]; i++)
    {
      int bits = base64_revmap[((const unsigned char*)ascii)[i]];
      if (length && outputno > *length)
        {
          *length = -1;
          return -1;
        }
      if (bits != 255)
        {
          switch (charno % 4)
            {
              case 0:
                carry = bits;
                break;
              case 1:
                bin[outputno] = (carry << 2) | (bits >> 4);
                outputno++;
                carry = bits & 15;
                break;
              case 2:
                bin[outputno] = (carry << 4) | (bits >> 2);
                outputno++;
                carry = bits & 3;
                break;
              case 3:
                bin[outputno] = (carry << 6) | bits;
                outputno++;
                carry = 0;
                break;
            }
          charno++;
        }
    }
  bin[outputno]=0;
  if (length)
    *length= outputno;
  return outputno;
}

void vt_audio (MrgVT *vt, const char *command)
{
  // the simplest form of audio is raw audio
  //
  // _Ar=8000,c=2,b=8,e=u
  //
  // multiple voices:
  //   ids to queue
  //
  // reusing samples
  //   .. pitch bend and be able to do a mod player?
}

void vt_gfx (MrgVT *vt, const char *command)
{
  const char *payload = NULL;
  char key = 0;
  int  value;
  int  pos = 1;

  if (vt->gfx.multichunk == 0)
  {
    memset (&vt->gfx, 0, sizeof (GfxState));
    vt->gfx.action='t';
    vt->gfx.transmission='d';
  }

  while (command[pos] != ';')
  {
    pos ++; // G or ,
    if (command[pos] == ';') break;
    key = command[pos]; pos++;
    if (command[pos] == ';') break;
    pos ++; // =
    if (command[pos] == ';') break;

    if (command[pos] >= '0' && command[pos] <= '9')
      value = atoi(&command[pos]);
    else
      value = command[pos];
    while (command[pos] &&
           command[pos] != ',' &&
	   command[pos] != ';') pos++;
    
    fprintf (stderr, " %c = %i\n", key, value);
    switch (key)
    {
      case 'a': vt->gfx.action = value; break;
      case 'i': vt->gfx.id = value; break;
      case 'S': vt->gfx.buf_size = value; break;
      case 's': vt->gfx.buf_width = value; break;
      case 'v': vt->gfx.buf_height = value; break;
      case 'f': vt->gfx.format = value; break;
      case 'm': vt->gfx.multichunk = value; break;
      case 'o': vt->gfx.compression = value; break;
      case 't': vt->gfx.transmission = value; break;
      case 'x': vt->gfx.x = value; break;
      case 'y': vt->gfx.y = value; break;
      case 'w': vt->gfx.w = value; break;
      case 'h': vt->gfx.h = value; break;
      case 'X': vt->gfx.x_cell_offset = value; break;
      case 'Y': vt->gfx.y_cell_offset = value; break;
      case 'c': vt->gfx.columns = value; break;
      case 'r': vt->gfx.rows = value; break;
      case 'z': vt->gfx.z_index = value; break;
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

     fprintf (stderr, "size : %i\n", vt->gfx.data_size);
    if (vt->gfx.transmission != 'd') /* */
    {
      char buf[256];
      sprintf (buf, "\e_Gi=%i;only direct transmission supported\e\\",
		      vt->gfx.id);
      vt_write (vt, buf, strlen(buf));
      if (vt->gfx.data)
	free (vt->gfx.data);
      vt->gfx.data_size=0;
      return;
    }

    {
      int bin_length = vt->gfx.data_size;
      uint8_t *data2 = malloc (vt->gfx.data_size);
      bin_length = vt_base642bin ((char*)vt->gfx.data,
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
      unsigned long actual_uncompressed_size = vt->gfx.buf_size;
      int z_result = uncompress (data2, &actual_uncompressed_size,
				 vt->gfx.data,
		                 vt->gfx.data_size);
      if (z_result != Z_OK)
      {
	fprintf (stderr, "zlib trouble\n");
        if (vt->gfx.data)
          free (vt->gfx.data);
        vt->gfx.data = NULL;
        vt->gfx.data_size=0;
        return;
      }
      free (vt->gfx.data);
      vt->gfx.data = data2;
      vt->gfx.data_size = actual_uncompressed_size;
      vt->gfx.compression = 0;
    }

    if (vt->gfx.format == 100)
    {
      int channels;
        fprintf (stderr, "hmmm.. :%i\n", vt->gfx.data_size);
      uint8_t *new_data = stbi_load_from_memory (vt->gfx.data, vt->gfx.data_size, &vt->gfx.buf_width, &vt->gfx.buf_height, &channels, 4);

      if (!new_data)
      {
        fprintf (stderr, "image decode error size:%i\n", vt->gfx.data_size);
        vt->gfx.data = NULL;
	return;
      }
      vt->gfx.format = 32;
      free (vt->gfx.data);
      vt->gfx.data = new_data;
      vt->gfx.data_size= vt->gfx.buf_width * vt->gfx.buf_height * 4;
    }

  Image *image = NULL;
  fprintf (stderr, "doing action %c\n", vt->gfx.action);
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
	break;
    case 'p': // present 
      if (image)
      {
        int i = 0;
	for (i = 0; vt->current_line->images[i] && i < 4; i++);
	if (i >= 4) i = 3;
	vt->current_line->images[i] = image;
	vt->current_line->image_col[i] = vt->cursor_x;
      }
      break;
    case 'q': // query
      if (image_query (vt->gfx.id))
      {
        char buf[256];
        sprintf (buf, "\e_Gi=%i;OK\e\\", vt->gfx.id);
        vt_write (vt, buf, strlen(buf));
      }
      break;
    case 'd': // delete
      break;
  }

    if (vt->gfx.data)
      free (vt->gfx.data);
    vt->gfx.data = NULL;
    vt->gfx.data_size=0;
  }
}

static void ctx_vt_feed_byte (MrgVT *vt, int byte)
{
  if (vt->log)
  {
    char buf[3]="";
    buf[0]=byte;
    fwrite (buf, 1, 1, vt->log);
    fflush (vt->log);
  }

  if (byte >= ' ' && byte <= '~')
  {
    VT_input ("%c", byte);
  }
  else
  {
    VT_input ("<%i>", byte);
  }

  if (vt->in_vt52)
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
	    char str[2] = {byte, 0};
            _ctx_vt_add_str (vt, str);
	  }
	  break;
	}
	break;
      case 1:
	vt->utf8_pos = 0;
        switch (byte)
        {
	  case 'A': vtcmd_cursor_up (vt, " "); break;
	  case 'B': vtcmd_cursor_down (vt, " "); break;
	  case 'C': vtcmd_cursor_forward (vt, " "); break;
	  case 'D': vtcmd_cursor_backward (vt, " "); break;
	  case 'F': vtcmd_set_alternate_font (vt, " "); break;
	  case 'G': vtcmd_set_default_font (vt, " "); break;
	  case 'H': _ctx_vt_move_to (vt, 1, 1); break;
	  case 'I': vtcmd_reverse_index (vt, " "); break;
	  case 'J': vtcmd_erase_in_display (vt, "[0J"); break;
	  case 'K': vtcmd_erase_in_line (vt, "[0K"); break;
	  case 'Y': vt->utf8_pos = 2; break;
	  case 'Z': vt_write (vt, "\e/Z", 3); break;
	  case '<': vt->in_vt52 = 0; break;
	  default: break;
        }
	break;
      case 2:
	_ctx_vt_move_to (vt, byte - 31, vt->cursor_x);
	vt->utf8_pos = 3;
	break;
      case 3:
	_ctx_vt_move_to (vt, vt->cursor_y, byte - 31);
	vt->utf8_pos = 0;
	break;

    }
    return;
  }
  else if (vt->in_ctx_ascii)
  {
    Ctx *ctx = vt->current_line->ctx;
    if (ctx)
    {
      //ctx_clear (ctx);
    }
    else
    {
      ctx = vt->current_line->ctx = ctx_new ();
    }
#if 0
    ctx_translate (ctx,
		   0,//(vt->cursor_x-1) * vt->cw,
		   (vt->cursor_y-1));
#endif

    switch (byte)
    {
      case '\r':
	break;
      case '\n':
        vt->utf8_holding[vt->utf8_pos]=0;
	if ((!strcmp ((char*)vt->utf8_holding, "q"))||
	    (!strcmp ((char*)vt->utf8_holding, "quit"))||
	    (!strcmp ((char*)vt->utf8_holding, "done")))
	{
	  vt->in_ctx_ascii = 0;
	}
	else
	{
	  if (strlen ((char*)vt->utf8_holding) > 2)
	  {
	    VT_info ("gfx: <%s>", vt->utf8_holding);
	    ctx_parse_str_line (ctx, (char*)vt->utf8_holding);
	  }
	}
	vt->utf8_pos=0;
        vt->utf8_holding[vt->utf8_pos]=0;
	break;
      default:
        vt->utf8_holding[vt->utf8_pos++]=byte;
        vt->utf8_holding[vt->utf8_pos]=0;
	break;
    }
    return;
  }
  else if (vt->in_ctx)
  {
    Ctx *ctx = vt->current_line->ctx;
    if (!vt->current_line->ctx)
    {
      ctx = vt->current_line->ctx = ctx_new ();
      ctx_translate (ctx,
		     (vt->cursor_x-1) * vt->cw * 10,
		     (vt->cursor_y-1) * vt->ch * 10);
    }


    /* we reuse the utf8 holding area from the default code path, and collect
     * 9 bytes at a time
     */
    vt->utf8_holding[vt->utf8_pos++]=byte;
    if (vt->utf8_pos == 9)
    {
       switch (vt->utf8_holding[0])
       {
	  case CTX_EXIT:
            vt->in_ctx = 0;
            vt->utf8_pos = 0;
	    break;
	  case CTX_CLEAR:
            //ctx_empty (vt->ctx);
            ctx_clear (ctx);
            ctx_translate (ctx,
	  	     (vt->cursor_x-1) * vt->cw * 10,
		     (vt->cursor_y-1) * vt->ch * 10);
	    break;
	  default:
            ctx_add_single (ctx, &vt->utf8_holding[0]);
	    break;
        }
        vt->utf8_pos = 0;
    }
    return;
  }
  else if (vt->in_pcm)
  {
    if (byte == 0x00) // byte value 0 terminates - replace
    {                 // any 0s in original data with 1
      vt->in_pcm = 0;
    }
    else
    {
      terminal_queue_pcm_sample (MuLawDecompressTable[byte]);
    }
    return;
    // use a special value for termination
  }

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
        return;
      }
    }
    else
    {
      vt->utf8_holding[0] = byte;
      vt->utf8_holding[0] &= 127;
      vt->utf8_holding[1] = 0;
      if (vt->utf8_holding[0] == 0)
        vt->utf8_holding[0] = 32;
    }
    break;
    case 1:
      if ( ! (byte>=0 && byte < 256))
        byte = 255;
      strcpy ((char *)&vt->utf8_holding[0], &charmap_cp437[byte][0]);
      vt->utf8_expected_bytes = mrg_utf8_len (byte) - 1; // ?
    break;
    default:
      vt->utf8_holding[0] = byte & 127;
      vt->utf8_holding[1] = 0;
    break;
  }

  {
  switch (vt->state)
  {
    case TERMINAL_STATE_NEUTRAL:

      if (_vt_handle_control (vt, byte) == 0)
      switch (byte)
      {
        case 27: /* ESCape */
          vt->state = TERMINAL_STATE_GOT_ESC;
          break;
        default:
          if (vt->charset[vt->shifted_in] != 0 &&
	      vt->charset[vt->shifted_in] != 'B')
	  {
	    char **charmap;
	    switch (vt->charset[vt->shifted_in])
	    { 
		case 'A': charmap = charmap_uk; break;
		case 'B': charmap = charmap_ascii; break;
		case '0': charmap = charmap_graphics; break;
		case '1': charmap = charmap_cp437; break;
		case '2': charmap = charmap_graphics; break;
		default:
		  charmap = charmap_ascii;
		  break;
	    }
            if ((vt->utf8_holding[0] >= ' ') && (vt->utf8_holding[0] <= '~'))
            {
              _ctx_vt_add_str (vt, charmap[vt->utf8_holding[0]-' ']);
            }
	  }
	  else
	  {
	    // ensure vt->utf8_holding contains a valid utf8
            uint32_t codepoint;
            uint32_t state = 0;

  	    for (int i = 0; vt->utf8_holding[i]; i++)
	       utf8_decode(&state, &codepoint, vt->utf8_holding[i]);
	    if (state != UTF8_ACCEPT)
	    {
	      /* otherwise mangle it so that it does */
	      vt->utf8_holding[0] &= 127;
	      vt->utf8_holding[1] = 0;
	      if (vt->utf8_holding[0] == 0)
	        vt->utf8_holding[0] = 32;
	    }

            _ctx_vt_add_str (vt, (char*)vt->utf8_holding);
	  }
          break;
      }
      break;
    case TERMINAL_STATE_GOT_ESC:
      if (_vt_handle_control (vt, byte) == 0)
      switch (byte)
      {
        case 27: /* ESCape */
		break;
        case ')':
        case '#':
        case '(':
          {
            char tmp[]={byte, '\0'};
            ctx_vt_argument_buf_reset(vt, tmp);
            vt->state = TERMINAL_STATE_GOT_ESC_FOO;
          }
          break;
        case '[':
        case '%':
        case '+':
        case '*':
          {
            char tmp[]={byte, '\0'};
            ctx_vt_argument_buf_reset(vt, tmp);
            vt->state = TERMINAL_STATE_GOT_ESC_SEQUENCE;
          }
          break;
        case ']':
        case '_':
          {
            char tmp[]={byte, '\0'};
            ctx_vt_argument_buf_reset(vt, tmp);
            vt->state = TERMINAL_STATE_GOT_ESC_SQRPAREN;
          }
          break;
        default:
          {
            char tmp[]={byte, '\0'};
            tmp[0]=byte;
            handle_sequence (vt, tmp);
            vt->state = TERMINAL_STATE_NEUTRAL;
          }
          break;
      }
      break;
    case TERMINAL_STATE_GOT_ESC_FOO:
      ctx_vt_argument_buf_add (vt, byte);
      handle_sequence (vt, vt->argument_buf);
      vt->state = TERMINAL_STATE_NEUTRAL;
      break;
    case TERMINAL_STATE_GOT_ESC_SEQUENCE:
      if (_vt_handle_control (vt, byte) == 0)
      {
        if (byte == 27)
	{
	}
	else if (byte >= '@' && byte <= '~')
        {
          ctx_vt_argument_buf_add (vt, byte);
          handle_sequence (vt, vt->argument_buf);
          vt->state = TERMINAL_STATE_NEUTRAL;
        }
        else
        {
          ctx_vt_argument_buf_add (vt, byte);
        }
      }
      break;
    case TERMINAL_STATE_GOT_ESC_SQRPAREN:
      // XXX: use handle sequence here as well,.. for consistency
      // /    it seems like OSC (and others according to
      // https://ttssh2.osdn.jp/manual/4/en/about/ctrlseq.html
      // and in "\e\" rather than just "\e", this would cause
      // a stray char
      if (byte == '\a' || byte == 27 || byte == 0 || byte < 32)
      {
	if (vt->argument_buf[0] == ']')
	{
	  int n = parse_int (vt->argument_buf, 0);
          switch (n)
	  {
	  case 0:
            ctx_vt_set_title (vt, vt->argument_buf + 3);
	  default:
	    if (!strcmp (vt->argument_buf, "]10;?"))
	    { /* request current foreground color, xterm does this to
	         determine if it can use 256 colors, when this test fails,
		 it still mixes in color 130 together with stock colors
		 though
	       */
	      char *buf = "\e]10;rgb:ff/ff/ff\e\\";
	      vt_write (vt, buf, strlen(buf));
	    }
	    if (!strcmp (vt->argument_buf, "]11;?"))
	    {
	      char *buf = "\e]11;rgb:00/00/00\e\\";
	      vt_write (vt, buf, strlen(buf));
	    }
	    break;
	  }
	}  else if (vt->argument_buf[0] == '_' &&
		    vt->argument_buf[1] == 'G')
	{
	  vt_gfx (vt, vt->argument_buf);
	}  else if (vt->argument_buf[0] == '_' &&
		    vt->argument_buf[1] == 'A')
	{
	  vt_audio (vt, vt->argument_buf);
	}

	if (byte == 27)
          vt->state = TERMINAL_STATE_SWALLOW;
	else
          vt->state = TERMINAL_STATE_NEUTRAL;
      }
      else
      {
        ctx_vt_argument_buf_add (vt, byte);
      }
      break;
    case TERMINAL_STATE_SWALLOW:
      vt->state = TERMINAL_STATE_NEUTRAL;
      // this better be a \\ so were leaving DCS
      // XXX check that byte is \\ .. otherwise,
      //
      break;
  }
  }
}

static unsigned char buf[2048];
static int buf_len = 0;

int ctx_vt_poll (MrgVT *vt, int timeout)
{

  int read_size = sizeof(buf);
  int got_data = 0;
  int max_consumed_chars = 4096;
  int len = 0;
#if 1
  if (vt->cursor_visible && vt->smooth_scroll)
  {
    max_consumed_chars = vt->cols / 2;
  }
  if (vt->in_scroll)
  {
    max_consumed_chars = 0;
    // XXX : need a bail condition -
    // /// so that we can stop accepting data until autowrap or similar
  }
#endif
  len = buf_len; 
  int was_in_scroll = vt->in_scroll;
  if (buf_len) goto b;

  while (timeout > 100 && vt_waitdata (vt, timeout))
  {
    len = vt_read (vt, buf, read_size);
    b:
    if (len > 0)
    {
      int i;
      buf_len = 0;
      for (i = 0; i < len; i++)
      {
        uint8_t byte = buf[i];
        ctx_vt_feed_byte (vt, byte);
	if ((vt->in_scroll && !was_in_scroll )
	    || (i > max_consumed_chars))
	{

          int remaining = len - i - 1;
	  if (remaining > 0)
	  {
	  for (int j = 0; j < remaining; j++)
	  {
            buf[j] = buf[j+i + 1];
	  }
	  buf_len = remaining;
          got_data+=len;
          vt->rev ++;
	  }
	  return got_data;
	}
      }
      buf_len = 0;
      got_data+=len;
      vt->rev ++;
    }
    timeout /= 2;
  }
  return got_data;
}

/******/

static const char *keymap_vt52[][2]={
  {"up",    "\033A" },
  {"down",  "\033B" },
  {"right", "\033C" },
  {"left",  "\033D" },
};

static const char *keymap_application[][2]={
  {"up",    "\033OA" },
  {"down",  "\033OB" },
  {"right", "\033OC" },
  {"left",  "\033OD" },
};

static const char *keymap_general[][2]={
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
  {"page-down" ,     "\033[6~"},
  {"return",         "\r"},
  {"shift-tab",      "\eZ"},
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

  {"control-F1",       "\033[>11~"},
  {"control-F2",       "\033[>12~"},   
  {"control-F3",       "\033[>13~"},
  {"control-F4",       "\033[>14~"},
  {"control-F5",       "\033[>15~"},

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
};



void ctx_vt_feed_keystring (MrgVT *vt, const char *str)
{
  if (vt->in_vt52)
  {
    for (int i = 0; i<sizeof (keymap_vt52)/sizeof(keymap_vt52[0]); i++)
      if (!strcmp (str, keymap_vt52[i][0]))
        { str = keymap_vt52[i][1]; goto done; }
  }
  else
  {
    if (vt->cursor_key_application)
    {
      for (int i = 0; i<sizeof (keymap_application)/sizeof(keymap_application[0]); i++)
        if (!strcmp (str, keymap_application[i][0]))
         { str = keymap_application[i][1]; goto done; }
    }
  }

  if (!strcmp (str, "return"))
  {
    if (vt->cr_on_lf)
      str = "\r\n";
    else
      str = "\r";
    
    goto done;
  }

  if (!strcmp (str, "control-space"))
  {
    str = "\0\0";
    vt_write (vt, str, 1);
    return;
  }

  for (int i = 0; i<sizeof (keymap_general)/sizeof(keymap_general[0]); i++)
  if (!strcmp (str, keymap_general[i][0]))
    { str = keymap_general[i][1];
      break;
    }

done:
  if (strlen (str))
  {
    vt_write (vt, str, strlen (str));
  }
}

void ctx_vt_paste (MrgVT *vt, const char *str)
{
  if (vt->bracket_paste)
  {
    vt_write (vt, "\e[200~", 6);
  }
  ctx_vt_feed_keystring (vt, str);
  if (vt->bracket_paste)
  {
    vt_write (vt, "\e[201~", 6);
  }
}

const char *ctx_vt_find_shell_command (void)
{
  int i;
  const char *command = NULL;
  struct stat stat_buf;
  static char *alts[][2] ={
    {"/bin/bash",     "/bin/bash -i"},
    {"/usr/bin/bash", "/usr/bin/bash -i"},
    {"/bin/sh",       "/bin/sh -i"},
    {"/usr/bin/sh",   "/usr/bin/sh -i"},
    {NULL, NULL}
  };
  for (i = 0; alts[i][0] && !command; i++)
  {
    lstat (alts[i][0], &stat_buf);
    if (S_ISREG(stat_buf.st_mode) || S_ISLNK(stat_buf.st_mode))
      command = alts[i][1];
  }
  return command;
}

static void signal_child (int signum)
{
  pid_t pid;
  int   status;
  while ((pid = waitpid (-1, &status, WNOHANG)) != -1)
    {
      if (pid)
      {
        for (VtList *l = vts; l; l=l->next)
        {
          MrgVT *vt = l->data;
          if (vt->vtpty.pid == pid)
            {
              vt->done = 1;
              vt->result = status;
            }
        }
      }
    }
}

static void ctx_vt_run_command (MrgVT *vt, const char *command)
{
  struct winsize ws;

  static int reaper_started = 0;
  if (!reaper_started)
  {
    reaper_started = 1;
    signal (SIGCHLD, signal_child);
  }

  ws.ws_row = vt->rows;
  ws.ws_col = vt->cols;
  ws.ws_xpixel = ws.ws_col * vt->cw;
  ws.ws_ypixel = ws.ws_row * vt->ch;

  vt->vtpty.pid = forkpty (&vt->vtpty.pty, NULL, NULL, &ws);
  if (vt->vtpty.pid == 0)
  {
    int i;
    for (i = 3; i<768;i++)close(i);/*hack, trying to close xcb */
    unsetenv ("TERM");
    unsetenv ("COLUMNS");
    unsetenv ("LINES");
    unsetenv ("TERMCAP");
    unsetenv ("COLOR_TERM");
    unsetenv ("COLORTERM");
    unsetenv ("VTE_VERSION");
    //setenv ("TERM", "ansi", 1);
    //setenv ("TERM", "vt102", 1);
    //setenv ("TERM", "vt100", 1);
    //setenv ("TERM", "xterm", 1);
    setenv ("TERM", "xterm-256color", 1);
    setenv ("COLORTERM", "truecolor", 1);
    vt->result = system (command);
    exit(0);
  }
  else if (vt->vtpty.pid < 0)
  {
    VT_error ("forkpty failed (%s)", command);
  }
  fcntl(vt->vtpty.pty, F_SETFL, O_NONBLOCK);
}

void ctx_vt_destroy (MrgVT *vt)
{
  while (vt->lines)
  {
    vt_string_free (vt->lines->data, 1);
    vt_list_remove (&vt->lines, vt->lines->data);
    vt->line_count--;
  }

  if (vt->set_style)
    free (vt->set_style);

  if (vt->set_unichar)
    free (vt->set_unichar);

  if (vt->ctx)
    ctx_free (vt->ctx);

  vt_list_remove (&vts, vt);

  kill (vt->vtpty.pid, 9);
  close (vt->vtpty.pty);
  free (vt);
}

int ctx_vt_get_line_count (MrgVT *vt)
{
  return vt->line_count;
}

const char *ctx_vt_get_line (MrgVT *vt, int no)
{
  VtList *l= vt_list_nth (vt->lines, no);
  VtString *str;
  if (!l)
    return NULL;
  str = l->data;
  return str->str;
}

int ctx_vt_get_cols (MrgVT *vt)
{
  return vt->cols;
}

int ctx_vt_get_rows (MrgVT *vt)
{
  return vt->rows;
}

int ctx_vt_get_cursor_x (MrgVT *vt)
{
  return vt->cursor_x;
}

int ctx_vt_get_cursor_y (MrgVT *vt)
{
  return vt->cursor_y;
}

static void draw_braille_bit (Ctx *ctx, float x, float y, float cw, float ch, int u, int v)
{
      ctx_new_path (ctx);
  ctx_rectangle (ctx, 0.167 * cw + x + u * cw * 0.5,
       	     y - ch + 0.1   * ch     + v * ch * 0.25, 

    	  0.33 *cw, 0.33 * cw);
  ctx_fill (ctx);
}

int vt_special_glyph (Ctx *ctx, MrgVT *vt, float x, float y, int cw, int ch, int unichar)
{
  switch (unichar)
  {
     case 0x2594: // UPPER_ONE_EIGHT_BLOCK
      ctx_new_path (ctx);
      { float factor = 1.0f/8.0f;
        ctx_rectangle (ctx, x, y - ch, cw, ch * factor);
        ctx_fill (ctx);
      }
      return 0;
     case 0x2581: // LOWER_ONE_EIGHT_BLOCK:
      ctx_new_path (ctx);
      { float factor = 1.0f/8.0f;
        ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
        ctx_fill (ctx);
      }
      return 0;
     case 0x2582: // LOWER_ONE_QUARTER_BLOCK:
      ctx_new_path (ctx);
      { float factor = 1.0f/4.0f;
        ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
        ctx_fill (ctx);
      }
      return 0;
     case 0x2583: // LOWER_THREE_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      { float factor = 3.0f/8.0f;
        ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
        ctx_fill (ctx);
      }
      return 0;
     case 0x2585: // LOWER_FIVE_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      { float factor = 5.0f/8.0f;
        ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
        ctx_fill (ctx);
      }
      return 0;
     case 0x2586: // LOWER_THREE_QUARTERS_BLOCK:
      ctx_new_path (ctx);
      { float factor = 3.0f/4.0f;
        ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
        ctx_fill (ctx);
      }
      return 0;
     case 0x2587: // LOWER_SEVEN_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      { float factor = 7.0f/8.0f;
        ctx_rectangle (ctx, x, y - ch * factor, cw, ch * factor);
        ctx_fill (ctx);
      }
      return 0;
     case 0x2589: // LEFT_SEVEN_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw*7/8, ch);
      ctx_fill (ctx);
      return 0;
     case 0x258A: // LEFT_THREE_QUARTERS_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw*3/4, ch);
      ctx_fill (ctx);
      return 0;
     case 0x258B: // LEFT_FIVE_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw*5/8, ch);
      ctx_fill (ctx);
      return 0;
     case 0x258D: // LEFT_THREE_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw*3/8, ch);
      ctx_fill (ctx);
      return 0;
     case 0x258E: // LEFT_ONE_QUARTER_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw/4, ch);
      ctx_fill (ctx);
      return 0;
     case 0x258F: // LEFT_ONE_EIGHT_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw/8, ch);
      ctx_fill (ctx);
      return 0;
     case 0x258C: // HALF_LEFT_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw/2, ch);
      ctx_fill (ctx);
      return 0;
     case 0x2590: // HALF_RIGHT_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2, y - ch, cw/2, ch);
      ctx_fill (ctx);
      return 0;

     case 0x1fb8f: // VT_RIGHT_SEVEN_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw*1/8, y - ch, cw*7/8, ch);
      ctx_fill (ctx);
      return 0;
     case 0x1fb8d: // VT_RIGHT_FIVE_EIGHTS_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw*3/8, y - ch, cw*5/8, ch);
      ctx_fill (ctx);
      return 0;

     case 0x1fb8b: // VT_RIGHT_ONE_QUARTER_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw*3/4, y - ch, cw/4, ch);
      ctx_fill (ctx);
      return 0;
     case 0x1fb8e: // VT_RIGHT_THREE_QUARTER_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw*1/4, y - ch, cw*3/4, ch);
      ctx_fill (ctx);
      return 0;
     case 0x2595: // VT_RIGHT_ONE_EIGHT_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw*7/8, y - ch, cw/8, ch);
      ctx_fill (ctx);
      return 0;
     case 0x2580: // HALF_UP_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw, ch/2);
      ctx_fill (ctx);
      return 0;
     case 0x2584: // _HALF_DOWN_BLOCK:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch/2, cw, ch/2);
      ctx_fill (ctx);
      return 0;

     case 0x2596: // _QUADRANT LOWER LEFT
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch/2, cw/2, ch/2);
      ctx_fill (ctx);
      return 0;
     case 0x2597: // _QUADRANT LOWER RIGHT
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x+cw/2, y - ch/2, cw/2, ch/2);
      ctx_fill (ctx);
      return 0;
     case 0x2598: // _QUADRANT UPPER LEFT
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw/2, ch/2);
      ctx_fill (ctx);
      return 0;
     case 0x259D: // _QUADRANT UPPER RIGHT
      ctx_new_path (ctx);
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
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw, ch);
      ctx_fill (ctx);
      return 0;
     case 0x2591: // LIGHT_SHADE:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw, ch);
      ctx_save (ctx);
      ctx_set_global_alpha (ctx, 0.25);
      ctx_fill (ctx);
      ctx_restore (ctx);
      return 0;
     case 0x2592: // MEDIUM_SHADE:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw, ch);
      ctx_save (ctx);
      ctx_set_global_alpha (ctx, 0.5);
      ctx_fill (ctx);
      ctx_restore (ctx);
      return 0;
     case 0x2593: // DARK SHADE:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch, cw, ch);
      ctx_save (ctx);
      ctx_set_global_alpha (ctx, 0.75);
      ctx_fill (ctx);
      ctx_restore (ctx);
      return 0;

     case 0x23BA: //HORIZONTAL_SCANLINE-1
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x,      y - ch + ch*0.1 - ch * 0.1,
		          cw, ch * 0.1);
      ctx_fill (ctx);
      return 0;
     case 0x23BB: //HORIZONTAL_SCANLINE-3
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x,      y - ch + ch*0.3 - ch * 0.075,
		          cw, ch * 0.1);
      ctx_fill (ctx);
      return 0;
     case 0x23BC: //HORIZONTAL_SCANLINE-7
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x,      y - ch + ch*0.7 - ch * 0.025,
		          cw, ch * 0.1);
      ctx_fill (ctx);
      return 0;

     case 0x23BD: //HORIZONTAL_SCANLINE-9
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x,      y - ch + ch*0.9 + ch * 0.0,
		          cw, ch * 0.1);
      ctx_fill (ctx);
      return 0;


     case 0x2500: //VT_BOX_DRAWINGS_LIGHT_HORIZONTAL // and scanline 5
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
      ctx_fill (ctx);
      return 0;
     case 0x2502: // VT_BOX_DRAWINGS_LIGHT_VERTICAL:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
      ctx_fill (ctx);
      return 0;
     case 0x250c: //VT_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, ch * 0.1, ch/2 + ch*0.1);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, cw/2+ ch * 0.1, ch*0.1);
      ctx_fill (ctx);
      return 0;
     case 0x2510: //VT_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, ch * 0.1, ch/2 + ch*0.1);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x, y - ch/2 - ch*0.1/2, cw/2+ ch * 0.1/2, ch*0.1);
      ctx_fill (ctx);
      return 0;
     case 0x2514: //VT_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch/2+ch*0.1/2);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2 - ch*0.1/2, cw/2 + ch * 0.1, ch*0.1);
      ctx_fill (ctx);
      return 0;
     case 0x2518: //VT_BOX_DRAWINGS_LIGHT_UP_AND_LEFT:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch/2+ ch*0.1/2);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x, y - ch/2-ch*0.1/2, cw/2+ch * 0.1/2, ch*0.1);

      ctx_fill (ctx);
      return 0;
     case 0x251C: //VT_BOX_DRAWINGS_LIGHT_VERTICAL_AND_RIGHT:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2-ch*0.1/2, cw/2+ch * 0.1, ch*0.1);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
      ctx_fill (ctx);
      return 0;
     case 0x2524: //VT_BOX_DRAWINGS_LIGHT_VERTICAL_AND_LEFT:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x, y - ch/2-ch*0.1/2, cw/2+ch * 0.1/2, ch*0.1);
      ctx_fill (ctx);

      return 0;
     case 0x252C: // VT_BOX_DRAWINGS_LIGHT_DOWN_AND_HORIZONTAL:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch/2-ch*0.1/2, ch * 0.1, ch/2+ch*0.1);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
      ctx_fill (ctx);
      return 0;
     case 0x2534: // VT_BOX_DRAWINGS_LIGHT_UP_AND_HORIZONTAL:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch /2+ch*0.1/2);
      ctx_fill (ctx);

      return 0;
     case 0x253C: // VT_BOX_DRAWINGS_LIGHT_VERTICAL_AND_HORIZONTAL:
      ctx_new_path (ctx);
      ctx_rectangle (ctx, x, y - ch/2 - ch * 0.1 / 2, cw, ch * 0.1);
      ctx_fill (ctx);
      ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
      ctx_fill (ctx);
      return 0;

     case 0x1fb70: // left triangular one quarter block
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y);
      ctx_rel_line_to (ctx, 0, -ch);
      ctx_rel_line_to (ctx, cw/2, -ch/2);
      ctx_fill (ctx);
      return 0;

     case 0x1fb72: // right triangular one quarter block
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y);
      ctx_rel_move_to (ctx, cw/2, -ch/2);
      ctx_rel_line_to (ctx, cw/2, -ch/2);
      ctx_rel_line_to (ctx, 0, ch);
      ctx_fill (ctx);
      return 0;
     case 0x1fb73: // lower triangular one quarter block
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y);
      ctx_rel_line_to (ctx, cw/2, -ch/2);
      ctx_rel_line_to (ctx, cw/2, ch/2);
      ctx_fill (ctx);
      return 0;
     case 0x1fb71: // upper triangular one quarter block
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y);
      ctx_rel_move_to (ctx, cw/2, -ch/2);
      ctx_rel_line_to (ctx, -cw/2, -ch/2);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_fill (ctx);
      return 0;
     case 0x25E2: // VT_BLACK_LOWER_RIGHT_TRIANGLE:
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y);
      ctx_rel_line_to (ctx, cw, -ch);
      ctx_rel_line_to (ctx, 0, ch);
      ctx_fill (ctx);
      return 0;
     case 0x25E3: //  VT_BLACK_LOWER_LEFT_TRIANGLE:
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y);
      ctx_rel_line_to (ctx, 0, -ch);
      ctx_rel_line_to (ctx, cw, ch);
      ctx_fill (ctx);
      return 0;
  case 0x25E4: // tri
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y);
      ctx_rel_line_to (ctx, 0, -ch);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_fill (ctx);
      return 0;
  case 0x25E5: // tri
      ctx_new_path (ctx);
      ctx_move_to (ctx, x, y - ch);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_rel_line_to (ctx, 0, ch);
      ctx_fill (ctx);
      return 0;
   case 0x2800: case 0x2801: case 0x2802: case 0x2803: case 0x2804: case 0x2805: case 0x2806: case 0x2807: case 0x2808: case 0x2809: case 0x280A: case 0x280B: case 0x280C: case 0x280D: case 0x280E: case 0x280F: case 0x2810: case 0x2811: case 0x2812: case 0x2813: case 0x2814: case 0x2815: case 0x2816: case 0x2817: case 0x2818: case 0x2819: case 0x281A: case 0x281B: case 0x281C: case 0x281D: case 0x281E: case 0x281F: case 0x2820: case 0x2821: case 0x2822: case 0x2823: case 0x2824: case 0x2825: case 0x2826: case 0x2827: case 0x2828: case 0x2829: case 0x282A: case 0x282B: case 0x282C: case 0x282D: case 0x282E: case 0x282F: case 0x2830: case 0x2831: case 0x2832: case 0x2833: case 0x2834: case 0x2835: case 0x2836: case 0x2837: case 0x2838: case 0x2839: case 0x283A: case 0x283B: case 0x283C: case 0x283D: case 0x283E: case 0x283F:

      ctx_new_path (ctx);
      {
	int bit_pattern = unichar - 0x2800;
	int bit = 0;
	int u = 0;
	int v = 0;
	for (bit = 0; bit < 6; bit++)
	{
	  if (bit_pattern & (1<<bit))
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

   case 0x2840: case 0x2841: case 0x2842: case 0x2843: case 0x2844: case 0x2845: case 0x2846: case 0x2847: case 0x2848: case 0x2849: case 0x284A: case 0x284B: case 0x284C: case 0x284D: case 0x284E: case 0x284F: case 0x2850: case 0x2851: case 0x2852: case 0x2853: case 0x2854: case 0x2855: case 0x2856: case 0x2857: case 0x2858: case 0x2859: case 0x285A: case 0x285B: case 0x285C: case 0x285D: case 0x285E: case 0x285F: case 0x2860: case 0x2861: case 0x2862: case 0x2863: case 0x2864: case 0x2865: case 0x2866: case 0x2867: case 0x2868: case 0x2869: case 0x286A: case 0x286B: case 0x286C: case 0x286D: case 0x286E: case 0x286F: case 0x2870: case 0x2871: case 0x2872: case 0x2873: case 0x2874: case 0x2875: case 0x2876: case 0x2877: case 0x2878: case 0x2879: case 0x287A: case 0x287B: case 0x287C: case 0x287D: case 0x287E: case 0x287F:
      ctx_new_path (ctx);
      draw_braille_bit (ctx, x, y, cw, ch, 0, 3);
      {
	int bit_pattern = unichar - 0x2840;
	int bit = 0;
	int u = 0;
	int v = 0;
	for (bit = 0; bit < 6; bit++)
	{
	  if (bit_pattern & (1<<bit))
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
      return 0;

   case 0x2880: case 0x2881: case 0x2882: case 0x2883: case 0x2884: case 0x2885: case 0x2886: case 0x2887: case 0x2888: case 0x2889: case 0x288A: case 0x288B: case 0x288C: case 0x288D: case 0x288E: case 0x288F: case 0x2890: case 0x2891: case 0x2892: case 0x2893: case 0x2894: case 0x2895: case 0x2896: case 0x2897: case 0x2898: case 0x2899: case 0x289A: case 0x289B: case 0x289C: case 0x289D: case 0x289E: case 0x289F: case 0x28A0: case 0x28A1: case 0x28A2: case 0x28A3: case 0x28A4: case 0x28A5: case 0x28A6: case 0x28A7: case 0x28A8: case 0x28A9: case 0x28AA: case 0x28AB: case 0x28AC: case 0x28AD: case 0x28AE: case 0x28AF: case 0x28B0: case 0x28B1: case 0x28B2: case 0x28B3: case 0x28B4: case 0x28B5: case 0x28B6: case 0x28B7: case 0x28B8: case 0x28B9: case 0x28BA: case 0x28BB: case 0x28BC: case 0x28BD: case 0x28BE: case 0x28BF:
      ctx_new_path (ctx);
      draw_braille_bit (ctx, x, y, cw, ch, 1, 3);
      {
	int bit_pattern = unichar - 0x2880;
	int bit = 0;
	int u = 0;
	int v = 0;
	for (bit = 0; bit < 6; bit++)
	{
	  if (bit_pattern & (1<<bit))
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
      return 0;
   case 0x28C0: case 0x28C1: case 0x28C2: case 0x28C3: case 0x28C4: case 0x28C5: case 0x28C6: case 0x28C7: case 0x28C8: case 0x28C9: case 0x28CA: case 0x28CB: case 0x28CC: case 0x28CD: case 0x28CE: case 0x28CF: case 0x28D0: case 0x28D1: case 0x28D2: case 0x28D3: case 0x28D4: case 0x28D5: case 0x28D6: case 0x28D7: case 0x28D8: case 0x28D9: case 0x28DA: case 0x28DB: case 0x28DC: case 0x28DD: case 0x28DE: case 0x28DF: case 0x28E0: case 0x28E1: case 0x28E2: case 0x28E3: case 0x28E4: case 0x28E5: case 0x28E6: case 0x28E7: case 0x28E8: case 0x28E9: case 0x28EA: case 0x28EB: case 0x28EC: case 0x28ED: case 0x28EE: case 0x28EF: case 0x28F0: case 0x28F1: case 0x28F2: case 0x28F3: case 0x28F4: case 0x28F5: case 0x28F6: case 0x28F7: case 0x28F8: case 0x28F9: case 0x28FA: case 0x28FB: case 0x28FC: case 0x28FD: case 0x28FE: case 0x28FF:
      draw_braille_bit (ctx, x, y, cw, ch, 0, 3);
      draw_braille_bit (ctx, x, y, cw, ch, 1, 3);
      {
	int bit_pattern = unichar - 0x28C0;
	int bit = 0;
	int u = 0;
	int v = 0;
	for (bit = 0; bit < 6; bit++)
	{
	  if (bit_pattern & (1<<bit))
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
      return 0;

  }
  return -1;
}

void vt_ctx_glyph (Ctx *ctx, MrgVT *vt, float x, float y, int unichar, int bold, float scale_x, float scale_y, float offset_y)
{
  if (unichar <= ' ')
    return;

  scale_x *= vt->scale_x;
  scale_y *= vt->scale_y;

  if (!vt_special_glyph (ctx, vt, x, y + offset_y * vt->ch, vt->cw * scale_x, vt->ch * scale_y, unichar))
    return;


  ctx_save (ctx);
  if (scale_x != 1.0 || scale_y != 1.0)
  {
    ctx_translate (ctx, x, y);
    ctx_scale (ctx, scale_x, scale_y);
    ctx_translate (ctx, -x, -y);
  }
  ctx_translate (ctx, 0, vt->font_size * offset_y);

  y -= vt->font_size * 0.2;

  if (bold)
  {
    ctx_move_to (ctx, x, y);
    ctx_set_line_width (ctx, vt->font_size/35.0);
    ctx_glyph (ctx, unichar, 1);
  }

  ctx_move_to (ctx, x, y);
  ctx_glyph (ctx, unichar, 0);
  ctx_restore (ctx);
}

//static uint8_t palette[256][3];

/* optimized for ANSI ART - and avoidance of human metamers
 * among color deficient vision - by distributing and pertubating
 * until all 64 combinations - sans self application, have
 * likely to be discernable by humans.
 */

static uint8_t palettes[][16][3]={
	{

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
 {196,196,196},// 7                light-gray
 { 85, 85, 85},// 8                dark gray

 {240, 60, 40},// 9                light red
 {170,240, 80},// 10               light green
 {248,248,  0},// 11               light yellow
 {  0, 40,255},// 12               light blue
 {204, 62,214},// 13               light magenta
 { 10,234,254},// 14               light cyan
 {255,255,255},// 15 - foreground (white)
	},
 /* inspired by DEC */
{{  0,  0,  0}, // 0 - background  black
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

void vt_ctx_set_color (MrgVT *vt, Ctx *ctx, int no, int intensity)
{
  uint8_t r = 0, g = 0, b = 0;

  if (no < 16 && no >= 0)
  {
    switch (intensity)
    {
      case 0: no = 0; break;
      case 1: 
	   // 15 becomes 7
	   if (no == 15) no = 8;
	   else if (no > 8) no -= 8;
	   break;
      case 2:
	   /* give the normal color special treatment, and in really normal
	    * cirumstances it is the dim variant of foreground that is used
	    */
	   if (no == 15) no = 7;
           break;
      case 3:
      case 4:
	   if (no < 8)
             no += 8;
           break;
      default:
	   break;
    }

    r = palettes[vt->palette_no][no][0];
    g = palettes[vt->palette_no][no][1];
    b = palettes[vt->palette_no][no][2];
  } else if (no < 16 + 6*6*6)
  {
    no = no-16;
    b = (no % 6) * 255 / 5;
    no /= 6;
    g = (no % 6) * 255 / 5;
    no /= 6;
    r = (no % 6) * 255 / 5;
  } else
  {
    int gray = no - (16 + 6*6*6);
    float val = gray * 255 / 24;
    r = g = b = val;
  }

  ctx_set_rgba_u8 (ctx, r, g, b, 255);
}

int ctx_vt_keyrepeat (MrgVT *vt)
{
  return vt->keyrepeat;
}

float ctx_vt_draw_cell (MrgVT *vt, Ctx *ctx,
 		        int   row, int col, // pass 0 to force draw - like
 		        float x0, float y0, // for scrollback visible
		        uint64_t style,
		        uint32_t unichar,
		        int      bg, int fg,
			int      dw, int dh,
			int in_scroll)
	              // dw is 0 or 1 
		      // dh is 0 1 or -1  1 is upper -1 is lower
{
  int on_white = vt->reverse_video;

  int color = 0;
  int bold = (style & STYLE_BOLD) != 0;
  int dim = (style & STYLE_DIM) != 0;
  int hidden = (style & STYLE_HIDDEN) != 0;
  int proportional = (style & STYLE_PROPORTIONAL) != 0;

  int fg_set = (style & STYLE_FG_COLOR_SET) != 0;

  int bg_intensity = 0;
  int fg_intensity = 2;

  int reverse = ((style & STYLE_REVERSE) != 0);

  int blink = ((style & STYLE_BLINK) != 0);
  int blink_fast = ((style & STYLE_BLINK_FAST) != 0);

  int cw = vt->cw;
  int ch = vt->ch;

  if (proportional)
  {
    if (vt->font_is_mono)
    {
      ctx_set_font (ctx, "regular");
      vt->font_is_mono = 0;
    }
    cw = ctx_glyph_width (ctx, unichar);
  }
  else
  {
    if (vt->font_is_mono == 0)
    {
      ctx_set_font (ctx, "mono");
      vt->font_is_mono = 1;

      if (col != 1) {
	int x = x0;
	int new_cw = cw - ((x % cw));
	if (new_cw < cw*3/2)
	  new_cw += cw;
	cw = new_cw;
      }
    }
  }

  float scale_x = 1.0f;
  float scale_y = 1.0f;
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

  if (in_scroll)
  {
    offset_y -= vt->scroll_offset / (dh?2:1);
  }


  cw *= scale_x;

  if (row && col && ! proportional && (scale_x == 1.0f))
  {
    if (vt->set_unichar[row*vt->cols*2+col] == unichar &&
        vt->set_style[row*vt->cols*2+col] == style)
      return cw;
    vt->set_unichar[row*vt->cols*2+col] = unichar;
    vt->set_style[row*vt->cols*2+col] = style;
  }

  if (blink_fast)
  {
    if ((vt->blink_state % 2) == 0)
      blink = 1;
    else
      blink = 0;
  } else if (blink)
  {
    if ((vt->blink_state % 10) < 5)
      blink = 1;
    else
      blink = 0;
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
  if ((reverse) == 0)
  {
     if (bold)
     {
       bg_intensity =           2;
       fg_intensity = blink?1:  3;
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
  else
  {
     if (bold)
     {
       bg_intensity = blink?2:  0;
       fg_intensity = blink?3:  3;
     }
     else if (dim)
     {
       bg_intensity = blink?2:  0;
       fg_intensity = blink?0:  1;
     }
     else
     {
       bg_intensity = blink?2:  0;
       fg_intensity = blink?0:  2;
     }
  }
  }
  else /* bright on dark */
  {
  if (reverse == 0)
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
  else
  {
     if (bold)
     {
       bg_intensity = blink?0:  2;
       fg_intensity = blink?3:  0;
     }
     else if (dim)
     {
       bg_intensity = blink?0:  2;
       fg_intensity = blink?1:  0;
     }
     else
     {
       bg_intensity = blink?0:  2;
       fg_intensity = blink?2:  0;
     }
  }
  }

  if (bg)  {

  ctx_new_path (ctx);
  if (style &  STYLE_BG24_COLOR_SET)
  {
    // XXX - this case seem to be missing reverse handling
    uint64_t temp = style >> 40;
    int r = temp & 0xff;
    temp >>= 8;
    int g = temp & 0xff;
    temp >>= 8;
    int b = temp & 0xff;
    if (dh)
      r= g = b = 30;
    ctx_set_rgba_u8 (ctx, r, g, b, 255);
  }
  else
  {
    if (style & STYLE_BG_COLOR_SET)
    {
       if (reverse)
       {
         color = (style >> 16) & 255;
       }
       else
       {
         color = (style >> 40) & 255;
       }
      bg_intensity = -1;

      vt_ctx_set_color (vt, ctx, color, bg_intensity);
    }
    else
    {
	uint8_t rgb[3]={0,};
	switch (bg_intensity)
	{
	  case 0:
	    for (int i = 0; i <3 ;i++)
              rgb[i] = vt->bg_color[i];
	    break;
	  case 1:
	    for (int i = 0; i <3 ;i++)
              rgb[i] = vt->bg_color[i] * 0.5 + vt->fg_color[i] * 0.5;
	    break;
	  case 2:
	    for (int i = 0; i <3 ;i++)
              rgb[i] = vt->bg_color[i] * 0.05 + vt->fg_color[i] * 0.95;
	    break;
	  case 3:
	    for (int i = 0; i <3 ;i++)
	      rgb[i] = vt->fg_color[i];
	    break;
	}
        ctx_set_rgba_u8 (ctx, rgb[0],
                              rgb[1],
                              rgb[2], 255);
    }
  }

    if (dh)
    {
      ctx_rectangle (ctx, x0, y0 - ch - ch * (vt->scroll_offset), cw, ch);
    }
    else
    {
      ctx_rectangle (ctx, x0, y0 - ch + ch * offset_y, cw, ch);
    }
    ctx_fill (ctx);
  }

  if (!fg) return cw;

  int italic        = (style & STYLE_ITALIC) != 0;
  int strikethrough = (style & STYLE_STRIKETHROUGH) != 0;
  int overline      = (style & STYLE_OVERLINE) != 0;
  int underline     = (style & STYLE_UNDERLINE) != 0;
  int underline_var = (style & STYLE_UNDERLINE_VAR) != 0;
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

  if (!hidden)
  {
    if (style & STYLE_FG24_COLOR_SET)
    {
      uint64_t temp = style >> 16;
      int r = temp & 0xff;
      temp >>= 8;
      int g = temp & 0xff;
      temp >>= 8;
      int b = temp & 0xff;
      ctx_set_rgba_u8 (ctx, r, g, b, 255);
    }
    else
    {
      if ((style & STYLE_FG_COLOR_SET) == 0)
      {
	uint8_t rgb[3]={0,};

	switch (fg_intensity)
	{
	  case 0:
	    for (int i = 0; i <3 ;i++)
              rgb[i] = vt->bg_color[i] * 0.7 + vt->fg_color[i] * 0.3;
	    break;
	  case 1:
	    for (int i = 0; i <3 ;i++)
              rgb[i] = vt->bg_color[i] * 0.5 + vt->fg_color[i] * 0.5;
	    break;
	  case 2:
	    for (int i = 0; i <3 ;i++)
              rgb[i] = vt->bg_color[i] * 0.20 + vt->fg_color[i] * 0.80;
	    break;
	  case 3:
	    for (int i = 0; i <3 ;i++)
              rgb[i] = vt->fg_color[i];
	}
        ctx_set_rgba_u8 (ctx, rgb[0],
                              rgb[1],
                              rgb[2], 255);
      }
      else
      {
        if (reverse) 
          color = (style >> 40) & 255;
        else
          color = (style >> 16) & 255;
        bg_intensity = -1;
        vt_ctx_set_color (vt, ctx, color, fg_intensity);
      }
    }
    if (italic)
    {
      ctx_save (ctx);
      ctx_translate (ctx, (x0 + cw/3), (y0 + vt->ch/2));
      ctx_scale (ctx, 0.9, 0.9);
      ctx_rotate (ctx, 0.15);
      ctx_translate (ctx, -(x0 + cw/3), -(y0 + vt->ch/2) );
    }

    vt_ctx_glyph (ctx, vt, x0, y0, unichar, bold, scale_x, scale_y, offset_y);
    if (italic)
    {
      ctx_restore (ctx);
    }
    if (curved_underline)
    {
      ctx_new_path (ctx);
      ctx_move_to (ctx, x0, y0 - vt->font_size * 0.07 - vt->ch * vt->scroll_offset);

      ctx_rel_line_to (ctx, (cw+2)/3, -vt->ch * 0.05);
      ctx_rel_line_to (ctx, (cw+2)/3, vt->ch * 0.1);
      ctx_rel_line_to (ctx, (cw+2)/3, -vt->ch * 0.05);
      //ctx_rel_line_to (ctx, cw, 0);

      ctx_set_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.050:0.04));
      ctx_stroke (ctx);
    }
    else if (double_underline)
    {
      ctx_new_path (ctx);
      ctx_move_to (ctx, x0, y0 - vt->font_size * 0.130 - vt->ch * vt->scroll_offset);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_move_to (ctx, x0, y0 - vt->font_size * 0.030 - vt->ch * vt->scroll_offset);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_set_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.050:0.04));
      ctx_stroke (ctx);
    }
    else if (underline)
    {
      ctx_new_path (ctx);
      ctx_move_to (ctx, x0, y0 - vt->font_size * 0.07 - vt->ch * vt->scroll_offset);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_set_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.075:0.05));
      ctx_stroke (ctx);
    }
    if (overline)
    {
      ctx_new_path (ctx);
      ctx_move_to (ctx, x0, y0 - vt->font_size * 0.94 - vt->ch * vt->scroll_offset);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_set_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.075:0.05));
      ctx_stroke (ctx);
    }
    if (strikethrough)
    {
      ctx_new_path (ctx);
      ctx_move_to (ctx, x0, y0 - vt->font_size * 0.43 - vt->ch * vt->scroll_offset);
      ctx_rel_line_to (ctx, cw, 0);
      ctx_set_line_width (ctx, vt->font_size * (style &  STYLE_BOLD?0.075:0.05));
      ctx_stroke (ctx);
    }
  }
  return cw;
}

int ctx_vt_has_blink (MrgVT *vt)
{
  return vt->has_blink + (vt->in_scroll ?  10 : 0);
}

void ctx_vt_draw (MrgVT *vt, Ctx *ctx, double x0, double y0)
{
  ctx_save (ctx);
  ctx_set_font (ctx, "regular");
  vt->font_is_mono = 0;
  ctx_set_font_size (ctx, vt->font_size * vt->font_to_cell_scale);

  vt->has_blink = 0;

  vt->blink_state++;
  //fprintf (stderr, "{%i}", vt->blink_state);
  int cursor_x_px = 0;
  int cursor_y_px = 0;
  int cursor_w = vt->cw;
  int cursor_h = vt->ch;

  cursor_x_px = x0 + (vt->cursor_x - 1) * vt->cw;
  cursor_y_px = y0 + (vt->cursor_y - 1) * vt->ch;
  cursor_w = vt->cw;
  cursor_h = vt->ch;

  if (vt->scroll)
  {
    ctx_new_path (ctx);
    ctx_rectangle (ctx, 0, 0, (vt->cols + 1) * vt->cw,
		              (vt->rows + 1) * vt->ch);
    if (vt->reverse_video)
    {
      ctx_set_rgba (ctx, 1,1,1,1);
      ctx_fill  (ctx);
      ctx_set_rgba (ctx, 0,0,0,1);
    }
    else
    {
      ctx_set_rgba (ctx, 0,0,0,1);
      ctx_fill  (ctx);
      ctx_set_rgba (ctx, 1,1,1,1);
    }
  
    ctx_translate (ctx, 0.0, vt->ch * vt->scroll);
  }

  /* draw terminal lines */

#if 0

   when in scroll...
	   first draw things in scrolling region
   then draw all else,

#endif

  {
    for (int row = 0; row <= (vt->scroll) + vt->rows; row ++)
    {
      VtList *l = vt_list_nth (vt->lines, row);
      float y = y0 + vt->ch * (vt->rows - row);
      if (row >= vt->rows)
      {
	 l = vt_list_nth (vt->scrollback, row-vt->rows);
      }

      if (l && y <= (vt->rows - vt->scroll) *  vt->ch)
      {
	VtString *line = l->data;
        const char *data = line->str;
        const char *d = data;
        float x = x0;
	uint64_t style = 0;
	uint32_t unichar = 0;
	int r = vt->rows - row;
	int in_scrolling_region = vt->in_scroll && ((r >= vt->margin_top && r <= vt->margin_bottom) || r <= 0);

	if (line->double_width)
          vt_cell_cache_clear_row (vt, r);
        for (int col = 1; col <= vt->cols * 1.33 && x < vt->cols * vt->cw; col++)
        {

	  int c = col;
	  int real_cw;
	  if (vt->scroll)
	  {
	    /* this prevents draw_cell from using cache */
            r = c = 0;
	  }
	  style = vt_string_get_style (line, col-1);
	  unichar = d?ctx_utf8_to_unichar (d):' ';

          real_cw=ctx_vt_draw_cell (vt, ctx, r, c, x, y, style, unichar, 1, 1,
	    line->double_width,
	    line->double_height_top?1:
	    line->double_height_bottom?-1:0,
	    in_scrolling_region);
	  if (r == vt->cursor_y && col == vt->cursor_x)
	  {
	    cursor_x_px = x;
	  }

	  x+=real_cw;

          if (style & STYLE_BLINK ||
              style & STYLE_BLINK_FAST)
	  {
	    vt->has_blink = 1;
            vt_cell_cache_reset (vt, r, c);
	  }
          if (style & STYLE_PROPORTIONAL)
	  {
            vt_cell_cache_clear_row (vt, r);
	  }

	  if (d)
	  {
            d = mrg_utf8_skip (d, 1);
	    if (!*d) d = NULL;
	  }
        }
	while (x < vt->cols * vt->cw)
	{
          x+=ctx_vt_draw_cell (vt, ctx, -1, -1, x, y, style, ' ', 1, 1,
			  
	    line->double_width,
	    line->double_height_top?1:
	    line->double_height_bottom?-1:0,
	    in_scrolling_region);
	}

	for (int i = 0; i < 4; i++)
	{
	  Image *image = line->images[i];
	  if (image)
	  {
	     ctx_save (ctx);
	     ctx_image_memory (ctx, image->width, image->height, image->kitty_format,
			     image->data, 
	      (line->image_col[i]-1) * vt->cw, y);
       //      ctx_set_rgba_u8 (ctx, 0xff,0,0,0xff);
	     ctx_rectangle (ctx, line->image_col[i] * vt->cw, y, image->width, image->height);
	     ctx_fill (ctx);
	     ctx_restore (ctx);
	     fprintf (stderr, "draw %ix%i image at col %i\n", image->width, image->height, line->image_col[i]);
	  }
	}
      }
    }
  }

  { /* draw ctx graphics */
    int got_ctx = 0;
    float y = y0 + vt->ch * vt->rows;
    for (int row = 0; y > -(vt->scroll + 8) * vt->ch; row ++)
    {
      VtList *l = vt_list_nth (vt->lines, row);
      if (row >= vt->rows)
      {
	 l = vt_list_nth (vt->scrollback, row-vt->rows);
      }
      if (l && y <= (vt->rows - vt->scroll) *  vt->ch)
      {
	VtString *line = l->data;
	if (line->ctx)
	{
          ctx_save (ctx);
          ctx_translate (ctx, 0, (vt->rows-row) * (vt->ch -1));
          //float factor = vt->cols * vt->cw / 1000.0;
          //ctx_scale (ctx, factor, factor);
          ctx_render_ctx (line->ctx, ctx);
          ctx_restore (ctx);
	  got_ctx = 1;
	}
      }
      y -= vt->ch;
    }
    if (got_ctx)
    {
      /* if we knew the bounds of rendered ctx data - we can do better */
      vt_cell_cache_clear (vt);
    }
  }

#define MIN(a,b)  ((a)<(b)?(a):(b))
  /* draw cursor */
  if (vt->cursor_visible)
  {
    vt_cell_cache_reset (vt, vt->cursor_y, vt->cursor_x);
    ctx_set_rgba (ctx, 1.0, 1.0, 0.0, 0.3333);
    ctx_new_path (ctx);
    ctx_rectangle (ctx,
               cursor_x_px, cursor_y_px,
               cursor_w, cursor_h);
    ctx_fill (ctx);
  }

  for (int i = 0; i < 4; i++)
  {
    if (vt->leds[i])
    {
       ctx_set_rgba (ctx, .5,1,.5,0.8);
       ctx_rectangle (ctx, vt->cw * i + vt->cw * 0.25, vt->ch * 0.25, vt->cw/2, vt->ch/2);
       ctx_fill (ctx);
    }
  }

  ctx_restore (ctx);

  if (vt->scroll != 0)
  {
    float disp_lines = vt->rows;
    float tot_lines = vt->line_count + vt->scrollback_count;
    float offset = (tot_lines - disp_lines - vt->scroll) / tot_lines;
    float win_len = disp_lines / tot_lines;

    vt_cell_cache_clear (vt);
    ctx_rectangle (ctx, vt->cw * (vt->cols - 2),
		        0, 2 * vt->cw,
			vt->rows * vt->ch);
    ctx_set_rgba (ctx, 1, 1, 1, .5);
    ctx_fill (ctx);

    ctx_rectangle (ctx, vt->cw * (vt->cols - 2 + 0.1),
		        offset * vt->rows * vt->ch, (2-0.2) * vt->cw,
			win_len * vt->rows * vt->ch);
    ctx_set_rgba (ctx, 0, 0, 0, .5);
    ctx_fill (ctx);

  }

//#define SCROLL_SPEED 0.25;
#define SCROLL_SPEED 0.05;

  if (vt->in_scroll)
  {
    vt_cell_cache_clear (vt);

    if (vt->in_scroll<0)
    {
      vt->scroll_offset += SCROLL_SPEED;
      if (vt->scroll_offset >= 0.0)
      {
        vt->scroll_offset = 0;
        vt->in_scroll = 0;
	vt->rev++;
      }
    }
    else
    {
      vt->scroll_offset -= SCROLL_SPEED;
      if (vt->scroll_offset <= 0.0)
      {
        vt->scroll_offset = 0;
        vt->in_scroll = 0;
	vt->rev++;
      }

    }
  }
}

int ctx_vt_is_done (MrgVT *vt)
{
  return vt->done;
}

int ctx_vt_get_result (MrgVT *vt)
{
  /* we could block - at least for a while, here..? */
  return vt->result;
}

void ctx_vt_set_scrollback_lines (MrgVT *vt, int scrollback_lines)
{
  vt->scrollback_limit = scrollback_lines;
}

int  ctx_vt_get_scrollback_lines (MrgVT *vt)
{
  return vt->scrollback_limit;
}

void ctx_vt_set_scroll (MrgVT *vt, int scroll)
{
  if (vt->scroll == scroll)
    return;

  vt->scroll = scroll;

  if (vt->scroll > vt_list_length (vt->scrollback) )
    vt->scroll = vt_list_length (vt->scrollback);
  if (vt->scroll < 0)
    vt->scroll = 0;

  vt_cell_cache_clear (vt);
}

int ctx_vt_get_scroll (MrgVT *vt)
{
  return vt->scroll;
}

void ctx_vt_mouse (MrgVT *vt, VtMouseEvent type, int x, int y, int px_x, int px_y)
{
  static int lastx=-1; // XXX  : need one per vt
  static int lasty=-1;

  if ((type == VT_MOUSE_DRAG || type == VT_MOUSE_PRESS)
      && x > vt->cols - 3)
  {
    float disp_lines = vt->rows;
    float tot_lines = vt->line_count + vt->scrollback_count;
    vt->scroll = tot_lines - disp_lines - (px_y*1.0/(vt->rows * vt->ch))* tot_lines;
  }


  char buf[64]="";
  int button_state = 0;

  if (!(vt->mouse | vt->mouse_all | vt->mouse_drag))
    return;

  if (type == VT_MOUSE_MOTION)
    button_state = 3;

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
       if (x==lastx && y==lasty)
         return;
       lastx = x; lasty = y;
       sprintf (buf, "\e[<35;%i;%iM", x, y);
       break;
    case VT_MOUSE_RELEASE:
       if (vt->mouse_decimal == 0)
         button_state = 3;
       break;
    case VT_MOUSE_PRESS:
       button_state = 0;
       break;
    case VT_MOUSE_DRAG:
       if (!(vt->mouse_all || vt->mouse_drag))
         return;
       button_state = 32;
       break;
  }
  // todo : mix in ctrl/meta state

  if (vt->mouse_decimal)
  {
    sprintf (buf, "\e[<%i;%i;%i%c", button_state, x, y, type == VT_MOUSE_RELEASE?'m':'M');
  }
  else
    sprintf (buf, "\e[M%c%c%c", button_state + 32, x + 32, y + 32);

  if (buf[0])
  {
    vt_write (vt, buf, strlen (buf));
    fflush (NULL);
  }
}

pid_t ctx_vt_get_pid (MrgVT *vt)
{
  return vt->vtpty.pid;
}
