#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=faac-1.29.9.2

rm -rf "$workdir/deploy" && rm -rf "$workdir/${src_dir_inside}" && \
cd "$workdir" && tar -zxvf "$workdir/source/${src_dir_inside}.tar.gz"

############ build faac ############
echo "--- build faac ---"
cd "$workdir/${src_dir_inside}/" && \

./configure \
    --prefix="${workdir}/deploy/" \
    --with-pic \
    --enable-shared=no && \
make -j8 && make install

if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build libvpx"
    exit -1
fi

echo "success!"
