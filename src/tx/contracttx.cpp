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
#include "commons/util.h"
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
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (!contract.IsValid()) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, contract is invalid"),
                         REJECT_INVALID, "vmscript-invalid");
    }

    uint64_t llFuel = GetFuel(context.height, context.fuel_rate);
    if (llFees < llFuel) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, fee too small to cover fuel: %llu < %llu",
                        llFees, llFuel), REJECT_INVALID, "fee-too-small-to-cover-fuel");
    }

    if (GetFeatureForkVersion(context.height) == MAJOR_VER_R2) {
        int32_t txSize  = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
        double feePerKb = double(llFees - llFuel) / txSize * 1000.0;
        if (feePerKb < CBaseTx::nMinRelayTxFee) {
            uint64_t minFee = ceil(double(CBaseTx::nMinRelayTxFee) * txSize / 1000.0 + llFuel);
            return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, fee too small in fee/kb: %llu < %llu",
                            llFees, minFee), REJECT_INVALID,
                            strprintf("fee-too-small-in-fee/kb: %llu < %llu", llFees, minFee));
        }
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, get account failed"),
                         REJECT_INVALID, "bad-getaccount");
    }
    if (!account.HaveOwnerPubKey()) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, account unregistered"), REJECT_INVALID,
                         "bad-account-unregistered");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);

    return true;
}

bool CLuaContractDeployTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::ExecuteTx, read regist addr %s account info error", txUid.ToString()),
                         UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccount accountLog(account);
    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, llFees)) {
            return state.DoS(100, ERRORMSG("CLuaContractDeployTx::ExecuteTx, operate account failed ,regId=%s",
                            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyid), account))
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::ExecuteTx, save account info error"), UPDATE_ACCOUNT_FAIL,
                         "bad-save-accountdb");

    // create script account
    CAccount contractAccount;
    CRegID contractRegId(context.height, context.index);
    CKeyID keyId           = Hash160(contractRegId.GetRegIdRaw());
    contractAccount.keyid  = keyId;
    contractAccount.regid  = contractRegId;
    contractAccount.nickid = CNickID();

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
        LogPrint("ERROR", "CUniversalContractDeployTx::GetFuel(), get min_fee failed! fee_symbol=%s\n", fee_symbol);
        throw runtime_error("CUniversalContractDeployTx::GetFuel(), get min_fee failed");
    }
    return std::max<uint64_t>(((nRunStep / 100.0f) * nFuelRate), minFee);
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
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_ARGUMENTS;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_APPID(app_uid.type());

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::CheckTx, read account failed, regId=%s",
                        txUid.get<CRegID>().ToString()), REJECT_INVALID, "bad-getaccount");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::CheckTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CLuaContractInvokeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    uint64_t fuelLimit;
    if (!GetFuelLimit(*this, context, fuelLimit))
        return false;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(context, srcAccount)) {
        return false;
    }

    if (!srcAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, llFees + coin_amount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, accounts hash insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, save account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(app_uid, desAccount)) {
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, get account info failed by regid:%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!desAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, coin_amount)) {
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, operate accounts error"),
                        UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(app_uid, desAccount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, save account error, kyeId=%s",
                        desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-script");

    CLuaVMRunEnv vmRunEnv;

    CLuaVMContext luaContext;
    luaContext.p_cw              = &cw;
    luaContext.height            = context.height;
    luaContext.block_time        = context.block_time;
    luaContext.p_base_tx         = this;
    luaContext.fuel_limit        = fuelLimit;
    luaContext.transfer_symbol   = SYMB::WICC;
    luaContext.transfer_amount   = coin_amount;
    luaContext.p_tx_user_account = &srcAccount;
    luaContext.p_app_account     = &desAccount;
    luaContext.p_contract        = &contract;
    luaContext.p_arguments       = &arguments;

    int64_t llTime = GetTimeMillis();
    auto pExecErr  = vmRunEnv.ExecuteContract(&luaContext, nRunStep);
    if (pExecErr)
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
                        GetHash().GetHex(), *pExecErr), UPDATE_ACCOUNT_FAIL, "run-script-error: " + *pExecErr);

    LogPrint("vm", "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    if (!cw.txReceiptCache.SetTxReceipts(GetHash(), vmRunEnv.GetReceipts()))
        return state.DoS(
            100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, set tx receipts failed!! txid=%s", GetHash().ToString()),
            REJECT_INVALID, "set-tx-receipt-failed");

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
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (!contract.IsValid()) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, contract is invalid"),
                         REJECT_INVALID, "vmscript-invalid");
    }

    uint64_t llFuel = GetFuel(context.height, context.fuel_rate);
    if (llFees < llFuel) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, fee too small to cover fuel: %llu < %llu",
                        llFees, llFuel), REJECT_INVALID, "fee-too-small-to-cover-fuel");
    }

    int32_t txSize  = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
    double feePerKb = double(llFees - llFuel) / txSize * 1000.0;
    if (feePerKb < CBaseTx::nMinRelayTxFee) {
        uint64_t minFee = ceil(double(CBaseTx::nMinRelayTxFee) * txSize / 1000.0 + llFuel);
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

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);

    return true;
}

bool CUniversalContractDeployTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::ExecuteTx, read regist addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccount accountLog(account);
    if (!account.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees)) {
            return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::ExecuteTx, operate account failed ,regId=%s",
                            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyid), account))
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::ExecuteTx, save account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    // create script account
    CAccount contractAccount;
    CRegID contractRegId(context.height, context.index);
    CKeyID keyId           = Hash160(contractRegId.GetRegIdRaw());
    contractAccount.keyid  = keyId;
    contractAccount.regid  = contractRegId;
    contractAccount.nickid = CNickID();

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

    return true;
}

uint64_t CUniversalContractDeployTx::GetFuel(int32_t height, uint32_t nFuelRate) {
    uint64_t minFee = 0;
    if (!GetTxMinFee(nTxType, height, fee_symbol, minFee)) {
        LogPrint("ERROR", "CUniversalContractDeployTx::GetFuel(), get min_fee failed! fee_symbol=%s\n", fee_symbol);
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
    result.push_back(Pair("memo",       HexStr(contract.memo)));
    result.push_back(Pair("abi",        contract.abi));

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class CUniversalContractInvokeTx

bool CUniversalContractInvokeTx::CheckTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    IMPLEMENT_DISABLE_TX_PRE_STABLE_COIN_RELEASE;
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_ARGUMENTS;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_APPID(app_uid.type());

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, read account failed, regId=%s",
                        txUid.get<CRegID>().ToString()), REJECT_INVALID, "bad-getaccount");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CUniversalContractInvokeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    uint64_t fuelLimit;
    if (!GetFuelLimit(*this, context, fuelLimit))
        return false;

    vector<CReceipt> receipts;

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(context, srcAccount)) {
        return false;
    }

    if (!srcAccount.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, accounts hash insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!srcAccount.OperateBalance(coin_symbol, BalanceOpType::SUB_FREE, coin_amount))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, accounts hash insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, save account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(app_uid, desAccount)) {
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, get account info failed by regid:%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!desAccount.OperateBalance(coin_symbol, BalanceOpType::ADD_FREE, coin_amount)) {
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, operate accounts error"),
                        UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(app_uid, desAccount))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, save account error, kyeId=%s",
                        desAccount.keyid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-script");

    CLuaVMRunEnv vmRunEnv;

    CLuaVMContext luaContext;
    luaContext.p_cw              = &cw;
    luaContext.height            = context.height;
    luaContext.block_time        = context.block_time;
    luaContext.p_base_tx         = this;
    luaContext.fuel_limit        = fuelLimit;
    luaContext.transfer_symbol   = coin_symbol;
    luaContext.transfer_amount   = coin_amount;
    luaContext.p_tx_user_account = &srcAccount;
    luaContext.p_app_account     = &desAccount;
    luaContext.p_contract        = &contract;
    luaContext.p_arguments       = &arguments;

    int64_t llTime = GetTimeMillis();
    auto pExecErr  = vmRunEnv.ExecuteContract(&luaContext, nRunStep);
    if (pExecErr)
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
            GetHash().GetHex(), *pExecErr), UPDATE_ACCOUNT_FAIL, "run-script-error: " + *pExecErr);

    receipts.insert(receipts.end(), vmRunEnv.GetReceipts().begin(), vmRunEnv.GetReceipts().end());

    LogPrint("vm", "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    // If fees paid by WUSD, send the fuel to risk reserve pool.
    if (fee_symbol == SYMB::WUSD) {
        uint64_t fuel = GetFuel(context.height, context.fuel_rate);
        CAccount fcoinGenesisAccount;
        cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);

        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, fuel)) {
            return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, operate balance failed"),
                             UPDATE_ACCOUNT_FAIL, "operate-scoins-genesis-account-failed");
        }

        CUserID fcoinGenesisUid(fcoinGenesisAccount.regid);
        receipts.emplace_back(nullId, fcoinGenesisUid, SYMB::WUSD, fuel, ReceiptCode::CONTRACT_FUEL_TO_RISK_RISERVE);
    }

    if (!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");

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