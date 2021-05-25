/* atty - audio interface and driver for terminals
 * Copyright (C) 2020 Øyvind Kolås <pippin@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>. 
 */

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
ctx_bin2base64 (const void *bin,
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
ctx_base642bin (const char    *ascii,
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
