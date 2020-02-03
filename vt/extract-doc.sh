#!/bin/bash

echo "Recognized escape sequences, see DEC VT terminal manuals online, as well as "
echo "https://bjh21.me.uk/all-escapes/all-escapes.txt for a reference of these and more"
echo ""
echo "This text file is generated from the sources."

while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "id:[A-Z]*" | sed s/id://`
  args=`echo -n $line | grep -Eow "args:[A-Za-z0-9;.]*" | sed s/args://`" "
  description=`echo -n $line | sed s/.*id:[A-Z]*// | sed 's:\*/.*$::'`
  prefix=`echo -n $line | cut -f 1 -d ' ' | sed 's/^ *{"//'  | sed s/\",//`

  suffix=`echo -n $line | cut -f 2 -d ' '`
  if [ $suffix = "0," ]; then
    suffix=''
  else
    suffix=" $args"`echo -n $suffix | sed "s/^ *'//" | sed "s/',//"`
  fi

  line="$id           "
  line="${line:0:7}"
  line="$line$description                                           "
  line=${line:0:45}"ESC $prefix$suffix"
  echo "$line"
done <<< `cat ctx-vt.c | grep  'id:' `

echo -en "\n\nModes:\n~~~~~~\n\n"


while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  name=`echo "$line" | cut -f 2 -d ';'`
  on_label=`echo "$line" | cut -f 3 -d ';'`
  off_label=`echo "$line" | cut -f 4 -d ';'`
  description=`echo "$line" | cut -f 5 -d ';'`

  line="$name                                   "
  line=${line:0:17}
  line="$line$on_label                          "
  line=${line:0:29}
  line="$line""ESC [?$id""h                      "
  line=${line:0:45}
  line="$line$off_label                         "
  line=${line:0:56}
  line="$line""ESC [?$id""l                      "
  line=${line:0:75}
  
  echo "$line"
done <<< `cat ctx-vt.c | grep  'MODE;' `


while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "case [0-9]*" | sed "s/case //"`
  name=`echo "$line" | cut -f 2 -d ';'`
  on_label=`echo "$line" | cut -f 3 -d ';'`
  off_label=`echo "$line" | cut -f 4 -d ';'`
  description=`echo "$line" | cut -f 5 -d ';'`

  line="$name                                   "
  line=${line:0:17}
  line="$line$on_label                          "
  line=${line:0:29}
  line="$line""ESC [$id""h                      "
  line=${line:0:45}
  line="$line$off_label                         "
  line=${line:0:56}
  line="$line""ESC [$id""l                      "
  line=${line:0:75}
  
  echo "$line"
done <<< `cat ctx-vt.c | grep  'MODE2;' `
