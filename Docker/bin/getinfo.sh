#!/bin/bash
#
# Usage: sh getinfo.sh [$CON_NAME] [$RPC_CMD]
#

CON_NAME='waykicoind-test'
RPC_CMD='getinfo'

if   [[ $# -eq 3 ]]; then
    CON_NAME=$1
    RPC_CMD="$2 $3"

elif [[ $# -eq 2 ]]; then
    CON_NAME=$1
    RPC_CMD=$2

elif [[ $# -eq 1 ]]; then
  if [[ $1 == "waykicoind"* ]]; then
    CON_NAME=$1
  else
    RPC_CMD=$1
  fi
fi

echo "execute RPC command: 'coind $RPC_CMD' on container[$CON_NAME] ..."
docker exec -it $CON_NAME sh -c "coind $RPC_CMD"