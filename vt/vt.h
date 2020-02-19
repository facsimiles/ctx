
typedef struct _MrgVT MrgVT;

MrgVT *ctx_vt_new (const char *command, int cols, int rows, float font_size, float line_spacing);

void ctx_vt_open_log (MrgVT *vt, const char *path);

int         ctx_vt_cw                 (MrgVT *vt);
int         ctx_vt_ch                 (MrgVT *vt);
void        ctx_vt_set_font_size      (MrgVT *vt, float font_size);
void        ctx_vt_set_line_spacing   (MrgVT *vt, float line_spacing);

const char *ctx_vt_find_shell_command (void);

int         ctx_vt_keyrepeat          (MrgVT *vt);

int         ctx_vt_get_result         (MrgVT *vt);
int         ctx_vt_is_done            (MrgVT *vt);
int         ctx_vt_poll               (MrgVT *vt, int timeout);
long        ctx_vt_rev                (MrgVT *vt);
void        ctx_vt_destroy            (MrgVT *vt);
void        ctx_vt_set_term_size      (MrgVT *vt, int cols, int rows);
int ctx_vt_has_blink (MrgVT *vt);

/* this is how mrg/mmm based key-events are fed into the vt engine
 */
void        ctx_vt_feed_keystring     (MrgVT *vt, const char *str);

/* not needed when passing a commandline for command to
 * run, but could be used for injecting commands, or
 * output from stored shell commands/sessions to display
 */
//void        ctx_vt_feed_byte          (MrgVT *vt, int byte);

#define DEFAULT_SCROLLBACK   2048
#define DEFAULT_ROWS         24
#define DEFAULT_COLS         80

int         ctx_vt_get_line_count       (MrgVT *vt);

pid_t       ctx_vt_get_pid              (MrgVT *vt);

const char *ctx_vt_get_line             (MrgVT *vt, int no);

void        ctx_vt_set_scrollback_lines (MrgVT *vt, int scrollback_lines);
int         ctx_vt_get_scrollback_lines (MrgVT *vt);

void        ctx_vt_set_scroll           (MrgVT *vt, int scroll);
int         ctx_vt_get_scroll           (MrgVT *vt);

int         ctx_vt_get_cols             (MrgVT *vt);
int         ctx_vt_get_rows             (MrgVT *vt);

int         ctx_vt_get_cursor_x         (MrgVT *vt);
int         ctx_vt_get_cursor_y         (MrgVT *vt);

void        ctx_vt_draw                 (MrgVT *vt, Ctx *ctx, double x, double y);

void        ctx_vt_rev_inc              (MrgVT *vt);

typedef enum VtMouseEvent {
	VT_MOUSE_MOTION = 0,
	VT_MOUSE_PRESS,
	VT_MOUSE_DRAG,
	VT_MOUSE_RELEASE,
} VtMouseEvent;

void ctx_vt_mouse (MrgVT *vt, VtMouseEvent type, int x, int y, int px_x, int px_y);
void ctx_vt_set_mmm (MrgVT *vt, void *mmm);
