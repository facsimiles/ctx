#!/bin/bash

echo "Recognized escape sequences, see DEC VT terminal manuals online, as well as "
echo "https://bjh21.me.uk/all-escapes/all-escapes.txt for a reference of these and more"
echo ""
echo "This text file is generated from the sources."

while IFS= read -r line; do
  id=`echo -n $line | grep -Eow "id:[A-Z]*" | sed s/id://`
  description=`echo -n $line | sed s/.*id:[A-Z]*// | sed 's:\*/.*$::'`
  prefix=`echo -n $line | cut -f 1 -d ' ' | sed 's/^ *{"//'  | sed s/\",//`

  suffix=`echo -n $line | cut -f 2 -d ' '`
  if [ $suffix = "0," ]; then
    suffix=''
  else
    suffix=" args "`echo -n $suffix | sed "s/^ *'//" | sed "s/',//"`
  fi

    echo
    echo "$id     \e$prefix$suffix"
    echo "   $description"
done <<< `cat ctx-vt.c | grep  'id:' `
