// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"
#include "main.h"
#include "persistence/cdpdb.h"

#include <math.h>

string CCDPStakeTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf("txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, keyId.ToAddress(), keyId.ToString());

    str += strprintf("cdpTxId=%s, bcoinsToStake=%d, collateralRatio=%d, scoinsInterest=%d",
                    cdpTxId.ToString(), bcoinsToStake, collateralRatio, scoinsInterest);

    return str;
}

Object CCDPStakeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",               GetHash().GetHex()));
    result.push_back(Pair("tx_type",            GetTxType(nTxType)));
    result.push_back(Pair("ver",                nVersion));
    result.push_back(Pair("tx_uid",             txUid.ToString()));
    result.push_back(Pair("addr",               keyId.ToAddress()));
    result.push_back(Pair("fees",               llFees));
    result.push_back(Pair("valid_height",       nValidHeight));

    result.push_back(Pair("cdp_txid",           cdpTxId.ToString()));
    result.push_back(Pair("bcoins_to_stake",    bcoinsToStake));
    result.push_back(Pair("collateral_ratio",   collateralRatio / kPercentBoost));
    result.push_back(Pair("scoins_interest",    scoinsInterest));

    return result;
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

    auto pSysBuyMarketOrder = CDEXSysOrder::CreateBuyMarketOrder(CoinType::WUSD, CoinType::WGRT, scoinsInterest);
    if (!cw.dexCache.CreateSysOrder(GetHash(), *pSysBuyMarketOrder, cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    return true;
}

// CDP owner can redeem his or her CDP that are in liquidation list
bool CCDPStakeTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    uint32_t _GlobalCollateralRatioMin;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, _GlobalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_RATIO_MIN error!!"),
                        REJECT_INVALID, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralFloorReached(cw.ppCache.GetBcoinMedianPrice(nHeight), _GlobalCollateralRatioMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralFloorReached!!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    uint32_t _GlobalCollateralCeiling;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_CEILING_AMOUNT, _GlobalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read GLOBAL_COLLATERAL_CEILING_AMOUNT error!!"),
                        REJECT_INVALID, "read-sysparamdb-err");
    }
    if (cw.cdpCache.CheckGlobalCollateralCeilingReached(bcoinsToStake, _GlobalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, GlobalCollateralCeilingReached!"),
                        REJECT_INVALID, "gloalcdplock_is_on");
    }

    uint32_t _StartingCdpCollateralRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, _StartingCdpCollateralRatio)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                        REJECT_INVALID, "read-sysparamdb-err");
    }
    if (cdpTxId.IsNull() && collateralRatio < _StartingCdpCollateralRatio) { // first-time CDP creation
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, collateral ratio (%d) is smaller than the minimal",
                            collateralRatio), REJECT_INVALID, "CDP-collateral-ratio-toosmall");
    }

    uint32_t _BcoinsToStakeAmountMin;
    (!cw.sysParamCache.GetParam(CDP_BCOINS_TOSTAKE_AMOUNT_MIN, _BcoinsToStakeAmountMin)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_BCOINS_TOSTAKE_AMOUNT_MIN error!!"),
                        REJECT_INVALID, "read-sysparamdb-err");
    }

    if (bcoinsToStake < _BcoinsToStakeAmountMin) {
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

    //2. mint scoins
    int mintedScoins = 0;
    if (cdpTxId.IsNull()) { // first-time CDP creation
        CUserCDP cdp(txUid.get<CRegID>(), GetHash());
        mintedScoins = bcoinsToStake * kPercentBoost / collateralRatio;
        //settle cdp state & persist for the 1st-time
        cw.cdpCache.StakeBcoinsToCdp(nHeight, bcoinsToStake, (uint64_t) mintedScoins, cdp, cw.txUndo.dbOpLogMap);

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

        uint64_t currentCollateralRatio = cdp.totalStakedBcoins * cw.ppCache.GetBcoinMedianPrice(nHeight)
                                        * kPercentBoost / cdp.totalOwedScoins;


        uint32_t _StartingCdpCollateralRatio;
        if (!cw.sysParamCache.GetParam(CDP_START_COLLATERAL_RATIO, _StartingCdpCollateralRatio)) {
            return state.DoS(100, ERRORMSG("CCDPStakeTx::CheckTx, read CDP_START_COLLATERAL_RATIO error!!"),
                            REJECT_INVALID, "read-sysparamdb-err");
        }
        if ( collateralRatio < currentCollateralRatio  && collateralRatio < _StartingCdpCollateralRatio) {
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

/************************************<< CCDPRedeemTx >>***********************************************/
string CCDPRedeemTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf("txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n", GetTxType(nTxType),
                     GetHash().ToString(), nVersion, keyId.ToAddress(), keyId.ToString());

    str += strprintf("cdpTxId=%s, scoinsToRedeem=%d, collateralRatio=%d, scoinsInterest=%d",
                    cdpTxId.ToString(), scoinsToRedeem, collateralRatio, scoinsInterest);

    return str;
 }
 Object CCDPRedeemTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",               GetHash().GetHex()));
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
    uint64_t scoinsInterestToRepay = cw.cdpCache.ComputeInterest(nHeight, cdp);
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

bool CCDPRedeemTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.CheckGlobalCollateralFloorReached(cw.ppCache.GetBcoinMedianPrice(nHeight))) {
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

/************************************<< CdpLiquidateTx >>***********************************************/
string CCDPLiquidateTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    string str = strprintf( "txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n"
                            "cdpTxId=%s, scoinsToLiquidate=%d, scoinsPenalty=%d",
                            GetTxType(nTxType), GetHash().ToString(), nVersion, keyId.ToAddress(), keyId.ToString(),
                            cdpTxId.ToString(), scoinsToLiquidate, scoinsPenalty);

    return str;
}
Object CCDPLiquidateTx::ToJson(const CAccountDBCache &accountCache) const {
  Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    result.push_back(Pair("hash",               GetHash().GetHex()));
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

bool CCDPLiquidateTx::CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (cw.cdpCache.CheckGlobalCollateralFloorReached(cw.ppCache.GetBcoinMedianPrice(nHeight))) {
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

    int collateralRatio = (double) cdp.totalStakedBcoins * cw.ppCache.GetBcoinMedianPrice(nHeight) * kPercentBoost
                            / cdp.totalOwedScoins;

    double liquidateRate; //unboosted
    double totalScoinsToReturnLiquidator, totalScoinsToLiquidate, totalScoinsToReturnSysFund, totalBcoinsToCDPOwner;

    uint32_t _StartingCdpLiquidateRatio;
    if (!cw.sysParamCache.GetParam(CDP_START_LIQUIDATE_RATIO, _StartingCdpLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_START_LIQUIDATE_RATIO error!",
                        collateralRatio), REJECT_INVALID, "read-sysparamdb-err");
    }

    uint32_t _NonReturnCdpLiquidateRatio;
    if (!cw.sysParamCache.GetParam(CDP_NONRETURN_LIQUIDATE_RATIO, _NonReturnCdpLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_START_LIQUIDATE_RATIO error!",
                        collateralRatio), REJECT_INVALID, "read-sysparamdb-err");
    }

    uint32_t _CdpLiquidateDiscountRate;
    if (!cw.sysParamCache.GetParam(CDP_LIQUIDATE_DISCOUNT_RATIO, _CdpLiquidateDiscountRate)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_LIQUIDATE_DISCOUNT_RATIO error!",
                        collateralRatio), REJECT_INVALID, "read-sysparamdb-err");
    }
    
    uint32_t _ForcedCdpLiquidateRatio;
    if (!cw.sysParamCache.GetParam(CDP_FORCE_LIQUIDATE_RATIO, _ForcedCdpLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP_FORCE_LIQUIDATE_RATIO error!",
                        collateralRatio), REJECT_INVALID, "read-sysparamdb-err");
    }
    
    if (collateralRatio > _StartingCdpLiquidateRatio) {        // 1.5++
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp collateralRatio(%d) > 150%!",
                        collateralRatio), REJECT_INVALID, "cdp-not-liquidate-ready");

    } else if (collateralRatio > _NonReturnCdpLiquidateRatio) { // 1.13 ~ 1.5
        totalScoinsToLiquidate = ((double) cdp.totalOwedScoins * _NonReturnCdpLiquidateRatio / kPercentBoost )
                                * _CdpLiquidateDiscountRate / kPercentBoost; //1.096N
        totalScoinsToReturnLiquidator = (double) cdp.totalOwedScoins * _NonReturnCdpLiquidateRatio / kPercentBoost; //1.13N
        totalScoinsToReturnSysFund = ((double) cdp.totalOwedScoins * _NonReturnCdpLiquidateRatio / kPercentBoost )
                                * _CdpLiquidateDiscountRate / kPercentBoost - cdp.totalOwedScoins;
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);

    } else if (collateralRatio > _ForcedCdpLiquidateRatio) {    // 1.04 ~ 1.13
        totalScoinsToReturnLiquidator = (double) cdp.totalStakedBcoins / cw.ppCache.GetBcoinMedianPrice(nHeight); //M
        totalScoinsToLiquidate = totalScoinsToReturnLiquidator * _CdpLiquidateDiscountRate / kPercentBoost; //M * 97%
        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.totalOwedScoins; // M - N
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);

    } else {                                                    // 0 ~ 1.04
        //Although not likely to happen. but if it does, execute it accordingly.
        totalScoinsToLiquidate = cdp.totalOwedScoins; //N
        totalScoinsToReturnLiquidator = (double) cdp.totalStakedBcoins / cw.ppCache.GetBcoinMedianPrice(nHeight); //M
        if (scoinsToLiquidate < cdp.totalOwedScoins) {
            totalScoinsToReturnLiquidator = scoinsToLiquidate * kPercentBoost / _CdpLiquidateDiscountRate;
            uint64_t totalCdpStakeInScoins = cdp.totalStakedBcoins * cw.ppCache.GetBcoinMedianPrice(nHeight);
            if (totalCdpStakeInScoins < totalScoinsToReturnLiquidator) {
                totalScoinsToReturnLiquidator = totalCdpStakeInScoins;
            }
        }
        totalScoinsToReturnSysFund = 0;
        totalBcoinsToCDPOwner = cdp.totalStakedBcoins - totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);
    }

    if (scoinsToLiquidate >= totalScoinsToLiquidate) {
        account.scoins -= totalScoinsToLiquidate;
        account.scoins -= scoinsPenalty;
        account.bcoins += totalScoinsToReturnLiquidator / cw.ppCache.GetBcoinMedianPrice(nHeight);

        cdpOwnerAccount.bcoins += totalBcoinsToCDPOwner;

        if (!SellPenaltyForFcoins((uint64_t) totalScoinsToReturnSysFund, nHeight, cdp, cw, state))
            return false;

        //close CDP
        cw.cdpCache.EraseCdp(cdp, cw.txUndo.dbOpLogMap);

    } else { //partial liquidation
        liquidateRate = (double) scoinsToLiquidate / totalScoinsToLiquidate;

        account.scoins -= scoinsToLiquidate;
        account.scoins -= scoinsPenalty;
        account.bcoins += totalScoinsToReturnLiquidator * liquidateRate / cw.ppCache.GetBcoinMedianPrice(nHeight);

        int bcoinsToCDPOwner = totalBcoinsToCDPOwner * liquidateRate;
        cdpOwnerAccount.bcoins += bcoinsToCDPOwner;

        cdp.totalOwedScoins -= scoinsToLiquidate;
        cdp.totalStakedBcoins -= bcoinsToCDPOwner;

        uint64_t scoinsToReturnSysFund = totalScoinsToReturnSysFund * liquidateRate;
        if (!SellPenaltyForFcoins(scoinsToReturnSysFund, nHeight, cdp, cw, state))
            return false;

        cw.cdpCache.SaveCdp(cdp, cw.txUndo.dbOpLogMap);
    }

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