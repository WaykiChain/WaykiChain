// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"

string CdpStakeTx::ToString(CAccountCache &view) {
    //TODO
    return "";
}

Object CdpStakeTx::ToJson(const CAccountCache &AccountView) const {
    //TODO
    return Object();
}

bool CdpStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CdpStakeTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (bcoinsToStake == 0) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::CheckTx, bcoin amount is zero"),
            REJECT_INVALID, "bad-tx-bcoins-is-zero-error");
    }

    if (collateralRatio < pCdMan->collateralRatio ) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::CheckTx, collateral ratio is smaller than minimal %d",
                        pCdMan->collateralRatio), REJECT_INVALID, "bad-tx-collateral-ratio-toosmall");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CdpStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    CRegID sendRegId;
    account.GetRegId(sendRegId);
    if (!pCdMan->pDelegateCache->ExistDelegate(sendRegId.ToString())) { // must be a miner
        return state.DoS(100, ERRORMSG("CdpStakeTx::CheckTx, txUid %s account is not a delegate error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-isnot-delegate");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CdpStakeTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txHash = GetHash();

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(account); //save account state before modification
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::ExecuteTx, deduct fees from regId=%s failed ,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }
    if (!account.StakeBcoinsToCdp(CoinType::WICC, MINUS_VALUE, bcoinsToStake)) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::ExecuteTx, stake foins from regId=%s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(acctLog);

    CDbOpLog cdpDbOpLog;
    cw.cdpCache->SetStakeBcoins(txUid, bcoinsToStake, collateralRatio, nHeight, cdpDbOpLog); //update cache & persist into ldb
    cw.txUndo.cdpOpLogs.push_back(cdpDbOpLog);

    IMPLEMENT_PERSIST_TX_KEYID(txUid, CUserID());
    return true;
}

bool CdpStakeTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CdpStakeTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CdpStakeTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CdpStakeTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    auto rIterCdpDBLog = cw.txUndo.cdpOpLogs.rbegin();
    for (; rIterCdpDBLog != cw.txUndo.cdpOpLogs.rend(); ++rIterCdpDBLog) {
        if (!cw.cdpCache.SetData(rIterCdpDBLog->key, rIterCdpDBLog->value))
            return state.DoS(100, ERRORMSG("CdpStakeTx::UndoExecuteTx, set cdp data error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-cdp");

        ++rIterCdpDBLog;
    }
    return true;
}