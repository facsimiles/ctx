#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>


static long int ticks (void)
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

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

int buffered_samples = 0;
int sample_rate = 8000;
int bits = 8;
int channels = 1;
int ulaw = 1;
int buffer_samples = 512;
int lost_time = 0;
int lost_start;
int lost_end;

int main (int argc, char **argv)
{
  int slack = buffer_samples;

  printf ("\033_Aa=q;\e\\");
  fflush (NULL);
  usleep (1000 * 100);
  char ret[256];
  int res = read (1, ret, 45);
  if (res > 0)
  {
    ret[res] = 0;

    if (!(ret[0] == '\e' &&
          ret[1] == '_' &&
          ret[2] == 'A'))
    {
     fprintf (stderr, "failed to initialize audio, unexpected response %li %c%c%c", strlen (ret), ret[1], ret[2], ret[3]);
     return -1;
    }
    if (strstr (ret, "s="))
    {
      sample_rate = atoi (strstr (ret, "s=")+2);
    }
    if (strstr (ret, "c="))
    {
      channels = atoi (strstr (ret, "c=")+2);
    }
    if (strstr (ret, "b="))
    {
      bits = atoi (strstr (ret, "b=")+2);
    }
    if (strstr (ret, "T=u"))
    {
      ulaw = 1;
    }
    else
    {
      ulaw = 0;
    }
    fprintf (stderr, "rate:%i bits:%i ulaw:%i channels:%i", sample_rate, bits, ulaw, channels);
  }
  else
  {
     fprintf (stderr, "failed to initialize audio, no response");
     exit (-1);
  }


  fprintf(stdout, "\033[?4444h");
  signal (SIGINT, signal_int);
  signal (SIGTERM, signal_int);
  atexit (at_exit);


  lost_start = ticks ();
  while (fread (buf, 1, 1, stdin) == 1)
  {
    fwrite (buf, 1, 1, stdout);
    lost_end = ticks();
    lost_time = (lost_end - lost_start);
    buffered_samples -= (sample_rate * lost_time / 1000);
    if (buffered_samples < 0)
      buffered_samples = 0;
    buffered_samples ++;

    if (buffered_samples > slack)
    {
      usleep (1000 * slack * 1000 / sample_rate);
      buffered_samples -= slack;
    }
    lost_start = ticks ();
  }

  return 0;
}
