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

  if (!dest_path)
  {
     ctx_render_stream (ctx, stdout);
     exit (0);
  }

  ctx_free (ctx);
  exit (0);

  if (!strcmp (argv[1], "parse"))
    return parse_main (argc - 1, argv + 1);
  if (!strcmp (argv[1], "--help"))
    return help_main (argc - 1, argv + 1);

  return help_main (1, argv);
}
