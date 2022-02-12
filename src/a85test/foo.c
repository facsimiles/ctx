#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "../a85.h"

char *foo[]=

{ "a",
  "hello ctx",
  "hello ctx.",
  "hello ctx..",
  "hello ctx...",
  "hello ctx....",
  "hello ctx.....",
  "hello ctx......",
  "hello ctx.......",
  NULL};


int main()
{
  char tmp[256];
  char tmp2[256];
  for (int i = 0; foo[i]; i++)
  {
    ctx_a85enc (foo[i], tmp, strlen (foo[i]));
    if (ctx_a85enc_len (strlen (foo[i])) < (int)strlen (tmp) + 1)
    {
      fprintf (stderr, "needed more bytes than affored %i %i\n",
        (int)ctx_a85enc_len (strlen (foo[i])), (int)strlen (tmp) + 1);
    }
    if (ctx_a85len (tmp, strlen(tmp)) != (int)strlen (foo[i]))
    {
      fprintf (stderr, "for: [%s] %i != %i\n", foo[i],
          ctx_a85len (tmp, strlen(tmp)),
          (int)strlen (foo[i]));
    }
    ctx_a85dec (tmp, tmp2, strlen (tmp));
    if (strcmp (foo[i], tmp2))
    {
      fprintf (stderr, "[%s]\n", foo[i]);
      fprintf (stderr, "[%s]\n", tmp);
      fprintf (stderr, "[%s]\n\n", tmp2);
    }
  }

  return 0;
}
