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
#include "vm/luavm/vmrunenv.h"

static bool GetKeyId(const CAccountDBCache &accountView, const string &userIdStr, CKeyID &keyid) {
    switch (userIdStr.size()) {
        case 6: {   // CRegID
            CRegID regId(userIdStr);
            keyid = regId.GetKeyId(accountView);
            break;
        }
        case 34: {  // Base58Addr
            string addr(userIdStr.begin(), userIdStr.end());
            keyid = CKeyID(addr);
            break;
        }
        //TODO: support nick ID
        default:
            return false;
    }

    if (keyid.IsEmpty())
        return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CLuaContractDeployTx

bool CLuaContractDeployTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (!contract.IsValid()) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, contract is invalid"),
                         REJECT_INVALID, "vmscript-invalid");
    }

    uint64_t llFuel = GetFuel(GetFuelRate(cw.contractCache));
    if (llFees < llFuel) {
        return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, fee too litter to afford fuel: %lld < %lld",
                        llFees, llFuel), REJECT_INVALID, "fee-too-litter-to-afford-fuel");
    }

    // If valid height range changed little enough(i.e. 3 blocks), remove it.
    if (GetFeatureForkVersion(height) == MAJOR_VER_R2) {
        uint64_t slideWindowBlockCount = 0;
        cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount);
        uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount);
        int32_t txSize            = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
        double dFeePerKb          = double(bcoinMedianPrice) / kPercentBoost * (llFees - llFuel) / (txSize / 1000.0);
        if (dFeePerKb != 0 && dFeePerKb < CBaseTx::nMinRelayTxFee) {
            return state.DoS(100, ERRORMSG("CLuaContractDeployTx::CheckTx, fee too litter in fee/kb: %.4f < %lld",
                            dFeePerKb, CBaseTx::nMinRelayTxFee), REJECT_INVALID, "fee-too-litter-in-fee/kb");
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

bool CLuaContractDeployTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
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
    CRegID contractRegId(height, index);
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

uint64_t CLuaContractDeployTx::GetFuel(uint32_t nFuelRate) {
    return std::max(uint64_t((nRunStep / 100.0f) * nFuelRate), 1 * COIN);
}

string CLuaContractDeployTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, llFees=%llu, valid_height=%d\n",
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

bool CLuaContractInvokeTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
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

    if (!srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::CheckTx, account unregistered"), REJECT_INVALID,
                         "bad-account-unregistered");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::CheckTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CLuaContractInvokeTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAccount;
    CAccount desAccount;

    if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(srcAccount, cw, state, height, index)) {
        return false;
    }

    if (!srcAccount.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, llFees + coin_amount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, accounts hash insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (!cw.accountCache.SetAccount(CUserID(srcAccount.keyid), srcAccount))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, save account info error"),
                         WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

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

    CVmRunEnv vmRunEnv;
    std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    uint64_t fuelRate = GetFuelRate(cw.contractCache);

    int64_t llTime = GetTimeMillis();
    tuple<bool, uint64_t, string> ret = vmRunEnv.ExecuteContract(pTx, height, cw, fuelRate, nRunStep);
    if (!std::get<0>(ret))
        return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
                        GetHash().GetHex(), std::get<2>(ret)), UPDATE_ACCOUNT_FAIL, "run-script-error: " + std::get<2>(ret));

    LogPrint("vm", "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    set<CKeyID> vAddress;
    CUserID userId;
    vector<std::shared_ptr<CAccount> > &vAccount = vmRunEnv.GetNewAccount();
    // Update accounts' info referred to the contract
    for (auto &itemAccount : vAccount) {
        vAddress.insert(itemAccount->keyid);
        userId = itemAccount->keyid;
        CAccount oldAcct;
        if (!cw.accountCache.GetAccount(userId, oldAcct)) {
            // The contract transfers money to an address for the first time.
            if (!itemAccount->keyid.IsNull()) {
                oldAcct.keyid = itemAccount->keyid;
            } else {
                return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, read account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
            }
        }
        if (!cw.accountCache.SetAccount(userId, *itemAccount))
            return state.DoS(100, ERRORMSG("CLuaContractInvokeTx::ExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
    for (auto & itemUserAccount : vAppUserAccount) {
        CKeyID itemKeyID;
        bool bValid = GetKeyId(cw.accountCache, itemUserAccount.get()->GetAccUserId(), itemKeyID);
        if (bValid) {
            vAddress.insert(itemKeyID);
        }
    }

    if (!cw.contractCache.SetTxRelAccout(GetHash(), vAddress))
        return ERRORMSG("CLuaContractInvokeTx::ExecuteTx, save tx relate account info to script db error");

    return true;
}

string CLuaContractInvokeTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, app_uid=%s, coin_amount=%llu, llFees=%llu, arguments=%s, "
        "valid_height=%d\n",
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

bool CUniversalContractDeployTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (!contract.IsValid()) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, contract is invalid"),
                         REJECT_INVALID, "vmscript-invalid");
    }

    uint64_t llFuel = GetFuel(GetFuelRate(cw.contractCache));
    if (llFees < llFuel) {
        return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, fee too litter to afford fuel: %lld < %lld",
                        llFees, llFuel), REJECT_INVALID, "fee-too-litter-to-afford-fuel");
    }

    // If valid height range changed little enough(i.e. 3 blocks), remove it.
    if (GetFeatureForkVersion(height) == MAJOR_VER_R2) {
        uint64_t slideWindowBlockCount = 0;
        cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount);
        uint64_t coinMedianPrice =
            coin_symbol == SYMB::WUSD
                ? 10000  // boosted by 10^4
                : cw.ppCache.GetMedianPrice(height, slideWindowBlockCount, CoinPricePair(coin_symbol, SYMB::USD));
        int32_t txSize   = ::GetSerializeSize(GetNewInstance(), SER_NETWORK, PROTOCOL_VERSION);
        double dFeePerKb = double(coinMedianPrice) / kPercentBoost * (llFees - llFuel) / (txSize / 1000.0);
        if (dFeePerKb != 0 && dFeePerKb < CBaseTx::nMinRelayTxFee) {
            return state.DoS(100, ERRORMSG("CUniversalContractDeployTx::CheckTx, fee too litter in fee/kb: %.4f < %lld",
                            dFeePerKb, CBaseTx::nMinRelayTxFee), REJECT_INVALID, "fee-too-litter-in-fee/kb");
        }
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

bool CUniversalContractDeployTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
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
    CRegID contractRegId(height, index);
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

uint64_t CUniversalContractDeployTx::GetFuel(uint32_t nFuelRate) {
    return std::max(uint64_t((nRunStep / 100.0f) * nFuelRate), 1 * COIN);
}

string CUniversalContractDeployTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, coin_symbol=%s, coin_amount=%llu, fee_symbol=%s, llFees=%llu, "
        "valid_height=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), coin_symbol,
        coin_amount, fee_symbol, llFees, valid_height);
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

bool CUniversalContractInvokeTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
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

    if (!srcAccount.HaveOwnerPubKey())
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, account unregistered"),
                        REJECT_INVALID, "bad-account-unregistered");

    CUniversalContract contract;
    if (!cw.contractCache.GetContract(app_uid.get<CRegID>(), contract))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::CheckTx, read script failed, regId=%s",
                        app_uid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.owner_pubkey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}

bool CUniversalContractInvokeTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAccount;
    CAccount desAccount;

    if (!cw.accountCache.GetAccount(txUid, srcAccount)) {
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (!GenerateRegID(srcAccount, cw, state, height, index)) {
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

    CVmRunEnv vmRunEnv;
    std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    uint64_t fuelRate = GetFuelRate(cw.contractCache);

    int64_t llTime = GetTimeMillis();
    tuple<bool, uint64_t, string> ret = vmRunEnv.ExecuteContract(pTx, height, cw, fuelRate, nRunStep);
    if (!std::get<0>(ret))
        return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
            GetHash().GetHex(), std::get<2>(ret)), UPDATE_ACCOUNT_FAIL, "run-script-error: " + std::get<2>(ret));

    LogPrint("vm", "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    set<CKeyID> vAddress;
    CUserID userId;
    vector<std::shared_ptr<CAccount> > &vAccount = vmRunEnv.GetNewAccount();
    // Update accounts' info referred to the contract
    for (auto &itemAccount : vAccount) {
        vAddress.insert(itemAccount->keyid);
        userId = itemAccount->keyid;
        CAccount oldAcct;
        if (!cw.accountCache.GetAccount(userId, oldAcct)) {
            // The contract transfers money to an address for the first time.
            if (!itemAccount->keyid.IsNull()) {
                oldAcct.keyid = itemAccount->keyid;
            } else {
                return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, read account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
            }
        }
        if (!cw.accountCache.SetAccount(userId, *itemAccount))
            return state.DoS(100, ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, write account info error"),
                UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
    for (auto & itemUserAccount : vAppUserAccount) {
        CKeyID itemKeyID;
        bool bValid = GetKeyId(cw.accountCache, itemUserAccount.get()->GetAccUserId(), itemKeyID);
        if (bValid) {
            vAddress.insert(itemKeyID);
        }
    }

    if (!cw.contractCache.SetTxRelAccout(GetHash(), vAddress))
        return ERRORMSG("CUniversalContractInvokeTx::ExecuteTx, save tx relate account info to script db error");

    return true;
}

string CUniversalContractInvokeTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, app_uid=%s, coin_symbol=%s, coin_amount=%llu, fee_symbol=%s, "
        "llFees=%llu, arguments=%s, valid_height=%d\n",
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