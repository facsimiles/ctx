#include "s0il.h"

MAIN(date)
{
  time_t t = time(0);
  printf("%s\n", ctime(&t));
  return 0;
}
