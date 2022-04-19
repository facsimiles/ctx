#include "ctx-split.h"

#if CTX_CAIRO

typedef struct _CtxCairo CtxCairo;
struct
  _CtxCairo
{
  CtxBackend        backend;
  cairo_t          *cr;
  cairo_pattern_t  *pat;
  cairo_surface_t  *image;
  int               preserve;

  // maintain separate fill and stroke state - even though the more limited use of ctx
  // then suffers?
  //
};

static void
ctx_cairo_process (Ctx *ctx, CtxCommand *c)
{
  CtxCairo *ctx_cairo = (void*)ctx->backend;
  CtxEntry *entry = (CtxEntry *) &c->entry;

#if CTX_CURRENT_PATH
  ctx_update_current_path (ctx, entry);
#endif

  cairo_t *cr = ctx_cairo->cr;
  switch (entry->code)
    {
      case CTX_LINE_TO:
        cairo_line_to (cr, c->line_to.x, c->line_to.y);
        break;
      case CTX_REL_LINE_TO:
        cairo_rel_line_to (cr, c->rel_line_to.x, c->rel_line_to.y);
        break;
      case CTX_MOVE_TO:
        cairo_move_to (cr, c->move_to.x, c->move_to.y);
        break;
      case CTX_REL_MOVE_TO:
        cairo_rel_move_to (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_CURVE_TO:
        cairo_curve_to (cr, ctx_arg_float (0), ctx_arg_float (1),
                        ctx_arg_float (2), ctx_arg_float (3),
                        ctx_arg_float (4), ctx_arg_float (5) );
        break;
      case CTX_REL_CURVE_TO:
        cairo_rel_curve_to (cr,ctx_arg_float (0), ctx_arg_float (1),
                            ctx_arg_float (2), ctx_arg_float (3),
                            ctx_arg_float (4), ctx_arg_float (5) );
        break;
      case CTX_PRESERVE:
        ctx_cairo->preserve = 1;
        break;
      case CTX_QUAD_TO:
        {
          double x0, y0;
          cairo_get_current_point (cr, &x0, &y0);
          float cx = ctx_arg_float (0);
          float cy = ctx_arg_float (1);
          float  x = ctx_arg_float (2);
          float  y = ctx_arg_float (3);
          cairo_curve_to (cr,
                          (cx * 2 + x0) / 3.0f, (cy * 2 + y0) / 3.0f,
                          (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                          x,                              y);
        }
        break;
      case CTX_REL_QUAD_TO:
        {
          double x0, y0;
          cairo_get_current_point (cr, &x0, &y0);
          float cx = ctx_arg_float (0) + x0;
          float cy = ctx_arg_float (1) + y0;
          float  x = ctx_arg_float (2) + x0;
          float  y = ctx_arg_float (3) + y0;
          cairo_curve_to (cr,
                          (cx * 2 + x0) / 3.0f, (cy * 2 + y0) / 3.0f,
                          (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                          x,                              y);
        }
        break;
      /* rotate/scale/translate does not occur in fully minified data stream */
      case CTX_ROTATE:
        cairo_rotate (cr, ctx_arg_float (0) );
        break;
      case CTX_SCALE:
        cairo_scale (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_TRANSLATE:
        cairo_translate (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_LINE_WIDTH:
        cairo_set_line_width (cr, ctx_arg_float (0) );
        break;
      case CTX_ARC:
#if 0
        fprintf (stderr, "F %2.1f %2.1f %2.1f %2.1f %2.1f %2.1f\n",
                        ctx_arg_float(0),
                        ctx_arg_float(1),
                        ctx_arg_float(2),
                        ctx_arg_float(3),
                        ctx_arg_float(4),
                        ctx_arg_float(5),
                        ctx_arg_float(6));
#endif
        if (ctx_arg_float (5) == 1)
          cairo_arc (cr, ctx_arg_float (0), ctx_arg_float (1),
                     ctx_arg_float (2), ctx_arg_float (3),
                     ctx_arg_float (4) );
        else
          cairo_arc_negative (cr, ctx_arg_float (0), ctx_arg_float (1),
                              ctx_arg_float (2), ctx_arg_float (3),
                              ctx_arg_float (4) );
        break;
      case CTX_SET_RGBA_U8:
        cairo_set_source_rgba (cr, ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ),
                               ctx_u8_to_float (ctx_arg_u8 (3) ) );
        break;

      case CTX_COLOR:
        int space =  ((int) ctx_arg_float (0)) & 511;
        switch (space) // XXX remove 511 after stroke source is complete
        {
           case CTX_RGBA:
           case CTX_DRGBA:
             cairo_set_source_rgba (cr, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
             break;
           case CTX_RGB:
             cairo_set_source_rgba (cr, c->rgba.r, c->rgba.g, c->rgba.b, 1.0f);
             break;
           case CTX_CMYKA:
           case CTX_DCMYKA:
           case CTX_CMYK:
           case CTX_DCMYK:
              break;
           case CTX_GRAYA:
             cairo_set_source_rgba (cr, c->graya.g, c->graya.g, c->graya.g, c->graya.a);
             break;
               /*FALLTHROUGH*/
           case CTX_GRAY:
             cairo_set_source_rgba (cr, c->graya.g, c->graya.g, c->graya.g, 1.0f);
             break;
            }
        break;

#if 0
      case CTX_SET_RGBA_STROKE: // XXX : we need to maintain
        //       state for the two kinds
        cairo_set_source_rgba (cr, ctx_arg_u8 (0) /255.0,
                               ctx_arg_u8 (1) /255.0,
                               ctx_arg_u8 (2) /255.0,
                               ctx_arg_u8 (3) /255.0);
        break;
#endif
      case CTX_RECTANGLE:
      case CTX_ROUND_RECTANGLE: // XXX - arcs
        cairo_rectangle (cr, c->rectangle.x, c->rectangle.y,
                         c->rectangle.width, c->rectangle.height);
        break;
      case CTX_SET_PIXEL:
        cairo_set_source_rgba (cr, ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ),
                               ctx_u8_to_float (ctx_arg_u8 (3) ) );
        cairo_rectangle (cr, ctx_arg_u16 (2), ctx_arg_u16 (3), 1, 1);
        cairo_fill (cr);
        break;
      case CTX_FILL:
        if (ctx_cairo->preserve)
        {
          cairo_fill_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_fill (cr);
        }
        break;
      case CTX_STROKE:
        if (ctx_cairo->preserve)
        {
          cairo_stroke_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_stroke (cr);
        }
        break;
      case CTX_IDENTITY:
        cairo_identity_matrix (cr);
        break;
      case CTX_CLIP:
        if (ctx_cairo->preserve)
        {
          cairo_clip_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_clip (cr);
        }
        break;
      case CTX_BEGIN_PATH:
        cairo_new_path (cr);
        break;
      case CTX_CLOSE_PATH:
        cairo_close_path (cr);
        break;
      case CTX_SAVE:
        cairo_save (cr);
        break;
      case CTX_RESTORE:
        cairo_restore (cr);
        break;
      case CTX_FONT_SIZE:
        cairo_set_font_size (cr, ctx_arg_float (0) );
        break;
      case CTX_MITER_LIMIT:
        cairo_set_miter_limit (cr, ctx_arg_float (0) );
        break;
      case CTX_LINE_CAP:
        {
          int cairo_val = CAIRO_LINE_CAP_SQUARE;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_CAP_ROUND:
                cairo_val = CAIRO_LINE_CAP_ROUND;
                break;
              case CTX_CAP_SQUARE:
                cairo_val = CAIRO_LINE_CAP_SQUARE;
                break;
              case CTX_CAP_NONE:
                cairo_val = CAIRO_LINE_CAP_BUTT;
                break;
            }
          cairo_set_line_cap (cr, cairo_val);
        }
        break;
      case CTX_BLEND_MODE:
        {
          // does not map to cairo
        }
        break;
      case CTX_COMPOSITING_MODE:
        {
          int cairo_val = CAIRO_OPERATOR_OVER;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_COMPOSITE_SOURCE_OVER:
                cairo_val = CAIRO_OPERATOR_OVER;
                break;
              case CTX_COMPOSITE_COPY:
                cairo_val = CAIRO_OPERATOR_SOURCE;
                break;
            }
          cairo_set_operator (cr, cairo_val);
        }
        break;
      case CTX_LINE_JOIN:
        {
          int cairo_val = CAIRO_LINE_JOIN_ROUND;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_JOIN_ROUND:
                cairo_val = CAIRO_LINE_JOIN_ROUND;
                break;
              case CTX_JOIN_BEVEL:
                cairo_val = CAIRO_LINE_JOIN_BEVEL;
                break;
              case CTX_JOIN_MITER:
                cairo_val = CAIRO_LINE_JOIN_MITER;
                break;
            }
          cairo_set_line_join (cr, cairo_val);
        }
        break;
      case CTX_LINEAR_GRADIENT:
        {
          if (ctx_cairo->pat)
            {
              cairo_pattern_destroy (ctx_cairo->pat);
              ctx_cairo->pat = NULL;
            }
          ctx_cairo->pat = cairo_pattern_create_linear (ctx_arg_float (0), ctx_arg_float (1),
                           ctx_arg_float (2), ctx_arg_float (3) );
          cairo_pattern_add_color_stop_rgba (ctx_cairo->pat, 0, 0, 0, 0, 1);
          cairo_pattern_add_color_stop_rgba (ctx_cairo->pat, 1, 1, 1, 1, 1);
          cairo_set_source (cr, ctx_cairo->pat);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          if (ctx_cairo->pat)
            {
              cairo_pattern_destroy (ctx_cairo->pat);
              ctx_cairo->pat = NULL;
            }
          ctx_cairo->pat = cairo_pattern_create_radial (ctx_arg_float (0), ctx_arg_float (1),
                           ctx_arg_float (2), ctx_arg_float (3),
                           ctx_arg_float (4), ctx_arg_float (5) );
          cairo_set_source (cr, ctx_cairo->pat);
        }
        break;
      case CTX_GRADIENT_STOP:
        cairo_pattern_add_color_stop_rgba (ctx_cairo->pat,
                                           ctx_arg_float (0),
                                           ctx_u8_to_float (ctx_arg_u8 (4) ),
                                           ctx_u8_to_float (ctx_arg_u8 (5) ),
                                           ctx_u8_to_float (ctx_arg_u8 (6) ),
                                           ctx_u8_to_float (ctx_arg_u8 (7) ) );
        break;
        // XXX  implement TEXTURE
#if 0
      case CTX_LOAD_IMAGE:
        {
          if (image)
            {
              cairo_surface_destroy (image);
              image = NULL;
            }
          if (pat)
            {
              cairo_pattern_destroy (pat);
              pat = NULL;
            }
          image = cairo_image_surface_create_from_png (ctx_arg_string() );
          cairo_set_source_surface (cr, image, ctx_arg_float (0), ctx_arg_float (1) );
        }
        break;
#endif
      case CTX_TEXT:
        /* XXX: implement some linebreaking/wrap, positioning
         *      behavior here?
         */
        cairo_show_text (cr, ctx_arg_string () );
        break;
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_END_FRAME:
        break;
    }
  ctx_process (ctx_cairo->backend.ctx, entry);
}

void ctx_cairo_destroy (CtxCairo *ctx_cairo)
{
  if (ctx_cairo->pat)
    { cairo_pattern_destroy (ctx_cairo->pat); }
  if (ctx_cairo->image)
    { cairo_surface_destroy (ctx_cairo->image); }
  ctx_free (ctx_cairo);
}

void
ctx_render_cairo (Ctx *ctx, cairo_t *cr)
{
  CtxCairo    ctx_cairo; /* on-stack backend */
  CtxBackend *backend = (CtxBackend*)&ctx_cairo;
  CtxIterator iterator;
  CtxCommand *command;
  ctx_cairo.cr = cr;
  backend->process = ctx_cairo_process;
  backend->ctx = ctx;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_cairo_process (ctx, command); }
}

Ctx *
ctx_new_for_cairo (cairo_t *cr)
{
  Ctx *ctx = _ctx_new_drawlist (640, 480);
  CtxCairo *ctx_cairo = ctx_calloc(sizeof(CtxCairo),1);
  CtxBackend *backend  = (CtxBackend*)ctx_cairo;
  backend->destroy = (void*)ctx_cairo_destroy;
  backend->process = ctx_cairo_process;
  backend->ctx = ctx;
  ctx_cairo->cr = cr;
  ctx_set_backend (ctx, (void*)ctx_cairo);
  return ctx;
}

#endif
