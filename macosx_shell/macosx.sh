#!/bin/bash

echo copy the libmemenv.a and libleveldb.a to leveldb/
echo copy autogen-coin-man.sh to topdir
cp -rf ./autogen-coin-man.sh ../
cp -rf ./libmemenv.a ../src/leveldb
cp -rf ./libleveldb.a ../src/leveldb
cp -rf ./makefile_liblua ../src/
cp -rf ./liblua53.a ../src/
