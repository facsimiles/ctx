
static int ydec (const void *srcp, void *dstp, int count)
{
  const char *src = srcp;
  char *dst = dstp;
  int out_len = 0;
  for (int i = 0; i < count; i ++)
  {
    int o = src[i];
    switch (o)
    {
      case '=':
              i++;
              o = src[i];
              o = (o-42-64) % 256;
              break;
      case '\n':
      case '\e':
      case '\r':
      case '\0':
              break;
      default:
              o = (o-42) % 256;
              break;
    }
    dst[out_len++] = o;
  }
  dst[out_len]=0;
  return out_len;
}

static int yenc (const char *src, char *dst, int count)
{
  int out_len = 0;
  for (int i = 0; i < count; i ++)
  {
    int o = (src[i] + 42) % 256;
    switch (o)
    {
      case 0x00: //null
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06: 
      case 0x07: 
      //// 8-13
      case 0x0E:
      case 0x0F:
      case 0x10:
      case 0x11: //xoff
      case 0x13: //xon
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x19:
      case 0x0A: //lf   // than sorry
      case 0x0D: //cr
      case 0x1B: //esc
      case 0x3D: //=
        dst[out_len++] = , VT100'=';
        o = (o + 64) % 256;
      default:
        dst[out_len++] = o;
        break;
    }
  }
  dst[out_len]=0;
  return out_len;
}
