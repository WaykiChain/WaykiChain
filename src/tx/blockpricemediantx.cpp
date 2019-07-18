// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockpricemediantx.h"
#include "main.h"


bool CBlockPriceMedianTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    return true;
}
/**
 *  force settle/liquidate any under-collateralized CDP (collateral ratio <= 100%)
 */
bool CBlockPriceMedianTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    cw.txUndo.txid = GetHash();

    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    uint64_t currRiskReserveScoins = fcoinGenesisAccount.scoins;
    CAccountLog fcoinGenesisAcctLog(fcoinGenesisAccount); //save account state before modification

    //0. Check Global Collateral Ratio floor & Collateral Ceiling if reached
    uint64_t globalCollateralRatioFloor = 0;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read global collateral ratio floor error"),
                         READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
    }

    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(cw.ppCache.GetBcoinMedianPrice(nHeight),
                                                      globalCollateralRatioFloor)) {
        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, GlobalCollateralFloorReached!!");
        return true;
    }

    //1. get all CDPs to be force settled
    set<CUserCDP> forceLiquidateCdps;
    uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(nHeight);
    uint64_t forceLiquidateRatio = 0;
    if (!cw.sysParamCache.GetParam(CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read force liquidate ratio error"),
                         READ_SYS_PARAM_FAIL, "read-force-liquidate-ratio-error");
    }
    cw.cdpCache.cdpMemCache.GetCdpListByCollateralRatio(forceLiquidateRatio, bcoinMedianPrice, forceLiquidateCdps);

    //2. force settle each cdp
    int cdpIndex = 0;
    for (auto cdp : forceLiquidateCdps) {
        if (++cdpIndex > kForceSettleCDPMaxCountPerBlock)
            break;

        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, begin to force settle CDP (%s)", cdp.ToString());
        if (currRiskReserveScoins < cdp.totalOwedScoins) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, currRiskReserveScoins(%lu) < cdp.totalOwedScoins(%lu) !!",
                    currRiskReserveScoins, cdp.totalOwedScoins);
            break;
        }

        // a) minus scoins from the risk reserve pool to repay CDP scoins
        uint64_t prevRiskReserveScoins = currRiskReserveScoins;
        currRiskReserveScoins -= cdp.totalOwedScoins;

        // b) sell WICC for WUSD to return to risk reserve pool
        auto pBcoinSellMarketOrder = CDEXSysOrder::CreateSellMarketOrder(CoinType::WUSD, AssetType::WICC, cdp.totalStakedBcoins);
        if (!cw.dexCache.CreateSysOrder(GetHash(), *pBcoinSellMarketOrder, cw.txUndo.dbOpLogMap)) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, CreateSysOrder SellBcoinForScoin (%s) failed!!",
                    pBcoinSellMarketOrder->ToString());
            break;
        }

        // c) inflate WGRT coins and sell them for WUSD to return to risk reserve pool
        assert(cdp.totalOwedScoins > cdp.totalStakedBcoins * bcoinMedianPrice);
        uint64_t fcoinsValueToInflate = cdp.totalOwedScoins - cdp.totalStakedBcoins * bcoinMedianPrice;
        uint64_t fcoinsToInflate = fcoinsValueToInflate / cw.ppCache.GetFcoinMedianPrice(nHeight);
        auto pFcoinSellMarketOrder = CDEXSysOrder::CreateSellMarketOrder(CoinType::WUSD, AssetType::WGRT, fcoinsToInflate);
        if (!cw.dexCache.CreateSysOrder(GetHash(), *pFcoinSellMarketOrder, cw.txUndo.dbOpLogMap)) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, CreateSysOrder SellFcoinForScoin (%s) failed!!",
                    pFcoinSellMarketOrder->ToString());
            break;
        }

        // d) Close the CDP
        cw.cdpCache.EraseCdp(cdp, cw.txUndo.dbOpLogMap);
        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, Force settled CDP: "
                "Placed BcoinSellMarketOrder:  %s\n"
                "Placed FcoinSellMarketOrder:  %s\n"
                "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                pBcoinSellMarketOrder->ToString(),
                pFcoinSellMarketOrder->ToString(),
                prevRiskReserveScoins,
                currRiskReserveScoins);

        //TODO: double check state consistence between MemCache & DBCache for CDP
    }

    fcoinGenesisAccount.scoins = currRiskReserveScoins;
    cw.accountCache.SaveAccount(fcoinGenesisAccount);
    cw.txUndo.accountLogs.push_back(fcoinGenesisAcctLog);

    bool ret = SaveTxAddresses(nHeight, nIndex, cw, state, {txUid});
    return ret;
}

bool CBlockPriceMedianTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    auto iter = cw.txUndo.accountLogs.rbegin();
    for (; iter != cw.txUndo.accountLogs.rend(); ++iter) {
        CAccount account;
        CUserID userId = iter->keyId;
        if (!cw.accountCache.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*iter)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }
        if (!cw.accountCache.SetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::UndoExecuteTx, write account info error"),
                             UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        }
    }

    if (!cw.cdpCache.UndoCdp(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::UndoExecuteTx, undo active buy order failed"),
                         REJECT_INVALID, "bad-undo-data");
    }

    if (!cw.dexCache.UndoSysOrder(cw.txUndo.dbOpLogMap)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::UndoExecuteTx, undo system buy order failed"),
                        UNDO_SYS_ORDER_FAILED, "undo-data-failed");
    }
    return true;
}

string CBlockPriceMedianTx::ToString(CAccountDBCache &accountCache) {
    string pricePoints;
    for (auto it = mapMedianPricePoints.begin(); it != mapMedianPricePoints.end(); ++it) {
        pricePoints += strprintf("{coin_type:%u, price_type:%u, price:%lld}",
                        it->first.coinType, it->first.priceType, it->second);
    };

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "median_price_points=%s\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        pricePoints);

    return str;
}

Object CBlockPriceMedianTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache);

    Array pricePointArray;
    for (auto it = mapMedianPricePoints.begin(); it != mapMedianPricePoints.end(); ++it) {
        Object subItem;
        subItem.push_back(Pair("coin_type",     it->first.coinType));
        subItem.push_back(Pair("price_type",    it->first.priceType));
        subItem.push_back(Pair("price",         it->second));
        pricePointArray.push_back(subItem);
    }
    result.push_back(Pair("median_price_points",   pricePointArray));

    return result;
}

bool CBlockPriceMedianTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

map<CCoinPriceType, uint64_t> CBlockPriceMedianTx::GetMedianPrice() const {
    return mapMedianPricePoints;
}
