#include "ctx-split.h"

#if CTX_IMPLEMENTATION

#define SHA1_IMPLEMENTATION
/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 *
 * The plain ANSIC sha1 functionality has been extracted from libtomcrypt,
 * and is included directly in the sources. /Øyvind K. - since libtomcrypt
 * is public domain the adaptations done here to make the sha1 self contained
 * also is public domain.
 */
#ifndef __SHA1_H
#define __SHA1_H
#if !__COSMOPOLITAN__
#include <inttypes.h>
#endif


int ctx_sha1_init(CtxSHA1 * sha1);
CtxSHA1 *ctx_sha1_new (void)
{
  CtxSHA1 *state = (CtxSHA1*)calloc (sizeof (CtxSHA1), 1);
  ctx_sha1_init (state);
  return state;
}
void ctx_sha1_free (CtxSHA1 *sha1)
{
  free (sha1);
}

#if 0
          CtxSHA1 sha1;
          ctx_sha1_init (&sha1);
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
#endif

#ifdef SHA1_FF0
#undef SHA1_FF0
#endif
#ifdef SHA1_FF1
#undef SHA1_FF1
#endif

#ifdef SHA1_IMPLEMENTATION
#if !__COSMOPOLITAN__
#include <stdlib.h>
#include <string.h>
#endif

#define STORE64H(x,                                                             y)                                                                     \
   { (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned                char)(((x)>>48)&255);     \
     (y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned                char)(((x)>>32)&255);     \
     (y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned                char)(((x)>>16)&255);     \
     (y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); }

#define STORE32H(x,                                                             y)                                                                     \
     { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned              char)(((x)>>16)&255);   \
       (y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned               char)((x)&255); }

#define LOAD32H(x, y)                            \
     { x = ((unsigned long)((y)[0] & 255)<<24) | \
           ((unsigned long)((y)[1] & 255)<<16) | \
           ((unsigned long)((y)[2] & 255)<<8)  | \
           ((unsigned long)((y)[3] & 255)); }

/* rotates the hard way */
#define ROL(x, y)  ((((unsigned long)(x)<<(unsigned long)((y)&31)) | (((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)
#define ROLc(x, y) ROL(x,y)

#define CRYPT_OK     0
#define CRYPT_ERROR  1
#define CRYPT_NOP    2

#ifndef MAX
   #define MAX(x, y) ( ((x)>(y))?(x):(y) )
#endif
#ifndef MIN
   #define MIN(x, y) ( ((x)<(y))?(x):(y) )
#endif

/* a simple macro for making hash "process" functions */
#define HASH_PROCESS(func_name, compress_name, state_var, block_size)               \
int func_name (CtxSHA1 *sha1, const unsigned char *in, unsigned long inlen)      \
{                                                                                   \
    unsigned long n;                                                                \
    int           err;                                                              \
    assert (sha1 != NULL);                                                          \
    assert (in != NULL);                                                            \
    if (sha1->curlen > sizeof(sha1->buf)) {                                         \
       return -1;                                                                   \
    }                                                                               \
    while (inlen > 0) {                                                             \
        if (sha1->curlen == 0 && inlen >= block_size) {                             \
           if ((err = compress_name (sha1, (unsigned char *)in)) != CRYPT_OK) {     \
              return err;                                                           \
           }                                                                        \
           sha1->length += block_size * 8;                                          \
           in             += block_size;                                            \
           inlen          -= block_size;                                            \
        } else {                                                                    \
           n = MIN(inlen, (block_size - sha1->curlen));                             \
           memcpy(sha1->buf + sha1->curlen, in, (size_t)n);                         \
           sha1->curlen += n;                                                       \
           in             += n;                                                     \
           inlen          -= n;                                                     \
           if (sha1->curlen == block_size) {                                        \
              if ((err = compress_name (sha1, sha1->buf)) != CRYPT_OK) {            \
                 return err;                                                        \
              }                                                                     \
              sha1->length += 8*block_size;                                         \
              sha1->curlen = 0;                                                     \
           }                                                                        \
       }                                                                            \
    }                                                                               \
    return CRYPT_OK;                                                                \
}

/**********************/

#define F0(x,y,z)  (z ^ (x & (y ^ z)))
#define F1(x,y,z)  (x ^ y ^ z)
#define F2(x,y,z)  ((x & y) | (z & (x | y)))
#define F3(x,y,z)  (x ^ y ^ z)

static int  ctx_sha1_compress(CtxSHA1 *sha1, unsigned char *buf)
{
    uint32_t a,b,c,d,e,W[80],i;

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4*i));
    }

    /* copy state */
    a = sha1->state[0];
    b = sha1->state[1];
    c = sha1->state[2];
    d = sha1->state[3];
    e = sha1->state[4];

    /* expand it */
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1); 
    }

    /* compress */
    /* round one */
    #define SHA1_FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
    #define SHA1_FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
    #define SHA1_FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
    #define SHA1_FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);
 
    for (i = 0; i < 20; ) {
       SHA1_FF0(a,b,c,d,e,i++);
       SHA1_FF0(e,a,b,c,d,i++);
       SHA1_FF0(d,e,a,b,c,i++);
       SHA1_FF0(c,d,e,a,b,i++);
       SHA1_FF0(b,c,d,e,a,i++);
    }

    /* round two */
    for (; i < 40; )  { 
       SHA1_FF1(a,b,c,d,e,i++);
       SHA1_FF1(e,a,b,c,d,i++);
       SHA1_FF1(d,e,a,b,c,i++);
       SHA1_FF1(c,d,e,a,b,i++);
       SHA1_FF1(b,c,d,e,a,i++);
    }

    /* round three */
    for (; i < 60; )  { 
       SHA1_FF2(a,b,c,d,e,i++);
       SHA1_FF2(e,a,b,c,d,i++);
       SHA1_FF2(d,e,a,b,c,i++);
       SHA1_FF2(c,d,e,a,b,i++);
       SHA1_FF2(b,c,d,e,a,i++);
    }

    /* round four */
    for (; i < 80; )  { 
       SHA1_FF3(a,b,c,d,e,i++);
       SHA1_FF3(e,a,b,c,d,i++);
       SHA1_FF3(d,e,a,b,c,i++);
       SHA1_FF3(c,d,e,a,b,i++);
       SHA1_FF3(b,c,d,e,a,i++);
    }

    #undef SHA1_FF0
    #undef SHA1_FF1
    #undef SHA1_FF2
    #undef SHA1_FF3

    /* store */
    sha1->state[0] = sha1->state[0] + a;
    sha1->state[1] = sha1->state[1] + b;
    sha1->state[2] = sha1->state[2] + c;
    sha1->state[3] = sha1->state[3] + d;
    sha1->state[4] = sha1->state[4] + e;

    return CRYPT_OK;
}

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int ctx_sha1_init(CtxSHA1 * sha1)
{
   assert(sha1 != NULL);
   sha1->state[0] = 0x67452301UL;
   sha1->state[1] = 0xefcdab89UL;
   sha1->state[2] = 0x98badcfeUL;
   sha1->state[3] = 0x10325476UL;
   sha1->state[4] = 0xc3d2e1f0UL;
   sha1->curlen = 0;
   sha1->length = 0;
   return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(ctx_sha1_process, ctx_sha1_compress, sha1, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (20 bytes)
   @return CRYPT_OK if successful
*/
int ctx_sha1_done(CtxSHA1 * sha1, unsigned char *out)
{
    int i;

    assert(sha1 != NULL);
    assert(out != NULL);

    if (sha1->curlen >= sizeof(sha1->buf)) {
       return -1;
    }

    /* increase the length of the message */
    sha1->length += sha1->curlen * 8;

    /* append the '1' bit */
    sha1->buf[sha1->curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (sha1->curlen > 56) {
        while (sha1->curlen < 64) {
            sha1->buf[sha1->curlen++] = (unsigned char)0;
        }
        ctx_sha1_compress(sha1, sha1->buf);
        sha1->curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (sha1->curlen < 56) {
        sha1->buf[sha1->curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(sha1->length, sha1->buf+56);
    ctx_sha1_compress(sha1, sha1->buf);

    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32H(sha1->state[i], out+(4*i));
    }
    return CRYPT_OK;
}
#endif

#endif
#endif
