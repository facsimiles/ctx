import canvas

ctx=canvas.ctx

down = False

def on_press(e):
    global down
    down = True

def on_release(e):
    global down
    down = False

x = ctx.width/2
y = ctx.height/2

def on_motion(e):
  global x,y
  x = e.x
  y = e.y

#machine.freq(200_000_000)
max_frame = 100.0
for frame_no in range(0,max_frame):
   ctx.start_frame()
   ctx.font_size = ctx.height * 0.1

   ctx.rectangle(0,0,ctx.width,ctx.height)
   ctx.listen(ctx.PRESS, on_press, False)
   ctx.listen(ctx.RELEASE, on_release, False)
   ctx.listen(ctx.MOTION, on_motion, False) 
  
   if (down):
     ctx.color([0,111,222])
   else:
     ctx.color([255,255,255])
 
   ctx.fill()
   dim = (frame_no/max_frame) * ctx.height
   if frame_no >= max_frame-2:
       dim = ctx.height
   ctx.logo(x,y, dim)
   ctx.end_frame()
   frame_no+=1
   
