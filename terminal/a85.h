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

static int a85enc (const void *srcp, char *dst, int count)
{
  const uint8_t *src = srcp;
  int out_len = 0;

  int padding = 4-(count % 4);
  if (padding == 4) padding = 0;

  for (int i = 0; i < (count+3)/4; i ++)
  {
    uint32_t input = 0;
    for (int j = 0; j < 4; j++)
    {
      input = (input << 8);
      if (i*4+j<=count)
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
        dst[out_len++] = ((input / divisor) % 85) + '!';
        divisor /= 85;
      }
    }
  }

  out_len -= padding;

  dst[out_len++]='~';
  dst[out_len]=0;
  return out_len;
}

static int a85dec (const char *src, char *dst, int count)
{
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
      val += src[i]-'!';
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
    val += 84;
    for (int j = k; j < 4; j++)
    {
      val *= 85;
      val += 84;
    }

    for (int j = 0; j < k-1; j++)
    {
      dst[out_len++] = (val & (0xff << 24)) >> 24;
      val <<= 8;
    }
    val = 0;
  }
  dst[out_len]=0;
  return out_len;
}

static int a85len (const char *src, int count)
{
  int out_len = 0;
  int k = 0;

  for (int i = 0; src[i] && i < count; i ++, k++)
  {
    if (src[i] == '~')
      break;
    else if (src[i] == 'z')
    {
      out_len += 4;
      k = 0;
    }
    else
    {
      if (k % 5 == 4)
      {
        out_len += 4;
      }
    }
  }
  k = k % 5;
  if (k)
  {
    out_len += 4;
    out_len -= (5-k);
  }
  return out_len + 10; // XXX redo a85len, a85dec seems to decode more
}
