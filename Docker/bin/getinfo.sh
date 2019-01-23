#!/bin/bash

# Usage: sh getinfo.sh $PARAM

PARAM=$1
[ -z "$PARAM" ] && PARAM=getinfo
docker exec -it waykicoind-test coind $PARAM