#include <s0il.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv) {
  for (int i = 0; i < 10; i++) {
    printf("Hi ! %i\n", i);
    sleep(1);
  }

  return 0;
}
