#include <stdio.h>
#include <unistd.h>

int main (int argc, char **argv)
{
  char buf[64];
  size_t len = 0;
  printf ("login: ");
  ssize_t size = fread(&buf[0], 4, 1, stdin);
  buf[4] = 0;
  //sleep (1);
  printf ("got: [%s]\n", buf);


  return 0;
}
