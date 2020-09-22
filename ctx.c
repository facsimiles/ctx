#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <SDL.h>
#include "ctx-font-regular.h"
#include "ctx-font-mono.h"

//#include "Roboto-Regular.h"

//#include "DejaVuSansMono.h"
//#include "DejaVuSans.h"
//#include "0xA000-Mono.h"
//#include "unscii-16.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define CTX_BACKEND_TEXT         1
#define CTX_PARSER               1
#define CTX_FORMATTER            1
#define CTX_EVENTS               1
#define CTX_BITPACK_PACKER       1
#define CTX_GRADIENT_CACHE       1
#define CTX_ENABLE_CMYK          1
#define CTX_ENABLE_CM            1
#define CTX_SHAPE_CACHE          0
#define CTX_SHAPE_CACHE_MAX_DIM  96
#define CTX_SHAPE_CACHE_DIM      (64*64)
#define CTX_SHAPE_CACHE_ENTRIES  (512)
#define CTX_MATH                 1
#define CTX_MAX_JOURNAL_SIZE     1024*64
//#define CTX_RASTERIZER_AA        5
//#define CTX_RASTERIZER_FORCE_AA  1

//#include <immintrin.h> // is detected by ctx, and enables AVX2

#define CTX_IMPLEMENTATION 1
//
// we let the vt contain the implementation
// since it has the biggest need for tuning
#include "ctx.h"
#include "svg.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ctx should be a busy box like wrapper
// for mmm-terminal, ctx ascii-to-ctx converter
// ctx-to-ascii converter
//
// for C tests .. provide the same API to use
// ctx with mmm as through the terminal, then
// rebuild mrg on top of ctx

static int ctx_rgba8_manhattan_diff (const uint8_t *a, const uint8_t *b)
{
  int c;
  int diff = 0;
  for (c = 0; c<3;c++)
    diff += ctx_pow2(a[c]-b[c]);
  return diff;
}

#if 1
CtxOutputmode outputmode = CTX_OUTPUT_MODE_BRAILLE;
#else
CtxOutputmode outputmode = CTX_OUTPUT_MODE_UI;
#endif


void ctx_utf8_output_buf (uint8_t *pixels,
                          int format,
                          int width,
                          int height,
                          int stride,
                          int reverse)
{
  char *utf8_gray_scale[]= {" ","░","▒","▓","█","█", NULL};
  int no = 0;
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
        switch (outputmode)  {
      default:
      case  CTX_OUTPUT_MODE_QUARTER:
        {
           char *unicode_quarters[]={
" ","▘","▝","▀","▖","▌","▞","▛","▗","▚","▐","▜","▄","▙","▟","█",};
        for (int row = 0; row < height/2; row++)
          {
            for (int col = 0; col < width /2; col++)
              {
                uint8_t rgba2[4] = {0,0,0,255};
                uint8_t rgba1[4] = {255,255,255,255};
                int best_diff = 0;

                for (int c = 0; c < 3; c++)
                {
                  int no = (row * 2) * stride + (col*2) * 4;
                  rgba2[c] = pixels[no+c];
                }

                for (int xi = 0; xi < 2; xi++)
                  for (int yi = 0; yi < 2; yi++)
                    for (int xj = 0; xj < 2; xj++)
                      for (int yj = 0; yj < 2; yj++)
                      {
                        if (!(xi == xj && yi == yj))
                        {
                           int noi = (row * 2 + yi) * stride + (col*2+xi) * 4;
                           int noj = (row * 2 + yj) * stride + (col*2+xj) * 4;
                           uint8_t rgbai[4]={0,};
                           uint8_t rgbaj[4]={0,};
                           for (int c = 0; c < 3; c++)
                           {
                             rgbai[c] = pixels[noi+c];
                             rgbaj[c] = pixels[noj+c];
                           }
                           int diff = ctx_rgba8_manhattan_diff (rgbai, rgbaj);
                           if (diff > best_diff)
                           {
                             best_diff = diff;
                             for (int c = 0; c < 3; c++)
                             {
                               rgba1[c] = rgbai[c];
                               rgba2[c] = rgbaj[c];
                             }
                           }
                        }
                      }
                // to determine color .. find two most different
                // colors in set.. and threshold between them..
                // even better dither between them.
                //
  printf ("\e[38;2;%i;%i;%im", rgba1[0], rgba1[1], rgba1[2]);
  printf ("\e[48;2;%i;%i;%im", rgba2[0], rgba2[1], rgba2[2]);

                int bits_set=0;

                int bit_no = 0;
                for (int y = 0; y < 2; y++)
                  for (int x = 0; x < 2; x++)
                    {
                      int no = (row * 2 + y) * stride + (col*2+x) * 4;
#define CHECK_IS_SET \
      (ctx_rgba8_manhattan_diff (&pixels[no], rgba1)< \
       ctx_rgba8_manhattan_diff (&pixels[no], rgba2))

                      int set = CHECK_IS_SET;
                      if (reverse) { set = !set; }
                      if (set)
                        { bits_set |=  (1<< (bit_no) ); }
                      bit_no++;
                    }
                printf ("%s", unicode_quarters[bits_set]);
       //         printf ("%i ", bits_set);
              }
            printf ("\e[38;2;%i;%i;%im", 255,255,255);
            printf ("\e[48;2;%i;%i;%im", 0,0,0);
            printf ("\n");
          }
        }
        break;
      case  CTX_OUTPUT_MODE_BRAILLE:
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
                        int diff = ctx_rgba8_manhattan_diff (&pixels[noi], rgba2);
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

                for (int x = 0; x < 2; x++)
                  for (int y = 0; y < 3; y++)
                    {
                      int no = (row * 4 + y) * stride + (col*2+x) * 4;
#define CHECK_IS_SET \
      (ctx_rgba8_manhattan_diff (&pixels[no], rgba1)< \
       ctx_rgba8_manhattan_diff (&pixels[no], rgba2))

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
            printf ("\n");
  printf ("\e[38;2;%i;%i;%im", 255,255,255);
  printf ("\e[48;2;%i;%i;%im", 0,0,0);
          }
        }
        break;
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
      case CTX_FORMAT_GRAYF:
        {
          float *pix = (float*)pixels;
          for (int c = 0; c < 1; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=1)
                    {
                      int val = (int) CTX_CLAMP ( (pix[no+c]*6.0), 0, 5);
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
          float *pix = (float*)pixels;
          for (int c = 0; c < 5; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=5)
                    {
                      int val = (int) CTX_CLAMP ( (pix[no+c]*6.0), 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
        break;
      case CTX_FORMAT_RGBAF:
        {
          float *pix = (float*)pixels;
          for (int c = 0; c < 4; c++)
            {
              int no = 0;
              for (int y= 0; y < height; y++)
                {
                  for (int x = 0; x < width; x++, no+=4)
                    {
                      int val = (int) CTX_CLAMP ( (pix[no+c]*6.0), 0, 5);
                      if (reverse)
                        { val = 5-val; }
                      printf ("%s", utf8_gray_scale[val]);
                    }
                  printf ("\n");
                }
            }
        }
        break;
    }
}

int parse_main (int argc, char **argv)
{
  char *line = NULL;
  Ctx *ctx = ctx_new ();
  if (argc &&  argv) {};
  _ctx_set_store_clear (ctx);
  size_t size;
  while (getline (&line, &size, stdin) != -1)
    {
      //
      //ctx_parse_str_line (ctx, line);
      // XXX : use svgp
    }
  struct termios termios_original;
  struct termios termios_set;
  tcgetattr (0, &termios_original);
  termios_set = termios_original;
  cfmakeraw (&termios_set);
  tcsetattr (0, TCSANOW, &termios_set);
  int count = ctx_get_renderstream_count (ctx);
  printf ("\033[?2222h");
  //printf ("%c        ", CTX_CLEAR);
  CtxEntry *entries = * (CtxEntry **) (ctx);
  for (int i = 0; i < count; i++)
    {
      printf ("%c", entries[i].code);
      for (int j = 0; j < 8; j++)
        {
          int val = entries[i].data.u8[j];
#if 1
          /* hack to be used if we're not in raw mode, avoids expansion of lf -> crlf transforms in cooked mode */
          if (val == 10) { val = 11; }
#endif
          printf ("%c", val);
        }
    }
  printf ("Xxxxxxxxx");
  tcsetattr (0, TCSANOW, &termios_original);
  fflush (NULL);
  ctx_free (ctx);
  return 0;
}

//
//  capabilities:
//     font-upload
//     graphics-upload
//     audio-output
//     mic-input
//     webcam-input
//     launch
//     add-layer
//     set-layer
//
//     file-system-access
//     location
//
//     list-clients        # for wm
//     manipulate-clients  # for wm
//
// set cap-mic-input desired
// set cap-mic-output
//

static int usage (void)
{
  printf (
    "Usage: ctx [options] <inputpath> [outputpath]\n"
    "   or: ctx [options] <interpreter> [file] parameters\n"
    "\n"
    "The input path is a file format recognized by ctx, for rendering.\n"
    "\n"
    "ctx alone should launch a terminal, that is part of a session which\n"
    "can have other clients. The interpreter cannot have the same extension\n"
    "as one of the supported formats.\n"
    "\n"
    "options:\n"
    "  --clear         clear and home between each frame\n"
    "  --braille       unicode braille char output mode\n"
    "  --quarter       unicode quad char output mode\n"
    "  --ctx           output ctx to terminal\n"
    "  --ctx-compact   output compact ctx to terminal\n"

    //  --color         use color in output\n"
    //"  --x      set pixel position of canvas (only means something when running in compositor)\n"
    //"  --y      set pixel position of canvas (only means something when running in compositor\n"
    "  --width  pixels sets width of canvas\n"
    "  --height pixels sets height of canvas\n"
    "  --rows   rows   configures number of em-rows\n"
    "  --cols   cols   configures number of em-cols\n");
  return 0;
}

const char *get_suffix (const char *path)
{
  if (!path)
    { return ""; }
  const char *p = strrchr (path, '.');
  if (p && *p)
    { return p; }
  else
    { return ""; }
}

static int
_file_get_contents (const char     *path,
                    unsigned char **contents,
                    long           *length)
{
  FILE *file;
  long  size;
  long  remaining;
  char *buffer;
  file = fopen (path, "rb");
  if (!file)
    { return -1; }
  fseek (file, 0, SEEK_END);
  size = remaining = ftell (file);
  if (length)
    { *length =size; }
  rewind (file);
  buffer = malloc (size + 8);
  if (!buffer)
    {
      fclose (file);
      return -1;
    }
  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      free (buffer);
      return -1;
    }
  fclose (file);
  *contents = (void *) buffer;
  buffer[size] = 0;
  return 0;
}

int dirty = 1;
float offset_x = 0.0;
float offset_y = 0.0;
float scale = 1.0f;

static void ui_drag (CtxEvent *event, void *data1, void *data2)
{
  offset_x += event->delta_x;
  offset_y += event->delta_y;
  dirty++;
}

int terminal_main (int argc, char **argv);

int main (int argc, char **argv)
{
  const char *source_path = NULL;
  const char *dest_path = NULL;
  //int   source_arg_no = 1;
  int width=-1;//=  ctx_terminal_width ();
  int height=-1;// = ctx_terminal_height ();
  float cols=-1;// = ctx_terminal_cols ();
  float rows=-1;// = ctx_terminal_rows ();
  int do_convert = 0;
  for (int i = 1; argv[i]; i++)
    {
      if (argv[i][0] == '-')
        {
          if (!strcmp ( argv[i], "--convert") ||
              !strcmp ( argv[i], "-c"))
            do_convert = 1;
          else if (!strcmp ( argv[i], "--quarter") )
            {
              outputmode = CTX_OUTPUT_MODE_QUARTER;
            }
          else if (!strcmp ( argv[i], "--ui") )
            {
              outputmode = CTX_OUTPUT_MODE_UI;
              if (!getenv ("CTX_VERSION"))
              {
                width = 640;
                height = 480;
              }
            }
          else if (!strcmp ( argv[i], "--braille") )
            {
              outputmode = CTX_OUTPUT_MODE_BRAILLE;
            }
          else if (!strcmp ( argv[i], "--ctx-compact") )
            {
              outputmode = CTX_OUTPUT_MODE_CTX_COMPACT;
            }
          else if (!strcmp ( argv[i], "--ctx") )
            {
              outputmode = CTX_OUTPUT_MODE_CTX;
            }
          if (!strcmp ( argv[i], "--width") )
            {
              if (argv[i+1])
                {
                  width = atoi (argv[i+1]);
                  i++;
                }
            }
          if (!strcmp (argv[i], "--help") ||
              !strcmp (argv[i], "--usage") ||
              !strcmp (argv[i], "-h") )
          { return usage();
          }
          if (!strcmp ( argv[i], "--height") )
            {
              if (argv[i+1])
                {
                  height = atoi (argv[i+1]);
                  i++;
                }
            }
          if (!strcmp ( argv[i], "--cols") )
            {
              if (argv[i+1])
                {
                  cols = atoi (argv[i+1]);
                  i++;
                }
            }
          if (!strcmp ( argv[i], "--rows") )
            {
              if (argv[i+1])
                {
                  rows = atoi (argv[i+1]);
                  i++;
                }
            }
        }
      else
        {
          if (!source_path)
            {
              source_path = argv[i];
              //source_arg_no = i;
            }
          else
            {
              if (dest_path)
                {
                  fprintf (stderr, "already got dest\n");
                  exit (-1);
                }
              dest_path = argv[i];
            }
        }
    }
  if (!do_convert)
  {
    return terminal_main (argc, argv);
  }

  if (dest_path && strstr (dest_path, "GRAY2")) outputmode = CTX_OUTPUT_MODE_GRAYS;
  if (dest_path && strstr (dest_path, "GRAY4")) outputmode = CTX_OUTPUT_MODE_GRAYS;
  if (dest_path && strstr (dest_path, "GRAY8")) outputmode = CTX_OUTPUT_MODE_GRAYS;

  if (width <= 0 || height <= 0)
  {
    width = ctx_terminal_width ();
    height = ctx_terminal_height ();
  switch (outputmode)
  {
    case CTX_OUTPUT_MODE_GRAYS:
      if (rows < 0) rows = ctx_terminal_rows ();
      if (cols < 0) cols = ctx_terminal_cols ();
      width  = cols;
      height = rows;
      rows /= 8;
      cols /= 8;
      break;
    case CTX_OUTPUT_MODE_QUARTER:
      if (rows < 0) rows = ctx_terminal_rows ();
      if (cols < 0) cols = ctx_terminal_cols ();
      width  = cols * 2;
      height = rows * 2;
      rows /= 5;
      cols /= 5;
      break;
    case CTX_OUTPUT_MODE_BRAILLE:
      if (rows < 0) rows = ctx_terminal_rows ();
      if (cols < 0) cols = ctx_terminal_cols ();
      width  = cols * 2;
      height = rows * 4;
      rows /= 6;
      cols /= 6;
      break;
    case CTX_OUTPUT_MODE_SIXELS:
    case CTX_OUTPUT_MODE_CTX:
    case CTX_OUTPUT_MODE_UI:
    case CTX_OUTPUT_MODE_CTX_COMPACT:
      if (rows < 0) rows = ctx_terminal_rows ();
      if (cols < 0) cols = ctx_terminal_cols ();
      break;
  }
  }

  if (cols < 0)
    cols = width / (height / rows);
#if 0
  if (dest_path)
    {
      if (!strcmp (dest_path, "GRAY1") ||
          !strcmp (dest_path, "RGBA8"))
        {
          width = 160;
          height = 80;
          rows = 3;
          cols = 24;
          if (height > 200)
            {
              cols = width/16;
              rows = height/16;
            }
          cols = width / (height / rows);
        }
      if (!strcmp (dest_path, "GRAY2") ||
          !strcmp (dest_path, "GRAY4") ||
          !strcmp (dest_path, "GRAY8") ||
          !strcmp (dest_path, "GRAYF") ||
          !strcmp (dest_path, "CMYKAF") ||
          !strcmp (dest_path, "RGB8") ||
          //!strcmp (dest_path, "RGBA8") ||
          !strcmp (dest_path, "CMYK8")
         )
        {
          width = 78;
          height = 24;
          rows = 2.4;
          cols = width/6;
          cols = width / (height / rows);
        }
    }
#endif

#if 0
  fprintf (stderr, "%s [%s]\n", source_path, get_suffix (source_path) );
  fprintf (stderr, "%s [%s]\n", dest_path, get_suffix (dest_path) );
#endif
  Ctx *ctx;
again:
  ctx = ctx_new ();
  _ctx_set_transformation (ctx, 0);
  if (!strcmp (get_suffix (source_path), ".html") ||
      !strcmp (get_suffix (source_path), ".svg") ||
      !strcmp (get_suffix (source_path), ".xml") )
    {
      Mrg *mrg = mrg_new (ctx, width, height);
      unsigned char *contents = NULL;
      long length;
      _file_get_contents (source_path, &contents, &length);
      mrg_print_xml (mrg, (char *) contents);
    }
  else if (!strcmp (get_suffix (source_path), ".ctx") )
    {
      unsigned char *contents = NULL;
      long length;
      _file_get_contents (source_path, &contents, &length);
      if (contents)
        {
          CtxParser *ctxp = ctx_parser_new (ctx, width, height, width/cols, height/rows, 1, 1, NULL, NULL, NULL, NULL, NULL);
          for (int i = 0; contents[i]; i++)
            {
              ctx_parser_feed_byte (ctxp, contents[i]);
            }
          ctx_parser_free (ctxp);
          free (contents);
        }
    }
  else
    {
      fprintf (stderr, "nothing to do\n");
      exit(0);
    }

  if (outputmode == CTX_OUTPUT_MODE_UI)
  {
    static Ctx *ui = NULL;
    if (!ui)
    {
      ui = ctx_new_ui (width, height);
      ctx_get_event (ui); // forces enabling of get event
    }
    dirty=1;
    for (;;)
    {
      if (dirty)
      {
        ctx_reset (ui);
        ctx_save (ui);
        ctx_translate (ui, offset_x, offset_y);
        ctx_scale (ui, scale, scale);
        ctx_render_ctx (ctx, ui);
        ctx_restore (ui);
#if 1
        ctx_begin_path (ui);
        ctx_rectangle (ui, 0, 0, width, height);
        ctx_listen (ui, CTX_DRAG_MOTION, ui_drag, NULL, NULL);
#endif
        ctx_flush (ui);
        dirty = 0;
      }

      CtxEvent *event;
      while ((event = ctx_get_event (ui)))
      {
        switch (event->type)
        {
          case CTX_SCROLL:
             fprintf (stderr, "scroll!\n");
             break;
#if 0
          case CTX_RELEASE:
             fprintf (stderr, "release\n");
             dirty++;
             break;
          case CTX_PRESS:
             fprintf (stderr, "press\n");
             dirty++;
             break;
          case CTX_MOTION:
             if (ctx_pointer_is_down (ui, 0))
             {
               offset_x += event->delta_x;
               offset_y += event->delta_y;
             dirty++;
             //fprintf (stderr, "drag\n");
             }
             else
             {
             //fprintf (stderr, "motion\n");
             }
             break;
#endif
          case CTX_KEY_DOWN:
             dirty++;
          if (!strcmp (event->string, "+")) scale *= 1.1;
          else if (!strcmp (event->string, "=")) scale *= 1.1;
          else if (!strcmp (event->string, "-")) scale /= 1.1;
          else if (!strcmp (event->string, "q")) exit (0);
          else if (!strcmp (event->string, "resize-event"))
          {
             width = ctx_width (ui);
             height = ctx_height (ui);
             ctx_free (ctx);
             goto again;
          }
          else
          {
             fprintf (stderr, "key-down: %s\n", event->string);
          }
          break;
          default:
            usleep (100);
          break;
        }
      }
    }
    exit(0);
  }

  if (outputmode == CTX_OUTPUT_MODE_CTX)
    {
      // determine terminal size
      fprintf (stdout, "\e[H\e[?25l\e[?7020h\nreset\n");
      ctx_render_stream (ctx, stdout, 1);
      fprintf (stdout, "\nX\n\e[?25h");
      exit (0);
    }
  if (outputmode == CTX_OUTPUT_MODE_CTX_COMPACT)
    {
      fprintf (stdout, "\e[H\e[?25l\e[?7020h\nreset\n");
      ctx_render_stream (ctx, stdout, 0);
      fprintf (stdout, "\nX\n\[e?25h");
      exit (0);
    }

  if (dest_path == NULL || !strcmp (dest_path, "RGBA8") )
    {
      int reverse = 0;
      int stride = width * 4;
      uint8_t pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_RGBA8);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf ( (uint8_t *) pixels,
                            CTX_FORMAT_RGBA8,
                            width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "GRAY1") )
    {
      int reverse = 0;
      int stride = width/8+ (width%8?1:0);
      uint8_t pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0],
                                           width, height, stride,
                                           CTX_FORMAT_GRAY1);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf (pixels,
                           CTX_FORMAT_GRAY1,
                           width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "GRAY2") )
    {
      int reverse = 1;
      int stride = width/4;
      uint8_t pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_GRAY2);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf (pixels,
                           CTX_FORMAT_GRAY2,
                           width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "GRAY4") )
    {
      int reverse = 0;
      int stride = width/2;
      uint8_t pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_GRAY4);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf (pixels,
                           CTX_FORMAT_GRAY4,
                           width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "GRAY8") )
    {
      int reverse = 0;
      int stride = width;
      uint8_t pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_GRAY8);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf (pixels,
                           CTX_FORMAT_GRAY8,
                           width, height, stride, reverse);
      int no = 0;
      static char *utf8_gray_scale[]= {" ","░","▒","▓","█","█", NULL};
      for (int y= 0; y < height; y++)
        {
          for (int x = 0; x < width; x++, no++)
            {
              int val = (int) CTX_CLAMP (pixels[no]/255.0*6.0, 0, 5);
              if (reverse)
                { val = 5-val; }
              printf ("%s", utf8_gray_scale[val]);
            }
          printf ("\n");
        }
    }
  else if (!strcmp (dest_path, "GRAYF") )
    {
      int reverse = 0;
      int stride = width * 4;
      float pixels[width*height];
      static char *utf8_gray_scale[]= {" ","░","▒","▓","█","█", NULL};
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_GRAYF);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      int no = 0;
      for (int y= 0; y < height; y++)
        {
          for (int x = 0; x < width; x++, no++)
            {
              int val = (int) CTX_CLAMP ( (pixels[no]*6.0), 0, 5);
              if (reverse)
                { val = 5-val; }
              printf ("%s", utf8_gray_scale[val]);
            }
          printf ("\n");
        }
      ctx_utf8_output_buf ( (uint8_t *) pixels,
                            CTX_FORMAT_GRAYF,
                            width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "CMYKAF") )
    {
      int reverse = 0;
      int stride = width * 4 * 5;
      float pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_CMYKAF);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf ( (uint8_t *) pixels,
                            CTX_FORMAT_CMYKAF,
                            width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "RGBAF") )
    {
      int reverse = 0;
      int stride = width * 4 * 4;
      float pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_RGBAF);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf ( (uint8_t *) pixels,
                            CTX_FORMAT_RGBAF,
                            width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "RGB8") )
    {
      int reverse = 0;
      int stride = width * 3;
      uint8_t pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_RGB8);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf ( (uint8_t *) pixels,
                            CTX_FORMAT_RGB8,
                            width, height, stride, reverse);
    }
  else if (!strcmp (dest_path, "CMYK8") )
    {
      int reverse = 0;
      int stride = width * 4;
      uint8_t pixels[stride*height];
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_CMYK8);
      memset (pixels, 0, sizeof (pixels) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      ctx_utf8_output_buf ( (uint8_t *) pixels,
                            CTX_FORMAT_CMYK8,
                            width, height, stride, reverse);
    }
  else if (!strcmp (get_suffix (dest_path), ".png") )
    {
      int stride = width * 4;
      uint8_t *pixels = malloc (stride*height);
      Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_RGBA8);
      memset (pixels, 0, sizeof (stride*height) );
      ctx_render_ctx (ctx, dctx);
      ctx_free (dctx);
      stbi_write_png (dest_path, width, height, 4, pixels, stride);
      free (pixels);
    }
  ctx_free (ctx);
  return 0;
}
