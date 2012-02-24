#!/bin/bash

export LD_LIBRARY_PATH=build/src:$LD_LIBRARY_PATH
if [ "$1" == "--debug" ]; then
    gdb --args build/tools/dvdomatic $2
elif [ "$1" == "--valgrind" ]; then
    valgrind --tool="memcheck" build/tools/dvdomatic $2
else
    build/tools/dvdomatic "$1"
fi
