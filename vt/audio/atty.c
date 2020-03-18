#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

static struct termios orig_attr; /* in order to restore at exit */
static int    nc_is_raw = 0;
static int    atexit_registered = 0;

int tty_fd = STDIN_FILENO;

static void _nc_noraw (void)
{
  if (nc_is_raw && tcsetattr (tty_fd, TCSAFLUSH, &orig_attr) != -1)
    nc_is_raw = 0;
}

static void
nc_at_exit (void)
{
  fprintf(stderr, "\033[?4445l");
  fflush (stderr);
  _nc_noraw();
}


static int _nc_raw (void)
{
  struct termios raw;
  if (!isatty (tty_fd))
    return -1;
  if (!atexit_registered)
    {
      atexit (nc_at_exit);
      atexit_registered = 1;
    }
  if (tcgetattr (tty_fd, &orig_attr) == -1)
    return -1;
  raw = orig_attr;
  cfmakeraw (&raw);
  if (tcsetattr (tty_fd, TCSAFLUSH, &raw) < 0)
    return -1;
  nc_is_raw = 1;
  return 0;
}


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
int compression = '0';
int encoding = '0';
int type = 'u';
int buffer_samples = 512;
int lost_time = 0;
int lost_start;
int lost_end;

enum {
  ACTION_STATUS  = 0,
  ACTION_RESET,
  ACTION_SPEAKER,
  ACTION_MIC,
};

int action = ACTION_STATUS;

int has_data (int fd, int delay_ms)
{
  struct timeval tv;
  int retval;
  fd_set rfds;

  FD_ZERO (&rfds);
  FD_SET (fd, &rfds);
  tv.tv_sec = 0; tv.tv_usec = delay_ms * 1000;
  retval = select (fd+1, &rfds, NULL, NULL, &tv);
  return retval == 1 && retval != -1;
}


const char *terminal_response(void)
{
  static char buf[BUFSIZ * 2];
  int len = 0;
  fflush (stdout);

  while (has_data (tty_fd, 200) && len < BUFSIZ - 2)
  {
    if (read (tty_fd, &buf[len++], (size_t)1) == 1)
    {
       //fprintf (stderr, "uh\n");
    }
  }
  buf[--len] = 0;
  //close (tty);
  return buf;
}

void atty_readconfig (void)
{
  const char *cmd = "\033_Aa=q;\e\\";
  write (tty_fd, cmd, strlen (cmd));
  const char *ret = terminal_response ();
  if (ret[0])
  {
    if (!(ret[0] == '\e' &&
          ret[1] == '_' &&
          ret[2] == 'A'))
    {
     fprintf (stderr, "failed to initialize audio, unexpected response %li", strlen (ret));
     fflush (NULL);
     exit (-1);
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
    if (strstr (ret, "T="))
    {
      type = strstr (ret, "T=")[2];
    }
    if (strstr (ret, "e="))
    {
      encoding = strstr (ret, "e=")[2];
    }
    if (strstr (ret, "o="))
    {
      compression = strstr (ret, "o=")[2];
    }
  }
  else
  {
     fprintf (stderr, "failed to initialize audio, no response");
     //exit (-1);
  }
  fflush (NULL);
}

void atty_status (void)
{
  _nc_noraw ();
  fprintf (stdout, "samplerate=%i ", sample_rate);
  fprintf (stdout, "channels=%i ", channels);
  fprintf (stdout, "bits=%i ", bits);

  switch (type)
  {
     case 'u':
        fprintf (stdout, "type=ulaw ");
        break;
     case 's':
        fprintf (stdout, "type=signed ");
        break;
     case 'f':
        fprintf (stdout, "type=float ");
        break;
     default:
        fprintf (stdout, "type=%c ", type);
        break;
  }

  switch (encoding)
  {
    default:
    case '0':
        fprintf (stdout, "encoding=none ");
        break;
    case 'b':
        fprintf (stdout, "encoding=base64 ");
        break;
    case 'a':
        fprintf (stdout, "encoding=ascii85 ");
        break;
    case 'y':
        fprintf (stdout, "encoding=yenc ");
        break;
  }
  switch (compression)
  {
    default:
        fprintf (stdout, "compression=none");
        break;
    case 'z':
        fprintf (stdout, "compression=z");
        break;
    case 'o':
        fprintf (stdout, "compression=opus");
        break;
  }
  fprintf (stdout, "\n");
  fflush (NULL);
}

void atty_speaker (void)
{
  char audio_packet[4096];
  int  len = 0;

  fprintf(stdout, "\033[?4444h");
  signal (SIGINT, signal_int);
  signal (SIGTERM, signal_int);
  atexit (at_exit);

  lost_start = ticks ();

  while (fread (buf, 1, 1, stdin) == 1)
  {
    audio_packet[len++]=buf[0];

    fwrite (buf, 1, 1, stdout);

    lost_end = ticks();
    lost_time = (lost_end - lost_start);
    buffered_samples -= (sample_rate * lost_time / 1000);
    if (buffered_samples < 0)
      buffered_samples = 0;

    if (len >  buffer_samples)
    {
      fwrite (audio_packet, 1, len, stdout);
      usleep (1000 * ( len * 1000 / sample_rate - (ticks()-lost_end)) );
      len = 0;
    }
    lost_start = ticks ();
  }
}

int main (int argc, char **argv)
{
  char path[512];
  sprintf (path, "/proc/%d/fd/1", getppid());
  tty_fd = open (path, O_RDWR);

#if 1
  if (_nc_raw ())
  {
    fprintf (stdout, "nc raw failed\n");
  }
#endif
  char config[512]="";

  for (int i = 1; argv[i]; i++)
  {
    if (strchr (argv[i], '='))
    {
      const char *value = strchr (argv[i], '=')+1;
      char *key = strdup (argv[i]);
      *strchr(key, '=') = 0;
      if (!strcmp (key, "samplerate") || !strcmp (key, "s"))
      {
        sprintf (&config[strlen(config)],
	         "%ss=%i", config[0]?",":"", atoi(value));
      }
      else if (!strcmp (key, "bits") ||  !strcmp (key, "b"))
      {
        sprintf (&config[strlen(config)],
	         "%sb=%i", config[0]?",":"", atoi(value));
      }
      else if (!strcmp (key, "channels") ||  !strcmp (key, "c"))
      {
        sprintf (&config[strlen(config)],
	         "%sc=%i", config[0]?",":"", atoi(value));
      }
      else if (!strcmp (key, "type") ||  !strcmp (key, "T"))
      {
	if (!strcmp (value, "u")  ||
	    !strcmp (value, "ulaw"))
	{
          sprintf (&config[strlen(config)], "%sT=u", config[0]?",":"");
	}
	else if (!strcmp (value, "s")  ||
	         !strcmp (value, "signed"))
	{
          sprintf (&config[strlen(config)], "%sT=s", config[0]?",":"");
	}
	else if (!strcmp (value, "f")  ||
	         !strcmp (value, "float"))
	{
          sprintf (&config[strlen(config)], "%sT=f", config[0]?",":"");
	}
      }
      else if (!strcmp (key, "encoding") || !strcmp (key, "e"))
      {
	if (!strcmp (value, "y")  ||
	    !strcmp (value, "yenc"))
	{
          sprintf (&config[strlen(config)], "%se=y", config[0]?",":"");
	}
	else if (!strcmp (value, "a")  ||
	         !strcmp (value, "ascii85"))
	{
          sprintf (&config[strlen(config)], "%se=a", config[0]?",":"");
	}
	else if (!strcmp (value, "b")  ||
	         !strcmp (value, "base64"))
	{
          sprintf (&config[strlen(config)], "%se=b", config[0]?",":"");
	}
	else
	{
          sprintf (&config[strlen(config)], "%se=0", config[0]?",":"");
	}
      }
      else if (!strcmp (key, "compression") || !strcmp (key, "o"))
      {
	if (!strcmp (value, "opus")||
	    !strcmp (value, "o"))
	{
          sprintf (&config[strlen(config)], "%so=o", config[0]?",":"");
	}
	else if (!strcmp (value, "deflate")||
	         !strcmp (value, "z"))
	{
          sprintf (&config[strlen(config)], "%so=z", config[0]?",":"");
	}
	else
	{
          sprintf (&config[strlen(config)], "%so=0", config[0]?",":"");
	}
      }
      free (key);
    }
    else
    {
      if (!strcmp (argv[i], "status"))
      {
	action = ACTION_STATUS;
      }
      else if (!strcmp (argv[i], "reset"))
      {
	action = ACTION_RESET;
      }
      else if (!strcmp (argv[i], "mic"))
      {
	action = ACTION_MIC;
      }
      else if (!strcmp (argv[i], "speaker"))
      {
	action = ACTION_SPEAKER;
      }
      else if (!strcmp (argv[i], "--help"))
      {
	printf ("Usage: tty [mic|speaker|status|] key=value key=value\n");
	printf ("\n");
	printf ("\n");
	return 0;
      }
    }
  }

  if (config[0])
  {
    printf ("\033_A%s;\e\\", config);
    fflush (NULL);
  }

  switch (action)
  {
    case ACTION_RESET:
      printf ("\033_As=8000,T=u,b=8,c=1,o=0,e=0;\e\\");
      fflush (NULL);
      /*  fallthrough */
    case ACTION_STATUS:
      atty_readconfig ();
      atty_status ();
      break;
    case ACTION_SPEAKER:
      atty_readconfig ();
      atty_speaker ();
      break;
    //case ACTION_MIC:
    //  atty_mic ();
    //  break;
  }

  return 0;
}
