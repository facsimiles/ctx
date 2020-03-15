#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

char buf[2];

static void
at_exit (void)
{
  //tcdrain(STDIN_FILENO);
  //tcflush(STDIN_FILENO, 1);
  buf[0]=0;
  fwrite (buf, 1, 1, stdout);
  //fprintf(stdout, "\033[?4444l");
  fflush (stdout);
}

void
signal_int (int signum)
{
  exit (0);
}

int main (int argc, char **argv)
{
  fprintf(stdout, "\033[?4444h");
  signal (SIGINT, signal_int);
  signal (SIGTERM, signal_int);
  atexit (at_exit);
  while (fread (buf, 1, 1, stdin) == 1)
  {
    fwrite (buf, 1, 1, stdout);
  }
  return 0;
}
