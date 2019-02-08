// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"
#include "rpctx.h"

#include "base58.h"
#include "init.h"
#include "main.h"
#include "ui_interface.h"
#include "util.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <memory>
#include "json/json_spirit_writer_template.h"
#include "../wallet/wallet.h"
using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace json_spirit;

static string strRPCUserColonPass;

// These are created by StartRPCThreads, destroyed in StopRPCThreads
static asio::io_service* rpc_io_service = NULL;
static map<string, std::shared_ptr<deadline_timer> > deadlineTimers;
static ssl::context* rpc_ssl_context = NULL;
static boost::thread_group* rpc_worker_group = NULL;
static boost::asio::io_service::work *rpc_dummy_work = NULL;
static vector< std::shared_ptr<ip::tcp::acceptor> > rpc_acceptors;

void RPCTypeCheck(const Array& params,
                  const list<Value_type>& typesExpected,
                  bool fAllowNull)
{
    unsigned int i = 0;
    for (auto t : typesExpected) {
        if (params.size() <= i)
            break;

        const Value& v = params[i];
        if (!((v.type() == t) || (fAllowNull && (v.type() == null_type)))) {
            string err = strprintf("Expected type %s, got %s for params[%d]",
                                   Value_type_name[t], Value_type_name[v.type()], i);
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
        i++;
    }
}

void RPCTypeCheck(const Object& o,
                  const map<string, Value_type>& typesExpected,
                  bool fAllowNull)
{
    for (const auto & t : typesExpected) {
        const Value& v = find_value(o, t.first);
        if (!fAllowNull && v.type() == null_type)
            throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing %s", t.first));

        if (!((v.type() == t.second) || (fAllowNull && (v.type() == null_type)))) {
            string err = strprintf("Expected type %s for %s, got %s",
                                   Value_type_name[t.second], t.first, Value_type_name[v.type()]);
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
    }
}


int64_t AmountToRawValue(const Value& value)
{
    double dAmount = value.get_real();
    int64_t nAmount = roundint64(dAmount);
    if (!MoneyRange(nAmount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    return nAmount;
}

Value ValueFromAmount(int64_t amount)
{
    return (double)amount / (double)COIN;
}

string HexBits(unsigned int nBits)
{
    union {
        int32_t nBits;
        char cBits[4];
    } uBits;
    uBits.nBits = htonl((int32_t)nBits);
    return HexStr(BEGIN(uBits.cBits), END(uBits.cBits));
}

uint256 ParseHashV(const Value& v, string strName)
{
    string strHex;
    if (v.type() == str_type)
        strHex = v.get_str();
    if (!IsHex(strHex)) // Note: IsHex("") is false
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    uint256 result;
    result.SetHex(strHex);
    return result;
}
uint256 ParseHashO(const Object& o, string strKey)
{
    return ParseHashV(find_value(o, strKey), strKey);
}
vector<unsigned char> ParseHexV(const Value& v, string strName)
{
    string strHex;
    if (v.type() == str_type)
        strHex = v.get_str();
    if (!IsHex(strHex))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return ParseHex(strHex);
}
vector<unsigned char> ParseHexO(const Object& o, string strKey)
{
    return ParseHexV(find_value(o, strKey), strKey);
}


///
/// Note: This interface may still be subject to change.
///

string CRPCTable::help(string strCommand) const
{
    string strRet;
    set<rpcfn_type> setDone;
    for (map<string, const CRPCCommand*>::const_iterator mi = mapCommands.begin(); mi != mapCommands.end(); ++mi)
    {
        const CRPCCommand *pcmd = mi->second;
        string strMethod = mi->first;
        // We already filter duplicates, but these deprecated screw up the sort order
        if (strMethod.find("label") != string::npos)
            continue;
        if (strCommand != "" && strMethod != strCommand)
            continue;

        if (pcmd->reqWallet && !pwalletMain)
            continue;
        try
        {
            Array params;
            rpcfn_type pfn = pcmd->actor;
            if (setDone.insert(pfn).second)
                (*pfn)(params, true);
        }
        catch (std::exception& e)
        {
            // Help text is returned in an exception
            string strHelp = string(e.what());
            if (strCommand == "")
                if (strHelp.find('\n') != string::npos)
                    strHelp = strHelp.substr(0, strHelp.find('\n'));
            strRet += strHelp + "\n";
        }
    }
    if (strRet == "")
        strRet = strprintf("help: unknown command: %s\n", strCommand);
    strRet = strRet.substr(0,strRet.size()-1);
    return strRet;
}

Value help(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "help ( \"command\" )\n"
            "\nList all commands, or get help for a specified command.\n"
            "\nArguments:\n"
            "1. \"command\"     (string, optional) The command to get help on\n"
            "\nResult:\n"
            "\"text\"     (string) The help text\n"
        );

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
    return "coin server stopping";
}



//
// Call Table
//
extern Value resetclient(const Array& params, bool fHelp);
extern Value gettxoperationlog(const Array& params, bool fHelp);
extern Value dropprivkey(const Array& params, bool fHelp);

static const CRPCCommand vRPCCommands[] =
{ //  name                      actor (function)         okSafeMode threadSafe reqWallet
  //  ------------------------  -----------------------  ---------- ---------- ---------
    /* Overall control/query calls */
    { "getinfo",                &getinfo,                true,      false,      false }, /* uses wallet if enabled */
    { "help",                   &help,                   true,      true,       false },
    { "stop",                   &stop,                   true,      true,       false },
    { "gencheckpoint",          &gencheckpoint,          true,      true,       false },
    { "setcheckpoint",          &setcheckpoint,          true,      true,       false },
    { "validateaddress",        &validateaddress,        true,      true,       false },
    { "gettxhashbyaddress",     &gettxhashbyaddress,     true,      true,       false },

    /* P2P networking */
    { "getnetworkinfo",         &getnetworkinfo,         true,      false,      false },
    { "addnode",                &addnode,                true,      true,       false },
    { "getaddednodeinfo",       &getaddednodeinfo,       true,      true,       false },
    { "getconnectioncount",     &getconnectioncount,     true,      false,      false },
    { "getnettotals",           &getnettotals,           true,      true,       false },
    { "getpeerinfo",            &getpeerinfo,            true,      false,      false },
    { "ping",                   &ping,                   true,      false,      false },
    { "getchainstate",          &getchainstate,          false,     false,      false }, //true

    /* Block chain and UTXO */
    { "getblockchaininfo",      &getblockchaininfo,      true,      false,      false },
    { "getbestblockhash",       &getbestblockhash,       true,      false,      false },
    { "getblockcount",          &getblockcount,          true,      true,       false },
    { "getblock",               &getblock,               false,     false,      false },
    { "getblockhash",           &getblockhash,           false,     false,      false },
    { "getdifficulty",          &getdifficulty,          true,      false,      false },
    { "getrawmempool",          &getrawmempool,          true,      false,      false },
    { "listcheckpoint",         &listcheckpoint,         true,      false,      false },
    { "verifychain",            &verifychain,            true,      false,      false },
    
    { "gettotalcoins",          &gettotalcoins,           false,     false,      false },
    { "gettotalassets",         &gettotalassets,         false,     false,      false },
    { "invalidateblock",        &invalidateblock,        true,      true,       false },
    { "reconsiderblock",        &reconsiderblock,        true,      true,       false },

    /* Mining */
    { "getmininginfo",          &getmininginfo,          true,      false,      false },
    { "getnetworkhashps",       &getnetworkhashps,       true,      false,      false },
    { "submitblock",            &submitblock,            true,      false,      false },

    /* Raw transactions */
    { "sendtoaddressraw",       &gensendtoaddresstxraw,  false,     false,      false },  /* deprecated */
    { "gensendtoaddresstxraw",  &gensendtoaddresstxraw,  false,     false,      false },
    { "registeraccountraw",     &genregisteraccounttxraw,  false,    false,     false },  /* deprecated */
    { "genregisteraccounttxraw",&genregisteraccounttxraw,  false,   false,     false },
    { "genregistercontracttxraw",&genregistercontracttxraw,  true,  false,     false },
    { "gencallcontracttxraw",   &gencallcontracttxraw,    true,      false,     false },
    { "genvotedelegatetxraw",   &genvotedelegatetxraw,    true,      false,     true },

    /* uses wallet if enabled */
    { "backupwallet",           &backupwallet,           true,      false,      true },
    { "dumpprivkey",            &dumpprivkey,            true,      false,      true },
    { "dumpwallet",             &dumpwallet,             true,      false,      true },
    { "encryptwallet",          &encryptwallet,          false,     false,      true },
    { "getaccountinfo",         &getaccountinfo,         true,      false,      true },
    { "getnewaddress",          &getnewaddress,          true,      false,      true },
    { "gettxdetail",            &gettxdetail,            true,      false,      true },
    { "listunconfirmedtx",      &listunconfirmedtx,      true,      false,      true },
    { "getwalletinfo",          &getwalletinfo,          true,      false,      true },
    { "importprivkey",          &importprivkey,          false,     false,      true },
    { "dropprivkey",            &dropprivkey,            false,     false,      true },

    { "importwallet",           &importwallet,           false,     false,      true },
    { "listaddr",               &listaddr,               true,      false,      true },
    { "listtransactions",       &listtransactions,       true,      false,      true },
    { "listtransactionsv2",     &listtransactionsv2,     true,      false,      true },
    { "listtx",                 &listtx,                 true,      false,      true },
    { "listcontracttx",         &listcontracttx,         true,      false,      true },
    { "gettransaction",         &gettransaction,         true,      false,      true },

    { "registaccounttx",        &registeraccounttx,      true,      false,      true }, /** deprecated */
    { "registeraccounttx",      &registeraccounttx,      true,      false,      true },
    { "registercontracttx",     &registercontracttx,     true,      false,      true },
    { "createcontracttx",       &callcontracttx,         true,      false,      true }, /** deprecated */
    { "callcontracttx",         &callcontracttx,         true,      false,      true },
    { "votedelegatetx",         &votedelegatetx,         true,      false,      true },
    
    { "settxfee",               &settxfee,               false,     false,      true },
    { "walletlock",             &walletlock,             true,      false,      true },
    { "walletpassphrasechange", &walletpassphrasechange, false,     false,      true },
    { "walletpassphrase",       &walletpassphrase,       true,      false,      true },
    { "setgenerate",            &setgenerate,            true,      true,       false},
    { "listcontracts",          &listcontracts,          true,      false,      true },
    { "getcontractinfo",        &getcontractinfo,        true,      false,      true },
    { "generateblock",          &generateblock,          true,      true,       true },
    { "listtxcache",            &listtxcache,            true,      false,      true },
    { "getcontractdata",        &getcontractdata,        true,      false,      true },
    { "getcontractdataraw",     &getcontractdataraw,     true,      false,      true },
    { "getcontractconfirmdata", &getcontractconfirmdata, true,      false,      true },
    { "signmessage",            &signmessage,            false,     false,      true },
    { "verifymessage",          &verifymessage,          false,     false,      false },
    { "sendtoaddress",          &sendtoaddress,          false,     false,      true },
    { "sendtoaddresswithfee",   &sendtoaddresswithfee,   false,     false,      true },
    { "getbalance",             &getbalance,             false,     false,      true },
    { "notionalpoolingbalance", &notionalpoolingbalance, false,     false,      true },
    // { "dispersebalance",        &dispersebalance,        false,     false,      true },
    { "notionalpoolingasset",   &notionalpoolingasset,   false,     false,      true },
    { "getassets",              &getassets,              false,     false,      true },
    { "listcontractasset",      &listcontractasset,      false,     false,      true },
    { "submittx",               &submittx,               true,      false,      false},

    { "sigstr",                 &sigstr,                 true,      false,      true },
    { "getcontractaccountinfo", &getcontractaccountinfo, true,      false,      true },
    { "getcontractkeyvalue",    &getcontractkeyvalue,    true,      false,      true },
    { "getsignature",           &getsignature,           true,      false,      true },
    { "listdelegates",          &listdelegates,          true,      false,      true },
    { "decoderawtransaction",   &decoderawtransaction,   false,     false,      false},

    /* for test code */
    { "gettxoperationlog",      &gettxoperationlog,      false,     false,      false},
    { "disconnectblock",        &disconnectblock,        true,      false,      true },
    { "resetclient",            &resetclient,            true,      false,      false},
    { "reloadtxcache",          &reloadtxcache,          true,      false,      true },
    { "listsetblockindexvalid", &listsetblockindexvalid, true,      false,      false},
    { "getcontractregid",       &getcontractregid,       true,      false,      false},
    { "getcontractitemcount",   &getcontractitemcount,   true,      false,      false},
    { "printblockdbinfo",       &printblockdbinfo,       true,      false,      false},
    { "getalltxinfo",           &getalltxinfo,           true,      false,      true },
    { "saveblocktofile",        &saveblocktofile,        true,      false,      true },
    { "gethash",                &gethash,                true,      false,      true },
};

CRPCTable::CRPCTable() {
    unsigned int vcidx;
    for (vcidx = 0; vcidx < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); vcidx++) {
        const CRPCCommand *pcmd;

        pcmd = &vRPCCommands[vcidx];
        mapCommands[pcmd->name] = pcmd;
    }
}

const CRPCCommand *CRPCTable::operator[](string name) const {
    map<string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
    if (it == mapCommands.end())
        return NULL;
    return (*it).second;
}


bool HTTPAuthorized(map<string, string>& mapHeaders) {
    string strAuth = mapHeaders["authorization"];
    if (strAuth.substr(0,6) != "Basic ")
        return false;
    string strUserPass64 = strAuth.substr(6); boost::trim(strUserPass64);
    string strUserPass = DecodeBase64(strUserPass64);
    return TimingResistantEqual(strUserPass, strRPCUserColonPass);
}

void ErrorReply(ostream& stream, const Object& objError, const Value& id) {
    // Send error reply from json-rpc error object
    int nStatus = HTTP_OK;
    // int nStatus = HTTP_INTERNAL_SERVER_ERROR;
    // int code = find_value(objError, "code").get_int();
    // if (code == RPC_INVALID_REQUEST) nStatus = HTTP_BAD_REQUEST;
    // else if (code == RPC_METHOD_NOT_FOUND) nStatus = HTTP_NOT_FOUND;
    string strReply = JSONRPCReply(Value::null, objError, id);
    stream << HTTPReply(nStatus, strReply, false) << flush;
}

bool ClientAllowed(const boost::asio::ip::address& address) {
    // Make sure that IPv4-compatible and IPv4-mapped IPv6 addresses are treated as IPv4 addresses
    if (address.is_v6()
     && (address.to_v6().is_v4_compatible()
      || address.to_v6().is_v4_mapped()))
        return ClientAllowed(address.to_v6().to_v4());

    if (address == asio::ip::address_v4::loopback()
     || address == asio::ip::address_v6::loopback()
     || (address.is_v4()
         // Check whether IPv4 addresses match 127.0.0.0/8 (loopback subnet)
      && (address.to_v4().to_ulong() & 0xff000000) == 0x7f000000))
        return true;

    const string strAddress = address.to_string();
    const vector<string>& vAllow = SysCfg().GetMultiArgs("-rpcallowip");
    for (auto strAllow : vAllow)
        if (WildcardMatch(strAddress, strAllow))
            return true;
    return false;
}

class AcceptedConnection
{
public:
    virtual ~AcceptedConnection() {}

    virtual iostream& stream() = 0;
    virtual string peer_address_to_string() const = 0;
    virtual void close() = 0;
};

template <typename Protocol>
class AcceptedConnectionImpl : public AcceptedConnection
{
public:
    AcceptedConnectionImpl(
            asio::io_service& io_service,
            ssl::context &context,
            bool fUseSSL) :
        sslStream(io_service, context),
        _d(sslStream, fUseSSL),
        _stream(_d)
    {
    }

    virtual iostream& stream()
    {
        return _stream;
    }

    virtual string peer_address_to_string() const
    {
        return peer.address().to_string();
    }

    virtual void close()
    {
        _stream.close();
    }

    typename Protocol::endpoint peer;
    asio::ssl::stream<typename Protocol::socket> sslStream;

private:
    SSLIOStreamDevice<Protocol> _d;
    iostreams::stream< SSLIOStreamDevice<Protocol> > _stream;
};

void ServiceConnection(AcceptedConnection *conn);

// Forward declaration required for RPCListen
template <typename Protocol, typename SocketAcceptorService>
static void RPCAcceptHandler(std::shared_ptr< basic_socket_acceptor<Protocol, SocketAcceptorService> > acceptor,
                             ssl::context& context,
                             bool fUseSSL,
                             std::shared_ptr< AcceptedConnection > conn,
                             const boost::system::error_code& error);

/**
 * Sets up I/O resources to accept and handle a new connection.
 */
template <typename Protocol, typename SocketAcceptorService>
static void RPCListen(std::shared_ptr< basic_socket_acceptor<Protocol, SocketAcceptorService> > acceptor,
                   ssl::context& context,
                   const bool fUseSSL) {
    // Accept connection
    std::shared_ptr< AcceptedConnectionImpl<Protocol> > conn(new AcceptedConnectionImpl<Protocol>(acceptor->get_io_service(), context, fUseSSL));

    acceptor->async_accept(
            conn->sslStream.lowest_layer(),
            conn->peer,
            boost::bind(&RPCAcceptHandler<Protocol, SocketAcceptorService>,
                acceptor,
                boost::ref(context),
                fUseSSL,
                conn,
                _1));
}


/**
 * Accept and handle incoming connection.
 */
template <typename Protocol, typename SocketAcceptorService>
static void RPCAcceptHandler(std::shared_ptr< basic_socket_acceptor<Protocol, SocketAcceptorService> > acceptor,
                             ssl::context& context,
                             const bool fUseSSL,
                             std::shared_ptr< AcceptedConnection > conn,
                             const boost::system::error_code& error) {
    // Immediately start accepting new connections, except when we're cancelled or our socket is closed.
    if (error != asio::error::operation_aborted && acceptor->is_open())
        RPCListen(acceptor, context, fUseSSL);

    AcceptedConnectionImpl<ip::tcp>* tcp_conn = dynamic_cast< AcceptedConnectionImpl<ip::tcp>* >(conn.get());

    if (error) {
        // TODO: Actually handle errors
        LogPrint("INFO","%s: Error: %s\n", __func__, error.message());
    }
    // Restrict callers by IP.  It is important to
    // do this before starting client thread, to filter out
    // certain DoS and misbehaving clients.
    else if (tcp_conn && !ClientAllowed(tcp_conn->peer.address())) {
        // Only send a 403 if we're not using SSL to prevent a DoS during the SSL handshake.
        if (!fUseSSL)
            conn->stream() << HTTPReply(HTTP_FORBIDDEN, "", false) << flush;
        conn->close();
    } else {
        ServiceConnection(conn.get());
        conn->close();
    }
}

void StartRPCThreads() {
    strRPCUserColonPass = SysCfg().GetArg("-rpcuser", "") + ":" + SysCfg().GetArg("-rpcpassword", "");
    string rpcuser = SysCfg().GetArg("-rpcuser", "");
    string rpcpassword = SysCfg().GetArg("-rpcpassword", "");
    // RPC user/password required but empty or equal to each other
    if (SysCfg().RequireRPCPassword() && (rpcuser == "" || rpcpassword == "" || rpcuser == rpcpassword)) {
        unsigned char rand_pwd[32];
        RAND_bytes(rand_pwd, 32);
        string strWhatAmI = "To use coind";
        if (SysCfg().IsArgCount("-server"))
            strWhatAmI = strprintf(_("To use the %s option"), "\"-server\"");
        else if (SysCfg().IsArgCount("-daemon"))
            strWhatAmI = strprintf(_("To use the %s option"), "\"-daemon\"");

        uiInterface.ThreadSafeMessageBox(strprintf(
            _("%s, you must set a rpcpassword in the configuration file:\n"
              "%s\n"
              "It is recommended you use the following random password:\n"
              "rpcuser=wiccrpc\n"
              "rpcpassword=%s\n"
              "(you do not need to remember this password)\n"
              "The username and password MUST NOT be the same.\n"
              "If the file does not exist, create it with owner-readable-only file permissions.\n"
              "It is also recommended to set alertnotify so you are notified of problems;\n"
              "for example: alertnotify=echo %%s | mail -s \"Coin Alert\" admin@foo.com\n"),
                strWhatAmI,
                GetConfigFile().string(),
                EncodeBase58(&rand_pwd[0],&rand_pwd[0]+32)),
                "", CClientUIInterface::MSG_ERROR);
        StartShutdown();
        return;
    }

    assert(rpc_io_service == NULL);
    rpc_io_service = new asio::io_service();
    rpc_ssl_context = new ssl::context(*rpc_io_service, ssl::context::sslv23);

    const bool fUseSSL = SysCfg().GetBoolArg("-rpcssl", false);

    if (fUseSSL) {
        rpc_ssl_context->set_options(ssl::context::no_sslv2);

        filesystem::path pathCertFile(SysCfg().GetArg("-rpcsslcertificatechainfile", "server.cert"));
        if (!pathCertFile.is_complete()) pathCertFile = filesystem::path(GetDataDir()) / pathCertFile;
        if (filesystem::exists(pathCertFile)) rpc_ssl_context->use_certificate_chain_file(pathCertFile.string());
        else LogPrint("INFO","ThreadRPCServer ERROR: missing server certificate file %s\n", pathCertFile.string());

        filesystem::path pathPKFile(SysCfg().GetArg("-rpcsslprivatekeyfile", "server.pem"));
        if (!pathPKFile.is_complete()) pathPKFile = filesystem::path(GetDataDir()) / pathPKFile;
        if (filesystem::exists(pathPKFile)) rpc_ssl_context->use_private_key_file(pathPKFile.string(), ssl::context::pem);
        else LogPrint("INFO","ThreadRPCServer ERROR: missing server private key file %s\n", pathPKFile.string());

        string strCiphers = SysCfg().GetArg("-rpcsslciphers", "TLSv1.2+HIGH:TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!3DES:@STRENGTH");
        SSL_CTX_set_cipher_list(rpc_ssl_context->impl(), strCiphers.c_str());
    }

    // Try a dual IPv6/IPv4 socket, falling back to separate IPv4 and IPv6 sockets
    const bool loopback = !SysCfg().IsArgCount("-rpcallowip");
    asio::ip::address bindAddress = loopback ? asio::ip::address_v6::loopback() : asio::ip::address_v6::any();
    ip::tcp::endpoint endpoint(bindAddress, SysCfg().GetArg("-rpcport", SysCfg().RPCPort()));
    boost::system::error_code v6_only_error;

    bool fListening = false;
    string strerr;
    try {
        std::shared_ptr<ip::tcp::acceptor> acceptor(new ip::tcp::acceptor(*rpc_io_service));
        acceptor->open(endpoint.protocol());
        acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

        // Try making the socket dual IPv6/IPv4 (if listening on the "any" address)
        acceptor->set_option(boost::asio::ip::v6_only(loopback), v6_only_error);

        acceptor->bind(endpoint);
        acceptor->listen(socket_base::max_connections);

        RPCListen(acceptor, *rpc_ssl_context, fUseSSL);

        rpc_acceptors.push_back(acceptor);
        fListening = true;
    } catch (boost::system::system_error &e) {
        strerr = strprintf(_("An error occurred while setting up the RPC port %u for listening on IPv6, falling back to IPv4: %s"), endpoint.port(), e.what());
    }
    try {
        // If dual IPv6/IPv4 failed (or we're opening loopback interfaces only), open IPv4 separately
        if (!fListening || loopback || v6_only_error) {
            bindAddress = loopback ? asio::ip::address_v4::loopback() : asio::ip::address_v4::any();
            endpoint.address(bindAddress);

            std::shared_ptr<ip::tcp::acceptor> acceptor(new ip::tcp::acceptor(*rpc_io_service));
            acceptor->open(endpoint.protocol());
            acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor->bind(endpoint);
            acceptor->listen(socket_base::max_connections);

            RPCListen(acceptor, *rpc_ssl_context, fUseSSL);

            rpc_acceptors.push_back(acceptor);
            fListening = true;
        }
    } catch (boost::system::system_error &e) {
        strerr = strprintf(_("An error occurred while setting up the RPC port %u for listening on IPv4: %s"), endpoint.port(), e.what());
    }

    if (!fListening) {
        uiInterface.ThreadSafeMessageBox(strerr, "", CClientUIInterface::MSG_ERROR);
        StartShutdown();
        return;
    }

    rpc_worker_group = new boost::thread_group();
    int total = SysCfg().GetArg("-rpcthreads", 4);
    for (int i = 0; i < total; i++)
        rpc_worker_group->create_thread(boost::bind(&asio::io_service::run, rpc_io_service));
}

void StartDummyRPCThread() {
    if (rpc_io_service == NULL) {
        rpc_io_service = new asio::io_service();
        /* Create dummy "work" to keep the thread from exiting when no timeouts active,
         * see http://www.boost.org/doc/libs/1_51_0/doc/html/boost_asio/reference/io_service.html#boost_asio.reference.io_service.stopping_the_io_service_from_running_out_of_work */
        rpc_dummy_work = new asio::io_service::work(*rpc_io_service);
        rpc_worker_group = new boost::thread_group();
        rpc_worker_group->create_thread(boost::bind(&asio::io_service::run, rpc_io_service));
    }
}

void StopRPCThreads() {
    if (rpc_io_service == NULL) return;

    // First, cancel all timers and acceptors
    // This is not done automatically by ->stop(), and in some cases the destructor of
    // asio::io_service can hang if this is skipped.
    boost::system::error_code ec;
    for (const auto &acceptor : rpc_acceptors) {
        acceptor->cancel(ec);
        if (ec)
            LogPrint("INFO","%s: Warning: %s when cancelling acceptor", __func__, ec.message());
    }
    rpc_acceptors.clear();
    for (const auto &timer : deadlineTimers) {
        timer.second->cancel(ec);
        if (ec)
            LogPrint("INFO","%s: Warning: %s when cancelling timer", __func__, ec.message());
    }
    deadlineTimers.clear();

    rpc_io_service->stop();
    if (rpc_worker_group != NULL)
        rpc_worker_group->join_all();
    delete rpc_dummy_work; rpc_dummy_work = NULL;
    delete rpc_worker_group; rpc_worker_group = NULL;
    delete rpc_ssl_context; rpc_ssl_context = NULL;
    delete rpc_io_service; rpc_io_service = NULL;
}

void RPCRunHandler(const boost::system::error_code& err, boost::function<void(void)> func) {
    if (!err)
        func();
}

void RPCRunLater(const string& name, boost::function<void(void)> func, int64_t nSeconds) {
    assert(rpc_io_service != NULL);

    if (deadlineTimers.count(name) == 0) {
        deadlineTimers.insert(make_pair(name, std::shared_ptr<deadline_timer>(new deadline_timer(*rpc_io_service))));
    }
    deadlineTimers[name]->expires_from_now(posix_time::seconds(nSeconds));
    deadlineTimers[name]->async_wait(boost::bind(RPCRunHandler, _1, func));
}

class JSONRequest
{
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
    if (strMethod != "getwork" && strMethod != "getblocktemplate")
        LogPrint("rpc", "ThreadRPCServer method=%s\n", strMethod);

    // Parse params
    Value valParams = find_value(request, "params");
    if (valParams.type() == array_type)
        params = valParams.get_array();
    else if (valParams.type() == null_type)
        params = Array();
    else
        throw JSONRPCError(RPC_INVALID_REQUEST, "Params must be an array");
}


static Object JSONRPCExecOne(const Value& req) {
    Object rpc_result;

    JSONRequest jreq;
    try {
        jreq.parse(req);

        Value result = tableRPC.execute(jreq.strMethod, jreq.params);
        rpc_result = JSONRPCReplyObj(result, Value::null, jreq.id);
    } catch (Object& objError) {
        rpc_result = JSONRPCReplyObj(Value::null, objError, jreq.id);
    } catch (std::exception& e) {
        rpc_result = JSONRPCReplyObj(Value::null, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
    }

    return rpc_result;
}

static string JSONRPCExecBatch(const Array& vReq) {
    Array ret;
    for (unsigned int reqIdx = 0; reqIdx < vReq.size(); reqIdx++)
        ret.push_back(JSONRPCExecOne(vReq[reqIdx]));

    return write_string(Value(ret), false) + "\n";
}

void ServiceConnection(AcceptedConnection *conn) {
    bool fRun = true;
    while (fRun && !ShutdownRequested()) {
        int nProto = 0;
        map<string, string> mapHeaders;
        string strRequest, strMethod, strURI;

        // Read HTTP request line
        if (!ReadHTTPRequestLine(conn->stream(), nProto, strMethod, strURI))
            break;

        // Read HTTP message headers and body
        ReadHTTPMessage(conn->stream(), mapHeaders, strRequest, nProto);

        if (strURI != "/") {
            conn->stream() << HTTPReply(HTTP_NOT_FOUND, "", false) << flush;
            break;
        }

        // Check authorization
        if (mapHeaders.count("authorization") == 0) {
            conn->stream() << HTTPReply(HTTP_UNAUTHORIZED, "", false) << flush;
            break;
        }
        if (!HTTPAuthorized(mapHeaders)) {
            LogPrint("INFO","ThreadRPCServer incorrect password attempt from %s\n", conn->peer_address_to_string());
            /* Deter brute-forcing short passwords.
                If this results in a DoS the user really
                shouldn't have their RPC port exposed. */
            if (SysCfg().GetArg("-rpcpassword", "").size() < 20)
                MilliSleep(250);

            conn->stream() << HTTPReply(HTTP_UNAUTHORIZED, "", false) << flush;
            break;
        }

        // disable http keepalive for client-wallet connection to bypass connection threading bugs
        // if (mapHeaders["connection"] == "close")
            fRun = false;

        JSONRequest jreq;
        try {
            // Parse request
            Value valRequest;
            if (!read_string(strRequest, valRequest))
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

            conn->stream() << HTTPReply(HTTP_OK, strReply, fRun) << flush;
        } catch (Object& objError) {
            ErrorReply(conn->stream(), objError, jreq.id);
            break;
        } catch (std::exception& e) {
            ErrorReply(conn->stream(), JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
            break;
        }
    }
}

json_spirit::Value CRPCTable::execute(const string &strMethod, const json_spirit::Array &params) const {
    // Find method
    const CRPCCommand *pcmd = tableRPC[strMethod];
    if (!pcmd)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found");

    if (pcmd->reqWallet && !pwalletMain)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found (disabled)");

    // Observe safe mode
    string strWarning = GetWarnings("rpc");
    if (strWarning != "" && !SysCfg().GetBoolArg("-disablesafemode", false) &&
        !pcmd->okSafeMode)
        throw JSONRPCError(RPC_FORBIDDEN_BY_SAFE_MODE, string("Safe mode: ") + strWarning);

    try {
        // Execute
        Value result;
        {
            if (pcmd->threadSafe)
                result = pcmd->actor(params, false);
            else if (!pwalletMain) {
                LOCK(cs_main);
                result = pcmd->actor(params, false);
            } else {
                LOCK2(cs_main, pwalletMain->cs_wallet);
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
        "\"method\": \"" + methodname + "\", \"params\": [" + args + "] }' -H 'Content-Type: application/json;' http://127.0.0.1:8332/\n";
}

const CRPCTable tableRPC;
