// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"

#include "config/const.h"
#include "main.h"
#include "persistence/cdpdb.h"

#include <cmath>

using namespace std;

/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *
 *  ==> ratio = a / Log10 (b+N)
 */
bool ComputeCDPInterest(const int32_t currBlockHeight, const uint32_t cdpLastBlockHeight, CCacheWrapper &cw,
                        const uint64_t total_owed_scoins, uint64_t &interestOut) {
    if (total_owed_scoins == 0) {
        interestOut = 0;

        return true;
    }

    int32_t blockInterval = currBlockHeight - cdpLastBlockHeight;
    int32_t loanedDays    = std::max<int32_t>(1, ceil((double)blockInterval / ::GetDayBlockCount(currBlockHeight)));

    uint64_t A;
    if (!cw.sysParamCache.GetParam(CDP_INTEREST_PARAM_A, A))
        return false;

    uint64_t B;
    if (!cw.sysParamCache.GetParam(CDP_INTEREST_PARAM_B, B))
        return false;

    uint64_t N                = total_owed_scoins;
    double annualInterestRate = 0.1 * A / log10(1.0 + B * N / (double)COIN);
    interestOut               = (uint64_t)(((double)N / 365) * loanedDays * annualInterestRate);

    LogPrint("CDP", "ComputeCDPInterest, currBlockHeight: %d, cdpLastBlockHeight: %d, loanedDays: %d, A: %llu, B: %llu, N: "
             "%llu, annualInterestRate: %f, interestOut: %llu\n",
             currBlockHeight, cdpLastBlockHeight, loanedDays, A, B, N, annualInterestRate, interestOut);

    return true;
}

// CDP owner can redeem his or her CDP that are in liquidation list
bool CCDPStakeTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (!kCDPCoinPairSet.count(std::pair<TokenSymbol, TokenSymbol>(bcoin_symbol, scoin_symbol))) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, invalid bcoin-scoin CDPCoinPair!"),
                        REJECT_INVALID, "invalid-CDPCoinPair-symbol");
    }

    uint64_t globalCollateralRatioMin;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_RATIO_MIN error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    uint64_t slideWindowBlockCount;
    if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error!!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(
            cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount), globalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "global-collateral-floor-reached");
    }

    uint64_t globalCollateralCeiling;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_CEILING_AMOUNT, globalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_CEILING_AMOUNT error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralCeilingReached(bcoins_to_stake, globalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralCeilingReached!"),
                        REJECT_INVALID, "global-collateral-ceiling-reached");
    }

    LogPrint("CDP", "CCDPStakeTx::CheckTx, globalCollateralRatioMin: %llu, slideWindowBlockCount: %llu, "
             "globalCollateralCeiling: %llu\n",
             globalCollateralRatioMin, slideWindowBlockCount, globalCollateralCeiling);

    if (cdp_txid.IsEmpty()) {  // 1st-time CDP creation
        vector<CUserCDP> userCdps;
        if (cw.cdpCache.GetCDPList(txUid.get<CRegID>(), userCdps) && userCdps.size() > 0) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, has open cdp"), REJECT_INVALID, "has-open-cdp");
        }
    } else {
        CUserCDP cdp;
        if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, the cdp not exist! cdp_txid=%s", cdp_txid.ToString()),
                             REJECT_INVALID, "cdp-not-exist");
        }

        if (txUid.get<CRegID>() != cdp.owner_regid) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, permission denied! cdp_txid=%s, owner(%s) vs operator(%s)",
                cdp_txid.ToString(), cdp.owner_regid.ToString(), txUid.ToString()), REJECT_INVALID, "permission-denied");
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
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");

    uint64_t slideWindowBlockCount;
    if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error!!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount);
    if (bcoinMedianPrice == 0) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, failed to acquire bcoin median price!!"),
                         REJECT_INVALID, "acquire-bcoin-median-price-err");
    }

    uint64_t partialCollateralRatio =
        scoins_to_mint == 0 ? UINT64_MAX : uint64_t(double(bcoins_to_stake) * bcoinMedianPrice / scoins_to_mint);

    if (cdp_txid.IsEmpty()) { // 1st-time CDP creation
        if (partialCollateralRatio < startingCdpCollateralRatio)
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, collateral ratio (%llu) is smaller than the minimal (%llu)",
                            partialCollateralRatio, startingCdpCollateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");

        CUserCDP cdp(txUid.get<CRegID>(), GetHash(), height, bcoin_symbol, scoin_symbol, bcoins_to_stake, scoins_to_mint);

        if (!cw.cdpCache.NewCDP(height, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, save new cdp to db failed"),
                            READ_SYS_PARAM_FAIL, "save-new-cdp-failed");
        }

        uint64_t bcoinsToStakeAmountMinInScoin;
        if (!cw.sysParamCache.GetParam(CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN, bcoinsToStakeAmountMinInScoin)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, read min coins to stake error"),
                            READ_SYS_PARAM_FAIL, "read-min-coins-to-stake-error");
        }

        uint64_t bcoinsToStakeAmountMin = bcoinsToStakeAmountMinInScoin / (double(bcoinMedianPrice) / kPercentBoost);
        if (cdp.total_staked_bcoins < bcoinsToStakeAmountMin) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, total staked bcoins (%llu vs %llu) is too small",
                        cdp.total_staked_bcoins, bcoinsToStakeAmountMin), REJECT_INVALID, "total-staked-bcoins-too-small");
        }
    } else { // further staking on one's existing CDP
        CUserCDP cdp;
        if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, the cdp not exist! cdp_txid=%s", cdp_txid.ToString()),
                             REJECT_INVALID, "cdp-not-exist");
        }

        CUserCDP oldCDP = cdp; // copy before modify.

        if (height < cdp.block_height) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, height: %d < cdp.block_height: %d",
                    height, cdp.block_height), UPDATE_ACCOUNT_FAIL, "height-error");
        }

        uint64_t totalBcoinsToStake   = cdp.total_staked_bcoins + bcoins_to_stake;
        uint64_t totalScoinsToOwe     = cdp.total_owed_scoins + scoins_to_mint;
        uint64_t totalCollateralRatio = uint64_t(double(totalBcoinsToStake) * bcoinMedianPrice / totalScoinsToOwe);

        if (partialCollateralRatio < startingCdpCollateralRatio && totalCollateralRatio < startingCdpCollateralRatio) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, collateral ratio (partial=%d, total=%d) is smaller than the minimal",
                        partialCollateralRatio, totalCollateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
        }

        uint64_t scoinsInterestToRepay;
        if (!ComputeCDPInterest(height, cdp.block_height, cw, cdp.total_owed_scoins, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, ComputeCDPInterest error!"),
                             REJECT_INVALID, "compute-interest-error");
        }

        uint64_t free_scoins = account.GetToken(scoin_symbol).free_amount;
        if (free_scoins < scoinsInterestToRepay) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoins balance: %llu < scoinsInterestToRepay: %llu",
                            free_scoins, scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
        }

        if (!SellInterestForFcoins(CTxCord(height, index), cdp, scoinsInterestToRepay, cw, state))
            return false;

        if (!account.OperateBalance(scoin_symbol, BalanceOpType::SUB_FREE, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoins balance: < scoinsInterestToRepay: %d",
                            scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
        }

        // settle cdp state & persist
        cdp.AddStake(height, bcoins_to_stake, scoins_to_mint);
        if (!cw.cdpCache.UpdateCDP(oldCDP, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, save changed cdp to db failed"),
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
    CReceipt receipt(nullId, txUid, scoin_symbol, scoins_to_mint, "minted scoins to cdp owner");
    receipts.push_back(receipt);
    if(!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");
    return true;
}

string CCDPStakeTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, cdp_txid=%s, bcoins_to_stake=%d, scoins_to_mint=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), cdp_txid.ToString(),
        bcoins_to_stake, scoins_to_mint);
}

Object CCDPStakeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("cdp_txid",           cdp_txid.ToString()));
    result.push_back(Pair("bcoin_symbol",       bcoin_symbol));
    result.push_back(Pair("scoin_symbol",       scoin_symbol));
    result.push_back(Pair("bcoins_to_stake",    bcoins_to_stake));
    result.push_back(Pair("scoins_to_mint",     scoins_to_mint));
    return result;
}

bool CCDPStakeTx::SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp,
    const uint64_t scoinsInterestToRepay,  CCacheWrapper &cw, CValidationState &state) {

    if (scoinsInterestToRepay == 0)
        return true;

    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    // send interest to fcoin genesis account
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, scoinsInterestToRepay)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, operate balance failed"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    // should freeze user's coin for buying the asset
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::FREEZE, scoinsInterestToRepay)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, account has insufficient funds"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, set account info error"),
                        WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol, SYMB::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateActiveOrder(GetHash(), *pSysBuyMarketOrder)) {
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

    uint64_t slideWindowBlockCount;
    if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, read MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error!!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(
            cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount), globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "global-cdp-lock-is-on");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if (cdp_txid.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, cdp_txid is empty"),
                        REJECT_INVALID, "empty-cdpid");
    }

    CUserCDP cdp;
    if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, the cdp not exist! cdp_txid=%s", cdp_txid.ToString()),
                            REJECT_INVALID, "cdp-not-exist");
    }

    if (txUid.get<CRegID>() != cdp.owner_regid) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, permission denied! cdp_txid=%s, owner(%s) vs operator(%s)",
            cdp_txid.ToString(), cdp.owner_regid.ToString(), txUid.ToString()), REJECT_INVALID, "permission-denied");
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
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, deduct fees from regId=%s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay interest fees in wusd
    CUserCDP cdp;
    if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, get cdp=%s failed",
                    cdp_txid.ToString()), REJECT_INVALID, "get-cdp-failed");
    }

    CUserCDP oldCDP = cdp; // copy before modify.

    if (height < cdp.block_height) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, height: %d < cdp.block_height: %d",
                        height, cdp.block_height), UPDATE_ACCOUNT_FAIL, "height-error");
    }

    uint64_t scoinsInterestToRepay = 0;
    if (!ComputeCDPInterest(height, cdp.block_height, cw, cdp.total_owed_scoins, scoinsInterestToRepay)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, ComputeCDPInterest error!"),
                         REJECT_INVALID, "compute-cdp-interest-error");
    }

    if (!account.OperateBalance(cdp.scoin_symbol, BalanceOpType::SUB_FREE, scoinsInterestToRepay)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, Deduct interest error!"),
                         REJECT_INVALID, "deduct-interest-error");
    }

    if (!SellInterestForFcoins(CTxCord(height, index), cdp, scoinsInterestToRepay, cw, state)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, SellInterestForFcoins error!"),
                            REJECT_INVALID, "sell-interest-for-fcoins-error");
    }

    uint64_t startingCdpCollateralRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");

    //3. redeem in scoins and update cdp
    if (bcoins_to_redeem > cdp.total_staked_bcoins) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, the redeemed bcoins=%llu can not bigger than total_staked_bcoins=%llu",
                        bcoins_to_redeem, cdp.total_staked_bcoins), UPDATE_CDP_FAIL, "bcoin_to_redeem-too-large");
    }

    // check account balance vs scoins_to_repay
    if (account.GetToken(cdp.scoin_symbol).free_amount < scoins_to_repay) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, account balance insufficient"), REJECT_INVALID,
                         "account-balance-insufficient");
    }

    uint64_t realRepayScoins = scoins_to_repay;
    if (scoins_to_repay >= cdp.total_owed_scoins) {
        realRepayScoins = cdp.total_owed_scoins;
    }
    cdp.Redeem(height, bcoins_to_redeem, realRepayScoins);

    // check and save CDP to db
    if (cdp.IsFinished()) {
        if (!cw.cdpCache.EraseCDP(oldCDP, cdp))
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, erase the finished CDP %s failed",
                            cdp.cdpid.ToString()), UPDATE_CDP_FAIL, "erase-cdp-failed");
    } else { // partial redeem
        if (bcoins_to_redeem != 0) {
            uint64_t slideWindowBlockCount;
            if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount)) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, read MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error!!"),
                                READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
            }
            uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount);
            if (bcoinMedianPrice == 0) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, failed to acquire bcoin median price!!"),
                                 REJECT_INVALID, "acquire-bcoin-median-price-err");
            }
            uint64_t collateralRatio  = cdp.ComputeCollateralRatio(bcoinMedianPrice);
            if (collateralRatio < startingCdpCollateralRatio) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, the cdp collatera ratio=%.2f%% cannot < %.2f%% after redeem",
                                100.0 * collateralRatio / (double)kPercentBoost, 100.0 * startingCdpCollateralRatio / (double)kPercentBoost),
                                UPDATE_CDP_FAIL, "invalid-collatera-ratio");
            }

            uint64_t bcoinsToStakeAmountMinInScoin;
            if (!cw.sysParamCache.GetParam(CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN, bcoinsToStakeAmountMinInScoin)) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, read min coins to stake error"),
                                 READ_SYS_PARAM_FAIL, "read-min-coins-to-stake-error");
            }

            uint64_t bcoinsToStakeAmountMin =
                bcoinsToStakeAmountMinInScoin / (double(bcoinMedianPrice) / kPercentBoost);
            if (cdp.total_staked_bcoins < bcoinsToStakeAmountMin) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, total staked bcoins (%llu vs %llu) is too small",
                                 cdp.total_staked_bcoins, bcoinsToStakeAmountMin), REJECT_INVALID, "total-staked-bcoins-too-small");
            }
        }

        if (!cw.cdpCache.UpdateCDP(oldCDP, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update CDP %s failed", cdp.cdpid.ToString()),
                            UPDATE_CDP_FAIL, "bad-save-cdp");
        }
    }

    if (!account.OperateBalance(cdp.scoin_symbol, BalanceOpType::SUB_FREE, realRepayScoins)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update account(%s) SUB WUSD(%lu) failed",
                        account.regid.ToString(), realRepayScoins), UPDATE_CDP_FAIL, "bad-operate-account");
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
    CReceipt receipt1(txUid, nullId, cdp.scoin_symbol, realRepayScoins, "real repaid scoins by cdp owner");
    receipts.push_back(receipt1);
    CReceipt receipt2(nullId, txUid, cdp.bcoin_symbol, bcoins_to_redeem, "redeemed bcoins to cdp owner");
    receipts.push_back(receipt2);
    cw.txReceiptCache.SetTxReceipts(GetHash(), receipts);

    return true;
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
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("cdp_txid",           cdp_txid.ToString()));
    result.push_back(Pair("scoins_to_repay",    scoins_to_repay));
    result.push_back(Pair("bcoins_to_redeem",   bcoins_to_redeem));

    return result;
}

bool CCDPRedeemTx::SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp,
                                        const uint64_t scoinsInterestToRepay, CCacheWrapper &cw,
                                        CValidationState &state) {
    if (scoinsInterestToRepay == 0)
        return true;

    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    // send interest to fcoin genesis account
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, scoinsInterestToRepay)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, operate balance failed"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    // should freeze user's coin for buying the asset
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::FREEZE, scoinsInterestToRepay)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, account has insufficient funds"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, set account info error"),
                        WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    auto pSysBuyMarketOrder =
        CDEXSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol, SYMB::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateActiveOrder(GetHash(), *pSysBuyMarketOrder)) {
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

    uint64_t slideWindowBlockCount;
    if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, read MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(
            cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount), globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, GlobalCollateralFloorReached!!"), REJECT_INVALID,
                        "global-cdp-lock-is-on");
    }

    if (cdp_txid.IsEmpty()) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, cdp_txid is empty"), REJECT_INVALID,
                        "empty-cdpid");
    }

    CUserCDP cdp;
    if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp (%s) not exist!", txUid.ToString()),
                        REJECT_INVALID, "cdp-not-exist");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, read txUid %s account info error", txUid.ToString()),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");

    uint64_t free_scoins = account.GetToken(cdp.scoin_symbol).free_amount;
    if (free_scoins < scoins_to_liquidate) {  // more applicable when scoinPenalty is omitted
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::CheckTx, account scoins %d < scoins_to_liquidate: %d", free_scoins,
                        scoins_to_liquidate), CDP_LIQUIDATE_FAIL, "account-scoins-insufficient");
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

    CUserCDP oldCDP = cdp; // copy before modify.

    CAccount cdpOwnerAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP Owner txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t slideWindowBlockCount;
    if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error!!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount);
    if (bcoinMedianPrice == 0) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, failed to acquire bcoin median price!!"),
                         REJECT_INVALID, "acquire-bcoin-median-price-err");
    }

    uint64_t collateralRatio = cdp.ComputeCollateralRatio(bcoinMedianPrice);

    uint64_t totalBcoinsToReturnLiquidator = 0;
    uint64_t totalScoinsToLiquidate        = 0;
    uint64_t totalScoinsToReturnSysFund    = 0;
    uint64_t totalBcoinsToCdpOwner         = 0;

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
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_FORCE_LIQUIDATE_RATIO error!"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    if (collateralRatio > startingCdpLiquidateRatio) {        // 1.5++
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp collateralRatio(%d) > 150%%!",
                        collateralRatio), REJECT_INVALID, "cdp-not-liquidate-ready");

    } else if (collateralRatio > nonReturnCdpLiquidateRatio) { // 1.13 ~ 1.5
        totalBcoinsToReturnLiquidator =  cdp.total_owed_scoins * (double)nonReturnCdpLiquidateRatio / bcoinMedianPrice; //1.13N
        assert(cdp.total_staked_bcoins >= totalBcoinsToReturnLiquidator);

        totalBcoinsToCdpOwner = cdp.total_staked_bcoins - totalBcoinsToReturnLiquidator;

        totalScoinsToLiquidate = ( cdp.total_owed_scoins * (double)nonReturnCdpLiquidateRatio / kPercentBoost )
                                * (double)cdpLiquidateDiscountRate / kPercentBoost; //1.096N

        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.total_owed_scoins;

    } else if (collateralRatio > forcedCdpLiquidateRatio) {    // 1.04 ~ 1.13
        totalBcoinsToReturnLiquidator = cdp.total_staked_bcoins; //M
        totalBcoinsToCdpOwner = 0;
        totalScoinsToLiquidate = totalBcoinsToReturnLiquidator * ((double) bcoinMedianPrice / kPercentBoost)
                                * cdpLiquidateDiscountRate / kPercentBoost; //M * 97%

        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.total_owed_scoins; // M * 97% - N

    } else {                                                    // 0 ~ 1.04
        // Although not likely to happen, but when it does, execute it accordingly.
        totalBcoinsToReturnLiquidator = cdp.total_staked_bcoins;
        totalBcoinsToCdpOwner         = 0;
        totalScoinsToLiquidate        = cdp.total_owed_scoins;  // N
        totalScoinsToReturnSysFund    = 0;
    }

    vector<CReceipt> receipts;

    if (scoins_to_liquidate >= totalScoinsToLiquidate) {
        account.OperateBalance(cdp.scoin_symbol, SUB_FREE, totalScoinsToLiquidate);
        account.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToReturnLiquidator);

        if (account.regid != cdpOwnerAccount.regid) {
            cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToCdpOwner);
            if (!cw.accountCache.SetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount))
                return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, write cdp owner account info error! owner_regid=%s",
                                cdp.owner_regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        } else {  // liquidate by oneself
            account.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToCdpOwner);
        }

        if (!ProcessPenaltyFees(CTxCord(height, index), cdp, (uint64_t)totalScoinsToReturnSysFund, cw, state, receipts))
            return false;

        // close CDP
        if (!cw.cdpCache.EraseCDP(oldCDP, cdp))
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, erase CDP failed! cdpid=%s",
                        cdp.cdpid.ToString()), UPDATE_CDP_FAIL, "erase-cdp-failed");

        CReceipt receipt1(txUid, nullId, cdp.scoin_symbol, totalScoinsToLiquidate,
                          "actual scoins paid by liquidator");
        receipts.push_back(receipt1);

        CReceipt receipt2(nullId, txUid, cdp.bcoin_symbol, totalBcoinsToReturnLiquidator,
                          "total bcoins to return liquidator");
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.owner_regid);
        CReceipt receipt3(nullId, ownerUserId, cdp.bcoin_symbol, (uint64_t)totalBcoinsToCdpOwner,
                          "total bcoins to return cdp owner");
        receipts.push_back(receipt3);

    } else {    // partial liquidation
        double liquidateRate = (double)scoins_to_liquidate / totalScoinsToLiquidate;  // unboosted on purpose
        assert(liquidateRate < 1);
        totalBcoinsToReturnLiquidator *= liquidateRate;

        account.OperateBalance(cdp.scoin_symbol, SUB_FREE, scoins_to_liquidate);
        account.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToReturnLiquidator);

        uint64_t bcoinsToCDPOwner = totalBcoinsToCdpOwner * liquidateRate;
        if (account.regid != cdpOwnerAccount.regid) {
            cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, ADD_FREE, bcoinsToCDPOwner);
            if (!cw.accountCache.SetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount))
                return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, write cdp owner account info error! owner_regid=%s",
                                cdp.owner_regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        } else {  // liquidate by oneself
            account.OperateBalance(cdp.bcoin_symbol, ADD_FREE, bcoinsToCDPOwner);
        }

        uint64_t scoinsToLiquidate = cdp.total_owed_scoins * liquidateRate;
        uint64_t bcoinsToLiquidate = totalBcoinsToReturnLiquidator + bcoinsToCDPOwner;

        assert(cdp.total_owed_scoins > scoinsToLiquidate);
        assert(cdp.total_staked_bcoins > bcoinsToLiquidate);
        cdp.LiquidatePartial(height, bcoinsToLiquidate, scoinsToLiquidate);

        uint64_t bcoinsToStakeAmountMinInScoin;
        if (!cw.sysParamCache.GetParam(CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN, bcoinsToStakeAmountMinInScoin)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read min coins to stake error"),
                             READ_SYS_PARAM_FAIL, "read-min-coins-to-stake-error");
        }

        uint64_t bcoinsToStakeAmountMin = bcoinsToStakeAmountMinInScoin / (double(bcoinMedianPrice) / kPercentBoost);
        if (cdp.total_staked_bcoins < bcoinsToStakeAmountMin) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, total staked bcoins (%llu vs %llu) is too small",
                            cdp.total_staked_bcoins, bcoinsToStakeAmountMin), REJECT_INVALID, "total-staked-bcoins-too-small");
        }

        uint64_t scoinsToReturnSysFund = totalScoinsToReturnSysFund * liquidateRate;
        if (!ProcessPenaltyFees(CTxCord(height, index), cdp, scoinsToReturnSysFund, cw, state, receipts))
            return false;

        if (!cw.cdpCache.UpdateCDP(oldCDP, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, update CDP failed! cdpid=%s",
                        cdp.cdpid.ToString()), UPDATE_CDP_FAIL, "bad-save-cdp");
        }

        CReceipt receipt1(txUid, nullId, cdp.scoin_symbol, scoins_to_liquidate,
                          "actual scoins paid by liquidator");
        receipts.push_back(receipt1);

        CReceipt receipt2(nullId, txUid, cdp.bcoin_symbol, totalBcoinsToReturnLiquidator,
                          "total bcoins to return liquidator");
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.owner_regid);
        CReceipt receipt3(nullId, ownerUserId, cdp.bcoin_symbol, bcoinsToCDPOwner,
                          "total bcoins to return cdp owner");
        receipts.push_back(receipt3);
    }

    if (!cw.accountCache.SetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, write txUid %s account info error",
            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");

    if (!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ExecuteTx, write tx receipt failed! txid=%s",
            GetHash().ToString()), REJECT_INVALID, "write-tx-receipt-failed");

    return true;
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
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("cdp_txid",            cdp_txid.ToString()));
    result.push_back(Pair("scoins_to_liquidate", scoins_to_liquidate));

    return result;
}

bool CCDPLiquidateTx::ProcessPenaltyFees(const CTxCord &txCord, const CUserCDP &cdp, uint64_t scoinPenaltyFees,
    CCacheWrapper &cw, CValidationState &state, vector<CReceipt> &receipts) {

    if (scoinPenaltyFees == 0)
        return true;

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ProcessPenaltyFees, read fcoinGenesisUid %s account info error"),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t minSysOrderPenaltyFee;
    if (!cw.sysParamCache.GetParam(CDP_SYSORDER_PENALTY_FEE_MIN, minSysOrderPenaltyFee)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ProcessPenaltyFees, read CDP_SYSORDER_PENALTY_FEE_MIN error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    if (scoinPenaltyFees > minSysOrderPenaltyFee ) { //10+ WUSD
        uint64_t halfScoinsPenalty = scoinPenaltyFees / 2;
        uint64_t leftScoinPenalty  = scoinPenaltyFees - scoinPenaltyFees;  // handle odd amount

        // 1) save 50% penalty fees into risk riserve
        fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, halfScoinsPenalty);

        // 2) sell 50% penalty fees for Fcoins and burn
        // send half scoin penalty to fcoin genesis account
        fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, leftScoinPenalty);

        // should freeze user's coin for buying the asset
        if (!fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::FREEZE, leftScoinPenalty)) {
            return state.DoS(100, ERRORMSG("CdpLiquidateTx::ProcessPenaltyFees, account has insufficient funds"),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }

        auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol, SYMB::WGRT, leftScoinPenalty);
        if (!cw.dexCache.CreateActiveOrder(GetHash(), *pSysBuyMarketOrder)) {
            return state.DoS(100, ERRORMSG("CdpLiquidateTx::ProcessPenaltyFees, create system buy order failed"),
                            CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
        }

        CUserID fcoinGenesisUid(fcoinGenesisAccount.regid);
        CReceipt receipt1(nullId, fcoinGenesisUid, cdp.scoin_symbol, halfScoinsPenalty,
                          "half scoin penalty into risk riserve");
        receipts.push_back(receipt1);

        CReceipt receipt2(nullId, fcoinGenesisAccount.regid, cdp.scoin_symbol, leftScoinPenalty,
                          "left half scoin penalty to create sys order: WGRT/WUSD");
        receipts.push_back(receipt2);
    } else {
        // save penalty fees into risk riserve
        fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, scoinPenaltyFees);

        CUserID fcoinGenesisUid(fcoinGenesisAccount.regid);
        CReceipt receipt(nullId, fcoinGenesisUid, cdp.scoin_symbol, scoinPenaltyFees,
                         "save penalty fees into risk riserve");
        receipts.push_back(receipt);
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("CAssetIssueTx::ProcessPenaltyFees, write fcoin genesis account info error!"),
                UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");

    return true;
}
