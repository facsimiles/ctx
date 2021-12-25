#include "ctx-split.h"

#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

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

typedef enum
{
  CTX_TERM_ASCII,
  CTX_TERM_ASCII_MONO,
  CTX_TERM_SEXTANT,
  CTX_TERM_BRAILLE_MONO,
  CTX_TERM_BRAILLE,
  CTX_TERM_QUARTER,
} CtxTermMode;

typedef struct _CtxTerm CtxTerm;
struct _CtxTerm
{
   CtxBackend  backender;
   int         width;
   int         height;
   int         cols;
   int         rows;
   int         was_down;

   uint8_t    *pixels;

   Ctx        *host;
   CtxList    *lines;
   CtxTermMode mode;
};

static int ctx_term_ch = 8;
static int ctx_term_cw = 8;

void ctx_term_set (CtxTerm *term,
                      int col, int row, const char *utf8,
                      uint8_t *fg, uint8_t *bg)
{
  if (col < 1 || row < 1 || col > term->cols  || row > term->rows) return;
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
     memset (&line->cells[line->size], 0, sizeof (CtxTermCell) * (new_size - line->size) );
     line->size = new_size;
  }
  if (col > line->maxcol) line->maxcol = col;
  strncpy (line->cells[col-1].utf8, (char*)utf8, 4);
  memcpy  (line->cells[col-1].fg, fg, 4);
  memcpy  (line->cells[col-1].bg, bg, 4);
}

static int _ctx_term256 = 0; // XXX TODO implement autodetect for this
static long _ctx_curfg = -1;
static long _ctx_curbg = -1;

static long ctx_rgb_to_long (int r,int g, int b)
{
  return r * 256 * 256 + g * 256 + b;
}


static void ctx_term_set_fg (int red, int green, int blue)
{
  long lc = ctx_rgb_to_long (red, green, blue);
  if (lc == _ctx_curfg)
    return;
  _ctx_curfg=lc;
  if (_ctx_term256 == 0)
  {
    printf("\e[38;2;%i;%i;%im", red,green,blue);
  }
  else
  {
    int gray = (green /255.0) * 24 + 0.5;
    int r    = (red/255.0)    * 6 + 0.5;
    int g    = (green/255.0)  * 6 + 0.5;
    int b    = (blue/255.0)   * 6 + 0.5;
    if (gray > 23) gray = 23;

    if (r > 5) r = 5;
    if (g > 5) g = 5;
    if (b > 5) b = 5;

    if (((int)(r/1.66)== (int)(g/1.66)) && ((int)(g/1.66) == ((int)(b/1.66))))
    {
      printf("\e[38;5;%im", 16 + 216 + gray);
    }
    else
      printf("\e[38;5;%im", 16 + r * 6 * 6 + g * 6  + b);
  }
}

static void ctx_term_set_bg(int red, int green, int blue)
{
  long lc = ctx_rgb_to_long (red, green, blue);
//if (lc == _ctx_curbg)
//  return;
  _ctx_curbg=lc;
  if (_ctx_term256 == 0)
  {
    printf("\e[48;2;%i;%i;%im", red,green,blue);
  }
  else
  {
    int gray = (green /255.0) * 24 + 0.5;
    int r    = (red/255.0)    * 6 + 0.5;
    int g    = (green/255.0)  * 6 + 0.5;
    int b    = (blue/255.0)   * 6 + 0.5;
    if (gray > 23) gray = 23;

    if (r > 5) r = 5;
    if (g > 5) g = 5;
    if (b > 5) b = 5;

    if (((int)(r/1.66)== (int)(g/1.66)) && ((int)(g/1.66) == ((int)(b/1.66))))
    {
      printf("\e[48;5;%im", 16 + 216 + gray);
    }
    else
      printf("\e[48;5;%im", 16 + r * 6 * 6 + g * 6  + b);
  }
}

static int _ctx_term_force_full = 0;

void ctx_term_scanout (CtxTerm *term)
{
  int row = 1;
  printf ("\e[H");
//  printf ("\e[?25l");
  printf ("\e[0m");

  int cur_fg[3]={-1,-1,-1};
  int cur_bg[3]={-1,-1,-1};

  for (CtxList *l = term->lines; l; l = l->next)
  {
    CtxTermLine *line = l->data;
    for (int col = 1; col <= line->maxcol; col++)
    {
      CtxTermCell *cell = &line->cells[col-1];

      if (strcmp(cell->utf8, cell->prev_utf8) ||
          memcmp(cell->fg, cell->prev_fg, 3) ||
          memcmp(cell->bg, cell->prev_bg, 3) || _ctx_term_force_full)
      {
        if (cell->fg[0] != cur_fg[0] ||
            cell->fg[1] != cur_fg[1] ||
            cell->fg[2] != cur_fg[2])
        {
          ctx_term_set_fg (cell->fg[0], cell->fg[1], cell->fg[2]);
          cur_fg[0]=cell->fg[0];
          cur_fg[1]=cell->fg[1];
          cur_fg[2]=cell->fg[2];
        }
        if (cell->bg[0] != cur_bg[0] ||
            cell->bg[1] != cur_bg[1] ||
            cell->bg[2] != cur_bg[2])
        {
          ctx_term_set_bg (cell->bg[0], cell->bg[1], cell->bg[2]);
          cur_bg[0]=cell->bg[0];
          cur_bg[1]=cell->bg[1];
          cur_bg[2]=cell->bg[2];
        }
        printf ("%s", cell->utf8);
      }
      else
      {
        // TODO: accumulate succesive such to be ignored items,
        // and compress them into one, making us compress largely
        // reused screens well
        printf ("\e[C");
      }
      strcpy (cell->prev_utf8, cell->utf8);
      memcpy (cell->prev_fg, cell->fg, 3);
      memcpy (cell->prev_bg, cell->bg, 3);
    }
    if (row != term->rows)
      printf ("\n\r");
    row ++;
  }
  printf ("\e[0m");
  //printf ("\e[?25h");
  //
}

// xx
// xx
// xx
//

static inline int _ctx_rgba8_manhattan_diff (const uint8_t *a, const uint8_t *b)
{
  int c;
  int diff = 0;
  for (c = 0; c<3;c++)
    diff += ctx_pow2(a[c]-b[c]);
  return ctx_sqrtf(diff);
  return diff;
}

static void ctx_term_output_buf_half (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term)
{
  int stride = width * 4;
  const char *sextants[]={
   " ","▘","▝","▀","▖","▌", "▞", "▛", "▗", "▚", "▐", "▜","▄","▙","▟","█",

  };
  for (int row = 0; row < height/2; row++)
    {
      for (int col = 0; col < width-3; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};
          int i = 0;

          int  rgbasum[2][4] = {0,};
          int  sumcount[2];

          int curdiff = 0;
          /* first find starting point colors */
          for (int yi = 0; yi < ctx_term_ch; yi++)
            for (int xi = 0; xi < ctx_term_cw; xi++, i++)
                {
                  int noi = (row * ctx_term_ch + yi) * stride + (col*ctx_term_cw+xi) * 4;

                  if (rgba[0][3] == 0)
                  {
                    for (int c = 0; c < 3; c++)
                      rgba[0][c] = pixels[noi + c];
                    rgba[0][3] = 255; // used only as mark of in-use
                  }
                  else
                  {
                    int diff = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[0]);
                    if (diff > curdiff)
                    {
                      curdiff = diff;
                      for (int c = 0; c < 3; c++)
                        rgba[1][c] = pixels[noi + c];
                    }
                  }

                }

          for (int iters = 0; iters < 1; iters++)
          {
                  i= 0;
          for (int i = 0; i < 4; i ++)
             rgbasum[0][i] = rgbasum[1][i]=0;
          sumcount[0] = sumcount[1] = 0;

          for (int yi = 0; yi < ctx_term_ch; yi++)
            for (int xi = 0; xi < ctx_term_cw; xi++, i++)
                {
                  int noi = (row * ctx_term_ch + yi) * stride + (col*ctx_term_cw+xi) * 4;

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
          for (int y = 0; y < ctx_term_ch; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }
           if (pixels_set == 4)
             ctx_term_set (term, col +1, row + 1, " ",
                           rgba[1], rgba[0]);
           else
             ctx_term_set (term, col +1, row + 1, sextants[unicode],
                           rgba[0], rgba[1]);
        }
    }
}

void ctx_term_find_color_pair (CtxTerm *term, int x0, int y0, int w, int h,
                uint8_t rgba[2][4])
        //uint8_t *rgba0, uint8_t *rgba1)
{
int curdiff = 0;
int stride = term->width * 4;
uint8_t *pixels = term->pixels;
/* first find starting point colors */
for (int y = y0; y < y0 + h; y++)
  for (int x = x0; x < x0 + w; x++)
      {
        int noi = (y) * stride + (x) * 4;

        if (rgba[0][3] == 0)
        {
          for (int c = 0; c < 3; c++)
            rgba[0][c] = pixels[noi + c];
          rgba[0][3] = 255; // used only as mark of in-use
        }
        else
        {
          int diff = _ctx_rgba8_manhattan_diff (&pixels[noi], &rgba[0][0]);
          if (diff > curdiff)
          {
            curdiff = diff;
            for (int c = 0; c < 3; c++)
              rgba[1][c] = pixels[noi + c];
          }
        }
      }
          int  rgbasum[2][4] = {0,};
          int  sumcount[2];

          for (int iters = 0; iters < 1; iters++)
          {
          for (int i = 0; i < 4; i ++)
             rgbasum[0][i] = rgbasum[1][i]=0;
          sumcount[0] = sumcount[1] = 0;

          for (int y = y0; y < y0 + h; y++)
            for (int x = x0; x < x0 + w; x++)
                {
                  int noi = (y) * stride + (x) * 4;

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

}



static void ctx_term_output_buf_quarter (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term)
{
  int stride = width * 4;
  const char *sextants[]={
   " ","▘","▝","▀","▖","▌", "▞", "▛", "▗", "▚", "▐", "▜","▄","▙","▟","█"

  };
  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};
          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);

          int pixels_set = 0;
          for (int y = 0; y < 2; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }
           if (pixels_set == 4)
             ctx_term_set (term, col +1, row + 1, " ",
                           rgba[1], rgba[0]);
           else
             ctx_term_set (term, col +1, row + 1, sextants[unicode],
                           rgba[0], rgba[1]);
        }
    }
}


static void ctx_term_output_buf_sextant (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term)
{
  int stride = width * 4;

  const char *sextants[]={
   " ","🬀","🬁","🬂","🬃","🬄","🬅","🬆","🬇","🬈","🬉","🬊","🬋","🬌","🬍","🬎","🬏","🬐","🬑","🬒","🬓","▌","🬔","🬕","🬖","🬗","🬘","🬙","🬚","🬛","🬜","🬝","🬞","🬟","🬠","🬡","🬢","🬣","🬤","🬥","🬦","🬧","▐","🬨","🬩","🬪","🬫","🬬","🬭","🬮","🬯","🬰","🬱","🬲","🬳","🬴","🬵","🬶","🬷","🬸","🬹","🬺","🬻","█"
  };

  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};

          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);

          int pixels_set = 0;
          for (int y = 0; y < ctx_term_ch; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }

          if (pixels_set == 6)
            ctx_term_set (term, col +1, row + 1, " ",
                          rgba[1], rgba[0]);
          else
            ctx_term_set (term, col +1, row + 1, sextants[unicode], rgba[0], rgba[1]);
        }
    }
}

static void ctx_term_output_buf_ascii (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term,
                          int mono)
{
  /* this is a crude ascii-mode built on a quick mapping of sexels to ascii */
  int stride = width * 4;
  const char *sextants[]={
   " ","`","'","^","🬃","`","~","\"","-","\"","'","\"","-","\"","~","^",",",";",
   "=","/","i","[","p","P","z",")","/","7","f",">","/","F",",","\\",":",":",
   "\\","\\","(","T","j","T","]","?","s","\\","<","q","_","=","=","=","c","L",
   "Q","C","a","b","J","]","m","b","d","@"
  };
  uint8_t black[4] = {0,0,0,255};
  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};

          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);


          if (_ctx_rgba8_manhattan_diff (black, rgba[1]) >
              _ctx_rgba8_manhattan_diff (black, rgba[0]))
          {
            for (int c = 0; c < 4; c ++)
            {
              int tmp = rgba[0][c];
              rgba[0][c] = rgba[1][c];
              rgba[1][c] = tmp;
            }
          }
          if (mono)
          {
            rgba[1][0] = 0;
            rgba[1][1] = 0;
            rgba[1][2] = 0;
          }


          int brightest_dark_diff = _ctx_rgba8_manhattan_diff (black, rgba[0]);

          int pixels_set = 0;
          for (int y = 0; y < ctx_term_ch; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }


           if (pixels_set == 6 && brightest_dark_diff < 40)
             ctx_term_set (term, col +1, row + 1, " ",
                           rgba[1], rgba[0]);
           else
             ctx_term_set (term, col +1, row + 1, sextants[unicode],
                           rgba[0], rgba[1]);
        }
    }
}

static void ctx_term_output_buf_braille (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term,
                          int mono)
{
  int reverse = 0;
  int stride = width * 4;
  uint8_t black[4] = {0,0,0,255};
  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};

          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);


          /* make darkest consistently be background  */
          if (_ctx_rgba8_manhattan_diff (black, rgba[1]) >
              _ctx_rgba8_manhattan_diff (black, rgba[0]))
          {
            for (int c = 0; c < 4; c ++)
            {
              int tmp = rgba[0][c];
              rgba[0][c] = rgba[1][c];
              rgba[1][c] = tmp;
            }
          }
          if (mono)
          {
            rgba[1][0] = 0;
            rgba[1][1] = 0;
            rgba[1][2] = 0;
          }

          int pixels_set = 0;
          for (int x = 0; x < 2; x++)
            for (int y = 0; y < 3; y++)
              {
                int no = (row * 4 + y) * stride + (col*2+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

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

#if 0
            if (pixels_set == 8)
            {
              if (rgba[0][0] < 32 && rgba[0][1] < 32 && rgba[0][2] < 32)
              {
                ctx_term_set (term, col +1, row + 1, " ",
                                 rgba[1], rgba[0]);
                continue;
              }
            }
#endif
            {
              ctx_term_set (term, col +1, row + 1, utf8,
                               rgba[0], rgba[1]);
            }
          }
        }
    }
}


inline static int
ctx_is_half_opaque (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
  {
    uint8_t ga[2];
    ctx_color_get_graya_u8 (rasterizer->state, &gstate->source_fill.color, ga);
    if ( (ga[1] * gstate->global_alpha_f) >= 127)
      return 1;
    return 0;
  }
  return gstate->global_alpha_f > 0.5f;
}

inline static void ctx_term_process (Ctx *ctx,
                                     CtxCommand *command)
{
  CtxTerm *term = (void*)ctx->backend;

#if CTX_BRAILLE_TEXT
  if (command->code == CTX_FILL)
  {
     CtxRasterizer *rasterizer = (CtxRasterizer*)term->host->backend;

     if (ctx_is_half_opaque (rasterizer))
     {
        CtxIntRectangle shape_rect = {
          ((int)(rasterizer->col_min / CTX_SUBDIV - 2))/2,
          ((int)(rasterizer->scan_min / 15 - 2))/3,
          ((int)(3+((int)rasterizer->col_max - rasterizer->col_min + 1) / CTX_SUBDIV))/2,
          ((int)(3+((int)rasterizer->scan_max - rasterizer->scan_min + 1) / 15))/3
        };
#if 0
  CtxGState *gstate = &rasterizer->state->gstate;
       fprintf (stderr, "{%i,%i %ix%i %.2f %i}",
                       shape_rect.x, shape_rect.y,
                       shape_rect.width, shape_rect.height,

                       gstate->global_alpha_f,
                       ga[1]
                       
                       );
   //  sleep(1);
#endif

       if (shape_rect.y > 0) // XXX : workaround 
       for (int row = shape_rect.y;
            row < (shape_rect.y+(int)shape_rect.height);
            row++)
       for (int col = shape_rect.x;
            col < (shape_rect.x+(int)shape_rect.width);
            col++)

       {
         for (CtxList *l = rasterizer->glyphs; l; l=l?l->next:NULL)
         {
           CtxTermGlyph *glyph = l->data;
           if ((glyph->row == row+1) &&
               (glyph->col == col+1))
           {
              ctx_list_remove (&rasterizer->glyphs, glyph);
              free (glyph);
              l = NULL;
           }
         }
       }
     }
  }
#endif

  /* directly forward */
  ctx_process (term->host, &command->entry);
}

inline static void ctx_term_flush (Ctx *ctx)
{
  CtxTerm *term = (CtxTerm*)ctx->backend;
  int width =  term->width;
  int height = term->height;
  switch (term->mode)
  {
    case CTX_TERM_QUARTER:
       ctx_term_output_buf_quarter (term->pixels,
                                width, height, term);
       break;
    case CTX_TERM_ASCII:
       ctx_term_output_buf_ascii (term->pixels,
                                width, height, term, 0);
       break;
    case CTX_TERM_ASCII_MONO:
       ctx_term_output_buf_ascii (term->pixels,
                                width, height, term, 1);
       break;
    case CTX_TERM_SEXTANT:
       ctx_term_output_buf_sextant (term->pixels,
                                width, height, term);
       break;
    case CTX_TERM_BRAILLE:
       ctx_term_output_buf_braille (term->pixels,
                                width, height, term, 0);
       break;
    case CTX_TERM_BRAILLE_MONO:
       ctx_term_output_buf_braille (term->pixels,
                                width, height, term, 1);
       break;
  }
#if CTX_BRAILLE_TEXT
  CtxRasterizer *rasterizer = (CtxRasterizer*)(term->host->backend);
  // XXX instead sort and inject along with braille
  //

  //uint8_t rgba_bg[4]={0,0,0,0};
  //uint8_t rgba_fg[4]={255,0,255,255};

  for (CtxList *l = rasterizer->glyphs; l; l = l->next)
  {
    CtxTermGlyph *glyph = l->data;

    uint8_t *pixels = term->pixels;
    long rgb_sum[4]={0,0,0};
    for (int v = 0; v <  ctx_term_ch; v ++)
    for (int u = 0; u <  ctx_term_cw; u ++)
    {
      int i = ((glyph->row-1) * ctx_term_ch + v) * rasterizer->blit_width + 
              ((glyph->col-1) * ctx_term_cw + u);
      for (int c = 0; c < 3; c ++)
        rgb_sum[c] += pixels[i*4+c];
    }
    for (int c = 0; c < 3; c ++)
      glyph->rgba_bg[c] = rgb_sum[c] / (ctx_term_ch * ctx_term_cw);
    char utf8[8];
    utf8[ctx_unichar_to_utf8(glyph->unichar, (uint8_t*)utf8)]=0;
    ctx_term_set (term, glyph->col, glyph->row, 
                     utf8, glyph->rgba_fg, glyph->rgba_bg);
    free (glyph);
  }

  printf ("\e[H");
  printf ("\e[0m");
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

float ctx_term_get_cell_width (Ctx *ctx)
{
  return ctx_term_cw;
}

float ctx_term_get_cell_height (Ctx *ctx)
{
  return ctx_term_ch;
}

Ctx *ctx_new_term (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  CtxTerm *term = (CtxTerm*)calloc (sizeof (CtxTerm), 1);
  CtxBackend *backend = (void*)term;
 
  const char *mode = getenv ("CTX_TERM_MODE");
  ctx_term_cw = 2;
  ctx_term_ch = 3;

  if (!mode) term->mode = CTX_TERM_SEXTANT;
  else if (!strcmp (mode, "sextant")) term->mode = CTX_TERM_SEXTANT;
  else if (!strcmp (mode, "ascii")) term->mode = CTX_TERM_ASCII_MONO;
  //else if (!strcmp (mode, "ascii-mono")) term->mode = CTX_TERM_ASCII_MONO;
  else if (!strcmp (mode, "quarter")) term->mode = CTX_TERM_QUARTER;
  //else if (!strcmp (mode, "braille")){
  //  term->mode = CTX_TERM_BRAILLE;
  //  ctx_term_ch = 4;
  //}
  else if (!strcmp (mode, "braille")){
    term->mode = CTX_TERM_BRAILLE_MONO;
    ctx_term_ch = 4;
  }
  else {
    fprintf (stderr, "recognized values for CTX_TERM_MODE:\n"
                    " sextant ascii quarter braille\n");
    exit (1);
  }

  mode = getenv ("CTX_TERM_FORCE_FULL");
  if (mode && strcmp (mode, "0") && strcmp (mode, "no"))
    _ctx_term_force_full = 1;

  fprintf (stdout, "\e[?1049h");
  fprintf (stdout, "\e[?25l"); // cursor off

  int maxwidth = ctx_terminal_cols  () * ctx_term_cw;
  int maxheight = (ctx_terminal_rows ()) * ctx_term_ch;
  if (width <= 0 || height <= 0)
  {
    width = maxwidth;
    height = maxheight;
  }
  if (width > maxwidth) width = maxwidth;
  if (height > maxheight) height = maxheight;
  backend->ctx = ctx;
  term->width  = width;
  term->height = height;

  term->cols = (width + 1) / ctx_term_cw;
  term->rows = (height + 2) / ctx_term_ch;
  term->lines = 0;
  term->pixels = (uint8_t*)malloc (width * height * 4);
  term->host = ctx_new_for_framebuffer (term->pixels,
                                           width, height,
                                           width * 4, CTX_FORMAT_RGBA8);
#if CTX_BRAILLE_TEXT
  ((CtxRasterizer*)term->host->backend)->term_glyphs=1;
#endif
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_backend (ctx, term);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, ctx_term_ch); 
  backend->backend = "term";
  backend->process = ctx_term_process;
  backend->flush   = ctx_term_flush;
  backend->free    = (void(*)(void*))ctx_term_free;
  backend->consume_events = ctx_nct_consume_events;
  backend->get_event_fds = (void*) ctx_stdin_get_event_fds;
  backend->has_event = (void*)ctx_nct_has_event; // doesnt use mice fd
#endif


  return ctx;
}

#endif
