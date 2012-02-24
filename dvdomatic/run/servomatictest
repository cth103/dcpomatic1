#!/bin/bash

export LD_LIBRARY_PATH=build/src:$LD_LIBRARY_PATH
if [ "$1" == "--debug" ]; then
    shift
    gdb --args build/tools/servomatictest $*
elif [ "$1" == "--valgrind" ]; then
    shift
    valgrind --tool="memcheck" build/tools/servomatictest $*
else
    build/tools/servomatictest $*
fi
