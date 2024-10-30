#!/bin/bash
set -e

# 创建或进入build目录
mkdir -p build
cd build

# 只在没有生成文件时运行cmake
if [ ! -f "Makefile" ]; then
    cmake ../
fi

# 编译项目，不清理之前的编译文件
make

if [ $? -eq 0 ]; then
    echo "Build succeeded."
    # 运行测试，使用 gdb 并在失败时打印回溯
    gdb -q -ex "run" -ex "bt" -ex "quit" --args ./test
    if [ $? -eq 0 ]; then
        echo "Test succeeded."
    else
        echo "Test failed."
    fi
else
    echo "Build failed."
fi
