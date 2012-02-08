#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:build/src
if [ "$1" == "--debug" ]; then
    gdb --args build/test/unit-tests
else
    build/test/unit-tests
fi

    
