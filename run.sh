#!/bin/bash

mkdir -p build
cd build
cmake ../
make clean
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    # ./hypar_shell ../testcase/case01/ > case01.out
    ./hypar_shell ../testcase/sample01/ > sample01.out
    if [ $? -eq 0 ]; then
        echo "Test succeeded."
    else
        echo "Test failed."
    fi
else
    echo "Build failed."
fi