#include "ctx-split.h"

#if CTX_EVENTS

#include <fcntl.h>
#include <sys/ioctl.h>

typedef struct _CtxBraille CtxBraille;
struct _CtxBraille
{
   void (*render) (void *braille, CtxCommand *command);
   void (*reset)  (void *braille);
   void (*flush)  (void *braille);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)   (void *braille);
   Ctx     *ctx;
   int      width;
   int      height;
   int      cols;
   int      rows;
   int      was_down;
   uint8_t *pixels;
   Ctx     *host;
};

static inline int _ctx_rgba8_manhattan_diff (const uint8_t *a, const uint8_t *b)
{
  int c;
  int diff = 0;
  for (c = 0; c<3;c++)
    diff += ctx_pow2(a[c]-b[c]);
  return diff;
}


static inline void _ctx_utf8_output_buf (uint8_t *pixels,
                          int format,
                          int width,
                          int height,
                          int stride,
                          int reverse)
{
  const char *utf8_gray_scale[]= {" ","░","▒","▓","█","█", NULL};
  int no = 0;
  printf ("\e[?25l"); // cursor off
  switch (format)
    {
      case CTX_FORMAT_GRAY2:
        {
          for (int y= 0; y < height; y++)
            {
              no = y * stride;
              for (int x = 0; x < width; x++)
                {
                  int val4= (pixels[no] & (3 << ( (x % 4) *2) ) ) >> ( (x%4) *2);
                  int val = (int) CTX_CLAMP (5.0 * val4 / 3.0, 0, 5);
                  if (!reverse)
                  { val = 5-val; }
                  printf ("%s", utf8_gray_scale[val]);
                  if ( (x % 4) == 3)
                    { no++; }
                }
              printf ("\n");
            }
        }
        break;
      case CTX_FORMAT_GRAY1:
        for (int row = 0; row < height/4; row++)
          {
            for (int col = 0; col < width /2; col++)
              {
                int unicode = 0;
                int bitno = 0;
                for (int x = 0; x < 2; x++)
                  for (int y = 0; y < 3; y++)
                    {
                      int no = (row * 4 + y) * stride + (col*2+x) /8;
                      int set = pixels[no] & (1<< ( (col * 2 + x) % 8) );
                      if (reverse) { set = !set; }
                      if (set)
                        { unicode |=  (1<< (bitno) ); }
                      bitno++;
                    }
                {
                  int x = 0;
                  int y = 3;
                  int no = (row * 4 + y) * stride + (col*2+x) /8;
                  int setA = pixels[no] & (1<< ( (col * 2 + x) % 8) );
                  no = (row * 4 + y) * stride + (col*2+x+1) /8;
                  int setB = pixels[no] & (1<< (   (col * 2 + x + 1) % 8) );
                  if (reverse) { setA = !setA; }
                  if (reverse) { setB = !setB; }
                  if (setA != 0 && setB==0)
                    { unicode += 0x2840; }
                  else if (setA == 0 && setB)
                    { unicode += 0x2880; }
                  else if ( (setA != 0) && (setB != 0) )
                    { unicode += 0x28C0; }
                  else
                    { unicode += 0x2800; }
                  uint8_t utf8[5];
                  utf8[ctx_unichar_to_utf8 (unicode, utf8)]=0;
                  printf ("%s", utf8);
                }
              }
            printf ("\n");
          }
        break;
      case CTX_FORMAT_RGBA8:
        {
        for (int row = 0; row < height/4; row++)
          {
            for (int col = 0; col < width /2; col++)
              {
                int unicode = 0;
                int bitno = 0;

                uint8_t rgba2[4] = {0,0,0,255};
                uint8_t rgba1[4] = {0,0,0,255};
                int     rgbasum[4] = {0,};
                int     col_count = 0;

                for (int xi = 0; xi < 2; xi++)
                  for (int yi = 0; yi < 4; yi++)
                      {
                        int noi = (row * 4 + yi) * stride + (col*2+xi) * 4;
                        int diff = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba2);
                        if (diff > 32*32)
                        {
                          for (int c = 0; c < 3; c++)
                          {
                            rgbasum[c] += pixels[noi+c];
                          }
                          col_count++;
                        }
                      }
                if (col_count)
                for (int c = 0; c < 3; c++)
                {
                  rgba1[c] = rgbasum[c] / col_count;
                }



                // to determine color .. find two most different
                // colors in set.. and threshold between them..
                // even better dither between them.
                //
  printf ("\e[38;2;%i;%i;%im", rgba1[0], rgba1[1], rgba1[2]);
  //printf ("\e[48;2;%i;%i;%im", rgba2[0], rgba2[1], rgba2[2]);

                for (int x = 0; x < 2; x++)
                  for (int y = 0; y < 3; y++)
                    {
                      int no = (row * 4 + y) * stride + (col*2+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba1)< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba2))

                      int set = CHECK_IS_SET;
                      if (reverse) { set = !set; }
                      if (set)
                        { unicode |=  (1<< (bitno) ); }
                      bitno++;
                    }
                {
                  int x = 0;
                  int y = 3;
                  int no = (row * 4 + y) * stride + (col*2+x) * 4;
                  int setA = CHECK_IS_SET;
                  no = (row * 4 + y) * stride + (col*2+x+1) * 4;
                  int setB = CHECK_IS_SET;
#undef CHECK_IS_SET
                  if (reverse) { setA = !setA; }
                  if (reverse) { setB = !setB; }
                  if (setA != 0 && setB==0)
                    { unicode += 0x2840; }
                  else if (setA == 0 && setB)
                    { unicode += 0x2880; }
                  else if ( (setA != 0) && (setB != 0) )
                    { unicode += 0x28C0; }
                  else
                    { unicode += 0x2800; }
                  uint8_t utf8[5];
                  utf8[ctx_unichar_to_utf8 (unicode, utf8)]=0;
                  printf ("%s", utf8);
                }
              }
            printf ("\n\r");
          }
          printf ("\e[38;2;%i;%i;%im", 255,255,255);
        }
        break;

      case CTX_FORMAT_GRAY4:
        {
          int no = 0;
          for (int y= 0; y < height; y++)
            {
              no = y * stride;
              for (int x = 0; x < width; x++)
                {
                  int val = (pixels[no] & (15 << ( (x % 2) *4) ) ) >> ( (x%2) *4);
                  val = val * 6 / 16;
                  if (reverse) { val = 5-val; }
                  val = CTX_CLAMP (val, 0, 4);
                  printf ("%s", utf8_gray_scale[val]);
                  if (x % 2 == 1)
                    { no++; }
                }
              printf ("\n");
            }
        }
        break;
      case CTX_FORMAT_CMYK8:
        {
          for (int c = 0; c < 4; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=4)
                    {
                      int val = (int) CTX_CLAMP (pixels[no+c]/255.0*6.0, 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
        break;
      case CTX_FORMAT_RGB8:
        {
          for (int c = 0; c < 3; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=3)
                    {
                      int val = (int) CTX_CLAMP (pixels[no+c]/255.0*6.0, 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
        break;
      case CTX_FORMAT_CMYKAF:
        {
          for (int c = 0; c < 5; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=5)
                    {
                      int val = (int) CTX_CLAMP ( (pixels[no+c]*6.0), 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
    }
  printf ("\e[?25h"); // cursor on
}

inline static void ctx_braille_flush (CtxBraille *braille)
{
  int width =  braille->width;
  int height = braille->height;
  ctx_render_ctx (braille->ctx, braille->host);
  printf ("\e[H");
  _ctx_utf8_output_buf (braille->pixels,
                        CTX_FORMAT_RGBA8,
                        width, height, width * 4, 0);
#if CTX_BRAILLE_TEXT
  CtxRasterizer *rasterizer = (CtxRasterizer*)(braille->host->renderer);
  // XXX instead sort and inject along with braille
  for (CtxList *l = rasterizer->glyphs; l; l = l->next)
  {
      CtxTermGlyph *glyph = l->data;
      printf ("\e[0m\e[%i;%iH%c", glyph->row, glyph->col, glyph->unichar);
      free (glyph);
  }
  while (rasterizer->glyphs)
    ctx_list_remove (&rasterizer->glyphs, rasterizer->glyphs->data);
#endif
}

void ctx_braille_free (CtxBraille *braille)
{
  nc_at_exit ();
  free (braille->pixels);
  ctx_free (braille->host);
  free (braille);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

int ctx_renderer_is_braille (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_braille_free)
          return 1;
  return 0;
}

Ctx *ctx_new_braille (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  CtxBraille *braille = (CtxBraille*)calloc (sizeof (CtxBraille), 1);
  int maxwidth = ctx_terminal_cols  () * 2;
  int maxheight = (ctx_terminal_rows ()-1) * 4;
  if (width <= 0 || height <= 0)
  {
    width = maxwidth;
    height = maxheight;
  }
  if (width > maxwidth) width = maxwidth;
  if (height > maxheight) height = maxheight;
  braille->ctx = ctx;
  braille->width  = width;
  braille->height = height;
  braille->cols = (width + 1) / 2;
  braille->rows = (height + 3) / 4;
  braille->pixels = (uint8_t*)malloc (width * height * 4);
  braille->host = ctx_new_for_framebuffer (braille->pixels,
                                           width, height,
                                           width * 4, CTX_FORMAT_RGBA8);
#if CTX_BRAILLE_TEXT
  ((CtxRasterizer*)braille->host->renderer)->term_glyphs=1;
#endif
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, braille);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, 4.0f);
  braille->flush = (void(*)(void*))ctx_braille_flush;
  braille->free  = (void(*)(void*))ctx_braille_free;
#endif
  return ctx;
}

#endif
