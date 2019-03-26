#!/bin/sh
echo 'copy autogen-coin-man.sh to top dir'
cp -f ./genbuild.sh ../share/
cp -f ./autogen-coin-man.sh ../

chmod +x ../src/leveldb/build_detect_platform
