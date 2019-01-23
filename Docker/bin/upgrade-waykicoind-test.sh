#!/bin/bash

docker pull wicc/waykicoind
docker stop waykicoind-test && docker rm waykicoind-test
cd /opt/docker-instances/waykicoind-test && sh bin/run-waykicoind-test.sh