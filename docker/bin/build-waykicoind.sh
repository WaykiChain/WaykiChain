#!/bin/bash

DEBUG=$1
[ -z "$DEBUG" ] && DEBUG=''

VER=$2
[ -z "$VER" ] && VER='3.0'

BRANCH=$3
[ -z "$BRANCH" ] && BRANCH='rel/v3.0.0'

echo "Setting SOURCE_BRANCH to ${BRANCH}"
sed -i "s@^ENV SOURCE_BRANCH \"release\"@ENV SOURCE_BRANCH \"${BRANCH}\"@g" Dockerfile

docker build \
    --build-arg debug=$DEBUG \
    -t wicc/waykicoind:$VER .
