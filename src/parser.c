#include "ctx-split.h"
/* the parser comes in the end, nothing in ctx knows about the parser  */

#if CTX_PARSER

/* ctx parser, */

struct
  _CtxParser
{
  int        t_args; // total number of arguments seen for current command
  Ctx       *ctx;
  int        state;
  uint8_t    holding[CTX_PARSER_MAXLEN]; /*  */
  int        line; /*  for error reporting */
  int        col;  /*  for error reporting */
  int        pos;
  double     numbers[12];
  int        n_numbers;
  int        decimal;
  CtxCode    command;
  int        n_args;
  uint32_t   set_key_hash;
  float      pcx;
  float      pcy;
  int        color_components;
  int        color_model; // 1 gray 3 rgb 4 cmyk
  float      left_margin; // set by last user provided move_to
  int        width;       // <- maybe should be float
  int        height;
  float      cell_width;
  float      cell_height;
  int        cursor_x;    // <- leaking in from terminal
  int        cursor_y;

  void (*exit) (void *exit_data);
  void *exit_data;
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len);
  int   (*get_prop)(void *prop_data, const char *key, char **data, int *len);
  void *prop_data;
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
                 void (*exit) (void *exit_data),
                 void *exit_data
                )
{
  ctx_memset (parser, 0, sizeof (CtxParser) );
  parser->line             = 1;
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
  int   (*set_prop)(void *prop_data, uint32_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
  void  *prop_data,
  void (*exit) (void *exit_data),
  void *exit_data)
{
  return ctx_parser_init ( (CtxParser *) calloc (sizeof (CtxParser), 1),
                           ctx,
                           width, height,
                           cell_width, cell_height,
                           cursor_x, cursor_y, set_prop, get_prop, prop_data,
                           exit, exit_data);
}

void ctx_parser_free (CtxParser *parser)
{
  free (parser);
}

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
      case CTX_SET_RGB_SPACE:
      case CTX_SET_CMYK_SPACE:
      case CTX_SET_DCMYK_SPACE:
      case CTX_SET_DRGB_SPACE:
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
      case CTX_RADIAL_GRADIENT:
        return 6;
      case CTX_TEXT_STROKE:
      case CTX_TEXT: // special case
      case CTX_SET:
      case CTX_GET:
        return 100; /* 100 is a special value,
                   which means string|number accepted
                 */
      //case CTX_SET_KEY:
      case CTX_COLOR:
      case CTX_SHADOW_COLOR:
        return 200;  /* 200 means number of components */
      case CTX_GRADIENT_STOP:
        return 201;  /* 201 means number of components+1 */
      case CTX_FUNCTION: /* special interpretation   */
        return 300;
        default:
#if 1
      case CTX_TEXTURE:
        case CTX_SET_RGBA_U8:
        case CTX_BITPIX:
        case CTX_BITPIX_DATA:
        case CTX_NOP:
        case CTX_NEW_EDGE:
        case CTX_EDGE:
        case CTX_EDGE_FLIPPED:
        case CTX_REPEAT_HISTORY:
        case CTX_CONT:
        case CTX_DATA:
        case CTX_DATA_REV:
        case CTX_DEFINE_GLYPH:
        case CTX_KERNING_PAIR:
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
#endif
        return 0;
    }
}

static int ctx_parser_set_command (CtxParser *parser, CtxCode code)
{
        //fprintf (stderr, "%i %s\n", code, parser->holding);
  if (code < 127 && code > 16)
  {
  parser->n_args = ctx_arguments_for_code (code);
  if (parser->n_args >= 200)
    {
      parser->n_args = (parser->n_args % 100) + parser->color_components;
    }
  //parser->t_args = 0;
#if 0
  else if (parser->n_args >= 100)
    {
      parser->n_args = (parser->n_args % 100);
    }
#endif
  }
  return code;
}

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model);

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
      case 'l': return ctx_parser_set_command (parser, CTX_MITER_LIMIT);
      case 't': return ctx_parser_set_command (parser, CTX_TEXT_ALIGN);
      case 'b': return ctx_parser_set_command (parser, CTX_TEXT_BASELINE);
      case 'd': return ctx_parser_set_command (parser, CTX_TEXT_DIRECTION);
      case 'j': return ctx_parser_set_command (parser, CTX_LINE_JOIN);
      case 'c': return ctx_parser_set_command (parser, CTX_LINE_CAP);
      case 'w': return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
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
      str_hash = ctx_strhash ( (char *) str, 0);
      switch (str_hash)
        {
#define CTX_ENABLE_DEFUN 1
#if CTX_ENABLE_DEFUN
#define CTX_function CTX_STRH('f','u','n','c','t','i','o','n',0,0,0,0,0,0)
#define CTX_endfun CTX_STRH('e','n','d','f','u','n',0,0,0,0,0,0,0,0)

          case CTX_function:  ret = CTX_FUNCTION; break;
          //case CTX_endfun:    ret = CTX_ENDFUN; break;
#endif
          /* first a list of mappings to one_char hashes, handled in a
           * separate fast path switch without hashing
           */
          case CTX_arcTo:         ret = CTX_ARC_TO; break;
          case CTX_arc:            ret = CTX_ARC; break;
          case CTX_curveTo:       ret = CTX_CURVE_TO; break;
          case CTX_setkey:         ret = CTX_SET; parser->t_args=0;break;
          case CTX_getkey:         ret = CTX_GET; break;
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
          case CTX_viewBox:       ret = CTX_VIEW_BOX; break;
          case CTX_smooth_to:      ret = CTX_SMOOTH_TO; break;
          case CTX_smooth_quad_to: ret = CTX_SMOOTHQ_TO; break;

          case CTX_clear:          ret = CTX_COMPOSITE_CLEAR; break;
          case CTX_copy:           ret = CTX_COMPOSITE_COPY; break;
          case CTX_destinationOver: ret = CTX_COMPOSITE_DESTINATION_OVER; break;

          case CTX_destinationIn:
                                    ret = CTX_COMPOSITE_DESTINATION_IN; break;
          case CTX_destinationOut:
                                    ret = CTX_COMPOSITE_DESTINATION_OUT; break;
          case CTX_sourceOver:
                                     ret = CTX_COMPOSITE_SOURCE_OVER; break;
          case CTX_sourceAtop:       ret = CTX_COMPOSITE_SOURCE_ATOP; break;
          case CTX_destinationAtop: ret = CTX_COMPOSITE_DESTINATION_ATOP; break;
          case CTX_sourceOut:       ret = CTX_COMPOSITE_SOURCE_OUT; break;
          case CTX_sourceIn:      ret = CTX_COMPOSITE_SOURCE_IN; break;
          case CTX_xor:            ret = CTX_COMPOSITE_XOR; break;
          case CTX_darken:         ret = CTX_BLEND_DARKEN; break;
          case CTX_lighten:        ret = CTX_BLEND_LIGHTEN; break;
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
          case CTX_verLineTo:  ret = CTX_VER_LINE_TO; break;
          case CTX_exit:
          case CTX_done:           ret = CTX_EXIT; break;
          case CTX_closePath:      ret =  CTX_CLOSE_PATH; break;
          case CTX_beginPath:
          case CTX_newPath:        ret =  CTX_BEGIN_PATH; break;
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
          case CTX_textStroke:     ret = CTX_TEXT_STROKE; break;
          case CTX_relVerLineTo:   ret = CTX_REL_VER_LINE_TO; break;
          case CTX_text:           ret = CTX_TEXT; break;
          case CTX_identity:       ret = CTX_IDENTITY; break;
          case CTX_transform:      ret = CTX_APPLY_TRANSFORM; break;
          case CTX_texture:        ret = CTX_TEXTURE; break;
          case CTX_rgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_RGB_SPACE);
          case CTX_cmykSpace:
            return ctx_parser_set_command (parser, CTX_SET_CMYK_SPACE);
          case CTX_drgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_DRGB_SPACE);
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
          case CTX_lineWidth:
          case CTX_setLineWidth:
            return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
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
          /* strings are handled directly here,
           * instead of in the one-char handler, using return instead of break
           */
          case CTX_gray:
            ctx_parser_set_color_model (parser, CTX_GRAY);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_graya:
            ctx_parser_set_color_model (parser, CTX_GRAYA);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgb:
            ctx_parser_set_color_model (parser, CTX_RGB);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgb:
            ctx_parser_set_color_model (parser, CTX_DRGB);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgba:
            ctx_parser_set_color_model (parser, CTX_RGBA);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgba:
            ctx_parser_set_color_model (parser, CTX_DRGBA);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmyk:
            ctx_parser_set_color_model (parser, CTX_CMYK);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmyka:
            ctx_parser_set_color_model (parser, CTX_CMYKA);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lab:
            ctx_parser_set_color_model (parser, CTX_LAB);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_laba:
            ctx_parser_set_color_model (parser, CTX_LABA);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lch:
            ctx_parser_set_color_model (parser, CTX_LCH);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lcha:
            ctx_parser_set_color_model (parser, CTX_LCHA);
            return ctx_parser_set_command (parser, CTX_COLOR);
          /* words that correspond to low integer constants
          */
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
#undef STR
#undef LOWER
          default:
            ret = str_hash;
        }
    }
  if (ret == CTX_CLOSE_PATH2)
    { ret = CTX_CLOSE_PATH; }
  /* handling single char, and ret = foo; break;  in cases above*/
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
} CTX_STATE;

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model)
{
  parser->color_model      = color_model;
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
      case CTX_GRAY:
        *red = *green = *blue = parser->numbers[offset + 0];
        break;
      default:
      case CTX_LABA: // NYI - needs RGB profile
      case CTX_LCHA: // NYI - needs RGB profile
      case CTX_RGBA:
        *alpha = parser->numbers[offset + 3];
      case CTX_LAB: // NYI
      case CTX_LCH: // NYI
      case CTX_RGB:
        *red = parser->numbers[offset + 0];
        *green = parser->numbers[offset + 1];
        *blue = parser->numbers[offset + 2];
        break;
      case CTX_CMYKA:
        *alpha = parser->numbers[offset + 4];
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

static void ctx_parser_set (CtxParser *parser, uint32_t key_hash,
                            const char *value, int val_len)
{
  if (parser->set_prop)
  {
     parser->set_prop (parser->prop_data, key_hash, value, val_len);
  }
}

char *ctx_parser_get (CtxParser *parser, const char *key)
{
  char *ret = NULL;
  int len = 0;
  if (parser->get_prop)
  {
     parser->get_prop (parser->prop_data, key, &ret, &len);
  }
  return ret;
}

static void ctx_parser_dispatch_command (CtxParser *parser)
{
  CtxCode cmd = parser->command;
  Ctx *ctx = parser->ctx;
  if (parser->n_args != 100 &&
      parser->n_args != parser->n_numbers)
    {
      ctx_log ("ctx:%i:%i %c got %i instead of %i args\n",
               parser->line, parser->col,
               cmd, parser->n_numbers, parser->n_args);
    }
#define arg(a)  (parser->numbers[a])
  parser->command = CTX_NOP;
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
      case CTX_RESTORE:
        ctx_restore (ctx);
        break;
#if CTX_ENABLE_CM
      case CTX_SET_DRGB_SPACE:
        ctx_set_drgb_space (ctx, arg (0) );
        break;
      case CTX_SET_DCMYK_SPACE:
        ctx_set_dcmyk_space (ctx, arg (0) );
        break;
      case CTX_SET_RGB_SPACE:
        ctx_rgb_space (ctx, arg (0) );
        break;
      case CTX_SET_CMYK_SPACE:
        ctx_set_cmyk_space (ctx, arg (0) );
        break;
#endif
      case CTX_COLOR:
        {
          switch (parser->color_model)
            {
              case CTX_GRAY:
                ctx_gray (ctx, arg (0) );
                break;
              case CTX_GRAYA:
                ctx_rgba (ctx, arg (0), arg (0), arg (0), arg (1) );
                break;
              case CTX_RGB:
                ctx_rgb (ctx, arg (0), arg (1), arg (2) );
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYK:
                ctx_cmyk (ctx, arg (0), arg (1), arg (2), arg (3) );
                break;
              case CTX_CMYKA:
                ctx_cmyka (ctx, arg (0), arg (1), arg (2), arg (3), arg (4) );
                break;
#else
              /* when there is no cmyk support at all in rasterizer
               * do a naive mapping to RGB on input.
               */
              case CTX_CMYK:
              case CTX_CMYKA:
                {
                  float r,g,b,a = 1.0f;
                  ctx_cmyk_to_rgb (arg (0), arg (1), arg (2), arg (3), &r, &g, &b);
                  if (parser->color_model == CTX_CMYKA)
                    { a = arg (4); }
                  ctx_rgba (ctx, r, g, b, a);
                }
                break;
#endif
              case CTX_RGBA:
                ctx_rgba (ctx, arg (0), arg (1), arg (2), arg (3) );
                break;
              case CTX_DRGB:
                ctx_drgba (ctx, arg (0), arg (1), arg (2), 1.0);
                break;
              case CTX_DRGBA:
                ctx_drgba (ctx, arg (0), arg (1), arg (2), arg (3) );
                break;
            }
        }
        break;
      case CTX_ARC_TO:
        ctx_arc_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4) );
        break;
      case CTX_REL_ARC_TO:
        ctx_rel_arc_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4) );
        break;
      case CTX_REL_SMOOTH_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          float ax = 2 * ctx_x (ctx) - cx;
          float ay = 2 * ctx_y (ctx) - cy;
          ctx_curve_to (ctx, ax, ay, arg (0) +  cx, arg (1) + cy,
                        arg (2) + cx, arg (3) + cy);
          parser->pcx = arg (0) + cx;
          parser->pcy = arg (1) + cy;
        }
        break;
      case CTX_SMOOTH_TO:
        {
          float ax = 2 * ctx_x (ctx) - parser->pcx;
          float ay = 2 * ctx_y (ctx) - parser->pcy;
          ctx_curve_to (ctx, ax, ay, arg (0), arg (1),
                        arg (2), arg (3) );
          parser->pcx = arg (0);
          parser->pcx = arg (1);
        }
        break;
      case CTX_SMOOTHQ_TO:
        ctx_quad_to (ctx, parser->pcx, parser->pcy, arg (0), arg (1) );
        break;
      case CTX_REL_SMOOTHQ_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          parser->pcx = 2 * ctx_x (ctx) - parser->pcx;
          parser->pcy = 2 * ctx_y (ctx) - parser->pcy;
          ctx_quad_to (ctx, parser->pcx, parser->pcy, arg (0) +  cx, arg (1) + cy);
        }
        break;
      case CTX_VER_LINE_TO:
        ctx_line_to (ctx, ctx_x (ctx), arg (0) );
        parser->command = CTX_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_HOR_LINE_TO:
        ctx_line_to (ctx, arg (0), ctx_y (ctx) );
        parser->command = CTX_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_HOR_LINE_TO:
        ctx_rel_line_to (ctx, arg (0), 0.0f);
        parser->command = CTX_REL_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_VER_LINE_TO:
        ctx_rel_line_to (ctx, 0.0f, arg (0) );
        parser->command = CTX_REL_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_ARC:
        ctx_arc (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        break;
      case CTX_APPLY_TRANSFORM:
        ctx_apply_transform (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        break;
      case CTX_CURVE_TO:
        ctx_curve_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        parser->pcx = arg (2);
        parser->pcy = arg (3);
        parser->command = CTX_CURVE_TO;
        break;
      case CTX_REL_CURVE_TO:
        parser->pcx = arg (2) + ctx_x (ctx);
        parser->pcy = arg (3) + ctx_y (ctx);
        ctx_rel_curve_to (ctx, arg (0), arg (1), arg (2), arg (3), arg (4), arg (5) );
        parser->command = CTX_REL_CURVE_TO;
        break;
      case CTX_LINE_TO:
        ctx_line_to (ctx, arg (0), arg (1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg (0);
        parser->pcy = arg (1);
        break;
      case CTX_MOVE_TO:
        ctx_move_to (ctx, arg (0), arg (1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg (0);
        parser->pcy = arg (1);
        parser->left_margin = parser->pcx;
        break;
      case CTX_FONT_SIZE:
        ctx_font_size (ctx, arg (0) );
        break;
      case CTX_MITER_LIMIT:
        ctx_miter_limit (ctx, arg (0) );
        break;
      case CTX_SCALE:
        ctx_scale (ctx, arg (0), arg (1) );
        break;
      case CTX_QUAD_TO:
        parser->pcx = arg (0);
        parser->pcy = arg (1);
        ctx_quad_to (ctx, arg (0), arg (1), arg (2), arg (3) );
        parser->command = CTX_QUAD_TO;
        break;
      case CTX_REL_QUAD_TO:
        parser->pcx = arg (0) + ctx_x (ctx);
        parser->pcy = arg (1) + ctx_y (ctx);
        ctx_rel_quad_to (ctx, arg (0), arg (1), arg (2), arg (3) );
        parser->command = CTX_REL_QUAD_TO;
        break;
      case CTX_CLIP:
        ctx_clip (ctx);
        break;
      case CTX_TRANSLATE:
        ctx_translate (ctx, arg (0), arg (1) );
        break;
      case CTX_ROTATE:
        ctx_rotate (ctx, arg (0) );
        break;
      case CTX_FONT:
        ctx_font (ctx, (char *) parser->holding);
        break;
      case CTX_SET:
        parser->t_args++;
        if (parser->t_args % 2 == 1)
        {
           if (parser->n_numbers == 1)
           {
             parser->set_key_hash = parser->numbers[0];
           }
           else
           {
             parser->set_key_hash = ctx_strhash ((char*)parser->holding, 0);
           }
        }
        else
        {
           if (parser->set_prop)
             ctx_parser_set (parser, parser->set_key_hash, (char*)parser->holding, parser->pos);
           //else
             ctx_set (ctx, parser->set_key_hash, (char*)parser->holding, parser->pos);
        }
        parser->command = CTX_SET;
        break;
      case CTX_GET:
        {
        char *val = ctx_parser_get (parser, (char*)parser->holding);
        if (val)
          free (val);
        parser->command = CTX_GET;
        }
        break;

      case CTX_TEXT_STROKE:
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
                if (cmd == CTX_TEXT_STROKE)
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
        if (cmd == CTX_TEXT_STROKE)
          { parser->command = CTX_TEXT_STROKE; }
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
        ctx_line_width (ctx, arg(0) );
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
          ctx_blend_mode (ctx, blend_mode);
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
      case CTX_TEXTURE:
        ctx_texture (ctx, arg(0), arg(1), arg(2));
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
        ctx_translate (ctx,
                       (parser->cursor_x-1) * parser->cell_width * 1.0,
                       (parser->cursor_y-1) * parser->cell_height * 1.0);
        break;
    }
#undef arg
  parser->n_numbers = 0;
}

static void ctx_parser_holding_append (CtxParser *parser, int byte)
{
  parser->holding[parser->pos++]=byte;
  if (parser->pos > (int) sizeof (parser->holding)-2)
    { parser->pos = sizeof (parser->holding)-2; }
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
          { *value *= ( (parser->height) /100.0); }
          return;
        }
      default: // even means x coord
        if (arg_no % 2 == 0)
          { *value  *= ( (parser->width) /100.0); }
        else
          { *value *= ( (parser->height) /100.0); }
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

static void ctx_parser_word_done (CtxParser *parser)
{
  parser->holding[parser->pos]=0;
  int command = ctx_parser_resolve_command (parser, parser->holding);
  if ((command >= 0 && command < 16) 
      || (command > 127) || (command < 0)
      )  // special case low enum values
    {                   // and enum values too high to be
                        // commands - permitting passing words
                        // for strings in some cases
      parser->numbers[parser->n_numbers] = command;

      // trigger transition from number
      parser->state = CTX_PARSER_NUMBER;
      ctx_parser_feed_byte (parser, ',');
    }
  else if (command > 0)
    {
      parser->command = (CtxCode) command;
      if (parser->n_args == 0)
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
          if (parser->command > 0)
            {
              if (parser->n_args == 0)
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
  parser->n_numbers = 0;
}

static void ctx_parser_string_done (CtxParser *parser)
{
  ctx_parser_dispatch_command (parser);
}

void ctx_parser_feed_byte (CtxParser *parser, int byte)
{
  switch (byte)
    {
      case '\n':
        parser->col=0;
        parser->line++;
        break;
      default:
        parser->col++;
    }
  switch (parser->state)
    {
      case CTX_PARSER_NEUTRAL:
        switch (byte)
          {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 11: case 12: case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31:
              break;
            case ' ': case '\t': case '\r': case '\n':
            case ';': case ',':
            case '(': case ')':
            case '{': case '}':
            case '=':
              break;
            case '#':
              parser->state = CTX_PARSER_COMMENT;
              break;
            case '\'':
              parser->state = CTX_PARSER_STRING_APOS;
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
          int new_neg = 0;
          switch (byte)
            {
              case 0:
              case 1:
              case 2:
              case 3:
              case 4:
              case 5:
              case 6:
              case 7:
              case 8:
              case 11:
              case 12:
              case 14:
              case 15:
              case 16:
              case 17:
              case 18:
              case 19:
              case 20:
              case 21:
              case 22:
              case 23:
              case 24:
              case 25:
              case 26:
              case 27:
              case 28:
              case 29:
              case 30:
              case 31:
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
                parser->state = CTX_PARSER_NEGATIVE_NUMBER;
                new_neg = 1;
                parser->numbers[parser->n_numbers+1] = 0;
                parser->decimal = 0;
                break;
              case '.':
                //if (parser->decimal) // TODO permit .13.32.43 to equivalent to .12 .32 .43
                parser->decimal = 1;
                break;
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
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
          if ( (parser->state != CTX_PARSER_NUMBER &&
                parser->state != CTX_PARSER_NEGATIVE_NUMBER) || new_neg)
            {
              parser->n_numbers ++;
              //parser->t_args ++;
              if (parser->n_numbers == parser->n_args || parser->n_args == 100)
                {
                  ctx_parser_dispatch_command (parser);
                }
              if (parser->n_numbers > 10)
                { parser->n_numbers = 10; }
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
      case CTX_PARSER_STRING_APOS:
        switch (byte)
          {
            case '\\': parser->state = CTX_PARSER_STRING_APOS_ESCAPED; break;
            case '\'': parser->state = CTX_PARSER_NEUTRAL;
              ctx_parser_string_done (parser); break;
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

void ctx_parse (Ctx *ctx, const char *string)
{
  if (!string)
    return;
  CtxParser *parser = ctx_parser_new (ctx, ctx_width(ctx),
                                           ctx_height(ctx),
                                           ctx_get_font_size(ctx),
                                           ctx_get_font_size(ctx),
                                           0, 0, NULL, NULL, NULL, NULL, NULL);
  for (int i = 0; string[i]; i++)
     ctx_parser_feed_byte (parser, string[i]);
  ctx_parser_free (parser);
}

#endif
