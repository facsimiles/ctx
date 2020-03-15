#!/bin/bash

# "\e_Ar=8000,b=8,f=u,c=1;\e\\"



echo -e "\e[?4444h"  # start of u-law pcm marker
 sox "$1" -t raw -r 8000 -c 1 -e u-law -b 8 - |  tr "\000" "\001" |  pv -q -L8000
 #ffmpeg -i "$1" -ac 1 -ar 8000 -acodec pcm_mulaw -f au -  | tr "\000" "\001" |  tail -b 20000-50000 | pv -q -L8000
echo -e "\000" # end marker
# ffmpeg -i "$1" -ac 1 -ar 8000 -acodec pcm_mulaw -f au -   |  tr "\000" "\001" > a.au
 #ffmpeg -i "$1" -ac 1 -ar 8000 -acodec pcm_mulaw -f au -  2>/dev/null

