#ifndef SQUOZE_H
#define SQUOZE_H


#ifndef SQUOZE_REF_SANITY
#define SQUOZE_REF_SANITY 0
  // report consistency errors and use more RAM
#endif

#ifndef SQUOZE_CLOBBER_ON_FREE
#define SQUOZE_CLOBBER_ON_FREE 1
  // clobber strings when freeing, not a full leak report
  // but better to always glitch than silently succeding or failing
#endif

#define SQUOZE_PEEK_STRINGS 4  // maximum number of concurrent same peeked strings
			       // in one expression, i.e. printf("%s %s\n", squoze_peek(a), squoze_peek(b));
			       // needs it to be 2 or higher, consume 16 bytes per entry per thread
			       //
#ifndef SQUOZE_THREADS
#define SQUOZE_THREADS      1  // use thread local storage for strings.
#endif

#ifndef SQUOZE_REF_COUNTING
#define SQUOZE_REF_COUNTING 1
#endif

#ifndef SQUOZE_IMPLEMENTATION_32
#define SQUOZE_IMPLEMENTATION_32 0
// include implementation for 32bit ids - this is the default and most versatile
// for embedded development, where 32bit integer ids and 32bit interned pointers
// are used.
#endif

#ifndef SQUOZE_ID_BITS
#define SQUOZE_ID_BITS 32
#endif

#if SQUOZE_ID_BITS==32
#define squoze_id_t uint32_t
#else
#define squoze_id_t uint64_t
#endif

// the precision to use for interned ids on 64bit platform

#ifndef SQUOZE_IMPLEMENTATION_52
#define SQUOZE_IMPLEMENTATION_52 0
// include implementation for 52bit ids - suitable for storage in doubles
#endif


#ifndef SQUOZE_IMPLEMENTATION_62
// include implementation for 62bit ids - suitable for storage in uint64_t
#define SQUOZE_IMPLEMENTATION_62 0
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if SQUOZE_IMPLEMENTATION_32
static uint32_t     squoze32        (const char *utf8);
static const char  *squoze32_decode (uint32_t    hash);
#endif

#if SQUOZE_IMPLEMENTATION_52
static uint64_t     squoze52        (const char *utf8);
static const char  *squoze52_decode (uint64_t    hash);
#endif

#if SQUOZE_IMPLEMENTATION_62
static uint64_t     squoze62        (const char *utf8);
static const char  *squoze62_decode (uint64_t    hash);
#endif

typedef const char        Squoze;


typedef struct _SquozePool SquozePool;



static SquozePool        *squoze_pool_new     (SquozePool *fallback);
static void               squoze_pool_ref     (SquozePool *pool);
static void               squoze_pool_unref   (SquozePool *pool);


static Squoze            *squoze_pool_add     (SquozePool *pool, const char *str);
static Squoze            *squoze              (const char *str);


static Squoze            *squoze_lookup_id    (SquozePool *pool, squoze_id_t id);

static inline const char *squoze_peek         (Squoze *squozed);
static inline squoze_id_t squoze_id           (Squoze *squozed);

#if SQUOZE_REF_COUNTING
static inline void        squoze_ref          (Squoze *squozed);
static inline void        squoze_unref        (Squoze *squozed);
#endif

//#define SQUOZE_NO_INTERNING  // this disables the interning - providing only a hash (and decode for non-overflowed hashes)

// extra value meaning in UTF5 mode
#define SQUOZE_ENTER_SQUEEZE    16

// value meanings in squeeze mode
#define SQUOZE_SPACE            0
#define SQUOZE_DEC_OFFSET_A     27
#define SQUOZE_INC_OFFSET_A     28
#define SQUOZE_DEC_OFFSET_B     29
#define SQUOZE_INC_OFFSET_B     30
#define SQUOZE_ENTER_UTF5       31


typedef struct _SquozeString SquozeString;

struct _SquozeString {
#if SQUOZE_REF_COUNTING
    int32_t       ref_count; // set to magic value for ROM strings?
#endif
    squoze_id_t   hash;
    char          string[];
};

static SquozeString squoze_empty_string = {65535,
#if SQUOZE_REF_COUNTING
	1, 
#endif
	""};


static uint64_t squoze_encode (SquozePool *pool, int squoze_dim, const char *utf8, Squoze **interned_ref);

static inline uint32_t squoze_utf8_to_unichar (const char *input);
static inline int      squoze_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
static inline int      squoze_utf8_len        (const unsigned char first_byte);


/* returns the base-offset of the segment this unichar belongs to,
 *
 * segments are 26 items long and are offset so that 'a'-'z' is
 * one segment.
 */
#define SQUOZE_JUMP_STRIDE      26
#define SQUOZE_JUMP_OFFSET      19
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

static inline int squoze_words_for_dim (int squoze_dim)
{
  return squoze_dim / 5;
}

static inline uint64_t
squoze_overflow_mask_for_dim (int squoze_dim)
{
  int words = squoze_words_for_dim (squoze_dim);
  return ((uint64_t)1<<(words * 5 + 1));
}

static int squoze_compute_cost_utf5 (int offset, int val, int next_val)
{
  int cost = 0; 
  cost += squoze_utf5_length (val);
  if (next_val)
  {
    cost += squoze_utf5_length (next_val);
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
#ifdef assert
          assert(0); // should not be reached
#endif
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

static inline uint64_t squoze_encode_no_intern (int squoze_dim, const char *utf8)
{
  char encoded[4096]="";
  int  encoded_len=0;
  squoze5_encode (utf8, strlen (utf8), encoded, &encoded_len, 1, 1);
  uint64_t hash = 0;
  int  utf5 = (encoded[0] != SQUOZE_ENTER_SQUEEZE);
  uint64_t multiplier = ((squoze_dim == 32) ? 0x123456789
                                           : 0x123456789abcdef);

  uint64_t overflowed_mask = squoze_overflow_mask_for_dim (squoze_dim);
  uint64_t all_bits        = overflowed_mask - 1;

  int rshift = (squoze_dim == 32) ? 6 : 13;
  int words = squoze_words_for_dim (squoze_dim);

  if (encoded_len - (!utf5) <= words)
  {
    for (int i = !utf5; i < encoded_len; i++)
    {
      uint64_t val = encoded[i];
      hash = hash | (val << (5*(i-(!utf5))));
    }
    hash <<= 1; // make room for the bit that encodes utf5 or squeeze
    hash |= utf5;
  }
  else
  {
    for (int i = 0; i < encoded_len; i++)
    {
      uint64_t val = encoded[i];
      hash = hash ^ val;
      hash = hash * multiplier;
      hash = hash & all_bits;
      hash = hash ^ (hash >> rshift);
    }
    hash |= overflowed_mask;
  }
  return hash;
}

static inline SquozeString *squoze_str_to_struct (const char *str)
{
  if (((size_t)(str)) & 0x1)
    return (void*)&squoze_empty_string;
  return (SquozeString*)(str - sizeof (SquozeString));
}

#ifdef __CTX_H__
#define strdup ctx_strdup
#define strstr ctx_strstr
#endif

struct _SquozePool
{
  int32_t        ref_count;
  SquozePool    *fallback;
  SquozeString **hashtable;
  int            count;
  int            size;
  SquozePool    *next;
};

SquozePool global_pool = {0, NULL, NULL, 0,0};

SquozePool *squoze_pools = NULL;

static int squoze_pool_find (SquozePool *pool, uint64_t hash)
{
  if (pool->size == 0)
    return -1;
  int pos = hash & (pool->size-1);
  if (!pool->hashtable[pos])
    return -1;
  while (pool->hashtable[pos]->hash != hash)
  {
    pos++;
    pos &= (pool->size-1);
    if (!pool->hashtable[pos])
      return -1;
  }
  return pos;
}
static int squoze_pool_add_entry (SquozePool *pool, SquozeString *str)
{
  if (pool->count + 1 >= pool->size / 2)
  {
     SquozeString **old = pool->hashtable;
     int old_size = pool->size;
     if (old_size == 0)
       pool->size = 256;
     else
       pool->size *= 2;
     pool->hashtable = calloc (pool->size * sizeof (void*), 1);
     if (old)
     {
       for (int i = 0; i < old_size; i++)
         if (old[i])
           squoze_pool_add_entry (pool, old[i]);
       free (old);
     }
  }
  pool->count++;

  int pos = str->hash & (pool->size-1);
  while (pool->hashtable[pos])
  {
    pos++;
    pos &= (pool->size-1);
  }
  pool->hashtable[pos]=str;
  return pos;
}

static int squoze_pool_remove (SquozePool *pool, Squoze *squozed, int do_free)
{
  SquozeString *str = squoze_str_to_struct (squozed);
  int no = squoze_pool_find (pool, str->hash);
  if (no < 0)
    return 0;
  if (do_free)
    free (str);
#ifdef assert
  assert (pool->hashtable[no] == squozed);
#endif
  pool->hashtable[no]=0;
  
  // check if there is another one to promote now
  for (int i = no+1; pool->hashtable[i]; i = (i+1)&(pool->size-1))
  {
    if ((pool->hashtable[i]->hash & (pool->size-1)) == (unsigned)no)
    {
      SquozeString *for_upgrade = pool->hashtable[i];
      squoze_pool_remove (pool, for_upgrade->string, 0);
      squoze_pool_add_entry (pool, for_upgrade);
      break;
    }
  }
  return 1;
}

static SquozeString *squoze_lookup_struct_by_id (SquozePool *pool, squoze_id_t id)
{
  int pos = squoze_pool_find (pool, id);
  if (pos >= 0)
    return pool->hashtable[pos];
  if (pool->fallback)
    return squoze_lookup_struct_by_id (pool->fallback, id);
  return NULL;
}

static Squoze *squoze_lookup_id (SquozePool *pool, squoze_id_t id)
{
  SquozeString *str = squoze_lookup_struct_by_id (pool, id);
  if (str)
    return str->string;
  return NULL;
}

 // we do 32bit also for 64bit - we want the same predetermined hashes to match
static Squoze *squoze_pool_add (SquozePool *pool, const char *str)
{
  if (!pool) pool = &global_pool;
  const char *interned = NULL;
  uint64_t hash = squoze_encode (pool, SQUOZE_ID_BITS, str, &interned);

  if (interned)
  {
#ifdef assert
    assert ((((size_t)interned)&0x1)==0);
#endif
    return interned;
  }
  else
    return (void*)(hash*2+1);
}

static SquozeString *squoze_lookup_struct_by_id (SquozePool *pool, squoze_id_t id);
// encodes utf8 to a squoze id of squoze_dim bits - if interned_ret is provided overflowed ids
// are interned and a new interned squoze is returned.
static uint64_t squoze_encode (SquozePool *pool, int squoze_dim, const char *utf8, Squoze **interned_ref)
{
  if (pool == NULL) pool = &global_pool;
  uint64_t hash = squoze_encode_no_intern (squoze_dim, utf8);
#ifdef SQUOZE_NO_INTERNING
  return hash;
#endif
  uint64_t overflowed_mask = squoze_overflow_mask_for_dim (squoze_dim);
  if (hash & overflowed_mask)
  {
    SquozeString *str = squoze_lookup_struct_by_id (pool, hash);
    if (str)
    {
#if SQUOZE_REF_COUNTING
      str->ref_count++;
#endif
      if (interned_ref) *interned_ref = str->string;
      return hash; // XXX: add verification that it is correct in debug mode
    }

    {
      uint32_t length = strlen (utf8);
      SquozeString *entry = calloc (length + 1 + sizeof(SquozeString), 1);
      entry->hash = hash;
      strcpy (entry->string, utf8);
      if (interned_ref) *interned_ref = entry->string;
      squoze_pool_add_entry (pool, entry);
    }
  }
  return hash;
}

static inline int squoze_is_inline (Squoze *squozed)
{
  return ((((size_t)(squozed))&1) == 1);
}

static inline int squoze_is_interned (Squoze *squozed)
{
  return !squoze_is_inline (squozed);
}

static inline const char *squoze_decode (int squoze_dim, uint64_t hash);

static Squoze *squoze (const char *str)
{
  return squoze_pool_add (NULL, str);
}
static const char *squoze_peek (Squoze *squozed)
{
  if (squoze_is_inline (squozed))
  {
    // we pass NULL as pool since we know it should not be in the pool
    // and we can always decode as 62bit - since we know we didnt overflow the
    // below type.
    return squoze_decode (62, ((size_t)squozed)/2);
  }
  else
    return squozed;//->string;
}

#if SQUOZE_REF_COUNTING
static inline void squoze_ref (Squoze *squozed)
{
  if (squoze_is_interned (squozed))
  {
    SquozeString *str = squoze_str_to_struct (squozed);
    if (str)
       str->ref_count ++;
  }
}

static inline void squoze_unref (Squoze *squozed)
{
  if (squoze_is_interned (squozed))
  {
    SquozeString *str = squoze_str_to_struct (squozed);
    if (str)
    {
      if (str->ref_count <= 0)
      {
#if SQUOZE_CLOBBER_ON_FREE
	str->string[-str->ref_count]='#';
#endif
#if SQUOZE_REF_SANITY
	if (str->ref_count < 0)
	  fprintf (stderr, "double unref for \"%s\"\n", str->string);
        str->ref_count--;
#else
        SquozePool *pool = &global_pool;
	if (squoze_pool_remove (pool, str->string, 1))
	{
		fprintf (stderr, "removed from global pool\n");
	  return;
	}
	pool = squoze_pools;
	if (pool)
	do {
	  if (squoze_pool_remove (pool, str->string, 1))
	  {
		fprintf (stderr, "removed from a pool\n");
	    return;
	  }
	  pool = pool->next;
	} while (pool);
        fprintf (stderr, "not found in pools\n");
#endif
      }
      else
      {
        str->ref_count--;
      }
    }
  }
}

#endif

static inline squoze_id_t squoze_id (Squoze *squozed)
{
  if (squoze_is_inline (squozed))
  {
    return ((size_t)(squozed))/2;
  }
  else
  {
    SquozeString *str = squoze_str_to_struct (squozed);
    if (str) return str->hash;
    return 0;
  }
}

#if SQUOZE_REF_COUNTING

#define SQUOZE_IMPLEMENT(dim, h_t) \
static h_t squoze##dim(const char *utf8)\
{\
  return squoze_encode (NULL, dim, utf8, NULL);\
}

#else

#define SQUOZE_IMPLEMENT(dim, h_t) \
static h_t squoze##dim(const char *utf8)\
{\
  return squoze_encode (NULL, dim, utf8, NULL);\
}
#endif

#if SQUOZE_IMPLEMENTATION_32
  SQUOZE_IMPLEMENT(32, uint32_t)
#endif

#if SQUOZE_IMPLEMENTATION_52
  SQUOZE_IMPLEMENT(52, uint64_t)
#endif

#if SQUOZE_IMPLEMENTATION_62
  SQUOZE_IMPLEMENT(62, uint64_t)
#endif


typedef struct SquozeUtf5Dec {
  int       is_utf5;
  int       offset;
  void     *write_data;
  uint32_t  current;
  void    (*append_unichar) (uint32_t unichar, void *write_data);
  int       jumped_amount;
  int       jump_mode;
} SquozeUtf5Dec;

typedef struct SquozeUtf5DecDefaultData {
  uint8_t *buf;
  int      length;
} SquozeUtf5DecDefaultData;

static void squoze_decode_utf5_append_unichar_as_utf8 (uint32_t unichar, void *write_data)
{
  SquozeUtf5DecDefaultData *data = (SquozeUtf5DecDefaultData*)write_data;
  int length = squoze_unichar_to_utf8 (unichar, &data->buf[data->length]);
  data->buf[data->length += length] = 0;
}

static void squoze_decode_jump (SquozeUtf5Dec *dec, uint8_t in)
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

static void squoze_decode_utf5 (SquozeUtf5Dec *dec, uint8_t in)
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
  SquozeUtf5DecDefaultData append_data = {(unsigned char*)output, 0};
  SquozeUtf5Dec dec = {is_utf5,
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
  uint64_t overflowed_mask = squoze_overflow_mask_for_dim (squoze_dim);

#if 0
  if (pool && (hash & overflowed_mask))
  {
    SquozeString *str = squoze_lookup_struct_by_id (pool, hash);
    if (str) return str->string;
    return NULL;
  }
#endif

  uint8_t utf5[140]=""; // we newer go really high since there isnt room
                        // in the integers
  uint64_t tmp = hash & (overflowed_mask-1);
  int len = 0;
  int is_utf5 = tmp & 1;
  tmp /= 2;
  int in_utf5 = is_utf5;
  utf5[len]=0;
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
static inline const char *squoze_decode (int squoze_dim, uint64_t hash)
{
  if (hash == 0 || hash == 1) return "";
#if SQUOZE_THREADS
  static __thread int no = 0;
  static __thread char ret[SQUOZE_PEEK_STRINGS][16];
#else
  static int  no = 0;
  static char ret[SQUOZE_PEEK_STRINGS][16];
#endif
  no ++;
  if (no >= SQUOZE_PEEK_STRINGS) no = 0;
  return squoze_decode_r (squoze_dim, hash, ret[no], 16);
}

static SquozePool *squoze_pool_new     (SquozePool *fallback)
{
  SquozePool *pool = calloc (sizeof (SquozePool), 1);
  pool->fallback = fallback;
  pool->next = squoze_pools;
  squoze_pools = pool;
  if (fallback)
    squoze_pool_ref (fallback);
  return pool;
}

static void squoze_pool_ref (SquozePool *pool)
{
  if (!pool) return;
  pool->ref_count--;
}

static void squoze_pool_unref (SquozePool *pool)
{
  if (!pool) return;
  if (pool->ref_count == 0)
  {
    for (int i = 0; i < pool->size; i++)
    {
      if (pool->hashtable[i])
        free (pool->hashtable[i]);
    }
    if (pool->fallback)
      squoze_pool_unref (pool->fallback);

    if (pool == squoze_pools)
    {
       squoze_pools = pool->next;
    }
    else
    {
      SquozePool *prev = NULL;
      SquozePool *iter = squoze_pools;
      while (iter && iter != pool)
      {
         prev = iter;
         iter = iter->next;
      }
      if (prev) // XXX not needed
        prev->next = pool->next;
    }
    free (pool);
  }
  else
  {
    pool->ref_count--;
  }
}

#if SQUOZE_IMPLEMENTATION_32
static const char *squoze32_decode (uint32_t hash)
{
  return squoze_decode (32, hash);
}
#endif

#if SQUOZE_IMPLEMENTATION_52
static const char *squoze52_decode (uint64_t hash)
{
  return squoze_decode (52, hash);
}
#endif

#if SQUOZE_IMPLEMENTATION_62
static const char *squoze62_decode (uint64_t hash)
{
  return squoze_decode (62, hash);
}
#endif





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
