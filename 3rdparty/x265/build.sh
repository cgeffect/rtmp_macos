#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=x265_4.1

rm -rf "$workdir/deploy" \
rm -rf "$workdir/${src_dir_inside}" && mkdir -p "$workdir/${src_dir_inside}" && cd "$workdir" && \
tar -zxvf "$workdir/source/${src_dir_inside}.tar.gz"

echo "--- build ${package} ---"
############ build ${package} ############
mkdir -p "$workdir/${src_dir_inside}/source/build" && cd "$workdir/${src_dir_inside}/source/build" && \
cmake -DENABLE_ALPHA=ON -DENABLE_SHARED=OFF -DCMAKE_SYSTEM_PROCESSOR=arm64 -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX="$workdir/deploy" .. && \
make -j8 && make install

if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build ${package}"
    exit -1
fi

echo "success!"
# 源码
# https://bitbucket.org/multicoreware/x265_git
# 发布库
# http://ftp.videolan.org/pub/videolan/x265/

# x265教程
# https://github.com/iAvoe/x264-x265-QAAC-ffprobe-Ultimatetutorial/blob/master/%E6%95%99%E7%A8%8B.md
