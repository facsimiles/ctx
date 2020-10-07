#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <SDL.h>

#include "ctx.h"

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

static int usage_main (int argc, char **argv)
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

int terminal_main (int argc, char **argv);
int convert_main (int argc, char **argv);

int main (int argc, char **argv)
{
  for (int i = 1; argv[i]; i++)
    {
      if (argv[i][0] == '-')
        {
          if (!strcmp ( argv[i], "--convert") ||
              !strcmp ( argv[i], "-c"))
            return convert_main (argc, argv);
          if (!strcmp ( argv[i], "--parse"))
            return parse_main (argc, argv);
          if (!strcmp ( argv[i], "--help"))
            return usage_main (argc, argv);
        }
    }

  return terminal_main (argc, argv);
}
