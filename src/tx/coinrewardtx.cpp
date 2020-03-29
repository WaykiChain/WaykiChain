// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "coinrewardtx.h"

#include "main.h"

bool CCoinRewardTx::CheckTx(CTxExecuteContext &context) {
    // Only used in stable coin genesis.
    return context.height == (int32_t) SysCfg().GetStableCoinGenesisHeight() ? true : false;
}

bool CCoinRewardTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    if (!txAccount.OperateBalance(coin_symbol, ADD_FREE, coin_amount, ReceiptCode::COIN_BLOCK_REWARD_TO_MINER, receipts))
        return state.DoS(100, ERRORMSG("CCoinRewardTx::ExecuteTx, operate account failed"), UPDATE_ACCOUNT_FAIL,
                         "operate-account-failed");

    return true;
}

string CCoinRewardTx::ToString(CAccountDBCache &accountCache) {
    assert(txUid.is<CPubKey>() || txUid.is<CNullID>());
    string toAddr = txUid.is<CPubKey>() ? txUid.get<CPubKey>().GetKeyId().ToAddress() : "";

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, coin_symbol=%s, coin_amount=%llu, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toAddr, coin_symbol,
                     coin_amount, valid_height);
}

Object CCoinRewardTx::ToJson(const CAccountDBCache &accountCache) const {
    assert(txUid.is<CPubKey>() || txUid.is<CNullID>());
    Object result;
    string toAddr = txUid.is<CPubKey>() ? txUid.get<CPubKey>().GetKeyId().ToAddress() : "";
    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("to_addr",        toAddr));
    result.push_back(Pair("coin_symbol",    coin_symbol));
    result.push_back(Pair("coin_amount",    coin_amount));
    result.push_back(Pair("valid_height",   valid_height));

    return result;
}
