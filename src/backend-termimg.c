#include "ctx-split.h"

#if CTX_EVENTS

#include <fcntl.h>
#include <sys/ioctl.h>

typedef struct _CtxTermImg CtxTermImg;
struct _CtxTermImg
{
   void (*render)         (void *termimg, CtxCommand *command);
   void (*reset)          (void *termimg);
   void (*flush)          (void *termimg);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard)  (void *ctxctx, const char *text);
   void (*free)           (void *termimg);
   Ctx      *ctx;
   int       width;
   int       height;
   int       was_down;
   uint8_t  *pixels;
   Ctx      *host;
   CtxList  *lines;
};

inline static void ctx_termimg_render (void       *ctx,
                                       CtxCommand *command)
{
  CtxTermImg *termimg = (void*)ctx;
  /* directly forward */
  ctx_process (termimg->host, &command->entry);
}

inline static void ctx_termimg_flush (CtxTermImg *termimg)
{
  int width =  termimg->width;
  int height = termimg->height;
  if (!termimg->pixels) return;
  uint8_t *encoded = malloc (width * height * 3 * 3);
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
     for (int n = 0; n < 4096 && i < encoded_len; n++)
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

int ctx_renderer_is_termimg (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_termimg_free)
          return 1;
  return 0;
}

Ctx *ctx_new_termimg (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  fprintf (stdout, "\e[?1049h");
  fprintf (stdout, "\e[?25l"); // cursor off
  CtxTermImg *termimg = (CtxTermImg*)calloc (sizeof (CtxTermImg), 1);


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
  termimg->ctx = ctx;
  termimg->width  = width;
  termimg->height = height;
  termimg->lines = 0;
  termimg->pixels = (uint8_t*)malloc (width * height * 3);
  termimg->host = ctx_new_for_framebuffer (termimg->pixels,
                                           width, height,
                                           width * 3, CTX_FORMAT_RGB8);
  //_ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, termimg);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, 14.0f);
  termimg->render = ctx_termimg_render;
  termimg->flush = (void(*)(void*))ctx_termimg_flush;
  termimg->free  = (void(*)(void*))ctx_termimg_free;
#endif

  return ctx;
}

#endif
