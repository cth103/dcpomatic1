#!/bin/bash

dsh -m cs2-5 -m cs2-6 -m cs2-7 -m cs2-8 \
    killall dcpomatic_server_cli
