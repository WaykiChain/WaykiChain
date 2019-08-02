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

/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *
 *  ==> ratio = a / Log10 (b+N)
 */
bool ComputeCdpInterest(const int32_t currBlockHeight, const uint32_t cpdLastBlockHeight, CCacheWrapper &cw,
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
    IMPLEMENT_CHECK_TX_FEE(fee_symbol);
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (!kCDPCoinPairSet.count(std::pair<TokenSymbol, TokenSymbol>(bcoin_symbol, scoin_symbol))) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, invalid bcoin-scoin CDPCoinPair! "),
                        REJECT_INVALID, "invalid-CDPCoinPair-symbol");
    }

    uint64_t globalCollateralRatioMin;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_RATIO_MIN error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(cw.ppCache.GetBcoinMedianPrice(height),
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

    uint64_t collateralRatio = scoins_to_mint == 0 ? 100000000 :
                                        bcoins_to_stake * cw.ppCache.GetBcoinMedianPrice(height) / scoins_to_mint;

    if (collateralRatio < startingCdpCollateralRatio) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal(%d)",
                        collateralRatio, startingCdpCollateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
    }

    if (cdp_txid.IsNull()) {  // 1st-time CDP creation
        vector<CUserCDP> userCdps;
        if (cw.cdpCache.GetCDPList(txUid.get<CRegID>(), userCdps) && userCdps.size() > 0) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, has open cdp"), REJECT_INVALID, "has-open-cdp");
        }
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
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(fee_symbol, BalanceOpType::SUB_FREE, llFees))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");

    //2. check collateral ratio: parital or total >= 200%
    uint64_t startingCdpCollateralRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");

    uint64_t partialCollateralRatio = scoins_to_mint == 0 ? 100000000 :
                                        bcoins_to_stake * cw.ppCache.GetBcoinMedianPrice(height) / scoins_to_mint;

    if (cdp_txid.IsNull()) { // 1st-time CDP creation
        if (partialCollateralRatio < startingCdpCollateralRatio)
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal",
                        partialCollateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");

        uint64_t bcoinsToStakeAmountMin;
        if (!cw.sysParamCache.GetParam(CDP_BCOINSTOSTAKE_AMOUNT_MIN, bcoinsToStakeAmountMin)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read min coins to stake error"),
                            READ_SYS_PARAM_FAIL, "read-min-coins-to-stake-error");
        }
        if (bcoins_to_stake < bcoinsToStakeAmountMin) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, bcoins to stake %d is too small,",
                        bcoins_to_stake), REJECT_INVALID, "bcoins-too-small-to-stake");
        }

        CUserCDP cdp(txUid.get<CRegID>(), GetHash(), height, bcoin_symbol, scoin_symbol, bcoins_to_stake, scoins_to_mint);

        if (!cw.cdpCache.NewCDP(height, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, save new cdp to db failed"),
                            READ_SYS_PARAM_FAIL, "save-new-cdp-failed");
        }

    } else { // further staking on one's existing CDP
        CUserCDP cdp;
        if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, invalid cdp_txid %s", cdp_txid.ToString()),
                             REJECT_INVALID, "invalid-stake-cdp-txid");
        }
        if (height < cdp.block_height) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, height: %d < cdp.block_height: %d",
                    height, cdp.block_height), UPDATE_ACCOUNT_FAIL, "height-error");
        }

        uint64_t totalBcoinsToStake = cdp.total_staked_bcoins + bcoins_to_stake;
        uint64_t totalScoinsToOwe   = cdp.total_owed_scoins + scoins_to_mint;
        uint64_t totalCollateralRatio = totalBcoinsToStake * cw.ppCache.GetBcoinMedianPrice(height) / totalScoinsToOwe;

        if (partialCollateralRatio < startingCdpCollateralRatio && totalCollateralRatio < startingCdpCollateralRatio) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (partial=%d, total=%d) is smaller than the minimal",
                        partialCollateralRatio, totalCollateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
        }

        uint64_t scoinsInterestToRepay;
        if (!ComputeCdpInterest(height, cdp.block_height, cw, cdp.total_owed_scoins, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, ComputeCdpInterest error!"),
                             REJECT_INVALID, "compute-interest-error");
        }

        uint64_t free_scoins = account.GetToken(scoin_symbol).free_amount;
        if (free_scoins < scoinsInterestToRepay) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoins balance: %d < scoinsInterestToRepay: %d",
                            free_scoins, scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
        }

        if (!SellInterestForFcoins(cdp, scoinsInterestToRepay, cw, state)) {
            //TODO add to error Log
            return false;
        }

        if (!account.OperateBalance(scoin_symbol, BalanceOpType::SUB_FREE, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoins balance: < scoinsInterestToRepay: %d",
                            scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
        }

        // settle cdp state & persist
        if (!cw.cdpCache.StakeBcoinsToCDP(height, bcoins_to_stake, scoins_to_mint, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, save changed cdp to db failed"),
                            READ_SYS_PARAM_FAIL, "save-changed-cdp-failed");
        }
    }

    // update account accordingly
    if (!account.OperateBalance(bcoin_symbol, BalanceOpType::SUB_FREE, bcoins_to_stake)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, bcoins insufficient"),
                        INTEREST_INSUFFICIENT, "bcoins-insufficient-error");
    }
    account.OperateBalance(scoin_symbol, BalanceOpType::ADD_FREE, scoins_to_mint);

    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }

    vector<CReceipt> receipts;
    CUserID nullUid;
    CReceipt receipt(nTxType, nullUid, txUid, scoin_symbol, scoins_to_mint);
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

    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)
    result.push_back(Pair("fee_symbol",         fee_symbol));
    result.push_back(Pair("cdp_txid",           cdp_txid.ToString()));
    result.push_back(Pair("bcoin_symbol",       bcoin_symbol));
    result.push_back(Pair("scoin_symbol",       scoin_symbol));
    result.push_back(Pair("bcoins_to_stake",    bcoins_to_stake));
    result.push_back(Pair("scoins_to_mint",     scoins_to_mint));
    return result;
}

bool CCDPStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCDPStakeTx::SellInterestForFcoins(const CUserCDP &cdp, const uint64_t scoinsInterestToRepay, CCacheWrapper &cw, CValidationState &state) {
    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(cdp.scoin_symbol, SYMB::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

/************************************<< CCDPRedeemTx >>***********************************************/
bool CCDPRedeemTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE(fee_symbol);
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
    if (!account.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, deduct fees from regId=%s failed,",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay interest fees in wusd
    CUserCDP cdp;
    if (cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        if (height < cdp.block_height) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, height: %d < cdp.block_height: %d",
                            height, cdp.block_height), UPDATE_ACCOUNT_FAIL, "height-error");
        }

        uint64_t scoinsInterestToRepay;
        if (!ComputeCdpInterest(height, cdp.block_height, cw, cdp.total_owed_scoins, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, ComputeCdpInterest error!"),
                            REJECT_INVALID, "interest-insufficient-error");
        }
        if (!account.OperateBalance(cdp.scoin_symbol, BalanceOpType::SUB_FREE, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, Deduct interest error!"),
                             REJECT_INVALID, "deduct-interest-error");
        }

        if (!SellInterestForFcoins(cdp, scoinsInterestToRepay, cw, state)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, SellInterestForFcoins error!"),
                             REJECT_INVALID, "sell-interest-for-fcoins-error");
        }

    } else {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, txUid(%s) not CDP owner",
                    txUid.ToString()), REJECT_INVALID, "target-cdp-not-exist");
    }

    //3. redeem in scoins and update cdp
    if (scoins_to_repay > cdp.total_owed_scoins)
        scoins_to_repay = cdp.total_owed_scoins;

    if (cdp.total_staked_bcoins <= bcoins_to_redeem) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, total_staked_bcoins %d <= target %d",
                        cdp.total_staked_bcoins, bcoins_to_redeem), REJECT_INVALID, "scoins_to_repay-larger-error");
    }
    if (!cw.cdpCache.RedeemBcoinsFromCDP(height, bcoins_to_redeem, scoins_to_repay, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update CDP %s failed", cdp.owner_regid.ToString()),
                         UPDATE_CDP_FAIL, "bad-save-cdp");
    }

    if (!account.OperateBalance(cdp.scoin_symbol, BalanceOpType::SUB_FREE, scoins_to_repay)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update account(%s) SUB WUSD(%lu) failed",
                        account.regid.ToString(), scoins_to_repay), UPDATE_CDP_FAIL, "bad-operate-account");
    }
    if (!account.OperateBalance(cdp.bcoin_symbol, BalanceOpType::ADD_FREE, bcoins_to_redeem)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update account(%s) ADD WICC(%lu) failed",
                        account.regid.ToString(), bcoins_to_redeem), UPDATE_CDP_FAIL, "bad-operate-account");
    }
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }

    vector<CReceipt> receipts;
    CUserID nullUid;
    CReceipt receipt1(nTxType, txUid, nullUid, cdp.scoin_symbol, scoins_to_repay);
    receipts.push_back(receipt1);
    CReceipt receipt2(nTxType, nullUid, txUid, cdp.bcoin_symbol, bcoins_to_redeem);
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

    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache);
    result.push_back(Pair("fee_symbol",         fee_symbol));
    result.push_back(Pair("cdp_txid",           cdp_txid.ToString()));
    result.push_back(Pair("scoins_to_repay",    scoins_to_repay));
    result.push_back(Pair("bcoins_to_redeem",   bcoins_to_redeem));

    return result;
 }

 bool CCDPRedeemTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
     //TODO
     return true;
 }


bool CCDPRedeemTx::SellInterestForFcoins(const CUserCDP &cdp, const uint64_t scoinsInterestToRepay, CCacheWrapper &cw, CValidationState &state) {
    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(cdp.scoin_symbol, SYMB::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

/************************************<< CdpLiquidateTx >>***********************************************/
bool CCDPLiquidateTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE(fee_symbol);
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

    CUserCDP cdp;
    if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp (%s) not exist!",
                        txUid.ToString()), REJECT_INVALID, "cdp-not-exist");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    uint64_t free_scoins = account.GetToken(cdp.scoin_symbol).free_amount;
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
    if (!account.OperateBalance(fee_symbol, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, deduct fees from regId=%s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay penalty fees: 0.13lN --> 50% burn, 50% to Risk Reserve
    CUserCDP cdp;
    if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp (%s) not exist!",
                        txUid.ToString()), REJECT_INVALID, "cdp-not-exist");
    }
    CAccount cdpOwnerAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP Owner txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t collateralRatio = (double)cdp.total_staked_bcoins * cw.ppCache.GetBcoinMedianPrice(height)
                                / cdp.total_owed_scoins;

    double liquidateRate; //unboosted on purpose
    double totalBcoinsToReturnLiquidator, totalScoinsToLiquidate, totalScoinsToReturnSysFund, totalBcoinsToCdpOwner;

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
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp collateralRatio(%d) > 150%%!",
                        collateralRatio), REJECT_INVALID, "cdp-not-liquidate-ready");

    } else if (collateralRatio > nonReturnCdpLiquidateRatio) { // 1.13 ~ 1.5
        totalBcoinsToReturnLiquidator = (double) cdp.total_owed_scoins * nonReturnCdpLiquidateRatio
                                            / cw.ppCache.GetBcoinMedianPrice(height) ; //1.13N

        totalBcoinsToCdpOwner = cdp.total_staked_bcoins - totalBcoinsToReturnLiquidator;

        totalScoinsToLiquidate = ((double) cdp.total_owed_scoins * nonReturnCdpLiquidateRatio / kPercentBoost )
                                * cdpLiquidateDiscountRate / kPercentBoost; //1.096N

        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.total_owed_scoins;

    } else if (collateralRatio > forcedCdpLiquidateRatio) {    // 1.04 ~ 1.13
        totalBcoinsToReturnLiquidator = cdp.total_staked_bcoins; //M
        totalBcoinsToCdpOwner = 0;
        totalScoinsToLiquidate = totalBcoinsToReturnLiquidator
                                * ((double) cw.ppCache.GetBcoinMedianPrice(height) / kPercentBoost)
                                * cdpLiquidateDiscountRate / kPercentBoost; //M * 97%

        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.total_owed_scoins; // M * 97% - N

    } else {                                                    // 0 ~ 1.04
        //Although not likely to happen, but when it does, execute it accordingly.
        totalBcoinsToReturnLiquidator = cdp.total_staked_bcoins;
        totalBcoinsToCdpOwner = 0;
        totalScoinsToLiquidate = cdp.total_owed_scoins;         // N
        totalScoinsToReturnSysFund = 0;
    }

    vector<CReceipt> receipts;

    if (scoins_to_liquidate >= totalScoinsToLiquidate) {
        account.OperateBalance(cdp.scoin_symbol, SUB_FREE, totalScoinsToLiquidate);
        account.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToReturnLiquidator);
        cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToCdpOwner);

        if (!ProcessPenaltyFees(cdp, (uint64_t) totalScoinsToReturnSysFund, cw, state))
            return false;

        //close CDP
        if (!cw.cdpCache.EraseCDP(cdp))
            return false;

        CUserID nullUid;
        CReceipt receipt1(nTxType, txUid, nullUid, cdp.scoin_symbol, (totalScoinsToLiquidate + totalScoinsToReturnSysFund));
        receipts.push_back(receipt1);

        CReceipt receipt2(nTxType, nullUid, txUid, cdp.bcoin_symbol, totalBcoinsToReturnLiquidator);
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.owner_regid);
        CReceipt receipt3(nTxType, nullUid, ownerUserId, cdp.bcoin_symbol, (uint64_t)totalBcoinsToCdpOwner);
        receipts.push_back(receipt3);

    } else { //partial liquidation
        liquidateRate = (double) scoins_to_liquidate / totalScoinsToLiquidate;
        totalBcoinsToReturnLiquidator *= liquidateRate;

        account.OperateBalance(cdp.scoin_symbol, SUB_FREE, scoins_to_liquidate);
        account.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToReturnLiquidator);

        int32_t bcoinsToCDPOwner = totalBcoinsToCdpOwner * liquidateRate;
        cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, ADD_FREE, bcoinsToCDPOwner);

        cdp.total_owed_scoins -= cdp.total_owed_scoins * liquidateRate;

        uint64_t totalBcoinsToDeduct = totalBcoinsToReturnLiquidator + bcoinsToCDPOwner;
        if (cdp.total_staked_bcoins <= totalBcoinsToDeduct) {
            cdp.total_staked_bcoins = 0;
        } else {
            cdp.total_staked_bcoins -= totalBcoinsToDeduct;
        }

        uint64_t scoinsToReturnSysFund = totalScoinsToReturnSysFund * liquidateRate;
        if (!ProcessPenaltyFees(cdp, scoinsToReturnSysFund, cw, state))
            return false;

        if (!cw.cdpCache.SaveCDP(cdp)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, update CDP %s failed",
                        cdp.owner_regid.ToString()), UPDATE_CDP_FAIL, "bad-save-cdp");
        }

        CUserID nullUid;
        CReceipt receipt1(nTxType, txUid, nullUid, cdp.scoin_symbol, (scoins_to_liquidate + totalScoinsToReturnSysFund));
        receipts.push_back(receipt1);

        CReceipt receipt2(nTxType, nullUid, txUid, cdp.bcoin_symbol, totalBcoinsToReturnLiquidator);
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.owner_regid);
        CReceipt receipt3(nTxType, nullUid, ownerUserId, cdp.bcoin_symbol, bcoinsToCDPOwner);
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

    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache)
    result.push_back(Pair("cdp_txid",            cdp_txid.ToString()));
    result.push_back(Pair("scoins_to_liquidate", scoins_to_liquidate));

    return result;
}

bool CCDPLiquidateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCDPLiquidateTx::ProcessPenaltyFees(const CUserCDP &cdp, uint64_t scoinPenaltyFees, CCacheWrapper &cw, CValidationState &state) {
    if (scoinPenaltyFees == 0)
        return true;

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read fcoinGenesisUid %s account info error"),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t halfScoinsPenalty = scoinPenaltyFees / 2;

    uint64_t minSysOrderPenaltyFee;
    if (!cw.sysParamCache.GetParam(CDP_SYSORDER_PENALTY_FEE_MIN, minSysOrderPenaltyFee)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, read CDP_SYSORDER_PENALTY_FEE_MIN error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    if (scoinPenaltyFees > minSysOrderPenaltyFee ) { //10+ WUSD
        // 1) save 50% penalty fees into risk riserve
        fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, halfScoinsPenalty);

        // 2) sell 50% penalty fees for Fcoins and burn
        auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(cdp.scoin_symbol, SYMB::WGRT, halfScoinsPenalty);
        if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder)) {
            return state.DoS(100, ERRORMSG("CdpLiquidateTx::ExecuteTx, create system buy order failed"),
                            CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
        }
    } else {
        // save penalty fees into risk riserve
        fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, scoinPenaltyFees);
    }

    return true;
}
