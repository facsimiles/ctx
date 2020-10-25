#!/bin/bash

# A basic audioplayer for atty/ctxterm 
# that uses the passed in arguments for a playlist
#

playlist=("$@")
sample_rate=8000
pos=0
length_seconds=0
paused=0
song_no=0

player_dirty=0
playlist_dirty=1

# configure atty for base64 encoding, samplerate of 8000, 8bits, 1 channel, type ulaw, 1024 samples buffersize
function atty_init(){
echo -ne "\e_Ae=b,s=$sample_rate,b=8,c=1,T=u,B=1024;\e"'\\'
}


function vt_init_rows_cols(){
# try to determine the terminal size, by moving the cursor 99 times
# to the right and down, and reading back the cursor position
echo -en "\e[299C\e[299B\e[6n"
read -s -n 2  # skip two bytes, blocking
dims=''
read -s -n 1 ch
while [ "$ch" != "R" ]; do
  dims="$dims$ch"
  read -s -n 1 ch
done
ROWS=`echo $dims|sed 's/;.*//'`
COLS=`echo $dims|sed 's/.*;//'|sed 's/R//'`
}


function prepare_path(){
  ffmpeg -i "$1" -ac 1 -ar $sample_rate -acodec pcm_mulaw -f au - > /tmp/audio.au 2>/tmp/audio.ff
  base64 /tmp/audio.au -w0  > /tmp/audio.b64
  title=$(cat /tmp/audio.ff | grep title | cut -f 2 -d ':'|head -n 1)
  artist=$(cat /tmp/audio.ff | grep artist | cut -f 2 -d ':'|head -n 1)

  length_base64=$(wc -c /tmp/audio.b64|cut -f 1 -d ' ')
  length_seconds=$(($length_base64*2/3/$sample_rate))
}

function mm_ss(){
   echo -n "$(($1/60)):"
   if [ $(($1%60)) -lt 10 ];then
      echo -n "0";
   fi
   echo -n "$(($1%60))"
}

echo -ne "\e[2J"
function draw_playlist()
{
  if [ x$playlist_dirty == x0 ];then return;fi

  echo -ne "\e[4;1H"
  no=0
  for i in "${playlist[@]}";do
    echo -en "\e[K"
    if [ $no == $song_no ];then echo -en '[';else echo -en ' ';fi
    echo -en "$i";
    if [ $no == $song_no ];then echo -e ']';else echo -e ' ';fi
    if [ $no -gt $(($ROWS-8)) ]; then return; fi
    no=$((no+1))
  done
  playlist_dirty=0
}

function draw_player()
{
  echo -ne "\e[1;1H\e[K"
  if [ x$preparing == x1 ]; then echo -n "[PREPARING $prepared_path]";return;fi
  if [ $paused == 1 ]; then echo -n "[PAUSED]";fi
  echo -en "("
  mm_ss $pos
  echo -n "/"
  mm_ss $length_seconds
  echo -n ") $artist   $title"
  echo -en "\r"
}

function update_ui()
{
  draw_playlist
  draw_player
}

function set_song_no()
{
  song_no=$1
  pos=0
  playlist_dirty=1
}

function update_audio()
{
    if [ x"$prepared_path" != x"${playlist[$song_no]}" ];then
      pos=0
      prepared_path="${playlist[$song_no]}"
      preparing=1
      draw_player
      prepare_path $prepared_path
      preparing=0
    fi

    echo -ne "\e_Af=$((sample_rate));"
    cut -b $(((pos*sample_rate*3)/2+1))-$((((pos+1)*sample_rate*3)/2)) /tmp/audio.b64
    echo -ne '\e\\'
    pos=$(($pos+1))

    for i in {1..4};do
      sleep 0.10s;
      keyboard_events
      update_ui
    done

    seconds=$(date +%S)
    while [ "x$seconds" == "x$prev_seconds" ];do
      seconds=$(date +%S)
    done
    prev_seconds=$seconds
}

function keyboard_events()
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
             IFS= read -t 0.2 -s -n 3 key_input_rest  # grab the next 3 if there
                          ;;
          esac
          seq="ESC"$key_input1$key_input2$key_input_rest
        else
          seq="ESC$key_input1"
        fi
     fi
     case "$seq" in
       "ESC"|"q") echo -e "\e[2J\e[H"; quit=1 ;;
       "ESC[A"|"ESC[5~") set_song_no $((song_no-2));pos=$((pos+100000)) ;;
       "ESC[B"|"n"|"ESC[6~") pos=$((pos+100000)) ;;
       "ESC[C")  pos=$(($pos+10)) ;;
       "ESC[D")  pos=$(($pos-10)) ;if [ $pos -lt 0 ] ; then pos=0; fi ;;
        ' ')  if [ $paused == 0 ];then paused=1;else paused=0;fi  ;;
        *)    ;;
      esac
  fi
}


function main()
{
  atty_init
  vt_init_rows_cols

  while [ x$quit == x ];do 
    seconds=$(date +%S)
    prev_seconds=$seconds
    while [ $pos -lt $(($length_seconds+1)) ];do
       update_ui
       if [ $paused == 0 ]; then
          update_audio
       fi
       keyboard_events
     done
     set_song_no $((song_no+1))
  done
  rm -f /tmp/audio.*
}

main

