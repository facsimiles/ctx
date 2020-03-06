#!/bin/bash
cx=500;cy=311;radius=300; 
echo -ne "\e[2J\e" # clear screen
for a in b{1..600}; do
hour_pos=`bc <<<"scale=3;($(date +%H|sed 's/^0//')/12.0+0.75)*3.14152*2"`
minute_pos=`bc <<<"scale=3;($(date +%M|sed 's/^0//')/60.0+0.75)*3.14152*2"`
second_pos=`bc <<<"scale=3;($(date +%S|sed 's/^0//')/60.0+0.75)*3.14152*2"`
echo -ne "\e[H\e[?7020h     # go home, enter vector mode on current line
clear                       # clear prior vector contents of line
arc($cx, $cy, $radius, 0.0, 6.4, 0);    # (,){} and ; are treated as white space
move_to $cx $cy             # commands/functions know their argument count
arc $cx $cy $(($radius * 80 / 100)) $minute_pos $minute_pos 0
M $cx $cy                   # where matching, the SVG d short-names can be used
arc $cx $cy $(($radius * 66 / 200)) $hour_pos $hour_pos 0
set_line_cap(ROUND); set_rgba(1,1,1,0.5); set_line_width($((radius/10)));
stroke  
move_to $cx $cy
arc $cx $cy $((radius * 80 / 100))  $second_pos $second_pos 0
set_rgba 1 0.1 0.1 1 ; set_line_width $((radius/40))
stroke
set_font_size 35
m 50,50
text('fnord');
m 90,100
x 'fnord'
done "; date +%H:%M:%S ; echo $hour_pos $minute_pos $second_pos; sleep 1; done
