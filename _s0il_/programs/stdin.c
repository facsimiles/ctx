#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  char buf[64];
  printf("login: ");
  fread(&buf[0], 4, 1, stdin);
  buf[4] = 0;
  printf("got: [%s]\n", buf);

  return 0;
}
