#!/bin/sh

ASCII_FONT=`ls -l ../../fonts/ctxf/ascii.ctxf|cut -f 5 -d ' '`
REGULAR_FONT=`ls -l ../../fonts/ctxf/regular.ctxf|cut -f 5 -d ' '`
BASELINE=`ls -l ../test-size/musl-static-baseline|cut -f 5 -d ' '`
TINY=`ls -l ../test-size/musl-static-tiny|cut -f 5 -d ' '`
SMALL=`ls -l ../test-size/musl-static-small|cut -f 5 -d ' '`
MEDIUM=`ls -l ../test-size/musl-static-medium|cut -f 5 -d ' '`

echo "ASCII+some latin:" $(($REGULAR_FONT)) " bytes"
echo "ASCII font:      " $(($ASCII_FONT)) " bytes"

echo "Sizes computed with static musl libc binaries - in this case"
echo "the numbers also include used implementations from the few libc"
echo "functions in use - making it realistic in terms of firmware impact."
echo ""
echo "RGBA8 Rasterizer:" $(($TINY-$BASELINE-$ASCII_FONT)) " bytes"
echo "CTX parser:      " $(($SMALL-$TINY)) " bytes"
echo "CTX formatter: : " $(($MEDIUM-$TINY)) " bytes"


BASELINE=`ls -l ../test-size/baseline|cut -f 5 -d ' '`
TINY=`ls -l ../test-size/tiny|cut -f 5 -d ' '`
SMALL=`ls -l ../test-size/small|cut -f 5 -d ' '`
MEDIUM=`ls -l ../test-size/medium|cut -f 5 -d ' '`

CtxRasterizer=`./ctx-info-32bit|grep Rasterizer|cut -f 3 -d ' '`
CtxParser=`./ctx-info-32bit|grep Parser|cut -f 3 -d ' '`
CtxState=`./ctx-info-32bit|grep CtxState|cut -f 3 -d ' '`
CtxFont=`./ctx-info-32bit|grep 'Font)'|cut -f 3 -d ' '`
CtxFontEngine=`./ctx-info-32bit|grep 'Font)'|cut -f 3 -d ' '`
Ctx=`./ctx-info-32bit|grep '(Ctx)'|cut -f 3 -d ' '`
echo ""
echo "Sizes computed with 32bit (i486 build) dynamic libc binaries,"
echo "version contains the same raw data and c library calls."

echo "RGBA8 Rasterizer:" $(($TINY-$BASELINE-$ASCII_FONT)) " bytes"
echo "CTX parser:      " $(($SMALL-$TINY)) " bytes"
echo "CTX formatter: : " $(($MEDIUM-$TINY)) " bytes"

echo Ctx + CtxRasterizer + CtxFont + CtxFontEngine + CtxParser
echo $Ctx + $CtxRasterizer + $CtxFont + $CtxFontEngine + $CtxParser
RAM=$(($Ctx + $CtxRasterizer + $CtxFont + $CtxFontEngine + $CtxParser))

echo "RAM needed for operation on 32bit: "
echo "  only rasterizer:   " $(($RAM - $CtxParser)) " bytes"
echo "  rasterizer+parser: " $RAM " bytes"

