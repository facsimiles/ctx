#!/bin/bash

# This shell script uses sox to convert to u-law 8bit 8khz
# and pv for rate limiting to 8000bytes/second
#
#  todo
#    make audio initialization.. and other prepwork async
#

shopt -s expand_aliases
shopt -s extglob
#shopt -s hist_append
shopt -s hostcomplete
shopt -u checkwinsize

alias ls='ls --color=auto'

playlist_length=0
item_no=1
alive=1
focus=1   # 1 is list 2 is item

current_path=''
chunk_seconds=1
buffer_seconds=3
paused=1
loop_single=0
#row=$((12 - $item_no ))
scroll=0
list_top=2
list_bottom=20
viewer='none'
had_command=0
entry_pos=0
ui_dirty=1

# define some terminal escape sequences we'll use

function vt_move_to(){    echo -en "\e[$1;$2H"; }
function vt_home(){       echo -en "\e[H"; }
function vt_wrap(){       echo -en "\e[?7h"; }
function vt_nowrap(){     echo -en "\e[?7l"; }
function vt_clear(){      echo -en "\e[2J"; vt_home; }
function vt_clear_eol(){  echo -en "\e[K"; }
function vt_move_right(){ echo -en "\e[$1C"; }
function vt_move_left(){  echo -en "\e[$1D"; } # or  just \b for one

# style

function vt_style_reset(){  echo -en "\e[m"; }
function vt_bold(){         echo -en "\e[1m"; }
function vt_unbold(){       echo -en "\e[22m"; } # actually reset style
function vt_inverse(){      echo -en "\e[7m";  }
function vt_positive(){     echo -en "\e[27m"; }
function vt_underline(){    echo -en "\e[4m";  }
function vt_nounderline(){  echo -en "\e[24m"; }
function vt_blink(){        echo -en "\e[5m";  }
function vt_steady(){       echo -en "\e[25m"; }
function vt_fg(){           echo -en "\e[38;5;$1m"; }
function vt_bg(){           echo -en "\e[48;5;$1m"; }

commandline=""

function draw_prompt()
{
  vt_move_to $ROWS 1
  vt_clear_eol
  echo -n "\$ $entry"
  vt_move_to $ROWS $(($entry_pos+3))
}

function draw_list()
{
  playlist_length=0
  scroll_items=$(($list_bottom - $list_top))
  vt_clear
  vt_move_to $((2)) 1
  echo -n $file_mimetype
  vt_move_to 1 1
  vt_nowrap
  echo -n "$file_info"
  vt_wrap

  no=1

  if [ $item_no -gt $(( $scroll_items - $scroll  )) ]; then
      scroll=$(( $scroll_items - $item_no ))
  fi

  if [ $item_no -lt $(( $list_top + 2 - $scroll )) ]; then
      scroll=$(( $list_top + 2 - $item_no))
  fi
  row=$scroll

  while IFS= read -r a; do
    playlist_length=$(($playlist_length+1))
    if [ $row -gt $(($list_top - 1)) ];then
      if [ $row -lt $(($list_bottom + 1)) ];then
	vt_move_to $row 2

	if [ x$item_no = x$no ];then
	  vt_inverse
	fi
        #echo -n `basename "$a"`
	echo ${a:0:20} 
	if [ x$item_no = x$no ];then
	  vt_positive
	fi
      fi
    fi
    row=$(($row+1))
    no=$(($no+1))
  done <<< "$playlist"

}
paused=0

file_mimetype=''
file_mimeclass=''
file_info='[-]'
temp_folder="/tmp/.au_frags"

function prepare_audio(){
  vt_move_to 22 2
  mkdir "$temp_folder" 2> /dev/null
  rm "$temp_folder/"* 2> /dev/null
  #sox "$1" -t raw -r 8000 -c 1 -e u-law -b 8 - 2>/dev/null |  tr "\000" "\001" > "$temp_folder/audio" || paused=1

  ffmpeg -i "$1" -ac 1 -ar 8000 -acodec pcm_mulaw -f au - 2>/dev/null | tr "\000" "\001" > "$temp_folder/audio" ||  paused=1

  #split "$temp_folder/audio" "$temp_folder/frag" -b $((8000 * $chunk_seconds)) -d
  # rm "$temp_folder/audio" 2> /dev/null

  duration=$((`wc -c $temp_folder/audio | cut -f 1 -d ' '` / 8000))
  viewer=audio
}

function prepare_text(){
  text_scroll=1
  text_col=1
  if [ -x /usr/share/source-highlight/src-hilite-lesspipe.sh ];then
    text_contents=`/usr/share/source-highlight/src-hilite-lesspipe.sh "$1"`
  else
    text_contents=`cat "$1"`
  fi
  text_lines=`echo "$text_contents" | wc -l | cut -f 1 -d ' '`
  text_viewer_start=2
  text_viewer_rows=$(($ROWS-$text_viewer_start - 1))
  viewer=text
}

function prepare_directory(){
  ls -sh1 "$1" > $temp_folder/text
  prepare_text $temp_folder/text
}

function prepare_html(){
  w3m -cols $(($COLS-20)) -dump "$1" > $temp_folder/text
  prepare_text $temp_folder/text
}

function prepare_file(){
	pos=0
	duration=5
	loaded_path="$current_path/$1"
	loaded_basename=$(basename -- "$loaded_path")

	if [ -x "`which mimetype`" ] ; then
	  # external tool is preferable
	  file_mimetype=`mimetype "$loaded_path" | sed 's/.*: *//'`
	else
	  if [ -d "$loaded_path" ]; then
	    file_mimetype='inode/directory'
	  elif [ -L "$loaded_path" ]; then
	    file_mimetype='inode/symbolic-link'
	  else
	    loaded_extension="${loaded_basename##*.}"
	    case "$loaded_extension" in
	            "json") file_mimetype='application/json' ;;
	            "html") file_mimetype='text/html' ;;
	            "jpg" ) file_mimetype='image/jpeg' ;;
	            "JPG" ) file_mimetype='image/jpeg' ;;
	            "gif" ) file_mimetype='image/gif' ;;
	            "png" ) file_mimetype='image/png' ;;
	            "txt")  file_mimetype='text/ascii' ;;
	            "inc")  file_mimetype='text/ascii' ;;
	            "ini")  file_mimetype='text/ascii' ;;
	            "c")    file_mimetype='text/x-csrc' ;;
	            "mp3")  file_mimetype='audio/mp3' ;;
	            "ogg")  file_mimetype='audio/ogg' ;;
	            "ogv")  file_mimetype='audio/ogv' ;;
	            "au")   file_mimetype='audio/sun-ulaw' ;;
	            "wav")  file_mimetype='audio/pcm' ;;
	            "h")    file_mimetype='text/x-chdr' ;;
	            "sh")   file_mimetype='text/sh' ;;
		    *)      file_mimetype='application/octetstream' ;;
	    esac
	  fi
	fi
	file_mimeclass=`echo -n $file_mimetype | sed 's:/.*::'`
	file_info=`file "$loaded_path" 2>/dev/null | sed 's/^.*: *//'`
	#file_info="$loaded_path"

	file_size=`stat --printf "%s" "$loaded_path"`

	viewer=none

	if [ $file_size -lt 85536 ]; then
	case $file_mimetype in
	  "inode/directory"|"inode/mount-point")
 	      prepare_directory "$current_path/$1" 
	      linenumbers=0
	      ;;
	  "text/html")
 	      prepare_html "$current_path/$1"
              linenumbers=0
	      ;;
	  "application/x-shellscript"|\
	  "application/json"|\
	  "application/x-yaml")
 	      prepare_text "$current_path/$1"
              inenumbers=1
	      ;;
          *)
	    case $file_mimeclass in
 	      "audio")  prepare_audio "$current_path/$1" ;;
 	      "text")   prepare_text "$current_path/$1"
		        linenumbers=1
			;;
	      *) ;;
          esac 
	  ;;
        esac
	fi

}

function draw_ui()
{
  if [ $ui_dirty != 0 ];then
    if [ $ui_dirty = 2 ]; then
      draw_content
    else
      draw_list
      draw_content
    fi
    if [ $focus != 2 ];then
      draw_prompt
    fi
    ui_dirty=0
  fi
}

function prepare_item(){
	     item=`echo -n "$playlist" | sed -n -e $item_no"p" `
	     prepare_file "$item"
}

function previous_page(){
	     item_no=$(($item_no - $scroll_items/2   ))
	     if [ $item_no -lt 1 ]; then item_no=1; fi
	     prepare_item
}

function previous_item(){
	     item_no=$(($item_no-1))
	     if [ $item_no -lt 1 ]; then item_no=1; fi
	     prepare_item
}

function next_item(){
	     item_no=$(($item_no+1))
	     if [ $item_no -gt $playlist_length ]; then item_no=1; fi
	     prepare_item
}

function next_page(){
	     item_no=$(($item_no + $scroll_items/2   ))
	     if [ $item_no -gt $playlist_length ]; then item_no=$playlist_length; fi
	     prepare_item
}

function first_item(){
	     item_no=1
	     prepare_item
}

function last_item(){
	     item_no=$playlist_length
	     prepare_item
}

audio_start=`date +%s`
audio_length=1

player_row=1


function print_mm_ss(){
  min="$(($1/60))"
  sec="$(($1 % 60))"
  echo -en "$min:"
  if [ $sec -le 9 ]; then
    echo -en "0"
  fi
  echo -en "$sec"
}

linenumbers=1

/* there is one texture..
size gets reported from renderer to
client - it could be implemented with
GeglBuffer behind the scenes.

*/

function draw_text ()
{
  vt_home
  lineno=0
  row=$(($text_viewer_start-$text_scroll))
  endrow=$(($text_viewer_start + $text_viewer_rows))

  vt_nowrap
  while IFS= read -r line; do
    if [ $row -ge $text_viewer_start ] && [ $row -le $endrow ]; then
      vt_move_to $row 16
      vt_clear_eol
      if [ $lineno -le $text_lines ]; then
        if [ $linenumbers = 1 ]; then
	  vt_fg 8
	  printf "%04d " $lineno
  	  vt_style_reset
	fi
        echo -n "$line" | cut -b $text_col-400
      fi
    fi
    row=$(($row+1))
    lineno=$(($lineno+1))
  done <<< $text_contents
  while [ $row -le $endrow ]; do
    vt_move_to $row 16
    vt_clear_eol
    row=$(($row+1))
  done
  vt_wrap
}

function draw_audio () {
  cursor="|"
  if [ $loop_single = 1 ];then
    cursor="@"
  fi
  if [ $paused = 1 ];then
    cursor="."
  fi
  echo -en "\e[$player_row;1H"
  vt_bold
  print_mm_ss $(($pos*$chunk_seconds))
  echo -en "/"
  print_mm_ss $duration
  echo -en "\e[K\e[$(($player_row));80H "
  if [ $duration -gt 0 ]; then :
    vt_move_to $player_row $(( $pos * $COLS / $duration))
    echo -n $cursor
  fi
  vt_unbold
  if [ $paused = 1 ]; then
    vt_inverse
    echo -en "\b\bPAUSED - space to play"
    vt_positive
  fi
}

function handle_key()
{
  key=$1
  if [ x"$1" = x'control-l' ];then
    ui_dirty=1 ; had_command=0; return
  fi

  if [ $focus = 1 ]; then
  if [ "x$entry" = x ] && [ $had_command = 0 ];then
  case "$1" in
    "esc")  alive=0 ;;
    "home") first_item  ;;
    "end") last_item  ;;
    "left")
	      base=`basename "$current_path"`
              current_path=`dirname "$current_path"|sed s:/$::`
              playlist=`ls -1 "$current_path"`

	      first_item

              found=-1
	      no=1
              while IFS= read -r a; do
		  if [ "$a" == "$base" ];then
		     item_no=$no
	             prepare_item
		  fi
		  no=$(($no+1))
              done <<< "$playlist"

	    ;;
    "tab") 
	  case "$viewer" in
               "audio"|"text")
	          focus=2 ;draw_content; return 0
	          ;;
          esac ;;
    "right") 
          case "$file_mimetype" in
	  "inode/directory"|"inode/mount-point")
                  loaded_path=`echo "$loaded_path"|sed s:/$::`
                  playlist=`ls -1 "$loaded_path"`
		  current_path=$loaded_path
		  first_item
		  ;;
	  *) case $viewer in
               "audio"|"text")
	          focus=2 ;draw_content; return 0
	          ;;
             esac ;;
          esac
	  ;;
    "return") 
          case "$file_mimetype" in
	  "inode/directory"|"inode/mount-point")
                  loaded_path=`echo "$loaded_path"|sed s:/$::`
                  playlist=`ls -1 "$loaded_path"`
		  current_path=$loaded_path
		  first_item
		  ;;
	  *) case $viewer in
               "audio")
	          focus=2 ;ui_dirty=1; return 0
	          ;;
               "text")
	          ui_dirty=1; vim $loaded_path;  return 0
	          ;;
             esac ;;
          esac
	  ;;
    "page-up") previous_page  ui_dirty=1 ;;
    "page-down") next_page  ui_dirty=1 ;;
    "up")  previous_item ; ui_dirty=1 ;;
    "down")  next_item ;ui_dirty=1 ;;
    "control-d") alive=0 ; ;;
    "backspace")  ;;
    "space")  ;;
    *)
       entry=$key
       entry_pos=1
       draw_prompt
    ;;
  esac
  if [ $had_command = 0 ]; then ui_dirty=1 ; fi
  else # got text in entry
    case "$1" in
        "left") entry_pos=$(($entry_pos-1)) ;
		if [ $entry_pos -le 0 ]; then entry_pos=0; fi
		draw_prompt ;;
        "control-d")
		if [ "x$entry" = "x" ]; then alive=0 ; fi
		;;
        "right") entry_pos=$(($entry_pos+1)) ;
		if [ $entry_pos -ge ${#entry} ]; then entry_pos=${#entry}; fi
		draw_prompt ;;
	"tab") 
		last="${entry##* }"
		completion=''
		hits=0

                while IFS= read -r a; do
		  if [[ $a == $last* ]];then
			  hits=$(($hits+1))
			  completion="${a:${#last}:200} "
			  ui_dirty=1
		  fi
                done <<< "$playlist"
		if [ $hits != 1 ]; then
		   # XXX find biggest common substring
		  completion=''
		fi

		entry="$entry$completion"
		entry_pos=${#entry}
		;;
        "up")  ;;
        "down")  ;;
        "return") 
	  if [ "x$entry" = x ]; then
		ui_dirty=1
		had_command=0
	  else
	      echo ""
	      oldpwd=`pwd`
              eval $entry
	      if [ "$oldpwd" != `pwd` ]; then
                 if [ $had_command = 1 ]; then
		   current_path=`pwd`
                   playlist=`ls -1 "$current_path"`
		 else
		   current_path=`pwd`
                   playlist=`ls -1 "$current_path"`
		   first_item
		   had_command=0
		   ui_dirty=1
		 fi
	      else
                had_command=1
	      fi
              entry=''
	      entry_pos=0
	      draw_prompt
	  fi
           ;;
	 "home"|"control-a") entry_pos=0 ; draw_prompt ;;
	 "end"|"control-e") 
		entry_pos=${#entry}
	        draw_prompt
		;;
         "delete")
	      if [ $entry_pos -lt ${#entry} ]; then
	        entry="${entry:0:$entry_pos}${entry:$entry_pos+1:200}"

	        draw_prompt
	      fi
	    ;;
         "backspace")
	      if [ $entry_pos -gt 0 ]; then
	        entry="${entry:0:$entry_pos-1}${entry:$entry_pos:200}"
	        entry_pos=$(($entry_pos-1))

	        draw_prompt
	      fi
	    ;;
	'space') 
	  entry="${entry:0:$entry_pos} ${entry:$entry_pos:200}"
	  entry_pos=$(($entry_pos+1))
          draw_prompt ;;
	*)
	  entry="${entry:0:$entry_pos}$key${entry:$entry_pos:200}"
	  entry_pos=$(($entry_pos+1))
          draw_prompt
	;;
    esac
  fi
  else
    ui_dirty=2
    case "$1" in
      "tab")   focus=1; ui_dirty=1 ; return ;;
      "esc")  alive=0 ;; 
    esac

    case "$viewer" in
      "audio")
        case "$1" in
          "right")  pos=$((pos + 10 / $chunk_seconds ))  ;;
          "left")  pos=$((pos - 10 / $chunk_seconds ))  ;;
          "up")  pos=$((pos - 60 / $chunk_seconds ))  ;;
          "down")  pos=$((pos + 60 / $chunk_seconds ))  ;;
          'space') if [ $paused = 1 ]; then paused=0; else paused=1; fi ;;
          "l") if [ x$loop_single = x1 ]; then loop_single=0; else loop_single=1; fi ;;
        esac
	;;
      "text")
        case "$1" in
          "home") text_scroll=1 ;; 
          "down") text_scroll=$(($text_scroll+1)) ;; 
          "up")   text_scroll=$(($text_scroll-1)) ;; 
          "right") text_col=$(($text_col+8)) ;; 
          "left")  if [ $text_col = 1 ]; then
		     focus=1;ui_dirty=1;
	           else
		     text_col=$(($text_col-8))
	           fi  ;;
          "page-down") text_scroll=$(($text_scroll+$text_viewer_rows)) ;; 
          "page-up")   text_scroll=$(($text_scroll-$text_viewer_rows)) ;; 
        esac
    esac
    if [ $text_scroll -lt 1 ]; then text_scroll=1 ; fi
    if [ $text_col -lt 1 ]; then text_col=1 ; fi

    ui_dirty=2
  fi
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
         $'\021')  key="control-q" ;;
         $'\022')  key="control-r" ;;
         $'\023')  key="control-s" ;;
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
       handle_key $key
    fi
}

function draw_content(){
  case $viewer in
    "audio") draw_audio ;;
    "text")  draw_text ;;
    "*") ;;
  esac
}

function loop(){

  if [ $paused = 0 ] && [  x$file_mimeclass = x'audio' ]; then
     if [ -f $temp_folder/audio ];then
      echo -en "\e[?4444h" # enter audio mode
      dd status=none if=$temp_folder/audio skip=$pos bs=8000 count=1
      echo -en "\000" # end of audio mode marker
      pos=$(($pos+1))
      ui_dirty=2
    fi
  fi

  audio_length=$(( $audio_length + 1))

  while [ $alive = 1 ] && [ $(($audio_start + $audio_length + $buffer_seconds)) -gt `date +%s` ]; do

    draw_ui
    keyboard_events
    #echo -en "\b"
  done

  if [ $pos -gt $duration ]; then
    if [ $loop_single = 1 ]; then
      pos=0
      ui_dirty=2
    else
      next_item
      ui_dirty=1
    fi
    draw_ui
  fi
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

function send_ctrl_c(){
  handle_key "control-c"
}

function setup(){
  trap send_ctrl_c SIGINT
  trap true  SIGTERM

  if [ -n ""$1 ] && [ -d $1 ]; then
    current_path=`echo -n "$1"|sed "s:/^::"`
  else
    current_path=`pwd`
  fi
  cd "$current_path"
  playlist=`ls -1 $current_path 2>/dev/null`
  vt_init_rows_cols
  list_bottom=$(($ROWS-4))

  vt_clear
  first_item
}


script_path=`realpath $0`
start_hash=`sha256sum $script_path`

function save_state()
{
  state="$item_no;$current_path;$focus;$entry_pos;$scroll_items;$text_scroll;$text_col;$scroll;$entry"
  # entry fails...
}

function load_state()
{
  state=$1
  item_no=`echo "$state" | cut -d ';' -f 1`
  if [ $item_no -lt 1 ]; then item_no=1; fi
  current_path=`echo "$state" | cut -d ';' -f 2`
  playlist=`ls -1 $current_path 2>/dev/null`
  prepare_item
  focus=`echo "$state" | cut -d ';' -f 3`
  entry_pos=`echo "$state" | cut -d ';' -f 4`
  scroll_items=`echo "$state" | cut -d ';' -f 5`
  text_scroll=`echo "$state" | cut -d ';' -f 6`
  text_col=`echo "$state" | cut -d ';' -f 7`
  scroll=`echo "$state" | cut -d ';' -f 8`
  entry=`echo "$state" | cut -d ';' -f 9`
}

setup  $*

if [ x$1 = x-s ];then
   state="$2"
   load_state "$state"
   shift; shift
fi

version_countdown=5

function main(){
  while [ $alive = 1 ]
  do
    version_countdown=$(($countdown-1))

    if [ $version_countdown -lt 1 ];then
    new_hash=`sha256sum $script_path`
    if [ "x$new_hash" != "x$start_hash" ]; then
       save_state
       exec $script_path -s "$state"
    fi
      version_countdown=10
    fi
    loop
  done
  vt_clear
}

main $*
