// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcclient.h"

#include "commons/json/json_spirit_writer_template.h"
#include "commons/util/util.h"
#include "config/chainparams.h"  // for Params().RPCPort()
#include "config/configuration.h"
#include "main.h"
#include "rpcprotocol.h"
#include "tx/tx.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace json_spirit;

Object CallRPC(const string& strMethod, const Array& params) {
    if (SysCfg().GetArg("-rpcuser", "") == "" && SysCfg().GetArg("-rpcpassword", "") == "")
        throw runtime_error(strprintf(
            _("You must set rpcpassword=<password> in the configuration file:\n%s\n"
              "If the file does not exist, create it with owner-readable-only file permissions."),
                GetConfigFile().string().c_str()));

    // Connect to localhost
    bool fUseSSL = SysCfg().GetBoolArg("-rpcssl", false);
    asio::io_service io_service;
    ssl::context context(io_service, ssl::context::sslv23);
    context.set_options(ssl::context::no_sslv2);
    asio::ssl::stream<asio::ip::tcp::socket> sslStream(io_service, context);
    SSLIOStreamDevice<asio::ip::tcp> d(sslStream, fUseSSL);
    iostreams::stream< SSLIOStreamDevice<asio::ip::tcp> > stream(d);

    bool fWait = SysCfg().GetBoolArg("-rpcwait", false); // -rpcwait means try until server has started
    do {
        bool fConnected = d.connect(SysCfg().GetArg("-rpcconnect", "127.0.0.1"),
            SysCfg().GetArg("-rpcport", itostr(SysCfg().RPCPort())));
        if (fConnected) break;
        if (fWait)
            MilliSleep(1000);
        else
            throw runtime_error("couldn't connect to server... pls wait for a while or check \"rpcserver=1\" setting.");
    } while (fWait);

    // HTTP basic authentication
    string strUserPass64 = EncodeBase64(SysCfg().GetArg("-rpcuser", "") + ":"
        + SysCfg().GetArg("-rpcpassword", ""));
    map<string, string> mapRequestHeaders;
    mapRequestHeaders["Authorization"] = string("Basic ") + strUserPass64;

    // Send request
    string strRequest = JSONRPCRequest(strMethod, params, 1);
    string strPost    = HTTPPost(strRequest, mapRequestHeaders);
    stream << strPost << flush;

    // Receive HTTP reply status
    int nProto  = 0;
    int nStatus = ReadHTTPStatus(stream, nProto);

    // Receive HTTP reply message headers and body
    map<string, string> mapHeaders;
    string strReply;
    ReadHTTPMessage(stream, mapHeaders, strReply, nProto);

    if (nStatus == HTTP_UNAUTHORIZED)
        throw runtime_error("incorrect rpcuser or rpcpassword (authorization failed)");
    else if (nStatus >= 400 && nStatus != HTTP_BAD_REQUEST && nStatus != HTTP_NOT_FOUND && nStatus != HTTP_INTERNAL_SERVER_ERROR)
        throw runtime_error(strprintf("server returned HTTP error %d", nStatus));
    else if (strReply.empty())
        throw runtime_error("no response from server");

    // Parse reply
    Value valReply;
    if (!read_string(strReply, valReply))
        throw runtime_error("couldn't parse reply from server");

    const Object& reply = valReply.get_obj();
    if (reply.empty())
        throw runtime_error("expected reply to have result, error and id properties");

    return reply;
}

template <typename T>
void ConvertTo(Value& value, bool fAllowNull = false) {
    if (fAllowNull && value.type() == null_type)
        return;
    if (value.type() == str_type) {
        // reinterpret string as unquoted json value
        Value value2;
        string strJSON = value.get_str();
        if (!read_string(strJSON, value2))
            throw runtime_error(string("Error parsing JSON:") + strJSON);
        ConvertTo<T>(value2, fAllowNull);
        value = value2;
    } else {
        value = value.get_value<T>();
    }
}

// Convert strings to command-specific RPC representation
Array RPCConvertValues(const string &strMethod, const vector<string> &strParams) {
    Array params;
    for (const auto &param : strParams)
        params.push_back(param);

    int n = params.size();

    //
    // Special case non-string parameter types
    //
    if (strMethod == "stop"                   && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "getaddednodeinfo"       && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "setgenerate"            && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "setgenerate"            && n > 1) ConvertTo<int64_t>(params[1]);

    if (strMethod == "walletpassphrase"       && n > 1) ConvertTo<int64_t>(params[1]);

    if (strMethod == "getblock"               && n > 1) ConvertTo<bool>(params[1]);
    if (strMethod == "getchaininfo"           && n > 0) ConvertTo<int32_t>(params[0]);
    if (strMethod == "getchaininfo"           && n > 1) ConvertTo<int32_t>(params[1]);
    if (strMethod == "verifychain"            && n > 0) ConvertTo<int64_t>(params[0]);
    if (strMethod == "verifychain"            && n > 1) ConvertTo<int64_t>(params[1]);
    if (strMethod == "getrawmempool"          && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "getnewaddr"             && n > 0) ConvertTo<bool>(params[0]);


    if (strMethod == "submitdelegatevotetx"         && n > 1) ConvertTo<Array>(params[1]);
    if (strMethod == "submitdelegatevotetx"         && n > 3) ConvertTo<int32_t>(params[3]);

    if (strMethod == "submitcontractdeploytx_r2"       && n > 3) ConvertTo<int32_t>(params[3]);

    if (strMethod == "submitcontractcalltx_r2"         && n > 3) ConvertTo<int64_t>(params[3]);
    if (strMethod == "submitcontractcalltx_r2"         && n > 5) ConvertTo<int32_t>(params[5]);

    if (strMethod == "submitucontractdeploytx"  && n > 3) ConvertTo<int32_t>(params[3]);

    if (strMethod == "submitucontractcalltx"    && n > 5) ConvertTo<int32_t>(params[5]);

    if (strMethod == "listaddr"               && n > 1) ConvertTo<bool>(params[1]);
    if (strMethod == "disconnectblock"        && n > 0) ConvertTo<int32_t>(params[0]);

    if (strMethod == "listcontracts"          && n > 0) ConvertTo<bool>(params[0]);
    if (strMethod == "getblock"               && n > 0) { if (params[0].get_str().size() < 32) ConvertTo<int32_t>(params[0]); }
    if (strMethod == "getblockundo"           && n > 0) { if (params[0].get_str().size() < 32) ConvertTo<int32_t>(params[0]); }

    /********************************************************************************************************************/
    if (strMethod == "getcontractdata"        && n > 2) ConvertTo<bool>(params[2]);

    if (strMethod == "listtx"                 && n > 0) ConvertTo<int32_t>(params[0]);
    if (strMethod == "listtx"                 && n > 1) ConvertTo<int32_t>(params[1]);
    if (strMethod == "listdelegates"          && n > 0) ConvertTo<int32_t>(params[0]);

    if (strMethod == "invalidateblock"        && n > 0) { if (params[0].get_str().size() < 32) ConvertTo<int32_t>(params[0]); }

    /** for mining */
    if (strMethod == "getminedblocks"           && n > 0) ConvertTo<int64_t>(params[0]);
    if (strMethod == "getminerbyblocktime"      && n > 0) ConvertTo<int64_t>(params[0]);
    if (strMethod == "getminerbyblocktime"      && n > 1) ConvertTo<int64_t>(params[1]);

    /* for dex */
    if (strMethod == "submitdexbuylimitordertx"     && n > 3) ConvertTo<int64_t>(params[3]);
    if (strMethod == "submitdexbuylimitordertx"     && n > 4) ConvertTo<int64_t>(params[4]);

    if (strMethod == "submitdexselllimitordertx"    && n > 3) ConvertTo<int64_t>(params[3]);
    if (strMethod == "submitdexselllimitordertx"     && n > 4) ConvertTo<int64_t>(params[4]);

    if (strMethod == "submitdexbuymarketordertx"     && n > 3) ConvertTo<int64_t>(params[3]);

    if (strMethod == "submitdexsellmarketordertx"     && n > 3) ConvertTo<int64_t>(params[3]);

    if (strMethod == "gendexoperatorordertx"     && n > 5) ConvertTo<int64_t>(params[5]);
    if (strMethod == "gendexoperatorordertx"     && n > 6) ConvertTo<int64_t>(params[6]);
    if (strMethod == "gendexoperatorordertx"     && n > 8) ConvertTo<int64_t>(params[8]);
    if (strMethod == "gendexoperatorordertx"     && n > 9) ConvertTo<int64_t>(params[9]);

    if (strMethod == "submitdexsettletx"            && n > 1) ConvertTo<Array>(params[1]);

    if (strMethod == "submitdexoperatorregtx"      && n > 6) ConvertTo<int64_t>(params[6]);
    if (strMethod == "submitdexoperatorregtx"      && n > 7) ConvertTo<int64_t>(params[7]);
    if (strMethod == "submitdexopupdatetx"   && n > 1) ConvertTo<int32_t>(params[1]);
    if (strMethod == "submitdexopupdatetx"   && n > 2) ConvertTo<int32_t>(params[2]);

    if (strMethod == "listdexsysorders"              && n > 0) ConvertTo<int64_t>(params[0]);

    if (strMethod == "listdexorders"              && n > 0) ConvertTo<int64_t>(params[0]);
    if (strMethod == "listdexorders"              && n > 1) ConvertTo<int64_t>(params[1]);
    if (strMethod == "listdexorders"              && n > 2) ConvertTo<int64_t>(params[2]);
    if (strMethod == "getdexoperator"            && n > 0) ConvertTo<int64_t>(params[0]);

    if (strMethod == "startcommontpstest"       && n > 0)    ConvertTo<int64_t>(params[0]);
    if (strMethod == "startcommontpstest"       && n > 1)    ConvertTo<int64_t>(params[1]);
    if (strMethod == "startcontracttpstest"     && n > 1)    ConvertTo<int64_t>(params[1]);
    if (strMethod == "startcontracttpstest"     && n > 2)    ConvertTo<int64_t>(params[2]);
    if (strMethod == "getblockfailures"         && n > 0)    ConvertTo<int32_t>(params[0]);

    /* for cdp */
    if (strMethod == "submitpricefeedtx"        && n > 1) ConvertTo<Array>(params[1]);

    if (strMethod == "submitcdpredeemtx"        && n > 2) ConvertTo<int64_t>(params[2]);
    if (strMethod == "submitcdpredeemtx"        && n > 3) ConvertTo<int64_t>(params[3]);

    if (strMethod == "submitcdpliquidatetx"     && n > 2) ConvertTo<int64_t>(params[2]);

    if (strMethod == "submitassetissuetx"       && n > 4) ConvertTo<int64_t>(params[4]);
    if (strMethod == "submitassetissuetx"       && n > 5) ConvertTo<bool>(params[5]);

    if (strMethod == "submitparamgovernproposal"   && n > 2) ConvertTo<int64_t>(params[2]);
    if (strMethod == "submitparamgovernproposal"   && n > 3) ConvertTo<int64_t>(params[3]);

    if (strMethod == "submitcdpparamgovernproposal"   && n > 2) ConvertTo<int64_t>(params[2]);

    if (strMethod == "submitgovernorupdateproposal"   && n > 2) ConvertTo<int64_t>(params[2]);
    if (strMethod == "submitpasswordprooftx"  && n > 2) ConvertTo<int64_t>(params[2]);
    if (strMethod == "submitsendmultitx"  && n > 1) ConvertTo<Array>(params[1]);

    if (strMethod == "submitaxccoinproposal"   && n > 2) ConvertTo<int64_t>(params[2]);
    if (strMethod == "submitaxccoinproposal"   && n > 3) ConvertTo<int64_t>(params[3]);
    if (strMethod == "submitdiaissueproposal"   && n > 3) ConvertTo<int64_t>(params[3]);


    if (strMethod == "submitdexswitchproposal"   && n > 1) ConvertTo<int64_t>(params[1]);
    if (strMethod == "submitdexswitchproposal"   && n > 2) ConvertTo<int64_t>(params[2]);
    if (strMethod == "submitfeedcoinpairproposal"   && n>3) ConvertTo<int64_t>(params[3]);
    if (strMethod == "submitminerfeeproposal"   && n > 1) ConvertTo<int64_t>(params[1]);

    if (strMethod == "submittotalbpssizeupdateproposal"   && n > 1) ConvertTo<int64_t>(params[1]);
    if (strMethod == "submittotalbpssizeupdateproposal"   && n > 2) ConvertTo<int64_t>(params[2]);

    if (strMethod == "genutxomultisignaddr"   && n > 0) ConvertTo<int64_t>(params[0]);
    if (strMethod == "genutxomultisignaddr"   && n > 1) ConvertTo<int64_t>(params[1]);
    if (strMethod == "genutxomultisignaddr"   && n > 2) ConvertTo<Array>(params[2]);

    if (strMethod == "submitutxotransfertx"   && n > 2) ConvertTo<Array>(params[2]);
    if (strMethod == "submitutxotransfertx"   && n > 3) ConvertTo<Array>(params[3]);
    if (strMethod == "signtxraw"   && n > 1) ConvertTo<Array>(params[1]);


    if (strMethod == "genutxomultiinputcondhash"   && n > 0) ConvertTo<int64_t>(params[0]);
    if (strMethod == "genutxomultiinputcondhash"   && n > 1) ConvertTo<int64_t>(params[1]);
    if (strMethod == "genutxomultiinputcondhash"   && n > 3) ConvertTo<int64_t>(params[3]);
    if (strMethod == "genutxomultiinputcondhash"   && n > 4) ConvertTo<Array>(params[4]);

    if (strMethod == "submitaccountpermproposal"   && n > 2) ConvertTo<Array>(params[2]);
    if (strMethod == "submitassetpermproposal"     && n > 2) ConvertTo<Array>(params[2]);


    if (strMethod == "submitaxcinproposal"   && n > 5) ConvertTo<int64_t>(params[5]);

    if (strMethod == "submitaxcoutproposal"   && n > 3) ConvertTo<int64_t>(params[3]);



    /* vm functions work in vm simulator */
    if (strMethod == "luavm_executescript"          && n > 3) ConvertTo<int64_t>(params[3]);


    return params;
}

inline string&   recover_enter(string& str, const string& old_value, const string& new_value, bool need_replace = true)
{
    while(true)   {
        string::size_type pos(0);
        if(   (pos=str.find(old_value))!=string::npos   ){
            // str.erase(pos, old_value.size());
            // if(need_replace) str.insert(pos, new_value.size(), new_value);
            str.replace(pos, old_value.size(), new_value);
        }
        else {
           break;
        }
    }
    return   str;
}


int CommandLineRPC(int argc, char *argv[])
{
    string strPrint;
    int nRet = 0;
    try
    {
        // Skip switches
        while (argc > 1 && IsSwitchChar(argv[1][0]))
        {
            argc--;
            argv++;
        }

        // Method
        if (argc < 2)
            throw runtime_error("too few parameters");

        string strMethod = argv[1];

        // Parameters default to strings
        vector<string> strParams(&argv[2], &argv[argc]);
        Array params = RPCConvertValues(strMethod, strParams);

        // Execute
        Object reply = CallRPC(strMethod, params);

        // Parse reply
        const Value& result = find_value(reply, "result");
        const Value& error  = find_value(reply, "error");

        if (error.type() != null_type)
        {
            // Error
            strPrint = "error: " + write_string(error, false);
            int code = find_value(error.get_obj(), "code").get_int();
            nRet = abs(code);
        }
        else
        {
            // Result
            if (result.type() == null_type)
                strPrint = "";
            else if (result.type() == str_type)
                strPrint = result.get_str();
            else
                strPrint = write_string(result, true);
        }
    }
    catch (boost::thread_interrupted) {
        throw;
    }
    catch (std::exception& e) {
        strPrint = string("error: ") + e.what();
        nRet = abs(RPC_MISC_ERROR);
    }
    catch (...) {
        PrintExceptionContinue(NULL, "CommandLineRPC()");
        throw;
    }

    if (strPrint != "")
    {
       if(nRet != 0){
           recover_enter(strPrint, R"(\n)", "\n        ");
           recover_enter(strPrint, R"(\)", "", false);//delete "\"
           std::cerr << strPrint <<std::endl;
       } else {
           std::cout << strPrint <<std::endl;
       }
       //fprintf((nRet == 0 ? stdout : stderr), "%s\n", strPrint.c_str());
    }
    return nRet;
}

string HelpMessageCli(bool mainProgram)
{
    string strUsage;
    if (mainProgram) {
        strUsage += _("Options:") + "\n";
        strUsage += "  -?                     " + _("This help message") + "\n";
        strUsage += "  -conf=<file>           " + _("Specify configuration file (default: ") + IniCfg().GetCoinName() + ".conf)" + "\n";
        strUsage += "  -datadir=<dir>         " + _("Specify data directory") + "\n";
        strUsage += "  -nettype=<network>     " + _("Specify network type: main/test/regtest (default: main)") + "\n";
    } else {
        strUsage += _("RPC client options:") + "\n";
    }

    strUsage += "  -rpcconnect=<ip>       " + _("Send commands to node running on <ip> (default: 127.0.0.1)") + "\n";
    strUsage += "  -rpcport=<port>        " + _("Connect to JSON-RPC on <port> (default: 8332 or testnet: 18332)") + "\n";
    strUsage += "  -rpcwait               " + _("Wait for RPC server to start") + "\n";

    if (mainProgram) {
        strUsage += "  -rpcuser=<user>        " + _("Username for JSON-RPC connections") + "\n";
        strUsage += "  -rpcpassword=<pw>      " + _("Password for JSON-RPC connections") + "\n";

        strUsage += "\n" + _("SSL options: (see the coin Wiki for SSL setup instructions)") + "\n";
        strUsage += "  -rpcssl                " + _("Use OpenSSL (https) for JSON-RPC connections") + "\n";
    }

    return strUsage;
}