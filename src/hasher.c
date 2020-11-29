#include "ctx-split.h"

#if CTX_RASTERIZER

struct _CtxHasher
{
  CtxRasterizer rasterizer;
  int           cols;
  int           rows;
  uint8_t      *hashes;
};

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
#include <inttypes.h>

typedef struct sha1_state {
    uint64_t length;
    uint32_t state[5], curlen;
    unsigned char buf[64];
} sha1_state;
int sha1_init(sha1_state * sha1);
int sha1_process(sha1_state *sha1, const unsigned char * msg, unsigned long len);
int sha1_done(sha1_state * sha1, unsigned char *out);

#ifdef FF0
#undef FF0
#endif
#ifdef FF1
#undef FF1
#endif

#ifdef SHA1_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>

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
int func_name (sha1_state *sha1, const unsigned char *in, unsigned long inlen)      \
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

static int  sha1_compress(sha1_state *sha1, unsigned char *buf)
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
    #define FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
    #define FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
    #define FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
    #define FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);
 
    for (i = 0; i < 20; ) {
       FF0(a,b,c,d,e,i++);
       FF0(e,a,b,c,d,i++);
       FF0(d,e,a,b,c,i++);
       FF0(c,d,e,a,b,i++);
       FF0(b,c,d,e,a,i++);
    }

    /* round two */
    for (; i < 40; )  { 
       FF1(a,b,c,d,e,i++);
       FF1(e,a,b,c,d,i++);
       FF1(d,e,a,b,c,i++);
       FF1(c,d,e,a,b,i++);
       FF1(b,c,d,e,a,i++);
    }

    /* round three */
    for (; i < 60; )  { 
       FF2(a,b,c,d,e,i++);
       FF2(e,a,b,c,d,i++);
       FF2(d,e,a,b,c,i++);
       FF2(c,d,e,a,b,i++);
       FF2(b,c,d,e,a,i++);
    }

    /* round four */
    for (; i < 80; )  { 
       FF3(a,b,c,d,e,i++);
       FF3(e,a,b,c,d,i++);
       FF3(d,e,a,b,c,i++);
       FF3(c,d,e,a,b,i++);
       FF3(b,c,d,e,a,i++);
    }

    #undef FF0
    #undef FF1
    #undef FF2
    #undef FF3

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
int sha1_init(sha1_state * sha1)
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
HASH_PROCESS(sha1_process, sha1_compress, sha1, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (20 bytes)
   @return CRYPT_OK if successful
*/
int sha1_done(sha1_state * sha1, unsigned char *out)
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
        sha1_compress(sha1, sha1->buf);
        sha1->curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (sha1->curlen < 56) {
        sha1->buf[sha1->curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(sha1->length, sha1->buf+56);
    sha1_compress(sha1, sha1->buf);

    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32H(sha1->state[i], out+(4*i));
    }
    return CRYPT_OK;
}
#endif

#endif

static int
ctx_rect_intersect (const CtxRectangle *a, const CtxRectangle *b)
{
  if (a->x >= b->x + b->width ||
      b->x >= a->x + a->width ||
      a->y >= b->y + b->height ||
      b->y >= a->y + a->height) return 0;

  return 1;
}

static void
_ctx_add_hash (CtxHasher *hasher, CtxRectangle *shape_rect, char *hash)
{
  CtxRectangle rect = {0,0, hasher->rasterizer.blit_width/hasher->cols,
                            hasher->rasterizer.blit_height/hasher->rows};
  int hno = 0;
  for (int row = 0; row < hasher->rows; row++)
    for (int col = 0; col < hasher->cols; col++, hno++)
     {
      rect.x = col * rect.width;
      rect.y = row * rect.height;
      if (ctx_rect_intersect (shape_rect, &rect))
      {
        int temp = hasher->hashes[(row * hasher->cols + col)  *20 + 0];
        for (int i = 0; i <19;i++)
           hasher->hashes[(row * hasher->cols + col)  *20 + i] =
             hasher->hashes[(row * hasher->cols + col)  *20 + i+1]^
             hash[i];
        hasher->hashes[(row * hasher->cols + col)  *20 + 19] =
                temp ^ hash[19];
      }
    }
}


static void
ctx_hasher_process (void *user_data, CtxCommand *command)
{
  CtxEntry *entry = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) user_data;
  CtxHasher *hasher = (CtxHasher*) user_data;
  CtxState *state = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  int aa = rasterizer->aa;
  switch (c->code)
    {
      case CTX_TEXT:
        {
          sha1_state sha1;
          sha1_init (&sha1);
          char sha1_hash[20];
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());


          float height = ctx_get_font_size (rasterizer->ctx);
           CtxRectangle shape_rect;
          
           shape_rect.x=rasterizer->x;
           shape_rect.y=rasterizer->y - height,
           shape_rect.width = width;
           shape_rect.height = height * 2;
          switch ((int)ctx_state_get (rasterizer->state, CTX_text_align))
          {
          case CTX_TEXT_ALIGN_END:
           shape_rect.x -= shape_rect.width;
           break;
          case CTX_TEXT_ALIGN_CENTER:
           shape_rect.x -= shape_rect.width/2;
           break;
                   // XXX : doesn't take all text-alignments into account
          }

          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color, (uint8_t*)(&color));
          sha1_process(&sha1, (const unsigned char*)ctx_arg_string(), strlen  (ctx_arg_string()));
          sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          sha1_process(&sha1, (unsigned char*)&color, 4);
          sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxRectangle));
          sha1_done(&sha1, (unsigned char*)sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_TEXT_STROKE:
        {
          sha1_state sha1;
          sha1_init (&sha1);
          char sha1_hash[20];
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());
          float height = ctx_get_font_size (rasterizer->ctx);

           CtxRectangle shape_rect = {
              rasterizer->x, rasterizer->y - height,
              width, height * 2
           };

          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color, (uint8_t*)(&color));
          sha1_process(&sha1, (unsigned char*)ctx_arg_string(), strlen  (ctx_arg_string()));
          sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          sha1_process(&sha1, (unsigned char*)&color, 4);
          sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxRectangle));
          sha1_done(&sha1, (unsigned char*)sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_GLYPH:
         {
          sha1_state sha1;
          sha1_init (&sha1);

          char sha1_hash[20];
          uint8_t string[8];
          string[ctx_unichar_to_utf8 (c->u32.a0, string)]=0;
          float width = ctx_text_width (rasterizer->ctx, (char*)string);
          float height = ctx_get_font_size (rasterizer->ctx);

          float tx = rasterizer->x;
          float ty = rasterizer->y;
          float tw = width;
          float th = height * 2;

          _ctx_user_to_device (rasterizer->state, &tx, &ty);
          _ctx_user_to_device_distance (rasterizer->state, &tw, &th);
          CtxRectangle shape_rect = {tx,ty-th/2,tw,th};


          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color, (uint8_t*)(&color));
          sha1_process(&sha1, string, strlen ((const char*)string));
          sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          sha1_process(&sha1, (unsigned char*)&color, 4);
          sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxRectangle));
          sha1_done(&sha1, (unsigned char*)sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
          ctx_rasterizer_reset (rasterizer);
         }
        break;
      case CTX_FILL:
        {
          sha1_state sha1;
          sha1_init (&sha1);
          char sha1_hash[20];

          /* we eant this hasher to be as good as possible internally,
           * since it is also used in the small shapes rasterization
           * cache
           */
        uint64_t hash = ctx_rasterizer_poly_to_hash (rasterizer);
#if 0
        CtxRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV,
          rasterizer->scan_min / aa,

          (rasterizer->col_max - rasterizer->col_min) / CTX_SUBDIV,

          (rasterizer->scan_max + aa-1)/ aa -
          rasterizer->scan_min / aa
        };
#else
        CtxRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV,
          rasterizer->scan_min / aa,
          (rasterizer->col_max - rasterizer->col_min + 1) / CTX_SUBDIV,
          (rasterizer->scan_max - rasterizer->scan_min + 1) / aa
        };
#endif

        hash ^= (rasterizer->state->gstate.fill_rule * 23);
        hash ^= (rasterizer->state->gstate.source.type * 117);

        sha1_process(&sha1, (unsigned char*)&hash, 8);

        uint32_t color;
        ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color, (uint8_t*)(&color));

          sha1_process(&sha1, (unsigned char*)&color, 4);
          sha1_done(&sha1, (unsigned char*)sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, sha1_hash);

        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;
        }
        break;
      case CTX_STROKE:
        {
          sha1_state sha1;
          sha1_init (&sha1);
          char sha1_hash[20];
        uint64_t hash = ctx_rasterizer_poly_to_hash (rasterizer);
        CtxRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV,
          rasterizer->scan_min / aa,

          (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV -
          rasterizer->col_min / CTX_SUBDIV,

          (rasterizer->scan_max + aa-1)/ aa -
          rasterizer->scan_min / aa
        };

        shape_rect.width += rasterizer->state->gstate.line_width * 2;
        shape_rect.height += rasterizer->state->gstate.line_width * 2;
        shape_rect.x -= rasterizer->state->gstate.line_width;
        shape_rect.y -= rasterizer->state->gstate.line_width;

        hash ^= (int)(rasterizer->state->gstate.line_width * 110);
        hash ^= (rasterizer->state->gstate.line_cap * 23);
        hash ^= (rasterizer->state->gstate.source.type * 117);

        sha1_process(&sha1, (unsigned char*)&hash, 8);

        uint32_t color;
        ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source.color, (uint8_t*)(&color));

          sha1_process(&sha1, (unsigned char*)&color, 4);

          sha1_done(&sha1, (unsigned char*)sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, sha1_hash);
        }
        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;
        break;
        /* the above cases are the painting cases and 
         * the only ones differing from the rasterizer's process switch
         */

      case CTX_LINE_TO:
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_LINE_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_MOVE_TO:
        ctx_rasterizer_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rasterizer_rel_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_CURVE_TO:
        ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
                                 c->c.x1, c->c.y1,
                                 c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
                                     c->c.x1, c->c.y1,
                                     c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_ARC:
        ctx_rasterizer_arc (rasterizer, c->arc.x, c->arc.y, c->arc.radius, c->arc.angle1, c->arc.angle2, c->arc.direction);
        break;
      case CTX_RECTANGLE:
        ctx_rasterizer_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                  c->rectangle.width, c->rectangle.height);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_rasterizer_round_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                        c->rectangle.width, c->rectangle.height,
                                        c->rectangle.radius);
        break;
      case CTX_SET_PIXEL:
        ctx_rasterizer_set_pixel (rasterizer, c->set_pixel.x, c->set_pixel.y,
                                  c->set_pixel.rgba[0],
                                  c->set_pixel.rgba[1],
                                  c->set_pixel.rgba[2],
                                  c->set_pixel.rgba[3]);
        break;
      case CTX_TEXTURE:
#if 0
        ctx_rasterizer_set_texture (rasterizer, ctx_arg_u32 (0),
                                    ctx_arg_float (2), ctx_arg_float (3) );
#endif
        break;
#if 0
      case CTX_LOAD_IMAGE:
        ctx_rasterizer_load_image (rasterizer, ctx_arg_string(),
                                   ctx_arg_float (0), ctx_arg_float (1) );
        break;
#endif
#if CTX_GRADIENTS
      case CTX_GRADIENT_STOP:
        {
          float rgba[4]= {ctx_u8_to_float (ctx_arg_u8 (4) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+1) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+2) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+3) )
                         };
          ctx_rasterizer_gradient_add_stop (rasterizer,
                                            ctx_arg_float (0), rgba);
        }
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_state_gradient_clear_stops (rasterizer->state);
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_state_gradient_clear_stops (rasterizer->state);
        break;
#endif
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_SAVE:
      case CTX_RESTORE:
        rasterizer->uses_transforms = 1;
        ctx_interpret_transforms (rasterizer->state, entry, NULL);

        
        break;
      case CTX_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_BEGIN_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLIP:
        // should perhaps modify a global state to include
        // in hash?
        ctx_rasterizer_clip (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_finish_shape (rasterizer);
        break;
    }
  ctx_interpret_pos_bare (rasterizer->state, entry, NULL);
  ctx_interpret_style (rasterizer->state, entry, NULL);
  if (command->code == CTX_LINE_WIDTH)
    {
      float x = state->gstate.line_width;
      /* normalize line width according to scaling factor
       */
      x = x * ctx_maxf (ctx_maxf (ctx_fabsf (state->gstate.transform.m[0][0]),
                                  ctx_fabsf (state->gstate.transform.m[0][1]) ),
                        ctx_maxf (ctx_fabsf (state->gstate.transform.m[1][0]),
                                  ctx_fabsf (state->gstate.transform.m[1][1]) ) );
      state->gstate.line_width = x;
    }
}


static CtxRasterizer *
ctx_hasher_init (CtxRasterizer *rasterizer, Ctx *ctx, CtxState *state, int width, int height, int cols, int rows)
{
  CtxHasher *hasher = (CtxHasher*)rasterizer;
  ctx_memset (rasterizer, 0, sizeof (CtxHasher) );
  rasterizer->vfuncs.process = ctx_hasher_process;
  rasterizer->vfuncs.free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  // XXX need own destructor to not leak ->hashes
  rasterizer->edge_list.flags |= CTX_RENDERSTREAM_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->ctx         = ctx;
  ctx_state_init (rasterizer->state);
  rasterizer->blit_x      = 0;
  rasterizer->blit_y      = 0;
  rasterizer->blit_width  = width;
  rasterizer->blit_height = height;
  rasterizer->state->gstate.clip_min_x  = 0;
  rasterizer->state->gstate.clip_min_y  = 0;
  rasterizer->state->gstate.clip_max_x  = width - 1;
  rasterizer->state->gstate.clip_max_y  = height - 1;
  rasterizer->scan_min    = 5000;
  rasterizer->scan_max    = -5000;
  rasterizer->aa          = 5;
  rasterizer->force_aa    = 0;

  hasher->rows = rows;
  hasher->cols = cols;

  hasher->hashes = (uint8_t*)ctx_calloc (20, rows * cols);

  return rasterizer;
}

Ctx *ctx_hasher_new (int width, int height, int cols, int rows)
{
  Ctx *ctx = ctx_new ();
  CtxState    *state    = &ctx->state;
  CtxRasterizer *rasterizer = (CtxRasterizer *) ctx_calloc (sizeof (CtxHasher), 1);
  ctx_hasher_init (rasterizer, ctx, state, width, height, cols, rows);
  ctx_set_renderer (ctx, (void*)rasterizer);
  return ctx;
}
uint8_t *ctx_hasher_get_hash (Ctx *ctx, int col, int row)
{
  CtxHasher *hasher = (CtxHasher*)ctx->renderer;
  if (row < 0) row =0;
  if (col < 0) col =0;
  if (row >= hasher->rows) row = hasher->rows-1;
  if (col >= hasher->cols) col = hasher->cols-1;

  return &hasher->hashes[(row*hasher->cols+col)*20];
}

#endif
