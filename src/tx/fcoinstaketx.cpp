// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "fcoinstaketx.h"

#include "configuration.h"
#include "main.h"

bool CFcoinStakeTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (fcoinsToStake == 0 || !CheckFundCoinRange(abs(fcoinsToStake))) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, fcoinsToStake out of range"),
                        REJECT_INVALID, "bad-tx-fcoins-outofrange");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CFcoinStakeTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    CAccountLog acctLog(account);
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, insufficient bcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-bcoins");
    }

    if (!account.OperateFcoinStaking(fcoinsToStake)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, insufficient fcoins in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "insufficient-fcoins");
    }

    if (!cw.accountCache.SaveAccount(account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    cw.txUndo.accountLogs.push_back(acctLog);
    cw.txUndo.txHash = GetHash();

    if (!SaveTxAddresses(nHeight, nIndex, cw, state, {txUid})) return false;

    return true;
}

bool CFcoinStakeTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CFcoinStakeTx::UndoExecuteTx, read account info error, userId=%s",
                userId.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CFcoinStakeTx::UndoExecuteTx, undo operate account error, keyId=%s",
                            account.keyId.ToString()), UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CFcoinStakeTx::UndoExecuteTx, save account error"),
                            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
        }
    }

    return true;
}

string CFcoinStakeTx::ToString(CAccountDBCache &accountCache) {
    string str = strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, fcoinsToStake=%ld, llFees=%ld, nValidHeight=%d\n",
                           GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fcoinsToStake, llFees,
                           nValidHeight);

    return str;
}

Object CFcoinStakeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("regid",              txUid.ToString()));
    result.push_back(Pair("addr",               keyId.ToAddress()));
    result.push_back(Pair("coins_to_stake",     fcoinsToStake));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("valid_height",       nValidHeight));

    return result;
  return Object();
}

bool CFcoinStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID keyId;
    if (!cw.accountCache.GetKeyId(txUid, keyId)) {
        return false;
    }
    keyIds.insert(keyId);
    return true;
}
