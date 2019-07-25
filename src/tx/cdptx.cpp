// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"

#include "config/const.h"
#include "main.h"
#include "entities/receipt.h"
#include "persistence/cdpdb.h"

#include <cmath>

bool ComputeCdpInterest(const int32_t currBlockHeight, const int32_t cpdLastBlockHeight, CCacheWrapper &cw,
                        const uint64_t &total_owed_scoins, uint64_t &interestOut) {
    int32_t blockInterval = currBlockHeight - cpdLastBlockHeight;
    int32_t loanedDays = ceil( (double) blockInterval / kDayBlockTotalCount );

    uint64_t A;
    if (!cw.sysParamCache.GetParam(CDP_INTEREST_PARAM_A, A))
        return false;

    uint64_t B;
    if (!cw.sysParamCache.GetParam(CDP_INTEREST_PARAM_B, B))
        return false;

    uint64_t N = total_owed_scoins;
    double annualInterestRate = 0.1 * (double) A / log10( 1 + B * N);
    interestOut = (uint64_t) (((double) N / 365) * loanedDays * annualInterestRate);

    return true;
}

// CDP owner can redeem his or her CDP that are in liquidation list
bool CCDPStakeTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    uint64_t globalCollateralRatioMin;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_RATIO_MIN error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached( cw.ppCache.GetBcoinMedianPrice(height),
                                                            globalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "global-cdp-lock-is-on");
    }

    uint64_t globalCollateralCeiling;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_CEILING_AMOUNT, globalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_CEILING_AMOUNT error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralCeilingReached(bcoins_to_stake, globalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralCeilingReached!"),
                        REJECT_INVALID, "global-cdp-lock-is-on");
    }

    uint64_t startingCdpCollateralRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
    }
    uint64_t collateralRatio = bcoins_to_stake * cw.ppCache.GetBcoinMedianPrice(height) * kPercentBoost /
                                scoins_to_mint;

    if (collateralRatio < startingCdpCollateralRatio) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal",
                        collateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
    }

    if (cdp_txid.IsNull()) {  // 1st-time CDP creation
        vector<CUserCDP> userCdps;
        if (cw.cdpCache.GetCdpList(txUid.get<CRegID>(), userCdps) && userCdps.size() > 0) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, has open cdp"),
                            REJECT_INVALID, "has-open-cdp");
        }
    }

    uint64_t bcoinsToStakeAmountMin;
    if (!cw.sysParamCache.GetParam(CDP_BCOINSTOSTAKE_AMOUNT_MIN, bcoinsToStakeAmountMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read min coins to stake error"),
                        READ_SYS_PARAM_FAIL, "read-min-coins-to-stake-error");
    }
    if (bcoins_to_stake < bcoinsToStakeAmountMin) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, bcoins to stake %d is too small,",
                    bcoins_to_stake), REJECT_INVALID, "bcoins-too-small-to-stake");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);
    return true;
}

bool CCDPStakeTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");
    }

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. check collateral ratio: parital or total >= 200%
    uint64_t startingCdpCollateralRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
    }
    uint64_t partialCollateralRatio = bcoins_to_stake * cw.ppCache.GetBcoinMedianPrice(height)
                                        * kPercentBoost / scoins_to_mint;

    if (cdp_txid.IsNull()) { // 1st-time CDP creation
        if (partialCollateralRatio < startingCdpCollateralRatio) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal",
                        partialCollateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
        }

        CUserCDP cdp(txUid.get<CRegID>(), GetHash());
        cw.cdpCache.StakeBcoinsToCdp(height, bcoins_to_stake, scoins_to_mint, cdp);

    } else { // further staking on one's existing CDP
        CUserCDP cdp(txUid.get<CRegID>(), cdp_txid);
        if (!cw.cdpCache.GetCdp(cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, invalid cdp_txid %s", cdp_txid.ToString()),
                             REJECT_INVALID, "invalid-stake-cdp-txid");
        }
        if (height < cdp.blockHeight) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, height: %d < cdp.blockHeight: %d",
                    height, cdp.blockHeight), UPDATE_ACCOUNT_FAIL, "height-error");
        }

        uint64_t totalBcoinsToStake = cdp.total_staked_bcoins + bcoins_to_stake;
        uint64_t totalScoinsToOwe = cdp.total_owed_scoins + scoins_to_mint;
        uint64_t totalCollateralRatio = totalBcoinsToStake * cw.ppCache.GetBcoinMedianPrice(height)
                                        * kPercentBoost / totalScoinsToOwe;

        if (partialCollateralRatio < startingCdpCollateralRatio &&
            totalCollateralRatio < startingCdpCollateralRatio) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (partial=%d, total=%d) is smaller than the minimal",
                        partialCollateralRatio, totalCollateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
        }

        uint64_t scoinsInterestToRepay;
        if (!ComputeCdpInterest(height, cdp.blockHeight, cw, cdp.total_owed_scoins, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, ComputeCdpInterest error!"),
                             REJECT_INVALID, "compute-interest-error");
        }

        uint64_t free_scoins = account.GetToken(SYMB::WUSD).free_amount;
        if (free_scoins < scoinsInterestToRepay) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoins balance: %d < scoinsInterestToRepay: %d",
                            free_scoins, scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
        }

        if (!SellInterestForFcoins(scoinsInterestToRepay, cw, state)) {
            //TODO add to error Log
            return false;
        }

        if (!account.OperateBalance(SYMB::WUSD, BalanceOpType::SUB_FREE, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoins balance: < scoinsInterestToRepay: %d",
                            scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
        }

        // settle cdp state & persist
        cw.cdpCache.StakeBcoinsToCdp(height, bcoins_to_stake, scoins_to_mint, cdp);
    }

    // update account accordingly
    if (!account.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, bcoins_to_stake)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, wicc coins insufficient"),
                        INTEREST_INSUFFICIENT, "wicc-insufficient-error");
    }
    account.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, scoins_to_mint);

    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }

    vector<CReceipt> receipts;
    CUserID nullUid;
    CReceipt receipt(nTxType, nullUid, txUid, SYMB::WUSD, scoins_to_mint);
    receipts.push_back(receipt);
    cw.txReceiptCache.SetTxReceipts(GetHash(), receipts);

    bool ret = SaveTxAddresses(height, index, cw, state, {txUid});
    return ret;
}

string CCDPStakeTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress());

    str += strprintf("cdp_txid=%s, bcoins_to_stake=%d, scoins_to_mint=%d",
                    cdp_txid.ToString(), bcoins_to_stake, scoins_to_mint);

    return str;
}

Object CCDPStakeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("txid",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("tx_uid",             txUid.ToString()));
    result.push_back(Pair("addr",               keyId.ToAddress()));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("valid_height",       nValidHeight));

    result.push_back(Pair("cdp_txid",           cdp_txid.ToString()));
    result.push_back(Pair("bcoins_to_stake",    bcoins_to_stake));
    result.push_back(Pair("scoins_to_mint",     scoins_to_mint));

    return result;
}

bool CCDPStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCDPStakeTx::SellInterestForFcoins(const uint64_t scoinsInterestToRepay, CCacheWrapper &cw, CValidationState &state) {
    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(SYMB::WUSD, SYMB::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

/************************************<< CCDPRedeemTx >>***********************************************/
bool CCDPRedeemTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    uint64_t globalCollateralRatioFloor = 0;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, read global collateral ratio floor error"),
                        READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(cw.ppCache.GetBcoinMedianPrice(height),
                                                           globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (cdp_txid.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, cdp_txid is empty"),
                        REJECT_INVALID, "EMPTY_CDP_TXID");
    }

    if (scoins_to_repay == 0) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, scoins_to_repay is 0"),
                        REJECT_DUST, "REJECT_DUST");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);
    return true;
 }

 bool CCDPRedeemTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(SYMB::WICC, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay interest fees in wusd
    CUserCDP cdp(txUid.get<CRegID>(), cdp_txid);
    if (cw.cdpCache.GetCdp(cdp)) {
        if (height < cdp.blockHeight) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, height: %d < cdp.blockHeight: %d",
                            height, cdp.blockHeight), UPDATE_ACCOUNT_FAIL, "height-error");
        }
        
        uint64_t scoinsInterestToRepay;
        if (!ComputeCdpInterest(height, cdp.blockHeight, cw, cdp.total_owed_scoins, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, ComputeCdpInterest error!"),
                            REJECT_INVALID, "interest-insufficient-error");
        }
        if (!account.OperateBalance(SYMB::WUSD, BalanceOpType::SUB_FREE, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, Deduct interest error!"),
                             REJECT_INVALID, "deduct-interest-error");
        }

        if (!SellInterestForFcoins(scoinsInterestToRepay, cdp, cw, state)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, SellInterestForFcoins error!"),
                             REJECT_INVALID, "sell-interest-for-fcoins-error");
        }

    } else {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, txUid(%s) not CDP owner",
                    txUid.ToString()), REJECT_INVALID, "target-cdp-not-exist");
    }

    //3. redeem in scoins and update cdp
    cdp.total_owed_scoins -= scoins_to_repay;
    if (cdp.total_staked_bcoins <= bcoins_to_redeem) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, total staked bcoins %d <= target %d",
                        cdp.total_staked_bcoins, bcoins_to_redeem), REJECT_INVALID, "scoins_to_repay-larger-error");
    }
    cdp.total_staked_bcoins -= bcoins_to_redeem;
    if (!cw.cdpCache.SaveCdp(cdp)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update CDP %s failed",
                        cdp.ownerRegId.ToString()), UPDATE_CDP_FAIL, "bad-save-cdp");
    }

    account.OperateBalance(SYMB::WUSD, BalanceOpType::SUB_FREE, scoins_to_repay);
    account.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, bcoins_to_redeem);
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    
    vector<CReceipt> receipts;
    CUserID nullUid;
    CReceipt receipt1(nTxType, txUid, nullUid, SYMB::WUSD, scoins_to_repay);
    receipts.push_back(receipt1);
    CReceipt receipt2(nTxType, nullUid, txUid, SYMB::WICC, bcoins_to_redeem);
    receipts.push_back(receipt2);
    cw.txReceiptCache.SetTxReceipts(GetHash(), receipts);

    bool ret = SaveTxAddresses(height, index, cw, state, {txUid});
    return ret;
 }

string CCDPRedeemTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress());

    str += strprintf("cdp_txid=%s, scoins_to_repay=%d, bcoins_to_redeem=%d",
                    cdp_txid.ToString(), scoins_to_repay, bcoins_to_redeem);

    return str;
 }

 Object CCDPRedeemTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("txid",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("tx_uid",             txUid.ToString()));
    result.push_back(Pair("addr",               keyId.ToAddress()));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("valid_height",       nValidHeight));

    result.push_back(Pair("cdp_txid",           cdp_txid.ToString()));
    result.push_back(Pair("scoins_to_repay",    scoins_to_repay));
    result.push_back(Pair("bcoins_to_redeem",   bcoins_to_redeem));

    return result;
 }

 bool CCDPRedeemTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
     //TODO
     return true;
 }


bool CCDPRedeemTx::SellInterestForFcoins(const uint64_t scoinsInterestToRepay, CCacheWrapper &cw, CValidationState &state) {
    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(SYMB::WUSD, SYMB::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

/************************************<< CdpLiquidateTx >>***********************************************/
bool CCDPLiquidateTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    uint64_t globalCollateralRatioFloor = 0;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, read global collateral ratio floor error"),
                         READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(cw.ppCache.GetBcoinMedianPrice(height), globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, GlobalCollateralFloorReached!!"), 
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    if (cdp_txid.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, cdp_txid is empty"),
                        REJECT_INVALID, "EMPTY_CDPTXID");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    uint64_t free_scoins = account.GetToken(SYMB::WUSD).free_amount;
    if (free_scoins < scoins_to_liquidate) { // more applicable when scoinPenalty is omitted
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, account scoins %d < scoins_to_liquidate: %d",
                        free_scoins, scoins_to_liquidate), CDP_LIQUIDATE_FAIL, "account-scoins-insufficient");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);
    return true;
}

/**
  * total_staked_bcoinsInScoins : total_owed_scoins = M : N
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
bool CCDPLiquidateTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(SYMB::WICC, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, deduct fees from regId=%s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay penalty fees: 0.13lN --> 50% burn, 50% to Risk Reserve
    CUserCDP cdp(txUid.get<CRegID>(), cdp_txid);
    if (!cw.cdpCache.GetCdp(cdp)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp (%s) not exist!",
                        txUid.ToString()), REJECT_INVALID, "cdp-not-exist");
    }
    CAccount cdpOwnerAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.ownerRegId), cdpOwnerAccount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP Owner txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t collateralRatio = (double)cdp.total_staked_bcoins * cw.ppCache.GetBcoinMedianPrice(height)
                            * kPercentBoost / cdp.total_owed_scoins;

    double liquidateRate; //unboosted on purpose
    double totalScoinsToReturnLiquidator, totalScoinsToLiquidate, totalScoinsToReturnSysFund, totalBcoinsToCDPOwner;

    uint64_t startingCdpLiquidateRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_LIQUIDATE_RATIO, startingCdpLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_START_LIQUIDATE_RATIO error!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    uint64_t nonReturnCdpLiquidateRatio;
    if (!cw.sysParamCache.GetParam(CDP_NONRETURN_LIQUIDATE_RATIO, nonReturnCdpLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_START_LIQUIDATE_RATIO error!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    uint64_t cdpLiquidateDiscountRate;
    if (!cw.sysParamCache.GetParam(CDP_LIQUIDATE_DISCOUNT_RATIO, cdpLiquidateDiscountRate)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_LIQUIDATE_DISCOUNT_RATIO error!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    uint64_t forcedCdpLiquidateRatio;
    if (!cw.sysParamCache.GetParam(CDP_FORCE_LIQUIDATE_RATIO, forcedCdpLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_FORCE_LIQUIDATE_RATIO error!",
                        collateralRatio), READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    if (collateralRatio > startingCdpLiquidateRatio) {        // 1.5++
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp collateralRatio(%d) > 150%!",
                        collateralRatio), REJECT_INVALID, "cdp-not-liquidate-ready");

    } else if (collateralRatio > nonReturnCdpLiquidateRatio) { // 1.13 ~ 1.5
        totalScoinsToLiquidate = ((double) cdp.total_owed_scoins * nonReturnCdpLiquidateRatio / kPercentBoost )
                                * cdpLiquidateDiscountRate / kPercentBoost; //1.096N
        totalScoinsToReturnLiquidator = (double) cdp.total_owed_scoins * nonReturnCdpLiquidateRatio / kPercentBoost; //1.13N
        totalScoinsToReturnSysFund = ((double) cdp.total_owed_scoins * nonReturnCdpLiquidateRatio / kPercentBoost )
                                * cdpLiquidateDiscountRate / kPercentBoost - cdp.total_owed_scoins;
        totalBcoinsToCDPOwner = cdp.total_staked_bcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(height);

    } else if (collateralRatio > forcedCdpLiquidateRatio) {    // 1.04 ~ 1.13
        totalScoinsToReturnLiquidator = (double) cdp.total_staked_bcoins / cw.ppCache.GetBcoinMedianPrice(height); //M
        totalScoinsToLiquidate = totalScoinsToReturnLiquidator * cdpLiquidateDiscountRate / kPercentBoost; //M * 97%
        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.total_owed_scoins; // M - N
        totalBcoinsToCDPOwner = cdp.total_staked_bcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(height);

    } else {                                                    // 0 ~ 1.04
        //Although not likely to happen. but if it does, execute it accordingly.
        totalScoinsToLiquidate = cdp.total_owed_scoins; //N
        totalScoinsToReturnLiquidator = (double) cdp.total_staked_bcoins / cw.ppCache.GetBcoinMedianPrice(height); //M
        if (scoins_to_liquidate < cdp.total_owed_scoins) {
            totalScoinsToReturnLiquidator = scoins_to_liquidate * kPercentBoost / cdpLiquidateDiscountRate;
            uint64_t totalCdpStakeInScoins = cdp.total_staked_bcoins * cw.ppCache.GetBcoinMedianPrice(height);
            if (totalCdpStakeInScoins < totalScoinsToReturnLiquidator) {
                totalScoinsToReturnLiquidator = totalCdpStakeInScoins;
            }
        }
        totalScoinsToReturnSysFund = 0;
        totalBcoinsToCDPOwner = cdp.total_staked_bcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(height);
    }

    vector<CReceipt> receipts;
    if (scoins_to_liquidate >= totalScoinsToLiquidate) {
        uint64_t totalBcoinsToReturnLiquidator = totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(height);

        account.OperateBalance(SYMB::WUSD, SUB_FREE, totalScoinsToLiquidate);
        account.OperateBalance(SYMB::WUSD, SUB_FREE, totalScoinsToReturnSysFund); //penalty fees: 50%: sell2burn & 50%: risk-reserve
        account.OperateBalance(SYMB::WICC, ADD_FREE, totalBcoinsToReturnLiquidator);
        cdpOwnerAccount.OperateBalance(SYMB::WICC, ADD_FREE, totalBcoinsToCDPOwner);

        if (!SellPenaltyForFcoins((uint64_t) totalScoinsToReturnSysFund, cw, state))
            return false;

        //close CDP
        if (!cw.cdpCache.EraseCdp(cdp))
            return false;

        CUserID nullUid;
        CReceipt receipt1(nTxType, txUid, nullUid, SYMB::WUSD, (totalScoinsToLiquidate + totalScoinsToReturnSysFund));
        receipts.push_back(receipt1);

        CReceipt receipt2(nTxType, nullUid, txUid, SYMB::WICC, totalBcoinsToReturnLiquidator);
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.ownerRegId);
        CReceipt receipt3(nTxType, nullUid, ownerUserId, SYMB::WICC, (uint64_t)totalBcoinsToCDPOwner);
        receipts.push_back(receipt3);

    } else { //partial liquidation
        liquidateRate = (double) scoins_to_liquidate / totalScoinsToLiquidate;
        uint64_t totalBcoinsToReturnLiquidator =
            totalScoinsToReturnLiquidator * liquidateRate / cw.ppCache.GetBcoinMedianPrice(height);

        account.OperateBalance(SYMB::WUSD, SUB_FREE, scoins_to_liquidate);
        account.OperateBalance(SYMB::WUSD, SUB_FREE, totalScoinsToReturnSysFund);
        account.OperateBalance(SYMB::WICC, ADD_FREE, totalBcoinsToReturnLiquidator);

        int32_t bcoinsToCDPOwner = totalBcoinsToCDPOwner * liquidateRate;
        cdpOwnerAccount.OperateBalance(SYMB::WICC, ADD_FREE, bcoinsToCDPOwner);

        cdp.total_owed_scoins -= scoins_to_liquidate;
        cdp.total_staked_bcoins -= bcoinsToCDPOwner;

        uint64_t scoinsToReturnSysFund = totalScoinsToReturnSysFund * liquidateRate;
        if (!SellPenaltyForFcoins(scoinsToReturnSysFund, cw, state))
            return false;

        cw.cdpCache.SaveCdp(cdp);

        CUserID nullUid;
        CReceipt receipt1(nTxType, txUid, nullUid, SYMB::WUSD, (scoins_to_liquidate + totalScoinsToReturnSysFund));
        receipts.push_back(receipt1);

        CReceipt receipt2(nTxType, nullUid, txUid, SYMB::WICC, totalBcoinsToReturnLiquidator);
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.ownerRegId);
        CReceipt receipt3(nTxType, nullUid, ownerUserId, SYMB::WICC, bcoinsToCDPOwner);
        receipts.push_back(receipt3);
    }
    cw.txReceiptCache.SetTxReceipts(GetHash(), receipts);

    bool ret = SaveTxAddresses(height, index, cw, state, {txUid});
    return ret;
}

string CCDPLiquidateTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s\n"
        "cdp_txid=%s, scoins_to_liquidate=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), cdp_txid.ToString(),
        scoins_to_liquidate);

    return str;
}

Object CCDPLiquidateTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("txid",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("tx_uid",             txUid.ToString()));
    result.push_back(Pair("addr",               keyId.ToAddress()));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("valid_height",       nValidHeight));

    result.push_back(Pair("cdp_txid",            cdp_txid.ToString()));
    result.push_back(Pair("scoins_to_liquidate", scoins_to_liquidate));

    return result;
}

bool CCDPLiquidateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCDPLiquidateTx::SellPenaltyForFcoins(uint64_t scoinPenaltyFees, CCacheWrapper &cw, CValidationState &state) {

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read fcoinGenesisUid %s account info error"),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t halfScoinsPenalty = scoinPenaltyFees / 2;
    // uint64_t halfFcoinsPenalty = halfScoinsPenalty / cw.ppCache.GetFcoinMedianPrice();

    fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, halfScoinsPenalty); // save to risk reserve

    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(SYMB::WUSD, SYMB::WGRT, halfScoinsPenalty);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder)) {
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::ExecuteTx, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}
