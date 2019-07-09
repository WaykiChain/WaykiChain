// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contracttx.h"

#include "accounts/vote.h"
#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "commons/util.h"
#include "config/version.h"
#include "vm/luavm/vmrunenv.h"

static bool GetKeyId(const CAccountDBCache &view, const string &userIdStr, CKeyID &KeyId) {
    if (userIdStr.size() == 6) {
        CRegID regId(userIdStr);
        KeyId = regId.GetKeyId(view);
    } else if (userIdStr.size() == 34) {
        string addr(userIdStr.begin(), userIdStr.end());
        KeyId = CKeyID(addr);
    } else {
        return false;
    }

    if (KeyId.IsEmpty()) return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CContractDeployTx

bool CContractDeployTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, read regist addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccount accountLog(account);
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
            return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, operate account failed ,regId=%s",
                            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyId), account))
        return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, save account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    cw.txUndo.accountLogs.push_back(accountLog);
    cw.txUndo.txid = GetHash();

    // create script account
    CAccount contractAccount;
    CRegID contractRegId(nHeight, nIndex);
    CKeyID keyId           = Hash160(contractRegId.GetRegIdRaw());
    contractAccount.keyId  = keyId;
    contractAccount.regId  = contractRegId;
    contractAccount.nickId = CNickID();

    // save new script content
    if (!cw.contractCache.SetScript(contractRegId, contractScript)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, save script id %s script info error",
            contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }
    if (!cw.accountCache.SaveAccount(contractAccount)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, create new account script id %s script info error",
            contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    nRunStep = contractScript.size();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CContractDeployTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {

    if (!UndoTxAddresses(cw, state)) return false;

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, read regist addr %s account info error",
                         account.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CRegID contractRegId(nHeight, nIndex);
    // delete script content
    if (!cw.contractCache.EraseScript(contractRegId)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, erase script id %s error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "erase-script-failed");
    }
    // delete account
    if (!cw.accountCache.EraseKeyId(contractRegId)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, erase script account %s error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "erase-appkeyid-failed");
    }
    CKeyID keyId = Hash160(contractRegId.GetRegIdRaw());
    if (!cw.accountCache.EraseAccountByKeyId(keyId)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, erase script account %s error",
                        contractRegId.ToString()), UPDATE_ACCOUNT_FAIL, "erase-appaccount-failed");
    }
    // LogPrint("INFO", "Delete regid %s app account\n", contractRegId.ToString());

    for (auto &itemLog : cw.txUndo.accountLogs) {
        if (!account.UndoOperateAccount(itemLog))
            return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, undo operate account error, keyId=%s",
                            account.keyId.ToString()), UPDATE_ACCOUNT_FAIL, "undo-account-failed");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyId), account))
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, save account error"),
                         UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");


    if (!cw.contractCache.UndoTxHashByAddress(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, undo contractCache data error"),
                         UPDATE_ACCOUNT_FAIL, "undo-contractCache-failed");
    }

    if (!cw.contractCache.EraseTxRelAccout(GetHash()))
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, erase tx rel account error"),
                         UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    return true;
}

bool CContractDeployTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);
    return true;
}

string CContractDeployTx::ToString(CAccountDBCache &view) {
    CKeyID keyId;
    view.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, accountId=%s, keyid=%s, llFees=%ld, nValidHeight=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.GetHex(), llFees,
                     nValidHeight);
}

Object CContractDeployTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    CAccountDBCache view(accountCache);

    CKeyID keyid;
    view.GetKeyId(txUid, keyid);

    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("tx_type", GetTxType(nTxType)));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid", txUid.get<CRegID>().ToString()));
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("script", "script_content"));
    result.push_back(Pair("fees", llFees));
    result.push_back(Pair("valid_height", nValidHeight));
    return result;
}

bool CContractDeployTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    CDataStream stream(contractScript, SER_DISK, CLIENT_VERSION);
    CVmScript vmScript;
    try {
        stream >> vmScript;
    } catch (exception &e) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, unserialize to vmScript error"),
                         REJECT_INVALID, "unserialize-error");
    }

    if (!vmScript.IsValid()) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, vmScript is invalid"),
                         REJECT_INVALID, "vmscript-invalid");
    }

    uint64_t llFuel = GetFuel(GetFuelRate(cw.contractCache));
    if (llFees < llFuel) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, fee too litter to afford fuel "
                         "(actual:%lld vs need:%lld)", llFees, llFuel),
                         REJECT_INVALID, "fee-too-litter-to-afford-fuel");
    }

    unsigned int nTxSize = ::GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    double dFeePerKb     = double(llFees - llFuel) / (double(nTxSize) / 1000.0);
    if (dFeePerKb < CBaseTx::nMinRelayTxFee) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, fee too litter in fees/Kb "
                        "(actual:%.4f vs need:%lld)", dFeePerKb, CBaseTx::nMinRelayTxFee),
                        REJECT_INVALID, "fee-too-litter-in-fees/Kb");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, get account failed"),
                         REJECT_INVALID, "bad-getaccount");
    }
    if (!account.IsRegistered()) {
        return state.DoS(
            100, ERRORMSG("CContractDeployTx::CheckTx, account unregistered"),
            REJECT_INVALID, "bad-account-unregistered");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(account.pubKey);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CContractInvokeTx

bool CContractInvokeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);
    CKeyID desKeyId;
    if (!cw.accountCache.GetKeyId(appUid, desKeyId))
        return false;

    keyIds.insert(desKeyId);

    //FIXME below:

    // CVmRunEnv vmRunEnv;
    // std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    // uint64_t fuelRate = GetFuelRate(cw.contractCache);

    // if (!pCdMan->pTxCache->HaveTx(GetHash())) {
    //     tuple<bool, uint64_t, string> ret =
    //         vmRunEnv.ExecuteContract(pTx, chainActive.Height() + 1, cw, fuelRate, nRunStep);

    //     if (!std::get<0>(ret))
    //         return ERRORMSG("CContractInvokeTx::GetInvolvedKeyIds, %s", std::get<2>(ret));

    //     vector<shared_ptr<CAccount> > vpAccount = vmRunEnv.GetNewAccount();

    //     for (auto & item : vpAccount)
    //         keyIds.insert(item->keyId);

    //     vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
    //     for (auto & itemUserAccount : vAppUserAccount) {
    //         CKeyID itemKeyID;
    //         bool bValid = GetKeyId(cw.accountCache, itemUserAccount.get()->GetAccUserId(), itemKeyID);
    //         if (bValid)
    //             keyIds.insert(itemKeyID);
    //     }
    // } else {
    //     set<CKeyID> vTxRelAccount;
    //     if (!cw.contractCache.GetTxRelAccount(GetHash(), vTxRelAccount))
    //         return false;

    //     keyIds.insert(vTxRelAccount.begin(), vTxRelAccount.end());
    // }
    return true;
}

string CContractInvokeTx::ToString(CAccountDBCache &view) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, appUid=%s, bcoins=%ld, llFees=%ld, arguments=%s, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), appUid.ToString(), bcoins, llFees,
        HexStr(arguments), nValidHeight);
}

Object CContractInvokeTx::ToJson(const CAccountDBCache &accountView) const {
    Object result;
    CAccountDBCache view(accountView);

    // auto GetRegIdString = [&](CUserID const &userId) {
    //     if (userId.type() == typeid(CRegID))
    //         return userId.get<CRegID>().ToString();
    //     return string("");
    // };

    CKeyID srcKeyId, desKeyId;
    view.GetKeyId(txUid, srcKeyId);
    view.GetKeyId(appUid, desKeyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("regid",          txUid.ToString()));
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));
    result.push_back(Pair("app_uid",        appUid.ToString()));
    result.push_back(Pair("app_addr",       desKeyId.ToAddress()));
    result.push_back(Pair("money",          bcoins));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("arguments",      HexStr(arguments)));
    result.push_back(Pair("valid_height",   nValidHeight));

    return result;
}

bool CContractInvokeTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAcct;
    CAccount desAcct;
    bool generateRegID = false;

    if (!cw.accountCache.GetAccount(txUid, srcAcct)) {
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        if (txUid.type() == typeid(CPubKey)) {
            srcAcct.pubKey = txUid.get<CPubKey>();

            CRegID regId;
            // If the source account does NOT have CRegID, need to generate a new CRegID.
            if (!cw.accountCache.GetRegId(txUid, regId)) {
                srcAcct.regId = CRegID(nHeight, nIndex);
                generateRegID = true;
            }
        }
    }

    CAccountLog srcAcctLog(srcAcct);
    CAccountLog desAcctLog;
    uint64_t minusValue = llFees + bcoins;
    if (!srcAcct.OperateBalance(CoinType::WICC, MINUS_VALUE, minusValue))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, accounts hash insufficient funds"),
            UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (generateRegID) {
        if (!cw.accountCache.SaveAccount(srcAcct))
            return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    } else {
        if (!cw.accountCache.SetAccount(CUserID(srcAcct.keyId), srcAcct))
            return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    if (!cw.accountCache.GetAccount(appUid, desAcct)) {
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, get account info failed by regid:%s",
            appUid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        desAcctLog.SetValue(desAcct);
    }

    if (!desAcct.OperateBalance(CoinType::WICC, ADD_VALUE, bcoins)) {
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, operate accounts error"),
                        UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }

    if (!cw.accountCache.SetAccount(appUid, desAcct))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, save account error, kyeId=%s",
            desAcct.keyId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    cw.txUndo.accountLogs.push_back(srcAcctLog);
    cw.txUndo.accountLogs.push_back(desAcctLog);

    string contractScript;
    if (!cw.contractCache.GetScript(appUid.get<CRegID>(), contractScript))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, read script failed, regId=%s",
            appUid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-script");

    CVmRunEnv vmRunEnv;
    std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    uint64_t fuelRate = GetFuelRate(cw.contractCache);

    int64_t llTime = GetTimeMillis();
    tuple<bool, uint64_t, string> ret = vmRunEnv.ExecuteContract(pTx, nHeight, cw, fuelRate, nRunStep);
    if (!std::get<0>(ret))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
            GetHash().GetHex(), std::get<2>(ret)), UPDATE_ACCOUNT_FAIL, "run-script-error: " + std::get<2>(ret));

    LogPrint("vm", "execute contract elapse: %lld, txid=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    set<CKeyID> vAddress;
    CUserID userId;
    vector<std::shared_ptr<CAccount> > &vAccount = vmRunEnv.GetNewAccount();
    // Update accounts' info referred to the contract
    for (auto &itemAccount : vAccount) {
        vAddress.insert(itemAccount->keyId);
        userId = itemAccount->keyId;
        CAccount oldAcct;
        if (!cw.accountCache.GetAccount(userId, oldAcct)) {
            // The contract transfers money to an address for the first time.
            if (!itemAccount->keyId.IsNull()) {
                oldAcct.keyId = itemAccount->keyId;
            } else {
                return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, read account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
            }
        }
        CAccountLog oldAcctLog(oldAcct);
        if (!cw.accountCache.SetAccount(userId, *itemAccount))
            return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, write account info error"),
                UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");

        cw.txUndo.accountLogs.push_back(oldAcctLog);
    }
    //cw.txUndo.dbOpLogMap.AddOpLogs(dbk::CONTRACT_DATA, *vmRunEnv.GetDbLog());

    vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
    for (auto & itemUserAccount : vAppUserAccount) {
        CKeyID itemKeyID;
        bool bValid = GetKeyId(cw.accountCache, itemUserAccount.get()->GetAccUserId(), itemKeyID);
        if (bValid) {
            vAddress.insert(itemKeyID);
        }
    }

    if (!cw.contractCache.SetTxRelAccout(GetHash(), vAddress))
        return ERRORMSG("CContractInvokeTx::ExecuteTx, save tx relate account info to script db error");

    cw.txUndo.txid = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid, appUid})) return false;

    return true;
}

bool CContractInvokeTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {

    if (!UndoTxAddresses(cw, state)) return false;

    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (account.IsEmptyValue() && account.regId.IsEmpty() &&
            (!account.pubKey.IsFullyValid() || account.pubKey.GetKeyId() != account.keyId)) {
            // Target account has NO CRegID(first involved in transacion)
            cw.accountCache.EraseAccountByKeyId(userId);
        } else if (account.regId == CRegID(nHeight, nIndex)) {
            // If the CRegID was generated by this CONTRACT_INVOKE_TX, need to remove CRegID.
            CPubKey empPubKey;
            account.pubKey      = empPubKey;
            account.minerPubKey = empPubKey;
            account.regId.Clean();

            if (!cw.accountCache.SetAccount(userId, account)) {
                return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }

            cw.accountCache.EraseKeyId(CRegID(nHeight, nIndex));
        } else {
            if (!cw.accountCache.SetAccount(userId, account)) {
                return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }
        }
    }

    if (!CVmRunEnv::UndoDatas(cw)) {
        return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, undo datas produced by contract error"),
                         UPDATE_ACCOUNT_FAIL, "undo-contract-datas-failed");
    }

    if (!cw.contractCache.EraseTxRelAccout(GetHash()))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, erase tx rel account error"),
                         UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

    return true;
}

bool CContractInvokeTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_ARGUMENTS;
    IMPLEMENT_CHECK_TX_REGID_OR_PUBKEY(txUid.type());
    IMPLEMENT_CHECK_TX_APPID(appUid.type());

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, read account failed, regId=%s",
                        txUid.get<CRegID>().ToString()), REJECT_INVALID, "bad-getaccount");

    if (!srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, account unregistered"),
                        REJECT_INVALID, "bad-account-unregistered");

    string contractScript;
    if (!cw.contractCache.GetScript(appUid.get<CRegID>(), contractScript))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, read script failed, regId=%s",
                        appUid.get<CRegID>().ToString()), REJECT_INVALID, "bad-read-script");

    CPubKey pubKey = (txUid.type() == typeid(CPubKey) ? txUid.get<CPubKey>() : srcAccount.pubKey);
    IMPLEMENT_CHECK_TX_SIGNATURE(pubKey);

    return true;
}