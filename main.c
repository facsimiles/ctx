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

int launch_main (int argc, char **argv)
{
  // check that we have a term
  // and that we can launch
  int   can_launch = 0;
  int   no_title = 0;
  int   no_move = 0;
  int   no_resize = 0;
  int   layer = 0;
  // escape subsequent arguments so that we dont have to pass a string?
  int   no = 1;
  float x = -1.0;
  float y = -1.0;
  float width = -1.0;
  float height = -1.0;
  for (no = 1; argv[no] && argv[no][0]=='-'; no++)
  {
    if (!strcmp (argv[no], "--no-title"))
    {
      no_title = 1;
    }
    if (!strcmp (argv[no], "--no-move"))
    {
      no_move = 1;
    }
    if (!strcmp (argv[no], "--no-resize"))
    {
      no_resize = 1;
    }
    if (!strcmp (argv[no], "--can-launch"))
    {
      can_launch = 1;
    }
    if (!strcmp (argv[no], "--can-launch=1"))
    {
      can_launch = 1;
    }
    else if (!strcmp (argv[no], "-z=0"))
    {
      layer = 0;
    }
    else if (!strcmp (argv[no], "-z=-1"))
    {
      layer = -1;
    }
    else if (!strcmp (argv[no], "-z=1"))
    {
      layer = 1;
    }
    else if (!strcmp (argv[no], "--layer=background"))
    {
      layer = -1;
    }
    else if (!strcmp (argv[no], "--layer=foreground"))
    {
      layer = 1;
    }
    else if (strstr (argv[no], "--x="))
    {
      x = atof (argv[no + strlen ("--x=")]);
    }
    else if (strstr (argv[no], "--y="))
    {
      y = atof (argv[no + strlen ("--y=")]);
    }
    else if (strstr (argv[no], "--width="))
    {
      width = atof (argv[no + strlen ("--width=")]);
    }
    else if (strstr (argv[no], "--height="))
    {
      height = atof (argv[no + strlen ("--height=")]);
    }
  }

  fprintf (stdout, "\e_C");
  if (layer == -1) fprintf (stdout, "z=-1,");
  if (layer ==  1) fprintf (stdout, "z=1,");
  if (can_launch)  fprintf (stdout, "can_launch=1,");
  if (x>0)         fprintf (stdout, "x=%.0f,", x);
  if (y>0)         fprintf (stdout, "y=%.0f,", y);
  if (width>0)     fprintf (stdout, "width=%.0f,", x);
  if (height>0)    fprintf (stdout, "height=%.0f,", y);
  if (no_title)    fprintf (stdout, "no_title=1,");
  if (no_move)     fprintf (stdout, "no_move=1,");
  if (no_resize)   fprintf (stdout, "no_resize=1,");
  fprintf (stdout, ";%s\e\\", argv[no]);
  return 0;
}

int main (int argc, char **argv)
{
  for (int i = 1; argv[i]; i++)
    if (!strcmp ( argv[i], "--help"))
      return usage_main (argc, argv);

  if (argv[1] && !strcmp (argv[1], "convert"))
    return convert_main (argc-1, argv+1);
  if (argv[1] && !strcmp (argv[1], "launch"))
    return launch_main (argc-1, argv+1);
  return terminal_main (argc, argv);
}
