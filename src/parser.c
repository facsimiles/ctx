#include "ctx-split.h"
/* the parser comes in the end, nothing in ctx knows about the parser  */

#if CTX_PARSER

/* ctx parser, */

#define CTX_ID_MAXLEN 64 // in use should not be more than 40!
                         // to offer headroom for multiplexing


#define CTX_REPORT_COL_ROW 0

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
  CtxCode    command;
  int        expected_args; /* low digits are literal higher values
                               carry special meaning */
  int        n_args;
  int        texture_done;
  uint8_t    texture_id[CTX_ID_MAXLEN]; // used in defineTexture only
  uint64_t   set_key_hash;
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

  void (*exit) (void *exit_data);
  void *exit_data;
  int   (*set_prop)(void *prop_data, uint64_t key, const char *data,  int len);
  int   (*get_prop)(void *prop_data, const char *key, char **data, int *len);
  void *prop_data;
  int   prev_byte;
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
  int   (*set_prop)(void *prop_data, uint64_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
                 void  *prop_data,
                 void (*exit) (void *exit_data),
                 void *exit_data
                )
{
  ctx_memset (parser, 0, sizeof (CtxParser) );
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
  parser->exit             = exit;
  parser->exit_data        = exit_data;
  parser->color_model      = CTX_RGBA;
  parser->color_stroke     = 0;
  parser->color_components = 4;
  parser->command          = CTX_MOVE_TO;
  parser->set_prop         = set_prop;
  parser->get_prop         = get_prop;
  parser->prop_data        = prop_data;
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
  int   (*set_prop)(void *prop_data, uint64_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
  void  *prop_data,
  void (*exit) (void *exit_data),
  void *exit_data)
{
  return ctx_parser_init ( (CtxParser *) ctx_calloc (sizeof (CtxParser), 1),
                           ctx,
                           width, height,
                           cell_width, cell_height,
                           cursor_x, cursor_y, set_prop, get_prop, prop_data,
                           exit, exit_data);
}

void ctx_parser_free (CtxParser *parser)
{
#if !CTX_PARSER_FIXED_TEMP
  if (parser->holding)
    free (parser->holding);
#endif
  free (parser);
}

#define CTX_ARG_COLLECT_NUMBERS             50
#define CTX_ARG_STRING_OR_NUMBER            100
#define CTX_ARG_NUMBER_OF_COMPONENTS        200
#define CTX_ARG_NUMBER_OF_COMPONENTS_PLUS_1 201

static int ctx_arguments_for_code (CtxCode code)
{
  switch (code)
    {
      case CTX_SAVE:
      case CTX_START_GROUP:
      case CTX_END_GROUP:
      case CTX_IDENTITY:
      case CTX_CLOSE_PATH:
      case CTX_BEGIN_PATH:
      case CTX_RESET:
      case CTX_FLUSH:
      case CTX_RESTORE:
      case CTX_STROKE:
      case CTX_FILL:
      case CTX_NEW_PAGE:
      case CTX_CLIP:
      case CTX_EXIT:
        return 0;
      case CTX_GLOBAL_ALPHA:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
      case CTX_FONT_SIZE:
      case CTX_LINE_JOIN:
      case CTX_LINE_CAP:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
      case CTX_IMAGE_SMOOTHING:
      case CTX_SHADOW_BLUR:
      case CTX_SHADOW_OFFSET_X:
      case CTX_SHADOW_OFFSET_Y:
      case CTX_FILL_RULE:
      case CTX_TEXT_ALIGN:
      case CTX_TEXT_BASELINE:
      case CTX_TEXT_DIRECTION:
      case CTX_MITER_LIMIT:
      case CTX_REL_VER_LINE_TO:
      case CTX_REL_HOR_LINE_TO:
      case CTX_HOR_LINE_TO:
      case CTX_VER_LINE_TO:
      case CTX_FONT:
      case CTX_ROTATE:
      case CTX_GLYPH:
        return 1;
      case CTX_TRANSLATE:
      case CTX_REL_SMOOTHQ_TO:
      case CTX_LINE_TO:
      case CTX_MOVE_TO:
      case CTX_SCALE:
      case CTX_REL_LINE_TO:
      case CTX_REL_MOVE_TO:
      case CTX_SMOOTHQ_TO:
        return 2;
      case CTX_LINEAR_GRADIENT:
      case CTX_REL_QUAD_TO:
      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_FILL_RECT:
      case CTX_STROKE_RECT:
      case CTX_REL_SMOOTH_TO:
      case CTX_VIEW_BOX:
      case CTX_SMOOTH_TO:
        return 4;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_ROUND_RECTANGLE:
        return 5;
      case CTX_ARC:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_APPLY_TRANSFORM:
      case CTX_SOURCE_TRANSFORM:
      case CTX_RADIAL_GRADIENT:
        return 6;
      case CTX_STROKE_TEXT:
      case CTX_TEXT:
      case CTX_COLOR_SPACE:
      case CTX_DEFINE_GLYPH:
      case CTX_KERNING_PAIR:
      case CTX_TEXTURE:
      case CTX_DEFINE_TEXTURE:
        return CTX_ARG_STRING_OR_NUMBER;
      case CTX_LINE_DASH: /* append to current dashes for each argument encountered */
        return CTX_ARG_COLLECT_NUMBERS;
      //case CTX_SET_KEY:
      case CTX_COLOR:
      case CTX_SHADOW_COLOR:
        return CTX_ARG_NUMBER_OF_COMPONENTS;
      case CTX_GRADIENT_STOP:
        return CTX_ARG_NUMBER_OF_COMPONENTS_PLUS_1;

        default:
#if 1
        case CTX_SET_RGBA_U8:
        case CTX_NOP:
        case CTX_NEW_EDGE:
        case CTX_EDGE:
        case CTX_EDGE_FLIPPED:
        case CTX_CONT:
        case CTX_DATA:
        case CTX_DATA_REV:
        case CTX_SET_PIXEL:
        case CTX_REL_LINE_TO_X4:
        case CTX_REL_LINE_TO_REL_CURVE_TO:
        case CTX_REL_CURVE_TO_REL_LINE_TO:
        case CTX_REL_CURVE_TO_REL_MOVE_TO:
        case CTX_REL_LINE_TO_X2:
        case CTX_MOVE_TO_REL_LINE_TO:
        case CTX_REL_LINE_TO_REL_MOVE_TO:
        case CTX_FILL_MOVE_TO:
        case CTX_REL_QUAD_TO_REL_QUAD_TO:
        case CTX_REL_QUAD_TO_S16:
        case CTX_STROKE_SOURCE:
#endif
        return 0;
    }
}

static int ctx_parser_set_command (CtxParser *parser, CtxCode code)
{
  if (code < 150 && code >= 32)
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
  uint64_t ret = str[0]; /* if it is single char it already is the CtxCode */

  /* this is handled outside the hashing to make it possible to be case insensitive
   * with the rest.
   */
  if (str[0] == CTX_SET_KEY && str[1] && str[2] == 0)
  {
    switch (str[1])
    {
      case 'm': return ctx_parser_set_command (parser, CTX_COMPOSITING_MODE);
      case 'B': return ctx_parser_set_command (parser, CTX_BLEND_MODE);
      case 'l': return ctx_parser_set_command (parser, CTX_MITER_LIMIT);
      case 't': return ctx_parser_set_command (parser, CTX_TEXT_ALIGN);
      case 'b': return ctx_parser_set_command (parser, CTX_TEXT_BASELINE);
      case 'd': return ctx_parser_set_command (parser, CTX_TEXT_DIRECTION);
      case 'j': return ctx_parser_set_command (parser, CTX_LINE_JOIN);
      case 'c': return ctx_parser_set_command (parser, CTX_LINE_CAP);
      case 'w': return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
      case 'D': return ctx_parser_set_command (parser, CTX_LINE_DASH_OFFSET);
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
      uint64_t str_hash;
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
          case CTX_arcTo:          ret = CTX_ARC_TO; break;
          case CTX_arc:            ret = CTX_ARC; break;
          case CTX_curveTo:        ret = CTX_CURVE_TO; break;
          case CTX_restore:        ret = CTX_RESTORE; break;
          case CTX_stroke:         ret = CTX_STROKE; break;
          case CTX_fill:           ret = CTX_FILL; break;
          case CTX_flush:          ret = CTX_FLUSH; break;
          case CTX_horLineTo:      ret = CTX_HOR_LINE_TO; break;
          case CTX_rotate:         ret = CTX_ROTATE; break;
          case CTX_color:          ret = CTX_COLOR; break;
          case CTX_lineTo:         ret = CTX_LINE_TO; break;
          case CTX_moveTo:         ret = CTX_MOVE_TO; break;
          case CTX_scale:          ret = CTX_SCALE; break;
          case CTX_newPage:        ret = CTX_NEW_PAGE; break;
          case CTX_quadTo:         ret = CTX_QUAD_TO; break;
          case CTX_viewBox:        ret = CTX_VIEW_BOX; break;
          case CTX_smooth_to:      ret = CTX_SMOOTH_TO; break;
          case CTX_smooth_quad_to: ret = CTX_SMOOTHQ_TO; break;
          case CTX_clear:          ret = CTX_COMPOSITE_CLEAR; break;
          case CTX_copy:           ret = CTX_COMPOSITE_COPY; break;
          case CTX_destinationOver:  ret = CTX_COMPOSITE_DESTINATION_OVER; break;
          case CTX_destinationIn:    ret = CTX_COMPOSITE_DESTINATION_IN; break;
          case CTX_destinationOut:   ret = CTX_COMPOSITE_DESTINATION_OUT; break;
          case CTX_sourceOver:       ret = CTX_COMPOSITE_SOURCE_OVER; break;
          case CTX_sourceAtop:       ret = CTX_COMPOSITE_SOURCE_ATOP; break;
          case CTX_destinationAtop:  ret = CTX_COMPOSITE_DESTINATION_ATOP; break;
          case CTX_sourceOut:        ret = CTX_COMPOSITE_SOURCE_OUT; break;
          case CTX_sourceIn:         ret = CTX_COMPOSITE_SOURCE_IN; break;
          case CTX_xor:              ret = CTX_COMPOSITE_XOR; break;
          case CTX_darken:           ret = CTX_BLEND_DARKEN; break;
          case CTX_lighten:          ret = CTX_BLEND_LIGHTEN; break;
          //case CTX_color:          ret = CTX_BLEND_COLOR; break;
          //
          //  XXX check that he special casing for color works
          //      it is the first collision and it is due to our own
          //      color, not w3c for now unique use of it
          //
          case CTX_hue:            ret = CTX_BLEND_HUE; break;
          case CTX_multiply:       ret = CTX_BLEND_MULTIPLY; break;
          case CTX_normal:         ret = CTX_BLEND_NORMAL;break;
          case CTX_screen:         ret = CTX_BLEND_SCREEN;break;
          case CTX_difference:     ret = CTX_BLEND_DIFFERENCE; break;
          case CTX_reset:          ret = CTX_RESET; break;
          case CTX_verLineTo:      ret = CTX_VER_LINE_TO; break;
          case CTX_exit:
          case CTX_done:           ret = CTX_EXIT; break;
          case CTX_closePath:      ret = CTX_CLOSE_PATH; break;
          case CTX_beginPath:
          case CTX_newPath:        ret = CTX_BEGIN_PATH; break;
          case CTX_relArcTo:       ret = CTX_REL_ARC_TO; break;
          case CTX_clip:           ret = CTX_CLIP; break;
          case CTX_relCurveTo:     ret = CTX_REL_CURVE_TO; break;
          case CTX_startGroup:     ret = CTX_START_GROUP; break;
          case CTX_endGroup:       ret = CTX_END_GROUP; break;
          case CTX_save:           ret = CTX_SAVE; break;
          case CTX_translate:      ret = CTX_TRANSLATE; break;
          case CTX_linearGradient: ret = CTX_LINEAR_GRADIENT; break;
          case CTX_relHorLineTo:   ret = CTX_REL_HOR_LINE_TO; break;
          case CTX_relLineTo:      ret = CTX_REL_LINE_TO; break;
          case CTX_relMoveTo:      ret = CTX_REL_MOVE_TO; break;
          case CTX_font:           ret = CTX_FONT; break;
          case CTX_radialGradient:ret = CTX_RADIAL_GRADIENT; break;
          case CTX_gradientAddStop:
          case CTX_addStop:        ret = CTX_GRADIENT_STOP; break;
          case CTX_relQuadTo:      ret = CTX_REL_QUAD_TO; break;
          case CTX_rectangle:
          case CTX_rect:           ret = CTX_RECTANGLE; break;
          case CTX_roundRectangle: ret = CTX_ROUND_RECTANGLE; break;
          case CTX_relSmoothTo:    ret = CTX_REL_SMOOTH_TO; break;
          case CTX_relSmoothqTo:   ret = CTX_REL_SMOOTHQ_TO; break;
          case CTX_strokeText:     ret = CTX_STROKE_TEXT; break;
          case CTX_strokeRect:     ret = CTX_STROKE_RECT; break;
          case CTX_fillRect:       ret = CTX_FILL_RECT; break;
          case CTX_relVerLineTo:   ret = CTX_REL_VER_LINE_TO; break;
          case CTX_text:           ret = CTX_TEXT; break;
          case CTX_identity:       ret = CTX_IDENTITY; break;
          case CTX_transform:      ret = CTX_APPLY_TRANSFORM; break;
          case CTX_sourceTransform: ret = CTX_SOURCE_TRANSFORM; break;
          case CTX_texture:        ret = CTX_TEXTURE; break;
          case CTX_defineTexture:  ret = CTX_DEFINE_TEXTURE; break;
#if 0
          case CTX_rgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_RGB_SPACE);
          case CTX_cmykSpace:
            return ctx_parser_set_command (parser, CTX_SET_CMYK_SPACE);
          case CTX_drgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_DRGB_SPACE);
#endif
          case CTX_defineGlyph:
            return ctx_parser_set_command (parser, CTX_DEFINE_GLYPH);
          case CTX_kerningPair:
            return ctx_parser_set_command (parser, CTX_KERNING_PAIR);

          case CTX_colorSpace:
            return ctx_parser_set_command (parser, CTX_COLOR_SPACE);
          case CTX_fillRule:
            return ctx_parser_set_command (parser, CTX_FILL_RULE);
          case CTX_fontSize:
          case CTX_setFontSize:
            return ctx_parser_set_command (parser, CTX_FONT_SIZE);
          case CTX_compositingMode:
            return ctx_parser_set_command (parser, CTX_COMPOSITING_MODE);

          case CTX_blend:
          case CTX_blending:
          case CTX_blendMode:
            return ctx_parser_set_command (parser, CTX_BLEND_MODE);

          case CTX_miterLimit:
            return ctx_parser_set_command (parser, CTX_MITER_LIMIT);
          case CTX_textAlign:
            return ctx_parser_set_command (parser, CTX_TEXT_ALIGN);
          case CTX_textBaseline:
            return ctx_parser_set_command (parser, CTX_TEXT_BASELINE);
          case CTX_textDirection:
            return ctx_parser_set_command (parser, CTX_TEXT_DIRECTION);
          case CTX_join:
          case CTX_lineJoin:
          case CTX_setLineJoin:
            return ctx_parser_set_command (parser, CTX_LINE_JOIN);
          case CTX_glyph:
            return ctx_parser_set_command (parser, CTX_GLYPH);
          case CTX_cap:
          case CTX_lineCap:
          case CTX_setLineCap:
            return ctx_parser_set_command (parser, CTX_LINE_CAP);
          case CTX_lineDash:
            return ctx_parser_set_command (parser, CTX_LINE_DASH);
          case CTX_lineWidth:
          case CTX_setLineWidth:
            return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
          case CTX_lineDashOffset:
            return ctx_parser_set_command (parser, CTX_LINE_DASH_OFFSET);
          case CTX_imageSmoothing:
            return ctx_parser_set_command (parser, CTX_IMAGE_SMOOTHING);
          case CTX_shadowColor:
            return ctx_parser_set_command (parser, CTX_SHADOW_COLOR);
          case CTX_shadowBlur:
            return ctx_parser_set_command (parser, CTX_SHADOW_BLUR);
          case CTX_shadowOffsetX:
            return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_X);
          case CTX_shadowOffsetY:
            return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_Y);
          case CTX_globalAlpha:
            return ctx_parser_set_command (parser, CTX_GLOBAL_ALPHA);

          case CTX_strokeSource:
            return ctx_parser_set_command (parser, CTX_STROKE_SOURCE);

          /* strings are handled directly here,
           * instead of in the one-char handler, using return instead of break
           */
          case CTX_gray:
            ctx_parser_set_color_model (parser, CTX_GRAY, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_graya:
            ctx_parser_set_color_model (parser, CTX_GRAYA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgb:
            ctx_parser_set_color_model (parser, CTX_RGB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgb:
            ctx_parser_set_color_model (parser, CTX_DRGB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgba:
            ctx_parser_set_color_model (parser, CTX_RGBA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgba:
            ctx_parser_set_color_model (parser, CTX_DRGBA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmyk:
            ctx_parser_set_color_model (parser, CTX_CMYK, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmyka:
            ctx_parser_set_color_model (parser, CTX_CMYKA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lab:
            ctx_parser_set_color_model (parser, CTX_LAB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_laba:
            ctx_parser_set_color_model (parser, CTX_LABA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lch:
            ctx_parser_set_color_model (parser, CTX_LCH, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lcha:
            ctx_parser_set_color_model (parser, CTX_LCHA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);

          /* and a full repeat of the above, with S for Stroke suffix */
          case CTX_grayS:
            ctx_parser_set_color_model (parser, CTX_GRAY, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_grayaS:
            ctx_parser_set_color_model (parser, CTX_GRAYA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgbS:
            ctx_parser_set_color_model (parser, CTX_RGB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgbS:
            ctx_parser_set_color_model (parser, CTX_DRGB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgbaS:
            ctx_parser_set_color_model (parser, CTX_RGBA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgbaS:
            ctx_parser_set_color_model (parser, CTX_DRGBA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmykS:
            ctx_parser_set_color_model (parser, CTX_CMYK, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmykaS:
            ctx_parser_set_color_model (parser, CTX_CMYKA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_labS:
            ctx_parser_set_color_model (parser, CTX_LAB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_labaS:
            ctx_parser_set_color_model (parser, CTX_LABA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lchS:
            ctx_parser_set_color_model (parser, CTX_LCH, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lchaS:
            ctx_parser_set_color_model (parser, CTX_LCHA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);

          /* words that correspond to low integer constants
          */
          case CTX_nonzero:     return CTX_FILL_RULE_WINDING;
          case CTX_non_zero:    return CTX_FILL_RULE_WINDING;
          case CTX_winding:     return CTX_FILL_RULE_WINDING;
          case CTX_evenOdd:
          case CTX_even_odd:    return CTX_FILL_RULE_EVEN_ODD;
          case CTX_bevel:       return CTX_JOIN_BEVEL;
          case CTX_round:       return CTX_JOIN_ROUND;
          case CTX_miter:       return CTX_JOIN_MITER;
          case CTX_none:        return CTX_CAP_NONE;
          case CTX_square:      return CTX_CAP_SQUARE;
          case CTX_start:       return CTX_TEXT_ALIGN_START;
          case CTX_end:         return CTX_TEXT_ALIGN_END;
          case CTX_left:        return CTX_TEXT_ALIGN_LEFT;
          case CTX_right:       return CTX_TEXT_ALIGN_RIGHT;
          case CTX_center:      return CTX_TEXT_ALIGN_CENTER;
          case CTX_top:         return CTX_TEXT_BASELINE_TOP;
          case CTX_bottom :     return CTX_TEXT_BASELINE_BOTTOM;
          case CTX_middle:      return CTX_TEXT_BASELINE_MIDDLE;
          case CTX_alphabetic:  return CTX_TEXT_BASELINE_ALPHABETIC;
          case CTX_hanging:     return CTX_TEXT_BASELINE_HANGING;
          case CTX_ideographic: return CTX_TEXT_BASELINE_IDEOGRAPHIC;

          case CTX_userRGB:     return CTX_COLOR_SPACE_USER_RGB;
          case CTX_deviceRGB:   return CTX_COLOR_SPACE_DEVICE_RGB;
          case CTX_userCMYK:    return CTX_COLOR_SPACE_USER_CMYK;
          case CTX_deviceCMYK:  return CTX_COLOR_SPACE_DEVICE_CMYK;
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
  *alpha = 1.0;
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
        *red = (1.0-parser->numbers[offset + 0]) *
               (1.0 - parser->numbers[offset + 3]);
        *green = (1.0-parser->numbers[offset + 1]) *
                 (1.0 - parser->numbers[offset + 3]);
        *blue = (1.0-parser->numbers[offset + 2]) *
                (1.0 - parser->numbers[offset + 3]);
        break;
    }
}

static void ctx_parser_dispatch_command (CtxParser *parser)
{
  CtxCode cmd = parser->command;
  Ctx *ctx = parser->ctx;

  if (parser->expected_args != CTX_ARG_STRING_OR_NUMBER &&
      parser->expected_args != CTX_ARG_COLLECT_NUMBERS &&
      parser->expected_args != parser->n_numbers)
    {
#if CTX_REPORT_COL_ROW
         fprintf (stderr, "ctx:%i:%i %c got %i instead of %i args\n",
               parser->line, parser->col,
               cmd, parser->n_numbers, parser->expected_args);
#endif
      //return;
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
          parser->color_space_slot = (CtxColorSpace) arg(0);
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
            parser->numbers[2] = strtod ((char*)parser->holding, NULL);
            {
              CtxEntry e = {CTX_KERNING_PAIR, };
              e.data.u16[0] = parser->numbers[0];
              e.data.u16[1] = parser->numbers[1];
              e.data.s32[1] = parser->numbers[2] * 256;
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
             int width  = arg(0);
             int height = arg(1);
             CtxPixelFormat format = (CtxPixelFormat)arg(2);
             int stride = ctx_pixel_format_get_stride (format, width);
             int data_len = stride * height;
             if (format == CTX_FORMAT_YUV420)
                 data_len = height * width + 2*(height/2) * (width/2);


             if (parser->pos != data_len)
             {
             fprintf (stderr, "unexpected datasize for define texture %s %ix%i\n size:%i != expected:%i - start of data: %i %i %i %i\n", eid, width, height,
                               parser->pos,
                               stride * height,
                               parser->holding[0],
                               parser->holding[1],
                               parser->holding[2],
                               parser->holding[3]
                               );
             }
             else
             ctx_define_texture (ctx, eid, width, height, stride, format, parser->holding, NULL);
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


      case CTX_DEFINE_GLYPH:
        /* XXX : reuse n_args logic - to enforce order */
        if (parser->n_numbers == 1)
        {
          CtxEntry e = {CTX_DEFINE_GLYPH, };
          e.data.u32[0] = parser->color_space_slot;
          e.data.u32[1] = arg(0) * 256;
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
        ctx_arc_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4));
        break;
      case CTX_REL_ARC_TO:
        ctx_rel_arc_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4) );
        break;
      case CTX_REL_SMOOTH_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          float ax = 2 * ctx_x (ctx) - cx;
          float ay = 2 * ctx_y (ctx) - cy;
          ctx_curve_to (ctx, ax, ay, arg(0) +  cx, arg(1) + cy,
                        arg(2) + cx, arg(3) + cy);
          parser->pcx = arg(0) + cx;
          parser->pcy = arg(1) + cy;
        }
        break;
      case CTX_SMOOTH_TO:
        {
          float ax = 2 * ctx_x (ctx) - parser->pcx;
          float ay = 2 * ctx_y (ctx) - parser->pcy;
          ctx_curve_to (ctx, ax, ay, arg(0), arg(1),
                        arg(2), arg(3) );
          parser->pcx = arg(0);
          parser->pcx = arg(1);
        }
        break;
      case CTX_SMOOTHQ_TO:
        ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0), arg(1) );
        break;
      case CTX_REL_SMOOTHQ_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          parser->pcx = 2 * ctx_x (ctx) - parser->pcx;
          parser->pcy = 2 * ctx_y (ctx) - parser->pcy;
          ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0) +  cx, arg(1) + cy);
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
        ctx_arc (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        break;
      case CTX_APPLY_TRANSFORM:
        ctx_apply_transform (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        break;
      case CTX_SOURCE_TRANSFORM:
        ctx_source_transform (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
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

      case CTX_STROKE_TEXT:
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
                if (cmd == CTX_STROKE_TEXT)
                  { ctx_text_stroke (ctx, c); }
                else
                  { ctx_text (ctx, c); }
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
        if (cmd == CTX_STROKE_TEXT)
          { parser->command = CTX_STROKE_TEXT; }
        else
          { parser->command = CTX_TEXT; }
        break;
      case CTX_REL_LINE_TO:
        ctx_rel_line_to (ctx, arg(0), arg(1) );
        parser->pcx += arg(0);
        parser->pcy += arg(1);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rel_move_to (ctx, arg(0), arg(1) );
        parser->pcx += arg(0);
        parser->pcy += arg(1);
        parser->left_margin = ctx_x (ctx);
        break;
      case CTX_LINE_WIDTH:
        ctx_line_width (ctx, arg(0));
        break;
      case CTX_LINE_DASH_OFFSET:
        ctx_line_dash_offset (ctx, arg(0));
        break;
      case CTX_IMAGE_SMOOTHING:
        ctx_image_smoothing (ctx, arg(0));
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
        ctx_line_join (ctx, (CtxLineJoin) arg(0) );
        break;
      case CTX_LINE_CAP:
        ctx_line_cap (ctx, (CtxLineCap) arg(0) );
        break;
      case CTX_COMPOSITING_MODE:
        ctx_compositing_mode (ctx, (CtxCompositingMode) arg(0) );
        break;
      case CTX_BLEND_MODE:
        {
          int blend_mode = arg(0);
          if (blend_mode == CTX_COLOR) blend_mode = CTX_BLEND_COLOR;
          ctx_blend_mode (ctx, (CtxBlend)blend_mode);
        }
        break;
      case CTX_FILL_RULE:
        ctx_fill_rule (ctx, (CtxFillRule) arg(0) );
        break;
      case CTX_TEXT_ALIGN:
        ctx_text_align (ctx, (CtxTextAlign) arg(0) );
        break;
      case CTX_TEXT_BASELINE:
        ctx_text_baseline (ctx, (CtxTextBaseline) arg(0) );
        break;
      case CTX_TEXT_DIRECTION:
        ctx_text_direction (ctx, (CtxTextDirection) arg(0) );
        break;
      case CTX_IDENTITY:
        ctx_identity (ctx);
        break;
      case CTX_RECTANGLE:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3) );
        break;
      case CTX_FILL_RECT:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3) );
        ctx_fill (ctx);
        break;
      case CTX_STROKE_RECT:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3) );
        ctx_stroke (ctx);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_round_rectangle (ctx, arg(0), arg(1), arg(2), arg(3), arg(4));
        break;
      case CTX_VIEW_BOX:
        ctx_view_box (ctx, arg(0), arg(1), arg(2), arg(3) );
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_linear_gradient (ctx, arg(0), arg(1), arg(2), arg(3) );
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
      case CTX_BEGIN_PATH:
        ctx_begin_path (ctx);
        break;
      case CTX_GLYPH:
        ctx_glyph (ctx, arg(0), 0);
        break;
      case CTX_CLOSE_PATH:
        ctx_close_path (ctx);
        break;
      case CTX_EXIT:
        if (parser->exit)
          { parser->exit (parser->exit_data);
            return;
          }
        break;
      case CTX_FLUSH:
        //ctx_flush (ctx);
        break;
      case CTX_RESET:
        ctx_reset (ctx);
        if (parser->translate_origin)
        {
          ctx_translate (ctx,
                         (parser->cursor_x-1) * parser->cell_width * 1.0,
                         (parser->cursor_y-1) * parser->cell_height * 1.0);
        }
        break;
    }
#undef arg
//  parser->n_numbers = 0;
}

static inline void ctx_parser_holding_append (CtxParser *parser, int byte)
{
#if !CTX_PARSER_FIXED_TEMP
  if (CTX_UNLIKELY(parser->hold_len < parser->pos + 1 + 1))
  {
    int new_len = parser->hold_len * 2;
    if (new_len < 512) new_len = 512;
    parser->holding = (uint8_t*)realloc (parser->holding, new_len);
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
              *value *= (parser->width/100.0);
              break;
            case 1:
            case 4:
              *value *= (parser->height/100.0);
              break;
            case 2:
            case 5:
              *value *= small/100.0;
              break;
          }
        break;
      case CTX_FONT_SIZE:
      case CTX_MITER_LIMIT:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
        {
          *value *= (small/100.0);
        }
        break;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
        if (arg_no > 3)
          {
            *value *= (small/100.0);
          }
        else
          {
            if (arg_no % 2 == 0)
              { *value  *= ( (parser->width) /100.0); }
            else
              { *value *= ( (parser->height) /100.0); }
          }
        break;
      case CTX_ROUND_RECTANGLE:
        if (arg_no == 4)
        {
          { *value *= ((parser->height)/100.0); }
          return;
        }
        /* FALLTHROUGH */
      default: // even means x coord
        if (arg_no % 2 == 0)
          { *value  *= ((parser->width)/100.0); }
        else
          { *value *= ((parser->height)/100.0); }
        break;
    }
}

static void ctx_parser_transform_percent_height (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  *value *= (parser->height/100.0);
}

static void ctx_parser_transform_percent_width (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  *value *= (parser->height/100.0);
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

// %h %v %m %M

static void ctx_parser_number_done (CtxParser *parser)
{

}

static void ctx_parser_word_done (CtxParser *parser)
{
  parser->holding[parser->pos]=0;
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
    }
  else
    {
      /* interpret char by char */
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

    if (CTX_LIKELY(parser->state == CTX_PARSER_STRING_YENC))
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
    else if (CTX_LIKELY(parser->state == CTX_PARSER_STRING_A85))
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
  switch (parser->state)
    {
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
              parser->decimal = 0;
              break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->numbers[parser->n_numbers] += (byte - '0');
              parser->decimal = 0;
              break;
            case '.':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
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
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '#':
                parser->state = CTX_PARSER_COMMENT;
                break;
              case '-':
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                parser->state = CTX_PARSER_NEGATIVE_NUMBER;
                parser->numbers[parser->n_numbers+1] = 0;
                parser->n_numbers ++;
                parser->decimal = 0;
                break;
              case '.':
                //if (parser->decimal) // TODO permit .13.32.43 to equivalent to .12 .32 .43
                parser->decimal = 1;
                break;
              case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9':
                if (parser->decimal)
                  {
                    parser->decimal *= 10;
                    parser->numbers[parser->n_numbers] += (byte - '0') / (1.0 * parser->decimal);
                  }
                else
                  {
                    parser->numbers[parser->n_numbers] *= 10;
                    parser->numbers[parser->n_numbers] += (byte - '0');
                  }
                break;
              case '@': // cells
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
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
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent_height (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '~': // percent of width
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent_width (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              default:
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                parser->state = CTX_PARSER_WORD;
                parser->pos = 0;
                ctx_parser_holding_append (parser, byte);
                break;
            }
          if ( (parser->state != CTX_PARSER_NUMBER) &&
               (parser->state != CTX_PARSER_NEGATIVE_NUMBER))
            {
              parser->n_numbers ++;
              ctx_parser_number_done (parser);

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
                      break;
                  }
                  parser->expected_args = tmp4;
                }
              if (parser->n_numbers > CTX_PARSER_MAX_ARGS)
                { parser->n_numbers = CTX_PARSER_MAX_ARGS;
                }
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
              parser->decimal = 0;
              break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->numbers[parser->n_numbers] += (byte - '0');
              parser->decimal = 0;
              break;
            case '.':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
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

void ctx_parse (Ctx *ctx, const char *string)
{
  if (!string)
    return;
  CtxParser *parser = ctx_parser_new (ctx, ctx_width(ctx),
                                           ctx_height(ctx),
                                           ctx_get_font_size(ctx),
                                           ctx_get_font_size(ctx),
                                           0, 0, NULL, NULL, NULL, NULL, NULL);
  ctx_parser_feed_bytes (parser, string, strlen (string));
  ctx_parser_free (parser);
}

#endif
