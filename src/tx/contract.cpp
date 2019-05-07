// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "contract.h"

#include "commons/serialize.h"
#include "txdb.h"
#include "crypto/hash.h"
#include "util.h"
#include "database.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "core.h"
#include "miner/miner.h"
#include "version.h"

static bool GetKeyId(const CAccountViewCache &view, const vector<unsigned char> &ret, CKeyID &KeyId) {
    if (ret.size() == 6) {
        CRegID regId(ret);
        KeyId = regId.GetKeyId(view);
    } else if (ret.size() == 34) {
        string addr(ret.begin(), ret.end());
        KeyId = CKeyID(addr);
    } else {
        return false;
    }

    if (KeyId.IsEmpty()) return false;

    return true;
}

bool CContractDeployTx::ExecuteTx(int nIndex, CAccountViewCache &view,CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CAccount acctInfo;
    CScriptDBOperLog operLog;
    if (!view.GetAccount(txUid, acctInfo)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, read regist addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccount acctInfoLog(acctInfo);
    uint64_t minusValue = llFees;
    if (minusValue > 0) {
        if(!acctInfo.OperateAccount(MINUS_FREE, minusValue, nHeight))
            return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, operate account failed ,regId=%s",
                            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");

        txundo.vAccountLog.push_back(acctInfoLog);
    }
    txundo.txHash = GetHash();

    CRegID regId(nHeight, nIndex);
    //create script account
    CKeyID keyId = Hash160(regId.GetRegIdRaw());
    CAccount account;
    account.keyID = keyId;
    account.regID = regId;
    account.nickID = CNickID();
    //save new script content
    if(!scriptDB.SetScript(regId, contractScript)){
        return state.DoS(100,
            ERRORMSG("CContractDeployTx::ExecuteTx, save script id %s script info error",
                regId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }
    if (!view.SaveAccountInfo(account)) {
        return state.DoS(100,
            ERRORMSG("CContractDeployTx::ExecuteTx, create new account script id %s script info error",
                regId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    nRunStep = contractScript.size();

    if(!operLog.vKey.empty()) {
        txundo.vScriptOperLog.push_back(operLog);
    }
    CUserID userId = acctInfo.keyID;
    if (!view.SetAccount(userId, acctInfo))
        return state.DoS(100, ERRORMSG("CContractDeployTx::ExecuteTx, save account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    if(SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(txUid, sendKeyId)) {
            return ERRORMSG("CContractDeployTx::ExecuteTx, get regAcctId by account error!");
        }

        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    return true;
}

bool CContractDeployTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CAccount account;
    CUserID userId;
    if (!view.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, read regist addr %s account info error", account.ToString()),
                         UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CRegID scriptId(nHeight, nIndex);
    //delete script content
    if (!scriptDB.EraseScript(scriptId)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, erase script id %s error", scriptId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "erase-script-failed");
    }
    //delete account
    if (!view.EraseKeyId(scriptId)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, erase script account %s error", scriptId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "erase-appkeyid-failed");
    }
    CKeyID keyId = Hash160(scriptId.GetRegIdRaw());
    if (!view.EraseAccountByKeyId(keyId)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, erase script account %s error", scriptId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "erase-appaccount-failed");
    }
    // LogPrint("INFO", "Delete regid %s app account\n", scriptId.ToString());

    for (auto &itemLog : txundo.vAccountLog) {
        if (itemLog.keyID == account.keyID) {
            if (!account.UndoOperateAccount(itemLog))
                return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, undo operate account error, keyId=%s", account.keyID.ToString()),
                                 UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, undo scriptdb data error"), UPDATE_ACCOUNT_FAIL, "undo-scriptdb-failed");
    }
    userId = account.keyID;
    if (!view.SetAccount(userId, account))
        return state.DoS(100, ERRORMSG("CContractDeployTx::UndoExecuteTx, save account error"), UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
    return true;
}

bool CContractDeployTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(txUid, keyId))
        return false;
    vAddr.insert(keyId);
    return true;
}

string CContractDeployTx::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(txUid, keyId);
    str += strprintf("txType=%s, hash=%s, ver=%d, accountId=%s, keyid=%s, llFees=%ld, nValidHeight=%d\n",
    GetTxType(nTxType), GetHash().ToString().c_str(), nVersion, txUid.ToString(), keyId.GetHex(), llFees, nValidHeight);
    return str;
}

Object CContractDeployTx::ToJson(const CAccountViewCache &AccountView) const{
    Object result;
    CAccountViewCache view(AccountView);

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

bool CContractDeployTx::CheckTx(CValidationState &state, CAccountViewCache &view,
                                           CScriptDBViewCache &scriptDB) {
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

    if (txUid.type() != typeid(CRegID)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, regAcctId must be CRegID"),
                         REJECT_INVALID, "regacctid-type-error");
    }

    if (!CheckMoneyRange(llFees)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, tx fee out of range"),
                         REJECT_INVALID, "fee-too-large");
    }

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(
            100, ERRORMSG("CContractDeployTx::CheckTx, tx fee smaller than MinTxFee"),
            REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    uint64_t llFuel = ceil(contractScript.size() / 100) * GetFuelRate(scriptDB);
    if (llFuel < 1 * COIN) {
        llFuel = 1 * COIN;
    }

    if (llFees < llFuel) {
        return state.DoS(100,
                         ERRORMSG("CContractDeployTx::CheckTx, register app tx fee too litter "
                                  "(actual:%lld vs need:%lld)",
                                  llFees, llFuel),
                         REJECT_INVALID, "fee-too-litter");
    }

    CAccount acctInfo;
    if (!view.GetAccount(txUid.get<CRegID>(), acctInfo)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, get account falied"),
                         REJECT_INVALID, "bad-getaccount");
    }

    if (!acctInfo.IsRegistered()) {
        return state.DoS(
            100, ERRORMSG("CContractDeployTx::CheckTx, account have not registed public key"),
            REJECT_INVALID, "bad-no-pubkey");
    }

    if (!CheckSignatureSize(signature)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, signature size invalid"),
                         REJECT_INVALID, "bad-tx-sig-size");
    }

    uint256 signhash = ComputeSignatureHash();
    if (!CheckSignScript(signhash, signature, acctInfo.pubKey)) {
        return state.DoS(100, ERRORMSG("CContractDeployTx::CheckTx, CheckSignScript failed"),
                         REJECT_INVALID, "bad-signscript-check");
    }
    return true;
}

/***----####################################################----*****/

bool CContractInvokeTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(txUid, keyId))
        return false;

    vAddr.insert(keyId);
    CKeyID desKeyId;
    if (!view.GetKeyId(appUid, desKeyId))
        return false;

    vAddr.insert(desKeyId);

    CVmRunEnv vmRunEnv;
    std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    uint64_t fuelRate = GetFuelRate(scriptDB);
    CScriptDBViewCache scriptDBView(scriptDB);

    if (!pTxCacheTip->HaveTx(GetHash())) {
        CAccountViewCache accountView(view);
        tuple<bool, uint64_t, string> ret = vmRunEnv.ExecuteContract(pTx, accountView, scriptDBView,
            chainActive.Height() + 1, fuelRate, nRunStep);

        if (!std::get<0>(ret))
            return ERRORMSG("CContractInvokeTx::GetAddress, %s", std::get<2>(ret));

        vector<shared_ptr<CAccount> > vpAccount = vmRunEnv.GetNewAccont();

        for (auto & item : vpAccount)
            vAddr.insert(item->keyID);

        vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
        for (auto & itemUserAccount : vAppUserAccount) {
            CKeyID itemKeyID;
            bool bValid = GetKeyId(view, itemUserAccount.get()->GetAccUserId(), itemKeyID);
            if (bValid)
                vAddr.insert(itemKeyID);
        }
    } else {
        set<CKeyID> vTxRelAccount;
        if (!scriptDBView.GetTxRelAccount(GetHash(), vTxRelAccount))
            return false;

        vAddr.insert(vTxRelAccount.begin(), vTxRelAccount.end());
    }
    return true;
}

string CContractInvokeTx::ToString(CAccountViewCache &view) const {
    string desId;
    if (txUid.type() == typeid(CKeyID)) {
        desId = appUid.get<CKeyID>().ToString();
    } else if (appUid.type() == typeid(CRegID)) {
        desId = appUid.get<CRegID>().ToString();
    }

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, srcId=%s, desId=%s, bcoins=%ld, llFees=%ld, arguments=%s, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString().c_str(), nVersion,
        txUid.get<CRegID>().ToString(), desId.c_str(), bcoins, llFees,
        HexStr(arguments).c_str(), nValidHeight);

    return str;
}

Object CContractInvokeTx::ToJson(const CAccountViewCache &AccountView) const {
    Object result;
    CAccountViewCache view(AccountView);

    // auto GetRegIdString = [&](CUserID const &userId) {
    //     if (userId.type() == typeid(CRegID))
    //         return userId.get<CRegID>().ToString();
    //     return string("");
    // };

    CKeyID srcKeyId, desKeyId;
    view.GetKeyId(txUid, srcKeyId);
    view.GetKeyId(appUid, desKeyId);

    result.push_back(Pair("hash",       GetHash().GetHex()));
    result.push_back(Pair("tx_type",    GetTxType(nTxType)));
    result.push_back(Pair("ver",        nVersion));
    result.push_back(Pair("regid",      txUid.ToString()));
    result.push_back(Pair("addr",       srcKeyId.ToAddress()));
    result.push_back(Pair("app_uid",    appUid.ToString()));
    result.push_back(Pair("app_addr",  desKeyId.ToAddress()));
    result.push_back(Pair("money",      bcoins));
    result.push_back(Pair("fees",       llFees));
    result.push_back(Pair("arguments",  HexStr(arguments)));
    result.push_back(Pair("valid_height", nValidHeight));

    return result;
}

bool CContractInvokeTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                            CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                            CScriptDBViewCache &scriptDB) {
    CAccount srcAcct;
    CAccount desAcct;
    CAccountLog desAcctLog;
    uint64_t minusValue = llFees + bcoins;
    if (!view.GetAccount(txUid, srcAcct))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, read source addr %s account info error",
            txUid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccountLog srcAcctLog(srcAcct);
    if (!srcAcct.OperateAccount(MINUS_FREE, minusValue, nHeight))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, accounts insufficient funds"),
            UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    CUserID userId = srcAcct.keyID;
    if (!view.SetAccount(userId, srcAcct))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, save account%s info error",
            txUid.get<CRegID>().ToString()), WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    uint64_t addValue = bcoins;
    if (!view.GetAccount(appUid, desAcct)) {
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, get account info failed by regid:%s",
            appUid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        desAcctLog.SetValue(desAcct);
    }

    if (!desAcct.OperateAccount(ADD_FREE, addValue, nHeight))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, operate accounts error"),
            UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");

    if (!view.SetAccount(appUid, desAcct))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, save account error, kyeId=%s",
            desAcct.keyID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    txundo.vAccountLog.push_back(srcAcctLog);
    txundo.vAccountLog.push_back(desAcctLog);

    vector<unsigned char> vScript;
    if (!scriptDB.GetScript(appUid.get<CRegID>(), vScript))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, read script faild, regId=%s",
            appUid.get<CRegID>().ToString()), READ_ACCOUNT_FAIL, "bad-read-script");

    CVmRunEnv vmRunEnv;
    std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    uint64_t fuelRate = GetFuelRate(scriptDB);

    int64_t llTime = GetTimeMillis();
    tuple<bool, uint64_t, string> ret = vmRunEnv.ExecuteContract(pTx, view, scriptDB, nHeight, fuelRate, nRunStep);
    if (!std::get<0>(ret))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, txid=%s run script error:%s",
            GetHash().GetHex(), std::get<2>(ret)), UPDATE_ACCOUNT_FAIL, "run-script-error: " + std::get<2>(ret));

    LogPrint("vm", "execute contract elapse:%lld, txhash=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    set<CKeyID> vAddress;
    vector<std::shared_ptr<CAccount> > &vAccount = vmRunEnv.GetNewAccont();
    // update accounts' info refered to the contract
    for (auto &itemAccount : vAccount) {
        vAddress.insert(itemAccount->keyID);
        userId = itemAccount->keyID;
        CAccount oldAcct;
        if (!view.GetAccount(userId, oldAcct)) {
            // The contract transfers money to an address for the first time.
            if (!itemAccount->keyID.IsNull()) {
                oldAcct.keyID = itemAccount->keyID;
            } else {
                return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, read account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
            }
        }
        CAccountLog oldAcctLog(oldAcct);
        if (!view.SetAccount(userId, *itemAccount))
            return state.DoS(100, ERRORMSG("CContractInvokeTx::ExecuteTx, write account info error"),
                UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");

        txundo.vAccountLog.push_back(oldAcctLog);
    }
    txundo.vScriptOperLog.insert(txundo.vScriptOperLog.end(), vmRunEnv.GetDbLog()->begin(),
                                 vmRunEnv.GetDbLog()->end());
    vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
    for (auto & itemUserAccount : vAppUserAccount) {
        CKeyID itemKeyID;
        bool bValid = GetKeyId(view, itemUserAccount.get()->GetAccUserId(), itemKeyID);
        if (bValid)
            vAddress.insert(itemKeyID);
    }

    if (!scriptDB.SetTxRelAccout(GetHash(), vAddress))
        return ERRORMSG("CContractInvokeTx::ExecuteTx, save tx relate account info to script db error");

    txundo.txHash = GetHash();

    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        CKeyID revKeyId;
        if (!view.GetKeyId(txUid, sendKeyId))
            return ERRORMSG("CContractInvokeTx::ExecuteTx, get keyid by txUid error!");

        if (!view.GetKeyId(appUid, revKeyId))
            return ERRORMSG("CContractInvokeTx::ExecuteTx, get keyid by appUid error!");

        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                         operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);

        if (!scriptDB.SetTxHashByAddress(revKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                         operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CContractInvokeTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                                CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                                CScriptDBViewCache &scriptDB) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = txundo.vAccountLog.rbegin();
    for (; rIterAccountLog != txundo.vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!view.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100,
                             ERRORMSG("CContractInvokeTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!view.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, undo scriptdb data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    if (!scriptDB.EraseTxRelAccout(GetHash()))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::UndoExecuteTx, erase tx rel account error"),
                         UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

    return true;
}

bool CContractInvokeTx::CheckTx(CValidationState &state, CAccountViewCache &view,
                          CScriptDBViewCache &scriptDB) {
    if (arguments.size() > kContractArgumentMaxSize)
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, arguments's size too large"),
                         REJECT_INVALID, "arguments-size-toolarge");

    if (txUid.type() != typeid(CRegID))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, srcRegId must be CRegID"),
                         REJECT_INVALID, "srcaddr-type-error");

    if (appUid.type() != typeid(CRegID))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, desUserId must be CRegID"),
                         REJECT_INVALID, "desaddr-type-error");

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, tx fee out of money range"),
                         REJECT_INVALID, "bad-appeal-fee-toolarge");

    if (!CheckMinTxFee(llFees))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, tx fee smaller than MinTxFee"),
                         REJECT_INVALID, "bad-tx-fee-toosmall");

    CAccount srcAccount;
    if (!view.GetAccount(txUid.get<CRegID>(), srcAccount))
        return state.DoS(100,
                         ERRORMSG("CContractInvokeTx::CheckTx, read account failed, regId=%s",
                                  txUid.get<CRegID>().ToString()),
                         REJECT_INVALID, "bad-getaccount");

    if (!srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, account pubkey not registered"),
                         REJECT_INVALID, "bad-account-unregistered");

    vector<unsigned char> vScript;
    if (!scriptDB.GetScript(appUid.get<CRegID>(), vScript))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, read script faild, regId=%s",
                        appUid.get<CRegID>().ToString()),
                        REJECT_INVALID, "bad-read-script");

    if (!CheckSignatureSize(signature))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, signature size invalid"),
                         REJECT_INVALID, "bad-tx-sig-size");

    uint256 sighash = ComputeSignatureHash();
    if (!CheckSignScript(sighash, signature, srcAccount.pubKey))
        return state.DoS(100, ERRORMSG("CContractInvokeTx::CheckTx, CheckSignScript failed"),
                         REJECT_INVALID, "bad-signscript-check");

    return true;
}