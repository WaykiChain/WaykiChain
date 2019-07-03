// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "coinrewardtx.h"

#include "main.h"

bool CCoinRewardTx::CheckTx(int height, CCacheWrapper &cw, CValidationState &state) {
    // TODO:
    return true;
}

bool CCoinRewardTx::ExecuteTx(int height, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCoinRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog accountLog(account);
    switch (coinType) {
        case CoinType::WICC: account.bcoins += coinValue; break;
        case CoinType::WUSD: account.scoins += coinValue; break;
        case CoinType::WGRT: account.fcoins += coinValue; break;
        default: return ERRORMSG("CCoinRewardTx::ExecuteTx, invalid coin type");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyId), account))
        return state.DoS(100, ERRORMSG("CCoinRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    cw.txUndo.accountLogs.push_back(accountLog);
    cw.txUndo.txid = GetHash();

    if (!SaveTxAddresses(height, nIndex, cw, state, {txUid}))
        return false;

    return true;
}

bool CCoinRewardTx::UndoExecuteTx(int height, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        if (!cw.accountCache.GetAccount(CUserID(rIterAccountLog->keyId), account)) {
            return state.DoS(100, ERRORMSG("CCoinRewardTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CCoinRewardTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(CUserID(rIterAccountLog->keyId), account)) {
            return state.DoS(100, ERRORMSG("CCoinRewardTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    return true;
}

string CCoinRewardTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyId=%s, coinType=%d, coinValue=%ld\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.GetHex(), coinType,
                     coinValue);
}

Object CCoinRewardTx::ToJson(const CAccountDBCache &accountCache) const{
    Object result;
    CKeyID keyId;
    accountCache.GetKeyId(txUid,            keyId);
    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("uid",            txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("coin_type",      GetCoinTypeName((CoinType)coinType)));
    result.push_back(Pair("coin_value",     coinValue));
    result.push_back(Pair("valid_height",   height));

    return result;
}

bool CCoinRewardTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);

    return true;
}