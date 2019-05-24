// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "fcoinstaketx.h"

#include "configuration.h"
#include "main.h"

bool CFcoinStakeTx::CheckTx(CValidationState &state, CAccountCache &view, CContractCache &scriptDB) {

    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (fcoinsToStake == 0 || !CheckFundCoinRange(abs(fcoinsToStake))) {
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::CheckTx, fcoinsToStake out of range"),
                        REJECT_INVALID, "bad-tx-fcoins-toolarge");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CFcoinStakeTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.pAccountCache->GetAccount(txUid, account))
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

    if (!cw.pAccountCache->SaveAccountInfo(account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::ExecuteTx, write source addr %s account info error",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    cw.pTxUndo->vAccountLog.push_back(acctLog);
    cw.pTxUndo->txHash = GetHash();
    if (SysCfg().GetAddressToTxFlag()) {
        CContractDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!cw.pAccountCache->GetKeyId(txUid, sendKeyId))
            return ERRORMSG("CFcoinStakeTx::ExecuteTx, get keyid by userId error!");

        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, cw.pTxUndo->txHash.GetHex(),
                                        operAddressToTxLog))
            return false;

        cw.pTxUndo->vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CFcoinStakeTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    for (auto &itemLog : cw.pTxUndo->vAccountLog) {
        if (itemLog.keyID == account.keyID) {
            if (!account.UndoOperateAccount(itemLog))
                return state.DoS(100, ERRORMSG("CFcoinStakeTx::UndoExecuteTx, undo operate account error, keyId=%s",
                                account.keyID.ToString()), UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }
    }

    vector<CContractDBOperLog>::reverse_iterator rIterScriptDBLog = cw.pTxUndo->vScriptOperLog.rbegin();
    for (; rIterScriptDBLog != cw.pTxUndo->vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if (!cw.pContractCache->UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("CFcoinStakeTx::UndoExecuteTx, undo scriptdb data error"),
                            UPDATE_ACCOUNT_FAIL, "undo-scriptdb-failed");
    }
    userId = account.keyID;
    if (!cw.pAccountCache->SetAccount(userId, account))
        return state.DoS(100, ERRORMSG("CFcoinStakeTx::UndoExecuteTx, save account error"),
                        UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    return true;
}


Object CFcoinStakeTx::ToJson(const CAccountCache &AccountView) const {
  //TODO
  return Object();
}

bool CFcoinStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}
