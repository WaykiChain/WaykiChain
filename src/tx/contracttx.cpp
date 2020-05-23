// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contracttx.h"

#include "commons/util/util.h"
#include "vm/luavm/luavmrunenv.h"
#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "config/version.h"

#include <sstream>

// get and check fuel limit
static bool GetFuelLimit(CBaseTx &tx, CTxExecuteContext &context, uint64_t &fuelLimit) {
    uint64_t fuelRate = context.fuel_rate;
    if (fuelRate == 0)
        return context.pState->DoS(100, ERRORMSG("fuelRate cannot be 0"), REJECT_INVALID, "invalid-fuel-rate");

    uint64_t minFee;
    if (!GetTxMinFee(*context.pCw, tx.nTxType, context.height, tx.fee_symbol, minFee))
        return context.pState->DoS(100, ERRORMSG("get minFee failed"), REJECT_INVALID, "get-min-fee-failed");

    assert(tx.llFees >= minFee);

    uint64_t reservedFeesForMiner = minFee * CONTRACT_CALL_RESERVED_FEES_RATIO / 100;
    uint64_t reservedFeesForGas   = tx.llFees - reservedFeesForMiner;

    fuelLimit = std::min<uint64_t>((reservedFeesForGas / fuelRate) * 100, MAX_BLOCK_FUEL);

    if (fuelLimit == 0)
        return context.pState->DoS(100, ERRORMSG("fuelLimit == 0"), REJECT_INVALID, "fuel-limit-equals-zero");

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CLuaContractDeployTx

bool CLuaContractDeployTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (!contract.IsValid())
        return state.DoS(100, ERRORMSG("contract is invalid"), REJECT_INVALID, "vmscript-invalid");

    uint64_t fuelFee = GetFuelFee(cw, context.height, context.fuel_rate);
    if (llFees < fuelFee)
        return state.DoS(100, ERRORMSG("the given fee is small than burned fuel fee: %llu < %llu",
                        llFees, fuelFee), REJECT_INVALID, "fee-too-small-for-burned-fuel");

    if (GetFeatureForkVersion(context.height) >= MAJOR_VER_R2) {
        int32_t txSize  = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
        double feePerKb = double(llFees - fuelFee) / txSize * 1000.0;
        if (feePerKb < MIN_RELAY_TX_FEE) {
            uint64_t minFee = ceil(double(MIN_RELAY_TX_FEE) * txSize / 1000.0 + fuelFee);
            return state.DoS(100, ERRORMSG(" fee too small in fee/kb: %llu < %llu",
                            llFees, minFee), REJECT_INVALID,
                            strprintf("fee-too-small-in-fee/kb: %llu < %llu", llFees, minFee));
        }
    }

    if (!sp_tx_account->HasOwnerPubKey()) {
        return state.DoS(100, ERRORMSG("account unregistered! txUid=%s", txUid.ToString()), REJECT_INVALID,
                         "account-unregistered");
    }

    return true;
}

bool CLuaContractDeployTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    // create script account
    CAccount contractAccount;
    CRegID contractRegId(context.height, context.index);
    CKeyID keyid = Hash160(contractRegId.GetRegIdRaw());

    auto spContractAccount = NewAccount(cw, keyid);
    if (!RegisterAccount(context, nullptr/*pPubkey*/, *spContractAccount)) // generate new regid for the account
        return false;
    assert(spContractAccount->regid  == contractRegId);

    // save new script content
    CUniversalContractStore contractStore = {
        VMType::LUA_VM,
        sp_tx_account->regid,   // maintainer
        false,
        contract.code,
        contract.memo,
        ""                      // abi
    };

    if (!cw.contractCache.SaveContract(contractRegId, contractStore))
        return state.DoS(100, ERRORMSG("save code for contract id %s error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

    fuel = contract.GetContractSize();

    return true;
}

uint64_t CLuaContractDeployTx::GetFuelFee(CCacheWrapper &cw, int32_t height, uint32_t nFuelRate) {
    uint64_t minFee = 0;
    if (!GetTxMinFee(cw, nTxType, height, fee_symbol, minFee)) {
        LogPrint(BCLog::ERROR, "get min_fee failed! fee_symbol=%s\n", fee_symbol);

        throw runtime_error("UCONTRACT_DEPLOY_TX::GetFuelFee(), get min_fee failed");
    }

    return std::max<uint64_t>(((fuel / 100.0f) * nFuelRate), minFee);
}

string CLuaContractDeployTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, llFees=%llu, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(),
                     llFees, valid_height);
}

Object CLuaContractDeployTx::ToJson(CCacheWrapper &cw) const {
    Object result = CBaseTx::ToJson(cw);

    result.push_back(Pair("contract_code", HexStr(contract.code)));
    result.push_back(Pair("contract_memo", HexStr(contract.memo)));

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CLuaContractInvokeTx

bool CLuaContractInvokeTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;
    IMPLEMENT_CHECK_TX_ARGUMENTS;

    IMPLEMENT_CHECK_TX_APPID(app_uid);

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("public key is invalid"), REJECT_INVALID, "bad-publickey");

    CUniversalContractStore contractStore;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contractStore))
        return state.DoS(100, ERRORMSG("read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    return true;
}

bool CLuaContractInvokeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    uint64_t fuelLimit;
    if (!GetFuelLimit(*this, context, fuelLimit))
        return false;

    auto spAppAccount = GetAccount(context, app_uid, "app_uid");
    if (!spAppAccount) return false;

    if (coin_amount > 0 && !sp_tx_account->OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, coin_amount,
                                ReceiptType::LUAVM_TRANSFER_ACTUAL_COINS, receipts, spAppAccount.get()))
        return state.DoS(100, ERRORMSG("txAccount has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    CUniversalContractStore contractStore;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contractStore))
        return state.DoS(100, ERRORMSG("read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-script");

    CLuaVMRunEnv vmRunEnv;
    CLuaVMContext luaContext;

    luaContext.p_cw              = &cw;
    luaContext.height            = context.height;
    luaContext.block_time        = context.block_time;
    luaContext.prev_block_time   = context.prev_block_time;
    luaContext.p_base_tx         = this;
    luaContext.fuel_limit        = fuelLimit;
    luaContext.transfer_symbol   = SYMB::WICC;
    luaContext.transfer_amount   = coin_amount;
    luaContext.sp_tx_account     = sp_tx_account;
    luaContext.sp_app_account    = spAppAccount;
    luaContext.p_contract        = &contractStore;
    luaContext.p_arguments       = &arguments;

    int64_t llTime = GetTimeMillis();
    string errMsg;
    if (!vmRunEnv.ExecuteContract(&luaContext, fuel, errMsg))
        return state.DoS(100, ERRORMSG("txid=%s run script error:%s", GetHash().GetHex(), errMsg),
                        UPDATE_ACCOUNT_FAIL, "run-script-error: " + errMsg);

    LogPrint(BCLog::LUAVM, "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    container::Append(receipts, vmRunEnv.GetReceipts());
    return true;
}

string CLuaContractInvokeTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, app_uid=%s, coin_amount=%llu, llFees=%llu, arguments=%s, "
        "valid_height=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), app_uid.ToString(), coin_amount, llFees,
        HexStr(arguments), valid_height);
}

Object CLuaContractInvokeTx::ToJson(CCacheWrapper &cw) const {
    Object result = CBaseTx::ToJson(cw);

    CKeyID appKeyId;
    cw.accountCache.GetKeyId(app_uid, appKeyId);
    result.push_back(Pair("to_addr",        appKeyId.ToAddress()));
    result.push_back(Pair("to_uid",         app_uid.ToString()));
    result.push_back(Pair("coin_symbol",    SYMB::WICC));
    result.push_back(Pair("coin_amount",    coin_amount));
    result.push_back(Pair("arguments",      HexStr(arguments)));

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CUniversalContractDeployTx

bool CUniversalContractDeployTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (contract.vm_type != VMType::LUA_VM)
        return state.DoS(100, ERRORMSG("support LuaVM only"), REJECT_INVALID, "vm-type-error");

    if (!contract.IsValid())
        return state.DoS(100, ERRORMSG("contract is invalid"), REJECT_INVALID, "vmscript-invalid");

    uint64_t fuelFee = GetFuelFee(cw, context.height, context.fuel_rate);
    if (llFees < fuelFee)
        return state.DoS(100, ERRORMSG("the given fee is small than burned fuel fee: %llu < %llu",
                        llFees, fuelFee), REJECT_INVALID, "fee-too-small-for-burned-fuel");

    int32_t txSize  = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
    double feePerKb = double(llFees - fuelFee) / txSize * 1000.0;
    if (feePerKb < MIN_RELAY_TX_FEE) {
        uint64_t minFee = ceil(double(MIN_RELAY_TX_FEE) * txSize / 1000.0 + fuelFee);
        return state.DoS(100, ERRORMSG("fee too small in fee/kb: %llu < %llu", llFees, minFee),
                        REJECT_INVALID, strprintf("fee-too-small-in-fee/kb: %llu < %llu", llFees, minFee));
    }

    if (!sp_tx_account->HasOwnerPubKey()) {
        return state.DoS(100, ERRORMSG("account unregistered! txUid=%s", txUid.ToString()), REJECT_INVALID,
                         "account-unregistered");
    }

    return true;
}

bool CUniversalContractDeployTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    // create script account
    CRegID contractRegId(context.height, context.index);
    CKeyID keyid = Hash160(contractRegId.GetRegIdRaw());

    auto spContractAccount = NewAccount(cw, keyid);
    if (!RegisterAccount(context, nullptr/*pPubkey*/, *spContractAccount)) // generate new regid for the account
        return false;
    assert(spContractAccount->regid  == contractRegId);

    // save new script content
    CUniversalContractStore contractStore = {
        contract.vm_type,
        sp_tx_account->regid,   // maintainer
        contract.upgradable,
        contract.code,
        contract.memo,
        contract.abi           // abi
    };
    if (!cw.contractCache.SaveContract(contractRegId, contractStore))
        return state.DoS(100, ERRORMSG("save code for contract id %s error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

    fuel = contract.GetContractSize();

    // If fees paid by WUSD, send the fuel to risk reserve pool.
    if (fee_symbol == SYMB::WUSD) {
        uint64_t fuel = GetFuelFee(cw, context.height, context.fuel_rate);
        auto spFcoinAccount = GetAccount(context, SysCfg().GetFcoinGenesisRegId(), "fcoin");
        if (!spFcoinAccount) return false;

        if (!spFcoinAccount->OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, fuel,
                                                ReceiptType::CONTRACT_FUEL_TO_RISK_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("operate balance failed"),
                             UPDATE_ACCOUNT_FAIL, "operate-scoins-genesis-account-failed");
        }
    }

    return true;
}

uint64_t CUniversalContractDeployTx::GetFuelFee(CCacheWrapper &cw, int32_t height, uint32_t nFuelRate) {
    uint64_t minFee = 0;
    if (!GetTxMinFee(cw, nTxType, height, fee_symbol, minFee)) {
        LogPrint(BCLog::ERROR, "get min_fee failed! fee_symbol=%s\n", fee_symbol);
        throw runtime_error("CUniversalContractDeployTx::GetFuelFee(), get min_fee failed");
    }

    return std::max<uint64_t>(((fuel / 100.0f) * nFuelRate), minFee);
}

string CUniversalContractDeployTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, fee_symbol=%s, llFees=%llu, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(),
                     fee_symbol, llFees, valid_height);
}

Object CUniversalContractDeployTx::ToJson(CCacheWrapper &cw) const {
    Object result = CBaseTx::ToJson(cw);

    result.push_back(Pair("vm_type",    contract.vm_type));
    result.push_back(Pair("upgradable", contract.upgradable));
    result.push_back(Pair("code",       HexStr(contract.code)));
    result.push_back(Pair("memo",       contract.memo));
    result.push_back(Pair("abi",        contract.abi));

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CUniversalContractInvokeTx

bool CUniversalContractInvokeTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    IMPLEMENT_CHECK_TX_ARGUMENTS;
    IMPLEMENT_CHECK_TX_APPID(app_uid);

    if ((txUid.is<CPubKey>()) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("public key is invalid"), REJECT_INVALID,
                         "bad-publickey");


    if (SysCfg().NetworkID() == TEST_NET && context.height < 260000) {
        // TODO: remove me if reset testnet.
    } else {
        if (!cw.assetCache.CheckAsset(coin_symbol))
            return state.DoS(100, ERRORMSG("invalid coin_symbol=%s", coin_symbol),
                                REJECT_INVALID, "invalid-coin-symbol");
    }

    CUniversalContractStore contractStore;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contractStore))
        return state.DoS(100, ERRORMSG("read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    return true;
}

bool CUniversalContractInvokeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    uint64_t fuelLimit;
    if (!GetFuelLimit(*this, context, fuelLimit))
        return false;

    auto spAppAccount = GetAccount(context, app_uid, "app_uid");
    if (!spAppAccount) return false;

    if (!sp_tx_account->OperateBalance(coin_symbol, BalanceOpType::SUB_FREE, coin_amount,
                                  ReceiptType::LUAVM_TRANSFER_ACTUAL_COINS, receipts, spAppAccount.get()))
        return state.DoS(100, ERRORMSG("txAccount has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    CUniversalContractStore contractStore;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contractStore))
        return state.DoS(100, ERRORMSG("read contract failed, regId=%s", app_uid.get<CRegID>().ToString()),
                        READ_ACCOUNT_FAIL, "bad-read-contract");

    CLuaVMRunEnv vmRunEnv;
    CLuaVMContext luaContext;

    luaContext.p_cw              = &cw;
    luaContext.height            = context.height;
    luaContext.block_time        = context.block_time;
    luaContext.prev_block_time   = context.prev_block_time;
    luaContext.p_base_tx         = this;
    luaContext.fuel_limit        = fuelLimit;
    luaContext.transfer_symbol   = coin_symbol;
    luaContext.transfer_amount   = coin_amount;
    luaContext.sp_tx_account     = sp_tx_account;
    luaContext.sp_app_account    = spAppAccount;
    luaContext.p_contract        = &contractStore;
    luaContext.p_arguments       = &arguments;

    int64_t llTime = GetTimeMillis();
    string errMsg;
    if (!vmRunEnv.ExecuteContract(&luaContext, fuel, errMsg))
        return state.DoS(100, ERRORMSG("txid=%s run script error: %s", GetHash().GetHex(), errMsg),
                        EXECUTE_SCRIPT_FAIL, "run-script-error: " + errMsg);

    receipts.insert(receipts.end(), vmRunEnv.GetReceipts().begin(), vmRunEnv.GetReceipts().end());

    LogPrint(BCLog::LUAVM, "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    // If fees paid by WUSD, send the fuel to risk reserve pool.
    if (fee_symbol == SYMB::WUSD) {
        uint64_t fuel = GetFuelFee(cw, context.height, context.fuel_rate);
        auto spFcoinAccount = GetAccount(context, SysCfg().GetFcoinGenesisRegId(), "fcoin");
        if (!spFcoinAccount) return false;

        if (!spFcoinAccount->OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, fuel,
                                                ReceiptType::CONTRACT_FUEL_TO_RISK_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("operate balance failed"),
                             UPDATE_ACCOUNT_FAIL, "operate-scoins-genesis-account-failed");
        }
    }

    return true;
}

string CUniversalContractInvokeTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, app_uid=%s, coin_symbol=%s, coin_amount=%llu, fee_symbol=%s, "
        "llFees=%llu, arguments=%s, valid_height=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), app_uid.ToString(), coin_symbol,
        coin_amount, fee_symbol, llFees, HexStr(arguments), valid_height);
}

Object CUniversalContractInvokeTx::ToJson(CCacheWrapper &cw) const {
    Object result = CBaseTx::ToJson(cw);

    CKeyID desKeyId;
    cw.accountCache.GetKeyId(app_uid, desKeyId);
    result.push_back(Pair("to_addr",        desKeyId.ToAddress()));
    result.push_back(Pair("to_uid",         app_uid.ToString()));
    result.push_back(Pair("coin_symbol",    coin_symbol));
    result.push_back(Pair("coin_amount",    coin_amount));
    result.push_back(Pair("arguments",      HexStr(arguments)));

    return result;
}
