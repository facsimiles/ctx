#!/bin/bash
echo -ne "\e[?1049h" # alt-screen on
echo -ne "\e[?25l"   # text-cursor off
function cleanup {
  echo -ne " X "        # leave ctx mode; in case we're there
  echo -ne "\e[?1049l"  # alt-screen off
  echo -ne "\e[?25h"    # text-cursor on
}
trap cleanup EXIT # restore terminal state on ctrl+c and regular exit

for a in b{1..1000}; do
hour_radians=`bc <<<"scale=3;(($(date +%H|sed 's/^0//')+($(date +%M|sed s'/^6//')/60.0))/12.0+0.75)*3.14152*2"`
minute_radians=`bc <<<"scale=3;($(date +%M|sed 's/^0//')/60.0+0.75)*3.14152*2"`
second_radians=`bc <<<"scale=3;($(date +%S|sed 's/^0//')/60.0+0.75)*3.14152*2"`
radius=45
echo -ne "\e[2J\e[H"
echo -ne "\e[?200h
rgba 1 1 1 0.5
arc 50% 50% $radius% 0.0 6.4 0
lineWidth $((radius/10))%
stroke
lineCap round
moveTo 50% 50%
arc 50% 50% $(($radius * 70 / 100))% $minute_radians $minute_radians 0
moveTo 50% 50%
arc 50% 50% $(($radius * 50 / 100))% $hour_radians $hour_radians 0
stroke
lineWidth $((radius/40))%
moveTo 50% 50%
arc 50% 50% $((radius * 90 / 100))%  $second_radians $second_radians 0
rgba 1 0.1 0.1 1
stroke
done "; sleep 0.25; done
