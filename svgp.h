
void
svgp_init (SvgP *svgp,
  Ctx       *ctx,
  int        cw,
  int        ch,
  int        cursor_x,
  int        cursor_y,
  int        cols,
  int        rows,
  void (*exit)(void *exit_data),
  void *exit_data
          )
{
  svgp->ctx = ctx;
  svgp->cw = cw; // cell width
  svgp->ch = ch; // cell height
  svgp->cursor_x = cursor_x;
  svgp->cursor_y = cursor_y;
  svgp->cols = cols;
  svgp->rows = rows;

  svgp->exit = exit;
  svgp->exit_data = exit_data;

  svgp->command = 'm';
  svgp->n_numbers = 0;
  svgp->decimal = 0;
  svgp->pos = 0;
  svgp->holding[svgp->pos=0]=0;
}

/* almost all single char uppercase and lowercase ASCII get covered by
 * serializing the full canvas drawing API to SVG commands, we can still
 * extend this , with non-printable as well as values >127 .. to encode
 * new - possibly short keywords that do not have an ascii shorthand
 */

typedef enum {
  SVGP_NONE = 0,
  SVGP_ARC_TO          = 'A',  // SVG, NYI
  SVGP_ARC             = 'B',
  SVGP_CURVE_TO        = 'C',  // SVG
  SVGP_RESTORE         = 'D',
  SVGP_STROKE          = 'E',
  SVGP_FILL            = 'F',
  SVGP_HOR_LINE_TO     = 'H', // SVG
  //SVGP_UNUSED        = 'I',
  SVGP_ROTATE          = 'J',
  SVGP_SET_COLOR       = 'K',
  SVGP_LINE_TO         = 'L', // SVG
  SVGP_MOVE_TO         = 'M', // SVG
  SVGP_SET_FONT_SIZE   = 'N',
  SVGP_SCALE           = 'O',
  //SVGP_NEW_PAGE      = 'P', // NYI
  SVGP_QUAD_TO         = 'Q',
  //SVGP_UNUSED        = 'R',
  SVGP_SMOOTH_TO       = 'S', // SVG
  SVGP_SMOOTHQ_TO      = 'T', // SVG
  SVGP_CLEAR           = 'U',
  SVGP_VER_LINE_TO     = 'V', // SVG
  SVGP_SET_LINE_CAP    = 'W',
  SVGP_EXIT            = 'X',
  SVGP_SET_COLOR_MODEL = 'Y',
  //                     'Z'  // SVG
  SVGP_REL_ARC_TO      = 'a', // SVG, NYI
  SVGP_CLIP            = 'b', // NYI (int ctx - fully implemented in parser)
  SVGP_REL_CURVE_TO    = 'c', // SVG
  SVGP_SAVE            = 'd',
  SVGP_TRANSLATE       = 'e',
  SVGP_LINEAR_GRADIENT = 'f',
  //SVP_UNUSED         = 'g', // -- for glyph
  SVGP_REL_HOR_LINE_TO = 'h', // SVG
  //SVGP_IMAGE         = 'i', // NYI
  SVGP_SET_LINE_JOIN   = 'j',
  //SVGP_UNUSED        = 'k', // -- for kerning pair?
  SVGP_REL_LINE_TO     = 'l', // SVG
  SVGP_REL_MOVE_TO     = 'm', // SVG
  SVGP_SET_FONT        = 'n',
  SVGP_RADIAL_GRADIENT = 'o',
  SVGP_GRADIENT_ADD_STOP = 'p',
  SVGP_REL_QUAD_TO     = 'q', // SVG
  SVGP_RECTANGLE       = 'r',
  SVGP_REL_SMOOTH_TO   = 's', // SVG
  SVGP_REL_SMOOTHQ_TO  = 't', // SVG
  SVGP_STROKE_TEXT     = 'u',
  SVGP_REL_VER_LINE_TO = 'v', // SVG
  SVGP_SET_LINE_WIDTH  = 'w',
  SVGP_TEXT            = 'x',
  //SVGP_IDENTITY      = 'y', // NYI
  SVGP_CLOSE_PATH      = 'z', // SVG

} SvgpCommand;
static void vt_svgp_set_color_model (SvgP *svgp, int color_model);

static int svgp_resolve_command (SvgP *svgp, const uint8_t*str, int *args)
{
  uint32_t str_hash = 0;

#define STR(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) (\
          (((uint32_t)a0))+ \
          (((uint32_t)a1)*11)+ \
          (((uint32_t)a2)*11*11)+ \
          (((uint32_t)a3)*11*11*11)+ \
          (((uint32_t)a4)*11*11*11*11) + \
          (((uint32_t)a5)*11*11*11*11*11) + \
          (((uint32_t)a6)*11*11*11*11*11*11) + \
          (((uint32_t)a7)*11*11*11*11*11*11*11) + \
          (((uint32_t)a8)*11*11*11*11*11*11*11*11) + \
          (((uint32_t)a9)*11*11*11*11*11*11*11*11*11) + \
          (((uint32_t)a10)*11*11*11*11*11*11*11*11*11*11) + \
          (((uint32_t)a11)*11*11*11*11*11*11*11*11*11*11*11))

/* The compiler will give us an error in the switch if there are duplicate
 * hashes as duplicate case values, since all the matching strings are reduced
 * to their integers at compile time
 */

  {
    int multiplier = 1;
    for (int i = 0; str[i] && i < 12; i++)
    {
      str_hash = str_hash + str[i] * multiplier;
      multiplier *= 11;
    }
  }

  switch (str_hash)
  {
    case STR('a','r','c','_','t','o',0,0,0,0,0,0):
    case 'A': *args = 7; return SVGP_ARC_TO;

    case STR('a','r','c',0,0,0,0,0,0,0,0,0):
    case 'B': *args = 6; return SVGP_ARC;

    case STR('c','u','r','v','e','_','t','o',0,0,0,0):
    case 'C': *args = 6; return SVGP_CURVE_TO;

    case STR('r','e','s','t','o','r','e',0,0,0,0,0):
    case 'D': *args = 0; return SVGP_RESTORE;

    case STR('s','t','r','o','k','e',0,0,0,0,0,0):
    case 'E': *args = 0; return SVGP_STROKE;

    case STR('f','i','l','l',0,0,0,0,0,0,0,0):
    case 'F': *args = 0; return SVGP_FILL;

    case STR('h','o','r','_','l','i','n','e','_','t','o',0):
    case 'H': *args = 1; return SVGP_HOR_LINE_TO;

    case STR('r','o','t','a','t','e',0,0,0,0,0,0):
    case 'J': *args = 1; return SVGP_ROTATE;

    case STR('c','o','l','o','r',0,0,0,0,0,0,0):
    case STR('s','e','t','_','c','o','l','o','r',0,0,0):
    case 'K': *args = svgp->color_components; return SVGP_SET_COLOR;

    case STR('l','i','n','e','_','t','o',0,0,0,0,0):
    case 'L': *args = 2; return SVGP_LINE_TO;

    case STR('m','o','v','e','_','t','o',0,0,0,0,0):
    case 'M': *args = 2; return SVGP_MOVE_TO;

    case STR('s','e','t','_','f','o','n','t','_','s','i','z'):
    case 'N': *args = 1; return SVGP_SET_FONT_SIZE;

    case STR('s','c','a','l','e',0,0,0,0,0,0,0):
    case 'O': *args = 2; return SVGP_SCALE;

    case STR('q','u','a','d','_','t','o',0,0,0,0,0):
    case 'Q': *args = 4; return SVGP_QUAD_TO;

    case STR('s','m','o','o','t','h','_','t','o',0,0,0):
    case 'S': *args = 4; return SVGP_SMOOTH_TO;

    case STR('s','m','o','o','t','h','_','q','u','a','d','_'):
    case 'T': *args = 2; return SVGP_SMOOTHQ_TO;

    case STR('c','l','e','a','r',0,0,0,0,0,0,0):
    case 'U': *args = 0; return SVGP_CLEAR;

    case STR('e','x','i','t',0,0,0,0,0,0,0,0):
    case STR('d','o','n','e',0,0,0,0,0,0,0,0):
    case 'X': *args = 0; return SVGP_EXIT;

    case STR('c','o','l','o','r','_','m','o','d','e','l', 0):
    case 'Y': *args = 1; return SVGP_SET_COLOR_MODEL;

    case STR('v','e','r','_','l','i','n','e','_','t','o',0):
    case 'V': *args = 1; return SVGP_VER_LINE_TO;

    case STR('s','e','t','_','l','i','n','e','_','c','a','p'):
    case STR('c','a','p',0,0,0,0,0,0,0,0,0):
    case 'W': *args = 1; return SVGP_SET_LINE_CAP;

    case STR('c','l','o','s','e','_','p','a','t','h',0,0):
    case 'Z':case 'z': *args = 0; return SVGP_CLOSE_PATH;

    case STR('r','e','l','_','a','r','c','_','t','o',0,0):
    case 'a': *args = 7; return SVGP_REL_ARC_TO;

    case STR('c','l','i','p',0,0,0,0,0,0,0,0):
    case 'b': *args = 0; return SVGP_CLIP;

    case STR('r','e','l','_','c','u','r','v','e','_','t','o'):
    case 'c': *args = 6; return SVGP_REL_CURVE_TO;

    case STR('s','a','v','e',0,0,0,0,0,0,0,0):
    case 'd': *args = 0; return SVGP_SAVE;

    case STR('t','r','a','n','s','l','a','t','e',0,0,0):
    case 'e': *args = 2; return SVGP_TRANSLATE;

    case STR('l','i','n','e','a','r','_','g','r','a','d','i'):
    case 'f': *args = 4; return SVGP_LINEAR_GRADIENT;

    case STR('r','e','l','_','h','o','r','_','l','i','n','e'):
    case 'h': *args = 1; return SVGP_REL_HOR_LINE_TO;

    case STR('s','e','t','_','l','i','n','e','_','j','o','i'):
    case STR('j','o','i','n',0,0,0,0,0,0,0,0):
    case 'j': *args = 1; return SVGP_SET_LINE_JOIN;

    case STR('r','e','l','_','l','i','n','e','_','t','o',0):
    case 'l': *args = 2; return SVGP_REL_LINE_TO;

    case STR('r','e','l','_','m','o','v','e','_','t','o',0):
    case 'm': *args = 2; return SVGP_REL_MOVE_TO;

    case STR('s','e','t','_','f','o','n','t',0,0,0,0):
    case 'n': *args = 100; return SVGP_SET_FONT;

    case STR('r','a','d','i','a','l','_','g','r','a','d','i'):
    case 'o': *args = 6; return SVGP_RADIAL_GRADIENT;

    case STR('g','r','a','d','i','e','n','t','_','a','d','d'):
    case STR('a','d','d','_','s','t','o','p',0,0,0,0):
    case 'p': *args = 5; return SVGP_GRADIENT_ADD_STOP;

    case STR('r','e','l','_','q','u','a','d','_','t','o',0):
    case 'q': *args = 4; return SVGP_REL_QUAD_TO;

    case STR('r','e','c','t','a','n','g','l','e',0,0,0):
    case STR('r','e','c','t',0,0,0,0,'e',0,0,0):
    case 'r': *args = 4; return SVGP_RECTANGLE;

    case STR('r','e','l','_','s','m','o','o','t','h','_','t'):
    case 's': *args = 4; return SVGP_REL_SMOOTH_TO;

    case STR('r','e','l','_','s','m','o','o','t','h','_','q'):
    case 't': *args = 2; return SVGP_REL_SMOOTHQ_TO;

    case STR('s','t','r','o','k','e','_','t','e','x','t', 0):
    case 'u': *args = 100; return SVGP_STROKE_TEXT;

    case STR('r','e','l','_','v','e','r','_','l','i','n','e'):
    case 'v': *args = 1;
      return SVGP_REL_VER_LINE_TO;

    case STR('s','e','t','_','l','i','n','e','_','w','i','d'):
    case STR('l','i','n','e','_','w','i','d','t','h',0,0):
    case 'w': *args = 1;
      return SVGP_SET_LINE_WIDTH;

    case STR('t','e','x','t',0,0,0,0,0,0,0,0):
    case 'x': *args = 100;
      return SVGP_TEXT;

    case STR('g','r','a','y',0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 1);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('g','r','a','y','a',0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 101);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('r','g','b',0,0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 3);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('r','g','b','a',0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 103);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('c','m','y','k',0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 4);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('l','a','b',0,0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 5);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('l','a','b','a',0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 105);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('l','c','h',0,0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 6);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('l','c','h','a',0,0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 106);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    case STR('c','m','y','k','a',0,0,0,0,0,0,0):
      vt_svgp_set_color_model (svgp, 104);
      *args = svgp->color_components;
      return SVGP_SET_COLOR;

    /* the following words in all caps map to integer constants
    */
    case STR('J','O','I','N','_','B','E','V','E','L',0,0):
    case STR('B','E','V','E','L',0, 0, 0, 0, 0, 0, 0):     return 0;
    case STR('J','O','I','N','_','R','O','U','N','D',0,0):
    case STR('R','O','U','N','D',0, 0, 0, 0, 0, 0, 0):     return 1;
    case STR('J','O','I','N','_','M','I','T','E','R',0,0):
    case STR('M','I','T','E','R',0, 0, 0, 0, 0, 0, 0):     return 2;
    case STR('C','A','P','_','N','O','N','E',0,0,0,0):
    case STR('N','O','N','E',0 ,0, 0, 0, 0, 0, 0, 0):      return 0;
    case STR('C','A','P','_','R','O','U','N','D',0,0,0):   return 1;
    case STR('C','A','P','_','S','Q','U','A','R','E',0,0):
    case STR('S','Q','U','A','R','E', 0, 0, 0, 0, 0, 0):   return 2;

    case STR('G','R','A','Y',0,0, 0, 0, 0, 0, 0, 0):       return 1; break;
    case STR('G','R','A','Y','A',0, 0, 0, 0, 0, 0, 0):     return 101; break;
    case STR('G','R','A','Y','A','_', 'A', 0, 0, 0, 0, 0): return 201; break;
    case STR('R','G','B',0,0,0, 0, 0, 0, 0, 0, 0):         return 3; break;
    case STR('R','G','B','A',0,0, 0, 0, 0, 0, 0, 0):       return 103; break;
    case STR('R','G','B','A','_','A', 0, 0, 0, 0, 0, 0):   return 203; break;
    case STR('C','M','Y','K',0,0, 0, 0, 0, 0, 0, 0):       return 4; break;
    case STR('C','M','Y','K','A','_','A', 0, 0, 0, 0, 0):  return 104; break;

#undef STR
  }
  return -1;
}


enum {
  SVGP_NEUTRAL = 0,
  SVGP_NUMBER,
  SVGP_NEG_NUMBER,
  SVGP_WORD,
  SVGP_COMMENT,
  SVGP_STRING1,
  SVGP_STRING2,
  SVGP_STRING1_ESCAPED,
  SVGP_STRING2_ESCAPED,
} SVGP_STATE;

static void vt_svgp_set_color_model (SvgP *svgp, int color_model)
{
  svgp->color_model      = color_model;
  svgp->color_components = color_model % 100;
  if (svgp->color_model >  99)
    svgp->color_components++;
}

void vt_svgp_get_color (SvgP *svgp, int offset, float *red, float *green, float *blue, float *alpha)
{
  *alpha = 1.0;
  switch (svgp->color_model)
  {
    case 101: // gray
      *alpha = svgp->numbers[offset + 1];
    case 1: // gray
      *red = *green = *blue = svgp->numbers[offset + 0];
    break;
    default:
    case 103: // rgba
      *alpha = svgp->numbers[offset + 3];
    case 3: // rgb
      *red = svgp->numbers[offset + 0];
      *green = svgp->numbers[offset + 1];
      *blue = svgp->numbers[offset + 2];
    break;
    case 104: // cmyka
      *alpha = svgp->numbers[offset + 4];
    case 4: // cmyk
      *red = (1.0-svgp->numbers[offset + 0]) *
               (1.0 - svgp->numbers[offset + 3]);
      *green = (1.0-svgp->numbers[offset + 1]) *
                 (1.0 - svgp->numbers[offset + 3]);
      *blue = (1.0-svgp->numbers[offset + 2]) *
                 (1.0 - svgp->numbers[offset + 3]);
    break;

    case 105: // LabA
    case 5:   // Lab
    case 106: // LchA
    case 6:   // Lch
      break;
    //
  }
}

static void svgp_dispatch_command (SvgP *svgp)
{
  SvgpCommand cmd = svgp->command;
  Ctx *ctx = svgp->ctx;

  if (svgp->n_args != 100 &&
      svgp->n_args != svgp->n_numbers)
  {
    fprintf (stderr, "unexpected args for '%c' expected %i but got %i\n",
      cmd, svgp->n_args, svgp->n_numbers);
  }

  svgp->command = SVGP_NONE;
  switch (cmd)
  {
    case SVGP_NONE:
    case SVGP_FILL: ctx_fill (ctx); break;
    case SVGP_SAVE: ctx_save (ctx); break;
    case SVGP_STROKE: ctx_stroke (ctx); break;
    case SVGP_RESTORE: ctx_restore (ctx); break;

    case SVGP_SET_COLOR:
      {
        float red, green, blue, alpha;
        vt_svgp_get_color (svgp, 0, &red, &green, &blue, &alpha);

        ctx_set_rgba (ctx, red, green, blue, alpha);
      }
      break;
    case SVGP_SET_COLOR_MODEL:
      vt_svgp_set_color_model (svgp, svgp->numbers[0]);
      break;

    case SVGP_ARC_TO: break;
    case SVGP_REL_ARC_TO: break;

    case SVGP_REL_SMOOTH_TO:
        {
	  float cx = svgp->pcx;
	  float cy = svgp->pcy;
	  float ax = 2 * ctx_x (ctx) - cx;
	  float ay = 2 * ctx_y (ctx) - cy;
	  ctx_curve_to (ctx, ax, ay, svgp->numbers[0] +  cx, svgp->numbers[1] + cy,
			     svgp->numbers[2] + cx, svgp->numbers[3] + cy);
	  svgp->pcx = svgp->numbers[0] + cx;
	  svgp->pcy = svgp->numbers[1] + cy;
        }
	break;
    case SVGP_SMOOTH_TO:
        {
	  float ax = 2 * ctx_x (ctx) - svgp->pcx;
	  float ay = 2 * ctx_y (ctx) - svgp->pcy;
	  ctx_curve_to (ctx, ax, ay, svgp->numbers[0], svgp->numbers[1],
			     svgp->numbers[2], svgp->numbers[3]);
	  svgp->pcx = svgp->numbers[0];
	  svgp->pcx = svgp->numbers[1];
        }
        break;

    case SVGP_SMOOTHQ_TO:
	ctx_quad_to (ctx, svgp->pcx, svgp->pcy, svgp->numbers[0], svgp->numbers[1]);
        break;
    case SVGP_REL_SMOOTHQ_TO:
        {
	  float cx = svgp->pcx;
	  float cy = svgp->pcy;
	  svgp->pcx = 2 * ctx_x (ctx) - svgp->pcx;
	  svgp->pcy = 2 * ctx_y (ctx) - svgp->pcy;
	  ctx_quad_to (ctx, svgp->pcx, svgp->pcy, svgp->numbers[0] +  cx, svgp->numbers[1] + cy);
        }
	break;

    case SVGP_STROKE_TEXT: ctx_text_stroke (ctx, (char*)svgp->holding); break;
    case SVGP_VER_LINE_TO: ctx_line_to (ctx, ctx_x (ctx), svgp->numbers[0]); svgp->command = SVGP_VER_LINE_TO;
	svgp->pcx = ctx_x (ctx);
	svgp->pcy = ctx_y (ctx);
        break;
    case SVGP_HOR_LINE_TO:
	ctx_line_to (ctx, svgp->numbers[0], ctx_y(ctx)); svgp->command = SVGP_HOR_LINE_TO;
	svgp->pcx = ctx_x (ctx);
	svgp->pcy = ctx_y (ctx);
	break;
    case SVGP_REL_HOR_LINE_TO: ctx_rel_line_to (ctx, svgp->numbers[0], 0.0f); svgp->command = SVGP_REL_HOR_LINE_TO;
	svgp->pcx = ctx_x (ctx);
	svgp->pcy = ctx_y (ctx);
        break;
    case SVGP_REL_VER_LINE_TO: ctx_rel_line_to (ctx, 0.0f, svgp->numbers[0]); svgp->command = SVGP_REL_VER_LINE_TO;
	svgp->pcx = ctx_x (ctx);
	svgp->pcy = ctx_y (ctx);
	break;

    case SVGP_ARC: ctx_arc (ctx, svgp->numbers[0], svgp->numbers[1],
			    svgp->numbers[2], svgp->numbers[3],
			    svgp->numbers[4], svgp->numbers[5]);
        break;

    case SVGP_CURVE_TO: ctx_curve_to (ctx, svgp->numbers[0], svgp->numbers[1],
					   svgp->numbers[2], svgp->numbers[3],
					   svgp->numbers[4], svgp->numbers[5]);
			svgp->pcx = svgp->numbers[2];
			svgp->pcy = svgp->numbers[3];
		        svgp->command = SVGP_CURVE_TO;
        break;
    case SVGP_REL_CURVE_TO:
			svgp->pcx = svgp->numbers[2] + ctx_x (ctx);
			svgp->pcy = svgp->numbers[3] + ctx_y (ctx);
			
			ctx_rel_curve_to (ctx, svgp->numbers[0], svgp->numbers[1],
					   svgp->numbers[2], svgp->numbers[3],
					   svgp->numbers[4], svgp->numbers[5]);
		        svgp->command = SVGP_REL_CURVE_TO;
        break;
    case SVGP_LINE_TO:
        ctx_line_to (ctx, svgp->numbers[0], svgp->numbers[1]);
        svgp->command = SVGP_LINE_TO;
        svgp->pcx = svgp->numbers[0];
        svgp->pcy = svgp->numbers[1];
        break;
    case SVGP_MOVE_TO:
        ctx_move_to (ctx, svgp->numbers[0], svgp->numbers[1]);
        svgp->command = SVGP_LINE_TO;
        svgp->pcx = svgp->numbers[0];
        svgp->pcy = svgp->numbers[1];
        svgp->left_margin = svgp->pcx;
        break;
    case SVGP_SET_FONT_SIZE:
	ctx_set_font_size (ctx, svgp->numbers[0]);
	break;
    case SVGP_SCALE:
	ctx_scale (ctx, svgp->numbers[0], svgp->numbers[1]);
	break;
    case SVGP_QUAD_TO:
	svgp->pcx = svgp->numbers[0];
        svgp->pcy = svgp->numbers[1];
        ctx_quad_to (ctx, svgp->numbers[0], svgp->numbers[1],
	             svgp->numbers[2], svgp->numbers[3]);
        svgp->command = SVGP_QUAD_TO;
	break;
    case SVGP_REL_QUAD_TO: 
        svgp->pcx = svgp->numbers[0] + ctx_x (ctx);
        svgp->pcy = svgp->numbers[1] + ctx_y (ctx);
        ctx_rel_quad_to (ctx, svgp->numbers[0], svgp->numbers[1],
        svgp->numbers[2], svgp->numbers[3]);
        svgp->command = SVGP_REL_QUAD_TO;
        break;
    case SVGP_SET_LINE_CAP:
	ctx_set_line_cap (ctx, svgp->numbers[0]);
	break;
    case SVGP_CLIP:
	ctx_clip (ctx);
        break;

    case SVGP_TRANSLATE:
	ctx_translate (ctx, svgp->numbers[0], svgp->numbers[1]);
	break;
    case SVGP_ROTATE:
	ctx_rotate (ctx, svgp->numbers[0]);
        break;
    case SVGP_TEXT:
	if (svgp->n_numbers == 1)
	  ctx_rel_move_to (ctx, -svgp->numbers[0], 0.0);  //  XXX : scale by font(size)
	else
        {
          char *copy = strdup ((char*)svgp->holding);
          char *c;
          for (c = copy; c; )
          {
            char *next_nl = strchr (c, '\n');
            if (next_nl)
            {
              *next_nl = 0;
            }

            /* do our own layouting on a per-word basis?, to get justified
             * margins? then we'd want explict margins rather than the
             * implicit ones from move_to's .. making move_to work within
             * margins.
             */
	    ctx_text (ctx, c);

            if (next_nl)
            {
              ctx_move_to (ctx, svgp->left_margin, ctx_y (ctx) + 
                                ctx_get_font_size (ctx));
              c = next_nl + 1;
            }
            else
            {
              c = NULL;
            }
          }
          free (copy);
        }
        svgp->command = SVGP_TEXT;
        break;
    case SVGP_SET_FONT: ctx_set_font (ctx, (char*)svgp->holding);
        break;
    case SVGP_REL_LINE_TO:
        ctx_rel_line_to (ctx , svgp->numbers[0], svgp->numbers[1]);
        svgp->pcx += svgp->numbers[0];
        svgp->pcy += svgp->numbers[1];
        break;
    case SVGP_REL_MOVE_TO:
	ctx_rel_move_to (ctx , svgp->numbers[0], svgp->numbers[1]);
        svgp->pcx += svgp->numbers[0];
        svgp->pcy += svgp->numbers[1];
        svgp->left_margin = ctx_x (ctx);
        break;
    case SVGP_SET_LINE_WIDTH:
        ctx_set_line_width (ctx, svgp->numbers[0]);
        break;
    case SVGP_SET_LINE_JOIN:
        ctx_set_line_join (ctx, svgp->numbers[0]);
	break;
    case SVGP_RECTANGLE:
        ctx_rectangle (ctx, svgp->numbers[0], svgp->numbers[1],
			    svgp->numbers[2], svgp->numbers[3]);
	break;
    case SVGP_LINEAR_GRADIENT:
	ctx_linear_gradient (ctx, svgp->numbers[0], svgp->numbers[1],
                                  svgp->numbers[2], svgp->numbers[3]);
	break;
    case SVGP_RADIAL_GRADIENT:
	ctx_radial_gradient (ctx, svgp->numbers[0], svgp->numbers[1],
                                  svgp->numbers[2], svgp->numbers[3],
                                  svgp->numbers[4], svgp->numbers[5]);
	break;
    case SVGP_GRADIENT_ADD_STOP:
      {
        float red, green, blue, alpha;
        vt_svgp_get_color (svgp, 1, &red, &green, &blue, &alpha);

        ctx_gradient_add_stop (ctx, svgp->numbers[0], red, green, blue, alpha);
      }
       break;
    case SVGP_CLOSE_PATH:
       ctx_close_path (ctx);
       break;
    case SVGP_EXIT:
       if (svgp->exit)
         svgp->exit (svgp->exit_data);
       break;
    case SVGP_CLEAR:
       ctx_clear (ctx);
       ctx_translate (ctx,
                     (svgp->cursor_x-1) * svgp->cw * 10,
                     (svgp->cursor_y-1) * svgp->ch * 10);
       break;
  }
  svgp->n_numbers = 0;
}

void svgp_feed_byte (SvgP *svgp, int byte)
{
  switch (svgp->state)
  {
    case SVGP_NEUTRAL:
      switch (byte)
      {
         case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
         case 8: case 11: case 12: case 14: case 15: case 16: case 17:
         case 18: case 19: case 20: case 21: case 22: case 23: case 24:
         case 25: case 26: case 27: case 28: case 29: case 30: case 31:
            break;
         case ' ':case '\t':case '\r':case '\n':case ';':case ',':case '(':case ')':case '=':
         case '{':case '}':
            break;
         case '#':
            svgp->state = SVGP_COMMENT;
            break;
         case '\'':
            svgp->state = SVGP_STRING1;
            svgp->pos = 0;
            svgp->holding[0] = 0;
            break;
         case '"':
            svgp->state = SVGP_STRING2;
            svgp->pos = 0;
            svgp->holding[0] = 0;
            break;
         case '-':
            svgp->state = SVGP_NEG_NUMBER;
            svgp->numbers[svgp->n_numbers] = 0;
            svgp->decimal = 0;
            break;
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            svgp->state = SVGP_NUMBER;
            svgp->numbers[svgp->n_numbers] = 0;
            svgp->numbers[svgp->n_numbers] += (byte - '0');
            svgp->decimal = 0;
            break;
         case '.':
            svgp->state = SVGP_NUMBER;
            svgp->numbers[svgp->n_numbers] = 0;
            svgp->decimal = 1;
            break;
         default:
            svgp->state = SVGP_WORD;
            svgp->pos = 0;
            svgp->holding[svgp->pos++]=byte;
            if (svgp->pos > 62) svgp->pos = 62;
            break;
      }
      break;
    case SVGP_NUMBER:
    case SVGP_NEG_NUMBER:
      {
        int new_neg = 0;
        switch (byte)
        {
           case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
           case 8: case 11: case 12: case 14: case 15: case 16: case 17:
           case 18: case 19: case 20: case 21: case 22: case 23: case 24:
           case 25: case 26: case 27: case 28: case 29: case 30: case 31:
              svgp->state = SVGP_NEUTRAL;
              break;
           case ' ':case '\t':case '\r':case '\n':case ';':case ',':case '(':case ')':case '=':
           case '{':case '}':
              if (svgp->state == SVGP_NEG_NUMBER)
                svgp->numbers[svgp->n_numbers] *= -1;
    
              svgp->state = SVGP_NEUTRAL;
              break;
           case '#':
              svgp->state = SVGP_COMMENT;
              break;
           case '-':
              svgp->state = SVGP_NEG_NUMBER;
              new_neg = 1;
              svgp->numbers[svgp->n_numbers+1] = 0;
              svgp->decimal = 0;
              break;
           case '.':
              svgp->decimal = 1;
              break;
           case '0': case '1': case '2': case '3': case '4':
           case '5': case '6': case '7': case '8': case '9':
              if (svgp->decimal)
              {
        	      svgp->decimal *= 10;
                svgp->numbers[svgp->n_numbers] += (byte - '0') / (1.0 * svgp->decimal);
              }
              else
              {
                svgp->numbers[svgp->n_numbers] *= 10;
                svgp->numbers[svgp->n_numbers] += (byte - '0');
              }
              break;
           case '@':
              if (svgp->state == SVGP_NEG_NUMBER)
                svgp->numbers[svgp->n_numbers] *= -1;
              if (svgp->n_numbers % 2 == 0) // even is x coord
              {
                svgp->numbers[svgp->n_numbers] *= svgp->cw;
              }
              else
              {
        	  if (! (svgp->command == 'r' && svgp->n_numbers > 1))
                  // height of rectangle is avoided,
                  // XXX for radial gradient there is more complexity here
        	  {
                  svgp->numbers[svgp->n_numbers] --;
        	  }

                svgp->numbers[svgp->n_numbers] =
                  (svgp->numbers[svgp->n_numbers]) * svgp->ch;
              }
              svgp->state = SVGP_NEUTRAL;
          break;
           case '%':
              if (svgp->state == SVGP_NEG_NUMBER)
                svgp->numbers[svgp->n_numbers] *= -1;
              if (svgp->n_numbers % 2 == 0) // even means x coord
              {
                svgp->numbers[svgp->n_numbers] =
        	   svgp->numbers[svgp->n_numbers] * ((svgp->cols * svgp->cw)/100.0);
              }
              else
              {
                svgp->numbers[svgp->n_numbers] =
        	   svgp->numbers[svgp->n_numbers] * ((svgp->rows * svgp->ch)/100.0);
              }
              svgp->state = SVGP_NEUTRAL;
              break;
           default:
              if (svgp->state == SVGP_NEG_NUMBER)
                svgp->numbers[svgp->n_numbers] *= -1;
              svgp->state = SVGP_WORD;
              svgp->pos = 0;
              svgp->holding[svgp->pos++]=byte;
              break;
    
        }
        if ((svgp->state != SVGP_NUMBER &&
             svgp->state != SVGP_NEG_NUMBER) || new_neg)
        {
                 svgp->n_numbers ++;
        	 if (svgp->n_numbers == svgp->n_args || svgp->n_args == 100)
        	 {
        	   svgp_dispatch_command (svgp);
        	 }
    
                 if (svgp->n_numbers > 10)
                   svgp->n_numbers = 10;
        }
      }
      break;

    case SVGP_WORD:
      switch (byte)
      {
         case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
         case 8: case 11: case 12: case 14: case 15: case 16: case 17:
         case 18: case 19: case 20: case 21: case 22: case 23: case 24:
         case 25: case 26: case 27: case 28: case 29: case 30: case 31:

         case ' ':case '\t':case '\r':case '\n':case ';':case ',':case '(':case ')':case '=':
         case '{':case '}':
            svgp->state = SVGP_NEUTRAL;
            break;
         case '#':
            svgp->state = SVGP_COMMENT;
            break;
         case '-':
            svgp->state = SVGP_NEG_NUMBER;
            svgp->numbers[svgp->n_numbers] = 0;
            svgp->decimal = 0;
            break;
         case '0': case '1': case '2': case '3': case '4':
         case '5': case '6': case '7': case '8': case '9':
            svgp->state = SVGP_NUMBER;
            svgp->numbers[svgp->n_numbers] = 0;
            svgp->numbers[svgp->n_numbers] += (byte - '0');
            svgp->decimal = 0;
            break;
         case '.':
            svgp->state = SVGP_NUMBER;
            svgp->numbers[svgp->n_numbers] = 0;
            svgp->decimal = 1;
            break;
         default:
            svgp->holding[svgp->pos++]=byte;
            if (svgp->pos > 62) svgp->pos = 62;
            break;
      }
      if (svgp->state != SVGP_WORD)
      {
        int args = 0;
        svgp->holding[svgp->pos]=0;
        int command = svgp_resolve_command (svgp, svgp->holding, &args);

        if (command >= 0 && command < 5)
        {
          svgp->numbers[svgp->n_numbers] = command;
          svgp->state = SVGP_NUMBER;
          svgp_feed_byte (svgp, ',');
        }
        else if (command > 0)
        {
           svgp->command = command;
           svgp->n_args = args;
           if (args == 0)
           {
      	     svgp_dispatch_command (svgp);
           }
        }
        else
        {
          /* interpret char by char */
          uint8_t buf[16]=" ";
          for (int i = 0; svgp->pos && svgp->holding[i] > ' '; i++)
          {
             buf[0] = svgp->holding[i];
             svgp->command = svgp_resolve_command (svgp, buf, &args);
             if (svgp->command > 0)
             {
               svgp->n_args = args;
               if (args == 0)
               {
      	         svgp_dispatch_command (svgp);
               }
             }
             else
             {
               fprintf (stderr, "unhandled command '%c'\n", buf[0]);
             }
          }
        }
        svgp->n_numbers = 0;
      }
      break;

    case SVGP_STRING1:
      switch (byte)
      {
         case '\\':
            svgp->state = SVGP_STRING1_ESCAPED;
            break;
         case '\'':
            svgp->state = SVGP_NEUTRAL;
            break;
         default:
            svgp->holding[svgp->pos++]=byte;
            svgp->holding[svgp->pos]=0;
            if (svgp->pos > 62) svgp->pos = 62;
            break;
      }
      if (svgp->state != SVGP_STRING1)
      {
        svgp_dispatch_command (svgp);
      }
      break;
    case SVGP_STRING1_ESCAPED:
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
      svgp->holding[svgp->pos++]=byte;
      svgp->holding[svgp->pos]=0;
      if (svgp->pos > 62) svgp->pos = 62;
      svgp->state = SVGP_STRING1;
      break;
    case SVGP_STRING2_ESCAPED:
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
      svgp->holding[svgp->pos++]=byte;
      svgp->holding[svgp->pos]=0;
      if (svgp->pos > 62) svgp->pos = 62;
      svgp->state = SVGP_STRING2;
      break;

    case SVGP_STRING2:
      switch (byte)
      {
         case '\\':
            svgp->state = SVGP_STRING2_ESCAPED;
            break;
         case '"':
            svgp->state = SVGP_NEUTRAL;
            break;
         default:
            svgp->holding[svgp->pos++]=byte;
            svgp->holding[svgp->pos]=0;
            if (svgp->pos > 62) svgp->pos = 62;
            break;
      }
      if (svgp->state != SVGP_STRING2)
      {
        svgp_dispatch_command (svgp);
      }
      break;
    case SVGP_COMMENT:
      switch (byte)
      {
        case '\r':
        case '\n':
          svgp->state = SVGP_NEUTRAL;
        default:
          break;
      }
      break;
  }
}
