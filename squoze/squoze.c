#include <stdio.h>
#include <libgen.h>
#include "squoze.h"

static int squoze_is_number (const char *str)
{
  if (str[0] == 0) return 0;
  for (const char *s = str; *s; s++)
    if (*s < '0' || *s > '9')
      return 0;
  return 1;
}

static int squoze_is_utfv (const char *str)
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
squoze_bin2base64 (const void *bin,
                int         bin_length,
                char       *ascii);

int
squoze_base642bin (const char    *ascii,
                int           *length,
                unsigned char *bin);

int usage (const char *base)
{
  fprintf (stderr,
"Usage: %s [options] <string|encoded> [string|encoded]\n", base);
  fprintf (stderr,
"\nwhere options are:\n"
"  --squoze32 -32  32 bit content adressed string hash\n"
"  --squoze52 -52 52 bit content adressed string hash\n"
"  --squoze62 -62 62 bit content adressed string hash\n"
"  --squoze5 -v   squoze-5 encoding, with [0-9][A-V] alphabet\n"
"  --utf-5 -5     UTF-5 encoding, with [0-9][A-V] alphabet\n"
"\n"
"  --encode -e    force encoding, the default is autodetect\n"
"  --base64 -b    use base64 as encoding for UTF-5/UTF-V\n"
"\n"
"Multiple short options can be set together -e32 forces encoding in SQUOZE32\n"
"Without options it is as if --squoze32 has been passed as arguments.\n"
"\n");
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

static int
squoze5_file_get_contents (const char     *path,
                           unsigned char **contents,
                           long           *length);


int main (int argc, char **argv)
{
  int dim = 10;
  int use_base64 = 0;
  int arg = 0;
  int force_encode = 0;
  char *base = basename (argv[arg++]);

  if (!argv[arg])
  {
     return usage (base);
  }
  const char *input_path = NULL;

  for (; argv[arg]; arg++)
  {
     if (argv[arg][0] != '-')
       break;
     if (argv[arg][1] == '-')
     {
     if (!strcmp (argv[arg], "--input")) { input_path = argv[arg+1];
             
             input_path = realpath (input_path, NULL);
             arg++; }

     if (!strcmp (argv[arg], "--help")) return usage (base);
     if (!strcmp (argv[arg], "--squoze32")) dim = 6;
     if (!strcmp (argv[arg], "--squoze52")) dim = 10;
     if (!strcmp (argv[arg], "--squoze62")) dim = 12;
     if (!strcmp (argv[arg], "--squoze-52")) dim = 10;
     if (!strcmp (argv[arg], "--squoze-62")) dim = 12;
     if (!strcmp (argv[arg], "--squoze-32")) dim = 6;
     if (!strcmp (argv[arg], "--utf-5")) dim = 5;
     if (!strcmp (argv[arg], "--utf5")) dim = 5;
     if (!strcmp (argv[arg], "--squoze5")) dim = 4;
     if (!strcmp (argv[arg], "--squoze-5")) dim = 4;
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
           switch (dim)
           {
             case 32: dim = 6; break;
             default:
             case 52: dim = 10; break;
             case 62: dim = 12; break;
             case 5: dim = 5; break;
             case 4: dim = 4; break;
           }
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


  if (input_path)
  {
    uint8_t  line[4095];
    int      len = 0;
    uint8_t *contents = NULL;
    long     length = 0;
    squoze5_file_get_contents (input_path, &contents, &length);
    if (length)
    {
      for (int i = 0; i <= length; i ++)
      {
        uint8_t val = 0;
        if (i < length) val = contents[i];

        switch (val)
        {
          case 0:
          case '\n':
            line[len] = 0;
            if (!strstr ((char*)line, "define TOKENHASH")
                    &&
               (strstr ((char*)line, "TOKENHASH(") &&
                strstr ((char*)line, "define")))
            {
              const char *input_str = NULL;
              
              if (strstr ((char*)line, "str="))
              {
                input_str = strdup ((strstr ((char*)line, "str=")+4));
              } else
              {
                if (strchr ((char*)line, '_'))
                {
                  input_str = strdup (strchr ((char*)line, '_') + 1);
                  if (strchr ((char*)input_str, ' '))
                    strchr ((char*)input_str, ' ')[0] = 0;
                }
                else
                  exit(-3);
              }

              char *escaped = strdup (input_str);

              for (int i = 0; escaped[i]; i++)
              {
                switch (escaped[i])
                {
                  case '-':
                  case ' ':
                    escaped[i] = '_';
                    break;
                  default:
                    break;
                }
              }
              {
                uint64_t val = 0;
                switch (dim)
                {
                  case 6:
                    val = squoze32 (input_str);
                    break;
                  case 10:
                    val = squoze52 (input_str);
                    break;
                  case 12:
                    val = squoze62 (input_str);
                    break;
                  default:
                    val = squoze52 (input_str);
                    break;
                }
                if (!strcmp (escaped, input_str))
                  printf ("#define CTX_%s TOKENHASH(%lu)\n", escaped, val);
                else
                  printf ("#define CTX_%s TOKENHASH(%lu)  // str=%s\n", escaped, val,
                                  input_str);
              }
              free (escaped);
            }
            else
            printf ("%s\n", line);
            len = 0;
            break;
          default:
            line[len++] = val;
            break;
        }
      }
    }
    return 0;
  }



  switch (dim)
  {
    case 4:
    case 5:
      for (int i = arg; argv[i]; i++)
      {
        print_sep ();
        if ((!force_encode) && (squoze_is_utfv (argv[i]) || use_base64  ))
        {
          int len = strlen (argv[i]);

          char *temp = NULL;
          char *result = malloc (len * 8);

          if (use_base64)
          {
            temp = calloc (len + 1, 1);
            uint8_t *binary = calloc (len + 10, 1);
            squoze_base642bin (argv[i], &len, binary);

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
          squoze_decode_utf5_bytes (1, (uint8_t*)temp, len,
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
          squoze5_encode (argv[i], strlen (argv[i]),
                            utf5, &utf5_len, dim==4, 0);
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
            squoze_bin2base64 (binary, (bitno+7)/8, (char*)base64);
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
        if ((!force_encode) && squoze_is_number (argv[i]))
          printf ("%s", squoze32_decode (atol(argv[i])));
        else
          printf ("%u", squoze32 (argv[i]));
      }
      break;
    case 10:
      for (int i = arg; argv[i]; i++)
      {
        print_sep ();
        if ((!force_encode) && squoze_is_number (argv[i]))
          printf ("%s", squoze52_decode (atol(argv[i])));
        else
          printf ("%lu", squoze52 (argv[i]));
      }
      break;
    case 12:
      for (int i = arg; argv[i]; i++)
      {
        print_sep ();
        if ((!force_encode) && squoze_is_number (argv[i]))
          printf ("%s", squoze62_decode (atol(argv[i])));
        else
          printf ("%lu", squoze62 (argv[i]));
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
squoze_bin2base64 (const void *bin,
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
squoze_base642bin (const char    *ascii,
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



static int
squoze5_file_get_contents (const char     *path,
                           unsigned char **contents,
                           long           *length)
{
  FILE *file;
  long  size;
  long  remaining;
  char *buffer;
  file = fopen (path, "rb");
  if (!file)
    { return -1; }
  fseek (file, 0, SEEK_END);
  size = remaining = ftell (file);
  if (length)
    { *length =size; }
  rewind (file);
  buffer = malloc (size + 8);
  if (!buffer)
    {
      fclose (file);
      return -1;
    }
  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      free (buffer);
      return -1;
    }
  fclose (file);
  *contents = (void *) buffer;
  buffer[size] = 0;
  return 0;
}

