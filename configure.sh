#!/bin/sh

HAVE_SDL=0
pkg-config sdl2 && HAVE_SDL=1
HAVE_BABL=0
pkg-config babl && HAVE_BABL=1
echo > build.conf
if [ $HAVE_SDL = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config sdl2 --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config sdl2 --libs`" >> build.conf
else
  echo "PKG_CFLAGS+= '-DNO_BABL' " >> build.conf
fi

if [ $HAVE_BABL = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config babl --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config babl --libs` " >> build.conf
else
  echo "PKG_CFLAGS+= '-DNO_BABL'" >> build.conf
fi
echo CCACHE=`which ccache` >> build.conf

#rm -f build.deps
#echo "Generating build.deps"
#make build.deps 2>/dev/null

echo "configuration of optional depenencies complete:"
if [ $HAVE_SDL = 1 ];  then echo "  SDL2 yes";
                       else echo "  SDL2 no";fi
if [ $HAVE_BABL = 1 ]; then echo "  babl yes";
                       else echo "  babl no";fi
echo
echo "Ready to build"

