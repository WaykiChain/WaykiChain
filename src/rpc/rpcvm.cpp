// Copyright (c) 2017-2019 WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "tx/txdb.h"

#include "commons/base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "miner/miner.h"
#include "util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "syncdatadb.h"

#include "configuration.h"
#include "main.h"
#include "vm/script.h"
#include "vm/vmrunenv.h"
#include <algorithm>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"


using namespace std;
using namespace json_spirit;


static const int REG_CONT_TX_FEE_MIN = 1 * COIN;

static bool FindKeyId(CAccountViewCache *pAccountView, string const &addr, CKeyID &keyId) {
    // first, try to parse regId
    CRegID regId(addr);
    if (!regId.IsEmpty()) {
         keyId = regId.GetKeyId(*pAccountView);
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
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::get<1>(syntax));

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Open script file (" + luaScriptFilePath + ") error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    if (lSize <= 0 || lSize > kContractScriptMaxSize) { // contract script file size must be <= 64 KB)
        fclose(file);
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            (lSize == -1) ? "File size is unknown"
                          : ((lSize == 0) ? "File is empty" : "File size exceeds 64 KB limit"));
    }

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }
    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);
        fclose(file);
        throw runtime_error("read script file error");
    } else {
        fclose(file);
    }

    CVmScript vmScript;
    vmScript.GetRom().insert(vmScript.GetRom().end(), buffer, buffer + lSize);

    if (buffer) {
        free(buffer);
    }

    vector<unsigned char> vscript;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << vmScript;
    vscript.assign(ds.begin(), ds.end());

    uint64_t nDefaultFee = SysCfg().GetTxFee();
    int nFuelRate = GetElementForBurn(chainActive.Tip());
    uint64_t regFee = std::max((int)ceil(vscript.size() / 100) * nFuelRate, REG_CONT_TX_FEE_MIN);
    uint64_t minFee = regFee + nDefaultFee;

    uint64_t totalFee = minFee + 10000000; // set default totalFee
    if (params.size() > 3) {
        totalFee = params[3].get_uint64();
    }


    if (totalFee < minFee) {
        throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                           strprintf("input fee could not smaller than: %ld sawi", minFee));
    }

    CTransactionDBCache txCacheTemp(*pTxCacheTip);
    CAccountViewCache acctViewTemp(*pAccountViewTip);
    CScriptDBViewCache scriptDBViewTemp(*pScriptDBTip);
    CValidationState state;
    CTxUndo txundo;

    CKeyID srcKeyid;
    if (!FindKeyId(&acctViewTemp, params[0].get_str(), srcKeyid)) {
        throw runtime_error("parse addr failed\n");
    }

    CUserID srcUserId   = srcKeyid;
    CAccount account;

    uint64_t balance = 0;
    if (acctViewTemp.GetAccount(srcUserId, account)) {
        balance = account.GetBCoinBalance();
    }

    if (!account.IsRegistered()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
    }
    if (!pwalletMain->HaveKey(srcKeyid)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
    }
    if (balance < totalFee) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
    }

    CRegID srcRegId;
    acctViewTemp.GetRegId(srcKeyid, srcRegId);

    Object registerContractTxObj;
    EnsureWalletIsUnlocked();
    int newHeight = chainActive.Tip()->nHeight + 1;
    assert(pwalletMain != NULL);
    {
        CRegisterContractTx tx;

        tx.regAcctId = srcRegId;
        tx.script    = vscript;
        tx.llFees    = regFee;
        tx.nRunStep  = vscript.size();
        tx.nValidHeight = newHeight;

        if (!pwalletMain->Sign(srcKeyid, tx.SignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        if (!tx.ExecuteTx(1, acctViewTemp, state, txundo, newHeight,
                                        txCacheTemp, scriptDBViewTemp)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Executetx register contract failed");
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
        CAccount secureAcc;

        if (!scriptDBViewTemp.HaveScript(appId)) {
            throw runtime_error(tinyformat::format("AppId %s is not exist\n", appId.ToString()));
        }
        contractTx.nTxType   = CONTRACT_TX;
        contractTx.srcRegId  = srcRegId;
        contractTx.desUserId = appId;
        contractTx.bcoinBalance  = amount;
        contractTx.llFees    = totalFee - regFee;
        contractTx.arguments = arguments;
        contractTx.nValidHeight = newHeight;

        vector<unsigned char> signature;
        if (!pwalletMain->Sign(srcKeyid, contractTx.SignatureHash(), contractTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        if (!contractTx.ExecuteTx(2, acctViewTemp, state, txundo, chainActive.Tip()->nHeight + 1,
                                        txCacheTemp, scriptDBViewTemp)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Executetx  contract failed");
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
