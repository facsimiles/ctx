#!/bin/sh

HAVE_SDL=0
pkg-config sdl2 && HAVE_SDL=1
HAVE_BABL=0
pkg-config babl && HAVE_BABL=1
HAVE_CAIRO=0
pkg-config cairo && HAVE_CAIRO=1
HAVE_HARFBUZZ=0
pkg-config harfbuzz && HAVE_HARFBUZZ=1
HAVE_LIBCURL=0
pkg-config libcurl && HAVE_LIBCURL=1
HAVE_ALSA=0
pkg-config alsa && HAVE_ALSA=1

ENABLE_KMS=1
ENABLE_FB=0

CFLAGS='-O3'

CFLAGS_BACKEND=''

while test $# -gt 0
do
    case "$1" in
     "--without-sdl") HAVE_SDL=0    ;;
     "--debug") CFLAGS=''    ;;
     "--asan") CFLAGS=" -fsanitize=address";LIBS=' -lasan'  ;;
     "--ubsan") CFLAGS=" -fsanitize=undefined";LIBS=' -lasan'  ;;
     "--enable-kms") ENABLE_KMS=1 ;;
     "--without-kms") ENABLE_KMS=0 ;;
     "--enable-fb") ENABLE_FB=1 ;;
     "--without-fb") ENABLE_FB=0 ;;
     "--without-babl") HAVE_BABL=0 ;;
     "--without-cairo") HAVE_CAIRO=0 ;;
     "--without-harfbuzz") HAVE_HARFBUZZ=0 ;;
     "--without-alsa") HAVE_ALSA=0 ;;
     "--without-libcurl") HAVE_LIBCURL=0 ;;
     *|"--help") 
       echo "usage: ./configure [--without-sdl] [--without-babl] [--without-libcurl] [--without-alsa] [--debug|--asan|--ubsan] [--without-kms] [--enable-fb] [--without-cairo] [--without-harfbuzz]"
       exit 0
       ;;
    esac
    shift
done


echo > build.conf
if [ $HAVE_SDL = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config sdl2 --cflags` -DCTX_SDL=1 " >> build.conf
  echo "PKG_LIBS+= `pkg-config sdl2 --libs`" >> build.conf
else
  echo "PKG_CFLAGS+= -DCTX_SDL=0 " >> build.conf
fi

if [ $HAVE_BABL  = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config babl  --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config babl  --libs` " >> build.conf
else
  echo "PKG_CFLAGS+= -DNO_CAIRO" >> build.conf
fi

if [ $HAVE_HARFBUZZ = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config harfbuzz --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config harfbuzz --libs` " >> build.conf
  echo "PKG_CFLAGS+= -DCTX_HARFBUZZ=1" >> build.conf
else
  echo "PKG_CFLAGS+= -DCTX_HARFBUZZ=0" >> build.conf
fi

if [ $HAVE_CAIRO = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config cairo --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config cairo --libs` " >> build.conf
  echo "PKG_CFLAGS+= -DCTX_CAIRO=1" >> build.conf
else
  echo "PKG_CFLAGS+= -DCTX_CAIRO=0" >> build.conf
fi

if [ $HAVE_LIBCURL = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config libcurl --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config libcurl --libs` " >> build.conf
else
  echo "PKG_CFLAGS+= -DNO_LIBCURL" >> build.conf
fi

if [ $HAVE_ALSA = 1 ];then
  echo "PKG_CFLAGS+= `pkg-config alsa --cflags`" >> build.conf
  echo "PKG_LIBS+= `pkg-config alsa --libs` " >> build.conf
else
  echo "PKG_CFLAGS+= -DNO_ALSA" >> build.conf
fi

if [ $ENABLE_KMS = 1 ];then
  echo "PKG_CFLAGS+= -DCTX_KMS=1 " >> build.conf
else
  echo "PKG_CFLAGS+= -DCTX_KMS=0 " >> build.conf
fi

if [ $ENABLE_FB = 1 ];then
  echo "PKG_CFLAGS+= -DCTX_FB=1 " >> build.conf
else
  echo "PKG_CFLAGS+= -DCTX_FB=0 " >> build.conf
fi

echo "CFLAGS=$CFLAGS" >> build.conf
echo "LIBS=$LIBS" >> build.conf

#echo CCACHE=`command -v ccache` >> build.conf

#rm -f build.deps
#echo "Generating build.deps"
#make build.deps 2>/dev/null

echo "configuration summary:"
if [ $HAVE_SDL = 1 ];    then echo "    SDL2 yes";
                         else echo "    SDL2 no";fi
if [ $HAVE_BABL = 1 ];   then echo "    babl yes";
                         else echo "    babl no";fi
if [ $HAVE_CAIRO = 1 ];  then echo "   cairo yes";
                         else echo "   cairo no";fi

if [ $HAVE_HARFBUZZ = 1 ];then echo "harfbuzz yes";
                         else echo "harfbuzz no";fi

if [ $HAVE_ALSA = 1 ];   then echo "    alsa yes";
                         else echo "    alsa no";fi
if [ $HAVE_LIBCURL = 1 ];then echo " libcurl yes";
                         else echo " libcurl no";fi
if [ $ENABLE_KMS = 1 ];  then echo "     kms yes";
                         else echo "     kms no";fi
if [ $ENABLE_FB = 1 ];   then echo "      fb yes";
                         else echo "      fb no";fi
echo
echo

