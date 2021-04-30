#!/bin/bash

# Make plot using terminal cell relative coordinates

echo -ne "\e[?200h
rgba 1 1 1 0.9
rectangle 0 0 80@ 10@
fill

lineWidth 0.1@
moveTo 1@ 1@
lineTo 1@ 9@
moveTo 1@ 5@
lineTo 79@ 5@
grayS 0.25
stroke

save
translate 1@ 5@
scale 1@ 8@
moveTo
0.0 0.0
lineTo 1 0.5
       3 -0.5
       4 0.5
       5 -0.5
       6 0.5
       7 -0.5
       8 0.5
       9 -0.5
       10 0.25
       11 -0.25
       12  0.25
       13 -0.125
       14  0.125
       16  0.0
restore
lineWidth 0.1@
rgbS 1 0 0
stroke
done ";
echo -en "\e[10E"  # advance cursor to below drawn canvas
