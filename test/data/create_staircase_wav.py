#!/usr/bin/python

import wave
import struct

f = wave.open('staircase.wav', 'wb')

f.setnchannels(1)
f.setsampwidth(2)
f.setframerate(48000)

N = 4800

data = bytearray(N * 2)
p = 0
for i in range(0, N):
    data[p] = i & 255
    data[p + 1] = i >> 8
    p += 2

f.writeframes(data)

f.close()
