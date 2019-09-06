#!/bin/bash

docker pull wicc/waykicoind
docker stop waykicoind-main && docker rm waykicoind-main
cd /opt/docker-instances/waykicoind-main && sh bin/run-waykicoind-main.sh