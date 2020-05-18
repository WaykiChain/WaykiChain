#!/bin/bash

DEBUG=$1
[ -z "$DEBUG" ] && DEBUG=''

VER=$2
[ -z "$VER" ] && VER='1.3'

BRANCH=$3
[ -z "$BRANCH" ] && BRANCH='release'

echo "Setting SOURCE_BRANCH to ${BRANCH}"
sed -i "s@^ENV SOURCE_BRANCH \"release\"@ENV SOURCE_BRANCH \"${BRANCH}\"@g" Dockerfile

docker build \
    --build-arg debug=$DEBUG \
    --build-arg branch=$BRANCH \
    -t wicc/waykicoind:$VER .