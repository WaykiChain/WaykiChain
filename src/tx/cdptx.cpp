// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"
#include "main.h"

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

    // bcoinsToStake can be zero since we allow downgrading collateral ratio to mint new scoins
    // but it must be grater than the fund committe defined minimum ratio value
    if (collateralRatio < pCdMan->collateralRatioMin ) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal (%d)",
                        collateralRatio, pCdMan->collateralRatioMin), REJECT_INVALID, "bad-tx-collateral-ratio-toosmall");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CRegID sendRegId;
    account.GetRegId(sendRegId);
    if (!pCdMan->pDelegateCache->ExistDelegate(sendRegId.ToString())) { // must be a miner
        return state.DoS(100, ERRORMSG("CdpStakeTx::CheckTx, txUid %s account is not a delegate error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "account-not-delegate");
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

    CUserCdp cdp;
    if (!cw.cdpCache.GetCdp(txUid.ToString(), cdp)) {
        // first-time staking?
    }
    int mintedScoins = (bcoinsToStake + cdp.totalStakedBcoins) * COIN / collateralRatio - cdp.totalOwedScoins;
    if (mintedScoins <= 0) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::ExecuteTx, over-collateralized from regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "cdp-overcollateralized");
    }

    if (!account.StakeBcoinsToCdp(CoinType::WICC, bcoinsToStake, (uint64_t) mintedScoins)) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::ExecuteTx, stake boins from regId=%s failed",
                        txUid.ToString()), STAKE_CDP_FAIL, "cdp-stake-bcoinsfailed");
    }
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CdpStakeTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(acctLog);

    CDbOpLog cdpDbOpLog;
    cw.cdpCache.StakeBcoins(txUid, bcoinsToStake, collateralRatio, (uint64_t) mintedScoins, nHeight, cdpDbOpLog); //update cache & persist into ldb
    cw.txUndo.mapDbOpLogs[DbOpLogType::COMMON_OP].push_back(cdpDbOpLog);

    if (!SaveTxAddresses(nHeight, nIndex, cw, {txUid})) return false;

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

    auto cdpLogs = cw.txUndo.mapDbOpLogs[DbOpLogType::COMMON_OP];
    for (auto cdpLog : cdpLogs) {
        if (!cw.cdpCache.UndoCdp(cdpLog)) {
            return state.DoS(100, ERRORMSG("CdpStakeTx::UndoExecuteTx, restore cdp error"),
                             UPDATE_ACCOUNT_FAIL, "bad-restore-cdp");
        }
    }

    return true;
}

/************************************<< CdpLiquidateTx >>***********************************************/
string CdpLiquidateTx::ToString(CAccountCache &view) {
    //TODO
    return "";
}
Object CdpLiquidateTx::ToJson(const CAccountCache &AccountView) const {
    //TODO
    return Object();
}
bool CdpLiquidateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}
bool CdpLiquidateTx::CheckTx(CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    //TODO: need to check if scoinsToRedeem is no less than outstanding value
    if (scoinsToRedeem == 0) {
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, scoin amount is zero"),
                        REJECT_INVALID, "bad-tx-scoins-is-zero-error");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}
bool CdpLiquidateTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    //TODO
    return true;
}
bool CdpLiquidateTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    //TODO
    return true;
}