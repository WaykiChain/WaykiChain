// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "multicoinblockrewardtx.h"

#include "main.h"

bool CMultiCoinBlockRewardTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CMultiCoinBlockRewardTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CAccountLog accountLog(account);
    if (0 == nIndex) {
        // When the reward transaction is immature, should NOT update account's balances.
    } else if (-1 == nIndex) {
        // When the reward transaction is mature, update account's balances, i.e, assgin the reward values to
        // the miner's account.
        for (const auto &item : rewardValues) {
            switch (item.first/* CoinType */) {
                case CoinType::WICC: account.bcoins += item.second; break;
                case CoinType::WUSD: account.scoins += item.second; break;
                case CoinType::WGRT: account.fcoins += item.second; break;
                default: return ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, invalid coin type");
            }
        }

        // Assign profits to the delegate's account.
        account.bcoins += profits;
    } else {
        return ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, invalid index");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyId), account))
        return state.DoS(100, ERRORMSG("CMultiCoinBlockRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    cw.txUndo.accountLogs.push_back(accountLog);
    cw.txUndo.txid = GetHash();

    // Block reward transaction will execute twice, but need to save once when index equals to zero.
    if (nIndex == 0 && !SaveTxAddresses(nHeight, nIndex, cw, state, {txUid}))
        return false;

    return true;
}

bool CMultiCoinBlockRewardTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        if (!cw.accountCache.GetAccount(CUserID(rIterAccountLog->keyId), account)) {
            return state.DoS(100, ERRORMSG("CMultiCoinBlockRewardTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CMultiCoinBlockRewardTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(CUserID(rIterAccountLog->keyId), account)) {
            return state.DoS(100, ERRORMSG("CMultiCoinBlockRewardTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    return true;
}

map<CoinType, uint64_t> CMultiCoinBlockRewardTx::GetValues() const {
    map<CoinType, uint64_t> rewardValuesOut;
    for (const auto &item : rewardValues) {
        rewardValuesOut.emplace(CoinType(item.first), item.second);
    }

    return rewardValuesOut;
}

string CMultiCoinBlockRewardTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string rewardValue;
    for (const auto &item : rewardValues) {
        rewardValue += strprintf("%s: %lu, ", GetCoinTypeName((CoinType)item.first), item.second);
    }

    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyId=%s, %s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), keyId.GetHex(), rewardValue);
}

Object CMultiCoinBlockRewardTx::ToJson(const CAccountDBCache &accountCache) const{
    Object result;
    CKeyID keyId;
    accountCache.GetKeyId(txUid,            keyId);

    Object rewardValue;
    for (const auto &item : rewardValues) {
        switch (item.first /* CoinType */) {
            case CoinType::WICC: rewardValue.push_back(Pair("WICC", item.second)); break;
            case CoinType::WUSD: rewardValue.push_back(Pair("WUSD", item.second)); break;
            case CoinType::WGRT: rewardValue.push_back(Pair("WGRT", item.second)); break;
            default: break;
        }
    }

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("uid",            txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("reward_value",   rewardValue));
    result.push_back(Pair("valid_height",   nHeight));

    return result;
}

bool CMultiCoinBlockRewardTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
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