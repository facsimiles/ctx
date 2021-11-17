#include <stdio.h>
#define CTX_IMPLEMENTATION
#include "ctx.h"

int is_number (const char *str)
{
  if (str[0] == 0) return 0;
  for (char *s = str; *s; s++)
    if (*s < '0' || *s > '9')
      return 0;
  return 1;
}

int usage (void)
{
  fprintf (stderr, "thash <string|integer>\n");
  return 0;
}

int main (int argc, char **argv)
{
  if (!argv[1])
  {
     return usage ();
  }
  for (int i = 1; argv[i]; i++)
  {
    if (is_number (argv[i]))
    {
      printf ("%s ", thash_decode (atol(argv[i])));
    }
    else
    {
      printf ("%li ", thash (argv[i]));
    }
  }
  printf ("\n");
  return 0;
}
