#!/bin/bash
#!/usr/bin/env -S ctx bash

# this demonstrates using the ctx events protocol in addition
# to vector protocol in one client.


# if this does not occur, the terminal is left in an unsurviable state,
# a shortcut should be added that fixes this

cleanup() {
  echo -ne "\e[2J\eH\e[?201l"
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

echo -ne "\e[2J\e[?201h"
stty -echo
v=0
event="event"
cx=20;cy=20;radius=30;
for a in b{1..1000000}; do

#vt_sync # sync before rendering

echo -ne "\e[2J\e[H\e[?200h"
echo -ne "reset
rectangle 0%  0% 100% 100%
rgba 0 0 0 1
fill
fontSize 40
moveTo 50 50
rgba 1 0 0 1
text \"$event $foo $a\"
rgba 1 1 1 1
lineWidth 2
rectangle 10% 10% 80% 80%
stroke
rectangle 20% 20% 60% 60%
stroke
rectangle 30% 30% 40% 40%
stroke
setkey \"x\" \"$v\"
beginPath
rectangle $((10+$v)) 10 30 30
rgba 0 0 1 1
fill
rectangle $((10+$v)) 50 30 30
beginPath
arc($cx, $cy, $radius, 0.0, 6.4, 0);
stroke  
flush
done ";

#sleep 0.5
v=$(($v+1))
event=""
ev=c

if [ $v -gt 1000 ];then
  v=0
fi

read -t 0.04 -s event

count=0

while [ "x$ev" == "xc" -a $count -lt 4 ];do
  ev=nope
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
   *"pm"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        ev=c
        ;;
   *"pd"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        ev=c
        ;;
   *"pp"*) 
        cx=`echo $event|cut -f 2 -d ' '`
        cy=`echo $event|cut -f 3 -d ' '`
        ;;
  esac

  if [ "x$event" != "x" ];then
    last_event=$event
  fi
  if [ "x$ev" == "xc" ];then
    event=""
    read -t 0.04 -s event
  fi
  count=$(($count+1))
done

event=$last_event
done

#done
