## 1. Publish a smart contract
```
root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur deploycontracttx WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH  lotteryV3.lua 110000000
{
    "txid" : "c2cb6f9ef2d72e77e1cdeb12dff74ece62c576f88e26e8bbd82a671656a77bb9"
}
```

## 2. Check smart contract

* 根据上一步得到的hash， 可以查询出部署好的智能合约信息：
```
root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur getcontractregid c2cb6f9ef2d72e77e1cdeb12dff74ece62c576f88e26e8bbd82a671656a77bb9
{
    "regid:" : "182808-1",
    "script" : "18ca02000100"
}
```

* 使用getaccountinfo 命令 根据 智能合约regid查询智能合约详情：
```
root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur getaccountinfo 182808-1
{
    "Address" : "WUJvWstbPM5VjMTcSj66N7mVtK49HEVNnE",
    "KeyID" : "3dd2b53fd5966a9d6638811797afb5e6a933e91b",
    "RegID" : "182808-1",
    "PublicKey" : "",
    "MinerPKey" : "",
    "Balance" : 0,
    "Votes" : 0,
    "UpdateHeight" : 0,
    "voteFundList" : [
    ],
    "position" : "inblock"
}
```

### 3. check assert:

* 使用getcontractaccountinfo 命令， 根据智能合约regid 和 账号地址 查询账号地址对于的该智能合约余额
```root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur getcontractaccountinfo 182808-1 WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH
{
    "mAccUserID" : "57695a7836727273426e3973486a7770766477744d4e4e58326f3331733344454848",
    "FreeValues" : 0,
    "FrozenFunds" : [
    ]
}
```
### 4. publish assert:

* 使用callcontracttx 命令发行资产， 具体字符串字段需要根据每个智能合约的定义来组装。
```root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur callcontracttx 18247-1 182808-1 0 f00657695a7836727273426e3973486a7770766477744d4e4e58326f3331733344454848000052acdfb2241d000052acdfb2241d0100000001000000 100000 0
{
    "txid" : "6f0033b27c0c531d33f61fdae125f6c265b0d7daacd1990f56b25b8a453ef869"
}
```
### 5. transfer assert:

* 使用callcontracttx 命令发行资产，具体字符串字段需要根据每个智能合约的定义来组装。
```root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur callcontracttx 18247-1 182808-1 0 f0075755774a6e314346574b4d64626d6638394472436e5032776b4336414b72454268480080c6a47e8d0300 100000 0
{
    "txid" : "93a89951ef71645f9910f2cb5c562389468416cb3921d69f931f904c6e653034"
}
```

### 6. check assert again:

* 再次检查资产，可以判定转账是否成功
```root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur getcontractaccountinfo 182808-1 WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH
{
    "mAccUserID" : "57695a7836727273426e3973486a7770766477744d4e4e58326f3331733344454848",
    "FreeValues" : 2099000000000000000,
    "FrozenFunds" : [
    ]
}
```

```
root@APP-NODE-MASTER:~/Deploy# ./coind -datadir=cur getcontractaccountinfo 182808-1 WUwJn1CFWKMdbmf89DrCnP2wkC6AKrEBhH
{
    "mAccUserID" : "5755774a6e314346574b4d64626d6638394472436e5032776b4336414b7245426848",
    "FreeValues" : 1000000000000000,
    "FrozenFunds" : [
    ]
}
```

**P.S. For detailed instruction, please refer to [Command Help Doc](CmdHelp.md)**

## 智能合约开发说明：

* main方法
智能合约中有个 main() 方法，调用智能合约是 会有一个全局变量 contract 作为字节数组 传入main()方法，然后根据字节数组里面的数组第二个字段判定，应该走到哪一个不同的方法。

* 调用mylib 接口 和 数据结构
不同方法里面都会调用mylib 的接口，通过不同的数据表结构 来做读写操作，所有mylib接口的说明，请参考《智能合约API》
