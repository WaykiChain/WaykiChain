// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php


#include "blockreward.h"

#include "commons/serialize.h"
#include "tx.h"
#include "persistence/contractdb.h"
#include "crypto/hash.h"
#include "util.h"
#include "database.h"
#include "main.h"
#include "vm/vmrunenv.h"
#include "core.h"
#include "miner/miner.h"
#include "version.h"

string CBlockRewardTx::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(txUid, keyId);
    CRegID regId;
    view.GetRegId(txUid, regId);
    str += strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyid=%s, rewardValue=%ld\n",
        GetTxType(nTxType), GetHash().ToString().c_str(), nVersion, regId.ToString(), keyId.GetHex(), rewardValue);

    return str;
}

Object CBlockRewardTx::ToJson(const CAccountViewCache &AccountView) const{
    Object result;
    CAccountViewCache view(AccountView);
    CKeyID keyid;
    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("tx_type", GetTxType(nTxType)));
    result.push_back(Pair("ver", nVersion));
    if (txUid.type() == typeid(CRegID)) {
        result.push_back(Pair("regid", txUid.get<CRegID>().ToString()));
    }
    if (txUid.type() == typeid(CPubKey)) {
        result.push_back(Pair("pubkey", txUid.get<CPubKey>().ToString()));
    }
    view.GetKeyId(txUid, keyid);
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("money", rewardValue));
    result.push_back(Pair("valid_height", nHeight));
    return result;
}

bool CBlockRewardTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view,
                           CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (txUid.type() == typeid(CRegID)) {
        if (!view.GetKeyId(txUid, keyId)) return false;
        vAddr.insert(keyId);
    } else if (txUid.type() == typeid(CPubKey)) {
        CPubKey pubKey = txUid.get<CPubKey>();
        if (!pubKey.IsFullyValid()) return false;
        vAddr.insert(pubKey.GetKeyId());
    }

    return true;
}

bool CBlockRewardTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                          CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                          CScriptDBViewCache &scriptDB) {
    if (txUid.type() != typeid(CRegID)) {
        return state.DoS(100,
            ERRORMSG("CBlockRewardTx::ExecuteTx, account %s error, data type must be CRegID",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-account");
    }

    CAccount acctInfo;
    if (!view.GetAccount(txUid, acctInfo)) {
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    // LogPrint("op_account", "before operate:%s\n", acctInfo.ToString());
    CAccountLog acctInfoLog(acctInfo);
    if (0 == nIndex) {
        // nothing to do here
    } else if (-1 == nIndex) {  // maturity reward tx, only update values
        acctInfo.bcoins += rewardValue;
    } else {  // never go into this step
        return ERRORMSG("nIndex type error!");
    }

    CUserID userId = acctInfo.keyID;
    if (!view.SetAccount(userId, acctInfo))
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    txundo.Clear();
    txundo.vAccountLog.push_back(acctInfoLog);
    txundo.txHash = GetHash();
    if (SysCfg().GetAddressToTxFlag() && 0 == nIndex) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if (!view.GetKeyId(txUid, sendKeyId))
            return ERRORMSG("CBlockRewardTx::ExecuteTx, get keyid by account error!");

        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    // LogPrint("op_account", "after operate:%s\n", acctInfo.ToString());
    return true;
}

bool CBlockRewardTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                              CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                              CScriptDBViewCache &scriptDB) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = txundo.vAccountLog.rbegin();
    for (; rIterAccountLog != txundo.vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!view.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBlockRewardTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CBlockRewardTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!view.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBlockRewardTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CBlockRewardTx::UndoExecuteTx, undo scriptdb data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    return true;
}