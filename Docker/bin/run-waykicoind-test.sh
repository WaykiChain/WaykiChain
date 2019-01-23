cd /opt/docker-instances/waykicoind-test && \
docker run --name waykicoind-test -p18920:18920 -p 6967:6968 \
       -v `pwd`/conf/WaykiChain.conf:/root/.WaykiChain/WaykiChain.conf \
       -v `pwd`/data:/root/.WaykiChain/testnet \
       -v `pwd`/bin:/opt/wicc/bin \
       -d wicc/waykicoind