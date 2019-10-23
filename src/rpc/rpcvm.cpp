// Copyright (c) 2017-2019 WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
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
#include "vm/luavm/luavmrunenv.h"
#include <algorithm>

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_reader.h"


using namespace std;
using namespace json_spirit;


static const uint32_t LCONTRACT_DEPLOY_TX_FEE_MIN = 1 * COIN;

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
    if (fHelp || params.size() < 2 || params.size() > 5) {
        throw runtime_error(
            "vmexecutescript \"addr\" \"script_path\" [\"arguments\"] [amount] [symbol:fee:unit]\n"
            "\nexecutes the script in vm simulator, and then returns the executing status.\n"
            "\nthe execution include submitcontractdeploytx and submitcontractcalltx.\n"
            "\nArguments:\n"
            "1.\"addr\":                (string required) contract owner address from this wallet\n"
            "2.\"script_path\":         (string required), the file path of the app script\n"
            "3.\"arguments\":           (string, optional) contract method invoke content (Hex encode required)\n"
            "4.\"amount\":              (numeric, optional) amount of WICC to send to app account\n"
            "5.\"symbol:fee:unit\":     (string:numeric:string, optional) fee paid for miner, default is WICC:110010000:sawi\n"
            "\nResult vm execute detail\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("vmexecutescript","\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/script.lua\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("vmexecutescript", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"/tmp/lua/script.lua\""));
    }

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, LUA_CONTRACT_LOCATION_PREFIX.size(), LUA_CONTRACT_LOCATION_PREFIX.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    std::tuple<bool, string> syntax = CLuaVM::CheckScriptSyntax(luaScriptFilePath.c_str());
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

    if (lSize <= 0 || lSize > MAX_CONTRACT_CODE_SIZE) { // contract script file size must be <= 64 KB)
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

    CLuaContract contract;

    contract.code = string(buffer, lSize);

    if (buffer) {
        free(buffer);
    }

    uint64_t amount = 0;
    if (params.size() > 3)
        amount = AmountToRawValue(params[3]);

    uint64_t regMinFee;
    uint64_t invokeMinFee;
    if (!GetTxMinFee(LCONTRACT_DEPLOY_TX, chainActive.Height(), SYMB::WICC, regMinFee) ||
        !GetTxMinFee(LCONTRACT_INVOKE_TX, chainActive.Height(), SYMB::WICC, invokeMinFee))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Get tx min fee failed");

    uint64_t minFee   = regMinFee + invokeMinFee;
    uint64_t totalFee = regMinFee + invokeMinFee * 10;  // set default totalFee
    if (params.size() > 4) {
        ComboMoney feeIn = RPC_PARAM::GetFee(params, 4, LCONTRACT_DEPLOY_TX);
        assert(feeIn.symbol == SYMB::WICC);
        totalFee = feeIn.GetSawiAmount();
    }

    if (totalFee < minFee) {
        throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                           strprintf("input fee could not smaller than: %ld sawi", minFee));
    }

    CBlockIndex *pTip      = chainActive.Tip();
    uint32_t fuelRate      = GetElementForBurn(pTip);
    uint32_t blockTime     = pTip->GetBlockTime();
    uint32_t prevBlockTime = pTip->pprev != nullptr ? pTip->pprev->GetBlockTime() : pTip->GetBlockTime();

    auto spCW = std::make_shared<CCacheWrapper>(pCdMan);
    CKeyID srcKeyId;
    if (!FindKeyId(&spCW->accountCache, params[0].get_str(), srcKeyId)) {
        throw runtime_error("parse addr failed\n");
    }

    CUserID srcUserId = srcKeyId;
    CAccount account;

    uint64_t balance = 0;
    if (spCW->accountCache.GetAccount(srcUserId, account)) {
        balance = account.GetToken(SYMB::WICC).free_amount;
    }

    if (!account.HaveOwnerPubKey()) {
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

    Object DeployContractTxObj;
    EnsureWalletIsUnlocked();
    int32_t newHeight = chainActive.Height() + 1;
    assert(pWalletMain != nullptr);
    {
        size_t contract_size = contract.GetContractSize();
        CLuaContractDeployTx tx;
        tx.txUid            = srcRegId;
        tx.contract         = contract;
        tx.llFees           = regMinFee;
        tx.nRunStep         = contract_size;
        tx.valid_height     = newHeight;

        if (!pWalletMain->Sign(srcKeyId, tx.ComputeSignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        CValidationState state;
        CTxExecuteContext context(newHeight, 1, fuelRate, blockTime, prevBlockTime, spCW.get(), &state);
        if (!tx.ExecuteTx(context)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Executetx register contract failed");
        }

        DeployContractTxObj.push_back(Pair("contract_size", contract_size));
        DeployContractTxObj.push_back(Pair("used_fuel", tx.GetFuel(newHeight, fuelRate)));
    }

    CRegID appId(newHeight, 1); //App RegId

    vector<uint8_t> arguments;
    if (params.size() > 2) {
        arguments = ParseHex(params[2].get_str());
    }

    CLuaContractInvokeTx contractInvokeTx;

    {
        if (!spCW->contractCache.HaveContract(appId)) {
            throw runtime_error(tinyformat::format("AppId %s is not exist\n", appId.ToString()));
        }
        contractInvokeTx.nTxType      = LCONTRACT_INVOKE_TX;
        contractInvokeTx.txUid        = srcRegId;
        contractInvokeTx.app_uid      = appId;
        contractInvokeTx.coin_amount  = amount;
        contractInvokeTx.llFees       = totalFee - regMinFee;
        contractInvokeTx.arguments    = string(arguments.begin(), arguments.end());
        contractInvokeTx.valid_height = newHeight;

        vector<uint8_t> signature;
        if (!pWalletMain->Sign(srcKeyId, contractInvokeTx.ComputeSignatureHash(), contractInvokeTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        CValidationState state;
        CTxExecuteContext context(chainActive.Height() + 1, 2, fuelRate, blockTime, prevBlockTime, spCW.get(), &state);
        if (!contractInvokeTx.ExecuteTx(context)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Executetx contract failed");
        }
    }

    Object callContractTxObj;

    callContractTxObj.push_back(Pair("run_steps", contractInvokeTx.nRunStep));
    callContractTxObj.push_back(Pair("used_fuel", contractInvokeTx.GetFuel(newHeight, contractInvokeTx.nFuelRate)));

    Object retObj;
    retObj.push_back(Pair("fuel_rate",              (int32_t)fuelRate));
    retObj.push_back(Pair("register_contract_tx",   DeployContractTxObj));
    retObj.push_back(Pair("call_contract_tx",       callContractTxObj));

    return retObj;
}
