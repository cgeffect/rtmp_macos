#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=ffmpeg-6.1.2

x264_dir="$workdir/../x264/deploy"
fdk_aac_dir="$workdir/../fdk-aac/deploy"
rtmp_dir="$workdir/../librtmp/deploy"

############ build ffmpeg ############
echo "--- build ffmpeg ${lib_type} ---"
rm -rf "$workdir/deploy" && rm -rf "$workdir/${src_dir_inside}" && \
cd "$workdir" && tar -zxvf "$workdir/source/${src_dir_inside}.tar.xz"

cd "$workdir/${src_dir_inside}" && \
./configure \
    --prefix="${workdir}/deploy/" \
    --target-os=darwin \
    --enable-ffmpeg \
    --enable-ffplay \
    --enable-ffprobe \
    --enable-doc \
    --enable-gpl \
    --enable-nonfree \
    --enable-version3 \
    --enable-static \
    --enable-pic \
    --enable-librtmp \
    --extra-cflags="-fPIC -mmacosx-version-min=14.0 -I$rtmp_dir/include" \
    --extra-ldflags="-fPIC -mmacosx-version-min=14.0 -L$rtmp_dir/lib" && \
make -j12 && make install


if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build ffmpeg"
    exit -1
fi

# x265 需要链接 stdc++ 库
echo "success!"
