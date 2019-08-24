// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockrewardtx.h"

#include "entities/receipt.h"
#include "main.h"

bool CBlockRewardTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CBlockRewardTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, read source addr %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (0 == index) {
        // When the reward transaction is immature, should NOT update account's balances.
    } else if (-1 == index) {
        // When the reward transaction is mature, update account's balances, i.e, assign the reward value to
        // the target account.
        account.OperateBalance(SYMB::WICC, ADD_FREE, reward);

    } else {
        return ERRORMSG("CBlockRewardTx::ExecuteTx, invalid index");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyid), account))
        return state.DoS(100, ERRORMSG("CBlockRewardTx::ExecuteTx, write secure account info error"),
                         UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    return true;
}

string CBlockRewardTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyId=%s, reward=%ld\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), keyId.GetHex(), reward);
}

Object CBlockRewardTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("to_addr",        keyId.ToAddress()));
    result.push_back(Pair("reward_value",   reward));
    result.push_back(Pair("valid_height",   valid_height));

    return result;
}

bool CUCoinBlockRewardTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CUCoinBlockRewardTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(
            100, ERRORMSG("CUCoinBlockRewardTx::ExecuteTx, read source addr %s account info error", txUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (0 == index) {
        // When the reward transaction is immature, should NOT update account's balances.
    } else if (-1 == index) {
        // When the reward transaction is mature, update account's balances, i.e, assgin the reward values to
        // the target account.
        for (const auto &item : rewards) {
            uint64_t rewardAmount  = item.second;
            TokenSymbol coinSymbol = item.first;
            // FIXME: support WICC/WUSD only.
            if (coinSymbol == SYMB::WICC || coinSymbol == SYMB::WUSD)
                account.OperateBalance(coinSymbol, ADD_FREE, rewardAmount);
            else
                return ERRORMSG("CUCoinBlockRewardTx::ExecuteTx, invalid coin type");
        }

        // Assign profits to the delegate's account.
        account.OperateBalance(SYMB::WICC, ADD_FREE, profits);
    } else {
        return ERRORMSG("CUCoinBlockRewardTx::ExecuteTx, invalid index");
    }

    if (!cw.accountCache.SetAccount(CUserID(account.keyid), account))
        return state.DoS(100, ERRORMSG("CUCoinBlockRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    return true;
}

string CUCoinBlockRewardTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string reward;
    for (const auto &item : rewards) {
        reward += strprintf("%s: %lu, ", item.first, item.second);
    }

    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, addr=%s, rewards=%s, profits=%llu, valid_height=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), reward,
                     profits, valid_height);
}

Object CUCoinBlockRewardTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    Object reward;
    for (const auto &item : rewards) {
        reward.push_back(Pair(item.first, item.second));
    }

    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("to_addr",        keyId.ToAddress()));
    result.push_back(Pair("reward_value",   reward));
    result.push_back(Pair("profits",        profits));
    result.push_back(Pair("valid_height",   valid_height));

    return result;
}
