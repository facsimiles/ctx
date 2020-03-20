#!/bin/bash
ffmpeg -i "$1" -ac 1 -ar 8000 -acodec pcm_mulaw -f au - | ./atty speaker

