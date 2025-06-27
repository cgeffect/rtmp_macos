#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

tar xjf "$workdir/source/mp4v2-2.0.0.tar.bz2"
cd "$workdir/mp4v2-2.0.0/"
./configure --prefix=${workdir}/deploy/
make -j12
make install

if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build libmp4v2"
    exit -1
fi

echo "success!"

