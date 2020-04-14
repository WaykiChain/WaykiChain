// Copyright (c) 2017-2019 WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "init.h"
#include "net.h"
#include "miner/miner.h"
#include "commons/util/util.h"
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

Value luavm_executescript(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 5) {
        throw runtime_error(
            "luavm_executescript \"addr\" \"script_path\" [\"arguments\"] [amount] [symbol:fee:unit]\n"
            "\nexecutes the script in vm simulator, and then returns the execution status.\n"
            "\nthe execution include submitcontractdeploytx_r2 and submitcontractcalltx_r2.\n"
            "\nArguments:\n"
            "1.\"addr\":                (string required) contract owner address from this wallet\n"
            "2.\"script_path\":         (string required), the file path of the app script\n"
            "3.\"arguments\":           (string, optional) contract method invoke content (Hex encode required)\n"
            "4.\"amount\":              (numeric, optional) amount of WICC to send to app account\n"
            "5.\"symbol:fee:unit\":     (string:numeric:string, optional) fee paid for miner, default is WICC:110010000:sawi\n"
            "\nResult vm execute detail\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("luavm_executescript","\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/script.lua\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("luavm_executescript", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"/tmp/lua/script.lua\""));
    }

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, LUA_CONTRACT_LOCATION_PREFIX.size(), LUA_CONTRACT_LOCATION_PREFIX.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");
    HeightType height = chainActive.Height();
    std::tuple<bool, string> syntax = CLuaVM::CheckScriptSyntax(luaScriptFilePath.c_str(), height);
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

    auto spCw = std::make_shared<CCacheWrapper>(pCdMan);
    uint64_t regMinFee;
    uint64_t invokeMinFee;
    if (!GetTxMinFee(*spCw, LCONTRACT_DEPLOY_TX, chainActive.Height(), SYMB::WICC, regMinFee) ||
        !GetTxMinFee(*spCw, LCONTRACT_INVOKE_TX, chainActive.Height(), SYMB::WICC, invokeMinFee))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Get tx min fee failed");

    regMinFee += 1 * COIN;
    uint64_t minFee   = regMinFee + invokeMinFee;
    uint64_t totalFee = regMinFee + invokeMinFee * 10;  // set default totalFee
    if (params.size() > 4) {
        ComboMoney feeIn = RPC_PARAM::GetFee(params, 4, LCONTRACT_DEPLOY_TX);
        assert(feeIn.symbol == SYMB::WICC);
        totalFee = feeIn.GetAmountInSawi();
    }

    if (totalFee < minFee) {
        throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                           strprintf("input fee could not smaller than: %ld sawi", minFee));
    }

    CBlockIndex *pTip      = chainActive.Tip();
    uint32_t fuelRate      = GetElementForBurn(pTip);
    uint32_t blockTime     = pTip->GetBlockTime();
    uint32_t prevBlockTime = pTip->pprev != nullptr ? pTip->pprev->GetBlockTime() : pTip->GetBlockTime();

    CKeyID srcKeyId;
    if (!FindKeyId(&spCw->accountCache, params[0].get_str(), srcKeyId)) {
        throw runtime_error("parse addr failed\n");
    }

    CUserID srcUserId = srcKeyId;
    CAccount account;

    uint64_t balance = 0;
    if (spCw->accountCache.GetAccount(srcUserId, account)) {
        balance = account.GetToken(SYMB::WICC).free_amount;
    }

    if (!account.HasOwnerPubKey()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
    }
    if (!pWalletMain->HasKey(srcKeyId)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
    }
    if (balance < totalFee) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
    }

    CRegID srcRegId;
    spCw->accountCache.GetRegId(srcKeyId, srcRegId);

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
        tx.fuel         = contract_size;
        tx.valid_height     = newHeight;

        if (!pWalletMain->Sign(srcKeyId, tx.GetHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        CValidationState state;
        CTxExecuteContext context(newHeight, 1, fuelRate, blockTime, prevBlockTime, spCw.get(), &state);
        if (!tx.CheckAndExecuteTx(context)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "CheckAndExecuteTx register contract failed");
        }

        DeployContractTxObj.push_back(Pair("contract_size", contract_size));
        DeployContractTxObj.push_back(Pair("used_fuel_fee", tx.GetFuelFee(newHeight, fuelRate)));
    }

    CRegID appId(newHeight, 1); //App RegId

    vector<uint8_t> arguments;
    if (params.size() > 2) {
        arguments = ParseHex(params[2].get_str());
    }

    CLuaContractInvokeTx contractInvokeTx;

    {
        if (!spCw->contractCache.HasContract(appId)) {
            throw runtime_error(strprintf("AppId %s is not exist\n", appId.ToString()));
        }
        contractInvokeTx.nTxType      = LCONTRACT_INVOKE_TX;
        contractInvokeTx.txUid        = srcRegId;
        contractInvokeTx.app_uid      = appId;
        contractInvokeTx.coin_amount  = amount;
        contractInvokeTx.llFees       = totalFee - regMinFee;
        contractInvokeTx.arguments    = string(arguments.begin(), arguments.end());
        contractInvokeTx.valid_height = newHeight;

        vector<uint8_t> signature;
        if (!pWalletMain->Sign(srcKeyId, contractInvokeTx.GetHash(), contractInvokeTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        CValidationState state;
        CTxExecuteContext context(chainActive.Height() + 1, 2, fuelRate, blockTime, prevBlockTime, spCw.get(), &state);
        if (!contractInvokeTx.CheckAndExecuteTx(context)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "CheckAndExecuteTx contract failed");
        }
    }

    Object callContractTxObj;

    callContractTxObj.push_back(Pair("run_steps", contractInvokeTx.fuel));
    callContractTxObj.push_back(Pair("used_fuel", contractInvokeTx.GetFuelFee(newHeight, fuelRate)));

    Object retObj;
    retObj.push_back(Pair("fuel_rate",              (int32_t)fuelRate));
    retObj.push_back(Pair("register_contract_tx",   DeployContractTxObj));
    retObj.push_back(Pair("call_contract_tx",       callContractTxObj));

    return retObj;
}

namespace RPC_PARAM {
    ComboMoney GetComboMoney(const Array& params, uint32_t index, const ComboMoney &defaultValue) {

        if (params.size() <= index)
            return defaultValue;
        return GetComboMoney(params[index], defaultValue.symbol);
    }
}

Value luavm_executecontract(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
            "luavm_executecontract \"addr\" \"app_id\" [\"arguments\"] [symbol:amount:unit]\n"
            "\nexecute the existed contract in vm simulator, and then returns the execution status.\n"
            "\nArguments:\n"
            "1.\"addr\":                (string required) contract owner address from this wallet\n"
            "2.\"app_id\":            (string required) regid of app account\n"
            "3.\"arguments\":           (string, optional) contract method invoke content (Hex encode required)\n"
            "4.\"symbol:amount:unit\":  (string:numeric:string, optional) amount of coin to send to app account\n"
            "\nResult vm execute detail\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("luavm_executecontract","\"100-3\" \"20-3\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("luavm_executecontract", "\"100-3\", \"20-3"));
    }

    const CUserID &txUid = RPC_PARAM::ParseUserIdByAddr(params[0]);
    const CUserID &appUid = RPC_PARAM::GetUserId(params[1]);
    vector<uint8_t> arguments;
    if (params.size() > 2) {
        arguments = ParseHex(params[2].get_str());
    }
    const ComboMoney cbAmount = RPC_PARAM::GetComboMoney(params, 3, ComboMoney(SYMB::WICC, 0, COIN_UNIT::SAWI));

    auto spCw = std::make_shared<CCacheWrapper>(pCdMan);
    CBlockIndex *pTip      = chainActive.Tip();
    HeightType height = pTip->height + 1;
    uint32_t txIndex = 1;
    uint32_t fuelRate      = GetElementForBurn(pTip);
    uint32_t blockTime     = pTip->GetBlockTime();
    uint32_t prevBlockTime = pTip->pprev != nullptr ? pTip->pprev->GetBlockTime() : pTip->GetBlockTime();
    vector<CReceipt> receipts;

    const TokenSymbol &feeSymbol = SYMB::WICC;
    uint64_t minFee = 0;
    if (!GetTxMinFee(*spCw, LCONTRACT_INVOKE_TX, height, feeSymbol, minFee))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Get tx min fee failed");

    uint64_t fees = minFee + 100000 * COIN; // give the enough fee to execute contract

    CAccount txAccount;
    if (!spCw->accountCache.GetAccount(txUid, txAccount)) {
        if (txUid.is<CKeyID>()) {
            txAccount.keyid = txUid.get<CKeyID>();
        } else if (txUid.is<CPubKey>()) {
            txAccount.owner_pubkey = txUid.get<CPubKey>();
            txAccount.keyid = txAccount.owner_pubkey.GetKeyId();
        } else {
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("txUid=%s does not have account", txUid.ToString()));
        }
    }

    if (txAccount.regid.IsEmpty()) {
        txAccount.regid = CRegID(height, txIndex);
    }
    if (txAccount.owner_pubkey.IsEmpty()) {
            if (!pWalletMain->GetPubKey(txAccount.keyid, txAccount.owner_pubkey) || !txAccount.owner_pubkey.IsFullyValid())
                throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Key not found in the local wallet! addr=%s",
                    txAccount.keyid.ToAddress()));
    }
    txAccount.OperateBalance(feeSymbol, ADD_FREE, fees, ReceiptType::COIN_MINT_ONCHAIN, receipts); // add coin to account for fees

    if (!spCw->accountCache.SaveAccount(txAccount))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("save account error! txUid=%s", txUid.ToString()));

    CAccount appAccount = RPC_PARAM::GetUserAccount(spCw->accountCache, appUid);

    EnsureWalletIsUnlocked();

    CLuaContractInvokeTx contractInvokeTx;

    {
        const CRegID &appRegid = appAccount.regid;
        if (!spCw->contractCache.HasContract(appRegid)) {
            throw runtime_error(strprintf("the contract app does not exist! app_id=%s\n", appUid.ToString()));
        }
        contractInvokeTx.nTxType      = LCONTRACT_INVOKE_TX;
        contractInvokeTx.txUid        = txAccount.regid;
        contractInvokeTx.app_uid      = appRegid;
        contractInvokeTx.coin_amount  = cbAmount.GetAmountInSawi();
        contractInvokeTx.llFees       = fees;
        contractInvokeTx.arguments    = string(arguments.begin(), arguments.end());
        contractInvokeTx.valid_height = height;

        vector<uint8_t> signature;
        if (!pWalletMain->Sign(txAccount.keyid, contractInvokeTx.GetHash(), contractInvokeTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        CValidationState state;
        CTxExecuteContext context(height, txIndex, fuelRate, blockTime, prevBlockTime, spCw.get(), &state);
        if (!contractInvokeTx.CheckAndExecuteTx(context)) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "CheckAndExecuteTx contract failed");
        }
    }

    Object retObj;
    retObj.push_back(Pair("fuel_rate",              (int32_t)fuelRate));
    retObj.push_back(Pair("burned_fuel", contractInvokeTx.fuel));
    retObj.push_back(Pair("fuel_fee", contractInvokeTx.GetFuelFee(height, contractInvokeTx.nFuelRate)));

    return retObj;
}