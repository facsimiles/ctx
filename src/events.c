#include "ctx-split.h"

#if !__COSMOPOLITAN__
#include <sys/time.h>
#endif

#ifdef EMSCRIPTEN
#include "emscripten.h"
#endif

#define usecs(time)    ((uint64_t)(time.tv_sec - start_time.tv_sec) * 1000000 + time.     tv_usec)

#if !__COSMOPOLITAN__
static struct timeval start_time;

static void
_ctx_init_ticks (void)
{
  static int done = 0;
  if (done)
    return;
  done = 1;
  gettimeofday (&start_time, NULL);
}

static inline unsigned long
_ctx_ticks (void)
{
  struct timeval measure_time;
  gettimeofday (&measure_time, NULL);
  return usecs (measure_time) - usecs (start_time);
}

CTX_EXPORT unsigned long
ctx_ticks (void)
{
  _ctx_init_ticks ();
  return _ctx_ticks ();
}



enum _CtxFlags {
   CTX_FLAG_DIRECT = (1<<0),
};
typedef enum _CtxFlags CtxFlags;


int _ctx_max_threads = 1;
int _ctx_enable_hash_cache = 1;
#if CTX_SHAPE_CACHE
extern int _ctx_shape_cache_enabled;
#endif

#if CTX_THREADS
static mtx_t _ctx_texture_mtx;
#endif

void _ctx_texture_lock (void)
{
#if CTX_THREADS
  mtx_lock (&_ctx_texture_mtx);
#endif
}

void _ctx_texture_unlock (void)
{
#if CTX_THREADS
  mtx_unlock (&_ctx_texture_mtx);
#endif
}

void
ctx_init (int *argc, char ***argv)
{
#if 0
  const char *backend = getenv ("CTX_BACKEND");
  if (!backend || ctx_strcmp (backend, "ctx"))
  {
    int i;
    char *new_argv[*argc+5];
    new_argv[0] = "ctx";
    new_argv[1] = "-e";
    new_argv[2] = "--";
    for (i = 0; i < *argc; i++)
    {
      new_argv[i+3] = *argv[i];
    }
    new_argv[i+3] = NULL;
    execvp (new_argv[0], new_argv);
  }
#endif
}

#if 0
int ctx_count (Ctx *ctx)
{
  return ctx->drawlist.count;
}
#endif

extern int _ctx_damage_control;


#if CTX_EVENTS

void ctx_list_backends(void)
{
    fprintf (stderr, "possible values for CTX_BACKEND:\n");
    fprintf (stderr, " ctx");
#if CTX_SDL
    fprintf (stderr, " SDL");
#endif
#if CTX_KMS
    fprintf (stderr, " kms");
#endif
#if CTX_FB
    fprintf (stderr, " fb");
#endif
    fprintf (stderr, " term");
    fprintf (stderr, " termimg");
    fprintf (stderr, "\n");
}

static uint32_t ctx_ms (Ctx *ctx)
{
  return _ctx_ticks () / 1000;
}

static int is_in_ctx (void);

#if EMSCRIPTEN

CTX_EXPORT Ctx *
ctx_wasm_get_context (int flags);

static Ctx *ctx_new_ui (int width, int height, const char *backend)
{
   return ctx_wasm_get_context (CTX_CB_HASH_CACHE);
}
#else

static Ctx *ctx_new_ui (int width, int height, const char *backend)
{
#if CTX_TILED
  if (getenv ("CTX_DAMAGE_CONTROL"))
  {
    const char * val = getenv ("CTX_DAMAGE_CONTROL");
    if (!ctx_strcmp (val, "0") ||
        !ctx_strcmp (val, "off"))
      _ctx_damage_control = 0;
    else
      _ctx_damage_control = 1;
  }
#endif

  if (getenv ("CTX_HASH_CACHE"))
  {
    const char * val = getenv ("CTX_HASH_CACHE");
    if (!ctx_strcmp (val, "0"))
      _ctx_enable_hash_cache = 0;
    if (!ctx_strcmp (val, "off"))
      _ctx_enable_hash_cache = 0;
  }

  if (getenv ("CTX_THREADS"))
  {
    int val = atoi (getenv ("CTX_THREADS"));
    _ctx_max_threads = val;
  }
  else
  {
    _ctx_max_threads = 2;
#ifdef _SC_NPROCESSORS_ONLN
    _ctx_max_threads = sysconf (_SC_NPROCESSORS_ONLN) / 2;
#endif
  }
  
#if CTX_THREADS
  mtx_init (&_ctx_texture_mtx, mtx_plain);
#endif

  if (_ctx_max_threads < 1) _ctx_max_threads = 1;
  if (_ctx_max_threads > CTX_MAX_THREADS) _ctx_max_threads = CTX_MAX_THREADS;

  //fprintf (stderr, "ctx using %i threads\n", _ctx_max_threads);
  if (!backend)
    backend = getenv ("CTX_BACKEND");

  if (backend && !ctx_strcmp (backend, ""))
    backend = NULL;
  if (backend && !ctx_strcmp (backend, "auto"))
    backend = NULL;
  if (backend && !ctx_strcmp (backend, "list"))
  {
    ctx_list_backends ();
    exit (-1);
  }

  Ctx *ret = NULL;

  /* we do the query on auto but not on directly set ctx
   *
   */
  if ((backend && !ctx_strcmp(backend, "ctx")) ||
      (backend == NULL && is_in_ctx ()))
  {
    if (!backend || !ctx_strcmp (backend, "ctx"))
    {
      // full blown ctx protocol - in terminal or standalone
      ret = ctx_new_ctx (width, height);
    }
  }

#if CTX_HEADLESS
  if (!ret)
    {
      if (backend && !ctx_strcmp (backend, "headless"))
        ret = ctx_new_headless (width, height);
    }
#endif

#if CTX_SDL
  if (!ret && getenv ("DISPLAY"))
  {
    if ((backend==NULL) || (!ctx_strcmp (backend, "SDL")))
      ret = ctx_new_sdl (width, height);
  }
#endif

#if CTX_KMS
  if (!ret && !getenv ("DISPLAY"))
  {
    if ((backend==NULL) || (!ctx_strcmp (backend, "kms")))
      ret = ctx_new_kms (width, height);
  }
#endif


#if CTX_FB
  if (!ret && !getenv ("DISPLAY"))
    {
      if ((backend==NULL) || (!ctx_strcmp (backend, "fb")))
        ret = ctx_new_fb (width, height);
    }
#endif

#if CTX_RASTERIZER
  // braille in terminal
  if (!ret)
  {
    if ((backend==NULL) || (!ctx_strcmp (backend, "term")))
    ret = ctx_new_term (width, height);
  }
  if (!ret)
  {
    if ((backend==NULL) || (!ctx_strcmp (backend, "termimg")))
    ret = ctx_new_termimg (width, height);
  }
#endif
  if (!ret)
  {
    fprintf (stderr, "no interactive ctx backend\n");
    ctx_list_backends ();
    exit (2);
  }
  ctx_get_event (ret); // enables events
  return ret;
}
#endif
#endif
#else
void _ctx_texture_unlock (void)
{
}
void _ctx_texture_lock (void)
{
}

#endif
void _ctx_resized (Ctx *ctx, int width, int height, long time);

void ctx_set_size (Ctx *ctx, int width, int height)
{
  if (ctx->width != width || ctx->height != height)
  {
    ctx->width = width;
    ctx->height = height;
#if CTX_EVENTS
    _ctx_resized (ctx, width, height, 0);
#endif
  }
}

#if CTX_EVENTS


static int is_in_ctx (void)
{
  char buf[1024];
  struct termios orig_attr;
  struct termios raw;
  tcgetattr (STDIN_FILENO, &orig_attr);
  raw = orig_attr;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return 0;
  fprintf (stderr, "\e[?200$p");
  //tcflush(STDIN_FILENO, 1);
#if !__COSMOPOLITAN__
  tcdrain(STDIN_FILENO);
#endif
  int length = 0;
  usleep (1000 * 60); // to account for possibly lowish latency ssh,
                      // should be made configurable ; perhaps in
                      // an env var
  struct timeval tv = {0,0};
  fd_set rfds;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  tv.tv_usec = 1000 * 5;

  for (int n = 0; select(1, &rfds, NULL, NULL, &tv) && n < 20; n++)
  {
    length += read (STDIN_FILENO, &buf[length], 1);
  }
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr);
  if (length == -1)
  {
    return 0;
  }
  char *semi = strchr (buf, ';');
  buf[length]=0;
  if (semi &&  semi[1] == '2')
  {
    return 1;
  }
  return 0;
}

typedef struct CtxIdleCb {
  int (*cb) (Ctx *ctx, void *idle_data);
  void *idle_data;

  void (*destroy_notify)(void *destroy_data);
  void *destroy_data;

  int   ticks_full;
  int   ticks_remaining;
  int   is_idle;
  int   id;
} CtxIdleCb;

void _ctx_events_init (Ctx *ctx)
{
  CtxEvents *events = &ctx->events;
  _ctx_init_ticks ();
  events->tap_delay_min  = 40;
  events->tap_delay_max  = 800;
  events->tap_delay_max  = 8000000; /* quick reflexes needed making it hard for some is an argument against very short values  */

  events->tap_delay_hold = 1000;
  events->tap_hysteresis = 32;  /* XXX: should be ppi dependent */
}


void _ctx_idle_iteration (Ctx *ctx)
{
  static unsigned long prev_ticks = 0;
  CtxList *l;
  unsigned long ticks = ctx_ticks ();
  long tick_delta = (prev_ticks == 0) ? 0 : ticks - prev_ticks;
  prev_ticks = ticks;


  if (!ctx->events.idles && !ctx->events.idles_to_add)
  {
#ifdef EMSCRIPTEN
#ifdef ASYNCIFY
    emscripten_sleep (10);
#endif
#endif
    return;
  }

  ctx->events.in_idle_dispatch=1;

  for (l = ctx->events.idles; l; l = l->next)
  {
    CtxIdleCb *item = l->data;

    long rem = item->ticks_remaining;
    if (item->ticks_remaining >= 0)
    {
      rem -= tick_delta;

      item->ticks_remaining -= tick_delta / 100;

    if (rem < 0)
    {
      int to_be_removed = 0;
      for (CtxList *l2 = ctx->events.idles_to_remove; l2; l2=l2->next)
      {
        CtxIdleCb *item2 = l2->data;
        if (item2 == item) to_be_removed = 1;
      }
      
      if (!to_be_removed)
      {
      if (item->cb (ctx, item->idle_data) == 0)
      {
        ctx_list_prepend (&ctx->events.idles_to_remove, item);
      }
      else
        item->ticks_remaining = item->ticks_full;
      }
    }
    else
        item->ticks_remaining = rem;
    }
    else
    {
      int to_be_removed = 0;
      for (CtxList *l2 = ctx->events.idles_to_remove; l2; l2=l2->next)
      {
        CtxIdleCb *item2 = l2->data;
        if (item2 == item) to_be_removed = 1;
      }
      
      if (!to_be_removed)
      {
        if (item->cb (ctx, item->idle_data) == 0)
        {
          ctx_list_prepend (&ctx->events.idles_to_remove, item);
        }
        else
          item->ticks_remaining = item->ticks_full;
      }
    }
  }

  while (ctx->events.idles_to_add)
  {
    CtxIdleCb *item = ctx->events.idles_to_add->data;
    ctx_list_prepend (&ctx->events.idles, item);
    ctx_list_remove (&ctx->events.idles_to_add, item);
  }

  while (ctx->events.idles_to_remove)
  {
    CtxIdleCb *item = ctx->events.idles_to_remove->data;
    ctx_list_remove (&ctx->events.idles, item);
    ctx_list_remove (&ctx->events.idles_to_remove, item);
    if (item->destroy_notify)
      item->destroy_notify (item->destroy_data);
  }
  ctx->events.in_idle_dispatch=0;
#if EMSCRIPTEN
#ifdef ASYNCIFY
   emscripten_sleep(1);
#endif
#endif
}


void ctx_add_key_binding_full (Ctx *ctx,
                           const char *key,
                           const char *action,
                           const char *label,
                           CtxCb       cb,
                           void       *cb_data,
                           CtxDestroyNotify destroy_notify,
                           void       *destroy_data)
{
  CtxEvents *events = &ctx->events;
  if (events->n_bindings +1 >= CTX_MAX_KEYBINDINGS)
  {
    fprintf (stderr, "warning: binding overflow\n");
    return;
  }
  events->bindings[events->n_bindings].nick = ctx_strdup (key);
  strcpy (events->bindings[events->n_bindings].nick, key);

  if (action)
    events->bindings[events->n_bindings].command = action ? ctx_strdup (action) : NULL;
  if (label)
    events->bindings[events->n_bindings].label = label ? ctx_strdup (label) : NULL;
  events->bindings[events->n_bindings].cb = cb;
  events->bindings[events->n_bindings].cb_data = cb_data;
  events->bindings[events->n_bindings].destroy_notify = destroy_notify;
  events->bindings[events->n_bindings].destroy_data = destroy_data;
  events->n_bindings++;
}

void ctx_add_key_binding (Ctx *ctx,
                          const char *key,
                          const char *action,
                          const char *label,
                          CtxCb       cb,
                          void       *cb_data)
{
  ctx_add_key_binding_full (ctx, key, action, label, cb, cb_data, NULL, NULL);
}

void ctx_clear_bindings (Ctx *ctx)
{
  CtxEvents *events = &ctx->events;
  int i;
  for (i = 0; events->bindings[i].nick; i ++)
  {
    if (events->bindings[i].destroy_notify)
      events->bindings[i].destroy_notify (events->bindings[i].destroy_data);
    free (events->bindings[i].nick);
    if (events->bindings[i].command)
      free (events->bindings[i].command);
    if (events->bindings[i].label)
      free (events->bindings[i].label);
  }
  memset (&events->bindings, 0, sizeof (events->bindings));
  events->n_bindings = 0;
}

static void
ctx_collect_events (CtxEvent *event, void *data, void *data2);
static void _ctx_bindings_key_press (CtxEvent *event, void *data1, void *data2)
{
  Ctx *ctx = event->ctx;
  CtxEvents *events = &ctx->events;
  int i;
  int handled = 0;

  for (i = events->n_bindings-1; i>=0; i--)
    if (!ctx_strcmp (events->bindings[i].nick, event->string))
    {
      if (events->bindings[i].cb)
      {
        events->bindings[i].cb (event, events->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
        handled = 1;
      }
    }
  if (!handled)
  for (i = events->n_bindings-1; i>=0; i--)
    if (!ctx_strcmp (events->bindings[i].nick, "any"))
    {
      if (events->bindings[i].cb)
      {
        events->bindings[i].cb (event, events->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
      }
    }
  ctx_collect_events (event, data1, data2);
}

CtxBinding *ctx_get_bindings (Ctx *ctx)
{
  return &ctx->events.bindings[0];
}

void ctx_remove_idle (Ctx *ctx, int handle)
{
  CtxList *l;
  //CtxList *to_remove = NULL;

  if (!ctx->events.idles)
  {
    return;
  }

  for (l = ctx->events.idles; l; l = l->next)
  {
    CtxIdleCb *item = l->data;
    if (item->id == handle)
    {
      ctx_list_prepend (&ctx->events.idles_to_remove, item);
    }
  }

  if (ctx->events.in_idle_dispatch)
    return;

  while (ctx->events.idles_to_remove)
  {
    CtxIdleCb *item = ctx->events.idles_to_remove->data;
    ctx_list_remove (&ctx->events.idles, item);
    ctx_list_remove (&ctx->events.idles_to_remove, item);
    if (item->destroy_notify)
      item->destroy_notify (item->destroy_data);
  }
}

int ctx_add_timeout_full (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                          void (*destroy_notify)(void *destroy_data), void *destroy_data)
{
  CtxIdleCb *item = calloc (sizeof (CtxIdleCb), 1);
  item->cb              = idle_cb;
  item->idle_data       = idle_data;
  item->id              = ++ctx->events.idle_id;
  item->ticks_full      = 
  item->ticks_remaining = ms * 1000;
  item->destroy_notify  = destroy_notify;
  item->destroy_data    = destroy_data;
  if (ctx->events.in_idle_dispatch)
  ctx_list_append (&ctx->events.idles_to_add, item);
  else
  ctx_list_append (&ctx->events.idles, item);
  return item->id;
}

int ctx_add_timeout (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data)
{
  return ctx_add_timeout_full (ctx, ms, idle_cb, idle_data, NULL, NULL);
}

int ctx_add_idle_full (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                                 void (*destroy_notify)(void *destroy_data), void *destroy_data)
{
  CtxIdleCb *item = calloc (sizeof (CtxIdleCb), 1);
  item->cb = idle_cb;
  item->idle_data = idle_data;
  item->id = ++ctx->events.idle_id;
  item->ticks_full =
  item->ticks_remaining = -1;
  item->is_idle = 1;
  item->destroy_notify = destroy_notify;
  item->destroy_data = destroy_data;
  ctx_list_append (&ctx->events.idles, item);
  return item->id;
}

int ctx_add_idle (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data)
{
  return ctx_add_idle_full (ctx, idle_cb, idle_data, NULL, NULL);
}

#endif
/* using bigger primes would be a good idea, this falls apart due to rounding
 * when zoomed in close
 */
static inline double ctx_path_hash (void *path)
{
  double ret = 0;
#if 0
  int i;
  cairo_path_data_t *data;
  if (!path)
    return 0.99999;
  for (i = 0; i <path->num_data; i += path->data[i].header.length)
  {
    data = &path->data[i];
    switch (data->header.type) {
      case CAIRO_PATH_MOVE_TO:
        ret *= 17;
        ret += data[1].point.x;
        ret *= 113;
        ret += data[1].point.y;
        break;
      case CAIRO_PATH_LINE_TO:
        ret *= 121;
        ret += data[1].point.x;
        ret *= 1021;
        ret += data[1].point.y;
        break;
      case CAIRO_PATH_CURVE_TO:
        ret *= 3111;
        ret += data[1].point.x;
        ret *= 23;
        ret += data[1].point.y;
        ret *= 107;
        ret += data[2].point.x;
        ret *= 739;
        ret += data[2].point.y;
        ret *= 3;
        ret += data[3].point.x;
        ret *= 51;
        ret += data[3].point.y;
        break;
      case CAIRO_PATH_CLOSE_PATH:
        ret *= 51;
        break;
    }
  }
#endif
  return ret;
}

#if CTX_EVENTS
void _ctx_item_ref (CtxItem *item)
{
  if (item->ref_count < 0)
  {
    fprintf (stderr, "EEEEK!\n");
  }
  item->ref_count++;
}


void _ctx_item_unref (CtxItem *item)
{
  if (item->ref_count <= 0)
  {
    fprintf (stderr, "EEEEK!\n");
    return;
  }
  item->ref_count--;
  if (item->ref_count <=0)
  {
    {
      int i;
      for (i = 0; i < item->cb_count; i++)
      {
        if (item->cb[i].finalize)
          item->cb[i].finalize (item->cb[i].data1, item->cb[i].data2,
                                   item->cb[i].finalize_data);
      }
    }
    if (item->path)
    {
      //cairo_path_destroy (item->path);
    }
    free (item);
  }
}


void _ctx_item_unref2 (void *data, void *data2)
{
  CtxItem *item = (CtxItem*)data;
  _ctx_item_unref (item);
}


static int
path_equal (void *path,
            void *path2)
{
  //  XXX
  return 0;
}

void ctx_listen_set_cursor (Ctx      *ctx,
                            CtxCursor cursor)
{
  if (ctx->events.last_item)
  {
    ctx->events.last_item->cursor = cursor;
  }
}

void ctx_listen_full (Ctx     *ctx,
                      float    x,
                      float    y,
                      float    width,
                      float    height,
                      CtxEventType  types,
                      CtxCb    cb,
                      void    *data1,
                      void    *data2,
                      void   (*finalize)(void *listen_data,
                                         void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data)
{
  if (!ctx->events.frozen)
  {
    CtxItem *item;

    /* early bail for listeners outside screen  */
    /* XXX: fixme respect clipping */
    {
      float tx = x;
      float ty = y;
      float tw = width;
      float th = height;
      _ctx_user_to_device (&ctx->state, &tx, &ty);
      _ctx_user_to_device_distance (&ctx->state, &tw, &th);
      if (ty > ctx->height * 2 ||
          tx > ctx->width * 2 ||
          tx + tw < 0 ||
          ty + th < 0)
      {
        if (finalize)
          finalize (data1, data2, finalize_data);
        return;
      }
    }

    item = calloc (sizeof (CtxItem), 1);
    item->x0 = x;
    item->y0 = y;
    item->x1 = x + width;
    item->y1 = y + height;
    item->cb[0].types = types;
    item->cb[0].cb = cb;
    item->cb[0].data1 = data1;
    item->cb[0].data2 = data2;
    item->cb[0].finalize = finalize;
    item->cb[0].finalize_data = finalize_data;
    item->cb_count = 1;
    item->types = types;
    //item->path = cairo_copy_path (cr); // XXX
    item->path_hash = ctx_path_hash (item->path);
    ctx_get_matrix (ctx, &item->inv_matrix);
    ctx_matrix_invert (&item->inv_matrix);

    if (ctx->events.items)
    {
      CtxList *l;
      for (l = ctx->events.items; l; l = l->next)
      {
        CtxItem *item2 = l->data;

        /* store multiple callbacks for one entry when the paths
         * are exact matches, reducing per event traversal checks at the
         * cost of a little paint-hit (XXX: is this the right tradeoff,
         * perhaps it is better to spend more time during event processing
         * than during paint?)
         */
        if (item->path_hash == item2->path_hash &&
            path_equal (item->path, item2->path))
        {
          /* found an item, copy over cb data  */
          item2->cb[item2->cb_count] = item->cb[0];
          free (item);
          item2->cb_count++;
          item2->types |= types;
          return;
        }
      }
    }
    item->ref_count       = 1;
    ctx->events.last_item = item;
    ctx_list_prepend_full (&ctx->events.items, item, _ctx_item_unref2, NULL);
      return;
  }
}

void ctx_event_stop_propagate (CtxEvent *event)
{
  if (event)
    event->stop_propagate = 1;
}

void ctx_listen (Ctx          *ctx,
                 CtxEventType  types,
                 CtxCb         cb,
                 void*         data1,
                 void*         data2)
{
  float x, y, width, height;
  /* generate bounding box of what to listen for - from current cairo path */
  if (types & CTX_KEY)
  {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
  }
  else
  {
     float ex1,ey1,ex2,ey2;
     ctx_path_extents (ctx, &ex1, &ey1, &ex2, &ey2);
     x = ex1;
     y = ey1;
     width = ex2 - ex1;
     height = ey2 - ey1;
  }

  if (types == CTX_DRAG_MOTION)
    types = CTX_DRAG_MOTION | CTX_DRAG_PRESS;
  return ctx_listen_full (ctx, x, y, width, height, types, cb, data1, data2, NULL, NULL);
}

void  ctx_listen_with_finalize (Ctx          *ctx,
                                CtxEventType  types,
                                CtxCb         cb,
                                void*         data1,
                                void*         data2,
                      void   (*finalize)(void *listen_data, void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data)
{
  float x, y, width, height;
  /* generate bounding box of what to listen for - from current cairo path */
  if (types & CTX_KEY)
  {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
  }
  else
  {
     float ex1,ey1,ex2,ey2;
     ctx_path_extents (ctx, &ex1, &ey1, &ex2, &ey2);
     x = ex1;
     y = ey1;
     width = ex2 - ex1;
     height = ey2 - ey1;
  }

  if (types == CTX_DRAG_MOTION)
    types = CTX_DRAG_MOTION | CTX_DRAG_PRESS;
  return ctx_listen_full (ctx, x, y, width, height, types, cb, data1, data2, finalize, finalize_data);
}


static void ctx_report_hit_region (CtxEvent *event,
                       void     *data,
                       void     *data2)
{
  const char *id = data;

  fprintf (stderr, "hit region %s\n", id);
  // XXX: NYI
}

void ctx_add_hit_region (Ctx *ctx, const char *id)
{
  char *id_copy = ctx_strdup (id);
  float x, y, width, height;
  /* generate bounding box of what to listen for - from current cairo path */
  {
     float ex1,ey1,ex2,ey2;
     ctx_path_extents (ctx, &ex1, &ey1, &ex2, &ey2);
     x = ex1;
     y = ey1;
     width = ex2 - ex1;
     height = ey2 - ey1;
  }
  
  return ctx_listen_full (ctx, x, y, width, height,
                          CTX_POINTER, ctx_report_hit_region,
                          id_copy, NULL, (void*)free, NULL);
}

typedef struct _CtxGrab CtxGrab;

struct _CtxGrab
{
  CtxItem *item;
  int      device_no;
  int      timeout_id;
  int      start_time;
  float    x; // for tap and hold
  float    y;
  CtxEventType  type;
};

static void grab_free (Ctx *ctx, CtxGrab *grab)
{
  if (grab->timeout_id)
  {
    ctx_remove_idle (ctx, grab->timeout_id);
    grab->timeout_id = 0;
  }
  _ctx_item_unref (grab->item);
  free (grab);
}

static void device_remove_grab (Ctx *ctx, CtxGrab *grab)
{
  ctx_list_remove (&ctx->events.grabs, grab);
  grab_free (ctx, grab);
}

static CtxGrab *device_add_grab (Ctx *ctx, int device_no, CtxItem *item, CtxEventType type)
{
  CtxGrab *grab = calloc (1, sizeof (CtxGrab));
  grab->item = item;
  grab->type = type;
  _ctx_item_ref (item);
  grab->device_no = device_no;
  ctx_list_append (&ctx->events.grabs, grab);
  return grab;
}

static CtxList *_ctx_device_get_grabs (Ctx *ctx, int device_no)
{
  CtxList *ret = NULL;
  CtxList *l;
  for (l = ctx->events.grabs; l; l = l->next)
  {
    CtxGrab *grab = l->data;
    if (grab->device_no == device_no)
      ctx_list_append (&ret, grab);
  }
  return ret;
}

static void _mrg_restore_path (Ctx *ctx, void *path)  //XXX
{
  //int i;
  //cairo_path_data_t *data;
  //cairo_new_path (cr);
  //cairo_append_path (cr, path);
}

CtxList *_ctx_detect_list (Ctx *ctx, float x, float y, CtxEventType type)
{
  CtxList *a;
  CtxList *ret = NULL;

  if (type == CTX_KEY_DOWN ||
      type == CTX_KEY_UP ||
      type == CTX_KEY_PRESS ||
      type == CTX_MESSAGE ||
      type == (CTX_KEY_DOWN|CTX_MESSAGE) ||
      type == (CTX_KEY_DOWN|CTX_KEY_UP) ||
      type == (CTX_KEY_DOWN|CTX_KEY_UP|CTX_MESSAGE))
  {
    for (a = ctx->events.items; a; a = a->next)
    {
      CtxItem *item = a->data;
      if (item->types & type)
      {
        ctx_list_prepend (&ret, item);
        return ret;
      }
    }
    return NULL;
  }

  for (a = ctx->events.items; a; a = a->next)
  {
    CtxItem *item= a->data;
  
    float u, v;
    u = x;
    v = y;
    _ctx_matrix_apply_transform (&item->inv_matrix, &u, &v);

    if (u >= item->x0 && v >= item->y0 &&
        u <  item->x1 && v <  item->y1 && 
        ((item->types & type) || ((type == CTX_SET_CURSOR) &&
        item->cursor)))
    {
      if (item->path)
      {
        _mrg_restore_path (ctx, item->path);
        if (ctx_in_fill (ctx, u, v))
        {
          ctx_begin_path (ctx);
          ctx_list_prepend (&ret, item);
        }
        ctx_begin_path (ctx);
      }
      else
      {
        ctx_list_prepend (&ret, item);
      }
    }
  }
  return ret;
}

CtxItem *_ctx_detect (Ctx *ctx, float x, float y, CtxEventType type)
{
  CtxList *l = _ctx_detect_list (ctx, x, y, type);
  if (l)
  {
    ctx_list_reverse (&l);
    CtxItem *ret = l->data;
    ctx_list_free (&l);
    return ret;
  }
  return NULL;
}

static int
_ctx_emit_cb_item (Ctx *ctx, CtxItem *item, CtxEvent *event, CtxEventType type, float x, float y)
{
  static CtxEvent s_event;
  CtxEvent transformed_event;
  int i;


  if (!event)
  {
    event = &s_event;
    event->type = type;
    event->x = x;
    event->y = y;
  }
  event->ctx = ctx;
  transformed_event = *event;
  transformed_event.device_x = event->x;
  transformed_event.device_y = event->y;

  {
    float tx, ty;
    tx = transformed_event.x;
    ty = transformed_event.y;
    _ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
    transformed_event.x = tx;
    transformed_event.y = ty;

    if ((type & CTX_DRAG_PRESS) ||
        (type & CTX_DRAG_MOTION) ||
        (type & CTX_MOTION))   /* probably a worthwhile check for the performance 
                                  benefit
                                */
    {
      tx = transformed_event.start_x;
      ty = transformed_event.start_y;
      _ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
      transformed_event.start_x = tx;
      transformed_event.start_y = ty;
    }


    tx = transformed_event.delta_x;
    ty = transformed_event.delta_y;
    _ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
    transformed_event.delta_x = tx;
    transformed_event.delta_y = ty;
  }

  transformed_event.state = ctx->events.modifier_state;
  transformed_event.type = type;

  for (i = item->cb_count-1; i >= 0; i--)
  {
    if (item->cb[i].types & type)
    {
      item->cb[i].cb (&transformed_event, item->cb[i].data1, item->cb[i].data2);
      event->stop_propagate = transformed_event.stop_propagate; /* copy back the response */
      if (event->stop_propagate)
        return event->stop_propagate;
    }
  }
  return 0;
}
#endif

#if CTX_EVENTS

//#include <stdatomic.h>

void ctx_consume_events (Ctx *ctx)
{
  CtxBackend *backend = ctx->backend;
  if (backend && backend->consume_events)
    backend->consume_events (ctx);
}

void ctx_stdin_get_event_fds (Ctx *ctx, int *fd, int *count)
{
  fd[0] = STDIN_FILENO;
  *count = 1;
}

void ctx_get_event_fds (Ctx *ctx, int *fd, int *count)
{
  CtxBackend *backend = ctx->backend;
  if (backend && backend->get_event_fds)
    backend->get_event_fds (ctx, fd, count);
  *count = 0;
}

CtxEvent *ctx_get_event (Ctx *ctx)
{
  static CtxEvent event_copy;
  if (ctx->events.events)
    {
      event_copy = *((CtxEvent*)(ctx->events.events->data));
      ctx_list_remove (&ctx->events.events, ctx->events.events->data);
      return &event_copy;
    }

  _ctx_idle_iteration (ctx);
#if 1
  if (ctx->events.ctx_get_event_enabled==0)
  {
    ctx->events.ctx_get_event_enabled = 1;
    ctx_queue_draw (ctx);
  }
#endif

  ctx_consume_events (ctx);

  if (ctx->events.events)
    {
      event_copy = *((CtxEvent*)(ctx->events.events->data));
      ctx_list_remove (&ctx->events.events, ctx->events.events->data);
      return &event_copy;
    }
  return NULL;
}

static int
_ctx_emit_cb (Ctx *ctx, CtxList *items, CtxEvent *event, CtxEventType type, float x, float y)
{
  CtxList *l;
  event->stop_propagate = 0;
  for (l = items; l; l = l->next)
  {
    _ctx_emit_cb_item (ctx, l->data, event, type, x, y);
    if (event->stop_propagate)
      return event->stop_propagate;
  }
  return 0;
}

/*
 * update what is the currently hovered item and returns it.. and the list of hits
 * a well.
 *
 */
static CtxItem *_ctx_update_item (Ctx *ctx, int device_no, float x, float y, CtxEventType type, CtxList **hitlist)
{
  CtxItem *current = NULL;

  CtxList *l = _ctx_detect_list (ctx, x, y, type);
  if (l)
  {
    ctx_list_reverse (&l);
    current = l->data;
  }
  if (hitlist)
    *hitlist = l;
  else
    ctx_list_free (&l);

  if (ctx->events.prev[device_no] == NULL || current == NULL || (current->path_hash != ctx->events.prev[device_no]->path_hash))
  {
// enter/leave should snapshot chain to root
// and compare with previous snapshotted chain to root
// and emit/enter/leave as appropriate..
//
// leave might be registered for emission on enter..emission?


    //int focus_radius = 2;
    if (current)
      _ctx_item_ref (current);

    if (ctx->events.prev[device_no])
    {
      {
#if 0
        CtxIntRectangle rect = {floor(ctx->events.prev[device_no]->x0-focus_radius),
                             floor(ctx->events.prev[device_no]->y0-focus_radius),
                             ceil(ctx->events.prev[device_no]->x1)-floor(ctx->events.prev[device_no]->x0) + focus_radius * 2,
                             ceil(ctx->events.prev[device_no]->y1)-floor(ctx->events.prev[device_no]->y0) + focus_radius * 2};
        mrg_queue_draw (mrg, &rect);
#endif 
      }

      _ctx_emit_cb_item (ctx, ctx->events.prev[device_no], NULL, CTX_LEAVE, x, y);
      _ctx_item_unref (ctx->events.prev[device_no]);
      ctx->events.prev[device_no] = NULL;
    }
    if (current)
    {
#if 0
      {
        CtxIntRectangle rect = {floor(current->x0-focus_radius),
                             floor(current->y0-focus_radius),
                             ceil(current->x1)-floor(current->x0) + focus_radius * 2,
                             ceil(current->y1)-floor(current->y0) + focus_radius * 2};
        mrg_queue_draw (mrg, &rect);
      }
#endif
      _ctx_emit_cb_item (ctx, current, NULL, CTX_ENTER, x, y);
      ctx->events.prev[device_no] = current;
    }
  }
  current = _ctx_detect (ctx, x, y, type);
  //fprintf (stderr, "%p\n", current);
  return current;
}

static int tap_and_hold_fire (Ctx *ctx, void *data)
{
  CtxGrab *grab = data;
  CtxList *list = NULL;
  ctx_list_prepend (&list, grab->item);
  CtxEvent event = {0, };

  event.ctx = ctx;
  event.time = ctx_ms (ctx);

  event.device_x = 
  event.x = ctx->events.pointer_x[grab->device_no];
  event.device_y = 
  event.y = ctx->events.pointer_y[grab->device_no];

  // XXX: x and y coordinates
  int ret = _ctx_emit_cb (ctx, list, &event, CTX_TAP_AND_HOLD,
      ctx->events.pointer_x[grab->device_no], ctx->events.pointer_y[grab->device_no]);

  ctx_list_free (&list);

  grab->timeout_id = 0;

  return 0;

  return ret;
}

CTX_EXPORT int
ctx_pointer_drop (Ctx *ctx, float x, float y, int device_no, uint32_t time,
                  char *string)
{
  CtxList *l;
  CtxList *hitlist = NULL;

  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    ctx->events.pointer_x[0] = x;
    ctx->events.pointer_y[0] = y;
  }

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &ctx->events.drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->ctx = ctx;
  event->x = x;
  event->y = y;

  event->delta_x = event->delta_y = 0;

  event->device_no = device_no;
  event->string    = string;
  event->time      = time;
  event->stop_propagate = 0;

  _ctx_update_item (ctx, device_no, x, y, CTX_DROP, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;
    _ctx_emit_cb_item (ctx, item, event, CTX_DROP, x, y);

    if (event->stop_propagate)
    {
      ctx_list_free (&hitlist);
      return 0;
    }
  }

  //mrg_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);

  return 0;
}

CTX_EXPORT int
ctx_pointer_press (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxEvents *events = &ctx->events;
  CtxList *hitlist = NULL;
  events->pointer_x[device_no] = x;
  events->pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    events->pointer_x[0] = x;
    events->pointer_y[0] = y;
  }

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &events->drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->x = event->start_x = event->prev_x = x;
  event->y = event->start_y = event->prev_y = y;

  event->delta_x = event->delta_y = 0;

  event->device_no = device_no;
  event->time      = time;
  event->stop_propagate = 0;

  if (events->pointer_down[device_no] == 1)
  {
    fprintf (stderr, "events thought device %i was already down\n", device_no);
  }
  /* doing just one of these two should be enough? */
  events->pointer_down[device_no] = 1;
  switch (device_no)
  {
    case 1:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON1;
      break;
    case 2:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON2;
      break;
    case 3:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON3;
      break;
    default:
      break;
  }

  CtxGrab *grab = NULL;
  CtxList *l;

  _ctx_update_item (ctx, device_no, x, y, 
      CTX_PRESS | CTX_DRAG_PRESS | CTX_TAP | CTX_TAP_AND_HOLD, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;
    if (item &&
        ((item->types & CTX_DRAG)||
         (item->types & CTX_TAP) ||
         (item->types & CTX_TAP_AND_HOLD)))
    {
      grab = device_add_grab (ctx, device_no, item, item->types);
      grab->start_time = time;

      if (item->types & CTX_TAP_AND_HOLD)
      {
         grab->timeout_id = ctx_add_timeout (ctx, events->tap_delay_hold, tap_and_hold_fire, grab);
      }
    }
    _ctx_emit_cb_item (ctx, item, event, CTX_PRESS, x, y);
    if (!event->stop_propagate)
      _ctx_emit_cb_item (ctx, item, event, CTX_DRAG_PRESS, x, y);

    if (event->stop_propagate)
    {
      ctx_list_free (&hitlist);
      return 0;
    }
  }

  //events_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);
  return 0;
}

void _ctx_resized (Ctx *ctx, int width, int height, long time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_PRESS);
  CtxEvent event = {0, };

  if (!time)
    time = ctx_ms (ctx);
  
  event.ctx = ctx;
  event.time = time;
  event.string = "resize-event"; /* gets delivered to clients as a key_down event, maybe message shouldbe used instead?
   */

  if (item)
  {
    event.stop_propagate = 0;
    _ctx_emit_cb_item (ctx, item, &event, CTX_KEY_PRESS, 0, 0);
  }

}

CTX_EXPORT int
ctx_pointer_release (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxEvents *events = &ctx->events;
  if (time == 0)
    time = ctx_ms (ctx);

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &events->drag_event[device_no];

  event->time = time;
  event->x = x;
  event->ctx = ctx;
  event->y = y;
  event->device_no = device_no;
  event->stop_propagate = 0;

  switch (device_no)
  {
    case 1:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON1)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON1;
      break;
    case 2:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON2)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON2;
      break;
    case 3:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON3)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON3;
      break;
    default:
      break;
  }

  //events_queue_draw (mrg, NULL); /* in case of style change */

  if (events->pointer_down[device_no] == 0)
  {
    //fprintf (stderr, "device %i already up\n", device_no);
  }
  events->pointer_down[device_no] = 0;

  events->pointer_x[device_no] = x;
  events->pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    events->pointer_x[0] = x;
    events->pointer_y[0] = y;
  }
  CtxList *hitlist = NULL;
  CtxList *grablist = NULL , *g= NULL;
  CtxGrab *grab;

  _ctx_update_item (ctx, device_no, x, y, CTX_RELEASE | CTX_DRAG_RELEASE, &hitlist);
  grablist = _ctx_device_get_grabs (ctx, device_no);

  for (g = grablist; g; g = g->next)
  {
    grab = g->data;

    if (!event->stop_propagate)
    {
      if (grab->item->types & CTX_TAP)
      {
        long delay = time - grab->start_time;

        if (delay > events->tap_delay_min &&
            delay < events->tap_delay_max &&
            (
              (event->start_x - x) * (event->start_x - x) +
              (event->start_y - y) * (event->start_y - y)) < ctx_pow2(events->tap_hysteresis)
            )
        {
          _ctx_emit_cb_item (ctx, grab->item, event, CTX_TAP, x, y);
        }
      }

      if (!event->stop_propagate && grab->item->types & CTX_DRAG_RELEASE)
      {
        _ctx_emit_cb_item (ctx, grab->item, event, CTX_DRAG_RELEASE, x, y);
      }
    }

    device_remove_grab (ctx, grab);
  }

  if (hitlist)
  {
    if (!event->stop_propagate)
      _ctx_emit_cb (ctx, hitlist, event, CTX_RELEASE, x, y);
    ctx_list_free (&hitlist);
  }
  ctx_list_free (&grablist);
  return 0;
}

/*  for multi-touch, we use a list of active grabs - thus a grab corresponds to
 *  a device id. even during drag-grabs events propagate; to stop that stop
 *  propagation.
 */
CTX_EXPORT int
ctx_pointer_motion (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxList *hitlist = NULL;
  CtxList *grablist = NULL, *g;
  CtxGrab *grab;

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &ctx->events.drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->ctx       = ctx;
  event->x         = x;
  event->y         = y;
  event->time      = time;
  event->device_no = device_no;
  event->stop_propagate = 0;
  
  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;

  if (device_no <= 3)
  {
    ctx->events.pointer_x[0] = x;
    ctx->events.pointer_y[0] = y;
  }

  grablist = _ctx_device_get_grabs (ctx, device_no);
  _ctx_update_item (ctx, device_no, x, y, CTX_MOTION, &hitlist);

  {
    CtxItem  *cursor_item = _ctx_detect (ctx, x, y, CTX_SET_CURSOR);
    if (cursor_item)
    {
      ctx_set_cursor (ctx, cursor_item->cursor);
    }
    else
    {
      ctx_set_cursor (ctx, CTX_CURSOR_ARROW);
    }
    CtxItem  *hovered_item = _ctx_detect (ctx, x, y, CTX_ANY);
    static CtxItem *prev_hovered_item = NULL;
    if (prev_hovered_item != hovered_item)
    {
      ctx_queue_draw (ctx);
    }
    prev_hovered_item = hovered_item;
  }

  event->delta_x = x - event->prev_x;
  event->delta_y = y - event->prev_y;
  event->prev_x  = x;
  event->prev_y  = y;

  CtxList *remove_grabs = NULL;

  for (g = grablist; g; g = g->next)
  {
    grab = g->data;

    if ((grab->type & CTX_TAP) ||
        (grab->type & CTX_TAP_AND_HOLD))
    {
      if (
          (
            (event->start_x - x) * (event->start_x - x) +
            (event->start_y - y) * (event->start_y - y)) >
              ctx_pow2(ctx->events.tap_hysteresis)
         )
      {
        //fprintf (stderr, "-");
        ctx_list_prepend (&remove_grabs, grab);
      }
      else
      {
        //fprintf (stderr, ":");
      }
    }

    if (grab->type & CTX_DRAG_MOTION)
    {
      _ctx_emit_cb_item (ctx, grab->item, event, CTX_DRAG_MOTION, x, y);
      if (event->stop_propagate)
        break;
    }
  }
  if (remove_grabs)
  {
    for (g = remove_grabs; g; g = g->next)
      device_remove_grab (ctx, g->data);
    ctx_list_free (&remove_grabs);
  }
  if (hitlist)
  {
    if (!event->stop_propagate)
      _ctx_emit_cb (ctx, hitlist, event, CTX_MOTION, x, y);
    ctx_list_free (&hitlist);
  }
  ctx_list_free (&grablist);
  return 0;
}

CTX_EXPORT void
ctx_incoming_message (Ctx *ctx, const char *message, long time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_MESSAGE);
  CtxEvent event = {0, };

  if (!time)
    time = ctx_ms (ctx);

  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_MESSAGE;
    event.time = time;
    event.string = message;

    fprintf (stderr, "{%s|\n", message);

      for (i = 0; i < item->cb_count; i++)
      {
        if (item->cb[i].types & (CTX_MESSAGE))
        {
          event.state = ctx->events.modifier_state;
          item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
          if (event.stop_propagate)
            return;// event.stop_propagate;
        }
      }
  }
}

CTX_EXPORT int
ctx_scrolled (Ctx *ctx, float x, float y, CtxScrollDirection scroll_direction, uint32_t time)
{
  CtxList *hitlist = NULL;
  CtxList *l;

  int device_no = 0;
  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;

  CtxEvent *event = &ctx->events.drag_event[device_no];  /* XXX: might
                                       conflict with other code
                                       create a sibling member
                                       of drag_event?*/
  if (time == 0)
    time = ctx_ms (ctx);

  event->x         = event->start_x = event->prev_x = x;
  event->y         = event->start_y = event->prev_y = y;
  event->delta_x   = event->delta_y = 0;
  event->device_no = device_no;
  event->time      = time;
  event->stop_propagate = 0;
  event->scroll_direction = scroll_direction;

  _ctx_update_item (ctx, device_no, x, y, CTX_SCROLL, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;

    _ctx_emit_cb_item (ctx, item, event, CTX_SCROLL, x, y);

    if (event->stop_propagate)
      l = NULL;
  }

  //mrg_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);
  return 0;
}

static int ctx_str_has_prefix (const char *string, const char *prefix)
{
  for (int i = 0; prefix[i]; i++)
  {
    if (!string[i]) return 0;
    if (string[i] != prefix[i]) return 0;
  }
  return 0;
}


static const char *ctx_keycode_to_keyname (CtxModifierState modifier_state,
                                           int keycode)
{
   static char temp[6]=" ";
   const char *str = &temp[0];
   if (keycode >= 65 && keycode <= 90)
   {
     if (modifier_state & CTX_MODIFIER_STATE_SHIFT)
       temp[0]=keycode-65+'A';
     else
       temp[0]=keycode-65+'a';
   }
   else if (keycode >= 112 && keycode <= 123)
   {
     sprintf (temp, "F%i", keycode-111);
   }
   else
   switch (keycode)
   {
     case 8: str="backspace"; break;
     case 9: str="tab"; break;
     case 13: str="return"; break;
     case 16: str="shift"; break;
     case 17: str="control"; break;
     case 18: str="alt"; break;
     case 27: str="escape"; break;
     case 32: str="space"; break;
     case 33: str="page-up"; break;
     case 34: str="page-down"; break;
     case 35: str="end"; break;
     case 36: str="home"; break;
     case 37: str="left"; break;
     case 38: str="up"; break;
     case 39: str="right"; break;
     case 40: str="down"; break;
     case 45: str="insert"; break;
     case 46: str="delete"; break;
     default:
       if (modifier_state & CTX_MODIFIER_STATE_SHIFT)
       switch (keycode)
       {
         case 173: str="_"; break;
         case 186: str=":"; break;
         case 187: str="+"; break;
         case 188: str="<"; break;
         case 189: str="_"; break;
         case 190: str=">"; break;
         case 191: str="?"; break;
         case 192: str="~"; break;
         case 219: str="{"; break;
         case 221: str="}"; break;
         case 220: str="|"; break;
         case 222: str="\""; break;
         case 48: str=")"; break;
         case 49: str="!"; break;
         case 50: str="@"; break;
         case 51: str="#"; break;
         case 52: str="$"; break;
         case 53: str="%"; break;
         case 54: str="^"; break;
         case 55: str="&"; break;
         case 56: str="*"; break;
         case 57: str="("; break;
         case 59: str=":"; break;
         case 61: str="+"; break;
         default:
           fprintf (stderr, "unhandled skeycode %i\n", keycode);
           str="?";
           break;
       }
       else
       switch (keycode)
       {
         case 61: str="="; break;
         case 59: str=";"; break;
         case 173: str="-"; break;
         case 186: str=";"; break;
         case 187: str="="; break;
         case 188: str=","; break;
         case 189: str="-"; break;
         case 190: str="."; break;
         case 191: str="/"; break;
         case 192: str="`"; break;
         case 219: str="["; break;
         case 221: str="]"; break;
         case 220: str="\\"; break;
         case 222: str="'"; break;
         default:
           if (keycode >= 48 && keycode <=66)
           {
             temp[0]=keycode-48+'0';
           }
           else
           {
             fprintf (stderr, "unhandled keycode %i\n", keycode);
             str="?";
           }
           break;
       }
   }
   return str;
}

CTX_EXPORT int
ctx_key_press (Ctx *ctx, unsigned int keyval,
               const char *string, uint32_t time)
{
  char temp_key[128]="";
  char event_type[128]="";
  float x, y; int b;
  if (!string)
  {
    string = ctx_keycode_to_keyname (ctx->events.modifier_state, keyval);

    if (!ctx_strcmp (string, "shift") ||
        !ctx_strcmp (string, "control") ||
        !ctx_strcmp (string, "alt"))
      return 0;

    if (ctx->events.modifier_state)
    {
       if (ctx->events.modifier_state & CTX_MODIFIER_STATE_SHIFT &&
           (ctx->events.modifier_state & CTX_MODIFIER_STATE_ALT||
            ctx->events.modifier_state & CTX_MODIFIER_STATE_CONTROL))
       {
         string = ctx_keycode_to_keyname (0, keyval);
         sprintf (&temp_key[ctx_strlen(temp_key)], "shift-");
       }

       if (ctx->events.modifier_state & CTX_MODIFIER_STATE_ALT)
       {
         sprintf (&temp_key[ctx_strlen(temp_key)], "alt-");
       }
       if (ctx->events.modifier_state & CTX_MODIFIER_STATE_CONTROL)
       {
         sprintf (&temp_key[ctx_strlen(temp_key)], "control-");
       }
       sprintf (&temp_key[ctx_strlen(temp_key)], "%s", string);
       string = temp_key;
    }
  }

  sscanf (string, "%s %f %f %i", event_type, &x, &y, &b);
  if (!ctx_strcmp (event_type, "pm") ||
      !ctx_strcmp (event_type, "pd"))
    return ctx_pointer_motion (ctx, x, y, b, 0);
  else if (!ctx_strcmp (event_type, "pp"))
    return ctx_pointer_press (ctx, x, y, b, 0);
  else if (!ctx_strcmp (event_type, "pr"))
    return ctx_pointer_release (ctx, x, y, b, 0);
  //else if (!ctx_strcmp (event_type, "keydown"))
  //  return ctx_key_down (ctx, keyval, string + 8, time);
  //else if (!ctx_strcmp (event_type, "keyup"))
  //  return ctx_key_up (ctx, keyval, string + 6, time);

  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_PRESS);
  CtxEvent event = {0,};

  if (time == 0)
    time = ctx_ms (ctx);
  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_KEY_PRESS;
    event.unicode = keyval; 
    if (string)
    event.string = ctx_strdup(string);
    else
    event.string = "--";
    event.stop_propagate = 0;
    event.time = time;

    for (i = 0; i < item->cb_count; i++)
    {
      if (item->cb[i].types & (CTX_KEY_PRESS))
      {
        event.state = ctx->events.modifier_state;
        item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
        if (event.stop_propagate)
        {
          free ((void*)event.string);
          return event.stop_propagate;
        }
      }
    }
    free ((void*)event.string);
  }
  return 0;
}

CTX_EXPORT int
ctx_key_down (Ctx *ctx, unsigned int keyval,
              const char *string, uint32_t time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_DOWN);
  CtxEvent event = {0,};
  if (!string)
    string = ctx_keycode_to_keyname (0, keyval);

  if (!ctx_strcmp (string, "shift"))
  {
    ctx->events.modifier_state |= CTX_MODIFIER_STATE_SHIFT;
  }
  else if (!ctx_strcmp (string, "control"))
  {
    ctx->events.modifier_state |= CTX_MODIFIER_STATE_CONTROL;
  }
  else if (!ctx_strcmp (string, "alt"))
  {
    ctx->events.modifier_state |= CTX_MODIFIER_STATE_ALT;
  }

  if (time == 0)
    time = ctx_ms (ctx);
  if (item)
  {
    int i;
    event.ctx     = ctx;
    event.type    = CTX_KEY_DOWN;
    event.unicode = keyval; 
    event.string  = ctx_strdup(string);
    event.stop_propagate = 0;
    event.time    = time;

    for (i = 0; i < item->cb_count; i++)
    {
      if (item->cb[i].types & (CTX_KEY_DOWN))
      {
        event.state = ctx->events.modifier_state;
        item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
        if (event.stop_propagate)
        {
          free ((void*)event.string);
          return event.stop_propagate;
        }
      }
    }
    free ((void*)event.string);
  }
  return 0;
}

CTX_EXPORT int
ctx_key_up (Ctx *ctx, unsigned int keyval,
            const char *string, uint32_t time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_UP);
  CtxEvent event = {0,};
  if (!string)
    string = ctx_keycode_to_keyname (0, keyval);

  if (!ctx_strcmp (string, "shift"))
  {
    ctx->events.modifier_state &= ~(CTX_MODIFIER_STATE_SHIFT);
  }
  else if (!ctx_strcmp (string, "control"))
  {
    ctx->events.modifier_state &= ~(CTX_MODIFIER_STATE_CONTROL);
  }
  else if (!ctx_strcmp (string, "alt"))
  {
    ctx->events.modifier_state &= ~(CTX_MODIFIER_STATE_ALT);
  }

  if (time == 0)
    time = ctx_ms (ctx);
  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_KEY_UP;
    event.unicode = keyval; 
    event.string = ctx_strdup(string);
    event.stop_propagate = 0;
    event.time = time;

    for (i = 0; i < item->cb_count; i++)
    {
      if (item->cb[i].types & (CTX_KEY_UP))
      {
        event.state = ctx->events.modifier_state;
        item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
        if (event.stop_propagate)
        {
          free ((void*)event.string);
          return event.stop_propagate;
        }
      }
    }
    free ((void*)event.string);
  }
  return 0;
}

void ctx_destroyze           (Ctx *ctx)
{
  ctx->events.frozen ++;
}

void ctx_thaw             (Ctx *ctx)
{
  ctx->events.frozen --;
}
int ctx_events_frozen (Ctx *ctx)
{
  return ctx && ctx->events.frozen;
}
void ctx_events_clear_items (Ctx *ctx)
{
  ctx_list_free (&ctx->events.items);
}

float ctx_pointer_x (Ctx *ctx)
{
  return ctx->events.pointer_x[0];
}

float ctx_pointer_y (Ctx *ctx)
{
  return ctx->events.pointer_y[0];
}

int ctx_pointer_is_down (Ctx *ctx, int no)
{
  if (no < 0 || no > CTX_MAX_DEVICES) return 0;
  return ctx->events.pointer_down[no];
}

void _ctx_debug_overlays (Ctx *ctx)
{
  CtxList *a;
  ctx_save (ctx);

  ctx_line_width (ctx, 2);
  ctx_rgba (ctx, 0,0,0.8,0.5);
  for (a = ctx->events.items; a; a = a->next)
  {
    float current_x = ctx_pointer_x (ctx);
    float current_y = ctx_pointer_y (ctx);
    CtxItem *item = a->data;
    CtxMatrix matrix = item->inv_matrix;

    _ctx_matrix_apply_transform (&matrix, &current_x, &current_y);

    if (current_x >= item->x0 && current_x < item->x1 &&
        current_y >= item->y0 && current_y < item->y1)
    {
      ctx_matrix_invert (&matrix);
      ctx_set_matrix (ctx, &matrix);
      _mrg_restore_path (ctx, item->path);
      ctx_stroke (ctx);
    }
  }
  ctx_restore (ctx);
}

void ctx_set_render_threads   (Ctx *ctx, int n_threads)
{
  // XXX
}
int ctx_get_render_threads   (Ctx *ctx)
{
  return _ctx_max_threads;
}
void ctx_set_hash_cache (Ctx *ctx, int enable_hash_cache)
{
  _ctx_enable_hash_cache = enable_hash_cache;
}
int ctx_get_hash_cache (Ctx *ctx)
{
  return _ctx_enable_hash_cache;
}

int ctx_need_redraw (Ctx *ctx)
{
  return (ctx->dirty != 0)
#if CTX_CLIENTS
    || ctx_clients_need_redraw (ctx)
#endif
    ;
}


/*
 * centralized global API for managing file descriptors that
 * wake us up, this to remove sleeping and polling
 */

#define CTX_MAX_LISTEN_FDS 128 // becomes max clients..

static int _ctx_listen_fd[CTX_MAX_LISTEN_FDS];
static int _ctx_listen_fds    = 0;
static int _ctx_listen_max_fd = 0;

void _ctx_add_listen_fd (int fd)
{
  _ctx_listen_fd[_ctx_listen_fds++]=fd;
  if (fd > _ctx_listen_max_fd)
    _ctx_listen_max_fd = fd;
}

void _ctx_remove_listen_fd (int fd)
{
  for (int i = 0; i < _ctx_listen_fds; i++)
  {
    if (_ctx_listen_fd[i] == fd)
    {
      _ctx_listen_fd[i] = _ctx_listen_fd[_ctx_listen_fds-1];
      _ctx_listen_fds--;
      return;
    }
  }
}
#ifdef EMSCRIPTEN
extern int em_in_len;
#endif

int ctx_input_pending (Ctx *ctx, int timeout)
{
  struct timeval tv;
  fd_set fdset;
  FD_ZERO (&fdset);
  for (int i = 0; i < _ctx_listen_fds; i++)
  {
    FD_SET (_ctx_listen_fd[i], &fdset);
  }
  int input_fds[5];
  int n_fds;
  ctx_get_event_fds (ctx, input_fds, &n_fds);
  for (int i = 0; i < n_fds; i++)
  {
    FD_SET (input_fds[i], &fdset);
  }
  tv.tv_sec = 0;
  tv.tv_usec = timeout;
  tv.tv_sec = timeout / 1000000;
  tv.tv_usec = timeout % 1000000;
  int retval = select (_ctx_listen_max_fd + 1, &fdset, NULL, NULL, &tv);
  if (retval == -1)
  {
    perror ("select");
    return 0;
  }
#ifdef EMSCRIPTEN
  retval += em_in_len;
#endif
  return retval;
}

void ctx_handle_events (Ctx *ctx)
{
#if CTX_CLIENTS
  ctx_clients_handle_events (ctx);
#endif
  while (ctx_get_event (ctx)){}
}


static void ctx_events_deinit (Ctx *ctx)
{
  ctx_list_free (&ctx->events.items);
  ctx->events.last_item = NULL;

  while (ctx->events.idles)
  {
    CtxIdleCb *item = ctx->events.idles->data;
    ctx_list_remove (&ctx->events.idles, item);
    if (item->destroy_notify)
      item->destroy_notify (item->destroy_data);
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
     return ctx_strdup ("");
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
        if ((int)ctx_strlen (ufb_keycodes[i].sequence) == length && ret)
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
              return ctx_strdup ("escape");
          }

        switch (fb_keyboard_match_keycode ((void*)buf, length + 1, &match))
          {
            case 1: /* unique match */
              if (!match)
                return NULL;
              return ctx_strdup (match->nick);
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
                    return ctx_strdup(ret); //XXX: simplify
                  }
                if (length == 0) /* ascii */
                  {
                    buf[1]=0;
                    strcpy (ret, (void*)buf);
                    return ctx_strdup(ret);
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
                return ctx_strdup(ret);
            }
              return NULL;
            default: /* continue */
              break;
          }
      }
    else
      return ctx_strdup("key read eek");
  return ctx_strdup("fail");
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


void ctx_queue_draw (Ctx *ctx)
{
  ctx->dirty ++;
}
