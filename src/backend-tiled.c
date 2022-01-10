#include "ctx-split.h"

#if CTX_TILED
static inline int
ctx_tiled_threads_done (CtxTiled *tiled)
{
  int sum = 0;
  for (int i = 0; i < _ctx_max_threads; i++)
  {
     if (tiled->rendered_frame[i] == tiled->render_frame)
       sum ++;
  }
  return sum;
}

int _ctx_damage_control = 0;

void ctx_tiled_free (CtxTiled *tiled)
{
  tiled->quit = 1;
  mtx_lock (&tiled->mtx);
  cnd_broadcast (&tiled->cond);
  mtx_unlock (&tiled->mtx);

  while (tiled->thread_quit < _ctx_max_threads)
    usleep (1000);

  if (tiled->pixels)
  {
    free (tiled->pixels);
    tiled->pixels = NULL;
    for (int i = 0 ; i < _ctx_max_threads; i++)
    {
      if (tiled->host[i])
        ctx_free (tiled->host[i]);
      tiled->host[i]=NULL;
    }
    ctx_free (tiled->ctx_copy);
  }
  // leak?
}
static unsigned char *sdl_icc = NULL;
static long sdl_icc_length = 0;

static void ctx_tiled_flush (Ctx *ctx)
{
  CtxTiled *tiled = (CtxTiled*)ctx->backend;
  mtx_lock (&tiled->mtx);
  if (tiled->shown_frame == tiled->render_frame)
  {
    int dirty_tiles = 0;
    ctx_set_drawlist (tiled->ctx_copy, &tiled->backend.ctx->drawlist.entries[0],
                                           tiled->backend.ctx->drawlist.count * 9);
    if (_ctx_enable_hash_cache)
    {
      Ctx *hasher = ctx_hasher_new (tiled->width, tiled->height,
                        CTX_HASH_COLS, CTX_HASH_ROWS);
      ctx_render_ctx (tiled->ctx_copy, hasher);

      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          uint8_t *new_hash = ctx_hasher_get_hash (hasher, col, row);
          if (new_hash && memcmp (new_hash, &tiled->hashes[(row * CTX_HASH_COLS + col) *  20], 20))
          {
            memcpy (&tiled->hashes[(row * CTX_HASH_COLS +  col)*20], new_hash, 20);
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
            dirty_tiles++;
          }
          else
          {
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = -1;
          }
        }
      free (((CtxHasher*)(hasher->backend))->hashes);
      ctx_free (hasher);
    }
    else
    {
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++)
          {
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
            dirty_tiles++;
          }
    }
    int dirty_no = 0;
    if (dirty_tiles)
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        if (tiled->tile_affinity[row * CTX_HASH_COLS + col] != -1)
        {
          tiled->tile_affinity[row * CTX_HASH_COLS + col] = dirty_no * (_ctx_max_threads) / dirty_tiles;
          dirty_no++;
          if (col > tiled->max_col) tiled->max_col = col;
          if (col < tiled->min_col) tiled->min_col = col;
          if (row > tiled->max_row) tiled->max_row = row;
          if (row < tiled->min_row) tiled->min_row = row;
        }
      }

    if (_ctx_damage_control)
    {
      for (int i = 0; i < tiled->width * tiled->height; i++)
      {
        tiled->pixels[i*4+2]  = (tiled->pixels[i*4+2] + 255)/2;
      }
    }

    tiled->render_frame = ++tiled->frame;

#if 0

          //if (tiled->tile_affinity[hno]==no)
          {
            int x0 = ((tiled->width)/CTX_HASH_COLS) * 0;
            int y0 = ((tiled->height)/CTX_HASH_ROWS) * 0;
            int width = tiled->width / CTX_HASH_COLS;
            int height = tiled->height / CTX_HASH_ROWS;
            Ctx *host = tiled->host[0];

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->backend;
            int swap_red_green = ((CtxRasterizer*)(host->backend))->swap_red_green;
            ctx_rasterizer_init (rasterizer,
                                 host, tiled->backend.ctx, &host->state,
                                 &tiled->pixels[tiled->width * 4 * y0 + x0 * 4],
                                 0, 0, 1, 1,
                                 tiled->width*4, CTX_FORMAT_BGRA8,
                                 tiled->antialias);
            ((CtxRasterizer*)(host->backend))->swap_red_green = swap_red_green;
            if (sdl_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, sdl_icc, sdl_icc_length);

            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (tiled->ctx_copy, host);
          }
#endif
    cnd_broadcast (&tiled->cond);
  }
  else
  {
    fprintf (stderr, "{drip}");
  }
  mtx_unlock (&tiled->mtx);
  ctx_drawlist_clear (ctx);
}

static
void ctx_tiled_render_fun (void **data)
{
  int      no = (size_t)data[0];
  CtxTiled *tiled = data[1];

  while (!tiled->quit)
  {
    Ctx *host = tiled->host[no];

    mtx_lock (&tiled->mtx);
    cnd_wait(&tiled->cond, &tiled->mtx);
    mtx_unlock (&tiled->mtx);

    if (tiled->render_frame != tiled->rendered_frame[no])
    {
      int hno = 0;
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++, hno++)
        {
          if (tiled->tile_affinity[hno]==no)
          {
            int x0 = ((tiled->width)/CTX_HASH_COLS) * col;
            int y0 = ((tiled->height)/CTX_HASH_ROWS) * row;
            int width = tiled->width / CTX_HASH_COLS;
            int height = tiled->height / CTX_HASH_ROWS;

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->backend;
#if 1 // merge horizontally adjecant tiles of same affinity into one job
            while (col + 1 < CTX_HASH_COLS &&
                   tiled->tile_affinity[hno+1] == no)
            {
              width += tiled->width / CTX_HASH_COLS;
              col++;
              hno++;
            }
#endif
            int swap_red_green = ((CtxRasterizer*)(host->backend))->swap_red_green;
            ctx_rasterizer_init (rasterizer,
                                 host, tiled->backend.ctx, &host->state,
                                 &tiled->pixels[tiled->width * 4 * y0 + x0 * 4],
                                 0, 0, width, height,
                                 tiled->width*4, CTX_FORMAT_BGRA8,
                                 tiled->antialias);
            ((CtxRasterizer*)(host->backend))->swap_red_green = swap_red_green;
            if (sdl_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, sdl_icc, sdl_icc_length);

            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (tiled->ctx_copy, host);
          }
        }
      tiled->rendered_frame[no] = tiled->render_frame;
    }
  }
  tiled->thread_quit++; // need atomic?
}


static int       ctx_tiled_cursor_drawn   = 0;
static int       ctx_tiled_cursor_drawn_x = 0;
static int       ctx_tiled_cursor_drawn_y = 0;
static CtxCursor ctx_tiled_cursor_drawn_shape = 0;


#define CTX_FB_HIDE_CURSOR_FRAMES 200

static int ctx_tiled_cursor_same_pos = CTX_FB_HIDE_CURSOR_FRAMES;

static inline int ctx_is_in_cursor (int x, int y, int size, CtxCursor shape)
{
  switch (shape)
  {
    case CTX_CURSOR_ARROW:
      if (x > ((size * 4)-y*4)) return 0;
      if (x < y && x > y / 16)
        return 1;
      return 0;

    case CTX_CURSOR_RESIZE_SE:
    case CTX_CURSOR_RESIZE_NW:
    case CTX_CURSOR_RESIZE_SW:
    case CTX_CURSOR_RESIZE_NE:
      {
        float theta = -45.0/180 * M_PI;
        float cos_theta;
        float sin_theta;

        if ((shape == CTX_CURSOR_RESIZE_SW) ||
            (shape == CTX_CURSOR_RESIZE_NE))
        {
          theta = -theta;
          cos_theta = cos (theta);
          sin_theta = sin (theta);
        }
        else
        {
          cos_theta = cos (theta);
          sin_theta = sin (theta);
        }
        int rot_x = x * cos_theta - y * sin_theta;
        int rot_y = y * cos_theta + x * sin_theta;
        x = rot_x;
        y = rot_y;
      }
      /*FALLTHROUGH*/
    case CTX_CURSOR_RESIZE_W:
    case CTX_CURSOR_RESIZE_E:
    case CTX_CURSOR_RESIZE_ALL:
      if (abs (x) < size/2 && abs (y) < size/2)
      {
        if (abs(y) < size/10)
        {
          return 1;
        }
      }
      if ((abs (x) - size/ (shape == CTX_CURSOR_RESIZE_ALL?2:2.7)) >= 0)
      {
        if (abs(y) < (size/2.8)-(abs(x) - (size/2)))
          return 1;
      }
      if (shape != CTX_CURSOR_RESIZE_ALL)
        break;
      /* FALLTHROUGH */
    case CTX_CURSOR_RESIZE_S:
    case CTX_CURSOR_RESIZE_N:
      if (abs (y) < size/2 && abs (x) < size/2)
      {
        if (abs(x) < size/10)
        {
          return 1;
        }
      }
      if ((abs (y) - size/ (shape == CTX_CURSOR_RESIZE_ALL?2:2.7)) >= 0)
      {
        if (abs(x) < (size/2.8)-(abs(y) - (size/2)))
          return 1;
      }
      break;
#if 0
    case CTX_CURSOR_RESIZE_ALL:
      if (abs (x) < size/2 && abs (y) < size/2)
      {
        if (abs (x) < size/10 || abs(y) < size/10)
          return 1;
      }
      break;
#endif
    default:
      return (x ^ y) & 1;
  }
  return 0;
}

static void ctx_tiled_undraw_cursor (CtxTiled *tiled)
{
    int cursor_size = ctx_height (tiled->backend.ctx) / 28;

    if (ctx_tiled_cursor_drawn)
    {
      int no = 0;
      int startx = -cursor_size;
      int starty = -cursor_size;
      if (ctx_tiled_cursor_drawn_shape == CTX_CURSOR_ARROW)
        startx = starty = 0;

      for (int y = starty; y < cursor_size; y++)
      for (int x = startx; x < cursor_size; x++, no+=4)
      {
        if (x + ctx_tiled_cursor_drawn_x < tiled->width && y + ctx_tiled_cursor_drawn_y < tiled->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, ctx_tiled_cursor_drawn_shape))
          {
            int o = ((ctx_tiled_cursor_drawn_y + y) * tiled->width + (ctx_tiled_cursor_drawn_x + x)) * 4;
            tiled->fb[o+0]^=0x88;
            tiled->fb[o+1]^=0x88;
            tiled->fb[o+2]^=0x88;
          }
        }
      }

    ctx_tiled_cursor_drawn = 0;
    }
}

static void ctx_tiled_draw_cursor (CtxTiled *tiled)
{
    int cursor_x    = ctx_pointer_x (tiled->backend.ctx);
    int cursor_y    = ctx_pointer_y (tiled->backend.ctx);
    int cursor_size = ctx_height (tiled->backend.ctx) / 28;
    CtxCursor cursor_shape = tiled->backend.ctx->cursor;
    int no = 0;

    if (cursor_x == ctx_tiled_cursor_drawn_x &&
        cursor_y == ctx_tiled_cursor_drawn_y &&
        cursor_shape == ctx_tiled_cursor_drawn_shape)
      ctx_tiled_cursor_same_pos ++;
    else
      ctx_tiled_cursor_same_pos = 0;

    if (ctx_tiled_cursor_same_pos >= CTX_FB_HIDE_CURSOR_FRAMES)
    {
      if (ctx_tiled_cursor_drawn)
        ctx_tiled_undraw_cursor (tiled);
      return;
    }

    /* no need to flicker when stationary, motion flicker can also be removed
     * by combining the previous and next position masks when a motion has
     * occured..
     */
    if (ctx_tiled_cursor_same_pos && ctx_tiled_cursor_drawn)
      return;

    ctx_tiled_undraw_cursor (tiled);

    no = 0;

    int startx = -cursor_size;
    int starty = -cursor_size;

    if (cursor_shape == CTX_CURSOR_ARROW)
      startx = starty = 0;

    for (int y = starty; y < cursor_size; y++)
      for (int x = startx; x < cursor_size; x++, no+=4)
      {
        if (x + cursor_x < tiled->width && y + cursor_y < tiled->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, cursor_shape))
          {
            int o = ((cursor_y + y) * tiled->width + (cursor_x + x)) * 4;
            tiled->fb[o+0]^=0x88;
            tiled->fb[o+1]^=0x88;
            tiled->fb[o+2]^=0x88;
          }
        }
      }
    ctx_tiled_cursor_drawn = 1;
    ctx_tiled_cursor_drawn_x = cursor_x;
    ctx_tiled_cursor_drawn_y = cursor_y;
    ctx_tiled_cursor_drawn_shape = cursor_shape;
}

#endif
#if CTX_EVENTS


#define evsource_has_event(es)   (es)->has_event((es))
#define evsource_get_event(es)   (es)->get_event((es))
#define evsource_destroy(es)     do{if((es)->destroy)(es)->destroy((es));}while(0)
#define evsource_set_coord(es,x,y) do{if((es)->set_coord)(es)->set_coord((es),(x),(y));}while(0)
#define evsource_get_fd(es)      ((es)->get_fd?(es)->get_fd((es)):0)

static int mice_has_event ();
static char *mice_get_event ();
static void mice_destroy ();
static int mice_get_fd (EvSource *ev_source);
static void mice_set_coord (EvSource *ev_source, double x, double y);

static EvSource ctx_ev_src_mice = {
  NULL,
  (void*)mice_has_event,
  (void*)mice_get_event,
  (void*)mice_destroy,
  mice_get_fd,
  mice_set_coord
};

typedef struct Mice
{
  int     fd;
  double  x;
  double  y;
  int     button;
  int     prev_state;
} Mice;

Mice *_mrg_evsrc_coord = NULL;
static int _ctx_mice_fd = 0;

static Mice  mice;
static Mice* mrg_mice_this = &mice;

static int mmm_evsource_mice_init ()
{
  unsigned char reset[]={0xff};
  /* need to detect which event */

  mrg_mice_this->prev_state = 0;
  mrg_mice_this->fd = open ("/dev/input/mice", O_RDONLY | O_NONBLOCK);
  if (mrg_mice_this->fd == -1)
  {
    fprintf (stderr, "error opening /dev/input/mice device, maybe add user to input group if such group exist, or otherwise make the rights be satisfied.\n");
    return -1;
  }
  if (write (mrg_mice_this->fd, reset, 1) == -1)
  {
    // might happen if we're a regular user with only read permission
  }
  _ctx_mice_fd = mrg_mice_this->fd;
  _mrg_evsrc_coord = mrg_mice_this;
  return 0;
}

static void mice_destroy ()
{
  if (mrg_mice_this->fd != -1)
    close (mrg_mice_this->fd);
}

static int mice_has_event ()
{
  struct timeval tv;
  int retval;

  if (mrg_mice_this->fd == -1)
    return 0;

  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET(mrg_mice_this->fd, &rfds);
  tv.tv_sec = 0; tv.tv_usec = 0;
  retval = select (mrg_mice_this->fd+1, &rfds, NULL, NULL, &tv);
  if (retval == 1)
    return FD_ISSET (mrg_mice_this->fd, &rfds);
  return 0;
}

static char *mice_get_event ()
{
  const char *ret = "pm";
  double relx, rely;
  signed char buf[3];
  int n_read = 0;
  CtxTiled *tiled = (void*)ctx_ev_src_mice.priv;
  n_read = read (mrg_mice_this->fd, buf, 3);
  if (n_read == 0)
     return strdup ("");
  relx = buf[1];
  rely = -buf[2];

  if (relx < 0)
  {
    if (relx > -6)
    relx = - relx*relx;
    else
    relx = -36;
  }
  else
  {
    if (relx < 6)
    relx = relx*relx;
    else
    relx = 36;
  }

  if (rely < 0)
  {
    if (rely > -6)
    rely = - rely*rely;
    else
    rely = -36;
  }
  else
  {
    if (rely < 6)
    rely = rely*rely;
    else
    rely = 36;
  }

  mrg_mice_this->x += relx;
  mrg_mice_this->y += rely;

  if (mrg_mice_this->x < 0)
    mrg_mice_this->x = 0;
  if (mrg_mice_this->y < 0)
    mrg_mice_this->y = 0;
  if (mrg_mice_this->x >= tiled->width)
    mrg_mice_this->x = tiled->width -1;
  if (mrg_mice_this->y >= tiled->height)
    mrg_mice_this->y = tiled->height -1;
  int button = 0;
  
  if ((mrg_mice_this->prev_state & 1) != (buf[0] & 1))
    {
      if (buf[0] & 1)
        {
          ret = "pp";
        }
      else
        {
          ret = "pr";
        }
      button = 1;
    }
  else if (buf[0] & 1)
  {
    ret = "pd";
    button = 1;
  }

  if (!button)
  {
    if ((mrg_mice_this->prev_state & 2) != (buf[0] & 2))
    {
      if (buf[0] & 2)
        {
          ret = "pp";
        }
      else
        {
          ret = "pr";
        }
      button = 3;
    }
    else if (buf[0] & 2)
    {
      ret = "pd";
      button = 3;
    }
  }

  if (!button)
  {
    if ((mrg_mice_this->prev_state & 4) != (buf[0] & 4))
    {
      if (buf[0] & 4)
        {
          ret = "pp";
        }
      else
        {
          ret = "pr";
        }
      button = 2;
    }
    else if (buf[0] & 4)
    {
      ret = "pd";
      button = 2;
    }
  }

  mrg_mice_this->prev_state = buf[0];

  {
    char *r = malloc (64);
    sprintf (r, "%s %.0f %.0f %i", ret, mrg_mice_this->x, mrg_mice_this->y, button);
    return r;
  }

  return NULL;
}

static int mice_get_fd (EvSource *ev_source)
{
  return mrg_mice_this->fd;
}

static void mice_set_coord (EvSource *ev_source, double x, double y)
{
  mrg_mice_this->x = x;
  mrg_mice_this->y = y;
}

static EvSource *evsource_mice_new (void)
{
  if (mmm_evsource_mice_init () == 0)
    {
      mrg_mice_this->x = 0;
      mrg_mice_this->y = 0;
      return &ctx_ev_src_mice;
    }
  return NULL;
}

static int evsource_kb_has_event (void);
static char *evsource_kb_get_event (void);
static void evsource_kb_destroy (int sign);
static int evsource_kb_get_fd (void);

/* kept out of struct to be reachable by atexit */
static EvSource ctx_ev_src_kb = {
  NULL,
  (void*)evsource_kb_has_event,
  (void*)evsource_kb_get_event,
  (void*)evsource_kb_destroy,
  (void*)evsource_kb_get_fd,
  NULL
};

static struct termios orig_attr;

static void real_evsource_kb_destroy (int sign)
{
  static int done = 0;

  if (sign == 0)
    return;

  if (done)
    return;
  done = 1;

  switch (sign)
  {
    case  -11:break; /* will be called from atexit with sign==-11 */
    case   SIGSEGV: break;//fprintf (stderr, " SIGSEGV\n");break;
    case   SIGABRT: fprintf (stderr, " SIGABRT\n");break;
    case   SIGBUS:  fprintf (stderr, " SIGBUS\n");break;
    case   SIGKILL: fprintf (stderr, " SIGKILL\n");break;
    case   SIGINT:  fprintf (stderr, " SIGINT\n");break;
    case   SIGTERM: fprintf (stderr, " SIGTERM\n");break;
    case   SIGQUIT: fprintf (stderr, " SIGQUIT\n");break;
    default: fprintf (stderr, "sign: %i\n", sign);
             fprintf (stderr, "%i %i %i %i %i %i %i\n", SIGSEGV, SIGABRT, SIGBUS, SIGKILL, SIGINT, SIGTERM, SIGQUIT);
  }
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr);
  //fprintf (stderr, "evsource kb destroy\n");
}

static void evsource_kb_destroy (int sign)
{
  real_evsource_kb_destroy (-11);
}

static int evsource_kb_init ()
{
//  ioctl(STDIN_FILENO, KDSKBMODE, K_RAW);
  //atexit ((void*) real_evsource_kb_destroy);
  signal (SIGSEGV, (void*) real_evsource_kb_destroy);
  signal (SIGABRT, (void*) real_evsource_kb_destroy);
  signal (SIGBUS,  (void*) real_evsource_kb_destroy);
  signal (SIGKILL, (void*) real_evsource_kb_destroy);
  signal (SIGINT,  (void*) real_evsource_kb_destroy);
  signal (SIGTERM, (void*) real_evsource_kb_destroy);
  signal (SIGQUIT, (void*) real_evsource_kb_destroy);

  struct termios raw;
  if (tcgetattr (STDIN_FILENO, &orig_attr) == -1)
    {
      fprintf (stderr, "error initializing keyboard\n");
      return -1;
    }
  raw = orig_attr;

  cfmakeraw (&raw);

  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return 0; // XXX? return other value?

  return 0;
}
static int evsource_kb_has_event (void)
{
  struct timeval tv;
  int retval;

  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET(STDIN_FILENO, &rfds);
  tv.tv_sec = 0; tv.tv_usec = 0;
  retval = select (STDIN_FILENO+1, &rfds, NULL, NULL, &tv);
  return retval == 1;
}

/* note that a nick can have multiple occurences, the labels
 * should be kept the same for all occurences of a combination.
 *
 * this table is taken from nchanterm.
 */
typedef struct MmmKeyCode {
  char *nick;          /* programmers name for key */
  char  sequence[10];  /* terminal sequence */
} MmmKeyCode;
static const MmmKeyCode ufb_keycodes[]={
  {"up",                  "\e[A"},
  {"down",                "\e[B"},
  {"right",               "\e[C"},
  {"left",                "\e[D"},

  {"shift-up",            "\e[1;2A"},
  {"shift-down",          "\e[1;2B"},
  {"shift-right",         "\e[1;2C"},
  {"shift-left",          "\e[1;2D"},

  {"alt-up",              "\e[1;3A"},
  {"alt-down",            "\e[1;3B"},
  {"alt-right",           "\e[1;3C"},
  {"alt-left",            "\e[1;3D"},
  {"alt-shift-up",         "\e[1;4A"},
  {"alt-shift-down",       "\e[1;4B"},
  {"alt-shift-right",      "\e[1;4C"},
  {"alt-shift-left",       "\e[1;4D"},

  {"control-up",          "\e[1;5A"},
  {"control-down",        "\e[1;5B"},
  {"control-right",       "\e[1;5C"},
  {"control-left",        "\e[1;5D"},

  /* putty */
  {"control-up",          "\eOA"},
  {"control-down",        "\eOB"},
  {"control-right",       "\eOC"},
  {"control-left",        "\eOD"},

  {"control-shift-up",    "\e[1;6A"},
  {"control-shift-down",  "\e[1;6B"},
  {"control-shift-right", "\e[1;6C"},
  {"control-shift-left",  "\e[1;6D"},

  {"control-up",          "\eOa"},
  {"control-down",        "\eOb"},
  {"control-right",       "\eOc"},
  {"control-left",        "\eOd"},

  {"shift-up",            "\e[a"},
  {"shift-down",          "\e[b"},
  {"shift-right",         "\e[c"},
  {"shift-left",          "\e[d"},

  {"insert",              "\e[2~"},
  {"delete",              "\e[3~"},
  {"page-up",             "\e[5~"},
  {"page-down",           "\e[6~"},
  {"home",                "\eOH"},
  {"end",                 "\eOF"},
  {"home",                "\e[H"},
  {"end",                 "\e[F"},
 {"control-delete",       "\e[3;5~"},
  {"shift-delete",        "\e[3;2~"},
  {"control-shift-delete","\e[3;6~"},

  {"F1",         "\e[25~"},
  {"F2",         "\e[26~"},
  {"F3",         "\e[27~"},
  {"F4",         "\e[26~"},


  {"F1",         "\e[11~"},
  {"F2",         "\e[12~"},
  {"F3",         "\e[13~"},
  {"F4",         "\e[14~"},
  {"F1",         "\eOP"},
  {"F2",         "\eOQ"},
  {"F3",         "\eOR"},
  {"F4",         "\eOS"},
  {"F5",         "\e[15~"},
  {"F6",         "\e[16~"},
  {"F7",         "\e[17~"},
  {"F8",         "\e[18~"},
  {"F9",         "\e[19~"},
  {"F9",         "\e[20~"},
  {"F10",        "\e[21~"},
  {"F11",        "\e[22~"},
  {"F12",        "\e[23~"},
  {"tab",         {9, '\0'}},
  {"shift-tab",   {27, 9, '\0'}}, // also generated by alt-tab in linux console
  {"alt-space",   {27, ' ', '\0'}},
  {"shift-tab",   "\e[Z"},
  {"backspace",   {127, '\0'}},
  {"space",       " "},
  {"\e",          "\e"},
  {"return",      {10,0}},
  {"return",      {13,0}},
  /* this section could be autogenerated by code */
  {"control-a",   {1,0}},
  {"control-b",   {2,0}},
  {"control-c",   {3,0}},
  {"control-d",   {4,0}},
  {"control-e",   {5,0}},
  {"control-f",   {6,0}},
  {"control-g",   {7,0}},
  {"control-h",   {8,0}}, /* backspace? */
  {"control-i",   {9,0}},
  {"control-j",   {10,0}},
  {"control-k",   {11,0}},
  {"control-l",   {12,0}},
  {"control-n",   {14,0}},
  {"control-o",   {15,0}},
  {"control-p",   {16,0}},
  {"control-q",   {17,0}},
  {"control-r",   {18,0}},
  {"control-s",   {19,0}},
  {"control-t",   {20,0}},
  {"control-u",   {21,0}},
  {"control-v",   {22,0}},
  {"control-w",   {23,0}},
  {"control-x",   {24,0}},
  {"control-y",   {25,0}},
  {"control-z",   {26,0}},
  {"alt-`",       "\e`"},
  {"alt-0",       "\e0"},
  {"alt-1",       "\e1"},
  {"alt-2",       "\e2"},
  {"alt-3",       "\e3"},
  {"alt-4",       "\e4"},
  {"alt-5",       "\e5"},
  {"alt-6",       "\e6"},
  {"alt-7",       "\e7"}, /* backspace? */
  {"alt-8",       "\e8"},
  {"alt-9",       "\e9"},
  {"alt-+",       "\e+"},
  {"alt--",       "\e-"},
  {"alt-/",       "\e/"},
  {"alt-a",       "\ea"},
  {"alt-b",       "\eb"},
  {"alt-c",       "\ec"},
  {"alt-d",       "\ed"},
  {"alt-e",       "\ee"},
  {"alt-f",       "\ef"},
  {"alt-g",       "\eg"},
  {"alt-h",       "\eh"}, /* backspace? */
  {"alt-i",       "\ei"},
  {"alt-j",       "\ej"},
  {"alt-k",       "\ek"},
  {"alt-l",       "\el"},
  {"alt-n",       "\em"},
  {"alt-n",       "\en"},
  {"alt-o",       "\eo"},
  {"alt-p",       "\ep"},
  {"alt-q",       "\eq"},
  {"alt-r",       "\er"},
  {"alt-s",       "\es"},
  {"alt-t",       "\et"},
  {"alt-u",       "\eu"},
  {"alt-v",       "\ev"},
  {"alt-w",       "\ew"},
  {"alt-x",       "\ex"},
  {"alt-y",       "\ey"},
  {"alt-z",       "\ez"},
  /* Linux Console  */
  {"home",       "\e[1~"},
  {"end",        "\e[4~"},
  {"F1",         "\e[[A"},
  {"F2",         "\e[[B"},
  {"F3",         "\e[[C"},
  {"F4",         "\e[[D"},
  {"F5",         "\e[[E"},
  {"F6",         "\e[[F"},
  {"F7",         "\e[[G"},
  {"F8",         "\e[[H"},
  {"F9",         "\e[[I"},
  {"F10",        "\e[[J"},
  {"F11",        "\e[[K"},
  {"F12",        "\e[[L"},
  {NULL, }
};
static int fb_keyboard_match_keycode (const char *buf, int length, const MmmKeyCode **ret)
{
  int i;
  int matches = 0;

  if (!strncmp (buf, "\e[M", MIN(length,3)))
    {
      if (length >= 6)
        return 9001;
      return 2342;
    }
  for (i = 0; ufb_keycodes[i].nick; i++)
    if (!strncmp (buf, ufb_keycodes[i].sequence, length))
      {
        matches ++;
        if ((int)strlen (ufb_keycodes[i].sequence) == length && ret)
          {
            *ret = &ufb_keycodes[i];
            return 1;
          }
      }
  if (matches != 1 && ret)
    *ret = NULL;
  return matches==1?2:matches;
}

//int is_active (void *host)
//{
//        return 1;
//}

static char *evsource_kb_get_event (void)
{
  unsigned char buf[20];
  int length;


  for (length = 0; length < 10; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
        const MmmKeyCode *match = NULL;

        //if (!is_active (ctx_ev_src_kb.priv))
        //  return NULL;

        /* special case ESC, so that we can use it alone in keybindings */
        if (length == 0 && buf[0] == 27)
          {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO (&rfds);
            FD_SET (STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * 120;
            if (select (STDIN_FILENO+1, &rfds, NULL, NULL, &tv) == 0)
              return strdup ("escape");
          }

        switch (fb_keyboard_match_keycode ((void*)buf, length + 1, &match))
          {
            case 1: /* unique match */
              if (!match)
                return NULL;
              return strdup (match->nick);
              break;
            case 0: /* no matches, bail*/
             {
                static char ret[256]="";
                if (length == 0 && ctx_utf8_len (buf[0])>1) /* read a
                                                             * single unicode
                                                             * utf8 character
                                                             */
                  {
                    int bytes = read (STDIN_FILENO, &buf[length+1], ctx_utf8_len(buf[0])-1);
                    if (bytes)
                    {
                      buf[ctx_utf8_len(buf[0])]=0;
                      strcpy (ret, (void*)buf);
                    }
                    return strdup(ret); //XXX: simplify
                  }
                if (length == 0) /* ascii */
                  {
                    buf[1]=0;
                    strcpy (ret, (void*)buf);
                    return strdup(ret);
                  }
                sprintf (ret, "unhandled %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c'",
                    length >=0 ? buf[0] : 0,
                    length >=0 ? buf[0]>31?buf[0]:'?' : ' ',
                    length >=1 ? buf[1] : 0,
                    length >=1 ? buf[1]>31?buf[1]:'?' : ' ',
                    length >=2 ? buf[2] : 0,
                    length >=2 ? buf[2]>31?buf[2]:'?' : ' ',
                    length >=3 ? buf[3] : 0,
                    length >=3 ? buf[3]>31?buf[3]:'?' : ' ',
                    length >=4 ? buf[4] : 0,
                    length >=4 ? buf[4]>31?buf[4]:'?' : ' ',
                    length >=5 ? buf[5] : 0,
                    length >=5 ? buf[5]>31?buf[5]:'?' : ' ',
                    length >=6 ? buf[6] : 0,
                    length >=6 ? buf[6]>31?buf[6]:'?' : ' '
                    );
                return strdup(ret);
            }
              return NULL;
            default: /* continue */
              break;
          }
      }
    else
      return strdup("key read eek");
  return strdup("fail");
}

static int evsource_kb_get_fd (void)
{
  return STDIN_FILENO;
}


static EvSource *evsource_kb_new (void)
{
  if (evsource_kb_init() == 0)
  {
    return &ctx_ev_src_kb;
  }
  return NULL;
}

#if CTX_BABL
static int _ctx_babl_inits = 0;
#endif
static void ctx_babl_init (void)
{
#if CTX_BABL
  _ctx_babl_inits ++;
  if (_ctx_babl_inits == 1)
  {
    babl_init ();
  }
#endif
}
static void ctx_babl_exit (void)
{
#if CTX_BABL
  _ctx_babl_inits --;
  if (_ctx_babl_inits == 0)
  {
    babl_exit ();
  }
#endif
}

static int event_check_pending (CtxTiled *tiled)
{
  int events = 0;
  for (int i = 0; i < tiled->evsource_count; i++)
  {
    while (evsource_has_event (tiled->evsource[i]))
    {
      char *event = evsource_get_event (tiled->evsource[i]);
      if (event)
      {
        if (tiled->vt_active)
        {
          ctx_key_press (tiled->backend.ctx, 0, event, 0); // we deliver all events as key-press, the key_press handler disambiguates
          events++;
        }
        free (event);
      }
    }
  }
  return events;
}

#endif
