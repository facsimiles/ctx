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
_ctx_add_hash (CtxHasher *hasher, CtxIntRectangle *shape_rect, char *hash)
{
  CtxIntRectangle rect = {0,0, hasher->rasterizer.blit_width/hasher->cols,
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
          CtxSHA1 sha1;
          ctx_sha1_init (&sha1);
          char ctx_sha1_hash[20];
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());


          float height = ctx_get_font_size (rasterizer->ctx);
           CtxIntRectangle shape_rect;
          
           shape_rect.x=rasterizer->x;
           shape_rect.y=rasterizer->y - height,
           shape_rect.width = width;
           shape_rect.height = height * 2;
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

          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
          ctx_sha1_process(&sha1, (const unsigned char*)ctx_arg_string(), strlen  (ctx_arg_string()));
          ctx_sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_STROKE_TEXT:
        {
          CtxSHA1 sha1;
          ctx_sha1_init (&sha1);
          char ctx_sha1_hash[20];
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());
          float height = ctx_get_font_size (rasterizer->ctx);

           CtxIntRectangle shape_rect = {
              rasterizer->x, rasterizer->y - height,
              width, height * 2
           };

          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));
          ctx_sha1_process(&sha1, (unsigned char*)ctx_arg_string(), strlen  (ctx_arg_string()));
          ctx_sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_GLYPH:
         {
          CtxSHA1 sha1;
          ctx_sha1_init (&sha1);

          char ctx_sha1_hash[20];
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
          CtxIntRectangle shape_rect = {tx,ty-th/2,tw,th};


          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
          ctx_sha1_process(&sha1, string, strlen ((const char*)string));
          ctx_sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
          ctx_rasterizer_reset (rasterizer);
         }
        break;
      case CTX_FILL:
        {
          CtxSHA1 sha1;
          ctx_sha1_init (&sha1);
          char ctx_sha1_hash[20];

          /* we eant this hasher to be as good as possible internally,
           * since it is also used in the small shapes rasterization
           * cache
           */
        uint64_t hash = ctx_rasterizer_poly_to_hash (rasterizer) + hasher->salt;
        CtxIntRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV,
          rasterizer->scan_min / aa,
          (rasterizer->col_max - rasterizer->col_min + 1) / CTX_SUBDIV,
          (rasterizer->scan_max - rasterizer->scan_min + 1) / aa
        };

        hash ^= (rasterizer->state->gstate.fill_rule * 23);
        hash ^= (rasterizer->state->gstate.source_fill.type * 117);

        ctx_sha1_process(&sha1, (unsigned char*)&hash, 8);

          int is = rasterizer->state->gstate.image_smoothing;
          ctx_sha1_process(&sha1, (uint8_t*)&is, sizeof(int));
        //
        //fprintf (stderr, "t:%i\n", rasterizer->state->gstate.source_fill.type == CTX_SOURCE_TEXTURE);
        //
        if (rasterizer->state->gstate.source_fill.type == CTX_SOURCE_TEXTURE)
        {
          ctx_sha1_process(&sha1, (uint8_t*)&rasterizer->state->gstate.source_fill, sizeof (CtxSource));//(unsigned char*)&color, 4);
          if (rasterizer->state->gstate.source_fill.texture.buffer->eid)
          {
            ctx_sha1_process(&sha1, (uint8_t*)rasterizer->state->gstate.source_fill.texture.buffer->eid, 
            strlen (rasterizer->state->gstate.source_fill.texture.buffer->eid));

            ctx_sha1_process(&sha1, (uint8_t *)&rasterizer->state->gstate.transform,
                          sizeof (CtxMatrix));


//          fprintf (stderr, "%s:%i:%s\n", __FILE__, __LINE__, rasterizer->state->gstate.source_fill.texture.buffer->eid);
          }
                          
        }
        else if (rasterizer->state->gstate.source_fill.type == CTX_SOURCE_COLOR)
        {
          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);
        }

          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;
        }
        break;
      case CTX_STROKE:
        {
          CtxSHA1 sha1;
          ctx_sha1_init (&sha1);
          char ctx_sha1_hash[20];
        uint64_t hash = ctx_rasterizer_poly_to_hash (rasterizer);
        CtxIntRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV - rasterizer->state->gstate.line_width,
          rasterizer->scan_min / aa - rasterizer->state->gstate.line_width,
          (rasterizer->col_max - rasterizer->col_min + 1) / CTX_SUBDIV + rasterizer->state->gstate.line_width,
          (rasterizer->scan_max - rasterizer->scan_min + 1) / aa + rasterizer->state->gstate.line_width
        };

        shape_rect.width += rasterizer->state->gstate.line_width * 2;
        shape_rect.height += rasterizer->state->gstate.line_width * 2;
        shape_rect.x -= rasterizer->state->gstate.line_width;
        shape_rect.y -= rasterizer->state->gstate.line_width;

        hash ^= (int)(rasterizer->state->gstate.line_width * 110);
        hash ^= (rasterizer->state->gstate.line_cap * 23);
        hash ^= (rasterizer->state->gstate.source_stroke.type * 117);

        ctx_sha1_process(&sha1, (unsigned char*)&hash, 8);

        uint32_t color;
        ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));

          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);

          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);
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
#if 0
      case CTX_TEXTURE:
        hasher->salt++;
#if 0
        ctx_rasterizer_set_texture (rasterizer, c->texture.eid,
                        c->texture.x, c->texture.y);
#endif
        break;
#endif
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
      case CTX_DEFINE_TEXTURE:
        {
#if 0
          uint8_t *pixel_data = ctx_define_texture_pixel_data (entry);
          ctx_rasterizer_define_texture (rasterizer, c->define_texture.eid,
                                         c->define_texture.width, c->define_texture.height,
                                         c->define_texture.format,
                                         pixel_data);
#endif
        hasher->salt = ctx_strhash (c->define_texture.eid, 0);
        }
        break;
      case CTX_TEXTURE:
        //fprintf (stderr, "sett %s\n", c->texture.eid);
    //  ctx_rasterizer_set_texture (rasterizer, c->texture.eid,
    //                              c->texture.x, c->texture.y);
        hasher->salt = ctx_strhash (c->texture.eid, 0);
        rasterizer->comp_op = NULL;
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
  rasterizer->edge_list.flags |= CTX_DRAWLIST_EDGE_LIST;
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
  Ctx *ctx           = ctx_new ();
  CtxState    *state = &ctx->state;
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
