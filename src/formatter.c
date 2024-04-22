#include "ctx-split.h"

#if CTX_FORMATTER

typedef struct _CtxFormatter  CtxFormatter;
struct _CtxFormatter 
{
  void *target; // FILE
  int   longform;
  int   indent;

  void (*add_str)(CtxFormatter *formatter, const char *str, int len);
};

static inline void ctx_formatter_addstr (CtxFormatter *formatter, const char *str, int len)
{
  formatter->add_str (formatter, str, len);
}

#if 0
static inline void ctx_formatter_addstrf (CtxFormatter *formatter, const char *format, ...)
{
   va_list ap;
   size_t needed;
   char *buffer;
   va_start (ap, format);
   needed = vsnprintf (NULL, 0, format, ap) + 1;
   buffer = (char*) ctx_malloc (needed);
   va_end (ap);
   va_start (ap, format);
   vsnprintf (buffer, needed, format, ap);
   va_end (ap);
   ctx_formatter_addstr (formatter, buffer, -1);
   ctx_free (buffer);
}
#endif


static void
ctx_print_int (CtxFormatter *formatter, int val)
{
  char buf[64];
  char *bp = &buf[0];
  int remainder;
  if (val < 0)
  {
    buf[0]='-';
    bp++;
    remainder = -val;
  }
  else
  remainder = val;

  int len = 0;
  do {
    int digit = remainder % 10;
    bp[len++] = digit + '0';
    remainder /= 10;
  } while (remainder);

  bp[len]=0;
  for (int i = 0; i < len/2; i++)
  {
    int tmp = bp[i];
    bp[i] = bp[len-1-i];
    bp[len-1-i] = tmp;
  }
  len += (val < 0);
  ctx_formatter_addstr (formatter, buf, len);
}

static void
ctx_print_float (CtxFormatter *formatter, float val)
{
  if (val < 0.0f)
  {
    ctx_formatter_addstr (formatter, "-", 1);
    val = -val;
  }
  int remainder = ((int)(val*10000))%10000;
  if (remainder % 10 > 5)
    remainder = remainder/10+1;
  else
    remainder /= 10;

  if (!formatter->longform && ((((int)val))==0) && (remainder))
  {
    // 
  }
  else
  {
    ctx_print_int (formatter, (int)val);
  }


  if (remainder)
  {
    ctx_formatter_addstr (formatter, ".", 1);
    if (remainder < 10)
      ctx_formatter_addstr (formatter, "0", 1);
    if (remainder < 100)
      ctx_formatter_addstr (formatter, "0", 1);
    ctx_print_int (formatter, remainder);
  }
}

static void _ctx_stream_addstr (CtxFormatter *formatter, const char *str, int len)
{
  if (!str || len == 0)
  {
    return;
  }
  if (len < 0) len = ctx_strlen (str);
  fwrite (str, len, 1, (FILE*)formatter->target);
}

static void _ctx_fd_addstr (CtxFormatter *formatter, const char *str, int len)
{
  if (!str || len == 0)
  {
    return;
  }
  if (len < 0) len = ctx_strlen (str);
  write ((size_t)formatter->target, str, len);
}


void _ctx_string_addstr (CtxFormatter *formatter, const char *str, int len)
{
  if (!str || len == 0)
    return;
  if (len < 0) len = ctx_strlen (str);
  ctx_string_append_data ((CtxString*)(formatter->target), str, len);
}

static void _ctx_print_endcmd (CtxFormatter *formatter)
{
  if (formatter->longform)
    ctx_formatter_addstr (formatter, ");\n", 3);
}

static void _ctx_indent (CtxFormatter *formatter)
{
  for (int i = 0; i < formatter->indent; i++)
    ctx_formatter_addstr (formatter, "  ", 2);
}

const char *_ctx_code_to_name (int code)
{
      switch (code)
        {
          case CTX_REL_LINE_TO_X4:           return "relLinetoX4"; break;
          case CTX_REL_LINE_TO_REL_CURVE_TO: return "relLineToRelCurveTo"; break;
          case CTX_REL_CURVE_TO_REL_LINE_TO: return "relCurveToRelLineTo"; break;
          case CTX_REL_CURVE_TO_REL_MOVE_TO: return "relCurveToRelMoveTo"; break;
          case CTX_REL_LINE_TO_X2:           return "relLineToX2"; break;
          case CTX_MOVE_TO_REL_LINE_TO:      return "moveToRelLineTo"; break;
          case CTX_REL_LINE_TO_REL_MOVE_TO:  return "relLineToRelMoveTo"; break;
          case CTX_FILL_MOVE_TO:             return "fillMoveTo"; break;
          case CTX_REL_QUAD_TO_REL_QUAD_TO:  return "relQuadToRelQuadTo"; break;
          case CTX_REL_QUAD_TO_S16:          return "relQuadToS16"; break;

          case CTX_SET_KEY:              return "setParam"; break;
          case CTX_COLOR:                return "setColor"; break;
          case CTX_DEFINE_GLYPH:         return "defineGlyph"; break;
          case CTX_DEFINE_FONT:          return "defineFont"; break;
          case CTX_KERNING_PAIR:         return "kerningPair"; break;
          case CTX_SET_PIXEL:            return "setPixel"; break;
          case CTX_GLOBAL_ALPHA:         return "globalAlpha"; break;
          case CTX_TEXT:                 return "text"; break;
          case CTX_STROKE_TEXT:          return "strokeText"; break;
          case CTX_SAVE:                 return "save"; break;
          case CTX_RESTORE:              return "restore"; break;
          case CTX_STROKE_SOURCE:        return "strokeSource"; break;
          case CTX_NEW_PAGE:             return "newPage"; break;
          case CTX_START_GROUP:          return "startGroup"; break;
          case CTX_END_GROUP:            return "endGroup"; break;
          case CTX_RECTANGLE:            return "rectangle"; break;
          case CTX_ROUND_RECTANGLE:      return "roundRectangle"; break;
          case CTX_LINEAR_GRADIENT:      return "linearGradient"; break;
          case CTX_CONIC_GRADIENT:       return "conicGradient"; break;
          case CTX_RADIAL_GRADIENT:      return "radialGradient"; break;
          case CTX_GRADIENT_STOP:        return "gradientAddStop"; break;
          case CTX_VIEW_BOX:             return "viewBox"; break;
          case CTX_MOVE_TO:              return "moveTo"; break;
          case CTX_LINE_TO:              return "lineTo"; break;
          case CTX_BEGIN_PATH:           return "beginPath"; break;
          case CTX_REL_MOVE_TO:          return "relMoveTo"; break;
          case CTX_REL_LINE_TO:          return "relLineTo"; break;
          case CTX_FILL:                 return "fill"; break;
          case CTX_PAINT:                return "paint"; break;
          case CTX_EXIT:                 return "exit"; break;
          case CTX_APPLY_TRANSFORM:      return "transform"; break;
          case CTX_SOURCE_TRANSFORM:     return "sourceTransform"; break;
          case CTX_REL_ARC_TO:           return "relArcTo"; break;
          case CTX_GLYPH:                return "glyph"; break;
          case CTX_TEXTURE:              return "texture"; break;
          case CTX_DEFINE_TEXTURE:       return "defineTexture"; break;
          case CTX_IDENTITY:             return "identity"; break;
          case CTX_CLOSE_PATH:           return "closePath"; break;
          case CTX_PRESERVE:             return "preserve"; break;
          case CTX_START_FRAME:          return "start_frame"; break;
          case CTX_END_FRAME:            return "end_frame"; break;
          case CTX_FONT:                 return "font"; break;
          case CTX_STROKE:               return "stroke"; break;
          case CTX_CLIP:                 return "clip"; break;
          case CTX_ARC:                  return "arc"; break;
          case CTX_SCALE:                return "scale"; break;
          case CTX_TRANSLATE:            return "translate"; break;
          case CTX_ROTATE:               return "rotate"; break;
          case CTX_ARC_TO:               return "arcTo"; break;
          case CTX_CURVE_TO:             return "curveTo"; break;
          case CTX_REL_CURVE_TO:         return "relCurveTo"; break;
          case CTX_REL_QUAD_TO:          return "relQuadTo"; break;
          case CTX_QUAD_TO:              return "quadTo"; break;
          case CTX_SMOOTH_TO:            return "smoothTo"; break;
          case CTX_REL_SMOOTH_TO:        return "relSmoothTo"; break;
          case CTX_SMOOTHQ_TO:           return "smoothqTo"; break;
          case CTX_REL_SMOOTHQ_TO:       return "relSmoothqTo"; break;
          case CTX_HOR_LINE_TO:          return "horLineTo"; break;
          case CTX_VER_LINE_TO:          return "verLineTo"; break;
          case CTX_REL_HOR_LINE_TO:      return "relHorLineTo"; break;
          case CTX_REL_VER_LINE_TO:      return "relVerLineTo"; break;
          case CTX_COMPOSITING_MODE:     return "compositingMode"; break;
          case CTX_BLEND_MODE:           return "blendMode"; break;
          case CTX_EXTEND:               return "extend"; break;
          case CTX_TEXT_ALIGN:           return "textAlign"; break;
          case CTX_TEXT_BASELINE:        return "textBaseline"; break;
          case CTX_TEXT_DIRECTION:       return "textDirection"; break;
          case CTX_FONT_SIZE:            return "fontSize"; break;
          case CTX_MITER_LIMIT:          return "miterLimit"; break;
          case CTX_LINE_JOIN:            return "lineJoin"; break;
          case CTX_LINE_CAP:             return "lineCap"; break;
          case CTX_LINE_WIDTH:           return "lineWidth"; break;
          case CTX_LINE_DASH_OFFSET:     return "lineDashOffset"; break;
          case CTX_STROKE_POS:           return "strokePos"; break;
          case CTX_FEATHER:              return "feather"; break;
          case CTX_LINE_HEIGHT:          return "lineHeight";break;
          case CTX_WRAP_LEFT:            return "wrapLeft"; break;
          case CTX_WRAP_RIGHT:           return "wrapRight"; break;
          case CTX_IMAGE_SMOOTHING:      return "imageSmoothing"; break;
          case CTX_SHADOW_BLUR:          return "shadowBlur";  break;
          case CTX_FILL_RULE:            return "fillRule"; break;
        }
      return NULL;
}

static void _ctx_print_name (CtxFormatter *formatter, int code)
{
#define CTX_VERBOSE_NAMES 1
#if CTX_VERBOSE_NAMES
  if (formatter->longform)
    {
      const char *name = NULL;
      _ctx_indent (formatter);
      //switch ((CtxCode)code)
      name = _ctx_code_to_name (code);
      if (name)
        {
          ctx_formatter_addstr (formatter, name, -1);
          ctx_formatter_addstr (formatter, " (", 2);
          if (code == CTX_SAVE)
            { formatter->indent ++; }
          else if (code == CTX_RESTORE)
            { formatter->indent --; }
          return;
        }
    }
#endif
  {
    char name[3];
    name[0]=CTX_SET_KEY;
    name[2]='\0';
    switch (code)
      {
        case CTX_GLOBAL_ALPHA:      name[1]='a'; break;
        case CTX_TEXT_BASELINE:     name[1]='b'; break;
        case CTX_LINE_CAP:          name[1]='c'; break;
        case CTX_TEXT_DIRECTION:    name[1]='d'; break;
        case CTX_EXTEND:            name[1]='e'; break;
        case CTX_FONT_SIZE:         name[1]='f'; break;
        case CTX_LINE_JOIN:         name[1]='j'; break;
        case CTX_MITER_LIMIT:       name[1]='l'; break;
        case CTX_COMPOSITING_MODE:  name[1]='m'; break;
        case CTX_STROKE_POS:        name[1]='p'; break;
        case CTX_FEATHER:           name[1]='F'; break;
        case CTX_FILL_RULE:         name[1]='r'; break;
        case CTX_SHADOW_BLUR:       name[1]='s'; break;
        case CTX_TEXT_ALIGN:        name[1]='t'; break;
        case CTX_LINE_WIDTH:        name[1]='w'; break;
        case CTX_SHADOW_OFFSET_X:   name[1]='x'; break;
        case CTX_SHADOW_OFFSET_Y:   name[1]='y'; break;
        case CTX_BLEND_MODE:        name[1]='B'; break;
        case CTX_SHADOW_COLOR:      name[1]='C'; break;
        case CTX_LINE_DASH_OFFSET:  name[1]='D'; break;
        case CTX_LINE_HEIGHT:       name[1]='H'; break;
        case CTX_WRAP_LEFT:         name[1]='L'; break;
        case CTX_WRAP_RIGHT:        name[1]='R'; break;
        case CTX_IMAGE_SMOOTHING:   name[1]='S'; break;
        default:
          name[0] = code;
          name[1] = 0;
          break;
      }
    ctx_formatter_addstr (formatter, name, -1);
    if (formatter->longform)
      ctx_formatter_addstr (formatter, " (", 2);
    else if (ctx_arguments_for_code (code)<1 || ctx_arguments_for_code(code)>8)
      ctx_formatter_addstr (formatter, " ", 1);
  }
}

static void
ctx_print_entry_enum (CtxFormatter *formatter, CtxEntry *entry, int args)
{
  _ctx_print_name (formatter, entry->code);
  for (int i = 0; i <  args; i ++)
    {
      int val = ctx_arg_u8 (i);
      if (i>0)
        { 
          ctx_formatter_addstr (formatter, " ", 1);
        }
#if CTX_VERBOSE_NAMES
      if (formatter->longform)
        {
          const char *str = NULL;
          switch (entry->code)
            {
              case CTX_TEXT_BASELINE:
                switch (val)
                  {
                    case CTX_TEXT_BASELINE_ALPHABETIC: str = "alphabetic"; break;
                    case CTX_TEXT_BASELINE_TOP:        str = "top";        break;
                    case CTX_TEXT_BASELINE_BOTTOM:     str = "bottom";     break;
                    case CTX_TEXT_BASELINE_HANGING:    str = "hanging";    break;
                    case CTX_TEXT_BASELINE_MIDDLE:     str = "middle";     break;
                    case CTX_TEXT_BASELINE_IDEOGRAPHIC:str = "ideographic";break;
                  }
                break;
              case CTX_TEXT_ALIGN:
                switch (val)
                  {
                    case CTX_TEXT_ALIGN_LEFT:   str = "left"; break;
                    case CTX_TEXT_ALIGN_RIGHT:  str = "right"; break;
                    case CTX_TEXT_ALIGN_START:  str = "start"; break;
                    case CTX_TEXT_ALIGN_END:    str = "end"; break;
                    case CTX_TEXT_ALIGN_CENTER: str = "center"; break;
                  }
                break;
              case CTX_LINE_CAP:
                switch (val)
                  {
                    case CTX_CAP_NONE:   str = "none"; break;
                    case CTX_CAP_ROUND:  str = "round"; break;
                    case CTX_CAP_SQUARE: str = "square"; break;
                  }
                break;
              case CTX_LINE_JOIN:
                switch (val)
                  {
                    case CTX_JOIN_MITER: str = "miter"; break;
                    case CTX_JOIN_ROUND: str = "round"; break;
                    case CTX_JOIN_BEVEL: str = "bevel"; break;
                  }
                break;
              case CTX_FILL_RULE:
                switch (val)
                  {
                    case CTX_FILL_RULE_WINDING:  str = "winding"; break;
                    case CTX_FILL_RULE_EVEN_ODD: str = "evenodd"; break;
                  }
                break;
              case CTX_EXTEND:
                switch (val)
                  {
                    case CTX_EXTEND_NONE:    str = "none"; break;
                    case CTX_EXTEND_PAD:     str = "pad"; break;
                    case CTX_EXTEND_REPEAT:  str = "repeat"; break;
                    case CTX_EXTEND_REFLECT: str = "reflect"; break;
                  }
                break;
              case CTX_BLEND_MODE:
                val = ctx_arg_u32 (i);
                switch (val)
                  {
            case CTX_BLEND_NORMAL:      str = "normal"; break;
            case CTX_BLEND_MULTIPLY:    str = "multiply"; break;
            case CTX_BLEND_SCREEN:      str = "screen"; break;
            case CTX_BLEND_OVERLAY:     str = "overlay"; break;
            case CTX_BLEND_DARKEN:      str = "darken"; break;
            case CTX_BLEND_LIGHTEN:     str = "lighten"; break;
            case CTX_BLEND_COLOR_DODGE: str = "colorDodge"; break;
            case CTX_BLEND_COLOR_BURN:  str = "colorBurn"; break;
            case CTX_BLEND_HARD_LIGHT:  str = "hardLight"; break;
            case CTX_BLEND_SOFT_LIGHT:  str = "softLight"; break;
            case CTX_BLEND_DIFFERENCE:  str = "difference"; break;
            case CTX_BLEND_EXCLUSION:   str = "exclusion"; break;
            case CTX_BLEND_HUE:         str = "hue"; break;
            case CTX_BLEND_SATURATION:  str = "saturation"; break;
            case CTX_BLEND_COLOR:       str = "color"; break; 
            case CTX_BLEND_LUMINOSITY:  str = "luminosity"; break;
                  }
                break;
              case CTX_COMPOSITING_MODE:
                val = ctx_arg_u32 (i);
                switch (val)
                  {
              case CTX_COMPOSITE_SOURCE_OVER: str = "sourceOver"; break;
              case CTX_COMPOSITE_COPY: str = "copy"; break;
              case CTX_COMPOSITE_CLEAR: str = "clear"; break;
              case CTX_COMPOSITE_SOURCE_IN: str = "sourceIn"; break;
              case CTX_COMPOSITE_SOURCE_OUT: str = "sourceOut"; break;
              case CTX_COMPOSITE_SOURCE_ATOP: str = "sourceAtop"; break;
              case CTX_COMPOSITE_DESTINATION: str = "destination"; break;
              case CTX_COMPOSITE_DESTINATION_OVER: str = "destinationOver"; break;
              case CTX_COMPOSITE_DESTINATION_IN: str = "destinationIn"; break;
              case CTX_COMPOSITE_DESTINATION_OUT: str = "destinationOut"; break;
              case CTX_COMPOSITE_DESTINATION_ATOP: str = "destinationAtop"; break;
              case CTX_COMPOSITE_XOR: str = "xor"; break;
                  }

               break;
            }
          if (str)
            {
              ctx_formatter_addstr (formatter, str, -1);
            }
          else
            {
              ctx_print_int (formatter, val);
            }
        }
      else
#endif
        {
          ctx_print_int (formatter, val);
        }
    }
  _ctx_print_endcmd (formatter);
}

#if 0
static void
ctx_print_a85 (CtxFormatter *formatter, uint8_t *data, int length)
{
  char *tmp = (char*)ctx_malloc (ctx_a85enc_len (length));
  ctx_a85enc (data, tmp, length);
  ctx_formatter_addstr (formatter, " ~", 2);
  ctx_formatter_addstr (formatter, tmp, -1);
  ctx_formatter_addstr (formatter, "~ ", 2);
  ctx_free (tmp);
}
#endif

static void
ctx_print_yenc (CtxFormatter *formatter, uint8_t *data, int length)
{
  char *tmp = (char*)ctx_malloc (length * 2 + 2);// worst case scenario
  int enclength = ctx_yenc ((char*)data, tmp, length);
  data[enclength]=0;
  ctx_formatter_addstr (formatter, " =", 2);
  ctx_formatter_addstr (formatter, tmp, enclength);
  ctx_formatter_addstr (formatter, "=y ", 2);
  ctx_free (tmp);
}

static void
ctx_print_escaped_string (CtxFormatter *formatter, const char *string)
{
  if (!string) { return; }
  for (int i = 0; string[i]; i++)
    {
      switch (string[i])
        {
          case '"':
            ctx_formatter_addstr (formatter, "\\\"", 2);
            break;
          case '\\':
            ctx_formatter_addstr (formatter, "\\\\", 2);
            break;
          case '\n':
            ctx_formatter_addstr (formatter, "\\n", 2);
            break;
          default:
            ctx_formatter_addstr (formatter, &string[i], 1);
        }
    }
}


static void
ctx_print_entry (CtxFormatter *formatter, CtxEntry *entry, int args)
{
  _ctx_print_name (formatter, entry->code);
  for (int i = 0; i <  args; i ++)
    {
      float val = ctx_arg_float (i);
      if (i>0 /* && val >= 0.0f */)
        {
          if (formatter->longform)
            {
              ctx_formatter_addstr (formatter, ", ", 2);
            }
          else
            {
              ctx_formatter_addstr (formatter, " ", 1);
            }
        }
      ctx_print_float (formatter, val);
    }
  _ctx_print_endcmd (formatter);
}

static void
ctx_print_glyph (CtxFormatter *formatter, CtxEntry *entry, int args)
{
  _ctx_print_name (formatter, entry->code);
  ctx_print_int (formatter, entry->data.u32[0]);
  _ctx_print_endcmd (formatter);
}

static void
ctx_formatter_process (void *user_data, CtxCommand *c);


static void
ctx_formatter_process (void *user_data, CtxCommand *c)
{
  CtxEntry *entry = &c->entry;
  CtxFormatter *formatter = (CtxFormatter*)user_data;

  switch (entry->code)
  //switch ((CtxCode)(entry->code))
    {
      case CTX_GLYPH:
        ctx_print_glyph (formatter, entry, 1);
        break;
      case CTX_LINE_TO:
      case CTX_REL_LINE_TO:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_MOVE_TO:
      case CTX_REL_MOVE_TO:
      case CTX_SMOOTHQ_TO:
      case CTX_REL_SMOOTHQ_TO:
        ctx_print_entry (formatter, entry, 2);
        break;
      case CTX_TEXTURE:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstr (formatter, "\"", -1);
        ctx_print_escaped_string (formatter, c->texture.eid);
        ctx_formatter_addstr (formatter, "\", ", 2);
        ctx_print_float (formatter, c->texture.x);
        ctx_formatter_addstr (formatter, ", ", 2);
        ctx_print_float (formatter, c->texture.y);
        ctx_formatter_addstr (formatter, " ", 1);
        _ctx_print_endcmd (formatter);
        break;

      case CTX_DEFINE_TEXTURE:
        {
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstr (formatter, "\"", 1);
        ctx_print_escaped_string (formatter, c->define_texture.eid);
        ctx_formatter_addstr (formatter, "\", ", 2);
        ctx_print_int (formatter, c->define_texture.width);
        ctx_formatter_addstr (formatter, ", ", 2);
        ctx_print_int (formatter, c->define_texture.height);
        ctx_formatter_addstr (formatter, ", ", 2);
        ctx_print_int (formatter, c->define_texture.format);
        ctx_formatter_addstr (formatter, ", ", 2);

        uint8_t *pixel_data = ctx_define_texture_pixel_data (entry);
#if 1

        int stride = ctx_pixel_format_get_stride ((CtxPixelFormat)c->define_texture.format, c->define_texture.width);
        int data_len = stride * c->define_texture.height;
        if (c->define_texture.format == CTX_FORMAT_YUV420)
          data_len = c->define_texture.height * c->define_texture.width +
                       2*(c->define_texture.height/2) * (c->define_texture.width/2);
        //fprintf (stderr, "encoding %i bytes\n", c->define_texture.height *stride);
        //ctx_print_a85 (formatter, pixel_data, c->define_texture.height * stride);
        ctx_print_yenc (formatter, pixel_data, data_len);
#else
        ctx_formatter_addstrf (formatter, "\"", 1);
        ctx_print_escaped_string (formatter, pixel_data);
        ctx_formatter_addstrf (formatter, "\" ");

#endif

        _ctx_print_endcmd (formatter);
        }
        break;

      case CTX_REL_ARC_TO:
      case CTX_ARC_TO:
        ctx_print_entry (formatter, entry, 7);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_print_entry (formatter, entry, 5);
        break;
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_RADIAL_GRADIENT:
        ctx_print_entry (formatter, entry, 6);
        break;
      case CTX_APPLY_TRANSFORM:
      case CTX_SOURCE_TRANSFORM:
        ctx_print_entry (formatter, entry, 9);
        break;
      case CTX_CONIC_GRADIENT:
      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_REL_QUAD_TO:
      case CTX_LINEAR_GRADIENT:
      case CTX_VIEW_BOX:
      case CTX_SMOOTH_TO:
      case CTX_REL_SMOOTH_TO:
        ctx_print_entry (formatter, entry, 4);
        break;
      case CTX_FONT_SIZE:
      case CTX_MITER_LIMIT:
      case CTX_ROTATE:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
      case CTX_STROKE_POS:
      case CTX_FEATHER:
      case CTX_LINE_HEIGHT:
      case CTX_WRAP_LEFT:
      case CTX_WRAP_RIGHT:
      case CTX_GLOBAL_ALPHA:
      case CTX_SHADOW_BLUR:
      case CTX_SHADOW_OFFSET_X:
      case CTX_SHADOW_OFFSET_Y:
      case CTX_VER_LINE_TO:
      case CTX_HOR_LINE_TO:
      case CTX_REL_VER_LINE_TO:
      case CTX_REL_HOR_LINE_TO:
        ctx_print_entry (formatter, entry, 1);
        break;
#if 0
      case CTX_SET:
        _ctx_print_name (formatter, entry->code);
        switch (c->set.key_hash)
        {
           case SQZ_x: ctx_formatter_addstrf (formatter, " 'x' "); break;
           case SQZ_y: ctx_formatter_addstrf (formatter, " 'y' "); break;
           case SQZ_width: ctx_formatter_addstrf (formatter, " width "); break;
           case SQZ_height: ctx_formatter_addstrf (formatter, " height "); break;
           default:
             ctx_formatter_addstrf (formatter, " %d ", c->set.key_hash);
        }
        ctx_formatter_addstrf (formatter, "\"", 1);
        ctx_print_escaped_string (formatter, (char*)c->set.utf8);
        ctx_formatter_addstrf (formatter, "\"", 1);
        _ctx_print_endcmd (formatter);
        break;
#endif
      case CTX_COLOR:
        if (1)//formatter->longform)
          {
            _ctx_indent (formatter);
            int model = (int) c->set_color.model;
            const char *suffix="";
            if (model & 512)
            {
              model = model & 511;
              suffix = "S";
            }
            switch (model)
              {
                case CTX_GRAY:
                  ctx_formatter_addstr (formatter, "gray", 4);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);

                  ctx_print_float (formatter, c->graya.g);
                  break;
                case CTX_GRAYA:
                  ctx_formatter_addstr (formatter, "graya", 5);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);

                  ctx_print_float (formatter, c->graya.g);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->graya.a);
                  break;
                case CTX_RGBA:
                  if (c->rgba.a != 1.0f)
                  {
                    ctx_formatter_addstr (formatter, "rgba", 4);
                    ctx_formatter_addstr (formatter, suffix, -1);
                    ctx_formatter_addstr (formatter, " ", 1);

                    ctx_print_float (formatter, c->rgba.r);
                    ctx_formatter_addstr (formatter, " ", 1);
                    ctx_print_float (formatter, c->rgba.g);
                    ctx_formatter_addstr (formatter, " ", 1);
                    ctx_print_float (formatter, c->rgba.b);
                    ctx_formatter_addstr (formatter, " ", 1);
                    ctx_print_float (formatter, c->rgba.a);
                    break;
                  }
                  /* FALLTHROUGH */
                case CTX_RGB:
                  if (c->rgba.r == c->rgba.g && c->rgba.g == c->rgba.b)
                  {
                    ctx_formatter_addstr (formatter, "gray", 4);
                    ctx_formatter_addstr (formatter, suffix, -1);
                    ctx_formatter_addstr (formatter, " ", 1);

                    ctx_print_float (formatter, c->rgba.r);
                    ctx_formatter_addstr (formatter, " ", 1);
                    break;
                  }
                  ctx_formatter_addstr (formatter, "rgb", 3);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.r);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.g);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.b);
                  break;
                case CTX_DRGB:
                  ctx_formatter_addstr (formatter, "drgb", 4);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);

                  ctx_print_float (formatter, c->rgba.r);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.g);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.b);
                  break;
                case CTX_DRGBA:
                  ctx_formatter_addstr (formatter, "drgba", 5);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);

                  ctx_print_float (formatter, c->rgba.r);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.g);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.b);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->rgba.a);
                  break;
                case CTX_CMYK:
                  ctx_formatter_addstr (formatter, "cmyk", 4);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);

                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.k);
                  break;
                case CTX_CMYKA:
                  ctx_formatter_addstr (formatter, "cmyk", 5);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.k);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.a);
                  break;
                case CTX_DCMYK:
                  ctx_formatter_addstr (formatter, "dcmyk", 6);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.k);
                  break;
                case CTX_DCMYKA:
                  ctx_formatter_addstr (formatter, "dcmyka", 7);
                  ctx_formatter_addstr (formatter, suffix, -1);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.k);
                  ctx_formatter_addstr (formatter, " ", 1);
                  ctx_print_float (formatter, c->cmyka.a);
                  break;
              }
          }
        else
          {
            ctx_print_entry (formatter, entry, 1);
          }
        break;
      case CTX_SET_RGBA_U8:
        if (formatter->longform)
            _ctx_indent (formatter);

	if (ctx_arg_u8(3)==255)
	{
	  int components = 3;
	  if (
            (ctx_arg_u8 (0) ==
            ctx_arg_u8 (1)) &&
            (ctx_arg_u8 (1) ==
             ctx_arg_u8 (2)))
	  {
	    components = 1;
          }

        ctx_formatter_addstr (formatter, components==1?"gray":"rgb", -1);
        if (formatter->longform)
            ctx_formatter_addstr (formatter, " (", -1);

        for (int c = 0; c < components; c++)
          {
            if (c)
              {
                if (formatter->longform)
                  ctx_formatter_addstr (formatter, ", ", 2);
                else
                  ctx_formatter_addstr (formatter, " ", 1);
              }
            ctx_print_float (formatter, ctx_u8_to_float (ctx_arg_u8 (c) ) );
          }

	}
	else
	{

        ctx_formatter_addstr (formatter, "rgba", -1);
        if (formatter->longform)
            ctx_formatter_addstr (formatter, " (", -1);

        for (int c = 0; c < 4; c++)
          {
            if (c)
              {
                if (formatter->longform)
                  ctx_formatter_addstr (formatter, ", ", 2);
                else
                  ctx_formatter_addstr (formatter, " ", 1);
              }
            ctx_print_float (formatter, ctx_u8_to_float (ctx_arg_u8 (c) ) );
          }
	}
        _ctx_print_endcmd (formatter);
        break;
      case CTX_SET_PIXEL:
#if 0
        ctx_set_pixel_u8 (d_ctx,
                          ctx_arg_u16 (2), ctx_arg_u16 (3),
                          ctx_arg_u8 (0),
                          ctx_arg_u8 (1),
                          ctx_arg_u8 (2),
                          ctx_arg_u8 (3) );
#endif
        break;
      case CTX_FILL:
      case CTX_PAINT:
      case CTX_START_FRAME:
      case CTX_STROKE:
      case CTX_IDENTITY:
      case CTX_CLIP:
      case CTX_BEGIN_PATH:
      case CTX_CLOSE_PATH:
      case CTX_SAVE:
      case CTX_PRESERVE:
      case CTX_START_GROUP:
      case CTX_NEW_PAGE:
      case CTX_END_GROUP:
      case CTX_RESTORE:
      case CTX_STROKE_SOURCE:
        ctx_print_entry (formatter, entry, 0);
        break;
      case CTX_TEXT_ALIGN:
      case CTX_TEXT_BASELINE:
      case CTX_TEXT_DIRECTION:
      case CTX_FILL_RULE:
      case CTX_LINE_CAP:
      case CTX_LINE_JOIN:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
      case CTX_EXTEND:
      case CTX_IMAGE_SMOOTHING:
        ctx_print_entry_enum (formatter, entry, 1);
        break;
      case CTX_GRADIENT_STOP:
        _ctx_print_name (formatter, entry->code);
        ctx_print_float (formatter, ctx_arg_float (0));
        for (int c = 0; c < 4; c++)
          {
            ctx_formatter_addstr (formatter, " ", 1);
            ctx_print_float (formatter, ctx_u8_to_float (ctx_arg_u8 (4+c) ) );
          }
        _ctx_print_endcmd (formatter);
        break;
      case CTX_TEXT:
      case CTX_STROKE_TEXT:
      case CTX_FONT:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstr (formatter, "\"", 1);
        ctx_print_escaped_string (formatter, ctx_arg_string() );
        ctx_formatter_addstr (formatter, "\"", 1);
        _ctx_print_endcmd (formatter);
        break;
      case CTX_CONT:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_END_FRAME:
        break;

      case CTX_DEFINE_FONT:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstr (formatter, "\"", 1);
        ctx_print_escaped_string (formatter, ctx_arg_string());
        ctx_formatter_addstr (formatter, "\"", 1);
        _ctx_print_endcmd (formatter);
        // XXX: todo, also print license if present
        break;

      case CTX_KERNING_PAIR:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstr (formatter, "\"", 1);
        {
           uint8_t utf8[16];
           utf8[ctx_unichar_to_utf8 (c->kern.glyph_before, utf8)]=0;
           ctx_print_escaped_string (formatter, (char*)utf8);
           ctx_formatter_addstr (formatter, "\", \"", -1);
           utf8[ctx_unichar_to_utf8 (c->kern.glyph_after, utf8)]=0;
           ctx_print_escaped_string (formatter, (char*)utf8);
           ctx_formatter_addstr (formatter, "\", ", 3);
           ctx_print_float (formatter, c->kern.amount/256.0f);
           ctx_print_escaped_string (formatter, (char*)utf8);
        }
        _ctx_print_endcmd (formatter);
        break;

      case CTX_DEFINE_GLYPH:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstr (formatter, "\"", 1);
        {
           uint8_t utf8[16];
           utf8[ctx_unichar_to_utf8 (entry->data.u32[0], utf8)]=0;
           ctx_print_escaped_string (formatter, (char*)utf8);
           ctx_formatter_addstr (formatter, "\", ", 3);
           ctx_print_float (formatter, entry->data.u32[1]/256.0f);
        }
        _ctx_print_endcmd (formatter);
        break;
    }
}

void
ctx_render_stream (Ctx *ctx, FILE *stream, int longform)
{
  CtxIterator iterator;
  CtxFormatter formatter;
  formatter.target= stream;
  formatter.longform = longform;
  formatter.indent = 0;
  formatter.add_str = _ctx_stream_addstr;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_formatter_process (&formatter, command); }
  fwrite ("\n", 1, 1, stream);
}

void
ctx_render_fd (Ctx *ctx, int fd, int longform)
{
  CtxIterator iterator;
  CtxFormatter formatter;
  formatter.target   = (void*)((size_t)fd);
  formatter.longform = longform;
  formatter.indent   = 0;
  formatter.add_str  = _ctx_fd_addstr;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_formatter_process (&formatter, command); }
  write (fd, "\n", 1);
}

char *
ctx_render_string (Ctx *ctx, int longform, int *retlen)
{
  CtxString *string = ctx_string_new ("");
  CtxIterator iterator;
  CtxFormatter formatter;
  formatter.target= string;
  formatter.longform = longform;
  formatter.indent = 0;
  formatter.add_str = _ctx_string_addstr;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_formatter_process (&formatter, command); }
  char *ret = string->str;
  if (retlen)
    *retlen = string->length;
  ctx_string_free (string, 0);
  return ret;
}


#endif
