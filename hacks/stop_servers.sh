#!/bin/bash

dsh -m cs2-5 -m cs2-6 -m cs2-7 -m cs2-8 -m cs2-17 -m cs2-18 -m cs2-19 -m cs2-20 \
    killall dcpomatic_server_cli
