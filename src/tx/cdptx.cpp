// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"
#include "main.h"

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

bool CCDPStakeTx::PayInterest(int nHeight, const CUserCdp &cdp, CCacheWrapper &cw, CValidationState &state) {
    if (nHeight < cdp.lastBlockHeight) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, nHeight: %d < cdp.lastBlockHeight: %d",
                    nHeight, cdp.lastBlockHeight), UPDATE_ACCOUNT_FAIL, "nHeight-error");
    }

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read fcoinGenesisUid account info error"),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog genesisAcctLog(fcoinGenesisAccount);

    uint64_t totalScoinsInterestToRepay = cw.cdpCache.ComputeInterest(nHeight, cdp);
    uint64_t fcoinMedianPrice = cw.ppCache.GetFcoinMedianPrice();
    uint64_t totalFcoinsInterestToRepay = totalScoinsInterestToRepay / fcoinMedianPrice;

    double restFcoins = totalFcoinsInterestToRepay - fcoinsInterest;
    double restScoins = (restFcoins/fcoinMedianPrice) * (100 + kScoinInterestIncreaseRate)/100;
    if (scoinsInterest < restScoins) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoinsInterest: %d < restScoins: %d",
                        scoinsInterest, restScoins), INTEREST_INSUFFICIENT, "interest-insufficient-error");
    }

    if (fcoinsInterest > 0) {
        account.fcoins -= fcoinsInterest; // burn away fcoins
        fcoinGensisAccount.fcoins += fcoinsInterest; // but keep total in balance
    }
    if (scoinsInterest) {
        account.scoins -= scoinsInterest;
        cw.dexCache.CreateBuyOrder(scoinsInterest, CoinType::WGRT); // DEX: wusd_micc
    }

    if (!cw.accountCache.SaveAccount(fcoinGensisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, update fcoinGensisAccount %s failed",
                        fcoinGenesisUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(fcoinGensisAccount);
}

// CDP owner can redeem his or her CDP that are in liquidation list
bool CCDPStakeTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.GetGlobalCDPLock()) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, CDP Global Lock is on!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    // bcoinsToStake can be zero since we allow downgrading collateral ratio to mint new scoins
    // but it must be grater than the fund committee defined minimum ratio value
    if (collateralRatio < pCdMan->initialCollateralRatioMin ) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal (%d)",
                        collateralRatio, pCdMan->initialCollateralRatioMin), REJECT_INVALID, "bad-tx-collateral-ratio-toosmall");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (account.fcoins < fcoinsInterest) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, account fcoins %d insufficent for interest %d",
                        account.fcoins, fcoinsInterest), READ_ACCOUNT_FAIL, "account-fcoins-insufficient");
    }
    if (account.scoins < scoinsInterest) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, account scoins %d insufficent for interest %d",
                        account.fcoins, scoinsInterest), READ_ACCOUNT_FAIL, "account-scoins-insufficient");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
}

bool CCDPStakeTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txHash = GetHash();
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

    if (cdpTxCord.IsEmpty()) {
        cdpTxCord = CTxCord(nHeight, nIndex);
    }
    CUserCdp cdp(txUid.get<CRegID>(), cdpTxCord);

    //2. pay interest fees in wusd or micc into the micc pool
    if (cw.cdpCache.GetCdp(cdp)) {
        // check if cdp owner is the same user who's staking!
        if (txUid.Get<CRegID>() != cdp.ownerRegId) {
                return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, txUid(%s) not CDP owner",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "account-not-CDP-owner");
        }

        if (!PayInterest(nHeight, cdp, cw, state))
            return false;
    }

    //3. mint scoins
    int mintedScoins = (bcoinsToStake + cdp.totalStakedBcoins) / collateralRatio / 100 - cdp.totalOwedScoins;
    if (mintedScoins < 0) { // can be zero since we allow increasing collateral ratio when staking bcoins
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, over-collateralized from regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "cdp-overcollateralized");
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

    CDbOpLog cdpDbOpLog;
    cw.cdpCache.StakeBcoinsToCdp(txUid.get<CRegID>(), bcoinsToStake, (uint64_t)mintedScoins, nHeight, cdp,
                                 cdpDbOpLog);  // update cache & persist into ldb

    cw.txUndo.dbOpLogMap.AddDbOpLog(dbk::CDP, cdpDbOpLog);

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

    auto cdpLogs = cw.txUndo.mapDbOpLogs[DbOpLogType::DB_OP_CDP];
    for (auto cdpLog : cdpLogs) {
        if (!cw.cdpCache.UndoCdp(cdpLog)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, restore cdp error"),
                             UPDATE_ACCOUNT_FAIL, "bad-restore-cdp");
        }
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
 bool CCDPRedeemTx::PayInterest(int nHeight, const CUserCdp &cdp, CCacheWrapper &cw, CValidationState &state) {
    if (nHeight < cdp.lastBlockHeight) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, nHeight: %d < cdp.lastBlockHeight: %d",
                    nHeight, cdp.lastBlockHeight), UPDATE_ACCOUNT_FAIL, "nHeight-error");
    }

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, read fcoinGenesisUid %s account info error"),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog genesisAcctLog(fcoinGenesisAccount);

    uint64_t totalScoinsInterestToRepay = cw.cdpCache.ComputeInterest(nHeight, cdp);
    uint64_t fcoinMedianPrice = cw.ppCache.GetFcoinMedianPrice();
    uint64_t totalFcoinsInterestToRepay = totalScoinsInterestToRepay / fcoinMedianPrice;

    double restFcoins = totalFcoinsInterestToRepay - fcoinsInterest;
    double restScoins = (restFcoins/fcoinMedianPrice) * (100 + kScoinInterestIncreaseRate)/100;
    if (scoinsToRedeem < restScoins) { //remaining interest must be paid from redeem scoins
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, scoinsInterest: %d < restScoins: %d",
                        scoinsInterest, restScoins), INTEREST_INSUFFICIENT, "interest-insufficient-error");
    }

    if (fcoinsInterest > 0) {
        account.fcoins -= fcoinsInterest; // burn away fcoins
        fcoinGensisAccount.fcoins += fcoinsInterest; // but keep total in balance
    }
    if (restScoins > 0) {
        account.scoins -= restScoins;
        cw.dexCache.CreateBuyOrder(restScoins, CoinType::WGRT); // DEX: wusd_micc

        scoinsToRedeem -= restScoins; // after interest deduction, the remaining scoins will be redeemed
    }

    if (!cw.accountCache.SaveAccount(fcoinGensisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update fcoinGensisAccount %s failed",
                        fcoinGenesisUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(fcoinGensisAccount);
}

bool CCDPRedeemTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.GetGlobalCDPLock()) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, CDP Global Lock is on!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (cdpTxCord.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, cdpTxCord is empty"),
                        REJECT_INVALID, "EMPTY_CDPTXCORD");
    }

    if (scoinsToRedeem == 0) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, scoinsToRedeem is 0"),
                        REJECT_DUST, "REJECT_DUST");
    }
    if (account.fcoins < fcoinsInterest) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, account fcoins %d insufficent for interest %d",
                        account.fcoins, fcoinsInterest), READ_ACCOUNT_FAIL, "account-fcoins-insufficient");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(txUid.get<CPubKey>());
    return true;
 }

 bool CCDPRedeemTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txHash = GetHash();
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

    //2. pay interest fees in wusd or micc into the micc pool
    CUserCdp cdp(txUid.get<CRegID>(), CTxCord(nHeight, nIndex));
    if (cw.cdpCache.GetCdp(cdp)) {
        // check if cdp owner is the same user who's redeeming!
        if (txUid.Get<CRegID>() != cdp.ownerRegId) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, txUid(%s) not CDP owner",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "account-not-CDP-owner");
        }

        if (!PayInterest(nHeight, cdp, cw, state))
            return false;
    }

    //3. redeem in scoins and update cdp
    cdp.totalOwedScoins -= scoinsToRedeem;
    double targetStakedBcoins = collateralRatio * cdp.totalOwedScoins / cw.ppCache.GetScoinMedianPrice();
    if (cdp.totalStakedBcoins < targetStakedBcoins) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, total staked bcoins %d is smaller than target %d",
                        cdp.totalStakedBcoins, targetStakedBcoins), UPDATE_ACCOUNT_FAIL, "targetStakedBcoins-error");
    }
    uint64_t releasedScoins = cdp.totalStakedBcoins - targetStakedBcoins;
    CAccount cdpAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.ownerRegId), cdpAccount)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, read CDP Owner %s account info error",
                        cdp.ownerRegId.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog cdpAcctLog(account);

    cdpAccount.UnFreezeDexCoin(CoinType:WICC, releasedScoins);
    if (!cw.accountCache.SaveAccount(cdpAccount)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update cdpAccount %s failed",
                        fcoinGenesisUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }

    cdp.totalStakedBcoins = (uint64_t) targetStakedBcoins;
    if (!cw.cdpCache.SaveCdp(cdp, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update CDP %s failed",
                        cdp.ownerRegId.ToString()), UPDATE_CDP_FAIL, "bad-save-cdp");
    }

    cw.txUndo.accountLogs.push_back(acctLog);
    cw.txUndo.accountLogs.push_back(cdpAcctLog);

    return true;
 }
 bool CCDPRedeemTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
     //TODO
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
bool CCDPLiquidateTx::PayPenaltyFee(int nHeight, const CUserCdp &cdp, CCacheWrapper &cw, CValidationState &state) {
    double scoins = (double) cdp.scoins;
    double liquidateRate = scoinsToLiquidate / totalOwedScoins;
    double scoinPenaltyFees = scoins * liquidateRate * kDefaultCdpPenaltyFeeRatio / kPencentBoost;

    int totalSubmittedPenaltyFees = scoinsPenalty + fcoinsPenalty * cw.ppCache.GetFcoinMedianPrice();
    if ((int) scoinPenaltyFees > totalSubmittedPenaltyFees) {
        return false; // not enough penalty fees submitted
    }

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read fcoinGenesisUid %s account info error",
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog genesisAcctLog(fcoinGenesisAccount);
    txUndo.push_back(genesisAcctLog);

    uint64_t restScoins = scoinsPenalty / 2;

    fcoinGenesisAccount.fcoins += fcoinsPenalty;    // burn WGRT coins
    fcoinGenesisAccount.scoins += restScoins;       // save into risk reserve fund

    if (!cw.dexCache.CreateBuyOrder(restScoins, CoinType::WGRT)) {
        return false; // DEX: wusd_micc
    }

    return true;
}

bool CCDPLiquidateTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.GetGlobalCDPLock()) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, CDP Global Lock is on!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    if (cdpTxCord.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, cdpTxCord is empty"),
                        REJECT_INVALID, "EMPTY_CDPTXCORD");
    }

    if (fcoinsPenalty == 0 && scoinsPenalty == 0) {
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
    if (fcoinsPenalty > 0 && account.fcoins < fcoinsPenalty) {
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, account fcoins %d < fcoinsPenalty: %d",
                        account.fcoins, fcoinsPenalty), CDP_LIQUIDATE_FAIL, "account-fcoins-insufficient");
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
    cw.txUndo.txHash = GetHash();
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(account); //save account state before modification

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, deduct fees from regId=%s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay penalty fees: 0.13lN --> 50% burn, 50% to Risk Reserve
    CUserCdp cdp(txUid.get<CRegID>(), CTxCord(nHeight, nIndex));
    if (cw.cdpCache.GetCdp(cdp)) {
        //check if CDP is open for liquidation
        uint16_t liquidateRatio = cw.cdpCache.GetDefaultOpenLiquidateRatio();
        uint16_t cdpLiquidateRatio = cw.ppCache.GetBcoinMedianPrice() * cdp.collateralRatioBase;
        if (cdpLiquidateRatio > liquidateRatio) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, CDP collateralRatio (%d) > liquidateRatio (%d)",
                        cdpLiquidateRatio, liquidateRatio), CDP_LIQUIDATE_FAIL, "CDP-liquidate-not-open");
        }
        if (!PayPenaltyFee(nHeight, cdp, cw, state))
            return false;
    }

    //TODO: add CDP log

    double liquidateRatio = scoinsToLiquidate * kPencentBoost / cdp.totalOwedScoins;
    uint64_t bcoinsToLiquidate = cdp.totalStakedBcoins * liquidateRatio;

    uint64_t fullProfitBcoins = cdp.totalStakedBcoins * kDefaultCdpLiquidateProfitRate / kPencentBoost;
    uint64_t profitBcoins = fullProfitBcoins * liquidateRatio;
    uint64_t profitScoins = profitBcoins / cw.ppCache.GetBcoinMedianPrice();

    double collteralRatio = (cdp.totalStakedBcoins / cw.ppCache.GetBcoinMedianPrice) *
                                kPencentBoost / cdp.totalOwedScoins;

    // 3. update liquidator's account
    account.fcoins -= fcoinsPenalty;
    account.scoins -= scoinsPenalty;
    account.scoins -= scoinsToLiquidate;
    account.bcoins += bcoinsToLiquidate; //liquidator income: lM

    // 4. update CDP owner's account: l(M - 1.16N)
    CAccount cdpAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.ownerRegId), cdpAccount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP Owner %s account info error",
                        cdp.ownerRegId.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog cdpAcctLog(account);

    //TODO: change below to add a method in cdpdb.h for cache fetching
    if (collateralRatio > kDefaultCdpLiquidateNonReturnRatio) {
        double deductedScoins = cdp.totalOwedScoins * kDefaultCdpLiquidateNonReturnRatio / kPencentBoost;
        double deductedBcoins = deductedScoins / cw.ppCache.GetBcoinMedianPrice();
        double returnBcoins = (cdp.totalStakedBcoins - deductedBcoins ) * liquidateRatio / kPencentBoost;
        cdpAccount.bcoins += returnBcoins;
        // cdpAccount.UnFreezeDexCoin(CoinType::WICC, returnBcoins); TODO: check if necessary
    }

    // 5. update CDP itself
    if (cdp.totalOwedScoins <= scoinsToLiquidate) {
        cw.cdpCache.EraseCdp(cdp);
    } else {
        uint64_t fcoinsPenaltyInScoins = fcoinsPenalty / cw.ppCache.GetFcoinMedianPrice();
        uint64_t totalDeductedScoins = fcoinsPenaltyInScoins + scoinsPenalty + profitScoins + scoinsToLiquidate;
        uint64_t totalDeductedBcoins = totalDeductedScoins / cw.ppCache.GetBcoinMedianPrice();

        cdp.totalOwedScoins     -= scoinsToLiquidate;
        cdp.totalStakedBcoins   -= totalDeductedBcoins;
        cw.cdpCache.SaveCdp(cdp, cw.txUndo.dbOpLogMap);
    }

    return true;
}
bool CdpLiquidateTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    //TODO
    return true;
}