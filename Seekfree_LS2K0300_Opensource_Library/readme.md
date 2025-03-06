这个目录下使用

cmake .
// 生成makefile

make
// 编译

scp -O root@192.168.2.24:/home/root/
// 传输

进入project/out/目录
使用cmake ../../
生成makefile

使用这个指令编译
cmake --build project/out/


在CMakeLists.txt中需要修改opencv的路劲:
# 设置OpenCV的安装路径
set(CMAKE_PREFIX_PATH "/opt/ls_2k0300_env/opencv_4_10_build") \

在cross.cmake中需要修改交叉编译器的路劲:
SET(TOOLCHAIN_DIR "/opt/ls_2k0300_env/loongson-gnu-toolchain-8.3-x86_64-loongarch64-linux-gnu-rc1.6")


使用 ./build.sh 既可编译。

编译完的工程在/project/out目录下