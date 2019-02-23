# Version
* 2019-02-21
* v1.2.5

# Run in docker
Run waykichain coind inside a docker container!

## Install Depdendencies
  * Docker 17.05 or higher is required
## Docker Environment Requirement
  * At least 4GB RAM (Docker -> Preferrences -> Advanced -> Memory -> 4GB or above)
  * If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build waykicoind docker image
### method-1: build from Dockerfile
1. ```git clone https://github.com/WaykiChain/docker-waykicoind.git```
1. ```cd docker-waykicoind && sh ./bin/build-waykicoind.sh```

### method-2: pull from Docker Hub without build
``` docker pull wicc/waykicoind ```

## Run WaykiChain Docker container
1. create a host dir to keep container data (you are free to choose your own preferred dir path)
   * For mainnet: ``` sudo mkdir -p /opt/docker-instances/waykicoind-main ```
   * For testnet: ``` sudo mkdir -p /opt/docker-instances/waykicoind-test ```
1. first, cd into the above created node host dir and create ```data``` and ```conf``` subdirs:
   * ``` sudo mkdir data conf ```
1. copy the entire bin dir from docker-waykicoind:
   * ``` sudo cp -r docker-waykicoind/bin . ```
1. copy WaykiCoind.conf into ```conf``` dir and modify its content accordingly
   * For mainnet, please make sure ```testnet=1``` is removed
   * For testnet, please make sure only ```testnet=1``` is provided
   * For regtest, please make suer only ```regtest=1``` is provided
   * For common nodes (no mining), please set ```gen=0``` to avoid computing resources waste
1. launch the node container:
   * For mainnet, run ```$sh ./bin/run-waykicoind-main.sh```
   * For testnet,  run ```$sh ./bin/run-waykicoind-test.sh```

## Lookup Help menu from coind
* ```docker exec -it waykicoind-test coind help```

## Stop coind (in a graceful way)
* ```docker exec -it waykicoind-test coind stop```

## Test
* ```$docker exec -it waykicoind-test coind getpeerinfo```
* ```$docker exec -it waykicoind-test coind getinfo```

## Q&A

|Q | A|
|--|--|
|How to modify JSON RPC port | Two options: <br> <li>modify [WaykiChain.conf](https://github.com/WaykiChain/WaykiChain/wiki/WaykiChain.conf) (```rpcport=6968```)<li>modify docker container mapping port |
|How to run a testnet | modify WaykiChain.conf by adding ```testnet=1```, otherwise it will run as mainnet |
|How to run a regtest | modify WaykiChain.conf by adding ```regtest=1```, otherwise it will run as mainnet |