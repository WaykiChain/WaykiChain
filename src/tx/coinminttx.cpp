// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "coinminttx.h"

#include "main.h"

bool CCoinMintTx::CheckTx(CTxExecuteContext &context) {
    // Only used in stable coin genesis.
    return context.height == (int32_t) SysCfg().GetStableCoinGenesisHeight() ? true : false;
}

bool CCoinMintTx::ExecuteTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;

    if (!txAccount.OperateBalance(coin_symbol, ADD_FREE, coin_amount, ReceiptCode::COIN_MINT_ONCHAIN, receipts))
        return state.DoS(100, ERRORMSG("CCoinMintTx::ExecuteTx, operate account failed"), UPDATE_ACCOUNT_FAIL,
                         "operate-account-failed");

    return true;
}

string CCoinMintTx::ToString(CAccountDBCache &accountCache) {
    assert(txUid.is<CPubKey>() || txUid.is<CNullID>());
    string toAddr = txUid.is<CPubKey>() ? txUid.get<CPubKey>().GetKeyId().ToAddress() : "";

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, coin_symbol=%s, coin_amount=%llu, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toAddr, coin_symbol,
                     coin_amount, valid_height);
}

Object CCoinMintTx::ToJson(const CAccountDBCache &accountCache) const {
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
