#!/bin/bash 

cd ../out

# 检查 cd 命令是否执行成功
if [ $? -ne 0 ]; then
    echo "无法进入 ./project/out 目录，请检查目录是否存在。"
    exit 1
fi

rm -rf *
# 检查 cd 命令是否执行成功
if [ $? -ne 0 ]; then
    echo "无法进入 ./project/out 目录，请检查目录是否存在。"
    exit 1
fi

cmake ../user
# 检查 cmake 命令是否执行成功
if [ $? -ne 0 ]; then
    echo "cmake 命令执行失败。"
    exit 1
fi

echo "cmake 命令执行成功。"
make -j12

echo "生成APP"
# cmake --build .

scp -O SEEKFREE_APP root@192.168.2.57:/home/root/
# echo "传输"

