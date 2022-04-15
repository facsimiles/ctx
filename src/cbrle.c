/*
 * each scanline is encoded as a header + rle data + free space + reversed palette
 * 
 * header:  u16_length, u8_colors, u8_0
 *
 * The u8_0 padding byte needs to be 0 in the future it might hold higher values, but
 * always smaller than colors, this to make it use a value that never naturally occurs
 * for u8 associated alpha, making it possible to store either CBRLE or raw pixel
 * values in 32bpp scanlines.
 * 
 * The indexed palette is per-scanline, the first eight palette entries are
 * fixed as a grayscale. The palette is allocated on the fly and is stored from
 * the end of the scanline. As the palette fills up it gets harder and harder
 * for new colors to warrant a allocating new entries, gradients only allocate
 * colors for their start and end colors.
 *
 * When we run out of free space the scanline gets recompressed, reducing color
 * depth and simplifying pixel data, for 32bpp scanlines this can in the future
 * mean fall over to uncompressed raw data. Doing composiing on RLE encoded
 * data can on high resolution framebuffers lead to significantly lower memory
 * bandwidth and computation required on frames with significant overdraw; to
 * fully realise this potential further integration is needed.
 *
 */



#if CTX_IMPLEMENTATION || CTX_SIMD_BUILD
#if CTX_ENABLE_CBRLE


#define GRADIENT_THRESHOLD     4
#define COLOR_THRESHOLD_START  3
#define COLOR_THRESHOLD_END    (color_budget<=128?32:21)
#define MERGE_THRESHOLD        64   //(no<2?50:98)
#define MERGE_CHANCE           42   //(no<2?25:80)

#define MAX_GRADIENT_LENGTH    (color_budget<=128?8:6)
#define MAX_RECOMPRESS         2
#define BORDER                 1

enum {
        CBRLE_MODE_SET=0,
        CBRLE_MODE_OVER=1,
        CBRLE_MODE_COPY=2,
        CBRLE_MODE_SET_COLOR=3
};


///  max 32 or 64 colors per scanline
//      
//     1  5 2
//     1  7    8
//     1         4  4  4 1 1 1  - the last 3bits are lengths 0 means 1 and 1 means 2 for 2 first
static inline int encode_pix (uint8_t *rgba8z, int pos, 
                              uint8_t idx, int range, int gradient,
                              int allow_merge)
{
#if 0
   if (allow_merge && pos > 3)
   {
#if 1 
      if (rgba8z[pos-1] < 128 &&  //
          rgba8z[pos-2] < 128 &&  // previous must be single
          (rgba8z[pos-1] & 31) == idx)
      {
         if ((rgba8z[pos-1] >> 5) + 1 + range <= 4)
         {
           range += (rgba8z[pos-1] >> 5) + 1;
           range --;
           rgba8z[pos-1] = range * 32 + idx;
           return 0;
         }
         else if ((rgba8z[pos-1] >> 5) + 1 + range < 62 && 0)
         {
           range += (rgba8z[pos-1] >> 5);
           rgba8z[pos-1] = range + 128 + 0 * 64;
           rgba8z[pos] = idx;
           return 1;
         }
      }
#endif
   }
#endif

   if (range > 64)
     fprintf (stderr, "ERROR trying to encode too big range %i!\n", range);
   //gradient = 0;
   if (idx < 32 && range <= 4 && !gradient)
   {
     rgba8z[pos] = (range-1) * 32 + idx;
     return 1;
   }
   else
   {
     rgba8z[pos] = (range) + 128 + gradient * 64;
     rgba8z[pos+1] = idx;
     return 2;
   }
}


// pix: pointer to data to decode
// ret_col: where to store 8bit color index
// length_px: number of pixels wide
// gradient: set to 1 if it is a gradient
//
// returns number of bytes in encoded form.
static inline int decode_pix (const uint8_t *pix,
                              uint8_t       *ret_col,
                              int           *length_px,
                              int           *gradient)
{
  int encoded_length = 1;
  int len = 1;
  int col = 0;
  if (gradient)
    *gradient = 0;
  if ((*pix & 0x80) == 0)
  {
    col = ( pix[0] & 31);
    len = pix[0] >> 5;
    len++;
  }
  else
  {
    len = pix[0]-128;
    if (len > 64)
    {
      len -= 64;
      if(gradient)
        *gradient = 1;
    }
    col = pix[1];
    encoded_length = 2;
  }
  if (length_px)
    *length_px = len;
  if (ret_col)
    *ret_col = col;
  return encoded_length;
}


static inline int decode_pix_len (const uint8_t *pix, int *length_px)
{
  return decode_pix (pix, NULL, length_px, NULL);
}


static inline uint32_t color_diff (uint32_t a, uint32_t b)
{
  uint32_t sum_dist = 0;
  for (int c = 0; c < 4; c++)
  {
    sum_dist += ((a&0xff)-(b&0xff))*((a&0xff)-(b&0xff));
    a>>=8;
    b>>=8;
  }
  return sum_dist;
}

static inline int is_b_good_middle (uint32_t a, uint32_t b, uint32_t c, int color_budget)
{
  uint32_t lerped = ctx_lerp_RGBA8 (a, c, 127);

  if (a == c) return 0; // gradients are more expensive than not
  uint32_t sum_dist = color_diff (lerped, b);
  return (sum_dist < (GRADIENT_THRESHOLD*GRADIENT_THRESHOLD*3));
}

static inline int
ctx_CBRLE_recompress (uint8_t *rgba8z, int size, int width, int pos, int level);

#define GRAYCOL(v) ((v) + (v) * 256 + (v) * 256 * 256 + (unsigned)255*256*256*256)
static const uint32_t hard_pal[8]={
        GRAYCOL(0*255/7),
        GRAYCOL(1*255/7),
        GRAYCOL(2*255/7),
        GRAYCOL(3*255/7),
        GRAYCOL(4*255/7),
        GRAYCOL(5*255/7),
        GRAYCOL(6*255/7),
        GRAYCOL(7*255/7)
};
#include <math.h>
#define pow2(a) ((a)*(a))
#define ctx_sqrtf sqrtf

static inline int
ctx_CBRLE_get_color_mask (int gen)
{
  switch (gen)
  {
    case 0:
    case 1:
    default:
      return 0xffffffff;
    case 2:
      return 0xfffefefe;
    case 3:
      return 0xfffcfcfc;
    case 4:
      return 0xfff8f8f8;
    case 5:
      return 0xfff0f0f0;
    case 6:
      return 0xffe0e0e0;
  }
  return 0xffe0e0e0;
}


static inline int
ctx_CBRLE_get_color_idx (uint8_t *rgba8z, int size, int color_budget, uint32_t prev_val, int gen)
{
  int found = 0;
  int colors = rgba8z[2];
  int idx = 0;
    uint32_t threshold =
       (uint32_t)
       ctx_lerpf (COLOR_THRESHOLD_START*COLOR_THRESHOLD_START*3,
                  COLOR_THRESHOLD_END*COLOR_THRESHOLD_END*3, 
                  //((colors  / ( color_budget-1.0f))) );
                  ctx_sqrtf((colors  / ( color_budget-1.0f))) );

#if 0 // reduce color-depth of grays
  if (gen > 0 &&
      (prev_val&0xf0) == ((prev_val >> 8) & 0xf0) &&
      (prev_val&0xf0) == ((prev_val >> 16) & 0xf0))
  {
    prev_val  = hard_pal[ (prev_val>>(5+8)) & 7];
  }
#endif

  prev_val = prev_val & ctx_CBRLE_get_color_mask (gen+1);

  uint32_t best_diff = 255*255*3;
  if (!found)
  {
    uint32_t diff;
    int best = -1;
    for (;idx < 8; idx++)
    if ((diff = color_diff (hard_pal[idx], prev_val)) < best_diff)
    {
      best_diff = diff;
      best = idx;
    }
    idx = best;
    if (best_diff < threshold)
    {
      found = 1;
    }
  }

  //if (gen > 8 || colors == color_budget)
  //    return idx;

  if (!found)
  {
    uint32_t diff;
    int best = -1;
    for (;idx < colors; idx++)
    if ((diff = color_diff (((uint32_t*)(&rgba8z[size-4-(idx)*4]))[0], prev_val)) < best_diff)
    {
      best_diff = diff;
      best = idx;
    }
    if (best !=-1) idx = best + 8;

    /* the color diff threshold is dynamic, as
     * palette space gets tighter we are less eager to add*/
    if (best_diff > threshold && colors < color_budget) // didn't find - store new color
    {
      idx = colors++;
      ((uint32_t*)(&rgba8z[size-4-idx*4]))[0] = prev_val;
      rgba8z[2] = colors;
      idx += 8;
    }
  }

  return idx;
}

static inline uint32_t
ctx_over_RGBA8 (uint32_t dst, uint32_t src, uint32_t cov);

static inline uint32_t
ctx_over_RGBA8_2 (uint32_t dst, uint32_t si_ga, uint32_t si_rb, uint32_t si_a, uint32_t cov);


static inline uint32_t
ctx_CBRLE_idx_to_color (const uint8_t *rgba8z, int size, int idx)
{
  if (idx < 8)
#if 0
    return  (idx * 17 +
             idx * 17 * 256 +
             idx * 17 * 256 * 256 +
             (unsigned)255*256*256*256);
#else
     return hard_pal[idx];
#endif
  else
     return ((uint32_t*)(&rgba8z[size-4-(idx-8)*4]))[0];
}

static inline int
ctx_CBRLE_compute_color_budget (int width, int size)
{
  int color_budget = 256;
  switch ((size*8/width)){
      case 1:
      case 2: color_budget  = 4;break;
      case 3: color_budget  = 8;break;
      case 4: color_budget  = 16;break;
      case 5: color_budget  = 32;break;
      case 6: color_budget  = 64;break;
      case 7: color_budget  = 128;break;
      case 8: color_budget  = 136;break;
      case 9: 
      case 10: color_budget = 196;break;
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
      case 16: color_budget = 256;break;
              break;
      case 17:case 18:case 19:case 20:case 21:case 22:case 23:
      case 24:
      case 32: color_budget = 256;break;
              break;
      default:
              color_budget = ctx_mini(256,size/2/4);
              break;
  }
  color_budget = ctx_maxi(color_budget,ctx_mini(256,size/5/4));
  return color_budget;
}

static inline void
ctx_CBRLE_compress (const uint8_t *rgba_in,
                     uint8_t       *rgba8z,
                     int            width,
                     int            size,
                     int            skip,
                     int            count,
                     int            mode,
                     uint8_t       *coverage,
                     int            allow_recompress)
{
  const uint8_t *rgba = rgba_in;
  int pos;

  uint32_t prev_val;
  int recompress = 0;

  int repeats;
  int colors = 0;

  uint32_t src_rgba= ((uint32_t*)rgba_in)[0];
  int color_budget = ctx_CBRLE_compute_color_budget (width, size);

  int in_gradient = 0;

  uint8_t trailer[size+2];
  int trailer_size = 0;

  uint8_t copy[(mode>=1&&mode<=2)?count*2+2:1];
  int copy_size = 0;

#if 1
  if (rgba8z[0] == 0 &&
      rgba8z[1] == 0)
  {
    pos = 4;
    for (int i =0; i < width/63;i++)
    {
      pos = encode_pix (rgba8z, pos, 0, 63, 0, 0);
    }
    ((uint16_t*)rgba8z)[0]=pos;
    rgba8z[2] = 0;
    rgba8z[3] = 16;
  }
#endif

  rgba = rgba_in;

  pos = 4;
  prev_val = 0xc0ffee;
  repeats = 0;
  
  int trailer_start = 0;

  int x = 0;

  if (skip || width != count)
  {
    int original_length = ((uint16_t*)rgba8z)[0];
    colors = rgba8z[2];
    pos = 4;

    for (x = 0; x < skip + count;)
    {
       int repeat = 0;
       int codelen = decode_pix_len (&rgba8z[pos], &repeat);
       if (x + repeat < skip + count && pos < original_length)
       {
         pos += codelen;
         x   += repeat;
       }
       else
       {
         uint8_t idx;
         int gradient;
         decode_pix (&rgba8z[pos], &idx, &repeat, &gradient);
         
         trailer_start = pos + codelen;
         if (x + repeat == skip + count)
         {
           trailer_size = original_length - trailer_start;
           if (trailer_size > 0)
           {
             memcpy (&trailer[0], &rgba8z[trailer_start], trailer_size);
           }
           else
           {
             trailer_size = 0;
           }
         }
         else
         {
           repeat -= (skip + count - x);

           if (repeat > 0)
           {
             trailer[0] = repeat + 128 + gradient * 64;
             trailer[1] = idx;
           }
           trailer_size = original_length - trailer_start;
           if (trailer_size > 0)
           {
             memcpy (&trailer[2], &rgba8z[pos + codelen], trailer_size);
             if (repeat>0)trailer_size += 2;
           }
           else
           {
             trailer_size = 0;
           }
         }
         break;
       }
    }

    pos  = 4;
    rgba = rgba_in;

    for (x = 0; x < skip && pos < original_length;)
    {
       int     repeat = 0;
       uint8_t idx;
       int     dec_repeat;
       int     gradient;
       //int len = decode_pix_len (&rgba8z[pos], &repeat);
       int len = decode_pix (&rgba8z[pos], &idx, &dec_repeat, &gradient);
       if (x + dec_repeat < skip)
       {
         pos  += len;
         x    += dec_repeat;
         rgba += 4 * dec_repeat;
       }
       else
       {
         repeat = skip - x;

         if (repeat>0)
         {
           x += repeat;

           if (mode==CBRLE_MODE_COPY ||
               mode==CBRLE_MODE_OVER)
           {
             int mpos = 0;
             if (dec_repeat != repeat)
               mpos = encode_pix (copy, 0, idx, dec_repeat-repeat, gradient, 0);
             if (original_length)
             {
               if (trailer_start == 0) trailer_start = original_length;
               copy_size = trailer_start - (pos + len);
               if (copy_size <0)
                  fprintf (stderr, "%i: uh? cs:%i %i | %i %i %i\n", __LINE__,
                                  copy_size, (int)sizeof(copy),
                                  trailer_start, pos, len);
               if (copy_size<0)copy_size=0;
               copy_size = ctx_mini(copy_size, sizeof(copy));
               if (copy_size)
               {
                 memcpy (&copy[mpos], &rgba8z[pos + len], copy_size);
               }
               copy_size += mpos;
             }
             else
             {
               copy_size = 0;
             }
           }
           else
           {
             rgba += 4 * repeat;
           }
           gradient = 0;
           pos += encode_pix (rgba8z, pos, idx, repeat, gradient, 1);
         }
         else
         {
           if (mode != CBRLE_MODE_SET)
           {
             int mpos = 0;
             copy_size = trailer_start - (pos + len);
             fprintf (stderr, "csb:%i %i\n", copy_size, (int)sizeof(copy));
             memcpy (&copy[mpos], &rgba8z[pos + len], copy_size);
           }
         }
  
         break;
       }
    }
  }
  else
  {
    colors = 0;
    rgba8z[0]=0; // length
    rgba8z[1]=0; // length
    rgba8z[2]=0; // pal size
    rgba8z[3]=0; // alpha
  }

  int prev_in_gradient = 0;

  if (mode == CBRLE_MODE_OVER)
  {
     uint32_t si_ga      = (src_rgba & 0xff00ff00) >> 8;
     uint32_t si_rb      = src_rgba & 0x00ff00ff;
     uint32_t si_a       = si_ga >> 16;

     for (int mpos = 0; mpos < copy_size && x < width && count > 0; )
     {
       uint8_t offsetA = 0;
       int repeat = 0;
       int gradient = 0;
       mpos += decode_pix (&copy[mpos], &offsetA, &repeat, &gradient);

       uint32_t color = ctx_CBRLE_idx_to_color (rgba8z, size, offsetA);

       repeat = ctx_mini (repeat, count);
       if (repeat)
       {
         uint32_t composited;
         do {
            int part = 0;
            int cov = coverage[0];
            while (coverage[0]==cov && count >0 && repeat > 0 && x < width)
            {
              x        ++;
              coverage ++;
              part     ++;
              count    --;
              repeat   --;
            }

            composited = ctx_over_RGBA8_2 (color, si_ga, si_rb, si_a, cov);
            int idx = ctx_CBRLE_get_color_idx (rgba8z, size,
                              ctx_mini(color_budget,  size-pos-colors*4 - 8), // XXX expensive?
                              composited, recompress);
            colors = rgba8z[2];
            pos += encode_pix  (rgba8z, pos, idx, part, gradient, 1);
         } while (repeat > 0);
       }
     }
  }
  else if (mode == CBRLE_MODE_COPY)
  {
     uint32_t si_ga      = (src_rgba & 0xff00ff00) >> 8;
     uint32_t si_rb      = src_rgba & 0x00ff00ff;
     for (int mpos = 0; mpos < copy_size && x < width && count > 0; )
     {
       uint8_t offsetA = 0;
       int repeat = 0;
       int gradient = 0;
       mpos += decode_pix (&copy[mpos], &offsetA, &repeat, &gradient);

       uint32_t color = ctx_CBRLE_idx_to_color (rgba8z, size, offsetA);

       repeat = ctx_mini (repeat, count);
       if (repeat)
       {
         uint32_t composited;
         do {
            int part = 0;
            int cov = coverage[0];
            while (coverage[0]==cov && count >0 && repeat > 0 && x < width)
            {
              x++;
              coverage++;
              part++;
              count --;
              repeat--;
            }

            composited = ctx_lerp_RGBA8_2 (color, si_ga, si_rb, cov);
            int idx = ctx_CBRLE_get_color_idx (rgba8z, size, ctx_mini(color_budget,  size-pos-colors*4 - 8), composited, recompress);
            colors = rgba8z[2];
            pos += encode_pix  (rgba8z, pos, idx, part, gradient, 1);
         } while (repeat > 0);
       }
     }
  }
  else
  if (mode == CBRLE_MODE_SET_COLOR)
  {
     int idx = ctx_CBRLE_get_color_idx (rgba8z, size, ctx_mini(color_budget,  size-pos-colors*4 ), src_rgba, recompress);
     while (count > 0)
     {
        int repeat = ctx_mini (count, 63);
        pos += encode_pix  (rgba8z, pos, idx, repeat, 0, 1);
        count -= repeat;
        x += repeat;
     }
  }
  else
  {

  for (; x < width && count--; x++)
    {
      uint32_t val = ((uint32_t*)rgba)[0] & 0xffffffff;
      // NOTE: we could probably drop precision to known lower precision reached here,
      //       but it requires more global state
      uint32_t next_val = 0x23ff00ff;

      if (x + 1 < width && count>1)
        next_val = ((uint32_t*)rgba)[1] & 0xffffffffu;

      if (pos > size - BORDER - trailer_size - colors * 4)
      {
        ((uint16_t*)rgba8z)[0]=pos;

        if (allow_recompress)
        do {
          pos = ctx_CBRLE_recompress (rgba8z, size, width, pos, recompress);
          colors = rgba8z[2];
          recompress ++;
        }
        while (pos > size - BORDER - trailer_size - colors * 4

                       
                        && recompress < MAX_RECOMPRESS);
        if (recompress >3) color_budget = 0;
        if (pos > size - BORDER - trailer_size - colors * 4)
           goto done;
      }

      if (val != prev_val || repeats>=63)
      {
        if (repeats)
        {

#if 1 
          if ((( repeats == 1 && in_gradient == 0) ||
                ( repeats < MAX_GRADIENT_LENGTH && in_gradient == 1)) 

              && is_b_good_middle (prev_val, val, next_val, color_budget)
          )
          {
            in_gradient = 1;
          }
          else
#endif
          if (repeats) // got incoming pixels
                            // of color to store
          {
            int idx = ctx_CBRLE_get_color_idx (rgba8z, size, ctx_mini(color_budget,  size-pos-colors*4-BORDER ), prev_val, recompress);
            colors = rgba8z[2];

            pos += encode_pix (rgba8z, pos, idx, repeats, ((prev_in_gradient==1) &&
                                                           (in_gradient == 1)), 1);

            prev_in_gradient = in_gradient;
            in_gradient = 0;
            repeats = 0;
          }
        }
      }
      repeats++;
      prev_val = val;
      if (!mode)
        rgba +=4;
    }

    if (repeats && pos  < size - colors * 4 - BORDER)
    {
          int idx = ctx_CBRLE_get_color_idx (rgba8z, size, ctx_mini(color_budget,  size-pos-colors*4-BORDER ), prev_val, recompress);
          colors = rgba8z[2];
  
          if (repeats && pos + 4 < size - colors * 4 - BORDER)
          {
            pos += encode_pix (rgba8z, pos, idx, repeats, 0, 1);
            repeats = 0;
          }
    }
    }

    if (trailer_size)
    {
       for (int i = 0; i < trailer_size;i++)
       {

      if (pos > size - trailer_size - BORDER - colors * 4)
      {
        ((uint16_t*)rgba8z)[0]=pos;
        rgba8z[2] = colors;

        if (allow_recompress)
        do {
          pos = ctx_CBRLE_recompress (rgba8z, size, width, pos, recompress);
          colors = rgba8z[2];
          recompress ++;
        }
        while (pos > size - trailer_size - BORDER - colors * 4 && recompress < MAX_RECOMPRESS);
        if (pos > size - trailer_size - BORDER - colors * 4)
           goto done;
      }

         rgba8z[pos++] = trailer[i];
       }
    }
done:
    ((uint16_t*)rgba8z)[0]=pos;
    rgba8z[2] = colors;
    //rgba8z[3] = 16;
}

static inline void
_ctx_CBRLE_decompress (const uint8_t *rgba8z, uint8_t *rgba8, int width, int size, int skip, int count)
{
  int x = 0;
  int pos = 4;
  uint32_t pixA, prev_pix;
  const uint8_t *codepix=rgba8z+4;
  pixA = prev_pix = 0;

  int length = ((uint16_t*)rgba8z)[0];
  //int colors = rgba8z[2]; 

  for (x = 0; x < skip;)
  {
     int repeat = 0;
     if (pos < length)
     {
     int len = decode_pix_len (&rgba8z[pos], &repeat);
     if (x + repeat < skip)
     {
       pos += len;
       codepix += len;
       x+=repeat;
     }
     else
       break;
     }
     else 
       x++;
  }
  //x=skip;

  for (; pos < length && x < width; )
  {
    uint8_t offsetA = 0;
    int repeat = 0;
    int gradient = 0;
    int codelen = decode_pix (codepix, &offsetA, &repeat, &gradient);
    //fprintf (stderr, "{%i r%i%s}", offsetA, repeat, gradient?"g":"");

    pixA = ctx_CBRLE_idx_to_color (rgba8z, size, offsetA);

    if (gradient)
    {
      for (int i = 0; i < repeat && x < width && count--; i++)
      {
        int has_head = 1;
        int has_tail = 0;
        float dt = (i+has_head+0.0f) / (repeat+ has_head + has_tail-1.0f);
        ((uint32_t*)(&rgba8[(x++)*4]))[0] = ctx_lerp_RGBA8 (prev_pix, pixA, (uint8_t)(dt*255.0f));
        //((uint32_t*)(&rgba8[(x++)*4]))[0] = (int)(dt * 255) | 0xff000000;
      }
    }
    else
    {
       for (int i = 0; i < repeat && x < width && count--; i++)
         ((uint32_t*)(&rgba8[(x++)*4]))[0] = pixA;
    }
    prev_pix = pixA;

    codepix += codelen;
    pos += codelen;
  }
  //fprintf (stderr, "\n");

  // replicate last value
  while (x < width && count--)
    ((uint32_t*)(&rgba8[(x++)*4]))[0] = prev_pix;
}


static inline int
ctx_CBRLE_recompress_sort_pal (uint8_t *rgba8z, int size, int width, int length, int no)
{
  uint8_t temp[width*4 + 32];
  int colors = rgba8z[2];
  //fprintf (stderr, "{%i %i %i}", size, colors, no);
  uint32_t mask = ctx_CBRLE_get_color_mask (no);
    for (int i = size - colors * 4; i < size-4; i+=4)
    {
      uint32_t *pix = (uint32_t*)(&rgba8z[i]);
      pix[i] &= mask;
    }

  _ctx_CBRLE_decompress (rgba8z, temp, width, size, 0, width);
  memset(rgba8z, 0, size);
  ctx_CBRLE_compress (temp, rgba8z, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);
  return ((uint16_t*)rgba8z)[0];
}


static inline int
ctx_CBRLE_recompress_drop_bits (uint8_t *rgba8z, int size, int width, int length, int no)
{
  uint8_t temp[width*4 + 32];
  int colors = rgba8z[2];
  //fprintf (stderr, "{%i %i %i}", size, colors, no);
  uint32_t mask = ctx_CBRLE_get_color_mask (no);
    for (int i = size - colors * 4; i < size-4; i+=4)
    {
      uint32_t *pix = (uint32_t*)(&rgba8z[i]);
      pix[i] &= mask;
    }
  _ctx_CBRLE_decompress (rgba8z, temp, width, size, 0, width);
  memset(rgba8z, 0, size);
  ctx_CBRLE_compress (temp, rgba8z, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);
  return ((uint16_t*)rgba8z)[0];
}

static inline int
ctx_CBRLE_recompress_merge_pairs (uint8_t *rgba8z, int size, int width, int length)
{
  uint8_t temp[width*4];
   // drop horizontal resolution
      _ctx_CBRLE_decompress (rgba8z, temp, width, size, 0, width);
      for (int i = 0; i < width-1; i++)
      if ((rand()%100<MERGE_CHANCE)
       &&  color_diff ( ((uint32_t*)(&temp[i*4]))[0],
                        ((uint32_t*)(&temp[(i+1)*4]))[0]) < MERGE_THRESHOLD*MERGE_THRESHOLD*3
         )
      for (int c = 0; c < 4; c++)
        temp[i*4+c]=temp[(i+1)*4+c];

      memset(rgba8z, 0, size);
      ctx_CBRLE_compress (temp, rgba8z, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);

  return ((uint16_t*)rgba8z)[0];
}

static inline int
ctx_CBRLE_recompress_smoothen (uint8_t *rgba8z, int size, int width, int length)
{
  uint8_t temp[width*4];
  _ctx_CBRLE_decompress (rgba8z, temp, width, size, 0, width);

#if 1
  for (int round = 0; round < 1; round++)
  for (int i = 1; i < width-1; i++)
  {
    for (int c = 0; c < 4; c++)
    {
      temp[i*4+c]=(uint8_t)(temp[(i+1)*4+c]*0.10f+temp[i*4+c]*0.8f+temp[(i-1)*4+c]*0.10f);
    }
  }
#endif

#if 1
  for (int round = 0; round < 1; round++)
  for (int i = 1; i < width-1; i++)
  {
    int g0 = temp[(i-1)*4+1];
    int g1 = temp[(i)*4+1];
    int g2 = temp[(i+1)*4+1];

    if (abs(g0-g1) < abs(g1-g2))
    {
      for (int c = 0; c < 4; c++)
      {
        temp[i*4+c]=(temp[(i-1)*4+c]+temp[i*4+c])/2;
      }
    }
    else if (abs(g0-g1) == abs(g1-g2))
    {
      for (int c = 0; c < 4; c++)
      {
        temp[i*4+c]=(temp[(i+1)*4+c]+temp[i*4+c]+temp[(i-1)*4+c])/3;
      }
    }
    else
    {
      for (int c = 0; c < 4; c++)
      {
        temp[i*4+c]=(temp[(i+1)*4+c]+temp[(i)*4+c])/2;
      }
    }
  }
#else
#endif

  memset(rgba8z, 0, size);
  ctx_CBRLE_compress (temp, rgba8z, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);

  return ((uint16_t*)rgba8z)[0];
}

static inline int
ctx_CBRLE_recompress (uint8_t *rgba8z, int size, int width, int length, int no)
{
  //uint8_t temp[width*4];
      length = ((uint16_t*)rgba8z)[0];

      //if (no>0)
      //fprintf (stderr, "len: %i cols:%i i:%i\n", length, rgba8z[2], no);

#if 0
//ctx_CBRLE_recompress_smoothen (rgba8z, size, width, length);
  ctx_CBRLE_recompress_smoothen (rgba8z, size, width, length);
  if (((uint16_t*)rgba8z)[0] < length - 8 || no == 0)
  {
//  fprintf (stderr, "rlen: %i cols:%i i:%i\n", ((uint16_t*)rgba8z)[0],
//                      rgba8z[2], no);
    return ((uint16_t*)rgba8z)[0];
  }
#endif
  if (no == 0)
  {
    ctx_CBRLE_recompress_smoothen (rgba8z, size, width, length);
    if (((uint16_t*)rgba8z)[0] < length -8)
      return ((uint16_t*)rgba8z)[0];
  }

  ctx_CBRLE_recompress_drop_bits (rgba8z, size, width, length, no);
  if (((uint16_t*)rgba8z)[0] < length -8)
    return ((uint16_t*)rgba8z)[0];


#if 0
  ctx_CBRLE_recompress_merge_pairs (rgba8z, size, width, length);
  if (((uint16_t*)rgba8z)[0] < length - 8 || no < 2)
  {
    return ((uint16_t*)rgba8z)[0];
  }
#endif

  return ((uint16_t*)rgba8z)[0];
}


#endif
#endif
