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


START="<tt>"
END="</tt>"
echo "<html><head>"
echo "<style>"
echo "          @import url(../ctx.css);"
echo " td {vertical-align:top;} tr:nth-child(odd){ background:#ddd; } "
echo " tt { border:0; background:none; }}"
echo "</style>"
echo "</head><body><div id='page'>"
echo "<table>"

echo "<tr><td><tt>\a</tt><td>BEL</td><td>warning bell</td></tr>"
echo "<tr><td><tt>\b</tt><td>BS</td><td>back space</td></tr>"
echo "<tr><td><tt>\t</tt><td>HT</td><td>horizontal tab</td></tr>"
echo "<tr><td><tt>\n</tt><td>LF</td><td>line feed</td></tr>"
echo "<tr><td><tt>\r</tt><td>CR</td><td>carriage return</td></tr>"

while IFS= read -r line; do
  id=`echo -n "$line" | grep -Eow 'id:[A-Z]*' | sed s/id://`
  ref=`echo -n "$line" | grep -Eow 'ref:[a-z]*' | sed s/ref://`
  args=`echo -n "$line" | grep -Eow 'args:[$A-Za-z0-9;.]*' | sed s/args://`
  description=`echo -n "$line" | sed -e "s/.*id:[A-Z]*//" -e 's:*/.*$::'`
  prefix=`echo -n "$line" | sed 's/^ *{"//'  | sed s/\",.*//`

  suffix=`echo -n $line | cut -f 2 -d ' '`
  if [ $suffix = "0," ]; then
    suffix=''
  else
    suffix="$args"`echo -n $suffix | sed "s/^ *'//" | sed "s/',//"`
  fi

  if [ x$ref = xnone ];then
  echo "<tr><td style='width:9em'><tt>\e$prefix$suffix</tt></td><td>$id</td> <td>$description</td> </tr>"
  else
  echo "<tr><td style='width:9em'>$ref<tt>\e$prefix$suffix</tt></td><td><a href='https://vt100.net/docs/vt510-rm/$id.html'>$id</a></td> <td>$description</td> </tr>"
  fi
done <<< `cat ../src/vt.c | grep  'id:' `

#echo -en "\n\nModes:\n~~~~~~\n\n"


while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  mnemonic=`echo "$line" | cut -f 2 -d ';'`
  name=`echo "$line" | cut -f 3 -d ';'`
  on_label=`echo "$line" | cut -f 4 -d ';'`
  off_label=`echo "$line" | cut -f 5 -d ';'`
  description=`echo "$line" | cut -f 6 -d ';'`

  name="$name                                   "
  name=${name:0:18}

  on_esc="\e[?$id""h                      "
  if [ x"$on_label" = x"" ]; then
    on_esc="                            "
  fi
  on_label="$on_label                          "
  on_label=${on_label:0:12}
  on_esc=${on_esc:0:14}

  off_esc="\e[?$id""l                      "
  if [ x"$off_label" = x"" ]; then
    off_esc="                      "
  fi
  off_label="  $off_label                          "
  off_label=${off_label:0:13}
  off_esc=${off_esc:0:11}

  echo "<tr><td><tt>$on_esc</tt></td><td>$mnemonic</td><td>$name $on_label</td></tr>"
  echo "<tr><td><tt>$off_esc</tt></td><td>$mnemonic</td><td>$name$off_label</td></tr>"
done <<< `cat ../src/vt.c | grep  'MODE;' `


while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  mnemonic=`echo "$line" | cut -f 2 -d ';'`
  name=`echo "$line" | cut -f 3 -d ';'`
  on_label=`echo "$line" | cut -f 4 -d ';'`
  off_label=`echo "$line" | cut -f 5 -d ';'`
  description=`echo "$line" | cut -f 6 -d ';'`

  name="$name                                   "

  on_esc="\e[$id""h                      "
  if [ x"$on_label" = x"" ]; then
    on_esc=""
  fi
  on_label="$on_label                          "

  off_esc="\e[$id""l                      "
  if [ x"$off_label" = x"" ]; then
    off_esc=""
  fi
  off_label="  $off_label                          "


  echo "<tr><td><tt>$on_esc<tt></td><td>$mnemonic</td><td>$name$on_label</td></tr>"
  echo "<tr><td><tt>$off_esc</tt></td><td>$mnemonic</td><td>$name$off_label</td></tr>"
  
done <<< `cat ../src/vt.c | grep  'MODE2;' `


while IFS= read -r line; do
  id="`echo \"$line\" | cut -f 2 -d '@'`"
  if [ x$id = x ] ;then
    id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  fi
  label=`echo "$line" | cut -f 3 -d '@'`
  description=`echo "$line" | cut -f 4 -d '@'`
  pad_id="                  $id"
  pad_id=`echo "$pad_id" | tail -c 16`
  echo "<tr><td><tt>\e[$id""m</tt></td><td></td><td>$label<br/>$description</td></tr>"

done <<< `cat ../src/vt.c | grep  'SGR@' `

echo "</table></div></body></html>"

