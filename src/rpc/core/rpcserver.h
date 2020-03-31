// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _COINRPC_SERVER_H_
#define _COINRPC_SERVER_H_

#include "commons/uint256.h"
#include "rpcprotocol.h"
#include "rpc/rpcapi.h"

#include <stdint.h>
#include <list>
#include <map>
#include <string>

#include "commons/json/json_spirit_reader_template.h"
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_writer_template.h"
using namespace std;
using namespace json_spirit ;
class CBlockIndex;

Value help(const Array& params, bool fHelp);
Value stop(const Array& params, bool fHelp);

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
extern string HexBits(unsigned int nBits);
extern string HelpRequiringPassphrase();
extern string HelpExampleCli(string methodname, string args);
extern string HelpExampleRpc(string methodname, string args);

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
