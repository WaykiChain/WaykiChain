// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"
#include "main.h"

#include <math.h>

string CCdpStakeTx::ToString(CAccountCache &view) {
    //TODO
    return "";
}

Object CCdpStakeTx::ToJson(const CAccountCache &AccountView) const {
    //TODO
    return Object();
}

bool CCdpStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCdpStakeTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    // bcoinsToStake can be zero since we allow downgrading collateral ratio to mint new scoins
    // but it must be grater than the fund committe defined minimum ratio value
    if (collateralRatio < pCdMan->collateralRatioMin ) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal (%d)",
                        collateralRatio, pCdMan->collateralRatioMin), REJECT_INVALID, "bad-tx-collateral-ratio-toosmall");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CRegID sendRegId;
    account.GetRegId(sendRegId);
    if (!pCdMan->pDelegateCache->ExistDelegate(sendRegId.ToString())) { // must be a miner
        return state.DoS(100, ERRORMSG("CCdpStakeTx::CheckTx, txUid %s account is not a delegate error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "account-not-delegate");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CCdpStakeTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txHash = GetHash();

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(account); //save account state before modification

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. deduct interest fees in scoins into the micc pool
    CAccount fcoinGensisAccount;
    CRegID fcoinGenesisRegId(kFcoinGenesisTxHeight, kFcoinGenesisTxIndex);
    CUserID fcoinGenesisUid(fcoinGenesisRegId);
    if (!cw.accountCache.GetAccount(fcoinGenesisUid, fcoinGensisAccount)) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, read fcoinGenesisUid %s account info error",
                        fcoinGenesisUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");
    }
    CAccountLog genesisAcctLog(account);

    CUserCdp cdp;
    if (!cw.cdpCache.GetCdp(txUid.ToString(), cdp)) {
        // first-time staking, no interest will be charged
    } else {
        if (nHeight < cdp.lastBlockHeight) {
            return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, nHeight: %d < cdp.lastBlockHeight: %d",
                        nHeight, cdp.lastBlockHeight), UPDATE_ACCOUNT_FAIL, "nHeight-smaller-error");
        }

        uint64_t totalScoinsInterestToRepay = cw.cdpCache.ComputeInterest(nHeight, cdp);
        uint64_t fcoinMedianPrice = cw.pricePointCache.GetFcoinMedianPrice();
        uint64_t totalFcoinsInterestToRepay = totalScoinsInterestToRepay / fcoinMedianPrice;
        if (account.fcoins >= totalFcoinsInterestToRepay) {
            account.fcoins -= totalFcoinsInterestToRepay; // burn away fcoins, total thus reduced
        } else {
            uint64_t restFcoins = totalFcoinsInterestToRepay - account.fcoins;
            account.fcoins = 0; // burn away fcoins, total thus reduced
            uint64_t restScoins = restFcoins * fcoinMedianPrice;
            if (account.scoins >= restScoins) {
                account.scoins -= restScoins;
                fcoinGensisAccount.scoins += restScoins; //add scoins into the common pool
            } else {
                return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, scoins not enough: %d, needs: %d",
                        account.scoins, restScoins), UPDATE_ACCOUNT_FAIL, "scoins-smaller-error");
            }
        }
    }
    if (!cw.accountCache.SaveAccount(fcoinGensisAccount)) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, update fcoinGensisAccount %s failed",
                        fcoinGenesisUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(fcoinGensisAccount);

    //3. mint scoins
    int mintedScoins = (bcoinsToStake + cdp.totalStakedBcoins) / collateralRatio / 100 - cdp.totalOwedScoins;
    if (mintedScoins < 0) { // can be zero since we allow increasing collateral ratio when staking bcoins
        return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, over-collateralized from regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "cdp-overcollateralized");
    }
    if (!account.StakeBcoinsToCdp(CoinType::WICC, bcoinsToStake, (uint64_t) mintedScoins)) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, stake bcoins from regId=%s failed",
                        txUid.ToString()), STAKE_CDP_FAIL, "cdp-stake-bcoins-failed");
    }
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CCdpStakeTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(acctLog);

    CDbOpLog cdpDbOpLog;
    cw.cdpCache.StakeBcoinsToCdp(txUid, bcoinsToStake, collateralRatio, (uint64_t) mintedScoins, nHeight, cdpDbOpLog); //update cache & persist into ldb
    cw.txUndo.mapDbOpLogs[DbOpLogType::DB_OP_CDP].push_back(cdpDbOpLog);

    bool ret = SaveTxAddresses(nHeight, nIndex, cw, {txUid});
    return ret;
}

bool CCdpStakeTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCdpStakeTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CCdpStakeTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCdpStakeTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    auto cdpLogs = cw.txUndo.mapDbOpLogs[DbOpLogType::DB_OP_CDP];
    for (auto cdpLog : cdpLogs) {
        if (!cw.cdpCache.UndoCdp(cdpLog)) {
            return state.DoS(100, ERRORMSG("CCdpStakeTx::UndoExecuteTx, restore cdp error"),
                             UPDATE_ACCOUNT_FAIL, "bad-restore-cdp");
        }
    }

    return true;
}

/************************************<< CdpLiquidateTx >>***********************************************/
string CCdpLiquidateTx::ToString(CAccountCache &view) {
    //TODO
    return "";
}
Object CCdpLiquidateTx::ToJson(const CAccountCache &AccountView) const {
    //TODO
    return Object();
}
bool CCdpLiquidateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}
bool CCdpLiquidateTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
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
bool CCdpLiquidateTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    //TODO
    //0. pay miner fees (WICC)
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCdpLiquidateTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }
    //2. pay fines
    //3. collect due-amount bcoins
    //4. return remaining bcoins to the cdp owner
    return true;
}
bool CdpLiquidateTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    //TODO
    return true;
}