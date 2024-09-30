
typedef struct VtPty
{
  int        pty; //    0 if thread
  pid_t      pid; //    0 if thread
  int        done;

  void      *userdata;

  uint8_t   *shm;
  int        shm_size;
} VtPty;



ssize_t vtpty_read     (void *vtpty, void *buf, size_t count);
ssize_t vtpty_write    (void *vtpty, const void *buf, size_t count);
void    vtpty_resize   (void *vtpty, int cols, int rows,
                        int px_width, int px_height);
int     vtpty_waitdata (void  *vtpty, int timeout);
#define MAX_COLS 2048 // used for tabstops


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

struct _VT
{
  VtPty      vtpty;
  int        empty_count;
  int       id;
  unsigned char buf[BUFSIZ]; // need one per vt
  int keyrepeat;
  int       lastx;
  int       lasty;
  int        result;
  //SDL_Rect   dirty;
  float  dirtpad;
  float  dirtpad1;
  float  dirtpad2;
  float  dirtpad3;

  CtxClient *client;

  ssize_t (*write)   (void *serial_obj, const void *buf, size_t count);
  ssize_t (*read)    (void *serial_obj, void *buf, size_t count);
  int     (*waitdata)(void *serial_obj, int timeout);
  void    (*resize)  (void *serial_obj, int cols, int rows, int px_width, int px_height);


  char     *title;
  void    (*state) (VT *vt, int byte);

  AudioState audio; // < want to move this one level up and share impl
  GfxState   gfx;

  CtxList   *saved_lines;
  int       in_alt_screen;
  int       had_alt_screen;
  int       saved_line_count;
  char      *arg_copy;
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
  
  int can_launch;

  int unit_pixels;
  int mouse;
  int mouse_drag;
  int mouse_all;
  int mouse_decimal;

#if CTX_PTY
  uint8_t    utf8_holding[64];
#else
  uint8_t    utf8_holding[4]; /* only 4 needed for utf8 - but it's purpose
                                 is also overloaded for ctx journal command
                                 buffering , and the bigger sizes for the svg-like
                                 ctx parsing mode */
#endif
  int        utf8_expected_bytes;
  int        utf8_pos;


  int        ref_len;
  char       reference[16];
  int        in_prev_match;
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
  int        scrollbar_visible;
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
  int        scroll_on_input;
  int        scroll_on_output;

  char       *argument_buf;
  int        argument_buf_len;
  int        argument_buf_cap;
  uint8_t    tabs[MAX_COLS];
  int        inert;

  int        width;
  int        height;

  int        cw; // cell width
  int        ch; // cell height
  float      font_to_cell_scale;
  float      font_size; // when set with set_font_size, cw and ch are recomputed
  float      line_spacing; // using line_spacing
  float      scale_x;
  float      scale_y;

  int        ctx_pos;  // 1 is graphics above text, 0 or -1 is below text
  Ctx       *root_ctx; /* only used for knowledge of top-level dimensions */

  int        blink_state;

  FILE      *log;

  int cursor_down;

  int select_begin_col;
  int select_begin_row;
  int select_start_col;
  int select_start_row;
  int select_end_col;
  int select_end_row;
  int select_begin_x;
  int select_begin_y;
  int select_active;

  int popped;


  /* used to make runs of background on one line be drawn
   * as a single filled rectangle
   */
  int   bg_active;
  float bg_x0;
  float bg_y0;
  float bg_width;
  float bg_height;
  uint8_t bg_rgba[4];
};


// add vt_new_cb - suitable for hooking up to generic stdout/stdin callbacks
VT *vt_new (const char *command, int width, int height, float font_size, float line_spacing, int id, int can_launch);
VT *vt_new_argv (char **argv, int width, int height, float font_size, float line_spacing, int id, int can_launch);
VT *vt_new_thread (void (*start_routine)(void *userdata), void *userdata,
                   int width, int height, float font_size, float line_spacing, int id, int can_launch);


void vt_open_log (VT *vt, const char *path);

int         ctx_vt_had_alt_screen (VT *vt);
void        vt_set_px_size        (VT *vt, int width, int height);
void        vt_set_term_size      (VT *vt, int cols, int rows);

int         vt_cw                 (VT *vt);
int         vt_ch                 (VT *vt);
void        vt_set_font_size      (VT *vt, float font_size);
float       vt_get_font_size      (VT *vt);
void        vt_set_line_spacing   (VT *vt, float line_spacing);

int         vt_keyrepeat          (VT *vt);

int         vt_get_result         (VT *vt);
int         vt_is_done            (VT *vt);
int         vt_poll               (VT *vt, int timeout);
long        vt_rev                (VT *vt);
void        vt_destroy            (VT *vt);
int         vt_has_blink (VT *vt);

/* this is how mrg/mmm based key-events are fed into the vt engine
 */
void        vt_feed_keystring     (VT *vt, CtxEvent *event, const char *str);

void        vt_paste              (VT *vt, const char *str);

/* not needed when passing a commandline for command to
 * run, but could be used for injecting commands, or
 * output from stored shell commands/sessions to display
 */
void        vt_feed_byte          (VT *vt, int byte);

//)#define DEFAULT_SCROLLBACK   (1<<16)

#if CTX_PTY
#define DEFAULT_SCROLLBACK   (1<<13)
#else
#define DEFAULT_SCROLLBACK   (1)
#endif
#define DEFAULT_ROWS         24
#define DEFAULT_COLS         80

int         vt_get_line_count       (VT *vt);

pid_t       vt_get_pid              (VT *vt);

const char *vt_get_line             (VT *vt, int no);

void        vt_set_scrollback_lines (VT *vt, int scrollback_lines);
int         vt_get_scrollback_lines (VT *vt);

void        vt_set_scroll           (VT *vt, int scroll);
int         vt_get_scroll           (VT *vt);

int         vt_get_cols             (VT *vt);
int         vt_get_rows             (VT *vt);

char       *vt_get_selection        (VT *vt);
int         vt_get_cursor_x         (VT *vt);
int         vt_get_cursor_y         (VT *vt);

void        vt_draw                 (VT *vt, Ctx *ctx, double x, double y);
#if 0
void        vt_register_events      (VT *vt, Ctx *ctx, double x0, double y0);
#endif

void        vt_rev_inc              (VT *vt);

int         vt_mic (VT *vt);
void        vt_set_ctx (VT *vt, Ctx *ctx);  /* XXX: rename, this sets the parent/global ctx  */


int         vt_get_local (VT *vt);           // this is a hack for the settings tab
void        vt_set_local (VT *vt, int local);


typedef enum VtMouseEvent
{
  VT_MOUSE_MOTION = 0,
  VT_MOUSE_PRESS,
  VT_MOUSE_DRAG,
  VT_MOUSE_RELEASE,
} VtMouseEvent;

void vt_mouse (VT *vt, CtxEvent *event, VtMouseEvent type, int button, int x, int y, int px_x, int px_y);

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
  if (vt && vt->resize)
    { vt->resize (&vt->vtpty, cols, rows, px_width, px_height); }
}

