
#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

############ build x264 ############
echo "--- build x264 ---"
cd "$workdir/source/x264/" && \
./configure \
    --extra-cflags=-fpic \
    --prefix="${workdir}/deploy/" \
    --target-os=darwin \
    --enable-static \
    --disable-opencl \
    --enable-pic && \
make -j 12 && \
make install
if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build x264"
    exit -1
fi

echo "success!"

