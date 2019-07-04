// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "cointransfertx.h"

#include "main.h"

bool CCoinTranferTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    // TODO: fees in WICC/WGRT/WUSD
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    IMPLEMENT_CHECK_TX_REGID(toUid.type());

     // TODO: check range
    if (kCoinTypeMapName.count(CoinType(coinType)) == 0 || kCoinTypeMapName.count(CoinType(feesCoinType)) == 0) {
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, invalid coin type"), REJECT_INVALID,
                         "invalid-coin-type");
    }

    if ((txUid.type() == typeid(CPubKey)) && !txUid.get<CPubKey>().IsFullyValid())
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, public key is invalid"), REJECT_INVALID,
                         "bad-publickey");

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTransferTx::CheckTx, read account failed"), REJECT_INVALID,
                         "bad-getaccount");

    IMPLEMENT_CHECK_TX_SIGNATURE(srcAccount.pubKey);

    return true;
}

bool CCoinTranferTx::ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    // TODO: stamp tax when transfer WUSD

    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTranferTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    CAccountLog srcAccountLog(srcAccount);
    if (!srcAccount.OperateBalance(CoinType(feesCoinType), MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCoinTranferTx::ExecuteTx, insufficient bcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-bcoins");
    }

    if (!srcAccount.OperateBalance(CoinType(coinType), MINUS_VALUE, coins)) {
        return state.DoS(100, ERRORMSG("CCoinTranferTx::ExecuteTx, insufficient coins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coins");
    }

    if (!cw.accountCache.SaveAccount(srcAccount))
        return state.DoS(100, ERRORMSG("CCoinTranferTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(toUid, desAccount))
        return state.DoS(100, ERRORMSG("CCoinTranferTx::ExecuteTx, read toUid %s account info error", toUid.ToString()),
                        FCOIN_STAKE_FAIL, "bad-read-accountdb");

    CAccountLog desAccountLog(desAccount);
    if (!srcAccount.OperateBalance(CoinType(coinType), ADD_VALUE, coins)) {
        return state.DoS(100, ERRORMSG("CCoinTranferTx::ExecuteTx, failed to add coins in toUid %s account", toUid.ToString()),
                        UPDATE_ACCOUNT_FAIL, "failed-add-coins");
    }

    if (!cw.accountCache.SaveAccount(desAccount))
        return state.DoS(100, ERRORMSG("CCoinTranferTx::ExecuteTx, write dest addr %s account info error", toUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    cw.txUndo.accountLogs.push_back(srcAccountLog);
    cw.txUndo.accountLogs.push_back(desAccountLog);
    cw.txUndo.txid = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid, toUid}))
        return false;

    return true;
}

bool CCoinTranferTx::UndoExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCoinTranferTx::UndoExecuteTx, read account info error, userId=%s",
                userId.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CCoinTranferTx::UndoExecuteTx, undo operate account error, keyId=%s",
                            account.keyId.ToString()), UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCoinTranferTx::UndoExecuteTx, save account error"),
                            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
        }
    }

    return true;
}

string CCoinTranferTx::ToString(CAccountDBCache &accountCache) {
    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, coins=%ld, coinType=%s, llFees=%ld, feesCoinType=%s, "
        "nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toUid.ToString(), coins,
        GetCoinTypeName(CoinType(coinType)), llFees, GetCoinTypeName(CoinType(feesCoinType)), nValidHeight);
}

Object CCoinTranferTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID srcKeyId;
    accountCache.GetKeyId(txUid, srcKeyId);

    CKeyID desKeyId;
    accountCache.GetKeyId(toUid, desKeyId);

    result.push_back(Pair("hash",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("tx_uid",             txUid.ToString()));
    result.push_back(Pair("tx_addr",            srcKeyId.ToAddress()));
    result.push_back(Pair("to_uid",             toUid.ToString()));
    result.push_back(Pair("to_addr",            desKeyId.ToAddress()));
    result.push_back(Pair("coins",              coins));
    result.push_back(Pair("coin_type",          GetCoinTypeName(CoinType(coinType))));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("fees_coin_type",     GetCoinTypeName(CoinType(feesCoinType))));
    result.push_back(Pair("valid_height",       nValidHeight));
    result.push_back(Pair("memo",               HexStr(memo)));

    return result;
}

bool CCoinTranferTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId))
        return false;

    keyIds.insert(keyId);
    CKeyID desKeyId;
    if (!cw.accountCache.GetKeyId(toUid, desKeyId))
        return false;

    keyIds.insert(desKeyId);

    return true;
}