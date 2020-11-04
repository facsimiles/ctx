#include "ctx-split.h"

#if CTX_FORMATTER

static void _ctx_print_endcmd (FILE *stream, int longform)
{
  if (longform)
    {
      fwrite (");\n", 3, 1, stream);
    }
}

static void _ctx_indent (FILE *stream, int level)
{
  for (int i = 0; i < level; i++)
    { fwrite ("  ", 1, 2, stream); }
}

const char *_ctx_code_to_name (int code)
{
      switch (code)
        {

                case CTX_REL_LINE_TO_X4: return "relLinetoX4"; break;
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
          case CTX_SET_PIXEL:            return "setPixel"; break;
          case CTX_GLOBAL_ALPHA:         return "globalAlpha"; break;
          case CTX_TEXT:                 return "text"; break;
          case CTX_TEXT_STROKE:          return "textStroke"; break;
          case CTX_SAVE:                 return "save"; break;
          case CTX_RESTORE:              return "restore"; break;
          case CTX_NEW_PAGE:             return "newPage"; break;
          case CTX_START_GROUP:          return "startGroup"; break;
          case CTX_END_GROUP:            return "endGroup"; break;
          case CTX_RECTANGLE:            return "rectangle"; break;
          case CTX_ROUND_RECTANGLE:      return "roundRectangle"; break;
          case CTX_LINEAR_GRADIENT:      return "linearGradient"; break;
          case CTX_RADIAL_GRADIENT:      return "radialGradient"; break;
          case CTX_GRADIENT_STOP:        return "gradientAddStop"; break;
          case CTX_VIEW_BOX:             return "viewBox"; break;
          case CTX_MOVE_TO:              return "moveTo"; break;
          case CTX_LINE_TO:              return "lineTo"; break;
          case CTX_BEGIN_PATH:           return "beginPath"; break;
          case CTX_REL_MOVE_TO:          return "relMoveTo"; break;
          case CTX_REL_LINE_TO:          return "relLineTo"; break;
          case CTX_FILL:                 return "fill"; break;
          case CTX_EXIT:                 return "exit"; break;
          case CTX_APPLY_TRANSFORM:      return "transform"; break;
          case CTX_REL_ARC_TO:           return "relArcTo"; break;
          case CTX_GLYPH:                return "glyph"; break;
          case CTX_TEXTURE:              return "texture"; break;
          case CTX_IDENTITY:             return "identity"; break;
          case CTX_CLOSE_PATH:           return "closePath"; break;
          case CTX_PRESERVE:             return "preserve"; break;
          case CTX_FLUSH:                return "flush"; break;
          case CTX_RESET:                return "reset"; break;
          case CTX_FONT:             return "font"; break;
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
          case CTX_TEXT_ALIGN:           return "textAlign"; break;
          case CTX_TEXT_BASELINE:        return "textBaseline"; break;
          case CTX_TEXT_DIRECTION:       return "textDirection"; break;
          case CTX_FONT_SIZE:            return "fontSize"; break;
          case CTX_MITER_LIMIT:          return "miterLimit"; break;
          case CTX_LINE_JOIN:            return "lineJoin"; break;
          case CTX_LINE_CAP:             return "lineCap"; break;
          case CTX_LINE_WIDTH:           return "lineWidth"; break;
          case CTX_SHADOW_BLUR:          return "shadowBlur";  break;
          case CTX_FILL_RULE:            return "fillRule"; break;
          case CTX_SET:                  return "setProp"; break;
        }
      return NULL;
}

static void _ctx_print_name (FILE *stream, int code, int longform, int *indent)
{
#define CTX_VERBOSE_NAMES 1
#if CTX_VERBOSE_NAMES
  if (longform)
    {
      const char *name = NULL;
      _ctx_indent (stream, *indent);
      //switch ((CtxCode)code)
      name = _ctx_code_to_name (code);
      if (name)
        {
          fwrite (name, 1, strlen ( (char *) name), stream);
          fwrite (" (", 1, 2, stream);
          if (code == CTX_SAVE)
            { (*indent)++; }
          else if (code == CTX_RESTORE)
            { (*indent)--; }
          return;
        }
    }
#endif
  {
    uint8_t name[3];
    name[0]=CTX_SET_KEY;
    name[2]='\0';
    switch (code)
      {
        case CTX_GLOBAL_ALPHA:
          name[1]='a';
          break;
        case CTX_COMPOSITING_MODE:
          name[1]='m';
          break;
        case CTX_BLEND_MODE:
          name[1]='B';
          break;
        case CTX_TEXT_ALIGN:
          name[1]='t';
          break;
        case CTX_TEXT_BASELINE:
          name[1]='b';
          break;
        case CTX_TEXT_DIRECTION:
          name[1]='d';
          break;
        case CTX_FONT_SIZE:
          name[1]='f';
          break;
        case CTX_MITER_LIMIT:
          name[1]='l';
          break;
        case CTX_LINE_JOIN:
          name[1]='j';
          break;
        case CTX_LINE_CAP:
          name[1]='c';
          break;
        case CTX_LINE_WIDTH:
          name[1]='w';
          break;
        case CTX_SHADOW_BLUR:
          name[1]='s';
          break;
        case CTX_SHADOW_COLOR:
          name[1]='C';
          break;
        case CTX_SHADOW_OFFSET_X:
          name[1]='x';
          break;
        case CTX_SHADOW_OFFSET_Y:
          name[1]='y';
          break;
        case CTX_FILL_RULE:
          name[1]='r';
          break;
        default:
          name[0] = code;
          name[1] = 0;
          break;
      }
    fwrite (name, 1, strlen ( (char *) name), stream);
    fwrite (" ", 1, 1, stream);
    if (longform)
      { fwrite ("(", 1, 1, stream); }
  }
}

static void
ctx_print_entry_enum (FILE *stream, int longform, int *indent, CtxEntry *entry, int args)
{
  _ctx_print_name (stream, entry->code, longform, indent);
  for (int i = 0; i <  args; i ++)
    {
      int val = ctx_arg_u8 (i);
      if (i>0)
        { fwrite (" ", 1, 1, stream); }
#if CTX_VERBOSE_NAMES
      if (longform)
        {
          const char *str = NULL;
          switch (entry->code)
            {
              case CTX_TEXT_BASELINE:
                switch (val)
                  {
                    case CTX_TEXT_BASELINE_ALPHABETIC:
                      str = "alphabetic";
                      break;
                    case CTX_TEXT_BASELINE_TOP:
                      str = "top";
                      break;
                    case CTX_TEXT_BASELINE_BOTTOM:
                      str = "bottom";
                      break;
                    case CTX_TEXT_BASELINE_HANGING:
                      str = "hanging";
                      break;
                    case CTX_TEXT_BASELINE_MIDDLE:
                      str = "middle";
                      break;
                    case CTX_TEXT_BASELINE_IDEOGRAPHIC:
                      str = "ideographic";
                      break;
                  }
                break;
              case CTX_TEXT_ALIGN:
                switch (val)
                  {
                    case CTX_TEXT_ALIGN_LEFT:
                      str = "left";
                      break;
                    case CTX_TEXT_ALIGN_RIGHT:
                      str = "right";
                      break;
                    case CTX_TEXT_ALIGN_START:
                      str = "start";
                      break;
                    case CTX_TEXT_ALIGN_END:
                      str = "end";
                      break;
                    case CTX_TEXT_ALIGN_CENTER:
                      str = "center";
                      break;
                  }
                break;
              case CTX_LINE_CAP:
                switch (val)
                  {
                    case CTX_CAP_NONE:
                      str = "none";
                      break;
                    case CTX_CAP_ROUND:
                      str = "round";
                      break;
                    case CTX_CAP_SQUARE:
                      str = "square";
                      break;
                  }
                break;
              case CTX_LINE_JOIN:
                switch (val)
                  {
                    case CTX_JOIN_MITER:
                      str = "miter";
                      break;
                    case CTX_JOIN_ROUND:
                      str = "round";
                      break;
                    case CTX_JOIN_BEVEL:
                      str = "bevel";
                      break;
                  }
                break;
              case CTX_FILL_RULE:
                switch (val)
                  {
                    case CTX_FILL_RULE_WINDING:
                      str = "winding";
                      break;
                    case CTX_FILL_RULE_EVEN_ODD:
                      str = "evenodd";
                      break;
                  }
                break;
              case CTX_BLEND_MODE:
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
            { fprintf (stream, "%s", str); }
          else
            { fprintf (stream, "%i", val); }
        }
      else
#endif
        {
          fprintf (stream, "%i", val);
        }
    }
  _ctx_print_endcmd (stream, longform);
}

static void
ctx_print_escaped_string (FILE *stream, const char *string)
{
  if (!string | !stream) { return; }
  for (int i = 0; string[i]; i++)
    {
      switch (string[i])
        {
          case '"':
            fwrite ("\\\"", 1, 2, stream);
            break;
          case '\\':
            fwrite ("\\\\", 1, 2, stream);
            break;
          case '\n':
            fwrite ("\\n", 1, 2, stream);
            break;
          default:
            fwrite (&string[i], 1, 1, stream);
        }
    }
}

static void
ctx_print_float (FILE *stream, float val)
{
  char temp[128];
  sprintf (temp, "%0.3f", val);
  int j;
  for (j = 0; temp[j]; j++)
    if (j == ',') { temp[j] = '.'; }
  j--;
  if (j>0)
    while (temp[j] == '0')
      {
        temp[j]=0;
        j--;
      }
  if (temp[j]=='.')
    { temp[j]='\0'; }
  fwrite (temp, 1, strlen (temp), stream);
}

static void
ctx_print_entry (FILE *stream, int longform, int *indent, CtxEntry *entry, int args)
{
  _ctx_print_name (stream, entry->code, longform, indent);
  for (int i = 0; i <  args; i ++)
    {
      float val = ctx_arg_float (i);
      if (i>0 && val >= 0.0f)
        {
          if (longform)
            {
                fwrite (", ", 2, 1, stream);
            }
          else
            {
              if (val >= 0.0f)
                { fwrite (" ", 1, 1, stream); }
            }
        }
      ctx_print_float (stream, val);
    }
  _ctx_print_endcmd (stream, longform);
}

static void
ctx_print_glyph (FILE *stream, int longform, int *indent, CtxEntry *entry, int args)
{
  char buf[32];
  _ctx_print_name (stream, entry->code, longform, indent);
  if (longform)
  {
    sprintf (buf, "%i", entry->data.u32[0]);
  }
  else
  {
    sprintf (buf, "%i", entry->data.u32[0]);
  }
  fwrite (buf, 1, strlen (buf), stream);

  _ctx_print_endcmd (stream, longform);
}

static void
ctx_stream_process (void *user_data, CtxCommand *c);

typedef struct _CtxFormatter  CtxFormatter;
struct _CtxFormatter 
{
  void *target; // FILE
  int   longform;
  int   indent;

  void (*add_str)(CtxFormatter *formatter, int len, const char *str);
};


void
ctx_render_stream (Ctx *ctx, FILE *stream, int longform)
{
  CtxIterator iterator;
  CtxFormatter formatter;
  formatter.target= stream;
  formatter.longform = longform;
  formatter.indent = 0;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->renderstream, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_stream_process (&formatter, command); }
  fprintf (stream, "\n");
}


static void
ctx_stream_process (void *user_data, CtxCommand *c)
{
  CtxEntry *entry = &c->entry;
  CtxFormatter *formatter = user_data;
  FILE *stream    = formatter->target;
  int   longform  = formatter->longform;
  int  *indent    = &(formatter->indent);

  switch (entry->code)
  //switch ((CtxCode)(entry->code))
    {
      case CTX_GLYPH:
        ctx_print_glyph (stream, longform, indent, entry, 1);
        break;
        break;
      case CTX_LINE_TO:
      case CTX_REL_LINE_TO:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_MOVE_TO:
      case CTX_REL_MOVE_TO:
      case CTX_SMOOTHQ_TO:
      case CTX_REL_SMOOTHQ_TO:
        ctx_print_entry (stream, longform, indent, entry, 2);
        break;
      case CTX_TEXTURE:
        ctx_print_entry (stream, longform, indent, entry, 3);
        break;
      case CTX_REL_ARC_TO:
      case CTX_ARC_TO:
      case CTX_ROUND_RECTANGLE:
        ctx_print_entry (stream, longform, indent, entry, 5);
        break;
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_RADIAL_GRADIENT:
      case CTX_APPLY_TRANSFORM:
        ctx_print_entry (stream, longform, indent, entry, 6);
        break;
      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_REL_QUAD_TO:
      case CTX_LINEAR_GRADIENT:
      case CTX_VIEW_BOX:
      case CTX_SMOOTH_TO:
      case CTX_REL_SMOOTH_TO:
        ctx_print_entry (stream, longform, indent, entry, 4);
        break;
      case CTX_FONT_SIZE:
      case CTX_MITER_LIMIT:
      case CTX_ROTATE:
      case CTX_LINE_WIDTH:
      case CTX_GLOBAL_ALPHA:
      case CTX_SHADOW_BLUR:
      case CTX_SHADOW_OFFSET_X:
      case CTX_SHADOW_OFFSET_Y:
      case CTX_VER_LINE_TO:
      case CTX_HOR_LINE_TO:
      case CTX_REL_VER_LINE_TO:
      case CTX_REL_HOR_LINE_TO:
        ctx_print_entry (stream, longform, indent, entry, 1);
        break;
      case CTX_SET:
        _ctx_print_name (stream, entry->code, longform, indent);
        switch (c->set.key_hash)
        {
           case CTX_x: fprintf (stream, " 'x' "); break;
           case CTX_y: fprintf (stream, " 'y' "); break;
           case CTX_width: fprintf (stream, " width "); break;
           case CTX_height: fprintf (stream, " height "); break;
           default:
             fprintf (stream, " %d ", c->set.key_hash);
        }
        fprintf (stream, "\"");
        ctx_print_escaped_string (stream, (char*)c->set.utf8);
        fprintf (stream, "\"");
        _ctx_print_endcmd (stream, longform);
        break;
      case CTX_COLOR:
        if (longform ||  1)
          {
            _ctx_indent (stream, *indent);
            switch ( (int) c->set_color.model)
              {
                case CTX_GRAY:
                  fprintf (stream, "gray ");
                  ctx_print_float (stream, c->graya.g);
                  break;
                case CTX_GRAYA:
                  fprintf (stream, "graya ");
                  ctx_print_float (stream, c->graya.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->graya.a);
                  break;
                case CTX_RGBA:
                  if (c->rgba.a != 1.0)
                  {
                    fprintf (stream, "rgba ");
                    ctx_print_float (stream, c->rgba.r);
                    fprintf (stream, " ");
                    ctx_print_float (stream, c->rgba.g);
                    fprintf (stream, " ");
                    ctx_print_float (stream, c->rgba.b);
                    fprintf (stream, " ");
                    ctx_print_float (stream, c->rgba.a);
                    break;
                  }
                case CTX_RGB:
                  if (c->rgba.r == c->rgba.g && c->rgba.g == c->rgba.b)
                  {
                    fprintf (stream, "gray ");
                    ctx_print_float (stream, c->rgba.r);
                    fprintf (stream, " ");
                    break;
                  }
                  fprintf (stream, "rgb ");
                  ctx_print_float (stream, c->rgba.r);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.b);
                  break;
                case CTX_DRGB:
                  fprintf (stream, "drgb ");
                  ctx_print_float (stream, c->rgba.r);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.b);
                  break;
                case CTX_DRGBA:
                  fprintf (stream, "drgba ");
                  ctx_print_float (stream, c->rgba.r);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.g);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.b);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->rgba.a);
                  break;
                case CTX_CMYK:
                  fprintf (stream, "cmyk ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  break;
                case CTX_CMYKA:
                  fprintf (stream, "cmyka ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.a);
                  break;
                case CTX_DCMYK:
                  fprintf (stream, "dcmyk ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  break;
                case CTX_DCMYKA:
                  fprintf (stream, "dcmyka ");
                  ctx_print_float (stream, c->cmyka.c);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.m);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.y);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.k);
                  fprintf (stream, " ");
                  ctx_print_float (stream, c->cmyka.a);
                  break;
              }
          }
        else
          {
            ctx_print_entry (stream, longform, indent, entry, 1);
          }
        break;
      case CTX_SET_RGBA_U8:
        if (longform)
          {
            _ctx_indent (stream, *indent);
            fwrite ("rgba (", 6, 1, stream);
          }
        else
          {
            fwrite ("rgba (", 5, 1, stream);
          }
        for (int c = 0; c < 4; c++)
          {
            if (c)
              {
                if (longform)
                  { fwrite (", ", 2, 1, stream); }
                else
                  { fwrite (" ", 1, 1, stream); }
              }
            ctx_print_float (stream, ctx_u8_to_float (ctx_arg_u8 (c) ) );
          }
        _ctx_print_endcmd (stream, longform);
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
      case CTX_RESET:
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
        ctx_print_entry (stream, longform, indent, entry, 0);
        break;
      case CTX_TEXT_ALIGN:
      case CTX_TEXT_BASELINE:
      case CTX_TEXT_DIRECTION:
      case CTX_FILL_RULE:
      case CTX_LINE_CAP:
      case CTX_LINE_JOIN:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
        ctx_print_entry_enum (stream, longform, indent, entry, 1);
        break;
      case CTX_GRADIENT_STOP:
        _ctx_print_name (stream, entry->code, longform, indent);
        for (int c = 0; c < 4; c++)
          {
            if (c)
              { fwrite ("  ", 1, 1, stream); }
            ctx_print_float (stream, ctx_u8_to_float (ctx_arg_u8 (4+c) ) );
          }
        _ctx_print_endcmd (stream, longform);
        break;
      case CTX_TEXT:
      case CTX_TEXT_STROKE:
      case CTX_FONT:
        _ctx_print_name (stream, entry->code, longform, indent);
        fprintf (stream, "\"");
        ctx_print_escaped_string (stream, ctx_arg_string() );
        fprintf (stream, "\"");
        _ctx_print_endcmd (stream, longform);
        break;
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
      case CTX_REPEAT_HISTORY:
        break;
    }
}



#endif
