/* Copyright (c) 2021-2022 Øyvind Kolås <pippin@gimp.org>

  License to be determined, the core implementation the snippets on
  https://squoz.org/ are ISC licensed

*/

#ifndef SQUOZE_H
#define SQUOZE_H

#include <stdint.h>

// configuration of internal squoze, these
// are values that must be set before both header
// and implementation uses of squoze.h the values only
// impact the string interning implementation and not
// the low-level APIs

#ifndef SQUOZE_USE_INTERN      // enable interning hash-table
#define SQUOZE_USE_INTERN 1    // without this only a single
                               // core implementation can be built
#endif

#ifndef SQUOZE_INTERN_DIRECT_STRING     // when 1 the pointers returned are
#define SQUOZE_INTERN_DIRECT_STRING  1  // directly string pointers
                                        // when 0 the struct of the per entry
                                        // allocation is returned, for integration
                                        // with garbage collectors that scan
                                        // for pointers 0 is preferable.
#endif

#ifndef SQUOZE_ID_BITS         // number of bits to use for interning API
#define SQUOZE_ID_BITS 64      // 32 52 62 or 64
#endif

#ifndef SQUOZE_ID_UTF5         // use UTF5+ as the embed encoding
#define SQUOZE_ID_UTF5 0       // if not set then UTF8 is used
#endif

#ifndef SQUOZE_ID_MURMUR       // use murmurhash and no embedding
#define SQUOZE_ID_MURMUR 0     //
#endif

#ifndef SQUOZE_REF_COUNTING    // build the refcounting support, adds
#define SQUOZE_REF_COUNTING 0  // per-interned-string overhead
#endif

#ifndef SQUOZE_STORE_LENGTH    // store byte-lengths as part of
#define SQUOZE_STORE_LENGTH 1  // per-interned-string data
#endif


#if SQUOZE_USE_INTERN

#if SQUOZE_ID_BITS==32
typedef uint32_t sqz_id_t;
#else
typedef uint64_t sqz_id_t;
#endif


typedef struct _Sqz      Sqz;      /* handle representing a squozed string  */


/* create a new string that is the concatenation of a and b
 */
Sqz         *sqz                  (const char *str);
const char  *sqz_decode           (Sqz *squozed, char *temp);


int          sqz_length           (Sqz *squozed);
sqz_id_t     sqz_id               (Sqz *squozed);
uint32_t     sqz_unichar_at       (Sqz *a, int pos);
int          sqz_strcmp           (Sqz *a, Sqz *b);
inline int   sqz_equal            (Sqz *a, Sqz *b) { return a == b; }


Sqz         *sqz_cat              (Sqz *a, Sqz *b);
Sqz         *sqz_substring        (Sqz *a, int pos, int length);

void         sqz_insert           (Sqz **a, int pos, Sqz *b);
void         sqz_assign           (Sqz **a, Sqz *b);
void         sqz_erase            (Sqz **a, int pos, int length);

Sqz         *sqz_printf           (const char *format, ...);
Sqz         *sqz_printf_va_list   (const char *format, va_list list);
Sqz         *sqz_unichar          (uint32_t unichar);

/* the following is APIs mostly implemented in terms of the above */

int          sqz_has_prefix       (Sqz *a, Sqz *prefix);
int          sqz_has_suffix       (Sqz *a, Sqz *suffix);

void         sqz_insert_unichar   (Sqz **a, int pos, uint32_t unichar);
void         sqz_replace_unichar  (Sqz **a, int pos, int length, uint32_t unichar);
int          sqz_has_prefix_utf8  (Sqz *a, const char *utf8);
int          sqz_has_suffix_utf8  (Sqz *a, const char *utf8);
void         sqz_insert_utf8      (Sqz **a, int pos, const char *utf8);
void         sqz_assign_utf8      (Sqz **a, const char *utf8);
void         sqz_replace_utf8     (Sqz **a, int pos, int length, const char *utf8);
void         sqz_assign_printf    (Sqz **a, const char *format, ...);
void         sqz_insert_printf    (Sqz **a, int pos, const char *format, ...);
void         sqz_replace_printf   (Sqz **a, int pos, int length, const char *format, ...);
/* increase reference count of string */
Sqz         *sqz_ref              (Sqz *squozed);
/* decrement reference count of string */
void         sqz_unref            (Sqz *squozed);
typedef struct _SqzPool  SqzPool;  /* a pool for grouping allocated strings */


/* create a new string pool, with fallback to another pool -
 * or NULL for fallback to default pool, takes a reference on fallback.
 */
SqzPool  *sqz_pool_new            (SqzPool *fallback);

/* increase reference count of pool
 */
void         sqz_pool_ref         (SqzPool *pool);

/* decrease reference point of pool, when matching _new() + _ref() calls
 * the pool is destoryed.
 */
void         sqz_pool_unref       (SqzPool *pool);

/* add a string to a squoze pool
 */
Sqz         *sqz_pool_add         (SqzPool *pool, const char *str);

Sqz *sqz_concat (Sqz *a, Sqz *b);

/* Report stats on interned strings 
 */
void sqz_pool_mem_stats (SqzPool *pool,
                         size_t     *size,
                         size_t     *slack,
                         size_t     *intern_alloc);

/* empty all pools
 */
void sqz_atexit (void);

#endif

#ifndef SQUOZE_REF_SANITY
#define SQUOZE_REF_SANITY      0 // report consistency errors and use more RAM
#endif

#ifndef SQUOZE_CLOBBER_ON_FREE
#define SQUOZE_CLOBBER_ON_FREE 0
  // clobber strings when freeing, not a full leak report
  // but better to always glitch than silently succeding or failing
#endif

#ifndef SQUOZE_INITIAL_POOL_SIZE
#define SQUOZE_INITIAL_POOL_SIZE   (1<<8)  // initial hash-table capacity
#endif

#ifndef SQUOZE_USE_BUILTIN_CLZ
#define SQUOZE_USE_BUILTIN_CLZ  1 // use builtin for determining highest bit in unicode char
#endif

#ifndef SQUOZE_UTF8_MANUAL_UNROLL
#define SQUOZE_UTF8_MANUAL_UNROLL 1 // use manually unrolled UTF8 code
#endif

#ifndef SQUOZE_LIMIT_IMPLEMENTATIONS
#define SQUOZE_LIMIT_IMPLEMENTATIONS 0
#endif

#ifndef SQUOZE_IMPLEMENTATION_32_UTF8
#define SQUOZE_IMPLEMENTATION_32_UTF8 (!SQUOZE_LIMIT_IMPLEMENTATIONS)
#endif
#ifndef SQUOZE_IMPLEMENTATION_32_UTF5
#define SQUOZE_IMPLEMENTATION_32_UTF5 (!SQUOZE_LIMIT_IMPLEMENTATIONS)
#endif
#ifndef SQUOZE_IMPLEMENTATION_52_UTF5
#define SQUOZE_IMPLEMENTATION_52_UTF5 (!SQUOZE_LIMIT_IMPLEMENTATIONS)
#endif
#ifndef SQUOZE_IMPLEMENTATION_62_UTF5
#define SQUOZE_IMPLEMENTATION_62_UTF5 (!SQUOZE_LIMIT_IMPLEMENTATIONS)
#endif
#ifndef SQUOZE_IMPLEMENTATION_64_UTF8
#define SQUOZE_IMPLEMENTATION_64_UTF8 (!SQUOZE_LIMIT_IMPLEMENTATIONS)
#endif

#if SQUOZE_IMPLEMENTATION_32_UTF5 || \
    SQUOZE_IMPLEMENTATION_52_UTF5 || \
    SQUOZE_IMPLEMENTATION_62_UTF5
#define SQUOZE_USE_UTF5 1
#else
#define SQUOZE_USE_UTF5 0
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if SQUOZE_IMPLEMENTATION_32_UTF5
uint32_t     squoze32_utf5        (const char *utf8, size_t len);
const char  *squoze32_utf5_decode (uint32_t    id,   char *dest);
#endif

#if SQUOZE_IMPLEMENTATION_32_UTF8
uint32_t     squoze32_utf8        (const char *utf8, size_t len);
const char  *squoze32_utf8_decode (uint32_t    id,   char *dest);
#endif

#if SQUOZE_IMPLEMENTATION_52_UTF5
uint64_t     squoze52_utf5        (const char *utf8, size_t len);
const char  *squoze52_utf5_decode (uint64_t    id,   char *dest);
#endif

#if SQUOZE_IMPLEMENTATION_62_UTF5
uint64_t     squoze62_utf5        (const char *utf8, size_t len);
const char  *squoze62_utf5_decode (uint64_t    id,   char *dest);
#endif

#if SQUOZE_IMPLEMENTATION_64_UTF8
uint64_t     squoze64_utf8        (const char *utf8, size_t len);
const char  *squoze62_utf8_decode (uint64_t    id,   char *dest);
#endif

#endif

#ifdef SQUOZE_IMPLEMENTATION


static inline uint32_t MurmurOAAT32 (const char * key, int len)
{
  size_t h = 3323198485ul;
  for (int i = 0;i < len;i++) {
    h ^= key[i];
    h *= 0x5bd1e995;
    //h &= 0xffffffff;
    h ^= h >> 15;
  }
  return h;
}

static inline uint64_t MurmurOAAT64 ( const char * key, int len)
{
  uint64_t h = 525201411107845655ull;
  for (int i = 0;i < len;i++) {
    h ^= key[i];
    h *= 0x5bd1e9955bd1e995;
    h ^= h >> 47;
  }
  return h;
}

#if SQUOZE_USE_UTF5 // YYY

// TODO:  UTF5+ should operate directly on bits instead of
//        going via bytes
static inline void squoze5_encode (const char *input, int inlen,
                                   char *output, int *r_outlen,
                                   int   permit_squeezed,
                                   int   escape_endzero);
static void squoze_decode_utf5_bytes (int is_utf5, 
                                      const unsigned char *input, int inlen,
                                      char *output, int *r_outlen);
static inline size_t squoze5_encode_int (const char *input, int inlen,
                                         int maxlen, int *overflow,
                                         int escape_endzero);

#endif


/* this should have the same behavior as the bitwidth and encoding
 * specific implementations
 */
static inline uint64_t squoze_encode_id (int squoze_dim, int utf5, const char *stf8, size_t len)
{
  int length = len;
  uint64_t id = 0;
#if SQUOZE_USE_UTF5
  if (utf5)
  {
    int max_quintets = squoze_dim / 5;
    if (length <= max_quintets)
    {
      int overflow = 0;
      id = squoze5_encode_int (stf8, length, max_quintets, &overflow, 1);
      if (!overflow)
        return id;
    }
    id = 0;
    id = MurmurOAAT32(stf8, length);
    id &= ~1;
  }
  else
#endif
  {
    const uint8_t *utf8 = (const uint8_t*)stf8;
    if (squoze_dim > 32)
      squoze_dim = 64;
    int bytes_dim = squoze_dim / 8;
  
    uint8_t first_byte = ((uint8_t*)utf8)[0];
    if (first_byte<128
        && first_byte != 11
        && (length <= bytes_dim))
    {
      id = utf8[0] * 2 + 1;
      for (int i = 1; i < length; i++)
        id += ((uint64_t)utf8[i]<<(8*(i)));
    }
    else if (length <= bytes_dim-1)
    {
      id = 23;
      for (int i = 0; i < length; i++)
        id += ((uint64_t)utf8[i]<<(8*(i+1)));
    }
    else
    {
      id = MurmurOAAT32(stf8, len);
      id &= ~1;  // make even - intern marker
    }
  }
  return id;
}

#ifdef __CTX_H__ // override with ctx variants if included from ctx
#define strdup ctx_strdup
#define strstr ctx_strstr
#endif


#if SQUOZE_IMPLEMENTATION_32_UTF5
uint32_t squoze32_utf5 (const char *utf8, size_t len)
{
  return squoze_encode_id (32, 1, utf8, len);
}
#endif

#if SQUOZE_IMPLEMENTATION_52_UTF5
uint64_t squoze52_utf5 (const char *utf8, size_t len)
{
  return squoze_encode_id (52, 1, utf8, len);
}
#endif

#if SQUOZE_IMPLEMENTATION_62_UTF5
uint64_t squoze62_utf5 (const char *utf8, size_t len)
{
  return squoze_encode_id (62, 1, utf8, len);
}
#endif

static inline uint64_t squoze_utf8 (size_t bytes_dim, const char *stf8, size_t length)
{
  uint64_t id;
  const uint8_t *utf8 = (const uint8_t*)stf8;

  uint8_t first_byte = ((uint8_t*)utf8)[0];
  if (   first_byte < 128
      && first_byte != 11
      && (length <= bytes_dim))
  {
      switch (length)
      {
#if SQUOZE_UTF8_MANUAL_UNROLL
        case 0: id = 1;
                break;
        case 1: id = utf8[0] * 2 + 1;
                break;
        case 2: id = utf8[0] * 2 + 1 + (utf8[1] << (8*1));
                break;
        case 3: id = utf8[0] * 2 + 1 + (utf8[1] << (8*1))
                                     + (utf8[2] << (8*2));
                break;
        case 4: id = utf8[0] * 2 + 1 + (utf8[1] << (8*1))
                                     + (utf8[2] << (8*2))
                                     + (utf8[3] << (8*3));
                break;
        case 5: id = utf8[0] * 2 + 1 + ((uint64_t)utf8[1] << (8*1))
                                     + ((uint64_t)utf8[2] << (8*2))
                                     + ((uint64_t)utf8[3] << (8*3))
                                     + ((uint64_t)utf8[4] << (8*4));
                break;
        case 6: id = utf8[0] * 2 + 1 + ((uint64_t)utf8[1] << (8*1))
                                     + ((uint64_t)utf8[2] << (8*2))
                                     + ((uint64_t)utf8[3] << (8*3))
                                     + ((uint64_t)utf8[4] << (8*4))
                                     + ((uint64_t)utf8[5] << (8*5));
                break;
        case 7: id = utf8[0] * 2 + 1 + ((uint64_t)utf8[1] << (8*1))
                                     + ((uint64_t)utf8[2] << (8*2))
                                     + ((uint64_t)utf8[3] << (8*3))
                                     + ((uint64_t)utf8[4] << (8*4))
                                     + ((uint64_t)utf8[5] << (8*5))
                                     + ((uint64_t)utf8[6] << (8*6));
                break;
        case 8: id = utf8[0] * 2 + 1 + ((uint64_t)utf8[1] << (8*1))
                                     + ((uint64_t)utf8[2] << (8*2))
                                     + ((uint64_t)utf8[3] << (8*3))
                                     + ((uint64_t)utf8[4] << (8*4))
                                     + ((uint64_t)utf8[5] << (8*5))
                                     + ((uint64_t)utf8[6] << (8*6))
                                     + ((uint64_t)utf8[7] << (8*7));
                break;
#endif
        default:
          id = utf8[0] * 2 + 1;
          for (unsigned int i = 1; i < length; i++)
            id += ((uint64_t)utf8[i]<<(8*(i)));
      }
    return id;
  }
  else if (length <= bytes_dim-1)
  {
      switch (length)
      {
#if SQUOZE_UTF8_MANUAL_UNROLL
        case 0: id = 23;
          break;
        case 1: id = 23 + (utf8[0] << (8*1));
          break;
        case 2: id = 23 + (utf8[0] << (8*1))
                        + (utf8[1] << (8*2));
          break;
        case 3: id = 23 + (utf8[0] << (8*1))
                        + (utf8[1] << (8*2))
                        + (utf8[2] << (8*3));
          break;
        case 4: id = 23 + ((uint64_t)utf8[0] << (8*1))
                        + ((uint64_t)utf8[1] << (8*2))
                        + ((uint64_t)utf8[2] << (8*3))
                        + ((uint64_t)utf8[3] << (8*4));
          break;
        case 5: id = 23 + ((uint64_t)utf8[0] << (8*1))
                        + ((uint64_t)utf8[1] << (8*2))
                        + ((uint64_t)utf8[2] << (8*3))
                        + ((uint64_t)utf8[3] << (8*4))
                        + ((uint64_t)utf8[4] << (8*5));
          break;
        case 6: id = 23 + ((uint64_t)utf8[0] << (8*1))
                        + ((uint64_t)utf8[1] << (8*2))
                        + ((uint64_t)utf8[2] << (8*3))
                        + ((uint64_t)utf8[3] << (8*4))
                        + ((uint64_t)utf8[4] << (8*5))
                        + ((uint64_t)utf8[5] << (8*6));
          break;
        case 7: id = 23 + ((uint64_t)utf8[0] << (8*1))
                        + ((uint64_t)utf8[1] << (8*2))
                        + ((uint64_t)utf8[2] << (8*3))
                        + ((uint64_t)utf8[3] << (8*4))
                        + ((uint64_t)utf8[4] << (8*5))
                        + ((uint64_t)utf8[5] << (8*6))
                        + ((uint64_t)utf8[6] << (8*7));
          break;
#endif
        default:
          id = 23;
          for (unsigned int i = 0; i < length; i++)
            id += ((uint64_t)utf8[i]<<(8*(i+1)));
      }
    return id;
  }

  id = MurmurOAAT32(stf8, length);
  id &= ~1;  // make even - intern marker
  return id;
}

#if SQUOZE_IMPLEMENTATION_64_UTF8
uint64_t squoze64_utf8 (const char *stf8, size_t length)
{
  return squoze_utf8 (8, stf8, length);
}
#endif

#if SQUOZE_IMPLEMENTATION_32_UTF8
uint32_t squoze32_utf8 (const char *stf8, size_t length)
{
  uint32_t id;
  const uint8_t *utf8 = (const uint8_t*)stf8;
  size_t bytes_dim = 4;

  uint8_t first_byte = ((uint8_t*)utf8)[0];
  if (first_byte    < 128
      && first_byte != 11
      && (length <= bytes_dim))
  {
      switch (length)
      {
#if SQUOZE_UTF8_MANUAL_UNROLL
        case 0: id = 1;
                break;
        case 1: id = utf8[0] * 2 + 1;
                break;
        case 2: id = utf8[0] * 2 + 1 + (utf8[1] << (8*1));
                break;
        case 3: id = utf8[0] * 2 + 1 + (utf8[1] << (8*1))
                                     + (utf8[2] << (8*2));
                break;
        case 4: id = utf8[0] * 2 + 1 + (utf8[1] << (8*1))
                                     + (utf8[2] << (8*2))
                                     + (utf8[3] << (8*3));
                break;
#endif
        default:
          id = utf8[0] * 2 + 1;
          for (unsigned int i = 1; i < length; i++)
            id += ((uint32_t)utf8[i]<<(8*(i)));
      }
    return id;
  }
  else if (length <= bytes_dim-1)
  {
      switch (length)
      {
#if SQUOZE_UTF8_MANUAL_UNROLL
        case 0: id = 23;
          break;
        case 1: id = 23 + (utf8[0] << (8*1));
          break;
        case 2: id = 23 + (utf8[0] << (8*1))
                        + (utf8[1] << (8*2));
          break;
        case 3: id = 23 + (utf8[0] << (8*1))
                        + (utf8[1] << (8*2))
                        + (utf8[2] << (8*3));
          break;
#endif
        default:
          id = 23;
          for (unsigned int i = 0; i < length; i++)
            id += (utf8[i]<<(8*(i+1)));
      }
    return id;
  }

  id = MurmurOAAT32(stf8, length);
  id &= ~1;  // make even - intern marker
  return id;
}
#endif


static const char *squoze_id_decode_r (int squoze_dim, uint64_t hash, char *ret, int retlen, int is_utf5)
{
#if SQUOZE_USE_UTF5
  if (is_utf5)
  {
    int is_utf5       = (hash & 2)!=0;
    uint8_t utf5[20]=""; // we newer go really high since there isnt room
                          // in the integers
    uint64_t tmp = hash;
    int len = 0;
    tmp /= 4;
    utf5[len]=0;
    while (tmp > 0)
    {
      utf5[len++] = tmp & 31;
      tmp /= 32;
    }
    utf5[len]=0;
    squoze_decode_utf5_bytes (is_utf5, utf5, len, ret, &retlen);
    return ret;
  }
  else
#endif
  {
    if (squoze_dim == 32)
    {
      if ((hash & 0xff) == 23)
      {
         memcpy (ret, ((char*)&hash)+1, 3);
         ret[3] = 0;
      }
      else
      {
        memcpy (ret, &hash, 4);
        ((unsigned char*)ret)[0]/=2;
        ret[4] = 0;
      }
    }
    else
    {
      if ((hash & 0xff) == 23)
      {
        memcpy (ret, ((char*)&hash)+1, 7);
        ret[7] = 0;
      }
      else
      {
        memcpy (ret, &hash, 8);
        ((unsigned char*)ret)[0]/=2;
        ret[8] = 0;
      }
    }
    return ret;
  }
}

const char *squoze_id_decode (int squoze_dim, uint64_t id, int is_utf5, char *dest)
{
  if (id == 0 || ((id & 1) == 0)) {dest[0]=0;return NULL; }
  else if (id == 3) { dest[0]=0;return NULL;}
  squoze_id_decode_r (squoze_dim, id, dest, 16, is_utf5);
  return dest;
}

#if SQUOZE_IMPLEMENTATION_32_UTF5
const char *squoze32_utf5_decode (uint32_t id, char *dest)
{
  return squoze_id_decode (32, id, 1, dest);
}
#endif

#if SQUOZE_IMPLEMENTATION_52_UTF5
const char *squoze52_utf5_decode (uint64_t id, char *dest)
{
  return squoze_id_decode (52, id, 1, dest);
}
#endif

#if SQUOZE_IMPLEMENTATION_62_UTF5
const char *squoze62_utf5_decode (uint64_t id, char *dest)
{
  return squoze_id_decode (62, id, 1, dest);
}
#endif

#if SQUOZE_IMPLEMENTATION_64_UTF8
const char *squoze64_utf8_decode (uint64_t id, char *dest)
{
  return squoze_id_decode (64, id, 0, dest);
}
#endif

#if SQUOZE_IMPLEMENTATION_32_UTF8
const char *squoze32_utf8_decode (uint32_t id, char *dest)
{
  return squoze_id_decode (32, id, 0, dest);
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

static inline int squoze_utf8_strlen (const char *s)
{
  int count;
  if (!s)
    { return 0; }
  for (count = 0; *s; s++)
    if ( (*s & 0xC0) != 0x80)
      { count++; }
  return count;
}

static inline int
squoze_utf8_len (const unsigned char first_byte)
{
  if      ( (first_byte & 0x80) == 0)
    { return 1; }
  else if ( (first_byte & 0xE0) == 0xC0)
    { return 2; }
  else if ( (first_byte & 0xF0) == 0xE0)
    { return 3; }
  else if ( (first_byte & 0xF8) == 0xF0)
    { return 4; }
  return 1;
}



/*  data structures and implementation for string interning, potentially
 *  with both ref-counting and pools of strings.
 */
#if SQUOZE_USE_INTERN
#include <stdarg.h>

struct _Sqz {
#if SQUOZE_REF_COUNTING
    int32_t       ref_count; // set to magic value for ROM strings?
                             // and store pointer in string data?
#endif
#if SQUOZE_STORE_LENGTH
    int32_t       length;
#endif
    sqz_id_t      hash;
    char          string[];
};


static inline uint64_t sqz_pool_encode     (SqzPool *pool, const char *utf8, size_t len, Sqz **interned_ref);

struct _SqzPool
{
  int32_t     ref_count;
  SqzPool    *fallback;
  Sqz       **hashtable;
  int         count;
  int         size;
  SqzPool    *next;
};

static SqzPool global_pool = {0, NULL, NULL, 0, 0, NULL};

static SqzPool *sqz_pools = NULL;

static int sqz_pool_find (SqzPool *pool, uint64_t hash, int length, const uint8_t *bytes)
{
  if (pool->size == 0)
    return -1;
  int pos = (hash/2) & (pool->size-1);
  if (!pool->hashtable[pos])
    return -1;
  while (pool->hashtable[pos]->hash != hash
#if SQUOZE_STORE_LENGTH
         || pool->hashtable[pos]->length != length
#endif
         || strcmp (pool->hashtable[pos]->string, (char*)bytes)
         )
  {
    pos++;
    pos &= (pool->size-1);
    if (!pool->hashtable[pos])
      return -1;
  }
  return pos;
}
static int sqz_pool_add_entry (SqzPool *pool, Sqz *str)
{
  if (pool->count + 1 >= pool->size / 2)
  {
     Sqz **old = pool->hashtable;
     int old_size = pool->size;
     if (old_size == 0)
       pool->size = SQUOZE_INITIAL_POOL_SIZE;
     else
       pool->size *= 2;
     pool->hashtable = (Sqz**)calloc (pool->size, sizeof (void*));
     if (old)
     {
       for (int i = 0; i < old_size; i++)
         if (old[i])
           sqz_pool_add_entry (pool, old[i]);
       free (old);
     }
  }
  pool->count++;

  int pos = (str->hash/2) & (pool->size-1);
  while (pool->hashtable[pos])
  {
    pos++;
    pos &= (pool->size-1);
  }
  pool->hashtable[pos]=str;
  return pos;
}

#if SQUOZE_REF_SANITY
static int sqz_pool_remove (SqzPool *pool, Sqz *squozed, int do_free)
{
  Sqz *str = squozed;
  int no = sqz_pool_find (pool, str->hash);
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
      Sqz *for_upgrade = pool->hashtable[i];
      sqz_pool_remove (pool, for_upgrade, 0);
      sqz_pool_add_entry (pool, for_upgrade);
      break;
    }
  }
  return 1;
}
#endif

static Sqz *sqz_lookup (SqzPool *pool, sqz_id_t id, int length, const uint8_t *bytes)
{
  int pos = sqz_pool_find (pool, id, length, bytes);
  if (pos >= 0)
    return pool->hashtable[pos];
  if (pool->fallback)
    return sqz_lookup (pool->fallback, id, length, bytes);
  return NULL;
}

void sqz_pool_mem_stats (SqzPool *pool,
                         size_t     *size,
                            size_t     *slack,
                            size_t     *intern_alloc)
{
  if (!pool) pool = &global_pool;
  if (size)
  {
    *size = sizeof (SqzPool) + pool->size * sizeof (void*);
  }
  if (slack)
  {
    *slack = (pool->size - pool->count) * sizeof (void*);
  }

  if (intern_alloc)
  {
    size_t sum = 0;
    for (int i = 0; i < pool->size; i++)
    {
      if (pool->hashtable[i])
      {
        Sqz *squoze = pool->hashtable[i];
        sum += strlen (squoze->string) + 1 + sizeof (Sqz);
      }
    }
    *intern_alloc = sum;
  }
}

 // we do 32bit also for 64bit - we want the same predetermined hashes to match
static inline Sqz *_sqz_pool_add (SqzPool *pool, const char *str)
{
  if (!pool) pool = &global_pool;
  Sqz *interned = NULL;
  uint64_t hash = sqz_pool_encode (pool, str, strlen (str), &interned);

  if (interned)
  {
#ifdef assert
    assert ((((size_t)interned)&0x1)==0);
#endif
#if SQUOZE_DIRECT_STRING
    return interned+1;
#else
    return interned;
#endif
  }
  else
    return (Sqz*)((size_t)hash);
}

Sqz *sqz_pool_add (SqzPool *pool, const char *str)
{
  return _sqz_pool_add (pool, str);
}

Sqz *sqz     (const char *str)
{
  return _sqz_pool_add (NULL, str);
}

// encodes utf8 to a squoze id of squoze_dim bits - if interned_ret is provided overflowed ids
// are interned and a new interned squoze is returned.
static uint64_t sqz_pool_encode (SqzPool *pool, const char *utf8, size_t len, Sqz **interned_ref)
{
#if   SQUOZE_ID_BITS==32 && SQUOZE_ID_MURMUR
   uint64_t hash = MurmurOAAT32(utf8, len) & ~1;
#elif  SQUOZE_ID_BITS==32 && SQUOZE_ID_UTF5
   uint64_t hash = squoze32_utf5 (utf8, len);
#elif SQUOZE_ID_BITS==32 && SQUOZE_ID_UTF8
   uint64_t hash = squoze32_utf8 (utf8, len);
#elif SQUOZE_ID_BITS==62 && SQUOZE_ID_UTF5
   uint64_t hash = squoze62_utf5 (utf8, len);
#elif SQUOZE_ID_BITS==52 && SQUOZE_ID_UTF5
   uint64_t hash = squoze52_utf5 (utf8, len);
#elif SQUOZE_ID_BITS==64 && SQUOZE_ID_UTF8
   uint64_t hash = squoze64_utf8 (utf8, len);
#else
   uint64_t hash = squoze_encode_id (SQUOZE_ID_BITS, SQUOZE_ID_UTF5, utf8, len);
#endif

  if (!interned_ref)
    return hash;
  if (pool == NULL) pool = &global_pool;
  if ((hash & 1)==0)
  {
    Sqz *str = sqz_lookup (pool, hash, len, (const uint8_t*)utf8);
    if (str)
    {
#if SQUOZE_REF_COUNTING
      str->ref_count++;
#endif
      if (interned_ref) *interned_ref = str + SQUOZE_INTERN_DIRECT_STRING;
      return hash; 
    }

    {
      Sqz *entry = (Sqz*)calloc (len + 1 + sizeof(Sqz), 1);
      entry->hash = hash;
#if SQUOZE_STORE_LENGTH
      entry->length = len;
#endif
      strcpy (entry->string, utf8);
      if (interned_ref) *interned_ref = entry + SQUOZE_INTERN_DIRECT_STRING;
      sqz_pool_add_entry (pool, entry);
    }
  }
  return hash;
}

static inline int sqz_is_interned (Sqz *squozed)
{
  return ((((size_t)(squozed))&1) == 0);
}

static inline int sqz_is_embedded (Sqz *squozed)
{
  return !sqz_is_interned (squozed);
}

/* returns either the string or temp with the decode
 * embedded string decoded
 */
const char *sqz_decode (Sqz *squozed, char *temp)
{
  if (!squozed) return NULL;
  if (sqz_is_embedded (squozed))
  {
#if   SQUOZE_ID_BITS==32 && SQUOZE_ID_UTF5
    return squoze32_utf5_decode ((size_t)squozed, temp);
#elif SQUOZE_ID_BITS==32 && SQUOZE_ID_UTF8
    return squoze32_utf8_decode ((size_t)squozed, temp);
#elif SQUOZE_ID_BITS==52 && SQUOZE_ID_UTF5
    return squoze52_utf5_decode ((size_t)squozed, temp);
#elif SQUOZE_ID_BITS==62 && SQUOZE_ID_UTF5
    return squoze62_utf5_decode ((size_t)squozed, temp);
#elif SQUOZE_ID_BITS==64 && SQUOZE_ID_UTF8
    return squoze64_utf8_decode ((size_t)squozed, temp);
#else
    return squoze_id_decode (SQUOZE_ID_BITS,
                             ((size_t)squozed),
                             SQUOZE_ID_UTF5,
                             temp);
#endif
  }
  else
  {
#if SQUOZE_INTERN_DIRECT_STRING
    return (char*)squozed;
#else
    return squozed->string;
#endif
  }
}

Sqz *sqz_ref (Sqz *squozed)
{
#if SQUOZE_REF_COUNTING
  if (sqz_is_interned (squozed))
  {
     (squozed-SQUOZE_INTERN_DIRECT_STRING)->ref_count ++;
  }
#endif
  return squozed;
}

void sqz_unref (Sqz *squozed)
{
#if SQUOZE_REF_COUNTING
  if (sqz_is_interned (squozed))
  {
#if SQUOZE_INTERN_DIRECT_STRING
      squozed--;
#endif
      if (squozed->ref_count <= 0)
      {
#if SQUOZE_CLOBBER_ON_FREE
        squozed->string[-squozed->ref_count]='#';
#endif
#if SQUOZE_REF_SANITY
        if (squozed->ref_count < 0)
          fprintf (stderr, "double unref for \"%s\"\n", squozed->string);
        squozed->ref_count--;
#else
        SqzPool *pool = &global_pool;
        if (sqz_pool_remove (pool, squozed, 1))
        {
          return;
        }
        pool = sqz_pools;
        if (pool)
        do {
          if (sqz_pool_remove (pool, squozed, 1))
          {
            return;
          }
          pool = pool->next;
        } while (pool);
#endif
      }
      else
      {
        squozed->ref_count--;
      }
  }
#endif
}

int sqz_has_prefix (Sqz *a, Sqz *prefix)
{
  char tmp_a[16];
  char tmp_prefix[16];
  const char *a_str = sqz_decode (a, tmp_a);
  const char *prefix_str = sqz_decode (prefix, tmp_prefix);
  return !strncmp (a_str, prefix_str, strlen (prefix_str));
}

int sqz_has_prefix_utf8 (Sqz *a, const char *utf8)
{
  Sqz *b = sqz (utf8);
  int ret = sqz_has_prefix (a, b);
  sqz_unref (b);
  return ret;
}

int sqz_has_suffix_utf8 (Sqz *a, const char *utf8)
{
  Sqz *b = sqz (utf8);
  int ret = sqz_has_suffix (a, b);
  sqz_unref (b);
  return ret;
}

int sqz_has_suffix (Sqz *a, Sqz *suffix)
{
  char        tmp_a[16];
  const char *a_str = sqz_decode (a, tmp_a);
  int         a_len = strlen (a_str);
  char        tmp_suffix[16];
  const char *suffix_str = sqz_decode (suffix, tmp_suffix);
  int         suffix_len = strlen (suffix_str);
  
  if (a_len < suffix_len)
    return 0;
  return strcmp (a_str + a_len - suffix_len, suffix_str);
}


static void _sqz_prepend (Sqz **squoze, Sqz *head)
{
  if (!squoze) return;
  Sqz *combined = sqz_cat (head, *squoze);
  sqz_unref (*squoze);
  *squoze=combined;
}

static void _sqz_append (Sqz **squoze, Sqz *tail)
{
  if (!squoze) return;
  Sqz *combined = sqz_cat (*squoze, tail);
  sqz_unref (*squoze);
  *squoze=combined;
}

Sqz *sqz_substring (Sqz *a, int pos, int length)
{
  int src_length = sqz_length (a);
  if (pos > src_length)
    return sqz ("");
  if (pos < 0)
    pos = src_length + pos + 1;
  char tmp[16];
  const char *src = sqz_decode (a, tmp);
  char *end;
  int allocated = 0;

  char *copy;
  if (src_length < 256)
  {
    copy = alloca (strlen (src) + 1);
    strcpy (copy, src);
  }
  else
  {
    copy  = strdup (src);
    allocated = 1;
  }
  char *p = copy;
  int i;
  for (i = 0; i < pos; i++)
    p += squoze_utf8_len (*p);
  end = p;
  for (i = 0; i < length && *end; i++)
    end += squoze_utf8_len (*end);
  *end = 0;

  Sqz *ret = sqz (p);
  if (allocated)
    free (copy);
  return ret;
}

void sqz_erase (Sqz **a, int pos, int length)
{
  if (!a) return;
  if (!*a) return;

  if (length < 1)
    return;
  if (pos < 0)
  {
    pos = sqz_length (*a) + pos + 1;
  }

  Sqz *pre  = sqz_substring (*a, 0, pos);
  Sqz *post = sqz_substring (*a, pos+length, 10000);
  sqz_unref (*a);
  *a = sqz_cat (pre, post);
  sqz_unref (pre);
  sqz_unref (post);
}

void sqz_insert (Sqz **a, int pos, Sqz *b)
{
  if (pos == 0)
  {
    _sqz_prepend (a, b);
    return;
  }
  if (pos == -1)
  {
    _sqz_append (a, b);
    return;
  }
  if (!a) return;
  if (!*a) return;
  if (pos < 0)
  {
    pos = sqz_length (*a) + pos + 1;
  }
  Sqz *pre  = sqz_substring (*a, 0, pos);
  Sqz *post = sqz_substring (*a, pos, 10000);
  sqz_unref (*a);

  *a = sqz_cat (pre, b);
  _sqz_append (a, post);
  sqz_unref (pre);
  sqz_unref (post);
}

void sqz_insert_utf8 (Sqz **a, int pos, const char *utf8)
{
  Sqz *b = sqz (utf8);
  sqz_insert (a, pos, b);
  sqz_unref (b);
}

Sqz *sqz_unichar (uint32_t unichar)
{
  char temp[5];
  temp[squoze_unichar_to_utf8 (unichar, (uint8_t*)temp)]=0;
  return sqz (temp);
}

void sqz_insert_unichar (Sqz **a, int pos, uint32_t unichar)
{
  Sqz *b = sqz_unichar (unichar);
  sqz_insert (a, pos, b);
  sqz_unref (b);
}

uint32_t sqz_unichar_at (Sqz *a, int pos)
{
  char tmp[16];
  const char *str = sqz_decode (a, tmp);
  const char *p = str;
  int i;
  if (pos < 0)
  {
    pos = sqz_length (a) + pos;
  }
  for (i = 0; i < pos; i++)
    p += squoze_utf8_len (*p);
  return squoze_utf8_to_unichar (p);
}

void sqz_replace (Sqz **a, int pos, int length, Sqz *b)
{
  sqz_erase (a, pos, length);
  sqz_insert (a, pos, b);
}

void sqz_replace_unichar  (Sqz **a, int pos, int length, uint32_t unichar)
{
  Sqz *b = sqz_unichar (unichar);
  sqz_erase (a, pos, length);
  sqz_insert (a, pos, b);
  sqz_unref (b);
}

void sqz_replace_utf8  (Sqz **a, int pos, int length, const char *utf8)
{
  sqz_erase (a, pos, length);
  sqz_insert_utf8 (a, pos, utf8);
}

#define SQZ_EXPAND_PRINTF \
  va_list ap; \
  size_t needed; \
  char *buffer; \
  va_start (ap, format); \
  needed = vsnprintf (NULL, 0, format, ap) + 1; \
  if (needed < 256) \
    buffer = alloca (needed);\
  else\
    buffer = malloc (needed);\
  va_end (ap);\
  va_start (ap, format);\
  vsnprintf (buffer, needed, format, ap);\
  va_end (ap);\
  Sqz *b = sqz (buffer);\
  if (needed >= 256)\
    free (buffer);

Sqz      *sqz_printf (const char *format, ...)
{
  SQZ_EXPAND_PRINTF;
  return b;
}

void sqz_insert_printf (Sqz **a, int pos, const char *format, ...)
{
  SQZ_EXPAND_PRINTF;
  sqz_insert (a, pos, b);
  sqz_unref (b);
}

void sqz_replace_printf (Sqz **a, int pos, int length, const char *format, ...)
{
  SQZ_EXPAND_PRINTF;
  sqz_replace (a, pos, length, b);
  sqz_unref (b);
}

int sqz_strcmp (Sqz *a, Sqz *b)
{
  if (a == b) return 0;
  char tmp_a[16];
  char tmp_b[16];
  return strcmp (sqz_decode (a, tmp_a), sqz_decode (b, tmp_b));
}

static void _sqz_steal (Sqz **a, Sqz *b)
{
  if (*a)
    sqz_unref (*a);
  *a = b;
}

void sqz_assign (Sqz **a, Sqz *b)
{
  if (*a)
    sqz_unref (*a);
  *a = sqz_ref (b);
}

void sqz_assign_utf8 (Sqz **a, const char *str)
{
  _sqz_steal (a, sqz (str));
}

void sqz_assign_printf (Sqz **a, const char *format, ...)
{
  SQZ_EXPAND_PRINTF;
  _sqz_steal (a, b);
}

sqz_id_t sqz_id (Sqz *squozed)
{
  if (!squozed) return 0;
  if (sqz_is_embedded (squozed))
    return ((size_t)(squozed));
  else
  {
#if SQUOZE_INTERN_DIRECT_STRING
    squozed--;
#endif
    return squozed->hash;
  }
}

int sqz_length (Sqz *squozed)
{
  char buf[15];
  if (!squozed) return 0;
  return squoze_utf8_strlen(sqz_decode (squozed, buf));
}

// XXX : not used - remove it, and be unicode native?
int sqz_byte_length (Sqz *squozed)
{
  char buf[15];
  if (!squozed) return 0;
#if 0
  return strlen(sqz_decode (squozed, buf));
#else
  if (sqz_is_embedded (squozed))
  {
    sqz_decode (squozed, buf);
    return strlen (buf);
  }
  else
  {
#if SQUOZE_INTERN_DIRECT_STRING
    squozed--;
#endif
#if SQUOZE_STORE_LENGTH
    return squozed->length;
#endif
    return strlen (squozed->string);
  }
#endif
  return 0;
}

Sqz *sqz_cat (Sqz *a, Sqz *b)
{
  char buf_a[16];
  char buf_b[16];
  const char *str_a = sqz_decode (a, buf_a);
  const char *str_b = sqz_decode (b, buf_b);
  int len_a = strlen (str_a);
  int len_b = strlen (str_b);
  if (len_a + len_b < 128)
  {
    char temp[128];
    temp[0]=0;
    strcpy (temp, str_a);
    if (str_b)
      strcpy (&temp[strlen(temp)], str_b);
    return sqz (temp);
  }
  else
  {
    char *temp = malloc (len_a + len_b + 1);
    temp[0]=0;
    strcpy (temp, str_a);
    if (str_b)
      strcpy (&temp[strlen(temp)], str_b);
    Sqz *ret = sqz (temp);
    free (temp);
    return ret;
  }
}


SqzPool *sqz_pool_new     (SqzPool *fallback)
{
  SqzPool *pool = (SqzPool*)calloc (sizeof (SqzPool), 1);
  pool->fallback = fallback;
  pool->next = sqz_pools;
  sqz_pools = pool;
  if (fallback)
    sqz_pool_ref (fallback);
  return pool;
}

void sqz_pool_ref (SqzPool *pool)
{
  if (!pool) return;
  pool->ref_count--;
}

static void sqz_pool_destroy (SqzPool *pool)
{
#if 0
    fprintf (stderr, "destorying pool: size:%i count:%i embedded:%i\n",
       pool->size, pool->count, pool->count_embedded);
#endif
    for (int i = 0; i < pool->size; i++)
    {
      if (pool->hashtable[i])
        free (pool->hashtable[i]);
      pool->hashtable[i] = 0;
    }
    if (pool->fallback)
      sqz_pool_unref (pool->fallback);

    if (pool == sqz_pools)
    {
       sqz_pools = pool->next;
    }
    else
    {
      SqzPool *prev = NULL;
      SqzPool *iter = sqz_pools;
      while (iter && iter != pool)
      {
         prev = iter;
         iter = iter->next;
      }
      if (prev) // XXX not needed
        prev->next = pool->next;
    }
    pool->size = 0;
    pool->count = 0;
    if (pool->hashtable)
      free (pool->hashtable);
    pool->hashtable = NULL;

    // XXX report non unreffed items based on config
}

void sqz_pool_unref (SqzPool *pool)
{
  if (!pool) return;
  if (pool->ref_count == 0)
  {
    sqz_pool_destroy (pool);
    free (pool);
  }
  else
  {
    pool->ref_count--;
  }
}

void
sqz_atexit (void)
{
  sqz_pool_destroy (&global_pool);
  // also destory other known pools
  // XXX : when debugging report leaked pools
}
#endif

// UTF5 implementation

#if SQUOZE_USE_UTF5

// extra value meaning in UTF5 mode
#define SQUOZE_ENTER_SQUEEZE    16

// value meanings in squeeze mode
#define SQUOZE_SPACE            0
#define SQUOZE_DEC_OFFSET_A     27
#define SQUOZE_INC_OFFSET_A     28
#define SQUOZE_DEC_OFFSET_B     29
#define SQUOZE_INC_OFFSET_B     30
#define SQUOZE_ENTER_UTF5       31


static inline uint32_t squoze_utf8_to_unichar (const char *input);
static inline int      squoze_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
static inline int      squoze_utf8_len        (const unsigned char first_byte);
static inline int      squoze_utf8_strlen     (const char *s);


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

static inline int squoze_needed_jump (uint32_t off, uint32_t unicha)
{
  int count = 0;
  int unichar = unicha;
  int offset = off;

  if (unichar == 32) // space is always in range
    return 0;

  /* TODO: replace this with direct computation of values instead of loop */
  while (unichar < offset)
  {
    offset -= SQUOZE_JUMP_STRIDE;
    count --;
  }
  if (count)
    return count;

  return (unichar - offset) / SQUOZE_JUMP_STRIDE;
}


static inline int
squoze_utf5_length (uint32_t unichar)
{
  if (unichar == 0)
    return 1;
#if SQUOZE_USE_BUILTIN_CLZ
  return __builtin_clz(unichar)/4+1;
#else
  int nibbles = 1;
  while (unichar)
  {
    nibbles ++;
    unichar /= 16;
  }
  return nibbles;
#endif
}

typedef struct EncodeUtf5 {
  int      is_utf5;
  int      offset;
  int      length;
  void    *write_data;
  uint32_t current;
} EncodeUtf5;

static inline int squoze_compute_cost_utf5 (int offset, int val, int utf5_length, int next_val, int next_utf5_length)
{
  int cost = 0; 
  cost += utf5_length;
  if (next_val)
  {
    cost += next_utf5_length;
  }
  return cost;
}

static inline int squoze_compute_cost_squeezed (int offset, int val, int needed_jump, int next_val, int next_utf5_length)
{
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

static inline void squoze5_encode (const char *input, int inlen,
                                   char *output, int *r_outlen,
                                   int   permit_squeezed,
                                   int   escape_endzero)
{
  int offset  = 97;//squoze_new_offset('a');
  int is_utf5 = 1;
  int len     = 0;

  int first_len;
  int next_val = squoze_utf8_to_unichar (&input[0]);
  int next_utf5_length = squoze_utf5_length (next_val);
  for (int i = 0; i < inlen; i+= first_len)
  {
    int val = next_val;
    int utf5_length = next_utf5_length;
    int needed_jump = squoze_needed_jump (offset, val);
    first_len = squoze_utf8_len (input[i]);
    if (i + first_len < inlen)
    {
      next_val = squoze_utf8_to_unichar (&input[i+first_len]);
      next_utf5_length = squoze_utf5_length (next_val);
    }

    if (is_utf5)
    {
      int change_cost    = squoze_compute_cost_squeezed (offset, val, needed_jump, next_val, next_utf5_length);
      int no_change_cost = squoze_compute_cost_utf5 (offset, val, utf5_length, next_val, next_utf5_length);
  
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
      int change_cost    = 1 + squoze_compute_cost_utf5 (offset, val, utf5_length, next_val, next_utf5_length);
      int no_change_cost = squoze_compute_cost_squeezed (offset, val, needed_jump, next_val, next_utf5_length);

      if (change_cost < no_change_cost)
      {
        output[len++] = SQUOZE_ENTER_UTF5;
        is_utf5 = 1;
      }
    }

    if (!is_utf5)
    {
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
      offset = squoze_new_offset (val);
      int quintet_no = 0;
      uint8_t temp[12]={0,};

      while (val)
      {
        int oval = val % 16;
        int hi = 16;
        if (val / 16)
          hi = 0;
        temp[quintet_no++] = oval + hi;
        val /= 16;
      }
      for (int i = 0; i < quintet_no; i++)
        output[len++] = temp[quintet_no-1-i];
    }
    else 
    {
      output[len++] = (val == ' ')?SQUOZE_SPACE:val-offset+1;
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

/* squoze_encode_int:
 * @input utf8 input data
 * @inlen length of @input in bytes
 * @maxlen maximum number of quintets to encode
 * @overflow pointer to int that gets set to 1 if we overflow
 * @permit_squeezed 
 *
 */
static inline size_t squoze5_encode_int (const char *input, int inlen,
                                         int maxlen, int *overflow,
                                         int escape_endzero)
{
  size_t ret  = 0;
  int offset  = 97;//squoze_new_offset('a');
  int is_utf5 = 1;
  int len     = 0;

  int start_utf5 = 1;
  int gotzero = 0;

#define ADD_QUINTET(q) \
  do { \
    if (len + inlen-i > maxlen) {\
      *overflow = 1;\
      return 0;\
    }\
    ret |= ((size_t)(q))<<(5*len++); gotzero = (q==0);\
  } while (0)

  int first_len;
  int next_val = squoze_utf8_to_unichar (&input[0]);
  int next_utf5_length = squoze_utf5_length (next_val);
  int i = 0;
  for (int i = 0; i < inlen; i+= first_len)
  {
    int val         = next_val;
    int utf5_length = squoze_utf5_length (val);
    int needed_jump = squoze_needed_jump (offset, val);
    first_len = squoze_utf8_len (input[i]);
    if (i + first_len < inlen)
    {
      next_val         = squoze_utf8_to_unichar (&input[i+first_len]);
      next_utf5_length = squoze_utf5_length (next_val);
    }
    else
    {
      next_val = 0;
      next_utf5_length = 0;
    }

    if (is_utf5)
    {
      int change_cost    = squoze_compute_cost_squeezed (offset, val, needed_jump, next_val, next_utf5_length);
      int no_change_cost = squoze_compute_cost_utf5 (offset, val, utf5_length, next_val, next_utf5_length);
  
      if (i != 0)          /* ignore cost of initial 'G' */
        change_cost += 1;

      if (change_cost <= no_change_cost)
      {
        if (i != 0)
        { 
          ADD_QUINTET(SQUOZE_ENTER_SQUEEZE);
        }
        else
          start_utf5 = 0;

        is_utf5 = 0;
      }
    }
    else
    {
      int change_cost    = 1 + squoze_compute_cost_utf5 (offset, val, utf5_length, next_val, next_utf5_length);
      int no_change_cost = squoze_compute_cost_squeezed (offset, val, needed_jump, next_val, next_utf5_length);

      if (change_cost < no_change_cost)
      {
        ADD_QUINTET(SQUOZE_ENTER_UTF5);
        is_utf5 = 1;
      }
    }

    if (!is_utf5)
    {
      if (needed_jump)
      {
        if (needed_jump >= -2 && needed_jump <= 2)
        {
          switch (needed_jump)
          {
            case -1: ADD_QUINTET(SQUOZE_DEC_OFFSET_B); break;
            case  1: ADD_QUINTET(SQUOZE_INC_OFFSET_B); break;
            case -2: ADD_QUINTET(SQUOZE_DEC_OFFSET_A); break;
            case  2: ADD_QUINTET(SQUOZE_INC_OFFSET_A); break;
          }
          offset += SQUOZE_JUMP_STRIDE * needed_jump;
        }
        else if (needed_jump >= -10 && needed_jump <= 10) {
              int encoded_val;
              if (needed_jump < -2)
                encoded_val = 5 - needed_jump;
              else
                encoded_val = needed_jump - 3;

              ADD_QUINTET ((encoded_val/4) + SQUOZE_DEC_OFFSET_A);
              ADD_QUINTET ((encoded_val%4) + SQUOZE_DEC_OFFSET_A);

              offset += SQUOZE_JUMP_STRIDE * needed_jump;
        }
        else
        {
#ifdef assert
          assert(0); // should not be reached
#endif
          ADD_QUINTET (SQUOZE_ENTER_UTF5);
          is_utf5 = 1;
        }
      }
    }

    if (is_utf5)
    {
      offset = squoze_new_offset (val);
      int quintet_no = 0;
      uint8_t temp[12]={0,};

      while (val)
      {
        temp[quintet_no++] = (val&0xf) + (val/16)?0:16;
        val /= 16;
      }
      for (int j = 0; j < quintet_no; j++)
        ADD_QUINTET(temp[quintet_no-1-j]);
    }
    else 
    {
      ADD_QUINTET((val == ' ')?SQUOZE_SPACE:val-offset+1);
    }
  }

#if 1
  if (escape_endzero && len && gotzero)
  {
    // do a mode-change after 0 to avoid 0 being interpreted
    // as end of quintets
    ADD_QUINTET(is_utf5?16:SQUOZE_ENTER_UTF5);
  }
#endif

#undef ADD_QUINTET

  return (ret<<2) | ((start_utf5*2)|1);
}

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
                     97,//squoze_new_offset('a'),
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
#endif

#endif
