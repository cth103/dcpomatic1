#!/bin/bash

export LD_LIBRARY_PATH=build/src:$LD_LIBRARY_PATH
if [ "$1" == "--debug" ]; then
    gdb --args build/test/unit-tests
elif [ "$1" == "--valgrind" ]; then
    valgrind --tool="memcheck" --leak-check=full build/test/unit-tests
else
    build/test/unit-tests
fi

    
