#!/bin/sh
echo 'copy the libmemenv.a and libleveldb.a to leveldb'
echo 'copy autogen-coin-man.sh to top dir'

cp -f ./genbuild.sh ../share/
cp -f ./libmemenv.a ../src/leveldb/
cp -f ./libleveldb.a ../src/leveldb/
cp -f ./autogen-coin-man.sh ../
