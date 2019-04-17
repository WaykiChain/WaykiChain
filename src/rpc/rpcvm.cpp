// Copyright (c) 2017-2019 WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "txdb.h"

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "syncdatadb.h"

#include "configuration.h"
#include "miner.h"
#include "main.h"
#include "vm/script.h"
#include "vm/vmrunenv.h"
#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"


using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

const int MAX_RPC_SIG_STR_LEN = 65 * 1024; // 65K

static bool FindKeyId(CAccountViewCache *pAccountView, string const &addr, CKeyID &keyId) {
    // first, try to parse regId 
    CRegID regId(addr);
    if (!regId.IsEmpty()) {
         keyId = regId.GetKeyID(*pAccountView);
         if (!keyId.IsEmpty()) {
             return true;
         }
    }

    // parse keyId from addr
    keyId = CKeyID(addr);
    return !keyId.IsEmpty();    
}

Value vmexecutescript(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
            "vmexecutescript \"addr\" \"script_path\"\n"
            "\nexecutes the script in vm simulator, and then returns the executing status.\n"
            "\nthe execution include registercontracttx and callcontracttx.\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) contract owner address from this wallet\n"
            "2.\"script_path\": (string required), the file path of the app script\n"
            "3.\"arguments\": (string, optional) contract method invoke content (Hex encode required)\n"
            "4.\"fee\": (numeric, optional) fee pay for miner, default is 110010000\n"
            "\nResult vm execute detail\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("vmexecutescript","\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/script.lua\"\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("vmexecutescript", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/script.lua\"\n"));
    }

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    std::tuple<bool, string> syntax = CVmlua::CheckScriptSyntax(luaScriptFilePath.c_str());
    bool bOK = std::get<0>(syntax);
    if (!bOK)
        throw JSONRPCError(RPC_INVALID_PARAMS, std::get<1>(syntax));

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw JSONRPCError(RPC_INVALID_PARAMS, "Open script file (" + luaScriptFilePath + ") error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    if (lSize <= 0 || lSize > kContractScriptMaxSize) { // contract script file size must be <= 64 KB)
        fclose(file);
        throw JSONRPCError(RPC_INVALID_PARAMS, (lSize == -1) ? "File size is unknown" : ((lSize == 0) ? "File is empty" : "File size exceeds 64 KB limit."));
    }

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }
    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);  //及时释放
        fclose(file);  //及时关闭
        throw runtime_error("read script file error");
    } else {
        fclose(file); //使用完关闭文件
    }

    CVmScript vmScript;
    vmScript.GetRom().insert(vmScript.GetRom().end(), buffer, buffer + lSize);

    if (buffer) {
        free(buffer);
    }

    uint64_t totalFee = 110010000;
    if (params.size() > 3) {
        totalFee = params[3].get_uint64();
    }

    uint64_t nDefaultFee = SysCfg().GetTxFee();

    if (totalFee < nDefaultFee * 2) {
        char errorMsg[100] = {'\0'};
        sprintf(errorMsg, "input fee smaller than mintxfee*2: %ld sawi", nDefaultFee * 2);
        throw JSONRPCError(RPC_INSUFFICIENT_FEE, errorMsg);
    }

    vector<unsigned char> vscript;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << vmScript;
    vscript.assign(ds.begin(), ds.end());

    CTransactionDBCache txCacheTemp(*pTxCacheTip, true);
    CAccountViewCache acctViewTemp(*pAccountViewTip, true);
    CScriptDBViewCache scriptDBViewTemp(*pScriptDBTip, true);
    CValidationState state;
    CTxUndo txundo;

    //get keyid
    CKeyID srcKeyid;
    if (!FindKeyId(&acctViewTemp, params[0].get_str(), srcKeyid)) {
        throw runtime_error("parse addr failed\n");
    }
    CUserID srcUserId   = srcKeyid;

    //balance
    CAccount account;

    uint64_t balance = 0;
    if (acctViewTemp.GetAccount(srcUserId, account)) {
        balance = account.GetRawBalance();
    }

    if (!account.IsRegistered()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "in registercontracttx Error: Account is not registered.");
    }
    if (!pwalletMain->HaveKey(srcKeyid)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "in registercontracttx Error: WALLET file is not correct.");
    }
    if (balance < totalFee) {
        throw JSONRPCError(RPC_WALLET_ERROR, "in registercontracttx Error: Account balance is insufficient.");
    }

    CRegID srcRegId;
    acctViewTemp.GetRegId(srcKeyid, srcRegId);


    int nFuelRate = GetElementForBurn(chainActive.Tip());
    Object registerContractTxObj;
    EnsureWalletIsUnlocked();
    int newHeight = chainActive.Tip()->nHeight + 1;    
    assert(pwalletMain != NULL);
    {
        CRegisterContractTx tx;

        tx.regAcctId = srcRegId;
        tx.script    = vscript;
        tx.llFees    = nDefaultFee;
        tx.nRunStep  = vscript.size();
        tx.nValidHeight = newHeight;

        if (!pwalletMain->Sign(srcKeyid, tx.SignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "registercontracttx Error: Sign failed.");
        }

        if (!tx.ExecuteTx(1, acctViewTemp, state, txundo, newHeight,
                                        txCacheTemp, scriptDBViewTemp)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Registercontracttx Error: executetx failed.");
        }

        registerContractTxObj.push_back(Pair("script_size", vscript.size()));
        registerContractTxObj.push_back(Pair("used_fuel", tx.GetFuel(nFuelRate)));
    }

    CRegID appId(newHeight, 1); //App RegId
    uint64_t amount = 0;

    vector<unsigned char> arguments;
    if (params.size() > 2) {
        arguments = ParseHex(params[2].get_str());
    }

    std::shared_ptr<CContractTx> contractTx_ptr = std::make_shared<CContractTx>();
    CContractTx &contractTx = *contractTx_ptr;

    {
        //balance
        CAccount secureAcc;

        if (!scriptDBViewTemp.HaveScript(appId)) {
            throw runtime_error(tinyformat::format("in callcontracttx : appId %s not exist\n", appId.ToString()));
        }
        contractTx.nTxType   = CONTRACT_TX;
        contractTx.srcRegId  = srcRegId;
        contractTx.desUserId = appId;
        contractTx.llValues  = amount;
        contractTx.llFees    = totalFee - nDefaultFee;
        contractTx.arguments = arguments;
        contractTx.nValidHeight = newHeight;

        vector<unsigned char> signature;
        if (!pwalletMain->Sign(srcKeyid, contractTx.SignatureHash(), contractTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "callcontracttx Error: Sign failed.");
        }

        if (!contractTx.ExecuteTx(2, acctViewTemp, state, txundo, chainActive.Tip()->nHeight + 1,
                                        txCacheTemp, scriptDBViewTemp)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "callcontracttx Error: executetx failed.");
        }
    }

    Object callContractTxObj;

    callContractTxObj.push_back(Pair("run_steps", contractTx.nRunStep));
    callContractTxObj.push_back(Pair("used_fuel", contractTx.GetFuel(contractTx.nFuelRate)));

    Object retObj;
    retObj.push_back(Pair("fuel_rate", nFuelRate));
    retObj.push_back(Pair("register_contract_tx", registerContractTxObj));
    retObj.push_back(Pair("call_contract_tx", callContractTxObj));

    return retObj;    
}

