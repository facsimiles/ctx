# you can paste a micropython script here,
# the filesystem is preseeded with a card10
# simulation firmware - and some card10
# applications might work without modification
#
# ctrl+return runs the script currently in the editor
# ctrl+s saves you can also create new files and
#        paths when saving, a reload of the
#        page clears the filesystem.
#
# Click in the display to get keyboard focus, 
# the cursor keys are the buttons, up is escape
# and down is select.

import ctx
import micropython

maxframe = 80.0  # adjust this to change
                  # the number of frames for each demo loop
def linear(start_val,end_val):
  return (frame/maxframe)*(end_val-start_val)+start_val

pressed=False

def draw_tvg(o, path, x, y, tdim):
  o.save()
  dim=o.tinyvg_get_size(path)
  o.translate(x,y)
  scale=tdim/dim[0];
  scaley=tdim/dim[1];
  if scale > scaley:
    scale=scaley
  o.scale(scale, scale)
  o.translate(-dim[0]/2, -dim[1]/2)
  o.tinyvg_draw(path)
  o.restore()

def press_cb (e):
  global pressed
  pressed = True
  
def release_cb (e):
  global pressed
  pressed = False
  
  
def zoom_text(o, string):
  o.font_size=linear(0,o.height/4)
  
  o.text_align=ctx.CENTER
  o.text_baseline=ctx.MIDDLE
  if pressed:
    o.move_to(o.width/2+0.5,o.height*0.8+0.5)
    o.color([0.0,0.0,0.5]).text(string)
    o.move_to(o.width/2+0.5,o.height*0.8+0.5)
    o.color([1.0,1.0,1.0]).text(string)
  else:
    o.move_to(o.width/2+0.5,o.height*0.8+0.5)
    o.color([0.0,0.0,0.5]).text(string)
    o.move_to(o.width/2,o.height*0.8)
    o.color([1.0,1.0,1.0]).text(string)
  o.rectangle(0,0,o.width,o.height)
  o.listen(ctx.PRESS, press_cb) 
  o.listen(ctx.RELEASE, release_cb) 

  o.begin_path()
    
o=ctx.get_context("2d") # mirroring the web,
                        # this gives us an
                        # interactive ctx context

for frame in range(0,maxframe):
  o.start_frame()
  o.color([0,0,0]).paint()
  o.global_alpha=linear(0.0, 1.0)
  o.logo(o.width/2,o.height/2, o.height)
  o.end_frame()

long_text="""ctx itself doesn't provide wordwrapping, but it provides facilities that
make it possible to implement in micropython, measuring words as well as making use
of the moving current point makes it possible.

This example is text rotated 90 degrees, and wrapped to fit the size of the canvas.

Reading rewrapping text is difficult.
"""

for frame in range(0,maxframe):
  o.start_frame()
  o.color([255,255,255]).paint()
  o.save()
  o.color([0,0,0])
  o.rotate(3.1415/2)
  font_size = o.height * linear(0.09, 0.14)
  o.font_size=font_size
  o.move_to(font_size/2,-o.width*0.9)
  
  space_width = o.text_width(' ')
  for i in long_text.split():
    word_width = o.text_width(i)
    if (o.x + word_width > o.height - font_size/2):
      o.move_to(font_size/2,o.y + font_size)
    o.text(i)
    o.move_to(o.x + space_width, o.y)
  o.restore()
  o.end_frame()
  
  
for frame in range(0,maxframe):
  o.start_frame()
  o.color([0,0,0]).paint()
  o.save()
  o.translate(o.width/2,o.height/2)
  o.rotate(linear(0, 3.1415*2))
  o.translate(-o.width/2,-o.height/2)
  o.logo(o.width/2,o.height/2, o.height)
  o.restore()
  o.end_frame()

for frame in range(0,maxframe):
  o.start_frame()
  o.color([0,0,0]).paint()
  o.logo(o.width/2,o.height/2, o.height)
  zoom_text(o, "ctx vector graphics")
  o.end_frame()

  
for frame in range(0,maxframe):
  o.start_frame()
  o.color([0,0,0]).paint()
  o.logo(o.width/2,o.height/2, o.height)
  zoom_text(o, "micropython")
  o.end_frame()

for frame in range(0,maxframe):
  o.start_frame()
  o.color([0,0,0]).paint()
  o.logo(o.width/2,o.height/2, o.height)
  zoom_text(o, "wasm")  
  o.end_frame()

    

for frame in range(0,maxframe):
  o.start_frame()
  o.color([0.0,0.0,0.0]).paint()
  draw_tvg(o, "fairydust.tvg", o.height/4, o.height/2, linear(o.height/2, o.height))
  draw_tvg(o, "chaos-knot.tvg", o.width/2, o.height/2, o.height/2)
  zoom_text(o, "tinyvg")  
  o.end_frame()

#for frame in range(0,maxframe):
#  o.start_frame()
#  o.color([0.0,0.0,0.0]).paint()
#  draw_tvg(o, "tiger.tvg", o.width/2, o.height/2, linear(o.height/2,o.height))
#  o.end_frame()

#import image
def test_texture(o):
  w, h, img = image.load("world-s.png")
  
  for frame in range(0,maxframe):
    o.start_frame()
    o.color([0.0,0.0,0.0]).paint()
    o.save()
    o.translate(o.width/2,o.height/2)
    o.scale(linear(2.0, 1.2), linear(2.0, 1.2))
    o.rotate(linear(-0.2,0.00))
    o.translate(-w/2,-h/2)
  
    o.texture(img, o.RGBA8, w, h, w*4).paint()
    o.restore()
    zoom_text(o, "stb_image")
    o.end_frame()
  
  for frame in range(0,maxframe):
    o.start_frame()
    o.color([0.0,0.0,0.0]).paint()
    o.logo(o.width/2,o.height/2, o.height*4)
    o.save()
    o.save()
    o.rotate(linear(0.0,0.4))
    o.rectangle(o.width * 0.2, o.height * 0.2,
                o.width * 0.9, o.height * 0.8);
    o.restore();
    o.clip();
    o.translate(o.width/2,o.height/2)
    o.scale(linear(2.0, 1.2), linear(2.0, 1.2))
    o.rotate(linear(-0.2,0.00))
    o.translate(-w/2,-h/2)
  
    o.texture(img, o.RGBA8, w, h, w*4).paint()
    o.restore()
    zoom_text(o, "clipping")
    o.restore()
    o.end_frame()
#test_texture(o)
  
for frame in range(0,maxframe):
  o.start_frame()
  o.color([0,0,0]).paint()  
  o.logo(o.width/2,o.height/2, linear(o.height*10,o.height))
  o.end_frame()


