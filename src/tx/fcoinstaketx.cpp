// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "fcoinstaketx.h"

#include "configuration.h"
#include "main.h"

bool CFcoinStakeTx::CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {

    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (fcoinsToStake == 0 || !CheckFundCoinRange(abs(fcoinsToStake))) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, fcoinsToStake out of range"),
                        REJECT_INVALID, "bad-tx-fcoins-toolarge");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CFcoinStakeTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                    int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CAccount account;
    if (!view.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), FCOIN_STAKE_FAIL, "bad-read-accountdb");

    CAccountLog acctLog(account);
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, not sufficient funds in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "not-sufficiect-funds");
    }

    if (!account.OperateFcoinStaking(fcoinsToStake)) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, not sufficient funds in txUid %s account",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "not-sufficiect-funds");
    }

    if (!view.SaveAccountInfo(account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    txundo.vAccountLog.push_back(acctLog);
    txundo.txHash = GetHash();
    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(txUid, sendKeyId))
            return ERRORMSG("CFcoinStakeTx::ExecuteTx, get keyid by userId error!");

        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CFcoinStakeTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                    CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                    CScriptDBViewCache &scriptDB) {
    //TODO
    return true;

}


Object CFcoinStakeTx::ToJson(const CAccountViewCache &AccountView) const {
  //TODO
  return Object();
}

bool CFcoinStakeTx::GetInvolvedKeyIds(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    //TODO
    return true;
}
