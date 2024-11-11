#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 <case> [-a <iterations>]"
    exit 1
fi

CASE=$1
ITERATIONS=1  # 默认运行一次

# 解析参数
if [ "$2" == "-a" ] && [ -n "$3" ]; then
    ITERATIONS=$3
fi

set -e
mkdir -p build
cd build
cmake ../
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
else
    echo "Build failed."
    exit 1
fi

cd ..

# 初始化变量存储 Hop Length 总和
total_hop_length=0

# 清空 output.log 文件，以便每次执行时从空文件开始记录
> ./build/output.log

for (( i=1; i<=ITERATIONS; i++ ))
do
    ./build/partitioner -t "testcase/${CASE}" -s "testcase/${CASE}/design.fpga.out" > ./build/outrubbish.log
    if [ $? -eq 0 ]; then
        echo "Test succeeded for iteration $i."
    else
        echo "Test failed for iteration $i."
        exit 1
    fi

    # 运行 evaluator，将输出追加到 output.log 文件
    echo "Iteration $i:" >> ./build/output.log
    ./evaluator/evaluator -t "testcase/${CASE}" -s "testcase/${CASE}/design.fpga.out" | tee -a ./build/output.log

    # 提取 Total Hop Length 并累加，仅获取最后一行的 Hop Length 值
    hop_length=$(grep "Total Hop Length =" ./build/output.log | tail -n 1 | awk '{print $NF}')
    total_hop_length=$((total_hop_length + hop_length))
done

# 计算平均值
average_hop_length=$(echo "scale=2; $total_hop_length / $ITERATIONS" | bc)
echo "Average Total Hop Length = $average_hop_length" | tee -a ./build/output.log
