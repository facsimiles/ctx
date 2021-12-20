# nim terminal context for ctx 2d vector graphics
#
# This is a bare-bones implementation, that is enough for most drawing needs and no
# readback of state is similar to what the HTML 5 2d context for canvas provides.
# 
# missing:
#   cmyk (easy)
#   images/textures (medium, needs yenc encoder in nim for use of raw buffers)
#   event handling (medium, callback handling)
#   read back of state (hard, requires keeping stack of transforms and
#                       fill/stroke sources)
#
#

import  std/exitprocs

type
  Ctx* = ref object
    foo: bool      #  unused; kept as syntax reminder


  FillRule* = enum
    FillRuleWinding, FillRuleEvenOdd
  LineCap* = enum
    LineCapButt, LineCapRound, LineCapSquare
  LineJoin* = enum
    LineJoinBevel, LineJoinRound, LineJoinMiter

proc ctx_init()=
  write stdout, "\e[?1049h" # alt-screen on
  write stdout, "\e[?25l"   # text-cursor off
  write stdout, "\e[J\e[H"  # clear and home

#proc newCtx(): owned(Ctx) {.gcsafe, raises: [].}

proc newCtx(): Ctx =
  ctx_init()
  new Ctx

var gCtx: owned(Ctx)

proc getContext*(val:string): Ctx =
  if isNil(gCtx):
    gCtx = newCtx()
  result = gCtx

proc cleanup()=
  write stdout, "X \e[?1049l" # alt-screen off
  write stdout, "\e[?25h"   # text-cursor on

proc gray*(ctx:Ctx, val: float)=
  echo "gray ", val

proc graya*(ctx:Ctx, val,alpha:float)=
  echo "graya ", val, " ", alpha

proc rgb*(ctx:Ctx, r,g,b:float)=
  echo "rgb ", r, " ", g, " ", b

proc rgba*(ctx:Ctx, r,g,b,a:float)=
  echo "rgba ", r, " ", g, " ", b, " ", a

proc relArcTo*(ctx:Ctx, a,b,c,d,e,f:float)=
  echo "a ", a, " ", b, " ", c, " ", d, " ", e, " ", f

proc clip*(ctx:Ctx)=
  echo "b";

proc relCurveTo*(ctx:Ctx, cx0,cy0,cx1,cy1,x,y:float)=
  echo "c ",cx0, " ", cy0, " ", cx1, " ", cy1, " ", x, " ", y

proc translate*(ctx:Ctx, x,y:float)=
  write stdout,"e ", x, " ", y

proc linearGradient*(ctx:Ctx, x0,y0,x1,y1:float)=
  echo "f ", x0," ",y0," ",x1," ",y1

proc save*(ctx:Ctx)=
  write stdout,"g"

proc preserve*(ctx:Ctx)=
  write stdout,"j"

proc globalAlpha*(ctx:Ctx, val:float)=
  echo "ka ", val

proc setGlobalAlpha*(ctx:Ctx, val:float)=
  ctx.globalAlpha(val)

proc textBaseline*(ctx:Ctx, val:float)=
  echo "kb ", val

proc blendMode*(ctx:Ctx, val:string)=
  echo "kB ", val

proc lineCap*(ctx:Ctx, val:FillRule)=
  echo "kc ", val

proc setLineCap*(ctx:Ctx, val:FillRule)=
  ctx.lineCap(val)

proc fontSize*(ctx:Ctx, val:float)=
  echo "kf ", val

proc lineJoin*(ctx:Ctx, val:string)=
  echo "kj ", val

proc setLineJoin*(ctx:Ctx, val:string)=
  ctx.lineJoin(val)

proc miterLimit*(ctx:Ctx, val:float)=
  echo "kl ", val

proc setMiterLimit*(ctx:Ctx, val:float)=
  ctx.miterLimit(val)

proc compositingMode*(ctx:Ctx, val:string)=
  echo "km ", val

proc setCompositingMode*(ctx:Ctx, val:string)=
  ctx.compositingMode(val)

proc textAlign*(ctx:Ctx, val:string)=
  echo "kt ", val

proc setTextAlign*(ctx:Ctx, val:string)=
  ctx.textAlign(val)

proc setLineWidth*(ctx:Ctx, val:float)=
  echo "kw ", val

proc relLineTo*(ctx:Ctx, x, y:float)=
  echo "l ", x, " ", y

proc relMoveTo*(ctx:Ctx, x, y:float)=
  echo "m ", x, " ", y

proc font*(ctx:Ctx, val:string)=
  echo "n \"", val, "\""

proc radialGradient*(ctx:Ctx, a,b,c,d,e,f:float)=
  echo "o ", a, " ", b, " ", c, " ", d, " ", e, " ", f

proc gradientAddStop*(ctx:Ctx, a,b,c,d,e:float)=
  echo "p ", a, " ", b, " ", c, " ", d, " ", e

proc relQuadTo*(ctx:Ctx, a,b,c,d:float)=
  echo "q ", a, " ", b, " ", c, " ", d

proc rectangle*(ctx:Ctx, a,b,c,d:float)=
  echo "r ", a, " ", b, " ", c, " ", d

proc relSmoothTo*(ctx:Ctx, a,b,c,d:float)=
  echo "s ", a, " ", b, " ", c, " ", d

proc relSmoothQuadTo*(ctx:Ctx, a,b:float)=
  echo "t ", a, " ", b

proc strokeText*(ctx:Ctx, val:string)=
  echo "u \"", val, "\""

proc relVerLine*(ctx:Ctx, val:float)=
  echo "v ", val

proc glyph*(ctx:Ctx, val:int)=
  echo "w ", val

proc text*(ctx:Ctx, val:string)=
  echo "x \"", val, "\""

proc identity*(ctx:Ctx)=
  echo "y "

proc closePath*(ctx:Ctx)=
  echo "z "

proc arcTo*(ctx:Ctx, a,b,c,d,e,f:float)=
  echo "A ", a, " ", b, " ", c, " ", d, " ", e, " ", f

proc arc*(ctx:Ctx, a,b,c,d,e,f:float)=
  echo "B ", a, " ", b, " ", c, " ", d, " ", e, " ", f

proc curveTo*(ctx:Ctx, a,b,c,d,e,f:float)=
  echo "C ", a, " ", b, " ", c, " ", d, " ", e, " ", f

proc stroke*(ctx:Ctx)=
  echo "E"

proc fill*(ctx:Ctx)=
  echo "F"

proc restore*(ctx:Ctx)=
  echo "G"

proc horLineTo*(ctx:Ctx, val:float)=
  echo "H ", val

proc rotate*(ctx:Ctx, val:float)=
  echo "J ", val

proc lineTo*(ctx:Ctx, x,y:float)=
  echo "L ", x, " ", y

proc moveTo*(ctx:Ctx, x,y:float)=
  echo "M ", x, " ", y

proc beginPath*(ctx:Ctx)=
  echo "N"

proc scale*(ctx:Ctx, x,y:float)=
  echo "O ", x, " ", y

proc quadTo*(ctx:Ctx, a,b,c,d:float)=
  echo "Q ", a, " ", b, " ", c, " ", d

proc smoothTo*(ctx:Ctx, a,b,c,d:float)=
  echo "S ", a, " ", b, " ", c, " ", d

proc smoothQuadTo*(ctx:Ctx, x,y:float)=
  echo "T ", x, " ", y

proc reset*(ctx:Ctx)=
  echo "U"

proc verLineTo*(ctx:Ctx, val:float)=
  echo "V ", val

proc done*(ctx:Ctx)=
  echo "X "

proc roundRectangle*(ctx:Ctx, a,b,c,d,e:float)=
  echo "Y ", a, " ", b, " ", c, " ", d, " ", e

proc closePath2*(ctx:Ctx)=
  echo "Z"

proc transform*(ctx:Ctx, a,b,c,d,e,f:float)=
  echo "W ", a, " ", b, " ", c, " ", d, " ", e, " ", f

proc srokeSource*(ctx:Ctx)=
  echo "_ "

proc startGroup*(ctx:Ctx)=
  echo "{ "

proc endGroup*(ctx:Ctx)=
  echo "} "

proc startFrame*(ctx:Ctx)=
  write stdout, "\e[H\e[?200h reset "

proc endFrame*(ctx:Ctx)=
  echo "flush X"

addExitProc(cleanup)
