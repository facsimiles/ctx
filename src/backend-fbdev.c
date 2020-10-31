#include "ctx-split.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#if CTX_FB
  #include <linux/fb.h>
  #include <linux/vt.h>
  #include <linux/kd.h>
  #include <sys/mman.h>
#include <threads.h>

 // 1 threads 13fps
 // 2 threads 20fps
 // 3 threads 27fps
 // 4 threads 29fps

typedef struct _EvSource EvSource;

struct _EvSource
{
  void   *priv; /* private storage  */

  /* returns non 0 if there is events waiting */
  int   (*has_event) (EvSource *ev_source);

  /* get an event, the returned event should be freed by the caller  */
  char *(*get_event) (EvSource *ev_source);

  /* destroy/unref this instance */
  void  (*destroy)   (EvSource *ev_source);

  /* get the underlying fd, useful for using select on  */
  int   (*get_fd)    (EvSource *ev_source);


  void  (*set_coord) (EvSource *ev_source, double x, double y);
  /* set_coord is needed to warp relative cursors into normalized range,
   * like normal mice/trackpads/nipples - to obey edges and more.
   */

  /* if this returns non-0 select can be used for non-blocking.. */
};


typedef struct _CtxFb CtxFb;
struct _CtxFb
{
   void (*render) (void *fb, CtxCommand *command);
   void (*flush)  (void *fb);
   void (*free)   (void *fb);
   Ctx          *ctx;
   Ctx          *ctx_copy;
   int           width;
   int           height;
   int           cols;
   int           rows;
   int           was_down;
   Ctx          *host[CTX_MAX_THREADS];
   CtxAntialias  antialias;
   uint8_t      *scratch_fb;
   int           quit;
   _Atomic int   thread_quit;
   int           shown_frame;
   int           render_frame;
   int           rendered_frame[CTX_MAX_THREADS];
   int           frame;
   int       min_col; // hasher cols and rows
   int       min_row;
   int       max_col;
   int       max_row;
   uint8_t  hashes[CTX_HASH_ROWS * CTX_HASH_COLS *  20];
   int8_t    tile_affinity[CTX_HASH_ROWS * CTX_HASH_COLS]; // which render thread no is
                                                           // responsible for a tile
                                                           //


   int           pointer_down[3];
   int           key_balance;
   int           key_repeat;
   int           lctrl;
   int           lalt;
   int           rctrl;

   uint8_t      *fb;

   int          fb_fd;
   char        *fb_path;
   int          fb_bits;
   int          fb_bpp;
   int          fb_mapped_size;
   struct       fb_var_screeninfo vinfo;
   struct       fb_fix_screeninfo finfo;
   int          vt;
   int          tty;
   int          vt_active;
   EvSource    *evsource[4];
   int          evsource_count;
};

static inline int
fb_render_threads_done (CtxFb *fb)
{
  int sum = 0;
  for (int i = 0; i < _ctx_threads; i++)
  {
     if (fb->rendered_frame[i] == fb->render_frame)
       sum ++;
  }
  return sum;
}

inline static uint32_t
ctx_swap_red_green2 (uint32_t orig)
{
  uint32_t  green_alpha = (orig & 0xff00ff00);
  uint32_t  red_blue    = (orig & 0x00ff00ff);
  uint32_t  red         = red_blue << 16;
  uint32_t  blue        = red_blue >> 16;
  return green_alpha | red | blue;
}

static int fb_cursor_drawn = 0;
static int fb_cursor_drawn_x = 0;
static int fb_cursor_drawn_y = 0;

#define CTX_FB_HIDE_CURSOR_FRAMES 200

static int fb_cursor_same_pos = CTX_FB_HIDE_CURSOR_FRAMES;
#define CURSOR_SIZE 64

static inline int ctx_is_in_cursor (int x, int y)
{
  if (x < y && x > y / 16)
  {
    return 1;
  }
  return 0;
}

static void ctx_fb_undraw_cursor (CtxFb *fb)
  {
    int cursor_size = ctx_height (fb->ctx) / 20;

    if (fb_cursor_drawn)
    {
      int no = 0;
      for (int y = 0; y < cursor_size; y++)
      for (int x = 0; x < cursor_size; x++, no+=4)
      {
        if (x + fb_cursor_drawn_x < fb->width && y + fb_cursor_drawn_y < fb->height)
        {
          if (ctx_is_in_cursor (x, y))
          {
            int o = ((fb_cursor_drawn_y + y) * fb->width + (fb_cursor_drawn_x + x)) * 4;
            fb->fb[o+0]^=0x88;
            fb->fb[o+1]^=0x88;
            fb->fb[o+2]^=0x88;
          }
        }
      }
    fb_cursor_drawn = 0;
    }
}

static void ctx_fb_draw_cursor (CtxFb *fb)
  {
    int cursor_x    = ctx_pointer_x (fb->ctx);
    int cursor_y    = ctx_pointer_y (fb->ctx);
    int cursor_size = ctx_height (fb->ctx) / 20;
    int no = 0;

    if (cursor_x == fb_cursor_drawn_x &&
        cursor_y == fb_cursor_drawn_y)
      fb_cursor_same_pos ++;
    else
      fb_cursor_same_pos = 0;


    if (fb_cursor_same_pos >= CTX_FB_HIDE_CURSOR_FRAMES)
    {
      if (fb_cursor_drawn)
        ctx_fb_undraw_cursor (fb);
      return;
    }

    /* no need to flicker when stationary, motion flicker can also be removed
     * by combining the previous and next position masks when a motion has
     * occured..
     */
    if (fb_cursor_same_pos && fb_cursor_drawn)
      return;

    ctx_fb_undraw_cursor (fb);

    no = 0;
    for (int y = 0; y < cursor_size; y++)
      for (int x = 0; x < cursor_size; x++, no+=4)
      {
        if (x + cursor_x < fb->width && y + cursor_y < fb->height)
        {
          if (ctx_is_in_cursor (x, y))
          {
            int o = ((cursor_y + y) * fb->width + (cursor_x + x)) * 4;
            fb->fb[o+0]^=0x88;
            fb->fb[o+1]^=0x88;
            fb->fb[o+2]^=0x88;
          }
        }
      }
    fb_cursor_drawn = 1;
    fb_cursor_drawn_x = cursor_x;
    fb_cursor_drawn_y = cursor_y;
  }

static void ctx_fb_show_frame (CtxFb *fb)
{
  if (fb->shown_frame != fb->render_frame &&
      fb_render_threads_done (fb) == _ctx_threads)
  {
    if (fb->vt_active)
    {
       int pre_skip = fb->min_row * fb->height/CTX_HASH_ROWS * fb->width;
       int post_skip = (CTX_HASH_ROWS-fb->max_row-1) * fb->height/CTX_HASH_ROWS * fb->width;

      fb->min_row = 100;
      fb->max_row = 0;
      fb->min_col = 100;
      fb->max_col = 0;

     ctx_fb_undraw_cursor (fb);

     __u32 dummy = 0;
     ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
     fprintf (stderr, "\e[H\e[J");
     switch (fb->fb_bits)
     {
       case 32:
#if 1
         memcpy (fb->fb, fb->scratch_fb, fb->width * fb->height *  4);
#else
         { int count = fb->width * fb->height;
           const uint32_t *src = (void*)fb->scratch_fb;
           uint32_t *dst = (void*)fb->fb;
           count-= pre_skip;
           src+= pre_skip;
           dst+= pre_skip;
           count-= post_skip;
           while (count -- > 0)
           {
             dst[0] = ctx_swap_red_green2 (src[0]);
             src++;
             dst++;
           }
         }
#endif
         break;
         /* XXX  :  note: converting a scanline (or all) to target and
          * then doing a bulk memcpy be faster (at least with som /dev/fbs)  */
       case 24:
         { int count = fb->width * fb->height;
           const uint8_t *src = fb->scratch_fb;
           uint8_t *dst = fb->fb;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 3;
           count-= post_skip;
           while (count -- > 0)
           {
             dst[0] = src[0];
             dst[1] = src[1];
             dst[2] = src[2];
             dst+=3;
             src+=4;
           }
         }
         break;
       case 16:
         { int count = fb->width * fb->height;
           const uint8_t *src = fb->scratch_fb;
           uint8_t *dst = fb->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 2;
           while (count -- > 0)
           {
             int big = ((src[0] >> 3)) +
                ((src[1] >> 2)<<5) +
                ((src[2] >> 3)<<11);
             dst[0] = big & 255;
             dst[1] = big >>  8;
             dst+=2;
             src+=4;
           }
         }
         break;
       case 15:
         { int count = fb->width * fb->height;
           const uint8_t *src = fb->scratch_fb;
           uint8_t *dst = fb->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 2;
           while (count -- > 0)
           {
             int big = ((src[2] >> 3)) +
                       ((src[1] >> 2)<<5) +
                       ((src[0] >> 3)<<10);
             dst[0] = big & 255;
             dst[1] = big >>  8;
             dst+=2;
             src+=4;
           }
         }
       case 8:
         { int count = fb->width * fb->height;
           const uint8_t *src = fb->scratch_fb;
           uint8_t *dst = fb->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip;
           while (count -- > 0)
           {
             dst[0] = ((src[0] >> 5)) +
                      ((src[1] >> 5)<<3) +
                      ((src[2] >> 6)<<6);
             dst+=1;
             src+=4;
           }
         }
         break;
     }
    fb_cursor_drawn = 0;
    ctx_fb_draw_cursor (fb);
    ioctl (fb->fb_fd, FBIOPAN_DISPLAY, &fb->vinfo);
    fb->shown_frame = fb->render_frame;
    }
  }
  else
  {
    if (fb->vt_active)
    {
      ctx_fb_draw_cursor (fb);
    }
  }
}


#define evsource_has_event(es)   (es)->has_event((es))
#define evsource_get_event(es)   (es)->get_event((es))
#define evsource_destroy(es)     do{if((es)->destroy)(es)->destroy((es));}while(0)
#define evsource_set_coord(es,x,y) do{if((es)->set_coord)(es)->set_coord((es),(x),(y));}while(0)
#define evsource_get_fd(es)      ((es)->get_fd?(es)->get_fd((es)):0)

#ifndef CTX_MIN
#define CTX_MIN(a,b)  (((a)<(b))?(a):(b))
#endif
#ifndef CTX_MAX
#define CTX_MAX(a,b)  (((a)>(b))?(a):(b))
#endif


static int mice_has_event ();
static char *mice_get_event ();
static void mice_destroy ();
static int mice_get_fd (EvSource *ev_source);
static void mice_set_coord (EvSource *ev_source, double x, double y);

static EvSource ev_src_mice = {
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

void _mmm_get_coords (Ctx *ctx, double *x, double *y)
{
  if (!_mrg_evsrc_coord)
    return;
  if (x)
    *x = _mrg_evsrc_coord->x;
  if (y)
    *y = _mrg_evsrc_coord->y;
}

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
    fprintf (stderr, "error opening /dev/input/mice device, maybe add to input group if such group exist, or otherwise make the rights be satisfied.\n");
//  sleep (1);
    return -1;
  }
  write (mrg_mice_this->fd, reset, 1);
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
  const char *ret = "mouse-motion";
  double relx, rely;
  signed char buf[3];
  CtxFb *fb = ev_src_mice.priv;
  read (mrg_mice_this->fd, buf, 3);
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
  if (mrg_mice_this->x >= fb->width)
    mrg_mice_this->x = fb->width -1;
  if (mrg_mice_this->y >= fb->height)
    mrg_mice_this->y = fb->height -1;
  int button = 0;
  
  if ((mrg_mice_this->prev_state & 1) != (buf[0] & 1))
    {
      if (buf[0] & 1)
        {
          ret = "mouse-press";
        }
      else
        {
          ret = "mouse-release";
        }
      button = 1;
    }
  else if (buf[0] & 1)
  {
    ret = "mouse-drag";
    button = 1;
  }

  if (!button)
  {
    if ((mrg_mice_this->prev_state & 2) != (buf[0] & 2))
    {
      if (buf[0] & 2)
        {
          ret = "mouse-press";
        }
      else
        {
          ret = "mouse-release";
        }
      button = 3;
    }
    else if (buf[0] & 2)
    {
      ret = "mouse-drag";
      button = 3;
    }
  }

  if (!button)
  {
    if ((mrg_mice_this->prev_state & 4) != (buf[0] & 4))
    {
      if (buf[0] & 4)
        {
          ret = "mouse-press";
        }
      else
        {
          ret = "mouse-release";
        }
      button = 2;
    }
    else if (buf[0] & 4)
    {
      ret = "mouse-drag";
      button = 2;
    }
  }


  mrg_mice_this->prev_state = buf[0];

  //if (!is_active (ev_src_mice.priv))
  //  return NULL;

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

EvSource *evsource_mice_new (void)
{
  if (mmm_evsource_mice_init () == 0)
    {
      mrg_mice_this->x = 0;
      mrg_mice_this->y = 0;
      return &ev_src_mice;
    }
  return NULL;
}



static int evsource_kb_has_event (void);
static char *evsource_kb_get_event (void);
static void evsource_kb_destroy (int sign);
static int evsource_kb_get_fd (void);

/* kept out of struct to be reachable by atexit */
static EvSource ev_src_kb = {
  NULL,
  (void*)evsource_kb_has_event,
  (void*)evsource_kb_get_event,
  (void*)evsource_kb_destroy,
  (void*)evsource_kb_get_fd,
  NULL
};

static struct termios orig_attr;

int is_active (void *host)
{
  return 1;
}
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
    case   SIGSEGV: fprintf (stderr, " SIGSEGV\n");break;
    case   SIGABRT: fprintf (stderr, " SIGABRT\n");break;
    case   SIGBUS: fprintf (stderr, " SIGBUS\n");break;
    case   SIGKILL: fprintf (stderr, " SIGKILL\n");break;
    case   SIGINT: fprintf (stderr, " SIGINT\n");break;
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
  atexit ((void*) real_evsource_kb_destroy);
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

  {"F1",        "\e[11~"},
  {"F2",        "\e[12~"},
  {"F3",        "\e[13~"},
  {"F4",        "\e[14~"},
  {"F1",        "\eOP"},
  {"F2",        "\eOQ"},
  {"F3",        "\eOR"},
  {"F4",        "\eOS"},
  {"F5",        "\e[15~"},
  {"F6",        "\e[16~"},
  {"F7",        "\e[17~"},
  {"F8",        "\e[18~"},
  {"F9",        "\e[19~"},
  {"F9",        "\e[20~"},
  {"F10",       "\e[21~"},
  {"F11",       "\e[22~"},
  {"F12",       "\e[23~"},
  {"tab",      {9, '\0'}},
  {"shift-tab",  "\e[Z"},
  {"backspace",{127, '\0'}},
  {"space",     " "},
  {"\e",       "\e"},
  {"return",   {10,0}},
  {"return",   {13,0}},
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
 {"control-k",  {11,0}},
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
{"alt-k",     "\ek"},
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

static char *evsource_kb_get_event (void)
{
  unsigned char buf[20];
  int length;


  for (length = 0; length < 10; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
        const MmmKeyCode *match = NULL;

        if (!is_active (ev_src_kb.priv))
           return NULL;

        /* special case ESC, so that we can use it alone in keybindings */
        if (length == 0 && buf[0] == 27)
          {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO (&rfds);
            FD_SET (STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * 120;
            if (select (1, &rfds, NULL, NULL, &tv) == 0)
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
                static char ret[256];
                if (length == 0 && ctx_utf8_len (buf[0])>1) /* read a
                                                             * single unicode
                                                             * utf8 character
                                                             */
                  {
                    read (STDIN_FILENO, &buf[length+1], ctx_utf8_len(buf[0])-1);
                    buf[ctx_utf8_len(buf[0])]=0;
                    strcpy (ret, (void*)buf);
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


EvSource *evsource_kb_new (void)
{
  if (evsource_kb_init() == 0)
  {
    return &ev_src_kb;
  }
  return NULL;
}

static int event_check_pending (CtxFb *fb)
{
  int events = 0;
  for (int i = 0; i < fb->evsource_count; i++)
  {
    while (evsource_has_event (fb->evsource[i]))
    {
      char *event = evsource_get_event (fb->evsource[i]);
      if (event)
      {
        if (fb->vt_active)
        {
          //fprintf (stderr, "ev %s\n", event);
          ctx_key_press (fb->ctx, 0, event, 0);
          events++;
        }
        free (event);
      }
    }
  }
  return events;
}

int ctx_fb_consume_events (Ctx *ctx)
{
  CtxFb *fb = (void*)ctx->renderer;
  ctx_fb_show_frame (fb);
  event_check_pending (fb);
  return 0;
}

inline static void ctx_fb_flush (CtxFb *fb)
{
  //int width =  sdl->width;
  int count = 0;
  while (fb->shown_frame != fb->render_frame && count < 10000)
  {
    usleep (100);
    ctx_fb_show_frame (fb);
    count++;
  }
  if (count >= 10000)
  {
    fb->shown_frame = fb->render_frame;
  }
  if (fb->shown_frame == fb->render_frame)
  {
    ctx_reset (fb->ctx_copy);
    ctx_set_renderstream (fb->ctx_copy, &fb->ctx->renderstream.entries[0],
                                         fb->ctx->renderstream.count * 9);
    int dirty_tiles = 0;
    if (_ctx_enable_hash_cache)
    {
    Ctx *hasher = ctx_hasher_new (fb->width, fb->height,
                      CTX_HASH_COLS, CTX_HASH_ROWS);
    ctx_render_ctx (fb->ctx_copy, hasher);

    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        uint8_t *new_hash = ctx_hasher_get_hash (hasher, col, row);
        if (new_hash && memcmp (new_hash, &fb->hashes[(row * CTX_HASH_COLS + col) *  20], 20))
        {
          memcpy (&fb->hashes[(row * CTX_HASH_COLS +  col)*20], new_hash, 20);
          fb->tile_affinity[row * CTX_HASH_COLS + col] = 1;
          dirty_tiles++;
        }
        else
        {
          fb->tile_affinity[row * CTX_HASH_COLS + col] = -1;
        }
      }
    ctx_free (hasher);
    }
    else
    {
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        fb->tile_affinity[row * CTX_HASH_COLS + col] = 1;
        dirty_tiles++;
      }
    }


    int dirty_no = 0;
    if (dirty_tiles)
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        if (fb->tile_affinity[row * CTX_HASH_COLS + col] != -1)
        {
          fb->tile_affinity[row * CTX_HASH_COLS + col] = dirty_no * (_ctx_threads) / dirty_tiles;
          dirty_no++;
          if (col > fb->max_col) fb->max_col = col;
          if (col < fb->min_col) fb->min_col = col;
          if (row > fb->max_row) fb->max_row = row;
          if (row < fb->min_row) fb->min_row = row;
        }
      }

    fb->render_frame = ++fb->frame;
  }
}

void ctx_fb_free (CtxFb *fb)
{

  memset (fb->fb, 0, fb->width * fb->height *  4);
  for (int i = 0 ; i < _ctx_threads; i++)
    ctx_free (fb->host[i]);
  system("stty sane");
  
  ioctl (0, KDSETMODE, KD_TEXT);

  free (fb->scratch_fb);
  //free (fb);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

static
void fb_render_fun (void **data)
{
  int      no = (size_t)data[0];
  CtxFb *fb = data[1];

  int sleep_time = 2000;
  while (!fb->quit)
  {
    if (fb->render_frame != fb->rendered_frame[no])
    {
      int hno = 0;
      sleep_time = 2000;
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++, hno++)
        {
          if (fb->tile_affinity[hno]==no)
          {
            int x0 = ((fb->width)/CTX_HASH_COLS) * col;
            int y0 = ((fb->height)/CTX_HASH_ROWS) * row;
            int width = fb->width / CTX_HASH_COLS;
            int height = fb->height / CTX_HASH_ROWS;

            Ctx *host = fb->host[no];
            CtxRasterizer *rasterizer = (CtxRasterizer*)host->renderer;
#if 1 // merge horizontally adjecant tiles of same affinity into one job
            while (col + 1 < CTX_HASH_COLS &&
                   fb->tile_affinity[hno+1] == no)
            {
              width += fb->width / CTX_HASH_COLS;
              col++;
              hno++;
            }
#endif

            ctx_rasterizer_init (rasterizer,
                                 host, NULL, &host->state,
                                 &fb->scratch_fb[fb->width * 4 * y0 + x0 * 4],
                                 0, 0, width, height,
                                 fb->width*4, CTX_FORMAT_RGBA8,
                                 fb->antialias);
                                              /* this is the format used */
            ((CtxRasterizer*)host->renderer)->texture_source = fb->ctx;
            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (fb->ctx_copy, host);
          }
        }
      fb->rendered_frame[no] = fb->render_frame;

      if (fb_render_threads_done (fb) == _ctx_threads)
      {
   //   ctx_render_stream (fb->ctx_copy, stdout, 1);
        ctx_reset (fb->ctx_copy);
      }
    }
    else
    {
      usleep (sleep_time);
      sleep_time *= 2;
      if (sleep_time > 1000000/8)
          sleep_time = 1000000/8;
    }
  }
  fb->thread_quit ++;
}


int ctx_renderer_is_fb (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_fb_free)
          return 1;
  return 0;
}

static CtxFb *ctx_fb = NULL;
static void vt_switch_cb (int sig)
{
  if (sig == SIGUSR1)
  {
    ioctl (0, VT_RELDISP, 1);
    ctx_fb->vt_active = 0;
    ioctl (0, KDSETMODE, KD_TEXT);
  }
  else
  {
    ioctl (0, VT_RELDISP, VT_ACKACQ);
    ctx_fb->vt_active = 1;
    // queue draw
    ctx_fb->render_frame = ++ctx_fb->frame;
    ioctl (0, KDSETMODE, KD_GRAPHICS);
  }
}

Ctx *ctx_new_fb (int width, int height)
{
#if CTX_RASTERIZER
  CtxFb *fb = calloc (sizeof (CtxFb), 1);
  fprintf (stderr, "\e[2J\e[H\e[?25l");
  fb->fb_fd = open ("/dev/fb0", O_RDWR);
  if (fb->fb_fd > 0)
    fb->fb_path = strdup ("/dev/fb0");
  else
  {
    fb->fb_fd = open ("/dev/graphics/fb0", O_RDWR);
    if (fb->fb_fd > 0)
    {
      fb->fb_path = strdup ("/dev/graphics/fb0");
    }
    else
    {
      free (fb);
      return NULL;
    }
  }

  if (ioctl(fb->fb_fd, FBIOGET_FSCREENINFO, &fb->finfo))
    {
      fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      free (fb->fb_path);
      free (fb);
      return NULL;
    }

   if (ioctl(fb->fb_fd, FBIOGET_VSCREENINFO, &fb->vinfo))
     {
       fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      free (fb->fb_path);
      free (fb);
      return NULL;
     }
  ctx_fb = fb;

//fprintf (stderr, "%s\n", fb->fb_path);
  width = fb->width = fb->vinfo.xres;
  height = fb->height = fb->vinfo.yres;

  fb->fb_bits = fb->vinfo.bits_per_pixel;
//fprintf (stderr, "fb bits: %i\n", fb->fb_bits);

  if (fb->fb_bits == 16)
    fb->fb_bits =
      fb->vinfo.red.length +
      fb->vinfo.green.length +
      fb->vinfo.blue.length;

   else if (fb->fb_bits == 8)
  {
    unsigned short red[256],  green[256],  blue[256];
    unsigned short original_red[256];
    unsigned short original_green[256];
    unsigned short original_blue[256];
    struct fb_cmap cmap = {0, 256, red, green, blue, NULL};
    struct fb_cmap original_cmap = {0, 256, original_red, original_green, original_blue, NULL};
    int i;

    /* do we really need to restore it ? */
    if (ioctl (fb->fb_fd, FBIOPUTCMAP, &original_cmap) == -1)
    {
      fprintf (stderr, "palette initialization problem %i\n", __LINE__);
    }

    for (i = 0; i < 256; i++)
    {
      red[i]   = ((( i >> 5) & 0x7) << 5) << 8;
      green[i] = ((( i >> 2) & 0x7) << 5) << 8;
      blue[i]  = ((( i >> 0) & 0x3) << 6) << 8;
    }

    if (ioctl (fb->fb_fd, FBIOPUTCMAP, &cmap) == -1)
    {
      fprintf (stderr, "palette initialization problem %i\n", __LINE__);
    }
  }

  fb->fb_bpp = fb->vinfo.bits_per_pixel / 8;
  fb->fb_mapped_size = fb->finfo.smem_len;
                                              
  fb->fb = mmap (NULL, fb->fb_mapped_size, PROT_READ|PROT_WRITE, MAP_SHARED, fb->fb_fd, 0);
  fb->scratch_fb = calloc (fb->fb_mapped_size, 1);
  ctx_fb_events = 1;

  fb->ctx = ctx_new ();
  fb->ctx_copy = ctx_new ();
  fb->width  = width;
  fb->height = height;
  fb->cols = 80;
  fb->rows = 20;

  ctx_set_renderer (fb->ctx, fb);
  ctx_set_renderer (fb->ctx_copy, fb);

  ctx_set_size (fb->ctx, width, height);
  ctx_set_size (fb->ctx_copy, width, height);
  fb->flush = (void*)ctx_fb_flush;
  fb->free  = (void*)ctx_fb_free;

  for (int i = 0; i < _ctx_threads; i++)
  {
    fb->host[i] = ctx_new_for_framebuffer (fb->scratch_fb,
                   fb->width/CTX_HASH_COLS, fb->height/CTX_HASH_ROWS,
                   fb->width * 4, CTX_FORMAT_RGBA8); // this format
                                  // is overriden in  thread
    ((CtxRasterizer*)fb->host[i]->renderer)->texture_source = fb->ctx;
  }

#define start_thread(no)\
  if(_ctx_threads>no){ \
    static void *args[2]={(void*)no, };\
    thrd_t tid;\
    args[1]=fb;\
    thrd_create (&tid, (void*)fb_render_fun, args);\
  }
  start_thread(0);
  start_thread(1);
  start_thread(2);
  start_thread(3);
  start_thread(4);
  start_thread(5);
  start_thread(6);
  start_thread(7);
  start_thread(8);
  start_thread(9);
  start_thread(10);
  start_thread(11);
  start_thread(12);
  start_thread(13);
  start_thread(14);
  start_thread(15);
#undef start_thread

#endif
  ctx_flush (fb->ctx);

  EvSource *kb = evsource_kb_new ();

  fb->evsource[fb->evsource_count++] = kb;
  kb->priv = fb;
  EvSource *mice  = evsource_mice_new ();
  fb->evsource[fb->evsource_count++] = mice;
  mice->priv = fb;

  fb->vt_active = 1;
//return fb->ctx;
  signal (SIGUSR1, vt_switch_cb);
  signal (SIGUSR2, vt_switch_cb);
  //return fb->ctx;

  struct vt_stat st;
  if (ioctl (0, VT_GETSTATE, &st) == -1)
    {
  //  return NULL;
    }
    ioctl(0, KDSETMODE, KD_GRAPHICS);

  fb->vt = st.v_active;

  struct vt_mode mode;
  mode.mode = VT_PROCESS;
  mode.relsig = SIGUSR1;
  mode.acqsig = SIGUSR2;
  if (ioctl (0, VT_SETMODE, &mode) < 0)
  {
    fprintf (stderr, "VT_SET_MODE on vt %i failed\n", fb->vt);
    exit (-1);
  }

  return fb->ctx;
}
#else

int ctx_renderer_is_fb (Ctx *ctx)
{
  return 0;
}
#endif
