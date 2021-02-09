/*
 *
 *  terminal state engine fuzzer
 *
 *  this is a program that generates adversarial input for vtxxx class
 *  emulators 2020 (c) pippin@gimp.org
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* the data generate on stdout is simultanously written to this file, for
 * reproducing and reducing test-cases
 */
#define LOG_PATH "/tmp/vtfuzz"

char *fragments[]= /* bits of input that are valid, change states, and more */
{
  "\e[?", "\e}", "\e]", "\e[?5ll", "\e[?53", "\e[22m", "\e[2m", "\e[9m", "\e[19m",
  "\e[4l", "\e[14;m", "\e[11;", "\e[;21;m", "\e[;43;m", "\e[13;m", "\e[10m", "\e[38;5;5;m",
  "\e[48;5;5;m", "\e[38;5;6;m", "\e[48;5;6;m", "\e[38;5;9;m", "\e[48;5;9;m",
  "\e[38;5;1;m", "\e[48;5;3;m", "\e[14;5;m", "\e[2;7;3;m", "\e[0a", "\e[?0g",
  "\e[?10q", "\e[2z", "\e[2D", "\e[1m", "\e[22m", "\e[7m", "\e[27m", "\e[4m",
  "\e[24m", "\e[5m", "\e[25m",
  "\e[10;10H", "\e[H", "\e[?7h", "\e[?7l", "\e[2J", "\e[K", "\e[1C", "\e[1D", "\n", "\e",
  "\r", "\0", "\08", "1", "<", ">", "~", ";", "2", "3", "5", "8", "a", "m", "n", "9",
  "l", "h", "d", "z", "c", "!", "10", "9", "f", "lorem ", "ipsum ", "dolor ", "sic ",
  "amet ", "mumpsimus ", "et ",
};

int main (int argc, char **argv)
{
  FILE *log;
  unlink (LOG_PATH);
  log = fopen (LOG_PATH, "w");
  int n_fragments = sizeof (fragments) / sizeof (fragments[0]);
  int seed = 23;
  if (argv[1])
    seed = atoi (argv[1]);    /* pass in an integer as argument to run with other seed  */
  srandom (seed);

  int max_noise_level = 10;
  int noise_level = 0;
  //for (int j = 0; j < 1000000; j++)
  for (int j = 0; j < 10000; j++)
  {

    if (j % 1000 == 0) {
        noise_level++;
        if (noise_level > max_noise_level)
	    noise_level = max_noise_level;
    }

    for (int i = 0; i < n_fragments; i++)
    {
      int shuf = random()%n_fragments;
      char *str = strdup (fragments[shuf]);
      int length = strlen (str);
      for (int l = 0; l < 1; l++)
      {
        for (int c = 0; c < length; c++)
        {
	  if ((random() >> 9) % 1000 < 5 * noise_level)
	    str[c] += random()%4;
	  if ((random() >> 9) % 1000 < 5 * noise_level)
	    str[c] -= random()%4;
	  if ((random() >> 9) % 1000 < 1 * noise_level)
	    str[c] = 0;
	  if ((random() >> 9) % 1000 < 1 * noise_level)
	    str[c] = 255;
        }
      }
      fwrite (str, length, 1, stdout);
      fwrite (str, length, 1, log);
      free (str);
    }
  }
  return 0;
}
