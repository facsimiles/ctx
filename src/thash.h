#ifndef THASH_H
#define THASH_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define THASH_NO_INTERNING   // ctx doesn't make use of thash_decode

#define THASH_ENTER_DIRECT     16

#define THASH_SPACE            0
#define THASH_DEC_OFFSET       29
#define THASH_INC_OFFSET       30
#define THASH_ENTER_UTF5       31
#define THASH_START_OFFSET     'l'
#define THASH_JUMP_OFFSET      27
#define THASH_MAXLEN           10

// todo: better whitespace handling for double version


static inline int thash_new_offset (uint32_t unichar)
{
   int offset = unichar % 32;
   return unichar - offset + 14; // this gives ~85% compression on test corpus
   return unichar; // this gives 88% compression on test corpus
}

static int thash_is_in_range (uint32_t offset, uint32_t unichar)
{
  if (unichar == 32)
    return 1;
  if (offset - unichar <= 13 ||
      unichar - offset <= 14)
      return 1;
  return 0;
}

static int thash_is_in_jump_range_dec (uint32_t offset, uint32_t unichar)
{
  return thash_is_in_range (offset - THASH_JUMP_OFFSET, unichar);
}

static int thash_is_in_jump_range_inc (uint32_t offset, uint32_t unichar)
{
  return thash_is_in_range (offset + THASH_JUMP_OFFSET, unichar);
}

uint32_t ctx_utf8_to_unichar (const char *input);
int ctx_unichar_to_utf8      (uint32_t ch, uint8_t  *dest);
int ctx_utf8_len             (const unsigned char first_byte);

static int thash_utf5_length (uint32_t unichar)
{
  int octets = 0;
  if (unichar == 0) return 1;
  while (unichar)
  {  octets ++;
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

void thash_encode_utf5 (const char *input, int inlen,
                   char *output, int *r_outlen)
{
  uint32_t offset = THASH_START_OFFSET;

  int is_utf5 = 1;
  int len = 0;

  for (int i = 0; i < inlen; i+= ctx_utf8_len (input[i]))
  {
    int val = ctx_utf8_to_unichar(&input[i]);
    int next_val = ' '; // always in range
    int next_next_val = ' ';
    int first_len = ctx_utf8_len (input[i]);
    if (i + first_len < inlen)
    {
        int next_len = ctx_utf8_to_unichar (&input[i + first_len]);
        if (i + first_len + next_len < inlen)
        {
          next_next_val = ctx_utf8_to_unichar (&input[i + first_len + next_len]);
        }
    }

    if (is_utf5)
    {
      int in_range = 
          thash_is_in_range (offset, val) +
          thash_is_in_range (offset, next_val) +
          thash_is_in_range (offset, next_next_val);
      int change_cost = 4;
      int no_change_cost = thash_utf5_length (val) + thash_utf5_length (next_val)
                                                   + thash_utf5_length (next_next_val);

      if (in_range > 2 && change_cost < no_change_cost)
      {
        output[len++] = THASH_ENTER_DIRECT;
        is_utf5 = 0;
      }
    } else
    {
      if (!thash_is_in_range(offset, val))
      {
        if (thash_is_in_jump_range_dec (offset, val))
        {
            output[len++] = THASH_DEC_OFFSET;
            offset -= THASH_JUMP_OFFSET;
        }
        else if (thash_is_in_jump_range_inc (offset, val))
        {
          output[len++] = THASH_INC_OFFSET;
          offset += THASH_JUMP_OFFSET;
        }
        else
        {
          output[len++] = THASH_ENTER_UTF5;
          is_utf5 = 1;
        }
      }
    }

    if (is_utf5)
    {
          int octets = 0;
          offset = thash_new_offset (val);
          while (val)
          {
            int oval = val % 16;
            int last = 0;
            if (val / 32 == 0) last = 16;
            output[len+ (octets++)] = oval + last;
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
        if (val == 32)
        {
          output[len++] = THASH_SPACE;
        }
        else
        {
          output[len++]= val-offset+14;
        }
      }
  }
  if (len && output[len-1]==0)
    output[len++] = 16;
  output[len]=0;
  *r_outlen = len;
}

uint64_t _thash (const char *utf8)
{
  char encoded[4096]="";
  int  encoded_len=0;
  int  wordlen = 0;
  thash_encode_utf5 (utf8, strlen (utf8), encoded, &encoded_len);
#if 0
  Word word = {0};
  word.utf5 = (encoded[0] != THASH_ENTER_DIRECT);
  for (int i = !word.utf5; i < encoded_len; i++)
    word_set_val (&word, wordlen++, encoded[i]);
  return word.hash;
#else
  uint64_t hash = 0;
  int  utf5 = (encoded[0] != THASH_ENTER_DIRECT);
  for (int i = !utf5; i < encoded_len; i++)
  {
    uint64_t val = encoded[i];

    if (wordlen < THASH_MAXLEN)
    {
      hash = hash | (val << (5*wordlen));
      hash &= (((uint64_t)1<<52)-1);
    }
    else
    {
      hash = hash ^ ((hash << 4) + val);
      hash &= (((uint64_t)1<<52)-1);
    }
    wordlen++;
  }
  hash <<= 1;
  if (wordlen >= THASH_MAXLEN) 
    hash |= ((uint64_t)1<<51); // overflowed
  return hash |  utf5;
#endif
}

typedef struct _Interned Interned;

struct _Interned {
    uint64_t   hash;
    char      *string;
};

static Interned *interned = NULL;
static int n_interned = 0;
static int s_interned = 0;
static int interned_sorted = 1;

static int interned_compare (const void *a, const void *b)
{
  const Interned *ia = a;
  const Interned *ib = b;
  if (ia->hash < ib->hash ) return -1;
  else if (ia->hash > ib->hash ) return 1;
  return 0;
}


uint64_t thash (const char *utf8)
{
  uint64_t hash = _thash (utf8);
#ifdef THASH_NO_INTERNING
  return hash;
#endif
  if (hash & ((uint64_t)1<<51)) /* overflowed */
  {
    int i;
    for (i = 0; i < n_interned; i++)
    {
      Interned *entry = &interned[i];
      if (entry->hash == hash)
        return hash;
    }

    if (n_interned + 1 >= s_interned)
    {
       s_interned = (s_interned + 128)*1.5;
       //fprintf (stderr, "\r%p %i ", interned, s_interned);
       interned = realloc (interned, s_interned * sizeof (Interned));
    }

    {
      Interned *entry = &interned[n_interned];
      entry->hash = hash;
      entry->string = strdup (utf8);
    }
    n_interned++;
    interned_sorted = 0;
  }
  return hash;
}
uint64_t ctx_strhash(const char *str, int ignored) { return thash (str);}

typedef struct ThashUtf5Dec {
  int      is_utf5;
  int      offset;
  void    *write_data;
  uint32_t current;
  void   (*append_unichar) (uint32_t unichar, void *write_data);
} ThashUtf5Dec;

typedef struct ThashUtf5DecDefaultData {
   uint8_t *buf;
   int     length;
} ThashUtf5DecDefaultData;

static void thash_decode_utf5_append_unichar_as_utf8 (uint32_t unichar, void *write_data)
{
  ThashUtf5DecDefaultData *data = write_data;
  unsigned char utf8[8]="";
  utf8[ctx_unichar_to_utf8 (unichar, utf8)]=0;
  for (int j = 0; utf8[j]; j++)
    data->buf[data->length++]=utf8[j];
  data->buf[data->length]=0;
}

void thash_decode_utf5 (ThashUtf5Dec *dec, uint8_t in)
{
  if (dec->is_utf5)
  {
      if (in > 16)
      {
        if (dec->current)
        {
          dec->offset = thash_new_offset (dec->current);
          dec->append_unichar (dec->current, dec->write_data);
          dec->current = 0;
        }
      }
      if (in == THASH_ENTER_DIRECT)
      {
        if (dec->current)
        {
          dec->offset = thash_new_offset (dec->current);
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
        case THASH_ENTER_UTF5: dec->is_utf5 = 1;  break;
        case THASH_SPACE:      dec->append_unichar (' ', dec->write_data); break;
        case THASH_DEC_OFFSET: dec->offset -= THASH_JUMP_OFFSET; break;
        case THASH_INC_OFFSET: dec->offset += THASH_JUMP_OFFSET; break;
        default:
          dec->append_unichar (dec->offset + in - 14, dec->write_data);
      }
  }
}

void thash_decode_utf5_bytes (int is_utf5, 
                        const unsigned char *input, int inlen,
                        char *output, int *r_outlen)
{
  ThashUtf5DecDefaultData append_data= {(unsigned char*)output, };
  ThashUtf5Dec dec = {is_utf5,
                    THASH_START_OFFSET,
                    &append_data,
  0, thash_decode_utf5_append_unichar_as_utf8
  };
  for (int i = 0; i < inlen; i++)
  {
    thash_decode_utf5 (&dec, input[i]);
  }
  if (dec.current)
    dec.append_unichar (dec.current, dec.write_data);
  if (r_outlen)*r_outlen = append_data.length;
}

const char *thash_decode (uint64_t hash)
{
  if (!interned_sorted)
  {
    qsort (interned, n_interned, sizeof (Interned), interned_compare);
    interned_sorted = 1;
  }
  if (hash &  ((uint64_t)1<<51))
  {

    for (int i = 0; i < n_interned; i++)
    {
      Interned *entry = &interned[i];
      if (entry->hash == hash)
        return entry->string;
    }
    return "[missing string]";
  }

  static char ret[4096]="";
  uint8_t utf5[40]="";
  uint64_t tmp = hash & (((uint64_t)1<<51)-1);
  int len = 0;
  int is_utf5 = tmp & 1;
  tmp /= 2;
  int in_utf5 = is_utf5;
  while (tmp > 0)
  {
    uint64_t remnant = tmp % 32;
    uint64_t val = remnant;

    if      ( in_utf5 && val == THASH_ENTER_DIRECT) in_utf5 = 0;
    else if (!in_utf5 && val == THASH_ENTER_UTF5) in_utf5 = 1;

    utf5[len++] = val;
    tmp -= remnant;
    tmp /= 32;
  }
  if (in_utf5 && len && utf5[len-1] > 'G')
  {
    utf5[len++] = 0;//utf5_alphabet[0]; 
  }
  utf5[len]=0;
  int retlen = sizeof (ret);
  thash_decode_utf5_bytes (is_utf5, utf5, len, ret, &retlen);
  ret[len]=0;
  return ret;
}

#if 0

#include <assert.h>
#pragma pack(push,1)
typedef union Word
{
  uint64_t hash;
  struct {
    unsigned int utf5:1;
    unsigned int c0:5;
    unsigned int c1:5;
    unsigned int c2:5;
    unsigned int c3:5;
    unsigned int c4:5;
    unsigned int c5:5;
    unsigned int c6:5;
    unsigned int c7:5;
    unsigned int c8:5;
    unsigned int c9:5;
    unsigned int overflowed:1;
  };
} Word;

static inline void word_set_val (Word *word, int no, int val)
{
#if 0
  switch(no)
  {
     case 0: word->c0 = val; break;
     case 1: word->c1 = val; break;
     case 2: word->c2 = val; break;
     case 3: word->c3 = val; break;
     case 4: word->c4 = val; break;
     case 5: word->c5 = val; break;
     case 6: word->c6 = val; break;
     case 7: word->c7 = val; break;
     case 8: word->c8 = val; break;
     case 9: word->c9 = val; break;
     default:
       // for overflow only works when setting all in sequence
       word->hash = word->hash + ((uint64_t)(val) << (5*no+1));
       word->overflowed = 1;
       break;
  }
#else
  word->hash = word->hash | ((uint64_t)(val) << (5*no+1));
  if (no >= 9) 
    word->hash |= ((uint64_t)1<<51);
  word->hash &= (((uint64_t)1<<52)-1);
#endif
}

static inline int word_get_val (Word *word, int no)
{
  switch(no)
  {
     case 0: return word->c0;break;
     case 1: return word->c1;break;
     case 2: return word->c2;break;
     case 3: return word->c3;break;
     case 4: return word->c4;break;
     case 5: return word->c5;break;
     case 6: return word->c6;break;
     case 7: return word->c7;break;
     case 8: return word->c8;break;
     case 9: return word->c9;break;
  }
}

static inline int word_get_length (Word *word)
{
   int len = 0;
   if (word->c0) len ++; else return len;
   if (word->c1) len ++; else return len;
   if (word->c2) len ++; else return len;
   if (word->c3) len ++; else return len;
   if (word->c4) len ++; else return len;
   if (word->c5) len ++; else return len;
   if (word->c6) len ++; else return len;
   if (word->c7) len ++; else return len;
   if (word->c8) len ++; else return len;
   if (word->c9) len ++; else return len;
   return len;
}


static Word *word_append_unichar (Word *word, uint32_t unichar)
{
  //word_set_char (word, word_get_length (word), unichar);
  // append unichar - possibly advancing.
  return word;
}
#endif

#endif
