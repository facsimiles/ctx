#include "s0il.h"

MAIN(date)
{
  time_t t = time(0);
  struct tm *local_time;
  local_time = localtime(&t);
  printf("%s", asctime(local_time));

  return 0;
}
