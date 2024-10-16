#!/bin/bash
set -e
mkdir -p build
cd build
cmake ../
make clean
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    # ./hypar_shell -s ../testcase/sample01 -t ../testcase/sample01/design.fpga.out
    ./hypar_shell -s ../testcase/case01 -t ../testcase/case01/design.fpga.out
    if [ $? -eq 0 ]; then
        echo "Test succeeded."
    else
        echo "Test failed."
    fi
else
    echo "Build failed."
fi