// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "fcointx.h"
#include "main.h"

bool CFcoinTransferTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    IMPLEMENT_CHECK_TX_REGID(toUid.type());

    if (!CheckFundCoinRange(fcoins)) {
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::CheckTx, fcoins out of range"),
                        REJECT_INVALID, "bad-tx-fcoins-outofrange");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CFcoinTransferTx::ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    CAccountLog srcAccountLog(srcAccount);
    if (!srcAccount.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::ExecuteTx, insufficient bcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-bcoins");
    }

    if (!srcAccount.OperateBalance(CoinType::WGRT, MINUS_VALUE, fcoins)) {
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::ExecuteTx, insufficient fcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-fcoins");
    }

    if (!cw.accountCache.SaveAccount(srcAccount))
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(toUid, desAccount))
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::ExecuteTx, read toUid %s account info error", toUid.ToString()),
                        FCOIN_STAKE_FAIL, "bad-read-accountdb");

    CAccountLog desAccountLog(desAccount);
    if (!srcAccount.OperateBalance(CoinType::WGRT, ADD_VALUE, fcoins)) {
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::ExecuteTx, failed to add fcoins in toUid %s account", toUid.ToString()),
                        UPDATE_ACCOUNT_FAIL, "failed-add-fcoins");
    }

    if (!cw.accountCache.SaveAccount(desAccount))
        return state.DoS(100, ERRORMSG("CFcoinTransferTx::ExecuteTx, write dest addr %s account info error", toUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    cw.txUndo.accountLogs.push_back(srcAccountLog);
    cw.txUndo.accountLogs.push_back(desAccountLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid, toUid}))
        return false;

    return true;
}

bool CFcoinTransferTx::UndoExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CFcoinTransferTx::UndoExecuteTx, read account info error, userId=%s",
                userId.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CFcoinTransferTx::UndoExecuteTx, undo operate account error, keyId=%s",
                            account.keyId.ToString()), UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CFcoinTransferTx::UndoExecuteTx, save account error"),
                            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
        }
    }

    return true;
}

string CFcoinTransferTx::ToString(CAccountDBCache &accountCache) {
    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, fcoins=%ld, llFees=%ld, nValidHeight=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), toUid.ToString(), fcoins,
                     llFees, nValidHeight);
}

Object CFcoinTransferTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID srcKeyId;
    accountCache.GetKeyId(txUid, srcKeyId);

    CKeyID desKeyId;
    accountCache.GetKeyId(toUid, desKeyId);

    result.push_back(Pair("hash",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("uid",                txUid.ToString()));
    result.push_back(Pair("addr",               srcKeyId.ToAddress()));
    result.push_back(Pair("dest_uid",           toUid.ToString()));
    result.push_back(Pair("dest_addr",          desKeyId.ToAddress()));
    result.push_back(Pair("fcoins",             fcoins));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("memo",               HexStr(memo)));
    result.push_back(Pair("valid_height",       nValidHeight));

    return result;
}

bool CFcoinTransferTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
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