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
  size_t size;
  while (getline(&line, &size, stdin) != -1) {
     ctx_parse_str_line (ctx, line);
  }

  struct termios termios_original;
  struct termios termios_set;
  tcgetattr(0, &termios_original);
  termios_set = termios_original;
  cfmakeraw(&termios_set);
  tcsetattr(0, TCSANOW, &termios_set);
  int count = ctx_get_renderstream_count (ctx);

  printf ("\e[?2222h");
  printf ("%c        ", CTX_CLEAR);

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
  printf ("Usage: ctx [command [command args]]\n\n"
	  "where command is one of:\n"
	  " vt    - virtual terminal, running ctx with no args\n    also launches a terminal\n"
	  " parse - parse ascii, and generate ctx on stdout\n");
  return 0;
}

int vt_main (int argc, char **argv);

int main (int argc, char **argv)
{
  if (!argv[1])
  {
    return vt_main (argc, argv);
  }
  if (!strcmp (argv[1], "parse"))
    return parse_main (argc - 1, argv + 1);
  if (!strcmp (argv[1], "vt"))
    return vt_main (argc - 1, argv + 1);
  if (!strcmp (argv[1], "--help"))
    return help_main (argc - 1, argv + 1);

  return help_main (1, argv);
}
