import atexit

class Context():
  def __init__(self, width=-1, height=-1):
    print "\33[?1049h" # alt-screen on  
    print "\33[?25l"   # text-cursor off  
    print "\33[J\33[H" # clear and home
    
  def gray(self, val):
    print (self, "gray ", val)
    return self
  
  def graya(self, val,alpha):
    print "graya ", val, " ", alpha
    return self
  
  def rgb(self, r,g,b):
    print "rgb ", r, " ", g, " ", b
    return self
  
  def rgba(self, r,g,b,a):
    print "rgba ", r, " ", g, " ", b, " ", a
    return self
  
  def relArcTo(self, a,b,c,d,e,f):
    print "a ", a, " ", b, " ", c, " ", d, " ", e, " ", f
    return self
  
  def clip(self):
    print "b";
    return self
  
  def relCurveTo(self, cx0,cy0,cx1,cy1,x,y):
    print "c ",cx0, " ", cy0, " ", cx1, " ", cy1, " ", x, " ", y
    return self
  
  def translate(self, x,y):
    print "e ", x, " ", y
    return self
  
  def linearGradient(self, x0,y0,x1,y1):
    print "f ", x0," ",y0," ",x1," ",y1
    return self
  
  def save(self):
    print "g"
    return self
  
  def preserve(self):
    print "j"
    return self
  
  def globalAlpha(self, val):
    print "ka ", val
    return self
  
  def setGlobalAlpha(self, val):
    globalAlpha(self, val)
    return self
  
  def textBaseline(self, val):
    print "kb ", val
    return self
  
  def blendMode(self, val):
    print "kB ", val
    return self
  
  def lineCap(self, val):
    print "kc ", val
    return self
  
  def setLineCap(self, val):
    lineCap(self, val)
    return self
  
  def fontSize(self, val):
    print "kf ", val
    return self
  
  def lineJoin(self, val):
    print "kj ", val
    return self
  
  def setLineJoin(self, val):
    lineJoin(self, val)
    return self
  
  def miterLimit(self, val):
    print "kl ", val
    return self
  
  def setMiterLimit(self, val):
    miterLimit(self, val)
    return self
  
  def compositingMode(self, val):
    print "km ", val
    return self
  
  def setCompositingMode(self, val):
    compositingMode(self, val)
    return self
  
  def textAlign(self, val):
    print "kt ", val
    return self
  
  def setTextAlign(self, val):
    textAlign(self, val)
    return self
  
  def setLineWidth(self, val):
    print "kw ", val
    return self
  
  def relLineTo(self, x, y):
    print "l ", x, " ", y
    return self
  
  def relMoveTo(self, x, y):
    print "m ", x, " ", y
    return self
  
  def font(self, val):
    print "n \"", val, "\""
    return self
  
  def radialGradient(self, a,b,c,d,e,f):
    print "o ", a, " ", b, " ", c, " ", d, " ", e, " ", f
    return self
  
  def gradientAddStop(self, a,b,c,d,e):
    print "p ", a, " ", b, " ", c, " ", d, " ", e
    return self
  
  def relQuadTo(self, a,b,c,d):
    print "q ", a, " ", b, " ", c, " ", d
    return self
  
  def rectangle(self, a,b,c,d):
    print "r ", a, " ", b, " ", c, " ", d
    return self
  
  def relSmoothTo(self, a,b,c,d):
    print "s ", a, " ", b, " ", c, " ", d
    return self
  
  def relSmoothQuadTo(self, a,b):
    print "t ", a, " ", b
    return self
  
  def strokeText(self, val):
    print "u \"", val, "\""
    return self
  
  def relVerLine(self, val):
    print "v ", val
    return self
  
  def glyph(self, val):
    print "w ", val
    return self
  
  def text(self, val):
    print "x \"", val, "\""
    return self
  
  def identity(self):
    print "y"
    return self
  
  def closePath(self):
    print "z"
    return self
  
  def arcTo(self, a,b,c,d,e,f):
    print "A ", a, " ", b, " ", c, " ", d, " ", e, " ", f
    return self
  
  def arc(self, a,b,c,d,e,f):
    print "B ", a, " ", b, " ", c, " ", d, " ", e, " ", f
    return self
  
  def curveTo(self, a,b,c,d,e,f):
    print "C ", a, " ", b, " ", c, " ", d, " ", e, " ", f
    return self
  
  def stroke(self):
    print "E"
    return self
  
  def fill(self):
    print "F"
    return self
  
  def restore(self):
    print "G"
    return self
  
  def horLineTo(self, val):
    print "H", val
    return self
  
  def rotate(self, val):
    print "J", val
    return self
  
  def lineTo(self, x,y):
    print "L ", x, " ", y
    return self
  
  def moveTo(self, x,y):
    print "M ", x, " ", y
    return self
  
  def beginPath(self):
    print "N"
    return self
  
  def scale(self, x,y):
    print "O ", x, " ", y
    return self
  
  def quadTo(self, a,b,c,d):
    print "Q ", a, " ", b, " ", c, " ", d
    return self
  
  def smoothTo(self, a,b,c,d):
    print "S ", a, " ", b, " ", c, " ", d
    return self
  
  def smoothQuadTo(self, x,y):
    print "T ", x, " ", y
    return self
  
  def reset(self):
    print "U"
    return self
  
  def verLineTo(self, val):
    print "V", val
    return self
  
  def done(self):
    print "X"
    return self
  
  def roundRectangle(self, a,b,c,d,e):
    print "Y ", a, " ", b, " ", c, " ", d, " ", e
    return self
  
  def closePath2(self):
    print "Z"
    return self
  
  def transform(self, a,b,c,d,e,f):
    print "W ", a, " ", b, " ", c, " ", d, " ", e, " ", f
    return self
  
  def strokeSource(self):
    print "_"
    return self
  
  def startGroup(self):
    print "{"
    return self
  
  def endGroup(self):
    print "}"
    return self
  
  def startFrame(self):
    print "\33[H\33[?200h U "
    return self
  
  def endFrame(self):
    print "; X"
    return self


#  getch()  copypasted from stackoverflow
#           we want something using the same APIs that keeps us in
#           raw mode
# 
def _find_getch():
    try:
        import termios
    except ImportError:
        # Non-POSIX. Return msvcrt's (Windows') getch.
        import msvcrt
        return msvcrt.getch

    # POSIX system. Create and return a getch that manipulates the tty.
    import sys, tty
    def _getch():
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    return _getch

getch = _find_getch()


def cleanup():
  print "X \33[?1049l" # alt-screen off
  print "\33[?25h"     # text-cursor on

atexit.register(cleanup)

