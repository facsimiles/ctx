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
ENABLE_IMAGE_WRITE=1

ENABLE_FB=1
ENABLE_VT=1
ENABLE_SCREENSHOT=1
ENABLE_BRAILLE_TEXT=1
ENABLE_PDF=1
ENABLE_BLOATY_FAST_PATHS=1
ENABLE_INLINED_NORMAL=1
ENABLE_STATIC_FONTS=1
ENABLE_SHAPE_CACHE=0
ENABLE_HEADLESS=1
ENABLE_FONTS_FROM_FILE=1
ENABLE_FONT_CTX_FS=0
ENABLE_TINYVG=1
ENABLE_TERM=1
ENABLE_TERMIMG=1
ENABLE_STUFF=1
ENABLE_AUDIO=1

ENABLE_RASTERIZER=1
ENABLE_PARSER=1
ENABLE_FORMATTER=1
ENABLE_CMYK=1
ENABLE_ONLY_RGBA8=0
ENABLE_DITHER=0
ENABLE_EVENTS=1
ENABLE_FRAGMENT_SPECIALIZE=1
ENABLE_FAST_FILL_RECT=1
ENABLE_SWITCH_DISPATCH=1

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


CFLAGS='-O2 -g -fno-omit-frame-pointer'

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
     "--enable-screenshot") ENABLE_SCREENSHOT=1 ;;
     "--enable-braille_text") ENABLE_BRAILLE_TEXT=1 ;;
     "--enable-audio") ENABLE_AUDIO=1 ;;
     "--enable-cmyk") ENABLE_CMYK=1 ;;
     "--enable-CMYK") ENABLE_CMYK=1 ;;
     "--enable-stuff") ENABLE_STUFF=1 ;;
     "--enable-tinyvg") ENABLE_TINYVG=1 ;;
     "--enable-pdf") ENABLE_PDF=1 ;;
     "--enable-bloaty_fast_paths") ENABLE_BLOATY_FAST_PATHS=1 ;;
     "--enable-inlined_normal") ENABLE_INLINED_NORMAL=1 ;;
     "--enable-static_fonts") ENABLE_STATIC_FONTS=1 ;;
     "--enable-fragment_specialize") ENABLE_FRAGMENT_SPECIALIZE=1 ;;
     "--enable-fast_fill_rect") ENABLE_FAST_FILL_RECT=1 ;;
     "--enable-switch_dispatch") ENABLE_SWITCH_DISPATCH=1 ;;
     "--enable-only_rgba8") ENABLE_ONLY_RGBA8=1 ;;
     "--enable-rasterizer") ENABLE_RASTERIZER=1 ;;
     "--enable-parser") ENABLE_PARSER=1 ;;
     "--enable-formatter") ENABLE_FORMATTER=1 ;;
     "--enable-headless") ENABLE_HEADLESS=1 ;;
     "--enable-shape_cache") ENABLE_SHAPE_CACHE=1 ;;
     "--enable-fonts_from_file") ENABLE_FONTS_FROM_FILE=1 ;;
     "--enable-font_ctx_fs") ENABLE_FONT_CTX_FS=1 ;;
     "--enable-dither") ENABLE_DITHER=1 ;;
     "--enable-events") ENABLE_EVENTS=1 ;;
     "--enable-term") ENABLE_TERM=1 ;;
     "--enable-termimg") ENABLE_TERMIMG=1 ;;
     "--enable-stb_image") HAVE_STB_IMAGE=1 ;;
     "--enable-stb_tt") HAVE_STB_TT=1 ;;
     "--enable-image_write") ENABLE_IMAGE_WRITE=1 ;;
     "--enable-simd") HAVE_SIMD=1 ;;
     "--disable-term") ENABLE_TERM=0 ;;
     "--disable-termimg") ENABLE_TERMIMG=0 ;;
     "--disable-pl-mpeg") HAVE_PL_MPEG=0 ;;
     "--disable-stb_tt") HAVE_STB_TT=0 ;;
     "--disable-stb_image") HAVE_STB_IMAGE=0 ;;
     "--disable-image_write") ENABLE_IMAGE_WRITE=0 ;;
     "--disable-pdf") ENABLE_PDF=0 ;;
     "--disable-bloaty_fast_paths") ENABLE_BLOATY_FAST_PATHS=0 ;;
     "--disable-inlined_normal") ENABLE_INLINED_NORMAL=0 ;;
     "--disable-static_fonts") ENABLE_STATIC_FONTS=0 ;;
     "--disable-fragment_specialize") ENABLE_FRAGMENT_SPECIALIZE=0 ;;
     "--disable-fast_fill_rect") ENABLE_FAST_FILL_RECT=0 ;;
     "--disable-switch_dispatch") ENABLE_SWITCH_DISPATCH=0 ;;
     "--disable-only_rgba8") ENABLE_ONLY_RGBA8=0 ;;
     "--disable-rasterizer") ENABLE_RASTERIZER=0 ;;
     "--disable-parser") ENABLE_PARSER=0 ;;
     "--disable-formatter") ENABLE_FORMATTER=0 ;;
     "--disable-headless") ENABLE_HEADLESS=0 ;;
     "--disable-shape_cache") ENABLE_SHAPE_CACHE=0 ;;
     "--disable-fonts_from_file") ENABLE_FONTS_FROM_FILE=0 ;;
     "--disable-font-ctx-fs") ENABLE_FONT_CTX_FS=0 ;;
     "--disable-dither") ENABLE_DITHER=0 ;;
     "--disable-events") ENABLE_EVENTS=0 ;;
     "--disable-kms") HAVE_KMS=0 ;;
     "--disable-vt") ENABLE_VT=0 ;;
     "--disable-screenshot") ENABLE_SCREENSHOT=0 ;;
     "--disable-braille_text") ENABLE_BRAILLE_TEXT=0 ;;
     "--disable-audio") ENABLE_AUDIO=0 ;;
     "--disable-cmyk") ENABLE_CMYK=0 ;;
     "--disable-CMYK") ENABLE_CMYK=0 ;;
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
        ENABLE_SCREENSHOT=0 
        ENABLE_BRAILLE_TEXT=0 
        ENABLE_AUDIO=0 
        ENABLE_CMYK=0 
        ENABLE_STUFF=0 
        ENABLE_TINYVG=0 
        ENABLE_PDF=0 
        ENABLE_BLOATY_FAST_PATHS=0 
        ENABLE_INLINED_NORMAL=0 
        ENABLE_STATIC_FONTS=0 
        ENABLE_FRAGMENT_SPECIALIZE=0 
        ENABLE_FAST_FILL_RECT=0 
	ENABLE_SWITCH_DISPATCH=0 
        ENABLE_ONLY_RGBA8=0 
        ENABLE_HEADLESS=0 
        ENABLE_SHAPE_CACHE=0 
        ENABLE_FORMATTER=0 
        ENABLE_PARSER=1 
        #ENABLE_RASTERIZER=0 
        #ENABLE_FONTS_FROM_FILE=0 
        ENABLE_FONT_CTX_FS=0 
        ENABLE_DITHER=0 
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
        ENABLE_IMAGE_WRITE=0
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
echo > local.conf

if [ $HAVE_SDL = 1 ];then
  echo "#define CTX_SDL 1 " >> local.conf
  echo "CTX_CFLAGS+= `pkg-config sdl2 --cflags` " >> build.conf
  echo "CTX_LIBS+= `pkg-config sdl2 --libs`" >> build.conf
else
  echo "#define CTX_SDL 0 " >> local.conf
fi

echo >> build.conf
if [ $HAVE_BABL  = 1 ];then
  echo "#define CTX_BABL 1 " >> local.conf
  echo "#define CTX_ENABLE_CM 1 " >> local.conf
  echo "CTX_CFLAGS+= `pkg-config babl  --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config babl  --libs` " >> build.conf
else
  echo "#define CTX_BABL 0 " >> local.conf
  echo "#define CTX_ENABLE_CM 0 " >> local.conf
fi

echo -n "#define CTX_AUDIO " >> local.conf; if [ $ENABLE_AUDIO = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_VT " >> local.conf; if [ $ENABLE_VT = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_SCREENSHOT " >> local.conf; if [ $ENABLE_SCREENSHOT = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_BRAILLE_TEXT " >> local.conf; if [ $ENABLE_BRAILLE_TEXT = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_ENABLE_CMYK " >> local.conf; if [ $ENABLE_CMYK = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_TINYVG " >> local.conf; if [ $ENABLE_TINYVG = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_PDF " >> local.conf; if [ $ENABLE_PDF = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_BLOATY_FAST_PATHS " >> local.conf; if [ $ENABLE_BLOATY_FAST_PATHS = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_INLINED_NORMAL " >> local.conf; if [ $ENABLE_INLINED_NORMAL = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_STATIC_FONTS " >> local.conf; if [ $ENABLE_STATIC_FONTS = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_FRAGMENT_SPECIALIZE " >> local.conf; if [ $ENABLE_FRAGMENT_SPECIALIZE = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_FAST_FILL_RECT " >> local.conf; if [ $ENABLE_FAST_FILL_RECT = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_RASTERIZER_SWITCH_DISPATCH " >> local.conf; if [ $ENABLE_SWITCH_DISPATCH = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_FONTS_FROM_FILE " >> local.conf; if [ $ENABLE_FONTS_FROM_FILE = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_FONT_ENGINE_CTX_FS " >> local.conf; if [ $ENABLE_FONT_CTX_FS = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_TERM " >> local.conf; if [ $ENABLE_TERM = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_TERMIMG " >> local.conf; if [ $ENABLE_TERMIMG = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_DITHER " >> local.conf; if [ $ENABLE_DITHER = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_EVENTS " >> local.conf; if [ $ENABLE_EVENTS = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_RASTERIZER " >> local.conf; if [ $ENABLE_RASTERIZER = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_PARSER " >> local.conf; if [ $ENABLE_PARSER = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_FORMATTER " >> local.conf; if [ $ENABLE_FORMATTER = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_HEADLESS " >> local.conf; if [ $ENABLE_HEADLESS = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi
echo -n "#define CTX_SHAPE_CACHE " >> local.conf; if [ $ENABLE_SHAPE_CACHE = 1 ];then echo "1" >> local.conf; else echo "0" >> local.conf; fi


echo -n "CTX_CFLAGS+= -DCTX_STUFF=" >> build.conf; if [ $ENABLE_STUFF = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_PL_MPEG=" >> build.conf; if [ $HAVE_PL_MPEG = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_STB_TT=" >> build.conf; if [ $HAVE_STB_TT = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_STB_IMAGE=" >> build.conf; if [ $HAVE_STB_IMAGE = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi
echo -n "CTX_CFLAGS+= -DCTX_IMAGE_WRITE=" >> build.conf; if [ $ENABLE_IMAGE_WRITE = 1 ];then echo "1" >> build.conf; else echo "0" >> build.conf; fi


if [ $ENABLE_ONLY_RGBA8 = 1 ];then
  echo "#define CTX_LIMIT_FORMATS   1 " >> local.conf
  echo "#define CTX_ENABLE_RGBA8    1 " >> local.conf
  echo "#define CTX_NATIVE_GRAYA8   0 " >> local.conf
  echo "#define CTX_U8_TO_FLOAT_LUT 0 " >> local.conf
fi

if [ $HAVE_HARFBUZZ = 1 ];then
  echo "#define CTX_HARFBUZZ 1 " >> local.conf
  echo "CTX_CFLAGS+= `pkg-config harfbuzz --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config harfbuzz --libs` " >> build.conf
else
  echo "#define CTX_HARFBUZZ 0 " >> local.conf
fi

if [ $HAVE_CAIRO = 1 ];then
  echo "#define CTX_CAIRO 1 " >> local.conf
  echo "CTX_CFLAGS+= `pkg-config cairo --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config cairo --libs` " >> build.conf
else
  echo "#define CTX_CAIRO 0 " >> local.conf
fi

if [ $HAVE_LIBCURL = 1 ];then
  echo "#define CTX_CURL 1 " >> local.conf
  echo "CTX_CFLAGS+= `pkg-config libcurl --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config libcurl --libs` " >> build.conf
else
  echo "#define CTX_CURL 0 " >> local.conf
fi

if [ $HAVE_ALSA = 1 ];then
  echo "#define CTX_ALSA 1 " >> local.conf
  echo "CTX_CFLAGS+= `pkg-config alsa --cflags`" >> build.conf
  echo "CTX_LIBS+= `pkg-config alsa --libs` " >> build.conf
else
  echo "#define CTX_ALSA 0 " >> local.conf
fi

if [ $HAVE_KMS = 1 ];then
  echo "#define CTX_KMS 1 " >> local.conf
  echo "CTX_CFLAGS+= `pkg-config libdrm --cflags`" >> build.conf
else
  echo "#define CTX_KMS 0 " >> local.conf
fi

if [ $ENABLE_FB = 1 ];then
  echo "#define CTX_FB 1 " >> local.conf
else
  echo "#define CTX_FB 0 " >> local.conf
fi

case "$ARCH" in
   "x86_64")  
      echo "CTX_CFLAGS+= -DCTX_X86_64 " >>  build.conf
      ;;
   "armv7l")
      echo "CTX_CFLAGS+= -DCTX_ARMV7L -march=armv7" >>  build.conf
      ;;
   *)         ;;
esac

if [ $HAVE_SIMD = 1 ];then
  echo "CTX_CFLAGS+= -DCTX_SIMD=1 " >> build.conf
  echo "CTX_SIMD=1" >>  build.conf
else
  echo "CTX_CFLAGS+= -DCTX_SIMD=0 " >> build.conf
  echo "CTX_SIMD=0" >>  build.conf
fi
echo "CTX_ARCH=$ARCH" >>  build.conf
echo "CFLAGS=$CFLAGS" >> build.conf
echo "LIBS=$LIBS" >> build.conf

#echo CCACHE=`command -v ccache` >> build.conf

#rm -f build.deps
#echo "Generating build.deps"
#make build.deps 2>/dev/null

echo -n "configuration summary, architecture $(arch)"
[ $HAVE_SIMD = 1 ]  && echo " SIMD multi-pass"
echo ""
echo "Backends:"
echo -n " kms      "; [ $HAVE_KMS = 1 ]        && echo "yes" || echo "no (libdrm-dev)"
echo -n " fb       "; [ $ENABLE_FB = 1 ]       && echo "yes" || echo "no"
echo -n " SDL2     "; [ $HAVE_SDL = 1 ]        && echo "yes" || echo "no (libsdl2-dev)"
echo -n " pdf      "; [ $ENABLE_PDF = 1 ]      && echo "yes" || echo "no"
echo -n " term     "; [ $ENABLE_TERM = 1 ]     && echo "yes" || echo "no"
echo -n " termimg  "; [ $ENABLE_TERMIMG = 1 ]  && echo "yes" || echo "no"
echo -n " cairo    "; [ $HAVE_CAIRO = 1 ]      && echo "yes" || echo "no (libcairo2-dev)"
echo -n " headless "; [ $ENABLE_HEADLESS = 1 ] && echo "yes" || echo "no"
echo ""
echo "External libraries:"
echo -n " babl            "; [ $HAVE_BABL = 1 ]     && echo "yes" || echo "no (libbabl-dev)"
echo -n " harfbuzz        "; [ $HAVE_HARFBUZZ = 1 ] && echo "yes" || echo "no (libharfbuzz-dev, currently unused)"
echo -n " alsa            "; [ $HAVE_ALSA = 1 ]     && echo "yes" || echo "no (libasound-dev)"
echo -n " libcurl         "; [ $HAVE_LIBCURL = 1 ]  && echo "yes" || echo "no (libcurl-4-openssl-dev)"
echo ""
echo "Built-in/vendored libraries"
echo -n " pl_mpeg         "; [ $HAVE_PL_MPEG = 1 ]    && echo -n "yes" || echo -n "no"
echo "    mpeg1 video player, works well as top-level but not as client due to lack of SHM"
echo -n " stb_tt          "; [ $HAVE_STB_TT  = 1 ]    && echo -n "yes" || echo -n "no"
echo "   optional support for using TTF/OTF fonts"
echo -n " stb_image       "; [ $HAVE_STB_IMAGE  = 1 ] && echo -n "yes" || echo -n "no"
echo "    support for loading jpg,png,gif"
echo ""
echo "Features"
echo -n " image_write "; [ $ENABLE_IMAGE_WRITE  = 1 ] && echo -n "yes" || echo -n "no"
echo "    support for writing PNG files/screenshots/thumbnails, using miniz - and lots of heap"
echo -n " vt              "; [ $ENABLE_VT = 1 ] && echo -n "yes" || echo -n "no" ; 
echo "   DEC+ctx terminal engine"
echo -n " braille_text    "; [ $ENABLE_BRAILLE_TEXT = 1 ] && echo -n "yes" || echo -n "no" ; 
echo "    use of text overrides in unicode graphics backend"
echo -n " audio           "; [ $ENABLE_AUDIO = 1 ] && echo -n "yes" || echo -n "no" ; 
echo "    audio handling (both alsa and ctx backend)"
echo -n " screenshot      "; [ $ENABLE_SCREENSHOT = 1 ] && echo -n "yes" || echo -n "no" ; 
echo "    code for saving PNG"
echo -n " stuff           "; [ $ENABLE_STUFF = 1 ] && echo -n "yes" || echo -n "no"
echo "   media creation and curation editor"
echo -n " tinyvg          "; [ $ENABLE_TINYVG = 1 ] && echo -n "yes" || echo -n "no"
echo "   tinyvg viewer"
echo -n " CMYK            "; [ $ENABLE_CMYK = 1 ] && echo -n "yes" || echo -n "no" ; 
echo "  extra code for handling CMYK color space"
echo -n " dither          "; [ $ENABLE_DITHER = 1 ] && echo "yes" || echo "no"
echo -n " events          "; [ $ENABLE_EVENTS = 1 ] && echo "yes" || echo "no"
echo -n " font_ctx_fs     "; [ $ENABLE_FONT_CTX_FS = 1 ] && echo "yes" || echo "no"
echo -n " fonts_from_file "; [ $ENABLE_FONTS_FROM_FILE = 1 ] && echo "yes" || echo "no"
echo -n " rasterizer      "; [ $ENABLE_RASTERIZER = 1 ] && echo "yes" || echo "no"
echo -n " parser          "; [ $ENABLE_PARSER = 1 ] && echo "yes" || echo "no"
echo -n " formatter       "; [ $ENABLE_FORMATTER = 1 ] && echo "yes" || echo "no"
echo -n " shape_cache     "; [ $ENABLE_SHAPE_CACHE = 1 ] && echo "yes" || echo "no"
echo -n " only_rgba8      "; [ $ENABLE_ONLY_RGBA8  = 1 ] && echo "yes" || echo "no"
echo -n " fragment_specialize "; [ $ENABLE_FRAGMENT_SPECIALIZE = 1 ] && echo "yes" || echo "no"
echo -n " fast_fill_rect      "; [ $ENABLE_FAST_FILL_RECT = 1 ] && echo "yes" || echo "no"
echo -n " switch_dispatch     "; [ $ENABLE_SWITCH_DISPATCH = 1 ] && echo "yes" || echo "no"
echo -n " static_fonts        "; [ $ENABLE_STATIC_FONTS = 1 ] && echo "yes" || echo "no"
echo -n " inlined_normal      "; [ $ENABLE_INLINED_NORMAL = 1 ] && echo "yes" || echo "no"
echo -n " bloaty_fast_paths   "; [ $ENABLE_BLOATY_FAST_PATHS = 1 ] && echo "yes" || echo "no"


echo
echo CFLAGS=$CFLAGS
echo
