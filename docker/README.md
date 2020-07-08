# Version
* v3.0.0

# Mainnet Upgrade Date
* 2020-07-16

# Run in docker
Run waykichain coind inside a docker container!

## Install Dependencies
  * Docker 17.05 or higher is required
## Docker Environment Requirement
  * At least 4GB RAM (Docker -> Preferences -> Advanced -> Memory -> 4GB or above)
  * If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build waykicoind docker image
### method-1: build from Dockerfile
1. ```git clone https://github.com/WaykiChain/WaykiChain.git```
1. ```cd WaykiChain/Docker && sh ./bin/build-waykicoind.sh```

### method-2: pull from Docker Hub without build
``` docker pull wicc/waykicoind ```

## Run WaykiChain Docker container
1. create a host dir to keep container data (you are free to choose your own preferred dir path)
   * For mainnet: ``` sudo mkdir -p /opt/docker-instances/waykicoind-main ```
   * For testnet: ``` sudo mkdir -p /opt/docker-instances/waykicoind-test ```
1. first, cd into the above created node host dir and create ```data``` and ```conf``` subdirs:
   * ``` sudo mkdir data conf ```
1. copy the entire docker/bin dir from WaykiChain repository:
   * ``` sudo cp -r ${your_path_of_WaykiChain}/docker/bin ./ ```
1. copy WaykiCoind.conf into ```conf``` dir from WaykiChain repository:
   * ``` sudo cp -r ${your_path_of_WaykiChain}/docker/WaykiChain.conf ./conf/ ```
1. modify content of ```WaykiCoind.conf``` accordingly
   * For mainnet, please make sure ```nettype=main``` is set
   * For testnet, please make sure only ```nettype=test``` is set
   * For regtest, please make suer only ```nettype=regtest``` is set
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
|How to run a testnet | modify WaykiChain.conf by adding ```testnet=test```,  |
|How to run a regtest | modify WaykiChain.conf by adding ```regtest=regtest```, |
|How to run a mainnet | modify WaykiChain.conf by adding ```regtest=main```,  |
