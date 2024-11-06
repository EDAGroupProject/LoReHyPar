#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <case>"
    exit 1
fi

CASE=$1

INPUT_DIR="testcase/${CASE}"
OUTPUT_FILE="testcase/${CASE}/design.fpga.out"

./evaluator/evaluator -t "$INPUT_DIR" -s "$OUTPUT_FILE"
