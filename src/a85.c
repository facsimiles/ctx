 /* Copyright (C) 2020 Øyvind Kolås <pippin@gimp.org>
 */

#if CTX_FORMATTER

/* returns the maximum string length including terminating \0 */
int ctx_a85enc_len (int input_length)
{
  return (input_length / 4 + 1) * 5;
}

int ctx_a85enc (const void *srcp, char *dst, int count)
{
  const uint8_t *src = (uint8_t*)srcp;
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
#if 0
    if (input == 0)
    {
        dst[out_len++] = 'z';
    }
    /* todo: encode 4 spaces as 'y' */
    else
#endif
    {
      for (int j = 0; j < 5; j++)
      {
        dst[out_len++] = ((input / divisor) % 85) + '!';
        divisor /= 85;
      }
    }
  }
  out_len -= padding;
  dst[out_len]=0;
  return out_len;
}
#endif

#if CTX_PARSER

int ctx_a85dec (const char *src, char *dst, int count)
{
  int out_len = 0;
  uint32_t val = 0;
  int k = 0;
  int i = 0;
  int p = 0;
  for (i = 0; i < count; i ++)
  {
    p = src[i];
    val *= 85;
    if (p == '~')
    {
      break;
    }
#if 0
    else if (p == 'z')
    {
      for (int j = 0; j < 4; j++)
        dst[out_len++] = 0;
      k = 0;
    }
    else if (p == 'y') /* lets support this extension */
    {
      for (int j = 0; j < 4; j++)
        dst[out_len++] = 32;
      k = 0;
    }
#endif
    else if (p >= '!' && p <= 'u')
    {
      val += p-'!';
      if (k % 5 == 4)
      {
         for (int j = 0; j < 4; j++)
         {
           dst[out_len++] = (val & (0xff << 24)) >> 24;
           val <<= 8;
         }
         val = 0;
      }
      k++;
    }
    // we treat all other chars as whitespace
  }
  if (p != '~')
  { 
    val *= 85;
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
  dst[out_len] = 0;
  return out_len;
}

#if 0
int ctx_a85len (const char *src, int count)
{
  int out_len = 0;
  int k = 0;
  for (int i = 0; i < count; i ++)
  {
    if (src[i] == '~')
      break;
    else if (src[i] == 'z')
    {
      for (int j = 0; j < 4; j++)
        out_len++;
      k = 0;
    }
    else if (src[i] >= '!' && src[i] <= 'u')
    {
      if (k % 5 == 4)
        out_len += 4;
      k++;
    }
    // we treat all other chars as whitespace
  }
  k = k % 5;
  if (k)
    out_len += k-1;
  return out_len;
}
#endif

#endif
