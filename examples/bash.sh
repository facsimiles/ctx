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

# received  frame_no
# presented frame_no

echo -ne "\e[?6150h"
stty -echo
v=0
event="event"
cx=20;cy=20;radius=30;
for a in b{1..1000000}; do

#vt_sync # we call this to sync before rendering

echo -ne "\e[2J\e[H\e[?7020h"
echo "clear
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
setkey \"x\" \"$v\"
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

sleep 0.5
v=$(($v+1))

if [ $v -gt 1000 ];then
  v=0
fi
  read -s event
  while [ $event =~ "idle" ]; do read -s event; done;
  while [ x"$event" != x"" ]; do
  case $event in
   "up")    cy=$(($cy - 1))   ;;
   "down")  cy=$(($cy + 1)) ;;
   "right") cx=$(($cx + 1)) ;;
   "left")  cx=$(($cx - 1)) ;;
   "=")     radius=$(($radius + 1)) ;;
   "+")     radius=$(($radius + 1)) ;;
   "-")     radius=$(($radius - 1)) ;;
   "q")  exit ;;
   "control-c") exit;;
   *"mouse-motion"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        ;;
   *"mouse-drag"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        ;;
   *"mouse-press"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        ;;
  esac
  last_event=$event
  event=""
  read -t 0.05 -s event
done
event=$last_event
done
