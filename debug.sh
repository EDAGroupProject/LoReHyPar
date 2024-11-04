#!/bin/bash
set -e
mkdir -p build
cd build
cmake ../
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    gdb -q -ex "run -t ../testcase/case03 -s ../testcase/case03/design.fpga.out" -ex "bt" -ex "quit" --args ./partitioner
    if [ $? -eq 0 ]; then
        echo "Test succeeded."
    else
        echo "Test failed."
    fi
else
    echo "Build failed."
fi