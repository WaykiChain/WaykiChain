## WaykiChain Command Reference

* "<>" : required,  "[]" : optional

|Command | Parameters| Description | Unlocked wallet required?|
|---|---|---|---|
| addnode | ```<node><add/remove/onetry>```| Attempts add or remove ```<node>``` from the addnode list or try a connection to ```<node>``` once.| N |
| backupwallet | ```<destination>``` | Safely copies wallet.dat to a target directory. | N |
| submitcontractcalltx | ```<sendaddress><appid><arguments><amount><fee>[height, default = the tip block height]``` | create contract invoke transaction | Y |
| dropminerkeys | | drop miner keys from wallet to become a cool miner| Y |
| dumpwallet | ```<filename>``` | Dumps all wallet keys in a human-readable format.And write to ```<filename>``` | Y |
| dumpprivkey | ```<wiccaddress>``` | Reveals the private key corresponding to ```<wiccaddress>``` | Y |
| encryptwallet | ```<passphrase>``` | Encrypts the wallet with ```<passphrase>``` | N |
| generateblock | ```<address>``` | create a block with the appointed address | N |
| getaccountinfo | ```<address>``` | Returns the account information  with the given address | N |
| getaddednodeinfo | ```<dns>``` [node] | Returns information about the given added node, or all added nodes. <br>(note that onetry addnodes are not listed here) If dns is false, only a list of added nodes will be provided, otherwise connected information will also be available. | N |
| getcontractaccountinfo | ```<regid><address>``` | get contract account info | N |
| getbalance | ```[account] [minconf=1]``` | If [account] is not specified, returns the server's total available balance. <br>If [account] is specified, returns the balance in the account. <br>If [minconf] is 1; Only include transactions confirmed. Default max value is 30, can configure -maxconf parameter changer the max value.| N |
| getblock | ```<hash or index>[verbose]``` |Returns information about the block with the given hash or index.If verbose is true,return a json object, false return the hex encoded data | N |
| getblockcount | | Returns the number of blocks in the longest block chain. | N |
| getblockchaininfo | | Returns an object containing various state info regarding block chain processing. | N |
| getnettotals | | Returns information about network traffic, including bytes in, bytes out | N |
| getconnectioncount | | Returns the number of connections to other nodes | N |
| getchainstate | ```<num>``` | Returns chain state by retrieving the recent num of blocks | N |
| getinfo | | Returns an object containing various state info | N |
| getmininginfo | | Returns an object containing mining-related information: <br> <ul>blocks</ul><ul>currentblocksize</ul><ul>currentblocktx</ul><ul>difficulty</ul><ul>errors</ul><ul>generate</ul><ul>pooledtx</ul><ul>testnet</ul> | N |
| getnewaddr | ```[isminer]``` | Returns a new  address for receiving payments. If [isminer] is ture will create a miner key,otherwise will only return a new address. | Y |
| getnetworkinfo | | Returns an object containing various state info regarding P2P network | N |
| getpeerinfo | | Returns data about each connected node | N |
| getrawmempool | ```[verbose]``` | Returns all transaction ids in memory pool. If verbose is true, return a json object, false return array of transaction ids. | N |
| getcontractdata | ```<contractregid><key><hexadecimal>``` | get contract data. <br> ```<contractregid><key><hexadecimal>``` | N |
| gettxdetail | ```<txid>``` | Returns an object about the transaction  detail information by ```<txid>``` | N |
| getwalletinfo | | Returns an object containing detaild wallet info | N |
| help | ```[command]``` | List commands, or get help for a command | N |
| importprivkey | ```<privkey>``` | Adds a private key (as returned by dumpprivkey) to your wallet. Note: There's no need to import public key, as in ECDSA (unlike RSA), which can be computed from private key. | Y |
| importwallet | ```<filename>``` | Import keys from a wallet dump file (see dumpwallet). | Y |
| invalidateblock | ```<hash>``` | Mark a block as invalid. | N |
| listaddr | | return Array containing address, balance, haveminerkey, regid information | N |
| listcontracts | ```<showdetail>``` | get all registered contracts: <br>1. showdetail: 0 | 1  (boolean, required) 1 to show scriptContent, otherwise not show it. | N |
| listtx | | get all confirmed transactions and all unconfirmed transactions from wallet | N |
| listtxcache | | get all transactions in cache | N |
| listunconfirmedtx | | get the list of unconfirmed tx | N |
| submitaccountregistertx | ```<address> [fee]``` | register an account from the local wallet node | Y |
| reconsiderblock | ```<hash>``` | Removes invalidity status of a block and its descendants, reconsider them for activation. | N |
| submitcontractdeploytx | ```<address><filepath><fee>[height][contract_description]``` | register a contract app | Y |
| setgenerate | ```<generate>``` [genblocklimit] | <generate> is true or false to turn generation on or off. Generation is limited to [genblocklimit] processors, -1 is unlimited. | N |
| settxfee | ```<amount>``` | ```<amount>``` is a real and is rounded to the nearest 0.00000001 | N |
| signmessage | ```<wiccaddress> <message>``` | Sign a message with the private key of an address. | Y |
| signtxraw | ```<transaction> <address>``` | signature transaction | N |
| stop | | Stop  WaykiCoind server | N |
| submitblock | ```<hexdata>``` [optional-params-obj] | Attempts to submit new block to network <br> 1. hexdata (string, required) the hex-encoded block data to submit | N |
| sendtxraw | ```<transaction>``` | send raw transaction | N |
| verifymessage | ```<wiccaddress>``` <signature> <message> | Verify a signed message. | N |
| verifychain | ```[checklevel][numblocks]``` | Verifies blockchain database: <br>1.    checklevel (numeric, optional, 0-4, default=3), How thorough the block verification is.<br>2.    numblocks (numeric, optional, default=288, 0=all) The number of blocks to check. | N |
| walletlock | | Removes the wallet encryption key from memory, locking the wallet. After calling this method, you will need to call walletpassphrase again before being able to call any methods which require the wallet to be unlocked. | N |
| walletpassphrase | ```<passphrase> <timeout>``` | Stores the wallet decryption key in memory for <timeout> seconds. | N |
| walletpassphrasechange | ```<oldpassphrase> <newpassphrase>``` | Changes the wallet passphrase from <oldpassphrase> to <newpassphrase> | N |
| ping | | Requests that a ping be sent to all other nodes, to measure ping time. | N |
| validateaddr | ```<address>``` | check whether the address is valid or not | N |