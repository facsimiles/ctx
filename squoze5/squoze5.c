#include <stdio.h>
#include <libgen.h>
#include "squoze5.h"

static int cash_is_number (const char *str)
{
  if (str[0] == 0) return 0;
  for (const char *s = str; *s; s++)
    if (*s < '0' || *s > '9')
      return 0;
  return 1;
}

static int cash_is_utfv (const char *str)
{
  if (str[0] == 0) return 0;
  for (const char *s = str; *s; s++)
    if ( ! ((s[0] >= '0' && s[0] <= '9') ||
            (s[0] >= 'A' && s[0] <= 'V') ||
            (s[0] >= 'a' && s[0] <= 'v')
            ))
      return 0;
  return 1;
}

void
cash_bin2base64 (const void *bin,
                int         bin_length,
                char       *ascii);

int
cash_base642bin (const char    *ascii,
                int           *length,
                unsigned char *bin);

int usage (const char *base)
{
  fprintf (stderr, "Usage: %s [options] <string|encoded> [string|encoded]\n", base);
  fprintf (stderr, "\nwhere options are:\n"
                   "  --cash6 -6   32 bit content adressed string hash\n"
                   "  --cash10 -10 52 bit content adressed string hash\n"
                   "  --cash12 -12 62 bit content adressed string hash\n"
                   "  --utf-v -v   UTF-V encoding, with [0-9][A-V] alphabet\n"
                   "  --utf-5 -5   UTF-5 encoding, with [0-9][A-V] alphabet\n"
                   "\n"
                   "  --encode -e  force encoding, the default is autodetect\n"
                   "  --base64 -b  use base64 as encoding for UTF-5/UTF-V\n"
                   "\n"
                   "Multiple short options can be set together -e6 forces encoding in CASH-6\n");
  return 0;
}

static const char *utf5_alphabet="0123456789ABCDEFGHIJKLMNOPQRSTUV";

void print_sep (void)
{
  static int count = 0;
  if (count > 0)
  {
    printf (" ");
  }
  count ++;
}

int main (int argc, char **argv)
{
  int dim = 10;
  int use_base64 = 0;
  int arg = 0;
  int force_encode = 0;
  char *base = basename (argv[arg++]);
  if (base && !strcmp (base, "cash10")) dim = 10;
  if (base && !strcmp (base, "cash12")) dim = 12;
  if (base && !strcmp (base, "cash6")) dim = 6;
  if (base && !strcmp (base, "cash5")) dim = 5;
  if (base && !strcmp (base, "cashV")) dim = 4;

  if (!argv[arg])
  {
     return usage (base);
  }

  for (; argv[arg]; arg++)
  {
     if (argv[arg][0] != '-')
       break;
     if (argv[arg][1] == '-')
     {
     if (!strcmp (argv[arg], "--help")) return usage (base);
     if (!strcmp (argv[arg], "--cash10")) dim = 10;
     if (!strcmp (argv[arg], "--cash12")) dim = 12;
     if (!strcmp (argv[arg], "--cash6")) dim = 6;
     if (!strcmp (argv[arg], "--utf-5")) dim = 5;
     if (!strcmp (argv[arg], "--utf5")) dim = 5;
     if (!strcmp (argv[arg], "--utfv")) dim = 4;
     if (!strcmp (argv[arg], "--utf-v")) dim = 4;
     if (!strcmp (argv[arg], "--encode")) force_encode = 1;
     if (!strcmp (argv[arg], "--base64")) use_base64 = 1;
     }
     else
     {
       for (int i = 1; argv[arg][i]; i++)
       {
         int c = argv[arg][i];
         if (c >= '0' && c <= '9')
         {
           dim = atoi (&argv[arg][i]);
           break;
         }
       }
       for (int i = 1; argv[arg][i]; i++)
       {
         int c = argv[arg][i];
         switch (c)
         {
           case 'v':
           case 'V': dim = 4; break;
           case 'e': force_encode = 1; break;
           case 'b': use_base64 = 1; break;
           case 'h': return usage (base);
         }
       }
     }
  }

  switch (dim)
  {
    case 4:
    case 5:
      for (int i = arg; argv[i]; i++)
      {
        print_sep ();
        if ((!force_encode) && (cash_is_utfv (argv[i]) || use_base64  ))
        {
          int len = strlen (argv[i]);

          char *temp = NULL;
          char *result = malloc (len * 8);

          if (use_base64)
          {
            temp = calloc (len + 1, 1);
            uint8_t *binary = calloc (len + 10, 1);
            cash_base642bin (argv[i], &len, binary);

#define get_bit(no) ((binary[no/8] & (1<<(no%8)))!=0)
            int bitno = 0;
            for (int j = 0; j < (len*8/5); j++)
            {
              for (int b = 0; b < 5; b++, bitno++)
                if (get_bit (bitno))
                  temp[j] |= (1<<(b));
            }
#undef getbit
          }
          else
          {
            temp = malloc (strlen (argv[i]) + 4);
            int j = 0;
            for (j = 0; argv[i][j]; j++)
            {
              char in = argv[i][j];
              if (in && in <='9')
                temp[j] = in - '0';
              else if (in >= 'A' && in <='V')
                temp[j] = in - 'A' + 10;
              else if (in >= 'a' && in <='v')
                temp[j] = in - 'a' + 10;
            }
            temp[j++]=16; // append a 'G' causing as side effect flush
            temp[j] = 0;  //
          }

          int outlen = 0;
          cash_decode_utf5_bytes (1, (uint8_t*)temp, len,
                                  result, &outlen);
          free (temp);
          result[outlen]=0;
          printf ("%s ", result);
          //free (result);
        }
        else
        {
          char utf5[1024];
          int utf5_len = 0;
          cash_encode_utf5 (argv[i], strlen (argv[i]),
                            utf5, &utf5_len, dim==4);
          if (use_base64)
          {
            uint8_t binary[1024];
            uint8_t base64[1024*2];
            memset (binary, 0, sizeof (binary));
#define set_bit(no, val) if (val) binary[no/8] |= (1<<(no%8))
            int bitno = 0;
            for (int j = 0; j < utf5_len; j++)
            {
              for (int b = 0; b < 5; b++, bitno++)
                set_bit(bitno, ( utf5[j] & (1<< (b))) != 0);
            }
#undef set_bit
            cash_bin2base64 (binary, (bitno+7)/8, (char*)base64);
            printf ("%s", base64);
          }
          else
          {
            for (int j = 0; j < utf5_len; j++)
            {
              printf ("%c", utf5_alphabet[(int)utf5[j]]);
            }
          }
        }
      }
      break;
    case 6:
      for (int i = arg; argv[i]; i++)
      {
        print_sep ();
        if ((!force_encode) && cash_is_number (argv[i]))
          printf ("%s", cash6_decode (atol(argv[i])));
        else
          printf ("%u", cash6 (argv[i]));
      }
      break;
    case 10:
      for (int i = arg; argv[i]; i++)
      {
        print_sep ();
        if ((!force_encode) && cash_is_number (argv[i]))
          printf ("%s", cash10_decode (atol(argv[i])));
        else
          printf ("%lu", cash10 (argv[i]));
      }
      break;
    case 12:
      for (int i = arg; argv[i]; i++)
      {
        print_sep ();
        if ((!force_encode) && cash_is_number (argv[i]))
          printf ("%s", cash12_decode (atol(argv[i])));
        else
          printf ("%lu", cash12 (argv[i]));
      }
      break;
  }
  printf ("\n");
  return 0;
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
cash_bin2base64 (const void *bin,
                int         bin_length,
                char       *ascii)
{
  /* this allocation is a hack to ensure we always produce the same result,
   * regardless of padding data accidentally taken into account.
   */
  unsigned char *bin2 = (unsigned char*)calloc (bin_length + 4, 1);
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
  /* include variants used in URI encodings for decoder,
   * even if that is not how we encode
  */
  base64_revmap['-']=62;
  base64_revmap['_']=63;
  base64_revmap['+']=62;
  base64_revmap['/']=63;

  done = 1;
}


int
cash_base642bin (const char    *ascii,
                int           *length,
                unsigned char *bin)
{
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

