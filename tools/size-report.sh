#!/bin/sh

FONT=`ls -l ../fonts/ctxf/ascii.ctxf|cut -f 5 -d ' '`
BASELINE=`ls -l ../examples/musl-static-baseline|cut -f 5 -d ' '`
TINY=`ls -l ../examples/musl-static-tiny|cut -f 5 -d ' '`
SMALL=`ls -l ../examples/musl-static-small|cut -f 5 -d ' '`

CtxRenderer=`./ctx-info|grep Renderer|cut -f 3 -d ' '`
CtxState=`./ctx-info|grep State|cut -f 3 -d ' '`
Ctx=`./ctx-info|grep '(Ctx)'|cut -f 3 -d ' '`

echo "Sizes computed with static musl libc binaries, where the baseline"
echo "version contains the same raw data and c library calls."

echo "ASCII font:      " $(($FONT)) " bytes"
echo "RGBA8 Renderer:  " $(($TINY-$BASELINE-$FONT)) " bytes"
echo "CTX text parser: " $(($SMALL-$TINY)) " bytes"

echo "Fixed RAM needed for direct rendering: " $(($Ctx + $CtxRenderer)) " bytes"


FONT=`ls -l ../fonts/ctxf/ascii.ctxf|cut -f 5 -d ' '`
BASELINE=`ls -l ../examples/baseline|cut -f 5 -d ' '`
TINY=`ls -l ../examples/tiny|cut -f 5 -d ' '`
SMALL=`ls -l ../examples/small|cut -f 5 -d ' '`

CtxRenderer=`./ctx-info|grep Renderer|cut -f 3 -d ' '`
CtxState=`./ctx-info|grep State|cut -f 3 -d ' '`
Ctx=`./ctx-info|grep '(Ctx)'|cut -f 3 -d ' '`

echo "Sizes computed with static musl libc binaries, where the baseline"
echo "version contains the same raw data and c library calls."

echo "ASCII font:      " $(($FONT)) " bytes"
echo "RGBA8 Renderer:  " $(($TINY-$BASELINE-$FONT)) " bytes"
echo "CTX text parser: " $(($SMALL-$TINY)) " bytes"

echo "Fixed RAM needed for direct rendering: " $(($Ctx + $CtxRenderer)) " bytes"

