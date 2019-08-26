// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _COINRPC_SERVER_H_
#define _COINRPC_SERVER_H_

#include "rpcprotocol.h"
#include "commons/uint256.h"

#include <stdint.h>
#include <list>
#include <map>
#include <string>

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_writer_template.h"
using namespace std;
class CBlockIndex;

/* Start RPC Server */

bool StartRPCServer();

/** interrupt RPC Server */
void InterruptRPCServer();

/* Stop RPC Server */
void StopRPCServer();

/*
  Type-check arguments; throws JSONRPCError if wrong type given. Does not check that
  the right number of arguments are passed, just that any passed are the correct type.
  Use like:  RPCTypeCheck(params, boost::assign::list_of(str_type)(int_type)(obj_type));
*/
void RPCTypeCheck(const json_spirit::Array& params,
                  const list<json_spirit::Value_type>& typesExpected, bool fAllowNull = false);
/*
  Check for expected keys/value types in an Object.
  Use like: RPCTypeCheck(object, boost::assign::map_list_of("name", str_type)("value", int_type));
*/
void RPCTypeCheck(const json_spirit::Object& o,
                  const map<string, json_spirit::Value_type>& typesExpected,
                  bool fAllowNull = false);

/*
  Run func nSeconds from now. Uses boost deadline timers.
  Overrides previous timer <name> (if any).
 */
void RPCRunLater(const std::string& name, std::function<void()> func, int64_t nSeconds);

typedef json_spirit::Value (*rpcfn_type)(const json_spirit::Array& params, bool fHelp);

class CRPCCommand {
public:
    string name;
    rpcfn_type actor;
    bool okSafeMode;
    bool threadSafe;
    bool reqWallet;
};

/**
 * Coin RPC command dispatcher.
 */
class CRPCTable {
private:
    map<string, const CRPCCommand*> mapCommands;

public:
    CRPCTable();
    const CRPCCommand* operator[](string name) const;
    string help(string name) const;

    /**
     * Execute a method.
     * @param method   Method to execute
     * @param params   Array of arguments (JSON objects)
     * @returns Result of the call.
     * @throws an exception (json_spirit::Value) when an error happens.
     */
    json_spirit::Value execute(const string& method, const json_spirit::Array& params) const;
};

extern const CRPCTable tableRPC;

//
// Utilities: convert hex-encoded Values
// (throws error if not hex).
//
extern uint256 ParseHashV(const json_spirit::Value& v, string strName);
extern uint256 ParseHashO(const json_spirit::Object& o, string strKey);
extern vector<unsigned char> ParseHexV(const json_spirit::Value& v, string strName);
extern vector<unsigned char> ParseHexO(const json_spirit::Object& o, string strKey);

extern int64_t nWalletUnlockTime;
extern int64_t AmountToRawValue(const json_spirit::Value& value);
extern json_spirit::Value ValueFromAmount(int64_t amount);
extern double GetDifficulty(const CBlockIndex* blockindex = NULL);
extern string HexBits(unsigned int nBits);
extern string HelpRequiringPassphrase();
extern string HelpExampleCli(string methodname, string args);
extern string HelpExampleRpc(string methodname, string args);

extern void EnsureWalletIsUnlocked();

extern json_spirit::Value getconnectioncount(const json_spirit::Array& params, bool fHelp); // in rpcnet.cpp
extern json_spirit::Value getpeerinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value ping(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value addnode(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getaddednodeinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getnettotals(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getchainstate(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value dumpprivkey(const json_spirit::Array& params, bool fHelp); // in rpcdump.cpp
extern json_spirit::Value importprivkey(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value dumpwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value importwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value dropminerkeys(const json_spirit::Array& params, bool fHelp);

//extern json_spirit::Value getgenerate(const json_spirit::Array& params, bool fHelp); // in rpcmining.cpp
extern json_spirit::Value setgenerate(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gethashespersec(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getmininginfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getwork(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblocktemplate(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getminedblocks(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getnewaddr(const json_spirit::Array& params, bool fHelp); // in rpcwallet.cpp
extern json_spirit::Value getaccount(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value verifymessage(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getcoinunitinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getbalance(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value addmulsigaddr(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value createmulsig(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value backupwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletpassphrase(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletpassphrasechange(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value walletlock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value encryptwallet(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getwalletinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockchaininfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getnetworkinfo(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value signmessage(const json_spirit::Array& params, bool fHelp);
// extern json_spirit::Value (const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getcontractassets(const json_spirit:: Array& params, bool fHelp);
extern json_spirit::Value send(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value gensendtoaddressraw    (const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value genregisteraccountraw  (const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value genregistercontractraw (const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value gencallcontractraw     (const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value genvotedelegateraw     (const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value genmulsigtx            (const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value sendtxraw(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value signtxraw(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value decodetxraw(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value decodemulsigscript(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value getblockcount(const json_spirit::Array& params, bool fHelp); // in rpcblockchain.cpp
extern json_spirit::Value getbestblockhash(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getdifficulty(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value settxfee(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getrawmempool(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockhash(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblock(const json_spirit::Array& params, bool fHelp);
// extern json_spirit::Value gettxoutsetinfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value verifychain(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getcontractregid(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value invalidateblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value reconsiderblock(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value startcommontpstest(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value startcontracttpstest(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getblockfailures(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value submitpricefeedtx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitstakefcointx(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value submitdexbuylimitordertx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitdexselllimitordertx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitdexbuymarketordertx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitdexsellmarketordertx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitdexcancelordertx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitdexsettletx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getdexorder(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getdexsysorders(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getdexorders(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value submitstakecdptx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitredeemcdptx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitliquidatecdptx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getscoininfo(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getcdp(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getusercdp(const json_spirit::Array& params, bool fHelp);

extern json_spirit::Value submitassetissuetx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value submitassetupdatetx(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value getassets(const json_spirit::Array& params, bool fHelp);

json_spirit::Object JSONRPCExecOne(const json_spirit::Value& req);

std::string JSONRPCExecBatch(const json_spirit::Array& vReq);

/** Opaque base class for timers returned by NewTimerFunc.
 * This provides no methods at the moment, but makes sure that delete
 * cleans up the whole state.
 */
class RPCTimerBase {
public:
    virtual ~RPCTimerBase() {}
};

/**
 * RPC timer "driver".
 */
class RPCTimerInterface {
public:
    virtual ~RPCTimerInterface() {}
    /** Implementation name */
    virtual const char* Name() = 0;
    /** Factory function for timers.
     * RPC will call the function to create a timer that will call func in *millis* milliseconds.
     * @note As the RPC mechanism is backend-neutral, it can use different implementations of
     * timers. This is needed to cope with the case in which there is no HTTP server, but only GUI
     * RPC console, and to break the dependency of pcserver on httprpc.
     */
    virtual RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) = 0;
};

/** Set the factory function for timers */
void RPCSetTimerInterface(RPCTimerInterface* iface);
/** Unset factory function for timers */
void RPCUnsetTimerInterface(RPCTimerInterface* iface);

#endif
