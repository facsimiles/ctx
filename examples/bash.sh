#!/bin/bash
#!/usr/bin/env -S ctx bash

cleanup() {
  echo -ne "\e[2J\eH\e[?6150l"
  stty echo
}
trap cleanup EXIT

function vt_sync(){
  # this requests a status report from the terminal,
  # it does a round-trip and will not proceed until buffered
  # data before the request so a response is generated.
  #
  # waiting for a specific message/mark for rendered and
  # presented, as well as ability to set discard policy
  # would be good

  echo -en "\e[5n"
  read -s -N 1 ch
  while [ x"$ch" != x"n" ]; do 
	read -s -N 1 ch ;
  done
}

echo -ne "\e[?6150h"
stty -echo
v=0
event="event"
dirty=true
cx=20;cy=20;radius=30;
for a in b{1..1000000}; do

#vt_sync # we call this to sync before rendering

if [ $dirty = true ];then 
echo -ne "\e[2J\e[H\e[?7020h\n"
echo "reset
rectangle 0%  0% 100% 100%
rgba 0 0 0 1
fill
font_size 40
move_to 50 50
rgba 1 0 0 1
text \"$event $foo\"
rgba 1 1 1 1
line_width 2
rectangle 10% 10% 80% 80%
stroke
rectangle 20% 20% 60% 60%
stroke
rectangle 30% 30% 40% 40%
stroke
#setkey \"x\" \"$v\"
new_path
rectangle $((10+$v)) 10 30 30
rgba 0 0 1 1
fill
rectangle $((10+$v)) 50 30 30
new_path
arc($cx, $cy, $radius, 0.0, 6.4, 0);
stroke  
flush
done
"; 
  dirty=false
fi

  IFS=$'\n' read -s event 
  case $event in
   "up")    cy=$(($cy - 1)) ;dirty=true  ;;
   "down")  cy=$(($cy + 1)) ;dirty=true  ;;
   "right") cx=$(($cx + 1)) ;dirty=true  ;;
   "left")  cx=$(($cx - 1)) ;dirty=true ;;
   "=")     radius=$(($radius + 1)) ; dirty=true ;;
   "+")     radius=$(($radius + 1)) ; dirty=true ;;
   "-")     radius=$(($radius - 1)) ; dirty=true ;;
   "a")     echo -en "\e[?7020h\nsetkey 'x' '200' X\n"; dirty=true ;;
   "b")     echo -en "\e[?7020h\nsetkey 'x' '300' X\n"; dirty=true ;;
   "q")     exit ;;
   "idle") sleep 0.1 ;;
   "control-c") exit;;
   "mouse-motion"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        ;;
   "mouse-drag"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        dirty=true
        ;;
   *"mouse-press"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        dirty=true
        ;;
    *) dirty=true
            ;;
  esac
done
