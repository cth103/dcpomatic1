#!/bin/bash

ffmpeg -i red_24.mp4 -i staircase.wav -acodec pcm_s16le -ac 1 staircase.mov

