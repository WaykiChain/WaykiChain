// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"

#include "main.h"
#include "entities/receipt.h"
#include "persistence/cdpdb.h"

#include <cmath>

// CDP owner can redeem his or her CDP that are in liquidation list
bool CCDPStakeTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    uint64_t globalCollateralRatioMin;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_RATIO_MIN error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached( cw.ppCache.GetBcoinMedianPrice(nHeight),
                                                            globalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "global-cdp-lock-is-on");
    }

    uint64_t globalCollateralCeiling;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_CEILING_AMOUNT, globalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_CEILING_AMOUNT error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralCeilingReached(bcoinsToStake, globalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralCeilingReached!"),
                        REJECT_INVALID, "global-cdp-lock-is-on");
    }

    uint64_t startingCdpCollateralRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
    }
    // TODO:
    // uint64_t collateralRatio = bcoinsToStake * cw.ppCache.GetBcoinMedianPrice(nHeight) * kPercentBoost / scoinsToMint;
    // if (collateralRatio < startingCdpCollateralRatio) {
    //     return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal",
    //                     collateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
    // }

    if (cdpTxId.IsNull()) {  // first-time CDP creation
        vector<CUserCDP> userCdps;
        if (cw.cdpCache.GetCdpList(txUid.get<CRegID>(), userCdps) && userCdps.size() > 0) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, has open cdp"),
                            REJECT_INVALID, "has-open-cdp");
        }
    }

    uint64_t bcoinsToStakeAmountMin;
    if (!cw.sysParamCache.GetParam(CDP_BCOINS_TOSTAKE_AMOUNT_MIN, bcoinsToStakeAmountMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read min coins to stake error"),
                        READ_SYS_PARAM_FAIL, "read-min-coins-to-stake-error");
    }

    if (bcoinsToStake < bcoinsToStakeAmountMin) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, bcoins to stake %d is too small,",
                    bcoinsToStake), REJECT_INVALID, "bcoins-too-small-to-stake");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    IMPLEMENT_CHECK_TX_SIGNATURE(account.pubKey);
    return true;
}

bool CCDPStakeTx::ExecuteTx(int32_t nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
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

    uint64_t startingCdpCollateralRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
    }
    // TODO: un-comment
    // uint64_t collateralRatio = bcoinsToStake * cw.ppCache.GetBcoinMedianPrice(nHeight) * kPercentBoost / scoinsToMint;
    // if (collateralRatio < startingCdpCollateralRatio) {
    //     return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal",
    //                     collateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
    // }

    //2. mint scoins
    if (cdpTxId.IsNull()) { // first-time CDP creation
        CUserCDP cdp(txUid.get<CRegID>(), GetHash());
        // settle cdp state & persist for the 1st-time
        cw.cdpCache.StakeBcoinsToCdp(nHeight, bcoinsToStake, scoinsToMint, cdp, cw.txUndo.dbOpLogMap);
    } else { // further staking on one's existing CDP
        CUserCDP cdp(txUid.get<CRegID>(), cdpTxId);
        if (!cw.cdpCache.GetCdp(cdp)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, invalid cdpTxId %s", cdpTxId.ToString()),
                             REJECT_INVALID, "invalid-stake-cdp-txid");
        }
        if (nHeight < cdp.blockHeight) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, nHeight: %d < cdp.blockHeight: %d",
                    nHeight, cdp.blockHeight), UPDATE_ACCOUNT_FAIL, "nHeight-error");
        }

        uint64_t scoinsInterestToRepay;
        if (!ComputeCdpInterest(nHeight, cdp.blockHeight, cw, cdp.totalOwedScoins, scoinsInterestToRepay)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, ComputeCdpInterest error!"),
                             REJECT_INVALID, "compute-interest-error");
        }

        if (account.scoins < scoinsInterestToRepay) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, scoins balance: %d < scoinsInterestToRepay: %d",
                            account.scoins, scoinsInterestToRepay), INTEREST_INSUFFICIENT, "interest-insufficient-error");
        }

        if (!SellInterestForFcoins(scoinsInterestToRepay, cw, state)) {
            return false;
        }

        account.scoins -= scoinsInterestToRepay;

        // settle cdp state & persist
        cw.cdpCache.StakeBcoinsToCdp(nHeight, bcoinsToStake, scoinsToMint, cdp, cw.txUndo.dbOpLogMap);
    }

    if (!account.StakeBcoinsToCdp(CoinType::WICC, bcoinsToStake, scoinsToMint)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, stake bcoins from regId=%s failed",
                        txUid.ToString()), STAKE_CDP_FAIL, "cdp-stake-bcoins-failed");
    }
    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    cw.txUndo.accountLogs.push_back(acctLog);

    vector<CReceipt> receipts;
    CUserID nullUid;
    CReceipt receipt(nTxType, nullUid, txUid, WUSD, scoinsToMint);
    receipts.push_back(receipt);
    cw.txReceiptCache.SetTxReceipts(cw.txUndo.txid, receipts);

    bool ret = SaveTxAddresses(nHeight, nIndex, cw, state, {txUid});
    return ret;
}

bool CCDPStakeTx::UndoExecuteTx(int32_t nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    auto iter = cw.txUndo.accountLogs.rbegin();
    for (; iter != cw.txUndo.accountLogs.rend(); ++iter) {
        CAccount account;
        CUserID userId = iter->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*iter)) {
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

    if (!cw.dexCache.UndoSysOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::UndoExecuteTx, undo system buy order failed"),
                        UNDO_SYS_ORDER_FAILED, "undo-data-failed");
    }

    return true;
}


bool ComputeCdpInterest(const int32_t currBlockHeight, const int32_t cpdLastBlockHeight, CCacheWrapper &cw,
                        const uint64_t &totalOwedScoins, uint64_t &interest) {
    int32_t blockInterval = currBlockHeight - cpdLastBlockHeight;
    int32_t loanedDays = ceil( (double) blockInterval / kDayBlockTotalCount );

    uint64_t A;
    if (!cw.sysParamCache.GetParam(CDP_INTEREST_PARAM_A, A))
        return false;

    uint64_t B;
    if (!cw.sysParamCache.GetParam(CDP_INTEREST_PARAM_B, B))
        return false;

    uint64_t N = totalOwedScoins;
    double annualInterestRate = 0.1 * (double) A / log10( 1 + B * N);
    interest = (uint64_t) (((double) N / 365) * loanedDays * annualInterestRate);

    return true;
}

string CCDPStakeTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToAddress(), keyId.ToAddress());

    str += strprintf("cdpTxId=%s, bcoinsToStake=%d, scoinsToMint=%d", cdpTxId.ToString(), bcoinsToStake, scoinsToMint);

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

    result.push_back(Pair("cdp_txid",           cdpTxId.ToString()));
    result.push_back(Pair("bcoins_to_stake",    bcoinsToStake));
    result.push_back(Pair("scoins_to_mint",     scoinsToMint));

    return result;
}

bool CCDPStakeTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCDPStakeTx::SellInterestForFcoins(const uint64_t scoinsInterestToRepay, CCacheWrapper &cw,
                                        CValidationState &state) {
    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(CoinType::WUSD, CoinType::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

/************************************<< CCDPRedeemTx >>***********************************************/
bool CCDPRedeemTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    uint64_t globalCollateralRatioFloor = 0;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, read global collateral ratio floor error"), READ_SYS_PARAM_FAIL,
                         "read-global-collateral-ratio-floor-error");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(cw.ppCache.GetBcoinMedianPrice(nHeight),
                                                           globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, GlobalCollateralFloorReached!!"), REJECT_INVALID,
                         "gloalcdplock_is_on");
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

    IMPLEMENT_CHECK_TX_SIGNATURE(account.pubKey);
    return true;
 }

 bool CCDPRedeemTx::ExecuteTx(int32_t nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
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
        if (nHeight < cdp.blockHeight) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, nHeight: %d < cdp.blockHeight: %d",
                            nHeight, cdp.blockHeight), UPDATE_ACCOUNT_FAIL, "nHeight-error");
        }
        if (!SellInterestForFcoins(nHeight, cdp, cw, state))
            return false;

        account.scoins -= scoinsInterest;
    } else {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, txUid(%s) not CDP owner",
                    txUid.ToString()), REJECT_INVALID, "target-cdp-not-exist");
    }

    //3. redeem in scoins and update cdp
    cdp.totalOwedScoins -= scoinsToRedeem;
    double targetStakedBcoins =
        ((double)collateralRatio / kPercentBoost) * cdp.totalOwedScoins / cw.ppCache.GetBcoinMedianPrice(nHeight);
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
    vector<CReceipt> receipts;
    CUserID nullUid;
    CReceipt receipt1(nTxType, txUid, nullUid, WUSD, scoinsToRedeem);
    receipts.push_back(receipt1);
    CReceipt receipt2(nTxType, nullUid, txUid, WICC, releasedBcoins);
    receipts.push_back(receipt2);
    cw.txReceiptCache.SetTxReceipts(cw.txUndo.txid, receipts);

    cw.txUndo.accountLogs.push_back(acctLog);

    return true;
 }

 bool CCDPRedeemTx::UndoExecuteTx(int32_t nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    auto iter = cw.txUndo.accountLogs.rbegin();
    for (; iter != cw.txUndo.accountLogs.rend(); ++iter) {
        CAccount account;
        CUserID userId = iter->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*iter)) {
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

    if (!cw.dexCache.UndoSysOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo system buy order failed"),
                        UNDO_SYS_ORDER_FAILED, "undo-data-failed");
    }

     return true;
 }

string CCDPRedeemTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress());

    str += strprintf("cdpTxId=%s, scoinsToRedeem=%d, collateralRatio=%d, scoinsInterest=%d",
                    cdpTxId.ToString(), scoinsToRedeem, collateralRatio, scoinsInterest);

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

    result.push_back(Pair("cdp_txid",            cdpTxId.ToString()));
    result.push_back(Pair("scoins_to_redeem",    scoinsToRedeem));
    result.push_back(Pair("collateral_ratio",    collateralRatio / kPercentBoost));
    result.push_back(Pair("scoins_interest",     scoinsInterest));

    return result;
 }

 bool CCDPRedeemTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
     //TODO
     return true;
 }
 
 bool CCDPRedeemTx::SellInterestForFcoins(const int nHeight, const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state) {
    uint64_t scoinsInterestToRepay;
    if (!ComputeCdpInterest(nHeight, cdp.blockHeight, cw, cdp.totalOwedScoins, scoinsInterestToRepay)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, ComputeCdpInterest error!"),
                        REJECT_INVALID, "interest-insufficient-error");
    }

    if (scoinsInterest < scoinsInterestToRepay) {
         return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, scoinsInterest: %d < scoinsInterestToRepay: %d",
                    scoinsInterest, scoinsInterestToRepay), UPDATE_ACCOUNT_FAIL, "scoins-interest-insufficient-error");
    }

    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(CoinType::WUSD, CoinType::WGRT, scoinsInterest);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

/************************************<< CdpLiquidateTx >>***********************************************/
bool CCDPLiquidateTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    uint64_t globalCollateralRatioFloor = 0;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, read global collateral ratio floor error"),
                         READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(cw.ppCache.GetBcoinMedianPrice(nHeight),
                                                           globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, GlobalCollateralFloorReached!!"), REJECT_INVALID,
                         "gloalcdplock_is_on");
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

    IMPLEMENT_CHECK_TX_SIGNATURE(account.pubKey);
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
bool CCDPLiquidateTx::ExecuteTx(int32_t nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txid = GetHash();
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog acctLog(account); //save account state before modification
    cw.txUndo.accountLogs.push_back(acctLog);

    //1. pay miner fees (WICC)
    if (!account.OperateBalance(CoinType::WICC, MINUS_VALUE, llFees)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, deduct fees from regId=%s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    //2. pay penalty fees: 0.13lN --> 50% burn, 50% to Risk Reserve
    CUserCDP cdp(cdpOwnerRegId, cdpTxId);
    if (!cw.cdpCache.GetCdp(cdp)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp (%s) not exist!",
                        txUid.ToString()), REJECT_INVALID, "cdp-not-exist");
    }
    CAccount cdpOwnerAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.ownerRegId), cdpOwnerAccount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP Owner txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccountLog cdpAcctLog(cdpOwnerAccount); //save account state before modification
    cw.txUndo.accountLogs.push_back(cdpAcctLog);

    uint64_t collateralRatio =
        (double)cdp.totalStakedBcoins * cw.ppCache.GetBcoinMedianPrice(nHeight) * kPercentBoost / cdp.totalOwedScoins;

    double liquidateRate; //unboosted
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
        totalScoinsToLiquidate = ((double) cdp.totalOwedScoins * nonReturnCdpLiquidateRatio / kPercentBoost )
                                * cdpLiquidateDiscountRate / kPercentBoost; //1.096N
        totalScoinsToReturnLiquidator = (double) cdp.totalOwedScoins * nonReturnCdpLiquidateRatio / kPercentBoost; //1.13N
        totalScoinsToReturnSysFund = ((double) cdp.totalOwedScoins * nonReturnCdpLiquidateRatio / kPercentBoost )
                                * cdpLiquidateDiscountRate / kPercentBoost - cdp.totalOwedScoins;
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);

    } else if (collateralRatio > forcedCdpLiquidateRatio) {    // 1.04 ~ 1.13
        totalScoinsToReturnLiquidator = (double) cdp.totalStakedBcoins / cw.ppCache.GetBcoinMedianPrice(nHeight); //M
        totalScoinsToLiquidate = totalScoinsToReturnLiquidator * cdpLiquidateDiscountRate / kPercentBoost; //M * 97%
        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.totalOwedScoins; // M - N
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);

    } else {                                                    // 0 ~ 1.04
        //Although not likely to happen. but if it does, execute it accordingly.
        totalScoinsToLiquidate = cdp.totalOwedScoins; //N
        totalScoinsToReturnLiquidator = (double) cdp.totalStakedBcoins / cw.ppCache.GetBcoinMedianPrice(nHeight); //M
        if (scoinsToLiquidate < cdp.totalOwedScoins) {
            totalScoinsToReturnLiquidator = scoinsToLiquidate * kPercentBoost / cdpLiquidateDiscountRate;
            uint64_t totalCdpStakeInScoins = cdp.totalStakedBcoins * cw.ppCache.GetBcoinMedianPrice(nHeight);
            if (totalCdpStakeInScoins < totalScoinsToReturnLiquidator) {
                totalScoinsToReturnLiquidator = totalCdpStakeInScoins;
            }
        }
        totalScoinsToReturnSysFund = 0;
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);
    }

    vector<CReceipt> receipts;
    if (scoinsToLiquidate >= totalScoinsToLiquidate) {
        uint64_t totalBcoinsToReturnLiquidator =
            totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);

        account.scoins -= totalScoinsToLiquidate;
        account.scoins -= scoinsPenalty;
        account.bcoins += totalBcoinsToReturnLiquidator;

        cdpOwnerAccount.bcoins += totalBcoinsToCDPOwner;

        if (!SellPenaltyForFcoins((uint64_t) totalScoinsToReturnSysFund, nHeight, cdp, cw, state))
            return false;

        //close CDP
        if (!cw.cdpCache.EraseCdp(cdp, cw.txUndo.dbOpLogMap))
            return false;

        CUserID nullUid;
        CReceipt receipt1(nTxType, txUid, nullUid, WUSD, (totalScoinsToLiquidate + scoinsPenalty));
        receipts.push_back(receipt1);

        CReceipt receipt2(nTxType, nullUid, txUid, WICC, totalBcoinsToReturnLiquidator);
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.ownerRegId);
        CReceipt receipt3(nTxType, nullUid, ownerUserId, WICC, (uint64_t)totalBcoinsToCDPOwner);
        receipts.push_back(receipt3);

    } else { //partial liquidation
        liquidateRate = (double) scoinsToLiquidate / totalScoinsToLiquidate;
        uint64_t totalBcoinsToReturnLiquidator =
            totalScoinsToReturnLiquidator * liquidateRate / cw.ppCache.GetBcoinMedianPrice(nHeight);

        account.scoins -= scoinsToLiquidate;
        account.scoins -= scoinsPenalty;
        account.bcoins += totalBcoinsToReturnLiquidator;

        int bcoinsToCDPOwner = totalBcoinsToCDPOwner * liquidateRate;
        cdpOwnerAccount.bcoins += bcoinsToCDPOwner;

        cdp.totalOwedScoins -= scoinsToLiquidate;
        cdp.totalStakedBcoins -= bcoinsToCDPOwner;

        uint64_t scoinsToReturnSysFund = totalScoinsToReturnSysFund * liquidateRate;
        if (!SellPenaltyForFcoins(scoinsToReturnSysFund, nHeight, cdp, cw, state))
            return false;

        cw.cdpCache.SaveCdp(cdp, cw.txUndo.dbOpLogMap);

        CUserID nullUid;
        CReceipt receipt1(nTxType, txUid, nullUid, WUSD, (scoinsToLiquidate + scoinsPenalty));
        receipts.push_back(receipt1);

        CReceipt receipt2(nTxType, nullUid, txUid, WICC, totalBcoinsToReturnLiquidator);
        receipts.push_back(receipt2);

        CUserID ownerUserId(cdp.ownerRegId);
        CReceipt receipt3(nTxType, nullUid, ownerUserId, WICC, bcoinsToCDPOwner);
        receipts.push_back(receipt3);
    }
    cw.txReceiptCache.SetTxReceipts(cw.txUndo.txid, receipts);

    return true;
}

bool CCDPLiquidateTx::UndoExecuteTx(int32_t nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    auto iter = cw.txUndo.accountLogs.rbegin();
    for (; iter != cw.txUndo.accountLogs.rend(); ++iter) {
        CAccount account;
        CUserID userId = iter->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*iter)) {
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

    if (!cw.dexCache.UndoSysOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::UndoExecuteTx, undo system buy order failed"),
                        UNDO_SYS_ORDER_FAILED, "undo-data-failed");
    }

    return true;
}

string CCDPLiquidateTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s\n"
        "cdpTxId=%s, scoinsToLiquidate=%d, scoinsPenalty=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), cdpTxId.ToString(),
        scoinsToLiquidate, scoinsPenalty);

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

    result.push_back(Pair("cdp_txid",            cdpTxId.ToString()));
    result.push_back(Pair("scoins_to_liquidate", scoinsToLiquidate));
    result.push_back(Pair("scoinsPenalty",       scoinsPenalty / kPercentBoost));

    return result;
}

bool CCDPLiquidateTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

bool CCDPLiquidateTx::SellPenaltyForFcoins(uint64_t scoinPenaltyToSysFund, const int nHeight, const CUserCDP &cdp, CCacheWrapper &cw, CValidationState &state) {

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

    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(CoinType::WUSD, CoinType::WGRT, halfScoinsPenalty);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::ExecuteTx, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}