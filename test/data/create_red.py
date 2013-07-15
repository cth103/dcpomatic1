#!/usr/bin/python

import os

os.system('ffmpeg -f lavfi -i color=red -frames:v 16 -r 24 red_24.mp4')
os.system('ffmpeg -f lavfi -i color=red -frames:v 16 -r 30 red_30.mp4')
