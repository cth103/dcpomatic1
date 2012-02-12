#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:build/src
if [ "$1" == "--debug" ]; then
    gdb --args build/tools/film_editor $2
elif [ "$1" == "--valgrind" ]; then
    valgrind --tool="memcheck" build/tools/film_editor $2
else
    build/tools/film_editor "$1"
fi
