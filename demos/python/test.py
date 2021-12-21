#!/usr/bin/env python

import ctx2d
event = ""

ctx = ctx2d.Context()

for frame in range(20):

  ctx.startFrame()

  ctx.rgb(1.0, 0, 0).moveTo(100, 100).text("last event: " + event)
  ctx.moveTo(100, 120).text("frame: " + str(frame))


  ctx.moveTo(10,100)
  ctx.setLineWidth(10)
  ctx.lineTo(100,100).lineTo(0,10)
  ctx.rgba(1.0,1.0,1.0,0.5);
  ctx.lineJoin(1)
  ctx.stroke()


  ctx.endFrame()

  event = ctx2d.getch()
  if event == 'q':
    exit()




