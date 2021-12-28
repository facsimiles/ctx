#!/bin/bash

# the latter part is enough to act as an import when installed
source ctx2d.bash || source `command -v ctx2d.bash`

function zoom_word(){
for frame in {1..99}; do
  startFrame
  rectangle 0 0 100% 100%
  if [ $frame -lt 10 ];then
    gray ".0"$frame
  else
    gray "0."$frame
  fi
  fill
  rgba 0.2 0.2 0.7 1
  fontSize $((frame))%
  moveTo 50% 50%
  textBaseline middle
  textAlign center
  text "$1"
  update_event
  endFrame
  #sleep 0.005
done
}

zoom_word "ctx"

event=''

while [ x$event != xq ];do
  startFrame

  rgba 0.2 0.2 0.7 1
  fontSize $((5+frame/4))%
  moveTo 50% 50%
  textAlign center
  textBaseline middle
  if [ x$event != x ]; then message=$event; fi
  text "$1 $message"
  endFrame
  update_event
done


