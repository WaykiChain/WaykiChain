// Copyright (c) 2017-2019 WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "init.h"
#include "net.h"
#include "miner/miner.h"
#include "commons/util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/txdb.h"
#include "persistence/contractdb.h"

#include "config/configuration.h"
#include "main.h"
#include "vm/luavm/script.h"
#include "vm/luavm/vmrunenv.h"
#include <algorithm>

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"


using namespace std;
using namespace json_spirit;


static const int CONTRACT_DEPLOY_TX_FEE_MIN = 1 * COIN;

static bool FindKeyId(CAccountDBCache *pAccountView, string const &addr, CKeyID &keyId) {
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

    string contractScript;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << vmScript;
    contractScript.assign(ds.begin(), ds.end());

    uint64_t nDefaultFee = SysCfg().GetTxFee();
    int nFuelRate = GetElementForBurn(chainActive.Tip());
    uint64_t regFee = std::max((int)ceil(contractScript.size() / 100) * nFuelRate, CONTRACT_DEPLOY_TX_FEE_MIN);
    uint64_t minFee = regFee + nDefaultFee;

    uint64_t totalFee = minFee + 10000000; // set default totalFee
    if (params.size() > 3) {
        totalFee = params[3].get_uint64();
    }


    if (totalFee < minFee) {
        throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                           strprintf("input fee could not smaller than: %ld sawi", minFee));
    }

    auto spCW = std::make_shared<CCacheWrapper>();
    spCW->accountCache.SetBaseView(pCdMan->pAccountCache);
    spCW->txCache.SetBaseView(pCdMan->pTxCache);
    spCW->contractCache.SetBaseView(pCdMan->pContractCache);
    spCW->delegateCache.SetBaseView(pCdMan->pDelegateCache);
    spCW->cdpCache.SetBaseView(pCdMan->pCdpCache);

    CKeyID srcKeyId;
    if (!FindKeyId(&spCW->accountCache, params[0].get_str(), srcKeyId)) {
        throw runtime_error("parse addr failed\n");
    }

    CUserID srcUserId = srcKeyId;
    CAccount account;

    uint64_t balance = 0;
    if (spCW->accountCache.GetAccount(srcUserId, account)) {
        balance = account.GetFreeBcoins();
    }

    if (!account.IsRegistered()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
    }
    if (!pWalletMain->HaveKey(srcKeyId)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
    }
    if (balance < totalFee) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
    }

    CRegID srcRegId;
    spCW->accountCache.GetRegId(srcKeyId, srcRegId);

    Object registerContractTxObj;
    EnsureWalletIsUnlocked();
    int newHeight = chainActive.Tip()->nHeight + 1;
    assert(pWalletMain != nullptr);
    {
        CContractDeployTx tx;
        tx.txUid          = srcRegId;
        tx.contractScript = contractScript;
        tx.llFees         = regFee;
        tx.nRunStep       = contractScript.size();
        tx.nValidHeight   = newHeight;

        if (!pWalletMain->Sign(srcKeyId, tx.ComputeSignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        CValidationState state;
        if (!tx.ExecuteTx(newHeight, 1, *spCW, state)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Executetx register contract failed");
        }

        registerContractTxObj.push_back(Pair("script_size", contractScript.size()));
        registerContractTxObj.push_back(Pair("used_fuel", tx.GetFuel(nFuelRate)));
    }

    CRegID appId(newHeight, 1); //App RegId
    uint64_t amount = 0;

    vector<unsigned char> arguments;
    if (params.size() > 2) {
        arguments = ParseHex(params[2].get_str());
    }

    CContractInvokeTx contractInvokeTx;

    {
        if (!spCW->contractCache.HaveContractScript(appId)) {
            throw runtime_error(tinyformat::format("AppId %s is not exist\n", appId.ToString()));
        }
        contractInvokeTx.nTxType      = CONTRACT_INVOKE_TX;
        contractInvokeTx.txUid        = srcRegId;
        contractInvokeTx.appUid       = appId;
        contractInvokeTx.bcoins       = amount;
        contractInvokeTx.llFees       = totalFee - regFee;
        contractInvokeTx.arguments    = arguments;
        contractInvokeTx.nValidHeight = newHeight;

        vector<unsigned char> signature;
        if (!pWalletMain->Sign(srcKeyId, contractInvokeTx.ComputeSignatureHash(), contractInvokeTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        CValidationState state;
        if (!contractInvokeTx.ExecuteTx(chainActive.Tip()->nHeight + 1, 2, *spCW, state)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Executetx  contract failed");
        }
    }

    Object callContractTxObj;

    callContractTxObj.push_back(Pair("run_steps", contractInvokeTx.nRunStep));
    callContractTxObj.push_back(Pair("used_fuel", contractInvokeTx.GetFuel(contractInvokeTx.nFuelRate)));

    Object retObj;
    retObj.push_back(Pair("fuel_rate", nFuelRate));
    retObj.push_back(Pair("register_contract_tx", registerContractTxObj));
    retObj.push_back(Pair("call_contract_tx", callContractTxObj));

    return retObj;
}
