#include "ctx-split.h"

#if CTX_EVENTS

#include <fcntl.h>
#include <sys/ioctl.h>

typedef struct CtxTermCell
{
  char    utf8[5];
  uint8_t fg[4];
  uint8_t bg[4];

  char    prev_utf8[5];
  uint8_t prev_fg[4];
  uint8_t prev_bg[4];
} CtxTermCell;

typedef struct CtxTermLine
{
  CtxTermCell *cells;
  int maxcol;
  int size;
} CtxTermLine;

typedef struct _CtxTerm CtxTerm;
struct _CtxTerm
{
   void (*render) (void *term, CtxCommand *command);
   void (*reset)  (void *term);
   void (*flush)  (void *term);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)   (void *term);
   Ctx      *ctx;
   int       width;
   int       height;
   int       cols;
   int       rows;
   int       was_down;
   uint8_t  *pixels;
   Ctx      *host;
   CtxList  *lines;
};

void ctx_term_set (CtxTerm *term,
                      int col, int row, const char *utf8,
                      uint8_t *fg, uint8_t *bg)
{
  while (ctx_list_length (term->lines) < row)
  {
    ctx_list_append (&term->lines, calloc (sizeof (CtxTermLine), 1));
  }
  CtxTermLine *line = ctx_list_nth_data (term->lines, row-1);
  assert (line);
  if (line->size < col)
  {
     int new_size = ((col + 128)/128)*128;
     line->cells = realloc (line->cells, sizeof (CtxTermCell) * new_size);
     line->size = new_size;
  }
  if (col > line->maxcol) line->maxcol = col;
  strncpy (line->cells[col-1].utf8, (char*)utf8, 4);
  memcpy  (line->cells[col-1].fg, fg, 3);
  memcpy  (line->cells[col-1].bg, bg, 3);
}

void ctx_term_scanout (CtxTerm *term)
{
  int row = 1;
  printf ("\e[H");
//  printf ("\e[?25l");
  printf ("\e[0m");
  uint8_t rgba_bg[4]={0,0,0,0};
  uint8_t rgba_fg[4]={255,255,255,255};
  for (CtxList *l = term->lines; l; l = l->next)
  {
    CtxTermLine *line = l->data;
    for (int col = 1; col < line->maxcol; col++)
    {
      CtxTermCell *cell = &line->cells[col-1];

      if (strcmp(cell->utf8, cell->prev_utf8) ||
          memcmp(cell->fg, cell->prev_fg, 3) ||
          memcmp(cell->bg, cell->prev_bg, 3))
      {
        if (memcmp (&cell->fg[0],  &rgba_fg[0], 3))
        {
          memcpy (&rgba_fg[0], &cell->fg[0], 3);
          printf ("\e[38;2;%i;%i;%im", rgba_fg[0], rgba_fg[1], rgba_fg[2]);
        }
        if (memcmp (&cell->bg[0],  &rgba_bg[0], 3))
        {
          memcpy (&rgba_bg[0], &cell->bg[0], 3);
          printf ("\e[48;2;%i;%i;%im", rgba_bg[0], rgba_bg[1], rgba_bg[2]);
        }
        printf ("%s", cell->utf8);
      }
      else
      {
        // TODO: accumulate succesive such, and compress them
        // into one
        printf ("\e[C");
      }
      strcpy (cell->prev_utf8, cell->utf8);
    }
    row ++;
    printf ("\n\r");
  }
  printf ("\e[0m");
  //printf ("\e[?25h");
  //
}

static inline int _ctx_rgba8_manhattan_diff (const uint8_t *a, const uint8_t *b)
{
  int c;
  int diff = 0;
  for (c = 0; c<3;c++)
    diff += ctx_pow2(a[c]-b[c]);
  return sqrtf(diff);
}

static inline void
_ctx_utf8_output_buf (uint8_t *pixels,
                      int      format,
                      int      width,
                      int      height,
                      int      stride,
                      int      reverse,
                      CtxTerm *term)
{
  const char *utf8_gray_scale[]= {" ","░","▒","▓","█","█", NULL};
  int no = 0;
  assert (term);
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
                  char utf8[5];
                  utf8[ctx_unichar_to_utf8 (unicode, (uint8_t*)utf8)]=0;
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
  printf ("\e[38;2;%i;%i;%im", rgba1[0], rgba1[1], rgba1[2]);

                int pixels_set = 0;
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
                        { unicode |=  (1<< (bitno) ); 
                          pixels_set ++; 
                        }
                      bitno++;
                    }
                {
                  int x = 0;
                  int y = 3;
                  int no = (row * 4 + y) * stride + (col*2+x) * 4;
                  int setA = CHECK_IS_SET;
                  no = (row * 4 + y) * stride + (col*2+x+1) * 4;
                  int setB = CHECK_IS_SET;

                  pixels_set += setA;
                  pixels_set += setB;
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
                  char utf8[5];
                  utf8[ctx_unichar_to_utf8 (unicode, (uint8_t*)utf8)]=0;
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

static void ctx_term_output_buf (uint8_t *pixels,
                          int format,
                          int width,
                          int height,
                          int stride,
                          int reverse,
                          CtxTerm *term)
{
  //reverse = !reverse;
  const char *sextants[]={
   " ","🬀","🬁","🬂","🬃","🬄","🬅","🬆","🬇","🬈","🬉","🬊","🬋","🬌","🬍","🬎","🬏","🬐","🬑","🬒","🬓","▌","🬔","🬕","🬖","🬗","🬘","🬙","🬚","🬛","🬜","🬝","🬞","🬟","🬠","🬡","🬢","🬣","🬤","🬥","🬦","🬧","▐","🬨","🬩","🬪","🬫","🬬","🬭","🬮","🬯","🬰","🬱","🬲","🬳","🬴","🬵","🬶","🬷","🬸","🬹","🬺","🬻","█"
  };
  switch (format)
    {
      case CTX_FORMAT_RGBA8:
        {
        for (int row = 0; row < height/3; row++)
          {
            for (int col = 0; col < width /2; col++)
              {
                int     unicode = 0;
                int     bitno = 0;
                uint8_t rgba_black[4] = {0,0,0,255};
                uint8_t rgba[2][4] = {
                                   {255,255,255,255},
                                   {0,0,0,255}};
                int     col_count = 0;

                int i = 0;

                int  rgbasum[2][4] = {0,};
                int  sumcount[2];

                for (int iters = 0; iters < 2; iters++)
                {
                        i= 0;
                for (int i = 0; i < 4; i ++)
                   rgbasum[0][i] = rgbasum[1][i]=0;
                sumcount[0] = sumcount[1] = 0;

                for (int yi = 0; yi < 3; yi++)
                  for (int xi = 0; xi < 2; xi++, i++)
                      {
                        int noi = (row * 3 + yi) * stride + (col*2+xi) * 4;

                        int diff1 = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[0]);
                        int diff2 = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[1]);
                        int cluster = 0;
                        if (diff1 <= diff2)
                          cluster = 0;
                        else
                          cluster = 1;
                        sumcount[cluster]++;
                        for (int c = 0; c < 3; c++)
                          rgbasum[cluster][c] += pixels[noi+c];
                      }


                if (sumcount[0])
                for (int c = 0; c < 3; c++)
                {
                  rgba[0][c] = rgbasum[0][c] / sumcount[0];
                }
                if (sumcount[1])
                for (int c = 0; c < 3; c++)
                {
                  rgba[1][c] = rgbasum[1][c] / sumcount[1];
                }
                }

                int pixels_set = 0;
                for (int y = 0; y < 3; y++)
                  for (int x = 0; x < 2; x++)
                    {
                      int no = (row * 3 + y) * stride + (col*2+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                      int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                      if (reverse) { set = !set; }
                      if (set)
                        { unicode |=  (1<< (bitno) ); 
                          pixels_set ++; 
                        }
                      bitno++;
                    }
                 ctx_term_set (term, col +1, row + 1, sextants[unicode],
                               rgba[0], rgba[1]);

              }
          }
        }
        break;
    }
  return;
  switch (format)
    {
      case CTX_FORMAT_RGBA8:
        {
        for (int row = 0; row < height/4; row++)
          {
            for (int col = 0; col < width /2; col++)
              {
                int     unicode = 0;
                int     bitno = 0;
                uint8_t rgba_black[4] = {0,0,0,255};
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
                int pixels_set = 0;
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
                        { unicode |=  (1<< (bitno) ); 
                          pixels_set ++; 
                        }
                      bitno++;
                    }
                {
                  int x = 0;
                  int y = 3;
                  int no = (row * 4 + y) * stride + (col*2+x) * 4;
                  int setA = CHECK_IS_SET;
                  no = (row * 4 + y) * stride + (col*2+x+1) * 4;
                  int setB = CHECK_IS_SET;

                  pixels_set += setA;
                  pixels_set += setB;
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
                  char utf8[5];
                  utf8[ctx_unichar_to_utf8 (unicode, (uint8_t*)utf8)]=0;
                  if (pixels_set >= 6)
                  {
                    ctx_term_set (term, col +1, row + 1, " ",
                                     rgba_black, rgba1);
                  }
                  else
                  {
                    ctx_term_set (term, col +1, row + 1, utf8,
                                     rgba1, rgba_black);
                  }
                }
              }
          }
        }
        break;
    }
}


inline static void ctx_term_render (void *ctx,
                                       CtxCommand *command)
{
  CtxTerm *term = (void*)ctx;
  /* directly forward */
  ctx_process (term->host, &command->entry);
}

inline static void ctx_term_flush (CtxTerm *term)
{
  int width =  term->width;
  int height = term->height;
  printf ("\e[H");
  ctx_term_output_buf (term->pixels,
                          CTX_FORMAT_RGBA8,
                          width, height, width * 4, 0, term);
#if CTX_BRAILLE_TEXT
  CtxRasterizer *rasterizer = (CtxRasterizer*)(term->host->renderer);
  // XXX instead sort and inject along with braille
  //

  //uint8_t rgba_bg[4]={0,0,0,0};
  //uint8_t rgba_fg[4]={255,0,255,255};

  printf ("\e[0m");
  for (CtxList *l = rasterizer->glyphs; l; l = l->next)
  {
    CtxTermGlyph *glyph = l->data;

    uint8_t *pixels = term->pixels;
    long rgb_sum[4]={0,0,0};
    for (int v = 0; v <  3; v ++)
    for (int u = 0; u <  2; u ++)
    {
      int i = ((glyph->row-1) * 3 + v) * rasterizer->blit_width + 
              ((glyph->col-1) * 2 + u);
      for (int c = 0; c < 3; c ++)
        rgb_sum[c] += pixels[i*4+c];
    }
    for (int c = 0; c < 3; c ++)
      glyph->rgba_bg[c] = rgb_sum[c] / (3 * 2);
    char utf8[8];
    utf8[ctx_unichar_to_utf8(glyph->unichar, (uint8_t*)utf8)]=0;
    ctx_term_set (term, glyph->col, glyph->row, 
                     utf8, glyph->rgba_fg, glyph->rgba_bg);
    free (glyph);
  }
  ctx_term_scanout (term);
  printf ("\e[0m");
  fflush(NULL);
  while (rasterizer->glyphs)
    ctx_list_remove (&rasterizer->glyphs, rasterizer->glyphs->data);
#endif
}

void ctx_term_free (CtxTerm *term)
{
  while (term->lines)
  {
    free (term->lines->data);
    ctx_list_remove (&term->lines, term->lines->data);
  }
  printf ("\e[?25h"); // cursor on
  nc_at_exit ();
  free (term->pixels);
  ctx_free (term->host);
  free (term);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

int ctx_renderer_is_term (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_term_free)
          return 1;
  return 0;
}

Ctx *ctx_new_term (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  fprintf (stdout, "\e[?1049h");
  fprintf (stdout, "\e[?25l"); // cursor off
  CtxTerm *term = (CtxTerm*)calloc (sizeof (CtxTerm), 1);
  int maxwidth = ctx_terminal_cols  () * 2;
  int maxheight = (ctx_terminal_rows ()-1) * 3;
  if (width <= 0 || height <= 0)
  {
    width = maxwidth;
    height = maxheight;
  }
  if (width > maxwidth) width = maxwidth;
  if (height > maxheight) height = maxheight;
  term->ctx = ctx;
  term->width  = width;
  term->height = height;
  term->cols = (width + 1) / 2;
  term->rows = (height + 2) / 3;
  term->lines = 0;
  term->pixels = (uint8_t*)malloc (width * height * 4);
  term->host = ctx_new_for_framebuffer (term->pixels,
                                           width, height,
                                           width * 4, CTX_FORMAT_RGBA8);
#if CTX_BRAILLE_TEXT
  ((CtxRasterizer*)term->host->renderer)->term_glyphs=1;
#endif
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, term);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, 3.0f);
  term->render = ctx_term_render;
  term->flush = (void(*)(void*))ctx_term_flush;
  term->free  = (void(*)(void*))ctx_term_free;
#endif


  return ctx;
}

#endif
