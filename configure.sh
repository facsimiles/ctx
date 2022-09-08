#!/bin/sh

HAVE_SDL=0
HAVE_BABL=0
HAVE_CAIRO=0
HAVE_HARFBUZZ=0
HAVE_LIBCURL=0
HAVE_ALSA=0
HAVE_KMS=0

HAVE_PL_MPEG=1
HAVE_STB_TT=1
HAVE_STB_IMAGE=1
HAVE_STB_IMAGE_WRITE=1

ENABLE_FB=1
ENABLE_VT=1
ENABLE_PDF=1
ENABLE_TINYVG=1
ENABLE_TERM=1
ENABLE_TERMIMG=1
ENABLE_STUFF=1

pkg-config sdl2    && HAVE_SDL=1
pkg-config babl    && HAVE_BABL=1
pkg-config cairo   && HAVE_CAIRO=1
pkg-config libcurl && HAVE_LIBCURL=1
pkg-config alsa    && HAVE_ALSA=1
pkg-config libdrm  && HAVE_KMS=1
#pkg-config harfbuzz && HAVE_HARFBUZZ=1



ARCH=`uname -m`

case "$ARCH" in
   "x86_64")  HAVE_SIMD=1 ;;
   "armv7l")  HAVE_SIMD=1 ;;
   *)         HAVE_SIMD=0 ;; 
esac


CFLAGS='-O2 -g '

CFLAGS_BACKEND=''

while test $# -gt 0
do
    case "$1" in
     "--debug") CFLAGS=' -g ' ; HAVE_SIMD=0   ;;
     "--static") CFLAGS='-Os' HAVE_SIMD=0 HAVE_SDL=0 HAVE_BABL=0 HAVE_CAIRO=0  HAVE_LIBCURL=0 HAVE_ALSA=0 HAVE_HARFBUZZ=0 ;;
     "--asan") CFLAGS=" -fsanitize=address -g";LIBS=' -lasan -g '  ;;
     "--ubsan") CFLAGS=" -fsanitize=undefined -g";LIBS=' -lasan -g '  ;;
     "--enable-kms") HAVE_KMS=1 ;;
     "--disable-sdl") HAVE_SDL=0    ;;
     "--enable-SDL2") HAVE_SDL=1 ;;
     "--disable-SDL2") HAVE_SDL=0    ;;
     "--enable-sdl") HAVE_SDL=1 ;;
     "--enable-cairo") HAVE_CAIRO=1 ;;
     "--enable-babl") HAVE_BABL=1 ;;
     "--enable-alsa") HAVE_ALSA=1 ;;
     "--enable-libcurl") HAVE_LIBCURL=1 ;;
     "--enable-harfbuzz") HAVE_HARFBUZZ=1 ;;
     "--enable-fb") ENABLE_FB=1 ;;
     "--enable-vt") ENABLE_VT=1 ;;
     "--enable-stuff") ENABLE_STUFF=1 ;;
     "--enable-tinyvg") ENABLE_TINYVG=1 ;;
     "--enable-pdf") ENABLE_PDF=1 ;;
     "--enable-term") ENABLE_TERM=1 ;;
     "--enable-termimg") ENABLE_TERMIMG=1 ;;
     "--enable-simd") HAVE_SIMD=1 ;;
     "--disable-term") ENABLE_TERM=0 ;;
     "--disable-termimg") ENABLE_TERMIMG=0 ;;
     "--disable-pl-mpeg") HAVE_PL_MPEG=0 ;;
     "--disable-stb_tt") HAVE_STB_TT=0 ;;
     "--disable-stb_image") HAVE_STB_IMAGE=0 ;;
     "--disable-stb_image_write") HAVE_STB_IMAGE_WRITE=0 ;;
     "--disable-pdf") ENABLE_PDF=0 ;;
     "--disable-kms") HAVE_KMS=0 ;;
     "--disable-vt") ENABLE_VT=0 ;;
     "--disable-stuff") ENABLE_STUFF=0 ;;
     "--disable-tinyvg") ENABLE_TINYVG=0 ;;
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
        ENABLE_TERM=0 
        ENABLE_TERMIMG=0 
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
       echo "  --enable-FEATURE   where FEATURE is one of the feature names listed in the output summary"
       echo "  --disable-FEATURE  "
       echo ""
       echo "  --disable-all      disable all features, to be used before enabling"
       echo "                     features individually"
       echo "  --static           configure for a static build (for use with the ctx.static build target)"
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

echo -n "CTX_CFLAGS+= -DCTX_VT=" >> build.conf; if [ $ENABLE_VT = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_TINYVG=" >> build.conf; if [ $ENABLE_TINYVG = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_PDF=" >> build.conf; if [ $ENABLE_PDF = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_TERM=" >> build.conf; if [ $ENABLE_TERM = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_TERMIMG=" >> build.conf; if [ $ENABLE_TERMIMG = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_STUFF=" >> build.conf; if [ $ENABLE_STUFF = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_PL_MPEG=" >> build.conf; if [ $HAVE_PL_MPEG = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_STB_TT=" >> build.conf; if [ $HAVE_STB_TT = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_STB_IMAGE=" >> build.conf; if [ $HAVE_STB_IMAGE = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_STB_IMAGE_WRITE=" >> build.conf; if [ $HAVE_STB_IMAGE_WRITE = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi

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

echo -n "configuration summary, architecture $(arch)"
[ $HAVE_SIMD = 1 ]  && echo "SIMD multi-pass" || echo ""
echo ""
echo "Backends:"
echo -n " kms     "; [ $HAVE_KMS = 1 ]     && echo "yes" || echo "no (libdrm-dev)"
echo -n " fb      "; [ $ENABLE_FB = 1 ]    && echo "yes" || echo "no"
echo -n " SDL2    "; [ $HAVE_SDL = 1 ]   && echo "yes" || echo "no (libsdl2-dev)"
echo -n " pdf     "; [ $ENABLE_PDF = 1 ] && echo "yes" || echo "no"
echo -n " term    "; [ $ENABLE_TERM = 1 ] && echo "yes" || echo "no"
echo -n " termimg "; [ $ENABLE_TERMIMG = 1 ] && echo "yes" || echo "no"
echo -n " cairo   "; [ $HAVE_CAIRO = 1 ] && echo "yes" || echo "no (libcairo2-dev)"
echo ""
echo "External libraries:"
echo -n " babl            "; [ $HAVE_BABL = 1 ]  && echo "yes" || echo "no (libbabl-dev)"
echo -n " harfbuzz        "; [ $HAVE_HARFBUZZ = 1 ] && echo "yes" || echo "no (libharfbuzz-dev, currently unused)"
echo -n " alsa            "; [ $HAVE_ALSA = 1 ]     && echo "yes" || echo "no (libasound-dev)"
echo -n " libcurl         "; [ $HAVE_LIBCURL = 1 ]  && echo "yes" || echo "no (libcurl-4-openssl-dev)"
echo ""
echo "Built-in/vendored libraries"
echo -n " pl_mpeg         "; [ $HAVE_PL_MPEG = 1 ] && echo "yes" || echo "no"
echo "   mpeg1 video player, works well as top-level but not as client due to lack of SHM"
echo -n " stb_tt          "; [ $HAVE_STB_TT  = 1 ] && echo "yes" || echo "no"
echo "   optional support for using TTF/OTF fonts"
echo -n " stb_image       "; [ $HAVE_STB_IMAGE  = 1 ] && echo -n "yes" || echo -n "no"
echo "   support for loading jpg,png,gif"
echo -n " stb_image_write "; [ $HAVE_STB_IMAGE_WRITE  = 1 ] && echo -n "yes" || echo -n "no"
echo "   support for writing PNG files/screenshots/thumbnails"
echo ""
echo "Features"
echo -n " vt              "; [ $ENABLE_VT = 1 ] && echo -n "yes" || echo -n "no" ; 
echo "   DEC+ctx terminal engine"
echo -n " stuff           "; [ $ENABLE_STUFF = 1 ] && echo -n "yes" || echo -n "no"
echo "   media creation and curation editor"
echo -n " tinyvg          "; [ $ENABLE_TINYVG = 1 ] && echo -n "yes" || echo -n "no"
echo "   tinyvg viewer"


echo
echo CFLAGS=$CFLAGS
echo
