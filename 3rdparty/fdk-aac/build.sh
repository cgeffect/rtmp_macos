#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=fdk-aac

rm -rf "$workdir/deploy" && rm -rf "$workdir/${src_dir_inside}" && \
cd "$workdir" && tar -zxvf "$workdir/source/${src_dir_inside}.tar.gz"

############ build fdk-aac ############
echo "--- build fdk-aac ---"
cd "$workdir/${src_dir_inside}/"

# v2.0.3
./autogen.sh
./configure \
    --prefix="${workdir}/deploy/" \
    --disable-frontend \
    --enable-shared=no \
    --with-pic \
    --enable-example \
    --enable-static && \
 make -j8 && make install

if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build fdk-aac"
    exit -1
fi

echo "success!"

# https://github.com/mstorsjo/fdk-aac