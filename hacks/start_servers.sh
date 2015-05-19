#!/bin/bash

dsh -m cs2-5 -m cs2-6 -m cs2-7 -m cs2-8 \
    "screen -dmS dcpomatic bash -c 'cd src/dcpomatic; LD_LIBRARY_PATH=$HOME/ubuntu/lib run/dcpomatic_server_cli --verbose'"
