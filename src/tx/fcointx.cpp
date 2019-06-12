// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "fcointx.h"
#include "main.h"

bool CFCoinRewardTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    return true;
};

bool CFCoinRewardTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CFCoinRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog accountLog(account);

    // Reward fcoins to account.
    account.fcoins += rewardValue;
    if (!cw.accountCache.SetAccount(CUserID(account.keyID), account))
        return state.DoS(100, ERRORMSG("CFCoinRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    cw.txUndo.accountLogs.push_back(accountLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, {txUid})) return false;

    return true;
}

bool CFCoinRewardTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        if (!cw.accountCache.GetAccount(CUserID(rIterAccountLog->keyID), account)) {
            return state.DoS(100, ERRORMSG("CFCoinRewardTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CFCoinRewardTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(CUserID(rIterAccountLog->keyID), account)) {
            return state.DoS(100, ERRORMSG("CFCoinRewardTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    return true;
}

string CFCoinRewardTx::ToString(CAccountCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf("txType=%s, hash=%s, ver=%d, uid=%s, keyid=%s, rewardValue=%ld\n", GetTxType(nTxType),
                           GetHash().ToString(), nVersion, txUid.ToString(), keyId.GetHex(), rewardValue);

    return str;
}

Object CFCoinRewardTx::ToJson(const CAccountCache &accountCache) const{
    Object result;
    CKeyID keyid;
    accountCache.GetKeyId(txUid, keyid);

    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("tx_type", GetTxType(nTxType)));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("uid", txUid.ToString()));
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("micc", rewardValue));
    result.push_back(Pair("valid_height", nHeight));

    return result;
}

bool CFCoinRewardTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (txUid.type() == typeid(CRegID)) {
        if (!cw.accountCache.GetKeyId(txUid, keyId))
            return false;

        keyIds.insert(keyId);
    } else if (txUid.type() == typeid(CPubKey)) {
        CPubKey pubKey = txUid.get<CPubKey>();
        if (!pubKey.IsFullyValid())
            return false;

        keyIds.insert(pubKey.GetKeyId());
    }

    return true;
}