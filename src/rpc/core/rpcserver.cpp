// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "rpc/rpcapiconf.h"

#include "logging.h"
#include "commons/base58.h"
#include "commons/util/util.h"
#include "init.h"
#include "main.h"

#include <boost/algorithm/string.hpp>
#include <memory>
#include "wallet/wallet.h"
#include "commons/json/json_spirit_writer_template.h"
#include "httpserver.h"

using namespace std;
using namespace json_spirit;

/** Simple one-shot callback timer to be used by the RPC mechanism to e.g.
 * re-lock the wallet.
 */
class HTTPRPCTimer : public RPCTimerBase {
public:
    HTTPRPCTimer(struct event_base* eventBase, std::function<void()>& func, int64_t millis)
        : ev(eventBase, false, func) {
        struct timeval tv;
        tv.tv_sec  = millis / 1000;
        tv.tv_usec = (millis % 1000) * 1000;
        ev.trigger(&tv);
    }

private:
    HTTPEvent ev;
};

class HTTPRPCTimerInterface : public RPCTimerInterface {
public:
    explicit HTTPRPCTimerInterface(struct event_base* _base) : base(_base) {}
    const char* Name() override { return "HTTP"; }
    RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) override {
        return new HTTPRPCTimer(base, func, millis);
    }

private:
    struct event_base* base;
};

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char* WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

static string strRPCUserColonPass;

/* Stored RPC timer interface (for unregistration) */
static std::unique_ptr<HTTPRPCTimerInterface> httpRPCTimerInterface;

/* Timer-creating functions */
static RPCTimerInterface* timerInterface = nullptr;
/* Map of name to timer. */
static std::map<std::string, std::unique_ptr<RPCTimerBase> > deadlineTimers;

//! Substitute for C++14 std::make_unique.
template <typename T, typename... Args>
std::unique_ptr<T> MakeUnique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

static bool JsonRPCHandler(HTTPRequest* req, const std::string&);

void RPCTypeCheck(const Array& params, const list<Value_type>& typesExpected, bool fAllowNull) {
    unsigned int i = 0;
    for (auto t : typesExpected) {
        if (params.size() <= i)
            break;

        const Value& v = params[i];
        if (!((v.type() == t) || (fAllowNull && (v.type() == null_type)))) {
            string err = strprintf("Expected type %s, got %s for params[%d]", Value_type_name[t],
                                   Value_type_name[v.type()], i);
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
        i++;
    }
}

void RPCTypeCheck(const Object& o, const map<string, Value_type>& typesExpected, bool fAllowNull) {
    for (const auto& t : typesExpected) {
        const Value& v = find_value(o, t.first);
        if (!fAllowNull && v.type() == null_type)
            throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing %s", t.first));

        if (!((v.type() == t.second) || (fAllowNull && (v.type() == null_type)))) {
            string err = strprintf("Expected type %s for %s, got %s", Value_type_name[t.second],
                                   t.first, Value_type_name[v.type()]);
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
    }
}

int64_t AmountToRawValue(const Value& value) {
    double dAmount  = value.get_real();
    int64_t nAmount = roundint64(dAmount);
    if (!CheckBaseCoinRange(nAmount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

    return nAmount;
}


string HexBits(unsigned int nBits) {
    union {
        int32_t nBits;
        char cBits[4];
    } uBits;
    uBits.nBits = htonl((int32_t)nBits);
    return HexStr(BEGIN(uBits.cBits), END(uBits.cBits));
}

uint256 ParseHashV(const Value& v, string strName) {
    string strHex;
    if (v.type() == str_type)
        strHex = v.get_str();
    if (!IsHex(strHex))  // Note: IsHex("") is false
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName + " must be hexadecimal string (not '" + strHex + "')");

    uint256 result;
    result.SetHex(strHex);

    return result;
}
uint256 ParseHashO(const Object& o, string strKey) {
    return ParseHashV(find_value(o, strKey), strKey);
}
vector<unsigned char> ParseHexV(const Value& v, string strName) {
    string strHex;
    if (v.type() == str_type) strHex = v.get_str();
    if (!IsHex(strHex))
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strName + " must be hexadecimal string (not '" + strHex + "')");
    return ParseHex(strHex);
}
vector<unsigned char> ParseHexO(const Object& o, string strKey) {
    return ParseHexV(find_value(o, strKey), strKey);
}

///
/// Note: This interface may still be subject to change.
///
inline void message_ltrim(string& s, char c){
    while(s[0] == c) {
        s = s.substr(1, s.size() -1);
    }
}

string CRPCTable::help(string strCommand) const {
    string strRet;
    set<rpcfn_type> setDone;
    for (map<string, const CRPCCommand*>::const_iterator mi = mapCommands.begin();
         mi != mapCommands.end(); ++mi) {
        const CRPCCommand* pcmd = mi->second;
        string strMethod        = mi->first;
        // We already filter duplicates, but these deprecated screw up the sort order
        // WASM_TRACE("help:%s", strMethod.c_str())

        if (strMethod.find("label") != string::npos)
            continue;
        if (strCommand != "" && strMethod != strCommand)
            continue;

        if (pcmd->reqWallet && !pWalletMain)
            continue;
        try {
            Array params;
            rpcfn_type pfn = pcmd->actor;
            if (setDone.insert(pfn).second)
                (*pfn)(params, true);
        } catch (std::exception& e) {
            // Help text is returned in an exception
            string strHelp = string(e.what());
            message_ltrim(strHelp, '\n');
            if (strCommand == ""){
                if (strHelp.find('\n') != string::npos){
                    message_ltrim(strHelp, ' ');
                    strHelp = strHelp.substr(0, strHelp.find('\n'));
                }

            }
            strRet += strHelp + "\n";
        }
    }
    if (strRet == "")
        strRet = strprintf("help: unknown command: %s\n", strCommand);
    strRet = strRet.substr(0, strRet.size() - 1);

    return strRet;
}

Value help(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "help ( \"command\" )\n"
            "\nList all commands, or get help for a specified command.\n"
            "\nArguments:\n"
            "1. \"command\"     (string, optional) The command to get help on\n"
            "\nResult:\n"
            "\"text\"     (string) The help text\n");

    string strCommand;
    if (params.size() > 0)
        strCommand = params[0].get_str();

    return tableRPC.help(strCommand);
}

Value stop(const Array& params, bool fHelp) {
    // Accept the deprecated and ignored 'detach' boolean argument
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "stop\n"
            "\nStop coin server.");
    // Shutdown will take long enough that the response should get back
    StartShutdown();
    return "coin daemon being stopped...";
}

CRPCTable::CRPCTable() {
    for (uint32_t index = 0; index < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); ++index) {
        const CRPCCommand* pCMD;
        pCMD                    = &vRPCCommands[index];
        mapCommands[pCMD->name] = pCMD;
    }
}

const CRPCCommand* CRPCTable::operator[](string name) const {
    map<string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
    if (it == mapCommands.end()) {
        LogPrint(BCLog::RPC, "Received an invalid request for rpc: %s\n", name);
        return nullptr;
    } else {
        LogPrint(BCLog::RPC, "Received a valid request for rpc: %s\n", name);
    }

    return (*it).second;
}

bool HTTPAuthorized(const std::string& strAuth) {
    if (strAuth.substr(0, 6) != "Basic ")
        return false;

    string strUserPass64 = strAuth.substr(6);
    boost::trim(strUserPass64);
    string strUserPass = DecodeBase64(strUserPass64);
    return TimingResistantEqual(strUserPass, strRPCUserColonPass);
}

static void ErrorReply(HTTPRequest* req, const json_spirit::Object& objError,
                           const json_spirit::Value& id) {
    // Send error reply from json-rpc error object
    int nStatus = HTTP_INTERNAL_SERVER_ERROR;
    Value codeObj = json_spirit::find_value(objError, "code");
    if (codeObj.type() == int_type) {
        int code = codeObj.get_int();

        if (code == RPC_INVALID_REQUEST)
            nStatus = HTTP_BAD_REQUEST;
        else if (code == RPC_METHOD_NOT_FOUND)
            nStatus = HTTP_NOT_FOUND;
    }
    std::string strReply = JSONRPCReply(Value::null, objError, id);
    req->WriteHeader("Content-Type", "application/json");
    req->WriteReply(nStatus, strReply);
}

static bool InitRPCAuthentication() {
    strRPCUserColonPass =
        SysCfg().GetArg("-rpcuser", "") + ":" + SysCfg().GetArg("-rpcpassword", "");
    string rpcuser     = SysCfg().GetArg("-rpcuser", "");
    string rpcpassword = SysCfg().GetArg("-rpcpassword", "");
    // RPC user/password required but empty or equal to each other
    if (SysCfg().RequireRPCPassword() && (rpcuser == "" || rpcuser == rpcpassword)) {
        unsigned char rand_pwd[32];
        RAND_bytes(rand_pwd, 32);
        string strWhatAmI = "To use coind";

        if (SysCfg().IsArgCount("-rpcserver")) {
            strWhatAmI = strprintf(_("To use the %s option"), "\"-rpcserver\"");

        } else if (SysCfg().IsArgCount("-daemon")) {
            strWhatAmI = strprintf(_("To use the %s option"), "\"-daemon\"");
        }

        LogPrint(BCLog::ERROR, "%s, you must set a rpcpassword in the configuration file:\n"
                  "%s\n"
                  "It is recommended you use the following random password:\n"
                  "rpcuser=wiccrpc\n"
                  "rpcpassword=%s\n"
                  "(you do not need to remember this password)\n"
                  "The username and password MUST NOT be the same.\n"
                  "If the file does not exist, create it with owner-readable-only file "
                  "permissions.\n"
                  "It is also recommended to set alertnotify so you are notified of problems;\n"
                  "for example: alertnotify=echo %%s | mail -s \"Coin Alert\" admin@foo.com\n",
                strWhatAmI, GetConfigFile().string(),
                EncodeBase58(&rand_pwd[0], &rand_pwd[0] + 32));

        StartShutdown();
        return false;
    }

    return true;
}

bool StartRPCServer() {
    LogPrint(BCLog::INFO, "Starting HTTP RPC server\n");
    if (!InitRPCAuthentication()) {
        return false;
    }
    if (!InitHTTPServer()) {
        StopRPCServer();
        return false;
    }

    RegisterHTTPHandler("/", true, JsonRPCHandler);

    struct event_base* eventBase = EventBase();
    assert(eventBase);
    httpRPCTimerInterface = MakeUnique<HTTPRPCTimerInterface>(eventBase);
    RPCSetTimerInterface(httpRPCTimerInterface.get());
    StartHTTPServer();

    return true;
}

void InterruptRPCServer() {
    LogPrint(BCLog::INFO, "Interrupting HTTP RPC server\n");
    InterruptHTTPServer();
}

void StopRPCServer() {
    LogPrint(BCLog::INFO, "Stopping HTTP RPC server\n");
    UnregisterHTTPHandler("/", true);

    if (httpRPCTimerInterface) {
        RPCUnsetTimerInterface(httpRPCTimerInterface.get());
        httpRPCTimerInterface.reset();
    }
    StopHTTPServer();
    FinalizeHTTPServer();
}

void RPCRunLater(const std::string& name, std::function<void()> func, int64_t nSeconds) {
    if (!timerInterface)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No timer handler registered for RPC");

    deadlineTimers.erase(name);

    LogPrint(BCLog::RPC, "queue run of timer %s in %i seconds (using %s)\n", name, nSeconds,
             timerInterface->Name());

    deadlineTimers.emplace(name, std::unique_ptr<RPCTimerBase>(timerInterface->NewTimer(func, nSeconds * 1000)));
}

class JSONRequest {
public:
    Value id;
    string strMethod;
    Array params;

    JSONRequest() { id = Value::null; }
    void parse(const Value& valRequest);
};

void JSONRequest::parse(const Value& valRequest) {
    // Parse request
    if (valRequest.type() != obj_type)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid Request object");

    const Object& request = valRequest.get_obj();

    // Parse id now so errors from here on will have the id
    id = find_value(request, "id");

    // Parse method
    Value valMethod = find_value(request, "method");
    if (valMethod.type() == null_type)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Missing method");

    if (valMethod.type() != str_type)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Method must be a string");

    strMethod = valMethod.get_str();
    LogPrint(BCLog::RPC, "ThreadRPCServer method=%s\n", strMethod);

    // Parse params
    Value valParams = find_value(request, "params");
    if (valParams.type() == array_type)
        params = valParams.get_array();
    else if (valParams.type() == null_type)
        params = Array();
    else
        throw JSONRPCError(RPC_INVALID_REQUEST, "Params must be an array");
}

Object JSONRPCExecOne(const Value& req) {
    Object rpc_result;

    JSONRequest jreq;
    try {
        jreq.parse(req);

        Value result = tableRPC.execute(jreq.strMethod, jreq.params);
        rpc_result   = JSONRPCReplyObj(result, Value::null, jreq.id);
    } catch (Object& objError) {
        rpc_result = JSONRPCReplyObj(Value::null, objError, jreq.id);
    } catch (std::exception& e) {
        rpc_result = JSONRPCReplyObj(Value::null, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
    }

    return rpc_result;
}

string JSONRPCExecBatch(const Array& vReq) {
    Array ret;
    for (unsigned int reqIdx = 0; reqIdx < vReq.size(); reqIdx++)
        ret.push_back(JSONRPCExecOne(vReq[reqIdx]));

    return write_string(Value(ret), false) + "\n";
}

json_spirit::Value CRPCTable::execute(const string& strMethod,
                                      const json_spirit::Array& params) const {
    // Find method
    const CRPCCommand* pcmd = tableRPC[strMethod];
    if (!pcmd)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found");

    if (pcmd->reqWallet && !pWalletMain)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found (disabled)");

    // Observe safe mode
    if (!SysCfg().GetBoolArg("-disablesafemode", false) && !pcmd->okSafeMode)
        throw JSONRPCError(RPC_FORBIDDEN_BY_SAFE_MODE, "Banned RPC method");

    auto whitelist = SysCfg().GetMultiArgs("-rpcwhitelistcmd");
    if (!whitelist.empty()) {
        // check whilelist
        bool found = false;
        for (auto whiteItem : whitelist) {
            if (whiteItem == strMethod) {
                found = true;
                break;
            }
        }
        if (!found)
            throw JSONRPCError(RPC_FORBIDDEN_BY_SAFE_MODE, strprintf("Banned RPC method=%s by whitelist", strMethod));
    } else {
        // check blacklist
        auto blacklist = SysCfg().GetMultiArgs("-rpcblacklistcmd");
        for (auto blackItem : blacklist) {
            if (blackItem == strMethod)
                throw JSONRPCError(RPC_FORBIDDEN_BY_SAFE_MODE, strprintf("Banned RPC method=%s by blacklist", strMethod));
        }
    }

    try {
        // Execute
        Value result;
        {
            if (pcmd->threadSafe)
                result = pcmd->actor(params, false);
            else if (!pWalletMain) {
                LOCK(cs_main);
                result = pcmd->actor(params, false);
            } else {
                LOCK2(cs_main, pWalletMain->cs_wallet);
                result = pcmd->actor(params, false);
            }
        }

        return result;
    } catch (std::exception& e) {
        throw JSONRPCError(RPC_MISC_ERROR, e.what());
    }
}

string HelpExampleCli(string methodname, string args) {
    return "> ./coind " + methodname + " " + args + "\n";
}

string HelpExampleRpc(string methodname, string args) {
    return "> curl --user myusername -d '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", "
           "\"method\": \"" +
           methodname + "\", \"params\": [" + args +
           "] }' -H 'Content-Type: application/json;' http://127.0.0.1:8332/\n";
}

const CRPCTable tableRPC;

/** json rpc handler registered to http server */
static bool JsonRPCHandler(HTTPRequest* req, const std::string&) {
    // JSONRPC handles only POST or GET
    auto reqMethod = req->GetRequestMethod();
    if (reqMethod != HTTPRequest::POST && reqMethod != HTTPRequest::GET) {
        req->WriteReply(HTTP_BAD_METHOD, "RPC server handles only POST or GET requests");
        return false;
    }
    // Check authorization
    std::pair<bool, std::string> authHeader = req->GetHeader("authorization");
    if (!authHeader.first) {
        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }

    JSONRequest jreq;

    if (!HTTPAuthorized(authHeader.second)) {
        LogPrint(BCLog::RPC, "RPCServer incorrect password attempt from %s\n",
                 req->GetPeer().ToString());

        /* Deter brute-forcing
           If this results in a DoS the user really
           shouldn't have their RPC port exposed. */
        MilliSleep(250);

        req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
        req->WriteReply(HTTP_UNAUTHORIZED);
        return false;
    }

    try {
        // Parse request
        json_spirit::Value valRequest;

        if (!read_string(req->ReadBody(), valRequest))
            throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");

        string strReply;

        // singleton request
        if (valRequest.type() == obj_type) {
            jreq.parse(valRequest);
            Value result = tableRPC.execute(jreq.strMethod, jreq.params);

            // Send reply
            strReply = JSONRPCReply(result, Value::null, jreq.id);

            // array of requests
        } else if (valRequest.type() == array_type)
            strReply = JSONRPCExecBatch(valRequest.get_array());
        else
            throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");

        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strReply);
    } catch (Object& objError) {
        ErrorReply(req, objError, jreq.id);
        return false;
    } catch (std::exception& e) {
        ErrorReply(req, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
        return false;
    }

    return true;
}

void RPCSetTimerInterface(RPCTimerInterface* iface) {
    timerInterface = iface;
}

void RPCUnsetTimerInterface(RPCTimerInterface* iface) {
    if (timerInterface == iface) {
        timerInterface = nullptr;
    }
}
