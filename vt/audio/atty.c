#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

int vt_a85enc (const void *srcp, char *dst, int count);
int vt_a85dec (const char *src, char *dst, int count);
int vt_a85len (const char *src, int count);
int vt_base642bin (const char    *ascii,
                   int           *length,
                   unsigned char *bin);
void
vt_bin2base64 (const void *bin,
               int         bin_length,
               char       *ascii);

static struct termios orig_attr; /* in order to restore at exit */
static int    nc_is_raw = 0;

int tty_fd = STDIN_FILENO;

static void _nc_noraw (void)
{
  if (nc_is_raw && tcsetattr (tty_fd, TCSAFLUSH, &orig_attr) != -1)
    nc_is_raw = 0;
}
int has_data (int fd, int delay_ms);

static void
at_exit_mic (void)
{
  fprintf(stderr, "\033_Am=0;\e\\");

  while (has_data (STDIN_FILENO, 100))
  {
    char c;
    read (STDIN_FILENO, &c, (size_t)1);
  }
  _nc_noraw();
  fflush (NULL);
  usleep (1000 * 100);
}

static void
at_exit_speaker (void)
{
  //tcdrain(STDIN_FILENO);
  //tcflush(STDIN_FILENO, 1);
  //fprintf(stderr, "\033[?4445l");
  //fprintf(stderr, "\033_Am=0;\e\\");
  _nc_noraw();
  fflush (NULL);
}

static int _nc_raw (void)
{
  struct termios raw;
  if (nc_is_raw)
    return 0;
  if (!isatty (tty_fd))
    return -1;
  if (tcgetattr (tty_fd, &orig_attr) == -1)
    return -1;
  raw = orig_attr;
  cfmakeraw (&raw);
  if (tcsetattr (tty_fd, TCSANOW, &raw) < 0)
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

void
signal_int_speaker (int signum)
{
  at_exit_speaker ();
  exit (0);
}

void
signal_int_mic (int signum)
{
  at_exit_mic ();
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

void atty_mic (void);
void atty_speaker (void)
{
  char audio_packet[4096 * 4];
  char audio_packet_a85[4096 * 8];
  int  len = 0;

  signal (SIGINT, signal_int_speaker);
  signal (SIGTERM, signal_int_speaker);
  atexit (at_exit_speaker);

  lost_start = ticks ();

  while (fread (buf, 1, 1, stdin) == 1)
  {
    audio_packet[len++]=buf[0];

    lost_end = ticks();
    lost_time = (lost_end - lost_start);
    buffered_samples -= (sample_rate * lost_time / 1000);
    if (buffered_samples < 0)
      buffered_samples = 0;

    if (len >  buffer_samples)
    {
      fprintf (stdout, "\033_Af=%i;", len / channels / (bits/8));

      int new_len = vt_a85enc (audio_packet, audio_packet_a85, len);
      audio_packet_a85[new_len]=0;
      fwrite (audio_packet_a85, 1, strlen (audio_packet_a85), stdout);
      fwrite ("\e\\", 1, 2, stdout);
      fflush (stdout);
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
        _nc_noraw();
	printf ("Usage: tty [mic|speaker] key1=value key2=value\n");
	printf ("\n");
	printf ("If neither mic nor speaker is specified as action, the\n");
	printf ("currently set keys are printed.\n");
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
      printf ("\033_As=8000,T=u,b=8,c=1,o=0,e=a;\e\\");
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
    case ACTION_MIC:
      atty_readconfig ();
      atty_mic ();
      break;
  }

  return 0;
}


static char a85_alphabet[]=
{
'!','"','#','$','%','&','\'','(',')','*',
'+',',','-','.','/','0','1','2','3','4',
'5','6','7','8','9',':',';','<','=','>',
'?','@','A','B','C','D','E','F','G','H',
'I','J','K','L','M','N','O','P','Q','R',
'S','T','U','V','W','X','Y','Z','[','\\',
']','^','_','`','a','b','c','d','e','f',
'g','h','i','j','k','l','m','n','o','p',
'q','r','s','t','u'
};

static char a85_decoder[256]="";

int vt_a85enc (const void *srcp, char *dst, int count)
{
  const uint8_t *src = srcp;
  int out_len = 0;
  int padding = 4 - (count % 4);
  for (int i = 0; i < (count+3)/4; i ++)
  {
    uint32_t input = 0;
    for (int j = 0; j < 4; j++)
    {
      input = (input << 8);
      if (i*4+j<count)
        input += src[i*4+j];
    }

    int divisor = 85 * 85 * 85 * 85;
    if (input == 0)
    {
        dst[out_len++] = 'z';
    }
    else
    {
      for (int j = 0; j < 5; j++)
      {
        dst[out_len++] = a85_alphabet[(input / divisor) % 85];
        divisor /= 85;
      }
    }
  }
  out_len -= padding;
  dst[out_len++]='~';
  dst[out_len++]='>';
  dst[out_len]=0;
  return out_len;
}

int vt_a85dec (const char *src, char *dst, int count)
{
  if (a85_decoder[0] == 0)
  {
    for (int i = 0; i < 85; i++)
    {
      a85_decoder[(int)a85_alphabet[i]]=i;
    }
  }
  int out_len = 0;
  uint32_t val = 0;
  int k = 0;

  for (int i = 0; i < count; i ++, k++)
  {
    val *= 85;

    if (src[i] == '~')
      break;
    else if (src[i] == 'z')
    {
      for (int j = 0; j < 4; j++)
        dst[out_len++] = 0;
      k = 0;
    }
    else
    {
      val += a85_decoder[(int)src[i]];
      if (k % 5 == 4)
      {
         for (int j = 0; j < 4; j++)
         {
           dst[out_len++] = (val & (0xff << 24)) >> 24;
           val <<= 8;
         }
         val = 0;
      }
    }
  }
  k = k % 5;
  if (k)
  {
    for (int j = k; j < 4; j++)
    {
      val += 84;
      val *= 85;
    }

    for (int j = 0; j < 4; j++)
    {
      dst[out_len++] = (val & (0xff << 24)) >> 24;
      val <<= 8;
    }
    val = 0;
    out_len -= (5-k);
  }
  dst[out_len]=0;
  return out_len;
}

int vt_a85len (const char *src, int count)
{
  int out_len = 0;
  int k = 0;

  for (int i = 0; src[i] && i < count; i ++, k++)
  {
    if (src[i] == '~')
      break;
    else if (src[i] == 'z')
    {
      out_len+=4;
      k = 0;
    }
    else
    {
      if (k % 5 == 4)
      {
         for (int j = 0; j < 4; j++)
         {
	   out_len++;
         }
      }
    }
  }
  k = k % 5;
  if (k)
  {
    for (int j = k; j < 4; j++)
    {
    }

    for (int j = 0; j < 4; j++)
    {
      out_len++;
    }
    out_len -= (5-k);
  }
  return out_len;
}


#define MIN(a,b) ((a)<(b)?(a):(b))

static int in_audio_data = 0;

static char audio_packet[65536];
static int audio_packet_pos = 0;

static int iterate (int timeoutms)
{
  unsigned char buf[20];
  int length;

  {
    int elapsed = 0;
    int got_event = 0;

    do {
#define DELAY_MS 100
      if (!got_event)
        got_event = has_data (STDIN_FILENO, MIN(DELAY_MS, timeoutms-elapsed));
      elapsed += MIN(DELAY_MS, timeoutms-elapsed);
      if (!got_event && timeoutms && elapsed >= timeoutms)
        return 1;
    } while (!got_event);
  }

  if (in_audio_data)
  {
    while (read (STDIN_FILENO, &buf[0], 1) != -1)
    {
      if (buf[0] == '\e')
      {
	in_audio_data = 2;
      }
      else if (buf[0] == '\\' &&
	       in_audio_data == 2)
      {
	if (encoding == 'a')
	{
          char *temp = malloc (vt_a85len (audio_packet, audio_packet_pos));
	  int len = vt_a85dec (audio_packet, temp, audio_packet_pos);
	  fwrite (temp, 1, len, stdout);
	  fflush (stdout);
	  free (temp);
	}
	else
	if (encoding == 'b')
	{
          uint8_t *temp = malloc (audio_packet_pos);
	  int len = audio_packet_pos;
	  vt_base642bin (audio_packet,
                  &len,
                  temp);
	  fwrite (temp, 1, len, stdout);
	  fflush (stdout);
	  free (temp);
	}

        audio_packet_pos = 0;
	in_audio_data = 0;
	return 1;
      }
      else
      {
	in_audio_data = 1;
        audio_packet[audio_packet_pos++] = buf[0];
	if (audio_packet_pos > 65535) audio_packet_pos = 65535;
      }
    }
    return 1;
  }

  for (length = 0; length < 10; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
         if (buf[0] == 3) /*  control-c */
         {
           return 0;
         }
	 else if (!strncmp ((void*)buf, "\033_A", MIN(length+1,3)))
         {
           int semis = 0;
           while (semis < 1 && read (STDIN_FILENO, &buf[0], 1) != -1)
	   {
	     if (buf[0] == ';') semis ++;
	   }
	   in_audio_data = 1;
	   return 1;
         }
      }
  return 1;
}

/*  return 0 if stopping */
int iterate (int timeoutms);

void atty_mic (void)
{
  signal(SIGINT,signal_int_mic);
  signal(SIGTERM,signal_int_mic);
  fprintf(stderr, "\033_Am=1;\e\\");
  _nc_raw ();
  fflush (NULL);
  while (iterate (1000));
  at_exit_mic ();
}

static const char *base64_map="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
static void bin2base64_group (const unsigned char *in, int remaining, char *out)
{
  unsigned char digit[4] = {0,0,64,64};
  int i;
  digit[0] = in[0] >> 2;
  digit[1] = ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4);
  if (remaining > 1)
    {
      digit[2] = ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6);
      if (remaining > 2)
        digit[3] = ((in[2] & 0x3f));
    }
  for (i = 0; i < 4; i++)
    out[i] = base64_map[digit[i]];
}

void
vt_bin2base64 (const void *bin,
               int         bin_length,
               char       *ascii)
{
  /* this allocation is a hack to ensure we always produce the same result,
   * regardless of padding data accidentally taken into account.
   */
  unsigned char *bin2 = calloc (bin_length + 4, 1);
  unsigned const char *p = bin2;
  int i;
  memcpy (bin2, bin, bin_length);
  for (i=0; i*3 < bin_length; i++)
   {
     int remaining = bin_length - i*3;
     bin2base64_group (&p[i*3], remaining, &ascii[i*4]);
   }
  free (bin2);
  ascii[i*4]=0;
}

static unsigned char base64_revmap[255];
static void base64_revmap_init (void)
{
  static int done = 0;
  if (done)
    return;

  for (int i = 0; i < 255; i ++)
    base64_revmap[i]=255;
  for (int i = 0; i < 64; i ++)
    base64_revmap[((const unsigned char*)base64_map)[i]]=i;
  /* include variants used in URI encodings for decoder */
  base64_revmap['-']=62;
  base64_revmap['_']=63;
  base64_revmap['+']=62;
  base64_revmap['/']=63;

  done = 1;
}

int
vt_base642bin (const char    *ascii,
               int           *length,
               unsigned char *bin)
{
  // XXX : it would be nice to transform this to be able to do
  //       the conversion in-place, reusing the allocation
  int i;
  int charno = 0;
  int outputno = 0;
  int carry = 0;
  base64_revmap_init ();
  for (i = 0; ascii[i]; i++)
    {
      int bits = base64_revmap[((const unsigned char*)ascii)[i]];
      if (length && outputno > *length)
        {
          *length = -1;
          return -1;
        }
      if (bits != 255)
        {
          switch (charno % 4)
            {
              case 0:
                carry = bits;
                break;
              case 1:
                bin[outputno] = (carry << 2) | (bits >> 4);
                outputno++;
                carry = bits & 15;
                break;
              case 2:
                bin[outputno] = (carry << 4) | (bits >> 2);
                outputno++;
                carry = bits & 3;
                break;
              case 3:
                bin[outputno] = (carry << 6) | bits;
                outputno++;
                carry = 0;
                break;
            }
          charno++;
        }
    }
  bin[outputno]=0;
  if (length)
    *length= outputno;
  return outputno;
}
