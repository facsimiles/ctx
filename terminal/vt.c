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
//static int vt_log_mask = VT_LOG_WARNING | VT_LOG_ERROR;// | VT_LOG_COMMAND;// | VT_LOG_INFO | VT_LOG_COMMAND;
//static int vt_log_mask = VT_LOG_WARNING | VT_LOG_ERROR | VT_LOG_INFO | VT_LOG_COMMAND | VT_LOG_INPUT;
//static int vt_log_mask = VT_LOG_ALL;

#if 1
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

#define MAX_COLS 2048 // used for tabstops

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
  return image;
}

typedef struct AudioState
{
  int action;
  int samplerate; // 8000
  int channels;   // 1
  int bits;       // 8
  int type;       // 'u'    u-law  f-loat  s-igned u-nsigned
  int buffer_size; // desired size of audiofragment in frames
  // (both for feeding SDL and as desired chunking
  //  size)


  int mic;        // <- should
  //    request permisson,
  //    and if gotten, start streaming
  //    audio packets in the incoming direction
  //
  int encoding;   // 'a' ascci85 'b' base64
  int compression; // '0': none , 'z': zlib   'o': opus(reserved)

  int frames;

  uint8_t *data;
  int      data_size;
} AudioState;


typedef struct GfxState
{
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

#include "terminal.h"

struct _VT
{
  VtPty      vtpty;
  int       id;
  unsigned char buf[BUFSIZ]; // need one per vt
  int keyrepeat;
  int       lastx;
  int       lasty;
  int        result;
  long       rev;
  //SDL_Rect   dirty;
  float  dirtpad;
  float  dirtpad1;
  float  dirtpad2;
  float  dirtpad3;

  void  *client;

  ssize_t (*write) (void *serial_obj, const void *buf, size_t count);
  ssize_t (*read) (void *serial_obj, void *buf, size_t count);
  int    (*waitdata) (void *serial_obj, int timeout);
  void  (*resize) (void *serial_obj, int cols, int rows, int px_width, int px_height);


  char     *title;
  void    (*state) (VT *vt, int byte);

  AudioState audio; // < want to move this one up and share impl
  GfxState   gfx;

  CtxList   *saved_lines;
  int       in_alt_screen;
  int       saved_line_count;
  CtxList   *lines;
  int       line_count;
  CtxList   *scrollback;
  int       scrollback_count;
  int       leds[4];
  uint64_t  cstyle;

  uint8_t   fg_color[3];
  uint8_t   bg_color[3];

  int       in_smooth_scroll;
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
  int       ctx_events;
  int       font_is_mono;
  int       palette_no;
  int       has_blink; // if any of the set characters are blinking
  // updated on each draw of the screen

  int unit_pixels;
  int mouse;
  int mouse_drag;
  int mouse_all;
  int mouse_decimal;

  uint8_t    utf8_holding[64]; /* only 4 needed for utf8 - but it's purpose
                                 is also overloaded for ctx journal command
                                 buffering , and the bigger sizes for the svg-like
                                 ctx parsing mode */
  int        utf8_expected_bytes;
  int        utf8_pos;


  CtxParser *ctxp;
  // text related data
  float      letter_spacing;

  float      word_spacing;
  float      font_stretch;  // horizontal expansion
  float      font_size_adjust;
  // font-variant
  // font-weight
  // text-decoration

  int        encoding;  // 0 = utf8 1=pc vga 2=ascii

  int        local_editing; /* terminal operates without pty  */

  int        insert_mode;
  int        autowrap;
  int        justify;
  float      cursor_x;
  int        cursor_y;
  int        cols;
  int        rows;
  VtLine    *current_line;


  int        cr_on_lf;
  int        cursor_visible;
  int        saved_x;
  int        saved_y;
  uint32_t   saved_style;
  int        saved_origin;
  int        cursor_key_application;
  int        key_2017;
  int        margin_top;
  int        margin_bottom;
  int        margin_left;
  int        margin_right;

  int        left_right_margin_mode;

  int        scrollback_limit;
  float      scroll;

#define MAX_ARGUMENT_BUF_LEN (8192 + 16)

  char       *argument_buf;
  int        argument_buf_len;
  int        argument_buf_cap;
  uint8_t    tabs[MAX_COLS];
  int        inert;


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

  int cursor_down;

  int select_begin_col;
  int select_begin_row;
  int select_start_col;
  int select_start_row;
  int select_end_col;
  int select_end_row;

};


/* on current line */
static int vt_col_to_pos (VT *vt, int col)
{
  int pos = col;
  if (vt->current_line->contains_proportional)
    {
      Ctx *ctx = ctx_new ();
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

static ssize_t vt_write (VT *vt, const void *buf, size_t count)
{
  if (!vt->write) { return 0; }
  return vt->write (&vt->vtpty, buf, count);
}
static ssize_t vt_read (VT *vt, void *buf, size_t count)
{
  if (!vt->read) { return 0; }
  return vt->read (&vt->vtpty, buf, count);
}
static int vt_waitdata (VT *vt, int timeout)
{
  if (!vt->waitdata) { return 0; }
  return vt->waitdata (&vt->vtpty, timeout);
}
static void vt_resize (VT *vt, int cols, int rows, int px_width, int px_height)
{
  if (vt->resize)
    { vt->resize (&vt->vtpty, cols, rows, px_width, px_height); }
}


void vt_rev_inc (VT *vt)
{
  vt->rev++;
}

long vt_rev (VT *vt)
{
  return vt->rev;
}

static void vtcmd_reset_to_initial_state (VT *vt, const char *sequence);
static int ct_set_prop (VT *vt, uint32_t key_hash, const char *val);

static void vt_set_title (VT *vt, const char *new_title)
{
  if (vt->inert) { return; }

  if (vt->title)
    { free (vt->title); }
  vt->title = strdup (new_title);
  //client_set_title (vt->id, new_title);//vt->title);
  ct_set_prop (vt, ctx_strhash ("title", 0), (char*)new_title);
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
  /* populate lines */
  for (int i=0; i<vt->rows; i++)
    {
      vt->current_line = vt_line_new_with_size ("", vt->cols);
      ctx_list_prepend (&vt->lines, vt->current_line);
      vt->line_count++;
    }
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


static void _vt_compute_cw_ch (VT *vt)
{
  vt->cw = (vt->font_size / vt->line_spacing * vt->scale_x) + 0.99;
  vt->ch = vt->font_size;
}

static void vtcmd_set_132_col (VT  *vt, int set)
{
  // this should probably force the window as well
  if (set == 0 && vt->scale_x == 1.0f) { return; }
  if (set == 1 && vt->scale_x != 1.0f) { return; }
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
  vt->vtpty.done              = 0;
  vt->result                 = -1;
  vt->state                  = vt_state_neutral;
  vt->unit_pixels   = 0;
  vt->mouse         = 0;
  vt->mouse_drag    = 0;
  vt->mouse_all     = 0;
  vt->mouse_decimal = 0;
  _vt_compute_cw_ch (vt);
  for (int i = 0; i < MAX_COLS; i++)
    { vt->tabs[i] = i % 8 == 0? 1 : 0; }
  _vt_move_to (vt, vt->margin_top, vt->cursor_x);
  vt_carriage_return (vt);
  if (vt->ctx)
    { ctx_reset (vt->ctx); }
  vt->audio.bits = 8;
  vt->audio.channels = 1;
  vt->audio.type = 'u';
  vt->audio.samplerate = 8000;
  vt->audio.buffer_size = 512;
  vt->audio.encoding = 'a';
  vt->audio.compression = '0';
  vt->audio.mic = 0;
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

VT *vt_new (const char *command, int cols, int rows, float font_size, float line_spacing, int id)
{
  VT *vt         = calloc (sizeof (VT), 1);
  vt->id = id;
  vt->lastx = -1;
  vt->lasty = -1;
  vt->state         = vt_state_neutral;
  vt->smooth_scroll = 0;
  vt->scroll_offset = 0.0;
  vt->waitdata      = vtpty_waitdata;
  vt->read          = vtpty_read;
  vt->write         = vtpty_write;
  vt->resize        = vtpty_resize;
  vt->font_to_cell_scale = 1.0;
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
  vt_set_font_size (vt, font_size);
  vt_set_line_spacing (vt, line_spacing);
  if (command)
    {
      vt_run_command (vt, command, "xterm");
    }
  if (cols <= 0) { cols = DEFAULT_COLS; }
  if (rows <= 0) { cols = DEFAULT_ROWS; }
  vt_set_term_size (vt, cols, rows);
  vt->fg_color[0] = 216;
  vt->fg_color[1] = 216;
  vt->fg_color[2] = 216;
  vt->bg_color[0] = 0;
  vt->bg_color[1] = 0;
  vt->bg_color[2] = 0;
  vtcmd_reset_to_initial_state (vt, NULL);
  //vt->ctx = ctx_new ();
  ctx_list_prepend (&vts, vt);
  return vt;
}

int vt_cw (VT *vt)
{
  return vt->cw;
}

int vt_ch (VT *vt)
{
  return vt->ch;
}

void vt_set_mmm (VT *vt, void *mmm)
{
  vt->mmm = mmm;
}

static int vt_trimlines (VT *vt, int max)
{
  CtxList *chop_point = NULL;
  CtxList *l;
  int i;
  if (vt->line_count < max)
    { return 0; }
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
  if (vt->scrollback_count > vt->scrollback_limit + 128)
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


void vt_set_term_size (VT *vt, int icols, int irows)
{
  if (vt->rows == irows && vt->cols == icols)
    { return; }
  while (irows > vt->rows)
    {
      if (vt->scrollback_count)
        {
          vt->scrollback_count--;
          ctx_list_append (&vt->lines, vt->scrollback->data);
          ctx_list_remove (&vt->scrollback, vt->scrollback->data);
        }
      else
        {
          ctx_list_append (&vt->lines, vt_line_new_with_size ("", vt->cols) );
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
  vt_trimlines (vt, vt->rows);
  vt->margin_top     = 1;
  vt->margin_left    = 1;
  vt->margin_bottom  = vt->rows;
  vt->margin_right   = vt->cols;
  vt->rev++;
  VT_info ("resize %i %i", irows, icols);
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
  if (vt->argument_buf_len + 1 >= 1024 * 1024 * 2)
    { return; } // XXX : perhaps we should bail at 1mb + 1kb ?
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
  vt->rev++;
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
            { return; }
        }
      else
        {
          vt->cursor_x = logical_margin_right;
        }
    }
  vt_line_set_style (vt->current_line, vt->cursor_x-1, vt->cstyle);
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
  vt->cursor_x += 1;
  vt->at_line_home = 0;
  vt->rev++;
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
  vt->rev++;
}

static void vtcmd_set_top_and_bottom_margins (VT *vt, const char *sequence)
{
  int top = 1, bottom = vt->rows;
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
          vt->current_line->string.utf8_length = mrg_utf8_strlen (vt->current_line->string.str);
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
          vt->current_line->string.utf8_length = mrg_utf8_strlen (vt->current_line->string.str); // should be a nop
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
          vt->current_line->string.utf8_length = mrg_utf8_strlen (vt->current_line->string.str);
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
      case 3: // XXX also clear scrollback
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
          /* SGR@38;5;n@\b256 color index foreground color@where n is 0-15 is system colors 16-(16+6*6*6) is a 6x6x6\n                RGB cube and in the end a grayscale without white and black.@ */
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
              /* SGR@38;2;50;70;180m@\b24 bit RGB foreground color@The example sets RGB the triplet 50 70 180@f@ */
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
          /* SGR@48;5;n@\b256 color index background color@where n is 0-15 is system colors 16-(16+6*6*6) is a 6x6x6\n                RGB cube and in the end a grayscale without white and black.@ */
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
          /* SGR@48;2;50;70;180m@\b24 bit RGB background color@The example sets RGB the triplet 50 70 180@ */
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
            case 3: /* SGR@@Rotalic@@ */
              vt->cstyle |= STYLE_ITALIC;
              break;
            case 4: /* SGR@@Underscore@@ */
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
  // XXX need style
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
  //ctx_parser_free (vt->ctxp);
  //vt->ctxp = NULL;
}

static int ct_set_prop (VT *vt, uint32_t key_hash, const char *val)
{
#if 1
  fprintf (stderr, "%i: %s\n", key_hash, val);
#else
  float fval = strtod (val, NULL);
  CtxClient *client = client_by_id (ct->id);
  uint32_t val_hash = ctx_strhash (val, 0);
  if (!client)
    return 0;

  if (key_hash == ctx_strhash("start_move", 0))
  {
    start_moving (client);
    moving_client = 1;
    return 0;
  }

  //fprintf (stderr, "%i %s\n", key_hash, val);

// set "pcm-hz"       "8000"
// set "pcm-bits"     "8"
// set "pcm-encoding" "ulaw"
// set "play-pcm"     "d41ata312313"
// set "play-pcm-ref" "foo.wav"

// get "free"
// storage of blobs for referencing when drawing or for playback
// set "foo.wav"      "\3\1\1\4\"
// set "fnord.png"    "PNG12.4a312"

  switch (key_hash)
  {
    case CTX_title:  client_set_title (ct->id, val); break;
    case CTX_x:      client->x = fval; break;
    case CTX_y:      client->y = fval; break;
    case CTX_width:  client_resize (ct->id, fval, client->height); break;
    case CTX_height: client_resize (ct->id, client->width, fval); break;
    case CTX_action:
      switch (val_hash)
      {
        case CTX_maximize:     client_maximize (client); break;
        case CTX_unmaximize:   client_unmaximize (client); break;
        case CTX_lower:        client_lower (client); break;
        case CTX_lower_bottom: client_lower_bottom (client);  break;
        case CTX_raise:        client_raise (client); break;
        case CTX_raise_top:    client_raise_top (client); break;
      }
      break;
  }
  ct->rev++;
  fprintf (stderr, "%s: %i %s %i\n", __FUNCTION__, key_hash, val, len);
#endif
  return 0;
}

static int ct_get_prop (VT *vt, const char *key, const char **val, int *len)
{
#if 0
  uint32_t key_hash = ctx_strhash (key, 0);
  char str[4096]="";
  fprintf (stderr, "%s: %s %i\n", __FUNCTION__, key, key_hash);
  CtxClient *client = client_by_id (ct->id);
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
          case 1: /*MODE;Cursor key mode;Application;Cursor;*/
            vt->cursor_key_application = set;
            break;
          case 2017: /*MODE;Cursor key mode;Application;Cursor;*/
            vt->key_2017 = set;
            break;
          case 2: /*MODE;VT52 emulation;;enable; */
            if (set==0)
              { vt->state = vt_state_vt52; }
            break;
          case 3: /*MODE;Column mode;132 columns;80 columns;*/
            vtcmd_set_132_col (vt, set);
            break; // set 132 col
          case 4: /*MODE;Smooth scroll;Smooth;Jump;*/
            vt->smooth_scroll = set;
            break; // set 132 col
          case 5: /*MODE;DECSCNM Screen mode;Reverse;Normal;*/
            vt->reverse_video = set;
            break;
          case 6: /*MODE;DECOM Origin mode;Relative;Absolute;*/
            vt->origin = set;
            if (set)
              {
                _vt_move_to (vt, vt->margin_top, 1);
                vt_carriage_return (vt);
              }
            else
              { _vt_move_to (vt, 1, 1); }
            break;
          case 7: /*MODE;Wraparound;On;Off;*/
            vt->autowrap = set;
            break;
          case 8: /*MODE;Auto repeat;On;Off;*/
            vt->keyrepeat = set;
            break;
          // 10 - Block DECEDM
          // 18 - Print form feed  DECPFF  default off
          // 19 - Print fullscreen DECPEX  default on
          case 12:
            vtcmd_ignore (vt, sequence);
            break; // blinking_cursor
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
          case 69:/*MODE;DECVSSM Left right margin mode;On;Off; */
            vt->left_right_margin_mode = set;
            break;
          case 80:/* DECSDM Sixel scrolling */
            break;
          case 437:/*MODE;Encoding/cp437mode;cp437;utf8; */
            vt->encoding = set ? 1 : 0;
            break;
          case 1000:/*MODE;Mouse reporting;On;Off;*/
            vt->mouse = set;
            break;
          case 1002:/*MODE;Mouse drag;On;Off;*/
            vt->mouse_drag = set;
            break;
          case 1003:/*MODE;Mouse all;On;Off;*/
            vt->mouse_all = set;
            break;
          case 1006:/*MODE;Mouse decimal;On;Off;*/
            vt->mouse_decimal = set;
            break;
          //case 47:
          //case 1047:
          //case 1048:
          case 1049:
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
            //   vtcmd_reset_to_initial_state (vt, sequence);
            break; // alt screen
          case 1010: // scroll to bottom on tty output (rxvt)
            break;
          case 1011: // scroll to bottom on key press (rxvt)
            break;
          case 2004:  /* MODE;bracketd paste:On;Off; */
            vt->bracket_paste = set;
            break;
          //case 2020: /*MODE;wordwrap;On;Off;*/
          //      vt->wordwrap = set; break;
          case 6150:/*MODE;Ctx-events;On;Off;*/
            vt->ctx_events = set;
            break;
          
          case 7020:/*MODE;Ctx ascii;On;;*/
            if (set)
              {
                if (!vt->current_line->ctx)
                  {
                    vt->current_line->ctx = ctx_new ();
                    _ctx_set_transformation (vt->current_line->ctx, 0);
                  }
                if (vt->ctxp)
                  ctx_parser_free (vt->ctxp);
                vt->ctxp = ctx_parser_new (vt->current_line->ctx,
                                           vt->cols * vt->cw, vt->rows * vt->ch,
                                           vt->cw, vt->ch, vt->cursor_x, vt->cursor_y,
                                           (void*)ct_set_prop, (void*)ct_get_prop, vt, vt_ctx_exit, vt);
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
          case 4:/*MODE2;IRM Insert Mode;Insert;Replace; */
            vt->insert_mode = set;
            break;
          case 9: /* interlace mode */
            break;
          case 12:/*MODE2;Local echo;On;Off; */
            vt->echo = set;
            break;
          case 20:/*MODE2;LNM Carriage Return on LF/Newline;On;Off;*/
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
      int is_set = -1;
      switch (qval)
        {
          case 1:
            is_set = vt->cursor_key_application;
            break;
          case 2017:
            is_set = vt->key_2017;
            break;
          case 2: /*MODE;VT52 emulation;;enable; */
          //if (set==0) vt->in_vt52 = 1;
          case 3:
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
          case 25:
            is_set = vt->cursor_visible;
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
          case 1002:
            is_set = vt->mouse_drag;
            break;
          case 1003:
            is_set = vt->mouse_all;
            break;
          case 1006:
            is_set = vt->mouse_decimal;
            break;
          case 6150:
            is_set = vt->ctx_events;
            break;
          case 2004:
            is_set = vt->bracket_paste;
            break;
          case 1010: // scroll to bottom on tty output (rxvt)
          case 1011: // scroll to bottom on key press (rxvt)
          case 1049:
            is_set = vt->in_alt_screen;
            break;
            break;
          case 7020:/*MODE;Ctx ascii;On;;*/
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
      if (is_set >= 0)
        { sprintf (buf, "\033[?%i;%i$y", qval, is_set?1:2); }
      else
        { sprintf (buf, "\033[?%i;%i$y", qval, 0); }
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
          case 4:/*MODE2;Insert Mode;Insert;Replace; */
            sprintf (buf, "\033[%i;%i$y", val, vt->insert_mode?1:2);
            break;
          case 9: /* interlace mode */
            sprintf (buf, "\033[%i;%i$y", val, 0);
            break;
          case 12:/*MODE2;Local echo;On;Off; */
            sprintf (buf, "\033[%i;%i$y", val, vt->echo?1:2);
            break;
          case 20:/*MODE2;Carriage Return on LF/Newline;On;Off;*/
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
  if (!strcmp (sequence, "[14t") ) /* request terminal dimensions */
    {
      char buf[128];
      sprintf (buf, "\033[4;%i;%it", vt->rows * vt->ch, vt->cols * vt->cw);
      vt_write (vt, buf, strlen (buf) );
    }
#if 0
  {"[", 's',  vtcmd_save_cursor_position, VT100}, /*args:PnSP id:DECSWBV Set warning bell volume */
#endif
  else if (sequence[strlen (sequence)-2]==' ') /* DECSWBV */
    {
      int val = parse_int (sequence, 0);
      if (val <= 1) { vt->bell = 0; }
      if (val <= 8) { vt->bell = val; }
    }
  else
    {
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
  while ( ! vt->tabs[ (int) vt->cursor_x-1] && vt->cursor_x > 1);
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
  vt->current_line->double_width = 1;
  vt->current_line->double_height_top = 1;
  vt->current_line->double_height_bottom = 0;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}
static void vtcmd_set_double_width_double_height_bottom_line
(VT *vt, const char *sequence)
{
  vt->current_line->double_width = 1;
  vt->current_line->double_height_top = 0;
  vt->current_line->double_height_bottom = 1;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}
static void vtcmd_set_single_width_single_height_line
(VT *vt, const char *sequence)
{
  vt->current_line->double_width = 0;
  vt->current_line->double_height_top = 0;
  vt->current_line->double_height_bottom = 0;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}
static void
vtcmd_set_double_width_single_height_line
(VT *vt, const char *sequence)
{
  vt->current_line->double_width = 1;
  vt->current_line->double_height_top = 0;
  vt->current_line->double_height_bottom = 0;
  //vt_cell_cache_clear_row (vt, vt->cursor_y);
}

static void vtcmd_set_led (VT *vt, const char *sequence)
{
  int val = 0;
  fprintf (stderr, "%s\n", sequence);
  for (const char *s = sequence; *s; s++)
    {
      switch (*s)
        {
          case '0':
            val = 0;
            break;
          case '1':
            val = 1;
            break;
          case '2':
            val = 2;
            break;
          case '3':
            val = 3;
            break;
          case '4':
            val = 4;
            break;
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
  fprintf (stderr, "gfx intro [%s]\n",sequence);
}

static void vtcmd_report (VT *vt, const char *sequence)
{
  char buf[64]="";
  if (!strcmp (sequence, "[?15n") ) // printer status
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
  else if (!strcmp (sequence, "[6n") ) // DSR cursor position report
    {
      sprintf (buf, "\033[%i;%iR", vt->cursor_y - (vt->origin? (vt->margin_top - 1) :0), (int) vt->cursor_x - (vt->origin? (VT_MARGIN_LEFT-1) :0) );
    }
  else if (!strcmp (sequence, "[?6n") ) // DECXPR extended cursor position report
    {
      sprintf (buf, "\033[?%i;%i;1R", vt->cursor_y - (vt->origin? (vt->margin_top - 1) :0), (int) vt->cursor_x - (vt->origin? (VT_MARGIN_LEFT-1) :0) );
    }
  else if (!strcmp (sequence, "[5n") ) // DSR decide status report
    {
      sprintf (buf, "\033[0n"); // we're always OK :)
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
    { vt_write (vt, buf, strlen (buf) ); }
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
    {"E",  0,   vtcmd_next_line},
    {"_", 'G',  vtcmd_graphics},
    {"H",   0,  vtcmd_horizontal_tab_set, VT100}, /* id:HTS Horizontal Tab Set */

    //{"I",  0,  vtcmd_char_tabulation_with_justification},
    //{"K",  0,  PLD partial line down
    //{"L",  0,  PLU partial line up
    {"M",  0,   vtcmd_reverse_index, VT100}, /* id:RI Reverse Index */
    //{"N",  0,  vtcmd_ignore}, /* Set Single Shift 2 - SS2*/
    //{"O",  0,  vtcmd_ignore}, /* Set Single Shift 3 - SS3*/


    /* these need to occur before vtcmd_preceding_line to have precedence */
    {"[0 F", 0, vtcmd_justify, ANSI}, /* id:JFY disable justification/wordwrap  */
    {"[1 F", 0, vtcmd_justify, ANSI}, /* id:JFY enable wordwrap  */
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
    {"[",  'j', vtcmd_cursor_backward, ANSI}, /* args:Pn id:HPB Horizontal Position Backward */
    {"[",  'k', vtcmd_cursor_up, ANSI}, /* args:Pn id:VPB Vertical Position Backward */
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
    {"[",  'S', vtcmd_scroll_up, VT100}, /* args:Pn id:SU Scroll Up */
    {"[",  'T', vtcmd_scroll_down, VT100}, /* args:Pn id:SD Scroll Down */
    {"[",/*SP*/'U', vtcmd_set_line_home, ANSI}, /* args:PnSP id=SLH Set Line Home */
    {"[",/*SP*/'V', vtcmd_set_line_limit, ANSI},/* args:PnSP id=SLL Set Line Limit */
    // [ W is cursor tabulation control
    // [ Pn Y  - cursor line tabulation
    //
    {"[",  'X', vtcmd_erase_n_chars}, /* args:Pn id:ECH Erase Character */
    {"[",  'Z', vtcmd_rev_n_tabs}, /* args:Pn id:CBT Cursor Backward Tabulation */
    {"[",  '^', vtcmd_scroll_down}, /* muphry alternate from ECMA */
    {"[",  '@', vtcmd_insert_character, VT102}, /* args:Pn id:ICH Insert Character */

    {"[",  'a', vtcmd_cursor_forward, ANSI}, /* args:Pn id:HPR Horizontal Position Relative */
    {"[",  'b', vtcmd_cursor_forward, ANSI}, /* REP previous char XXX incomplete */
    {"[",  'c', vtcmd_report}, /* id:DA args:... Device Attributes */
    {"[",  'd', vtcmd_goto_row},       /* args:Pn id:VPA Vertical Position Absolute  */
    {"[",  'e', vtcmd_cursor_down},    /* args:Pn id:VPR Vertical Position Relative */
    {"[",  'f', vtcmd_cursor_position, VT100}, /* args:Pl;Pc id:HVP Cursor Position */
    {"[g", 0,   vtcmd_clear_current_tab, VT100}, /* id:TBC clear current tab */
    {"[0g", 0,  vtcmd_clear_current_tab, VT100}, /* id:TBC clear current tab */
    {"[3g", 0,  vtcmd_clear_all_tabs, VT100},    /* id:TBC clear all tabs */
    {"[",  'm', vtcmd_set_graphics_rendition}, /* args:Ps;Ps;.. id:SGR Select Graphics Rendition */
    {"[",  'n', vtcmd_report},           /* id:DSR args:... CPR Cursor Position Report  */
    {"[",  'r', vtcmd_set_top_and_bottom_margins, VT100}, /* args:Pt;Pb id:DECSTBM Set Top and Bottom Margins */
#if 0
    // handled by set_left_and_right_margins - in if 0 to be documented
    {"[s",  0,  vtcmd_save_cursor_position, VT100}, /*id:SCP Save Cursor Position */
#endif
    {"[u",  0,  vtcmd_restore_cursor_position, VT100}, /*id:RCP Restore Cursor Position */
    {"[",  's', vtcmd_set_left_and_right_margins, VT400}, /* args:Pl;Pr id:DECSLRM Set Left and Right Margins */
    {"[",  '`', vtcmd_horizontal_position_absolute, ANSI},  /* args:Pn id:HPA Horizontal Position Absolute */

    {"[",  'h', vtcmd_set_mode, VT100},   /* args:Pn[;...] id:SM Set Mode */
    {"[",  'l', vtcmd_set_mode, VT100}, /* args:Pn[;...]  id:RM Reset Mode */
    {"[",  't', vtcmd_set_t},
    {"[",  'q', vtcmd_set_led, VT100}, /* args:Ps id:DECLL Load LEDs */
    {"[",  'x', vtcmd_report}, /* id:DECREQTPARM */
    {"[",  'z', vtcmd_DECELR}, /* id:DECELR set locator res  */

    {"5",   0,  vtcmd_char_at_cursor, VT300}, /* id:DECXMIT */
    {"6",   0,  vtcmd_back_index, VT400}, /* id:DECBI Back index */
    {"7",   0,  vtcmd_save_cursor, VT100}, /* id:DECSC Save Cursor */
    {"8",   0,  vtcmd_restore_cursor, VT100}, /* id:DECRC Restore Cursor */
    {"9",   0,  vtcmd_forward_index, VT400}, /* id:DECFI Forward index */

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

    {"#3",  0,   vtcmd_set_double_width_double_height_top_line, VT100}, /*id: DECDHL Top half of double-width, double-height line */
    {"#4",  0,   vtcmd_set_double_width_double_height_bottom_line, VT100}, /*id:DECDHL Bottom half of double-width, double-height line */
    {"#5",  0,   vtcmd_set_single_width_single_height_line, VT100}, /* id:DECSWL Single-width line */
    {"#6",  0,   vtcmd_set_double_width_single_height_line, VT100}, /* id:DECDWL Double-width line */

    {"#8",  0,   vtcmd_screen_alignment_display, VT100}, /* id:DECALN Screen Alignment Display */
    {"=",   0,   vtcmd_ignore},  // keypad mode change
    {">",   0,   vtcmd_ignore},  // keypad mode change
    {"c",   0,   vtcmd_reset_to_initial_state, VT100}, /* id:RIS Reset to Initial State */
    {"[!", 'p',  vtcmd_ignore},       // soft reset?
    {"[",  'p',  vtcmd_request_mode}, // soft reset?

    {NULL, 0, NULL}
  };

  static void handle_sequence (VT *vt, const char *sequence)
{
  int i0 = strlen (sequence)-1;
  int i;
  vt->rev ++;
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

#include "vt-encodings.h"

#if 1
#include "vt-audio.h"

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

static void audio_task (AudioState *audio, int click)
{
}

static void vt_bell (VT *vt)
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
        // XXX : if we are at left margin, keep it!
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
    }
  return 0;
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
  for (i = 0; vt->current_line->images[i] && i < 4; i++);
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
        bin_length = base642bin ( (char *) vt->gfx.data,
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
              char buf[256]= "\033_Go=z;zlib error\033\\";
              vt_write (vt, buf, strlen (buf) );
              goto cleanup;
            }
          free (vt->gfx.data);
          vt->gfx.data = data2;
          vt->gfx.data_size = actual_uncompressed_size;
          vt->gfx.compression = 0;
        }
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
                            case 'a': /* all images visible on screen */
                              match = 1;
                              break;
                            case 'I':
                              free_resource = 1;
                            case 'i': /* all images with specified id */
                              if ( ( (Image *) (line->images[i]) )->id == vt->gfx.id)
                                { match = 1; }
                              break;
                            case 'P':
                              free_resource = 1;
                            case 'p': /* all images intersecting cell
          specified with x and y */
                              if (line->image_col[i] == vt->gfx.x &&
                                  row == vt->gfx.y)
                                { match = 1; }
                              break;
                            case 'Q':
                              free_resource = 1;
                            case 'q': /* all images with specified cell (x), row(y) and z */
                              if (line->image_col[i] == vt->gfx.x &&
                                  row == vt->gfx.y)
                                { match = 1; }
                              break;
                            case 'Y':
                              free_resource = 1;
                            case 'y': /* all images with specified row (y) */
                              if (row == vt->gfx.y)
                                { match = 1; }
                              break;
                            case 'X':
                              free_resource = 1;
                            case 'x': /* all images with specified column (x) */
                              if (line->image_col[i] == vt->gfx.x)
                                { match = 1; }
                              break;
                            case 'Z':
                              free_resource = 1;
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
    }
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
    { return; }
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
  vt->rev++;
}

static void vt_state_ctx (VT *vt, int byte)
{
  if (vt->ctxp)
    ctx_parser_feed_byte (vt->ctxp, byte);
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
            vt_set_title (vt, vt->argument_buf + 3);
            break;
          case 10:
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
            break;
          case 11:
            {
              /* get background color */
              char buf[128];
              sprintf (buf, "\033]11;rgb:%2x/%2x/%2x\033\\",
                       vt->bg_color[0], vt->bg_color[1], vt->bg_color[2]);
              vt_write (vt, buf, strlen (buf) );
            }
            break;
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
                    bin_length = base642bin ( (char *) p,
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
          case 104:
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


static void vt_state_apc_generic (VT *vt, int byte)
{
  if ( (byte < 32) && ( (byte < 8) || (byte > 13) ) )
    {
      if (vt->argument_buf[1] == 'G') /* graphics - from kitty */
        {
          vt_gfx (vt, vt->argument_buf);
        }
      vt->state = ( (byte == 27) ?  vt_state_swallow : vt_state_neutral);
    }
  else
    {
      vt_argument_buf_add (vt, byte);
    }
}

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
  if (_vt_handle_control (vt, byte) != 0)
    { return; }
  switch (byte)
    {
      case 27: /* ESCape */
        vt->state = vt_state_esc;
        break;
      default:
        if (vt_decoder_feed (vt, byte) )
          { return; }
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
        break;
    }
}



int vt_poll (VT *vt, int timeout)
{
  int read_size = sizeof (vt->buf);
  int got_data = 0;
  int remaining_chars = 1024 * 1024;
  int len = 0;
  audio_task (vt, 0);
#if 1
  if (vt->cursor_visible && vt->smooth_scroll)
    {
      remaining_chars = vt->cols / 2;
    }
  if (vt->in_smooth_scroll)
    {
      remaining_chars = 0;
      // XXX : need a bail condition -
      // /// so that we can stop accepting data until autowrap or similar
    }
#endif
  read_size = MIN (read_size, remaining_chars);
  while (timeout > 100 &&
         remaining_chars > 0 &&
         vt_waitdata (vt, timeout) )
    {
      len = vt_read (vt, vt->buf, read_size);
      for (int i = 0; i < len; i++)
        { vt->state (vt, vt->buf[i]); }
      got_data+=len;
      remaining_chars -= len;
      timeout -= 10;
      audio_task (vt, 0);
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


};

void vt_feed_keystring (VT *vt, const char *str)
{
  if (vt->ctx_events)
  {
    if (!strcmp (str, "shift-control-l") )
    {
      vt->ctx_events = 0;
    }
    vt_write (vt, str, strlen (str) );
    vt_write (vt, "\n", 1);
    return;
  }
#if 0
  if (!strstr (str, "-page"))
    vt_set_scroll (vt, 0);
#endif

  if (!strcmp (str, "idle") )
     return;
  else if (!strcmp (str, "shift-page-up") )
    {
      int new_scroll = vt_get_scroll (vt) + vt_get_rows (vt) /2;
      vt_set_scroll (vt, new_scroll);
      vt_rev_inc (vt);
      return;
    }
  else if (!strcmp (str, "shift-page-down") )
    {
      int new_scroll = vt_get_scroll (vt) - vt_get_rows (vt) /2;
      if (new_scroll < 0) { new_scroll = 0; }
      vt_set_scroll (vt, new_scroll);
      vt_rev_inc (vt);
      return;
    }
  else if (!strcmp (str, "shift-control--") ||
           !strcmp (str, "control--") )
    {
      float font_size = vt_get_font_size (vt);
      int vt_width = vt->cols * vt->cw;
      int vt_height = vt->rows * vt->ch;
      font_size /= 1.15;
      font_size = (int) (font_size);
      if (font_size < 5) { font_size = 5; }
      vt_set_font_size (vt, font_size);
      vt_set_term_size (vt, vt_width / vt_cw (vt), vt_height / vt_ch (vt) );
      return;
    }
  else if (!strcmp (str, "shift-control-=") ||
           !strcmp (str, "control-=") )
    {
      float font_size = vt_get_font_size (vt);
      int vt_width = vt->cols * vt->cw;
      int vt_height = vt->rows * vt->ch;
      float old = font_size;
      font_size *= 1.15;
      font_size = (int) (font_size);
      if (old == font_size) { font_size = old+1; }
      if (font_size > 200) { font_size = 200; }
      vt_set_font_size (vt, font_size);
      vt_set_term_size (vt, vt_width / vt_cw (vt), vt_height / vt_ch (vt) );
      return;
    }
  else if (!strcmp (str, "shift-control-r") )
    {
      vt_open_log (vt, "/tmp/ctx-vt");
    }
  else if (!strcmp (str, "shift-control-l") )
    {
      vt_set_local (vt, !vt_get_local (vt) );
      return;
    }
  else if (!strncmp (str, "mouse-", 5) )
    {
      int cw = vt_cw (vt);
      int ch = vt_ch (vt);
      if (!strncmp (str + 6, "motion", 6) )
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
                  vt_mouse (vt, VT_MOUSE_MOTION, x/cw + 1, y/ch + 1, x, y);
                }
            }
        }
      else if (!strncmp (str + 6, "press", 5) )
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
                  vt_mouse (vt, VT_MOUSE_PRESS, x/cw + 1, y/ch + 1, x, y);
                }
            }
          //clients[active].drawn_rev = 0;
        }
      else if (!strncmp (str + 6, "drag", 4) )
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
                  vt_mouse (vt, VT_MOUSE_DRAG, x/cw + 1, y/ch + 1, x, y);
                }
            }
          //clients[active].drawn_rev = 0;
        }
      else if (!strncmp (str + 6, "release", 7) )
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
                  vt_mouse (vt, VT_MOUSE_RELEASE, x/cw + 1, y/ch + 1, x, y);
                }
            }
          //clients[active].drawn_rev = 0;
          // queue-draw
        }
      return;
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
  if (!strcmp (str, "control-space") )
    {
      str = "\0\0";
      vt_write (vt, str, 1);
      return;
    }
  for (unsigned int i = 0; i<sizeof (keymap_general) /sizeof (keymap_general[0]); i++)
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
  vt_feed_keystring (vt, str);
  if (vt->bracket_paste)
    {
      vt_write (vt, "\033[201~", 6);
    }
}

const char *vt_find_shell_command (void)
{
  int i;
  const char *command = NULL;
  struct stat stat_buf;
  static char *alts[][2] =
  {
    {"/bin/bash",     "/bin/bash -i"},
    {"/usr/bin/bash", "/usr/bin/bash -i"},
    {"/bin/sh",       "/bin/sh -i"},
    {"/usr/bin/sh",   "/usr/bin/sh -i"},
    {NULL, NULL}
  };
  for (i = 0; alts[i][0] && !command; i++)
    {
      lstat (alts[i][0], &stat_buf);
      if (S_ISREG (stat_buf.st_mode) || S_ISLNK (stat_buf.st_mode) )
        { command = alts[i][1]; }
    }
  return command;
}


static void vt_run_command (VT *vt, const char *command, const char *term)
{
  struct winsize ws;
  //signal (SIGCHLD,signal_child);
  ws.ws_row = vt->rows;
  ws.ws_col = vt->cols;
  ws.ws_xpixel = ws.ws_col * vt->cw;
  ws.ws_ypixel = ws.ws_row * vt->ch;
  vt->vtpty.pid = forkpty (&vt->vtpty.pty, NULL, NULL, &ws);
  if (vt->vtpty.pid == 0)
    {
      int i;
      for (i = 3; i<768; i++) { close (i); } /*hack, trying to close xcb */
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
      //setenv ("TERM", "xterm-256color", 1);
      setenv ("TERM", term?term:"xterm", 1);
      setenv ("COLORTERM", "truecolor", 1);
      setenv ("CTX_VERSION", "0", 1);
      //execlp ("/bin/bash", "/bin/bash", NULL);
      system (command);
      //vt->done = 1;
      exit (0);
    }
  else if (vt->vtpty.pid < 0)
    {
      VT_error ("forkpty failed (%s)", command);
    }
  fprintf (stderr, "fpty %i\n", vt->vtpty.pid);
  //fcntl(vt->vtpty.pty, F_SETFL, O_NONBLOCK);
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
  if (vt->ctx)
    { ctx_free (vt->ctx); }
  free (vt->argument_buf);
  ctx_list_remove (&vts, vt);
  kill (vt->vtpty.pid, 9);
  close (vt->vtpty.pty);
#if 1
  if (vt->title)
    free (vt->title);
#endif
  free (vt);
}

int vt_get_line_count (VT *vt)
{
  return vt->line_count;
}

const char *vt_get_line (VT *vt, int no)
{
  CtxList *l= ctx_list_nth (vt->lines, no);
  VtString *str;
  if (!l)
    { return NULL; }
  str = l->data;
  return str->str;
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
  ctx_begin_path (ctx);
  ctx_rectangle (ctx, 0.167 * cw + x + u * cw * 0.5,
                 y - ch + 0.080 * ch + v * ch * 0.25,
                 0.33 *cw, 0.33 * cw);
  ctx_fill (ctx);
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
        ctx_rectangle (ctx, x + cw/2 - ch * 0.1 / 2, y - ch, ch * 0.1, ch);
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
        return 0;
    }
  return -1;
}

void vt_ctx_glyph (Ctx *ctx, VT *vt, float x, float y, int unichar, int bold, float scale_x, float scale_y, float offset_y)
{
  int did_save = 0;
  if (unichar <= ' ')
    { return; }
  scale_x *= vt->scale_x;
  scale_y *= vt->scale_y;
  if (!vt_special_glyph (ctx, vt, x, y + offset_y * vt->ch, vt->cw * scale_x, vt->ch * scale_y, unichar) )
    { return; }

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
  if (bold)
    {
      ctx_move_to (ctx, x, y);
      ctx_line_width (ctx, vt->font_size/35.0);
      ctx_glyph (ctx, unichar, 1);
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

static uint8_t palettes[][16][3]=
{
  {
    {0, 0, 0},
    {127, 5, 33},
    {72, 160, 75},
    {163, 96, 0},
    {10, 10, 170},
    {145, 49, 190},
    {50, 130, 174},
    {167, 167, 167},
    {88, 88, 88},
    {255, 155, 130},
    {90, 245, 160},
    {255, 230, 0},
    {0, 0, 255},
    {222, 144, 242},
    {142, 208, 253},
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

void vt_ctx_set_color (VT *vt, Ctx *ctx, int no, int intensity)
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
  ctx_rgba8 (ctx, r, g, b, 255);
}

int vt_keyrepeat (VT *vt)
{
  return vt->keyrepeat;
}

float vt_draw_cell (VT *vt, Ctx *ctx,
                    int   row, int col, // pass 0 to force draw - like
                    float x0, float y0, // for scrollback visible
                    uint64_t style,
                    uint32_t unichar,
                    int      bg, int fg,
                    int      dw, int dh,
                    int in_smooth_scroll,
                    int in_select)
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
      if ( (reverse) == 0)
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
      else
        {
          if (bold)
            {
              bg_intensity = blink?2:  0;
              fg_intensity = blink?0:  3;
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
  if (bg)
    {
      //ctx_begin_path (ctx);
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
            { r= g = b = 30; }
          if (r == 0 && g == r && b == g)
            goto bg_done;
          ctx_rgba8 (ctx, r, g, b, 255);
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
              if (color == 0)
                goto bg_done;
              vt_ctx_set_color (vt, ctx, color, bg_intensity);
            }
          else
            {
              uint8_t rgb[3]= {0,};
              switch (bg_intensity)
                {
                  case 0:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->bg_color[i]; }
                    break;
                  case 1:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->bg_color[i] * 0.5 + vt->fg_color[i] * 0.5; }
                    break;
                  case 2:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->bg_color[i] * 0.05 + vt->fg_color[i] * 0.95; }
                    break;
                  case 3:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->fg_color[i]; }
                    break;
                }
              if (rgb[0] == 0 && rgb[0] == rgb[1] && rgb[2] == rgb[1])
                goto bg_done;
              ctx_rgba8 (ctx, rgb[0], rgb[1], rgb[2], 255);
            }
        }
      if (dh)
        {
          ctx_rectangle (ctx, ctx_floorf(x0), ctx_floorf(y0 - ch - ch * (vt->scroll_offset)), cw, ch);
        }
      else
        {
          ctx_rectangle (ctx, x0, y0 - ch + ch * offset_y, cw, ch);
        }
      ctx_fill (ctx);
bg_done:
      {
      };
    }
  if (!fg) { return cw; }
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

  if (unichar == ' ' && !(underline || double_underline || curved_underline))
          hidden = 1;

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
          ctx_rgba8 (ctx, r, g, b, 255);
        }
      else
        {
          if ( (style & STYLE_FG_COLOR_SET) == 0)
            {
              uint8_t rgb[3]= {0,};
              switch (fg_intensity)
                {
                  case 0:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->bg_color[i] * 0.7 + vt->fg_color[i] * 0.3; }
                    break;
                  case 1:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->bg_color[i] * 0.5 + vt->fg_color[i] * 0.5; }
                    break;
                  case 2:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->bg_color[i] * 0.20 + vt->fg_color[i] * 0.80; }
                    break;
                  case 3:
                    for (int i = 0; i <3 ; i++)
                      { rgb[i] = vt->fg_color[i]; }
                }
              ctx_rgba8 (ctx, rgb[0],
                             rgb[1],
                             rgb[2], 255);
            }
          else
            {
              if (reverse)
                { color = (style >> 40) & 255; }
              else
                { color = (style >> 16) & 255; }
              bg_intensity = -1;
              vt_ctx_set_color (vt, ctx, color, fg_intensity);
            }
        }
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
  return vt->has_blink + (vt->in_smooth_scroll ?  10 : 0);
}

void vt_mouse_event (CtxEvent *event, void *data, void *data2)
{
  VT *vt = data;
  float x = event->x;
  float y = event->y;
  char buf[128]="";

  switch (event->type)
  {
    case CTX_MOTION:
      sprintf (buf, "mouse-motion %.0f %.0f", x, y);
      vt_feed_keystring (vt, buf);
      break;
    case CTX_PRESS:
      sprintf (buf, "mouse-press %.0f %.0f", x, y);
      vt_feed_keystring (vt, buf);
      break;
    case CTX_RELEASE:
      sprintf (buf, "mouse-release %.0f %.0f", x, y);
      vt_feed_keystring (vt, buf);
      break;
    default:
      break;
  }
}

static int scrollbar_focused = 0;
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

static void scrollbar_pressed (CtxEvent *event, void *data, void *data2)
{
  VT *vt = data;
  float disp_lines = vt->rows;
  float tot_lines = vt->line_count + vt->scrollback_count;
  vt->scroll = tot_lines - disp_lines - (event->y*1.0/ (vt->rows * vt->ch) ) * tot_lines + disp_lines/2;
  if (vt->scroll < 0) { vt->scroll = 0.0; }
  if (vt->scroll > vt->scrollback_count) { vt->scroll = vt->scrollback_count; }
  fprintf (stderr, "press %f %f scroll:%f\n", event->x, event->y, vt->scroll);
  vt->rev++;
  event->stop_propagate = 1;
}

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

void vt_draw (VT *vt, Ctx *ctx, double x0, double y0)
{
  int image_id = 0;
  ctx_save (ctx);
  ctx_font (ctx, "mono");
  vt->font_is_mono = 0;
  ctx_font_size (ctx, vt->font_size * vt->font_to_cell_scale);
  vt->has_blink = 0;
  vt->blink_state++;
  int cursor_x_px = 0;
  int cursor_y_px = 0;
  int cursor_w = vt->cw;
  int cursor_h = vt->ch;
  cursor_x_px = x0 + (vt->cursor_x - 1) * vt->cw;
  cursor_y_px = y0 + (vt->cursor_y - 1) * vt->ch;
  cursor_w = vt->cw;
  cursor_h = vt->ch;
  //if (vt->scroll || full)
    {
      ctx_begin_path (ctx);
      ctx_rectangle (ctx, 0, 0, (vt->cols + 1) * vt->cw,
                     (vt->rows + 1) * vt->ch);
      if (vt->reverse_video)
        {
          ctx_rgba (ctx, 1,1,1,1);
          ctx_fill  (ctx);
          ctx_rgba (ctx, 0,0,0,1);
        }
      else
        {
          ctx_rgba (ctx, 0,0,0,1);
          ctx_fill  (ctx);
          ctx_rgba (ctx, 1,1,1,1);
        }
      if (vt->scroll != 0.0f)
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
             if (line->string.str[0] == 0 && r < vt->rows && 0)
                     // this being 0 is not enough, the string as used
                     // here is not neccesarily null terminated.
             {
               ctx_rectangle (ctx, x0, y - vt->ch, vt->cw * vt->cols, vt->ch);
       if (vt->reverse_video)
         {
           ctx_rgba (ctx, 1,1,1,1);
           ctx_fill  (ctx);
           ctx_rgba (ctx, 0,0,0,1);
         }
       else
         {
           ctx_rgba (ctx, 0,0,0,1);
           ctx_fill  (ctx);
           ctx_rgba (ctx, 1,1,1,1);
         }
               continue;
             }
             const char *data = line->string.str;
             const char *d = data;
             float x = x0;
             uint64_t style = 0;
             uint32_t unichar = 0;
             int in_scrolling_region = vt->in_smooth_scroll && ( (r >= vt->margin_top && r <= vt->margin_bottom) || r <= 0);
             int got_selection = 0;
             for (int col = 1; col <= vt->cols * 1.33 && x < vt->cols * vt->cw; col++)
               {
                 int c = col;
                 int real_cw;
                 int in_selected_region = 0;
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
                 got_selection |= in_selected_region;
                 //if (vt->scroll || full)
                   {
                     /* this prevents draw_cell from using cache */
            //       r = c = 0;
                   }
                 style = vt_line_get_style (line, col-1);
                 unichar = d?ctx_utf8_to_unichar (d) :' ';
                 real_cw=vt_draw_cell (vt, ctx, r, c, x, y, style, unichar, 1, 1,
                                       line->double_width,
                                       line->double_height_top?1:
                                       line->double_height_bottom?-1:0,
                                       in_scrolling_region,
                                       in_selected_region);
                 if (r == vt->cursor_y && col == vt->cursor_x)
                   {
                     cursor_x_px = x;
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
            for (int i = 0; i < 4; i++)
              {
                Image *image = line->images[i];
                if (image)
                  {
                    int u = (line->image_col[i]-1) * vt->cw + (line->image_X[i] * vt->cw);
                    int v = y - vt->ch + (line->image_Y[i] * vt->ch);
                //  int rows = (image->height + (vt->ch-1) ) /vt->ch;
                    ctx_save (ctx);
                    // we give each texture a unique-id - if we use more ids than
                    // there is, ctx will alias the first image.
                    ctx_texture_init (ctx, image_id, image->width, image->height, image->kitty_format,
                                      image->data, NULL, NULL);
                    ctx_texture (ctx, image_id, u, v);
                    image_id ++;
                    ctx_rectangle (ctx, u, v, image->width, image->height);
                    ctx_fill (ctx);
                    ctx_restore (ctx);
                  }
              }
          }
      }
  }

  {
    /* draw ctx graphics */
    float y = y0 + vt->ch * vt->rows;
    for (int row = 0; y > - (vt->scroll + 8) * vt->ch; row ++)
      {
        CtxList *l = ctx_list_nth (vt->lines, row);
        if (row >= vt->rows)
          {
            l = ctx_list_nth (vt->scrollback, row-vt->rows);
          }
        if (l && y <= (vt->rows - vt->scroll) *  vt->ch)
          {
            VtLine *line = l->data;
            if (line->ctx)
              {
                ctx_save (ctx);
                ctx_translate (ctx, 0, (vt->rows-row-1) * (vt->ch) );
                //float factor = vt->cols * vt->cw / 1000.0;
                //ctx_scale (ctx, factor, factor);
                ctx_render_ctx (line->ctx, ctx);
                ctx_restore (ctx);
              }
          }
        y -= vt->ch;
      }
  }

  /* draw cursor */
  if (vt->cursor_visible)
    {
      ctx_rgba (ctx, 1.0, 1.0, 0.0, 0.3333);
      ctx_begin_path (ctx);
      ctx_rectangle (ctx,
                     cursor_x_px, cursor_y_px,
                     cursor_w, cursor_h);
      ctx_fill (ctx);
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
#define SCROLL_SPEED 0.2;
  if (vt->in_smooth_scroll)
    {
      if (vt->in_smooth_scroll<0)
        {
          vt->scroll_offset += SCROLL_SPEED;
          if (vt->scroll_offset >= 0.0)
            {
              vt->scroll_offset = 0;
              vt->in_smooth_scroll = 0;
              vt->rev++;
            }
        }
      else
        {
          vt->scroll_offset -= SCROLL_SPEED;
          if (vt->scroll_offset <= 0.0)
            {
              vt->scroll_offset = 0;
              vt->in_smooth_scroll = 0;
              vt->rev++;
            }
        }
    }
#if 0
  float width = (vt->cols + 1) * vt->cw;
  float height = (vt->rows + 1) * vt->ch;
  float scroll_width = height * 0.05;

    ctx_rectangle (ctx, width - scroll_width, 0, scroll_width, height);
    ctx_rgba (ctx,1,0,0,0.5);
    ctx_listen (ctx, CTX_PRESS, pressed, vt, NULL);
    ctx_fill (ctx);
#endif

     ctx_rectangle (ctx, 0, 0, vt->cols * vt->cw, vt->rows * vt->ch);
     ctx_listen (ctx, CTX_PRESS|CTX_RELEASE|CTX_MOTION, vt_mouse_event, vt, NULL);
     ctx_begin_path (ctx);

     if (!vt->in_alt_screen)
     {
       float disp_lines = vt->rows;
       float tot_lines = vt->line_count + vt->scrollback_count;
       float offset = (tot_lines - disp_lines - vt->scroll) / tot_lines;
       float win_len = disp_lines / tot_lines;
       ctx_rectangle (ctx, vt->cw * (vt->cols - 1.5),
                      0, 1.5 * vt->cw,
                      vt->rows * vt->ch);
       ctx_listen (ctx, CTX_PRESS, scrollbar_pressed, vt, NULL);
       ctx_listen (ctx, CTX_ENTER, scrollbar_enter, vt, NULL);
       ctx_listen (ctx, CTX_LEAVE, scrollbar_leave, vt, NULL);
       if (vt->scroll != 0 || scrollbar_focused)
         ctx_rgba (ctx, 0.5, 0.5, 0.5, .25);
       else
         ctx_rgba (ctx, 0.5, 0.5, 0.5, .15);
       ctx_fill (ctx);
       ctx_round_rectangle (ctx, vt->cw * (vt->cols - 1.5 + 0.1),
                            offset * vt->rows * vt->ch, (1.5-0.2) * vt->cw,
                            win_len * vt->rows * vt->ch,
                            vt->cw * 1.5 /2);
       ctx_listen (ctx, CTX_DRAG, scroll_handle_drag, vt, NULL);
       if (vt->scroll != 0 || scrollbar_focused)
         ctx_rgba (ctx, 1, 1, 1, .25);
       else
         ctx_rgba (ctx, 1, 1, 1, .15);
       ctx_fill (ctx);
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
     { return; }
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
   VtString *str = vt_string_new ("");
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
           vt_string_append_utf8char (str, c);
         }
     }
   ret = str->str;
   vt_string_free (str, 0);
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

 void vt_mouse (VT *vt, VtMouseEvent type, int x, int y, int px_x, int px_y)
 {
  char buf[64]="";
  int button_state = 0;
  if (! (vt->mouse | vt->mouse_all | vt->mouse_drag) )
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
          vt->rev++;
        }
      else if (type == VT_MOUSE_RELEASE)
      {
        vt->cursor_down = 0;
      }
      else if (type == VT_MOUSE_MOTION && vt->cursor_down)
        {
          if ((y - (int)vt->scroll >= vt->select_begin_row) || ((y - (int)vt->scroll == vt->select_begin_row) && (x >= vt->select_begin_col)))
          {
            vt->select_start_col = vt->select_begin_col;
            vt->select_start_row = vt->select_begin_row;
            vt->select_end_col = x;
            vt->select_end_row = y - (int)vt->scroll;
          }
          else
          {
            vt->select_start_col = x;
            vt->select_start_row = y - (int)vt->scroll;
            vt->select_end_col = vt->select_begin_col;
            vt->select_end_row = vt->select_begin_row;
          }

          vt->rev++;
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
          { return; }
        if (x==vt->lastx && y==vt->lasty)
          { return; }
        vt->lastx = x;
        vt->lasty = y;
 //     sprintf (buf, "\033[<35;%i;%iM", x, y);
        break;
      case VT_MOUSE_RELEASE:
        if (vt->mouse_decimal == 0)
          { button_state = 3; }
        break;
      case VT_MOUSE_PRESS:
        button_state = 0;
        break;
      case VT_MOUSE_DRAG: // XXX not really used - remove
        if (! (vt->mouse_all || vt->mouse_drag) )
          { return; }
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
