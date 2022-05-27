#!/bin/bash

echo
echo "Executing $1"
echo "=============================================================="
echo
echo

LD_LIBRARY_PATH=$(pwd)/bin:$LD_LIBRARY_PATH ceps \
 $1 \
 --pluginliblearn-struct-plugin.so --pe --format ansi
