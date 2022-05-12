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


#define CBRLE_GRADIENT  1
#define ALLOW_MERGE     1

#define PAL_GRAY_PREDEF 0

#define GRADIENT_THRESHOLD     1
#define COLOR_THRESHOLD_START  6
#define COLOR_THRESHOLD_END    (color_budget<=128?32:21)

#define MAX_GRADIENT_LENGTH    (color_budget<=128?63:63)

// with high numbers, repeated recompressions steal a *lot* of time
// when data is overflowing
//
#define MAX_RECOMPRESS         2

// increase to have larger safety margin between pixeldata
// and palette
#define BORDER                 1


//  -- currently unused
#define MERGE_THRESHOLD        64   //(no<2?50:98)
#define MERGE_CHANCE           42   //(no<2?25:80)

#define PAL_GRAY_OFFSET      (PAL_GRAY_PREDEF * 8)

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
static inline int encode_pix (uint8_t *cbrle, int pos, 
                              uint8_t idx, int range, int gradient,
                              int allow_merge)
{
#if ALLOW_MERGE
   if (allow_merge && pos > 3)
   {
      if (cbrle[pos-1]        < 128 &&  //
          cbrle[pos-2]        < 128 &&  // previous must be single
          (cbrle[pos-1] >> 4) == 0 && idx <4 && range == 1 &&
          (cbrle[pos-1]&15) <4)
      {
         int previdx = cbrle[pos-1]&15;
         cbrle[pos-1] = (8-1) * 16 + (idx << 2) + previdx;
         return 0;
      }
   }
#endif

   if (range > 64)
     fprintf (stderr, "ERROR trying to encode too big range %i!\n", range);
   if (idx <= 15 && range < 8 && !gradient)
   {
     cbrle[pos] = (range-1) * 16 + idx;
     return 1;
   }
   else
   {
     cbrle[pos] = (range) + 128 + gradient * 64;
     cbrle[pos+1] = idx;
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
                              uint8_t       *ret_col1,
                              uint8_t       *ret_col2,
                              int           *length_px,
                              int           *gradient)
{
  int encoded_length = 1;
  int len = 1;
  int col = 0;
  int col2 = 0;
  if (gradient)
    *gradient = 0;
  if ((*pix & 0x80) == 0)
  {
    len = pix[0] >> 4;
    len++;
    if (len == 8)
    {
      len = 2;
      col = pix[0] & 15;
      col2 = col >> 2;
      col = col & 3;
      if (ret_col2)
        *ret_col2 = col2;
    }
    else
    {
      col = ( pix[0] & 15);
      if (ret_col2)
        *ret_col2 = col;
    }
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
    if (ret_col2)
      *ret_col2 = col;
  }
  if (length_px)
    *length_px = len;
  if (ret_col1)
    *ret_col1 = col;
  return encoded_length;
}


static inline int decode_pix_len (const uint8_t *pix, int *length_px)
{
  return decode_pix (pix, NULL, NULL, length_px, NULL);
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
ctx_CBRLE_recompress (uint8_t *cbrle, int size, int width, int pos, int level);

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
ctx_CBRLE_get_color_idx (uint8_t *cbrle, int size, int color_budget, uint32_t color, int gen)
{
  int found = 0;
  int colors = cbrle[2];
  int idx = 0;
    uint32_t threshold =
       (uint32_t)
       ctx_lerpf (COLOR_THRESHOLD_START*COLOR_THRESHOLD_START*3,
                  COLOR_THRESHOLD_END*COLOR_THRESHOLD_END*3, 
                  //((colors  / ( color_budget-1.0f))) );
                  ctx_sqrtf((colors  / ( color_budget-1.0f))) );

#if 0 // reduce color-depth of grays
  if (//gen > 0 &&
      (color&0xf0) == ((color >> 8) & 0xf0) &&
      (color&0xf0) == ((color >> 16) & 0xf0))
  {
    color  &= 0xfff0f0f0u;
  }
#endif

  color = color & ctx_CBRLE_get_color_mask (gen+2);

  uint32_t best_diff = 255*255*3;


#if PAL_GRAY_PREDEF // predefined 8 grays
  if (!found)
  {
    uint32_t diff;
    int best = -1;
    for (;idx < 8; idx++)
    if ((diff = color_diff (hard_pal[idx], color)) < best_diff)
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
#endif

  if (!found)
  {
    uint32_t diff;
    int best = -1;
    for (;idx < colors; idx++)
    if ((diff = color_diff (((uint32_t*)(&cbrle[size-4-(idx)*4]))[0], color)) < best_diff)
    {
      best_diff = diff;
      best = idx;
    }
    if (best !=-1) idx = best + PAL_GRAY_OFFSET;

    /* the color diff threshold is dynamic, as
     * palette space gets tighter we are less eager to add*/
    if (best_diff > threshold && colors < color_budget-1) // didn't find - store new color
    {
      idx = colors++;
      ((uint32_t*)(&cbrle[size-4-idx*4]))[0] = color;
      cbrle[2] = colors;
      idx += PAL_GRAY_OFFSET;
    }
  }

  return idx;
}

static inline uint32_t
ctx_over_RGBA8 (uint32_t dst, uint32_t src, uint32_t cov);

static inline uint32_t
ctx_over_RGBA8_2 (uint32_t dst, uint32_t si_ga, uint32_t si_rb, uint32_t si_a, uint32_t cov);


static inline uint32_t
ctx_CBRLE_idx_to_color (const uint8_t *cbrle, int size, int idx)
{
#if PAL_GRAY_PREDEF
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
#endif
     return ((uint32_t*)(&cbrle[size-4-(idx-PAL_GRAY_OFFSET)*4]))[0];
}

static inline int
ctx_CBRLE_compute_color_budget (int width, int size)
{
  int color_budget = 256;
  //return color_budget;
  switch ((size*8/width)){
      case 1:
      case 2: color_budget  = 8;break;
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
                     uint8_t       *cbrle,
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

#if CBRLE_GRADIENT
  int in_gradient = 0;
#endif

  if (mode == 0)
  {
#if 0
  for (int round = 0; round < 1; round++)
  for (int i = 1; i < width-1; i++)
  {
    int g0 = rgba[(i-1)*4+1];
    int g1 = rgba[(i)*4+1];
    int g2 = rgba[(i+1)*4+1];

    if (abs(g0-g1) < abs(g1-g2))
    {
      for (int c = 0; c < 4; c++)
      {
        rgba[i*4+c]=(rgba[(i-1)*4+c]+rgba[i*4+c])/2;
      }
    }
    else if (abs(g0-g1) == abs(g1-g2))
    {
    }
    else
    {
      for (int c = 0; c < 4; c++)
      {
        rgba[i*4+c]=(rgba[(i+1)*4+c]+rgba[(i)*4+c])/2;
      }
    }
  }
#endif
  }

  uint8_t trailer[size+2];
  int trailer_size = 0;

  uint8_t copy[(mode>=1&&mode<=2)?count*2+2:1];
  int copy_size = 0;

#if 1
  if (cbrle[0] == 0 &&
      cbrle[1] == 0)
  {
    pos = 4;
    for (int i =0; i < width/63;i++)
    {
      pos = encode_pix (cbrle, pos, 0, 63, 0, 0);
    }
    ((uint16_t*)cbrle)[0]=pos;
    cbrle[2] = 0;
    cbrle[3] = 16;
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
    int original_length = ((uint16_t*)cbrle)[0];
    colors = cbrle[2];
    pos = 4;

    for (x = 0; x < skip + count;)
    {
       int repeat = 0;
       int codelen = decode_pix_len (&cbrle[pos], &repeat);
       if (x + repeat < skip + count && pos < original_length)
       {
         pos += codelen;
         x   += repeat;
       }
       else
       {
         uint8_t idx;
         uint8_t idx2;
         int gradient;
         decode_pix (&cbrle[pos], &idx, &idx2, &repeat, &gradient);
         
         trailer_start = pos + codelen;
         if (x + repeat == skip + count)
         {
           trailer_size = original_length - trailer_start;
           if (trailer_size > 0)
           {
             memcpy (&trailer[0], &cbrle[trailer_start], trailer_size);
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
             memcpy (&trailer[2], &cbrle[pos + codelen], trailer_size);
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
       uint8_t idx2;
       int     dec_repeat;
       int     gradient;
       //int len = decode_pix_len (&cbrle[pos], &repeat);
       int len = decode_pix (&cbrle[pos], &idx, &idx2, &dec_repeat, &gradient);
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
                 memcpy (&copy[mpos], &cbrle[pos + len], copy_size);
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
           pos += encode_pix (cbrle, pos, idx, repeat, gradient, 0);
         }
         else
         {
           if (mode != CBRLE_MODE_SET)
           {
             int mpos = 0;
             copy_size = trailer_start - (pos + len);
             fprintf (stderr, "csb:%i %i\n", copy_size, (int)sizeof(copy));
             memcpy (&copy[mpos], &cbrle[pos + len], copy_size);
           }
         }
  
         break;
       }
    }
  }
  else
  {
    colors = 0;
    cbrle[0]=0; // length
    cbrle[1]=0; // length
    cbrle[2]=0; // pal size
    cbrle[3]=0; // alpha
  }

#if CBRLE_GRADIENT
  int prev_in_gradient = 0;
#endif

  if (mode == CBRLE_MODE_OVER)
  {
     uint32_t si_ga      = (src_rgba & 0xff00ff00) >> 8;
     uint32_t si_rb      = src_rgba & 0x00ff00ff;
     uint32_t si_a       = si_ga >> 16;

     for (int mpos = 0; mpos < copy_size && x < width && count > 0; )
     {
       uint8_t offsetA = 0;
       uint8_t offset2 = 0;
       int repeat = 0;
       int gradient = 0;
       mpos += decode_pix (&copy[mpos], &offsetA, &offset2, &repeat, &gradient);

       uint32_t color = ctx_CBRLE_idx_to_color (cbrle, size, offsetA);

       repeat = ctx_mini (repeat, count);
       if (repeat)
       {
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

            {
         uint32_t composited = ctx_over_RGBA8_2 (color, si_ga, si_rb, si_a, cov);
         int idx = ctx_CBRLE_get_color_idx (cbrle, size,
                              ctx_mini(color_budget,  size-pos-colors*4 - 2), // XXX expensive?
                              composited, recompress);
         colors = cbrle[2];

            pos += encode_pix  (cbrle, pos, idx, part, gradient, 1);
            }
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
       uint8_t offset2 = 0;
       int repeat = 0;
       int gradient = 0;
       mpos += decode_pix (&copy[mpos], &offsetA, &offset2, &repeat, &gradient);

       uint32_t color = ctx_CBRLE_idx_to_color (cbrle, size, offsetA);

       repeat = ctx_mini (repeat, count);
       if (repeat)
       {
         colors = cbrle[2];
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

            {
         uint32_t composited;
         composited = ctx_lerp_RGBA8_2 (color, si_ga, si_rb, cov);
         int idx = ctx_CBRLE_get_color_idx (cbrle, size, ctx_mini(color_budget,  size-pos-colors*4 - 8), composited, recompress);
            pos += encode_pix  (cbrle, pos, idx, part, gradient, 1);
            }
         } while (repeat > 0);
       }
     }
  }
  else
  if (mode == CBRLE_MODE_SET_COLOR)
  {
     int idx = ctx_CBRLE_get_color_idx (cbrle, size, ctx_mini(color_budget,  size-pos-colors*4 ), src_rgba, recompress);
     while (count > 0)
     {
        int repeat = ctx_mini (count, 63);
        pos += encode_pix  (cbrle, pos, idx, repeat, 0, 1);
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
#if CBRLE_GRADIENT
      uint32_t next_val = 0x23ff00ff;

      if (x + 1 < width && count>1)
        next_val = ((uint32_t*)rgba)[1] & 0xffffffffu;
#endif

      if (pos > size - BORDER - trailer_size - colors * 4)
      {
        ((uint16_t*)cbrle)[0]=pos;

        if (allow_recompress)
        do {
          pos = ctx_CBRLE_recompress (cbrle, size, width, pos, recompress);
          colors = cbrle[2];
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

#if CBRLE_GRADIENT
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
            int idx = ctx_CBRLE_get_color_idx (cbrle, size, ctx_mini(color_budget,  size-pos-colors*4-BORDER ), prev_val, recompress);
            colors = cbrle[2];

            pos += encode_pix (cbrle, pos, idx, repeats,
                            
#if CBRLE_GRADIENT
                      ((prev_in_gradient==1) && (in_gradient == 1))
#else
                      0
#endif
                      
                      
                      , 1);

#if CBRLE_GRADIENT
            prev_in_gradient = in_gradient;
            in_gradient = 0;
#endif
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
          int idx = ctx_CBRLE_get_color_idx (cbrle, size, ctx_mini(color_budget,  size-pos-colors*4-BORDER ), prev_val, recompress);
          colors = cbrle[2];
  
          if (repeats && pos + 4 < size - colors * 4 - BORDER)
          {
            pos += encode_pix (cbrle, pos, idx, repeats, 0, 1);
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
        ((uint16_t*)cbrle)[0]=pos;
        cbrle[2] = colors;

        if (allow_recompress)
        do {
          pos = ctx_CBRLE_recompress (cbrle, size, width, pos, recompress);
          colors = cbrle[2];
          recompress ++;
        }
        while (pos > size - trailer_size - BORDER - colors * 4 && recompress < MAX_RECOMPRESS);
        if (pos > size - trailer_size - BORDER - colors * 4)
           goto done;
      }

         cbrle[pos++] = trailer[i];
       }
    }
done:
    ((uint16_t*)cbrle)[0]=pos;
    cbrle[2] = colors;
    //cbrle[3] = 16;
}

static inline void
_ctx_CBRLE_decompress (const uint8_t *cbrle, uint8_t *rgba8, int width, int size, int skip, int count)
{
  int x = 0;
  int pos = 4;
  uint32_t pixA = 0;
#if CBRLE_GRADIENT
  uint32_t prev_pix = 0;
#endif
  const uint8_t *codepix=cbrle+4;

  int length = ((uint16_t*)cbrle)[0];
  //int colors = cbrle[2]; 

  for (x = 0; x < skip;)
  {
     int repeat = 0;
     if (pos < length)
     {
     int len = decode_pix_len (&cbrle[pos], &repeat);
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
    uint8_t offset2 = 0;
    int repeat = 0;
    int gradient = 0;
    int codelen = decode_pix (codepix, &offsetA, &offset2, &repeat, &gradient);
    //fprintf (stderr, "{%i r%i%s}", offsetA, repeat, gradient?"g":"");

    pixA = ctx_CBRLE_idx_to_color (cbrle, size, offsetA);

#if CBRLE_GRADIENT
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
#endif
    {
       if (offsetA != offset2 && repeat == 2)
       {
         uint32_t pixB = ctx_CBRLE_idx_to_color (cbrle, size, offset2);
         ((uint32_t*)(&rgba8[(x++)*4]))[0] = pixA;
         ((uint32_t*)(&rgba8[(x++)*4]))[0] = pixB;
       }
       else
       {
         for (int i = 0; i < repeat && x < width && count--; i++)
           ((uint32_t*)(&rgba8[(x++)*4]))[0] = pixA;
       }
    }
#if CBRLE_GRADIENT
    prev_pix = pixA;
#endif

    codepix += codelen;
    pos += codelen;
  }
  //fprintf (stderr, "\n");

  // replicate last value
  while (x < width && count--)
    ((uint32_t*)(&rgba8[(x++)*4]))[0] = prev_pix;
}


static inline int
ctx_CBRLE_recompress_sort_pal (uint8_t *cbrle, int size, int width, int length, int no)
{
  uint8_t temp[width*4 + 32];
  int colors = cbrle[2];
  //fprintf (stderr, "{%i %i %i}", size, colors, no);
  uint32_t mask = ctx_CBRLE_get_color_mask (no);
    for (int i = size - colors * 4; i < size-4; i+=4)
    {
      uint32_t *pix = (uint32_t*)(&cbrle[i]);
      pix[i] &= mask;
    }

  _ctx_CBRLE_decompress (cbrle, temp, width, size, 0, width);
  memset(cbrle, 0, size);
  ctx_CBRLE_compress (temp, cbrle, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);
  return ((uint16_t*)cbrle)[0];
}


static inline int
ctx_CBRLE_recompress_drop_bits (uint8_t *cbrle, int size, int width, int length, int no)
{
  uint8_t temp[width*4 + 32];
  int colors = cbrle[2];
  //fprintf (stderr, "{%i %i %i}", size, colors, no);
  uint32_t mask = ctx_CBRLE_get_color_mask (no);
    for (int i = size - colors * 4; i < size-4; i+=4)
    {
      uint32_t *pix = (uint32_t*)(&cbrle[i]);
      pix[i] &= mask;
    }
  _ctx_CBRLE_decompress (cbrle, temp, width, size, 0, width);
  memset(cbrle, 0, size);
  ctx_CBRLE_compress (temp, cbrle, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);
  return ((uint16_t*)cbrle)[0];
}

static inline int
ctx_CBRLE_recompress_merge_pairs (uint8_t *cbrle, int size, int width, int length)
{
  uint8_t temp[width*4];
   // drop horizontal resolution
      _ctx_CBRLE_decompress (cbrle, temp, width, size, 0, width);
      for (int i = 0; i < width-1; i++)
      if ((rand()%100<MERGE_CHANCE)
       &&  color_diff ( ((uint32_t*)(&temp[i*4]))[0],
                        ((uint32_t*)(&temp[(i+1)*4]))[0]) < MERGE_THRESHOLD*MERGE_THRESHOLD*3
         )
      for (int c = 0; c < 4; c++)
        temp[i*4+c]=temp[(i+1)*4+c];

      memset(cbrle, 0, size);
      ctx_CBRLE_compress (temp, cbrle, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);

  return ((uint16_t*)cbrle)[0];
}

static inline int
ctx_CBRLE_recompress_smoothen (uint8_t *cbrle, int size, int width, int length)
{
  uint8_t temp[width*4];
  _ctx_CBRLE_decompress (cbrle, temp, width, size, 0, width);

#if 0
  for (int round = 0; round < 2; round++)
  for (int i = 1; i < width-1; i++)
  {
    for (int c = 0; c < 4; c++)
    {
      temp[i*4+c]=(uint8_t)(temp[(i+1)*4+c]*0.10f+temp[i*4+c]*0.8f+temp[(i-1)*4+c]*0.10f);
    }
  }
#endif

#if 1
  for (int round = 0; round < 2; round++)
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
#if 0
      for (int c = 0; c < 4; c++)
      {
        temp[i*4+c]=(temp[(i+1)*4+c]+temp[i*4+c]+temp[(i-1)*4+c])/3;
      }
#endif
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

  memset(cbrle, 0, size);
  ctx_CBRLE_compress (temp, cbrle, width, size, 0, width, CBRLE_MODE_SET, NULL, 0);

  return ((uint16_t*)cbrle)[0];
}

static inline int
ctx_CBRLE_recompress (uint8_t *cbrle, int size, int width, int length, int no)
{
  //uint8_t temp[width*4];
      length = ((uint16_t*)cbrle)[0];

      //if (no>0)
      //fprintf (stderr, "len: %i cols:%i i:%i\n", length, cbrle[2], no);

#if 0
//ctx_CBRLE_recompress_smoothen (cbrle, size, width, length);
  ctx_CBRLE_recompress_smoothen (cbrle, size, width, length);
  if (((uint16_t*)cbrle)[0] < length - 8 || no == 0)
  {
//  fprintf (stderr, "rlen: %i cols:%i i:%i\n", ((uint16_t*)cbrle)[0],
//                      cbrle[2], no);
    return ((uint16_t*)cbrle)[0];
  }
#endif
#if 1
  if (no == 0)
  {
    ctx_CBRLE_recompress_smoothen (cbrle, size, width, length);
    if (((uint16_t*)cbrle)[0] < length -8)
      return ((uint16_t*)cbrle)[0];
  }
#endif

#if 0
  ctx_CBRLE_recompress_drop_bits (cbrle, size, width, length, no);
  if (((uint16_t*)cbrle)[0] < length -8)
    return ((uint16_t*)cbrle)[0];
#endif


#if 0
  ctx_CBRLE_recompress_merge_pairs (cbrle, size, width, length);
  if (((uint16_t*)cbrle)[0] < length - 8 || no < 2)
  {
    return ((uint16_t*)cbrle)[0];
  }
#endif

  return ((uint16_t*)cbrle)[0];
}


#endif
#endif
