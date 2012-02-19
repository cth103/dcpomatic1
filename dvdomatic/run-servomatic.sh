#!/bin/bash

export LD_LIBRARY_PATH=build/src:$LD_LIBRARY_PATH
if [ "$1" == "--debug" ]; then
    gdb --args build/tools/servomatic
elif [ "$1" == "--valgrind" ]; then
    valgrind --tool="memcheck" build/tools/servomatic
else
    build/tools/servomatic
fi
