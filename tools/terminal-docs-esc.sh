#!/bin/bash


function vt_move_to(){            echo -en "\e[$1;$2H"; }
function vt_home(){               echo -en "\e[H"; }
function vt_wrap(){               echo -en "\e[?7h"; }
function vt_nowrap(){             echo -en "\e[?7l"; }
function vt_clear(){              echo -en "\e[2J"; vt_home; }
function vt_clear_eol(){          echo -en "\e[K"; }
function vt_move_right(){         echo -en "\e[$1C"; }
function vt_move_left(){          echo -en "\e[$1D"; } # or  just \b for one
function vt_margin_tb(){          echo -en "\e[$1;$1r"; } 
function vt_margin_lr(){          echo -en "\e[$1;$1s"; } 
function vt_margin_lr_enable(){   echo -en "\e[?59h"; } 
function vt_margin_lr_disable(){  echo -en "\e[?59h"; } 
function vt_line_size_2_2_top(){  echo -en "\e#3"; }
function vt_line_size_2_2_bottom(){  echo -en "\e#4"; }
function vt_line_size_1_1(){      echo -en "\e#5"; }
function vt_line_size_2_1(){      echo -en "\e#6"; }


START="\e[31m"
END="\e[m"

echo "Recognized escape sequences, see DEC VT terminal manuals online, as well as "
echo "https://bjh21.me.uk/all-escapes/all-escapes.txt for a reference of these and more"
echo ""
echo "This text file is generated from the sources."

while IFS= read -r line; do
  id=`echo -n "$line" | grep -Eow 'id:[A-Z]*' | sed s/id://`
  args=`echo -n "$line" | grep -Eow 'args:[A-Za-z0-9;.]*' | sed s/args://`" "
  description=`echo -n "$line" | sed -e "s/.*id:[A-Z]*//" -e 's:*/.*$::'`
  prefix=`echo -n "$line" | sed 's/^ *{"//'  | sed s/\",.*//`

  suffix=`echo -n $line | cut -f 2 -d ' '`
  if [ $suffix = "0," ]; then
    suffix=''
  else
    suffix=" $args"`echo -n $suffix | sed "s/^ *'//" | sed "s/',//"`
  fi

  line="$id           "
  line="${line:0:7}"
  line="$line$description                                        "
  line=${line:0:45}$START"ESC $prefix$suffix"$END
  echo -e "$line"
done <<< `cat ../src/vt.c | grep  'id:' `

echo -en "\n\nModes:\n~~~~~~\n\n"


while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  name=`echo "$line" | cut -f 2 -d ';'`
  on_label=`echo "$line" | cut -f 3 -d ';'`
  off_label=`echo "$line" | cut -f 4 -d ';'`
  description=`echo "$line" | cut -f 5 -d ';'`

  name="$name                                   "
  name=${name:0:18}

  on_esc="ESC [?$id""h                      "
  if [ x"$on_label" = x"" ]; then
    on_esc="                            "
  fi
  on_label="$on_label                          "
  on_label=${on_label:0:12}
  on_esc=${on_esc:0:14}

  off_esc="ESC [?$id""l                      "
  if [ x"$off_label" = x"" ]; then
    off_esc="                      "
  fi
  off_label="  $off_label                          "
  off_label=${off_label:0:13}
  off_esc=${off_esc:0:11}

  line="$name$on_label$START$on_esc$END$off_label$START$off_esc$END"
  
  echo -e "$line"
done <<< `cat ../src/vt.c | grep  'MODE;' `


while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  name=`echo "$line" | cut -f 2 -d ';'`
  on_label=`echo "$line" | cut -f 3 -d ';'`
  off_label=`echo "$line" | cut -f 4 -d ';'`
  description=`echo "$line" | cut -f 5 -d ';'`

  name="$name                                   "
  name=${name:0:18}

  on_esc="ESC [$id""h                      "
  if [ x"$on_label" = x"" ]; then
    on_esc=""
  fi
  on_label="$on_label                          "
  on_label=${on_label:0:12}
  on_esc=${on_esc:0:14}

  off_esc="ESC [$id""l                      "
  if [ x"$off_label" = x"" ]; then
    off_esc=""
  fi
  off_label="  $off_label                          "
  off_label=${off_label:0:13}
  off_esc=${off_esc:0:10}

  line="$name$on_label$START$on_esc$END$off_label$START$off_esc$END"
  
  echo -e "$line"

done <<< `cat ../src/vt.c | grep  'MODE2;' `

echo
echo
echo -e "Set graphics Rendition, integers valid inside "$START"ESC ["$END" .. "$START"m"$END" sequences\n"
echo 

while IFS= read -r line; do
  id="`echo \"$line\" | cut -f 2 -d '@'`"
  if [ x$id = x ] ;then
    id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  fi
  label=`echo "$line" | cut -f 3 -d '@'`
  description=`echo "$line" | cut -f 4 -d '@'`
  pad_id="                  $id"
  pad_id=`echo "$pad_id" | tail -c 16`
  echo -e "$START$pad_id$END \e[$id""m$label\e[m\e[10m"

  if [ x"$description" != x"" ];then
	  :
    echo -e "                $description"
  fi

done <<< `cat ../src/vt.c | grep  'SGR@' `

vt_line_size_2_2_top
echo  "Hmmmm :)"
vt_line_size_2_2_bottom
echo  "Hmmmm :)"
vt_line_size_1_1


