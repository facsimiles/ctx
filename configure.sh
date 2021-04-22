#!/bin/sh

HAVE_SDL=0
pkg-config sdl2 && HAVE_SDL=1
HAVE_BABL=0
pkg-config babl && HAVE_BABL=1
HAVE_LIBCURL=0
pkg-config libcurl && HAVE_LIBCURL=1
CFLAGS='-O3'


while test $# -gt 0
do
    case "$1" in
     "--without-sdl") HAVE_SDL=0    ;;
     "--debug") CFLAGS=''    ;;
     "--asan") CFLAGS="$CFLAGS -fsanitize=address";LIBS=' -lasan'  ;;
     "--without-babl") HAVE_BABL=0 ;;
     "--without-libcurl") HAVE_LIBCURL=0 ;;
     *|"--help") 
       echo "usage: ./configure [--without-sdl] [--without-babl] [--without-libcurl]"
       exit 0
       ;;
    esac
    shift
done


echo > build.conf
if [ $HAVE_SDL = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config sdl2 --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config sdl2 --libs`" >> build.conf
else
  echo "PKG_CFLAGS+= '-DNO_SDL' " >> build.conf
fi

if [ $HAVE_BABL = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config babl --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config babl --libs` " >> build.conf
else
  echo "PKG_CFLAGS+= '-DNO_BABL'" >> build.conf
fi
if [ $HAVE_LIBCURL = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config libcurl --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config libcurl --libs` " >> build.conf
else
  echo "PKG_CFLAGS+= '-DNO_LIBCURL'" >> build.conf
fi

echo "CFLAGS=$CFLAGS" >> build.conf
echo "LIBS=$LIBS" >> build.conf

echo CCACHE=`which ccache` >> build.conf

#rm -f build.deps
#echo "Generating build.deps"
#make build.deps 2>/dev/null

echo "configuration of optional depenencies complete:"
if [ $HAVE_SDL = 1 ];  then echo "    SDL2 yes";
                       else echo "    SDL2 no";fi
if [ $HAVE_BABL = 1 ]; then echo "    babl yes";
                       else echo "    babl no";fi
if [ $HAVE_LIBCURL = 1 ];then echo " libcurl yes";
                       else echo " libcurl no";fi
echo
echo "Ready to build"

