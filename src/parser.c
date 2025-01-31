static void ctx_svg_arc_circle_to (Ctx *ctx,
                                   float radius,
                                   int large,
                                   int sweep,
                                   float x1, float y1)
{
  float x0, y0;
  ctx_current_point (ctx, &x0, &y0);
  int left_side = (large && !sweep) || (sweep && !large);

  float delta_x = (x1-x0) * 0.5f;
  float delta_y = (y1-y0) * 0.5f;

  float midpoint_x = x0 + delta_x;
  float midpoint_y = y0 + delta_y;

  float radius_vec_x;
  float radius_vec_y;
  float r = radius;

  if (left_side)
  {
    radius_vec_x = -delta_y;
    radius_vec_y = delta_x;
  }
  else
  {
    radius_vec_x = delta_y;
    radius_vec_y = -delta_x;
  }

  float len_squared = ctx_pow2(radius_vec_x) + ctx_pow2(radius_vec_y);
  if (len_squared - 0.03f > r * r || r < 0)
  {
    r = ctx_sqrtf (len_squared);
  }

  float center_x = midpoint_x +
           radius_vec_x * ctx_sqrtf(ctx_maxf(0, r * r / len_squared-1));
  float center_y = midpoint_y +
           radius_vec_y * ctx_sqrtf(ctx_maxf(0, r * r / len_squared-1));

  float arc = ctx_asinf(ctx_clampf(ctx_sqrtf(len_squared)/r, -1.0, 1.0))*2;
  if (large) arc = CTX_PI*2-arc;

  float start_angle = ctx_atan2f(y0 - center_y, x0 - center_x);
  float end_angle = sweep?start_angle+arc:start_angle-arc;

  ctx_arc (ctx, center_x, center_y, r, start_angle, end_angle, !sweep);
}


static inline void ctx_svg_arc_to (Ctx *ctx, float rx, float ry, 
                            float rotation,  int large, int sweep,
                            float x1, float y1)
{
  ctx_svg_arc_circle_to (ctx, rx, large, sweep, x1, y1);
  return;
   // XXX the following fails, one reason is that
   // ctx_current_point returns the point in the previous user_space
   // not the current.

  float x0, y0;
  ctx_current_point (ctx, &x0, &y0);
  float radius_min = ctx_hypotf (x1-x0,y1-y0)/2.0f;
  float radius_lim = ctx_hypotf (rx, ry);
  float up_scale = 1.0f;
  if (radius_lim < radius_min)
    up_scale = radius_min / radius_lim;
  float ratio = rx / ry;
  ctx_save (ctx);
  ctx_scale (ctx, up_scale * ratio, up_scale);

  //  the following is a hack, current_point should change instead,
  //  but that can have performance impact on adding coordinates
  ctx->state.x /= (up_scale * ratio);
  ctx->state.y /= (up_scale);


  //ctx_rotate (ctx, rotation);
  
  x1 = x1 / (up_scale * ratio);
  y1 = y1 / (up_scale);

  ctx_svg_arc_circle_to (ctx, rx, large, sweep, x1, y1);

  ctx_restore (ctx);
}

#include "ctx-split.h"
/* the parser comes in the end, nothing in ctx knows about the parser  */

#if CTX_PARSER

/* ctx parser, */

#define CTX_ID_MAXLEN 64 // in use should not be more than 40!
                         // to offer headroom for multiplexing


#define CTX_REPORT_COL_ROW 1

struct
  _CtxParser
{
  Ctx       *ctx;
  int        t_args; // total number of arguments seen for current command
  int        state;
#if CTX_PARSER_FIXED_TEMP
  uint8_t    holding[CTX_PARSER_MAXLEN]; /*  */
#else
  uint8_t   *holding;
#endif
  int        hold_len;
  int        pos;

#if CTX_REPORT_COL_ROW
  int        line; /*  for error reporting */
  int        col;  /*  for error reporting */
#endif
  float      numbers[CTX_PARSER_MAX_ARGS+1];
  int        n_numbers;
  int        decimal;
  int        exponent;
  int        exp;
  CtxCode    command;
  int        expected_args; /* low digits are literal higher values
                               carry special meaning */
  int        n_args;
  int        texture_done;
  uint8_t    texture_id[CTX_ID_MAXLEN]; // used in defineTexture only
  uint32_t   set_key_hash;
  float      pcx;
  float      pcy;
  int        color_components;
  int        color_stroke; // 0 is fill source  1 is stroke source
  CtxColorModel   color_model; // 1 gray 3 rgb 4 cmyk
  float      left_margin; // set by last user provided move_to
  int        width;       // <- maybe should be float
  int        height;
  float      cell_width;
  float      cell_height;
  int        cursor_x;    // <- leaking in from terminal
  int        cursor_y;

  int        translate_origin;

  CtxColorSpace   color_space_slot;

  void (*frame_done) (void *frame_done_data);
  void *frame_done_data;
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len);
  int   (*get_prop)(void *prop_data, const char *key, char **data, int *len);
  void *prop_data;
  int   prev_byte;

#if CTX_REPORT_COL_ROW
  char *error;
  int   error_col;
  int   error_row;
#endif
};

void
ctx_parser_set_size (CtxParser *parser,
                 int        width,
                 int        height,
                 float      cell_width,
                 float      cell_height)
{
  if (cell_width > 0)
    parser->cell_width       = cell_width;
  if (cell_height > 0)
    parser->cell_height      = cell_height;
  if (width > 0)
    parser->width            = width;
  if (height > 0)
    parser->height           = height;
}

static CtxParser *
ctx_parser_init (CtxParser *parser,
                 Ctx       *ctx,
                 int        width,
                 int        height,
                 float      cell_width,
                 float      cell_height,
                 int        cursor_x,
                 int        cursor_y,
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
                 void  *prop_data,
                 void (*frame_done) (void *frame_done_data),
                 void *frame_done_data
                )
{
  memset (parser, 0, sizeof (CtxParser) );
#if CTX_REPORT_COL_ROW
  parser->line             = 1;
#endif
  parser->ctx              = ctx;
  parser->cell_width       = cell_width;
  parser->cell_height      = cell_height;
  parser->cursor_x         = cursor_x;
  parser->cursor_y         = cursor_y;
  parser->width            = width;
  parser->height           = height;
  parser->frame_done       = frame_done;
  parser->frame_done_data  = frame_done_data;
  parser->color_model      = CTX_RGBA;
  parser->color_stroke     = 0;
  parser->color_components = 4;
  parser->command          = CTX_MOVE_TO;
  parser->set_prop         = set_prop;
  parser->get_prop         = get_prop;
  parser->prop_data        = prop_data;
  
  int new_len = 512;
#if CTX_PARSER_FIXED_TEMP
  parser->hold_len = CTX_PARSER_MAXLEN;
#else
  parser->holding = (uint8_t*)ctx_realloc (parser->holding, parser->hold_len, new_len);
  parser->hold_len = new_len;
#endif
  return parser;
}

CtxParser *ctx_parser_new (
  Ctx       *ctx,
  int        width,
  int        height,
  float      cell_width,
  float      cell_height,
  int        cursor_x,
  int        cursor_y,
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
  void  *prop_data,
  void (*frame_done) (void *frame_done_data),
  void *frame_done_data)
{
  return ctx_parser_init ( (CtxParser *) ctx_calloc (sizeof (CtxParser), 1),
                           ctx,
                           width, height,
                           cell_width, cell_height,
                           cursor_x, cursor_y, set_prop, get_prop, prop_data,
                           frame_done, frame_done_data);
}

void ctx_parser_destroy (CtxParser *parser)
{
#if !CTX_PARSER_FIXED_TEMP
  if (parser->holding)
    ctx_free (parser->holding);
#endif
  if (parser->error)
  {
    fprintf (stderr, "ctx parse error: %s\n", parser->error);
    ctx_free (parser->error);
  }
  ctx_free (parser);
}



static int ctx_parser_set_command (CtxParser *parser, CtxCode code)
{
  if (code <= CTX_LAST_COMMAND && code >= 32)
  {
  parser->expected_args = ctx_arguments_for_code (code);
  parser->n_args = 0;
  parser->texture_done = 0;
  if (parser->expected_args >= CTX_ARG_NUMBER_OF_COMPONENTS)
    {
      parser->expected_args = (parser->expected_args % 100) + parser->color_components;
    }
  }
  return code;
}

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model, int stroke);

static int ctx_parser_resolve_command (CtxParser *parser, const uint8_t *str)
{
  uint32_t ret = str[0]; /* if it is single char it already is the CtxCode */

  /* this is handled outside the hashing to make it possible to be case insensitive
   * with the rest.
   */
  if (str[0] == CTX_SET_KEY && str[1] && str[2] == 0)
  {
    switch (str[1])
    {
      case 'm': return ctx_parser_set_command (parser, CTX_COMPOSITING_MODE);
      case 'B': return ctx_parser_set_command (parser, CTX_BLEND_MODE);
      case 'e': return ctx_parser_set_command (parser, CTX_EXTEND);
      case 'l': return ctx_parser_set_command (parser, CTX_MITER_LIMIT);
      case 't': return ctx_parser_set_command (parser, CTX_TEXT_ALIGN);
      case 'b': return ctx_parser_set_command (parser, CTX_TEXT_BASELINE);
      case 'd': return ctx_parser_set_command (parser, CTX_TEXT_DIRECTION);
      case 'j': return ctx_parser_set_command (parser, CTX_LINE_JOIN);
      case 'c': return ctx_parser_set_command (parser, CTX_LINE_CAP);
      case 'w': return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
      case 'D': return ctx_parser_set_command (parser, CTX_LINE_DASH_OFFSET);
      case 'p': return ctx_parser_set_command (parser, CTX_STROKE_POS);
      case 'F': return ctx_parser_set_command (parser, CTX_FEATHER);
      case 'H': return ctx_parser_set_command (parser, CTX_LINE_HEIGHT);
      case 'L': return ctx_parser_set_command (parser, CTX_WRAP_LEFT);
      case 'R': return ctx_parser_set_command (parser, CTX_WRAP_RIGHT);
      case 'S': return ctx_parser_set_command (parser, CTX_IMAGE_SMOOTHING);
      case 'C': return ctx_parser_set_command (parser, CTX_SHADOW_COLOR);
      case 's': return ctx_parser_set_command (parser, CTX_SHADOW_BLUR);
      case 'x': return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_X);
      case 'y': return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_Y);
      case 'a': return ctx_parser_set_command (parser, CTX_GLOBAL_ALPHA);
      case 'f': return ctx_parser_set_command (parser, CTX_FONT_SIZE);
      case 'r': return ctx_parser_set_command (parser, CTX_FILL_RULE);
    }
  }

  if (str[0] && str[1])
    {
      uint32_t str_hash;
      /* trim ctx_ and CTX_ prefix */
      if ( (str[0] == 'c' && str[1] == 't' && str[2] == 'x' && str[3] == '_') ||
           (str[0] == 'C' && str[1] == 'T' && str[2] == 'X' && str[3] == '_') )
        {
          str += 4;
        }
      if ( (str[0] == 's' && str[1] == 'e' && str[2] == 't' && str[3] == '_') )
        { str += 4; }
      str_hash = ctx_strhash ( (char *) str);
      switch (str_hash)
        {
          /* first a list of mappings to one_char hashes, handled in a
           * separate fast path switch without hashing
           */
          case SQZ_arcTo:          ret = CTX_ARC_TO; break;
          case SQZ_arc:            ret = CTX_ARC; break;
          case SQZ_curveTo:        ret = CTX_CURVE_TO; break;
          case SQZ_restore:        ret = CTX_RESTORE; break;
          case SQZ_stroke:         ret = CTX_STROKE; break;
          case SQZ_fill:           ret = CTX_FILL; break;
          case SQZ_paint:          ret = CTX_PAINT; break;
          case SQZ_preserve:       ret = CTX_PRESERVE; break;
          case SQZ_endFrame:       ret = CTX_END_FRAME; break;
          case SQZ_horLineTo:      ret = CTX_HOR_LINE_TO; break;
          case SQZ_rotate:         ret = CTX_ROTATE; break;
          case SQZ_color:          ret = CTX_COLOR; break;
          case SQZ_lineTo:         ret = CTX_LINE_TO; break;
          case SQZ_moveTo:         ret = CTX_MOVE_TO; break;
          case SQZ_scale:          ret = CTX_SCALE; break;
          case SQZ_newPage:        ret = CTX_NEW_PAGE; break;
          case SQZ_quadTo:         ret = CTX_QUAD_TO; break;
          case SQZ_viewBox:        ret = CTX_VIEW_BOX; break;
          case SQZ_smoothTo:       ret = CTX_SMOOTH_TO; break;
          case SQZ_smoothQuadTo:   ret = CTX_SMOOTHQ_TO; break;
          case SQZ_clear:          ret = CTX_COMPOSITE_CLEAR; break;
          case SQZ_copy:           ret = CTX_COMPOSITE_COPY; break;
          case SQZ_destinationOver:  ret = CTX_COMPOSITE_DESTINATION_OVER; break;
          case SQZ_destinationIn:    ret = CTX_COMPOSITE_DESTINATION_IN; break;
          case SQZ_destinationOut:   ret = CTX_COMPOSITE_DESTINATION_OUT; break;
          case SQZ_sourceOver:       ret = CTX_COMPOSITE_SOURCE_OVER; break;
          case SQZ_sourceAtop:       ret = CTX_COMPOSITE_SOURCE_ATOP; break;
          case SQZ_destinationAtop:  ret = CTX_COMPOSITE_DESTINATION_ATOP; break;
          case SQZ_sourceOut:        ret = CTX_COMPOSITE_SOURCE_OUT; break;
          case SQZ_sourceIn:         ret = CTX_COMPOSITE_SOURCE_IN; break;
          case SQZ_xor:              ret = CTX_COMPOSITE_XOR; break;
          case SQZ_darken:           ret = CTX_BLEND_DARKEN; break;
          case SQZ_lighten:          ret = CTX_BLEND_LIGHTEN; break;
          //case SQZ_color:          ret = CTX_BLEND_COLOR; break;
          //
          //  XXX check that he special casing for color works
          //      it is the first collision and it is due to our own
          //      color, not w3c for now unique use of it
          //
          case SQZ_hue:            ret = CTX_BLEND_HUE; break;
          case SQZ_multiply:       ret = CTX_BLEND_MULTIPLY; break;
          case SQZ_normal:         ret = CTX_BLEND_NORMAL;break;
          case SQZ_screen:         ret = CTX_BLEND_SCREEN;break;
          case SQZ_difference:     ret = CTX_BLEND_DIFFERENCE; break;
          case SQZ_startFrame:     ret = CTX_START_FRAME; break;
          case SQZ_verLineTo:      ret = CTX_VER_LINE_TO; break;
          case SQZ_exit:
          case SQZ_done:           ret = CTX_EXIT; break;
          case SQZ_closePath:      ret = CTX_CLOSE_PATH; break;
          case SQZ_resetPath:
          case SQZ_beginPath:
          case SQZ_newPath:        ret = CTX_RESET_PATH; break;
          case SQZ_relArcTo:       ret = CTX_REL_ARC_TO; break;
          case SQZ_clip:           ret = CTX_CLIP; break;
          case SQZ_relCurveTo:     ret = CTX_REL_CURVE_TO; break;
          case SQZ_startGroup:     ret = CTX_START_GROUP; break;
          case SQZ_endGroup:       ret = CTX_END_GROUP; break;
          case SQZ_save:           ret = CTX_SAVE; break;
          case SQZ_translate:      ret = CTX_TRANSLATE; break;
          case SQZ_linearGradient: ret = CTX_LINEAR_GRADIENT; break;
          case SQZ_conicGradient:  ret = CTX_CONIC_GRADIENT; break;
          case SQZ_relHorLineTo:   ret = CTX_REL_HOR_LINE_TO; break;
          case SQZ_relLineTo:      ret = CTX_REL_LINE_TO; break;
          case SQZ_relMoveTo:      ret = CTX_REL_MOVE_TO; break;
          case SQZ_font:           ret = CTX_FONT; break;
          case SQZ_radialGradient:ret = CTX_RADIAL_GRADIENT; break;
          case SQZ_gradientAddStop:
          case SQZ_addStop:        ret = CTX_GRADIENT_STOP; break;
          case SQZ_relQuadTo:      ret = CTX_REL_QUAD_TO; break;
          case SQZ_rectangle:
          case SQZ_rect:           ret = CTX_RECTANGLE; break;
          case SQZ_roundRectangle: ret = CTX_ROUND_RECTANGLE; break;
          case SQZ_relSmoothTo:    ret = CTX_REL_SMOOTH_TO; break;
          case SQZ_relSmoothqTo:   ret = CTX_REL_SMOOTHQ_TO; break;
          case SQZ_strokeRect:     ret = CTX_STROKE_RECT; break;
          case SQZ_fillRect:       ret = CTX_FILL_RECT; break;
          case SQZ_relVerLineTo:   ret = CTX_REL_VER_LINE_TO; break;
          case SQZ_text:           ret = CTX_TEXT; break;
          case SQZ_identity:       ret = CTX_IDENTITY; break;
          case SQZ_transform:      ret = CTX_APPLY_TRANSFORM; break;
          case SQZ_sourceTransform: ret = CTX_SOURCE_TRANSFORM; break;
          case SQZ_texture:        ret = CTX_TEXTURE; break;
          case SQZ_defineTexture:  ret = CTX_DEFINE_TEXTURE; break;
#if 0
          case SQZ_rgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_RGB_SPACE);
          case SQZ_cmykSpace:
            return ctx_parser_set_command (parser, CTX_SET_CMYK_SPACE);
          case SQZ_drgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_DRGB_SPACE);
#endif
          case SQZ_defineFont:
            return ctx_parser_set_command (parser, CTX_DEFINE_FONT);
          case SQZ_defineGlyph:
            return ctx_parser_set_command (parser, CTX_DEFINE_GLYPH);
          case SQZ_kerningPair:
            return ctx_parser_set_command (parser, CTX_KERNING_PAIR);

          case SQZ_colorSpace:
            return ctx_parser_set_command (parser, CTX_COLOR_SPACE);
          case SQZ_fillRule:
            return ctx_parser_set_command (parser, CTX_FILL_RULE);
          case SQZ_fontSize:
          case SQZ_setFontSize:
            return ctx_parser_set_command (parser, CTX_FONT_SIZE);
          case SQZ_compositingMode:
            return ctx_parser_set_command (parser, CTX_COMPOSITING_MODE);

          case SQZ_extend:
            return ctx_parser_set_command (parser, CTX_EXTEND);

          case SQZ_blend:
          case SQZ_blending:
          case SQZ_blendMode:
            return ctx_parser_set_command (parser, CTX_BLEND_MODE);

          case SQZ_miterLimit:
            return ctx_parser_set_command (parser, CTX_MITER_LIMIT);
          case SQZ_textAlign:
            return ctx_parser_set_command (parser, CTX_TEXT_ALIGN);
          case SQZ_textBaseline:
            return ctx_parser_set_command (parser, CTX_TEXT_BASELINE);
          case SQZ_textDirection:
            return ctx_parser_set_command (parser, CTX_TEXT_DIRECTION);
          case SQZ_join:
          case SQZ_lineJoin:
          case SQZ_setLineJoin:
            return ctx_parser_set_command (parser, CTX_LINE_JOIN);
          case SQZ_glyph:
            return ctx_parser_set_command (parser, CTX_GLYPH);
          case SQZ_cap:
          case SQZ_lineCap:
          case SQZ_setLineCap:
            return ctx_parser_set_command (parser, CTX_LINE_CAP);
          case SQZ_lineDash:
            return ctx_parser_set_command (parser, CTX_LINE_DASH);
          case SQZ_lineWidth:
          case SQZ_setLineWidth:
            return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
          case SQZ_lineDashOffset:
            return ctx_parser_set_command (parser, CTX_LINE_DASH_OFFSET);
	  case SQZ_strokePos:
            return ctx_parser_set_command (parser, CTX_STROKE_POS);
	  case SQZ_feather:
            return ctx_parser_set_command (parser, CTX_FEATHER);
          case SQZ_lineHeight:
            return ctx_parser_set_command (parser, CTX_LINE_HEIGHT);
          case SQZ_wrapLeft:
            return ctx_parser_set_command (parser, CTX_WRAP_LEFT);
          case SQZ_wrapRight:
            return ctx_parser_set_command (parser, CTX_WRAP_RIGHT);
          case SQZ_imageSmoothing:
            return ctx_parser_set_command (parser, CTX_IMAGE_SMOOTHING);
          case SQZ_shadowColor:
            return ctx_parser_set_command (parser, CTX_SHADOW_COLOR);
          case SQZ_shadowBlur:
            return ctx_parser_set_command (parser, CTX_SHADOW_BLUR);
          case SQZ_shadowOffsetX:
            return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_X);
          case SQZ_shadowOffsetY:
            return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_Y);
          case SQZ_globalAlpha:
            return ctx_parser_set_command (parser, CTX_GLOBAL_ALPHA);

          case SQZ_strokeSource:
            return ctx_parser_set_command (parser, CTX_STROKE_SOURCE);

          /* strings are handled directly here,
           * instead of in the one-char handler, using return instead of break
           */
          case SQZ_gray:
            ctx_parser_set_color_model (parser, CTX_GRAY, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_graya:
            ctx_parser_set_color_model (parser, CTX_GRAYA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_rgb:
            ctx_parser_set_color_model (parser, CTX_RGB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_drgb:
            ctx_parser_set_color_model (parser, CTX_DRGB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_rgba:
            ctx_parser_set_color_model (parser, CTX_RGBA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_drgba:
            ctx_parser_set_color_model (parser, CTX_DRGBA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_cmyk:
            ctx_parser_set_color_model (parser, CTX_CMYK, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_cmyka:
            ctx_parser_set_color_model (parser, CTX_CMYKA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_lab:
            ctx_parser_set_color_model (parser, CTX_LAB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_laba:
            ctx_parser_set_color_model (parser, CTX_LABA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_lch:
            ctx_parser_set_color_model (parser, CTX_LCH, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_lcha:
            ctx_parser_set_color_model (parser, CTX_LCHA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);

          /* and a full repeat of the above, with S for Stroke suffix */
          case SQZ_grayS:
            ctx_parser_set_color_model (parser, CTX_GRAY, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_grayaS:
            ctx_parser_set_color_model (parser, CTX_GRAYA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_rgbS:
            ctx_parser_set_color_model (parser, CTX_RGB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_drgbS:
            ctx_parser_set_color_model (parser, CTX_DRGB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_rgbaS:
            ctx_parser_set_color_model (parser, CTX_RGBA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_drgbaS:
            ctx_parser_set_color_model (parser, CTX_DRGBA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_cmykS:
            ctx_parser_set_color_model (parser, CTX_CMYK, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_cmykaS:
            ctx_parser_set_color_model (parser, CTX_CMYKA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_labS:
            ctx_parser_set_color_model (parser, CTX_LAB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_labaS:
            ctx_parser_set_color_model (parser, CTX_LABA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_lchS:
            ctx_parser_set_color_model (parser, CTX_LCH, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case SQZ_lchaS:
            ctx_parser_set_color_model (parser, CTX_LCHA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);

          /* words that correspond to low integer constants
          */
          case SQZ_nonzero:     return CTX_FILL_RULE_WINDING;
          case SQZ_winding:     return CTX_FILL_RULE_WINDING;
          case SQZ_evenOdd:     return CTX_FILL_RULE_EVEN_ODD;
          case SQZ_bevel:       return CTX_JOIN_BEVEL;
          case SQZ_round:       return CTX_JOIN_ROUND;
          case SQZ_miter:       return CTX_JOIN_MITER;
          case SQZ_none:        return CTX_CAP_NONE;
          case SQZ_square:      return CTX_CAP_SQUARE;
          case SQZ_start:       return CTX_TEXT_ALIGN_START;
          case SQZ_end:         return CTX_TEXT_ALIGN_END;
          case SQZ_left:        return CTX_TEXT_ALIGN_LEFT;
          case SQZ_right:       return CTX_TEXT_ALIGN_RIGHT;
          case SQZ_center:      return CTX_TEXT_ALIGN_CENTER;
          case SQZ_top:         return CTX_TEXT_BASELINE_TOP;
          case SQZ_bottom :     return CTX_TEXT_BASELINE_BOTTOM;
          case SQZ_middle:      return CTX_TEXT_BASELINE_MIDDLE;
          case SQZ_alphabetic:  return CTX_TEXT_BASELINE_ALPHABETIC;
          case SQZ_hanging:     return CTX_TEXT_BASELINE_HANGING;
          case SQZ_ideographic: return CTX_TEXT_BASELINE_IDEOGRAPHIC;

          case SQZ_userRGB:     return CTX_COLOR_SPACE_USER_RGB;
          case SQZ_deviceRGB:   return CTX_COLOR_SPACE_DEVICE_RGB;
          case SQZ_userCMYK:    return CTX_COLOR_SPACE_USER_CMYK;
          case SQZ_deviceCMYK:  return CTX_COLOR_SPACE_DEVICE_CMYK;
#undef STR
#undef LOWER
          default:
            ret = str_hash;
        }
    }
  if (ret == CTX_CLOSE_PATH2)
   {
     ret = CTX_CLOSE_PATH;
   }

  return ctx_parser_set_command (parser, (CtxCode) ret);
}

enum
{
  CTX_PARSER_NEUTRAL = 0,
  CTX_PARSER_NUMBER,
  CTX_PARSER_NEGATIVE_NUMBER,
  CTX_PARSER_WORD,
  CTX_PARSER_COMMENT,
  CTX_PARSER_STRING_APOS,
  CTX_PARSER_STRING_QUOT,
  CTX_PARSER_STRING_APOS_ESCAPED,
  CTX_PARSER_STRING_QUOT_ESCAPED,
  CTX_PARSER_STRING_A85,
  CTX_PARSER_STRING_YENC,
} CTX_STATE;

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model, int stroke)
{
  parser->color_model      = color_model;
  parser->color_stroke     = stroke;
  parser->color_components = ctx_color_model_get_components (color_model);
}

static void ctx_parser_get_color_rgba (CtxParser *parser, int offset, float *red, float *green, float *blue, float *alpha)
{
  /* XXX - this function is to be deprecated */
  *alpha = 1.0f;
  switch (parser->color_model)
    {
      case CTX_GRAYA:
        *alpha = parser->numbers[offset + 1];
        /* FALLTHROUGH */
      case CTX_GRAY:
        *red = *green = *blue = parser->numbers[offset + 0];
        break;
      default:
      case CTX_LABA: // NYI - needs RGB profile
      case CTX_LCHA: // NYI - needs RGB profile
      case CTX_RGBA:
        *alpha = parser->numbers[offset + 3];
        /* FALLTHROUGH */
      case CTX_LAB: // NYI
      case CTX_LCH: // NYI
      case CTX_RGB:
        *red = parser->numbers[offset + 0];
        *green = parser->numbers[offset + 1];
        *blue = parser->numbers[offset + 2];
        break;
      case CTX_CMYKA:
        *alpha = parser->numbers[offset + 4];
        /* FALLTHROUGH */
      case CTX_CMYK:
        /* should use profile instead  */
        *red = (1.0f-parser->numbers[offset + 0]) *
               (1.0f - parser->numbers[offset + 3]);
        *green = (1.0f-parser->numbers[offset + 1]) *
                 (1.0f - parser->numbers[offset + 3]);
        *blue = (1.0f-parser->numbers[offset + 2]) *
                (1.0f - parser->numbers[offset + 3]);
        break;
    }
}

static inline int ctx_clamp (int val, int min, int max)
{
  if (val < min) return min;
  if (val > max) return max;
  return val;
}

static void ctx_parser_dispatch_command (CtxParser *parser)
{
  CtxCode cmd = parser->command;
  Ctx *ctx = parser->ctx;
  if (parser->error)
    return;

  if (parser->expected_args != CTX_ARG_STRING_OR_NUMBER &&
      parser->expected_args != CTX_ARG_COLLECT_NUMBERS &&
      parser->expected_args != parser->n_numbers)
    {
#if CTX_REPORT_COL_ROW
       char *error = (char*)ctx_malloc (256);
       sprintf (error, "ctx:%i:%i %c got %i instead of %i args\n",
               parser->line, parser->col,
               cmd, parser->n_numbers, parser->expected_args);
       parser->error = error;
#endif
      return;
    }

#define arg(a)  (parser->numbers[a])
  parser->command = CTX_NOP;
  //parser->n_args = 0;
  switch (cmd)
    {
      default:
        break; // to silence warnings about missing ones
      case CTX_PRESERVE:
        ctx_preserve (ctx);
        break;
      case CTX_FILL:
        ctx_fill (ctx);
        break;
      case CTX_PAINT:
        ctx_paint (ctx);
        break;
      case CTX_SAVE:
        ctx_save (ctx);
        break;
      case CTX_START_GROUP:
        ctx_start_group (ctx);
        break;
      case CTX_END_GROUP:
        ctx_end_group (ctx);
        break;
      case CTX_STROKE:
        ctx_stroke (ctx);
        break;
      case CTX_STROKE_SOURCE:
        ctx_stroke_source (ctx);
        break;
      case CTX_RESTORE:
        ctx_restore (ctx);
        break;
#if CTX_ENABLE_CM
      case CTX_COLOR_SPACE:
        if (parser->n_numbers == 1)
        {
          parser->color_space_slot = (CtxColorSpace) ctx_clamp(arg(0), 0, CTX_COLOR_SPACE_LAST);
          parser->command = CTX_COLOR_SPACE; // did this work without?
        }
        else
        {
          ctx_colorspace (ctx, (CtxColorSpace)parser->color_space_slot,
                               parser->holding, parser->pos);
        }
        break;
#endif
      case CTX_KERNING_PAIR:
        switch (parser->n_args)
        {
          case 0:
            parser->numbers[0] = ctx_utf8_to_unichar ((char*)parser->holding);
            break;
          case 1:
            parser->numbers[1] = ctx_utf8_to_unichar ((char*)parser->holding);
            break;
          case 2:
            parser->numbers[2] = _ctx_parse_float ((char*)parser->holding, NULL);
            {
              CtxEntry e = {CTX_KERNING_PAIR, {{0},}};
              e.data.u16[0] = (uint16_t)parser->numbers[0];
              e.data.u16[1] = (uint16_t)parser->numbers[1];
              e.data.s32[1] = (int32_t)(parser->numbers[2] * 256);
              ctx_process (ctx, &e);
            }
            break;
        }
        parser->command = CTX_KERNING_PAIR;
        parser->n_args ++; // make this more generic?
        break;             
      case CTX_TEXTURE:
        if (parser->texture_done)
        {
        }
        else
        if (parser->n_numbers == 2)
        {
          const char *eid = (char*)parser->holding;
          float x0 = arg(0);
          float x1 = arg(1);
          ctx_texture (ctx, eid, x0, x1);
          parser->texture_done = 1;
        }
        parser->command = CTX_TEXTURE;
        //parser->n_args++;
        break;
      case CTX_DEFINE_TEXTURE:
        if (parser->texture_done)
        {
          if (parser->texture_done++ == 1)
          {
             const char *eid = (char*)parser->texture_id;
             int width  = (int)arg(0);
             int height = (int)arg(1);
             CtxPixelFormat format = (CtxPixelFormat)arg(2);
	     if (width > 0 && height > 0 && width < 65536 && height < 65536 && ctx_pixel_format_info (format))
             {
             int stride = ctx_pixel_format_get_stride (format, width);
             int data_len = stride * height;
             if (format == CTX_FORMAT_YUV420)
                 data_len = height * width + 2*(height/2) * (width/2);
	  {


             if (parser->pos != data_len)
             {
#if 0
             fprintf (stderr, "unexpected datasize for define texture %s %ix%i\n size:%i != expected:%i - start of data: %i %i %i %i\n", eid, width, height,
                               parser->pos,
                               stride * height,
                               parser->holding[0],
                               parser->holding[1],
                               parser->holding[2],
                               parser->holding[3]
                               );
#endif
             }
             else
             ctx_define_texture (ctx, eid, width, height, stride, format, parser->holding, NULL);
	     }
	     }
          }
        }
        else
        {
        switch (parser->n_numbers)
        {
          case 0:
             strncpy ((char*)parser->texture_id, (char*)parser->holding, sizeof(parser->texture_id));
             parser->texture_id[sizeof(parser->texture_id)-1]=0;
             break;
          case 1:
          case 2:
             break;
          case 3:
             parser->texture_done = 1;
             break;
          default:
             fprintf (stderr, "!!%i\n", parser->n_numbers);
             break;
        }
        }
        parser->command = CTX_DEFINE_TEXTURE;
        break;

      case CTX_DEFINE_FONT:
        // XXX: todo
        break;

      case CTX_DEFINE_GLYPH:
        /* XXX : reuse n_args logic - to enforce order */
        if (parser->n_numbers == 1)
        {
          CtxEntry e = {CTX_DEFINE_GLYPH, {{0},}};
          e.data.u32[0] = parser->color_space_slot;
          e.data.u32[1] = (int)arg(0) * 256;
          ctx_process (ctx, &e);
        }
        else
        {
          int unichar = ctx_utf8_to_unichar ((char*)parser->holding);
          parser->color_space_slot = (CtxColorSpace)unichar;
        }
        parser->command = CTX_DEFINE_GLYPH;
        break;             

      case CTX_COLOR:
        {
          switch (parser->color_model)
            {
              case CTX_GRAY:
              case CTX_GRAYA:
              case CTX_RGB:
              case CTX_RGBA:
              case CTX_DRGB:
              case CTX_DRGBA:
                ctx_color_raw (ctx, parser->color_model, parser->numbers, parser->color_stroke);
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYK:
              case CTX_CMYKA:
                ctx_color_raw (ctx, parser->color_model, parser->numbers, parser->color_stroke);
                break;
#else
              /* when there is no cmyk support at all in rasterizer
               * do a naive mapping to RGB on input.
               */
              case CTX_CMYK:
              case CTX_CMYKA:
              case CTX_DCMYKA:
                {
                  float rgba[4] = {1,1,1,1.0f};

                  ctx_cmyk_to_rgb (arg(0), arg(1), arg(2), arg(3), &rgba[0], &rgba[1], &rgba[2]);
                  if (parser->color_model == CTX_CMYKA)
                    { rgba[3] = arg(4); }
                  ctx_color_raw (ctx, CTX_RGBA, rgba, parser->color_stroke);
                }
                break;
#endif
              case CTX_LAB:
              case CTX_LCH:
              default:
                break;
            }
        }
        break;
      case CTX_LINE_DASH:
        if (parser->n_numbers)
        {
          ctx_line_dash (ctx, parser->numbers, parser->n_numbers);
        }
        else
        {
          ctx_line_dash (ctx, NULL, 0);
        }
        //append_dash_val (ctx, arg(0));
        break;
      case CTX_ARC_TO:
        ctx_svg_arc_to (ctx, arg(0), arg(1), arg(2), (int)arg(3), (int)arg(4), arg(5), arg(6));
        break;
      case CTX_REL_ARC_TO:
        //ctx_rel_arc_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4) );
        //
        {
          float x = ctx_x (ctx);
          float y = ctx_y (ctx);
          ctx_svg_arc_to (ctx, arg(0), arg(1), arg(2), (int)arg(3), (int)arg(4), arg(5)+x, arg(6)+y);
        }
        break;
      case CTX_REL_SMOOTH_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
	  float ox = ctx_x (ctx);
	  float oy = ctx_y (ctx);
          float ax = 2 * ox - cx;
          float ay = 2 * oy - cy;
          parser->pcx = arg(0) + ox;
          parser->pcy = arg(1) + oy;
          ctx_curve_to (ctx, ax, ay, arg(0) +  ox, arg(1) + oy,
                        arg(2) + ox, arg(3) + oy);
        }
        break;
      case CTX_SMOOTH_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
	  float ox = ctx_x (ctx);
	  float oy = ctx_y (ctx);
          float ax = 2 * ox - cx;
          float ay = 2 * oy - cy;
          ctx_curve_to (ctx, ax, ay, arg(0), arg(1),
                        arg(2), arg(3) );
          parser->pcx = arg(0);
          parser->pcy = arg(1);
        }
        break;
      case CTX_SMOOTHQ_TO:
        parser->pcx = 2 * ctx_x (ctx) - parser->pcx;
        parser->pcy = 2 * ctx_y (ctx) - parser->pcy;
        ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0), arg(1) );
        break;
      case CTX_REL_SMOOTHQ_TO:
        {
          float x = ctx_x (ctx);
          float y = ctx_y (ctx);
          parser->pcx = 2 * x - parser->pcx;
          parser->pcy = 2 * y - parser->pcy;
          ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0) + x, arg(1) + y);
        }
        break;
      case CTX_VER_LINE_TO:
        ctx_line_to (ctx, ctx_x (ctx), arg(0) );
        parser->command = CTX_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_HOR_LINE_TO:
        ctx_line_to (ctx, arg(0), ctx_y (ctx) );
        parser->command = CTX_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_HOR_LINE_TO:
        ctx_rel_line_to (ctx, arg(0), 0.0f);
        parser->command = CTX_REL_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_VER_LINE_TO:
        ctx_rel_line_to (ctx, 0.0f, arg(0) );
        parser->command = CTX_REL_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_ARC:
        ctx_arc (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), (int)arg(5));
        break;
      case CTX_APPLY_TRANSFORM:
        ctx_apply_transform (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) , arg(6), arg(7), arg(8));
        break;
      case CTX_SOURCE_TRANSFORM:
        ctx_source_transform (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5), arg(6), arg(7), arg(8));
        break;
      case CTX_CURVE_TO:
        ctx_curve_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        parser->pcx = arg(2);
        parser->pcy = arg(3);
        parser->command = CTX_CURVE_TO;
        break;
      case CTX_REL_CURVE_TO:
        parser->pcx = arg(2) + ctx_x (ctx);
        parser->pcy = arg(3) + ctx_y (ctx);
        ctx_rel_curve_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        parser->command = CTX_REL_CURVE_TO;
        break;
      case CTX_LINE_TO:
        ctx_line_to (ctx, arg(0), arg(1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        break;
      case CTX_MOVE_TO:
        ctx_move_to (ctx, arg(0), arg(1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        parser->left_margin = parser->pcx;
        break;
      case CTX_FONT_SIZE:
        ctx_font_size (ctx, arg(0) );
        break;
      case CTX_MITER_LIMIT:
        ctx_miter_limit (ctx, arg(0) );
        break;
      case CTX_SCALE:
        ctx_scale (ctx, arg(0), arg(1) );
        break;
      case CTX_NEW_PAGE:
        ctx_new_page (ctx);
        break;
      case CTX_QUAD_TO:
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        ctx_quad_to (ctx, arg(0), arg(1), arg(2), arg(3) );
        parser->command = CTX_QUAD_TO;
        break;
      case CTX_REL_QUAD_TO:
        parser->pcx = arg(0) + ctx_x (ctx);
        parser->pcy = arg(1) + ctx_y (ctx);
        ctx_rel_quad_to (ctx, arg(0), arg(1), arg(2), arg(3) );
        parser->command = CTX_REL_QUAD_TO;
        break;
      case CTX_CLIP:
        ctx_clip (ctx);
        break;
      case CTX_TRANSLATE:
        ctx_translate (ctx, arg(0), arg(1) );
        break;
      case CTX_ROTATE:
        ctx_rotate (ctx, arg(0) );
        break;
      case CTX_FONT:
        ctx_font (ctx, (char *) parser->holding);
        break;

      case CTX_TEXT:
        if (parser->n_numbers == 1)
          { ctx_rel_move_to (ctx, -parser->numbers[0], 0.0); }  //  XXX : scale by font(size)
        else
          {
            for (char *c = (char *) parser->holding; c; )
              {
                char *next_nl = ctx_strchr (c, '\n');
                if (next_nl)
                  { *next_nl = 0; }
                /* do our own layouting on a per-word basis?, to get justified
                 * margins? then we'd want explict margins rather than the
                 * implicit ones from move_to's .. making move_to work within
                 * margins.
                 */
                ctx_text (ctx, c);

                if (next_nl)
                  {
                    *next_nl = '\n'; // swap it newline back in
                    ctx_move_to (ctx, parser->left_margin, ctx_y (ctx) +
                                 ctx_get_font_size (ctx) );
                    c = next_nl + 1;
                    if (c[0] == 0)
                      { c = NULL; }
                  }
                else
                  {
                    c = NULL;
                  }
              }
          }
          parser->command = CTX_TEXT;
        break;
      case CTX_REL_LINE_TO:
        ctx_rel_line_to (ctx, arg(0), arg(1) );
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rel_move_to (ctx, arg(0), arg(1) );
        parser->command = CTX_REL_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        parser->left_margin = ctx_x (ctx);
        break;
      case CTX_LINE_WIDTH:
        ctx_line_width (ctx, arg(0));
        break;
      case CTX_LINE_DASH_OFFSET:
        ctx_line_dash_offset (ctx, arg(0));
        break;
      case CTX_STROKE_POS:
        ctx_stroke_pos (ctx, arg(0));
        break;
      case CTX_FEATHER:
        ctx_feather (ctx, arg(0));
        break;
      case CTX_LINE_HEIGHT:
        ctx_line_height (ctx, arg(0));
        break;
      case CTX_WRAP_LEFT:
        ctx_wrap_left (ctx, arg(0));
        break;
      case CTX_WRAP_RIGHT:
        ctx_wrap_right (ctx, arg(0));
        break;
      case CTX_IMAGE_SMOOTHING:
        ctx_image_smoothing (ctx, (int)arg(0));
        break;
      case CTX_SHADOW_COLOR:
        ctx_shadow_rgba (ctx, arg(0), arg(1), arg(2), arg(3));
        break;
      case CTX_SHADOW_BLUR:
        ctx_shadow_blur (ctx, arg(0) );
        break;
      case CTX_SHADOW_OFFSET_X:
        ctx_shadow_offset_x (ctx, arg(0) );
        break;
      case CTX_SHADOW_OFFSET_Y:
        ctx_shadow_offset_y (ctx, arg(0) );
        break;
      case CTX_LINE_JOIN:
        ctx_line_join (ctx, (CtxLineJoin) ctx_clamp (arg(0), 0, 2));
        break;
      case CTX_LINE_CAP:
        ctx_line_cap (ctx, (CtxLineCap) ctx_clamp (arg(0), 0, 2));
        break;
      case CTX_COMPOSITING_MODE:
	{
	  int val = (int)arg(0);
	  val = ctx_clamp (val, 0, CTX_COMPOSITE_LAST);
          ctx_compositing_mode (ctx, (CtxCompositingMode) val );
	}
        break;
      case CTX_BLEND_MODE:
        {
          int blend_mode = (int)arg(0);
          if (blend_mode == CTX_COLOR) blend_mode = CTX_BLEND_COLOR;
	  blend_mode = ctx_clamp (blend_mode, 0, CTX_BLEND_LAST);
          ctx_blend_mode (ctx, (CtxBlend)blend_mode);
        }
        break;
      case CTX_EXTEND:
        ctx_extend (ctx, (CtxExtend)ctx_clamp(arg(0), 0, CTX_EXTEND_LAST));
        break;
      case CTX_FILL_RULE:
        ctx_fill_rule (ctx, (CtxFillRule) ctx_clamp(arg(0), 0, 1));
        break;
      case CTX_TEXT_ALIGN:
        ctx_text_align (ctx, (CtxTextAlign) ctx_clamp(arg(0), 0, CTX_TEXT_ALIGN_RIGHT));
        break;
      case CTX_TEXT_BASELINE:
        ctx_text_baseline (ctx, (CtxTextBaseline) ctx_clamp(arg(0), 0, CTX_TEXT_BASELINE_BOTTOM));
        break;
      case CTX_TEXT_DIRECTION:
        ctx_text_direction (ctx, (CtxTextDirection) ctx_clamp(arg(0), 0, CTX_TEXT_DIRECTION_RTL));
        break;
      case CTX_IDENTITY:
        ctx_identity (ctx);
        break;
      case CTX_RECTANGLE:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3));
        break;
      case CTX_FILL_RECT:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3));
        ctx_fill (ctx);
        break;
      case CTX_STROKE_RECT:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3));
        ctx_stroke (ctx);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_round_rectangle (ctx, arg(0), arg(1), arg(2), arg(3), arg(4));
        break;
      case CTX_VIEW_BOX:
	{
	  float x = arg(0);
	  float y = arg(1);
	  float w = arg(2);
	  float h = arg(3);

	  if (w > 1 && h > 1)
	  {
            ctx_view_box (ctx, x, y, w, h);
            ctx_parser_set_size (parser, w, h, 0, 0);
	  }
	}
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_linear_gradient (ctx, arg(0), arg(1), arg(2), arg(3));
        break;
      case CTX_CONIC_GRADIENT:
	// TODO - default arg3 to 1 if unspecified
        ctx_conic_gradient (ctx, arg(0), arg(1), arg(2), arg(3));
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_radial_gradient (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        break;
      case CTX_GRADIENT_STOP:
        {
          float red, green, blue, alpha;
          ctx_parser_get_color_rgba (parser, 1, &red, &green, &blue, &alpha);
          ctx_gradient_add_stop (ctx, arg(0), red, green, blue, alpha);
        }
        break;
      case CTX_GLOBAL_ALPHA:
        ctx_global_alpha (ctx, arg(0) );
        break;
      case CTX_RESET_PATH:
        ctx_reset_path (ctx);
        break;
      case CTX_GLYPH:
        ctx_glyph (ctx, (uint32_t)arg(0), 0);
        break;
      case CTX_CLOSE_PATH:
        ctx_close_path (ctx);
        break;
      case CTX_EXIT: // XXX // should be END_FRAME ?
        if (parser->frame_done)
          { parser->frame_done (parser->frame_done_data);
            return;
          }
        break;
      case CTX_END_FRAME:
        //ctx_flush (ctx); // XXX  XXX  flush only does things inside backends
	//ctx_end_frame (ctx);
        // XXX - ignoring start frame works for now

	break;
      case CTX_START_FRAME: // XXX is it right to do things here?
        ctx_start_frame (ctx);
        if (parser->translate_origin)
        {
          ctx_translate (ctx,
                         (parser->cursor_x-1) * parser->cell_width * 1.0f,
                         (parser->cursor_y-1) * parser->cell_height * 1.0f);
        }
        break;
    }
#undef arg
}

static inline void ctx_parser_holding_append (CtxParser *parser, int byte)
{
#if !CTX_PARSER_FIXED_TEMP
  if (CTX_UNLIKELY(parser->hold_len < parser->pos + 1 + 1))
  {
    int new_len = parser->hold_len * 2;
    if (new_len < 512) new_len = 512;
    parser->holding = (uint8_t*)ctx_realloc (parser->holding, parser->hold_len, new_len);
    parser->hold_len = new_len;
  }
#endif

  parser->holding[parser->pos++]=byte;
#if CTX_PARSER_FIXED_TEMP
  if (CTX_UNLIKELY(parser->pos > (int) sizeof (parser->holding)-2))
    { parser->pos = sizeof (parser->holding)-2; }
#endif
  parser->holding[parser->pos]=0;
}

static void ctx_parser_transform_percent (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  int big   = parser->width;
  int small = parser->height;
  if (big < small)
    {
      small = parser->width;
      big   = parser->height;
    }
  switch (code)
    {
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
        switch (arg_no)
          {
            case 0:
            case 3:
              *value *= (parser->width/100.0f);
              break;
            case 1:
            case 4:
              *value *= (parser->height/100.0f);
              break;
            case 2:
            case 5:
              *value *= small/100.0f;
              break;
          }
        break;
      case CTX_STROKE_POS:
      case CTX_FEATHER:
      case CTX_FONT_SIZE:
      case CTX_MITER_LIMIT:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
        {
          *value *= (small/100.0f);
        }
        break;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
        if (arg_no > 3)
          {
            *value *= (small/100.0f);
          }
        else
          {
            if (arg_no % 2 == 0)
              { *value  *= ( (parser->width) /100.0f); }
            else
              { *value *= ( (parser->height) /100.0f); }
          }
        break;
      case CTX_ROUND_RECTANGLE:
        if (arg_no == 4)
        {
          { *value *= ((parser->height)/100.0f); }
          return;
        }
        /* FALLTHROUGH */
      default: // even means x coord
        if (arg_no % 2 == 0)
          { *value  *= ((parser->width)/100.0f); }
        else
          { *value *= ((parser->height)/100.0f); }
        break;
    }
}

static void ctx_parser_transform_percent_height (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  *value *= (parser->height/100.0f);
}

static void ctx_parser_transform_percent_width (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  *value *= (parser->height/100.0f);
}

static void ctx_parser_transform_cell (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  float small = parser->cell_width;
  if (small > parser->cell_height)
    { small = parser->cell_height; }
  switch (code)
    {
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
        switch (arg_no)
          {
            case 0:
            case 3:
              *value *= parser->cell_width;
              break;
            case 1:
            case 4:
              *value *= parser->cell_height;
              break;
            case 2:
            case 5:
              *value *= small; // use height?
              break;
          }
        break;
      case CTX_MITER_LIMIT:
      case CTX_FONT_SIZE:
      case CTX_STROKE_POS:
      case CTX_FEATHER:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
        {
          *value *= parser->cell_height;
        }
        break;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
        if (arg_no > 3)
          {
            *value *= small;
          }
        else
          {
            *value *= (arg_no%2==0) ?parser->cell_width:parser->cell_height;
          }
        break;
      case CTX_RECTANGLE:
        if (arg_no % 2 == 0)
          { *value *= parser->cell_width; }
        else
          {
            if (! (arg_no > 1) )
              { (*value) -= 1.0f; }
            *value *= parser->cell_height;
          }
        break;
      default: // even means x coord odd means y coord
        *value *= (arg_no%2==0) ?parser->cell_width:parser->cell_height;
        break;
    }
}

static void ctx_parser_word_done (CtxParser *parser)
{
  parser->holding[parser->pos]=0;

  if (parser->pos > 1 && (parser->holding[0]=='Z' || 
			  parser->holding[0]=='z'))
  {
    ctx_close_path (parser->ctx);
    memmove (parser->holding, parser->holding+1, parser->pos-1);
    parser->pos--;
    ctx_parser_word_done (parser);
    return;
  }

  //int old_args = parser->expected_args;
  int command = ctx_parser_resolve_command (parser, parser->holding);
  if ((command >= 0 && command < 32)
      || (command > 150) || (command < 0)
      )  // special case low enum values
    {                   // and enum values too high to be
                        // commands - permitting passing words
                        // for strings in some cases
      parser->numbers[parser->n_numbers] = command;

      // trigger transition from number
      parser->state = CTX_PARSER_NUMBER;
      char c = ',';
      ctx_parser_feed_bytes (parser, &c, 1);
    }
  else if (command > 0)
    {
#if 0
      if (old_args == CTX_ARG_COLLECT_NUMBERS ||
          old_args == CTX_ARG_STRING_OR_NUMBER)
      {
        int tmp1 = parser->command;
        int tmp2 = parser->expected_args;
        int tmp3 = parser->n_numbers;
 //     int tmp4 = parser->n_args;
        ctx_parser_dispatch_command (parser);
        parser->command = (CtxCode)tmp1;
        parser->expected_args = tmp2;
        parser->n_numbers = tmp3;
 //     parser->n_args = tmp4;
      }
#endif

      parser->command = (CtxCode) command;
      parser->n_numbers = 0;
      parser->n_args = 0;
      if (parser->expected_args == 0)
        {
          ctx_parser_dispatch_command (parser);
        }
      //parser->numbers[0] = 0;
    }
  else
    { 
#if 0
      uint8_t buf[16]=" ";
      for (int i = 0; parser->pos && parser->holding[i] > ' '; i++)
        {
          buf[0] = parser->holding[i];
          parser->command = (CtxCode) ctx_parser_resolve_command (parser, buf);
          parser->n_numbers = 0;
          parser->n_args = 0;
          if (parser->command > 0)
            {
              if (parser->expected_args == 0)
                {
                  ctx_parser_dispatch_command (parser);
                }
            }
          else
            {
              ctx_log ("unhandled command '%c'\n", buf[0]);
            }
        }
#endif
        fprintf (stderr, "unhandled command '%s'\n", parser->holding);
    }
}

static void ctx_parser_string_done (CtxParser *parser)
{
  if (parser->expected_args == CTX_ARG_STRING_OR_NUMBER)
  {
          /*
    if (parser->state != CTX_PARSER_NUMBER &&
        parser->state != CTX_PARSER_NEGATIVE_NUMBER &&
        parser->state != CTX_PARSER_STRING_A85 &&
        parser->state != CTX_PARSER_STRING_APOS &&
        parser->state != CTX_PARSER_STRING_QUOT
        )
        */
    {
    int tmp1 = parser->command;
    int tmp2 = parser->expected_args;
    int tmp3 = parser->n_numbers;
    int tmp4 = parser->n_args;
    ctx_parser_dispatch_command (parser);
    parser->command = (CtxCode)tmp1;
    parser->expected_args = tmp2;
    parser->n_numbers = tmp3;
    parser->n_args = tmp4;
    }
  }
  else
  {
    ctx_parser_dispatch_command (parser);
  }
}
static inline void ctx_parser_finish_number (CtxParser *parser)
{
  if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
     { parser->numbers[parser->n_numbers] *= -1; }
  if (parser->exp > 100) parser->exp = 100;
  if (parser->exponent < 0)
  {
    for (int i = 0; i < parser->exp; i++)
     parser->numbers[parser->n_numbers] *= 0.1f;
  }
  else if (parser->exponent > 0)
  {
    for (int i = 0; i < parser->exp; i++)
     parser->numbers[parser->n_numbers] *= 10.0f;
  }
  parser->exponent = 0;
}

static inline void ctx_parser_feed_byte (CtxParser *parser, char byte)
{
#if CTX_REPORT_COL_ROW
    if (CTX_UNLIKELY(byte == '\n'))
    {
        parser->col=0;
        parser->line++;
    }
    else
    {
        parser->col++;
    }
#endif

  switch (parser->state)
    {

    case CTX_PARSER_STRING_YENC:
    {
        if (CTX_UNLIKELY((parser->prev_byte == '=') && (byte == 'y')))
        {
          parser->state = CTX_PARSER_NEUTRAL;
                 //   fprintf (stderr, "got %i\n", parser->pos);
          parser->pos = ctx_ydec ((char*)parser->holding, (char*)parser->holding, parser->pos) - 1;
#if 0
          if (parser->pos > 5)
                    fprintf (stderr, "dec got %i %c %c %c %c\n", parser->pos,
                                    parser->holding[0],
                                    parser->holding[1],
                                    parser->holding[2],
                                    parser->holding[3]
                                    );
#endif
          ctx_parser_string_done (parser);
        }
        else
        {
          ctx_parser_holding_append (parser, byte);
        }
        parser->prev_byte = byte;
        return;
    }

    case CTX_PARSER_STRING_A85:
    {
        /* since these are our largest bulk transfers, minimize
         * overhead for this case. */
        if (CTX_LIKELY(byte!='~')) 
        {
          ctx_parser_holding_append (parser, byte);
        }
        else
        {
          parser->state = CTX_PARSER_NEUTRAL;
                 //   fprintf (stderr, "got %i\n", parser->pos);
          parser->pos = ctx_a85dec ((char*)parser->holding, (char*)parser->holding, parser->pos);
                 //   fprintf (stderr, "dec got %i\n", parser->pos);
          ctx_parser_string_done (parser);
        }
        return;
    }


      case CTX_PARSER_NEUTRAL:
        switch (byte)
          {
            case  0: case  1: case  2: case  3:  case 4:  case 5:
            case  6: case  7: case  8: case 11: case 12: case 14:
            case 15: case 16: case 17: case 18: case 19: case 20:
            case 21: case 22: case 23: case 24: case 25: case 26:
            case 27: case 28: case 29: case 30: case 31:
              break;
            case ' ': case '\t': case '\r': case '\n':
            case ';': case ',':
            case '(': case ')':
            case '{': case '}':
            //case '=':
              break;
            case '#':
              parser->state = CTX_PARSER_COMMENT;
              break;
            case '\'':
              parser->state = CTX_PARSER_STRING_APOS;
              parser->pos = 0;
              parser->holding[0] = 0;
              break;
            case '=':
              parser->state = CTX_PARSER_STRING_YENC;
              parser->pos = 0;
              parser->holding[0] = 0;
              break;
            case '~':
              parser->state = CTX_PARSER_STRING_A85;
              parser->pos = 0;
              parser->holding[0] = 0;
              break;
            case '"':
              parser->state = CTX_PARSER_STRING_QUOT;
              parser->pos = 0;
              parser->holding[0] = 0;
              break;
            case '-':
              parser->state = CTX_PARSER_NEGATIVE_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
	      parser->exponent =
              parser->decimal = 0;
              break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->numbers[parser->n_numbers] += (byte - '0');
	      parser->exponent =
              parser->decimal = 0;
              break;
            case '.':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
	      parser->exponent = 0;
              parser->decimal = 1;
              break;
            default:
              parser->state = CTX_PARSER_WORD;
              parser->pos = 0;
              ctx_parser_holding_append (parser, byte);
              break;
          }
        break;
      case CTX_PARSER_NUMBER:
      case CTX_PARSER_NEGATIVE_NUMBER:
        {
	  int do_process = 0;
          switch (byte)
            {
              case 0: case 1: case 2: case 3: case 4: case 5:
              case 6: case 7: case 8:
              case 11: case 12: case 14: case 15: case 16:
              case 17: case 18: case 19: case 20: case 21:
              case 22: case 23: case 24: case 25: case 26:
              case 27: case 28: case 29: case 30: case 31:
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case ' ':
              case '\t':
              case '\r':
              case '\n':
              case ';':
              case ',':
              case '(':
              case ')':
              case '{':
              case '}':
              case '=':
                ctx_parser_finish_number (parser);
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '#':
                parser->state = CTX_PARSER_COMMENT;
                break;
              case '-':
		if (parser->exponent==1)
		{
		  parser->exponent = -1;
		}
		else
		{
                  ctx_parser_finish_number (parser);
                  parser->state = CTX_PARSER_NEGATIVE_NUMBER;
 		  if (parser->n_numbers < CTX_PARSER_MAX_ARGS)
                    parser->n_numbers ++;
                  parser->numbers[parser->n_numbers] = 0;
	          parser->exponent =
                  parser->decimal = 0;
		  do_process = 1;
		}
                break;
              case '.':
                if (parser->decimal){
                  ctx_parser_finish_number (parser);
                  parser->state = CTX_PARSER_NUMBER;
		  if (parser->n_numbers < CTX_PARSER_MAX_ARGS)
                    parser->n_numbers ++;
                  parser->numbers[parser->n_numbers] = 0;
		  do_process = 1;
		}
	        parser->exponent = 0;
                parser->decimal = 1;
                break;
              case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9':
		if (parser->exponent)
		{
		   parser->exp *= 10;
		   parser->exp += (byte - '0');
		}
		else if (parser->decimal)
                  {
                    parser->decimal *= 10;
                    parser->numbers[parser->n_numbers] += (byte - '0') / (1.0f * parser->decimal);
                  }
                else
                  {
                    parser->numbers[parser->n_numbers] *= 10;
                    parser->numbers[parser->n_numbers] += (byte - '0');
                  }
                break;
              case '@': // cells
                ctx_parser_finish_number (parser);
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_cell (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '%': // percent of width/height
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '^': // percent of height
                ctx_parser_finish_number (parser);
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent_height (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '~': // percent of width
                ctx_parser_finish_number (parser);
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent_width (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
	      case 'e':
	      case 'E':
		parser->exponent = 1;
		parser->exp = 0;
		break;
              default:
                ctx_parser_finish_number (parser);

                parser->state = CTX_PARSER_WORD;
                parser->pos = 0;
                ctx_parser_holding_append (parser, byte);
                break;
            }
          if (do_process ||
	       ((parser->state != CTX_PARSER_NUMBER) &&
               (parser->state != CTX_PARSER_NEGATIVE_NUMBER)))
            {
	      if (!do_process)
	      {
	        if (parser->n_numbers < CTX_PARSER_MAX_ARGS)
                  parser->n_numbers ++;
	      }

              if (parser->n_numbers == parser->expected_args ||
                  parser->expected_args == CTX_ARG_COLLECT_NUMBERS ||
                  parser->expected_args == CTX_ARG_STRING_OR_NUMBER)
                {
                  int tmp1 = parser->n_numbers;
                  int tmp2 = parser->n_args;
                  CtxCode tmp3 = parser->command;
                  int tmp4 = parser->expected_args;
                  ctx_parser_dispatch_command (parser);
                  parser->command = tmp3;
                  switch (parser->command)
                  {
                    case CTX_DEFINE_TEXTURE:
                    case CTX_TEXTURE:
                      parser->n_numbers = tmp1;
                      parser->n_args = tmp2;
                      break;
                          default:
                      parser->n_numbers = 0;
                      parser->n_args = 0;
		      parser->numbers[0] = parser->numbers[tmp1];
                      break;
                  }
                  parser->expected_args = tmp4;
                }
              //if (parser->n_numbers > CTX_PARSER_MAX_ARGS)
              //  { parser->n_numbers = CTX_PARSER_MAX_ARGS;
              //  }
            }
        }
        break;
      case CTX_PARSER_WORD:
        switch (byte)
          {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
            case 8: case 11: case 12: case 14: case 15: case 16: case 17:
            case 18: case 19: case 20: case 21: case 22: case 23: case 24:
            case 25: case 26: case 27: case 28: case 29: case 30: case 31:
            case ' ': case '\t': case '\r': case '\n':
            case ';': case ',':
            case '(': case ')': case '=': case '{': case '}':
              parser->state = CTX_PARSER_NEUTRAL;
              break;
            case '#':
              parser->state = CTX_PARSER_COMMENT;
              break;
            case '-':
              parser->state = CTX_PARSER_NEGATIVE_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
	      parser->exponent =
              parser->decimal = 0;
              break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->numbers[parser->n_numbers] += (byte - '0');
	      parser->exponent =
              parser->decimal = 0;
              break;
            case '.':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
	      parser->exponent = 0;
              parser->decimal = 1;
              break;
            default:
              ctx_parser_holding_append (parser, byte);
              break;
          }
        if (parser->state != CTX_PARSER_WORD)
          {
            ctx_parser_word_done (parser);
          }
        break;
#if 0
      case CTX_PARSER_STRING_A85:
        if (CTX_LIKELY(byte!='~'))
        {
          ctx_parser_holding_append (parser, byte);
        }
        else
        {
          parser->state = CTX_PARSER_NEUTRAL;
                 //   fprintf (stderr, "got %i\n", parser->pos);
          parser->pos = ctx_a85dec ((char*)parser->holding, (char*)parser->holding, parser->pos);
                 //   fprintf (stderr, "dec got %i\n", parser->pos);
          ctx_parser_string_done (parser);
        }
        break;
#endif
      case CTX_PARSER_STRING_APOS:
        switch (byte)
          {
            case '\\': parser->state = CTX_PARSER_STRING_APOS_ESCAPED; break;
            case '\'': parser->state = CTX_PARSER_NEUTRAL;
              ctx_parser_string_done (parser);
              break;
            default:
              ctx_parser_holding_append (parser, byte); break;
          }
        break;
      case CTX_PARSER_STRING_APOS_ESCAPED:
        switch (byte)
          {
            case '0': byte = '\0'; break;
            case 'b': byte = '\b'; break;
            case 'f': byte = '\f'; break;
            case 'n': byte = '\n'; break;
            case 'r': byte = '\r'; break;
            case 't': byte = '\t'; break;
            case 'v': byte = '\v'; break;
            default: break;
          }
        ctx_parser_holding_append (parser, byte);
        parser->state = CTX_PARSER_STRING_APOS;
        break;
      case CTX_PARSER_STRING_QUOT_ESCAPED:
        switch (byte)
          {
            case '0': byte = '\0'; break;
            case 'b': byte = '\b'; break;
            case 'f': byte = '\f'; break;
            case 'n': byte = '\n'; break;
            case 'r': byte = '\r'; break;
            case 't': byte = '\t'; break;
            case 'v': byte = '\v'; break;
            default: break;
          }
        ctx_parser_holding_append (parser, byte);
        parser->state = CTX_PARSER_STRING_QUOT;
        break;
      case CTX_PARSER_STRING_QUOT:
        switch (byte)
          {
            case '\\':
              parser->state = CTX_PARSER_STRING_QUOT_ESCAPED;
              break;
            case '"':
              parser->state = CTX_PARSER_NEUTRAL;
              ctx_parser_string_done (parser);
              break;
            default:
              ctx_parser_holding_append (parser, byte);
              break;
          }
        break;
      case CTX_PARSER_COMMENT:
        switch (byte)
          {
            case '\r':
            case '\n':
              parser->state = CTX_PARSER_NEUTRAL;
            default:
              break;
          }
        break;
    }
}

void ctx_parser_feed_bytes (CtxParser *parser, const char *data, int count)
{
  for (int i = 0; i < count; i++)
    ctx_parser_feed_byte (parser, data[i]);
}

CTX_EXPORT void
ctx_parse (Ctx *ctx, const char *string)
{
  if (!string)
    return;
  CtxParser *parser = ctx_parser_new (ctx, ctx_width(ctx),
                                           ctx_height(ctx),
                                           ctx_get_font_size(ctx),
                                           ctx_get_font_size(ctx),
                                           0, 0, NULL, NULL, NULL, NULL, NULL);
  ctx_parser_feed_bytes (parser, string, ctx_strlen (string));
  ctx_parser_feed_bytes (parser, " ", 1);
  ctx_parser_destroy (parser);
}

CTX_EXPORT void
ctx_parse_animation (Ctx *ctx, const char *string,
		     float *scene_elapsed_time, 
                     int *scene_no_p)
{
  float time = *scene_elapsed_time;
  int scene_no = *scene_no_p;
  CtxString *str = ctx_string_new ("");
  int in_var = 0;
  float scene_duration = 5.0f;

  int i;

//again:
  i = 0;

  // XXX : this doesn't work when there are [  or ('s in text

  int scene_pos = 0;
  int last_scene = 0;
  int scene_start = 0;
  int got_duration = 0;

  {
    int start = 0;
    for (; string[i]; i++)
    {
       if (!strncmp (&string[i], "newPage", 7))
       {
         if (scene_pos == scene_no)
	 {
            if (scene_duration < time)
            {
              scene_no ++;
              (*scene_no_p)++;
              *scene_elapsed_time = time = time- scene_duration;
	    }
	    else
	    {
	      scene_start = start;
	    }
	 }

	 scene_pos++;
	 last_scene = scene_pos;
	 start = i + 7;
	 scene_duration = 5.0f;
	 got_duration = 0;
       }

       if (!got_duration && !strncmp (&string[i], "duration ", 9))
       {
	 scene_duration = _ctx_parse_float (&string[i+9], NULL);
	 got_duration = 1;
       }
    }
  }
  i = scene_start;
  if (last_scene)
    last_scene --;
#if 0
  {
  int in_scene_marker = 0;
  float duration = -1;

  // go through the string,
  //
  // post:
  //   last_scene = highest scene seen
  //   i = byte offset of start of scene
  //   scene_duration = duration of current scene
  for (; string[i]; i++)
  {
    char p = string[i];
    if (in_scene_marker)
    {
       if (p == ']')
       {
          in_scene_marker = 0;
       //   printf ("scene: %i time: %f scene %i: %f\n", scene_no, time, scene_pos, duration);
          last_scene = scene_pos;
          if (scene_pos == scene_no)
          {
            scene_duration = duration;
            if (scene_duration < time)
            {
              scene_no ++;
              (*scene_no_p)++;
              *scene_elapsed_time = time = time- scene_duration;
            }
            else
            {
              break;
            }
          }
          scene_pos++;
       }
       else if (p>='0' && p<='9' && duration < 0)
       {
          duration = _ctx_parse_float (&string[i], NULL);
       }
    }
    else
    {
       if (p == '[')
       {
          in_scene_marker = 1;
          duration = -1;
       }
    }
  }
  }
#endif

  if (scene_no > last_scene)
  {
     scene_no = 0;
     (*scene_no_p) = 0;
     return;
     //goto again;
  }
  
  if (scene_no == 0 && last_scene==0 && string[i]==0)
    i=0;

#define MAX_KEY_FRAMES 64
  float keys[MAX_KEY_FRAMES];
  float values[MAX_KEY_FRAMES];
  int n_keys = 0;
  int smooth = 1; // default to catmull rom

  for (; string[i]; i++)
  {
    char p = string[i];
    if (in_var == 0)
    {
      if (!strncmp (&string[i], "newPage", 7))
        break;
      else if (p == '(')
      {
        in_var = 1;
        n_keys = 0;
      }
      else
      {
        ctx_string_append_byte (str, p);
      }
    }
    else
    {
      if (p == ')')
      {
        float resolved_val = -100000.0;
        float prev_val = 0;
        for (int i = 0; i < n_keys; i++)
        {
          float key = keys[i];
          float val = values[i];
          //printf ("%f=%f\n", key, val);
          if (key>=time && resolved_val <=-10000.0f)
          {
            if (smooth == 0) // linear interpolation
            {
              if (i == 0)
                resolved_val = val;
              else
                resolved_val = ctx_lerpf (values[i-1], val, 
                                (time-keys[i-1])/(key-keys[i-1]));
            }
            else
            {
              if (i == 0)
              {
                resolved_val = val;
              }
              else if (n_keys<=2)
              {
                resolved_val = ctx_lerpf (values[i-1], val, 
                                 (time-keys[i-1])/(key-keys[i-1]));
              } else if (i == 1)
              {
                resolved_val = ctx_catmull_rom_left (values[i-1], values[i],
                                 values[i+1],
                                 (time-keys[i-1])/(key-keys[i-1]));
              }
              else if (i > 1 && i+1 < n_keys)
              {
                resolved_val = ctx_catmull_rom (values[i-2], values[i-1],
                                 val, values[i+1],
                                 (time-keys[i-1])/(key-keys[i-1]));
              }
              else if (i >= 2 && i < n_keys)
              {
                resolved_val = ctx_catmull_rom_right (values[i-2], values[i-1],
                                 values[i],
                                 (time-keys[i-1])/(key-keys[i-1]));
              }
            }
          }
          prev_val = val;
        }
        if (resolved_val <= -100000.0f) resolved_val = prev_val;
        ctx_string_append_printf (str, "%f", (double)resolved_val);
        in_var = 0;
      }
      else if (p>='0' && p<='9')
      {
        char *sp = (char*)&string[i];
        char *ep = sp;
        float key      = _ctx_parse_float (sp, &ep);
        char *eq       = strchr (sp, '=');
        float val      = 0.0;

        if (eq)
           val = _ctx_parse_float (eq+1, &ep);

        keys[n_keys] = key;
	if (n_keys < MAX_KEY_FRAMES-1)
        values[n_keys++] = val;

        i+=(ep-sp)-1;
      }
      else if (p=='s')
      {
        smooth = 1;
      } else if (p=='l')
      {
        smooth = 0;
      }
      else
      {
        /* ignore */
      }

    }
  }

  /* we've now built up the frame, and parse
   * it with the regular parser
   */
  ctx_parse (ctx, str->str);
  ctx_string_free (str, 1);
}

#endif
