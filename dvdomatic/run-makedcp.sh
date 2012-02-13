#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:build/src
if [ "$1" == "--debug" ]; then
    gdb --args build/tools/makedcp $2
elif [ "$1" == "--valgrind" ]; then
    valgrind --tool="memcheck" build/tools/makedcp $2
else
    build/tools/makedcp $*
fi
