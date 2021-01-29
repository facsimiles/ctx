#!/bin/bash

# a plot using terminal cell relative coordinates

echo -ne "\e[?200h
rgba 1 1 1 0.9
rectangle 0 0 80@ 10@
fill

moveTo 1@ 1@
lineTo 1@ 9@
moveTo 1@ 5@
lineTo 79@ 5@
gray 0.5
lineWidth 0.1@
stroke

save
translate 1@ 5@
scale 1@ 8@
rgb 1 0 0
moveTo 0.0 0.0
L 1.0 0.5 2.0 -0.5 3.0 0.5 4.0 -0.5 5.0 0.5 6.0 -0.5 7.0 0.5 8.0 -0.25 9.0 0.125 10.0 -0.06125 11.0 0
restore
stroke
done ";
echo -en "\e[10E"  # advance cursor to below drawn canvas
