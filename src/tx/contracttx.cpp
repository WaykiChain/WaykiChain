// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contracttx.h"

#include "entities/vote.h"
#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "commons/util/util.h"
#include "config/version.h"
#include "vm/luavm/luavmrunenv.h"

// get and check fuel limit
static bool GetFuelLimit(CBaseTx &tx, CTxExecuteContext &context, uint64_t &fuelLimit) {
    uint64_t fuelRate = context.fuel_rate;
    if (fuelRate == 0)
        return context.pState->DoS(100, ERRORMSG("GetFuelLimit, fuelRate cannot be 0"), REJECT_INVALID, "invalid-fuel-rate");

    uint64_t minFee;
    if (!GetTxMinFee(tx.nTxType, context.height, tx.fee_symbol, minFee))
        return context.pState->DoS(100, ERRORMSG("GetFuelLimit, get minFee failed"), REJECT_INVALID, "get-min-fee-failed");

    assert(tx.llFees >= minFee);

    uint64_t reservedFeesForMiner = minFee * CONTRACT_CALL_RESERVED_FEES_RATIO / 100;
    uint64_t reservedFeesForGas   = tx.llFees - reservedFeesForMiner;

    fuelLimit = std::min<uint64_t>((reservedFeesForGas / fuelRate) * 100, MAX_BLOCK_RUN_STEP);

    if (fuelLimit == 0) {
        return context.pState->DoS(100, ERRORMSG("GetFuelLimit, fuelLimit == 0"), REJECT_INVALID,
                                   "fuel-limit-equals-zero");
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CLuaContractDeployTx

bool CLuaContractDeployTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (!contract.IsValid())
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, contract is invalid"),
                         REJECT_INVALID, "vmscript-invalid");

    uint64_t llFuel = GetFuel(context.height, context.fuel_rate);
    if (llFees < llFuel)
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, fee too small to cover fuel: %llu < %llu",
                        llFees, llFuel), REJECT_INVALID, "fee-too-small-to-cover-fuel");

    if (GetFeatureForkVersion(context.height) >= MAJOR_VER_R2) {
        int32_t txSize  = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
        double feePerKb = double(llFees - llFuel) / txSize * 1000.0;
        if (feePerKb < MIN_RELAY_TX_FEE) {
            uint64_t minFee = ceil(double(MIN_RELAY_TX_FEE) * txSize / 1000.0 + llFuel);
            return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, fee too small in fee/kb: %llu < %llu",
                            llFees, minFee), REJECT_INVALID,
                            strprintf("fee-too-small-in-fee/kb: %llu < %llu", llFees, minFee));
        }
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, get account failed"),
                         REJECT_INVALID, "bad-getaccount");

    if (!account.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, account unregistered"), REJECT_INVALID,
                         "bad-account-unregistered");

    return true;
}

bool CLuaContractDeployTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    // create script account
    CAccount contractAccount;
    CRegID contractRegId(context.height, context.index);
    contractAccount.keyid  = Hash160(contractRegId.GetRegIdRaw());
    contractAccount.regid  = contractRegId;

    // save new script content
    if (!cw.contractCache.SaveContract(contractRegId, CUniversalContract(contract.code, contract.memo))) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::ExecuteTx, save code for contract id %s error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    if (!cw.accountCache.SaveAccount(contractAccount)) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::ExecuteTx, create new account script id %s script info error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    nRunStep = contract.GetContractSize();

    return true;
}

uint64_t CLuaContractDeployTx::GetFuel(int32_t height, uint32_t nFuelRate) {
    uint64_t minFee = 0;
    if (!GetTxMinFee(nTxType, height, fee_symbol, minFee)) {
        LogPrint(BCLog::ERROR, "CUniversalContractDeployTx::GetFuel(), get min_fee failed! fee_symbol=%s\n", fee_symbol);
        throw runtime_error("CUniversalContractDeployTx::GetFuel(), get min_fee failed");
    }

    return std::max<uint64_t>(((nRunStep / 100.0f) * nFuelRate), minFee);
}

string CLuaContractDeployTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, llFees=%llu, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), llFees,
                     valid_height);
}

Object CLuaContractDeployTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

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
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::CheckTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    return true;
}

bool CLuaContractInvokeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    uint64_t fuelLimit;
    if (!GetFuelLimit(*this, context, fuelLimit))
        return false;

    CAccount appAccount;
    if (!cw.accountCache.GetAccount(app_uid, appAccount)) {
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, get account info failed by regid:%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!txAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, coin_amount,
                                ReceiptCode::LUAVM_TRANSFER_ACTUAL_COINS, receipts, &appAccount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, txAccount has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!cw.accountCache.SetAccount(app_uid, appAccount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, save account error, kyeId=%s",
                        appAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, read script failed, regId=%s",
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
    luaContext.p_tx_user_account = &txAccount;
    luaContext.p_app_account     = &appAccount;
    luaContext.p_contract        = &contract;
    luaContext.p_arguments       = &arguments;

    int64_t llTime = GetTimeMillis();
    auto pExecErr  = vmRunEnv.ExecuteContract(&luaContext, nRunStep);
    if (pExecErr)
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
                        GetHash().GetHex(), *pExecErr), UPDATE_ACCOUNT_FAIL, "run-script-error: " + *pExecErr);

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

Object CLuaContractInvokeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    CKeyID desKeyId;
    accountCache.GetKeyId(app_uid, desKeyId);
    result.push_back(Pair("to_addr",        desKeyId.ToAddress()));
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
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, support LuaVM only"), REJECT_INVALID,
                         "vm-type-error");

    if (!contract.IsValid())
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, contract is invalid"),
                         REJECT_INVALID, "vmscript-invalid");

    uint64_t llFuel = GetFuel(context.height, context.fuel_rate);
    if (llFees < llFuel)
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, fee too small to cover fuel: %llu < %llu",
                        llFees, llFuel), REJECT_INVALID, "fee-too-small-to-cover-fuel");

    int32_t txSize  = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
    double feePerKb = double(llFees - llFuel) / txSize * 1000.0;
    if (feePerKb < MIN_RELAY_TX_FEE) {
        uint64_t minFee = ceil(double(MIN_RELAY_TX_FEE) * txSize / 1000.0 + llFuel);
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, fee too small in fee/kb: %llu < %llu",
                        llFees, minFee), REJECT_INVALID,
                        strprintf("fee-too-small-in-fee/kb: %llu < %llu", llFees, minFee));
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, get account failed"),
                         REJECT_INVALID, "bad-getaccount");
    }

    if (!account.HaveOwnerPubKey()) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, account unregistered"),
            REJECT_INVALID, "bad-account-unregistered");
    }

    return true;
}

bool CUniversalContractDeployTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    // create script account
    CAccount contractAccount;
    CRegID contractRegId(context.height, context.index);
    contractAccount.keyid  = Hash160(contractRegId.GetRegIdRaw());
    contractAccount.regid  = contractRegId;

    // save new script content
    if (!cw.contractCache.SaveContract(contractRegId, contract)) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::ExecuteTx, save code for contract id %s error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }
    if (!cw.accountCache.SaveAccount(contractAccount)) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::ExecuteTx, create new account script id %s script info error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    nRunStep = contract.GetContractSize();

    // If fees paid by WUSD, send the fuel to risk reserve pool.
    if (fee_symbol == SYMB::WUSD) {
        uint64_t fuel = GetFuel(context.height, context.fuel_rate);
        CAccount fcoinGenesisAccount;
        cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, fuel,
                                                ReceiptCode::CONTRACT_FUEL_TO_RISK_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::ExecuteTx, operate balance failed"),
                             UPDATE_ACCOUNT_FAIL, "operate-scoins-genesis-account-failed");
        }

        if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
            return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::ExecuteTx, write fcoin genesis account info error!"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    return true;
}

uint64_t CUniversalContractDeployTx::GetFuel(int32_t height, uint32_t nFuelRate) {
    uint64_t minFee = 0;
    if (!GetTxMinFee(nTxType, height, fee_symbol, minFee)) {
        LogPrint(BCLog::ERROR, "CUniversalContractDeployTx::GetFuel(), get min_fee failed! fee_symbol=%s\n", fee_symbol);
        throw runtime_error("CUniversalContractDeployTx::GetFuel(), get min_fee failed");
    }

    return std::max<uint64_t>(((nRunStep / 100.0f) * nFuelRate), minFee);
}

string CUniversalContractDeployTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, fee_symbol=%s, llFees=%llu, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(),
                     fee_symbol, llFees, valid_height);
}

Object CUniversalContractDeployTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

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
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    if (!cw.assetCache.CheckAsset(coin_symbol))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, invalid coin_symbol=%s", coin_symbol),
                        REJECT_INVALID, "invalid-coin-symbol");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    return true;
}

bool CUniversalContractInvokeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw       = *context.pCw;
    CValidationState &state = *context.pState;

    uint64_t fuelLimit;
    if (!GetFuelLimit(*this, context, fuelLimit))
        return false;

    CAccount appAccount;
    if (!cw.accountCache.GetAccount(app_uid, appAccount)) {
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, get account info failed by regid:%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!txAccount.OperateBalance(coin_symbol, BalanceOpType::SUB_FREE, coin_amount,
                                  ReceiptCode::LUAVM_TRANSFER_ACTUAL_COINS, receipts, &appAccount))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, txAccount has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!cw.accountCache.SetAccount(app_uid, appAccount))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, save account error, kyeId=%s",
                        appAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-script");

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
    luaContext.p_tx_user_account = &txAccount;
    luaContext.p_app_account     = &appAccount;
    luaContext.p_contract        = &contract;
    luaContext.p_arguments       = &arguments;

    int64_t llTime = GetTimeMillis();
    auto pExecErr  = vmRunEnv.ExecuteContract(&luaContext, nRunStep);
    if (pExecErr)
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
            GetHash().GetHex(), *pExecErr), UPDATE_ACCOUNT_FAIL, "run-script-error: " + *pExecErr);

    receipts.insert(receipts.end(), vmRunEnv.GetReceipts().begin(), vmRunEnv.GetReceipts().end());

    LogPrint(BCLog::LUAVM, "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    // If fees paid by WUSD, send the fuel to risk reserve pool.
    if (fee_symbol == SYMB::WUSD) {
        uint64_t fuel = GetFuel(context.height, context.fuel_rate);
        CAccount fcoinGenesisAccount;
        cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);

        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, fuel,
                                                ReceiptCode::CONTRACT_FUEL_TO_RISK_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, operate balance failed"),
                             UPDATE_ACCOUNT_FAIL, "operate-scoins-genesis-account-failed");
        }

        if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
            return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, write fcoin genesis account info error!"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
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

Object CUniversalContractInvokeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    CKeyID desKeyId;
    accountCache.GetKeyId(app_uid, desKeyId);
    result.push_back(Pair("to_addr",        desKeyId.ToAddress()));
    result.push_back(Pair("to_uid",         app_uid.ToString()));
    result.push_back(Pair("coin_symbol",    coin_symbol));
    result.push_back(Pair("coin_amount",    coin_amount));
    result.push_back(Pair("arguments",      HexStr(arguments)));

    return result;
}