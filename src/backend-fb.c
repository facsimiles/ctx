#include "ctx-split.h"

#if CTX_EVENTS

#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#if CTX_FB
  #include <linux/fb.h>
  #include <linux/vt.h>
  #include <linux/kd.h>
  #include <sys/mman.h>
  #include <threads.h>
  #include <libdrm/drm.h>
  #include <libdrm/drm_mode.h>

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
   void (*reset)  (void *fb);
   void (*flush)  (void *fb);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)   (void *fb);
   Ctx          *ctx;
   Ctx          *ctx_copy;
   int           width;
   int           height;
   int           cols; // unused
   int           rows; // unused
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
   int           min_col; // hasher cols and rows
   int           min_row;
   int           max_col;
   int           max_row;
   uint8_t       hashes[CTX_HASH_ROWS * CTX_HASH_COLS *  20];
   int8_t        tile_affinity[CTX_HASH_ROWS * CTX_HASH_COLS]; // which render thread no is
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
   int          is_drm;
   cnd_t        cond;
   mtx_t        mtx;
   struct drm_mode_crtc crtc;
};

static char *ctx_fb_clipboard = NULL;
static void ctx_fb_set_clipboard (CtxFb *fb, const char *text)
{
  if (ctx_fb_clipboard)
    free (ctx_fb_clipboard);
  ctx_fb_clipboard = NULL;
  if (text)
  {
    ctx_fb_clipboard = strdup (text);
  }
}

static char *ctx_fb_get_clipboard (CtxFb *sdl)
{
  if (ctx_fb_clipboard) return strdup (ctx_fb_clipboard);
  return strdup ("");
}

#if UINTPTR_MAX == 0xffFFffFF
  #define fbdrmuint_t uint32_t
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
  #define fbdrmuint_t uint64_t
#endif

void *ctx_fbdrm_new (CtxFb *fb, int *width, int *height)
{
   int got_master = 0;
   fb->fb_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
   if (!fb->fb_fd)
     return NULL;
   static fbdrmuint_t res_conn_buf[20]={0}; // this is static since its contents
                                         // are used by the flip callback
   fbdrmuint_t res_fb_buf[20]={0};
   fbdrmuint_t res_crtc_buf[20]={0};
   fbdrmuint_t res_enc_buf[20]={0};
   struct   drm_mode_card_res res={0};

   if (ioctl(fb->fb_fd, DRM_IOCTL_SET_MASTER, 0))
     goto cleanup;
   got_master = 1;

   if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
     goto cleanup;
   res.fb_id_ptr=(fbdrmuint_t)res_fb_buf;
   res.crtc_id_ptr=(fbdrmuint_t)res_crtc_buf;
   res.connector_id_ptr=(fbdrmuint_t)res_conn_buf;
   res.encoder_id_ptr=(fbdrmuint_t)res_enc_buf;
   if(ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
      goto cleanup;


   unsigned int i;
   for (i=0;i<res.count_connectors;i++)
   {
     struct drm_mode_modeinfo conn_mode_buf[20]={0};
     fbdrmuint_t conn_prop_buf[20]={0},
                     conn_propval_buf[20]={0},
                     conn_enc_buf[20]={0};

     struct drm_mode_get_connector conn={0};

     conn.connector_id=res_conn_buf[i];

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     conn.modes_ptr=(fbdrmuint_t)conn_mode_buf;
     conn.props_ptr=(fbdrmuint_t)conn_prop_buf;
     conn.prop_values_ptr=(fbdrmuint_t)conn_propval_buf;
     conn.encoders_ptr=(fbdrmuint_t)conn_enc_buf;

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     //Check if the connector is OK to use (connected to something)
     if (conn.count_encoders<1 || conn.count_modes<1 || !conn.encoder_id || !conn.connection)
       continue;

//------------------------------------------------------------------------------
//Creating a dumb buffer
//------------------------------------------------------------------------------
     struct drm_mode_create_dumb create_dumb={0};
     struct drm_mode_map_dumb    map_dumb={0};
     struct drm_mode_fb_cmd      cmd_dumb={0};
     create_dumb.width  = conn_mode_buf[0].hdisplay;
     create_dumb.height = conn_mode_buf[0].vdisplay;
     create_dumb.bpp   = 32;
     create_dumb.flags = 0;
     create_dumb.pitch = 0;
     create_dumb.size  = 0;
     create_dumb.handle = 0;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) ||
         !create_dumb.handle)
       goto cleanup;

     cmd_dumb.width =create_dumb.width;
     cmd_dumb.height=create_dumb.height;
     cmd_dumb.bpp   =create_dumb.bpp;
     cmd_dumb.pitch =create_dumb.pitch;
     cmd_dumb.depth =24;
     cmd_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb))
       goto cleanup;

     map_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb))
       goto cleanup;

     void *base = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fb->fb_fd, map_dumb.offset);
     if (!base)
     {
       goto cleanup;
     }
     *width  = create_dumb.width;
     *height = create_dumb.height;

     struct drm_mode_get_encoder enc={0};
     enc.encoder_id=conn.encoder_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETENCODER, &enc))
        goto cleanup;

     fb->crtc.crtc_id=enc.crtc_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCRTC, &fb->crtc))
        goto cleanup;

     fb->crtc.fb_id=cmd_dumb.fb_id;
     fb->crtc.set_connectors_ptr=(fbdrmuint_t)&res_conn_buf[i];
     fb->crtc.count_connectors=1;
     fb->crtc.mode=conn_mode_buf[0];
     fb->crtc.mode_valid=1;
     return base;
   }
cleanup:
   if (got_master)
     ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
   fb->fb_fd = 0;
   return NULL;
}

void ctx_fbdrm_flip (CtxFb *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_MODE_SETCRTC, &fb->crtc);
}

void ctx_fbdrm_close (CtxFb *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
  close (fb->fb_fd);
  fb->fb_fd = 0;
}

static void ctx_fb_flip (CtxFb *fb)
{
  if (fb->is_drm)
    ctx_fbdrm_flip (fb);
  else
    ioctl (fb->fb_fd, FBIOPAN_DISPLAY, &fb->vinfo);
}

static inline int
fb_render_threads_done (CtxFb *fb)
{
  int sum = 0;
  for (int i = 0; i < _ctx_max_threads; i++)
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

static int       fb_cursor_drawn   = 0;
static int       fb_cursor_drawn_x = 0;
static int       fb_cursor_drawn_y = 0;
static CtxCursor fb_cursor_drawn_shape = CTX_CURSOR_ARROW;


#define CTX_FB_HIDE_CURSOR_FRAMES 200

static int fb_cursor_same_pos = CTX_FB_HIDE_CURSOR_FRAMES;

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
          //cos_theta = -0.707106781186548;
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

static void ctx_fb_undraw_cursor (CtxFb *fb)
  {
    int cursor_size = ctx_height (fb->ctx) / 28;

    if (fb_cursor_drawn)
    {
      int no = 0;
      for (int y = -cursor_size; y < cursor_size; y++)
      for (int x = -cursor_size; x < cursor_size; x++, no+=4)
      {
        if (x + fb_cursor_drawn_x < fb->width && y + fb_cursor_drawn_y < fb->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, fb_cursor_drawn_shape))
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
    int cursor_size = ctx_height (fb->ctx) / 28;
    CtxCursor cursor_shape = fb->ctx->cursor;
    int no = 0;

    if (cursor_x == fb_cursor_drawn_x &&
        cursor_y == fb_cursor_drawn_y &&
        cursor_shape == fb_cursor_drawn_shape)
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
    for (int y = -cursor_size; y < cursor_size; y++)
      for (int x = -cursor_size; x < cursor_size; x++, no+=4)
      {
        if (x + cursor_x < fb->width && y + cursor_y < fb->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, cursor_shape))
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
    fb_cursor_drawn_shape = cursor_shape;
  }

static void ctx_fb_show_frame (CtxFb *fb, int block)
{
  if (fb->shown_frame == fb->render_frame)
  {
    if (block == 0) // consume event call
    {
      ctx_fb_draw_cursor (fb);
      ctx_fb_flip (fb);
    }
    return;
  }

  if (block)
  {
    int count = 0;
    while (fb_render_threads_done (fb) != _ctx_max_threads)
    {
      usleep (500);
      count ++;
      if (count > 2000)
      {
        fb->shown_frame = fb->render_frame;
        return;
      }
    }
  }
  else
  {
    if (fb_render_threads_done (fb) != _ctx_max_threads)
      return;
  }

    if (fb->vt_active)
    {
       int pre_skip = fb->min_row * fb->height/CTX_HASH_ROWS * fb->width;
       int post_skip = (CTX_HASH_ROWS-fb->max_row-1) * fb->height/CTX_HASH_ROWS * fb->width;

       int rows = ((fb->width * fb->height) - pre_skip - post_skip)/fb->width;

       int col_pre_skip = fb->min_col * fb->width/CTX_HASH_COLS;
       int col_post_skip = (CTX_HASH_COLS-fb->max_col-1) * fb->width/CTX_HASH_COLS;
#if CTX_DAMAGE_CONTROL
       pre_skip = post_skip = col_pre_skip = col_post_skip = 0;
#endif

       if (pre_skip < 0) pre_skip = 0;
       if (post_skip < 0) post_skip = 0;

     __u32 dummy = 0;

       if (fb->min_row == 100){
          pre_skip = 0;
          post_skip = 0;
          // not when drm ?
          ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
          ctx_fb_undraw_cursor (fb);
       }
       else
       {

      fb->min_row = 100;
      fb->max_row = 0;
      fb->min_col = 100;
      fb->max_col = 0;

     // not when drm ?
     ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
     ctx_fb_undraw_cursor (fb);
     switch (fb->fb_bits)
     {
       case 32:
#if 1
         {
           uint8_t *dst = fb->fb + pre_skip * 4;
           uint8_t *src = fb->scratch_fb + pre_skip * 4;
           int pre = col_pre_skip * 4;
           int post = col_post_skip * 4;
           int core = fb->width * 4 - pre - post;
           for (int i = 0; i < rows; i++)
           {
             dst  += pre;
             src  += pre;
             memcpy (dst, src, core);
             src  += core;
             dst  += core;
             dst  += post;
             src  += post;
           }
         }
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
         break;
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
    }
    fb_cursor_drawn = 0;
    ctx_fb_draw_cursor (fb);
    ctx_fb_flip (fb);
    fb->shown_frame = fb->render_frame;
  }
}


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
    fprintf (stderr, "error opening /dev/input/mice device, maybe add user to input group if such group exist, or otherwise make the rights be satisfied.\n");
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
          ctx_key_press (fb->ctx, 0, event, 0); // we deliver all events as key-press, it disamibuates
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
  ctx_fb_show_frame (fb, 0);
  event_check_pending (fb);
  return 0;
}

inline static void ctx_fb_reset (CtxFb *fb)
{
  ctx_fb_show_frame (fb, 1);
}

inline static void ctx_fb_flush (CtxFb *fb)
{
  if (fb->shown_frame == fb->render_frame)
  {
    int dirty_tiles = 0;
    ctx_set_renderstream (fb->ctx_copy, &fb->ctx->renderstream.entries[0],
                                         fb->ctx->renderstream.count * 9);
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
      free (((CtxHasher*)(hasher->renderer))->hashes);
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
          fb->tile_affinity[row * CTX_HASH_COLS + col] = dirty_no * (_ctx_max_threads) / dirty_tiles;
          dirty_no++;
          if (col > fb->max_col) fb->max_col = col;
          if (col < fb->min_col) fb->min_col = col;
          if (row > fb->max_row) fb->max_row = row;
          if (row < fb->min_row) fb->min_row = row;
        }
      }

#if CTX_DAMAGE_CONTROL
    for (int i = 0; i < fb->width * fb->height; i++)
    {
      int new = (fb->scratch_fb[i*4+0]+ fb->scratch_fb[i*4+1]+ fb->scratch_fb[i*4+2])/3;
      if (new>1) new--;
      fb->scratch_fb[i*4]= (fb->scratch_fb[i*4] + new)/2;
      fb->scratch_fb[i*4+1]= (fb->scratch_fb[i*4+1] + new)/2;
      fb->scratch_fb[i*4+2]= (fb->scratch_fb[i*4+1] + new)/2;
    }
#endif


    fb->render_frame = ++fb->frame;

    mtx_lock (&fb->mtx);
    cnd_broadcast (&fb->cond);
    mtx_unlock (&fb->mtx);
  }
}

void ctx_fb_free (CtxFb *fb)
{
  mtx_lock (&fb->mtx);
  cnd_broadcast (&fb->cond);
  mtx_unlock (&fb->mtx);

  memset (fb->fb, 0, fb->width * fb->height *  4);
  for (int i = 0 ; i < _ctx_max_threads; i++)
    ctx_free (fb->host[i]);

  if (fb->is_drm)
  {
    ctx_fbdrm_close (fb);
  }

  ioctl (0, KDSETMODE, KD_TEXT);
  system("stty sane");
  free (fb->scratch_fb);
  //free (fb);
}

static unsigned char *fb_icc = NULL;
static long fb_icc_length = 0;

static
void fb_render_fun (void **data)
{
  int      no = (size_t)data[0];
  CtxFb *fb = data[1];

  while (!fb->quit)
  {
    mtx_lock (&fb->mtx);
    cnd_wait (&fb->cond, &fb->mtx);
    mtx_unlock (&fb->mtx);

    if (fb->render_frame != fb->rendered_frame[no])
    {
      int hno = 0;

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
      /* merge horizontally adjecant tiles of same affinity into one job
       * this reduces redundant overhead and gets better cache behavior
       *
       * giving different threads more explicitly different rows
       * could be a good idea.
       */
            while (col + 1 < CTX_HASH_COLS &&
                   fb->tile_affinity[hno+1] == no)
            {
              width += fb->width / CTX_HASH_COLS;
              col++;
              hno++;
            }
            ctx_rasterizer_init (rasterizer,
                                 host, NULL, &host->state,
                                 &fb->scratch_fb[fb->width * 4 * y0 + x0 * 4],
                                 0, 0, width, height,
                                 fb->width*4, CTX_FORMAT_RGBA8,
                                 fb->antialias);
                                              /* this is the format used */
            if (fb->fb_bits == 32)
              rasterizer->swap_red_green = 1; 
            if (fb_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, fb_icc, fb_icc_length);
            ((CtxRasterizer*)host->renderer)->texture_source = fb->ctx;
            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (fb->ctx_copy, host);
          }
        }
      fb->rendered_frame[no] = fb->render_frame;

      if (fb_render_threads_done (fb) == _ctx_max_threads)
      {
   //   ctx_render_stream (fb->ctx_copy, stdout, 1);
   //   ctx_reset (fb->ctx_copy);
      }
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
    if (ctx_fb->is_drm)
      ioctl(ctx_fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
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
    if (ctx_fb->is_drm)
    {
      ioctl(ctx_fb->fb_fd, DRM_IOCTL_SET_MASTER, 0);
      ctx_fb_flip (ctx_fb);
    }
    else
    {
      ctx_fb->ctx->dirty=1;

      for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        ctx_fb->hashes[(row * CTX_HASH_COLS + col) *  20] += 1;
      }
    }

  }
}

Ctx *ctx_new_fb (int width, int height, int drm)
{
#if CTX_RASTERIZER
  CtxFb *fb = calloc (sizeof (CtxFb), 1);

  ctx_fb = fb;
  if (drm)
    fb->fb = ctx_fbdrm_new (fb, &fb->width, &fb->height);
  if (fb->fb)
  {
    fb->is_drm         = 1;
    width              = fb->width;
    height             = fb->height;
    fb->fb_mapped_size = fb->width * fb->height * 4;
    fb->fb_bits        = 32;
    fb->fb_bpp         = 4;
  }
  else
  {
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
  }
  if (!fb->fb)
    return NULL;
  fb->scratch_fb = calloc (fb->fb_mapped_size, 1);
  ctx_fb_events = 1;

  fb->ctx      = ctx_new ();
  fb->ctx_copy = ctx_new ();
  fb->width    = width;
  fb->height   = height;

  ctx_set_renderer (fb->ctx, fb);
  ctx_set_renderer (fb->ctx_copy, fb);

  ctx_set_size (fb->ctx, width, height);
  ctx_set_size (fb->ctx_copy, width, height);
  fb->flush = (void*)ctx_fb_flush;
  fb->reset = (void*)ctx_fb_reset;
  fb->free  = (void*)ctx_fb_free;
  fb->set_clipboard = (void*)ctx_fb_set_clipboard;
  fb->get_clipboard = (void*)ctx_fb_get_clipboard;

  for (int i = 0; i < _ctx_max_threads; i++)
  {
    fb->host[i] = ctx_new_for_framebuffer (fb->scratch_fb,
                   fb->width/CTX_HASH_COLS, fb->height/CTX_HASH_ROWS,
                   fb->width * 4, CTX_FORMAT_RGBA8); // this format
                                  // is overriden in  thread
    ((CtxRasterizer*)fb->host[i]->renderer)->texture_source = fb->ctx;
  }

  _ctx_file_get_contents ("/tmp/ctx.icc", &fb_icc, &fb_icc_length);

  mtx_init (&fb->mtx, mtx_plain);
  cnd_init (&fb->cond);

#define start_thread(no)\
  if(_ctx_max_threads>no){ \
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

  ctx_flush (fb->ctx);

  EvSource *kb = evsource_kb_new ();
  if (kb)
  {
    fb->evsource[fb->evsource_count++] = kb;
    kb->priv = fb;
  }
  EvSource *mice  = evsource_mice_new ();
  if (mice)
  {
    fb->evsource[fb->evsource_count++] = mice;
    mice->priv = fb;
  }

  fb->vt_active = 1;
  ioctl(0, KDSETMODE, KD_GRAPHICS);
  signal (SIGUSR1, vt_switch_cb);
  signal (SIGUSR2, vt_switch_cb);
  struct vt_stat st;
  if (ioctl (0, VT_GETSTATE, &st) == -1)
  {
    ctx_log ("VT_GET_MODE on vt %i failed\n", fb->vt);
    return NULL;
  }

  fb->vt = st.v_active;

  struct vt_mode mode;
  mode.mode   = VT_PROCESS;
  mode.relsig = SIGUSR1;
  mode.acqsig = SIGUSR2;
  if (ioctl (0, VT_SETMODE, &mode) < 0)
  {
    ctx_log ("VT_SET_MODE on vt %i failed\n", fb->vt);
    return NULL;
  }

  return fb->ctx;
#else
  return NULL;
#endif
}
#else

int ctx_renderer_is_fb (Ctx *ctx)
{
  return 0;
}
#endif
#endif
