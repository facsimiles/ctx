#include <stdint.h>
#include <termios.h>
#include <unistd.h>

#include "ctx-font-regular.h"
#include "ctx-font-mono.h"

//#define CTX_IMPLEMENTATION 1
//
// we let the vt contain the implementation
// since it has the biggest need for tuning
#include "ctx.h"

// ctx should be a busy box like wrapper
// for mmm-terminal, ctx ascii-to-ctx converter
// ctx-to-ascii converter
//
// for C tests .. provide the same API to use
// ctx with mmm as through the terminal, then
// rebuild mrg on top of ctx

int parse_main (int argc, char **argv)
{
  char *line = NULL;
  Ctx *ctx = ctx_new ();
  if (argc &&  argv) {};
  _ctx_set_store_clear (ctx);
  size_t size;
  while (getline(&line, &size, stdin) != -1) {
     //
     //ctx_parse_str_line (ctx, line);
     // XXX : use svgp
  }

  struct termios termios_original;
  struct termios termios_set;
  tcgetattr(0, &termios_original);
  termios_set = termios_original;
  cfmakeraw(&termios_set);
  tcsetattr(0, TCSANOW, &termios_set);
  int count = ctx_get_renderstream_count (ctx);

  printf ("\e[?2222h");
  //printf ("%c        ", CTX_CLEAR);
 
  CtxEntry *entries = *(CtxEntry **)(ctx);
  for (int i = 0; i < count; i++)
  {
     printf ("%c", entries[i].code);
    for (int j = 0; j < 8; j++)
    {
      int val = entries[i].data.u8[j];
#if 1
      /* hack to be used if we're not in raw mode, avoids expansion of lf -> crlf transforms in cooked mode */
      if (val == 10) val = 11;
#endif
      printf ("%c", val);
    }
  }
  printf ("Xxxxxxxxx");
  tcsetattr(0, TCSANOW, &termios_original);
  fflush (NULL);
  ctx_free (ctx);
  return 0;
}

int help_main (int argc, char **argv)
{
  if (argc &&  argv){};
  printf ("Usage: ctx [command [command args]]\n\n"
          "where command is one of:\n"
          " vt    - virtual terminal, running ctx with no args\n    also launches a terminal\n"
          " parse - parse ascii, and generate ctx on stdout\n");
  return 0;
}


//  ctx API
//    nchanterm + ctx APIs
//      braille
//      sixels
//      ctx
//
//


int vt_main (int argc, char **argv);


const char *get_suffix (const char *path)
{
   if (!path)
      return "";
   const char *p = strrchr (path, '.');
   if (p && *p)
     return p;
   else
     return "";
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
    return -1;

  fseek (file, 0, SEEK_END);
  size = remaining = ftell (file);
  if (length)
    *length =size;
  rewind (file);
  buffer = malloc(size + 8);

  if (!buffer)
    {
      fclose(file);
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
  *contents = (void*)buffer;
  buffer[size] = 0;
  return 0;
}

int main (int argc, char **argv)
{
  const char *source_path = NULL;
  const char *dest_path = NULL;
  int width = 640;
  int height = 480;
  int cols = 80;
  int rows = 24;

  if (!argv[1])
    return vt_main (argc, argv);
  if (!strcmp (argv[1], "vt"))
    return vt_main (argc - 1, argv + 1);

  for (int i = 1; argv[i]; i++)
  {
    if (argv[i][0] == '-')
    {
      if (!strcmp ( argv[i], "--width"))
      {
         if (argv[i+1])
           width = atoi(argv[i+1]);
      }
      if (!strcmp ( argv[i], "--height"))
      {
         if (argv[i+1])
           height = atoi(argv[i+1]);
      }
      if (!strcmp ( argv[i], "--cols"))
      {
         if (argv[i+1])
           cols = atoi(argv[i+1]);
      }
      if (!strcmp ( argv[i], "--rows"))
      {
         if (argv[i+1])
           rows = atoi(argv[i+1]);
      }
    }
    else
    {
      if (!source_path)
      {
         source_path = argv[i];
      }
      else
      {
         if (dest_path)
         {
            fprintf (stderr, "already got dest\n");exit(-1);
         }
         dest_path = argv[i];
      }
    }
  }
  if (dest_path)
  {
  if (!strcmp (dest_path, "1bit"))
  {
     width = 158; height = 120; cols = width/6; rows = height / 2;
  }
  if (!strcmp (dest_path, "2bit") ||
      !strcmp (dest_path, "4bit") ||
      !strcmp (dest_path, "8bit"))
  {
     width = 72; height = 24; cols = width/3; rows = height/3;
  }
  }
  
#if 0
  fprintf (stderr, "%s [%s]\n", source_path, get_suffix (source_path));
  fprintf (stderr, "%s [%s]\n", dest_path, get_suffix (dest_path));
#endif

  Ctx *ctx = ctx_new ();

  _ctx_set_transformation (ctx, 0);

  if (!strcmp (get_suffix (source_path), ".ctx"))
  {
     unsigned char *contents = NULL;
     long length;
     _file_get_contents (source_path, &contents, &length);
     if (contents)
     {
       CtxP *ctxp = ctxp_new (ctx, width, height, width/cols, height/rows, 1, 1, NULL, NULL);

       for (int i = 0; contents[i]; i++)
       {
         ctxp_feed_byte (ctxp, contents[i]);
       }

       ctxp_free (ctxp);
       free (contents);
     }
  }
  else
  {
     fprintf (stderr, "unhandled input suffix\n");exit(-1);
  }

  if (!dest_path)
  {
     ctx_render_stream (ctx, stdout);
     exit (0);
  }

  if (!strcmp (dest_path, "1bit"))
  {
    int reverse = 0;
    int stride = width/8+(width%8?:0);
    uint8_t pixels[stride*height];
    Ctx *dctx = ctx_new_for_framebuffer (&pixels[0],
                                         width, height, stride,
                                         CTX_FORMAT_GRAY1);
    memset (pixels, 0, sizeof (pixels));
    ctx_render_ctx (ctx, dctx);
    ctx_free (dctx);
    for (int row = 0; row < height/4; row++)
    {
      for (int col = 0; col < width /2; col++)
      {
        int unicode = 0;
        int bitno = 0;
        for (int x = 0; x < 2; x++)
          for (int y = 0; y < 3; y++)
          {
            int no = (row * 4 + y) * stride + (col*2+x)/8;
            int set = pixels[no] & (1<< ((col * 2 + x) % 8));
            if (reverse) set = !set;
             if (set)
               unicode |=  (1<<(bitno));
            bitno++;
          }
        {
           int x = 0; int y = 3;
           int no = (row * 4 + y) * stride + (col*2+x)/8;
           int setA = pixels[no] & (1<< ((col * 2 + x) % 8));
           no = (row * 4 + y) * stride + (col*2+x+1)/8;
           int setB = pixels[no] & (1<< (   (col * 2 + x + 1) % 8));
           if (reverse) setA = !setA;
           if (reverse) setB = !setB;
           if (setA != 0 && setB==0)
             unicode += 0x2840;
           else if (setA == 0 && setB)
             unicode += 0x2880;
           else if ((setA != 0) && (setB != 0))
             unicode += 0x28C0;
           else
             unicode += 0x2800;
          uint8_t utf8[5];
          utf8[ctx_unichar_to_utf8 (unicode, utf8)]=0;
          printf ("%s", utf8);
        }
      }
      printf ("\n");
    }
  }

  if (!strcmp (dest_path, "2bit"))
  {
     int reverse = 1;
     int stride = width/4;
     uint8_t pixels[stride*height];
     static char *utf8_gray_scale[]={" ","░","▓","█", NULL};

     Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_GRAY2);
     memset (pixels, 0, sizeof (pixels));
     ctx_render_ctx (ctx, dctx);
     ctx_free (dctx);

    int no = 0;
    for (int y= 0; y < height; y++)
    {
      no = y * stride;
      for (int x = 0; x < width; x++)
      {
        int val = (pixels[no] & (3 << ((x % 4)*2)) ) >> ((x%4)*2);
        if (reverse)
          printf ("%s", utf8_gray_scale[val]);
        else
          printf ("%s", utf8_gray_scale[3-val]);
        if ((x % 4) == 3)
          no++;
      }
      printf ("\n");
    }
  }

  if (!strcmp (dest_path, "4bit"))
  {
     int reverse = 1;
     int stride = width/2;
     uint8_t pixels[stride*height];
     static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};

     Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_GRAY4);
     memset (pixels, 0, sizeof (pixels));
     ctx_render_ctx (ctx, dctx);
     ctx_free (dctx);

    int no = 0;
    for (int y= 0; y < height; y++)
    {
      no = y * stride;
      for (int x = 0; x < width; x++)
      {
        int val = (pixels[no] & (15 << ((x % 2)*4)) ) >> ((x%2)*4);
        val = val * 6 / 16;
        if (reverse)val = 5-val;
          val = CTX_CLAMP(val, 0, 4);
        printf ("%s", utf8_gray_scale[val]);
        if (x % 2 == 1)
          no++;
      }
      printf ("\n");
    }
  }

  if (!strcmp (dest_path, "8bit"))
  {
     int reverse = 1;
     int stride = width;
     uint8_t pixels[stride*height];
     static char *utf8_gray_scale[]={" ","░","▒","▓","█","█", NULL};

     Ctx *dctx = ctx_new_for_framebuffer (&pixels[0], width, height, stride, CTX_FORMAT_GRAY8);
     memset (pixels, 0, sizeof (pixels));
     ctx_render_ctx (ctx, dctx);
     ctx_free (dctx);

     int no = 0;
     for (int y= 0; y < height; y++)
     {
       for (int x = 0; x < width; x++, no++)
       {
          int val = (int)CTX_CLAMP(pixels[no]/255.0*6.0, 0, 5);
          if (reverse)
            val = 5-val;
          printf ("%s", utf8_gray_scale[val]);
       }
       printf ("\n");
    }
  }

  ctx_free (ctx);
  exit (0);

  if (!strcmp (argv[1], "parse"))
    return parse_main (argc - 1, argv + 1);

  if (!strcmp (argv[1], "--help"))
    return help_main (argc - 1, argv + 1);

  return help_main (1, argv);
}
