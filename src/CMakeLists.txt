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
list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/include)

list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/faac/deploy/lib/libfaac.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/fdk-aac/deploy/lib/libfdk-aac.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/librtmp/deploy/lib/librtmp.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libavformat.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libavfilter.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libavcodec.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libavutil.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libpostproc.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libswresample.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libswscale.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/ffmpeg/deploy/lib/libavdevice.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/x264/deploy/lib/libx264.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/x265/deploy/lib/libx265.a)

find_library(LZMA_FRAMEWORK lzma)
find_library(LZ_FRAMEWORK z)
find_library(LBZ2_FRAMEWORK bz2)
find_library(LICONV_FRAMEWORK iconv)
set(SYS_LIBS ${LZMA_FRAMEWORK} ${LZ_FRAMEWORK} ${LBZ2_FRAMEWORK} ${LICONV_FRAMEWORK})
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
list(APPEND SYS_LIBS ${COREFOUNDATION_FRAMEWORK})
find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
list(APPEND SYS_LIBS ${COREVIDEO_FRAMEWORK})
find_library(VIDEOTOOLBOX VideoToolbox)
list(APPEND SYS_LIBS ${VIDEOTOOLBOX})
find_library(CORE_MEDIA CoreMedia)
list(APPEND SYS_LIBS ${CORE_MEDIA})
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
list(APPEND SYS_LIBS ${AUDIOTOOLBOX_FRAMEWORK})
find_library(SECURITY_FRAMEWORK Security)
list(APPEND SYS_LIBS ${SECURITY_FRAMEWORK})

file(GLOB_RECURSE utils 
    ${CMAKE_CURRENT_SOURCE_DIR}/util/*.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/util/*.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${utils})

file(GLOB_RECURSE libflv 
    ${CMAKE_CURRENT_SOURCE_DIR}/libflv/*.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/libflv/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libflv/*.c
)
file(GLOB_RECURSE libmpeg 
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpeg/*.h 
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpeg/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmpeg/*.c
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${libflv} ${libmpeg})

add_executable(send_flv_ff ${CMAKE_CURRENT_SOURCE_DIR}/send_flv_ff.cpp)
target_include_directories(send_flv_ff PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(send_flv_ff PRIVATE ${libs_trd_srv} ${SYS_LIBS})

add_executable(send_flv ${CMAKE_CURRENT_SOURCE_DIR}/send_flv.cpp)
target_include_directories(send_flv PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(send_flv PRIVATE ${libs_trd_srv})

add_executable(send_264 ${CMAKE_CURRENT_SOURCE_DIR}/send_264.cpp ${CMAKE_CURRENT_SOURCE_DIR}/util/librtmp_send264.cpp)
target_include_directories(send_264 PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(send_264 PRIVATE ${libs_trd_srv})

add_executable(recv_flv ${CMAKE_CURRENT_SOURCE_DIR}/recv_flv.cpp)
target_include_directories(recv_flv PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(recv_flv PRIVATE ${libs_trd_srv})

add_executable(recv_264_aac ${CMAKE_CURRENT_SOURCE_DIR}/recv_264_aac.cpp ${utils})
target_include_directories(recv_264_aac PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(recv_264_aac PRIVATE ${libs_trd_srv})

add_executable(flv_parse ${CMAKE_CURRENT_SOURCE_DIR}/flv_parse.cpp)
target_include_directories(flv_parse PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(flv_parse PRIVATE ${libs_trd_srv})

add_executable(avc2flv ${CMAKE_CURRENT_SOURCE_DIR}/avc2flv.cpp ${libflv})
target_include_directories(avc2flv PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(avc2flv PRIVATE ${libs_trd_srv})

add_executable(flv2avc ${CMAKE_CURRENT_SOURCE_DIR}/flv2avc.cpp ${libflv})
target_include_directories(flv2avc PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(flv2avc PRIVATE ${libs_trd_srv})

add_executable(flv2ts ${CMAKE_CURRENT_SOURCE_DIR}/flv2ts.cpp ${libflv} ${libmpeg})
target_include_directories(flv2ts PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(flv2ts PRIVATE ${libs_trd_srv})

add_executable(flv2ps ${CMAKE_CURRENT_SOURCE_DIR}/flv2ps.cpp ${libflv} ${libmpeg})
target_include_directories(flv2ps PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(flv2ps PRIVATE ${libs_trd_srv})
