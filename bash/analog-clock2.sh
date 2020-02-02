#!/bin/bash
cx=500;cy=311;radius=200; 
echo -ne "\e[2J\e[H"  # clear and home
for a in b{1..600}; do
	i=$(($i+1))
hour_pos=`bc <<<"scale=3;($(date +%H|sed 's/^0//')/12.0+0.75)*3.14152*2"`
minute_pos=`bc <<<"scale=3;($(date +%M|sed 's/^0//')/60.0+0.75)*3.14152*2"`
second_pos=`bc <<<"scale=3;($(date +%S|sed 's/^0//')/60.0+0.75)*3.14152*2"`
echo -n "
clear
clear
set_rgba 1 1 1 0.5
arc $cx $cy $radius 0.0 6.4 0
set_line_width $((radius/10))
move_to $cx $cy
arc $cx $cy $(($radius * 80 / 100)) $minute_pos $minute_pos 0
move_to $cx $cy
arc $cx $cy $(($radius * 66 / 200)) $hour_pos $hour_pos 0
set_line_cap 1
stroke
set_line_width $((radius/40))
move_to $cx $cy
arc $cx $cy $((radius * 80 / 100))  $second_pos $second_pos 0
set_rgba 1 0.1 0.1 1
stroke
done
" |  ../ctx parse ; sleep 0.25s; done

