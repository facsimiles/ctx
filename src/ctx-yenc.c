#include <stdio.h>
#include <string.h>

static int ctx_yenc (const char *src, char *dst, int count)
{
  int out_len = 0;
  for (int i = 0; i < count; i ++)
  {
    int o = (src[i] + 42) % 256;
    switch (o)
    {
      case 0x00: //null
      case 0x20: //space// but better safe
      case 0x0A: //lf   // than sorry
      case 0x0D: //cr
      case 0x09: //tab  // not really needed
      case 0x10: //datalink escape (used by ctx)
      case 0x11: //xoff
      case 0x13: //xon
      case 0x1b: //
      case 0xff: //
      case 0x3D: //=
        dst[out_len++] = '=';
        o = (o + 64) % 256;
        /* FALLTHROUGH */
      default:
        dst[out_len++] = o;
        break;
    }
  }
  dst[out_len]=0;
  return out_len;
}

static int ctx_ydec (const char *tmp_src, char *dst, int count)
{
  const char *src = tmp_src;
#if 0
  if (tmp_src == dst)
  {
    src = malloc (count);
    memcpy (src, tmp_src, count);
  }
#endif
  int out_len = 0;
  for (int i = 0; i < count; i ++)
  {
    int o = src[i];
    switch (o)
    {
      case '=':
        i++;
        o = src[i];
        if (o == 'y')
        {
          dst[out_len]=0;
#if 0
          if (tmp_src == dst) free (src);
#endif
          return out_len;
        }
        o = (o-42-64) % 256;
        dst[out_len++] = o;
        break;
      case '\n':
      case '\e':
      case '\r':
      case '\0':
        break;
      default:
        o = (o-42) % 256;
        dst[out_len++] = o;
        break;
    }
  }
  dst[out_len]=0;
#if 0
  if (tmp_src == dst) free (src);
#endif
  return out_len;
}

#if 0
int main (){
  char *input="this is a testæøåÅØ'''\"!:_asdac\n\r";
  char  encoded[256]="";
  char  decoded[256]="";
  int   in_len = strlen (input);
  int   out_len;
  int   dec_len;

  printf ("input: %s\n", input);

  out_len = ctx_yenc (input, encoded, in_len);
  printf ("encoded: %s\n", encoded);

  dec_len = ydec (encoded, encoded, out_len);

  printf ("decoded: %s\n", encoded);

  return 0;
}
#endif
