#!/bin/bash

DEBUG=$1
[ -z "$DEBUG" ] && DEBUG=''

VER=$2
[ -z "$VER" ] && VER='1.1.5'

BRANCH=$3
[ -z "$BRANCH" ] && BRANCH='release'

docker build \
    --build-arg debug=$DEBUG \
    --build-arg branch=$BRANCH \
    -t wicc/waykicoind:$VER .