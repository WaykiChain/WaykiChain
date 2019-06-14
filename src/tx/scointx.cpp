// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "scointx.h"
#include "main.h"

bool CScoinTransferTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    // Transaction fee should be no less than nMinTxFee in WICC or WUSD.
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_MEMO;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    IMPLEMENT_CHECK_TX_REGID(toUid.type());

    if (feesCoinType != CoinType::WICC && feesCoinType != CoinType::WUSD) {
        return state.DoS(100, ERRORMSG("CScoinTransferTx::CheckTx, fees coin type error"),
                        REJECT_INVALID, "bad-tx-fees-coin-type-error");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CScoinTransferTx::ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount srcAccount;
    if (!cw.accountCache.GetAccount(txUid, srcAccount))
        return state.DoS(100, ERRORMSG("CScoinTransferTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccountLog srcAccountLog(srcAccount);
    if (!srcAccount.OperateBalance(feesCoinType, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CScoinTransferTx::ExecuteTx, insufficient coins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-coins");
    }

    // TODO: stamp tax

    if (!srcAccount.OperateBalance(CoinType::WUSD, MINUS_VALUE, scoins)) {
        return state.DoS(100, ERRORMSG("CScoinTransferTx::ExecuteTx, insufficient scoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-scoins");
    }

    if (!cw.accountCache.SaveAccount(srcAccount))
        return state.DoS(100, ERRORMSG("CScoinTransferTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccount desAccount;
    if (!cw.accountCache.GetAccount(toUid, desAccount))
        return state.DoS(100, ERRORMSG("CScoinTransferTx::ExecuteTx, read toUid %s account info error", toUid.ToString()),
                        FCOIN_STAKE_FAIL, "bad-read-accountdb");

    CAccountLog desAccountLog(desAccount);
    if (!srcAccount.OperateBalance(CoinType::WUSD, ADD_VALUE, scoins)) {
        return state.DoS(100, ERRORMSG("CScoinTransferTx::ExecuteTx, failed to add scoins in toUid %s account", toUid.ToString()),
                        UPDATE_ACCOUNT_FAIL, "failed-add-scoins");
    }

    if (!cw.accountCache.SaveAccount(desAccount))
        return state.DoS(100, ERRORMSG("CScoinTransferTx::ExecuteTx, write dest addr %s account info error", toUid.ToString()),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    cw.txUndo.accountLogs.push_back(srcAccountLog);
    cw.txUndo.accountLogs.push_back(desAccountLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, {txUid, toUid}))
        return false;

    return true;
}

bool CScoinTransferTx::UndoExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CScoinTransferTx::UndoExecuteTx, read account info error, userId=%s",
                userId.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CScoinTransferTx::UndoExecuteTx, undo operate account error, keyId=%s",
                            account.keyID.ToString()), UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CScoinTransferTx::UndoExecuteTx, save account error"),
                            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
        }
    }

    return true;
}

string CScoinTransferTx::ToString(CAccountCache &accountCache) {
    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, toUid=%s, scoins=%ld, llFees=%ld, feesCoinType=%d,nValidHeight=%d\n",
        GetTxType(nTxType), GetHash().ToString().c_str(), nVersion, txUid.ToString(), toUid.ToString(), scoins, llFees,
        feesCoinType, nValidHeight);

    return str;
}

Object CScoinTransferTx::ToJson(const CAccountCache &accountCache) const {
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
    result.push_back(Pair("scoins",             scoins));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("fees_coin_type",     feesCoinType));
    result.push_back(Pair("memo",               HexStr(memo)));
    result.push_back(Pair("valid_height",       nValidHeight));

    return result;
}

bool CScoinTransferTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
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