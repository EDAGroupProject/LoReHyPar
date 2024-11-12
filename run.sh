#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <case>"
    exit 1
fi

CASE=$1

set -e
mkdir -p build
cd build
cmake ../
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    ./partitioner -t "../testcase/${CASE}" -s "../testcase/${CASE}/design.fpga.out" > output.log
    if [ $? -eq 0 ]; then
        echo "Test succeeded."
    else
        echo "Test failed."
    fi
else
    echo "Build failed."
fi

cd ..

./evaluator/evaluator -t "testcase/${CASE}" -s "testcase/${CASE}/design.fpga.out"