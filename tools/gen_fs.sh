#!/bin/bash

echo "typedef struct _CacheEntry2{const char *uri;"
echo "long length; "
echo "const char *data;}CacheEntry2;"
echo "static CacheEntry2 itk_fs[]={"
echo "#define ITK_HAVE_FS"

remove_base=$1/

for a in `find $1 -type f`;do
   b=`echo $a|sed s:$remove_base::`;
   echo "{\"$b\", `wc -c $a|cut -f 1 -d ' '`,";
   cat $a | sed 's/\\/\\\\/g' |\
            sed 's/\r/a/'     |\
            sed 's/"/\\"/g'   |\
            sed 's/^/"/'      |\
            sed 's/$/\\n"/'
            echo "},"
done
echo "{\"\", 6, \"fnord\"}," ;
echo "{NULL,0,NULL}};";

