#!/bin/bash

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
done <<< `cat mrg-vt.c | grep  'id:' `
