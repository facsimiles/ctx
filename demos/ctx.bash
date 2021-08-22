#!/bin/bash
# this is a bash language binding for ctx, to use it:
#
#   source ctx.bash
#   ctx_init
#   startFrame
#   rgba 1 1 1 1
#   fontSize 10%
#   moveTo 0 12%
#   text "hello"
#   endFrame
#   sleep 4

function ctx_cleanup {
  echo -ne " X "        # leave ctx mode; in case we're there
  echo -ne "\e[?1049l"  # alt-screen off
  echo -ne "\e[?25h"    # text-cursor on
  clear
}
function ctx_init()
{
  echo -ne "\e[?200\$p"  #  DECRQM - request DEC mode 200
  read -s -t 0.1 -n 1 ch >  /dev/null
  tries=10; # for timing out on terminals without DECRQM
  while [ "$ch" != 'y' -a $tries != 0 ];do 
    # append read char until y
    terminal_response="$terminal_response$ch"
    tries=$(($tries+1))
    read -s -t 0.1 -n 1 ch > /dev/null
  done;
  terminal_response="$terminal_response$ch"
  # expected response is "\e?200;2$y", we only check 7th char
  if [ x${terminal_response:7:1} != x2 ];then
     echo "needs to run in ctx terminal";
     exit
  fi
  echo -ne "\e[?1049h" # alt-screen on
  echo -ne "\e[?25l"   # text-cursor off
  echo -ne "\e[J\e[H"  # clear and home
  trap ctx_cleanup EXIT # restore terminal state on ctrl+c and regular exit
}


function gray()            { echo "gray $1" ; }
function graya()           { echo "graya $1 $2" ; }
function rgb()             { echo "rgb $1 $2 $3 $4" ; }
function rgba()            { echo "rgba $1 $2 $3 $4" ; }

function relArcTo()        { echo "a $1 $2 $3 $4 $5 $6"; }
function clip()            { echo "b"; }
function relCurveTo()      { echo "c $1 $2 $3 $4 $5 $6"; }
function save()            { echo "g"; }
function translate()       { echo "e $1 $2" ; }
function linearGradient()  { echo "f $1 $2 $3 $4" ; }
function relHorLine()      { echo "h $1" ; }
function globalAlpha()     { echo "ka $1" ; }
function textBaseline()    { echo "kb $1" ; }
function blendMode()       { echo "kB $1" ; }
function lineCap()         { echo "kc $1" ; }
function fontSize()        { echo "kf $1" ; }
function lineJoin()        { echo "kj $1" ; }
function miterLimit()      { echo "kl $1" ; }
function compositingMode() { echo "km $1" ; }
function textAlign()       { echo "kt $1" ; }
function lineWidth()       { echo "kw $1" ; }
function relLineTo()       { echo "l $1 $2" ; }
function relMoveTo()       { echo "m $1 $2" ; }
function font()            { echo "n \"$1\"" ; }
function radialGradient()  { echo "o $1 $2 $3 $4 $5 $6"; }
function gradientAddStop() { echo "p $1 $2 $3 $4 $5"; }
function relQuadTo()       { echo "q $1 $2 $3 $4"; }
function rectangle()       { echo "r $1 $2 $3 $4"; }
function relSmoothTo()     { echo "s $1 $2 $3 $4"; }
function relSmoothQuadTo() { echo "t $1 $2"; }
function strokeText()      { echo "u \"$1\""; }
function relVerLine()      { echo "v $1"; }
function glyph()           { echo "w $1"; }
function text()            { echo "x \"$1\""; }
function identity()        { echo "y "; };
function closePath()       { echo "z "; };
function arcTo()           { echo "A $1 $2 $3 $4 $5 $6"; }
function arc()             { echo "B $1 $2 $3 $4 $5 $6"; }
function curveTo()         { echo "C $1 $2 $3 $4 $5 $6"; }
function stroke()          { echo "E "; }
function fill()            { echo "F "; }
function restore()         { echo "G "; }
function horLineTo()       { echo "H $1"; }
function rotate()          { echo "J $1"; }
function lineTo()          { echo "L $1 $2"; }
function moveTo()          { echo "M $1 $2"; }
function beginPath()       { echo "N"; }
function scale()           { echo "O $1 $2"; }
function quadTo()          { echo "Q $1 $2 $3 $4"; }
function smoothTo()        { echo "S $1 $2 $3 $4"; }
function smoothQuadTo()    { echo "T $1 $2"; }
function reset()           { echo "U"; }
function verLineTo()       { echo "V $1"; }
function ctxDone()         { echo "X"; }
function roundRectangle()  { echo "Y $1 $2 $3 $4 $5"; }
function closePath()       { echo "Z"; }
function transform()       { echo "W $1 $2 $3 $4 $5 $6"; }
function startGroup()      { echo "{ "; }
function endGroup()        { echo "} "; }

function startFrame()      { echo -e "\e[H\e[?200h reset " ; }
function endFrame()        { echo -n "flush X "; }

function update_event()
{
    IFS=$'\a' read -d '' -r -s -t 0.1 -n 1  key_input
    if [ x"" != x"$key_input" ];then
       seq="$key_input"

       if [ x$seq = x`echo -en "\e"` ] ; then
          IFS= read -s -t 0.2 -n 1 key_input1
	  if [ x$key_input1 = x'[' ];then
            IFS= read -s -n 1 key_input2  # at least one more char
	    key_input_rest=''
            case $key_input2 in
		    "A") ;; "B") ;; "C") ;; "D") ;;
		    "3"|"5"|"6") 
                        IFS= read -s -n 1 key_input_rest 
			;;
		    *)
               IFS= read -t 0.2 -s -n 3 key_input_rest  # grab the next 4 if there
			    ;;
	    esac
            seq="ESC"$key_input1$key_input2$key_input_rest
	  else
            seq="ESC$key_input1"
	  fi
       fi
       case "$seq" in
	 "ESC")    key="esc"   ;;
         "ESC[A")  key="up"    ;;
	 "ESC[B")  key="down"  ;;
         "ESC[C")  key="right" ;;
         "ESC[D")  key="left"  ;;
         "ESC[H")  key="home" ;;
         "ESC[F")  key="end" ;;
         "ESC[Z")  key="shift-tab" ;;
	  ' ')     key="space"       ;;
         $'\177'|$'\b')  key="backspace" ;;
         "ESC"$'\177')   key="alt-backspace" ;;
	 $'\n')    key=return ;;
	 $'\t')    key=tab  ;;
         $'\001')  key="control-a" ;;
         $'\002')  key="control-b" ;;
         $'\003')  key="control-c" ;;
         $'\004')  key="control-d" ;;
         $'\005')  key="control-e" ;;
         $'\006')  key="control-f" ;;
         $'\007')  key="control-g" ;;
         $'\010')  key="control-h" ;;
         $'\011')  key="control-i" ;;
         $'\012')  key="control-j" ;;
         $'\013')  key="control-k" ;;
         $'\014')  key="control-l" ;;
         $'\015')  key="control-m" ;;
         $'\016')  key="control-n" ;;
         $'\017')  key="control-o" ;;
         $'\020')  key="control-p" ;;
         $'\021')  key="" ;; #control-q" ;;
         $'\022')  key="control-r" ;;
         $'\023')  key="" ;; #control-s" ;;
         $'\024')  key="control-t" ;;
         $'\025')  key="control-u" ;;
         $'\026')  key="control-v" ;;
         $'\027')  key="control-w" ;;
         $'\030')  key="control-x" ;;
         $'\031')  key="control-y" ;;
         $'\032')  key="control-z" ;;
         "ESC[3~") key=delete;;
         "ESC[5~") key="page-up"   ;;
         "ESC[6~") key="page-down" ;;
	  *)       key="$seq"
	  ;;
       esac
       event=$key
    else
       event=""
    fi
}

function next_event(){
  event=''
  while [ x$event == x ];do
    update_event
  done
}

if [ `basename $0` == ctx.bash ];then
   ctx_init
   startFrame
   rgba 1 1 1 1
   fontSize 10%
   moveTo 0 12%
   text "hello"
   endFrame
   sleep 2
   startFrame
   rgba 1 1 1 1
   fontSize 10%
   moveTo 0 12%
   text "this is ctx"
   endFrame
   sleep 3
fi
