cmake_minimum_required(VERSION 3.4.1)

file(GLOB rtmp_src *.c)

# 只要有一个C++文件，就是属于C++ == CXX CMAKE_CXX_FLAGS
# 不修改源码的情况下，解决报错，添加宏  -D
# 下面这两种方式都可以设置宏定义
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")
add_definitions(-DNO_CRYPTO)

# CPU架构
set(CMAKE_SYSTEM_PROCESSOR "arm64;x86_64")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")

add_library(
        rtmp
        STATIC # librtmp.a
        ${rtmp_src}
)