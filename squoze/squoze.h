#ifndef SQUOZE_H
#define SQUOZE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

uint32_t squoze32 (const char *utf8);
uint64_t squoze52 (const char *utf8);
uint64_t squoze62 (const char *utf8);
const char *squoze32_decode (uint32_t hash);
const char *squoze52_decode (uint64_t hash);
const char *squoze62_decode (uint64_t hash);

//#define SQUOZE_NO_INTERNING  // this disables the interning - providing only a hash (and decode for non-overflowed hashes)

#define SQUOZE_ENTER_SQUEEZE    16

#define SQUOZE_SPACE            0
#define SQUOZE_DEC_OFFSET_A     27
#define SQUOZE_INC_OFFSET_A     28
#define SQUOZE_DEC_OFFSET_B     29
#define SQUOZE_INC_OFFSET_B     30
#define SQUOZE_ENTER_UTF5       31

#define SQUOZE_JUMP_STRIDE      26
#define SQUOZE_JUMP_OFFSET      19

static inline uint32_t squoze_utf8_to_unichar (const char *input);
static inline int      squoze_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
static inline int      squoze_utf8_len        (const unsigned char first_byte);


/* returns the base-offset of the segment this unichar belongs to,
 *
 * segments are 26 items long and are offset so that the 'a'-'z' is
 * one segment.
 */
static inline int squoze_new_offset (uint32_t unichar)
{
  uint32_t ret = unichar - (unichar % SQUOZE_JUMP_STRIDE) + SQUOZE_JUMP_OFFSET;
  if (ret > unichar) ret -= SQUOZE_JUMP_STRIDE;
  return ret;
}

static int squoze_needed_jump (uint32_t off, uint32_t unicha)
{
  int count = 0;
  int unichar = unicha;
  int offset = off;

  if (unichar == 32) // space is always in range
    return 0;

  /* TODO: replace this with direct computation of values instead of loops */

  while (unichar < offset)
  {
    offset -= SQUOZE_JUMP_STRIDE;
    count ++;
  }
  if (count)
  {
    return -count;
  }
  while (unichar - offset >= SQUOZE_JUMP_STRIDE)
  {
    offset += SQUOZE_JUMP_STRIDE;
    count ++;
  }
  return count;
}

static inline int
squoze_utf5_length (uint32_t unichar)
{
  int octets = 0;
  if (unichar == 0)
    return 1;
  while (unichar)
  {
    octets ++;
    unichar /= 16;
  }
  return octets;
}

typedef struct EncodeUtf5 {
  int      is_utf5;
  int      offset;
  int      length;
  void    *write_data;
  uint32_t current;
} EncodeUtf5;

static inline uint64_t
squoze_overflow_mask_for_dim (int squoze_dim)
{
  return ((uint64_t)1<<(squoze_dim * 5 + 1));
}

static int squoze_compute_cost_utf5 (int offset, int val, int next_val)
{
  int cost = 0; 
  cost += squoze_utf5_length (val);
  if (next_val)
  {
    int no_change_cost = squoze_utf5_length (next_val);
    // XXX remove this dead code
#if 0 // not hit in test-corpus, it is easier to specify and
      // port the hash consistently without it
    offset = squoze_new_offset (val);
    int change_cost = 1;
    int needed_jump = squoze_needed_jump (offset, next_val);

    if (needed_jump == 0)
    {
      change_cost += 1;
    } else if (needed_jump >= -2 && needed_jump <= 2)
    {
      change_cost += 2;
    }
    else if (needed_jump >= -10 && needed_jump <= -10)
    {
      change_cost += 3;
    }
    else
    {
      change_cost += 100;
    }

    if (change_cost < no_change_cost)
    {
      cost += change_cost;
    }
    else
#endif
    {
      cost += no_change_cost;
    }

  }



  return cost;
}

static int squoze_compute_cost_squeezed (int offset, int val, int next_val)
{
  int needed_jump = squoze_needed_jump (offset, val);
  int cost = 0;
  if (needed_jump == 0)
  {
    cost += 1;
  }
  else if (needed_jump >= -2 && needed_jump <= 2)
  {
    cost += 2;
    offset += SQUOZE_JUMP_STRIDE * needed_jump;
  }
  else if (needed_jump >= -10 && needed_jump <= 10)
  {
    cost += 3;
    offset += SQUOZE_JUMP_STRIDE * needed_jump;
  }
  else
  {
    cost += 100; // very expensive, makes the other choice win
  }

  if (next_val)
  {
    int change_cost = 1 + squoze_utf5_length (next_val);
    int no_change_cost = 0;
    needed_jump = squoze_needed_jump (offset, next_val);

    if (needed_jump == 0)
    {
      no_change_cost += 1;
    }
    else if (needed_jump >= -2 && needed_jump <= 2)
    {
      no_change_cost += 2;
    }
    else if (needed_jump >= -10 && needed_jump <= 10)
    {
      no_change_cost += 3;
      offset += SQUOZE_JUMP_STRIDE * needed_jump;
    }
    else
    {
      no_change_cost = change_cost;
    }
    if (change_cost < no_change_cost)
      cost += change_cost;
    else
      cost += no_change_cost;
  }

  return cost;
}


static void squoze5_encode (const char *input, int inlen,
                            char *output, int *r_outlen,
                            int permit_squeezed,
                            int escape_endzero)
{
  int offset  = squoze_new_offset('a');
  int is_utf5 = 1;
  int len     = 0;

  for (int i = 0; i < inlen; i+= squoze_utf8_len (input[i]))
  {
    int val = squoze_utf8_to_unichar (&input[i]);
    int next_val = 0;
    int first_len = squoze_utf8_len (input[i]);
    if (i + first_len < inlen)
      next_val = squoze_utf8_to_unichar (&input[i+first_len]);

    if (is_utf5)
    {
      int change_cost    = squoze_compute_cost_squeezed (offset, val, next_val);
      int no_change_cost = squoze_compute_cost_utf5 (offset, val, next_val);
  
      if (i != 0)          /* ignore cost of initial 'G' */
        change_cost += 1;

      if (permit_squeezed && change_cost <= no_change_cost)
      {
        output[len++] = SQUOZE_ENTER_SQUEEZE;
        is_utf5 = 0;
      }
    }
    else
    {
      int change_cost    = 1 + squoze_compute_cost_utf5 (offset, val, next_val);
      int no_change_cost = squoze_compute_cost_squeezed (offset, val, next_val);

      if (change_cost < no_change_cost)
      {
        output[len++] = SQUOZE_ENTER_UTF5;
        is_utf5 = 1;
      }
    }

    if (!is_utf5)
    {
      int needed_jump = squoze_needed_jump (offset, val);
      if (needed_jump)
      {
        if (needed_jump >= -2 && needed_jump <= 2)
        {
          switch (needed_jump)
          {
            case -1: output[len++] = SQUOZE_DEC_OFFSET_B; break;
            case  1: output[len++] = SQUOZE_INC_OFFSET_B; break;
            case -2: output[len++] = SQUOZE_DEC_OFFSET_A; break;
            case  2: output[len++] = SQUOZE_INC_OFFSET_A; break;
          }
          offset += SQUOZE_JUMP_STRIDE * needed_jump;
        }
        else if (needed_jump >= -10 && needed_jump <= 10) {
              int encoded_val;
              if (needed_jump < -2)
                encoded_val = 5 - needed_jump;
              else
                encoded_val = needed_jump - 3;

              output[len++] = (encoded_val / 4) + SQUOZE_DEC_OFFSET_A;
              output[len++] = (encoded_val % 4) + SQUOZE_DEC_OFFSET_A;

              offset += SQUOZE_JUMP_STRIDE * needed_jump;
        }
        else
        {
          assert(0); // should not be reached
          output[len++] = SQUOZE_ENTER_UTF5;
          is_utf5 = 1;
        }
      }
    }

    if (is_utf5)
    {
      int octets = 0;
      offset = squoze_new_offset (val);
      while (val)
      {
        int oval = val % 16;
        int hi = 16;
        if (val / 16) hi = 0;
        output[len+ (octets++)] = oval + hi;
        val /= 16;
      }
      for (int j = 0; j < octets/2; j++) // mirror in-place
      {                                  // TODO refactor to be single pass
        int tmp = output[len+j];
        output[len+j] = output[len+octets-1-j];
        output[len+octets-1-j] = tmp;
      }
      len += octets;
    }
    else 
    {
       if (val == ' ')
       {
         output[len++] = SQUOZE_SPACE;
       }
       else
       {
         output[len++] = val-offset+1;
       }
    }
  }

  if (escape_endzero && len && output[len-1]==0)
  {
    if (is_utf5)
      output[len++] = 16;
    else
      output[len++] = SQUOZE_ENTER_UTF5;
  }
  output[len]=0;
  if (r_outlen)
    *r_outlen = len;
}

static inline uint64_t _squoze (int squoze_dim, const char *utf8)
{
  char encoded[4096]="";
  int  encoded_len=0;
  squoze5_encode (utf8, strlen (utf8), encoded, &encoded_len, 1, 1);
  uint64_t hash = 0;
  int  utf5 = (encoded[0] != SQUOZE_ENTER_SQUEEZE);
  uint64_t multiplier = ((squoze_dim == 6) ? 0x25bd1e975
                                           : 0x98173415bd1e975);

  uint64_t overflowed_mask = squoze_overflow_mask_for_dim (squoze_dim);
  uint64_t all_bits        = overflowed_mask - 1;

  int rshift = (squoze_dim == 6) ? 8 : 16;


  if (encoded_len - (!utf5) <= squoze_dim)
  {
    for (int i = !utf5; i < encoded_len; i++)
    {
      uint64_t val = encoded[i];
      hash = hash | (val << (5*(i-(!utf5))));
    }
    hash <<= 1; // make room for the bit that encodes utf5 or squeeze
  }
  else
  {
    for (int i = 0; i < encoded_len; i++)
    {
      uint64_t val = encoded[i];
      hash = hash ^ val;
      hash = hash * multiplier;
      hash = hash & all_bits;
      hash = hash ^ ((hash >> rshift));
    }
    hash |= overflowed_mask;
  }
  return hash | utf5;
}

typedef struct _CashInterned CashInterned;

struct _CashInterned {
    uint64_t   hash;
    char      *string;
};

static CashInterned *interned = NULL;
static int n_interned = 0;
static int s_interned = 0;

static int squoze_interned_find (uint64_t hash)
{
#if 1
  int min = 0;
  int max = n_interned - 1;
  if (max <= 0)
    return 0;
  do
  {
     int pos = (min + max)/2;
     if (interned[pos].hash == hash)
       return pos;
     else if (min == max - 1)
       return max;
     else if (interned[pos].hash < hash)
       min = pos;
     else
       max = pos;
  } while (min != max);
  return max;
#else
  for (int i = 0; i < n_interned; i++)
    if (interned[i].hash > hash)
      return i;
  return 0;
#endif
}

#ifdef __CTX_H__
#define strdup ctx_strdup
#define strstr ctx_strstr
#endif

static inline uint64_t squoze (int squoze_dim, const char *utf8)
{
  uint64_t hash = _squoze (squoze_dim, utf8);
#ifdef SQUOZE_NO_INTERNING
  return hash;
#endif
  uint64_t overflowed_mask = squoze_overflow_mask_for_dim (squoze_dim);
  if (hash & overflowed_mask)
  {
    int pos = squoze_interned_find (hash);
    if (interned && interned[pos].hash == hash)
      return hash;

    if (n_interned + 1 >= s_interned)
    {
       s_interned = (s_interned + 128)*2;
       interned = (CashInterned*)realloc (interned, s_interned * sizeof (CashInterned));
    }

    n_interned++;
#if 1
    if (n_interned-pos)
      memmove (&interned[pos+1], &interned[pos], (n_interned-pos) * sizeof (CashInterned));
    // the memmove is the expensive part of testing for collisions
    // insertions should be cheaper! at least looking up strings
    // is cheap
#else
    pos = n_interned-1;
#endif
    {
      CashInterned *entry = &interned[pos];
      entry->hash = hash;
      entry->string = strdup (utf8);
    }

  }
  return hash;
}

uint32_t squoze32 (const char *utf8)
{
  return squoze (6, utf8);
}

uint64_t squoze52 (const char *utf8)
{
  return squoze (10, utf8);
}

uint64_t squoze62 (const char *utf8)
{
  return squoze (12, utf8);
}

uint32_t ctx_strhash(const char *str) {
  return squoze (6, str);
}

typedef struct CashUtf5Dec {
  int       is_utf5;
  int       offset;
  void     *write_data;
  uint32_t  current;
  void    (*append_unichar) (uint32_t unichar, void *write_data);
  int       jumped_amount;
  int       jump_mode;
} CashUtf5Dec;

typedef struct CashUtf5DecDefaultData {
  uint8_t *buf;
  int      length;
} CashUtf5DecDefaultData;

static void squoze_decode_utf5_append_unichar_as_utf8 (uint32_t unichar, void *write_data)
{
  CashUtf5DecDefaultData *data = (CashUtf5DecDefaultData*)write_data;
  int length = squoze_unichar_to_utf8 (unichar, &data->buf[data->length]);
  data->buf[data->length += length] = 0;
}

static void squoze_decode_jump (CashUtf5Dec *dec, uint8_t in)
{
  dec->offset -= SQUOZE_JUMP_STRIDE * dec->jumped_amount;
  int jump_len = (dec->jump_mode - SQUOZE_DEC_OFFSET_A) * 4 +
                 (in - SQUOZE_DEC_OFFSET_A);
  if (jump_len > 7)
    jump_len = 5 - jump_len;
  else
    jump_len += 3;
  dec->offset += jump_len * SQUOZE_JUMP_STRIDE;
  dec->jumped_amount = 0;
}

static void squoze_decode_utf5 (CashUtf5Dec *dec, uint8_t in)
{
  if (dec->is_utf5)
  {
    if (in >= 16)
    {
      if (dec->current)
      {
        dec->offset = squoze_new_offset (dec->current);
        dec->append_unichar (dec->current, dec->write_data);
        dec->current = 0;
      }
    }
    if (in == SQUOZE_ENTER_SQUEEZE)
    {
      if (dec->current)
      {
        dec->offset = squoze_new_offset (dec->current);
        dec->append_unichar (dec->current, dec->write_data);
        dec->current = 0;
      }
      dec->is_utf5 = 0;
    }
    else
    {
      dec->current = dec->current * 16 + (in % 16);
    }
  }
  else
  {
    if (dec->jumped_amount)
    {
      switch (in)
      {
        case SQUOZE_DEC_OFFSET_A:
        case SQUOZE_DEC_OFFSET_B:
        case SQUOZE_INC_OFFSET_A:
        case SQUOZE_INC_OFFSET_B:
          squoze_decode_jump (dec, in);
          break;
        default:
          dec->append_unichar (dec->offset + (in - 1), dec->write_data);
          dec->jumped_amount = 0;
          dec->jump_mode = 0;
          break;
      }
    }
    else
    {
      switch (in)
      {
        case SQUOZE_ENTER_UTF5:
          dec->is_utf5 = 1;
          dec->jumped_amount = 0;
          dec->jump_mode = 0;
          break;
        case SQUOZE_SPACE: 
          dec->append_unichar (' ', dec->write_data);
          dec->jumped_amount = 0;
          dec->jump_mode = 0;
          break;
        case SQUOZE_DEC_OFFSET_A:
          dec->jumped_amount = -2;
          dec->jump_mode = in;
          dec->offset += dec->jumped_amount * SQUOZE_JUMP_STRIDE;
          break;
        case SQUOZE_INC_OFFSET_A:
          dec->jumped_amount = 2;
          dec->jump_mode = in;
          dec->offset += dec->jumped_amount * SQUOZE_JUMP_STRIDE;
          break;
        case SQUOZE_DEC_OFFSET_B:
          dec->jumped_amount = -1;
          dec->jump_mode = in;
          dec->offset += dec->jumped_amount * SQUOZE_JUMP_STRIDE;
          break;
        case SQUOZE_INC_OFFSET_B:
          dec->jumped_amount = 1;
          dec->jump_mode = in;
          dec->offset += dec->jumped_amount * SQUOZE_JUMP_STRIDE;
          break;
        default:
          dec->append_unichar (dec->offset + (in - 1), dec->write_data);
          dec->jumped_amount = 0;
          dec->jump_mode = 0;
      }
    }
  }
}

static void squoze_decode_utf5_bytes (int is_utf5, 
                        const unsigned char *input, int inlen,
                        char *output, int *r_outlen)
{
  CashUtf5DecDefaultData append_data = {(unsigned char*)output, 0};
  CashUtf5Dec dec = {is_utf5,
                     squoze_new_offset('a'),
                     &append_data,
                     0,
                     squoze_decode_utf5_append_unichar_as_utf8,
                     0, 0
                    };
  for (int i = 0; i < inlen; i++)
    squoze_decode_utf5 (&dec, input[i]);
  if (dec.current)
    dec.append_unichar (dec.current, dec.write_data);
  if (r_outlen)
    *r_outlen = append_data.length;
}

static const char *squoze_decode_r (int squoze_dim, uint64_t hash, char *ret, int retlen)
{
  uint64_t overflowed_mask = ((uint64_t)1<<(squoze_dim * 5 + 1));

  if (hash & overflowed_mask)
  {
#if 0
    for (int i = 0; i < n_interned; i++)
    {
      CashInterned *entry = &interned[i];
      if (entry->hash == hash)
        return entry->string;
    }
#else
    int pos = squoze_interned_find (hash);
    if (!interned || (interned[pos].hash!=hash))
      return NULL;
    return interned[pos].string;
#endif
    return NULL;
  }

  uint8_t utf5[140]=""; // we newer go really high since there isnt room
                        // in the integers
  uint64_t tmp = hash & (overflowed_mask-1);
  int len = 0;
  int is_utf5 = tmp & 1;
  tmp /= 2;
  int in_utf5 = is_utf5;
  while (tmp > 0)
  {
    uint64_t remnant = tmp % 32;
    uint64_t val = remnant;

    if      ( in_utf5 && val == SQUOZE_ENTER_SQUEEZE) in_utf5 = 0;
    else if (!in_utf5 && val == SQUOZE_ENTER_UTF5) in_utf5 = 1;

    utf5[len++] = val;
    tmp -= remnant;
    tmp /= 32;
  }
  utf5[len]=0;
  squoze_decode_utf5_bytes (is_utf5, utf5, len, ret, &retlen);
  //ret[retlen]=0;
  return ret;
}

/* copy the value as soon as possible, some mitigation is in place
 * for more than one value in use and cross-thread interactions.
 */
static const char *squoze_decode (int squoze_dim, uint64_t hash)
{
#if CTX_THREADS
#define THREAD __thread  // use thread local storage
  static THREAD int no = 0;
  static THREAD char ret[3][256];
  no ++;
  if (no > 7) no = 0;
  return squoze_decode_r (squoze_dim, hash, ret[no], 256);
#undef THREAD
#else
  static char ret[64];
  return squoze_decode_r (squoze_dim, hash, ret, 256);
#endif
}

const char *squoze32_decode (uint32_t hash)
{
  return squoze_decode (6, hash);
}

const char *squoze52_decode (uint64_t hash)
{
  return squoze_decode (10, hash);
}

const char *squoze62_decode (uint64_t hash)
{
  return squoze_decode (12, hash);
}

static inline uint32_t
squoze_utf8_to_unichar (const char *input)
{
  const uint8_t *utf8 = (const uint8_t *) input;
  uint8_t c = utf8[0];
  if ( (c & 0x80) == 0)
    { return c; }
  else if ( (c & 0xE0) == 0xC0)
    return ( (utf8[0] & 0x1F) << 6) |
           (utf8[1] & 0x3F);
  else if ( (c & 0xF0) == 0xE0)
    return ( (utf8[0] & 0xF)  << 12) |
           ( (utf8[1] & 0x3F) << 6) |
           (utf8[2] & 0x3F);
  else if ( (c & 0xF8) == 0xF0)
    return ( (utf8[0] & 0x7)  << 18) |
           ( (utf8[1] & 0x3F) << 12) |
           ( (utf8[2] & 0x3F) << 6) |
           (utf8[3] & 0x3F);
  else if ( (c & 0xFC) == 0xF8)
    return ( (utf8[0] & 0x3)  << 24) |
           ( (utf8[1] & 0x3F) << 18) |
           ( (utf8[2] & 0x3F) << 12) |
           ( (utf8[3] & 0x3F) << 6) |
           (utf8[4] & 0x3F);
  else if ( (c & 0xFE) == 0xFC)
    return ( (utf8[0] & 0x1)  << 30) |
           ( (utf8[1] & 0x3F) << 24) |
           ( (utf8[2] & 0x3F) << 18) |
           ( (utf8[3] & 0x3F) << 12) |
           ( (utf8[4] & 0x3F) << 6) |
           (utf8[5] & 0x3F);
  return 0;
}
static inline int
squoze_unichar_to_utf8 (uint32_t  ch,
                      uint8_t  *dest)
{
  /* http://www.cprogramming.com/tutorial/utf8.c  */
  /*  Basic UTF-8 manipulation routines
    by Jeff Bezanson
    placed in the public domain Fall 2005 ... */
  if (ch < 0x80)
    {
      dest[0] = (char) ch;
      return 1;
    }
  if (ch < 0x800)
    {
      dest[0] = (ch>>6) | 0xC0;
      dest[1] = (ch & 0x3F) | 0x80;
      return 2;
    }
  if (ch < 0x10000)
    {
      dest[0] = (ch>>12) | 0xE0;
      dest[1] = ( (ch>>6) & 0x3F) | 0x80;
      dest[2] = (ch & 0x3F) | 0x80;
      return 3;
    }
  if (ch < 0x110000)
    {
      dest[0] = (ch>>18) | 0xF0;
      dest[1] = ( (ch>>12) & 0x3F) | 0x80;
      dest[2] = ( (ch>>6) & 0x3F) | 0x80;
      dest[3] = (ch & 0x3F) | 0x80;
      return 4;
    }
  return 0;
}

static inline int
squoze_utf8_len (const unsigned char first_byte)
{
  if      ( (first_byte & 0x80) == 0)
    { return 1; } /* ASCII */
  else if ( (first_byte & 0xE0) == 0xC0)
    { return 2; }
  else if ( (first_byte & 0xF0) == 0xE0)
    { return 3; }
  else if ( (first_byte & 0xF8) == 0xF0)
    { return 4; }
  return 1;
}

#endif
