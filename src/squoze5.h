#ifndef CASH_H
#define CASH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define CASH_NO_INTERNING  // this disables the interning - providing only a hash (and decode for non-overflowed hashes)

#define CASH_ENTER_DIRECT     16
#define CASH_SPACE            0
#define CASH_DEC_OFFSET_A     27
#define CASH_INC_OFFSET_A     28
#define CASH_DEC_OFFSET_B     29
#define CASH_INC_OFFSET_B     30
#define CASH_ENTER_UTF5       31
#define CASH_JUMP_OFFSET      26
#define CASH_RANGE            26
#define CASH_JUMP_MOD         26

static inline uint32_t cash_utf8_to_unichar (const char *input);
static inline int      cash_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
static inline int      cash_utf8_len        (const unsigned char first_byte);

static inline int cash_new_offset (uint32_t unichar)
{
  uint32_t ret = unichar - (unichar % CASH_JUMP_MOD) + 19;
  if (ret > unichar) ret -= CASH_JUMP_MOD;
  return ret;
}

static int cash_is_in_range (uint32_t offset, uint32_t unichar)
{
  if (unichar == 32) // space is always in range
    return 1;
  if (unichar - offset < CASH_RANGE)
    return 1;
  return 0;
}

static int cash_range_check (uint32_t offset, uint32_t unichar, int step)
{
  return cash_is_in_range (offset + (CASH_JUMP_OFFSET * step), unichar);
}


static int cash_needed_jump (uint32_t offset, uint32_t unichar)
{ // XXX not yet properly nested
  int count = 0;
  while (unichar < offset)
  {
    offset -= CASH_JUMP_OFFSET;
    count ++;
  }
  while (unichar - offset >= CASH_RANGE)
  {
    offset += CASH_JUMP_OFFSET;
    count ++;
  }
  return count;
}

static inline int
cash_utf5_length (uint32_t unichar)
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
cash_overflow_mask_for_dim (int cash_dim)
{
  return ((uint64_t)1<<(cash_dim * 5 + 1));
}

/* could be named encode_5bit instead since it both encodes
 * utf5 and direct encoded
 */
static void cash_encode_utf5 (const char *input, int inlen,
                               char *output, int *r_outlen,
                               int permit_direct)
{
  uint32_t offset = cash_new_offset('a');

  int is_utf5 = 1;
  int len     = 0;

  for (int i = 0; i < inlen; i+= cash_utf8_len (input[i]))
  {
    int val = cash_utf8_to_unichar (&input[i]);

    if (is_utf5)
    {
      int first_len = cash_utf8_len (input[i]);
      int change_cost    = 1;
      int no_change_cost = 0;

      int next_offset = offset;
      int next_val    = ' ';

      no_change_cost += cash_utf5_length (val);
      /* XXX: replace this if/else chain with correct code,
       *      verified against this manual chain first
       */
      if (cash_range_check (offset, val, 0))
      {
        change_cost += 1;
        next_offset = offset;
      }
      else if (cash_range_check (offset, val, -1))
      {
        change_cost += 2;
        next_offset = offset - CASH_JUMP_OFFSET * 1;
      }
      else if (cash_range_check (offset, val, 1))
      {
        change_cost += 2;
        next_offset = offset + CASH_JUMP_OFFSET * 1;
      }
      else if (cash_range_check (offset, val, -2))
      {
        change_cost += 2;
        next_offset = offset - CASH_JUMP_OFFSET * 2;
      }
      else if (cash_range_check (offset, val, 2))
      {
        change_cost += 2;
        next_offset = offset + CASH_JUMP_OFFSET * 2;
      }
      else if (cash_range_check (offset, val, -3))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 3;
      }
      else if (cash_range_check (offset, val, 3))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 3;
      }
      else if (cash_range_check (offset, val, -4))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 4;
      }
      else if (cash_range_check (offset, val, 4))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 4;
      }
      else if (cash_range_check (offset, val, -5))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 5;
      }
      else if (cash_range_check (offset, val, 5))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 5;
      }
      else if (cash_range_check (offset, val, -6))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 6;
      }
      else if (cash_range_check (offset, val, 6))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 6;
      }
      else if (cash_range_check (offset, val, -7))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 7;
      }
      else if (cash_range_check (offset, val, 7))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 7;
      }
      else if (cash_range_check (offset, val, -8))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 8;
      }
      else if (cash_range_check (offset, val, 8))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 8;
      }
      else if (cash_range_check (offset, val, -9))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 9;
      }
      else if (cash_range_check (offset, val, 9))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 9;
      }
      else if (cash_range_check (offset, val, -10))
      {
        change_cost += 3;
        next_offset = offset - CASH_JUMP_OFFSET * 10;
      }
      else if (cash_range_check (offset, val, 10))
      {
        change_cost += 3;
        next_offset = offset + CASH_JUMP_OFFSET * 10;
      }
      else
      {
        change_cost += 100;
      }

      /* XXX: replace this if/else chain with correct code,
       *      verified against this manual chain first
       */
      if (i + first_len < inlen)
      {
        next_val        = cash_utf8_to_unichar (&input[i+first_len]);
        no_change_cost += cash_utf5_length (next_val);

        if      (cash_range_check (next_offset, next_val,  0))
        {
          change_cost += 1;
        }
        else if (cash_range_check (next_offset, next_val, -1))
        {
          change_cost += 2;
        }
        else if (cash_range_check (next_offset, next_val,  1))
        {
          change_cost += 2;
        }
        else if (cash_range_check (next_offset, next_val, -2))
        {
          change_cost += 2;
        }
        else if (cash_range_check (next_offset, next_val,  2))
        {
          change_cost += 2;
        }
        else if (cash_range_check (next_offset, next_val, -3))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  3))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val, -4))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  4))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val, -5))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  5))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val, -6))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  6))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val, -7))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  7))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val, -8))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  8))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val, -9))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  9))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val, -10))
        {
          change_cost += 3;
        }
        else if (cash_range_check (next_offset, next_val,  10))
        {
          change_cost += 3;
        }
        else
        {
          change_cost += 1; // change to UTF-5 mode
          change_cost += cash_utf5_length (next_val); // encoding of char
        }
      }

#if 0
      fprintf (stderr, "{(%c) %c %c change:%i no_change:%i}", offset,
                      val, next_val, change_cost, no_change_cost);
#endif
      if (permit_direct && change_cost - ((i == 0)*1) <= no_change_cost)
      {
        output[len++] = CASH_ENTER_DIRECT;
        is_utf5 = 0;
      }
    }
    else
    {
      // we're already in direct mode, check if it was better if we were not
      int first_len = cash_utf8_len (input[i]);
      int change_cost    = 1;
      int no_change_cost = 0;

      int next_offset = offset;
      int next_val    = ' ';

      change_cost += cash_utf5_length (val);


      /* XXX: replace this if/else chain with correct code,
       *      verified against this manual chain first
       */
      if (cash_range_check (offset, val, 0))
      {
        no_change_cost += 1;
      }
      else if (cash_range_check (offset, val, 1))
      {
        no_change_cost += 2;
        next_offset    += CASH_JUMP_OFFSET * 1;
      }
      else if (cash_range_check (offset, val, -1))
      {
        no_change_cost += 2;
        next_offset    += CASH_JUMP_OFFSET * -1;
      }
      else if (cash_range_check (offset, val, -2))
      {
        no_change_cost += 2;
        next_offset    += CASH_JUMP_OFFSET * -2;
      }
      else if (cash_range_check (offset, val, 2))
      {
        no_change_cost += 2;
        next_offset    += CASH_JUMP_OFFSET * 2;
      }
      else if (cash_range_check (offset, val, -3))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -3;
      }
      else if (cash_range_check (offset, val, 3))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 3;
      }
      else if (cash_range_check (offset, val, -4))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -4;
      }
      else if (cash_range_check (offset, val, 4))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 4;
      }
      else if (cash_range_check (offset, val, -5))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -5;
      }
      else if (cash_range_check (offset, val, 5))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 5;
      }
      else if (cash_range_check (offset, val, -6))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -6;
      }
      else if (cash_range_check (offset, val, 6))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 6;
      }
      else if (cash_range_check (offset, val, -7))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -7;
      }
      else if (cash_range_check (offset, val, 7))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 7;
      }
      else if (cash_range_check (offset, val, -8))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -8;
      }
      else if (cash_range_check (offset, val, 8))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 8;
      }
      else if (cash_range_check (offset, val, -9))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -9;
      }
      else if (cash_range_check (offset, val, 9))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 9;
      }
      else if (cash_range_check (offset, val, -10))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * -10;
      }
      else if (cash_range_check (offset, val, 10))
      {
        no_change_cost += 3;
        next_offset    += CASH_JUMP_OFFSET * 10;
      }
      else
      {
        no_change_cost += 100;
      }
      
      int next_change_cost = 0;
      int next_no_change_cost = 0;

      if (i + first_len < inlen)
      {
        next_val          = cash_utf8_to_unichar (&input[i+first_len]);
        next_change_cost += cash_utf5_length (next_val);

        if (cash_range_check (next_offset, next_val, 0))
        {
          next_no_change_cost += 1;
        }
        else if (cash_range_check (next_offset, next_val, -1) ||
                 cash_range_check (next_offset, next_val,  1) ||
                 cash_range_check (next_offset, next_val, -2) ||
                 cash_range_check (next_offset, next_val,  2))
        {
          next_no_change_cost += 2;
        }
        else if (cash_range_check (next_offset, next_val, -3) ||
                 cash_range_check (next_offset, next_val,  3) ||
                 cash_range_check (next_offset, next_val, -4) ||
                 cash_range_check (next_offset, next_val,  4) ||
                 cash_range_check (next_offset, next_val, -5) ||
                 cash_range_check (next_offset, next_val,  5) ||
                 cash_range_check (next_offset, next_val, -6) ||
                 cash_range_check (next_offset, next_val,  6) ||
                 cash_range_check (next_offset, next_val, -7) ||
                 cash_range_check (next_offset, next_val,  7) ||
                 cash_range_check (next_offset, next_val, -8) ||
                 cash_range_check (next_offset, next_val,  8) ||
                 cash_range_check (next_offset, next_val, -9) ||
                 cash_range_check (next_offset, next_val,  9) ||
                 cash_range_check (next_offset, next_val, -10) ||
                 cash_range_check (next_offset, next_val,  10))
        {
          next_no_change_cost += 3;
        }

      }

      if (change_cost + next_change_cost + 1 < no_change_cost + next_no_change_cost
         )//|| change_cost < no_change_cost)
      {
        output[len++] = CASH_ENTER_UTF5;
        is_utf5 = 1;
      }

    }

    if (!is_utf5)
    {
      if (!cash_range_check(offset, val, 0))
      {
        int test_delta = -10;
        int found = -100;
        while (test_delta <= 10)
        {
          if (cash_range_check (offset, val, test_delta))
          {
            found = test_delta;
          }
          test_delta ++;
        }
        if (found == -100)
        {
          output[len++] = CASH_ENTER_UTF5;
          is_utf5 = 1;
        }
        else //  |1|2| (8)  10| (32)  42 |  (127)  169   ..  169 * 26 = 4394
        {
          offset += CASH_JUMP_OFFSET * found;
          switch (found)
          {
            case -1: output[len++] = CASH_DEC_OFFSET_B; break;
            case  1: output[len++] = CASH_INC_OFFSET_B; break;
            case -2: output[len++] = CASH_DEC_OFFSET_A; break;
            case  2: output[len++] = CASH_INC_OFFSET_A; break;
            default:
            {
              int encoded_val;
              if (found < -2)
                encoded_val = 5 - found;
              else
                encoded_val = found - 3;

              output[len++] = (encoded_val / 4) + CASH_DEC_OFFSET_A;
              output[len++] = (encoded_val % 4) + CASH_DEC_OFFSET_A;
            }
          }
        }
      }
    }

    if (is_utf5)
    {
      int octets = 0;
      offset = cash_new_offset (val);
      while (val)
      {
        int oval = val % 16;
        int hi = 16;
        if (val / 16) hi = 0;
        output[len+ (octets++)] = oval + hi;
        val /= 16;
      }
      for (int j = 0; j < octets/2; j++) // mirror in-place
      {
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
         output[len++] = CASH_SPACE;
       }
       else
       {
         output[len++] = val-offset+1;
       }
    }
  }


  /* post-process padding, we do not want to end in 0 as that
   * is ambigious for integer encoding */
  if (len && output[len-1]==0)
  {
    if (is_utf5)
      output[len++] = 16;
    else
      output[len++] = CASH_ENTER_UTF5;

  }
  output[len]=0;
  if (r_outlen)
    *r_outlen = len;
}

static inline uint64_t _cash (int cash_dim, const char *utf8)
{
  char encoded[4096]="";
  int  encoded_len=0;
  cash_encode_utf5 (utf8, strlen (utf8), encoded, &encoded_len, 1);
  uint64_t hash = 0;
  int  utf5 = (encoded[0] != CASH_ENTER_DIRECT);
  uint64_t multiplier = cash_dim == 6 ? 0x5bd1e995
                                      : 0x98173415bd1e995;

  uint64_t overflowed_mask = cash_overflow_mask_for_dim (cash_dim);
  uint64_t all_bits        = overflowed_mask - 1;

  if (cash_dim == 6)
    multiplier = 0x5bd1e995;

  if (encoded_len - (!utf5) <= cash_dim)
  {
    for (int i = !utf5; i < encoded_len; i++)
    {
      uint64_t val = encoded[i];
      hash = hash | (val << (5*(i-(!utf5))));
    }
    hash <<= 1;
  }
  else
  {
    for (int i = !utf5; i < encoded_len; i++)
    {
      uint64_t val = encoded[i];
      hash = hash ^ val;
      hash = hash * multiplier;
      hash = hash & all_bits;
      hash = hash ^ ((hash >> 16));
    }
    hash <<= 1; // making room for the bit that encodes utf5 or direct
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
static int interned_sorted = 1;

static int interned_compare (const void *a, const void *b)
{
  const CashInterned *ia = (CashInterned*)a;
  const CashInterned *ib = (CashInterned*)b;
  if (ia->hash < ib->hash ) return -1;
  else if (ia->hash > ib->hash ) return 1;
  return 0;
}

static inline uint64_t cash (int cash_dim, const char *utf8)
{
  uint64_t hash = _cash (cash_dim, utf8);
#ifdef CASH_NO_INTERNING
  return hash;
#endif
  uint64_t overflowed_mask = cash_overflow_mask_for_dim (cash_dim);
  if (hash & overflowed_mask)
  {
    int i;
    for (i = 0; i < n_interned; i++)
    {
      CashInterned *entry = &interned[i];
      if (entry->hash == hash)
        return hash;
    }

    if (n_interned + 1 >= s_interned)
    {
       s_interned = (s_interned + 128)*1.5;
       interned = (CashInterned*)realloc (interned, s_interned * sizeof (CashInterned));
    }

    {
      CashInterned *entry = &interned[n_interned];
      entry->hash = hash;
      entry->string = strdup (utf8);
    }
    n_interned++;
    interned_sorted = 0;
  }
  return hash;
}

uint32_t cash6 (const char *utf8)
{
  return cash (6, utf8);
}

uint64_t cash10 (const char *utf8)
{
  return cash (10, utf8);
}

uint64_t cash12 (const char *utf8)
{
  return cash (12, utf8);
}

uint64_t ctx_strhash(const char *str) {
  return cash (10, str);
}

typedef struct CashUtf5Dec {
  int       is_utf5;
  int       offset;
  void     *write_data;
  uint32_t  current;
  void    (*append_unichar) (uint32_t unichar, void *write_data);
  int       jump_mode;
} CashUtf5Dec;

typedef struct CashUtf5DecDefaultData {
  uint8_t *buf;
  int      length;
} CashUtf5DecDefaultData;

static void cash_decode_utf5_append_unichar_as_utf8 (uint32_t unichar, void *write_data)
{
  CashUtf5DecDefaultData *data = (CashUtf5DecDefaultData*)write_data;
  int length = cash_unichar_to_utf8 (unichar, &data->buf[data->length]);
  data->buf[data->length += length] = 0;
}

static void cash_decode_jump (CashUtf5Dec *dec, uint8_t in)
{
  switch (dec->jump_mode) /*first we re-adjust offset caused by interpreting first part*/
  {
    case CASH_DEC_OFFSET_A: dec->offset += CASH_JUMP_OFFSET * 2; break;
    case CASH_INC_OFFSET_A: dec->offset -= CASH_JUMP_OFFSET * 2; break;
    case CASH_DEC_OFFSET_B: dec->offset += CASH_JUMP_OFFSET * 1; break;
    case CASH_INC_OFFSET_B: dec->offset -= CASH_JUMP_OFFSET * 1; break;
  }
  int jump_len = (dec->jump_mode - CASH_DEC_OFFSET_A) * 4 +
                 (in - CASH_DEC_OFFSET_A);
  if (jump_len > 7)
  {
    jump_len = 5 - jump_len;
  }
  else
  {
    jump_len += 3;
  }
  dec->offset += jump_len * CASH_JUMP_OFFSET;
  dec->jump_mode = 0;
}

static void cash_decode_utf5 (CashUtf5Dec *dec, uint8_t in)
{
  if (dec->is_utf5)
  {
      if (in >= 16)
      {
        if (dec->current)
        {
          dec->offset = cash_new_offset (dec->current);
          dec->append_unichar (dec->current, dec->write_data);
          dec->current = 0;
        }
      }
      if (in == CASH_ENTER_DIRECT)
      {
        if (dec->current)
        {
          dec->offset = cash_new_offset (dec->current);
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
      switch (in)
      {
        case CASH_ENTER_UTF5:
          dec->is_utf5 = 1;
          dec->jump_mode = 0;
          break;
        case CASH_SPACE: 
          dec->append_unichar (' ', dec->write_data);
          dec->jump_mode = 0;
          break;
        case CASH_DEC_OFFSET_A:
          if (dec->jump_mode)
          {
            cash_decode_jump (dec, in);
          }
          else
          {
            dec->offset -= CASH_JUMP_OFFSET * 2;
            dec->jump_mode = in;
          }
          break;
        case CASH_INC_OFFSET_A:
          if (dec->jump_mode)
          {
            cash_decode_jump (dec, in);
          }
          else
          {
            dec->offset += CASH_JUMP_OFFSET * 2;
            dec->jump_mode = in;
          }
          break;
        case CASH_DEC_OFFSET_B:
          if (dec->jump_mode)
          {
            cash_decode_jump (dec, in);
          }
          else
          {
            dec->offset -= CASH_JUMP_OFFSET;
            dec->jump_mode = in;
          }
          break;
        case CASH_INC_OFFSET_B:
          if (dec->jump_mode)
              cash_decode_jump (dec, in);
          else
          {
              dec->offset += CASH_JUMP_OFFSET;
              dec->jump_mode = in;
          }
          break;
        default:
          dec->append_unichar (dec->offset + (in - 1), dec->write_data);
          dec->jump_mode = 0;
      }
  }
}

static void cash_decode_utf5_bytes (int is_utf5, 
                        const unsigned char *input, int inlen,
                        char *output, int *r_outlen)
{
  CashUtf5DecDefaultData append_data= {(unsigned char*)output, };
  CashUtf5Dec dec = {is_utf5,
                     cash_new_offset('a'),
                     &append_data,
                     0,
                     cash_decode_utf5_append_unichar_as_utf8,
                     0
                    };
  for (int i = 0; i < inlen; i++)
  {
    cash_decode_utf5 (&dec, input[i]);
  }
  if (dec.current)
    dec.append_unichar (dec.current, dec.write_data);
  if (r_outlen)*r_outlen = append_data.length;
}

const char *cash_decode_r (int cash_dim, uint64_t hash, char *ret, int retlen)
{
  uint64_t overflowed_mask = ((uint64_t)1<<(cash_dim * 5 + 1));

  if (!interned_sorted && interned)
  {
    qsort (interned, n_interned, sizeof (CashInterned), interned_compare);
    interned_sorted = 1;
  }
  if (hash & overflowed_mask)
  {
    /* XXX this could be a binary search */
    for (int i = 0; i < n_interned; i++)
    {
      CashInterned *entry = &interned[i];
      if (entry->hash == hash)
        return entry->string;
    }
    return NULL;
  }

  uint8_t utf5[140]=""; // we newer get to 40
  uint64_t tmp = hash & (overflowed_mask-1);
  int len = 0;
  int is_utf5 = tmp & 1;
  tmp /= 2;
  int in_utf5 = is_utf5;
  while (tmp > 0)
  {
    uint64_t remnant = tmp % 32;
    uint64_t val = remnant;

    if      ( in_utf5 && val == CASH_ENTER_DIRECT) in_utf5 = 0;
    else if (!in_utf5 && val == CASH_ENTER_UTF5) in_utf5 = 1;

    utf5[len++] = val;
    tmp -= remnant;
    tmp /= 32;
  }
  //if (in_utf5 && len && utf5[len-1] > 'G')
  //{
  //  utf5[len++] = 0;
  //}
  utf5[len]=0;
  cash_decode_utf5_bytes (is_utf5, utf5, len, ret, &retlen);
  //ret[retlen]=0;
  return ret;
}

const char *cash_decode (int cash_dim, uint64_t hash)
{
  /* this is not really re-entrant - copy the value as soon as possible
   * since another thread might be churning through decodes as well.
   *
   * having thread local storage for this string could make it safe.
   */
#define THREAD __thread  // use thread local storage
  static THREAD int no = 0;
  static THREAD char ret[8][256];
  no ++;
  if (no > 7) no = 0;
  return cash_decode_r (cash_dim, hash, ret[no], 256);
#undef THREAD
}

const char *cash6_decode (uint32_t hash)
{
  return cash_decode (6, hash);
}

const char *cash10_decode (uint64_t hash)
{
  return cash_decode (10, hash);
}

const char *cash12_decode (uint64_t hash)
{
  return cash_decode (12, hash);
}

static inline uint32_t
cash_utf8_to_unichar (const char *input)
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
cash_unichar_to_utf8 (uint32_t  ch,
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
cash_utf8_len (const unsigned char first_byte)
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
