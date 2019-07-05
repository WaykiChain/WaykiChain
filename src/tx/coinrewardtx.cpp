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
    assert(txUid.type() == typeid(CNullID) || txUid.type() == typeid(CPubKey));

    CAccount account;
    CRegID regId(height, index);
    CPubKey pubKey = txUid.type() == typeid(CNullID) ? CPubKey() : txUid.get<CPubKey>();
    CKeyID keyId = txUid.type() == typeid(CNullID) ? Hash160(regId.GetRegIdRaw()) : txUid.get<CPubKey>().GetKeyId();
    // Contstuct an empty account log which will delete account automatically if the blockchain rollbacked.
    CAccountLog accountLog(keyId);

    account.nickId = CNickID();
    account.pubKey = pubKey;
    account.regId  = regId;
    account.keyId  = keyId;

    switch (coinType) {
        case CoinType::WICC: account.bcoins += coins; break;
        case CoinType::WUSD: account.scoins += coins; break;
        case CoinType::WGRT: account.fcoins += coins; break;
        default: return ERRORMSG("CCoinRewardTx::ExecuteTx, invalid coin type");
    }

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CCoinRewardTx::ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    cw.txUndo.accountLogs.push_back(accountLog);
    cw.txUndo.txid = GetHash();

    if (!SaveTxAddresses(height, index, cw, state, {txUid}))
        return false;

    return true;
}

bool CCoinRewardTx::UndoExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        if (!cw.accountCache.EraseAccountByKeyId(CUserID(rIterAccountLog->keyId)) ||
            !cw.accountCache.EraseKeyId(CRegID(height, index))) {
            return state.DoS(100, ERRORMSG("CCoinRewardTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    return true;
}

string CCoinRewardTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyId=%s, coinType=%d, coins=%ld\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), txUid.get<CPubKey>().GetKeyId().GetHex(),
                     coinType, coins);
}

Object CCoinRewardTx::ToJson(const CAccountDBCache &accountCache) const{
    Object result;
    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("uid",            txUid.ToString()));
    result.push_back(Pair("addr",           txUid.get<CPubKey>().GetKeyId().GetHex()));
    result.push_back(Pair("coin_type",      GetCoinTypeName(CoinType(coinType))));
    result.push_back(Pair("coins",          coins));
    result.push_back(Pair("valid_height",   height));

    return result;
}

bool CCoinRewardTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    keyIds.insert(txUid.get<CPubKey>().GetKeyId());

    return true;
}