#!/bin/bash

echo -ne "\e[?200h
rectangle 0 0 100% 1@
lineWidth 2
rgba 1 0 0 0.5
stroke
gray 1
linearGradient 0 0 100% 0
rectangle 0 0 100% 1@
fill
done ";
echo ""
