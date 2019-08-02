// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockpricemediantx.h"
#include "main.h"


bool CBlockPriceMedianTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    // TODO: txUid == miner?
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    return true;
}

/**
 *  force settle/liquidate any under-collateralized CDP (collateral ratio <= 100%)
 */
bool CBlockPriceMedianTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    map<CoinPricePair, uint64_t> mapMedianPricePoints;
    if (!cw.ppCache.GetBlockMedianPricePoints(height, mapMedianPricePoints)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, failed to get block median price points"),
                         READ_PRICE_POINT_FAIL, "bad-read-price-points");
    }

    if (mapMedianPricePoints != median_price_points) {
        string pricePoints;
        for (const auto item : mapMedianPricePoints) {
            pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first,
                                     item.first.second, item.second);
        };

        LogPrint("ERROR", "CBlockPriceMedianTx::ExecuteTx, from cache, height: %d, price points: %s\n", height, pricePoints);

        pricePoints.clear();
        for (const auto item : median_price_points) {
            pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first,
                                     item.first.second, item.second);
        };

        LogPrint("ERROR", "CBlockPriceMedianTx::ExecuteTx, from median tx, height: %d, price points: %s\n", height, pricePoints);

        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, invalid median price points"), REJECT_INVALID,
                         "bad-median-price-points");
    }

    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    uint64_t currRiskReserveScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;

    //0. Check Global Collateral Ratio floor & Collateral Ceiling if reached
    uint64_t globalCollateralRatioFloor = 0;
    if (!cw.sysParamCache.GetParam(GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read global collateral ratio floor error"),
                         READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
    }

    uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(height);
    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(bcoinMedianPrice, globalCollateralRatioFloor)) {
        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, GlobalCollateralFloorReached!!");
        return true;
    }

    //1. get all CDPs to be force settled
    set<CUserCDP> forceLiquidateCdps;
    uint64_t forceLiquidateRatio = 0;
    if (!cw.sysParamCache.GetParam(CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read force liquidate ratio error"),
                         READ_SYS_PARAM_FAIL, "read-force-liquidate-ratio-error");
    }
    cw.cdpCache.cdpMemCache.GetCdpListByCollateralRatio(forceLiquidateRatio, bcoinMedianPrice, forceLiquidateCdps);

    //2. force settle each cdp
    int32_t cdpIndex = 0;
    for (auto cdp : forceLiquidateCdps) {
        if (++cdpIndex > kForceSettleCDPMaxCountPerBlock)
            break;

        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, begin to force settle CDP (%s)", cdp.ToString());
        if (currRiskReserveScoins < cdp.total_owed_scoins) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, currRiskReserveScoins(%lu) < cdp.total_owed_scoins(%lu) !!",
                    currRiskReserveScoins, cdp.total_owed_scoins);
            break;
        }

        // a) minus scoins from the risk reserve pool to repay CDP scoins
        uint64_t prevRiskReserveScoins = currRiskReserveScoins;
        currRiskReserveScoins -= cdp.total_owed_scoins;

        // b) sell WICC for WUSD to return to risk reserve pool
        auto pBcoinSellMarketOrder = CDEXSysOrder::CreateSellMarketOrder(SYMB::WUSD, SYMB::WICC, cdp.total_staked_bcoins);
        if (!cw.dexCache.CreateSysOrder(GetHash(), *pBcoinSellMarketOrder)) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, CreateSysOrder SellBcoinForScoin (%s) failed!!",
                    pBcoinSellMarketOrder->ToString());
            break;
        }

        // c) inflate WGRT coins and sell them for WUSD to return to risk reserve pool
        assert(cdp.total_owed_scoins > cdp.total_staked_bcoins * bcoinMedianPrice);
        uint64_t fcoinsValueToInflate = cdp.total_owed_scoins - cdp.total_staked_bcoins * bcoinMedianPrice;
        uint64_t fcoinsToInflate = fcoinsValueToInflate / cw.ppCache.GetFcoinMedianPrice(height);
        auto pFcoinSellMarketOrder = CDEXSysOrder::CreateSellMarketOrder(SYMB::WUSD, SYMB::WGRT, fcoinsToInflate);
        if (!cw.dexCache.CreateSysOrder(GetHash(), *pFcoinSellMarketOrder)) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, CreateSysOrder SellFcoinForScoin (%s) failed!!",
                    pFcoinSellMarketOrder->ToString());
            break;
        }

        // d) Close the CDP
        cw.cdpCache.EraseCDP(cdp);
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

    uint64_t prevScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
    fcoinGenesisAccount.OperateBalance(SYMB::WUSD, ADD_FREE, currRiskReserveScoins - prevScoins);

    cw.accountCache.SaveAccount(fcoinGenesisAccount);

    return SaveTxAddresses(height, index, cw, state, {txUid});
}

string CBlockPriceMedianTx::ToString(CAccountDBCache &accountCache) {
    string pricePoints;
    for (const auto item : median_price_points) {
        pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}",
                                item.first.first, item.first.second, item.second);
    };

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, median_price_points=%s, nValidHeight=%d\n",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, txUid.ToString(), llFees, pricePoints,
                     nValidHeight);
}

Object CBlockPriceMedianTx::ToJson(const CAccountDBCache &accountCache) const {
    Array pricePointArray;
    for (const auto &item : median_price_points) {
        Object subItem;
        subItem.push_back(Pair("coin_symbol",     item.first.first));
        subItem.push_back(Pair("price_symbol",    item.first.second));
        subItem.push_back(Pair("price",           item.second));
        pricePointArray.push_back(subItem);
    }

    Object result;
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache);
    result.push_back(Pair("median_price_points",   pricePointArray));

    return result;
}

bool CBlockPriceMedianTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

map<CoinPricePair, uint64_t> CBlockPriceMedianTx::GetMedianPrice() const {
    return median_price_points;
}
