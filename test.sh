#!/bin/bash

# 检查是否没有参数传入
if [ $# -ne 0 ]; then
    echo "Usage: $0 (no arguments needed)"
    exit 1
fi

# 开始构建过程
set -e  # 如果任何命令失败，脚本立即退出

# 创建构建目录并编译项目
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

cd ..  # 返回到项目根目录

# 清空输出日志文件
> ./build/output.log

# 遍历测试案例 case01 到 case04
for CASE in case01 case02 case03 case04; do
    echo "Running test for $CASE..." >> ./build/output.log

    # 记录 partitioner 的执行时间并将其输出到 output.log
    { time ./build/partitioner -t "testcase/${CASE}" -s "testcase/${CASE}/design.fpga.out"; } 2>&1 | \
    tee -a ./build/output.log | \
    grep real | \
    sed "s/^/Partitioner time for $CASE: /" >> ./build/output.log

    if [ $? -eq 0 ]; then
        echo "Partitioner test for $CASE succeeded." >> ./build/output.log
        echo "Running evaluator for $CASE..." >> ./build/output.log

        # 运行 evaluator 并将输出附加到 output.log
        ./evaluator/evaluator -t "testcase/${CASE}" -s "testcase/${CASE}/design.fpga.out" >> ./build/output.log
    else
        echo "Partitioner test for $CASE failed." >> ./build/output.log
    fi
done

echo "All tests completed."
