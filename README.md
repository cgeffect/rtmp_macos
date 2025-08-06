
# RTMP 知识点记录

主要记录rtmp相关的一些知识点，包括收流推流、解析与封装

## 工程结构
- **rtmp**: 使用Objective-C++编写的rtmp库
- **src**: 所有的文件都在src目录下
  - demux_mp4.cpp: 解复用mp4文件
  - flv_parse.cpp: 解析flv文件
  - flv2avc-aac-manual.cpp: 手动解析flv文件，提取avc和aac数据
  - flv2avc-aac.cpp: 使用libflv解析flv文件，提取avc和aac数据
  - flv2ps.cpp: 解析flv文件，提取avc和aac数据，生成ps文件
  - flv2ts.cpp: 解析flv文件，提取avc和aac数据，生成ts文件
  - h264_to_fmp4.cpp: 把h264文件转换为fmp4文件
  - parse_ps.cpp: 解析ps文件
  - parse_ts.cpp: 解析ts文件
  - parse-ts.cpp: 解析ts文件，提取avc和aac数据
  - recv_264_aac.cpp: 使用librtmp接收flv文件的avc和aac数据
  - recv_flv.cpp: 使用librtmp接收flv文件
  - send_264.cpp: 使用librtmp发送h264文件
  - send_aac.cpp: 使用librtmp发送aac文件
  - send_flv_ff.cpp: 使用ffmpeg发送flv文件到服务器
  - send_flv.cpp: 使用librtmp发送flv文件到服务器

## 使用说明
1. 由于调试使用的是Xcode，Xcode和CMake的路径访问方式不同，所以工程中文件的路径都写成了绝对路径，需要根据自己的情况修改
2. 和流媒体服务器相关的案例需要搭建一个流媒体服务器，可以使用srs或者ZLMediaKit，可以在本地搭建，也可以在Linux服务器上搭建使用
3. 流媒体服务器搭建完成后，需要修改代码中的服务器地址和端口
4. 运行项目，可以在流媒体服务器上查看推流情况
5. 可以使用ffplay或者vlc等播放器查看推流情况

## 排查错误方法
- 可以使用hexdump查看二进制数据，方便排查错误：
  ```bash
  hexdump -C ../res/wubai.aac | head -20
  ```
- 可以使用ffplay或者vlc等播放器查看推流情况

## 运行环境
1. 操作系统: macOS 14.3.1, 架构: arm64, 芯片: Apple M1
2. 可执行程序和3rdparty下的库都是支持arm64架构, 需要使用arm64架构的macOS电脑运行


