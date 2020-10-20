
typedef struct _VT VT;
typedef struct _CT CT;

VT *vt_new (const char *command, int width, int height, float font_size, float line_spacing, int id);

void vt_open_log (VT *vt, const char *path);

void        vt_set_px_size        (VT *vt, int width, int height);
void        vt_set_term_size      (VT *vt, int cols, int rows);

int         vt_cw                 (VT *vt);
int         vt_ch                 (VT *vt);
void        vt_set_font_size      (VT *vt, float font_size);
float       vt_get_font_size      (VT *vt);
void        vt_set_line_spacing   (VT *vt, float line_spacing);

const char *vt_find_shell_command (void);

int         vt_keyrepeat          (VT *vt);

int         vt_get_result         (VT *vt);
int         vt_is_done            (VT *vt);
int         vt_poll               (VT *vt, int timeout);
long        vt_rev                (VT *vt);
void        vt_destroy            (VT *vt);
int         vt_has_blink (VT *vt);

/* this is how mrg/mmm based key-events are fed into the vt engine
 */
void        vt_feed_keystring     (VT *vt, const char *str);

void        vt_paste              (VT *vt, const char *str);

/* not needed when passing a commandline for command to
 * run, but could be used for injecting commands, or
 * output from stored shell commands/sessions to display
 */
//void        vt_feed_byte          (VT *vt, int byte);

#define DEFAULT_SCROLLBACK   2048
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
int         vt_get_local (VT *vt);
void        vt_set_local (VT *vt, int local);
void        vt_rev_inc              (VT *vt);
int vt_mic (VT *vt);
void vt_set_ctx (VT *vt, Ctx *ctx);

typedef enum VtMouseEvent
{
  VT_MOUSE_MOTION = 0,
  VT_MOUSE_PRESS,
  VT_MOUSE_DRAG,
  VT_MOUSE_RELEASE,
} VtMouseEvent;

void vt_mouse (VT *vt, VtMouseEvent type, int button, int x, int y, int px_x, int px_y);
void vt_set_mmm (VT *vt, void *mmm);
