#include "ctx-split.h"

#if CTX_EVENTS

static int ctx_find_largest_matching_substring
 (const char *X, const char *Y, int m, int n, int *offsetY, int *offsetX) 
{ 
  int longest_common_suffix[2][n+1];
  int best_length = 0;
  for (int i=0; i<=m; i++)
  {
    for (int j=0; j<=n; j++)
    {
      if (i == 0 || j == 0 || !(X[i-1] == Y[j-1]))
      {
        longest_common_suffix[i%2][j] = 0;
      }
      else
      {
          longest_common_suffix[i%2][j] = longest_common_suffix[(i-1)%2][j-1] + 1;
          if (best_length < longest_common_suffix[i%2][j])
          {
            best_length = longest_common_suffix[i%2][j];
            if (offsetY) *offsetY = j - best_length;
            if (offsetX) *offsetX = i - best_length;
          }
      }
    }
  }
  return best_length;
} 

typedef struct CtxSpan {
  int from_prev;
  int start;
  int length;
} CtxSpan;

#define CHUNK_SIZE 32
#define MIN_MATCH  7        // minimum match length to be encoded
#define WINDOW_PADDING 16   // look-aside amount

#if 0
static void _dassert(int line, int condition, const char *str, int foo, int bar, int baz)
{
  if (!condition)
  {
    FILE *f = fopen ("/tmp/cdebug", "a");
    fprintf (f, "%i: %s    %i %i %i\n", line, str, foo, bar, baz);
    fclose (f);
  }
}
#define dassert(cond, foo, bar, baz) _dassert(__LINE__, cond, #cond, foo, bar ,baz)
#endif
#define dassert(cond, foo, bar, baz)

/* XXX repeated substring matching is slow, we'll be
 * better off with a hash-table with linked lists of
 * matching 3-4 characters in previous.. or even
 * a naive approach that expects rough alignment..
 */
static char *encode_in_terms_of_previous (
                const char *src,  int src_len,
                const char *prev, int prev_len,
                int *out_len,
                int max_ticks)
{
  CtxString *string = ctx_string_new ("");
  CtxList *encoded_list = NULL;

  /* TODO : make expected position offset in prev slide based on
   * matches and not be constant */

  long ticks_start = ctx_ticks ();
  int start = 0;
  int length = CHUNK_SIZE;
  for (start = 0; start < src_len; start += length)
  {
    CtxSpan *span = calloc (sizeof (CtxSpan), 1);
    span->start = start;
    if (start + length > src_len)
      span->length = src_len - start;
    else
      span->length = length;
    span->from_prev = 0;
    ctx_list_append (&encoded_list, span);
  }

  for (CtxList *l = encoded_list; l; l = l->next)
  {
    CtxSpan *span = l->data;
    if (!span->from_prev)
    {
      if (span->length >= MIN_MATCH)
      {
         int prev_pos = 0;
         int curr_pos = 0;
         assert(1);
#if 0
         int prev_start =  0;
         int prev_window_length = prev_len;
#else
         int window_padding = WINDOW_PADDING;
         int prev_start = span->start - window_padding;
         if (prev_start < 0)
           prev_start = 0;

         dassert(span->start>=0 , 0,0,0);

         int prev_window_length = prev_len - prev_start;
         if (prev_window_length > span->length + window_padding * 2 + span->start)
           prev_window_length = span->length + window_padding * 2 + span->start;
#endif
         int match_len = 0;
         if (prev_window_length > 0)
           match_len = ctx_find_largest_matching_substring(prev + prev_start, src + span->start, prev_window_length, span->length, &curr_pos, &prev_pos);
#if 1
         prev_pos += prev_start;
#endif

         if (match_len >= MIN_MATCH)
         {
            int start  = span->start;
            int length = span->length;

            span->from_prev = 1;
            span->start     = prev_pos;
            span->length    = match_len;
            dassert (span->start >= 0, prev_pos, prev_start, span->start);
            dassert (span->length > 0, prev_pos, prev_start, span->length);

            if (curr_pos)
            {
              CtxSpan *prev = calloc (sizeof (CtxSpan), 1);
              prev->start = start;
              prev->length =  curr_pos;
            dassert (prev->start >= 0, prev_pos, prev_start, prev->start);
            dassert (prev->length > 0, prev_pos, prev_start, prev->length);
              prev->from_prev = 0;
              ctx_list_insert_before (&encoded_list, l, prev);
            }


            if (match_len + curr_pos < start + length)
            {
              CtxSpan *next = calloc (sizeof (CtxSpan), 1);
              next->start = start + curr_pos + match_len;
              next->length = (start + length) - next->start;
            dassert (next->start >= 0, prev_pos, prev_start, next->start);
      //    dassert (next->length > 0, prev_pos, prev_start, next->length);
              next->from_prev = 0;
              if (next->length)
              {
                if (l->next)
                  ctx_list_insert_before (&encoded_list, l->next, next);
                else
                  ctx_list_append (&encoded_list, next);
              }
              else
                free (next);
            }

            if (curr_pos) // step one item back for forloop
            {
              CtxList *tmp = encoded_list;
              int found = 0;
              while (!found && tmp && tmp->next)
              {
                if (tmp->next == l)
                {
                  l = tmp;
                  break;
                }
                tmp = tmp->next;
              }
            }
         }
      }
    }

    if (ctx_ticks ()-ticks_start > (unsigned long)max_ticks)
      break;
  }

  /* merge adjecant prev span references  */
  {
    for (CtxList *l = encoded_list; l; l = l->next)
    {
      CtxSpan *span = l->data;
again:
      if (l->next)
      {
        CtxSpan *next_span = l->next->data;
        if (span->from_prev && next_span->from_prev &&
            span->start + span->length == 
            next_span->start)
        {
           span->length += next_span->length;
           ctx_list_remove (&encoded_list, next_span);
           goto again;
        }
      }
    }
  }

  while (encoded_list)
  {
    CtxSpan *span = encoded_list->data;
    if (span->from_prev)
    {
      char ref[128];
      sprintf (ref, "%c%i %i%c", CTX_CODEC_CHAR, span->start, span->length, CTX_CODEC_CHAR);
      ctx_string_append_data (string, ref, strlen(ref));
    }
    else
    {
      for (int i = span->start; i< span->start+span->length; i++)
      {
        if (src[i] == CTX_CODEC_CHAR)
        {
          char bytes[2]={CTX_CODEC_CHAR, CTX_CODEC_CHAR};
          ctx_string_append_data (string, bytes, 2);
        }
        else
        {
          ctx_string_append_data (string, &src[i], 1);
        }
      }
    }
    free (span);
    ctx_list_remove (&encoded_list, span);
  }

  char *ret = string->str;
  if (out_len) *out_len = string->length;
  ctx_string_free (string, 0);
  return ret;
}

#if 0 // for documentation/reference purposes
static char *decode_ctx (const char *encoded, int enc_len, const char *prev, int prev_len, int *out_len)
{
  CtxString *string = ctx_string_new ("");
  char reference[32]="";
  int ref_len = 0;
  int in_ref = 0;
  for (int i = 0; i < enc_len; i++)
  {
    if (encoded[i] == CTX_CODEC_CHAR)
    {
      if (!in_ref)
      {
        in_ref = 1;
      }
      else
      {
        int start = atoi (reference);
        int len = 0;
        if (strchr (reference, ' '))
          len = atoi (strchr (reference, ' ')+1);

        if (start < 0)start = 0;
        if (start >= prev_len)start = prev_len-1;
        if (len + start > prev_len)
          len = prev_len - start;

        if (start == 0 && len == 0)
          ctx_string_append_byte (string, CTX_CODEC_CHAR);
        else
          ctx_string_append_data (string, prev + start, len);
        ref_len = 0;
        in_ref = 0;
      }
    }
    else
    {
      if (in_ref)
      {
        if (ref_len < 16)
        {
          reference[ref_len++] = encoded[i];
          reference[ref_len] = 0;
        }
      }
      else
      ctx_string_append_data (string, &encoded[i], 1);
    }
  }
  char *ret = string->str;
  if (out_len) *out_len = string->length;
  ctx_string_free (string, 0);
  return ret;
}
#endif

#define CTX_START_STRING "U\n"  // or " reset "
#define CTX_END_STRING   "\nX"  // or "\ndone"
#define CTX_END_STRING2  "\n"

int ctx_frame_ack = -1;
static char *prev_frame_contents = NULL;
static int   prev_frame_len = 0;

static void ctx_ctx_flush (CtxCtx *ctxctx)
{
#if 0
  FILE *debug = fopen ("/tmp/ctx-debug", "a");
  fprintf (debug, "------\n");
#endif

  if (ctx_native_events)
    fprintf (stdout, "\e[?201h");
  fprintf (stdout, "\e[H\e[?25l\e[?200h");
#if 1
  fprintf (stdout, CTX_START_STRING);
  ctx_render_stream (ctxctx->backend.ctx, stdout, 0);
  fprintf (stdout, CTX_END_STRING);
#else
  {
    int cur_frame_len = 0;
    char *rest = ctx_render_string (ctxctx->ctx, 0, &cur_frame_len);
    char *cur_frame_contents = malloc (cur_frame_len + strlen(CTX_START_STRING) + strlen (CTX_END_STRING) + 1);

    cur_frame_contents[0]=0;
    strcat (cur_frame_contents, CTX_START_STRING);
    strcat (cur_frame_contents, rest);
    strcat (cur_frame_contents, CTX_END_STRING);
    free (rest);
    cur_frame_len += strlen (CTX_START_STRING) + strlen (CTX_END_STRING);

    if (prev_frame_contents && 0)  // XXX : 
    {
      char *encoded;
      int encoded_len = 0;
      //uint64_t ticks_start = ctx_ticks ();

      encoded = encode_in_terms_of_previous (cur_frame_contents, cur_frame_len, prev_frame_contents, prev_frame_len, &encoded_len, 1000 * 10);
//    encoded = strdup (cur_frame_contents);
//    encoded_len = strlen (encoded);
      //uint64_t ticks_end = ctx_ticks ();

      fwrite (encoded, encoded_len, 1, stdout);
//    fwrite (encoded, cur_frame_len, 1, stdout);
#if 0
      fprintf (debug, "---prev-frame(%i)\n%s", (int)strlen(prev_frame_contents), prev_frame_contents);
      fprintf (debug, "---cur-frame(%i)\n%s", (int)strlen(cur_frame_contents), cur_frame_contents);
      fprintf (debug, "---encoded(%.4f %i)---\n%s--------\n",
                      (ticks_end-ticks_start)/1000.0,
                      (int)strlen(encoded), encoded);
#endif
      free (encoded);
    }
    else
    {
      fwrite (cur_frame_contents, cur_frame_len, 1, stdout);
    }

    if (prev_frame_contents)
      free (prev_frame_contents);
    prev_frame_contents = cur_frame_contents;
    prev_frame_len = cur_frame_len;
  }
#endif
  fprintf (stdout, CTX_END_STRING2);
#if 0
    fclose (debug);
#endif

#if CTX_SYNC_FRAMES
  fprintf (stdout, "\e[5n");
  fflush (stdout);

  ctx_frame_ack = 0;
  do {
     ctx_consume_events (ctxctx->backend.ctx);
  } while (ctx_frame_ack != 1);
#else
  fflush (stdout);
#endif
}

void ctx_ctx_free (CtxCtx *ctx)
{
  nc_at_exit ();
  free (ctx);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

void ctx_ctx_consume_events (Ctx *ctx)
{
  //int ix, iy;
  CtxCtx *ctxctx = (CtxCtx*)ctx->backend;
  const char *event = NULL;
#if CTX_AUDIO
  ctx_ctx_pcm (ctx);
#endif
  assert (ctx_native_events);

#if 1
    { /* XXX : this is a work-around for signals not working properly, we are polling the
         size with an ioctl per consume-events
         */
      struct winsize ws;
      ioctl(0,TIOCGWINSZ,&ws);
      ctxctx->cols = ws.ws_col;
      ctxctx->rows = ws.ws_row;
      ctx_set_size (ctx, ws.ws_xpixel, ws.ws_ypixel);
    }
#endif
    //char *cmd = ctx_strdup_printf ("touch /tmp/ctx-%ix%i", ctxctx->width, ctxctx->height);
    //system (cmd);
    //free (cmd);

  if (ctx_native_events)
    {

      float x = 0, y = 0;
      int b = 0;
      char event_type[128]="";
      event = ctx_native_get_event (ctx, 1000/120);

      if (event)
      {
      sscanf (event, "%s %f %f %i", event_type, &x, &y, &b);
      if (!strcmp (event_type, "idle"))
      {
      }
      else if (!strcmp (event_type, "pp"))
      {
        ctx_pointer_press (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "pd")||
               !strcmp (event_type, "pm"))
      {
        ctx_pointer_motion (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "pr"))
      {
        ctx_pointer_release (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "message"))
      {
        ctx_incoming_message (ctx, event + strlen ("message"), 0);
      } else if (!strcmp (event, "size-changed"))
      {
        fprintf (stdout, "\e[H\e[2J\e[?25l");
        ctxctx->cols = ctx_terminal_cols ();
        ctxctx->rows = ctx_terminal_rows ();

        //system ("touch /tmp/ctx-abc");

        ctx_set_size (ctx, ctx_terminal_width(), ctx_terminal_height());

        if (prev_frame_contents)
          free (prev_frame_contents);
        prev_frame_contents = NULL;
        prev_frame_len = 0;
        ctx_set_dirty (ctx, 1);

      //   ctx_key_press(ctx,0,"size-changed",0);
      }
      else if (!strcmp (event_type, "keyup"))
      {
        char buf[4]={ x, 0 };
        ctx_key_up (ctx, (int)x, buf, 0);
      }
      else if (!strcmp (event_type, "keydown"))
      {
        char buf[4]={ x, 0 };
        ctx_key_down (ctx, (int)x, buf, 0);
      }
      else
      {
        ctx_key_press (ctx, 0, event, 0);
      }
      }
    }
}

Ctx *ctx_new_ctx (int width, int height)
{
  float font_size = 12.0;
  Ctx    *ctx    = ctx_new ();
  CtxCtx *ctxctx = (CtxCtx*)calloc (sizeof (CtxCtx), 1);
  CtxBackend *backend = (CtxBackend*)ctxctx;
  fprintf (stdout, "\e[?1049h");
  fflush (stdout);
  //fprintf (stderr, "\e[H");
  //fprintf (stderr, "\e[2J");
  ctx_native_events = 1;
  if (width <= 0 || height <= 0)
  {
    ctxctx->cols = ctx_terminal_cols ();
    ctxctx->rows = ctx_terminal_rows ();
    width  = ctx->events.width  = ctx_terminal_width ();
    height = ctx->events.height = ctx_terminal_height ();
    font_size = height / ctxctx->rows;
    ctx_font_size (ctx, font_size);
  }
  else
  {
    ctx->events.width  = width;
    ctx->events.height = height;
    ctxctx->cols   = width / 80;
    ctxctx->rows   = height / 24;
  }
  backend->ctx = ctx;
  if (!ctx_native_events)
    _ctx_mouse (ctx, NC_MOUSE_DRAG);
  backend->flush = (void(*)(void *))ctx_ctx_flush;
  backend->free  = (void(*)(void *))ctx_ctx_free;
  backend->consume_events = (void(*)(void *))ctx_ctx_consume_events;
  ctx_set_backend (ctx, ctxctx);
  ctx_set_size (ctx, width, height);
  return ctx;
}

void ctx_ctx_pcm (Ctx *ctx);


#endif
