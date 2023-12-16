#!/bin/bash

cd $1
for a in `find -L . -type f`;do
   xxd --include $a
done

echo "void mount_$2(void){"
echo "static int done=0;if(done)return;done=1;"
for a in `find -L . -type f`;do
   b=`echo $a|sed s:\./::`;
   c=__`echo $b|sed s:[./-]:_:g`;
   echo "s0il_add_file(\"/$2/$b\", (char*)$c, "$c"_len, RUN_READONLY);";
done
echo "}";
