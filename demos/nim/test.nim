import ctx2d
import terminal

var ctx = getContext("2d")
var a:char

for i in 0 .. 10:
  ctx.startFrame

  #ctx.translate i.float*10, i.float*20
  ctx.rgb 0,0,0
  ctx.rectangle 0,0,1000,1000
  ctx.fill

  ctx.rgb 1, 0, 0
  ctx.moveTo 20*i.float, 20*i.float
  ctx.lineTo 100+20*i.float, 20+20*i.float
  ctx.lineTo 60, 400
  ctx.fill

  ctx.rgb 1,1,1
  ctx.moveTo 300, 300
  ctx.text "hello" & "frame:" & $i & "  a:  " & $(a.int)
  
  ctx.endFrame
  a = getch()


