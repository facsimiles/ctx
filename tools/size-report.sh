#!/bin/sh

FONT=`ls -l ../fonts/ctxf/ascii.ctxf|cut -f 5 -d ' '`
BASELINE=`ls -l ../examples/musl-static-baseline|cut -f 5 -d ' '`
TINY=`ls -l ../examples/musl-static-tiny|cut -f 5 -d ' '`
SMALL=`ls -l ../examples/musl-static-small|cut -f 5 -d ' '`

CtxRasterizer=`./ctx-info|grep Rasterizer|cut -f 3 -d ' '`
CtxState=`./ctx-info|grep State|cut -f 3 -d ' '`
Ctx=`./ctx-info|grep '(Ctx)'|cut -f 3 -d ' '`

echo "Sizes computed with static musl libc binaries, where the baseline"
echo "version contains the same raw data and c library calls."

echo "ASCII font:      " $(($FONT)) " bytes"
echo "RGBA8 Rasterizer:  " $(($TINY-$BASELINE-$FONT)) " bytes"
echo "CTX text parser: " $(($SMALL-$TINY)) " bytes"

BASELINE=`ls -l ../examples/baseline|cut -f 5 -d ' '`
TINY=`ls -l ../examples/tiny|cut -f 5 -d ' '`
SMALL=`ls -l ../examples/small|cut -f 5 -d ' '`

CtxRasterizer=`./ctx-info|grep Rasterizer|cut -f 3 -d ' '`
CtxParser=`./ctx-info|grep Parser|cut -f 3 -d ' '`
CtxState=`./ctx-info|grep CtxState|cut -f 3 -d ' '`
CtxFont=`./ctx-info|grep 'Font)'|cut -f 3 -d ' '`
CtxFontEngine=`./ctx-info|grep 'Font)'|cut -f 3 -d ' '`
Ctx=`./ctx-info|grep '(Ctx)'|cut -f 3 -d ' '`



echo "Sizes computed with static musl libc binaries, where the baseline"
echo "version contains the same raw data and c library calls."

echo "ASCII font:      " $(($FONT)) " bytes"
echo "RGBA8 Rasterizer:  " $(($TINY-$BASELINE-$FONT)) " bytes"
echo "CTX text parser: " $(($SMALL-$TINY)) " bytes"

echo $Ctx + $CtxRasterizer + $CtxFont + $CtxFontEngine + $CtxParser
RAM=$(($Ctx + $CtxRasterizer + $CtxFont + $CtxFontEngine + $CtxParser ))
#RAM=$(($CtxState + $CtxRasterizer + $CtxFont + $CtxFontEngine + $CtxParser ))

echo "RAM needed for operation with font in ROM: " $RAM " bytes"
echo "without parser " $(($RAM - $CtxParser)) " bytes"

