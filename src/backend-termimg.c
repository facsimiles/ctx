#include "ctx-split.h"

#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

typedef struct _CtxTermImg CtxTermImg;
struct _CtxTermImg
{
   CtxBackend backend;
   int         width;
   int         height;
   int         cols;
   int         rows;
   int         was_down;
   // we need to have the above members in that order up to here
   uint8_t    *pixels;
   Ctx        *host;
   CtxList    *lines;
};

inline static void ctx_termimg_process (Ctx        *ctx,
                                        CtxCommand *command)
{
  CtxTermImg *termimg = (void*)ctx->backend;
  /* directly forward */
  ctx_process (termimg->host, &command->entry);
}

inline static void ctx_termimg_flush (Ctx *ctx)
{
  CtxTermImg *termimg = (CtxTermImg*)ctx->backend;
  int width =  termimg->width;
  int height = termimg->height;
  if (!termimg->pixels) return;
  char *encoded = malloc (width * height * 3 * 3);
  ctx_bin2base64 (termimg->pixels, width * height * 3,
                  encoded);
  int encoded_len = strlen (encoded);

  int i = 0;

  printf ("\e[H");
  printf ("\e_Gf=24,s=%i,v=%i,t=d,a=T,m=1;\e\\", width, height);
  while (i <  encoded_len)
  {
     if (i + 4096 <  encoded_len)
     {
       printf  ("\e_Gm=1;");
     }
     else
     {
       printf  ("\e_Gm=0;");
     }
     for (int n = 0; n < 4000 && i < encoded_len; n++)
     {
       printf ("%c", encoded[i]);
       i++;
     }
     printf ("\e\\");
  }
  free (encoded);
  
  fflush (NULL);
}

void ctx_termimg_free (CtxTermImg *termimg)
{
  while (termimg->lines)
  {
    free (termimg->lines->data);
    ctx_list_remove (&termimg->lines, termimg->lines->data);
  }
  printf ("\e[?25h"); // cursor on
  nc_at_exit ();
  free (termimg->pixels);
  ctx_free (termimg->host);
  free (termimg);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

Ctx *ctx_new_termimg (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  fprintf (stdout, "\e[?1049h");
  fprintf (stdout, "\e[?25l"); // cursor off
  CtxTermImg *termimg = (CtxTermImg*)calloc (sizeof (CtxTermImg), 1);
  CtxBackend *backend = (void*)termimg;


  int maxwidth = ctx_terminal_width ();

  int colwidth = maxwidth/ctx_terminal_cols ();
  maxwidth-=colwidth;

  int maxheight = ctx_terminal_height ();
  if (width <= 0 || height <= 0)
  {
    width  = maxwidth;
    height = maxheight;
  }
  if (width > maxwidth) width = maxwidth;
  if (height > maxheight) height = maxheight;
  termimg->width  = width;
  termimg->height = height;
  termimg->lines = 0;
  termimg->pixels = (uint8_t*)malloc (width * height * 3);
  termimg->host = ctx_new_for_framebuffer (termimg->pixels,
                                           width, height,
                                           width * 3, CTX_FORMAT_RGB8);
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_backend (ctx, termimg);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, 14.0f);

  backend->ctx = ctx;
  backend->backend = "termimg";
  backend->process = ctx_termimg_process;
  backend->flush = ctx_termimg_flush;
  backend->free  = (void(*)(void*))ctx_termimg_free;
  backend->consume_events = ctx_nct_consume_events;
  backend->get_event_fds = (void*) ctx_stdin_get_event_fds;
#endif

  return ctx;
}

#endif
