#!/bin/bash

DEBUG=$1
[ -z "$DEBUG" ] && DEBUG=''

VER=$2
[ -z "$VER" ] && VER='1.3'

BRANCH=$3
[ -z "$BRANCH" ] && BRANCH='release'

echo "Setting SOURCE_BRANCH to ${BRANCH}"
sed -i '' "s@^ENV SOURCE_BRANCH \"release\"@ENV SOURCE_BRANCH \"${BRANCH}\"@g" Dockerfile

COMMIT=$(curl -s 'https://api.github.com/repos/WaykiChain/WaykiChain/commits' | grep sha | head -1 | cut -c 13-20)
echo "Setting SOURCE_COMMIT: ${COMMIT}"
sed -i '' "s@SOURCE_COMMIT@${COMMIT}@g" Dockerfile

docker build \
    --build-arg debug=$DEBUG \
    --build-arg branch=$BRANCH \
    -t wicc/waykicoind:$VER .