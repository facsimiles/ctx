
//#define SQUOZE_REF_SANITY      1
//#define SQUOZE_CLOBBER_ON_FREE 1

#define SQUOZE_IMPLEMENTATION
#include "squoze.h"

int main (int argc, char **argv)
{
  char temp[16];
  Sqz *string = NULL;
#define log(str) \
  printf ("%i:[%s] length:%i id:%lu\n", __LINE__, sqz_decode (str, temp), sqz_length (str), sqz_id (str));
  log(string);

  sqz_set_utf8 (&string, "");
  log(string);
  
  sqz_set_utf8 (&string, "hello");
  log(string);


  sqz_insert_unichar (&string, -1, ' ');
  log(string);

  sqz_insert_utf8 (&string, -1, "æøå");
  log(string);
  
  sqz_erase (&string, -1, 1);
  log(string);
  
  sqz_replace_utf8 (&string, -1, 1, "_");
  log(string);
  
  sqz_erase (&string, -3, 1);
  log(string);

  sqz_erase (&string, -2, 2);
  log(string);

  sqz_erase (&string, 0, 1);
  log(string);

  sqz_insert_utf8 (&string, 0, "p");
  log(string);

  sqz_unset (&string);
  log(string);

  sqz_set_printf (&string, "%f", 3/11.0);
  log(string);

  //sqz_cleanup ();
  return 0;
}

