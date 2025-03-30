#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=rtmpdump-2.3

rm -rf "$workdir/deploy" && rm -rf "$workdir/${src_dir_inside}" && \
cd "$workdir" && tar -zxvf "$workdir/source/${src_dir_inside}.tar"

# 把CMakeList.txt文件拷贝到librtmp文件, 因为2010年还没流行CMakeList.txt, 还是是Makefile
cp "$workdir/source/CMakeLists.txt" "$workdir/rtmpdump-2.3/librtmp/"
mkdir "$workdir/rtmpdump-2.3/librtmp/build" && cd "$workdir/rtmpdump-2.3/librtmp/build"
cmake .. && make

if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build fdk-aac"
    exit -1
fi

echo "success!"

# http://rtmpdump.mplayerhq.hu/download/