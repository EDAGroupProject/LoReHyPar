#!/bin/bash
set -e
mkdir -p build
cd build
cmake ../
# make clean
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    ./test > test.log
    if [ $? -eq 0 ]; then
        echo "Test succeeded."
    else
        echo "Test failed."
    fi
else
    echo "Build failed."
fi