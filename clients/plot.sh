#!/bin/bash

# a plot using terminal cell relative coordinates

echo -ne "\e[?200h
gray 1
rectangle 0 0 40@ 10@
fill
font 'sans'
gray 0
moveTo 1@ 1@
text 'the data is in cell coords'

moveTo 1@ 1@
lineTo 1@ 9@
moveTo 1@ 5@
lineTo 39@ 5@
gray 0.5
lineWidth 0.05@
stroke

save
translate 1@ 5@
scale 1@ 8@
rgb 1 0 0
moveTo 0.0 0.0
lineTo 1.0 0.5
2.0 -0.5
3.0 0.5
4.0 -0.5
5.0 0.5
6.0 -0.5
7.0 0.5
8.0 0.0
stroke
restore
done ";
echo -en "\e[10E"
