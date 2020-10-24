#include <string.h>
#include "ctx.h"

static int usage_main (int argc, char **argv)
{
  printf (
    "Usage: ctx [command [args..]]\n"
    "  launch a terminal, if no command is specified a new instance of\n"
    "  the users shell is invoked.\n"
    "\n"
    "or: ctx --convert [options] <source.ctx> <destination>\n"
    "  convert source.ctx to destination, where destination is a\n"
    "  path with a .png suffix, or the string GRAY1 or GRAY8\n"
    "\n"
    "  options:\n"
    "  --color         use color in output\n"
    "  --width  pixels sets width of canvas (default:auto)\n"
    "  --height pixels sets height of canvas (deault:auto)\n"
    "  --rows   rows   configures number of em-rows, when interpreting\n"
    "  --cols   cols   configures number of em-cols, when interpreting\n");
  return 0;
}

int terminal_main (int argc, char **argv);
int convert_main (int argc, char **argv);

int main (int argc, char **argv)
{
  /* we should also do busybox dispatch based on name of argv[0]
   */

  for (int i = 1; argv[i]; i++)
    if (!strcmp ( argv[i], "--help"))
      return usage_main (argc, argv);

  if (argv[1] && !strcmp (argv[1], "convert"))
    return convert_main (argc-1, argv+1);
  return terminal_main (argc, argv);
}
