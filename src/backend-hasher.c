#include "ctx-split.h"

#if CTX_RASTERIZER


static int
ctx_rect_intersect (const CtxIntRectangle *a, const CtxIntRectangle *b)
{
  if (a->x >= b->x + b->width ||
      b->x >= a->x + a->width ||
      a->y >= b->y + b->height ||
      b->y >= a->y + a->height) return 0;

  return 1;
}


static void
_ctx_add_hash (CtxHasher *hasher, CtxIntRectangle *shape_rect, uint32_t hash)
{
  CtxIntRectangle rect = {0,0, hasher->rasterizer.blit_width/hasher->cols,
                               hasher->rasterizer.blit_height/hasher->rows};
  uint32_t active = 0;
  int hno = 0;
  for (int row = 0; row < hasher->rows; row++)
    for (int col = 0; col < hasher->cols; col++, hno++)
     {
      rect.x = col * rect.width;
      rect.y = row * rect.height;
      if (ctx_rect_intersect (shape_rect, &rect))
      {
        hasher->hashes[(row * hasher->cols + col)] ^= hash;
        hasher->hashes[(row * hasher->cols + col)] += 11;
        active |= (1<<hno);
      }
    }

  if (hasher->prev_command>=0)
  {
    hasher->drawlist->entries[hasher->prev_command].data.u32[1] = active;
  }

  hasher->prev_command = hasher->pos;
}

static int
ctx_str_count_lines (const char *str)
{
  int count = 0;
  for (const char *p = str; *p; p++)
    if (*p == '\n') count ++;
  return count;
}

static inline uint32_t murmur_32_scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
}

static inline void murmur3_32_process(CtxMurmur *murmur, const uint8_t* key, size_t len)
{
    // code direct from the wikipedia article, it appears there without
    // a license
    uint32_t h = murmur->state[0];
    uint32_t k;
    /* Read in groups of 4. */
    for (size_t i = len >> 2; i; i--) {
        // Here is a source of differing results across endiannesses.
        // A swap here has no effects on hash properties though.
        memcpy(&k, key, sizeof(uint32_t));
        key += sizeof(uint32_t);
        h ^= murmur_32_scramble(k);
        h = (h << 13) | (h >> 19);
        h = h * 5 + 0xe6546b64;
    }
    /* Read the rest. */
    k = 0;
    for (size_t i = len & 3; i; i--) {
        k <<= 8;
        k |= key[i - 1];
    }
    // A swap is *not* necessary here because the preceding loop already
    // places the low bytes in the low places according to whatever endianness
    // we use. Swaps only apply when the memory is copied in a chunk.
    h ^= murmur_32_scramble(k);
    murmur->state[0] = h;
    murmur->state[1] += len;
}

static inline void murmur3_32_init (CtxMurmur *murmur)
{
  murmur->state[0]=0;
  murmur->state[1]=0;
}
static inline void murmur3_32_free (CtxMurmur *murmur)
{
  ctx_free (murmur);
}
static inline uint32_t murmur3_32_finalize (CtxMurmur *murmur)
{
  uint32_t h = murmur->state[0];
  /* Finalize. */
  h ^= murmur->state[1];
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

static inline int murmur3_32_done (CtxMurmur *murmur, unsigned char *out)
{
  murmur3_32_finalize (murmur);
  for (int i = 0; i < 4; i++)
    out[i]=0;
  memcpy (out, &murmur->state[0], 4);
  return murmur->state[0];
}

/*
 * the hasher should store a list of
 * times when the activeness of each tile changes
 *
 * on replay path and text/glyph commands as well
 * as stroke/fill can be ignored  clips outside
 * should mean no more drawing until restore
 */

static inline void
ctx_device_corners_to_user_rect (CtxState *state,
                                 float x0, float y0, float x1, float y1,
                                 CtxIntRectangle *shape_rect)
{
  int itw, ith;
  int itx = 0, ity = 0, itx2 = 0, ity2 = 0;
  _ctx_user_to_device_prepped (state, x0, y0, &itx, &ity);
  _ctx_user_to_device_prepped (state, x1, y1, &itx2, &ity2);
  itx /= CTX_SUBDIV;
  itx2 /= CTX_SUBDIV;
  ity /= CTX_FULL_AA;
  ity2 /= CTX_FULL_AA;
  itw = itx2-itx;
  ith = ity2-ity;
  shape_rect->x=itx;
  shape_rect->y=ity;
  shape_rect->width = itw;
  shape_rect->height = ith;
}

static void
ctx_hasher_process (Ctx *ctx, CtxCommand *command)
{
  CtxEntry      *entry      = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) ctx->backend;
  CtxHasher     *hasher     = (CtxHasher*) ctx->backend;
  CtxState      *state      = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  int aa = 15;//rasterizer->aa;

  ctx_interpret_pos_bare (rasterizer->state, entry, NULL);
  ctx_interpret_style (rasterizer->state, entry, NULL);

  switch (c->code)
    {
      case CTX_TEXT:
        {
          const char *str = ctx_arg_string();
          CtxMurmur murmur;
          memcpy (&murmur, &hasher->murmur_fill[hasher->source_level], sizeof (CtxMurmur));
          float width = ctx_text_width (rasterizer->backend.ctx, str);


          float height = ctx_get_font_size (rasterizer->backend.ctx);
           CtxIntRectangle shape_rect;

           float tx = rasterizer->x;
           float ty = rasterizer->y - height * 1.2f;
           float tx2 = tx+width;
           float ty2 = ty+height * (ctx_str_count_lines (str) + 1.5f);

           ctx_device_corners_to_user_rect (rasterizer->state, tx,ty,tx2,ty2, &shape_rect);
          switch ((int)ctx_state_get (rasterizer->state, CTX_text_align))
          {
          case CTX_TEXT_ALIGN_LEFT:
          case CTX_TEXT_ALIGN_START:
                  break;
          case CTX_TEXT_ALIGN_END:
          case CTX_TEXT_ALIGN_RIGHT:
           shape_rect.x -= shape_rect.width;
           break;
          case CTX_TEXT_ALIGN_CENTER:
           shape_rect.x -= shape_rect.width/2;
           break;
                   // XXX : doesn't take all text-alignments into account
          }

          murmur3_32_process(&murmur, (const unsigned char*)ctx_arg_string(), ctx_strlen  (ctx_arg_string()));
#if 1
        murmur3_32_process(&murmur, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
    //      murmur3_32_process(&murmur, (unsigned char*)&color, 4);
#endif
          murmur3_32_process(&murmur, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          _ctx_add_hash (hasher, &shape_rect, murmur3_32_finalize (&murmur));

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_STROKE_TEXT:
        {
          CtxMurmur murmur;
          const char *str = ctx_arg_string();
          memcpy (&murmur, &hasher->murmur_stroke[hasher->source_level], sizeof (CtxMurmur));
          float width = ctx_text_width (rasterizer->backend.ctx, str);
          float height = ctx_get_font_size (rasterizer->backend.ctx);

           CtxIntRectangle shape_rect;

           float tx = rasterizer->x;
           float ty = rasterizer->y - height * 1.2f;
           float tx2 = tx+width;
           float ty2 = ty+height * (ctx_str_count_lines (str) + 1.5f);
           ctx_device_corners_to_user_rect (rasterizer->state, tx,ty,tx2,ty2, &shape_rect);


#if 0
          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));
#endif
          murmur3_32_process(&murmur, (unsigned char*)ctx_arg_string(), ctx_strlen  (ctx_arg_string()));
#if 1
          murmur3_32_process(&murmur, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
    //    murmur3_32_process(&murmur, (unsigned char*)&color, 4);
#endif
          murmur3_32_process(&murmur, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          _ctx_add_hash (hasher, &shape_rect, murmur3_32_finalize (&murmur));

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_GLYPH:
         {
          CtxMurmur murmur;
          memcpy (&murmur, &hasher->murmur_fill[hasher->source_level], sizeof (CtxMurmur));

          uint8_t string[8];
          string[ctx_unichar_to_utf8 (c->u32.a0, string)]=0;
          float width = ctx_text_width (rasterizer->backend.ctx, (char*)string);
          float height = ctx_get_font_size (rasterizer->backend.ctx);

          float tx = rasterizer->x;
          float ty = rasterizer->y;
          float tx2 = rasterizer->x + width;
          float ty2 = rasterizer->y + height * 2;
          CtxIntRectangle shape_rect;
          ctx_device_corners_to_user_rect (rasterizer->state, tx,ty,tx2,ty2, &shape_rect);

          shape_rect.y-=shape_rect.height/2;


#if 0
          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
#endif
          murmur3_32_process(&murmur, string, ctx_strlen ((const char*)string));
          murmur3_32_process(&murmur, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
#if 0
          murmur3_32_process(&murmur, (unsigned char*)&color, 4);
#endif
          murmur3_32_process(&murmur, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          _ctx_add_hash (hasher, &shape_rect, murmur3_32_finalize (&murmur));

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
          ctx_rasterizer_reset (rasterizer);
         }
        break;

      case CTX_CLIP:
      case CTX_PAINT:
        {
        CtxMurmur murmur;
        memcpy (&murmur, &hasher->murmur_fill[hasher->source_level], sizeof (CtxMurmur));
        if (rasterizer->edge_list.count)
          murmur3_32_process(&murmur,  (uint8_t*)rasterizer->edge_list.entries, sizeof(CtxSegment) * rasterizer->edge_list.count);

        {
          int is = rasterizer->state->gstate.fill_rule;
          murmur3_32_process(&murmur, (uint8_t*)&is, sizeof(int));
        }
        CtxIntRectangle shape_rect = {-100,-100,
                rasterizer->blit_width*10,
                rasterizer->blit_height*10};
        _ctx_add_hash (hasher, &shape_rect, murmur3_32_finalize (&murmur));
        }

        break;
      case CTX_FILL:
        {
          CtxMurmur murmur;
          memcpy (&murmur, &hasher->murmur_fill[hasher->source_level], sizeof (CtxMurmur));

          /* we eant this hasher to be as good as possible internally,
           * since it is also used in the small shapes rasterization
           * cache
           */
        //uint64_t hash = ctx_rasterizer_poly_to_hash2 (rasterizer); // + hasher->salt;
        CtxIntRectangle shape_rect = {
          (int)(rasterizer->col_min / CTX_SUBDIV - 3),
          (int)(rasterizer->scan_min / aa - 3),
          (int)(5+(rasterizer->col_max - rasterizer->col_min + CTX_SUBDIV-1) / CTX_SUBDIV),
          (int)(5+(rasterizer->scan_max - rasterizer->scan_min + aa-1) / aa)
        };

        if (rasterizer->edge_list.count)
          murmur3_32_process(&murmur,  (uint8_t*)rasterizer->edge_list.entries, sizeof(CtxSegment) * rasterizer->edge_list.count);

        {
          int is = rasterizer->state->gstate.fill_rule;
          murmur3_32_process(&murmur, (uint8_t*)&is, sizeof(int));
        }
        {
          int is = rasterizer->state->gstate.image_smoothing;
          murmur3_32_process(&murmur, (uint8_t*)&is, sizeof(int));
        }
        {
          int e = rasterizer->state->gstate.extend;
          murmur3_32_process(&murmur, (uint8_t*)&e, sizeof(int));
        }

          _ctx_add_hash (hasher, &shape_rect, murmur3_32_finalize (&murmur));

        if (c->code == CTX_CLIP)
          ctx_rasterizer_clip (rasterizer);

        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;

        }
        break;
      case CTX_STROKE:
        {
          CtxMurmur murmur;
          memcpy (&murmur, &hasher->murmur_stroke[hasher->source_level], sizeof (CtxMurmur));
        if (rasterizer->edge_list.count)
        murmur3_32_process(&murmur,  (uint8_t*)rasterizer->edge_list.entries, sizeof(CtxSegment) * rasterizer->edge_list.count);
        CtxIntRectangle shape_rect = {
          (int)(rasterizer->col_min / CTX_SUBDIV - rasterizer->state->gstate.line_width),
          (int)(rasterizer->scan_min / aa - rasterizer->state->gstate.line_width),
          (int)((rasterizer->col_max - rasterizer->col_min + 1) / CTX_SUBDIV + rasterizer->state->gstate.line_width),
          (int)((rasterizer->scan_max - rasterizer->scan_min + 1) / aa + rasterizer->state->gstate.line_width)
        };

        shape_rect.width += rasterizer->state->gstate.line_width * 2;
        shape_rect.height += rasterizer->state->gstate.line_width * 2;
        shape_rect.x -= rasterizer->state->gstate.line_width;
        shape_rect.y -= rasterizer->state->gstate.line_width;

        {
          float f;
          int i;
          f = rasterizer->state->gstate.line_width;
          murmur3_32_process(&murmur, (uint8_t*)&f, sizeof(float));
          i = rasterizer->state->gstate.line_cap;
          murmur3_32_process(&murmur, (uint8_t*)&i, sizeof(int));
          i = rasterizer->state->gstate.line_join;
          murmur3_32_process(&murmur, (uint8_t*)&i, sizeof(int));
          i = rasterizer->state->gstate.source_stroke.type;
          murmur3_32_process(&murmur, (uint8_t*)&i, sizeof(int));
        }

        uint32_t color;
        ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));

          murmur3_32_process(&murmur, (unsigned char*)&color, 4);

          _ctx_add_hash (hasher, &shape_rect, murmur3_32_finalize (&murmur));
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
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        ctx_rasterizer_line_to (rasterizer, c->c.x1, c->c.y1);
        ctx_rasterizer_line_to (rasterizer, c->c.x2, c->c.y2);
        //ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
        //                         c->c.x1, c->c.y1,
        //                         c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x2, c->c.y2);
        //ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
        //                             c->c.x1, c->c.y1,
        //                             c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        ctx_rasterizer_line_to (rasterizer, c->c.x1, c->c.y1);
        //ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x1, c->c.y1);
        //ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
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
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_SAVE:
      case CTX_RESTORE:

        if (c->code == CTX_SAVE)
        {
           if (hasher->source_level + 1 < CTX_MAX_STATES)
           {
             hasher->source_level++;
             hasher->murmur_fill[hasher->source_level] =
               hasher->murmur_fill[hasher->source_level-1];
             hasher->murmur_stroke[hasher->source_level] =
               hasher->murmur_stroke[hasher->source_level-1];
           }
        }
        else
        {
           if (hasher->source_level - 1 >= 0)
           {
             hasher->source_level--;
             hasher->murmur_fill[hasher->source_level] =
               hasher->murmur_fill[hasher->source_level+1];
             hasher->murmur_stroke[hasher->source_level] =
               hasher->murmur_stroke[hasher->source_level+1];
           }
        }

        /* FALLTHROUGH */
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_APPLY_TRANSFORM:

        ctx_interpret_transforms (rasterizer->state, entry, NULL);
        break;
      case CTX_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_BEGIN_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_finish_shape (rasterizer);
        break;
      case CTX_DEFINE_TEXTURE:
        {
        murmur3_32_init (&hasher->murmur_fill[hasher->source_level]);
        murmur3_32_process(&hasher->murmur_fill[hasher->source_level], &rasterizer->state->gstate.global_alpha_u8, 1);
        murmur3_32_process (&hasher->murmur_fill[hasher->source_level], (uint8_t*)c->define_texture.eid, ctx_strlen (c->define_texture.eid));
        murmur3_32_process(&hasher->murmur_fill[hasher->source_level], (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));

        rasterizer->comp_op = NULL; // why?
        }
        break;
      case CTX_TEXTURE:
        murmur3_32_init (&hasher->murmur_fill[hasher->source_level]);
        murmur3_32_process(&hasher->murmur_fill[hasher->source_level], &rasterizer->state->gstate.global_alpha_u8, 1);
        murmur3_32_process (&hasher->murmur_fill[hasher->source_level], (uint8_t*)c->texture.eid, ctx_strlen (c->texture.eid));
        murmur3_32_process (&hasher->murmur_fill[hasher->source_level], (uint8_t*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
        rasterizer->comp_op = NULL; // why?
        break;
      case CTX_COLOR:
        {
          uint32_t color;
          if (((int)(ctx_arg_float(0))&512))
          {
            ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));
            murmur3_32_init (&hasher->murmur_stroke[hasher->source_level]);
            murmur3_32_process(&hasher->murmur_stroke[hasher->source_level], &rasterizer->state->gstate.global_alpha_u8, 1);
            murmur3_32_process(&hasher->murmur_stroke[hasher->source_level], (unsigned char*)&color, 4);
          }
          else
          {
            ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
            murmur3_32_init (&hasher->murmur_fill[hasher->source_level]);
            murmur3_32_process(&hasher->murmur_fill[hasher->source_level], &rasterizer->state->gstate.global_alpha_u8, 1);
            murmur3_32_process(&hasher->murmur_fill[hasher->source_level], (unsigned char*)&color, 4);
          }
        }
        break;
      case CTX_LINEAR_GRADIENT:
          murmur3_32_init (&hasher->murmur_fill[hasher->source_level]);
          murmur3_32_process(&hasher->murmur_fill[hasher->source_level], &rasterizer->state->gstate.global_alpha_u8, 1);
          murmur3_32_process(&hasher->murmur_fill[hasher->source_level], 
                           (uint8_t*)c, sizeof (c->linear_gradient));
          murmur3_32_process (&hasher->murmur_fill[hasher->source_level], (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
        break;
      case CTX_RADIAL_GRADIENT:
          murmur3_32_init (&hasher->murmur_fill[hasher->source_level]);
          murmur3_32_process(&hasher->murmur_fill[hasher->source_level], &rasterizer->state->gstate.global_alpha_u8, 1);
          murmur3_32_process(&hasher->murmur_fill[hasher->source_level], 
                           (uint8_t*)c, sizeof (c->radial_gradient));
          murmur3_32_process (&hasher->murmur_fill[hasher->source_level], (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
        //ctx_state_gradient_clear_stops (rasterizer->state);
        break;
#if CTX_GRADIENTS
      case CTX_GRADIENT_STOP:
        {
          float rgba[4]= {ctx_u8_to_float (ctx_arg_u8 (4) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+1) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+2) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+3) )
                         };
          murmur3_32_process(&hasher->murmur_fill[hasher->source_level], 
                           (uint8_t*) &rgba[0], sizeof(rgba));
        }
        break;
#endif
    }

#if 0
  if (command->code == CTX_START_FRAME)
  {
  }
#endif

    hasher->pos += ctx_conts_for_entry ((CtxEntry*)(command))+1;
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
ctx_hasher_init (CtxRasterizer *rasterizer, Ctx *ctx, CtxState *state, int width, int height, int cols, int rows, CtxDrawlist *drawlist)
{
  CtxHasher *hasher = (CtxHasher*)rasterizer;
  ctx_memset (rasterizer, 0, sizeof (CtxHasher) );
  CtxBackend *backend = (CtxBackend*)hasher;
  backend->ctx         = ctx;
  backend->process = ctx_hasher_process;
  backend->destroy = (CtxDestroyNotify)ctx_rasterizer_destroy;
  // XXX need own destructor to not leak ->hashes
  rasterizer->edge_list.flags |= CTX_DRAWLIST_EDGE_LIST;
  rasterizer->state       = state;
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
  //rasterizer->aa          = 15;

  hasher->rows = rows;
  hasher->cols = cols;
  hasher->pos  = 0;

  hasher->drawlist = drawlist;
  hasher->prev_command = -1;

  memset(hasher->hashes,0, sizeof (hasher->hashes));
  murmur3_32_init (&hasher->murmur_fill[hasher->source_level]);
  murmur3_32_init (&hasher->murmur_stroke[hasher->source_level]);

  return rasterizer;
}

Ctx *ctx_hasher_new (int width, int height, int cols, int rows, CtxDrawlist *drawlist)
{
  Ctx *ctx           = _ctx_new_drawlist (width, height);
  CtxState    *state = &ctx->state;
  CtxRasterizer *rasterizer = (CtxRasterizer *) ctx_calloc (sizeof (CtxHasher), 1);
  ctx_hasher_init (rasterizer, ctx, state, width, height, cols, rows, drawlist);
  ctx_set_backend (ctx, (void*)rasterizer);
  return ctx;
}

uint32_t ctx_hasher_get_hash (Ctx *ctx, int col, int row)
{
  CtxHasher *hasher = (CtxHasher*)ctx->backend;
  if (row < 0) row =0;
  if (col < 0) col =0;
  if (row >= hasher->rows) row = hasher->rows-1;
  if (col >= hasher->cols) col = hasher->cols-1;

  hasher->drawlist->entries[hasher->prev_command].data.u32[1] = 0xffffffff;

  return hasher->hashes[(row*hasher->cols+col)];
}

#endif
