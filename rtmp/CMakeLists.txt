cmake_minimum_required(VERSION 3.26.1)
message("--begin rtcapp complie---")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-builtins -Wno-deprecated-declarations")

set(CMAKE_C_FLAGS_DEBUG   "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# 设置目标架构为
set(CMAKE_SYSTEM_PROCESSOR "arm64")
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(libSRV_TRD_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty)
list(APPEND libSRV_INCLUDES_DIR ./)
list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/faac/deploy/include)
list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/fdk-aac/deploy/include)
list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/librtmp/deploy/include)
list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/x264/deploy/include)
list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/x265/deploy/include)

list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/faac/deploy/lib/libfaac.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/fdk-aac/deploy/lib/libfdk-aac.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/librtmp/deploy/lib/librtmp.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/x264/deploy/lib/libx264.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/x265/deploy/lib/libx265.a)

# 设置源文件路径
file(GLOB_RECURSE SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.m 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.mm 
    ${CMAKE_CURRENT_SOURCE_DIR}/live/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})

add_executable(rtmpApp ${HEADERS} ${SOURCES})
set_target_properties(rtmpApp PROPERTIES MACOSX_BUNDLE TRUE)
target_include_directories(rtmpApp PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(rtmpApp PRIVATE ${libs_trd_srv} "-framework Cocoa")
