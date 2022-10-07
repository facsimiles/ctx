#!/bin/bash

echo "typedef struct _CacheEntry2{const char *uri;"
echo "long length; "
echo "const unsigned char *data;}CacheEntry2;"
echo "#define ITK_HAVE_FS"

cd $1

for a in `find . -type f`;do
   xxd --include $a
done

echo "static CacheEntry2 itk_fs[]={"
for a in `find . -type f`;do
   b=`echo $a|sed s:\./::`;
   c=__`echo $b|sed s:[./]:_:g`;
   echo "{\"$b\", `wc -c $a|cut -f 1 -d ' '`,";
   echo "$c},"
done
echo "{NULL,0,NULL}};";

