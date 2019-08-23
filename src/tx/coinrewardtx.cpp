// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "coinrewardtx.h"

#include "main.h"

bool CCoinRewardTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    // Only used in stable coin genesis.
    return height == (int32_t)SysCfg().GetStableCoinGenesisHeight() ? true : false;
}

bool CCoinRewardTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    assert(txUid.type() == typeid(CPubKey));

    CAccount account;
    CRegID regId(height, index);
    CPubKey pubKey = txUid.get<CPubKey>();
    CKeyID keyId   = pubKey.IsFullyValid() ? txUid.get<CPubKey>().GetKeyId() : Hash160(regId.GetRegIdRaw());
    // Contstuct an empty account log which will delete account automatically if the blockchain rollbacked.

    account.nickid       = CNickID();
    account.owner_pubkey = pubKey;
    account.regid        = regId;
    account.keyid        = keyId;

    if (!account.OperateBalance(coin_symbol, ADD_FREE, coin_amount))
        return ERRORMSG("CCoinRewardTx::ExecuteTx: OperateBalance failed");

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CCoinRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    return true;
}

string CCoinRewardTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, addr=%s, coin_symbol=%s, coin_amount=%ld\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(),
                     txUid.get<CPubKey>().IsFullyValid() ? txUid.get<CPubKey>().GetKeyId().ToAddress() : "", coin_symbol,
                     coin_amount);
}

Object CCoinRewardTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    assert(txUid.type() == typeid(CPubKey));
    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("to_addr",        txUid.get<CPubKey>().GetKeyId().ToAddress()));
    result.push_back(Pair("coin_symbol",    coin_symbol));
    result.push_back(Pair("coin_amount",    coin_amount));
    result.push_back(Pair("valid_height",   valid_height));

    return result;
}
