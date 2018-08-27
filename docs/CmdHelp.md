## WaykiChain Command Reference

* "<>" : required,  "[]" : optional

|Command | Parameters| Description | Unlocked wallet required?|
|---|---|---|---|
| addnode | <node><add/remove/onetry>| Attempts add or remove <node> from the addnode list or try a connection to <node> once.| N |
| backupwallet | <destination> | Safely copies wallet.dat to destination, which can be a directory or a path with filename. | N |
| createcontracttx | <userregid><appid><amount><contract><fee>[height  ,default = the tip block height] | create contract transaction | Y |
| createcontracttxraw | <height><fee><amount><address><contract> | Create contract transaction from hex string | N |
| dropprivkey | | drop private key from wallet | Y |
| dumpwallet | <filename> | Dumps all wallet keys in a human-readable format.And write to <filename> | Y |
| dumpprivkey | <dacrsaddress> | Reveals the private key corresponding to <dacrsaddress> | Y |
| encryptwallet | <passphrase> | Encrypts the wallet with <passphrase> | N |
| generateblock | <address> | cteate a block with the appointed address | N |
| getaccountinfo | <address> | Returns the account information  with the given address | N |
| getaddednodeinfo | <dns> [node] | Returns information about the given added node, or all added nodes. <br>(note that onetry addnodes are not listed here) If dns is false, only a list of added nodes will be provided, otherwise connected information will also be available. | N |
| getappaccinfo | <scriptid><address> | get appaccount info | N |
| getappkeyvalue | <scriptid><array> | get application key value | N |
| getbalance | [account] [minconf=1] | If [account] is not specified, returns the server's total available balance. <br>If [account] is specified, returns the balance in the account. <br>If [minconf] is 1; Only include transactions confirmed. Default max value is 30, can configure -maxconf parameter changer the max value.| N |
| getbestblockhash | | Returns the hash of the best (tip) block in the longest block chain. | N |
| getblock | <hash or index>[verbose] |Returns information about the block with the given hash or index.If verbose is true,return a json object, false return the hex encoded data | N |
| getblockcount | | Returns the number of blocks in the longest block chain. | N |
| getblockchaininfo | | Returns an object containing various state info regarding block chain processing. | N |
| getblockhash | <index> | Returns hash of block in best-block-chain at <index>; index 0 is the genesis block | N |
| getnettotals | | Returns information about network traffic, including bytes in, bytes out | N |
| getconnectioncount | | Returns the number of connections to other nodes | N |
| getdacrsstate | <num> | Returns state data  about the recently num blocks | N |
| getdifficulty | | Returns the proof-of-work difficulty as a multiple of the minimum difficulty | N |
| getinfo | | Returns an object containing various state info | N |
| getmininginfo | | Returns an object containing mining-related information: <br> <ul>blocks</ul><ul>currentblocksize</ul><ul>currentblocktx</ul><ul>difficulty</ul><ul>errors</ul><ul>generate</ul><ul>genproclimit</ul><ul>hashespersec</ul><ul>pooledtx</ul><ul>testnet</ul> | N |
| getnewaddress | [isminer] | Returns a new  address for receiving payments. If [isminer] is ture will create a miner key,otherwise will only return a new address. | Y |
| getnetworkhashps | [blocks][height] | Returns the estimated network hashes per second based on the last n blocks.<br><li>.    blocks</li> (numeric, optional, default=120) The number of blocks, or -1 for blocks since last difficulty change</li><li>2.    height (numeric, optional, default=-1) To estimate at the time of the given height.</li>| N |
| getnetworkinfo | Returns an object containing various state info regarding P2P network | N |
| getpeerinfo | Returns data about each connected node | N |
| getrawmempool | [verbose] | Returns all transaction ids in memory pool.If verbose is true,return  a json object, false return array of transaction ids. | N |
| getscriptdata | <scriptid><pagsize or key>[index] | get the script data by given scripted. <br> < scriptid ><key>  or < scriptid >< pagsize >[index] | N |
| getscriptvalidedata | <scriptid><pagsize><index> | get script valide data | N |
| gettxdetail | <txhash> | Returns an object about the transaction  detail information by <txhash> | N |
| getwalletinfo | | Returns an object containing various wallet state info | N |
| help | [command] | List commands, or get help for a command | N | 
| importprivkey | <dacrsprivkey> [label] [rescan=true] | Adds a private key (as returned by dumpprivkey) to your wallet. This may take a while, as a rescan is done, looking for existing transactions. Note: There's no need to import public key, as in ECDSA (unlike RSA) this can be computed from private key. | Y |
| importwallet | <filename> | Import keys from a wallet dump file (see dumpwallet). | Y |
| islocked | | Return an object about whether the wallet is being locked or unlocked | N |
| listaddr | | return Array containing address,balance,haveminerkey,regid information | N |
| listapp | <showDetail> | get the list register script: <br>1. showDetail  (boolean, required)true to show scriptContent,otherwise to not show it. | N |
| listcheckpoint | | Returns the list of checkpoint | N |
| listtx | | get all confirm transactions and all unconfirm transactions from wallet | N |
| listtxcache | | get all transactions in cahce | N |
| listunconfirmedtx | | get the list  of unconfirmedtx | N |
| registaccounttx | <address><fee> | register secure account | Y |
| registaccounttxraw | <height><fee><publickey>[minerpublickey] | create a register account transaction | N |
| registerapptx | <address><filepath><fee>[height][scriptdescription] | create a register script transaction | Y |
| registerscripttxraw | <height><fee><address><flag><script or scriptid><script description> | Register script: <br>1.    Height(numeric required) :valod height<br> 2.    Fee: (numeric required) pay to miner<br>3.    address: (string required)for send<br>4.    flag: (numeric, required) 0-1<br>5.    script or scriptid: (string required), if flag=0 is script's file path, else if flag=1 scriptid<br>6.    script description:(string optional) new script description.<br>| N |
| sendtoaddress | [dacrsaddress]<[receive address><amount> | Send an amount to a given address. The amount is a real and is rounded to the nearest 0.00000001. Returns the transaction ID <txhash> if successful | Y |
| sendtoaddressraw | ```<height><fee><amount><srcaddress><recvaddress>``` | create normal transaction by hegiht,fee,amount,srcaddress, recvaddress | N |
| sendtoaddresswithfee | [sendaddress]<recvaddress><amount><fee> | Send an amount to a given address with fee. The amount is a real and is rounded to the nearest 0.00000001 (Sendaddress is optional) | Y |
| setgenerate | ```<generate>``` [genproclimit] | <generate> is true or false to turn generation on or off. Generation is limited to [genproclimit] processors, -1 is unlimited. | N |
| settxfee | ```<amount>``` | ```<amount>``` is a real and is rounded to the nearest 0.00000001 | N |
| signmessage | ```<dacrsaddress> <message>``` | Sign a message with the private key of an address. | Y | 
| sigstr | ```<transaction><address>``` | signature transaction | N |
| stop | | Stop  Dacrs server | N |
| submitblock | ```<hexdata>``` [optional-params-obj] | Attempts to submit new block to network <br> 1. hexdata (string, required) the hex-encoded block data to submit | N |
| submittx | ```<transaction>``` | submit transaction | Y |
| verifymessage | ```<dacrsaddress>``` <signature> <message> | Verify a signed message. | N | 
| verifychain | [checklevel][numblocks] | Verifies blockchain database: <br>1.    checklevel (numeric, optional, 0-4, default=3), How thorough the block verification is.<br>2.    numblocks (numeric, optional, default=288, 0=all) The number of blocks to check. | N |
| walletlock | | Removes the wallet encryption key from memory, locking the wallet. After calling this method, you will need to call walletpassphrase again before being able to call any methods which require the wallet to be unlocked. | N |
| walletpassphrase | ```<passphrase> <timeout>``` | Stores the wallet decryption key in memory for <timeout> seconds. | N | 
| walletpassphrasechange | ```<oldpassphrase> <newpassphrase>``` | Changes the wallet passphrase from <oldpassphrase> to <newpassphrase> | N |
| ping | | Requests that a ping be sent to all other nodes, to measure ping time. | N | 
| validateaddress | ```<address>``` | check the address is valide | N | 
| getalltxinfo | [nlimitCount] | if no input params, return all transactions in wallet include those confirmed and unconfirmed, else return the number of nlimitCount transaction relate. | N |
