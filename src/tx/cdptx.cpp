// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"
#include "main.h"
#include "persistence/cdpdb.h"

#include <math.h>

string CCDPStakeTx::ToString(CAccountDBCache &view) {
    //TODO
    return "";
}

Object CCDPStakeTx::ToJson(const CAccountDBCache &AccountView) const {
    //TODO
    return Object();
}

bool CCDPStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCDPStakeTx::SellInterestForFcoins(const int nHeight, const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state) {
    uint64_t scoinsInterestToRepay = cw.cdpCache.ComputeInterest(nHeight, cdp);
    if (scoinsInterest < scoinsInterestToRepay) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, scoinsInterest: %d < scoinsInterestToRepay: %d",
                        scoinsInterest, scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
    }

    CDEXSysBuyOrder sysBuyOrder(CoinType::WUSD, CoinType::WGRT, scoinsInterest);
    if (!cw.dexCache.CreateSysBuyOrder(GetHash(), sysBuyOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }
}

// CDP owner can redeem his or her CDP that are in liquidation list
bool CCDPStakeTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.CheckGlobalCollateralFloorReached(cw.ppCache.GetBcoinMedianPrice())) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }
    if (cw.cdpCache.CheckGlobalCollateralCeilingReached(bcoinsToStake)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralCeilingReached!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    if (bcoinsToStake < kBcoinsToStakeAmountMin) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, bcoins to stake %d is too small,",
                    bcoinsToStake), REJECT_INVALID, "bcoins-too-small-to-stake");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (account.scoins < scoinsInterest) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, account scoins %d insufficent for interest %d",
                        account.fcoins, scoinsInterest), READ_ACCOUNT_FAIL, "account-scoins-insufficient");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}
bool CCDPStakeTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txid = GetHash();
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(account); //save account state before modification
    //1. pay miner fees (WICC)
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. mint scoins
    int mintedScoins = 0;
    CUserCDP cdp(txUid.get<CRegID>(), cdpTxId);
    if (!cw.cdpCache.GetCdp(cdp)) { // first-time CDP creation
        if (collateralRatio < kStartingCdpCollateralRatio) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal", collateralRatio), 
                            REJECT_INVALID, "bad-tx-collateral-ratio-toosmall");
        }

        cdp.cdpTxId = GetHash();
        mintedScoins = bcoinsToStake * kPercentBoost / collateralRatio;

        //settle cdp state & persist for the 1st-time
        cw.cdpCache.StakeBcoinsToCdp(nHeight, bcoinsToStake, (uint64_t) mintedScoins, cdp);

    } else { // further staking on one's existing CDP
        // check if cdp owner is the same user who's staking!
        if (txUid.get<CRegID>() != cdp.ownerRegId) {
                return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, txUid(%s) not CDP owner",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "account-not-CDP-owner");
        }

        if (nHeight < cdp.blockHeight) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, nHeight: %d < cdp.blockHeight: %d",
                    nHeight, cdp.blockHeight), UPDATE_ACCOUNT_FAIL, "nHeight-error");
        }

        int currentCollateralRatio = cdp.totalStakedBcoins * cw.ppCache.GetBcoinMedianPrice() * kPercentBoost / cdp.totalOwedScoins;
        if ( collateralRatio < currentCollateralRatio  && collateralRatio < kStartingCdpCollateralRatio) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, currentCollateralRatio: %d vs new CollateralRatio: %d not allowed",
                            currentCollateralRatio, collateralRatio), REJECT_INVALID, "collateral-ratio-error");
        }

        if (!SellInterestForFcoins(nHeight, cdp, cw, state))
            return false;

        account.scoins -= scoinsInterest;

        mintedScoins = (bcoinsToStake + cdp.totalStakedBcoins) * kPercentBoost / collateralRatio  - cdp.totalOwedScoins;
        if (mintedScoins < 0) { // can be zero since we allow increasing collateral ratio when staking bcoins
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, over-collateralized from regId=%s",
                            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "cdp-overcollateralized");
        }

        //settle cdp state & persist
        cw.cdpCache.StakeBcoinsToCdp(nHeight, bcoinsToStake, (uint64_t) mintedScoins, cdp, cw.txUndo.dbOpLogMap);
    }

    if (!account.StakeBcoinsToCdp(CoinType::WICC, bcoinsToStake, (uint64_t) mintedScoins)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, stake bcoins from regId=%s failed",
                        txUid.ToString()), STAKE_CDP_FAIL, "cdp-stake-bcoins-failed");
    }
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(acctLog);

    bool ret = SaveTxAddresses(nHeight, nIndex, cw, state, {txUid});
    return ret;
}

bool CCDPStakeTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }
        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    if (!cw.cdpCache.UndoCdp(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, undo active buy order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (!cw.dexCache.UndoSysBuyOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, undo system buy order failed"),
                        UNDO_SYS_ORDER_FAILED, "undo-data-failed");
    }

    return true;
}

/************************************<< CCDPRedeemTx >>***********************************************/
string CCDPRedeemTx::ToString(CAccountDBCache &view) {
     //TODO
     return "";
 }
 Object CCDPRedeemTx::ToJson(const CAccountDBCache &AccountView) const {
     //TODO
     return Object();
 }
 bool CCDPRedeemTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
     //TODO
     return true;
 }
 bool CCDPRedeemTx::SellInterestForFcoins(const int nHeight, const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state) {
    uint64_t scoinsInterestToRepay = cw.cdpCache.ComputeInterest(nHeight, cdp);
    if (scoinsInterest < scoinsInterestToRepay) {
         return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, scoinsInterest: %d < scoinsInterestToRepay: %d",
                    scoinsInterest, scoinsInterestToRepay), UPDATE_ACCOUNT_FAIL, "scoins-interest-insufficient-error");
    }

    CDEXSysBuyOrder sysBuyOrder(CoinType::WUSD, CoinType::WGRT, scoinsInterest);
    if (!cw.dexCache.CreateSysBuyOrder(GetHash(), sysBuyOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }
}

bool CCDPRedeemTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.CheckGlobalCollateralFloorReached(cw.ppCache.GetBcoinMedianPrice())) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (cdpTxId.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, cdpTxId is empty"),
                        REJECT_INVALID, "EMPTY_CDP_TXID");
    }

    if (scoinsToRedeem == 0) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, scoinsToRedeem is 0"),
                        REJECT_DUST, "REJECT_DUST");
    }

    if (account.scoins < scoinsInterest) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, account scoins %d insufficent for interest %d",
                        account.fcoins, scoinsInterest), REJECT_INVALID, "account-scoins-insufficient");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
 }

 bool CCDPRedeemTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txid = GetHash();
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(account); //save account state before modification
    //1. pay miner fees (WICC)
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay interest fees in wusd
    CUserCDP cdp(txUid.get<CRegID>(), cdpTxId);
    if (cw.cdpCache.GetCdp(cdp)) {
        // check if cdp owner is the same user who's redeeming!
        if (txUid.get<CRegID>() != cdp.ownerRegId) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, txUid(%s) not CDP owner",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "account-not-CDP-owner");
        }
        if (nHeight < cdp.blockHeight) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, nHeight: %d < cdp.blockHeight: %d",
                    nHeight, cdp.blockHeight), UPDATE_ACCOUNT_FAIL, "nHeight-error");
        }
        if (!SellInterestForFcoins(nHeight, cdp, cw, state))
            return false;

        account.scoins -= scoinsInterest;
    }

    //3. redeem in scoins and update cdp
    cdp.totalOwedScoins -= scoinsToRedeem;
    double targetStakedBcoins = ((double) collateralRatio / kPercentBoost) * cdp.totalOwedScoins / cw.ppCache.GetBcoinMedianPrice();
    if (cdp.totalStakedBcoins <= targetStakedBcoins) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, total staked bcoins %d <= target %d",
                        cdp.totalStakedBcoins, targetStakedBcoins), UPDATE_ACCOUNT_FAIL, "targetStakedBcoins-error");
    }
    uint64_t releasedBcoins = cdp.totalStakedBcoins - targetStakedBcoins;
    account.scoins -= scoinsToRedeem;
    account.bcoins += releasedBcoins;

    cdp.totalStakedBcoins = (uint64_t) targetStakedBcoins;
    if (!cw.cdpCache.SaveCdp(cdp, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update CDP %s failed",
                        cdp.ownerRegId.ToString()), UPDATE_CDP_FAIL, "bad-save-cdp");
    }

    cw.txUndo.accountLogs.push_back(acctLog);

    return true;
 }
 bool CCDPRedeemTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }
        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    if (!cw.cdpCache.UndoCdp(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo active buy order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (!cw.dexCache.UndoSysBuyOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo system buy order failed"),
                        UNDO_SYS_ORDER_FAILED, "undo-data-failed");
    }

     return true;
 }

/************************************<< CdpLiquidateTx >>***********************************************/
string CCDPLiquidateTx::ToString(CAccountDBCache &view) {
    //TODO
    return "";
}
Object CCDPLiquidateTx::ToJson(const CAccountDBCache &AccountView) const {
    //TODO
    return Object();
}
bool CCDPLiquidateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}
bool CCDPLiquidateTx::SellPenaltyForFcoins(int scoinPenaltyToSysFund, const int nHeight, const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state) {
   
    if (scoinsPenalty < scoinPenaltyToSysFund) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoinsPenalty %d < SellPenaltyForFcoins %d",
                       scoinsPenalty, scoinPenaltyToSysFund), REJECT_INVALID, "bad-read-accountdb");
    }
    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read fcoinGenesisUid %s account info error"),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog genesisAcctLog(fcoinGenesisAccount);
    cw.txUndo.accountLogs.push_back(genesisAcctLog);

    uint64_t halfScoinsPenalty = scoinsPenalty / 2;
    // uint64_t halfFcoinsPenalty = halfScoinsPenalty / cw.ppCache.GetFcoinMedianPrice();

    fcoinGenesisAccount.scoins += halfScoinsPenalty;    // save into risk reserve fund
    CDEXSysBuyOrder sysBuyOrder(CoinType::WUSD, CoinType::WGRT, halfScoinsPenalty);
    if (!cw.dexCache.CreateSysBuyOrder(GetHash(), sysBuyOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::ExecuteTx, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

bool CCDPLiquidateTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.CheckGlobalCollateralFloorReached(cw.ppCache.GetBcoinMedianPrice())) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    if (cdpTxId.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, cdpTxId is empty"),
                        REJECT_INVALID, "EMPTY_CDPTXID");
    }

    if (scoinsPenalty == 0) {
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, penalty amount is zero"),
                        REJECT_INVALID, "bad-tx-zero-penalty-error");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    if (account.scoins < scoinsToLiquidate + scoinsPenalty) {
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, account scoins %d < scoinsToLiquidate: %d",
                        account.scoins, scoinsToLiquidate), CDP_LIQUIDATE_FAIL, "account-scoins-insufficient");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

/**
  * TotalStakedBcoinsInScoins : TotalOwedScoins = M : N
  *
  * Liquidator paid         1.13lN          (0 < l ≤ 100%)
  *   Liquidate Amount:     l * N       = lN
  *   Penalty Fees:         l * N * 13% = 0.13lN
  * Liquidator received:    Bcoins only
  *   Bcoins:               1.13lN ~ 1.16lN (WICC)
  *   Net Profit:           0 ~ 0.03lN (WICC)
  *
  * CDP Owner returned
  *   Bcoins:               lM - 1.16lN = l(M - 1.16N)
  *
  *  when M is 1.16 N and below, there'll be no return to the CDP owner
  *  when M is 1.13 N and below, there'll be no profit for the liquidator, hence requiring force settlement
  */
bool CCDPLiquidateTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txid = GetHash();
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(account); //save account state before modification

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, deduct fees from regId=%s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay penalty fees: 0.13lN --> 50% burn, 50% to Risk Reserve
    CUserCDP cdp(txUid.get<CRegID>(), cdpTxId);
    if (!cw.cdpCache.GetCdp(cdp)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp (%s) not exist!",
                        txUid.ToString()), REJECT_INVALID, "cdp-not-exist");
    }
    CAccount cdpOwnerAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.ownerRegId), cdpOwnerAccount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP Owner txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(cdpOwnerAccount); //save account state before modification

    int collateralRatio = (double) cdp.totalStakedBcoins * cw.ppCache.GetBcoinMedianPrice() * kPercentBoost 
                            / cdp.totalOwedScoins;

    double liquidateRate; //unboosted
    double totalScoinsToReturnLiquidator, totalScoinsToLiquidate, totalScoinsToReturnSysFund, totalBcoinsToCDPOwner;

    if (collateralRatio > kStartingCdpLiquidateRatio) {        // 1.5++
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp collateralRatio(%d) > 150%!",
                        collateralRatio), REJECT_INVALID, "cdp-not-liquidate-ready");

    } else if (collateralRatio > kNonReturnCdpLiquidateRatio) { // 1.13 ~ 1.5
        totalScoinsToLiquidate = ((double) cdp.totalOwedScoins * kNonReturnCdpLiquidateRatio / kPercentBoost ) 
                                * kCdpLiquidateDiscountRate / kPercentBoost; //1.096N
        totalScoinsToReturnLiquidator = (double) cdp.totalOwedScoins * kNonReturnCdpLiquidateRatio / kPercentBoost; //1.13N
        totalScoinsToReturnSysFund = ((double) cdp.totalOwedScoins * kNonReturnCdpLiquidateRatio / kPercentBoost ) 
                                * kCdpLiquidateDiscountRate / kPercentBoost - cdp.totalOwedScoins;
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice();

    } else if (collateralRatio > kForcedCdpLiquidateRatio) {    // 1.03 ~ 1.13
        totalScoinsToReturnLiquidator = (double) cdp.totalStakedBcoins / cw.ppCache.GetBcoinMedianPrice(); //M
        totalScoinsToLiquidate = totalScoinsToReturnLiquidator * kCdpLiquidateDiscountRate / kPercentBoost; //M * 97%
        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.totalOwedScoins; // M - N
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice();
       
    } else {                                                    // 0 ~ 1.03
        //TODO although not likely to happen. but if it does. figure below properly
        totalScoinsToReturnLiquidator = (double) cdp.totalStakedBcoins / cw.ppCache.GetBcoinMedianPrice(); //M
        totalScoinsToLiquidate = totalScoinsToReturnLiquidator * kCdpLiquidateDiscountRate / kPercentBoost; //M * 97%
        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.totalOwedScoins; // M - N
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice();
    }

    if (scoinsToLiquidate >= totalScoinsToLiquidate) {
        account.scoins -= totalScoinsToLiquidate;
        account.scoins -= scoinsPenalty;
        account.bcoins += totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice();

        cdpOwnerAccount.bcoins += totalBcoinsToCDPOwner;

        if (!SellPenaltyForFcoins(totalScoinsToReturnSysFund, nHeight, cdp, cw, state))
            return false;

        //close CDP
        cw.cdpCache.EraseCdp(cdp);

    } else { //partial liquidation
        liquidateRate = (double) scoinsToLiquidate / totalScoinsToLiquidate;

        account.scoins -= scoinsToLiquidate;
        account.scoins -= scoinsPenalty;
        account.bcoins += totalScoinsToReturnLiquidator * liquidateRate / cw.ppCache.GetBcoinMedianPrice();
        
        int bcoinsToCDPOwner = totalBcoinsToCDPOwner * liquidateRate;
        cdpOwnerAccount.bcoins += bcoinsToCDPOwner;

        cdp.totalOwedScoins -= scoinsToLiquidate;
        cdp.totalStakedBcoins -= bcoinsToCDPOwner;

        int scoinsToReturnSysFund = totalScoinsToReturnSysFund * liquidateRate;
        if (!SellPenaltyForFcoins(scoinsToReturnSysFund, nHeight, cdp, cw, state))
            return false;

        cw.cdpCache.SaveCdp(cdp, cw.txUndo.dbOpLogMap);
    }

    return true;
}
bool CCDPLiquidateTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = cw.txUndo.accountLogs.rbegin();
    for (; rIterAccountLog != cw.txUndo.accountLogs.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }
        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    if (!cw.cdpCache.UndoCdp(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo active buy order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (!cw.dexCache.UndoSysBuyOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo system buy order failed"),
                        UNDO_SYS_ORDER_FAILED, "undo-data-failed");
    }

    return true;
}