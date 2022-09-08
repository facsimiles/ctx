#!/bin/sh

HAVE_SDL=0
pkg-config sdl2 && HAVE_SDL=1
HAVE_BABL=0
pkg-config babl && HAVE_BABL=1
HAVE_CAIRO=0
pkg-config cairo && HAVE_CAIRO=1
HAVE_HARFBUZZ=0
# commented out to not auto-detect
#pkg-config harfbuzz && HAVE_HARFBUZZ=1
HAVE_LIBCURL=0
pkg-config libcurl && HAVE_LIBCURL=1
HAVE_ALSA=0
pkg-config alsa && HAVE_ALSA=1
HAVE_KMS=0
pkg-config libdrm && HAVE_KMS=1


HAVE_PL_MPEG=1
HAVE_STB_TT=1
HAVE_STB_IMAGE=1
HAVE_STB_IMAGE_WRITE=1

ENABLE_VT=1
ENABLE_STUFF=1
ENABLE_TINYVG=1
ENABLE_PDF=1

ARCH=`uname -m`

case "$ARCH" in
   "x86_64")  HAVE_SIMD=1 ;;
   "armv7l")  HAVE_SIMD=1 ;;
   *)         HAVE_SIMD=0 ;; 
esac

ENABLE_FB=1

CFLAGS='-O2 -g '

CFLAGS_BACKEND=''

while test $# -gt 0
do
    case "$1" in
     "--disable-sdl") HAVE_SDL=0    ;;
     "--debug") CFLAGS=' -g ' ; HAVE_SIMD=0   ;;
     "--static") CFLAGS='-Os' HAVE_SIMD=0 HAVE_SDL=0 HAVE_BABL=0 HAVE_CAIRO=0  HAVE_LIBCURL=0 HAVE_ALSA=0 HAVE_HARFBUZZ=0 ;;
     "--asan") CFLAGS=" -fsanitize=address -g";LIBS=' -lasan -g '  ;;
     "--ubsan") CFLAGS=" -fsanitize=undefined -g";LIBS=' -lasan -g '  ;;
     "--enable-kms") HAVE_KMS=1 ;;
     "--enable-sdl") HAVE_SDL=1 ;;
     "--enable-cairo") HAVE_CAIRO=1 ;;
     "--enable-babl") HAVE_BABL=1 ;;
     "--enable-alsa") HAVE_ALSA=1 ;;
     "--enable-libcurl") HAVE_LIBCURL=1 ;;
     "--enable-harfbuzz") HAVE_HARFBUZZ=1 ;;
     "--disable-kms") HAVE_KMS=0 ;;
     "--enable-fb") ENABLE_FB=1 ;;
     "--enable-vt") ENABLE_VT=1 ;;
     "--enable-stuff") ENABLE_STUFF=1 ;;
     "--disable-stuff") ENABLE_STUFF=1 ;;
     "--enable-tinyvg") ENABLE_TINYVG=1 ;;
     "--disable-tinyvg") ENABLE_TINYVG=1 ;;
     "--enable-pdf") ENABLE_PDF=1 ;;
     "--disable-pdf") ENABLE_PDF=1 ;;
     "--disable-vt") ENABLE_VT=0 ;;
     "--disable-pl-mpeg") HAVE_PL_MPEG=0 ;;
     "--disable-stb_tt") HAVE_STB_TT=0 ;;
     "--disable-stb_image") HAVE_STB_IMAGE=0 ;;
     "--disable-stb_image_write") HAVE_STB_IMAGE_WRITE=0 ;;
     "--enable-simd") HAVE_SIMD=1 ;;
     "--disable-simd") HAVE_SIMD=0 ;;
     "--disable-fb") ENABLE_FB=0 ;;
     "--disable-babl") HAVE_BABL=0 ;;
     "--disable-cairo") HAVE_CAIRO=0 ;;
     "--disable-harfbuzz") HAVE_HARFBUZZ=0 ;;
     "--disable-alsa") HAVE_ALSA=0 ;;
     "--disable-libcurl") HAVE_LIBCURL=0 ;;
     "--disable-all")
        HAVE_LIBCURL=0
        HAVE_KMS=0
        HAVE_SIMD=0 
        ENABLE_FB=0 
        ENABLE_VT=0 
        ENABLE_STUFF=0 
        ENABLE_TINYVG=0 
        ENABLE_PDF=0 
        HAVE_BABL=0 
        HAVE_SDL=0 
        HAVE_CAIRO=0 
        HAVE_HARFBUZZ=0 
        HAVE_ALSA=0 
        HAVE_LIBCURL=0 
        HAVE_LIBCURL=0
        HAVE_PL_MPEG=0
        HAVE_STB_TT=0
        HAVE_STB_IMAGE=0
        HAVE_STB_IMAGE_WRITE=0
             ;;
     *|"--help") 
       if [ "x$1" != "x--help" ]; then echo unrecognized option $1 ; fi
       echo "usage: ./configure [options]"
       echo "Where recognized options are: "
       echo "  --enable-FEATURE   where FEATURE is one of cairo, harfbuzz"
       echo "  --disable-FEATURE  fb, kms, sdl, alsa, babl, libcurl and simd"
       echo ""
       echo "  --disable-all      disable all features, to be used before enabling"
       echo "                     features individually"
       echo "  --asan             do an asan build"
       echo "  --ubsan            do an ubsan build"
       echo "  --debug            do a debug build (faster)"
       exit 0
       ;;
    esac
    shift
done

echo > build.conf
if [ $HAVE_SDL = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_SDL=1 " >> build.conf
  echo "CTX_CFLAGS+= `pkg-config sdl2 --cflags` " >> build.conf
  echo "CTX_LIBS+= `pkg-config sdl2 --libs`" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_SDL=0 " >> build.conf
fi

echo >> build.conf
if [ $HAVE_BABL  = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_BABL=1" >> build.conf
  echo "CTX_CFLAGS+= `pkg-config babl  --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config babl  --libs` " >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_BABL=0" >> build.conf
fi

if [ $HAVE_PL_MPEG = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_PL_MPEG=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_PL_MPEG=0" >> build.conf
fi

if [ $ENABLE_VT = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_VT=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_VT=0" >> build.conf
fi

if [ $ENABLE_TINYVG = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_TINYVG=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_TINYVG=0" >> build.conf
fi

if [ $ENABLE_PDF = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_PDF=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_PDF=0" >> build.conf
fi


if [ $ENABLE_STUFF = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_STUFF=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_STUFF=0" >> build.conf
fi

if [ $HAVE_STB_TT = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_STB_TT=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_STB_TT=0" >> build.conf
fi

if [ $HAVE_STB_IMAGE = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_STB_IMAGE=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_STB_IMAGE=0" >> build.conf
fi

if [ $HAVE_STB_IMAGE_WRITE = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_STB_IMAGE_WRITE=1" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_STB_IMAGE_WRITE=0" >> build.conf
fi

echo >> build.conf
if [ $HAVE_HARFBUZZ = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_HARFBUZZ=1" >> build.conf
  echo "CTX_CFLAGS+= `pkg-config harfbuzz --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config harfbuzz --libs` " >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_HARFBUZZ=0" >> build.conf
fi

echo >> build.conf
if [ $HAVE_CAIRO = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_CAIRO=1" >> build.conf
  echo "CTX_CFLAGS+= `pkg-config cairo --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config cairo --libs` " >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_CAIRO=0" >> build.conf
fi

echo >> build.conf
if [ $HAVE_LIBCURL = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_CURL=1" >> build.conf
  echo "CTX_CFLAGS+= `pkg-config libcurl --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config libcurl --libs` " >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_CURL=0" >> build.conf
fi

echo >> build.conf
if [ $HAVE_ALSA = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_ALSA=1" >> build.conf
  echo "CTX_CFLAGS+= `pkg-config alsa --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config alsa --libs` " >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_ALSA=0" >> build.conf
fi

echo >> build.conf
if [ $HAVE_KMS = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_KMS=1" >> build.conf
  echo "CTX_CFLAGS+= `pkg-config libdrm --cflags`" >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_KMS=0 " >> build.conf
fi

echo >> build.conf
if [ $ENABLE_FB = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_FB=1 " >> build.conf
else
  echo "CTX_CFLAGS+= -DCTX_FB=0 " >> build.conf
fi
echo >> build.conf

if [ x$ARCH = "xx86_64" ]; then echo "CTX_CFLAGS+= -DCTX_X86_64 " >>  build.conf; fi

case "$ARCH" in
   "x86_64")  
      echo "CTX_CFLAGS+= -DCTX_X86_64 " >>  build.conf
      ;;
   "armv7l")
      echo "CTX_CFLAGS+= -DCTX_ARMV7L -march=armv7" >>  build.conf
      ;;
   *)         ;;
esac

echo >> build.conf
if [ $HAVE_SIMD = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_SIMD=1 " >> build.conf
  echo "CTX_SIMD=1" >>  build.conf
else
  echo "CTX_CFLAGS+= -DCTX_SIMD=0 " >> build.conf
  echo "CTX_SIMD=0" >>  build.conf
fi
echo "CTX_ARCH=$ARCH" >>  build.conf


echo >> build.conf
echo "CFLAGS=$CFLAGS" >> build.conf
echo "LIBS=$LIBS" >> build.conf

#echo CCACHE=`command -v ccache` >> build.conf

#rm -f build.deps
#echo "Generating build.deps"
#make build.deps 2>/dev/null

echo "configuration summary, architecture $(arch):"
echo -n " SIMD     "; [ $HAVE_SIMD = 1 ]  && echo "yes" || echo "no"
echo -n " SDL2     "; [ $HAVE_SDL = 1 ]   && echo "yes" || echo "no (libsdl2-dev)"
echo -n " babl     "; [ $HAVE_BABL = 1 ]  && echo "yes" || echo "no (libbabl-dev)"
echo -n " cairo    "; [ $HAVE_CAIRO = 1 ] && echo "yes" || echo "no (libcairo2-dev)"

echo -n " harfbuzz "; [ $HAVE_HARFBUZZ = 1 ] && echo "yes" || echo "no (libharfbuzz-dev)"
echo -n " alsa     "; [ $HAVE_ALSA = 1 ]     && echo "yes" || echo "no (libasound-dev)"
echo -n " libcurl  "; [ $HAVE_LIBCURL = 1 ]  && echo "yes" || echo "no (libcurl-4-openssl-dev)"
echo -n " kms      "; [ $HAVE_KMS = 1 ]     && echo "yes" || echo "no (libdrm-dev)"

echo -n " fb       "; [ $ENABLE_FB = 1 ]    && echo "yes" || echo "no"
echo -n " pl_mpeg  "; [ $HAVE_PL_MPEG = 1 ] && echo "yes" || echo "no"
echo -n " stb_tt   "; [ $HAVE_STB_TT  = 1 ] && echo "yes" || echo "no"

echo -n " vt       "; [ $ENABLE_VT = 1 ] && echo "yes" || echo "no"
echo -n " stuff    "; [ $ENABLE_STUFF = 1 ] && echo "yes" || echo "no"
echo -n " tinyvg   "; [ $ENABLE_TINYVG = 1 ] && echo "yes" || echo "no"
echo -n " pdf      "; [ $ENABLE_PDF = 1 ] && echo "yes" || echo "no"



echo
echo CFLAGS=$CFLAGS
echo
