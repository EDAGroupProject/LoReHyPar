#!/bin/bash
set -e
mkdir -p build
cd build
cmake ../
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    # ./partitioner -t ../testcase/sample01 -s ../testcase/sample01/design.fpga.out
    ./partitioner -t ../testcase/case03 -s ../testcase/case03/design.fpga.out
    if [ $? -eq 0 ]; then
        echo "Test succeeded."
    else
        echo "Test failed."
    fi
else
    echo "Build failed."
fi